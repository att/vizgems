#pragma prototyped

#include <ast.h>
#include <option.h>
#include <regex.h>
#include <dirent.h>
#include <swift.h>
#include <dds.h>

static const char usage[] =
"[-1p1s1D?\n@(#)$Id: ddsinfo (AT&T Labs Research) 2001-09-24 $\n]"
USAGE_LICENSE
"[+NAME?ddsinfo - print information about DDS files]"
"[+DESCRIPTION?\bddsinfo\b prints information about the specified DDS files."
" The default information is the schema name and the number of records,"
" but other data may also be printed."
" A regular expression can be specified instead of the list of files."
"]"
"[100:is?specifies the schema of the input DDS file."
" This flag overrides any schema specified in the input file."
" Can be used to \aimport\a a fixed record data file that has no DDS header."
"]:[schemafile]"
"[320:fp?specifies that \ain addition\a to any files specified on the command"
" line, \bddsinfo\b will scan for files with names matching this shell-style"
" regular expression."
"]:['regular expression']"
"[326:sum?specifies the sum of all the records in all the files should also be"
" printed."
"]"
"[327:o2i?specifies that the index of the record at offset \boffset\b should"
" be printed in addition to any other info."
"]:[offset]"
"[328:V?specifies that the full schema should be printed, instead of just the"
" schema name."
"]"
"[329:q?specifies that only the fields that can be retrieved quickly should be"
" extracted."
"]"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"\n"
"\n[ file1 ... ]\n"
"\n"
"[+EXAMPLES?Get information on a file:]{"
"[+]"
"[+ddsinfo a.dds\b]"
"}"
"[+?Get information on all files that match a pattern and also print the sum"
" of records:]{"
"[+]"
"[+ddsinfo -sum -fp '[A-Z]][A-Z]]*'\b]"
"}"
"[+SEE ALSO?\bdds\b(1), \bdds\b(3), \baggr\b(1)]"
;

#ifdef FEATURE_NO64FS
typedef off_t off64_t;
#endif

static char *schemaname;
static DDSschema_t *schemap;
static regex_t prog;
static int sumflag, o2iflag, qflag;
static long long sumrecs;
static off64_t o2idata;
static int verbose;

static char *fieldtypestring[] = {
    NULL,
    /* DDS_FIELD_TYPE_CHAR */     "char",
    /* DDS_FIELD_TYPE_SHORT */    "short",
    /* DDS_FIELD_TYPE_INT */      "int",
    /* DDS_FIELD_TYPE_LONG */     "long",
    /* DDS_FIELD_TYPE_LONGLONG */ "long long",
    /* DDS_FIELD_TYPE_FLOAT */    "float",
    /* DDS_FIELD_TYPE_DOUBLE */   "double"
};

static int scan (char *);
static int printinfo (char *);

int main (int argc, char **argv) {
    int norun;
    int argi;
    char *fpat;

    schemaname = NULL;
    schemap = NULL;
    fpat = NULL;
    sumflag = 0, o2iflag = 0, qflag = 0;
    sumrecs = 0;
    verbose = 0;
    norun = 0;
    for (;;) {
        switch (optget (argv, usage)) {
        case -100:
            schemaname = opt_info.arg;
            continue;
        case -320:
            fpat = opt_info.arg;
            continue;
        case -326:
            sumflag = 1;
            continue;
        case -327:
            o2iflag = 1;
            o2idata = strtol (opt_info.arg, NULL, 0);
            continue;
        case -328:
            verbose++;
            continue;
        case -329:
            qflag = 1;
            continue;
        case -999:
            SUwarnlevel++;
            continue;
        case '?':
            SUusage (0, "ddsinfo", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "ddsinfo", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;
    DDSinit ();
    if (schemaname)
        if (!(schemap = DDSloadschema (schemaname, NULL, NULL)))
            SUerror ("ddsinfo", "DDSloadschema failed");
    for (argi = 0; argi < argc; argi++)
        if (printinfo (argv[argi]) == -1)
            SUerror ("ddsinfo", "printinfo failed for %s", argv[argi]);
    if (fpat) {
        if (regcomp (
            &prog, fpat,
            REG_NULL | REG_LENIENT | REG_AUGMENTED |
            REG_NOSUB | REG_SHELL | REG_LEFT | REG_RIGHT
        ) != 0)
            SUerror ("ddsinfo", "regcomp failed");
        if (scan (".") == -1)
            SUerror ("ddsinfo", "scan failed");
    }
    if (argc == 0 && !fpat)
        if (printinfo (NULL) == -1)
            SUerror ("ddsinfo", "printinfo failed for stdin");
    if (sumflag)
        sfprintf (sfstdout, "total: %lld\n", sumrecs);
    DDSterm ();
    return 0;
}

static int scan (char *dir) {
    DIR *dp;
    struct dirent *dep;
    int rtn;

    if (!(dp = opendir (dir))) {
        SUwarning (0, "ddsinfo", "opendir failed");
        return -1;
    }
    rtn = 0;
    while ((dep = readdir (dp))) {
        if (regexec (
            &prog, dep->d_name, 0, NULL,
            REG_NULL | REG_LENIENT | REG_AUGMENTED | REG_NOSUB |
            REG_SHELL | REG_LEFT | REG_RIGHT
        ) != 0)
            continue;
        if (dep->d_name[0] == '.' && (dep->d_name[1] == 0 || (
            dep->d_name[1] == '.' && dep->d_name[2] == 0
        )))
            continue;
        if (printinfo (sfprints ("%s/%s", dir, dep->d_name)) == -1) {
            SUwarning (1, "scan", "printinfo failed for %s", dep->d_name);
            return -1;
        }
    }
    closedir (dp);
    return rtn;
}

static int printinfo (char *name) {
    Sfio_t *fp;
    int ret;
    DDSheader_t ihdr;
    DDSschema_t *sp;
    DDSfield_t *fieldp;
    int fieldi;
    off64_t off1, off2;
    long long nrecs;

    sfprintf (sfstdout, "file: %s\n", (name) ? name : "<stdin>");
    if (name) {
        if (!(fp = sfopen (NULL, name, "r"))) {
            SUwarning (1, "printinfo", "open failed for %s", name);
            return -1;
        }
    } else
        fp = sfstdin;
    if ((ret = DDSreadheader (fp, &ihdr)) == -1) {
        SUwarning (1, "printinfo", "DDSreadheader failed for %s", name);
        return -1;
    }
    if (ret == 0 && ihdr.schemaname)
        sfprintf (sfstdout, "schema: %s\n", ihdr.schemaname);
    else if (schemaname)
        sfprintf (sfstdout, "schema: %s\n", schemaname);
    else {
        SUwarning (1, "printinfo", "no schemaname for file %s", name);
        return -1;
    }
    if (ret == 0 && ihdr.schemap)
        sp = ihdr.schemap;
    else if (schemap)
        sp = schemap;
    else {
        SUwarning (1, "printinfo", "no schema data for file %s", name);
        return -1;
    }
    if (verbose > 0) {
        for (fieldi = 0; fieldi < sp->fieldn; fieldi++) {
            fieldp = &sp->fields[fieldi];
            if (fieldp->isunsigned)
                sfprintf (
                    sfstdout, "field: %s %s unsigned %d\n",
                    fieldp->name, fieldtypestring[fieldp->type], fieldp->n
                );
            else
                sfprintf (
                    sfstdout, "field: %s %s %d\n",
                    fieldp->name, fieldtypestring[fieldp->type], fieldp->n
                );
        }
    }
    if (!qflag) {
        if (name) {
            if (
                (off1 = sfseek (fp, 0, SEEK_CUR)) == -1 ||
                (off2 = sfseek (fp, 0, SEEK_END)) == -1
            ) {
                SUwarning (1, "printinfo", "cannot find size of %s", name);
                return -1;
            }
        } else {
            if (
                (off1 = sftell (fp)) < 0 ||
                sfmove (fp, NULL, -1, -1) == -1 ||
                (off2 = sftell (fp)) < 0
            ) {
                SUwarning (1, "printinfo", "cannot find size of input");
                return -1;
            }
        }
        nrecs = (off2 - off1) / sp->recsize;
        sumrecs += nrecs;
        if (o2iflag) {
            sfprintf (
                sfstdout, "offset-to-index: %lld %d\n",
                o2idata, (o2idata - off1) / sp->recsize
            );
        }
        sfprintf (sfstdout, "offset: %lld\n", off1);
        sfprintf (sfstdout, "length: %lld\n", off2);
        if (nrecs * sp->recsize != off2 - off1)
            sfprintf (
                sfstdout, "ERROR: incomplete file %s, nrecs: %f\n",
                name, (double) (off2 - off1) / sp->recsize
            );
        else
            sfprintf (sfstdout, "nrecs: %lld\n", nrecs);
    }
    sfprintf (sfstdout, "recsize: %d\n", sp->recsize);
    if (ret == 0 && ihdr.vczspec[0])
        sfprintf (sfstdout, "compression: %s\n", ihdr.vczspec);
    else
        sfprintf (sfstdout, "compression: none\n");
    if (ihdr.schemap)
        DDSdestroyschema (ihdr.schemap);
    if (name)
        sfclose (fp);
    return 0;
}
