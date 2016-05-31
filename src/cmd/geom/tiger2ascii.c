#pragma prototyped

#include <ast.h>
#include <option.h>
#include <tok.h>
#include <cdt.h>
#include <math.h>
#include <swift.h>
#include "tiger.h"

static const char usage[] =
"[-1p1s1D?\n@(#)$Id: tiger2ascii (AT&T Labs Research) 2001-09-24 $\n]"
USAGE_LICENSE
"[+NAME?tiger2ascii - generate ascii format geometry]"
"[+DESCRIPTION?\btiger2ascii\b generates ascii SWIFT geometry files from the"
" specified dataset."
" It can generate up to four types of geometry: points, vertical lines,"
" polygon outlines, and filled polygons."
"]"
"[100:i?specifies the directory containing the input files."
" The default is the current directory."
"]:[inputdir]"
"[101:o?specifies the directory for the output files."
" The default is the current directory."
"]:[outputdir]"
"[200:sm?specifies the splice mode for vertices of degree two."
" When a vertex only has 2 edges connected to it, the two edges can be merged"
" into one and the old vertex becomes a shape vertex in the new edge."
" This reduces the number of edges and speeds up several algorithms."
" A value of \b0\b disables splicing."
" A value of \b1\b (default) excludes boundary edges."
" A value of \b2\b operates on all edges."
"]#[(0|1|2)]"
"[201:p?specifies the accuracy level for edge thining."
" This reduces the number of shape vertices in each edge."
" Shape vertex that are \aclose\a to the straight line between the edge"
" endpoints are eliminated."
" The argument to this option specifies the accuracy level when determining"
" closeness."
" This is a floating point number with the range: \b0-9\b."
" Higher numbers result in more shape edges qualifing for elimination."
" A value of \b-1\b disables this processing."
" The default value is \b6\b."
"]:[level]"
"[300:a?specifies the attribute to use as the geometry item keys."
" The default is \bstate\b."
"]:[(state|county|ctbna|blkg|blk|blks|zip|npanxxloc)]"
"[302:n?specifies the prefix of the geometry files."
" The default is \btiger\b."
"]:[outputprefix]"
"[306:bb?specifies that bounding box information should be printed out."
"]"
"[600:np?disables the generation of the points geometry."
"]"
"[601:nl?disables the generation of the vertical lines geometry."
"]"
"[602:nP?disables the generation of the polygon outlines geometry."
"]"
"[603:nt?disables the generation of the filled polygons geometry."
"]"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"\n"
"[+SEE ALSO?\btiger\b(1)]"
;

#ifndef MAXFLOAT
#ifdef HUGE
#define MAXFLOAT HUGE
#else
#define MAXFLOAT 3.40282347e+38F
#endif
#endif

#define ISCCW 1
#define ISCW  2
#define ISON  3

#define INDINCR    1
#define POINTINCR 10
#define TRIINCR   10

typedef struct point_t {
    float x, y;
} point_t;

typedef struct tri_t {
    point_t a, b, c;
} tri_t;

typedef struct item_t {
    Dtlink_t link;
    /* begin key */
    char *name;
    /* end key */
    point_t *points;
    int pointm, pointn;
    int *inds;
    int indm, indn;
    tri_t *tris;
    int trim, trin;
} item_t;

static Dtdisc_t itemdisc = {
    sizeof (Dtlink_t), -1, 0, NULL, NULL, NULL, NULL, NULL, NULL
};

static Dt_t *itemdict;
static item_t **itemps;
static int itempn;

static point_t minp, maxp;

static int savemask;

static int genpoints (int);
static int gentris (void);
static int savepoints (char *, char *);
static int savelines (char *, char *);
static int savepolys (char *, char *);
static int savetris (char *, char *);

static int triangulate (item_t *, int, int);
static int rectriangulate (item_t *itemp, point_t *, int);
static int ccw (point_t, point_t, point_t);
static int isdiagonal (int, int, point_t *, int);
static int intersects (point_t, point_t, point_t, point_t);
static int between (point_t, point_t, point_t);

static int *getind (item_t *, int);
static point_t *getpoint (item_t *, xy_t);
static tri_t *gettri (item_t *, point_t, point_t, point_t);

static int calcbbox (void);
static int printbbox (void);

int main (int argc, char **argv) {
    int norun;
    char *startbrk, *endbrk;
    char *indir, *outdir, *outprefix;
    int attr;
    int splicemode;
    float digits;
    int bboxflag;

    startbrk = (char *) sbrk (0);
    indir = ".";
    outdir = ".";
    outprefix = "tiger";
    attr = T_EATTR_STATE;
    splicemode = 1;
    digits = 6;
    bboxflag = 0;
    savemask = 15;
    norun = 0;
    for (;;) {
        switch (optget (argv, usage)) {
        case -100:
            indir = opt_info.arg;
            continue;
        case -101:
            outdir = opt_info.arg;
            continue;
        case -200:
            splicemode = opt_info.num;
            continue;
        case -201:
            if ((digits = atof (opt_info.arg)) < 0)
                digits = -1;
            continue;
        case -300:
            if (strcmp (opt_info.arg, "zip") == 0)
                attr = T_EATTR_ZIP;
            else if (strcmp (opt_info.arg, "npanxxloc") == 0)
                attr = T_EATTR_NPANXXLOC;
            else if (strcmp (opt_info.arg, "state") == 0)
                attr = T_EATTR_STATE;
            else if (strcmp (opt_info.arg, "county") == 0)
                attr = T_EATTR_COUNTY;
            else if (strcmp (opt_info.arg, "ctbna") == 0)
                attr = T_EATTR_CTBNA;
            else if (strcmp (opt_info.arg, "blkg") == 0)
                attr = T_EATTR_BLKG;
            else if (strcmp (opt_info.arg, "blk") == 0)
                attr = T_EATTR_BLK;
            else if (strcmp (opt_info.arg, "blks") == 0)
                attr = T_EATTR_BLKS;
            else if (strcmp (opt_info.arg, "country") == 0)
                attr = T_EATTR_COUNTRY;
            else
                SUerror ("tiger2ascii", "bad argument for -a option");
            continue;
        case -302:
            outprefix = opt_info.arg;
            continue;
        case -306:
            bboxflag = 1;
            continue;
        case -600:
            savemask &= 14;
            continue;
        case -601:
            savemask &= 13;
            continue;
        case -602:
            savemask &= 11;
            continue;
        case -603:
            savemask &= 7;
            continue;
        case -999:
            SUwarnlevel++;
            continue;
        case '?':
            SUusage (0, "tiger2ascii", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "tiger2ascii", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;

    initrecords ();
    if (!(itemdict = dtopen (&itemdisc, Dtset)))
        SUerror ("tiger2ascii", "dtopen failed");
    if (loaddata (indir) == -1)
        SUerror ("tiger2ascii", "loaddata failed");
    if (splicemode > 0 && removedegree2vertices (splicemode) == -1)
        SUerror ("tiger2ascii", "removedegree2vertices");
    if (digits >= 0 && simplifyxys (digits) == -1)
        SUerror ("tiger2ascii", "simplifyxys failed");
    if (genpoints (attr) == -1)
        SUerror ("tiger2ascii", "genpoints failed");
    calcbbox ();
    if (bboxflag)
        printbbox ();
    if ((savemask & 1) && savepoints (outdir, outprefix) == -1)
        SUerror (
            "tiger2ascii", "savepoints failed for %s/%s", outdir, outprefix
        );
    if ((savemask & 2) && savelines (outdir, outprefix) == -1)
        SUerror (
            "tiger2ascii", "savelines failed for %s/%s", outdir, outprefix
        );
    if ((savemask & 4) && savepolys (outdir, outprefix) == -1)
        SUerror (
            "tiger2ascii", "savepolys failed for %s/%s", outdir, outprefix);
    if ((savemask & 8) && gentris () == -1)
        SUerror ("tiger2ascii", "gentris failed");
    if ((savemask & 8) && savetris (outdir, outprefix) == -1)
        SUerror ("tiger2ascii", "savetris failed for %s/%s", outdir, outprefix);
    printstats ();
    endbrk = (char *) sbrk (0);
    SUmessage (1, "tiger2ascii", "memory usage %d", endbrk - startbrk);
    return 0;
}

static int genpoints (int attr) {
    poly_t *pp;
    int zip, npanxxloc, ctbna;
    short county, blk;
    char state, blks;
    char *s;
    item_t *itemmem, *itemp;
    int itemi;
    int *indp;
    edge_t *e1p, *e2p;
    int edgepi;
    vertex_t *v1p, *v2p, *v3p;
    int xyi;

    for (
        pp = (poly_t *) dtflatten (polydict); pp;
        pp = (poly_t *) dtlink (polydict, pp)
    ) {
        if (pp->edgepl == 0)
            continue;
        e1p = pp->edgeps[0];
        if (e1p->p0p == pp) {
            blks = e1p->blksl;
            blk = e1p->blkl;
            ctbna = e1p->ctbnal;
            county = e1p->countyl;
            state = e1p->statel;
            zip = e1p->zipl;
            npanxxloc = e1p->npanxxlocl;
        } else {
            blks = e1p->blksr;
            blk = e1p->blkr;
            ctbna = e1p->ctbnar;
            county = e1p->countyr;
            state = e1p->stater;
            zip = e1p->zipr;
            npanxxloc = e1p->npanxxlocr;
        }
        s = NULL;
        switch (attr) {
        case T_EATTR_BLKS:
            if ((state > 0) && (county > 0) && (ctbna > 0) && (blk > 0))
                s = sfprints (
                    "%02d%03d%06d%03d%c", state, county, ctbna, blk, blks
                );
            break;
        case T_EATTR_BLK:
            if ((state > 0) && (county > 0) && (ctbna > 0) && (blk > 0))
                s = sfprints ("%02d%03d%06d%03d", state, county, ctbna, blk);
            break;
        case T_EATTR_BLKG:
            if ((state > 0) && (county > 0) && (ctbna > 0) && (blk > 0))
                s = sfprints (
                    "%02d%03d%06d%d", state, county, ctbna, blk / 100
                );
            break;
        case T_EATTR_CTBNA:
            if ((state > 0) && (county > 0) && (ctbna > 0))
                s = sfprints ("%02d%03d%06d", state, county, ctbna);
            break;
        case T_EATTR_COUNTY:
            if ((state > 0) && (county > 0))
                s = sfprints ("%02d%03d", state, county);
            break;
        case T_EATTR_STATE:
            if ((state > 0))
                s = sfprints ("%02d", state);
            break;
        case T_EATTR_ZIP:
            if ((zip > 0))
                s = sfprints ("%05d", zip);
            break;
        case T_EATTR_NPANXXLOC:
            if ((npanxxloc > -1))
                s = sfprints ("%d", npanxxloc);
            break;
        case T_EATTR_COUNTRY:
            s = "USA";
            break;
        }
        if (!s)
            continue;
        if (!(itemmem = malloc (sizeof (item_t)))) {
            SUwarning (1, "genpoints", "malloc failed for itemmem");
            return -1;
        }
        itemmem->name = strdup (s);
        if (!(itemp = dtinsert (itemdict, itemmem)) || itemp != itemmem) {
            SUwarning (1, "genpoints", "dtinsert failed for itemp");
            return -1;
        }
        itemp->pointn = 0, itemp->pointm = POINTINCR;
        if (!(itemp->points = malloc (sizeof (point_t) * itemp->pointm))) {
            SUwarning (1, "genpoints", "malloc failed for points");
            return -1;
        }
        itemp->indn = 0, itemp->indm = INDINCR;
        if (!(itemp->inds = malloc (sizeof (int) * itemp->indm))) {
            SUwarning (1, "genpoints", "malloc failed for inds");
            return -1;
        }
        itemp->trin = 0, itemp->trim = TRIINCR;
        if (!(itemp->tris = malloc (sizeof (tri_t) * itemp->trim))) {
            SUwarning (1, "genpoints", "malloc failed for tris");
            return -1;
        }
        orderedges (pp);
        for (v1p = NULL, edgepi = 0; edgepi < pp->edgepl; edgepi++) {
            e1p = pp->edgeps[edgepi];
            if (!v1p) {
                if (!(indp = getind (itemp, 0))) {
                    SUwarning (1, "genpoints", "getind failed");
                    return -1;
                }
                if (e1p->p0p == pp)
                    v1p = e1p->v0p, v2p = e1p->v1p;
                else
                    v1p = e1p->v1p, v2p = e1p->v0p;
                v3p = v1p;
            }
            (*indp)++;
            if (!getpoint (itemp, v1p->xy)) {
                SUwarning (1, "genpoints", "getpoint failed (1)");
                return -1;
            }
            if (v1p == e1p->v0p) {
                for (xyi = 0; xyi < e1p->xyn; xyi++) {
                    (*indp)++;
                    if (!getpoint (itemp, e1p->xys[xyi])) {
                        SUwarning (1, "genpoints", "getpoint failed (2)");
                        return -1;
                    }
                }
            } else {
                for (xyi = e1p->xyn - 1; xyi >= 0; xyi--) {
                    (*indp)++;
                    if (!getpoint (itemp, e1p->xys[xyi])) {
                        SUwarning (1, "genpoints", "getpoint failed (3)");
                        return -1;
                    }
                }
            }
            v1p = NULL;
            if (edgepi + 1 < pp->edgepl) {
                e2p = pp->edgeps[edgepi + 1];
                if (e2p->v0p == v2p)
                    v1p = e2p->v0p, v2p = e2p->v1p;
                else if (e2p->v1p == v2p)
                    v1p = e2p->v1p, v2p = e2p->v0p;
            }
            if (!v1p) {
                (*indp)++;
                if (!getpoint (itemp, v2p->xy)) {
                    SUwarning (1, "genpoints", "getpoint failed (4)");
                    return -1;
                }
                if (savemask & 8) {
                    if (v2p->xy.x != v3p->xy.x || v2p->xy.y != v3p->xy.y) {
                        (*indp)++;
                        if (!getpoint (itemp, v3p->xy)) {
                            SUwarning (1, "genpoints", "getpoint failed (5)");
                            return -1;
                        }
                    }
                }
            }
        }
    }
    itempn = dtsize (itemdict);
    if (!(itemps = malloc (sizeof (item_t *) * itempn))) {
        SUwarning (1, "genpoints", "malloc failed for itemps");
        return -1;
    }
    for (
        itemi = 0, itemp = (item_t *) dtflatten (itemdict); itemp;
        itemp = (item_t *) dtlink (itemdict, itemp)
    )
        itemps[itemi++] = itemp;
    return 0;
}

static int gentris (void) {
    item_t *itemp;
    int itempi;
    int indi, pointi, msgstep;

    if ((msgstep = itempn / 10) == 0)
        msgstep = 1;
    for (itempi = 0; itempi < itempn; itempi++) {
        itemp = itemps[itempi];
        if (itempi % msgstep == 0)
            SUmessage (
                1, "gentris", "triangulating item %d/%d", itempi, itempn
            );
        for (pointi = 0, indi = 0; indi < itemp->indn; indi++) {
            triangulate (itemp, pointi, itemp->inds[indi]);
            pointi += itemp->inds[indi];
        }
    }
    return 0;
}

static int savepoints (char *dir, char *prefix) {
    char *fname;
    Sfio_t *fp;
    item_t *itemp;
    int itempi;
    int pointi;
    double x, y;

    fname = strdup (sfprints ("%s/%s_points.txt", dir, prefix));
    SUmessage (1, "savepoints", "saving file %s", fname);
    if (!(fp = sfopen (NULL, fname, "w"))) {
        SUwarning (1, "savepoints", "open failed for %s", fname);
        return -1;
    }
    /* (x, y, z) of each point */
    for (itempi = 0; itempi < itempn; itempi++) {
        itemp = itemps[itempi];
        if (itemp->pointn == 0)
            continue;
        sfprintf (fp, "\"%s\" \"", itemp->name);
        for (x = y = 0.0, pointi = 0; pointi < itemp->pointn; pointi++)
            x += itemp->points[pointi].x, y += itemp->points[pointi].y;
        x /= itemp->pointn, y /= itemp->pointn;
        sfprintf (fp, "%f %f", x, y);
        sfprintf (fp, "\" \"\"\n");
    }
    sfclose (fp);
    return 0;
}

static int savelines (char *dir, char *prefix) {
    char *fname;
    Sfio_t *fp;
    item_t *itemp;
    int itempi;
    int pointi;
    double x, y;

    fname = strdup (sfprints ("%s/%s_lines.txt", dir, prefix));
    SUmessage (1, "savepoints", "saving file %s", fname);
    if (!(fp = sfopen (NULL, fname, "w"))) {
        SUwarning (1, "savepoints", "open failed for %s", fname);
        return -1;
    }
    /* (x, y, z) of each point */
    for (itempi = 0; itempi < itempn; itempi++) {
        itemp = itemps[itempi];
        if (itemp->pointn == 0)
            continue;
        sfprintf (fp, "\"%s\" \"", itemp->name);
        for (x = y = 0.0, pointi = 0; pointi < itemp->pointn; pointi++)
            x += itemp->points[pointi].x, y += itemp->points[pointi].y;
        x /= itemp->pointn, y /= itemp->pointn;
        sfprintf (fp, "%f %f %f %f", x, y, x, y);
        sfprintf (fp, "\" \"\"\n");
    }
    sfclose (fp);
    return 0;
}

static int savepolys (char *dir, char *prefix) {
    char *fname;
    Sfio_t *fp;
    item_t *itemp;
    int itempi;
    int indi;
    int pointi, pointj;

    fname = strdup (sfprints ("%s/%s_polys.txt", dir, prefix));
    SUmessage (1, "savepolys", "saving file %s", fname);
    if (!(fp = sfopen (NULL, fname, "w"))) {
        SUwarning (1, "savepolys", "open failed for %s", fname);
        return -1;
    }
    /* (x, y, z) of each point */
    for (itempi = 0; itempi < itempn; itempi++) {
        itemp = itemps[itempi];
        if (itemp->pointn == 0)
            continue;
        sfprintf (fp, "\"%s\" \"", itemp->name);
        for (pointi = 0, indi = 0; indi < itemp->indn; indi++) {
            if (indi > 0)
                sfprintf (fp, " | | ");
            sfprintf (
                fp, "%f %f", itemp->points[pointi].x,
                itemp->points[pointi].y
            );
            pointi++;
            for (pointj = 1; pointj < itemp->inds[indi]; pointi++, pointj++)
                sfprintf (
                    fp, " %f %f", itemp->points[pointi].x,
                    itemp->points[pointi].y
                );
        }
        sfprintf (fp, "\" \"\"\n");
    }
    sfclose (fp);
    return 0;
}

static int savetris (char *dir, char *prefix) {
    char *fname;
    Sfio_t *fp;
    item_t *itemp;
    int itempi;
    int trii;

    fname = strdup (sfprints ("%s/%s_tris.txt", dir, prefix));
    SUmessage (1, "savetris", "saving file %s", fname);
    if (!(fp = sfopen (NULL, fname, "w"))) {
        SUwarning (1, "savetris", "open failed for %s", fname);
        return -1;
    }
    /* (x, y, z) of each point */
    for (itempi = 0; itempi < itempn; itempi++) {
        itemp = itemps[itempi];
        if (itemp->trin == 0)
            continue;
        sfprintf (fp, "\"%s\" \"", itemp->name);
        for (trii = 0; trii < itemp->trin; trii++) {
            if (trii > 0)
                sfprintf (fp, " | | ");
            sfprintf (
                fp, "%f %f %f %f %f %f",
                itemp->tris[trii].a.x, itemp->tris[trii].a.y,
                itemp->tris[trii].b.x, itemp->tris[trii].b.y,
                itemp->tris[trii].c.x, itemp->tris[trii].c.y
            );
        }
        sfprintf (fp, "\" \"\"\n");
    }
    sfclose (fp);
    return 0;
}

/* triangulation functions */

static int triangulate (item_t *itemp, int pointf, int pointn) {
    point_t *points;
    int pointi, pointj, mini;
    double minx;
    point_t p1, p2, p3;

    points = &itemp->points[pointf];
    /* make sure polygon is CCW */
    for (pointi = 0, pointj = 0; pointj < pointn; pointi++) {
        points[pointi] = points[pointj++];
        while (pointj < pointn && ccw (
            points[pointi], points[pointj], points[(pointj + 1) % pointn]
        ) == ISON)
            pointj++;
    }
    pointn = pointi;
    for (pointi = 0, minx = MAXFLOAT, mini = -1; pointi < pointn; pointi++)
        if (minx > points[pointi].x)
            minx = points[pointi].x, mini = pointi;
    p2 = points[mini];
    p1 = points[((mini == 0) ? pointn - 1: mini - 1)];
    p3 = points[((mini == pointn - 1) ? 0 : mini + 1)];
    if (
        ((p1.x == p2.x && p2.x == p3.x) && (p3.y > p2.y)) ||
        ccw (p1, p2, p3) != ISCCW
    ) {
        for (pointi = pointn - 1; pointi >= 0; pointi--) {
            if ((pointj = pointn - pointi - 1) >= pointi)
                break;
            p1 = points[pointi];
            points[pointi] = points[pointj];
            points[pointj] = p1;
        }
    }
    rectriangulate (itemp, points, pointn);
    return 0;
}

static int rectriangulate (item_t *itemp, point_t *pp, int pn) {
    int i, ip1, ip2;
    int ccwres;

again:
    if (pn > 3) {
        for (i = 0; i < pn; i++) {
            ip1 = (i + 1) % pn;
            ip2 = (i + 2) % pn;
            if ((ccwres = ccw (pp[i], pp[ip1], pp[ip2])) == ISCW)
                continue;
            if (ccwres == ISON) {
                for (i = ip1; i < pn - 1; i++)
                    pp[i] = pp[i + 1];
                pn--;
                goto again;
            } else if (isdiagonal (i, ip2, pp, pn)) {
                if (!gettri (itemp, pp[i], pp[ip1], pp[ip2])) {
                    SUwarning (1, "rectriangulate", "gettri failed (1)");
                    return -1;
                }
                for (i = ip1; i < pn - 1; i++)
                    pp[i] = pp[i + 1];
                rectriangulate (itemp, pp, pn - 1);
                return 0;
            }
        }
        SUwarning (0, "rectriangulate", "failed for %d points", pn);
        for (i = 0; i < pn; i++)
            SUwarning (0, "rectriangulate", "%d %f %f", i, pp[i].x, pp[i].y);
    } else if (pn == 3) {
        if (!gettri (itemp, pp[0], pp[1], pp[2]) == -1) {
            SUwarning (1, "rectriangulate", "gettri failed (2)");
            return -1;
        }
    }
    return 0;
}

static int ccw (point_t p1, point_t p2, point_t p3) {
    double d;

    d = ((p1.y - p2.y) * (p3.x - p2.x)) - ((p3.y - p2.y) * (p1.x - p2.x));
    return (d > 0) ? ISCCW : ((d < 0) ? ISCW : ISON);
}

/* check if (i, i + 2) is a diagonal */
static int isdiagonal (int i, int ip2, point_t *pp, int pn) {
    int ip1, im1, j, jp1;

    /* neighborhood test */
    ip1 = (i + 1) % pn;
    im1 = (i + pn - 1) % pn;
    if (ccw (pp[im1], pp[i], pp[ip1]) == ISCCW)
        if (ccw (pp[i], pp[ip2], pp[im1]) != ISCCW)
            return FALSE;

    /* check against all other edges */
    for (j = 0; j < pn; j++) {
        jp1 = (j + 1) % pn;
        if (!((j == i) || (jp1 == i) || (j == ip2) || (jp1 == ip2)))
            if (intersects (pp[i], pp[ip2], pp[j], pp[jp1]))
                return FALSE;
    }
    return TRUE;
}

/* line to line intersection */
static int intersects (point_t pa, point_t pb, point_t pc, point_t pd) {
    int ccw1, ccw2, ccw3, ccw4;

    if (
        ccw (pa, pb, pc) == ISON || ccw (pa, pb, pd) == ISON ||
        ccw (pc, pd, pa) == ISON || ccw (pc, pd, pb) == ISON
    ) {
        if (
            between (pa, pb, pc) || between (pa, pb, pd) ||
            between (pc, pd, pa) || between (pc, pd, pb)
        )
            return TRUE;
    } else {
        ccw1 = (ccw (pa, pb, pc) == ISCCW) ? 1 : 0;
        ccw2 = (ccw (pa, pb, pd) == ISCCW) ? 1 : 0;
        ccw3 = (ccw (pc, pd, pa) == ISCCW) ? 1 : 0;
        ccw4 = (ccw (pc, pd, pb) == ISCCW) ? 1 : 0;
        return (ccw1 ^ ccw2) && (ccw3 ^ ccw4);
    }
    return FALSE;
}

static int between (point_t pa, point_t pb, point_t pc) {
    point_t pba, pca;

    pba.x = pb.x - pa.x, pba.y = pb.y - pa.y;
    pca.x = pc.x - pa.x, pca.y = pc.y - pa.y;
    if (ccw (pa, pb, pc) != ISON)
        return FALSE;
    return (
        pca.x * pba.x + pca.y * pba.y >= 0) &&
        (pca.x * pca.x + pca.y * pca.y <= pba.x * pba.x + pba.y * pba.y
    );
}

/* allocation functions */

static int *getind (item_t *itemp, int pointn) {
    int *indp;

    if (itemp->indn == itemp->indm) {
        if (!(itemp->inds = realloc (
            itemp->inds, sizeof (int) * (itemp->indm + INDINCR)
        ))) {
            SUwarning (1, "getind", "realloc failed");
            return NULL;
        }
        itemp->indm += INDINCR;
    }
    indp = &itemp->inds[itemp->indn++];
    *indp = pointn;
    return indp;
}

static point_t *getpoint (item_t *itemp, xy_t xy) {
    point_t *pointp;

    if (itemp->pointn == itemp->pointm) {
        if (!(itemp->points = realloc (
            itemp->points, sizeof (point_t) * (itemp->pointm + POINTINCR)
        ))) {
            SUwarning (1, "getpoint", "realloc failed");
            return NULL;
        }
        itemp->pointm += POINTINCR;
    }
    pointp = &itemp->points[itemp->pointn++];
    pointp->x = xy.x / 1000000.0, pointp->y = xy.y / 1000000.0;
    return pointp;
}

static tri_t *gettri (item_t *itemp, point_t a, point_t b, point_t c) {
    tri_t *trip;

    if (itemp->trin == itemp->trim) {
        if (!(itemp->tris = realloc (
            itemp->tris, sizeof (tri_t) * (itemp->trim + TRIINCR)
        ))) {
            SUwarning (1, "gettri", "realloc failed");
            return NULL;
        }
        itemp->trim += TRIINCR;
    }
    trip = &itemp->tris[itemp->trin++];
    trip->a = a, trip->b = b, trip->c = c;
    return trip;
}

static int calcbbox (void) {
    point_t p;
    item_t *itemp;
    int itempi;
    int pointi;

    minp.x = minp.y = 999999;
    maxp.x = maxp.y = -999999;
    for (itempi = 0; itempi < itempn; itempi++) {
        itemp = itemps[itempi];
        if (itemp->pointn == 0)
            continue;
        for (pointi = 0; pointi < itemp->pointn; pointi++) {
            p = itemp->points[pointi];
            if (minp.x > p.x)
                minp.x = p.x;
            if (minp.y > p.y)
                minp.y = p.y;
            if (maxp.x < p.x)
                maxp.x = p.x;
            if (maxp.y < p.y)
                maxp.y = p.y;
        }
    }
    return 0;
}

static int printbbox (void) {
    sfprintf (
        sfstdout, "bbox: %f %f 0 %f %f 0\n", minp.x, minp.y, maxp.x, maxp.y
    );
    return 0;
}
