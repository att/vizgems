#pragma prototyped

#include <ast.h>
#include <vmalloc.h>
#include <cdt.h>
#include <math.h>
#include <swift.h>
#include "tiger.h"

typedef struct geo2poly_t {
    Dtlink_t link;
    /* begin key */
    int zip, npanxxloc, ctbna;
    short county, blk;
    char state, blkg, blks;
    /* end key */
    poly_t *pp;
} geo2poly_t;

static Dtdisc_t geo2polydisc = {
    sizeof (Dtlink_t),
    3 * sizeof (int) + 2 * sizeof (short) + 3 * sizeof (char),
    0, NULL, NULL, NULL, NULL, NULL, NULL
};

static Dt_t *geo2polydict;

static geo2poly_t emptyg2p = {
    { NULL, { 0 } }, -1, -1, -1, -1, -1, -1, -1, -1, NULL
};

static double *ts;
static int tn;

static geo2poly_t *creategeo2poly (geo2poly_t *);
static geo2poly_t *findgeo2poly (geo2poly_t *);
static int destroygeo2poly (geo2poly_t *);
static void recsimplify (xy_t, xy_t, xy_t *, int, int, double *, double, int *);
static int maxdist (xy_t, xy_t, xy_t *, int, int, double *, double);
static double dist (xy_t, xy_t);

int mergeonesidededges (void) {
    edge_t **edgeps;
    int edgepn, edgepi, edgepj;
    edge_t *e0p, *e1p;
    int e1i;
    int count;

    SUmessage (1, "mergeonesidededges", "merging onesided edges");
    count = 0;
    for (
        edgepn = 0, e0p = (edge_t *) dtflatten (edgedict); e0p;
        e0p = (edge_t *) dtlink (edgedict, e0p)
    )
        if (e0p->onesided)
            edgepn++;
    if (!(edgeps = vmalloc (Vmheap, sizeof (edge_t *) * edgepn))) {
        SUwarning (1, "mergeonesidededges", "vmalloc failed");
        return -1;
    }
    for (
        edgepi = 0, e0p = (edge_t *) dtflatten (edgedict); e0p;
        e0p = (edge_t *) dtlink (edgedict, e0p)
    )
        if (e0p->onesided)
            edgeps[edgepi++] = e0p;
    for (edgepi = 0; edgepi < edgepn; edgepi++) {
        if (!(e0p = edgeps[edgepi]))
            continue;
        for (e1i = 0; e1i < e0p->v0p->edgepl; e1i++) {
            e1p = e0p->v0p->edgeps[e1i];
            if (e0p == e1p || !e1p->onesided)
                continue;
            if (
                (e1p->v0p == e0p->v0p && e1p->v1p == e0p->v1p) ||
                (e1p->v0p == e0p->v1p && e1p->v1p == e0p->v0p)
            ) {
                mergeedges (e0p, e1p);
                if (++count % 1000 == 0)
                    SUmessage (
                        1, "mergeonesidededges", "%d done, %d left",
                        count, dtsize (edgedict)
                    );
                for (edgepj = edgepi + 1; edgepj < edgepn; edgepj++)
                    if (edgeps[edgepj] == e1p) {
                        edgeps[edgepj] = NULL;
                        break;
                    }
            }
        }
    }
    vmfree (Vmheap, edgeps);
    SUmessage (
        1, "mergeonesidededges", "%d done, %d left", count, dtsize (edgedict)
    );
    return 0;
}

int removedegree2vertices (int mode) {
    vertex_t **vertexps;
    int vertexpn, vertexpi;
    vertex_t *vp;
    int count;

    SUmessage (1, "removedegree2vertices", "joining edges");
    count = 0;
    vertexpn = dtsize (vertexdict);
    if (!(vertexps = vmalloc (Vmheap, sizeof (vertex_t *) * vertexpn))) {
        SUwarning (1, "removedegree2vertices", "vmalloc failed");
        return -1;
    }
    for (
        vertexpi = 0, vp = (vertex_t *) dtflatten (vertexdict); vp;
        vp = (vertex_t *) dtlink (vertexdict, vp)
    )
        vertexps[vertexpi++] = vp;
    for (vertexpi = 0; vertexpi < vertexpn; vertexpi++) {
        vp = vertexps[vertexpi];
        if (
            vp->edgepl == 2 && vp->edgeps[0] != vp->edgeps[1] && (
                mode == 2 ||
                (!vp->edgeps[0]->onesided && !vp->edgeps[1]->onesided)
            )
        ) {
            if (spliceedges (vp) == -1) {
                SUwarning (
                    1, "removedegree2vertices", "spliceedges failed for %d %d",
                    vp->edgeps[0]->tlid, vp->edgeps[1]->tlid
                );
                return -1;
            }
            if (++count % 10000 == 0)
                SUmessage (
                    1, "removedegree2vertices", "%d done, %d left",
                    count, dtsize (vertexdict)
                );
        }
    }
    vmfree (Vmheap, vertexps);
    SUmessage (
        1, "removedegree2vertices", "%d done, %d left",
        count, dtsize (vertexdict)
    );
    return 0;
}

int mergepolysbyattr (int attr) {
    edge_t **edgeps;
    int edgepn, edgepi;
    edge_t *ep;
    int v;
    int count;

    SUmessage (1, "mergepolysbyattr", "merging polys by attribute");
    count = 0;
    edgepn = dtsize (edgedict);
    if (!(edgeps = vmalloc (Vmheap, sizeof (edge_t *) * edgepn))) {
        SUwarning (1, "mergepolysbyattr", "vmalloc failed");
        return -1;
    }
    for (
        edgepi = 0, ep = (edge_t *) dtflatten (edgedict); ep;
        edgepi++, ep = (edge_t *) dtlink (edgedict, ep)
    )
        edgeps[edgepi] = ep;
    for (edgepi = 0; edgepi < edgepn; edgepi++) {
        ep = edgeps[edgepi];
        if (ep->onesided)
            continue;
        switch (attr) {
        case T_EATTR_BLKS:
            v = (
                (ep->blksl == ep->blksr) && (ep->blkl == ep->blkr) &&
                (ep->ctbnal == ep->ctbnar) &&
                (ep->countyl == ep->countyr) && (ep->statel == ep->stater)
            );
            break;
        case T_EATTR_BLK:
            v = (
                (ep->blkl == ep->blkr) && (ep->ctbnal == ep->ctbnar) &&
                (ep->countyl == ep->countyr) && (ep->statel == ep->stater)
            );
            break;
        case T_EATTR_BLKG:
            v = (
                (ep->blkl / 100 == ep->blkr / 100) &&
                (ep->ctbnal == ep->ctbnar) &&
                (ep->countyl == ep->countyr) && (ep->statel == ep->stater)
            );
            break;
        case T_EATTR_CTBNA:
            v = (
                (ep->ctbnal == ep->ctbnar) &&
                (ep->countyl == ep->countyr) && (ep->statel == ep->stater)
            );
            break;
        case T_EATTR_COUNTY:
            v = (ep->countyl == ep->countyr) && (ep->statel == ep->stater);
            break;
        case T_EATTR_STATE:
            v = (ep->statel == ep->stater);
            break;
        case T_EATTR_ZIP:
            v = (ep->zipl == ep->zipr);
            break;
        case T_EATTR_NPANXXLOC:
            v = (ep->npanxxlocl == ep->npanxxlocr);
            break;
        case T_EATTR_COUNTRY:
            v = (!ep->onesided);
            break; /* bogus */
        }
        if (!v)
            continue;
        mergepolys (ep);
        if (++count % 10000 == 0)
            SUmessage (
                1, "mergepolysbyattr", "%d done, %d left",
                count, dtsize (edgedict)
            );
    }
    vmfree (Vmheap, edgeps);
    SUmessage (
        1, "mergepolysbyattr", "%d done, %d left", count, dtsize (edgedict)
    );
    return 0;
}

int buildpolysbyattr (int attr) {
    edge_t **edgeps;
    int edgepn, edgepi;
    edge_t *ep;
    geo2poly_t g2pd, *g2p0p, *g2p1p;
    polydata_t pd;
    edge2polydata_t e2pd;
    int noleftside, norightside;
    geo2poly_t **g2pps, *g2pp;
    int g2ppn, g2ppi;
    int count;

    SUmessage (1, "buildpolysbyattr", "building polys by attribute");
    count = 0;
    pd.polyid = 1, pd.cenid = 1;
    if (!(geo2polydict = dtopen (&geo2polydisc, Dtset))) {
        SUwarning (1, "buildpolysbyattr", "dtopen failed");
        return -1;
    }
    for (
        ep = (edge_t *) dtflatten (edgedict); ep;
        ep = (edge_t *) dtlink (edgedict, ep)
    )
        if (ep->p0p || ep->p1p)
            unlinkedgenpolys (ep);
    while (dtsize (polydict) > 0)
        destroypoly (dtfirst (polydict));
    edgepn = dtsize (edgedict);
    if (!(edgeps = vmalloc (Vmheap, sizeof (edge_t *) * edgepn))) {
        SUwarning (1, "buildpolysbyattr", "vmalloc failed for edgeps");
        return -1;
    }
    for (
        edgepi = 0, ep = (edge_t *) dtflatten (edgedict); ep;
        edgepi++, ep = (edge_t *) dtlink (edgedict, ep)
    )
        edgeps[edgepi] = ep;
    for (edgepi = 0; edgepi < edgepn; edgepi++) {
        ep = edgeps[edgepi];
        e2pd.tlid = ep->tlid;
        noleftside = norightside = FALSE;
        if (ep->statel == 0)
            noleftside = TRUE;
        else if (ep->stater == 0)
            norightside = TRUE;
        g2pd = emptyg2p;
        switch (attr) {
        case T_EATTR_BLKS:
            g2pd.blks = ep->blksl, g2pd.blk = ep->blkl;
            g2pd.ctbna = ep->ctbnal, g2pd.county = ep->countyl;
            g2pd.state = ep->statel;
            break;
        case T_EATTR_BLK:
            g2pd.blk = ep->blkl, g2pd.ctbna = ep->ctbnal;
            g2pd.county = ep->countyl, g2pd.state = ep->statel;
            break;
        case T_EATTR_BLKG:
            g2pd.blkg = ep->blkl / 100, g2pd.ctbna = ep->ctbnal;
            g2pd.county = ep->countyl, g2pd.state = ep->statel;
            break;
        case T_EATTR_CTBNA:
            g2pd.ctbna = ep->ctbnal, g2pd.county = ep->countyl;
            g2pd.state = ep->statel;
            break;
        case T_EATTR_COUNTY:
            g2pd.county = ep->countyl, g2pd.state = ep->statel;
            break;
        case T_EATTR_STATE:
            g2pd.state = ep->statel;
            break;
        case T_EATTR_ZIP:
            g2pd.zip = ep->zipl;
            break;
        case T_EATTR_NPANXXLOC:
            g2pd.npanxxloc = ep->npanxxlocl;
            break;
        case T_EATTR_COUNTRY:
            break;
        }
        if (!noleftside) {
            if (!(g2p0p = findgeo2poly (&g2pd))) {
                if (!(g2p0p = creategeo2poly (&g2pd))) {
                    SUwarning (
                        1, "buildpolysbyattr", "creategeo2poly failed (1)"
                    );
                    return -1;
                }
                pd.cenid = ep->statel * 1000 + ep->countyl;
                pd.polyid++;
                if (!(g2p0p->pp = createpoly (&pd))) {
                    SUwarning (1, "buildpolysbyattr", "createpoly failed (1)");
                    return -1;
                }
            }
            e2pd.cenidl = g2p0p->pp->cenid, e2pd.polyidl = g2p0p->pp->polyid;
        } else
            e2pd.cenidl = 0, e2pd.polyidl = 0;
        g2pd = emptyg2p;
        switch (attr) {
        case T_EATTR_BLKS:
            g2pd.blks = ep->blksr, g2pd.blk = ep->blkr;
            g2pd.ctbna = ep->ctbnar, g2pd.county = ep->countyr;
            g2pd.state = ep->stater;
            break;
        case T_EATTR_BLK:
            g2pd.blk = ep->blkr, g2pd.ctbna = ep->ctbnar;
            g2pd.county = ep->countyr, g2pd.state = ep->stater;
            break;
        case T_EATTR_BLKG:
            g2pd.blkg = ep->blkr / 100, g2pd.ctbna = ep->ctbnar;
            g2pd.county = ep->countyr, g2pd.state = ep->stater;
            break;
        case T_EATTR_CTBNA:
            g2pd.ctbna = ep->ctbnar, g2pd.county = ep->countyr;
            g2pd.state = ep->stater;
            break;
        case T_EATTR_COUNTY:
            g2pd.county = ep->countyr, g2pd.state = ep->stater;
            break;
        case T_EATTR_STATE:
            g2pd.state = ep->stater;
            break;
        case T_EATTR_ZIP:
            g2pd.zip = ep->zipr;
            break;
        case T_EATTR_NPANXXLOC:
            g2pd.npanxxloc = ep->npanxxlocr;
            break;
        case T_EATTR_COUNTRY:
            break;
        }
        if (!norightside) {
            if (!(g2p1p = findgeo2poly (&g2pd))) {
                if (!(g2p1p = creategeo2poly (&g2pd))) {
                    SUwarning (
                        1, "buildpolysbyattr", "creategeo2poly failed (2)"
                    );
                    return -1;
                }
                pd.cenid = ep->stater * 1000 + ep->countyr;
                pd.polyid++;
                if (!(g2p1p->pp = createpoly (&pd))) {
                    SUwarning (1, "buildpolysbyattr", "createpoly failed (2)");
                    return -1;
                }
            }
            e2pd.cenidr = g2p1p->pp->cenid, e2pd.polyidr = g2p1p->pp->polyid;
        } else
            e2pd.cenidr = 0, e2pd.polyidr = 0;
        if (linkedgenpolys (&e2pd) == -1) {
            SUwarning (1, "buildpolysbyattr", "linkedgenpolys failed");
            return -1;
        }
        if (++count % 10000 == 0)
            SUmessage (
                1, "buildpolysbyattr", "%d edges done, %d left",
                count, dtsize (edgedict)
            );
    }
    vmfree (Vmheap, edgeps);
    g2ppn = dtsize (geo2polydict);
    if (!(g2pps = vmalloc (Vmheap, sizeof (geo2poly_t *) * g2ppn))) {
        SUwarning (1, "buildpolysbyattr", "vmalloc failed for g2pps");
        return -1;
    }
    for (
        g2ppi = 0, g2pp = (geo2poly_t *) dtflatten (geo2polydict); g2pp;
        g2ppi++, g2pp = (geo2poly_t *) dtlink (geo2polydict, g2pp)
    )
        g2pps[g2ppi] = g2pp;
    for (g2ppi = 0; g2ppi < g2ppn; g2ppi++)
        destroygeo2poly (g2pps[g2ppi]);
    vmfree (Vmheap, g2pps);
    dtclose (geo2polydict);
    SUmessage (
        1, "buildpolysbyattr", "%d edges done, %d left",
        count, dtsize (edgedict)
    );
    return 0;
}

int removezeroedges (void) {
    edge_t **edgeps;
    int edgepn, edgepi;
    edge_t *ep;
    poly_t *pp;
    int count;

    SUmessage (1, "removezeroedges", "removing zero ctbna and blk edges");
    count = 0;
    edgepn = dtsize (edgedict);
    if (!(edgeps = vmalloc (Vmheap, sizeof (edge_t *) * edgepn))) {
        SUwarning (1, "removezeroedges", "vmalloc failed for edgeps");
        return -1;
    }
    for (
        edgepi = 0, ep = (edge_t *) dtflatten (edgedict); ep;
        edgepi++, ep = (edge_t *) dtlink (edgedict, ep)
    )
        edgeps[edgepi] = ep;
again:
    for (edgepi = 0; edgepi < edgepn; edgepi++) {
        ep = edgeps[edgepi];
        if (ep->p0p && ep->ctbnal == 0) {
            pp = ep->p0p;
            unlinkpolynedges (pp), destroypoly (pp);
            goto again;
        }
        if (ep->p1p && ep->ctbnar == 0) {
            pp = ep->p1p;
            unlinkpolynedges (pp), destroypoly (pp);
            goto again;
        }
    }
    for (edgepi = 0; edgepi < edgepn; edgepi++) {
        ep = edgeps[edgepi];
        if (!ep->p0p && !ep->p1p)
            destroyedge (ep), count++;
    }
    vmfree (Vmheap, edgeps);
    SUmessage (
        1, "removezeroedges", "%d edges done, %d left",
        count, dtsize (edgedict)
    );
    return 0;
}

int extractbyattr (int attr, int val) {
    edge_t **edgeps;
    int edgepn, edgepi;
    edge_t *ep;
    poly_t *pp;
    int v;
    int count;

    SUmessage (1, "extractbyattr", "extracting by attribute");
    count = 0;
    edgepn = dtsize (edgedict);
    if (!(edgeps = vmalloc (Vmheap, sizeof (edge_t *) * edgepn))) {
        SUwarning (1, "extractbyattr", "vmalloc failed");
        return -1;
    }
    for (
        edgepi = 0, ep = (edge_t *) dtflatten (edgedict); ep;
        edgepi++, ep = (edge_t *) dtlink (edgedict, ep)
    )
        edgeps[edgepi] = ep;
    for (edgepi = 0; edgepi < edgepn; edgepi++) {
        ep = edgeps[edgepi];
        v = 0;
        switch (attr) {
        case T_EATTR_BLKS:
            if (ep->blksl == val)
                v += 1;
            if (ep->blksr == val)
                v += 2;
            break;
        case T_EATTR_BLK:
            if (ep->blkl == val)
                v += 1;
            if (ep->blkr == val)
                v += 2;
            break;
        case T_EATTR_BLKG:
            if (ep->blkl / 100 == val)
                v += 1;
            if (ep->blkr / 100 == val)
                v += 2;
            break;
        case T_EATTR_CTBNA:
            if (ep->ctbnal == val)
                v += 1;
            if (ep->ctbnar == val)
                v += 2;
            break;
        case T_EATTR_COUNTY:
            if (ep->countyl == val)
                v += 1;
            if (ep->countyr == val)
                v += 2;
            break;
        case T_EATTR_STATE:
            if (ep->statel == val)
                v += 1;
            if (ep->stater == val)
                v += 2;
            break;
        case T_EATTR_ZIP:
            if (ep->zipl == val)
                v += 1;
            if (ep->zipr == val)
                v += 2;
            break;
        case T_EATTR_NPANXXLOC:
            if (ep->npanxxlocl == val)
                v += 1;
            if (ep->npanxxlocr == val)
                v += 2;
            break;
        case T_EATTR_COUNTRY:
            if (!ep->onesided)
                v += 3;
            break; /* bogus */
        case T_EATTR_CFCC:
            if (ep->cfccid / 256 == val / 256 && ep->cfccid / 10 <= val / 10)
                v += 3;
            break;
        }
        if (v == 0)
            destroyedge (ep);
        else {
            if (v == 1 && ep->p1p)
                pp = ep->p1p, unlinkpolynedges (pp), destroypoly (pp);
            else if (v == 2 && ep->p0p)
                pp = ep->p0p, unlinkpolynedges (pp), destroypoly (pp);
            if (++count % 1000 == 0)
                SUmessage (
                    1, "extractbyattr", "%d edges done, %d left",
                    count, dtsize (edgedict)
                );
        }
    }
    vmfree (Vmheap, edgeps);
    SUmessage (
        1, "extractbyattr", "%d edges done, %d left", count, dtsize (edgedict)
    );
    return 0;
}

int simplifyxys (float digits) {
    edge_t *ep;
    int xyi, xyn;
    int count;
    double acc;

    SUmessage (1, "simplifyxys", "simplifying shape xys");
    if (digits < 0) {
        SUwarning (1, "simplifyxys", "negative number of digits (%d)", digits);
        return -1;
    }
    count = 0;
    acc = 0.0;
    if (digits > 0.0)
        acc = pow (10.0, (digits - 1.0));
    for (
        ep = (edge_t *) dtflatten (edgedict); ep;
        ep = (edge_t *) dtlink (edgedict, ep)
    ) {
        if (ep->xyn == 0)
            continue;
        if (tn < ep->xyn + 1) {
            if (!(ts = vmresize (
                Vmheap, ts, (ep->xyn + 1) * sizeof (double), VM_RSMOVE
            ))) {
                SUwarning (1, "simplifyxys", "vmresize failed");
                return -1;
            }
            tn = ep->xyn + 1;
        }
        ts[0] = dist (ep->v0p->xy, ep->xys[0]);
        for (xyi = 0; xyi < ep->xyn - 1; xyi++)
            ts[xyi + 1] = dist (ep->xys[xyi], ep->xys[xyi + 1]);
        ts[ep->xyn] = dist (ep->xys[ep->xyn - 1], ep->v1p->xy);

        xyn = 0;
        recsimplify (
            ep->v0p->xy, ep->v1p->xy, ep->xys, 0, ep->xyn - 1, ts, acc, &xyn
        );
        count += (ep->xyn - xyn);
        SUmessage (2, "simplifyxys", "eliminated %d points", count);
        ep->xyn = xyn;
        if (ep->xyn == 0)
            vmfree (Vmheap, ep->xys), ep->xys = NULL, ep->xyn = 0;
    }
    SUmessage (1, "simplifyxys", "eliminated %d points", count);
    return 0;
}

#define X(x) ((x) - minx) / (double) (maxx - minx)
#define Y(y) ((y) - miny) / (double) (maxy - miny)

void printps (int showshapexys) {
    edge_t *ep;
    int minx, miny, maxx, maxy;
    int xyi;

    minx = miny = INT_MAX;
    maxx = maxy = INT_MIN;
    for (
        ep = (edge_t *) dtflatten (edgedict); ep;
        ep = (edge_t *) dtlink (edgedict, ep)
    ) {
        if (minx > ep->v0p->xy.x)
            minx = ep->v0p->xy.x;
        if (miny > ep->v0p->xy.y)
            miny = ep->v0p->xy.y;
        if (minx > ep->v1p->xy.x)
            minx = ep->v1p->xy.x;
        if (miny > ep->v1p->xy.y)
            miny = ep->v1p->xy.y;
        if (maxx < ep->v0p->xy.x)
            maxx = ep->v0p->xy.x;
        if (maxy < ep->v0p->xy.y)
            maxy = ep->v0p->xy.y;
        if (maxx < ep->v1p->xy.x)
            maxx = ep->v1p->xy.x;
        if (maxy < ep->v1p->xy.y)
            maxy = ep->v1p->xy.y;
        if (showshapexys) {
            for (xyi = 0; xyi < ep->xyn; xyi++) {
                if (minx > ep->xys[xyi].x)
                    minx = ep->xys[xyi].x;
                if (miny > ep->xys[xyi].y)
                    miny = ep->xys[xyi].y;
                if (maxx < ep->xys[xyi].x)
                    maxx = ep->xys[xyi].x;
                if (maxy < ep->xys[xyi].y)
                    maxy = ep->xys[xyi].y;
            }
        }
    }
    sfprintf (sfstdout, "%%!PS\n");
    sfprintf (
        sfstdout, "%%Area: %f %f %f %f\n", minx / 1000000.0, miny / 1000000.0,
        maxx / 1000000.0, maxy / 1000000.0
    );
    sfprintf (sfstdout, "8 72 mul 10 72 mul scale 0 setlinewidth\n");
    sfprintf (sfstdout, "/n { newpath } def\n");
    sfprintf (sfstdout, "/m { moveto } def\n");
    sfprintf (sfstdout, "/l { lineto } def\n");
    sfprintf (sfstdout, "/s { stroke } def\n");
    if (showshapexys) {
        for (
            ep = (edge_t *) dtflatten (edgedict); ep;
            ep = (edge_t *) dtlink (edgedict, ep)
        ) {
            sfprintf (
                sfstdout, "n %f %f m\n", X (ep->v0p->xy.x), Y (ep->v0p->xy.y)
            );
            for (xyi = 0; xyi < ep->xyn; xyi++)
                sfprintf (
                    sfstdout, "%f %f l\n",
                    X (ep->xys[xyi].x), Y (ep->xys[xyi].y)
                );
            sfprintf (
                sfstdout, "%f %f l s\n", X (ep->v1p->xy.x), Y (ep->v1p->xy.y)
            );
        }
    } else {
        for (
            ep = (edge_t *) dtflatten (edgedict); ep;
            ep = (edge_t *) dtlink (edgedict, ep)
        ) {
            sfprintf (
                sfstdout, "n %f %f m %f %f l s\n",
                X (ep->v0p->xy.x), Y (ep->v0p->xy.y),
                X (ep->v1p->xy.x), Y (ep->v1p->xy.y)
            );
        }
    }
    sfprintf (sfstdout, "showpage\n");
}

int cfccstr2id (char *cfcc) {
    return (cfcc[0] - 'A') * 256 + atoi (&cfcc[1]);
}

int cenidstr2id (char *cenid) {
    return (cenid[0] - 'A') * 10000 + atoi (&cenid[1]);
}

char *cfccid2str (int cfccid) {
    return sfprints ("%c%d", 'A' + cfccid / 256, cfccid % 256);
}

char *cenidid2str (int cenid) {
    return sfprints ("%c%d", 'A' + cenid / 10000, cenid % 10000);
}

static geo2poly_t *creategeo2poly (geo2poly_t *g2pdatap) {
    geo2poly_t *g2pmemp, *g2pp;

    g2pmemp = g2pp = NULL;
    if (!(g2pmemp = vmalloc (Vmheap, sizeof (geo2poly_t))))
        goto abortcreategeo2poly;
    g2pmemp->state = g2pdatap->state;
    g2pmemp->county = g2pdatap->county;
    g2pmemp->ctbna = g2pdatap->ctbna;
    g2pmemp->blkg = g2pdatap->blkg;
    g2pmemp->blk = g2pdatap->blk;
    g2pmemp->blks = g2pdatap->blks;
    g2pmemp->zip = g2pdatap->zip;
    g2pmemp->npanxxloc = g2pdatap->npanxxloc;
    if (!(g2pp = dtinsert (geo2polydict, g2pmemp)))
        goto abortcreategeo2poly;
    if (g2pp != g2pmemp) {
        vmfree (Vmheap, g2pmemp);
        return g2pp;
    }
    g2pp->pp = NULL;
    return g2pp;

abortcreategeo2poly:
    SUwarning (1, "creategeo2poly", "create failed");
    if (g2pp) {
        if (g2pp == g2pmemp)
            dtdelete (geo2polydict, g2pp);
    }
    if (g2pmemp)
        vmfree (Vmheap, g2pmemp);
    return NULL;
}

static geo2poly_t *findgeo2poly (geo2poly_t *g2pdatap) {
    geo2poly_t g2pmem, *g2pp;

    g2pmem.state = g2pdatap->state;
    g2pmem.county = g2pdatap->county;
    g2pmem.ctbna = g2pdatap->ctbna;
    g2pmem.blkg = g2pdatap->blkg;
    g2pmem.blk = g2pdatap->blk;
    g2pmem.blks = g2pdatap->blks;
    g2pmem.zip = g2pdatap->zip;
    g2pmem.npanxxloc = g2pdatap->npanxxloc;
    if (!(g2pp = dtsearch (geo2polydict, &g2pmem)))
        return NULL;
    return g2pp;
}

static int destroygeo2poly (geo2poly_t *g2pp) {
    if (!dtdelete (geo2polydict, g2pp)) {
        SUwarning (1, "destroygeo2poly", "dtdelete failed");
        return -1;
    }
    vmfree (Vmheap, g2pp);
    return 0;
}

static void recsimplify (
    xy_t xy0, xy_t xy1, xy_t *xys, int xyfi, int xyli,
    double *ts, double mind, int *xyip
) {
    int xymi;

    if ((xymi = maxdist (xy0, xy1, xys, xyfi, xyli, ts, mind)) == -1)
        return;
    if (xymi > xyfi)
        recsimplify (xy0, xys[xymi], xys, xyfi, xymi - 1, ts, mind, xyip);
    xys[*xyip] = xys[xymi], (*xyip)++;
    if (xyli > xymi)
        recsimplify (xys[xymi], xy1, xys, xymi + 1, xyli, ts, mind, xyip);
}

static int maxdist (
    xy_t xy0, xy_t xy1, xy_t *xys, int xyfi, int xyli, double *ts, double mind
) {
    double totl, l, t, dx, dy, d;
    xy_t xyt;
    int xyi, mini;

    dx = xy1.x - xy0.x, dy = xy1.y - xy0.y;
    totl = ts[xyfi];
    for (xyi = xyfi; xyi < xyli; xyi++)
        totl += ts[xyi + 1];
    totl += ts[xyli + 1];
    mini = -1;
    l = ts[xyfi];
    t = l / totl;
    xyt.x = xy0.x + dx * t, xyt.y = xy0.y + dy * t;
    if ((d = dist (xys[xyfi], xyt)) > mind)
        mind = d, mini = xyfi;
    for (xyi = xyfi; xyi < xyli; xyi++) {
        l += ts[xyi + 1];
        t = l / totl;
        xyt.x = xy0.x + dx * t, xyt.y = xy0.y + dy * t;
        if ((d = dist (xys[xyi + 1], xyt)) > mind)
            mind = d, mini = xyi;
    }
    return mini;
}

static double dist (xy_t xya, xy_t xyb) {
    double ax, ay, bx, by;
    ax = xya.x / 1000000.0, ay = xya.y / 1000000.0;
    bx = xyb.x / 1000000.0, by = xyb.y / 1000000.0;
    return 1000000.0 * sqrt (
        (bx - ax) * (bx - ax) + (by - ay) * (by - ay)
    );
}
