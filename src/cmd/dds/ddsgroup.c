#pragma prototyped

#include <ast.h>
#include <option.h>
#include <sys/resource.h>
#include <swift.h>
#include <dds.h>

static const char usage[] =
"[-1p1s1D?\n@(#)$Id: ddsgroup (AT&T Labs Research) 2001-09-24 $\n]"
USAGE_LICENSE
"[+NAME?ddsgroup - split input stream into multiple files]"
"[+DESCRIPTION?\bddsgroup\b reads records from a DDS data file (or standard"
" input) writes them out on separate files, one for each unique value of the"
" specified expression."
" This expression can be specified on the command line, loaded in from a file,"
" or loaded in in a precompiled shared object."
" The C-style expression has 4, optional, sections: \bVARS\b, \bBEGIN\b,"
" \bLOOP\b, \bEND\b."
" Each of these keywords must be followed by C-style statements enclosed in"
" \b{}\b."
" \bVARS\b is used to define global variables."
" \bBEGIN\b and \bEND\b is code that is executed once at the beginning and end"
" of the run."
" The \bLOOP\b section is called once per record."
" The \bLOOP\b keyword is optional."
" In the \bLOOP\b section, the fields of the current record can be referenced"
" by using their names."
" Any changes to the values of these fields are reflected in the output."
" On every invocation, the LOOP expression must assign a non-negative value to"
" the special variable \bGROUP\b."
" This will be used as the group id of the record and the record will be"
" written to the file corresponding to this id."
" See the information about the \bp\b option below.]"
"[100:is?specifies the schema of the input DDS file."
" This flag overrides any schema specified in the input file."
" Can be used to \aimport\a a fixed record data file that has no DDS header."
"]:[schemafile]"
"[102:osm?specifies that the output file should contain the complete schema,"
" just the name of the schema, or no schema at all."
" When a data file contains just the schema name, DDS searches for a file with"
" that name in the user's path (pointed to by the PATH environment variable)."
" For every directory in the path, such as /home/ek/bin, the schema is searched"
" for in the corresponding lib/swift directory, such as /home/ek/lib/swift."
" The last option, none, can be used to \aexport\a a data file."
" The default mode is \bfull\b."
"]:[(full|name|none):oneof]{[+full][+name][+none]}"
"[104:ozm?specifies the compression mode for the output file."
" The mode can be a vcodex spec, or \bnone\b for no compression,"
" or \brdb/rtb\b for the vcodex \brdb/rtable\b codex,"
" using the DDS schema field sizes."
" The default mode is \bnone\b."
"]:['vcodex spec|none|rdb|rtb']"
"[240:ge?specifies a grouping expression on the command line."
"]:['expression']"
"[241:gf?specifies a file containing a grouping expression."
"]:[file]"
"[242:gso?specifies the file containing the precompiled shared object for"
" grouping."
"]:[file]"
"[300:p?specifies either the prefix or the name format for the output files."
" If string includes printf-style format directives, it is used as the name"
" format."
" In this case it must include one directive for the group id."
" Otherwise the string is assumed to be the prefix, in which case \bddsgroup\b"
" will append a dot and the value for the group id."
" The default prefix is \bddsgroup\b."
"]:[(prefix|format)]"
"[308:append?specifies that output files are not to be erased before using."
" Any new data will be appended to existing data."
" This allows \bddsgroup\b to work in an incremental mode."
" The default action is to erase the files."
"]"
"[400:ccflags?specifies extra flags for the compiler such as -I flags."
"]:[ccflags]"
"[401:ldflags?specifies extra flags for the compiler such as -L flags."
"]:[ldflags]"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"\n"
"\n[ file ]\n"
"\n"
"[+EXAMPLES?Assuming the following 4 records:]{"
"[+]"
"[+f1 f2 f3\b]"
"[+--------\b]"
"[+10 11  5\b]"
"[+10 11 15\b]"
"[+10 21 25\b]"
"[+20 31 35\b]"
"[+]"
"[+?\bddsgroup -ge '{ GROUP = f1 / 10; }' -p out\b]"
"}"
"[+?will create 2 files, out.1 (first 3 records) and out.2 (last record).]"
"{"
"[+]"
"[+?\bddsgroup -ge '{ GROUP = (f2 - f1); }' -p out\b]"
"}"
"[+?will create 2 files, out.1 (first 2 records), and out.11 (last 2 records).]"
"[+SEE ALSO?\bdds\b(1), \bdds\b(3), \baggr\b(1)]"
;

static int contmode;
static DDSheader_t hdr;
static int osm;
static char *ozm;

void *channelopen (char *);
int channelclose (void *);
void *channelreopen (char *);

int main (int argc, char **argv) {
    int norun;
    char *schemaname;
    char *gexpr, *gfile, *gsofile;
    char *prefix;
    char *ccflags, *ldflags;
    DDSexpr_t ge;
    DDSschema_t *schemap;
    DDSgrouper_t *grouperp;
    struct rlimit rl;
    Sfio_t *ifp, *ofp;
    int ret;

    schemaname = NULL;
    osm = DDS_HDR_NAME | DDS_HDR_DESC;
    ozm = "none";
    contmode = 0;
    gexpr = NULL, gfile = NULL, gsofile = NULL;
    prefix = "ddsgroup";
    schemap = NULL;
    ccflags = ldflags = "";
    norun = 0;
    for (;;) {
        switch (optget (argv, usage)) {
        case -100:
            schemaname = opt_info.arg;
            continue;
        case -102:
            if (strcmp (opt_info.arg, "none") == 0)
                osm = 0;
            else if (strcmp (opt_info.arg, "name") == 0)
                osm = DDS_HDR_NAME;
            else if (strcmp (opt_info.arg, "full") == 0)
                osm = DDS_HDR_NAME | DDS_HDR_DESC;
            continue;
        case -104:
            ozm = opt_info.arg;
            continue;
        case -240:
            gexpr = opt_info.arg;
            continue;
        case -241:
            gfile = opt_info.arg;
            continue;
        case -242:
            gsofile = opt_info.arg;
            continue;
        case -300:
            prefix = opt_info.arg;
            continue;
        case -308:
            contmode = 1;
            continue;
        case -400:
            ccflags = opt_info.arg;
            continue;
        case -401:
            ldflags = opt_info.arg;
            continue;
        case -999:
            SUwarnlevel++;
            continue;
        case '?':
            SUusage (0, "ddsgroup", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "ddsgroup", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;
    if (!gexpr && !gfile && !gsofile) {
        SUusage (1, "ddsgroup", opt_info.arg);
        return 1;
    }
    if (argc > 1)
        SUerror ("ddsgroup", "too many parameters");
    if (getrlimit (RLIMIT_NOFILE, &rl) != -1) {
        rl.rlim_cur = rl.rlim_max;
        setrlimit (RLIMIT_NOFILE, &rl);
    }
    DDSinit ();
    ifp = sfstdin;
    if (argc == 1 && !(ifp = sfopen (NULL, argv[0], "r")))
        SUerror ("ddsgroup", "sfopen failed for %s", argv[0]);
    if (!(ifp = SUsetupinput (ifp, 1048576)))
        SUerror ("ddsgroup", "setupinput failed");
    if (!gsofile && DDSparseexpr (gexpr, gfile, &ge) == -1)
        SUerror ("ddsgroup", "DDSparseexpr failed");
    if ((ret = DDSreadheader (ifp, &hdr)) == -1)
        SUerror ("ddsgroup", "DDSreadheader failed");
    if (ret == -2)
        goto done;
    if (ret == 0) {
        if (hdr.schemap)
            schemap = hdr.schemap;
        else if (hdr.schemaname)
            schemaname = hdr.schemaname;
    }
    if (!schemap) {
        if (!schemaname)
            SUerror ("ddsgroup", "no schema specified");
        if (!(schemap = DDSloadschema (schemaname, NULL, NULL)))
            SUerror ("ddsgroup", "DDSloadschema failed");
    }
    if (!hdr.schemaname && schemaname)
        hdr.schemaname = schemaname;
    if (!hdr.schemap && schemap)
        hdr.schemap = schemap;
    hdr.contents = osm;
    hdr.vczspec = ozm;
    if (!gsofile) {
        if (!(grouperp = DDScreategrouper (schemap, &ge, ccflags, ldflags)))
            SUerror ("ddsgroup", "DDScreategrouper failed");
    } else {
        if (!(grouperp = DDSloadgrouper (schemap, gsofile)))
            SUerror ("ddsgroup", "DDSloadgrouper failed");
    }
    if ((*grouperp->init) (prefix, channelopen, channelreopen, channelclose))
        SUerror ("ddsgroup", "grouperp->init failed");
    while ((ret = DDSreadrecord (
        ifp, schemap->recbuf, schemap
    )) == schemap->recsize) {
        if ((ofp = (*grouperp->work) (
            schemap->recbuf, schemap->recsize
        ))) {
            if (ofp == sfstdin)
                continue;
            if (DDSwriterecord (
                ofp, schemap->recbuf, schemap
            ) != schemap->recsize)
                SUerror ("ddsgroup", "incomplete write");
        } else
            SUerror ("ddsgroup", "grouperp->work failed");
    }
    if (ret != 0)
        SUerror ("ddsgroup", "incomplete read");
    if ((*grouperp->term) ())
        SUerror ("ddsgroup", "grouperp->term failed");
    if (sfsync (NULL) < 0)
        SUerror ("ddsgroup", "sync failed");
    DDSdestroygrouper (grouperp);

done:
    DDSterm ();
    return 0;
}

void *channelopen (char *name) {
    Sfio_t *fp;

    if (contmode) {
        if (!(fp = sfopen (NULL, name, "a")))
            SUerror ("ddsgroup", "open failed for %s", name);
        if (!(fp = SUsetupoutput (NULL, fp, 1048576)))
            SUerror ("ddsgroup", "setupoutput failed");
        if (sfsize (fp) == 0 && osm && DDSwriteheader (fp, &hdr) == -1)
            SUerror ("ddsgroup", "DDSwriteheader failed");
    } else {
        if (!(fp = sfopen (NULL, name, "w")))
            SUerror ("ddsgroup", "open failed for %s", name);
        if (!(fp = SUsetupoutput (NULL, fp, 1048576)))
            SUerror ("ddsgroup", "setupoutput failed");
        if (osm && DDSwriteheader (fp, &hdr) == -1)
            SUerror ("ddsgroup", "DDSwriteheader failed");
    }
    return (void *) fp;
}

int channelclose (void *fp) {
    return sfclose ((Sfio_t *) fp) ? 0 : -1;
}

void *channelreopen (char *name) {
    Sfio_t *fp;

    if (!(fp = sfopen (NULL, name, "a")))
        SUerror ("ddsgroup", "open failed for %s", name);
    if (!(fp = SUsetupoutput (NULL, fp, 1048576)))
        SUerror ("ddsgroup", "setupoutput failed");
    return (void *) fp;
}
