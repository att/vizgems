#pragma prototyped

#include <ast.h>
#include <option.h>
#include <swift.h>
#include "geom.h"

static const char usage[] =
"[-1p1s1D?\n@(#)$Id: geomprint (AT&T Labs Research) 2001-09-24 $\n]"
USAGE_LICENSE
"[+NAME?geomprint - generate ascii format geometry]"
"[+DESCRIPTION?\bgeomprint\b generates ascii output of a SWIFT geometry file."
"]"
"[700:ps?generates postscript output."
"]"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"\n"
"\n[ file ]\n"
"\n"
"[+SEE ALSO?\btiger\b(1)]"
;

#define X(px) ((px) - minx) / (double) (maxx - minx)
#define Y(py) ((py) - miny) / (double) (maxy - miny)

typedef struct point_t {
    float x, y;
} point_t;

static char *typemap[] = {
    /* GEOM_TYPE_POINTS     */  "points",
    /* GEOM_TYPE_LINES      */  "lines",
    /* GEOM_TYPE_LINESTRIPS */  "linestrips",
    /* GEOM_TYPE_POLYS      */  "polys",
    /* GEOM_TYPE_TRIS       */  "tris",
    /* GEOM_TYPE_QUADS      */  "quads",
                                NULL,
};

static char *keytypemap[] = {
    /* GEOM_KEYTYPE_CHAR   */  "char",
    /* GEOM_KEYTYPE_SHORT  */  "short",
    /* GEOM_KEYTYPE_INT    */  "int",
    /* GEOM_KEYTYPE_LLONG  */  "llong",
    /* GEOM_KEYTYPE_FLOAT  */  "float",
    /* GEOM_KEYTYPE_DOUBLE */  "double",
    /* GEOM_KEYTYPE_STRING */  "string",
                               NULL,
};

static int printgeomfile1 (char *);
static int printgeomfile2 (char *);
static void *alloc (void *, size_t, int);

int main (int argc, char **argv) {
    int norun;
    int psmode;

    psmode = FALSE;
    norun = 0;
    for (;;) {
        switch (optget (argv, usage)) {
        case -700:
            psmode = TRUE;
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
    if (argc < 1)
        SUerror ("geomprint", "missing file name");

    if (!psmode) {
        if (printgeomfile1 (argv[0]) == -1)
            SUerror ("geomprint", "printgeomfile1 failed for %s", argv[0]);
    } else {
        if (printgeomfile2 (argv[0]) == -1)
            SUerror ("geomprint", "printgeomfile2 failed for %s", argv[0]);
    }
    return 0;
}

static int printgeomfile1 (char *fname) {
    Sfio_t *fp;
    GEOM_t *geomp;
    int itemi;
    GEOMpoint_t *pointp;
    int pointi;
    GEOMtpoint_t *tpointp;
    GEOMcolor_t *colorp;
    int colori;
    int indi;
    int p2ii, flagi, labeli;

    if (!(fp = sfopen (NULL, fname, "r"))) {
        SUwarning (1, "printgeomfile1", "cannot open file %s", fname);
        return -1;
    }
    if (!(geomp = GEOMload (fp, alloc))) {
        SUwarning (1, "printgeomfile1", "cannot read geometry");
        return -1;
    }
    sfprintf (sfstdout, "name: %s\n", geomp->name);
    sfprintf (sfstdout, "type: %s\n", typemap[geomp->type]);
    sfprintf (sfstdout, "class: %s\n", geomp->class);
    sfprintf (sfstdout, "items: %d\n", geomp->itemn);
    sfprintf (sfstdout, "dict length: %d\n", geomp->dictlen);
    sfprintf (sfstdout, "key type: %s\n", keytypemap[geomp->keytype]);
    sfprintf (sfstdout, "key length: %d\n", geomp->keylen);
    for (itemi = 0; itemi < geomp->itemn; itemi++) {
        sfprintf (sfstdout, "item: %d ", itemi);
        switch (geomp->keytype) {
        case GEOM_KEYTYPE_CHAR:
            sfprintf (sfstdout, "%c\n", *(char *) &geomp->items[itemi]);
            break;
        case GEOM_KEYTYPE_SHORT:
            sfprintf (sfstdout, "%d\n", *(short *) &geomp->items[itemi]);
            break;
        case GEOM_KEYTYPE_INT:
            sfprintf (sfstdout, "%d\n", *(int *) &geomp->items[itemi]);
            break;
        case GEOM_KEYTYPE_LLONG:
            sfprintf (sfstdout, "%lld\n", *(long long *) &geomp->items[itemi]);
            break;
        case GEOM_KEYTYPE_FLOAT:
            sfprintf (sfstdout, "%f\n", *(float *) &geomp->items[itemi]);
            break;
        case GEOM_KEYTYPE_DOUBLE:
            sfprintf (sfstdout, "%f\n", *(double *) &geomp->items[itemi]);
            break;
        case GEOM_KEYTYPE_STRING:
            sfprintf (sfstdout, "%s\n", geomp->items[itemi]);
            break;
        }
    }
    if (geomp->points) {
        sfprintf (sfstdout, "points: %d\n", geomp->pointn);
        for (pointi = 0; pointi < geomp->pointn; pointi++) {
            pointp = &geomp->points[pointi];
            sfprintf (
                sfstdout, "point: %d (%f,%f,%f)",
                pointi, (*pointp)[0], (*pointp)[1], (*pointp)[2]
            );
            if (geomp->tpoints) {
                tpointp = &geomp->tpoints[pointi];
                sfprintf (
                    sfstdout, " (%f,%f)", (*tpointp)[0], (*tpointp)[1]
                );
            }
            sfprintf (sfstdout, "\n");
        }
    }
    if (geomp->colors) {
        sfprintf (sfstdout, "colors: %d\n", geomp->colorn);
        for (colori = 0; colori < geomp->colorn; colori++) {
            colorp = &geomp->colors[colori];
            sfprintf (
                sfstdout, "color: %d (%f,%f,%f,%f)\n",
                colori, (*colorp)[0], (*colorp)[1], (*colorp)[2], (*colorp)[3]
            );
        }
    }
    if (geomp->inds) {
        sfprintf (sfstdout, "inds: %d\n", geomp->indn);
        for (indi = 0; indi < geomp->indn; indi++)
            sfprintf (sfstdout, "ind: %d %d\n", indi, geomp->inds[indi]);
    }
    if (geomp->p2is) {
        sfprintf (sfstdout, "p2is: %d\n", geomp->p2in);
        for (p2ii = 0; p2ii < geomp->p2in; p2ii++)
            sfprintf (sfstdout, "p2i: %d %d\n", p2ii, geomp->p2is[p2ii]);
    }
    if (geomp->flags) {
        sfprintf (sfstdout, "flags: %d\n", geomp->flagn);
        for (flagi = 0; flagi < geomp->flagn; flagi++)
            sfprintf (sfstdout, "flag: %d %d\n", flagi, geomp->flags[flagi]);
    }
    if (geomp->labels) {
        sfprintf (sfstdout, "labels: %d\n", geomp->labeln);
        for (labeli = 0; labeli < geomp->labeln; labeli++) {
            sfprintf (sfstdout, "label: %d %s (%f,%f,%f)-(%f,%f,%f) %d\n",
            labeli, geomp->labels[labeli].label, geomp->labels[labeli].o[0],
            geomp->labels[labeli].o[1], geomp->labels[labeli].o[2],
            geomp->labels[labeli].c[0], geomp->labels[labeli].c[1],
            geomp->labels[labeli].o[2], geomp->labels[labeli].itemi
        );
        }
    }
    return 0;
}

static int printgeomfile2 (char *fname) {
    Sfio_t *fp;
    GEOM_t *geomp;
    double minx, miny, maxx, maxy;
    GEOMpoint_t *pointp;
    int pointi, pointn;
    int indi;

    minx = miny = 99999999.0, maxx = maxy = -99999999.0;
    if (!(fp = sfopen (NULL, fname, "r"))) {
        SUwarning (1, "printgeomfile2", "cannot open file %s", fname);
        return -1;
    }
    if (!(geomp = GEOMload (fp, alloc))) {
        SUwarning (1, "printgeomfile2", "cannot read geometry");
        return -1;
    }
    if (!geomp->points)
        return 0;
    sfprintf (sfstdout, "%%!PS\n");
    sfprintf (sfstdout, "8 72 mul 10 72 mul scale 0 setlinewidth\n");
    sfprintf (sfstdout, "/n { newpath } def\n");
    sfprintf (sfstdout, "/m { moveto } def\n");
    sfprintf (sfstdout, "/l { lineto } def\n");
    sfprintf (
        sfstdout, "/p { /y exch def /x exch def x y moveto x y lineto } def\n"
    );
    sfprintf (sfstdout, "/s { closepath stroke } def\n");
    sfprintf (sfstdout, "/f { closepath fill } def\n");
    for (pointi = 0; pointi < geomp->pointn; pointi++) {
        pointp = &geomp->points[pointi];
        if (minx > (*pointp)[0])
            minx = (*pointp)[0];
        if (maxx < (*pointp)[0])
            maxx = (*pointp)[0];
        if (miny > (*pointp)[1])
            miny = (*pointp)[1];
        if (maxy < (*pointp)[1])
            maxy = (*pointp)[1];
    }
    if (geomp->inds) {
        for (indi = 0, pointi = 0; indi < geomp->indn; indi++) {
            pointn = geomp->inds[indi];
            sfprintf (
                sfstdout, "n %f %f m\n",
                X (geomp->points[pointi][0]), Y (geomp->points[pointi][1])
            );
            pointi++, pointn--;
            for (; pointn > 0; pointi++, pointn--)
                sfprintf (
                    sfstdout, "%f %f l\n",
                    X (geomp->points[pointi][0]), Y (geomp->points[pointi][1])
                );
            sfprintf (sfstdout, "stroke\n");
        }
    } else {
        if (geomp->type == GEOM_TYPE_POINTS) {
            for (pointi = 0; pointi < geomp->pointn; pointi++)
                sfprintf (
                    sfstdout, "n %f %f p s\n",
                    X (geomp->points[pointi][0]), Y (geomp->points[pointi][1])
                );
        } else if (geomp->type == GEOM_TYPE_LINES) {
            for (pointi = 0; pointi < geomp->pointn; pointi += 2)
                sfprintf (
                    sfstdout, "n %f %f p s\n",
                    X (geomp->points[pointi][0]), Y (geomp->points[pointi][1])
                );
        } else if (geomp->type == GEOM_TYPE_TRIS) {
            for (pointi = 0; pointi < geomp->pointn; pointi += 3)
                sfprintf (
                    sfstdout, "n %f %f m %f %f l %f %f l f\n",
                    X (geomp->points[pointi][0]),
                    Y (geomp->points[pointi][1]),
                    X (geomp->points[pointi + 1][0]),
                    Y (geomp->points[pointi + 1][1]),
                    X (geomp->points[pointi + 2][0]),
                    Y (geomp->points[pointi + 2][1])
                );
        }
        sfprintf (sfstdout, "showpage\n");
    }
    return 0;
}

static void *alloc (void *oldp, size_t itemsize, int n) {
    void *p;

    if (!oldp)
        p = malloc (itemsize * n);
    else
        p = realloc (oldp, itemsize * n);
    if (!p) {
        SUwarning (
            1, "alloc", "alloc failed for (%x, %d, %d)", oldp, itemsize, n
        );
        return NULL;
    }
    return p;
}
