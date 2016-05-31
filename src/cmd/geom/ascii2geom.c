#pragma prototyped

#include <ast.h>
#include <option.h>
#include <tok.h>
#include <cdt.h>
#include <math.h>
#include <swift.h>
#include "geom.h"

static const char usage[] =
"[-1p1s1D?\n@(#)$Id: ascii2geom (AT&T Labs Research) 2001-09-24 $\n]"
USAGE_LICENSE
"[+NAME?ascii2geom - generate SWIFT geometry]"
"[+DESCRIPTION?\bascii2geom\b generates SWIFT geometry files from an ascii"
" input file."
" It can generate up to four types of geometry: points, vertical lines,"
" polygon outlines, and filled polygons."
"]"
"[100:i?specifies the input file containing the coordinates."
" The default is \bascii.coords\b."
"]:[inputfile]"
"[101:o?specifies the prefix for the output files."
" The default is \bascii\b."
"]:[outputprefix]"
"[201:p?specifies the accuracy level for edge thining."
" This reduces the number of shape vertices in each edge."
" Shape vertex that are \aclose\a to the straight line between the edge"
" endpoints are eliminated."
" The argument to this option specifies the accuracy level when determining"
" closeness."
" This is a floating point number with the range: \b0-9\b."
" Higher numbers result in more shape edges qualifing for elimination."
" A value of \b-1\b (default) disables this processing."
"]:[level]"
"[303:cl?specifies the class name of the geometry files."
" The default is \bascii\b."
"]:[classname]"
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

#define WRITEINT(fp, a) do { \
    int i; i = a; sfwrite (fp, &i, sizeof (int)); \
} while (0)
#define WRITEXYZ(fp, a, b, c) do { \
    float xyz[3]; \
    xyz[0] = a, xyz[1] = b, xyz[2] = c; \
    sfwrite (fp, xyz, 3 * sizeof (float)); \
} while (0)

#define ISCCW 1
#define ISCW  2
#define ISON  3

#define POINTINCR 100
#define INDINCR     1
#define FLAGINCR  100
#define TRIINCR    10
#define LABELINCR  10

typedef struct point_t {
    float x, y, tx, ty;
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
    int *flags;
    int flagm, flagn;
    int *inds;
    int indm, indn;
    tri_t *tris;
    int trim, trin;
    GEOMlabel_t *labels;
    int labelm, labeln;
    int tpointn;
} item_t;

static Dtdisc_t itemdisc = {
    sizeof (Dtlink_t), -1, 0, NULL, NULL, NULL, NULL, NULL, NULL
};

static Dt_t *itemdict;
static item_t **itemps;
static int itempn;

static double *ts;
static int tn;

static int loadcoord (char *);
static int simplifypoints (float digits);
static void recsimplify (item_t *, int, int, int, double *, double, int *);
static int gentris (void);
static int savepoints (char *, char *);
static int savelines (char *, char *);
static int savepolys (char *, char *);
static int savetris (char *, char *);

static int maxdist (point_t *, int, int, double *, double);
static double dist (point_t, point_t);
static int triangulate (item_t *, int, int);
static int rectriangulate (item_t *itemp, point_t *, int);
static int ccw (point_t, point_t, point_t);
static int isdiagonal (int, int, point_t *, int);
static int intersects (point_t, point_t, point_t, point_t);
static int between (point_t, point_t, point_t);

static tri_t *gettri (item_t *, point_t, point_t, point_t);

static int printbbox (void);

int main (int argc, char **argv) {
    int norun;
    char *incoord, *outprefix, *classp;
    int bboxflag;
    int savemask;
    float digits;

    incoord = "ascii.coords";
    outprefix = "ascii";
    classp = "ascii";
    bboxflag = 0;
    savemask = 15;
    digits = -1;
    norun = 0;
    for (;;) {
        switch (optget (argv, usage)) {
        case -100:
            incoord = opt_info.arg;
            continue;
        case -101:
            outprefix = opt_info.arg;
            continue;
        case -201:
            if ((digits = atof (opt_info.arg)) < 0)
                digits = -1;
            continue;
        case -303:
            classp = opt_info.arg;
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
            SUusage (0, "ascii2geom", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "ascii2geom", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;

    if (!(itemdict = dtopen (&itemdisc, Dtset)))
        SUerror ("ascii2geom", "dtopen failed for itemdict");
    if (loadcoord (incoord) == -1)
        SUerror ("ascii2geom", "loadcoord failed for file %s", incoord);
    if (digits >= 0 && simplifypoints (digits) == -1)
        SUerror ("ascii2geom", "simplifypoints failed");
    if (bboxflag)
        printbbox ();
    if ((savemask & 1) && savepoints (outprefix, classp) == -1)
        SUerror ("ascii2geom", "save failed for points");
    if ((savemask & 2) && savelines (outprefix, classp) == -1)
        SUerror ("ascii2geom", "save failed for lines");
    if ((savemask & 4) && savepolys (outprefix, classp) == -1)
        SUerror ("ascii2geom", "save failed for polys");
    if ((savemask & 8) && gentris () == -1)
        SUerror ("ascii2geom", "gentris failed");
    if ((savemask & 8) && savetris (outprefix, classp) == -1)
        SUerror ("ascii2geom", "save failed for tris");
    return 0;
}

static int loadcoord (char *fname) {
    Sfio_t *fp;
    char *line, *s, *tp, *xstr, *ystr, *flagstr, *labelstr;
    char *oxstr, *oystr, *cxstr, *cystr, *avs[5];
    item_t *itemmem, *itemp;
    int itemi, l;

    SUmessage (1, "loadcoord", "loading coord file %s", fname);
    if (!(fp = sfopen (NULL, fname, "r"))) {
        SUwarning (1, "loadcoord", "open failed for file %s", fname);
        return -1;
    }
    while ((line = sfgetr (fp, '\n', 1))) {
        avs[2] = avs[3] = avs[4] = "";
        if (tokscan (
            line, &s, "%s %s %s %s %s\n",
            &avs[0], &avs[1], &avs[2], &avs[3], &avs[4]
        ) < 2) {
            SUwarning (1, "loadcoord", "bad line %s", line);
            continue;
        }
        if (!(itemmem = malloc (sizeof (item_t)))) {
            SUwarning (1, "loadcoord", "malloc failed for itemmem");
            return -1;
        }
        itemmem->name = strdup (avs[0]);
        if (!(itemp = dtinsert (itemdict, itemmem)) || itemp != itemmem) {
            SUwarning (1, "loadcoord", "dtinsert failed for itemp");
            return -1;
        }
        itemp->pointn = itemp->pointm = 0, itemp->points = NULL;
        itemp->flagn = itemp->flagm = 0, itemp->flags = NULL;
        itemp->indn = itemp->indm = 0, itemp->inds = NULL;
        itemp->trin = itemp->trim = 0, itemp->tris = NULL;
        itemp->labeln = itemp->labelm = 0, itemp->labels = NULL;
        itemp->tpointn = 0;
        if (!(tp = tokopen (avs[1], 1))) {
            SUwarning (1, "loadcoord", "tokopen failed (1)");
            return -1;
        }
        while ((xstr = tokread (tp))) {
            xstr = strdup (xstr);
            if (!(ystr = tokread (tp))) {
                SUwarning (1, "loadcoord", "missing y value: %s", avs[1]);
                return -1;
            }
            if (strcmp (xstr, "|") == 0 && strcmp (ystr, "|") == 0) {
                itemp->indn++;
                continue;
            }
            if (itemp->pointn >= itemp->pointm) {
                if (!itemp->points)
                    itemp->points = malloc (sizeof (point_t) * POINTINCR);
                else
                    itemp->points = realloc (
                        itemp->points,
                        sizeof (point_t) * (itemp->pointm + POINTINCR)
                    );
                if (!itemp->points) {
                    SUwarning (1, "loadcoord", "alloc failed for points");
                    return -1;
                }
                itemp->pointm += POINTINCR;
            }
            itemp->points[itemp->pointn].x = atof (xstr);
            itemp->points[itemp->pointn].y = atof (ystr);
            itemp->pointn++;
            if (itemp->indn == 0)
                itemp->indn = 1;
            if (itemp->indn > itemp->indm) {
                if (!itemp->inds)
                    itemp->inds = malloc (sizeof (int) * INDINCR);
                else
                    itemp->inds = realloc (
                        itemp->inds,
                        sizeof (int) * (itemp->indm + INDINCR)
                    );
                if (!itemp->inds) {
                    SUwarning (1, "loadcoord", "alloc failed for inds");
                    return -1;
                }
                itemp->indm += INDINCR;
                itemp->inds[itemp->indn - 1] = 0;
            }
            itemp->inds[itemp->indn - 1]++;
            free (xstr);
        }
        tokclose (tp);
        if (!(tp = tokopen (avs[2], 1))) {
            SUwarning (1, "loadcoord", "tokopen failed (2)");
            return -1;
        }
        while ((flagstr = tokread (tp))) {
            if (strcmp (flagstr, "|") == 0) {
                itemp->indn++;
                continue;
            }
            if (itemp->flagn >= itemp->flagm) {
                if (!itemp->flags)
                    itemp->flags = malloc (sizeof (int) * FLAGINCR);
                else
                    itemp->flags = realloc (
                        itemp->flags,
                        sizeof (int) * (itemp->flagm + FLAGINCR)
                    );
                if (!itemp->flags) {
                    SUwarning (1, "loadcoord", "alloc failed for flags");
                    return -1;
                }
                itemp->flagm += FLAGINCR;
            }
            itemp->flags[itemp->flagn] = atof (flagstr);
            itemp->flagn++;
        }
        tokclose (tp);
        if (itemp->flagn > 0 && itemp->flagn != itemp->pointn) {
            SUwarning (1, "loadcoord", "number of flags must match points");
            return -1;
        }
        if (!(tp = tokopen (avs[3], 1))) {
            SUwarning (1, "loadcoord", "tokopen failed (3)");
            return -1;
        }
        while ((labelstr = tokread (tp))) {
            if (itemp->labeln >= itemp->labelm) {
                if (!itemp->labels)
                    itemp->labels = malloc (sizeof (GEOMlabel_t) * LABELINCR);
                else
                    itemp->labels = realloc (
                        itemp->labels,
                        sizeof (GEOMlabel_t) * (itemp->labelm + LABELINCR)
                    );
                if (!itemp->labels) {
                    SUwarning (1, "loadcoord", "alloc failed for labels");
                    return -1;
                }
                itemp->labelm += LABELINCR;
            }
            l = strlen (labelstr);
            if (
                (labelstr[0] == '\'' && labelstr[l - 1] == '\'') ||
                (labelstr[0] == '"' && labelstr[l - 1] == '"')
            )
                labelstr[l - 1] = 0, labelstr++;
            itemp->labels[itemp->labeln].label = strdup (labelstr);
            if (!(oxstr = tokread (tp))) {
                SUwarning (1, "loadcoord", "missing ox value: %s", avs[3]);
                return -1;
            }
            itemp->labels[itemp->labeln].o[0] = atof (oxstr);
            if (!(oystr = tokread (tp))) {
                SUwarning (1, "loadcoord", "missing oy value: %s", avs[3]);
                return -1;
            }
            itemp->labels[itemp->labeln].o[1] = atof (oystr);
            itemp->labels[itemp->labeln].o[2] = 0.0;
            if (!(cxstr = tokread (tp))) {
                SUwarning (1, "loadcoord", "missing cx value: %s", avs[3]);
                return -1;
            }
            itemp->labels[itemp->labeln].c[0] = atof (cxstr);
            if (!(cystr = tokread (tp))) {
                SUwarning (1, "loadcoord", "missing coy value: %s", avs[3]);
                return -1;
            }
            itemp->labels[itemp->labeln].c[1] = atof (cystr);
            itemp->labels[itemp->labeln].c[2] = 0.0;
            itemp->labeln++;
        }
        tokclose (tp);
        if (!(tp = tokopen (avs[4], 1))) {
            SUwarning (1, "loadcoord", "tokopen failed (4)");
            return -1;
        }
        while ((xstr = tokread (tp))) {
            xstr = strdup (xstr);
            if (!(ystr = tokread (tp))) {
                SUwarning (1, "loadcoord", "missing y value: %s", avs[4]);
                return -1;
            }
            if (strcmp (xstr, "|") == 0 && strcmp (ystr, "|") == 0) {
                continue;
            }
            if (itemp->tpointn >= itemp->pointn) {
                SUwarning (1, "loadcoord", "too many texture points");
                return -1;
            }
            itemp->points[itemp->tpointn].tx = atof (xstr);
            itemp->points[itemp->tpointn].ty = atof (ystr);
            itemp->tpointn++;
            free (xstr);
        }
        tokclose (tp);
    }
    itempn = dtsize (itemdict);
    if (!(itemps = malloc (sizeof (item_t *) * itempn))) {
        SUwarning (1, "loadcoord", "malloc failed for itemps");
        return -1;
    }
    for (
        itemi = 0, itemp = (item_t *) dtflatten (itemdict); itemp;
        itemp = (item_t *) dtlink (itemdict, itemp)
    )
        itemps[itemi++] = itemp;
    sfclose (fp);
    return 0;
}

static int simplifypoints (float digits) {
    item_t *itemp;
    int itempi;
    int pointi, pointj, pointk, pointn, indi;
    int count;
    double acc;

    SUmessage (1, "simplifypoints", "simplifying shape xys");
    if (digits < 0) {
        SUwarning (
            1, "simplifypoints", "negative number of digits (%d)", digits
        );
        return -1;
    }
    count = 0;
    acc = 0.0;
    if (digits > 0.0)
        acc = pow (10.0, (digits - 1.0)) / 1000000.0;
    for (itempi = 0; itempi < itempn; itempi++) {
        itemp = itemps[itempi];
        if (itemp->pointn == 0)
            continue;
        for (pointi = pointj = 0, indi = 0; indi < itemp->indn; indi++) {
            if (tn < itemp->inds[indi]) {
                if (!(ts = realloc (
                    ts, itemp->inds[indi] * sizeof (double)
                ))) {
                    SUwarning (1, "simplifypoints", "realloc failed");
                    return -1;
                }
                tn = itemp->inds[indi];
            }
            for (pointk = 0; pointk < itemp->inds[indi] - 1; pointk++)
                ts[pointk] = dist (
                    itemp->points[pointj + pointk],
                    itemp->points[pointj + pointk + 1]
                );
            pointn = 0;
            recsimplify (
                itemp, pointi, pointj, itemp->inds[indi], ts, acc, &pointn
            );
            if (itemp->flagn > 0)
                itemp->flags[pointi + pointn] = (
                    itemp->flags[pointj + itemp->inds[indi] - 1]
                );
            itemp->points[pointi + pointn++] = (
                itemp->points[pointj + itemp->inds[indi] - 1]
            );
            count += (itemp->inds[indi] - pointn);
            SUmessage (2, "simplifypoints", "eliminated %d points", count);
            pointj += itemp->inds[indi];
            itemp->pointn -= (itemp->inds[indi] - pointn);
            itemp->inds[indi] = pointn;
            pointi += pointn;
        }
    }
    SUmessage (1, "simplifypoints", "eliminated %d points", count);
    return 0;
}

static void recsimplify (
    item_t *itemp, int pointi, int pointj, int pointn, double *ts, double mind,
    int *pointnp
) {
    int pointm;

    if ((pointm = maxdist (itemp->points, pointj, pointn, ts, mind)) == -1) {
        if (itemp->flagn > 0)
            itemp->flags[pointi + *pointnp] = itemp->flags[pointj];
        itemp->points[pointi + *pointnp] = itemp->points[pointj], (*pointnp)++;
        return;
    }
    if (pointm > pointj)
        recsimplify (
            itemp, pointi, pointj, pointm - pointj + 1, ts, mind, pointnp
        );
    if (pointm - pointj + 1 < pointn)
        recsimplify (
            itemp, pointi, pointm, pointn - pointm + pointj, ts, mind, pointnp
        );
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

static int savepoints (char *prefix, char *classp) {
    GEOM_t geom;
    char *fname;
    Sfio_t *fp;
    item_t *itemp;
    int itempi, itemn;
    int dictl;
    int pointi;
    int labeli, labeln;
    double x, y;

    for (
        itemn = 0, itempi = 0, dictl = 0, labeln = 0; itempi < itempn; itempi++
    ) {
        itemp = itemps[itempi];
        if (itemp->pointn == 0)
            continue;
        itemn++;
        dictl += strlen (itemp->name) + 1;
        labeln += itemp->labeln;
    }
    if (
        !(geom.items = malloc (sizeof (char *) * itemn)) ||
        !(geom.points = malloc (sizeof (GEOMpoint_t) * itemn)) ||
        !(geom.p2is = malloc (sizeof (int) * itemn))
    ) {
        SUwarning (1, "savepoints", "malloc failed");
        return -1;
    }
    geom.labels = NULL;
    geom.labeln = labeln;
    if (
        labeln > 0 && !(geom.labels = malloc (sizeof (GEOMlabel_t) * labeln))
    ) {
        SUwarning (1, "savepoints", "malloc failed");
        return -1;
    }
    geom.name = prefix;
    geom.type = GEOM_TYPE_POINTS;
    geom.class = classp;
    for (
        itempi = 0, geom.itemn = geom.pointn = geom.p2in = geom.labeln = 0;
        itempi < itempn; itempi++
    ) {
        itemp = itemps[itempi];
        if (itemp->pointn == 0)
            continue;
        geom.p2is[geom.p2in++] = geom.itemn;
        for (x = y = 0.0, pointi = 0; pointi < itemp->pointn; pointi++)
            x += itemp->points[pointi].x, y += itemp->points[pointi].y;
        x /= itemp->pointn, y /= itemp->pointn;
        geom.points[geom.pointn][0] = x;
        geom.points[geom.pointn][1] = y;
        geom.points[geom.pointn++][2] = 0.0;
        for (labeli = 0; labeli < itemp->labeln; labeli++) {
            itemp->labels[labeli].itemi = itempi;
            geom.labels[geom.labeln++] = itemp->labels[labeli];
        }
        geom.items[geom.itemn++] = (unsigned char *) itemp->name;
    }
    geom.dict = NULL;
    geom.dictlen = dictl;
    geom.keytype = GEOM_KEYTYPE_STRING;
    geom.keylen = -1;
    geom.colors = NULL;
    geom.colorn = -1;
    geom.inds = NULL;
    geom.indn = -1;
    geom.flags = NULL;
    geom.flagn = -1;
    geom.tpoints = NULL;
    geom.tpointn = -1;

    fname = strdup (sfprints ("%s_points.single", prefix));
    SUmessage (1, "savepoints", "saving file %s", fname);
    if (!(fp = sfopen (NULL, fname, "w"))) {
        SUwarning (1, "savepoints", "open failed for file %s", fname);
        return -1;
    }
    if (GEOMsave (fp, &geom) == -1) {
        SUwarning (1, "savepoints", "save failed for file %s", fname);
        return -1;
    }
    sfclose (fp);
    return 0;
}

static int savelines (char *prefix, char *classp) {
    GEOM_t geom;
    char *fname;
    Sfio_t *fp;
    item_t *itemp;
    int itempi, itemn;
    int dictl;
    int pointi;
    int labeli, labeln;
    double x, y;

    for (
        itemn = 0, itempi = 0, dictl = 0, labeln = 0; itempi < itempn; itempi++
    ) {
        itemp = itemps[itempi];
        if (itemp->pointn == 0)
            continue;
        itemn++;
        dictl += strlen (itemp->name) + 1;
        labeln += itemp->labeln;
    }
    if (
        !(geom.items = malloc (sizeof (char *) * itemn)) ||
        !(geom.points = malloc (sizeof (GEOMpoint_t) * 2 * itemn)) ||
        !(geom.p2is = malloc (sizeof (int) * 2 * itemn)) ||
        !(geom.flags = malloc (sizeof (int) * 2 * itemn))
    ) {
        SUwarning (1, "savelines", "malloc failed");
        return -1;
    }
    geom.labels = NULL;
    geom.labeln = labeln;
    if (
        labeln > 0 && !(geom.labels = malloc (sizeof (GEOMlabel_t) * labeln))
    ) {
        SUwarning (1, "savepoints", "malloc failed");
        return -1;
    }
    geom.name = prefix;
    geom.type = GEOM_TYPE_LINES;
    geom.class = classp;
    for (
        itempi = 0,
        geom.itemn = geom.pointn = geom.p2in = geom.flagn = geom.labeln = 0;
        itempi < itempn; itempi++
    ) {
        itemp = itemps[itempi];
        if (itemp->pointn == 0)
            continue;
        geom.p2is[geom.p2in++] = geom.itemn;
        geom.p2is[geom.p2in++] = geom.itemn;
        geom.flags[geom.flagn++] = 8;
        geom.flags[geom.flagn++] = 12;
        for (x = y = 0.0, pointi = 0; pointi < itemp->pointn; pointi++)
            x += itemp->points[pointi].x, y += itemp->points[pointi].y;
        x /= itemp->pointn, y /= itemp->pointn;
        geom.points[geom.pointn][0] = x;
        geom.points[geom.pointn][1] = y;
        geom.points[geom.pointn++][2] = 0.0;
        geom.points[geom.pointn][0] = x;
        geom.points[geom.pointn][1] = y;
        geom.points[geom.pointn++][2] = 1.0;
        for (labeli = 0; labeli < itemp->labeln; labeli++) {
            itemp->labels[labeli].itemi = itempi;
            geom.labels[geom.labeln++] = itemp->labels[labeli];
        }
        geom.items[geom.itemn++] = (unsigned char *) itemp->name;
    }
    geom.dict = NULL;
    geom.dictlen = dictl;
    geom.keytype = GEOM_KEYTYPE_STRING;
    geom.keylen = -1;
    geom.colors = NULL;
    geom.colorn = -1;
    geom.inds = NULL;
    geom.indn = -1;
    geom.tpoints = NULL;
    geom.tpointn = -1;

    fname = strdup (sfprints ("%s_lines.single", prefix));
    SUmessage (1, "savelines", "saving file %s", fname);
    if (!(fp = sfopen (NULL, fname, "w"))) {
        SUwarning (1, "savelines", "open failed for file %s", fname);
        return -1;
    }
    if (GEOMsave (fp, &geom) == -1) {
        SUwarning (1, "savelines", "save failed for file %s", fname);
        return -1;
    }
    sfclose (fp);
    return 0;
}

static int savepolys (char *prefix, char *classp) {
    GEOM_t geom;
    char *fname;
    Sfio_t *fp;
    item_t *itemp;
    int itempi, itemn;
    int dictl;
    int indi, indn;
    int pointi, pointn;
    int labeli, labeln;

    for (
        pointn = indn = itemn = 0, itempi = 0, dictl = 0, labeln = 0;
        itempi < itempn; itempi++
    ) {
        itemp = itemps[itempi];
        if (itemp->pointn == 0)
            continue;
        itemn++;
        pointn += itemps[itempi]->pointn;
        indn += itemps[itempi]->indn;
        dictl += strlen (itemp->name) + 1;
        labeln += itemp->labeln;
    }
    if (
        !(geom.items = malloc (sizeof (char *) * itemn)) ||
        !(geom.points = malloc (sizeof (GEOMpoint_t) * pointn)) ||
        !(geom.inds = malloc (sizeof (int) * indn)) ||
        !(geom.p2is = malloc (sizeof (int) * pointn))
    ) {
        SUwarning (1, "savepoints", "malloc failed");
        return -1;
    }
    geom.labels = NULL;
    geom.labeln = labeln;
    if (
        labeln > 0 && !(geom.labels = malloc (sizeof (GEOMlabel_t) * labeln))
    ) {
        SUwarning (1, "savepoints", "malloc failed");
        return -1;
    }
    geom.name = prefix;
    geom.type = GEOM_TYPE_LINESTRIPS;
    geom.class = classp;
    for (
        itempi = 0,
        geom.itemn = geom.pointn = geom.indn = geom.p2in = geom.labeln = 0;
        itempi < itempn; itempi++
    ) {
        itemp = itemps[itempi];
        if (itemp->pointn == 0)
            continue;
        for (indi = 0; indi < itemp->indn; indi++)
            geom.inds[geom.indn++] = itemp->inds[indi];
        for (pointi = 0; pointi < itemp->pointn; pointi++) {
            geom.p2is[geom.p2in++] = geom.itemn;
            geom.points[geom.pointn][0] = itemp->points[pointi].x;
            geom.points[geom.pointn][1] = itemp->points[pointi].y;
            geom.points[geom.pointn++][2] = 0.0;
        }
        for (labeli = 0; labeli < itemp->labeln; labeli++) {
            itemp->labels[labeli].itemi = itempi;
            geom.labels[geom.labeln++] = itemp->labels[labeli];
        }
        geom.items[geom.itemn++] = (unsigned char *) itemp->name;
    }
    geom.dict = NULL;
    geom.dictlen = dictl;
    geom.keytype = GEOM_KEYTYPE_STRING;
    geom.keylen = -1;
    geom.colors = NULL;
    geom.colorn = -1;
    geom.flags = NULL;
    geom.flagn = -1;
    geom.tpoints = NULL;
    geom.tpointn = -1;

    fname = strdup (sfprints ("%s_polys.single", prefix));
    SUmessage (1, "savepolys", "saving file %s", fname);
    if (!(fp = sfopen (NULL, fname, "w"))) {
        SUwarning (1, "savepolys", "open failed for file %s", fname);
        return -1;
    }
    if (GEOMsave (fp, &geom) == -1) {
        SUwarning (1, "savepolys", "save failed for file %s", fname);
        return -1;
    }
    sfclose (fp);
    return 0;
}

static int savetris (char *prefix, char *classp) {
    GEOM_t geom;
    char *fname;
    Sfio_t *fp;
    item_t *itemp;
    int itempi, itemn;
    int dictl;
    int pointn, havetpoints;
    int labeli, labeln;
    int trii;

    for (
        pointn = itemn = 0, itempi = 0, dictl = 0, labeln = 0, havetpoints = 0;
        itempi < itempn; itempi++
    ) {
        itemp = itemps[itempi];
        if (itemp->trin == 0)
            continue;
        itemn++;
        pointn += 3 * itemps[itempi]->trin;
        dictl += strlen (itemp->name) + 1;
        labeln += itemp->labeln;
        if (itemp->tpointn > 0)
            havetpoints = 1;
    }
    if (
        !(geom.items = malloc (sizeof (char *) * itemn)) ||
        !(geom.points = malloc (sizeof (GEOMpoint_t) * pointn)) ||
        !(geom.p2is = malloc (sizeof (int) * pointn))
    ) {
        SUwarning (1, "savetris", "malloc failed");
        return -1;
    }
    geom.labels = NULL;
    geom.labeln = labeln;
    if (
        labeln > 0 && !(geom.labels = malloc (sizeof (GEOMlabel_t) * labeln))
    ) {
        SUwarning (1, "savepoints", "malloc failed");
        return -1;
    }
    geom.tpoints = NULL;
    geom.tpointn = 0;
    if (havetpoints > 0) {
        if (!(geom.tpoints = malloc (sizeof (GEOMtpoint_t) * pointn))) {
            SUwarning (1, "savepoints", "malloc failed");
            return -1;
        }
        geom.tpointn = pointn;
    }
    geom.name = prefix;
    geom.type = GEOM_TYPE_TRIS;
    geom.class = classp;
    for (
        itempi = 0, geom.itemn = geom.pointn = geom.p2in = geom.labeln = 0;
        itempi < itempn; itempi++
    ) {
        itemp = itemps[itempi];
        if (itemp->trin == 0)
            continue;
        for (trii = 0; trii < itemp->trin; trii++) {
            geom.p2is[geom.p2in++] = geom.itemn;
            geom.p2is[geom.p2in++] = geom.itemn;
            geom.p2is[geom.p2in++] = geom.itemn;
            if (havetpoints) {
                geom.tpoints[geom.pointn][0] = itemp->tris[trii].a.tx;
                geom.tpoints[geom.pointn][1] = itemp->tris[trii].a.ty;
            }
            geom.points[geom.pointn][0] = itemp->tris[trii].a.x;
            geom.points[geom.pointn][1] = itemp->tris[trii].a.y;
            geom.points[geom.pointn++][2] = 0.0;
            if (havetpoints) {
                geom.tpoints[geom.pointn][0] = itemp->tris[trii].b.tx;
                geom.tpoints[geom.pointn][1] = itemp->tris[trii].b.ty;
            }
            geom.points[geom.pointn][0] = itemp->tris[trii].b.x;
            geom.points[geom.pointn][1] = itemp->tris[trii].b.y;
            geom.points[geom.pointn++][2] = 0.0;
            if (havetpoints) {
                geom.tpoints[geom.pointn][0] = itemp->tris[trii].c.tx;
                geom.tpoints[geom.pointn][1] = itemp->tris[trii].c.ty;
            }
            geom.points[geom.pointn][0] = itemp->tris[trii].c.x;
            geom.points[geom.pointn][1] = itemp->tris[trii].c.y;
            geom.points[geom.pointn++][2] = 0.0;
        }
        for (labeli = 0; labeli < itemp->labeln; labeli++) {
            itemp->labels[labeli].itemi = itempi;
            geom.labels[geom.labeln++] = itemp->labels[labeli];
        }
        geom.items[geom.itemn++] = (unsigned char *) itemp->name;
    }
    geom.dict = NULL;
    geom.dictlen = dictl;
    geom.keytype = GEOM_KEYTYPE_STRING;
    geom.keylen = -1;
    geom.colors = NULL;
    geom.colorn = -1;
    geom.inds = NULL;
    geom.indn = -1;
    geom.flags = NULL;
    geom.flagn = -1;

    fname = strdup (sfprints ("%s_tris.single", prefix));
    SUmessage (1, "savetris", "saving file %s", fname);
    if (!(fp = sfopen (NULL, fname, "w"))) {
        SUwarning (1, "savetris", "open failed for file %s", fname);
        return -1;
    }
    if (GEOMsave (fp, &geom) == -1) {
        SUwarning (1, "savetris", "save failed for file %s", fname);
        return -1;
    }
    sfclose (fp);
    return 0;
}

/* co-linear test utilities */

static int maxdist (
    point_t *points, int pointj, int pointn, double *ts, double mind
) {
    double totl, l, t, dx, dy, d;
    point_t pointt;
    int pointi, mini;

    if (pointn < 2)
        return -1;
    dx = points[pointj + pointn - 1].x - points[pointj].x;
    dy = points[pointj + pointn - 1].y - points[pointj].y;
    for (totl = 0.0, pointi = 0; pointi < pointn - 1; pointi++)
        totl += ts[pointj + pointi];
    mini = -1;
    for (l = 0.0, d = 0.0, pointi = 0; pointi < pointn - 1; pointi++) {
        l += ts[pointj + pointi];
        t = l / totl;
        pointt.x = points[pointj].x + dx * t;
        pointt.y = points[pointj].y + dy * t;
        if ((d = dist (points[pointj + pointi + 1], pointt)) > mind)
            mind = d, mini = pointj + pointi + 1;
    }
    return mini;
}

static double dist (point_t pointa, point_t pointb) {
    double ax, ay, bx, by;

    ax = pointa.x, ay = pointa.y;
    bx = pointb.x, by = pointb.y;
    return sqrt (
        (bx - ax) * (bx - ax) + (by - ay) * (by - ay)
    );
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
#if 0
    if (
        ((p1.x == p2.x && p2.x == p3.x) && (p3.y > p2.y)) ||
        ccw (p1, p2, p3) != ISCCW
    ) {
#else
    if (
        ((p1.x == p2.x && p2.x == p3.x) && (p3.y > p2.y)) ||
        ccw (p1, p2, p3) == ISCW
    ) {
#endif
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

static tri_t *gettri (item_t *itemp, point_t a, point_t b, point_t c) {
    tri_t *trip;

    if (itemp->trin == itemp->trim) {
        if (!itemp->tris) {
            if (!(itemp->tris = malloc (sizeof (tri_t) * TRIINCR))) {
                SUwarning (1, "gettri", "malloc failed");
                return NULL;
            }
        } else {
            if (!(itemp->tris = realloc (
                itemp->tris, sizeof (tri_t) * (itemp->trim + TRIINCR)
            ))) {
                SUwarning (1, "gettri", "realloc failed");
                return NULL;
            }
        }
        itemp->trim += TRIINCR;
    }
    trip = &itemp->tris[itemp->trin++];
    if (ccw (a, b, c) == ISCCW)
        trip->a = a, trip->b = b, trip->c = c;
    else
        trip->a = c, trip->b = b, trip->c = b;
    return trip;
}

static int printbbox (void) {
    point_t minp, maxp, p;
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
    sfprintf (
        sfstdout, "bbox: %f %f 0 %f %f 0\n", minp.x, minp.y, maxp.x, maxp.y
    );
    return 0;
}
