#pragma prototyped

#include <ast.h>
#include <option.h>
#include <tok.h>
#include <swift.h>
#include <dds.h>

static const char usage[] =
"[-1p1s1D?\n@(#)$Id: ddsload (AT&T Labs Research) 2001-09-24 $\n]"
USAGE_LICENSE
"[+NAME?ddsload - convert ascii input to DDS records]"
"[+DESCRIPTION?\bddsload\b reads lines of ascii text from a file (or standard"
" input) and generates a DDS data file."
"]"
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
"[280:le?specifies the character to use as field separator."
" The default is space."
" This has no effect when the type option is binary."
"]:['separator']"
"[281:lnts?specifies that all char fields are to be null terminated."
" The default is to allow strings that are as big as the field size."
" This has no effect when the type option is binary."
"]"
"[282:lso?specifies the file containing the precompiled shared object for"
" loading."
"]:[file]"
"[283:dec?specifies the type of decoding to perform on ascii input."
"]:[(std|simple|url|html):oneof]{"
"[+std?std, handle '\\x' sequences]"
"[+simple?simple, un-escape just the '\\delimiter' sequence]"
"[+url?url, un-escape url encodings]"
"[+html?html, un-escape html encodings]"
"}"
"[316:type?specifies the format of the input stream."
"]:[(ascii|binary):oneof]{"
"[+ascii?ascii character delimited format][+binary?binary fixed length format]"
"}"
"[317:skip?specifies number of lines (ascii type) or bytes (binary type)"
" to skip over before starting to read data."
" The default is \b0\b."
"]#[n]"
"[400:ccflags?specifies extra flags for the compiler such as -I flags."
"]:[ccflags]"
"[401:ldflags?specifies extra flags for the compiler such as -L flags."
"]:[ldflags]"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"\n"
"\n[ file ]\n"
"\n"
"[+EXAMPLES?Load an ascii file containing 2 fields separated by \b|\b:]{"
"[+]"
"[+ddsload -os two.schema -le '|'\b]"
"}"
"[+SEE ALSO?\bdds\b(1), \bdds\b(3), \baggr\b(1)]"
;

#define DEC_STD    1
#define DEC_SIMPLE 2
#define DEC_URL    3
#define DEC_HTML   4

static void simpledec (char *, char);
static void urldec (char *);
static void htmldec (char *);

int main (int argc, char **argv) {
    int norun;
    char *oschemaname;
    int osm;
    char *ozm;
    int dec;
    int asciitype;
    char *cmds, *lexpr, *lsofile;
    char *ccflags, *ldflags;
    int skip, ntsflag, i;
    char c;
    DDSschema_t *oschemap;
    DDSloader_t *loaderp;
    DDSheader_t ohdr;
    Sfio_t *ifp, *ofp;
    char *ws[10000], *line, *s;
    int wn, wi, n, ret2;

    oschemaname = NULL;
    oschemap = NULL;
    osm = DDS_HDR_NAME | DDS_HDR_DESC;
    ozm = "none";
    dec = DEC_STD;
    asciitype = TRUE;
    lexpr = " ", lsofile = NULL;
    skip = 0;
    ntsflag = FALSE;
    ccflags = ldflags = "";
    cmds = NULL;
    norun = 0;
    for (;;) {
        switch (optget (argv, usage)) {
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
        case -280:
            lexpr = opt_info.arg;
            continue;
        case -281:
            ntsflag = TRUE;
            continue;
        case -282:
            lsofile = opt_info.arg;
            continue;
        case -283:
            if (strcmp (opt_info.arg, "std") == 0)
                dec = DEC_STD;
            else if (strcmp (opt_info.arg, "simple") == 0)
                dec = DEC_SIMPLE;
            else if (strcmp (opt_info.arg, "url") == 0)
                dec = DEC_URL;
            else if (strcmp (opt_info.arg, "html") == 0)
                dec = DEC_HTML;
            continue;
        case -316:
            if (strcmp (opt_info.arg, "ascii") == 0)
                asciitype = TRUE;
            else if (strcmp (opt_info.arg, "binary") == 0)
                asciitype = FALSE;
            continue;
        case -317:
            skip = opt_info.num;
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
            SUusage (0, "ddsload", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "ddsload", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;
    if (asciitype && !lexpr && !lsofile) {
        SUusage (1, "ddsload", opt_info.arg);
        return 1;
    }
    if (argc > 1)
        SUerror ("ddsload", "too many parameters");
    DDSinit ();
    ifp = sfstdin;
    if (argc == 1 && !(ifp = sfopen (NULL, argv[0], "r")))
        SUerror ("ddsload", "sfopen failed for %s", argv[0]);
    if (!(ofp = SUsetupoutput (cmds, sfstdout, 1048576)))
        SUerror ("ddsload", "setupoutput failed");
    if (!oschemaname)
        SUerror ("ddsload", "no output schema specified");
    if (!(oschemap = DDSloadschema (oschemaname, NULL, NULL)))
        SUerror ("ddsload", "DDSloadschema failed for oschemap");
    ohdr.schemaname = oschemaname;
    ohdr.schemap = oschemap;
    ohdr.contents = osm;
    ohdr.vczspec = ozm;
    if (osm && DDSwriteheader (ofp, &ohdr) == -1)
        SUerror ("ddsload", "DDSwriteheader failed");
    if (asciitype) {
        if (skip > 0)
            for (i = 0; i < skip; i++)
                if (!sfgetr (ifp, '\n', SF_STRING))
                    SUerror ("ddsload", "EOF in header");
        if (!lsofile) {
            if (!(loaderp = DDScreateloader (oschemap, ccflags, ldflags)))
                SUerror ("ddsload", "DDScreateloader failed");
        } else {
            if (!(loaderp = DDSloadloader (oschemap, lsofile)))
                SUerror ("ddsload", "DDSloadloader failed");
        }
        if ((*loaderp->init) () == -1)
            SUerror ("ddsload", "loaderp->init failed");
        n = 1;
        while ((line = sfgetr (ifp, '\n', SF_STRING))) {
            if (lexpr[0] == ' ' || lexpr[0] == '\t') {
                if ((wn = tokscan (line, &s, " %v ", ws, 10000)) < 0)
                    SUerror ("ddsload", "tokscan failed");
            } else {
                s = line;
                for (wn = 0; wn < 10000; ) {
                    ws[wn++] = s;
                    while (*s && (
                        *s != lexpr[0] || (s != line && *(s - 1) == '\\')
                    ))
                        s++;
                    if (*s == lexpr[0])
                        *s++ = 0;
                    else if (!*s)
                        break;
                }
            }
            for (wi = 0; wi < wn; wi++) {
                switch (dec) {
                case DEC_STD:    stresc (ws[wi]);              break;
                case DEC_SIMPLE: simpledec (ws[wi], lexpr[0]); break;
                case DEC_URL:    urldec (ws[wi]);              break;
                case DEC_HTML:   htmldec (ws[wi]);             break;
                }
            }
            if ((ret2 = (*loaderp->work) (
                ws, wn, oschemap->recbuf, oschemap->recsize, ntsflag
            )) == -1)
                SUwarning (0, "ddsload", "bad record");
            else if (ret2 == -2)
                SUwarning (1, "ddsload", "record %d truncated", n);
            if (DDSwriterecord (
                ofp, oschemap->recbuf, oschemap
            ) != oschemap->recsize)
                SUerror ("ddsload", "incomplete write");
            n++;
        }
        if ((*loaderp->term) () == -1)
            SUerror ("ddsload", "loaderp->term failed");
        if (sfsync (NULL) < 0)
            SUerror ("ddsload", "sync failed");
        if (ofp != sfstdout)
            sfclose (ofp);
        DDSdestroyloader (loaderp);
    } else {
        if (skip > 0)
            for (i = 0; i < skip; i++)
                if (sfread (ifp, &c, 1) != 1)
                    SUerror ("ddsload", "EOF in header");
        if (sfmove (ifp, ofp, -1, -1) == -1)
            SUerror ("ddsload", "move failed");
        if (sfsync (NULL) < 0)
            SUerror ("ddsload", "sync failed");
        if (ofp != sfstdout)
            sfclose (ofp);
    }
    DDSterm ();
    return 0;
}

static void simpledec (char *str, char sep) {
    char *is, *os;

    for (is = os = str; *is; ) {
        if (*is == '\\' && *(is + 1) == sep) {
            *os++ = *(is + 1);
            is += 2;
        } else if (*is == '\r')
            is++;
        else
            *os++ = *is++;
    }
    *os = 0;

    return;
}

static void urldec (char *str) {
    char *is, *os, c1, c2;
    int i1, i2;

    for (is = os = str; *is; ) {
        if (*is == '%' && *(is + 1) && *(is + 2)) {
            c1 = *(is + 1), c2 = *(is + 2);
            if (c1 >= '0' && c1 <= '9')
                i1 = c1 - '0';
            else if (c1 >= 'A' && c1 <= 'F')
                i1 = 10 + c1 - 'A';
            else if (c1 >= 'a' && c1 <= 'f')
                i1 = 10 + c1 - 'a';
            else {
                *os++ = *is++;
                continue;
            }
            if (c2 >= '0' && c2 <= '9')
                i2 = c2 - '0';
            else if (c2 >= 'A' && c2 <= 'F')
                i2 = 10 + c2 - 'A';
            else if (c2 >= 'a' && c2 <= 'f')
                i2 = 10 + c2 - 'a';
            else {
                *os++ = *is++;
                continue;
            }
            *os++ = i1 * 16 + i2;
            is += 3;
        } else
            *os++ = *is++;
    }
    *os = 0;

    return;
}

static void htmldec (char *str) {
    char *is, *os, c1, c2;
    int i1, i2;

    for (is = os = str; *is; ) {
        if (
            *is == '&' && *(is + 1) == '#' && *(is + 2) == 'x' &&
            *(is + 3) && *(is + 4)
        ) {
            c1 = *(is + 3), c2 = *(is + 4);
            if (c1 >= '0' && c1 <= '9')
                i1 = c1 - '0';
            else if (c1 >= 'A' && c1 <= 'F')
                i1 = 10 + c1 - 'A';
            else if (c1 >= 'a' && c1 <= 'f')
                i1 = 10 + c1 - 'a';
            else {
                *os++ = *is++;
                continue;
            }
            if (c2 >= '0' && c2 <= '9')
                i2 = c2 - '0';
            else if (c2 >= 'A' && c2 <= 'F')
                i2 = 10 + c2 - 'A';
            else if (c2 >= 'a' && c2 <= 'f')
                i2 = 10 + c2 - 'a';
            else {
                *os++ = *is++;
                continue;
            }
            *os++ = i1 * 16 + i2;
            is += 5;
        } else
            *os++ = *is++;
    }
    *os = 0;

    return;
}
