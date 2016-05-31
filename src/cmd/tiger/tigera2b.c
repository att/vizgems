#pragma prototyped

#include <ast.h>
#include <option.h>
#include <vmalloc.h>
#include <ftwalk.h>
#include <swift.h>
#include "tiger.h"

static const char usage[] =
"[-1p1s1D?\n@(#)$Id: tigera2b (AT&T Labs Research) 2001-09-24 $\n]"
USAGE_LICENSE
"[+NAME?tigera2b - convert raw data into the TIGER format]"
"[+DESCRIPTION?\btigera2b\b reads raw TIGER/Line files and converts them into"
" the binary SWIFT TIGER format."
"]"
"[100:i?specifies the directory containing the raw files."
" The default is the current directory."
"]:[inputdir]"
"[101:o?specifies the directory for the output files."
" The default is the current directory."
"]:[outputdir]"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"\n"
"[+SEE ALSO?\btiger\b(1)]"
;

#define GETFIELD(rec, start, len, var) ( \
    strncpy (var, &rec[start - 1], len), var[len] = 0 \
)

#define POPENFMT "gunzip < %s | pax -rvf - -s ',.*,.,'"

static xy_t *xys;
static int xyl, xyn;

static int ecount, pcount, e2pcount;

static int ftwalkufunc (struct FTW *);
static int ftwalkcfunc (struct FTW *, struct FTW *);

static int loadfile1 (char *);
static int loadfile2 (char *);
static int loadfileP (char *);
static int loadfileI (char *);

int main (int argc, char **argv) {
    int norun;
    char *startbrk, *endbrk;
    char *indir, *outdir;

    startbrk = (char *) sbrk (0);
    indir = ".";
    outdir = ".";
    norun = 0;
    for (;;) {
        switch (optget (argv, usage)) {
        case -100:
            indir = opt_info.arg;
            continue;
        case -101:
            outdir = opt_info.arg;
            continue;
        case -999:
            SUwarnlevel++;
            continue;
        case '?':
            SUusage (0, "tigera2b", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "tigera2b", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;

    initrecords ();
    if (!(xys = vmalloc (Vmheap, 10 * sizeof (xy_t))))
        SUerror ("tigera2b", "vmalloc failed for xys");
    xyn = 10;
    ftwalk (indir, ftwalkufunc, FTW_DOT | FTW_POST, ftwalkcfunc);
    SUmessage (
        1, "tigera2b", "read %d edges, %d polys, %d e2ps",
        ecount, pcount, e2pcount
    );
    mergeonesidededges ();
    savedata (outdir);
    printstats ();
    endbrk = (char *) sbrk (0);
    SUmessage (1, "tigera2b", "memory usage %d", endbrk - startbrk);
    return 0;
}

static int ftwalkufunc (struct FTW *ftwp) {
    char *prefix, *suffix, *s;

    if (ftwp->info != FTW_F)
        return 0;
    if ((prefix = strrchr (ftwp->path, '/')))
        prefix++;
    else
        prefix = ftwp->path;
    if ((s = strstr (ftwp->path, ".pax.gz")) && s > ftwp->path)
        suffix = s - 1;
    else
        suffix = &ftwp->path[strlen (ftwp->path) - 1];
    if (
        *prefix != 'T' || (*suffix != '1' && *suffix != '2' &&
        *suffix != 'P' && *suffix != 'I')
    )
        return  0;
    SUmessage (1, "ftwalkufunc", "loading file %s", ftwp->path);
    switch (*suffix) {
    case '1': loadfile1 (ftwp->path); break;
    case '2': loadfile2 (ftwp->path); break;
    case 'P': loadfileP (ftwp->path); break;
    case 'I': loadfileI (ftwp->path); break;
    }
    return 0;
}

static int ftwalkcfunc (struct FTW *ftw1p, struct FTW *ftw2p) {
    char *prefix1, *suffix1, *prefix2, *suffix2, *s;
    int type1, type2;

    if ((prefix1 = strrchr (ftw1p->name, '/')))
        prefix1++;
    else
        prefix1 = ftw1p->name;
    if ((s = strstr (ftw1p->name, ".pax.gz")) && s > ftw1p->name)
        suffix1 = s - 1;
    else
        suffix1 = &ftw1p->name[strlen (ftw1p->name) - 1];
    if ((prefix2 = strrchr (ftw2p->name, '/')))
        prefix2++;
    else
        prefix2 = ftw2p->name;
    if ((s = strstr (ftw2p->name, ".pax.gz")) && s > ftw2p->name)
        suffix2 = s - 1;
    else
        suffix2 = &ftw2p->name[strlen (ftw2p->name) - 1];
    if (*prefix1 == 'T') {
        switch (*suffix1) {
        case '1': type1 = 2; break;
        case '2': type1 = 3; break;
        case 'P': type1 = 4; break;
        case 'I': type1 = 5; break;
        default:  type1 = 1; break;
        }
    } else
        type1 = 0;
    if (*prefix2 == 'T') {
        switch (*suffix2) {
        case '1': type2 = 2; break;
        case '2': type2 = 3; break;
        case 'P': type2 = 4; break;
        case 'I': type2 = 5; break;
        default:  type2 = 1; break;
        }
    } else
        type2 = 0;
    if (type1 != type2)
        return type1 - type2;
    return strcmp (ftw1p->name, ftw2p->name);
}

int loadfile1 (char *fname) {
    Sfio_t *fp;
    edgedata_t ed;
    char *line, name[31], num[12], cfcc[4], blksl[2], blksr[2];

    if (strstr (fname, ".pax.gz")) {
        if (!(fp = sfpopen (NULL, sfprints (POPENFMT, fname), "r"))) {
            SUwarning (1, "loadfile1", "open failed for file %s (1)", fname);
            return -1;
        }
    } else {
        if (!(fp = sfopen (NULL, fname, "r"))) {
            SUwarning (1, "loadfile1", "open failed file %s (2)", fname);
            return -1;
        }
    }
    sfsetbuf (fp, NULL, 1048576);
    while ((line = sfgetr (fp, '\n', 1))) {
        GETFIELD (line, 6, 10, num), ed.tlid = atoi (num);
        ed.onesided = (line[15] == '1') ? 1 : 0;
        GETFIELD (line, 56, 3, cfcc);
        ed.cfccid = cfccstr2id (cfcc);
        GETFIELD (line, 107, 5, num), ed.zipl = atoi (num);
        GETFIELD (line, 112, 5, num), ed.zipr = atoi (num);
        ed.npanxxlocl = ed.npanxxlocr = -1;
        GETFIELD (line, 131, 2, num), ed.statel = atoi (num);
        GETFIELD (line, 133, 2, num), ed.stater = atoi (num);
        GETFIELD (line, 135, 3, num), ed.countyl = atoi (num);
        GETFIELD (line, 138, 3, num), ed.countyr = atoi (num);
        GETFIELD (line, 171, 6, num), ed.ctbnal = atoi (num);
        if (num[4] == ' ' && num[5] == ' ')
            ed.ctbnal *= 100;
        GETFIELD (line, 177, 6, num), ed.ctbnar = atoi (num);
        if (num[4] == ' ' && num[5] == ' ')
            ed.ctbnar *= 100;
        GETFIELD (line, 183, 3, num), ed.blkl = atoi (num);
        GETFIELD (line, 186, 1, blksl);
        ed.blksl = blksl[0];
        GETFIELD (line, 187, 3, num), ed.blkr = atoi (num);
        GETFIELD (line, 190, 1, blksr);
        ed.blksr = blksr[0];
        GETFIELD (line, 191, 10, num), ed.xy0.x = atoi (num);
        GETFIELD (line, 201, 9, num), ed.xy0.y = atoi (num);
        GETFIELD (line, 210, 10, num), ed.xy1.x = atoi (num);
        GETFIELD (line, 220, 9, num), ed.xy1.y = atoi (num);
        ecount++;
        GETFIELD (line, 20, 30, name);
        if (ed.ctbnal == 0) {
            ed.zipl = 0;
            ed.statel = 0;
            ed.countyl = 0;
            ed.blkl = 0;
            ed.blksl = 32;
        }
        if (ed.ctbnar == 0) {
            ed.zipr = 0;
            ed.stater = 0;
            ed.countyr = 0;
            ed.blkr = 0;
            ed.blksr = 32;
        }
        if (!createedge (&ed, name)) {
            SUwarning (1, "loadfile1", "createedge failed");
            return -1;
        }
    }
    sfclose (fp);
    return 0;
}

int loadfile2 (char *fname) {
    Sfio_t *fp;
    char *line, num[12];
    int tlid, seqn, xyi;
    edge_t *ep;

    if (strstr (fname, ".pax.gz")) {
        if (!(fp = sfpopen (NULL, sfprints (POPENFMT, fname), "r"))) {
            SUwarning (1, "loadfile2", "open failed for file %s (1)", fname);
            return -1;
        }
    } else {
        if (!(fp = sfopen (NULL, fname, "r"))) {
            SUwarning (1, "loadfile2", "open failed for file %s (2)", fname);
            return -1;
        }
    }
    xyl = 0;
    sfsetbuf (fp, NULL, 1048576);
    while ((line = sfgetr (fp, '\n', 1))) {
        GETFIELD (line, 6, 10, num), tlid = atoi (num);
        GETFIELD (line, 16, 3, num), seqn = atoi (num);
        if (seqn == 1) {
            if (xyl > 0) {
                if (attachshapexys (ep, xys, xyl) == -1) {
                    SUwarning (1, "loadfile2", "attachshapexys failed (1)");
                    return -1;
                }
                xyl = 0;
            }
            if (!(ep = findedge (tlid))) {
                SUwarning (1, "loadfile2", "findedge failed for %d", tlid);
                continue;
            }
        } else if (xyl + 10 > xyn) {
            if (!(xys = vmresize (
                Vmheap, xys, (xyn + 10) * sizeof (xy_t), VM_RSCOPY
            ))) {
                SUwarning (1, "loadfile2", "vmresize failed for xys");
                return -1;
            }
            xyn += 10;
        }
        for (xyi = 0; xyi < 10; xyi++) {
            GETFIELD (line, 19 + xyi * 19, 10, num), xys[xyl].x = atoi (num);
            GETFIELD (line, 29 + xyi * 19, 9, num), xys[xyl].y = atoi (num);
            if (xys[xyl].x == 0 && xys[xyl].y == 0)
                break;
            xyl++;
        }
    }
    if (xyl > 0) {
        if (attachshapexys (ep, xys, xyl) == -1) {
            SUwarning (1, "loadfile2", "attachshapexys failed (2)");
            return -1;
        }
    }
    sfclose (fp);
    return 0;
}

int loadfileP (char *fname) {
    Sfio_t *fp;
    polydata_t pd;
    char *line, num[12], cenid[6];

    if (strstr (fname, ".pax.gz")) {
        if (!(fp = sfpopen (NULL, sfprints (POPENFMT, fname), "r"))) {
            SUwarning (1, "loadfileP", "open failed for file %s (1)", fname);
            return -1;
        }
    } else {
        if (!(fp = sfopen (NULL, fname, "r"))) {
            SUwarning (1, "loadfileP", "open failed for file %s (2)", fname);
            return -1;
        }
    }
    sfsetbuf (fp, NULL, 1048576);
    while ((line = sfgetr (fp, '\n', 1))) {
        GETFIELD (line, 11, 5, cenid);
        pd.cenid = cenidstr2id (cenid);
        GETFIELD (line, 16, 10, num), pd.polyid = atoi (num);
        pcount++;
        if (!createpoly (&pd)) {
            SUwarning (1, "loadfileP", "createpoly failed");
            return -1;
        }
    }
    sfclose (fp);
    return 0;
}

int loadfileI (char *fname) {
    Sfio_t *fp;
    edge2polydata_t e2pd;
    char *line, num[12], cenidl[6], cenidr[6];

    if (strstr (fname, ".pax.gz")) {
        if (!(fp = sfpopen (NULL, sfprints (POPENFMT, fname), "r"))) {
            SUwarning (1, "loadfileI", "open failed for file %s (1)", fname);
            return -1;
        }
    } else {
        if (!(fp = sfopen (NULL, fname, "r"))) {
            SUwarning (1, "loadfileI", "open failed for file %s (2)", fname);
            return -1;
        }
    }
    sfsetbuf (fp, NULL, 1048576);
    while ((line = sfgetr (fp, '\n', 1))) {
        GETFIELD (line, 11, 10, num), e2pd.tlid = atoi (num);
        GETFIELD (line, 41, 5, cenidl);
        e2pd.cenidl = cenidstr2id (cenidl);
        GETFIELD (line, 46, 10, num), e2pd.polyidl = atoi (num);
        GETFIELD (line, 56, 5, cenidr);
        e2pd.cenidr = cenidstr2id (cenidr);
        GETFIELD (line, 61, 10, num), e2pd.polyidr = atoi (num);
        e2pcount++;
        if (linkedgenpolys (&e2pd) == -1) {
            SUwarning (1, "loadfileI", "linkedgenpolys failed");
            return -1;
        }
    }
    sfclose (fp);
    return 0;
}
