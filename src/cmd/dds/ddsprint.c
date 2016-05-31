#pragma prototyped

#include <ast.h>
#include <option.h>
#include <swift.h>
#include <dds.h>

static const char usage[] =
"[-1p1s1D?\n@(#)$Id: ddsprint (AT&T Labs Research) 2001-09-24 $\n]"
USAGE_LICENSE
"[+NAME?ddsprint - print a DDS file]"
"[+DESCRIPTION?\bddsprint\b reads records from a DDS data file (or standard"
" input) and outputs them in ascii format.]"
" input) and uses the specified expression to print the record to standard"
" output."
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
" The \bLOOP\b code can print the current record by using the \bsfprintf\b"
" function of the SFIO library."
"]"
"[100:is?specifies the schema of the input DDS file."
" This flag overrides any schema specified in the input file."
" Can be used to \aimport\a a fixed record data file that has no DDS header."
"]:[schemafile]"
"[290:pe?specifies the print expression on the command line."
"]:['expression']"
"[291:pf?specifies the file containing the print expression."
"]:[file]"
"[292:pso?specifies the file containing the precompiled shared object for"
" printing."
"]:[file]"
"[400:ccflags?specifies extra flags for the compiler such as -I flags."
"]:[ccflags]"
"[401:ldflags?specifies extra flags for the compiler such as -L flags."
"]:[ldflags]"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"\n"
"\n[ file ]\n"
"\n"
"[+EXAMPLES?Print a file to standard output:]{"
"[+]"
"[+ddsprint a.dds\b]"
"}"
"[+SEE ALSO?\bdds\b(1), \bdds\b(3), \baggr\b(1)]"
;

static DDSheader_t hdr;

int main (int argc, char **argv) {
    int norun;
    char *schemaname;
    char *pexpr, *pfile, *psofile;
    char *ccflags, *ldflags;
    DDSexpr_t pe;
    DDSschema_t *schemap;
    DDSprinter_t *printerp;
    Sfio_t *ifp;
    int ret;

    schemaname = NULL;
    schemap = NULL;
    pexpr = NULL, pfile = NULL, psofile = NULL;
    ccflags = ldflags = "";
    norun = 0;
    for (;;) {
        switch (optget (argv, usage)) {
        case -100:
            schemaname = opt_info.arg;
            continue;
        case -290:
            pexpr = opt_info.arg;
            continue;
        case -291:
            pfile = opt_info.arg;
            continue;
        case -292:
            psofile = opt_info.arg;
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
            SUusage (0, "ddsprint", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "ddsprint", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;
    if (argc > 1)
        SUerror ("ddsprint", "too many parameters");
    DDSinit ();
    ifp = sfstdin;
    if (argc == 1 && !(ifp = sfopen (NULL, argv[0], "r")))
        SUerror ("ddsprint", "sfopen failed for %s", argv[0]);
    if (!(ifp = SUsetupinput (ifp, 1048576)))
        SUerror ("ddsprint", "setupinput failed");
    if (!pexpr && !pfile && !psofile)
        pexpr = strdup ("");
    if (!psofile && DDSparseexpr (pexpr, pfile, &pe) == -1)
        SUerror ("ddsprint", "DDSparseexpr failed");
    if ((ret = DDSreadheader (ifp, &hdr)) == -1)
        SUerror ("ddsprint", "DDSreadheader failed");
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
            SUerror ("ddsprint", "no schema specified");
        if (!(schemap = DDSloadschema (schemaname, NULL, NULL)))
            SUerror ("ddsprint", "DDSloadschema failed");
    }
    if (!psofile) {
        if (!(printerp = DDScreateprinter (schemap, &pe, ccflags, ldflags)))
            SUerror ("ddsprint", "DDScreateprinter failed");
    } else {
        if (!(printerp = DDSloadprinter (schemap, psofile)))
            SUerror ("ddsprint", "DDSloadprinter failed");
    }
    if ((*printerp->init) ())
        SUerror ("ddsprint", "printerp->init failed");
    while ((ret = DDSreadrecord (
        ifp, schemap->recbuf, schemap
    )) == schemap->recsize) {
        (*printerp->work) (schemap->recbuf, schemap->recsize, sfstdout);
    }
    if (ret != 0)
        SUerror ("ddsprint", "incomplete read");
    if ((*printerp->term) ())
        SUerror ("ddsprint", "printerp->term failed");
    if (sfsync (NULL) < 0)
        SUerror ("ddsprint", "sync failed");
    DDSdestroyprinter (printerp);

done:
    DDSterm ();
    return 0;
}
