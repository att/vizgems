#pragma prototyped

#include <ast.h>
#include <option.h>
#include <swift.h>
#include <aggr.h>
#include <dds.h>

static const char usage[] =
"[-1p1s1D?\n@(#)$Id: ddscc (AT&T Labs Research) 2001-09-24 $\n]"
USAGE_LICENSE
"[+NAME?ddscc - compile expressions into shared objects]"
"[+DESCRIPTION?\bddscc\b pre-compiles expressions into shared objects to be"
" loaded by other DDS tools."
" For expressions that are to be used many times, pre-compiling them saves a"
" significant amount of time."
" As with all other DDS tools, expressions can be specified on the command"
" line or read from files."
" \bddscc\b needs to know the schema of the data that the shared object is to"
" be used with."
" This can be accomplished by providing a sample file as standard input, but"
" the recommended approach is to specify the schema with the \bis\b option"
" \aand\a assigning \b/dev/null\b to standard input."
" One of the following options must be specified to indicate the type of"
" shared object (SO) to build.]"
"[150:loader?builds a SO for importing ascii data."
" Relevant flags: \bos\b."
"]"
"[151:sorter?builds a SO for sorting data."
" Relevant flags: \bke\b."
"]"
"[152:filter?builds a SO for filtering data."
" Relevant flags: \bfe\b or \bff\b."
"]"
"[153:splitter?builds a SO for splitting a single DDS stream into multiple"
" streams of data."
" Relevant flags: \bse\b."
"]"
"[154:grouper?builds a SO for splitting a single DDS stream into multiple"
" streams of data."
" Relevant flags: \bge\b or \bgf\b."
"]"
"[155:extractor?builds a SO for extracting specific data fields."
" Relevant flags: \bee\b."
"]"
"[156:converter?builds a SO for converting data from one DDS schema to another."
" Relevant flags: \bos\b and either \bce\b or \bcf\b."
"]"
"[157:counter?builds a SO for counting up numbers of records."
" Relevant flags: \bke\b and \bce\b."
"]"
"[158:aggregator?builds a SO for generating AGGR datasets."
" Relevant flags: \bvt\b and either \bae\b or \af\b."
"]"
"[159:printer?builds a SO for printing data in ascii."
" Relevant flags: none."
"]"
"[200:so?specifies the name of the generated SO file."
" The default name is ddsobj.so."
"]:[file]"
"[100:is?specifies the schema of the input DDS file."
" This flag overrides any schema specified in the input file."
" Can be used to \aimport\a a fixed record data file that has no DDS header."
"]:[schemafile]"
"[101:os?specifies the schema of the output DDS file."
" This is required for building loader and converter SO files."
"]:[schemafile]"
"[210:ke?specifies the names of fields to be used as a key for sorting and"
" counting."
"]:['field1 ...']"
"[220:fe?specifies a filtering expression on the command line."
"]:['expression']"
"[221:ff?specifies a file containing a filtering expression."
"]:[file]"
"[230:se?specifies the names of fields to be used as a key for splitting."
"]:['field1 ...']"
"[240:ge?specifies a grouping expression on the command line."
"]:['expression']"
"[241:gf?specifies a file containing a grouping expression."
"]:[file]"
"[250:ee?specifies the names of fields to extract from a DDS file."
"]:['field1 ...']"
"[260:ce?specifies a converter or counter expression on the command line."
"]:['expression'|'field1 ...']"
"[261:cf?specifies a file containing a converter expression."
"]:[file]"
"[270:ae?specifies a aggregator expression on the command line."
"]:['expression']"
"[271:af?specifies a file containing a aggregator expression."
"]:[file]"
"[290:pe?specifies a print expression on the command line."
"]:['expression']"
"[291:pf?specifies a file containing the print expression."
"]:[file]"
"[303:vt?specifies the data type of the output AGGR data."
" The main type name may be following by a colon and a number, specifying how"
" many elements of that type make up the value, e.g. \bint:5\b would mean that"
" the value is a sequence of 5 integers."
" Such a value should appear in the input stream as 5 integers separated by"
" colons: 10:22:3:1:1."
" The default type is \bfloat\b."
"]:[(char|short|int|llong|float|double)[::n]]]"
"[400:ccflags?specifies extra flags for the compiler such as -I flags."
"]:[ccflags]"
"[401:ldflags?specifies extra flags for the compiler such as -L flags."
"]:[ldflags]"
"[402:fast?specifies to use a faster method if possible (e.g. for sorting)."
"]"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"\n"
"\n[ file ]\n"
"\n"
"[+SEE ALSO?\bdds\b(1), \bdds\b(3)]"
;

#define MODE_LOADER      1
#define MODE_SORTER      2
#define MODE_FILTER      3
#define MODE_SPLITTER    4
#define MODE_GROUPER     5
#define MODE_EXTRACTOR   6
#define MODE_CONVERTER   7
#define MODE_COUNTER     8
#define MODE_AGGREGATOR  9
#define MODE_PRINTER    10

int main (int argc, char **argv) {
    int norun;
    int mode;
    char *sofile;
    char *schemaname, *oschemaname;
    DDSschema_t *schemap, *oschemap;
    int fieldi;
    DDSheader_t hdr;
    char buf[10000];
    Sfio_t *ifp;
    int ret;
    char *kexpr;
    char *fexpr, *ffile;
    char *sexpr;
    char *gexpr, *gfile;
    char *eexpr;
    char *cexpr, *cfile;
    char *aexpr, *afile;
    char *pexpr, *pfile;
    int valtype, vallen;
    int fastflag;
    char *ccflags, *ldflags;
    DDSexpr_t fe;
    DDSexpr_t ge;
    DDSexpr_t ce;
    DDSexpr_t ae;
    DDSexpr_t pe;

    if (AGGRinit () == -1)
        SUerror ("ddscc", "AGGRinit failed");
    DDSinit ();
    mode = 0;
    sofile = "ddsobj.so";
    schemaname = oschemaname = NULL;
    schemap = NULL, oschemap = NULL;
    kexpr = NULL;
    fexpr = NULL, ffile = NULL;
    sexpr = NULL;
    gexpr = NULL, gfile = NULL;
    eexpr = NULL;
    cexpr = NULL, cfile = NULL;
    aexpr = NULL, afile = NULL;
    pexpr = NULL, pfile = NULL;
    valtype = AGGR_TYPE_INT, vallen = AGGRtypelens[valtype];
    fastflag = FALSE;
    ccflags = ldflags = "";
    norun = 0;
    for (;;) {
        switch (optget (argv, usage)) {
        case -150:
            mode = MODE_LOADER;
            continue;
        case -151:
            mode = MODE_SORTER;
            continue;
        case -152:
            mode = MODE_FILTER;
            continue;
        case -153:
            mode = MODE_SPLITTER;
            continue;
        case -154:
            mode = MODE_GROUPER;
            continue;
        case -155:
            mode = MODE_EXTRACTOR;
            continue;
        case -156:
            mode = MODE_CONVERTER;
            continue;
        case -157:
            mode = MODE_COUNTER;
            continue;
        case -158:
            mode = MODE_AGGREGATOR;
            continue;
        case -159:
            mode = MODE_PRINTER;
            continue;
        case -200:
            sofile = opt_info.arg;
            continue;
        case -100:
            schemaname = opt_info.arg;
            continue;
        case -101:
            oschemaname = opt_info.arg;
            continue;
        case -210:
            kexpr = opt_info.arg;
            continue;
        case -220:
            fexpr = opt_info.arg;
            continue;
        case -221:
            ffile = opt_info.arg;
            continue;
        case -230:
            sexpr = opt_info.arg;
            continue;
        case -240:
            gexpr = opt_info.arg;
            continue;
        case -241:
            gfile = opt_info.arg;
            continue;
        case -250:
            eexpr = opt_info.arg;
            continue;
        case -260:
            cexpr = opt_info.arg;
            continue;
        case -261:
            cfile = opt_info.arg;
            continue;
        case -270:
            aexpr = opt_info.arg;
            continue;
        case -271:
            afile = opt_info.arg;
            continue;
        case -290:
            pexpr = opt_info.arg;
            continue;
        case -291:
            pfile = opt_info.arg;
            continue;
        case -303:
            if (AGGRparsetype (opt_info.arg, &valtype, &vallen) == -1)
                SUerror ("ddscc", "bad argument for -vt option");
            continue;
        case -400:
            ccflags = opt_info.arg;
            continue;
        case -401:
            ldflags = opt_info.arg;
            continue;
        case -402:
            fastflag = TRUE;
            continue;
        case -999:
            SUwarnlevel++;
            continue;
        case '?':
            SUusage (0, "ddscc", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "ddscc", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;
    if (mode == MODE_LOADER)
        schemaname = oschemaname;
    if (mode == 0 || argc > 1) {
        SUusage (0, "ddscc", opt_info.arg);
        return 1;
    }
    ifp = sfstdin;
    if (argc == 1)
        if (!(ifp = sfopen (NULL, argv[0], "r")))
            SUerror ("ddscc", "sfopen failed for %s", argv[0]);
    if ((ret = DDSreadheader (ifp, &hdr)) == -1)
        SUerror ("ddscc", "DDSreadheader failed");
    if (ret == 0) {
        if (hdr.schemap)
            schemap = hdr.schemap;
        if (hdr.schemaname)
            schemaname = hdr.schemaname;
    }
    if (!schemap) {
        if (!schemaname)
            SUerror ("ddscc", "no schema specified");
        if (!(schemap = DDSloadschema (schemaname, NULL, NULL)))
            SUerror ("ddscc", "DDSloadschema failed");
    }
    if (mode == MODE_LOADER) {
        if (!oschemaname)
            SUerror ("ddscc", "no output schema specified");
        if (!(oschemap = DDSloadschema (oschemaname, NULL, NULL)))
            SUerror ("ddscc", "DDSloadschema failed for oschemap");
        if (!DDScompileloader (oschemap, sofile, ccflags, ldflags))
            SUerror ("ddscc", "DDScompileloader failed");
    }
    if (mode == MODE_SORTER) {
        if (!kexpr) {
            for (fieldi = 0; fieldi < schemap->fieldn; fieldi++) {
                if (fieldi > 0)
                    strcat (buf, " ");
                strcat (buf, schemap->fields[fieldi].name);
            }
            kexpr = buf;
        }
        if (!DDScompilesorter (
            schemap, kexpr, sofile, ccflags, ldflags, fastflag
        ))
            SUerror ("ddscc", "DDScompilesorter failed");
    }
    if (mode == MODE_FILTER) {
        if (!fexpr && !ffile)
            fexpr = "{ KEEP; }";
        if (DDSparseexpr (fexpr, ffile, &fe) == -1)
            SUerror ("ddscc", "DDSparseexpr failed");
        if (!DDScompilefilter (schemap, &fe, sofile, ccflags, ldflags))
            SUerror ("ddscc", "DDScompilefilter failed");
    }
    if (mode == MODE_SPLITTER) {
        if (!sexpr)
            SUerror ("ddscc", "missing expression");
        if (!DDScompilesplitter (schemap, sexpr, sofile, ccflags, ldflags))
            SUerror ("ddscc", "DDScompilesplitter failed");
    }
    if (mode == MODE_GROUPER) {
        if (!gexpr && !gfile)
            SUerror ("ddscc", "missing expression");
        if (DDSparseexpr (gexpr, gfile, &ge) == -1)
            SUerror ("ddscc", "DDSparseexpr failed");
        if (!DDScompilegrouper (schemap, &ge, sofile, ccflags, ldflags))
            SUerror ("ddscc", "DDScompilegrouper failed");
    }
    if (mode == MODE_EXTRACTOR) {
        if (!eexpr)
            SUerror ("ddscc", "missing expression");
        if (!DDScompileextractor (
            schemap, NULL, eexpr, sofile, ccflags, ldflags
        ))
            SUerror ("ddscc", "DDScompileextractor failed");
    }
    if (mode == MODE_CONVERTER) {
        if (!cexpr && !cfile)
            SUerror ("ddscc", "missing expression");
        if (!oschemaname)
            SUerror ("ddscc", "no output schema specified");
        if (!(oschemap = DDSloadschema (oschemaname, NULL, NULL)))
            SUerror ("ddscc", "DDSloadschema failed for oschemap");
        if (DDSparseexpr (cexpr, cfile, &ce) == -1)
            SUerror ("ddscc", "DDSparseexpr failed");
        if (!DDScompileconverter (
            schemap, oschemap, &ce, sofile, ccflags, ldflags
        ))
            SUerror ("ddscc", "DDScompileconverter failed");
    }
    if (mode == MODE_COUNTER) {
        if (!kexpr || strcmp (kexpr, "all") == 0) {
            for (fieldi = 0; fieldi < schemap->fieldn; fieldi++) {
                if (cexpr && strcmp (cexpr, schemap->fields[fieldi].name) == 0)
                    continue;
                if (fieldi > 0)
                    strcat (buf, " ");
                strcat (buf, schemap->fields[fieldi].name);
            }
            kexpr = buf;
        }
        if (!DDScompilecounter (
            schemap, kexpr, cexpr, sofile, ccflags, ldflags
        ))
            SUerror ("ddscc", "DDScompilecounter failed");
    }
    if (mode == MODE_AGGREGATOR) {
        if (!aexpr && !afile)
            aexpr = "{ KEEP; }";
        if (DDSparseexpr (aexpr, afile, &ae) == -1)
            SUerror ("ddscc", "DDSparseexpr failed");
        if (!DDScompileaggregator (
            schemap, &ae, valtype, sofile, ccflags, ldflags
        ))
            SUerror ("ddscc", "DDScompileaggregator failed");
    }
    if (mode == MODE_PRINTER) {
        if (!pexpr && !pfile)
            pexpr = "{ KEEP; }";
        if (DDSparseexpr (pexpr, pfile, &pe) == -1)
            SUerror ("ddscc", "DDSparseexpr failed");
        if (!DDScompileprinter (schemap, &pe, sofile, ccflags, ldflags))
            SUerror ("ddscc", "DDScompileprinter failed");
    }
    DDSterm ();
    return 0;
}
