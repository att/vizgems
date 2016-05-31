#pragma prototyped

#include <ast.h>
#include <option.h>
#include <swift.h>
#include <dds.h>

static const char usage[] =
"[-1p1s1D?\n@(#)$Id: ddsextract (AT&T Labs Research) 2001-09-24 $\n]"
USAGE_LICENSE
"[+NAME?ddsextract - extract fields]"
"[+DESCRIPTION?\bddsextract\b reads records from a DDS data file (or standard"
" input) and writes out records that include only the specified fields.]"
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
"[250:ee?specifies the names of fields to extract from the DDS file."
"]:['field1 ...']"
"[252:eso?specifies the file containing the precompiled shared object for"
" extraction."
"]:[file]"
"[400:ccflags?specifies extra flags for the compiler such as -I flags."
"]:[ccflags]"
"[401:ldflags?specifies extra flags for the compiler such as -L flags."
"]:[ldflags]"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"\n"
"\n[ file ]\n"
"\n"
"[+EXAMPLES?extract two fields out of the input file:"
" \bddsextract -ee 'f1 f2'\b"
"]"
"[+SEE ALSO?\bdds\b(1), \bdds\b(3), \baggr\b(1)]"
;

int main (int argc, char **argv) {
    int norun;
    char *schemaname;
    int osm;
    char *ozm;
    char *cmds, *ename, *eexpr, *esofile;
    char *ccflags, *ldflags;
    DDSschema_t *schemap;
    DDSheader_t hdr;
    DDSextractor_t *extractorp;
    Sfio_t *ifp, *ofp;
    int ret;

    schemaname = NULL;
    osm = DDS_HDR_NAME | DDS_HDR_DESC;
    ozm = "none";
    ename = NULL, eexpr = NULL, esofile = NULL;
    schemap = NULL;
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
        case -250:
            eexpr = opt_info.arg;
            continue;
        case -252:
            esofile = opt_info.arg;
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
            SUusage (0, "ddsextract", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "ddsextract", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;
    if (!eexpr && !esofile) {
        SUusage (1, "ddsextract", opt_info.arg);
        return 1;
    }
    if (argc > 1)
        SUerror ("ddsextract", "too many parameters");
    DDSinit ();
    ifp = sfstdin;
    if (argc == 1 && !(ifp = sfopen (NULL, argv[0], "r")))
        SUerror ("ddsextract", "sfopen failed for %s", argv[0]);
    if (!(ifp = SUsetupinput (ifp, 1048576)))
        SUerror ("ddsextract", "setupinput failed");
    if (!(ofp = SUsetupoutput (cmds, sfstdout, 1048576)))
        SUerror ("ddsextract", "setupoutput failed");
    if ((ret = DDSreadheader (ifp, &hdr)) == -1)
        SUerror ("ddsextract", "DDSreadheader failed");
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
            SUerror ("ddsextract", "no schema specified");
        if (!(schemap = DDSloadschema (schemaname, NULL, NULL)))
            SUerror ("ddsextract", "DDSloadschema failed");
    }
    if (!esofile) {
        if (!(extractorp = DDScreateextractor (
            schemap, ename, eexpr, ccflags, ldflags
        )))
            SUerror ("ddsextract", "DDScreateextractor failed");
    } else {
        if (!(extractorp = DDSloadextractor (schemap, esofile)))
            SUerror ("ddsextract", "DDSloadextractor failed");
    }
    hdr.schemaname = extractorp->schemap->name;
    hdr.schemap = extractorp->schemap;
    hdr.contents = osm;
    hdr.vczspec = ozm;
    if (osm && DDSwriteheader (ofp, &hdr) == -1)
        SUerror ("ddsextract", "DDSwriteheader failed");
    if ((*extractorp->init) ())
        SUerror ("ddsextract", "extractorp->init failed");
    while ((ret = DDSreadrecord (
        ifp, schemap->recbuf, schemap
    )) == schemap->recsize) {
        (*extractorp->work) (
            schemap->recbuf, schemap->recsize,
            extractorp->schemap->recbuf, extractorp->schemap->recsize
        );
        if (DDSwriterecord (
            ofp, extractorp->schemap->recbuf, extractorp->schemap
        ) != extractorp->schemap->recsize)
            SUerror ("ddsextract", "incomplete write");
    }
    if (ret != 0)
        SUerror ("ddsextract", "incomplete read");
    if ((*extractorp->term) ())
        SUerror ("ddsextract", "extractorp->term failed");
    if (sfsync (NULL) < 0)
        SUerror ("ddsextract", "sync failed");
    if (ofp != sfstdout)
        sfclose (ofp);
    DDSdestroyextractor (extractorp);

done:
    DDSterm ();
    return 0;
}
