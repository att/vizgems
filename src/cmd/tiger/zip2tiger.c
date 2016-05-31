#pragma prototyped

#include <ast.h>
#include <option.h>
#include <cdt.h>
#include <swift.h>
#include <math.h>
#include "tiger.h"

static const char usage[] =
"[-1p1s1D?\n@(#)$Id: zip2tiger (AT&T Labs Research) 2001-09-24 $\n]"
USAGE_LICENSE
"[+NAME?zip2tiger - load zip information to a TIGER dataset]"
"[+DESCRIPTION?\bzip2tiger\b populates the zip fields of the specified"
" dataset from the supplied zip file."
"]"
"[100:i?specifies the directory containing the input files."
" The default is the current directory."
"]:[inputdir]"
"[101:o?specifies the directory for the output files."
" The default is the current directory."
"]:[outputdir]"
"[500:z?specifies the file mapping lat/longs to zip ids."
" The default is \bzip.coords\b."
"]:[zipcoords]"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"\n"
"[+SEE ALSO?\btiger\b(1)]"
;

typedef struct xyzip_t {
    int x, y, zip;
} xyzip_t;

static xyzip_t *xyzips;
static int xyzipm, xyzipn;

static int loadpointfile (char *);
static int markallpolys (void);
static int propagatebyedge (void);
static int xyzipinpoly (poly_t *, xyzip_t);
static double dist2poly (poly_t *, xyzip_t);

int main (int argc, char **argv) {
    int norun;
    char *startbrk, *endbrk;
    char *indir, *outdir, *zipfile;

    startbrk = (char *) sbrk (0);
    indir = ".";
    outdir = ".";
    zipfile = "zip.coords";
    norun = 0;
    for (;;) {
        switch (optget (argv, usage)) {
        case -100:
            indir = opt_info.arg;
            continue;
        case -101:
            outdir = opt_info.arg;
            continue;
        case -500:
            zipfile = opt_info.arg;
            continue;
        case -999:
            SUwarnlevel++;
            continue;
        case '?':
            SUusage (0, "zip2tiger", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "zip2tiger", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;

    initrecords ();
    if (loaddata (indir) == -1)
        SUerror ("zip2tiger", "cannot load data");
    if (mergeonesidededges () == -1)
        SUerror ("zip2tiger", "cannot merge onesided edges");
    if (loadpointfile (zipfile) == -1)
        SUerror ("zip2tiger", "cannot load point file");
    if (markallpolys () == -1)
        SUerror ("zip2tiger", "cannot mark all polys");
    while (propagatebyedge () > 0)
        ;
    savedata (outdir);
    printstats ();
    endbrk = (char *) sbrk (0);
    SUmessage (1, "zip2tiger", "memory usage %d", endbrk - startbrk);
    return 0;
}

static int loadpointfile (char *infile) {
    char *line, *s1, *s2;
    Sfio_t *fp;

    SUmessage (1, "loadpointfile", "loading zip points");
    xyzipm = 25000;
    if (!(xyzips = malloc (xyzipm * sizeof (xyzip_t)))) {
        SUwarning (1, "loadpointfile", "malloc failed for xyzips");
        return -1;
    }
    if (!(fp = sfopen (NULL, infile, "r"))) {
        SUwarning (1, "loadpointfile", "open failed for file %s", infile);
        return -1;
    }
    xyzipn = 0;
    while ((line = sfgetr (fp, '\n', 1))) {
        if (!(s1 = strchr (line, ' ')) || !(s2 = strchr (s1 + 1, ' '))) {
            SUwarning (1, "loadpointfile", "bad line %s", line);
            return -1;
        }
        if (xyzipn == xyzipm) {
            if (!(xyzips = realloc (xyzips, (xyzipm * 2) * sizeof (xyzip_t)))) {
                SUwarning (1, "loadpointfile", "realloc failed for xyzips");
                return -1;
            }
            xyzipm *= 2;
        }
        xyzips[xyzipn].y = atof (s2 + 1) * 1000000;
        *s2 = 0;
        xyzips[xyzipn].x = atof (s1 + 1) * 1000000;
        *s1 = 0;
        xyzips[xyzipn].zip = atoi (line);
        xyzipn++;
    }
    sfclose (fp);
    return 0;
}

static int markallpolys (void) {
    xy_t pmaxxy, pminxy;
    edge_t *ep;
    poly_t *pp;
    int xyzipi, minxyzipi, edgepi;
    double d, mind;
    int count;

    SUmessage (1, "markallpolys", "matching zips to polys");
    for (
        ep = (edge_t *) dtflatten (edgedict); ep;
        ep = (edge_t *) dtlink (edgedict, ep)
    )
        ep->zipl = ep->zipr = 0;
    count = 0;
    for (
        pp = (poly_t *) dtflatten (polydict); pp;
        pp = (poly_t *) dtlink (polydict, pp)
    ) {
        if (
            pp->edgepl > 0 &&
            ((pp->edgeps[0]->p0p == pp &&
            (pp->edgeps[0]->zipl > 0)) ||
            (pp->edgeps[0]->p1p == pp &&
            (pp->edgeps[0]->zipr > 0)))
        )
            continue;
        pminxy.x = INT_MAX, pminxy.y = INT_MAX;
        pmaxxy.x = INT_MIN, pmaxxy.y = INT_MIN;
        for (edgepi = 0; edgepi < pp->edgepl; edgepi++) {
            ep = pp->edgeps[edgepi];
            if (pmaxxy.x < ep->v0p->xy.x)
                pmaxxy.x = ep->v0p->xy.x;
            if (pmaxxy.y < ep->v0p->xy.y)
                pmaxxy.y = ep->v0p->xy.y;
            if (pminxy.x > ep->v0p->xy.x)
                pminxy.x = ep->v0p->xy.x;
            if (pminxy.y > ep->v0p->xy.y)
                pminxy.y = ep->v0p->xy.y;
            if (pmaxxy.x < ep->v1p->xy.x)
                pmaxxy.x = ep->v1p->xy.x;
            if (pmaxxy.y < ep->v1p->xy.y)
                pmaxxy.y = ep->v1p->xy.y;
            if (pminxy.x > ep->v1p->xy.x)
                pminxy.x = ep->v1p->xy.x;
            if (pminxy.y > ep->v1p->xy.y)
                pminxy.y = ep->v1p->xy.y;
        }
        for (xyzipi = 0; xyzipi < xyzipn; xyzipi++) {
            if (
                xyzips[xyzipi].x > pmaxxy.x || xyzips[xyzipi].x < pminxy.x ||
                xyzips[xyzipi].y > pmaxxy.y || xyzips[xyzipi].y < pminxy.y
            )
                continue;
            if (!xyzipinpoly (pp, xyzips[xyzipi]))
                continue;
            for (edgepi = 0; edgepi < pp->edgepl; edgepi++) {
                ep = pp->edgeps[edgepi];
                if (ep->p0p == pp)
                    ep->zipl = xyzips[xyzipi].zip;
                if (ep->p1p == pp)
                    ep->zipr = xyzips[xyzipi].zip;
            }
            if (++count % 1000 == 0)
                SUmessage (
                    1, "markallpolys", "%d done, %d left", count, xyzipn
                );
        }
    }
    if (count == 0) {
        for (
            pp = (poly_t *) dtflatten (polydict); pp;
            pp = (poly_t *) dtlink (polydict, pp)
        ) {
            if (
                pp->edgepl > 0 &&
                ((pp->edgeps[0]->p0p == pp &&
                (pp->edgeps[0]->zipl > 0)) ||
                (pp->edgeps[0]->p1p == pp &&
                (pp->edgeps[0]->zipr > 0)))
            )
                continue;
            mind = (double) INT_MAX, minxyzipi = -1;
            for (xyzipi = 0; xyzipi < xyzipn; xyzipi++)
                if ((d = dist2poly (pp, xyzips[xyzipi])) < mind)
                    mind = d, minxyzipi = xyzipi;
            if (minxyzipi == -1)
                continue;
            for (edgepi = 0; edgepi < pp->edgepl; edgepi++) {
                ep = pp->edgeps[edgepi];
                if (ep->p0p == pp)
                    ep->zipl = xyzips[minxyzipi].zip;
                if (ep->p1p == pp)
                    ep->zipr = xyzips[minxyzipi].zip;
            }
            count++;
            SUmessage (1, "markallpolys", "found nearest point");
            break;
        }
    }
    SUmessage (1, "markallpolys", "%d done, %d left", count, xyzipn);
    return 0;
}

static int propagatebyedge (void) {
    int edgepi;
    edge_t *e0p, *e1p;
    int count;

    static int mark;

    SUmessage (1, "propagatebyedge", "propagating zips by edge");
    mark++;
    count = 0;
    for (
        e0p = (edge_t *) dtflatten (edgedict); e0p;
        e0p = (edge_t *) dtlink (edgedict, e0p)
    ) {
        if (e0p->zipl > 0 && e0p->p0p->mark != mark) {
            if (e0p->zipr == 0 && e0p->p1p && e0p->p1p != e0p->p0p) {
                count++;
                for (edgepi = 0; edgepi < e0p->p1p->edgepl; edgepi++) {
                    e1p = e0p->p1p->edgeps[edgepi];
                    if (e1p->p0p == e0p->p1p)
                        e1p->zipl = e0p->zipl;
                    if (e1p->p1p == e0p->p1p)
                        e1p->zipr = e0p->zipl;
                }
                e0p->p1p->mark = mark;
            }
        }
        if (e0p->zipr > 0 && e0p->p1p->mark != mark) {
            if (e0p->zipl == 0 && e0p->p0p && e0p->p0p != e0p->p1p) {
                count++;
                for (edgepi = 0; edgepi < e0p->p0p->edgepl; edgepi++) {
                    e1p = e0p->p0p->edgeps[edgepi];
                    if (e1p->p0p == e0p->p0p)
                        e1p->zipl = e0p->zipr;
                    if (e1p->p1p == e0p->p0p)
                        e1p->zipr = e0p->zipr;
                }
                e0p->p0p->mark = mark;
            }
        }
    }
    SUmessage (1, "propagatebyedge", "assigned zips to %d polys", count);
    return count;
}

static int xyzipinpoly (poly_t *pp, xyzip_t xyzip) {
    xy_t xya, xyb, minxy, maxxy;
    int edgepi, n, t;

    n = 0;
    for (edgepi = 0; edgepi < pp->edgepl; edgepi++) {
        xya = pp->edgeps[edgepi]->v0p->xy;
        xyb = pp->edgeps[edgepi]->v1p->xy;
        minxy = xya, maxxy = xyb;
        if (minxy.x > maxxy.x)
            t = minxy.x, minxy.x = maxxy.x, maxxy.x = t;
        if (minxy.y > maxxy.y)
            t = minxy.y, minxy.y = maxxy.y, maxxy.y = t;
        if (maxxy.x < xyzip.x || minxy.y > xyzip.y || maxxy.y <= xyzip.y)
            continue;
        if (
            ((xyb.x - xya.x) / (double) (xyb.y - xya.y)) *
            (xyzip.y - xya.y) + xya.x < xyzip.x
        )
            continue;
        n++;
    }
    if (n % 2 == 1)
        return TRUE;
    return FALSE;
}

static double dist2poly (poly_t *pp, xyzip_t xyzip) {
    xy_t xya, xyb;
    int edgepi;
    double mind, d;

    mind = (double) INT_MAX;
    for (edgepi = 0; edgepi < pp->edgepl; edgepi++) {
        xya = pp->edgeps[edgepi]->v0p->xy;
        xyb = pp->edgeps[edgepi]->v1p->xy;
        if (
            (d = sqrt ((xya.x - xyzip.x) * (xya.x - xyzip.x) +
            (xya.y - xyzip.y) * (xya.y - xyzip.y))) < mind
        )
            mind = d;
        if (
            (d = sqrt ((xyb.x - xyzip.x) * (xyb.x - xyzip.x) +
            (xyb.y - xyzip.y) * (xyb.y - xyzip.y))) < mind
        )
            mind = d;
    }
    return mind;
}
