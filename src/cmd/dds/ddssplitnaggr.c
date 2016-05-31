#pragma prototyped

#include <ast.h>
#include <option.h>
#include <sys/resource.h>
#include <swift.h>
#include <aggr.h>
#include <dds.h>

static const char usage[] =
"[-1p1s1D?\n@(#)$Id: ddssplitnaggr (AT&T Labs Research) 2001-09-24 $\n]"
USAGE_LICENSE
"[+NAME?ddssplitnaggr - compute AGGR sums by category]"
"[+DESCRIPTION?\bddssplitnaggr\b reads records from a DDS data file (or"
" standard input) and generates one AGGR output file, per unique combination"
" of the supplied fields, containing counts based on the supplied expression."
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
" On every invocation, the LOOP expression must call one of two macros:]{"
"[+]"
"[+KEYFRAMEVALUE(k, f, v)\b]"
"[+ITEMFRAMEVALUE(i, f, v)\b]"
"}"
"[+?\bf\b specifies the frame number, i.e. one of the two dimensions."
" The other dimension is specified by either the \bi\b or the \bk\b field."
" \bi\b is a number between 0 and the maximum number of items in the specific"
" aggregation type."
" \bk\b is a pointer to a key."
" The \bLOOP\b expression can call one of these macros multiple times per"
" invocation."
" Each call to these macros specifies that the value \bv\b is to be added to"
" the item at location \bi\b or at the location specified using the key \bk\b"
" on frame \bf\b."
" This tool is a combination of the \bddssplit\b and \bddsaggr\b tools.]"
"[100:is?specifies the schema of the input DDS file."
" This flag overrides any schema specified in the input file."
" Can be used to \aimport\a a fixed record data file that has no DDS header."
"]:[schemafile]"
"[230:se?specifies the names of fields to be used as a key for splitting."
"]:['field1 ...']"
"[232:sso?specifies the file containing the precompiled shared object for"
" splitting."
"]:[file]"
"[270:ae?specifies a aggregator expression on the command line."
"]:['expression']"
"[271:af?specifies the file containing the aggregator expression."
"]:[file]"
"[272:aso?specifies the file containing the precompiled shared object for"
" aggregation."
"]:[file]"
"[300:p?specifies either the prefix or the name format for the output files."
" If string includes printf-style format directives, it is used as the name"
" format."
" In this case it must include as many directives as there are fields in the"
" list, and in the same order."
" Otherwise the string is assumed to be the prefix, in which case"
" \bddssplitnaggr\b will append a dot and the value for each field."
" The default prefix is \bdds.aggr\b."
"]:[(prefix|format)]"
"[301:cl?specifies a string describing the output datasets."
" Operations that combine multiple AGGR files will only work across files with"
" the same description."
" The default class is \bddssplitnaggr\b."
"]:[class]"
"[302:kt?specifies the data type of the key field."
" The main type name may be following by a colon and a number, specifying how"
" many elements of that type make up the key, e.g. \bint:5\b would mean that"
" the key is a sequence of 5 integers."
" Such a key should appear in the input stream as 5 integers separated by"
" colons: 10:22:3:1:1."
" The default type is \bstring\b."
"]:[(char|short|int|llong|float|double|string)[::n]]]"
"[303:vt?specifies the data type of the value fields."
" The main type name may be following by a colon and a number, specifying how"
" many elements of that type make up the value, e.g. \bint:5\b would mean that"
" the value is a sequence of 5 integers."
" Such a value should appear in the input stream as 5 integers separated by"
" colons: 10:22:3:1:1."
" The default type is \bfloat\b."
"]:[(char|short|int|llong|float|double)[::n]]]"
"[304:fr?specifies the number of frames (the second dimension in AGGR files)"
" to start with."
"]#[n]"
"[305:di?specifies the increment step for adding new keys to the dictionary."
" The default value is \b1\b."
" Higher values may improve performance when loading large data files since"
" they result in less frequent rearranging of the output files."
"]#[n]"
"[306:ii?specifies the increment step for adding new items to the file."
" The default value is \b1\b."
" Higher values may improve performance when loading large data files since"
" they result in less frequent rearranging of the output files."
"]#[n]"
"[307:ql?specifies the length of the queue for adding new items to the file."
" The default value is \b1,000,000\b."
" Higher values may improve performance when loading large data files since"
" they result in less frequent rearranging of the output files."
"]#[n]"
"[308:append?specifies that output AGGR files are not to be cleared before"
" using."
" Any new data will be appended to existing values."
" This allows \bddssplitnaggr\b to work in an incremental mode."
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
"[+EXAMPLES?For a sequence of records representing phonecalls, count up how"
" many calls start up per 10 min. interval, generating a separate file for"
" each type of service, such as 1-800 calls:]{"
"[+]"
"[+ddssplitnaggr -ae '{\b]"
"[+    static int spi = 10 * 60;\b]"
"[+    int framei;\b]"
"[+    if ((framei = time / spi) < 144)\b]"
"[+        KEYFRAMEVALUE (\"total\", framei, 1);\b]"
"[+}' -kt string -vt int -cl total -fr 144 -se tos -p out\b]"
"}"
"[+?For the same sequence of records, count up how many calls start up per 10"
" min. interval, per different area code and exchange of origination:]{"
"[+]"
"[+ddssplitnaggr -ae $'{\b]"
"[+    static int spi = 10 * 60;\b]"
"[+    int framei;\b]"
"[+    KEYFRAMEVALUE (&npanxx, framei, 1);\b]"
"[+}' -kt int -vt int -cl npanxx -fr 144 -se tos -p out\b]"
"}"
"[+SEE ALSO?\bdds\b(1), \bdds\b(3), \baggr\b(1)]"
;

typedef struct channel_t {
    AGGRfile_t *afp;
} channel_t;

char *classp;
int keytype, keylen, valtype, vallen;
int framen, dictincr, itemincr, queuen;
int appendflag;
DDSexpr_t ae;
static DDSschema_t *schemap;

static DDSaggregator_t *aggregatorp;

void *channelopen (char *);
int channelclose (void *);
void *channelreopen (char *);

int main (int argc, char **argv) {
    int norun;
    char *schemaname;
    char *sexpr, *ssofile;
    char *aexpr, *afile, *asofile;
    char *prefix;
    char *ccflags, *ldflags;
    DDSsplitter_t *splitterp;
    struct rlimit rl;
    channel_t *chanp;
    DDSheader_t hdr;
    Sfio_t *ifp;
    int ret;

    if (AGGRinit () == -1)
        SUerror ("ddssplitnaggr", "AGGRinit failed");
    DDSinit ();
    schemaname = NULL;
    sexpr = NULL, ssofile = NULL;
    aexpr = NULL, afile = NULL, asofile = NULL;
    prefix = "dds.aggr";
    classp = "ddssplitnaggr";
    keytype = AGGR_TYPE_STRING, keylen = -1;
    valtype = AGGR_TYPE_FLOAT, vallen = AGGRtypelens[valtype];
    framen = 1;
    dictincr = 1, itemincr = 1;
    queuen = 1000000;
    appendflag = 0;
    schemap = NULL;
    ccflags = ldflags = "";
    norun = 0;
    for (;;) {
        switch (optget (argv, usage)) {
        case -100:
            schemaname = opt_info.arg;
            continue;
        case -230:
            sexpr = opt_info.arg;
            continue;
        case -232:
            ssofile = opt_info.arg;
            continue;
        case -270:
            aexpr = opt_info.arg;
            continue;
        case -271:
            afile = opt_info.arg;
            continue;
        case -272:
            asofile = opt_info.arg;
            continue;
        case -300:
            prefix = opt_info.arg;
            continue;
        case -301:
            classp = opt_info.arg;
            continue;
        case -302:
            if (AGGRparsetype (opt_info.arg, &keytype, &keylen) == -1)
                SUerror ("ddssplitnaggr", "bad argument for -kt option");
            continue;
        case -303:
            if (AGGRparsetype (opt_info.arg, &valtype, &vallen) == -1)
                SUerror ("ddssplitnaggr", "bad argument for -vt option");
            continue;
        case -304:
            framen = opt_info.num;
            continue;
        case -305:
            dictincr = opt_info.num;
            continue;
        case -306:
            itemincr = opt_info.num;
            continue;
        case -307:
            queuen = opt_info.num;
            continue;
        case -308:
            appendflag = 1;
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
            SUusage (0, "ddssplitnaggr", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "ddssplitnaggr", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;
    if (!sexpr && !ssofile) {
        SUusage (1, "ddssplitnaggr", opt_info.arg);
        return 1;
    }
    if (!aexpr && !afile && !asofile) {
        SUusage (1, "ddssplitnaggr", opt_info.arg);
        return 1;
    }
    if (argc > 1)
        SUerror ("ddssplitnaggr", "too many parameters");
    if (getrlimit (RLIMIT_NOFILE, &rl) != -1) {
        rl.rlim_cur = rl.rlim_max;
        setrlimit (RLIMIT_NOFILE, &rl);
    }
    ifp = sfstdin;
    if (argc == 1 && !(ifp = sfopen (NULL, argv[0], "r")))
        SUerror ("ddssplitnaggr", "sfopen failed for %s", argv[0]);
    if (!(ifp = SUsetupinput (ifp, 1048576)))
        SUerror ("ddssplitnaggr", "setupinput failed");
    if (!asofile && DDSparseexpr (aexpr, afile, &ae) == -1)
        SUerror ("ddssplitnaggr", "DDSparseexpr failed");
    if (!asofile && !ae.loop) {
        SUusage (1, "ddssplitnaggr", opt_info.arg);
        return 1;
    }
    if ((ret = DDSreadheader (ifp, &hdr)) == -1)
        SUerror ("ddssplitnaggr", "DDSreadheader failed");
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
            SUerror ("ddssplitnaggr", "no schema specified");
        if (!(schemap = DDSloadschema (schemaname, NULL, NULL)))
            SUerror ("ddssplitnaggr", "DDSloadschema failed");
    }
    if (!ssofile) {
        if (!(splitterp = DDScreatesplitter (schemap, sexpr, ccflags, ldflags)))
            SUerror ("ddssplitnaggr", "DDScreatesplitter failed");
    } else {
        if (!(splitterp = DDSloadsplitter (schemap, ssofile)))
            SUerror ("ddssplitnaggr", "DDSloadsplitter failed");
    }
    if ((*splitterp->init) (prefix, channelopen, channelreopen, channelclose))
        SUerror ("ddssplitnaggr", "splitterp->init failed");
    if (!asofile) {
        if (!(aggregatorp = DDScreateaggregator (
            schemap, &ae, valtype, ccflags, ldflags
        )))
            SUerror ("ddssplitnaggr", "DDScreateaggregator failed");
    } else {
        if (!(aggregatorp = DDSloadaggregator (schemap, asofile)))
            SUerror ("ddssplitnaggr", "DDSloadaggregator failed");
    }
    while ((ret = DDSreadrecord (
        ifp, schemap->recbuf, schemap
    )) == schemap->recsize) {
        if (!(chanp = (*splitterp->work) (schemap->recbuf, schemap->recsize)))
            SUerror ("ddssplitnaggr", "splitterp->work failed");
        (*aggregatorp->work) (schemap->recbuf, schemap->recsize, chanp->afp);
        ret = 0;
    }
    if (ret != 0)
        SUerror ("ddssplitnaggr", "incomplete read");
    if ((*splitterp->term) ())
        SUerror ("ddssplitnaggr", "splitterp->term failed");
    DDSdestroyaggregator (aggregatorp);
    DDSdestroysplitter (splitterp);

done:
    if (AGGRterm () == -1)
        SUerror ("ddssplitnaggr", "AGGRterm failed");
    DDSterm ();
    return 0;
}

void *channelopen (char *name) {
    channel_t *chanp;

    if (!(chanp = vmalloc (Vmheap, sizeof (channel_t))))
        SUerror ("ddssplitnaggr", "vmalloc failed for chanp");
    chanp->afp = NULL;
    if (appendflag) {
        if (!(chanp->afp = AGGRopen (name, AGGR_RWMODE_RW, queuen, FALSE)))
            SUwarning (1, "ddssplitnaggr", "AGGRopen failed");
    }
    if (!chanp->afp && !(chanp->afp = AGGRcreate (
        name, 1, framen, keytype, valtype, classp,
        keylen, vallen, dictincr, itemincr, queuen, FALSE
    )))
        SUerror ("ddssplitnaggr", "AGGRcreate failed");
    if ((*aggregatorp->init) (chanp->afp))
        SUerror ("ddssplitnaggr", "aggregatorp->init failed");
    return (void *) chanp;
}

int channelclose (void *p) {
    channel_t *chanp;

    chanp = p;
    if ((*aggregatorp->term) (chanp->afp))
        SUerror ("ddssplitnaggr", "aggregatorp->term failed");
    if (AGGRclose (chanp->afp) == -1)
        SUerror ("ddssplitnaggr", "AGGRclose failed");
    vmfree (Vmheap, chanp);
    return 0;
}

void *channelreopen (char *name) {
    channel_t *chanp;

    if (!(chanp = vmalloc (Vmheap, sizeof (channel_t))))
        SUerror ("ddssplitnaggr", "vmalloc failed for chanp");
    if (!(chanp->afp = AGGRopen (name, AGGR_RWMODE_RW, queuen, FALSE)))
        SUwarning (1, "ddssplitnaggr", "AGGRopen failed");
    return (void *) chanp;
}
