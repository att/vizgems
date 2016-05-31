#pragma prototyped

#include <ast.h>
#include <option.h>
#include <sys/resource.h>
#include <recsort.h>
#include <swift.h>
#include <dds.h>

#define STEPFPN 10

static const char usage[] =
"[-1p1s1D?\n@(#)$Id: ddssort (AT&T Labs Research) 2001-09-24 $\n]"
USAGE_LICENSE
"[+NAME?ddssort - sort records]"
"[+DESCRIPTION?\bddssort\b either reads records from a single DDS file"
" (or standard input) and sorts them by the combination of fields specified,"
" or it merge-sorts the data in the files specified (the \bm\b option)."
" In the merge-sort case if any of the files begins with \bicmd:\b,"
" it is assumed to be a shell command rather than a real file.]"
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
"[210:ke?specifies the names of fields to be used as the key for sorting."
"]:['field1 ...']"
"[212:kso?specifies the file containing the precompiled shared object for"
" sorting."
"]:[file]"
"[309:maxm?specifies the maximum memory the program is allowed to use before"
" it starts writing intermediate results to disk for a merge-sort phase at"
" the end."
" The default value is 8MB."
"]#[n]"
"[310:u?specifies that only one record per unique key will be generated."
" The default action is to output all records."
"]"
"[311:r?reverses the sorting order."
"]"
"[312:m?specifies that the input files are to be merge-sorted."
" The records in the input files must be already sorted, by the"
" same key specified in the current invocation."
"]"
"[400:ccflags?specifies extra flags for the compiler such as -I flags."
"]:[ccflags]"
"[401:ldflags?specifies extra flags for the compiler such as -L flags."
"]:[ldflags]"
"[402:fast?specifies to use a faster method if possible (e.g. for sorting)."
"]"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"\n"
"\n[ file1 ... ]\n"
"\n"
"[+EXAMPLES?Assuming the following 4 records:]{"
"[+]"
"[+f1 f2 f3\b]"
"[+--------\b]"
"[+20 31 35\b]"
"[+10 21 25\b]"
"[+10 11  5\b]"
"[+10 11 15\b]"
"}"
"[+?\bddssort -ke f1\b will output:]{"
"[+]"
"[+f1 f2 f3\b]"
"[+--------\b]"
"[+10 21 25\b]"
"[+10 11  5\b]"
"[+10 11 15\b]"
"[+20 31 35\b]"
"}"
"[+?while \bddssort -ke 'f1 f2'\b will output:]{"
"[+]"
"[+f1 f2 f3\b]"
"[+--------\b]"
"[+10 11  5\b]"
"[+10 11 15\b]"
"[+10 21 25\b]"
"[+20 31 35\b]"
"}"
"[+SEE ALSO?\bdds\b(1), \bdds\b(3), \baggr\b(1)]"
;

typedef struct sel_s {
    int ifpi, reci, nseli;
} sel_t;
static sel_t *sels;
static int seln, fseli;

static char **recs;
static int recn;

static int cmpoff, cmpsiz;

static int mergesel (void);

int main (int argc, char **argv) {
    int norun;
    int argi;
    char *schemaname;
    int osm;
    char *ozm;
    char *kexpr, *ksofile;
    int rsflags, fastflag;
    char *ccflags, *ldflags;
    DDSschema_t *schemap;
    int fieldi;
    DDSsorter_t *sorterp;
    DDSheader_t hdr;
    char buf[10000];
    struct rlimit rl;
    Sfio_t *ifps[1000];
    int ifpn;
    Sfio_t *ifp, *ofp;
    Sfio_t **tmpfps;
    int tmpfpn, tmpfpm, tmpfpi;
    int reci;
    int seli;
    int maxmem, mergemode;
    int ret, ok2write;

    schemaname = NULL;
    schemap = NULL;
    osm = DDS_HDR_NAME | DDS_HDR_DESC;
    ozm = "none";
    kexpr = NULL, ksofile = NULL;
    rsflags = 0, fastflag = FALSE;
    maxmem = 8 * 1024 * 1024;
    mergemode = FALSE;
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
        case -104:
            ozm = opt_info.arg;
            continue;
        case -210:
            kexpr = opt_info.arg;
            continue;
        case -212:
            ksofile = opt_info.arg;
            continue;
        case -309:
            maxmem = opt_info.num;
            continue;
        case -310:
            rsflags |= RS_UNIQ;
            continue;
        case -311:
            rsflags |= RS_REVERSE;
            continue;
        case -312:
            mergemode = TRUE;
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
            SUusage (0, "ddssort", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "ddssort", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;
    if (argc > 1 && !mergemode)
        SUerror ("ddssort", "too many parameters");
    DDSinit ();
    ifp = sfstdin;
    if (argc == 1 && !mergemode)
        if (!(ifp = sfopen (NULL, argv[0], "r")))
            SUerror ("ddssort", "sfopen failed for %s", argv[0]);
    if (!(ifp = SUsetupinput (ifp, 1048576)))
        SUerror ("ddssort", "setupinput failed");
    if (!(ofp = SUsetupoutput (NULL, sfstdout, 1048576)))
        SUerror ("ddssort", "setupoutput failed");
    if (getrlimit (RLIMIT_NOFILE, &rl) != -1) {
        rl.rlim_cur = rl.rlim_max;
        setrlimit (RLIMIT_NOFILE, &rl);
    }
    if (mergemode) {
        for (argi = 0; argi < argc; argi++) {
            if (strncmp (argv[argi], "icmd:", 5) == 0 && argv[argi][5]) {
                if (!(ifps[argi] = sfpopen (NULL, argv[argi] + 5, "r")))
                    SUerror ("ddssort", "sfpopen failed");
            } else {
                if (!(ifps[argi] = sfopen (NULL, argv[argi], "r")))
                    SUerror ("ddssort", "sfopen failed");
            }
            if (!(ifps[argi] = SUsetupinput (ifps[argi], 1048576)))
                SUerror ("ddssort", "setupinput failed");
            if ((ret = DDSreadheader (ifps[argi], &hdr)) == -1)
                SUerror ("ddssort", "DDSreadheader failed");
            if (ret == 0) {
                if (hdr.schemaname) {
                    if (!schemaname) {
                        if (!(schemaname = vmstrdup (Vmheap, hdr.schemaname)))
                            SUerror ("ddssort", "vmstrdup failed");
                    }
                }
            }
            if (!schemap) {
                if (hdr.schemap)
                    schemap = hdr.schemap;
                else if (
                    !schemaname ||
                    !(schemap = DDSloadschema (schemaname, NULL, NULL))
                )
                    SUerror ("ddssort", "DDSloadschema failed");
                hdr.schemaname = schemaname;
                hdr.schemap = schemap;
                hdr.contents = osm;
                hdr.vczspec = ozm;
                if (osm && DDSwriteheader (ofp, &hdr) == -1)
                    SUerror ("ddssort", "DDSwriteheader failed");
            }
        }
        ifpn = argi;
        if (argc < 1) {
            ifps[argi] = ifp;
            ifpn = 1;
            if ((ret = DDSreadheader (ifps[argi], &hdr)) == -1)
                SUerror ("ddssort", "DDSreadheader failed");
            if (ret == 0) {
                if (hdr.schemaname) {
                    if (!schemaname) {
                        if (!(schemaname = vmstrdup (Vmheap, hdr.schemaname)))
                            SUerror ("ddssort", "vmstrdup failed");
                    }
                }
            }
            if (!schemap) {
                if (hdr.schemap)
                    schemap = hdr.schemap;
                else if (
                    !schemaname ||
                    !(schemap = DDSloadschema (schemaname, NULL, NULL))
                )
                    SUerror ("ddssort", "DDSloadschema failed");
                hdr.schemaname = schemaname;
                hdr.schemap = schemap;
                hdr.contents = osm;
                hdr.vczspec = ozm;
                if (osm && DDSwriteheader (ofp, &hdr) == -1)
                    SUerror ("ddssort", "DDSwriteheader failed");
            }
        }
    } else {
        if ((ret = DDSreadheader (ifp, &hdr)) == -1)
            SUerror ("ddssort", "DDSreadheader failed");
        if (ret == -2)
            goto done;
        if (ret == 0) {
            if (hdr.schemap)
                schemap = hdr.schemap;
            if (hdr.schemaname)
                schemaname = hdr.schemaname;
        }
        if (!schemap) {
            if (!schemaname)
                SUerror ("ddssort", "no schema specified");
            if (!(schemap = DDSloadschema (schemaname, NULL, NULL)))
                SUerror ("ddssort", "DDSloadschema failed");
        }
        if (!hdr.schemaname && schemaname)
            hdr.schemaname = schemaname;
        if (!hdr.schemap && schemap)
            hdr.schemap = schemap;
        hdr.contents = osm;
        hdr.vczspec = ozm;
        if (osm && DDSwriteheader (ofp, &hdr) == -1)
            SUerror ("ddssort", "DDSwriteheader failed");
    }
    if (!kexpr && !ksofile) {
        for (fieldi = 0; fieldi < schemap->fieldn; fieldi++) {
            if (fieldi > 0)
                strcat (buf, " ");
            strcat (buf, schemap->fields[fieldi].name);
        }
        kexpr = buf;
    }
    if (!ksofile) {
        if (!(sorterp = DDScreatesorter (
            schemap, kexpr, rsflags, ccflags, ldflags, fastflag
        )))
            SUerror ("ddssort", "DDScreatesorter failed");
    } else {
        if (!(sorterp = DDSloadsorter (schemap, ksofile, rsflags)))
            SUerror ("ddssort", "DDSloadsorter failed");
    }
    if (sorterp->init && (*sorterp->init) () == -1)
        SUerror ("ddssort", "sorterp->init failed");
    if (mergemode) {
        if (sorterp->work == NULL) {
            recn = ifpn;
            if (!(recs = vmresize (
                Vmheap, NULL, recn * sizeof (char *), VM_RSZERO
            )))
                SUerror ("ddssort", "vmresize failed");
            for (reci = 0; reci < recn; reci++)
                if (!(recs[reci] = vmalloc (
                    Vmheap, schemap->recsize * sizeof (char)
                )))
                    SUerror ("ddssort", "cannot allocate record %d", reci);

            seln = ifpn;
            if (!(sels = vmresize (
                Vmheap, NULL, seln * sizeof (sel_t), VM_RSZERO
            )))
                SUerror ("ddssort", "vmresize failed");
            memset (sels, 0, seln * sizeof (sel_t));
            for (seli = 0; seli < seln; seli++) {
                sels[seli].ifpi = sels[seli].reci = seli;
                sels[seli].nseli = -1;
            }

            cmpoff = sorterp->rsdisc.key, cmpsiz = sorterp->rsdisc.keylen;
            fseli = -1;
            for (seli = seln - 1; seli >= 0; seli--) {
                if ((ret = DDSreadrawrecord (
                    ifps[sels[seli].ifpi], recs[sels[seli].reci], schemap
                )) != schemap->recsize) {
                    if (ret != 0)
                        SUwarning (0, "ddssort", "incomplete read");
                    continue;
                }
                sels[seli].nseli = fseli, fseli = seli;
                if (mergesel () == -1)
                    SUerror ("ddssort", "cannot merge record from %d", seli);
            }

            ok2write = 2;
            while (fseli >= 0) {
                if (rsflags & RS_UNIQ) {
                    if (ok2write == 2)
                        ok2write = 1;
                    else
                        ok2write = (memcmp (
                            schemap->recbuf + cmpoff,
                            recs[sels[fseli].reci] + cmpoff, cmpsiz
                        ) == 0) ? 0 : 1;
                    memcpy (
                        schemap->recbuf, recs[sels[fseli].reci],
                        schemap->recsize
                    );
                } else
                    ok2write = 1;
                if (ok2write == 1 && DDSwriterawrecord (
                    ofp, recs[sels[fseli].reci], schemap
                ) != schemap->recsize)
                    SUerror ("ddssort", "incomplete write");
                if ((ret = DDSreadrawrecord (
                    ifps[sels[fseli].ifpi], recs[sels[fseli].reci], schemap
                )) != schemap->recsize) {
                    if (ret != 0)
                        SUwarning (0, "ddssort", "incomplete read");
                    sels[fseli].reci = -1;
                }
                if (mergesel () == -1)
                    SUerror ("ddssort", "cannot merge record from %d", fseli);
            }
        } else {
            if (rsmerge (
                sorterp->rsp, ofp, ifps, argc, RS_ITEXT | RS_OTEXT
            ) == -1)
                SUerror ("ddssort", "rsmerge failed");
        }
    } else {
        tmpfpm = STEPFPN, tmpfpn = 0, tmpfps = NULL;
        if ((recn = maxmem / schemap->recsize) < 1)
            recn = 1;
        if (!(recs = vmresize (
            Vmheap, NULL, recn * sizeof (char *), VM_RSZERO
        )))
            SUerror ("ddssort", "vmresize failed");
        reci = 0;
        if (!(recs[reci] = vmalloc (Vmheap, schemap->recsize * sizeof (char))))
            SUerror ("ddssort", "vmalloc failed");
        while ((ret = sfread (
            ifp, recs[reci], schemap->recsize)
        ) == schemap->recsize) {
            if (rsprocess (
                sorterp->rsp, recs[reci], - schemap->recsize
            ) != schemap->recsize)
                SUerror ("ddssort", "rsprocess failed");
            if (++reci == recn) {
                if (!(tmpfps = vmresize (
                    Vmheap, tmpfps, (tmpfpn + 1) * sizeof (Sfio_t *),
                    VM_RSCOPY | VM_RSZERO
                )))
                    SUerror ("ddssort", "vmresize failed");
                if (!(tmpfps[tmpfpn] = sftmp (0)))
                    SUerror ("ddssort", "sftmp failed");
                if (rswrite (sorterp->rsp, tmpfps[tmpfpn], 0) == -1)
                    SUerror ("ddssort", "rswrite failed");
                rsclear (sorterp->rsp);
                tmpfpn++;
                SUwarning (
                    1, "ddssort", "wrote %d recs in tmp file #%d", reci, tmpfpn
                );
                if (tmpfpn > tmpfpm) {
                    tmpfpm += STEPFPN;
                    recn *= 2;
                    if (!(recs = vmresize (
                        Vmheap, recs, recn * sizeof (char *),
                        VM_RSCOPY | VM_RSZERO
                    )))
                        SUerror ("ddssort", "vmresize failed");
                }
                reci = 0;
            } else if (!recs[reci]) {
                if (!(recs[reci] = vmalloc (
                    Vmheap, schemap->recsize * sizeof (char)
                )))
                    SUerror ("ddssort", "vmalloc failed");
            }
        }
        if (ret != 0)
            SUerror ("ddssort", "incomplete read");
        if (tmpfpn > 0) {
            if (!(tmpfps = vmresize (
                Vmheap, tmpfps, (tmpfpn + 1) * sizeof (Sfio_t *),
                VM_RSCOPY | VM_RSZERO
            )))
                SUerror ("ddssort", "vmresize failed");
            if (!(tmpfps[tmpfpn] = sftmp (0)))
                SUerror ("ddssort", "sftmp failed");
            if (rswrite (sorterp->rsp, tmpfps[tmpfpn], 0) == -1)
                SUerror ("ddssort", "rswrite failed");
            rsclear (sorterp->rsp);
            tmpfpn++;
            SUwarning (
                1, "ddssort", "wrote %d recs in tmp file #%d", reci, tmpfpn
            );
            for (tmpfpi = 0; tmpfpi < tmpfpn; tmpfpi++)
                if (sfseek (tmpfps[tmpfpi], 0, SEEK_SET) != 0)
                    SUerror ("ddssort", "sfseek failed");
            if (rsmerge (
                sorterp->rsp, ofp, tmpfps, tmpfpn, RS_OTEXT
            ) == -1)
                SUerror ("ddssort", "rsmerge failed");
            for (tmpfpi = 0; tmpfpi < tmpfpn; tmpfpi++)
                sfclose (tmpfps[tmpfpi]);
        } else
            if (reci > 0 && rswrite (sorterp->rsp, ofp, RS_OTEXT) == -1)
                SUerror ("ddssort", "rswrite failed");
    }
    if (sorterp->term && (*sorterp->term) () == -1)
        SUerror ("ddssort", "sorterp->term failed");
    if (sfsync (NULL) < 0)
        SUerror ("ddssort", "sync failed");
    DDSdestroysorter (sorterp);

done:
    DDSterm ();
    return 0;
}

static int mergesel (void) {
    int nfseli, seli;
    int freci, nreci;

    if (sels[fseli].reci == -1) {
        fseli = sels[fseli].nseli;
        return 0;
    }

    freci = sels[fseli].reci;
    for (seli = fseli; sels[seli].nseli != -1; seli = sels[seli].nseli) {
        nreci = sels[sels[seli].nseli].reci;
        if (memcmp (recs[freci] + cmpoff, recs[nreci] + cmpoff, cmpsiz) <= 0)
            break;
    }
    if (seli == fseli)
        return 0;

    nfseli = sels[fseli].nseli;
    sels[fseli].nseli = sels[seli].nseli;
    sels[seli].nseli = fseli;
    fseli = nfseli;
    return 0;
}
