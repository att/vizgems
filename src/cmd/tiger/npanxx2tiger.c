#pragma prototyped

#include <ast.h>
#include <option.h>
#include <cdt.h>
#include <swift.h>
#include <math.h>
#include "tiger.h"

static const char usage[] =
"[-1p1s1D?\n@(#)$Id: npanxx2tiger (AT&T Labs Research) 2001-09-24 $\n]"
USAGE_LICENSE
"[+NAME?npanxx2tiger - load npanxx information to a TIGER dataset]"
"[+DESCRIPTION?\bnpanxx2tiger\b populates the npanxxloc fields of the"
" specified dataset from the supplied npanxxloc files."
"]"
"[100:i?specifies the directory containing the input files."
" The default is the current directory."
"]:[inputdir]"
"[101:o?specifies the directory for the output files."
" The default is the current directory."
"]:[outputdir]"
"[500:p?specifies the file mapping lat/longs to npanxxloc ids."
" The default is \bnpanxxloc.coords\b."
"]:[npanxxcoords]"
"[501:s?specifies the file mapping states to npanxxloc ids."
" The default is \bnpanxxloc.states\b."
"]:[npanxxstates]"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"\n"
"[+SEE ALSO?\btiger\b(1)]"
;

typedef struct statemap_t {
    int id;
    char *name;
} statemap_t;

static statemap_t statemap[] = {
    {  1, "al" }, {  2, "ak" }, {  4, "az" }, {  5, "ar" },
    {  6, "ca" }, {  8, "co" }, {  9, "ct" }, { 10, "de" },
    { 11, "dc" }, { 12, "fl" }, { 13, "ga" }, { 15, "hi" },
    { 16, "id" }, { 17, "il" }, { 18, "in" }, { 19, "ia" },
    { 20, "ks" }, { 21, "ky" }, { 22, "la" }, { 23, "me" },
    { 24, "md" }, { 25, "ma" }, { 26, "mi" }, { 27, "mn" },
    { 28, "ms" }, { 29, "mo" }, { 30, "mt" }, { 31, "ne" },
    { 32, "nv" }, { 33, "nh" }, { 34, "nj" }, { 35, "nm" },
    { 36, "ny" }, { 37, "nc" }, { 38, "nd" }, { 39, "oh" },
    { 40, "ok" }, { 41, "or" }, { 42, "pa" }, { 44, "ri" },
    { 45, "sc" }, { 46, "sd" }, { 47, "tn" }, { 48, "tx" },
    { 49, "ut" }, { 50, "vt" }, { 51, "va" }, { 53, "wa" },
    { 54, "wv" }, { 55, "wi" }, { 56, "wy" }, { 60, "as" },
    { 66, "gu" }, { 69, "mp" }, { 72, "pr" }, { 74, "um" },
    { 78, "vi" }, { -1, NULL }
};

static xy_t *points;
static int pointm, pointn;

static int *states;
static int statem, staten;

static int loadpointfile (char *);
static int loadstatefile (char *);
static int markallpolys (void);
static int propagatebyedge (void);
static int pointinpoly (poly_t *, xy_t);
static double dist2poly (poly_t *, xy_t);

int main (int argc, char **argv) {
    int norun;
    char *startbrk, *endbrk;
    char *indir, *outdir, *npanxxfile, *statefile;

    startbrk = (char *) sbrk (0);
    indir = ".";
    outdir = ".";
    npanxxfile = "npanxxloc.coords";
    statefile = "npanxxloc.states";
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
            npanxxfile = opt_info.arg;
            continue;
        case -501:
            statefile = opt_info.arg;
            continue;
        case -999:
            SUwarnlevel++;
            continue;
        case '?':
            SUusage (0, "npanxx2tiger", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "npanxx2tiger", opt_info.arg);
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
        SUerror ("npanxx2tiger", "cannot load data");
    if (mergeonesidededges () == -1)
        SUerror ("npanxx2tiger", "cannot merge onesided edges");
    if (loadpointfile (npanxxfile) == -1)
        SUerror ("npanxx2tiger", "cannot load point file");
    if (loadstatefile (statefile) == -1)
        SUerror ("npanxx2tiger", "cannot load states file");
    if (markallpolys () == -1)
        SUerror ("npanxx2tiger", "cannot mark all polys");
    while (propagatebyedge () > 0)
        ;
    savedata (outdir);
    printstats ();
    endbrk = (char *) sbrk (0);
    SUmessage (1, "npanxx2tiger", "memory usage %d", endbrk - startbrk);
    return 0;
}

static int loadpointfile (char *infile) {
    char *line, *s1, *s2;
    Sfio_t *fp;

    SUmessage (1, "loadpointfile", "loading npanxxloc points");
    pointm = 25000;
    if (!(points = malloc (pointm * sizeof (xy_t)))) {
        SUwarning (1, "loadpointfile", "malloc failed for points");
        return -1;
    }
    if (!(fp = sfopen (NULL, infile, "r"))) {
        SUwarning (1, "loadpointfile", "open failed for file %s", infile);
        return -1;
    }
    pointn = 0;
    while ((line = sfgetr (fp, '\n', 1))) {
        if (!(s1 = strchr (line, ' ')) || !(s2 = strchr (s1 + 1, ' '))) {
            SUwarning (1, "loadpointfile", "bad line %s", line);
            return -1;
        }
        if (pointn == pointm) {
            if (!(points = realloc (points, (pointm * 2) * sizeof (xy_t)))) {
                SUwarning (1, "loadpointfile", "realloc failed for points");
                return -1;
            }
            pointm *= 2;
        }
        points[pointn].y = atoi (s2 + 1);
        *s2 = 0;
        points[pointn].x = atoi (s1 + 1);
        pointn++;
    }
    sfclose (fp);
    return 0;
}

static int loadstatefile (char *infile) {
    char *line, *s1;
    Sfio_t *fp;
    int smi;

    SUmessage (1, "loadstatefile", "loading npanxxloc states");
    statem = 25000;
    if (!(states = malloc (statem * sizeof (int)))) {
        SUwarning (1, "loadstatefile", "malloc failed for states");
        return -1;
    }
    if (!(fp = sfopen (NULL, infile, "r"))) {
        SUwarning (1, "loadstatefile", "open failed for file %s", infile);
        return -1;
    }
    staten = 0;
    while ((line = sfgetr (fp, '\n', 1))) {
        if (!(s1 = strchr (line, ' '))) {
            SUwarning (1, "loadstatefile", "bad line %s", line);
            return -1;
        }
        if (staten == statem) {
            if (!(states = realloc (states, (statem * 2) * sizeof (int)))) {
                SUwarning (1, "loadstatefile", "realloc failed for states");
                return -1;
            }
            statem *= 2;
        }
        for (smi = 0; statemap[smi].id != -1; smi++)
            if (strcmp (s1 + 1, statemap[smi].name) == 0)
                break;
        if (statemap[smi].id == -1)
            SUwarning (1, "loadstatefile", "find failed for state %s", s1 + 1);
        states[staten] = statemap[smi].id;
        staten++;
    }
    sfclose (fp);
    return 0;
}

static int markallpolys (void) {
    xy_t maxxy, minxy;
    vertex_t *vp;
    edge_t *ep;
    poly_t *pp;
    int pointi, minpointi, edgepi;
    double d, mind;
    int count;

    SUmessage (1, "markallpolys", "matching npanxxlocs to polys");
    minxy.x = INT_MAX, minxy.y = INT_MAX;
    maxxy.x = INT_MIN, maxxy.y = INT_MIN;
    for (
        vp = (vertex_t *) dtflatten (vertexdict); vp;
        vp = (vertex_t *) dtlink (vertexdict, vp)
    ) {
        if (maxxy.x < vp->xy.x)
            maxxy.x = vp->xy.x;
        if (maxxy.y < vp->xy.y)
            maxxy.y = vp->xy.y;
        if (minxy.x > vp->xy.x)
            minxy.x = vp->xy.x;
        if (minxy.y > vp->xy.y)
            minxy.y = vp->xy.y;
    }
    for (
        ep = (edge_t *) dtflatten (edgedict); ep;
        ep = (edge_t *) dtlink (edgedict, ep)
    )
        ep->npanxxlocl = ep->npanxxlocr = -1;
    count = 0;
    for (pointi = 0; pointi < pointn; pointi++) {
        if (
            points[pointi].x > maxxy.x || points[pointi].x < minxy.x ||
            points[pointi].y > maxxy.y || points[pointi].y < minxy.y
        )
            continue;
        for (
            pp = (poly_t *) dtflatten (polydict); pp;
            pp = (poly_t *) dtlink (polydict, pp)
        ) {
            if (
                pp->edgepl > 0 &&
                ((pp->edgeps[0]->p0p == pp &&
                (pp->edgeps[0]->npanxxlocl >= 0 ||
                pp->edgeps[0]->statel != states[pointi])) ||
                (pp->edgeps[0]->p1p == pp &&
                (pp->edgeps[0]->npanxxlocr >= 0 ||
                pp->edgeps[0]->stater != states[pointi])))
            )
                continue;
            if (!pointinpoly (pp, points[pointi]))
                continue;
            for (edgepi = 0; edgepi < pp->edgepl; edgepi++) {
                ep = pp->edgeps[edgepi];
                if (ep->p0p == pp)
                    ep->npanxxlocl = pointi;
                if (ep->p1p == pp)
                    ep->npanxxlocr = pointi;
            }
            if (++count % 1000 == 0)
                SUmessage (
                    1, "markallpolys", "%d done, %d left", count, pointn
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
                (pp->edgeps[0]->npanxxlocl >= 0 ||
                pp->edgeps[0]->statel != states[pointi])) ||
                (pp->edgeps[0]->p1p == pp &&
                (pp->edgeps[0]->npanxxlocr >= 0 ||
                pp->edgeps[0]->stater != states[pointi])))
            )
                continue;
            mind = (double) INT_MAX, minpointi = -1;
            for (pointi = 0; pointi < pointn; pointi++)
                if ((d = dist2poly (pp, points[pointi])) < mind)
                    mind = d, minpointi = pointi;
            if (minpointi == -1)
                continue;
            for (edgepi = 0; edgepi < pp->edgepl; edgepi++) {
                ep = pp->edgeps[edgepi];
                if (ep->p0p == pp)
                    ep->npanxxlocl = minpointi;
                if (ep->p1p == pp)
                    ep->npanxxlocr = minpointi;
            }
            count++;
            SUmessage (1, "markallpolys", "found nearest point");
            break;
        }
    }
    SUmessage (1, "markallpolys", "%d done, %d left", count, pointn);
    return 0;
}

static int propagatebyedge (void) {
    int edgepi;
    edge_t *e0p, *e1p;
    int count;

    static int mark;

    SUmessage (1, "propagatebyedge", "propagating npanxxlocs by edge");
    mark++;
    count = 0;
    for (
        e0p = (edge_t *) dtflatten (edgedict); e0p;
        e0p = (edge_t *) dtlink (edgedict, e0p)
    ) {
        if (e0p->npanxxlocl >= 0 && e0p->p0p->mark != mark) {
            if (e0p->npanxxlocr == -1 && e0p->p1p && e0p->p1p != e0p->p0p) {
                count++;
                for (edgepi = 0; edgepi < e0p->p1p->edgepl; edgepi++) {
                    e1p = e0p->p1p->edgeps[edgepi];
                    if (e1p->p0p == e0p->p1p)
                        e1p->npanxxlocl = e0p->npanxxlocl;
                    if (e1p->p1p == e0p->p1p)
                        e1p->npanxxlocr = e0p->npanxxlocl;
                }
                e0p->p1p->mark = mark;
            }
        }
        if (e0p->npanxxlocr >= 0 && e0p->p1p->mark != mark) {
            if (e0p->npanxxlocl == -1 && e0p->p0p && e0p->p0p != e0p->p1p) {
                count++;
                for (edgepi = 0; edgepi < e0p->p0p->edgepl; edgepi++) {
                    e1p = e0p->p0p->edgeps[edgepi];
                    if (e1p->p0p == e0p->p0p)
                        e1p->npanxxlocl = e0p->npanxxlocr;
                    if (e1p->p1p == e0p->p0p)
                        e1p->npanxxlocr = e0p->npanxxlocr;
                }
                e0p->p0p->mark = mark;
            }
        }
    }
    SUmessage (1, "propagatebyedge", "assigned npanxxlocs to %d polys", count);
    return count;
}

static int pointinpoly (poly_t *pp, xy_t xy) {
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
        if (maxxy.x < xy.x || minxy.y > xy.y || maxxy.y <= xy.y)
            continue;
        if (
            ((xyb.x - xya.x) / (double) (xyb.y - xya.y)) *
            (xy.y - xya.y) + xya.x < xy.x
        )
            continue;
        n++;
    }
    if (n % 2 == 1)
        return TRUE;
    return FALSE;
}

static double dist2poly (poly_t *pp, xy_t xy) {
    xy_t xya, xyb;
    int edgepi;
    double mind, d;

    mind = (double) INT_MAX;
    for (edgepi = 0; edgepi < pp->edgepl; edgepi++) {
        xya = pp->edgeps[edgepi]->v0p->xy;
        xyb = pp->edgeps[edgepi]->v1p->xy;
        if (
            (d = sqrt ((xya.x - xy.x) * (xya.x - xy.x) +
            (xya.y - xy.y) * (xya.y - xy.y))) < mind
        )
            mind = d;
        if (
            (d = sqrt ((xyb.x - xy.x) * (xyb.x - xy.x) +
            (xyb.y - xy.y) * (xyb.y - xy.y))) < mind
        )
            mind = d;
    }
    return mind;
}
