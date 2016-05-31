#pragma prototyped

#include <ast.h>
#include <option.h>
#include <sys/resource.h>
#include <swift.h>
#include <dds.h>

static const char usage[] =
"[-1p1s1D?\n@(#)$Id: ddssplit (AT&T Labs Research) 2001-09-24 $\n]"
USAGE_LICENSE
"[+NAME?ddssplit - split input stream into multiple files]"
"[+DESCRIPTION?\bddssplit\b reads records from a DDS data file (or standard"
" input) and writes them out on separate files, one for each unique"
" combination of the specified fields.]"
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
"[230:se?specifies the names of fields to be used as a key for splitting."
"]:['field1 ...']"
"[232:sso?specifies the file containing the precompiled shared object for"
" splitting."
"]:[file]"
"[300:p?specifies either the prefix or the name format for the output files."
" If string includes printf-style format directives, it is used as the name"
" format."
" In this case it must include as many directives as there are fields in the"
" list, and in the same order."
" Otherwise the string is assumed to be the prefix, in which case \bddssplit\b"
" will append a dot and the value for each field."
" The default prefix is \bddssplit\b."
"]:[(prefix|format)]"
"[308:append?specifies that output files are not to be erased before using."
" Any new data will be appended to existing data."
" This allows \bddssplit\b to work in an incremental mode."
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
"[+?\bddssplit -se f1 -p out\b]"
"}"
"[+?will create 2 files, out.10 (first 3 records) and out.20 (last record).]"
"{"
"[+]"
"[+?\bddssplit -se 'f1 f2' -p out\b]"
"}"
"[+?will create 3 files, out.10.11 (first 2 records), out.10.21 (3rd record),"
" and out.20.31 (last record).]"
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
    char *sexpr, *ssofile;
    char *prefix;
    char *ccflags, *ldflags;
    DDSschema_t *schemap;
    DDSsplitter_t *splitterp;
    struct rlimit rl;
    Sfio_t *ifp, *ofp;
    int ret;

    schemaname = NULL;
    osm = DDS_HDR_NAME | DDS_HDR_DESC;
    ozm = "none";
    contmode = 0;
    sexpr = NULL, ssofile = NULL;
    prefix = "ddssplit";
    ccflags = ldflags = "";
    schemap = NULL;
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
        case -230:
            sexpr = opt_info.arg;
            continue;
        case -232:
            ssofile = opt_info.arg;
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
            SUusage (0, "ddssplit", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "ddssplit", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;
    if (!sexpr && !ssofile) {
        SUusage (1, "ddssplit", opt_info.arg);
        return 1;
    }
    if (argc > 1)
        SUerror ("ddssplit", "too many parameters");
    if (getrlimit (RLIMIT_NOFILE, &rl) != -1) {
        rl.rlim_cur = rl.rlim_max;
        setrlimit (RLIMIT_NOFILE, &rl);
    }
    DDSinit ();
    ifp = sfstdin;
    if (argc == 1 && !(ifp = sfopen (NULL, argv[0], "r")))
        SUerror ("ddssplit", "sfopen failed for %s", argv[0]);
    if (!(ifp = SUsetupinput (ifp, 1048576)))
        SUerror ("ddssplit", "setupinput failed");
    if ((ret = DDSreadheader (ifp, &hdr)) == -1)
        SUerror ("ddssplit", "DDSreadheader failed");
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
            SUerror ("ddssplit", "no schema specified");
        if (!(schemap = DDSloadschema (schemaname, NULL, NULL)))
            SUerror ("ddssplit", "DDSloadschema failed");
    }
    if (!hdr.schemaname && schemaname)
        hdr.schemaname = schemaname;
    if (!hdr.schemap && schemap)
        hdr.schemap = schemap;
    hdr.contents = osm;
    hdr.vczspec = ozm;
    if (!ssofile) {
        if (!(splitterp = DDScreatesplitter (schemap, sexpr, ccflags, ldflags)))
            SUerror ("ddssplit", "DDScreatesplitter failed");
    } else {
        if (!(splitterp = DDSloadsplitter (schemap, ssofile)))
            SUerror ("ddssplit", "DDSloadsplitter failed");
    }
    if ((*splitterp->init) (prefix, channelopen, channelreopen, channelclose))
        SUerror ("ddssplit", "splitterp->init failed");
    while ((ret = DDSreadrecord (
        ifp, schemap->recbuf, schemap
    )) == schemap->recsize) {
        if ((ofp = (*splitterp->work) (
            schemap->recbuf, schemap->recsize
        ))) {
            if (DDSwriterecord (
                ofp, schemap->recbuf, schemap
            ) != schemap->recsize)
                SUerror ("ddssplit", "incomplete write");
        } else
            SUerror ("ddssplit", "splitterp->work failed");
    }
    if (ret != 0)
        SUerror ("ddssplit", "incomplete read");
    if ((*splitterp->term) ())
        SUerror ("ddssplit", "splitterp->term failed");
    if (sfsync (NULL) < 0)
        SUerror ("ddssplit", "sync failed");
    DDSdestroysplitter (splitterp);

done:
    DDSterm ();
    return 0;
}

void *channelopen (char *name) {
    Sfio_t *fp;

    if (contmode) {
        if (!(fp = sfopen (NULL, name, "a")))
            SUerror ("ddssplit", "open failed for %s", name);
        if (!(fp = SUsetupoutput (NULL, fp, 1048576)))
            SUerror ("ddssplit", "setupoutput failed");
        if (sfsize (fp) == 0 && osm && DDSwriteheader (fp, &hdr) == -1)
            SUerror ("ddssplit", "DDSwriteheader failed");
    } else {
        if (!(fp = sfopen (NULL, name, "w")))
            SUerror ("ddssplit", "open failed for %s", name);
        if (!(fp = SUsetupoutput (NULL, fp, 1048576)))
            SUerror ("ddssplit", "setupoutput failed");
        if (osm && DDSwriteheader (fp, &hdr) == -1)
            SUerror ("ddssplit", "DDSwriteheader failed");
    }
    return (void *) fp;
}

int channelclose (void *fp) {
    return sfclose ((Sfio_t *) fp) ? 0 : -1;
}

void *channelreopen (char *name) {
    Sfio_t *fp;

    if (!(fp = sfopen (NULL, name, "a")))
        SUerror ("ddssplit", "open failed for %s", name);
    if (!(fp = SUsetupoutput (NULL, fp, 1048576)))
        SUerror ("ddssplit", "setupoutput failed");
    return (void *) fp;
}
