#pragma prototyped

#include <ast.h>
#include <option.h>
#include <tok.h>
#include <cdt.h>
#include <math.h>
#include <swift.h>
#include "tiger.h"

static const char usage[] =
"[-1p1s1D?\n@(#)$Id: tiger2off (AT&T Labs Research) 2001-09-24 $\n]"
USAGE_LICENSE
"[+NAME?tiger2off - generate OFF format geometry]"
"[+DESCRIPTION?\btiger2off\b generates OFF geometry files from the"
" specified dataset."
" It can generate up to four types of geometry: points, vertical lines,"
" polygon outlines, and filled polygons."
"]"
"[100:i?specifies the directory containing the input files."
" The default is the current directory."
"]:[inputdir]"
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

#define INDINCR    1
#define POINTINCR 10
#define TRIINCR   10

typedef struct point_t {
    float x, y;
} point_t;

typedef struct tri_t {
    point_t a, b, c;
} tri_t;

typedef struct vi_t {
    Dtlink_t link;
    /* begin key */
    xy_t xy;
    /* end key */
    int id;
} vi_t;

static Dtdisc_t vidisc = {
    sizeof (Dtlink_t), sizeof (xy_t), 0, NULL, NULL, NULL, NULL, NULL, NULL
};

static Dt_t *vidict;

static int savemask;

static int saveoff (int);

int main (int argc, char **argv) {
    int norun;
    char *startbrk, *endbrk;
    char *indir;
    int attr;
    int splicemode;
    float digits;

    startbrk = (char *) sbrk (0);
    indir = ".";
    attr = T_EATTR_STATE;
    splicemode = 1;
    digits = 6;
    savemask = 15;
    norun = 0;
    for (;;) {
        switch (optget (argv, usage)) {
        case -100:
            indir = opt_info.arg;
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
                SUerror ("tiger2off", "bad argument for -a option");
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
            SUusage (0, "tiger2off", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "tiger2off", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;

    initrecords ();
    if (!(vidict = dtopen (&vidisc, Dtset)))
        SUerror ("tiger2off", "dtopen failed for vidict");
    if (loaddata (indir) == -1)
        SUerror ("tiger2off", "loaddata failed");
    if (splicemode > 0 && removedegree2vertices (splicemode) == -1)
        SUerror ("tiger2off", "removedegree2vertices failed");
    if (digits >= 0 && simplifyxys (digits) == -1)
        SUerror ("tiger2off", "simplifyxys failed");
    if (saveoff (attr) == -1)
        SUerror ("tiger2off", "genpoints failed");
    printstats ();
    endbrk = (char *) sbrk (0);
    SUmessage (1, "tiger2off", "memory usage %d", endbrk - startbrk);
    return 0;
}

static int saveoff (int attr) {
    Sfio_t *fp1, *fp2;
    poly_t *pp;
    char *as;
    int zip, npanxxloc, ctbna;
    short county, blk;
    char state, blks;
    int pn;
    int vn;
    vertex_t *vp;
    char *s;
    int n;
    vi_t *vimem, *vip;
    edge_t *ep, *e1p, *e2p;
    int edgepi;
    vertex_t *v1p, *v2p;
    int xyi;

    static int id;

    if (!(fp1 = sfopen (NULL, NULL, "sw+"))) {
        SUwarning (1, "genpoints", "cannot create string stream");
        return -1;
    }
    if (!(fp2 = sfopen (NULL, NULL, "sw+"))) {
        SUwarning (1, "genpoints", "cannot create string stream");
        return -1;
    }
    vn = 0;
    for (
        vp = (vertex_t *) dtflatten (vertexdict); vp;
        vp = (vertex_t *) dtlink (vertexdict, vp)
    ) {
        if (!(vimem = malloc (sizeof (vi_t)))) {
            SUwarning (1, "genpoints", "malloc failed for vimem");
            return -1;
        }
        vimem->xy = vp->xy;
        if (!(vip = dtinsert (vidict, vimem))) {
            SUwarning (1, "genpoints", "dtinsert failed");
            return -1;
        }
        if (vip == vimem) {
            vip->id = id++;
            sfprintf (
                fp1, "%f %f\n", vp->xy.x / 1000000.0, vp->xy.y / 1000000.0
            );
            vn++;
        }
    }
    for (
        ep = (edge_t *) dtflatten (edgedict); ep;
        ep = (edge_t *) dtlink (edgedict, ep)
    ) {
        for (xyi = 0; xyi < ep->xyn; xyi++) {
            if (!(vimem = malloc (sizeof (vi_t)))) {
                SUwarning (1, "genpoints", "malloc failed for vimem");
                return -1;
            }
            vimem->xy = ep->xys[xyi];
            if (!(vip = dtinsert (vidict, vimem))) {
                SUwarning (1, "genpoints", "dtinsert failed");
                return -1;
            }
            if (vip == vimem) {
                vip->id = id++;
                sfprintf (
                    fp1, "%f %f\n",
                    ep->xys[xyi].x / 1000000.0, ep->xys[xyi].y / 1000000.0
                );
                vn++;
            }
        }
    }
    sfprintf (sfstdout, "%d ", vn);
    pn = 0, vn = 0;
    for (
        pp = (poly_t *) dtflatten (polydict); pp;
        pp = (poly_t *) dtlink (polydict, pp)
    ) {
        if (pp->edgepl == 0)
            continue;
        orderedges (pp);
        as = NULL;
        if (attr > -1) {
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
            switch (attr) {
            case T_EATTR_BLKS:
                if ((state > 0) && (county > 0) && (ctbna > 0) && (blk > 0))
                    as = sfprints (
                        "%02d%03d%06d%03d%c", state, county, ctbna, blk, blks
                    );
                break;
            case T_EATTR_BLK:
                if ((state > 0) && (county > 0) && (ctbna > 0) && (blk > 0))
                    as = sfprints (
                        "%02d%03d%06d%03d", state, county, ctbna, blk
                    );
                break;
            case T_EATTR_BLKG:
                if ((state > 0) && (county > 0) && (ctbna > 0) && (blk > 0))
                    as = sfprints (
                        "%02d%03d%06d%d", state, county, ctbna, blk / 100
                    );
                break;
            case T_EATTR_CTBNA:
                if ((state > 0) && (county > 0) && (ctbna > 0))
                    as = sfprints ("%02d%03d%06d", state, county, ctbna);
                break;
            case T_EATTR_COUNTY:
                if ((state > 0) && (county > 0))
                    as = sfprints ("%02d%03d", state, county);
                break;
            case T_EATTR_STATE:
                if ((state > 0))
                    as = sfprints ("%02d", state);
                break;
            case T_EATTR_ZIP:
                if ((zip > 0))
                    as = sfprints ("%05d", zip);
                break;
            case T_EATTR_NPANXXLOC:
                if ((npanxxloc > -1))
                    as = sfprints ("%d", npanxxloc);
                break;
            case T_EATTR_COUNTRY:
                break;
            }
        }
        for (v1p = NULL, edgepi = 0; edgepi < pp->edgepl; edgepi++) {
            e1p = pp->edgeps[edgepi];
            if (!v1p) {
                if (vn > 2) {
                    if (
                        (n = sfseek (fp2, 0, SEEK_CUR)) == -1 ||
                        sfseek (fp2, 0, SEEK_SET) != 0 ||
                        !(s = sfreserve (fp2, n, 0))
                    ) {
                        SUwarning (1, "genpoints", "rewind failed (1)");
                        return -1;
                    }
                    if (as)
                        sfprintf (fp1, "%s ", as);
                    sfprintf (fp1, "%d", vn);
                    sfwrite (fp1, s, n);
                    sfprintf (fp1, "\n");
                    pn++;
                }
                if (sfseek (fp2, 0, SEEK_SET) != 0) {
                    SUwarning (1, "genpoints", "rewind failed (2)");
                    return -1;
                }
                vn = 0;
                if (e1p->p0p == pp)
                    v1p = e1p->v0p, v2p = e1p->v1p;
                else
                    v1p = e1p->v1p, v2p = e1p->v0p;
            }
            vn++;
            if (!(vip = dtmatch (vidict, &v1p->xy))) {
                SUwarning (1, "genpoints", "dtmatch failed (1)");
                return -1;
            }
            sfprintf (fp2, " %d", vip->id);
            if (v1p == e1p->v0p) {
                for (xyi = 0; xyi < e1p->xyn; xyi++) {
                    vn++;
                    if (!(vip = dtmatch (vidict, &e1p->xys[xyi]))) {
                        SUwarning (1, "genpoints", "dtmatch failed (2)");
                        return -1;
                    }
                    sfprintf (fp2, " %d", vip->id);
                }
            } else {
                for (xyi = e1p->xyn - 1; xyi >= 0; xyi--) {
                    vn++;
                    if (!(vip = dtmatch (vidict, &e1p->xys[xyi]))) {
                        SUwarning (1, "genpoints", "dtmatch failed (3)");
                        return -1;
                    }
                    sfprintf (fp2, " %d", vip->id);
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
        }
        if (vn > 2) {
            if (
                (n = sfseek (fp2, 0, SEEK_CUR)) == -1 ||
                sfseek (fp2, 0, SEEK_SET) != 0 ||
                !(s = sfreserve (fp2, n, 0))
            ) {
                SUwarning (1, "genpoints", "rewind failed (3)");
                return -1;
            }
            if (as)
                sfprintf (fp1, "%s ", as);
            sfprintf (fp1, "%d", vn);
            sfwrite (fp1, s, n);
            sfprintf (fp1, "\n");
            pn++;
        }
        if (sfseek (fp2, 0, SEEK_SET) != 0) {
            SUwarning (1, "genpoints", "rewind failed (4)");
            return -1;
        }
        vn = 0;
    }
    sfclose (fp2);
    if (
        (n = sfseek (fp1, 0, SEEK_CUR)) == -1 ||
        sfseek (fp1, 0, SEEK_SET) != 0 ||
        !(s = sfreserve (fp1, n, 0))
    ) {
        SUwarning (1, "genpoints", "rewind failed (3)");
        return -1;
    }
    sfprintf (sfstdout, "%d 12\n", pn);
    sfwrite (sfstdout, s, n);
    sfclose (fp1);
    return 0;
}
