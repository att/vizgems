#pragma prototyped

#include <ast.h>
#include <option.h>
#include <swift.h>
#include <dds.h>

static const char usage[] =
"[-1p1s1D?\n@(#)$Id: ddsconv (AT&T Labs Research) 2001-09-24 $\n]"
USAGE_LICENSE
"[+NAME?ddsconv - convert records from one schema to another]"
"[+DESCRIPTION?\bddsconv\b reads records from a DDS data file (or standard"
" input) and uses the supplied expression to convert them to a different"
" record format."
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
" In the \bLOOP\b section, the fields of the current input record can be"
" referenced by using their names prefixed with \bIN->\b."
" The output fields can be referenced by using their names prefixed with"
" \bOUT->\b."
" On every invocation, the LOOP expression must assign all the fields of the"
" output record using the fields in the input record."
" To emit multiple output records per input record, the macro \bEMIT\b can"
" be used in the \bLOOP\b section."
" Calling this macro causes the current output record to be written out."
" The code can then assign new values to the output record and call the"
" macro again."
" The code \aMUST\a re-assign all fields before calling \bEMIT\b again."
" The special variable \bNREC\b holds the total number of records in the input"
" file, or \b-1\b if the input is not a seekable file."
" The special variable \bINDEX\b holds the index of the current record."
" When \bNREC\b is not \b-1\b, \bINDEX\b may be set to a value between \b0\b"
" and \bNREC - 1\b to indicate that \bddsconv\b should read the record with"
" that index next."
" If \bINDEX\b is not set, \bddsconv\b will read all the records in sequence."
"]"
"[100:is?specifies the schema of the input DDS file."
" This flag overrides any schema specified in the input file."
" Can be used to \aimport\a a fixed record data file that has no DDS header."
"]:[schemafile]"
"[101:os?specifies the schema of the output DDS file."
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
"[103:oce?specifies that the output data should be written to one or more"
" pipelines."
" Each command in the argument is assumed to be startable using the user's"
" shell (pointed to by the SHELL environment variable)."
" All commands must be DDS commands, or if the commands are scripts, they must"
" use a DDS command to process the input data."
" Each command should be enclosed in quotes if it contains spaces."
" The entire option string should be enclosed in quotes (different than the"
" ones used per command)."
" The default is to write the data to the standard output."
"]:['cmd1 ...']"
"[104:ozm?specifies the compression mode for the output file."
" The mode can be a vcodex spec, or \bnone\b for no compression,"
" or \brdb/rtb\b for the vcodex \brdb/rtable\b codex,"
" using the DDS schema field sizes."
" The default mode is \bnone\b."
"]:['vcodex spec|none|rdb|rtb']"
"[260:ce?specifies the converter on the command line."
"]:['expression']"
"[261:cf?specifies the file containing the converter expression."
"]:[file]"
"[262:cso?specifies the file containing the precompiled shared object for"
" conversion."
"]:[file]"
"[400:ccflags?specifies extra flags for the compiler such as -I flags."
"]:[ccflags]"
"[401:ldflags?specifies extra flags for the compiler such as -L flags."
"]:[ldflags]"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"\n"
"\n[ file ]\n"
"\n"
"[+EXAMPLES?Convert the time field from seconds to minutes:]{"
"[+]"
"[+ddsconv -ce '{ OUT->time = IN->time / 60; }'\b]"
"}"
"[+SEE ALSO?\bdds\b(1), \bdds\b(3), \baggr\b(1)]"
;

#ifdef FEATURE_NO64FS
typedef off_t off64_t;
#endif

static DDSschema_t *ischemap, *oschemap;
static Sfio_t *emitfp;

static int emit (Sfio_t *, void *, DDSschema_t *, size_t);

int main (int argc, char **argv) {
    int norun;
    char *ischemaname, *oschemaname;
    int osm;
    char *ozm;
    char *cmds, *cexpr, *cfile, *csofile;
    char *ccflags, *ldflags;
    DDSexpr_t ce;
    DDSconverter_t *converterp;
    DDSheader_t ihdr, ohdr;
    Sfio_t *ifp, *ofp;
    off64_t base, offset;
    Sfoff_t size;
    long long pindex, index, nrec;
    int ret;

    ischemaname = NULL;
    ischemap = NULL;
    oschemaname = NULL;
    oschemap = NULL;
    osm = DDS_HDR_NAME | DDS_HDR_DESC;
    ozm = "none";
    cexpr = NULL, cfile = NULL, csofile = NULL;
    cmds = NULL;
    ccflags = ldflags = "";
    norun = 0;
    for (;;) {
        switch (optget (argv, usage)) {
        case -100:
            ischemaname = opt_info.arg;
            continue;
        case -101:
            oschemaname = opt_info.arg;
            continue;
        case -102:
            if (strcmp (opt_info.arg, "none") == 0)
                osm = 0;
            else if (strcmp (opt_info.arg, "name") == 0)
                osm = DDS_HDR_NAME;
            else if (strcmp (opt_info.arg, "full") == 0)
                osm = DDS_HDR_NAME | DDS_HDR_DESC;
            continue;
        case -103:
            cmds = opt_info.arg;
            continue;
        case -104:
            ozm = opt_info.arg;
            continue;
        case -260:
            cexpr = opt_info.arg;
            continue;
        case -261:
            cfile = opt_info.arg;
            continue;
        case -262:
            csofile = opt_info.arg;
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
            SUusage (0, "ddsconv", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "ddsconv", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;
    if (!cexpr && !cfile && !csofile) {
        SUusage (1, "ddsconv", opt_info.arg);
        return 1;
    }
    if (argc > 1)
        SUerror ("ddsconv", "too many parameters");
    DDSinit ();
    ifp = sfstdin;
    if (argc == 1 && !(ifp = sfopen (NULL, argv[0], "r")))
        SUerror ("ddsconv", "sfopen failed for %s", argv[0]);
    if (!(ifp = SUsetupinput (ifp, 1048576)))
        SUerror ("ddsconv", "setupinput failed");
    if (!(ofp = SUsetupoutput (cmds, sfstdout, 1048576)))
        SUerror ("ddsconv", "setupoutput failed");
    emitfp = ofp;
    if (!csofile && DDSparseexpr (cexpr, cfile, &ce) == -1)
        SUerror ("ddsconv", "DDSparseexpr failed");
    if ((ret = DDSreadheader (ifp, &ihdr)) == -1)
        SUerror ("ddsconv", "DDSreadheader failed");
    if (ret == -2)
        goto done;
    if (ret == 0) {
        if (ihdr.schemap)
            ischemap = ihdr.schemap;
        if (ihdr.schemaname)
            ischemaname = ihdr.schemaname;
    }
    if (!ischemap) {
        if (!ischemaname)
            SUerror ("ddsconv", "no input schema specified");
        if (!(ischemap = DDSloadschema (ischemaname, NULL, NULL)))
            SUerror ("ddsconv", "DDSloadschema failed for ischemap");
    }
    if (!oschemaname)
        SUerror ("ddsconv", "no output schema specified");
    if (!(oschemap = DDSloadschema (oschemaname, NULL, NULL)))
        SUerror ("ddsconv", "DDSloadschema failed for oschemap");
    ohdr.schemaname = oschemaname;
    ohdr.schemap = oschemap;
    ohdr.contents = osm;
    ohdr.vczspec = ozm;
    if (osm && DDSwriteheader (ofp, &ohdr) == -1)
        SUerror ("ddsconv", "DDSwriteheader failed");
    if (!csofile) {
        if (!(converterp = DDScreateconverter (
            ischemap, oschemap, &ce, ccflags, ldflags
        )))
            SUerror ("ddsconv", "DDScreateconverter failed");
    } else {
        if (!(converterp = DDSloadconverter (ischemap, oschemap, csofile)))
            SUerror ("ddsconv", "DDSloadconverter failed");
    }
    if ((base = sfseek (ifp, 0, SEEK_CUR)) != -1 && (size = sfsize (ifp)) != -1)
        nrec = (size - base) / ischemap->recsize;
    else
        nrec = -1;
    pindex = index = 0;
    if ((*converterp->init) () == -1)
        SUerror ("ddsconv", "converterp->init failed");
    while ((ret = DDSreadrecord (
        ifp, ischemap->recbuf, ischemap
    )) == ischemap->recsize) {
        if ((*converterp->work) (
            ischemap->recbuf, ischemap->recsize, ischemap,
            oschemap->recbuf, oschemap->recsize, oschemap,
            &index, nrec, emit, emitfp
        ) == 1) {
            if (DDSwriterecord (
                ofp, oschemap->recbuf, oschemap
            ) != oschemap->recsize)
                SUerror ("ddsconv", "incomplete write");
        }
        if (index != pindex) {
            if (nrec == -1)
                SUerror ("ddsconv", "input channel not seekable");
            if ((offset = base + index * ischemap->recsize) > size)
                offset = size;
            if (index != pindex + 1 && sfseek (ifp, offset, SEEK_SET) != offset)
                SUerror ("ddsconv", "sfseek-set failed");
        } else
            index++;
        pindex = index;
    }
    if (ret != 0)
        SUerror ("ddsconv", "incomplete read");
    if ((*converterp->term) () == -1)
        SUerror ("ddsconv", "converterp->term failed");
    if (sfsync (NULL) < 0)
        SUerror ("ddsconv", "sync failed");
    if (ofp != sfstdout)
        sfclose (ofp);
    DDSdestroyconverter (converterp);

done:
    DDSterm ();
    return 0;
}

static int emit (Sfio_t *fp, void *buf, DDSschema_t *schemap, size_t len) {
    if (DDSwriterecord (fp, buf, schemap) != len)
        SUerror ("ddsconv", "incomplete write");
    return 0;
}
