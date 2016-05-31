#pragma prototyped

#include <ast.h>
#include <vmalloc.h>
#include <cdt.h>
#include <swift.h>
#include "tiger.h"

static Dtdisc_t stringdisc = {
    sizeof (Dtlink_t), -1, 0, NULL, NULL, NULL, NULL, NULL, NULL
};

static Dtdisc_t vertexdisc = {
    sizeof (Dtlink_t), sizeof (xy_t), 0, NULL, NULL, NULL, NULL, NULL, NULL
};

static Dtdisc_t edgedisc = {
    sizeof (Dtlink_t), sizeof (int), 0, NULL, NULL, NULL, NULL, NULL, NULL
};

static Dtdisc_t polydisc = {
    sizeof (Dtlink_t), 2 * sizeof (int), 0, NULL, NULL, NULL, NULL, NULL, NULL
};

Dt_t *stringdict, *vertexdict, *edgedict, *polydict;

static Vmalloc_t *svm, *vvm, *evm, *pvm;

int initrecords (void) {
    if (
        !(svm = vmopen (Vmdcheap, Vmbest, 0)) ||
        !(vvm = vmopen (Vmdcheap, Vmpool, 0)) ||
        !(evm = vmopen (Vmdcheap, Vmpool, 0)) ||
        !(pvm = vmopen (Vmdcheap, Vmpool, 0))
    ) {
        SUwarning (1, "initrecords", "vmopens failed");
        return -1;
    }
    if (
        !(stringdict = dtopen (&stringdisc, Dtset)) ||
        !(vertexdict = dtopen (&vertexdisc, Dtset)) ||
        !(edgedict = dtopen (&edgedisc, Dtset)) ||
        !(polydict = dtopen (&polydisc, Dtset))
    ) {
        SUwarning (1, "initrecords", "dtopens failed");
        return -1;
    }
    return 0;
}

string_t *createstring (char *str) {
    string_t *smemp, *sp;
    char *s1, *s2;
    int i;

    for (s2 = &str[strlen (str) - 1]; s2 >= str && *s2 == ' '; s2--)
        ;
    for (s1 = &str[0]; s1 <= s2 && *s1 == ' '; s1++)
        ;
    smemp = sp = NULL;
    if (
        !(smemp = vmalloc (svm, sizeof (string_t))) ||
        !(smemp->str = vmalloc (svm, (s2 - s1 + 2) * sizeof (char)))
    )
        goto abortcreatestring;
    for (i = 0; s1 <= s2; s1++, i++)
        smemp->str[i] = *s1;
    smemp->str[i] = 0;
    smemp->id = dtsize (stringdict);
    smemp->refcount = 1;
    if (!(sp = dtinsert (stringdict, smemp)))
        goto abortcreatestring;
    if (sp != smemp) {
        vmfree (svm, smemp->str);
        vmfree (svm, smemp);
        sp->refcount++;
        return sp;
    }
    return sp;

abortcreatestring:
    SUwarning (1, "createstring", "create failed for string %s", str);
    if (sp)
        if (sp == smemp)
            dtdelete (stringdict, sp);
    if (smemp) {
        if (smemp->str)
            vmfree (svm, smemp->str);
        vmfree (svm, smemp);
    }
    return NULL;
}

string_t *findstring (char *str) {
    string_t *sp;

    if (!(sp = dtmatch (stringdict, str)))
        return NULL;
    return sp;
}

int destroystring (string_t *sp) {
    if (--sp->refcount > 0)
        return 0;
    if (!dtdelete (stringdict, sp)) {
        SUwarning (
            1, "destroystring", "dtdelete failed for string %s", sp->str
        );
        return -1;
    }
    vmfree (svm, sp->str);
    vmfree (svm, sp);
    return 0;
}

vertex_t *createvertex (xy_t xy) {
    vertex_t *vmemp, *vp;

    vmemp = vp = NULL;
    if (!(vmemp = vmalloc (vvm, sizeof (vertex_t))))
        goto abortcreatevertex;
    vmemp->xy = xy;
    if (!(vp = dtinsert (vertexdict, vmemp)))
        goto abortcreatevertex;
    if (vp != vmemp) {
        vmfree (vvm, vmemp);
        return vp;
    }
    if (!(vp->edgeps = vmalloc (Vmheap, 4 * sizeof (edge_t *))))
        goto abortcreatevertex;
    vp->edgepn = 4;
    vp->edgepl = 0;
    return vp;

abortcreatevertex:
    SUwarning (
        1, "createvertex", "create failed for vertex (%d,%d)", xy.x, xy.y
    );
    if (vp) {
        if (vp == vmemp)
            dtdelete (vertexdict, vp);
        if (vp->edgeps)
            vmfree (Vmheap, vp->edgeps);
    }
    if (vmemp)
        vmfree (vvm, vmemp);
    return NULL;
}

vertex_t *findvertex (xy_t xy) {
    vertex_t *vp;

    if (!(vp = dtmatch (vertexdict, &xy)))
        return NULL;
    return vp;
}

int destroyvertex (vertex_t *vp) {
    if (vp->edgepl > 0)
        return 0;
    if (!dtdelete (vertexdict, vp)) {
        SUwarning (
            1, "destroyvertex", "dtdelete failed for vertex (%d,%d)",
            vp->xy.x, vp->xy.y
        );
        return -1;
    }
    vmfree (Vmheap, vp->edgeps);
    vmfree (vvm, vp);
    return 0;
}

edge_t *createedge (edgedata_t *edp, char *name) {
    edge_t *ememp, *ep;
    vertex_t *v0p, *v1p;

    ememp = ep = NULL;
    if (!(ememp = vmalloc (evm, sizeof (edge_t))))
        goto abortcreateedge;
    ememp->tlid = edp->tlid;
    if (!(ep = dtinsert (edgedict, ememp)))
        goto abortcreateedge;
    if (ep != ememp) {
        SUwarning (2, "createedge", "duplicate edge %d", edp->tlid);
        if (!edp->onesided || !ep->onesided) {
            SUwarning (
                1, "createedge", "cannot handle two-sided duplicate edge %d",
                edp->tlid
            );
            return ep;
        }
        ep->onesided = 0;
        if (ep->statel == 0) {
            if (edp->statel != 0) {
                ep->zipl = edp->zipl;
                ep->npanxxlocl = edp->npanxxlocl;
                ep->statel = edp->statel;
                ep->countyl = edp->countyl;
                ep->ctbnal = edp->ctbnal;
                ep->blkl = edp->blkl;
                ep->blksl = edp->blksl;
            } else {
                ep->zipl = edp->zipr;
                ep->npanxxlocl = edp->npanxxlocr;
                ep->statel = edp->stater;
                ep->countyl = edp->countyr;
                ep->ctbnal = edp->ctbnar;
                ep->blkl = edp->blkr;
                ep->blksl = edp->blksr;
            }
        } else {
            if (edp->statel != 0) {
                ep->zipr = edp->zipl;
                ep->npanxxlocr = edp->npanxxlocl;
                ep->stater = edp->statel;
                ep->countyr = edp->countyl;
                ep->ctbnar = edp->ctbnal;
                ep->blkr = edp->blkl;
                ep->blksr = edp->blksl;
            } else {
                ep->zipr = edp->zipr;
                ep->npanxxlocr = edp->npanxxlocr;
                ep->stater = edp->stater;
                ep->countyr = edp->countyr;
                ep->ctbnar = edp->ctbnar;
                ep->blkr = edp->blkr;
                ep->blksr = edp->blksr;
            }
        }
        return ep;
    }
    ep->onesided = edp->onesided;
    ep->v0p = ep->v1p = NULL;
    if (!(ep->sp = createstring (name)))
        goto abortcreateedge;
    ep->cfccid = edp->cfccid;
    ep->zipl = edp->zipl, ep->zipr = edp->zipr;
    ep->npanxxlocl = edp->npanxxlocl, ep->npanxxlocr = edp->npanxxlocr;
    ep->statel = edp->statel, ep->stater = edp->stater;
    ep->countyl = edp->countyl, ep->countyr = edp->countyr;
    ep->ctbnal = edp->ctbnal, ep->ctbnar = edp->ctbnar;
    ep->blkl = edp->blkl, ep->blkr = edp->blkr;
    ep->blksl = edp->blksl, ep->blksr = edp->blksr;
    if (
        !(v0p = createvertex (edp->xy0)) || !(v1p = createvertex (edp->xy1)) ||
        linkedgenvertices (ep, v0p, v1p) == -1
    )
        goto abortcreateedge;
    ep->p0p = ep->p1p = NULL;
    ep->p0i = ep->p1i = -1;
    ep->xys = NULL;
    ep->xyn = 0;
    ep->mark = 0;
    return ep;

abortcreateedge:
    SUwarning (1, "createedge", "create failed for edge %d", edp->tlid);
    if (ep) {
        if (ep == ememp)
            dtdelete (edgedict, ep);
        if (ep->sp)
            destroystring (ep->sp);
        if (ep->v0p || ep->v1p)
            unlinkedgenvertices (ep);
        if (ep->v0p)
            destroyvertex (ep->v0p);
        if (ep->v1p)
            destroyvertex (ep->v1p);
    }
    if (ememp)
        vmfree (evm, ememp);
    return NULL;
}

edge_t *findedge (int tlid) {
    edge_t *ep;

    if (!(ep = dtmatch (edgedict, &tlid)))
        return NULL;
    return ep;
}

int attachshapexys (edge_t *ep, xy_t *xys, int xyn) {
    int xyi;

    if (ep->xys) {
        SUwarning (1, "attachshapexys", "edge %d has shape xys", ep->tlid);
        return 1;
    }
    if (!(ep->xys = vmalloc (Vmheap, xyn * sizeof (xy_t)))) {
        SUwarning (1, "attachshapexys", "vmalloc failed for xys");
        return -1;
    }
    ep->xyn = xyn;
    for (xyi = 0; xyi < xyn; xyi++)
        ep->xys[xyi] = xys[xyi];
    return 0;
}

int destroyedge (edge_t *ep) {
    vertex_t *v0p, *v1p;
    poly_t *p0p, *p1p;

    if (!dtdelete (edgedict, ep)) {
        SUwarning (1, "destroyedge", "dtdelete failed for ep");
        return -1;
    }
    if (destroystring (ep->sp) == -1) {
        SUwarning (1, "destroyedge", "destroystring failed");
        return -1;
    }
    v0p = ep->v0p, v1p = ep->v1p;
    if (unlinkedgenvertices (ep) == -1) {
        SUwarning (1, "destroyedge", "unlinkedgenvertices failed");
        return -1;
    }
    if (
        destroyvertex (v0p) == -1 || (v0p != v1p && destroyvertex (v1p) == -1)
    ) {
        SUwarning (1, "destroyedge", "destroyvertex's failed");
        return -1;
    }
    p0p = ep->p0p, p1p = ep->p1p;
    if (unlinkedgenpolys (ep) == -1) {
        SUwarning (1, "destroyedge", "unlinkedgenpolys failed");
        return -1;
    }
    if (
        (p0p && destroypoly (p0p) == -1) ||
        (p1p && p0p != p1p && destroypoly (p1p) == -1)
    ) {
        SUwarning (1, "destroyedge", "destroypolys failed");
        return -1;
    }
    if (ep->xys)
        vmfree (Vmheap, ep->xys);
    vmfree (evm, ep);
    return 0;
}

int linkedgenvertices (edge_t *ep, vertex_t *v0p, vertex_t *v1p) {
    if (ep->v0p || ep->v1p) {
        SUwarning (1, "linkedgenvertices", "edge %d has vertices", ep->tlid);
        return -1;
    }
    if (v0p) {
        ep->v0p = v0p;
        if (v0p->edgepl == v0p->edgepn) {
            if (!(v0p->edgeps = vmresize (
                Vmheap, v0p->edgeps,
                (v0p->edgepn * 2) * sizeof (edge_t *), VM_RSCOPY
            ))) {
                SUwarning (1, "linkedgenvertices", "vmresize failed (1)");
                return -1;
            }
            v0p->edgepn *= 2;
        }
        v0p->edgeps[(ep->v0i = v0p->edgepl++)] = ep;
    }
    if (v1p) {
        ep->v1p = v1p;
        if (v1p->edgepl == v1p->edgepn) {
            if (!(v1p->edgeps = vmresize (
                Vmheap, v1p->edgeps,
                (v1p->edgepn * 2) * sizeof (edge_t *), VM_RSCOPY
            ))) {
                SUwarning (1, "linkedgenvertices", "vmresize failed (2)");
                return -1;
            }
            v1p->edgepn *= 2;
        }
        v1p->edgeps[(ep->v1i = v1p->edgepl++)] = ep;
    }
    return 0;
}

int unlinkedgenvertices (edge_t *ep) {
    edge_t *nep;

    if (ep->v0p) {
        nep = ep->v0p->edgeps[ep->v0i] = ep->v0p->edgeps[--ep->v0p->edgepl];
        if (nep->v0p == ep->v0p && nep->v0i == ep->v0p->edgepl)
            nep->v0i = ep->v0i;
        else
            nep->v1i = ep->v0i;
    }
    if (ep->v1p) {
        nep = ep->v1p->edgeps[ep->v1i] = ep->v1p->edgeps[--ep->v1p->edgepl];
        if (nep->v0p == ep->v1p && nep->v0i == ep->v1p->edgepl)
            nep->v0i = ep->v1i;
        else
            nep->v1i = ep->v1i;
    }
    ep->v0p = ep->v1p = NULL;
    ep->v0i = ep->v1i = -1;
    return 0;
}

int spliceedges (vertex_t *vp) {
    edge_t *e0p, *e1p;
    vertex_t *vtp;
    int vti;
    xy_t *xys;
    int xyn, xyi, xyj;

    if (vp->edgepl != 2 || (e0p = vp->edgeps[0]) == (e1p = vp->edgeps[1])) {
        SUwarning (
            1, "spliceedges", "vertex (%d,%d) has wrong number of edges",
            vp->xy.x, vp->xy.y
        );
        return -1;
    }
    xyn = e0p->xyn + e1p->xyn + 1;
    if (!(xys = vmalloc (Vmheap, xyn * sizeof (xy_t)))) {
        SUwarning (1, "spliceedges", "vmalloc failed for xys");
        return -1;
    }
    xyi = 0;
    if (e0p->v1p == vp) {
        for (xyj = 0; xyj < e0p->xyn; xyj++)
            xys[xyi++] = e0p->xys[xyj];
        xys[xyi++] = vp->xy;
        if (e1p->v0p == vp)
            for (xyj = 0; xyj < e1p->xyn; xyj++)
                xys[xyi++] = e1p->xys[xyj];
        else
            for (xyj = 0; xyj < e1p->xyn; xyj++)
                xys[xyi++] = e1p->xys[e1p->xyn - xyj - 1];
    } else {
        if (e1p->v1p == vp)
            for (xyj = 0; xyj < e1p->xyn; xyj++)
                xys[xyi++] = e1p->xys[xyj];
        else
            for (xyj = 0; xyj < e1p->xyn; xyj++)
                xys[xyi++] = e1p->xys[e1p->xyn - xyj - 1];
        xys[xyi++] = vp->xy;
        for (xyj = 0; xyj < e0p->xyn; xyj++)
            xys[xyi++] = e0p->xys[xyj];
    }
    if (e0p->xys)
        vmfree (Vmheap, e0p->xys);
    e0p->xys = xys;
    e0p->xyn = xyn;
    if (e0p->v1p == vp) {
        if (e1p->v0p == vp) {
            e0p->v1p->edgeps[e0p->v1i] = e1p;
            e1p->v1p->edgeps[e1p->v1i] = e0p;
            vtp = e0p->v1p, e0p->v1p = e1p->v1p, e1p->v1p = vtp;
            vti = e0p->v1i, e0p->v1i = e1p->v1i, e1p->v1i = vti;
        } else {
            e0p->v1p->edgeps[e0p->v1i] = e1p;
            e1p->v0p->edgeps[e1p->v0i] = e0p;
            vtp = e0p->v1p, e0p->v1p = e1p->v0p, e1p->v0p = vtp;
            vti = e0p->v1i, e0p->v1i = e1p->v0i, e1p->v0i = vti;
        }
    } else {
        if (e1p->v1p == vp) {
            e0p->v0p->edgeps[e0p->v0i] = e1p;
            e1p->v0p->edgeps[e1p->v0i] = e0p;
            vtp = e0p->v0p, e0p->v0p = e1p->v0p, e1p->v0p = vtp;
            vti = e0p->v0i, e0p->v0i = e1p->v0i, e1p->v0i = vti;
        } else {
            e0p->v0p->edgeps[e0p->v0i] = e1p;
            e1p->v1p->edgeps[e1p->v1i] = e0p;
            vtp = e0p->v0p, e0p->v0p = e1p->v1p, e1p->v1p = vtp;
            vti = e0p->v0i, e0p->v0i = e1p->v1i, e1p->v1i = vti;
        }
    }
    destroyedge (e1p);
    return 0;
}

poly_t *createpoly (polydata_t *pdp) {
    poly_t *pmemp, *pp;

    pmemp = pp = NULL;
    if (!(pmemp = vmalloc (pvm, sizeof (poly_t))))
        goto abortcreatepoly;
    pmemp->cenid = pdp->cenid;
    pmemp->polyid = pdp->polyid;
    if (!(pp = dtinsert (polydict, pmemp)))
        goto abortcreatepoly;
    if (pp != pmemp) {
        vmfree (pvm, pmemp);
        return pp;
    }
    if (!(pp->edgeps = vmalloc (Vmheap, 4 * sizeof (edge_t *))))
        goto abortcreatepoly;
    pp->edgepn = 4;
    pp->edgepl = 0;
    pp->mark = 0;
    return pp;

abortcreatepoly:
    SUwarning (
        1, "createpoly", "create failed for poly %d/%d", pdp->cenid, pdp->polyid
    );
    if (pp) {
        if (pp == pmemp)
            dtdelete (polydict, pp);
        if (pp->edgeps)
            vmfree (Vmheap, pp->edgeps);
    }
    if (pmemp)
        vmfree (pvm, pmemp);
    return NULL;
}

poly_t *findpoly (int polyid, int cenid) {
    poly_t pmem, *pp;

    pmem.cenid = cenid;
    pmem.polyid = polyid;
    if (!(pp = dtsearch (polydict, &pmem)))
        return NULL;
    return pp;
}

int destroypoly (poly_t *pp) {
    if (pp->edgepl > 0)
        return 0;
    if (!dtdelete (polydict, pp)) {
        SUwarning (
            1, "destroypoly", "dtdelete failed for poly %d/%d",
            pp->cenid, pp->polyid
        );
        return -1;
    }
    vmfree (Vmheap, pp->edgeps);
    vmfree (pvm, pp);
    return 0;
}

int linkedgenpolys (edge2polydata_t *e2pdp) {
    edge_t *ep;
    poly_t *p0p, *p1p, *pp;

    if (!(ep = findedge (e2pdp->tlid))) {
        SUwarning (1, "linkedgenpolys", "findedge failed");
        return -1;
    }
    p0p = p1p = NULL;
    if (e2pdp->polyidl && !(p0p = findpoly (e2pdp->polyidl, e2pdp->cenidl))) {
        SUwarning (1, "linkedgenpolys", "findpoly failed for left poly");
        return -1;
    }
    if (e2pdp->polyidr && !(p1p = findpoly (e2pdp->polyidr, e2pdp->cenidr))) {
        SUwarning (1, "linkedgenpolys", "findpoly failed for right poly");
        return -1;
    }
    if (
        (ep->p0p && p0p && !ep->p1p && !p1p) ||
        (!ep->p0p && !p0p && ep->p1p && p1p)
    )
        pp = p0p, p0p = p1p, p1p = pp;
    if ((ep->p0p && p0p) || (ep->p1p && p1p))
        SUwarning (1, "linkedgenpolys", "edge has polys attached");
    if (p0p && !ep->p0p) {
        ep->p0p = p0p;
        if (p0p->edgepl == p0p->edgepn) {
            if (!(p0p->edgeps = vmresize (
                Vmheap, p0p->edgeps,
                (p0p->edgepn * 2) * sizeof (edge_t *), VM_RSCOPY
            ))) {
                SUwarning (1, "linkedgenpolys", "vmresize failed (1)");
                return -1;
            }
            p0p->edgepn *= 2;
        }
        p0p->edgeps[(ep->p0i = p0p->edgepl++)] = ep;
    }
    if (p1p && !ep->p1p) {
        ep->p1p = p1p;
        if (p1p->edgepl == p1p->edgepn) {
            if (!(p1p->edgeps = vmresize (
                Vmheap, p1p->edgeps,
                (p1p->edgepn * 2) * sizeof (edge_t *), VM_RSCOPY
            ))) {
                SUwarning (1, "linkedgenpolys", "vmresize failed (2)");
                return -1;
            }
            p1p->edgepn *= 2;
        }
        p1p->edgeps[(ep->p1i = p1p->edgepl++)] = ep;
    }
    return 0;
}

int unlinkedgenpolys (edge_t *ep) {
    edge_t *nep;

    if (ep->p0p) {
        nep = ep->p0p->edgeps[ep->p0i] = ep->p0p->edgeps[--ep->p0p->edgepl];
        if (nep->p0p == ep->p0p && nep->p0i == ep->p0p->edgepl)
            nep->p0i = ep->p0i;
        else
            nep->p1i = ep->p0i;
    }
    if (ep->p1p) {
        nep = ep->p1p->edgeps[ep->p1i] = ep->p1p->edgeps[--ep->p1p->edgepl];
        if (nep->p0p == ep->p1p && nep->p0i == ep->p1p->edgepl)
            nep->p0i = ep->p1i;
        else
            nep->p1i = ep->p1i;
    }
    ep->p0p = ep->p1p = NULL;
    ep->p0i = ep->p1i = -1;
    return 0;
}

int unlinkpolynedges (poly_t *pp) {
    edge_t *ep;
    int edgepi;

    for (edgepi = pp->edgepl - 1; edgepi >= 0; edgepi--) {
        ep = pp->edgeps[edgepi];
        if (ep->p0p == pp && ep->p0i == edgepi) {
            ep->p0p = NULL, ep->p0i = -1;
            ep->zipl = 0;
            ep->npanxxlocl = -1;
            ep->statel = 0;
            ep->countyl = 0;
            ep->ctbnal = 0;
            ep->blkl = 0;
            ep->blksl = 32;
        }
        if (ep->p1p == pp && ep->p1i == edgepi) {
            ep->p1p = NULL, ep->p1i = -1;
            ep->zipr = 0;
            ep->npanxxlocr = -1;
            ep->stater = 0;
            ep->countyr = 0;
            ep->ctbnar = 0;
            ep->blkr = 0;
            ep->blksr = 32;
        }
        ep->onesided = TRUE;
    }
    pp->edgepl = 0;
    return 0;
}

int orderedges (poly_t *pp) {
    vertex_t *vp;
    edge_t *e1p, *e2p, *e3p;
    int edgep1i, edgep2i, polyn;

    polyn = 0;
    for (vp = NULL, edgep1i = 0; edgep1i < pp->edgepl; edgep1i++) {
        e1p = pp->edgeps[edgep1i];
        if (!vp) {
            polyn++;
            if (e1p->p0p == pp)
                vp = e1p->v1p;
            else
                vp = e1p->v0p;
        }
        for (e3p = NULL, edgep2i = 0; edgep2i < vp->edgepl; edgep2i++) {
            if (
                (e2p = vp->edgeps[edgep2i]) == e1p ||
                ((e2p->p0p != pp || e2p->p0i <= edgep1i) &&
                (e2p->p1p != pp || e2p->p1i <= edgep1i))
            )
                continue;
            e3p = e2p;
            if (e3p->p0p == e3p->p1p)
                break;
        }
        vp = NULL;
        if (!e3p || edgep1i + 1 == pp->edgepl)
            continue;
        e2p = pp->edgeps[edgep1i + 1];
        if (e3p->p0p == pp && e3p->p0i > edgep1i) {
            pp->edgeps[edgep1i + 1] = e3p;
            pp->edgeps[e3p->p0i] = e2p;
            if (e2p->p0p == pp && e2p->p0i == edgep1i + 1)
                e2p->p0i = e3p->p0i, e3p->p0i = edgep1i + 1;
            else
                e2p->p1i = e3p->p0i, e3p->p0i = edgep1i + 1;
            vp = e3p->v1p;
        } else if (e3p->p1p == pp && e3p->p1i > edgep1i) {
            pp->edgeps[edgep1i + 1] = e3p;
            pp->edgeps[e3p->p1i] = e2p;
            if (e2p->p0p == pp && e2p->p0i == edgep1i + 1)
                e2p->p0i = e3p->p1i, e3p->p1i = edgep1i + 1;
            else
                e2p->p1i = e3p->p1i, e3p->p1i = edgep1i + 1;
            vp = e3p->v0p;
        }
    }
    return polyn;
}

int mergeedges (edge_t *e0p, edge_t *e1p) {
    int zip, npanxxloc, ctbna;
    short county, blk;
    char state, blks;
    poly_t *pp;
    int pi;

    if (!e0p->onesided || !e1p->onesided) {
        SUwarning (
            1, "mergeedges", "cannot merge two onesided edge(s) %d %d",
            e0p->tlid, e1p->tlid
        );
        return -1;
    }
    /* e[01]p->p[01]p should be symetric so replace e1p with e0p */
    if (e1p->p1p) {
        e1p->p1p->edgeps[e1p->p1i] = e0p;
        pi = e1p->p1i, pp = e1p->p1p;
        zip = e1p->zipr, npanxxloc = e1p->npanxxlocr, state = e1p->stater;
        county = e1p->countyr, ctbna = e1p->ctbnar;
        blk = e1p->blkr, blks = e1p->blksr;
    } else {
        e1p->p0p->edgeps[e1p->p0i] = e0p;
        pi = e1p->p0i, pp = e1p->p0p;
        zip = e1p->zipl, npanxxloc = e1p->npanxxlocl, state = e1p->statel;
        county = e1p->countyl, ctbna = e1p->ctbnal;
        blk = e1p->blkl, blks = e1p->blksl;
    }
    if (e0p->p0p) {
        e0p->p1p = pp, e0p->p1i = pi;
        pp->edgeps[pi] = e0p;
        e0p->zipr = zip, e0p->npanxxlocr = npanxxloc, e0p->stater = state;
        e0p->countyr = county, e0p->ctbnar = ctbna;
        e0p->blkr = blk, e0p->blksr = blks;
    } else {
        e0p->p0p = pp, e0p->p0i = pi;
        pp->edgeps[pi] = e0p;
        e0p->zipl = zip, e0p->npanxxlocl = npanxxloc, e0p->statel = state;
        e0p->countyl = county, e0p->ctbnal = ctbna;
        e0p->blkl = blk, e0p->blksl = blks;
    }
    e0p->onesided = 0;
    e1p->p0p = e1p->p1p = NULL;
    e1p->p0i = e1p->p1i = -1;
    destroyedge (e1p);
    return 0;
}

int mergepolys (edge_t *mep) {
    int edgepi;
    edge_t *ep;
    poly_t *p0p, *p1p;

    if (!mep->p0p || !mep->p1p) {
        SUwarning (
            1, "mergepolys", "edge %d does not have both poly pointers",
            mep->tlid
        );
        return -1;
    }
    if (mep->p0p != mep->p1p) {
        if (mep->p1p->edgepl > mep->p0p->edgepl)
            p0p = mep->p1p, p1p = mep->p0p;
        else
            p0p = mep->p0p, p1p = mep->p1p;
        if (p0p->edgepl + p1p->edgepl > p0p->edgepn) {
            if (!(p0p->edgeps = vmresize (
                Vmheap, p0p->edgeps, (p0p->edgepl +
                p1p->edgepl) * sizeof (edge_t *), VM_RSCOPY
            ))) {
                SUwarning (1, "mergepolys", "vmresize failed");
                return -1;
            }
            p0p->edgepn = p0p->edgepl + p1p->edgepl;
        }
        for (edgepi = 0; edgepi < p1p->edgepl; edgepi++) {
            ep = p1p->edgeps[edgepi];
            if (ep->p0p == p1p)
                ep->p0p = p0p, ep->p0i = p0p->edgepl;
            else if (ep->p1p == p1p)
                ep->p1p = p0p, ep->p1i = p0p->edgepl;
            p0p->edgeps[p0p->edgepl++] = ep;
        }
        p1p->edgepl = 0;
        destroypoly (p1p);
    }
    destroyedge (mep);
    return 0;
}

int checkrecords (void) {
    vertex_t *vp;
    edge_t *ep;
    poly_t *pp;
    int edgepi;

    SUmessage (0, "checkrecords", "checking data structure consistency");
    for (
        vp = (vertex_t *) dtflatten (vertexdict); vp;
        vp = (vertex_t *) dtlink (vertexdict, vp)
    ) {
        for (edgepi = 0; edgepi < vp->edgepl; edgepi++) {
            ep = vp->edgeps[edgepi];
            if (ep->v0p != vp && ep->v1p != vp)
                SUerror (
                    "checkrecords", "bad vertex list pointer: %x, %x - %x",
                    ep->v0p, ep->v1p, vp
                );
            if (ep->v0i != edgepi && ep->v1i != edgepi)
                SUerror (
                    "checkrecords", "bad vertex list index: %d, %d - %d",
                    ep->v0i, ep->v1i, edgepi
                );
        }
    }
    for (
        ep = (edge_t *) dtflatten (edgedict); ep;
        ep = (edge_t *) dtlink (edgedict, ep)
    ) {
        if (
            !((ep->v0p && ep->v0p->edgeps[ep->v0i] == ep) ||
            (!ep->v0p && ep->v0i == -1))
        )
            SUerror (
                "checkrecords", "vertex error: %d, %d", ep->tlid, ep->v0i
            );
        if (
            !((ep->v1p && ep->v1p->edgeps[ep->v1i] == ep) ||
            (!ep->v1p && ep->v1i == -1))
        )
            SUerror (
                "checkrecords", "vertex error: %d, %d", ep->tlid, ep->v1i
            );
        if (
            !((ep->p0p && ep->p0p->edgeps[ep->p0i] == ep) ||
            (!ep->p0p && ep->p0i == -1))
        )
            SUerror (
                "checkrecords", "poly error: %d, %d", ep->tlid, ep->p0i
            );
        if (
            !((ep->p1p && ep->p1p->edgeps[ep->p1i] == ep) ||
            (!ep->p1p && ep->p1i == -1))
        )
            SUerror (
                "checkrecords", "poly error: %d, %d", ep->tlid, ep->p1i
            );
    }
    for (
        pp = (poly_t *) dtflatten (polydict); pp;
        pp = (poly_t *) dtlink (polydict, pp)
    ) {
        for (edgepi = 0; edgepi < pp->edgepl; edgepi++) {
            ep = pp->edgeps[edgepi];
            if (ep->p0p != pp && ep->p1p != pp)
                SUerror (
                    "checkrecords", "bad poly list pointer: %x, %x - %x",
                    ep->p0p, ep->p1p, pp
                );
            if (ep->p0i != edgepi && ep->p1i != edgepi)
                SUerror (
                    "checkrecords", "bad poly list pointer: %d, %d - %d",
                    ep->p0i, ep->p1i, edgepi
                );
        }
    }
    return 0;
}

void printrecords (void) {
    vertex_t *vp;
    edge_t *ep;
    poly_t *pp;
    int ei;

    SUmessage (1, "printrecords", "printing data structures");
    for (
        vp = (vertex_t *) dtflatten (vertexdict); vp;
        vp = (vertex_t *) dtlink (vertexdict, vp)
    ) {
        sfprintf (sfstdout, "vertex: %x %d %d\n", vp, vp->xy.x, vp->xy.y);
        for (ei = 0; ei < vp->edgepl; ei++)
            sfprintf (sfstdout, "\tedge %d\n", vp->edgeps[ei]->tlid);
    }
    for (
        ep = (edge_t *) dtflatten (edgedict); ep;
        ep = (edge_t *) dtlink (edgedict, ep)
    ) {
        sfprintf (
            sfstdout, "edge: %d %s %s\n", ep->tlid, ep->sp->str,
            cfccid2str (ep->cfccid)
        );
        sfprintf (sfstdout, "\tzip %d %d\n", ep->zipl, ep->zipr);
        sfprintf (
            sfstdout, "\tnpanxxloc %d %d\n", ep->npanxxlocl, ep->npanxxlocr
        );
        sfprintf (sfstdout, "\tstate %d %d\n", ep->statel, ep->stater);
        sfprintf (sfstdout, "\tcounty %d %d\n", ep->countyl, ep->countyr);
        sfprintf (sfstdout, "\tctbna %d %d\n", ep->ctbnal, ep->ctbnar);
        sfprintf (sfstdout, "\tblk %d %d\n", ep->blkl, ep->blkr);
        sfprintf (sfstdout, "\tblks %d %d\n", ep->blksl, ep->blksr);
        sfprintf (
            sfstdout, "\tvertices %x/%d %x/%d\n",
            ep->v0p, ep->v0i, ep->v1p, ep->v1i
        );
        if (ep->p0p)
            sfprintf (
                sfstdout, "\tl poly %d-%d/%d\n",
                ep->p0p->cenid, ep->p0p->polyid, ep->p0i
            );
        if (ep->p1p)
            sfprintf (
                sfstdout, "\tr poly %d-%d/%d\n",
                ep->p1p->cenid, ep->p1p->polyid, ep->p1i
            );
    }
    for (
        pp = (poly_t *) dtflatten (polydict); pp;
        pp = (poly_t *) dtlink (polydict, pp)
    ) {
        sfprintf (sfstdout, "poly: %d-%d\n", pp->cenid, pp->polyid);
        for (ei = 0; ei < pp->edgepl; ei++)
            sfprintf (sfstdout, "\tedge %d\n", pp->edgeps[ei]->tlid);
    }
}

void printstats (void) {
    SUmessage (
        1, "printstats", "%d strings, %d vertices, %d edges, %d polys",
        dtsize (stringdict), dtsize (vertexdict),
        dtsize (edgedict), dtsize (polydict)
    );
}
