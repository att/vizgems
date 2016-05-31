#pragma prototyped

#include <ast.h>
#include <option.h>
#include <regex.h>
#include <dirent.h>
#include <vmalloc.h>
#include <cdt.h>
#include <sys/resource.h>
#include <swift.h>
#include <dds.h>

static const char usage[] =
"[-1p1s1D?\n@(#)$Id: ddscat (AT&T Labs Research) 2001-09-24 $\n]"
USAGE_LICENSE
"[+NAME?ddscat - concatenate DDS files]"
"[+DESCRIPTION?\bddscat\b concatenates multiple DDS data files."
" All the input files must have the same schema."
" A regular expression can be specified instead of the list of files."
" The order in which records from the files are copied is not specified."
" It does not depend on the order of the file names on the command line."
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
"[320:fp?specifies that \ain addition\a to any files specified on the command"
" line, \bddscat\b will scan for files with names matching this shell-style"
" regular expression."
"]:['regular expression']"
"[321:d?specifies the directory that all files names are relative to."
" The default directory is the current working directory."
"]:[directory]"
"[322:c?specifies the maximum number of records to copy from each file per"
" iteration."
" The default is to copy all available records in a single iteration."
"]#[n]"
"[323:t?specifies that \bddscat\b should wait for so many seconds before it"
" exits."
" During this time and if the \bfp\b option is used, \bddscat\b keeps checking"
" for new files of the specified pattern."
" Also, if the \bf\b option is used, \bddscat\b will monitor the size of"
" existing files and if a file grows in size, \bddscat\b will output the new"
" set of records from that file."
" When more than \bseconds\b seconds have elapsed without any activity,"
" \bddscat\b will exit."
"]#[seconds]"
"[324:f?specifies that \bddscat\b should run in tail mode, monitoring the size"
" of all open files."
" If a file grows in size, \bddscat\b will output the new set of records from"
" that file."
" \bddscat\b will do this indefinitely, unless the \bt\b flag is set, in which"
" case \bddscat\b will exit after that many seconds of inactivity."
"]"
"[325:sf?specifies that \bddscat\b should maintain a state file, containing"
" information on all the files it has already processed."
" If this state file exists and is not corrupted, \bddscat\b will only copy"
" new files, or the new section of files that grew."
" Before processing any files, \bddscat\b will truncate the state file."
" On exit, \bddscat\b will update this file."
" This procedure helps detect if a \bddscat\b invocation crashes halfway"
" through."
" If the state file is corrupted, \bddscat\b will exit without doing any work."
"]:[statefile]"
"[326:checksf?specifies that \bddscat\b should check the specifieda state file,"
" for consistency."
" If this state file exists and is corrupted, \bddscat\b will exit with code 2."
" Otherwise \bddscat\b will exit with code 0."
"]:[statefile]"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"\n"
"\n[ file1 ... ]\n"
"\n"
"[+EXAMPLES?Copy two files, a.dds and b.dds, to standard output:]{"
"[+]"
"[+ddscat a.dds b.dds\b]"
"}"
"[+?Copy all files that match a pattern:]{"
"[+]"
"[+ddscat -fp '[A-Z]][A-Z]]*'\b]"
"}"
"[+SEE ALSO?\bdds\b(1), \bdds\b(3), \baggr\b(1)]"
;

#ifdef FEATURE_NO64FS
typedef off_t off64_t;
#define stat64 stat
#endif

typedef struct ddsfile_t {
    Dtlink_t link;
    /* begin key */
    char *name;
    /* end key */
    Sfio_t *fp;
    struct stat64 sb;
    off64_t off;
    int vczmode;
} ddsfile_t;

static Dtdisc_t ddsfiledisc = {
    sizeof (Dtlink_t), -1, 0, NULL, NULL, NULL, NULL, NULL, NULL
};

static Dt_t *ddsfiledict;

static char *schemaname;
static DDSschema_t *schemap;
static int osm;
static char *ozm;
static regex_t prog;
static char *dir;
static Sfio_t *ofp;
static char *fpat;
static int tailmode, fcount, sleptfor, timeout;
static long long maxnrec, count;

static int loop (void);
static int scan (char *);
static int checkstate (char *);
static int loadstate (char *);
static int savestate (char *);
static int addfile (char *, int, off64_t);
static int getstat (ddsfile_t *);

int main (int argc, char **argv) {
    int norun;
    int argi;
    char *cmds, *statefile, *checkstatefile;
    struct rlimit rl;
    ddsfile_t *dfp;
    DDSheader_t ohdr;

    schemaname = NULL;
    osm = DDS_HDR_NAME | DDS_HDR_DESC;
    ozm = "none";
    cmds = NULL;
    fpat = NULL;
    dir = ".";
    timeout = 0;
    tailmode = 0;
    statefile = NULL;
    checkstatefile = NULL;
    schemap = NULL;
    maxnrec = -1;
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
        case -320:
            fpat = opt_info.arg;
            continue;
        case -321:
            dir = opt_info.arg;
            continue;
        case -322:
            maxnrec = opt_info.num;
            continue;
        case -323:
            timeout = opt_info.num;
            continue;
        case -324:
            tailmode = 1;
            continue;
        case -325:
            statefile = opt_info.arg;
            continue;
        case -326:
            checkstatefile = opt_info.arg;
            continue;
        case -999:
            SUwarnlevel++;
            continue;
        case '?':
            SUusage (0, "ddscat", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "ddscat", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;
    if (checkstatefile) {
        if (checkstate (checkstatefile) == -1) {
            SUwarning (0, "ddscat", "state file corrupted");
            return 2;
        }
        return 0;
    }
    if (argc == 0 && !fpat)
        SUwarning (1, "ddscat", "no files specified");
    DDSinit ();
    if (!(ofp = SUsetupoutput (cmds, sfstdout, 4194304)))
        SUerror ("ddscat", "setupoutput failed");
    if (getrlimit (RLIMIT_NOFILE, &rl) != -1) {
        rl.rlim_cur = rl.rlim_max;
        setrlimit (RLIMIT_NOFILE, &rl);
    }
    if (schemaname) {
        if (!(schemap = DDSloadschema (schemaname, NULL, NULL)))
            SUerror ("ddscat", "DDSloadschema failed");
        ohdr.schemaname = schemaname;
        ohdr.schemap = schemap;
        ohdr.contents = osm;
        ohdr.vczspec = ozm;
        if (osm && DDSwriteheader (ofp, &ohdr) == -1)
            SUerror ("ddscat", "DDSwriteheader failed");
    }
    if (!(ddsfiledict = dtopen (&ddsfiledisc, Dtset)))
        SUerror ("ddscat", "dtopen failed");
    if (statefile && loadstate (statefile) == -1)
        SUerror ("ddscat", "loadstate failed");
    sleptfor = 0;
    count = 0;
    for (argi = 0; argi < argc; argi++) {
        if (
            argv[argi][0] == '/' ||
            (argv[argi][0] == '.' && argv[argi][1] == '/') ||
            (
                argv[argi][0] == '.' && argv[argi][1] == '.' &&
                argv[argi][2] == '/'
            )
        ) {
            if (addfile (argv[argi], FALSE, -1) == -1)
                SUerror ("ddscat", "addfile failed (1)");
        } else {
            if (addfile (sfprints ("%s/%s", dir, argv[argi]), FALSE, -1) == -1)
                SUerror ("ddscat", "addfile failed (2)");
        }
        while (loop () == 0)
            ;
    }
    if (fpat) {
        if (regcomp (
            &prog, fpat,
            REG_NULL | REG_LENIENT | REG_AUGMENTED |
            REG_NOSUB | REG_SHELL | REG_LEFT | REG_RIGHT
        ) != 0)
            SUerror ("ddscat", "regcomp failed");
        fcount = 0;
        if (scan (dir) == -1)
            SUerror ("ddscat", "scan failed (1)");
    }
    while (loop () == 0)
        ;
    if (sfsync (NULL) < 0)
        SUerror ("ddscat", "sync failed");
    if (ofp != sfstdout)
        sfclose (ofp);
    for (dfp = dtfirst (ddsfiledict); dfp; dfp = dtnext (ddsfiledict, dfp))
        if (dfp->fp)
            sfclose (dfp->fp);
    if (statefile && savestate (statefile) == -1)
        SUerror ("ddscat", "savestate failed");
    SUwarning (1, "ddscat", "processed %lld records", count);
    DDSterm ();
    return 0;
}

/* returns -1 on error, 0 on success, 1 when there's no more to do */
static int loop (void) {
    ddsfile_t *dfp;
    long long nrec;
    int progress;

    fcount = 0;
    progress = 0;
    for (dfp = dtfirst (ddsfiledict); dfp; dfp = dtnext (ddsfiledict, dfp)) {
        if (!dfp->fp)
            continue;
        if ((nrec = (dfp->sb.st_size - dfp->off) / schemap->recsize) > 0) {
            if (
                sfmove (dfp->fp, ofp, (off_t) nrec * schemap->recsize, -1) !=
                (off_t) nrec * schemap->recsize
            )
                SUerror ("ddscat", "sfmove failed");
            dfp->off += ((off_t) nrec * schemap->recsize);
            progress = 1, sleptfor = 0;
            fcount++;
            SUwarning (
                3, "ddscat", "processed %lld records (%s)", nrec, dfp->name
            );
            count += nrec;
            SUwarning (1, "ddscat", "processed %lld records", count);
        } else if (!tailmode) {
            SUwarning (2, "ddscat", "finished file %s", dfp->name);
            sfclose (dfp->fp), dfp->fp = NULL;
        }
    }
    if (fcount < 1 && fpat) {
        fcount = 0;
        if (scan (dir) == -1)
            SUerror ("ddscat", "scan failed (2)");
        if (fcount > 0)
            progress = 1;
    }
    if (progress)
        return 0;
    if (timeout == 0)
        return 1;
    sleep (5), sleptfor += 5;
    if (timeout > 0 && sleptfor > timeout)
        return 1;
    for (dfp = dtfirst (ddsfiledict); dfp; dfp = dtnext (ddsfiledict, dfp)) {
        if (!dfp->fp)
            continue;
        if (getstat (dfp) == -1)
            SUerror ("ddscat", "getstat failed");
        if (dfp->vczmode) {
            if ((dfp->sb.st_size = sfseek (dfp->fp, 0, SEEK_END)) == -1)
                SUerror ("ddscat", "sfseek failed");
            if (sfseek (dfp->fp, dfp->off, SEEK_SET) != dfp->off)
                SUerror ("ddscat", "sfseek failed");
        }
    }
    return 0;
}

static int scan (char *dir) {
    DIR *dp;
    struct dirent *dep;
    int rtn;

    if (!(dp = opendir (dir))) {
        SUwarning (0, "ddscat", "opendir failed");
        return -1;
    }
    rtn = 0;
    while ((dep = readdir (dp))) {
        if (fcount > 100) {
            rtn = 1;
            break;
        }
        if (regexec (
            &prog, dep->d_name, 0, NULL,
            REG_NULL | REG_LENIENT | REG_AUGMENTED | REG_NOSUB |
            REG_SHELL | REG_LEFT | REG_RIGHT
        ) != 0)
            continue;
        if (dtmatch (ddsfiledict, dep->d_name))
            continue;
        if (dep->d_name[0] == '.' && (dep->d_name[1] == 0 || (
            dep->d_name[1] == '.' && dep->d_name[2] == 0
        )))
            continue;
        if ((rtn = addfile (
            sfprints ("%s/%s", dir, dep->d_name), FALSE, -1
        )) == -1) {
            SUwarning (1, "scan", "addfile failed for %s", dep->d_name);
            rtn = 0;
            continue;
        } else if (rtn == 0)
            fcount++;
    }
    closedir (dp);
    return rtn;
}

static int checkstate (char *file) {
    Sfio_t *fp;
    int state;
    char *line1, *line2;

    if (!(fp = sfopen (NULL, file, "r")))
        return 1;
    state = 0;
    while ((line1 = sfgetr (fp, '\n', 1))) {
        if (strcmp (line1, "BEGIN") == 0 && state == 0) {
            state = 1;
            continue;
        }
        if (strcmp (line1, "END") == 0 && state == 1) {
            state = 2;
            continue;
        }
        if (state != 1)
            break;
        if (
            !(line2 = sfgetr (fp, '\n', 1)) ||
            strtoll ((*line2 == 'Z') ? line2 + 1 : line2, NULL, 10) < 0
        ) {
            SUwarning (1, "checkstate", "read failed for %s", file);
            return -1;
        }
    }
    sfclose (fp);
    if (state != 2) {
        SUwarning (1, "checkstate", "incomplete state file");
        return -1;
    }
    return 0;
}

static int loadstate (char *file) {
    Sfio_t *fp;
    int state;
    char *line1, *line2;
    char fname[PATH_MAX];
    off64_t off;

    if (!(fp = sfopen (NULL, file, "r")))
        return 1;
    state = 0;
    while ((line1 = sfgetr (fp, '\n', 1))) {
        if (strcmp (line1, "BEGIN") == 0 && state == 0) {
            state = 1;
            continue;
        }
        if (strcmp (line1, "END") == 0 && state == 1) {
            state = 2;
            continue;
        }
        if (state != 1)
            break;
        strcpy (fname, line1);
        if (
            !(line2 = sfgetr (fp, '\n', 1)) ||
            (off = strtoll ((*line2 == 'Z') ? line2 + 1 : line2, NULL, 10)) < 0
        ) {
            SUwarning (1, "loadstate", "read failed for %s", file);
            return -1;
        }
        if (addfile (fname, (*line2 == 'Z') ? TRUE : FALSE, off) == -1)
            SUwarning (1, "loadstate", "addfile failed for %s", fname);
    }
    sfclose (fp);
    if (state != 2) {
        SUwarning (1, "loadstate", "incomplete state file");
        return -1;
    }
    if (!(fp = sfopen (NULL, file, "w")) || sfclose (fp) == -1) {
        SUwarning (1, "loadstate", "truncate failed for %s", file);
        return -1;
    }
    return 0;
}

static int savestate (char *file) {
    Sfio_t *fp;
    ddsfile_t *dfp;

    if (!(fp = sfopen (NULL, file, "w"))) {
        SUwarning (1, "savestate", "open failed for %s", file);
        return -1;
    }
    if (sfputr (fp, "BEGIN", '\n') == -1) {
        SUwarning (1, "savestate", "sfputr failed for %s (1)", file);
        return -1;
    }
    for (dfp = dtfirst (ddsfiledict); dfp; dfp = dtnext (ddsfiledict, dfp)) {
        if (dfp->fp)
            continue;
        if (sfputr (fp, dfp->name, '\n') == -1) {
            SUwarning (1, "savestate", "sfputr failed for %s (2)", file);
            return -1;
        }
        if (sfprintf (
            fp, "%s%lld\n", (dfp->vczmode) ? "Z" : "", dfp->off
        ) == -1) {
            SUwarning (1, "savestate", "sfprintf failed for %s", file);
            return -1;
        }
    }
    if (sfputr (fp, "END", '\n') == -1) {
        SUwarning (1, "savestate", "sfputr failed for %s (3)", file);
        return -1;
    }
    if (sfclose (fp) == -1) {
        SUwarning (1, "savestate", "close failed for %s", file);
        return -1;
    }
    return 0;
}

static int addfile (char *name, int vczmode, off64_t off) {
    ddsfile_t *dfmemp, *dfp;
    DDSheader_t ihdr;
    int ret;

    dfp = NULL;
    if (dtmatch (ddsfiledict, name))
        return 1;
    if (
        !(dfmemp = vmalloc (Vmheap, sizeof (ddsfile_t))) ||
        !(dfmemp->name = vmstrdup (Vmheap, name))
    )
        goto abortaddfile;
    if (!(dfp = dtinsert (ddsfiledict, dfmemp)) || dfp != dfmemp)
        goto abortaddfile;
    dfp->fp = NULL;
    if (getstat (dfp) == -1)
        goto abortaddfile;
    if (vczmode) {
        if (!(dfp->fp = sfopen (NULL, dfmemp->name, "r")))
            goto abortaddfile;
        if ((dfp->sb.st_size = sfseek (dfp->fp, 0, SEEK_END)) == -1)
            goto abortaddfile;
        if (sfseek (dfp->fp, 0, SEEK_SET) != 0)
            goto abortaddfile;
    }
    dfp->vczmode = vczmode;
    if (schemap && off != -1 && !tailmode && off == dfp->sb.st_size) {
        if (dfp->fp)
            sfclose (dfp->fp);
        dfp->off = off, dfp->fp = NULL;
        return 2;
    }
    if (!dfp->fp && !(dfp->fp = sfopen (NULL, dfmemp->name, "r")))
        goto abortaddfile;
    if (!(dfp->fp = SUsetupinput (dfp->fp, 1048576)))
        goto abortaddfile;
    if ((ret = DDSreadheader (dfp->fp, &ihdr)) == -1)
        goto abortaddfile;
    if (ret == 0) {
        if (ihdr.schemaname) {
            if (!schemaname) {
                if (!(schemaname = vmstrdup (Vmheap, ihdr.schemaname)))
                    goto abortaddfile;
#if 0
            } else if (strcmp (schemaname, ihdr.schemaname) != 0)
                goto abortaddfile;
#endif
            }
        }
        if ((dfp->off = sfseek (dfp->fp, 0, SEEK_CUR)) == -1)
            goto abortaddfile;
        if (*ihdr.vczspec) {
            dfp->vczmode = TRUE;
            if ((dfp->sb.st_size = sfseek (dfp->fp, 0, SEEK_END)) == -1)
                goto abortaddfile;
            if (sfseek (dfp->fp, dfp->off, SEEK_SET) != dfp->off)
                goto abortaddfile;
        }
    } else
        dfp->off = 0;
    if (!schemap) {
        if (ihdr.schemap)
            schemap = ihdr.schemap;
        else if (
            !schemaname || !(schemap = DDSloadschema (schemaname, NULL, NULL))
        )
            goto abortaddfile;
        ihdr.schemaname = schemaname;
        ihdr.schemap = schemap;
        ihdr.contents = osm;
        ihdr.vczspec = ozm;
        if (osm && DDSwriteheader (ofp, &ihdr) == -1)
            SUerror ("ddscat", "DDSwriteheader failed");
    } else
        DDSdestroyschema (ihdr.schemap);
    if (off != -1) {
        if ((dfp->off = sfseek (dfp->fp, off, SEEK_SET)) == -1)
            goto abortaddfile;
        if (!tailmode && dfp->off == dfp->sb.st_size) {
            sfclose (dfp->fp), dfp->fp = NULL;
            return 2;
        }
    }
    if (maxnrec == -1)
        maxnrec = 1048576 / schemap->recsize;
    SUwarning (2, "addfile", "added file %s", dfmemp->name);
    return 0;

abortaddfile:
    SUwarning (1, "addfile", "failed to add file %s", name);
    if (dfp && dfp->fp)
        sfclose (dfp->fp);
    if (dfp != dfmemp)
        dtdelete (ddsfiledict, dfp);
    if (dfmemp) {
        if (dfp && dfp->fp)
            sfclose (dfp->fp);
        if (dfp == dfmemp)
            dtdelete (ddsfiledict, dfp);
        if (dfmemp->name)
            vmfree (Vmheap, dfmemp->name);
        vmfree (Vmheap, dfmemp);
    }
    return -1;
}

static int getstat (ddsfile_t *dfp) {
    if (stat64 (dfp->name, &dfp->sb) == -1) {
        SUwarning (1, "getstat", "stat failed for %s", dfp->name);
        return -1;
    }
    return 0;
}
