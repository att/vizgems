#pragma prototyped

#include <ast.h>
#include <option.h>
#include <swift.h>
#include <dds.h>

static const char usage[] =
"[-1p1s1D?\n@(#)$Id: ddsfilter (AT&T Labs Research) 2001-09-24 $\n]"
USAGE_LICENSE
"[+NAME?ddsfilter - filter records]"
"[+DESCRIPTION?\bddsfilter\b reads records from a DDS data file (or standard"
" input) and uses the specified expression to decide whether to keep or drop"
" each record from the output."
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
" On every invocation, the \bLOOP\b expression must call one of two macros:"
" \bKEEP\b or \bDROP\b."
" The special variable \bNREC\b holds the total number of records in the input"
" file, or \b-1\b if the input is not a seekable file."
" The special variable \bINDEX\b holds the index of the current record."
" When \bNREC\b is not \b-1\b, \bINDEX\b may be set to a value between \b0\b"
" and \bNREC - 1\b to indicate that \bddsfilter\b should read the record with"
" that index next."
" If \bINDEX\b is not set, \bddsfilter\b will read all the records in sequence."
"]"
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
"[220:fe?specifies the filtering expression on the command line."
"]:['expression']"
"[221:ff?specifies the file containing the filtering expression."
"]:[file]"
"[222:fso?specifies the file containing the precompiled shared object for"
" filtering."
"]:[file]"
"[400:ccflags?specifies extra flags for the compiler such as -I flags."
"]:[ccflags]"
"[401:ldflags?specifies extra flags for the compiler such as -L flags."
"]:[ldflags]"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"\n"
"\n[ file ]\n"
"\n"
"[+EXAMPLES?Keep only phone records whose dialed area code is 800:]{"
"[+]"
"[+ddsfilter -fe '{ if (dialednpa == 800) KEEP; else DROP; }'\b]"
"}"
"[+?Another way to specify the same:]{"
"[+]"
"[+ddsfilter -fe '{ DROP; if (dialednpa == 800) KEEP; }'\b]"
"}"
"[+?Keep only records with timestamps between 0 and 3 hours."
" The time field is in seconds:]{"
"[+]"
"[+ddsfilter -fe '{\b]"
"[+    int t;\b]"
"[+    t = time / (60 * 60);\b]"
"[+    if (t >= 0 && t <= 3)\b]"
"[+        KEEP;\b]"
"[+    else\b]"
"[+        DROP;\b]"
"[+}'\b]"
"}"
"[+?Convert all values of 99999 to 0 for a field:]{"
"[+]"
"[+ddsfilter -fe '{ KEEP; if (ecode == 99999) ecode = 0; }'\b]"
"}"
"[+SEE ALSO?\bdds\b(1), \bdds\b(3), \baggr\b(1)]"
;

#ifdef FEATURE_NO64FS
typedef off_t off64_t;
#endif

int main (int argc, char **argv) {
    int norun;
    char *schemaname;
    int osm;
    char *ozm;
    char *cmds, *fexpr, *ffile, *fsofile;
    char *ccflags, *ldflags;
    DDSexpr_t fe;
    DDSschema_t *schemap;
    DDSfilter_t *filterp;
    DDSheader_t hdr;
    Sfio_t *ifp, *ofp;
    off64_t base, offset;
    Sfoff_t size;
    long long pindex, index, nrec;
    int ret;

    schemaname = NULL;
    schemap = NULL;
    osm = DDS_HDR_NAME | DDS_HDR_DESC;
    ozm = "none";
    fexpr = NULL, ffile = NULL, fsofile = NULL;
    cmds = NULL;
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
        case -103:
            cmds = opt_info.arg;
            continue;
        case -104:
            ozm = opt_info.arg;
            continue;
        case -220:
            fexpr = opt_info.arg;
            continue;
        case -221:
            ffile = opt_info.arg;
            continue;
        case -222:
            fsofile = opt_info.arg;
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
            SUusage (0, "ddsfilter", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "ddsfilter", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;
    if (argc > 1)
        SUerror ("ddsfilter", "too many parameters");
    DDSinit ();
    ifp = sfstdin;
    if (argc == 1 && !(ifp = sfopen (NULL, argv[0], "r")))
        SUerror ("ddsfilter", "sfopen failed for %s", argv[0]);
    if (!(ifp = SUsetupinput (ifp, 1048576)))
        SUerror ("ddsfilter", "setupinput failed");
    if (!(ofp = SUsetupoutput (cmds, sfstdout, 1048576)))
        SUerror ("ddsfilter", "setupoutput failed");
    if (!fexpr && !ffile && !fsofile)
        fexpr = strdup ("{ KEEP; }");
    if (!fsofile && DDSparseexpr (fexpr, ffile, &fe) == -1)
        SUerror ("ddsfilter", "DDSparseexpr failed");
    if ((ret = DDSreadheader (ifp, &hdr)) == -1)
        SUerror ("ddsfilter", "DDSreadheader failed");
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
            SUerror ("ddsfilter", "no schema specified");
        if (!(schemap = DDSloadschema (schemaname, NULL, NULL)))
            SUerror ("ddsfilter", "DDSloadschema failed");
    }
    if (!hdr.schemaname && schemaname)
        hdr.schemaname = schemaname;
    if (!hdr.schemap && schemap)
        hdr.schemap = schemap;
    hdr.contents = osm;
    hdr.vczspec = ozm;
    if (osm && DDSwriteheader (ofp, &hdr) == -1)
        SUerror ("ddsfilter", "DDSwriteheader failed");
    if (!fsofile) {
        if (!(filterp = DDScreatefilter (schemap, &fe, ccflags, ldflags)))
            SUerror ("ddsfilter", "DDScreatefilter failed");
    } else {
        if (!(filterp = DDSloadfilter (schemap, fsofile)))
            SUerror ("ddsfilter", "DDSloadfilter failed");
    }
    if ((base = sfseek (ifp, 0, SEEK_CUR)) != -1 && (size = sfsize (ifp)) != -1)
        nrec = (size - base) / schemap->recsize;
    else
        nrec = -1;
    pindex = index = 0;
    if ((*filterp->init) () == -1)
        SUerror ("ddsfilter", "filterp->init failed");
    while ((ret = DDSreadrecord (
        ifp, schemap->recbuf, schemap
    )) == schemap->recsize) {
        if ((*filterp->work) (
            schemap->recbuf, schemap->recsize, &index, nrec
        ) == 1) {
            if (DDSwriterecord (
                ofp, schemap->recbuf, schemap
            ) != schemap->recsize)
                SUerror ("ddsfilter", "incomplete write");
        }
        if (index != pindex) {
            if (nrec == -1)
                SUerror ("ddsfilter", "input channel not seekable");
            if ((offset = base + index * schemap->recsize) > size)
                offset = size;
            if (index != pindex + 1 && sfseek (ifp, offset, SEEK_SET) != offset)
                SUerror ("ddsfilter", "sfseek-set failed");
        } else
            index++;
        pindex = index;
    }
    if (ret != 0)
        SUerror ("ddsfilter", "incomplete read");
    if ((*filterp->term) () == -1)
        SUerror ("ddsfilter", "filterp->term failed");
    if (sfsync (NULL) < 0)
        SUerror ("ddsfilter", "sync failed");
    if (ofp != sfstdout)
        sfclose (ofp);
    DDSdestroyfilter (filterp);

done:
    DDSterm ();
    return 0;
}
