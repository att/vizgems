#pragma prototyped

#include <ast.h>
#include <option.h>
#include <swift.h>
#include <dds.h>

static const char usage[] =
"[-1p1s1D?\n@(#)$Id: ddschoose (AT&T Labs Research) 2001-09-24 $\n]"
USAGE_LICENSE
"[+NAME?ddschoose - extract specific records]"
"[+DESCRIPTION?\bddschoose\b reads indices of records from standard input and"
" extracts the records from the DDS file specified as its argument or from a"
" list of files specified with the \bmf\b option."
" The indices are by default 8 byte binary numbers."
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
"[318:mf?specifies a file containing information on multiple DDS files. "
" This file should be of the form: \bstartindex endindex filename\b."
" For each index, \bddschoose\b searches through this list to find the correct"
" index range and identify which file to use."
" It will then extract the record with index \bindex - startindex\b from that"
" file."
" The sequence of indices need not be sorted, although the program will run"
" faster if it is."
"]:[file]"
"[319:d?specifies the directory that all files names are relative to."
" The default directory is the current working directory."
"]:[directory]"
"[330:il?specifies the length of the indices."
" The default is 8 bytes (long long) but 4 bytes is also allowed (int)."
"]#[(4|8):oneof]{[+4][+8]}"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"\n"
"\n[ file ]\n"
"\n"
"[+SEE ALSO?\bdds\b(1), \bdds\b(3), \baggr\b(1)]"
;

#if SWIFT_LITTLEENDIAN == 1
static unsigned char *__cp, __c;
#define SW4(ip) ( \
    __cp = (char *) (ip), \
    __c = __cp[0], __cp[0] = __cp[3], __cp[3] = __c, \
    __c = __cp[1], __cp[1] = __cp[2], __cp[2] = __c \
)
#define SW8(dp) ( \
    __cp = (char *) (dp), \
    __c = __cp[0], __cp[0] = __cp[7], __cp[7] = __c, \
    __c = __cp[1], __cp[1] = __cp[6], __cp[6] = __c, \
    __c = __cp[2], __cp[2] = __cp[5], __cp[5] = __c, \
    __c = __cp[3], __cp[3] = __cp[4], __cp[4] = __c \
)
#else
#define SW4(sp) (sp)
#define SW8(sp) (sp)
#endif

typedef struct rangemap_t {
    long long start, end;
    char *fname;
} rangemap_t;

static rangemap_t *rangemaps;
static int rangemapn, rangemapm;

static char *schemaname;
static int osm;
static char *ozm;
static DDSschema_t *schemap;
static DDSheader_t hdr;

static Sfoff_t filebase, filesize, currbase, currsize;
static Sfio_t *ofp;
static char *currp;
static long long maxrecn;

static void loadmap (char *);
static Sfio_t *openfile (char *, char *, long long);
static char *getrecord (Sfio_t *, long long);

int main (int argc, char **argv) {
    int norun;
    char *cmds;
    char *mapfile, *dir;
    Sfio_t *ifp;
    long long index, count;
    int iindex, ilen;
    void *indexp;
    rangemap_t *rangemapp;
    int rangemapi, rangemapj;
    int ret;
    char *recbuf;

    schemaname = NULL;
    schemap = NULL;
    osm = DDS_HDR_NAME | DDS_HDR_DESC;
    ozm = "none";
    mapfile = NULL;
    cmds = NULL;
    ifp = NULL;
    mapfile = NULL;
    dir = ".";
    ilen = 8;
    count = 0;
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
        case -318:
            mapfile = opt_info.arg;
            continue;
        case -319:
            dir = opt_info.arg;
            continue;
        case -330:
            ilen = opt_info.num;
            if (ilen != 4 && ilen != 8) {
                SUusage (0, "ddschoose", "index length must be 4 or 8");
                norun = 1;
            }
            continue;
        case -999:
            SUwarnlevel++;
            continue;
        case '?':
            SUusage (0, "ddschoose", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "ddschoose", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;
    if (SUcanseek (sfstdin))
        sfsetbuf (sfstdin, NULL, 1048576);
    if (!(ofp = SUsetupoutput (cmds, sfstdout, 1048576)))
        SUerror ("ddschoose", "setupoutput failed");
    if ((mapfile && argc != 0) || (!mapfile && argc != 1)) {
        SUusage (1, "ddschoose", opt_info.arg);
        return 1;
    }
    DDSinit ();
    if (mapfile)
        loadmap (mapfile);
    else if (!(ifp = openfile (dir, argv[0], -1)))
        goto done;
    rangemapj = -1;
    if (ilen == 8)
        indexp = &index;
    else if (ilen == 4)
        indexp = &iindex;
    while ((ret = sfread (sfstdin, indexp, ilen)) == ilen) {
        if (ilen == 8) {
            SW8(&index);
        } else {
            SW4(&iindex);
            index = iindex;
        }
        if (mapfile) {
            for (rangemapi = 0; rangemapi < rangemapm; rangemapi++) {
                rangemapp = &rangemaps[rangemapi];
                if (index >= rangemapp->start && index <= rangemapp->end)
                    break;
            }
            if (rangemapi == rangemapm)
                SUerror ("ddschoose", "cannot find index %d", index);
            index -= rangemapp->start;
            if (rangemapj != rangemapi) {
                rangemapj = rangemapi;
                if (ifp) {
                    sfclose (ifp);
                    SUwarning (1, "ddschoose", "processed %lld records", count);
                }
                if (!(ifp = openfile (
                    dir, rangemapp->fname, rangemapp->end - rangemapp->start + 1
                )))
                    SUerror (
                        "ddschoose", "openfile failed on %s", rangemapp->fname
                    );
            }
        }
        recbuf = getrecord (ifp, index);
        if (sfwrite (ofp, recbuf, schemap->recsize) != schemap->recsize)
            SUerror ("ddschoose", "incomplete write");
        count++;
    }
    SUwarning (1, "ddschoose", "processed %lld records", count);
    if (ret != 0)
        SUerror ("ddschoose", "incomplete read for index stream");
    if (sfsync (NULL) < 0)
        SUerror ("ddschoose", "sync failed");
    if (ofp != sfstdout)
        sfclose (ofp);

done:
    DDSterm ();
    return 0;
}

static void loadmap (char *fname) {
    Sfio_t *fp;
    rangemap_t *rangemapp;
    char *line, *s1, *s2;

    if (!(fp = sfopen (NULL, fname, "r")))
        SUerror ("ddschoose", "sfopen failed for %s", fname);
    rangemapm = 0;
    while ((line = sfgetr (fp, '\n', 1))) {
        if (rangemapm >= rangemapn) {
            if (!(rangemaps = vmresize (
                Vmheap, rangemaps,
                (1000 + rangemapn) * sizeof (rangemap_t), VM_RSCOPY
            )))
                SUerror ("ddschoose", "vmresize failed");
            rangemapn += 1000;
        }
        rangemapp = &rangemaps[rangemapm++];
        s1 = strchr (line, ' ');
        *s1 = 0, rangemapp->start = strtoll (line, &s2, 10), line = ++s1;
        s1 = strchr (line, ' ');
        *s1 = 0, rangemapp->end = strtoll (line, &s2, 10), line = ++s1;
        if (!(rangemapp->fname = vmstrdup (Vmheap, line)))
            SUerror ("ddschoose", "vmresize failed");
    }
    sfclose (fp);
}

static Sfio_t *openfile (char *dir, char *fname, long long nrec) {
    Sfio_t *fp;
    int ret;

    static int firsttime = TRUE;

    SUwarning (1, "ddschoose", "opening file %s", fname);
    if (fname[0] == '/') {
        if (!(fp = sfopen (NULL, fname, "r")))
            SUerror ("ddschoose", "sfopen failed for %s", fname);
    } else {
        if (!(fp = sfopen (NULL, sfprints ("%s/%s", dir, fname), "r")))
            SUerror ("ddschoose", "sfopen failed for %s", fname);
    }
    if (!(fp = SUsetupinput (fp, 128 * 1024)))
        SUerror ("ddschoose", "setupinput failed");
    if ((ret = DDSreadheader (fp, &hdr)) == -1)
        SUerror ("ddschoose", "DDSreadheader failed");
    if (ret == -2)
        return NULL;
    if ((filesize = sfsize (fp)) == -1)
        SUerror ("ddschoose", "sfsize failed for %s", fname);
    if (firsttime) {
        firsttime = FALSE;
        if (ret == 0) {
            if (hdr.schemap)
                schemap = hdr.schemap;
            else if (hdr.schemaname)
                schemaname = hdr.schemaname;
        }
        if (!schemap) {
            if (!schemaname)
                SUerror ("ddschoose", "no schema specified");
            if (!(schemap = DDSloadschema (schemaname, NULL, NULL)))
                SUerror ("ddschoose", "DDSloadschema failed");
        }
        if (!hdr.schemaname && schemaname)
            hdr.schemaname = schemaname;
        if (!hdr.schemap && schemap)
            hdr.schemap = schemap;
        hdr.contents = osm;
        hdr.vczspec = ozm;
        if (osm && DDSwriteheader (ofp, &hdr) == -1)
            SUerror ("ddschoose", "DDSwriteheader failed");
        if ((maxrecn = (16 * 1024) / schemap->recsize) < 1)
            maxrecn = 1;
    }
    if ((filebase = sfseek (fp, 0, SEEK_CUR)) == -1)
        SUerror ("ddschoose", "sfseek-get failed");
    filesize -= filebase;
    if (nrec != -1 && nrec * schemap->recsize != filesize)
        SUerror ("ddschoose", "incompatible size");
    currp = NULL;
    currbase = currsize = -1;
    return fp;
}

static char *getrecord (Sfio_t *fp, long long index) {
    Sfoff_t base, size;
    char *p;

    base = filebase + index * schemap->recsize;
    if (base >= filebase + filesize)
        SUerror ("ddschoose", "index out of range %lld", index);
    size = maxrecn * schemap->recsize;
    if (base + size >= filebase + filesize)
        size = filebase + filesize - base;
    if (base < currbase || base + schemap->recsize > currbase + currsize) {
        if ((currbase = sfseek (fp, base, SEEK_SET)) == -1)
            SUerror ("ddschoose", "sfseek-set failed");
        if (!(currp = sfreserve (fp, size, 0)) || sfvalue (fp) < size)
            SUerror ("ddschoose", "sfreserve failed");
        currsize = size;
        p = currp;
    } else
        p = currp + (base - currbase);
    return p;
}
