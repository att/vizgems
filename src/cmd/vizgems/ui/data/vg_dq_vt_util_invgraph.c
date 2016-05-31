#include <ast.h>
#include <swift.h>
#include "vg_dq_vt_util.h"

int IGRccexists, IGRclexists, IGRrkexists, IGRndexists, IGRedexists;

IGRcc_t **IGRccs;
int IGRccn;
IGRcl_t **IGRcls;
int IGRcln;
IGRrk_t **IGRrks;
int IGRrkn;
IGRnd_t **IGRnds, **IGRwnds;
int IGRndn;
IGRed_t **IGReds;
int IGRedn;

static char *pagemode;
static int pageindex, pagei, pagem, pageemit;
static char pagebuf[1000];

static Dt_t *ccdict;
static Dtdisc_t ccdisc = {
    sizeof (Dtlink_t), SZ_level + SZ_id, 0, NULL, NULL, NULL, NULL, NULL, NULL
};
static Dt_t *cldict;
static Dtdisc_t cldisc = {
    sizeof (Dtlink_t), SZ_level + SZ_id, 0, NULL, NULL, NULL, NULL, NULL, NULL
};
static Dt_t *rkdict;
static Dtdisc_t rkdisc = {
    sizeof (Dtlink_t), SZ_level + SZ_id, 0, NULL, NULL, NULL, NULL, NULL, NULL
};
static Dt_t *nddict;
static Dtdisc_t nddisc = {
    sizeof (Dtlink_t), SZ_level + SZ_id, 0, NULL, NULL, NULL, NULL, NULL, NULL
};
static Dt_t *eddict;
static Dtdisc_t eddisc = {
    sizeof (Dtlink_t), sizeof (IGRnd_t *) * 2,
    0, NULL, NULL, NULL, NULL, NULL, NULL
};

#define MW 4

static int width, height;

static Agsym_t *gvsyms[UT_ATTRGROUP_SIZE][UT_ATTR_SIZE];

static GVC_t *gvc;
static Agraph_t *rootgp;
static int gid, nid;

#if !_BLD_swift && defined(__IMPORT__)
#define __PUBLIC_DATA__ __IMPORT__
#else
#define __PUBLIC_DATA__
#endif

extern __PUBLIC_DATA__ gvplugin_library_t gvplugin_dot_layout_LTX_library;
extern __PUBLIC_DATA__ gvplugin_library_t gvplugin_neato_layout_LTX_library;
extern __PUBLIC_DATA__ gvplugin_library_t gvplugin_gd_LTX_library;
extern __PUBLIC_DATA__ gvplugin_library_t gvplugin_core_LTX_library;
static lt_symlist_t lt_symlist[5];

#undef __PUBLIC_DATA__

static RI_t *rip;

static int debugmode;

static int getattrs (Agraph_t *, Agnode_t *, Agedge_t *);
static int cmp (const void *, const void *);

int IGRinit (char *pm) {
    if (!(ccdict = dtopen (&ccdisc, Dtset))) {
        SUwarning (0, "IGRinit", "cannot create ccdict");
        return -1;
    }
    if (!(cldict = dtopen (&cldisc, Dtset))) {
        SUwarning (0, "IGRinit", "cannot create cldict");
        return -1;
    }
    if (!(rkdict = dtopen (&rkdisc, Dtset))) {
        SUwarning (0, "IGRinit", "cannot create rkdict");
        return -1;
    }
    if (!(nddict = dtopen (&nddisc, Dtset))) {
        SUwarning (0, "IGRinit", "cannot create nddict");
        return -1;
    }
    if (!(eddict = dtopen (&eddisc, Dtset))) {
        SUwarning (0, "IGRinit", "cannot create eddict");
        return -1;
    }
    IGRccs = NULL;
    IGRccn = 0;
    IGRcls = NULL;
    IGRcln = 0;
    IGRrks = NULL;
    IGRrkn = 0;
    IGRnds = IGRwnds = NULL;
    IGRndn = 0;
    IGReds = NULL;
    IGRedn = 0;

    pagemode = pm;
    pageindex = pagei = pagem = 0;
    if (pagemode && pagemode[0] && strcmp (pagemode, "all") != 0)
        pagem = atoi (pagemode);
    if (pagem < 0)
        pagem = 0;

    UTattrgroups[UT_ATTRGROUP_GRAPH][UT_ATTR_BB].name = "bb";
    UTattrgroups[UT_ATTRGROUP_GRAPH][UT_ATTR_LP].name = "lp";
    UTattrgroups[UT_ATTRGROUP_GRAPH][UT_ATTR_LWIDTH].name = "lwidth";
    UTattrgroups[UT_ATTRGROUP_GRAPH][UT_ATTR_LHEIGHT].name = "lheight";
    UTattrgroups[UT_ATTRGROUP_GRAPH][UT_ATTR_COLOR].name = "color";
    UTattrgroups[UT_ATTRGROUP_GRAPH][UT_ATTR_FILLCOLOR].name = "fillcolor";
    UTattrgroups[UT_ATTRGROUP_GRAPH][UT_ATTR_BGCOLOR].name = "bgcolor";
    UTattrgroups[UT_ATTRGROUP_GRAPH][UT_ATTR_FONTNAME].name = "fontname";
    UTattrgroups[UT_ATTRGROUP_GRAPH][UT_ATTR_FONTSIZE].name = "fontsize";
    UTattrgroups[UT_ATTRGROUP_GRAPH][UT_ATTR_FONTCOLOR].name = "fontcolor";
    UTattrgroups[UT_ATTRGROUP_GRAPH][UT_ATTR_INFO].name = "info";
    UTattrgroups[UT_ATTRGROUP_GRAPH][UT_ATTR_DRAW].name = "_draw_";
    UTattrgroups[UT_ATTRGROUP_GRAPH][UT_ATTR_LDRAW].name = "_ldraw_";

    UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_POS].name = "pos";
    UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_WIDTH].name = "width";
    UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_HEIGHT].name = "height";
    UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_COLOR].name = "color";
    UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_FILLCOLOR].name = "fillcolor";
    UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_FONTNAME].name = "fontname";
    UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_FONTSIZE].name = "fontsize";
    UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_FONTCOLOR].name = "fontcolor";
    UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_INFO].name = "info";
    UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_DRAW].name = "_draw_";
    UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_LDRAW].name = "_ldraw_";

    UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_LP].name = "lp";
    UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_LWIDTH].name = "lwidth";
    UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_LHEIGHT].name = "lheight";
    UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_COLOR].name = "color";
    UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_FONTNAME].name = "fontname";
    UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_FONTSIZE].name = "fontsize";
    UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_FONTCOLOR].name = "fontcolor";
    UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_INFO].name = "info";
    UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_DRAW].name = "_draw_";
    UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_LDRAW].name = "_ldraw_";
    UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_HDRAW].name = "_hdraw_";
    UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_TDRAW].name = "_tdraw_";
    UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_HLDRAW].name = "_hldraw_";
    UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_TLDRAW].name = "_tldraw_";

    lt_symlist[0].name = "gvplugin_dot_layout_LTX_library";
    lt_symlist[0].address = (void *) &gvplugin_dot_layout_LTX_library;
    lt_symlist[1].name = "gvplugin_neato_layout_LTX_library";
    lt_symlist[1].address = (void *) &gvplugin_neato_layout_LTX_library;
    lt_symlist[2].name = "gvplugin_gd_LTX_library";
    lt_symlist[2].address = (void *) &gvplugin_gd_LTX_library;
    lt_symlist[3].name = "gvplugin_core_LTX_library";
    lt_symlist[3].address = (void *) &gvplugin_core_LTX_library;

    aginit ();
    gvc = gvContextPlugins (lt_symlist, 0);

    if (getenv ("DEBUG"))
        debugmode = TRUE;

    return 0;
}

/* graph structure */

IGRcc_t *IGRinsertcc (char *level, char *id) {
    IGRcc_t cc, *ccp, *ccmem;

    memcpy (cc.level, level, SZ_level);
    memcpy (cc.id, id, SZ_id);
    IGRccexists = TRUE;
    if (!(ccp = dtsearch (ccdict, &cc))) {
        IGRccexists = FALSE;
        if (!(ccmem = vmalloc (Vmheap, sizeof (IGRcc_t)))) {
            SUwarning (0, "IGRinsertcc", "cannot allocate ccmem");
            return NULL;
        }
        memset (ccmem, 0, sizeof (IGRcc_t));
        memcpy (ccmem->level, level, SZ_level);
        memcpy (ccmem->id, id, SZ_id);
        if ((ccp = dtinsert (ccdict, ccmem)) != ccmem) {
            SUwarning (0, "IGRinsertcc", "cannot insert cc");
            vmfree (Vmheap, ccmem);
            return NULL;
        }
    }
    return ccp;
}

IGRcc_t *IGRlinkcc2cc (IGRcc_t *ccp1, IGRcc_t *ccp2) {
    IGRcc_t *ccp, *pccp1, *pccp2;

    if (ccp2 == ccp1)
        return ccp1;

    for (pccp2 = ccp2, ccp = ccp2->lccp; ccp; ccp = ccp->lccp) {
        if (ccp == ccp1)
            return ccp1;
        pccp2 = ccp;
    }
    for (pccp1 = ccp1, ccp = ccp1->lccp; ccp; ccp = ccp->lccp) {
        if (ccp == ccp2)
            return ccp1;
        pccp1 = ccp;
    }
    if (pccp1 == pccp2)
        return ccp1;
    if (pccp1 < pccp2)
        pccp1->lccp = pccp2;
    else
        pccp2->lccp = pccp1;
    return ccp1;
}

IGRcl_t *IGRinsertcl (char *level, char *id, short nclass) {
    IGRcl_t cl, *clp, *clmem;

    memcpy (cl.level, level, SZ_level);
    memcpy (cl.id, id, SZ_id);
    IGRclexists = TRUE;
    if (!(clp = dtsearch (cldict, &cl))) {
        IGRclexists = FALSE;
        if (!(clmem = vmalloc (Vmheap, sizeof (IGRcl_t)))) {
            SUwarning (0, "IGRinsertcl", "cannot allocate clmem");
            return NULL;
        }
        memset (clmem, 0, sizeof (IGRcl_t));
        memcpy (clmem->level, level, SZ_level);
        memcpy (clmem->id, id, SZ_id);
        clmem->nclass = nclass;
        if ((clp = dtinsert (cldict, clmem)) != clmem) {
            SUwarning (0, "IGRinsertcl", "cannot insert cl");
            vmfree (Vmheap, clmem);
            return NULL;
        }
    }
    return clp;
}

IGRrk_t *IGRinsertrk (char *level, char *id) {
    IGRrk_t rk, *rkp, *rkmem;

    memcpy (rk.level, level, SZ_level);
    memcpy (rk.id, id, SZ_id);
    IGRrkexists = TRUE;
    if (!(rkp = dtsearch (rkdict, &rk))) {
        IGRrkexists = FALSE;
        if (!(rkmem = vmalloc (Vmheap, sizeof (IGRrk_t)))) {
            SUwarning (0, "IGRinsertrk", "cannot allocate rkmem");
            return NULL;
        }
        memset (rkmem, 0, sizeof (IGRrk_t));
        memcpy (rkmem->level, level, SZ_level);
        memcpy (rkmem->id, id, SZ_id);
        if ((rkp = dtinsert (rkdict, rkmem)) != rkmem) {
            SUwarning (0, "IGRinsertrk", "cannot insert rk");
            vmfree (Vmheap, rkmem);
            return NULL;
        }
    }
    return rkp;
}

IGRnd_t *IGRinsertnd (char *level, char *id, short nclass) {
    IGRnd_t nd, *ndp, *ndmem;

    memcpy (nd.level, level, SZ_level);
    memcpy (nd.id, id, SZ_id);
    IGRndexists = TRUE;
    if (!(ndp = dtsearch (nddict, &nd))) {
        IGRndexists = FALSE;
        if (!(ndmem = vmalloc (Vmheap, sizeof (IGRnd_t)))) {
            SUwarning (0, "IGRinsertnd", "cannot allocate ndmem");
            return NULL;
        }
        memset (ndmem, 0, sizeof (IGRnd_t));
        memcpy (ndmem->level, level, SZ_level);
        memcpy (ndmem->id, id, SZ_id);
        if ((ndp = dtinsert (nddict, ndmem)) != ndmem) {
            SUwarning (0, "IGRinsertnd", "cannot insert nd");
            vmfree (Vmheap, ndmem);
            return NULL;
        }
        ndp->nclass = nclass;
    }
    return ndp;
}

IGRnd_t *IGRfindnd (char *level, char *id) {
    IGRnd_t nd;

    memcpy (nd.level, level, SZ_level);
    memcpy (nd.id, id, SZ_id);
    return dtsearch (nddict, &nd);
}

IGRnd_t *IGRlinknd2cc (IGRnd_t *ndp, IGRcc_t *ccp) {
    int cci;

    for (cci = 0; cci < ndp->ccn; cci++)
        if (ndp->ccs[cci] == ccp)
            return ndp;

    if (!(ndp->ccs = vmresize (
        Vmheap, ndp->ccs, (ndp->ccn + 1) * sizeof (IGRcc_t *), VM_RSCOPY
    ))) {
        SUwarning (0, "IGRlinknd2cc", "cannot grow ccs array");
        return NULL;
    }
    ndp->ccs[ndp->ccn++] = ccp;
    if (ndp->nclass == VG_QUERY_N_CLASS_PRIMARY)
        ccp->prc++;
    else
        ccp->src++;
    return ndp;
}

IGRnd_t *IGRunlinknd2cc (IGRnd_t *ndp, IGRcc_t *ccp) {
    int cci;

    for (cci = 0; cci < ndp->ccn; cci++) {
        if (ndp->ccs[cci] != ccp)
            continue;
        if (ndp->nclass == VG_QUERY_N_CLASS_PRIMARY)
            ndp->ccs[cci]->prc--;
        else
            ndp->ccs[cci]->src--;
        ndp->ccs[cci] = NULL;
    }
    return ndp;
}

IGRnd_t *IGRlinknd2cl (IGRnd_t *ndp, IGRcl_t *clp) {
    int cli;

    for (cli = 0; cli < ndp->cln; cli++)
        if (ndp->cls[cli] == clp)
            return ndp;

    if (!(ndp->cls = vmresize (
        Vmheap, ndp->cls, (ndp->cln + 1) * sizeof (IGRcl_t *), VM_RSCOPY
    ))) {
        SUwarning (0, "IGRlinknd2cl", "cannot grow cls array");
        return NULL;
    }
    ndp->cls[ndp->cln++] = clp;
    clp->rc++;
    return ndp;
}

IGRnd_t *IGRunlinknd2cl (IGRnd_t *ndp, IGRcl_t *clp) {
    int cli;

    for (cli = 0; cli < ndp->cln; cli++) {
        if (ndp->cls[cli] != clp)
            continue;
        ndp->cls[cli]->rc--;
        ndp->cls[cli] = NULL;
    }
    return ndp;
}

IGRnd_t *IGRlinknd2rk (IGRnd_t *ndp, IGRrk_t *rkp) {
    int rki;

    for (rki = 0; rki < ndp->rkn; rki++)
        if (ndp->rks[rki] == rkp)
            return ndp;

    if (!(ndp->rks = vmresize (
        Vmheap, ndp->rks, (ndp->rkn + 1) * sizeof (IGRrk_t *), VM_RSCOPY
    ))) {
        SUwarning (0, "IGRlinknd2rk", "cannot grow rks array");
        return NULL;
    }
    ndp->rks[ndp->rkn++] = rkp;
    rkp->rc++;
    return ndp;
}

IGRnd_t *IGRunlinknd2rk (IGRnd_t *ndp, IGRrk_t *rkp) {
    int rki;

    for (rki = 0; rki < ndp->rkn; rki++) {
        if (ndp->rks[rki] != rkp)
            continue;
        ndp->rks[rki]->rc--;
        ndp->rks[rki] = NULL;
    }
    return ndp;
}

IGRed_t *IGRinserted (IGRnd_t *ndp1, IGRnd_t *ndp2) {
    IGRed_t ed, *edp, *edmem;

    ed.ndp1 = ndp1;
    ed.ndp2 = ndp2;
    IGRedexists = TRUE;
    if (!(edp = dtsearch (eddict, &ed))) {
        IGRedexists = FALSE;
        if (!(edmem = vmalloc (Vmheap, sizeof (IGRed_t)))) {
            SUwarning (0, "IGRinserted", "cannot allocate edmem");
            return NULL;
        }
        memset (edmem, 0, sizeof (IGRed_t));
        edmem->ndp1 = ndp1, edmem->ndp2 = ndp2;
        if ((edp = dtinsert (eddict, edmem)) != edmem) {
            SUwarning (0, "IGRinserted", "cannot insert ed");
            vmfree (Vmheap, edmem);
            return NULL;
        }
    }
    return edp;
}

int IGRflatten (void) {
    IGRcc_t *ccp1, *ccp2;
    int cci;
    IGRcl_t *clp;
    int cli;
    IGRrk_t *rkp;
    int rki;
    IGRnd_t *ndp;
    int ndi;
    IGRed_t *edp;
    int edi;

    IGRccn = dtsize (ccdict);
    if (!(IGRccs = vmalloc (Vmheap, sizeof (IGRcc_t *) * IGRccn))) {
        SUwarning (0, "IGRflatten", "cannot allocate ccs array");
        return -1;
    }
    for (
        cci = 0, ccp1 = dtfirst (ccdict); ccp1; ccp1 = dtnext (ccdict, ccp1)
    ) {
        IGRccs[cci++] = ccp1;
        if (!ccp1->lccp)
            continue;
        for (ccp2 = ccp1; ccp2->lccp; ccp2 = ccp2->lccp)
            ;
        ccp1->lccp = ccp2;
    }
    qsort (IGRccs, IGRccn, sizeof (IGRcc_t *), cmp);

    IGRcln = dtsize (cldict);
    if (!(IGRcls = vmalloc (Vmheap, sizeof (IGRcl_t *) * IGRcln))) {
        SUwarning (0, "IGRflatten", "cannot allocate IGRcls array");
        return -1;
    }
    for (cli = 0, clp = dtfirst (cldict); clp; clp = dtnext (cldict, clp))
        IGRcls[cli++] = clp;

    IGRrkn = dtsize (rkdict);
    if (!(IGRrks = vmalloc (Vmheap, sizeof (IGRrk_t *) * IGRrkn))) {
        SUwarning (0, "IGRflatten", "cannot allocate IGRrks array");
        return -1;
    }
    for (rki = 0, rkp = dtfirst (rkdict); rkp; rkp = dtnext (rkdict, rkp))
        IGRrks[rki++] = rkp;

    IGRndn = dtsize (nddict);
    if (!(IGRnds = vmalloc (Vmheap, sizeof (IGRnd_t *) * IGRndn))) {
        SUwarning (0, "IGRflatten", "cannot allocate IGRnds array");
        return -1;
    }
    for (ndi = 0, ndp = dtfirst (nddict); ndp; ndp = dtnext (nddict, ndp))
        IGRnds[ndi++] = ndp;
    if (!(IGRwnds = vmalloc (Vmheap, sizeof (IGRnd_t *) * IGRndn))) {
        SUwarning (0, "IGRflatten", "cannot allocate IGRwnds array");
        return -1;
    }

    IGRedn = dtsize (eddict);
    if (!(IGReds = vmalloc (Vmheap, sizeof (IGRed_t *) * IGRedn))) {
        SUwarning (0, "IGRflatten", "caenot allocate IGReds array");
        return -1;
    }
    for (edi = 0, edp = dtfirst (eddict); edp; edp = dtnext (eddict, edp))
        IGReds[edi++] = edp;

    return 0;
}

/* graph layout */

int IGRreset (char *style) {
    gvFreeLayout (gvc, rootgp);
    agclose (rootgp);
    rootgp = NULL;
    return 0;
}

int IGRlayout (char *style) {
    UTattr_t *ap;
    int ai;

    gvLayout (gvc, rootgp, style);
#ifdef _UWIN
    gvRender (gvc, rootgp, "xdot", NULL);
#else
    gvRender (gvc, rootgp, "xdot", debugmode ? stderr : NULL);
#endif

    for (ai = 0; ai < UT_ATTR_SIZE; ai++) {
        ap = &UTattrgroups[UT_ATTRGROUP_GRAPH][ai];
        if (!ap->name)
            continue;
        gvsyms[UT_ATTRGROUP_GRAPH][ai] = agfindattr (rootgp, ap->name);
    }
    for (ai = 0; ai < UT_ATTR_SIZE; ai++) {
        ap = &UTattrgroups[UT_ATTRGROUP_NODE][ai];
        if (!ap->name)
            continue;
        gvsyms[UT_ATTRGROUP_NODE][ai] = agfindattr (rootgp->proto->n, ap->name);
    }
    for (ai = 0; ai < UT_ATTR_SIZE; ai++) {
        ap = &UTattrgroups[UT_ATTRGROUP_EDGE][ai];
        if (!ap->name)
            continue;
        gvsyms[UT_ATTRGROUP_EDGE][ai] = agfindattr (rootgp->proto->e, ap->name);
    }

    return 0;
}

int IGRnewgraph (IGRcc_t *ccp, char *attrstr) {
    char buf[32];
    UTattr_t *ap;
    int ai;
    EMbb_t bb;
    int havebb;
    Agsym_t *sp;

    if (UTsplitattr (UT_ATTRGROUP_GRAPH, attrstr) == -1) {
        SUwarning (0, "IGRnewgraph", "cannot parse attrstr");
        return -1;
    }
    sfsprintf (buf, 32, "g%d", gid++);
    if (!(ccp->gp = rootgp = agopen (buf, AGDIGRAPH))) {
        SUwarning (0, "IGRnewgraph", "cannot create graph");
        return -1;
    }
    havebb = FALSE;
    for (ai = 0; ai < UTattrm[UT_ATTRGROUP_GRAPH]; ai++) {
        ap = &UTattrs[UT_ATTRGROUP_GRAPH][ai];
        if (
            strcmp (ap->name, "label") == 0 &&
            strncmp (ap->value, "EMBED:", 6) == 0
        ) {
            if (EMparsenode (
                ccp->level, ccp->id, ap->value + 6, EM_DIR_V,
                &ccp->opi, &ccp->opn, &bb
            ) == -1) {
                SUwarning (0, "IGRnewgraph", "cannot parse graph EM contents");
                return -1;
            }
            havebb = TRUE;
            ap->value[0] = 0;
        }
        if (!(sp = agfindattr (rootgp, ap->name))) {
            if (!(sp = agraphattr (rootgp, ap->name, ""))) {
                SUwarning (0, "IGRnewgraph", "cannot insert attr");
                return -1;
            }
        }
        agxset (rootgp, sp->index, ap->value);
    }
    if (havebb) {
        sfsprintf (buf, 32, "%f", (bb.w / 72.0) / 1.35);
        if (!(sp = agfindattr (rootgp, "lwidth"))) {
            if (!(sp = agraphattr (rootgp, "lwidth", ""))) {
                SUwarning (0, "IGRnewgraph", "cannot insert lwidth attr");
                return -1;
            }
        }
        agxset (rootgp, sp->index, buf);
        sfsprintf (buf, 32, "%f", (bb.h / 72.0) / 1.35);
        if (!(sp = agfindattr (rootgp, "lheight"))) {
            if (!(sp = agraphattr (rootgp, "lheight", ""))) {
                SUwarning (0, "IGRnewgraph", "cannot insert lheight attr");
                return -1;
            }
        }
        agxset (rootgp, sp->index, buf);
    }

    return 0;
}

int IGRnewccgraph (Agraph_t *gp, IGRcc_t *ccp, char *attrstr) {
    char buf[32], *sgprefix;
    UTattr_t *ap;
    int ai;
    EMbb_t bb;
    int havebb;
    Agsym_t *sp;

    if (UTsplitattr (UT_ATTRGROUP_GRAPH, attrstr) == -1) {
        SUwarning (0, "IGRnewccgraph", "cannot parse attrstr");
        return -1;
    }
    sgprefix = "sg";
    for (ai = 0; ai < UTattrm[UT_ATTRGROUP_GRAPH]; ai++) {
        ap = &UTattrs[UT_ATTRGROUP_GRAPH][ai];
        if (strcmp (ap->name, "label") == 0 && ap->value[0] != 0) {
            sgprefix = "cluster";
            break;
        }
    }
    sfsprintf (buf, 32, "%s_g%d", sgprefix, gid++);
    if (!(ccp->gp = agsubg (gp, buf))) {
        SUwarning (0, "IGRnewccgraph", "cannot create sgraph");
        return -1;
    }
    havebb = FALSE;
    for (ai = 0; ai < UTattrm[UT_ATTRGROUP_GRAPH]; ai++) {
        ap = &UTattrs[UT_ATTRGROUP_GRAPH][ai];
        if (
            strcmp (ap->name, "label") == 0 &&
            strncmp (ap->value, "EMBED:", 6) == 0
        ) {
            if (EMparsenode (
                ccp->level, ccp->id, ap->value + 6, EM_DIR_V,
                &ccp->opi, &ccp->opn, &bb
            ) == -1) {
                SUwarning (
                    0, "IGRnewccgraph", "cannot parse graph EM contents"
                );
                return -1;
            }
            havebb = TRUE;
            ap->value[0] = 0;
        }
        if (!(sp = agfindattr (rootgp, ap->name))) {
            if (!(sp = agraphattr (rootgp, ap->name, ""))) {
                SUwarning (0, "IGRnewccgraph", "cannot insert attr");
                return -1;
            }
        }
        agxset (ccp->gp, sp->index, ap->value);
    }
    if (havebb) {
        sfsprintf (buf, 32, "%f", (bb.w / 72.0) / 1.35);
        if (!(sp = agfindattr (rootgp, "lwidth"))) {
            if (!(sp = agraphattr (rootgp, "lwidth", ""))) {
                SUwarning (0, "IGRnewccgraph", "cannot insert lwidth attr");
                return -1;
            }
        }
        agxset (ccp->gp, sp->index, buf);
        sfsprintf (buf, 32, "%f", (bb.h / 72.0) / 1.35);
        if (!(sp = agfindattr (rootgp, "lheight"))) {
            if (!(sp = agraphattr (rootgp, "lheight", ""))) {
                SUwarning (0, "IGRnewccgraph", "cannot insert lheight attr");
                return -1;
            }
        }
        agxset (ccp->gp, sp->index, buf);
    }

    return 0;
}

int IGRnewclgraph (Agraph_t *gp, IGRcl_t *clp, char *attrstr) {
    char buf[32], *sgprefix;
    UTattr_t *ap;
    int ai;
    EMbb_t bb;
    int havebb;
    Agsym_t *sp;

    if (UTsplitattr (UT_ATTRGROUP_GRAPH, attrstr) == -1) {
        SUwarning (0, "IGRnewclgraph", "cannot parse attrstr");
        return -1;
    }
    sgprefix = "sg";
    for (ai = 0; ai < UTattrm[UT_ATTRGROUP_GRAPH]; ai++) {
        ap = &UTattrs[UT_ATTRGROUP_GRAPH][ai];
        if (strcmp (ap->name, "label") == 0 && ap->value[0] != 0) {
            sgprefix = "cluster";
            break;
        }
    }
    sfsprintf (buf, 32, "%s_g%d", sgprefix, gid++);
    if (!(clp->gp = agsubg (gp, buf))) {
        SUwarning (0, "IGRnewclgraph", "cannot create sgraph");
        return -1;
    }
    havebb = FALSE;
    for (ai = 0; ai < UTattrm[UT_ATTRGROUP_GRAPH]; ai++) {
        ap = &UTattrs[UT_ATTRGROUP_GRAPH][ai];
        if (
            strcmp (ap->name, "label") == 0 &&
            strncmp (ap->value, "EMBED:", 6) == 0
        ) {
            if (EMparsenode (
                clp->level, clp->id, ap->value + 6, EM_DIR_V,
                &clp->opi, &clp->opn, &bb
            ) == -1) {
                SUwarning (
                    0, "IGRnewclgraph", "cannot parse graph EM contents"
                );
                return -1;
            }
            havebb = TRUE;
            ap->value[0] = 0;
        }
        if (!(sp = agfindattr (rootgp, ap->name))) {
            if (!(sp = agraphattr (rootgp, ap->name, ""))) {
                SUwarning (0, "IGRnewclgraph", "cannot insert attr");
                return -1;
            }
        }
        agxset (clp->gp, sp->index, ap->value);
    }
    if (havebb) {
        sfsprintf (buf, 32, "%f", (bb.w / 72.0) / 1.35);
        if (!(sp = agfindattr (rootgp, "lwidth"))) {
            if (!(sp = agraphattr (rootgp, "lwidth", ""))) {
                SUwarning (0, "IGRnewclgraph", "cannot insert lwidth attr");
                return -1;
            }
        }
        agxset (clp->gp, sp->index, buf);
        sfsprintf (buf, 32, "%f", (bb.h / 72.0) / 1.35);
        if (!(sp = agfindattr (rootgp, "lheight"))) {
            if (!(sp = agraphattr (rootgp, "lheight", ""))) {
                SUwarning (0, "IGRnewclgraph", "cannot insert lheight attr");
                return -1;
            }
        }
        agxset (clp->gp, sp->index, buf);
    }

    return 0;
}

int IGRnewrkgraph (Agraph_t *gp, IGRrk_t *rkp, char *attrstr) {
    char buf[32], *sgprefix;
    UTattr_t *ap;
    int ai;
    EMbb_t bb;
    int havebb;
    Agsym_t *sp;

    if (UTsplitattr (UT_ATTRGROUP_GRAPH, attrstr) == -1) {
        SUwarning (0, "IGRnewrkgraph", "cannot parse attrstr");
        return -1;
    }
    sgprefix = "sg";
    for (ai = 0; ai < UTattrm[UT_ATTRGROUP_GRAPH]; ai++) {
        ap = &UTattrs[UT_ATTRGROUP_GRAPH][ai];
        if (strcmp (ap->name, "label") == 0 && ap->value[0] != 0) {
            sgprefix = "cluster";
            break;
        }
    }
    sfsprintf (buf, 32, "%s_g%d", sgprefix, gid++);
    if (!(rkp->gp = agsubg (gp, buf))) {
        SUwarning (0, "IGRnewrkgraph", "cannot create sgraph");
        return -1;
    }
    havebb = FALSE;
    for (ai = 0; ai < UTattrm[UT_ATTRGROUP_GRAPH]; ai++) {
        ap = &UTattrs[UT_ATTRGROUP_GRAPH][ai];
        if (
            strcmp (ap->name, "label") == 0 &&
            strncmp (ap->value, "EMBED:", 6) == 0
        ) {
            if (EMparsenode (
                rkp->level, rkp->id, ap->value + 6, EM_DIR_V,
                &rkp->opi, &rkp->opn, &bb
            ) == -1) {
                SUwarning (
                    0, "IGRnewrkgraph", "cannot parse graph EM contents"
                );
                return -1;
            }
            havebb = TRUE;
            ap->value[0] = 0;
        }
        if (!(sp = agfindattr (rootgp, ap->name))) {
            if (!(sp = agraphattr (rootgp, ap->name, ""))) {
                SUwarning (0, "IGRnewrkgraph", "cannot insert attr");
                return -1;
            }
        }
        agxset (rkp->gp, sp->index, ap->value);
    }
    if (havebb) {
        sfsprintf (buf, 32, "%f", (bb.w / 72.0) / 1.35);
        if (!(sp = agfindattr (rootgp, "lwidth"))) {
            if (!(sp = agraphattr (rootgp, "lwidth", ""))) {
                SUwarning (0, "IGRnewrkgraph", "cannot insert lwidth attr");
                return -1;
            }
        }
        agxset (rkp->gp, sp->index, buf);
        sfsprintf (buf, 32, "%f", (bb.h / 72.0) / 1.35);
        if (!(sp = agfindattr (rootgp, "lheight"))) {
            if (!(sp = agraphattr (rootgp, "lheight", ""))) {
                SUwarning (0, "IGRnewrkgraph", "cannot insert lheight attr");
                return -1;
            }
        }
        agxset (rkp->gp, sp->index, buf);
    }

    return 0;
}

int IGRnewnode (Agraph_t *gp, IGRnd_t *ndp, char *attrstr) {
    char buf[32];
    UTattr_t *ap;
    int ai;
    EMbb_t bb;
    int havebb;
    Agsym_t *sp;

    if (UTsplitattr (UT_ATTRGROUP_NODE, attrstr) == -1) {
        SUwarning (0, "IGRnewnode", "cannot parse attrstr");
        return -1;
    }
    sfsprintf (buf, 32, "n%d", nid++);
    if (!(ndp->np = agnode (gp, buf))) {
        SUwarning (0, "IGRnewnode", "cannot create node");
        return -1;
    }
    havebb = FALSE;
    for (ai = 0; ai < UTattrm[UT_ATTRGROUP_NODE]; ai++) {
        ap = &UTattrs[UT_ATTRGROUP_NODE][ai];
        if (
            strcmp (ap->name, "label") == 0 &&
            strncmp (ap->value, "EMBED:", 6) == 0
        ) {
            if (EMparsenode (
                ndp->level, ndp->id, ap->value + 6, EM_DIR_V,
                &ndp->opi, &ndp->opn, &bb
            ) == -1) {
                SUwarning (0, "IGRnewnode", "cannot parse node EM contents");
                return -1;
            }
            havebb = TRUE;
            ndp->w = bb.w, ndp->h = bb.h, ndp->havewh = TRUE;
            ap->value[0] = 0;
        }
        if (!(sp = agfindattr (rootgp->proto->n, ap->name))) {
            if (!(sp = agnodeattr (rootgp, ap->name, ""))) {
                SUwarning (0, "IGRnewnode", "cannot insert attr");
                return -1;
            }
        }
        agxset (ndp->np, sp->index, ap->value);
    }
    if (havebb) {
        if (bb.w > 5) {
            sfsprintf (buf, 32, "%f", ((bb.w + 4) / 72.0) / 1.35);
            if (!(sp = agfindattr (rootgp->proto->n, "width"))) {
                if (!(sp = agnodeattr (rootgp, "width", ""))) {
                    SUwarning (0, "IGRnewnode", "cannot insert width attr");
                    return -1;
                }
            }
            agxset (ndp->np, sp->index, buf);
        }
        if (bb.h > 5) {
            sfsprintf (buf, 32, "%f", ((bb.h + 4) / 72.0) / 1.35);
            if (!(sp = agfindattr (rootgp->proto->n, "height"))) {
                if (!(sp = agnodeattr (rootgp, "height", ""))) {
                    SUwarning (0, "IGRnewnode", "cannot insert height attr");
                    return -1;
                }
            }
            agxset (ndp->np, sp->index, buf);
        }
    }

    return 0;
}

int IGRnewedge (Agraph_t *gp, IGRed_t *edp, char *attrstr) {
    char buf[32];
    UTattr_t *ap;
    int ai;
    EMbb_t bb;
    int havebb;
    Agsym_t *sp;

    if (UTsplitattr (UT_ATTRGROUP_EDGE, attrstr) == -1) {
        SUwarning (0, "IGRnewedge", "cannot parse attrstr");
        return -1;
    }
    if (!(edp->ep = agedge (gp, edp->ndp1->np, edp->ndp2->np))) {
        SUwarning (0, "IGRnewedge", "cannot create edge");
        return -1;
    }
    havebb = FALSE;
    for (ai = 0; ai < UTattrm[UT_ATTRGROUP_EDGE]; ai++) {
        ap = &UTattrs[UT_ATTRGROUP_EDGE][ai];
        if (
            strcmp (ap->name, "label") == 0 &&
            strncmp (ap->value, "EMBED:", 6) == 0
        ) {
            if (EMparseedge (
                edp->ndp1->level, edp->ndp1->id,
                edp->ndp2->level, edp->ndp2->id,
                ap->value + 6, EM_DIR_V, &edp->opi, &edp->opn, &bb
            ) == -1) {
                SUwarning (0, "IGRnewedge", "cannot parse edge EM contents");
                return -1;
            }
            havebb = TRUE;
            ap->value[0] = 0;
        } else if (!(sp = agfindattr (rootgp->proto->e, ap->name))) {
            if (!(sp = agedgeattr (rootgp, ap->name, ""))) {
                SUwarning (0, "IGRnewedge", "cannot insert attr");
                return -1;
            }
        }
        agxset (edp->ep, sp->index, ap->value);
    }
    if (havebb) {
        sfsprintf (buf, 32, "%f", (bb.w / 72.0) / 1.35);
        if (!(sp = agfindattr (rootgp->proto->e, "lwidth"))) {
            if (!(sp = agedgeattr (rootgp, "lwidth", ""))) {
                SUwarning (0, "IGRnewedge", "cannot insert lwidth attr");
                return -1;
            }
        }
        agxset (edp->ep, sp->index, buf);
        sfsprintf (buf, 32, "%f", (bb.h / 72.0) / 1.35);
        if (!(sp = agfindattr (rootgp->proto->e, "lheight"))) {
            if (!(sp = agedgeattr (rootgp, "lheight", ""))) {
                SUwarning (0, "IGRnewedge", "cannot insert lheight attr");
                return -1;
            }
        }
        agxset (edp->ep, sp->index, buf);
    }

    return 0;
}

/* graph drawing */

int IGRbegindraw (
    char *fprefix, int findex, RIop_t *ops, int opn,
    char *obj, char *url
) {
    char *bgcolor, *lncolor, *flcolor, *fncolor, *fnname, *fnsize, *info;
    int x, y, w, h;
    UTattr_t *ap;
    int ai;
    int opi;
    RIop_t op;
    RIarea_t a;

    if (getattrs (rootgp, NULL, NULL) == -1) {
        SUwarning (0, "IGRbegindraw", "cannot get main graph attr");
        return -1;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_GRAPH][UT_ATTR_BB];
    if (!ap->value || RIparseattr (ap->value, RI_OP_BB, &op) == -1) {
        SUwarning (0, "IGRbegindraw", "cannot get graph bbox");
        return -1;
    }
    width = op.u.rect.s.x + 2 * MW, height = op.u.rect.s.y + 2 * MW;
    if (!(rip = RIopen (RI_TYPE_IMAGE, fprefix, findex, width, height))) {
        SUwarning (0, "IGRbegindraw", "cannot open image file #%d", findex);
        return -1;
    }
    pageemit = 0;
    if (pagem > 0 && pagei++ % pagem == 0) {
        pageemit = 1;
        sfsprintf (pagebuf, 1000, "image %d", pagei);
    }
    if (pageemit)
        RIaddtoc (rip, ++pageindex, pagebuf);
    ap = &UTattrgroups[UT_ATTRGROUP_GRAPH][UT_ATTR_LP];
    if (ap->value && ap->value[0]) {
        if (RIparseattr (ap->value, RI_OP_POS, &op) == -1) {
            SUwarning (0, "IGRbegindraw", "cannot get label pos");
            return -1;
        }
        x = op.u.point.x, y = RIY (rip, op.u.point.y);
    } else
        x = y = 0;
    ap = &UTattrgroups[UT_ATTRGROUP_GRAPH][UT_ATTR_LWIDTH];
    if (ap->value && ap->value[0]) {
        if (RIparseattr (ap->value, RI_OP_LEN, &op) == -1) {
            SUwarning (0, "IGRbegindraw", "cannot get label width");
            return -1;
        }
        w = op.u.length.l;
    } else
        w = 0;
    ap = &UTattrgroups[UT_ATTRGROUP_GRAPH][UT_ATTR_LHEIGHT];
    if (ap->value && ap->value[0]) {
        if (RIparseattr (ap->value, RI_OP_LEN, &op) == -1) {
            SUwarning (0, "IGRbegindraw", "cannot get label height");
            return -1;
        }
        h = op.u.length.l;
    } else
        h = 0;
    x -= w / 2, y -= h / 2;
    ap = &UTattrgroups[UT_ATTRGROUP_GRAPH][UT_ATTR_COLOR];
    lncolor = (ap->value) ? ap->value : "black";
    ap = &UTattrgroups[UT_ATTRGROUP_GRAPH][UT_ATTR_FILLCOLOR];
    flcolor = (ap->value) ? ap->value : NULL;
    ap = &UTattrgroups[UT_ATTRGROUP_GRAPH][UT_ATTR_FONTCOLOR];
    fncolor = (ap->value) ? ap->value : "black";
    ap = &UTattrgroups[UT_ATTRGROUP_GRAPH][UT_ATTR_FONTNAME];
    fnname = (ap->value) ? ap->value : "Times-Roman";
    ap = &UTattrgroups[UT_ATTRGROUP_GRAPH][UT_ATTR_FONTSIZE];
    fnsize = (ap->value) ? ap->value : "8";
    ap = &UTattrgroups[UT_ATTRGROUP_GRAPH][UT_ATTR_BGCOLOR];
    bgcolor = (ap->value) ? ap->value : NULL;
    if (!flcolor)
        flcolor = bgcolor;
    if (!flcolor)
        flcolor = lncolor;
    if (bgcolor) {
        RIaddop (rip, NULL); // reset
        if (UTaddbox (rip, 0, 0, width, height, bgcolor, TRUE) == -1) {
            SUwarning (0, "IGRbegindraw", "cannot add bg box");
            return -1;
        }
        if (RIexecop (
            rip, NULL, 0, NULL, lncolor, flcolor, fncolor, fnname, fnsize
        ) == -1)
            SUwarning (0, "IGRbegindraw", "cannot draw em ops");
    }
    RIsetviewport (rip, TRUE, MW, -MW, width - MW, height - MW);
    ap = &UTattrgroups[UT_ATTRGROUP_GRAPH][UT_ATTR_INFO];
    info = ap->value;
    for (ai = UT_ATTR_MINDRAW; ai <= UT_ATTR_MAXDRAW; ai++) {
        ap = &UTattrgroups[UT_ATTRGROUP_GRAPH][ai];
        if (!ap->value)
            continue;
        if (RIparseop (rip, ap->value, TRUE) == -1) {
            SUwarning (0, "IGRbegindraw", "cannot parse ops for attr %d", ai);
            continue;
        }
        if (RIexecop (
            rip, NULL, 0, NULL, lncolor, flcolor, fncolor, fnname, fnsize
        ) == -1) {
            SUwarning (0, "IGRbegindraw", "cannot draw ops for attr %d", ai);
            continue;
        }
    }
    if (opn > 0) {
        RIaddop (rip, NULL); // reset
        RIsetviewport (rip, TRUE, x + MW, y - MW, w, h);
        for (opi = 0; opi < opn; opi++) {
            if (RIaddop (rip, &ops[opi]) == -1) {
                SUwarning (0, "IGRbegindraw", "cannot add em op");
                return -1;
            }
        }
        RIsetviewport (rip, TRUE, MW, -MW, width - MW, height - MW);
        if (RIexecop (
            rip, NULL, 0, NULL, lncolor, flcolor, fncolor, fnname, fnsize
        ) == -1)
            SUwarning (0, "IGRbegindraw", "cannot draw em ops");
    }
    if (url && url[0] && obj && obj[0] && w > 0 && h > 0) {
        a.mode = RI_AREA_ALL;
        a.type = RI_AREA_RECT;
        a.u.rect.x1 = x, a.u.rect.y1 = y;
        a.u.rect.x2 = x + w - 1, a.u.rect.y2 = y + h - 1;
        a.pass = 0;
        a.obj = obj;
        a.url = url;
        a.info = info;
        if (RIaddarea (rip, &a) == -1)
            SUwarning (0, "IGRbegindraw", "cannot add area");
    }
    return 0;
}

int IGRenddraw (void) {
    if (RIclose (rip) == -1) {
        SUwarning (0, "IGRenddraw", "cannot save image");
        return -1;
    }
    return 0;
}

int IGRdrawsgraph (Agraph_t *gp, RIop_t *ops, int opn, char *obj, char *url) {
    char *bgcolor, *lncolor, *flcolor, *fncolor, *fnname, *fnsize, *info;
    int x, y, w, h;
    UTattr_t *ap;
    int ai;
    int opi;
    RIop_t op;
    RIarea_t a;

    if (getattrs (gp, NULL, NULL) == -1) {
        SUwarning (0, "IGRdrawsgraph", "cannot get sgraph attr");
        return -1;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_GRAPH][UT_ATTR_BB];
    if (ap->value && ap->value[0]) {
        if (RIparseattr (ap->value, RI_OP_BB, &op) == -1) {
            SUwarning (0, "IGRdrawsgraph", "cannot get sgraph bbox");
            return -1;
        }
    }
    ap = &UTattrgroups[UT_ATTRGROUP_GRAPH][UT_ATTR_LP];
    if (ap->value && ap->value[0]) {
        if (RIparseattr (ap->value, RI_OP_POS, &op) == -1) {
            SUwarning (0, "IGRdrawsgraph", "cannot get label pos");
            return -1;
        }
        x = op.u.point.x, y = RIY (rip, op.u.point.y);
    } else
        x = y = 0;
    ap = &UTattrgroups[UT_ATTRGROUP_GRAPH][UT_ATTR_LWIDTH];
    if (ap->value && ap->value[0]) {
        if (RIparseattr (ap->value, RI_OP_LEN, &op) == -1) {
            SUwarning (0, "IGRdrawsgraph", "cannot get label width");
            return -1;
        }
        w = op.u.length.l;
    } else
        w = 0;
    ap = &UTattrgroups[UT_ATTRGROUP_GRAPH][UT_ATTR_LHEIGHT];
    if (ap->value && ap->value[0]) {
        if (RIparseattr (ap->value, RI_OP_LEN, &op) == -1) {
            SUwarning (0, "IGRdrawsgraph", "cannot get label height");
            return -1;
        }
        h = op.u.length.l;
    } else
        h = 0;
    x -= w / 2, y -= h / 2;
    ap = &UTattrgroups[UT_ATTRGROUP_GRAPH][UT_ATTR_BGCOLOR];
    bgcolor = (ap->value) ? ap->value : NULL;
    ap = &UTattrgroups[UT_ATTRGROUP_GRAPH][UT_ATTR_COLOR];
    lncolor = (ap->value) ? ap->value : "black";
    ap = &UTattrgroups[UT_ATTRGROUP_GRAPH][UT_ATTR_FILLCOLOR];
    flcolor = (ap->value) ? ap->value : NULL;
    ap = &UTattrgroups[UT_ATTRGROUP_GRAPH][UT_ATTR_FONTCOLOR];
    fncolor = (ap->value) ? ap->value : "black";
    ap = &UTattrgroups[UT_ATTRGROUP_GRAPH][UT_ATTR_FONTNAME];
    fnname = (ap->value) ? ap->value : "Times-Roman";
    ap = &UTattrgroups[UT_ATTRGROUP_GRAPH][UT_ATTR_FONTSIZE];
    fnsize = (ap->value) ? ap->value : "8";
    if (!flcolor)
        flcolor = bgcolor;
    if (!flcolor)
        flcolor = lncolor;
    ap = &UTattrgroups[UT_ATTRGROUP_GRAPH][UT_ATTR_INFO];
    info = ap->value;
    for (ai = UT_ATTR_MINDRAW; ai <= UT_ATTR_MAXDRAW; ai++) {
        ap = &UTattrgroups[UT_ATTRGROUP_GRAPH][ai];
        if (!ap->value)
            continue;
        if (RIparseop (rip, ap->value, TRUE) == -1) {
            SUwarning (0, "IGRdrawsgraph", "cannot parse ops %d", ai);
            continue;
        }
        if (RIexecop (
            rip, NULL, 0, NULL, lncolor, flcolor, fncolor, fnname, fnsize
        ) == -1) {
            SUwarning (0, "IGRdrawsgraph", "cannot draw ops %d", ai);
            continue;
        }
    }
    if (opn > 0) {
        RIaddop (rip, NULL); // reset
        RIsetviewport (rip, TRUE, x + MW, y - MW, w, h);
        for (opi = 0; opi < opn; opi++) {
            if (RIaddop (rip, &ops[opi]) == -1) {
                SUwarning (0, "IGRdrawsgraph", "cannot add em op");
                return -1;
            }
        }
        RIsetviewport (rip, TRUE, MW, -MW, width - MW, height - MW);
        if (RIexecop (
            rip, NULL, 0, NULL, lncolor, flcolor, fncolor, fnname, fnsize
        ) == -1)
            SUwarning (0, "IGRdrawsgraph", "cannot draw em ops");
    }
    if (url && url[0] && obj && obj[0] && w > 0 && h > 0) {
        a.mode = RI_AREA_AREA;
        a.type = RI_AREA_RECT;
        a.u.rect.x1 = x, a.u.rect.y1 = y;
        a.u.rect.x2 = x + w - 1, a.u.rect.y2 = y + h - 1;
        a.pass = 0;
        a.obj = obj;
        a.url = url;
        a.info = info;
        if (RIaddarea (rip, &a) == -1)
            SUwarning (0, "IGRdrawsgraph", "cannot add area");
    }
    return 0;
}

int IGRdrawnode (
    Agnode_t *np, RIop_t *ops, int opn, int havewh, int nw, int nh,
    char *obj, char *url
) {
    char *lncolor, *flcolor, *fncolor, *fnname, *fnsize, *info;
    int x, y, w, h;
    UTattr_t *ap;
    int ai;
    int opi;
    RIop_t op;
    RIarea_t a;

    if (getattrs (NULL, np, NULL) == -1) {
        SUwarning (0, "IGRdrawnode", "cannot get node attr");
        return -1;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_POS];
    if (!ap->value || RIparseattr (ap->value, RI_OP_POS, &op) == -1) {
        SUwarning (0, "IGRdrawnode", "cannot get node pos");
        return -1;
    }
    x = op.u.point.x, y = RIY (rip, op.u.point.y);
    ap = &UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_WIDTH];
    if (ap->value && ap->value[0]) {
        if (RIparseattr (ap->value, RI_OP_LEN, &op) == -1) {
            SUwarning (0, "IGRdrawnode", "cannot get label width");
            return -1;
        }
        w = op.u.length.l;
    } else
        w = 0;
    ap = &UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_HEIGHT];
    if (ap->value && ap->value[0]) {
        if (RIparseattr (ap->value, RI_OP_LEN, &op) == -1) {
            SUwarning (0, "IGRdrawnode", "cannot get label height");
            return -1;
        }
        h = op.u.length.l;
    } else
        h = 0;
    x -= w / 2.0, y -= h / 2.0;
    ap = &UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_COLOR];
    lncolor = (ap->value) ? ap->value : "black";
    ap = &UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_FILLCOLOR];
    flcolor = (ap->value) ? ap->value : NULL;
    ap = &UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_FONTCOLOR];
    fncolor = (ap->value) ? ap->value : "black";
    ap = &UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_FONTNAME];
    fnname = (ap->value) ? ap->value : "Times-Roman";
    ap = &UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_FONTSIZE];
    fnsize = (ap->value) ? ap->value : "8";
    if (!flcolor)
        flcolor = lncolor;
    ap = &UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_INFO];
    info = ap->value;

    RIaddop (rip, NULL); // reset
    UTlockcolor = TRUE;
    if (lncolor && lncolor[0]) {
        memset (&op, 0, sizeof (RIop_t));
        op.type = RI_OP_c;
        op.u.color.t = lncolor;
        if (RIaddop (rip, &op) == -1) {
            SUwarning (0, "IGRdrawnode", "cannot add c op");
            return -1;
        }
    }
    UTlockcolor = FALSE;
    if (flcolor && flcolor[0]) {
        memset (&op, 0, sizeof (RIop_t));
        op.type = RI_OP_C;
        op.u.color.t = flcolor;
        if (RIaddop (rip, &op) == -1) {
            SUwarning (0, "IGRdrawnode", "cannot add C op");
            return -1;
        }
    }
    if (fncolor && fncolor[0]) {
        memset (&op, 0, sizeof (RIop_t));
        op.type = RI_OP_FC;
        op.u.color.t = fncolor;
        if (RIaddop (rip, &op) == -1) {
            SUwarning (0, "IGRdrawnode", "cannot add FC op");
            return -1;
        }
    }
    for (opi = 0; opi < rip->opm; opi++)
        if (rip->ops[opi].type == RI_OP_c)
            rip->ops[opi].u.color.locked = TRUE;
    for (ai = UT_ATTR_MINDRAW; ai <= UT_ATTR_MAXDRAW; ai++) {
        ap = &UTattrgroups[UT_ATTRGROUP_NODE][ai];
        if (!ap->value)
            continue;
        if (ai == UT_ATTR_DRAW && opn > 0) {
            if (RIparseop (rip, ap->value, FALSE) == -1) {
                SUwarning (0, "IGRdrawnode", "cannot parse ops %d", ai);
                continue;
            }
            RIsetviewport (rip, TRUE, x + 2 + MW, y - MW, w, h);
            if (havewh && (nw != w || nh != h))
                RIcenterops (ops, opn, nw, nh, w, h);
            for (opi = 0; opi < opn; opi++) {
                if (ops[opi].type == RI_OP_c)
                    UTlockcolor = TRUE;
                if (RIaddop (rip, &ops[opi]) == -1) {
                    SUwarning (0, "IGRdrawnode", "cannot add em op");
                    return -1;
                }
                UTlockcolor = FALSE;
            }
            RIsetviewport (rip, TRUE, MW, -MW, width - MW, height - MW);
        } else {
            if (RIparseop (rip, ap->value, FALSE) == -1) {
                SUwarning (0, "IGRdrawnode", "cannot parse ops %d", ai);
                continue;
            }
        }
    }
    if (RIexecop (
        rip, NULL, 0, NULL, lncolor, flcolor, fncolor, fnname, fnsize
    ) == -1)
        SUwarning (0, "IGRdrawnode", "cannot draw ops %d", ai);

    if (url && url[0] && obj && obj[0] && w > 0 && h > 0) {
        a.mode = RI_AREA_AREA;
        a.type = RI_AREA_RECT;
        a.u.rect.x1 = x, a.u.rect.y1 = y;
        a.u.rect.x2 = x + w - 1, a.u.rect.y2 = y + h - 1;
        a.pass = 0;
        a.obj = obj;
        a.url = url;
        a.info = info;
        if (RIaddarea (rip, &a) == -1)
            SUwarning (0, "IGRdrawnode", "cannot add area");
    }
    return 0;
}

int IGRdrawedge (Agedge_t *ep, RIop_t *ops, int opn, char *obj, char *url) {
    char *lncolor, *flcolor, *fncolor, *fnname, *fnsize;
    int x, y, w, h;
    UTattr_t *ap;
    int ai;
    int opi;
    RIop_t op;

    if (getattrs (NULL, NULL, ep) == -1) {
        SUwarning (0, "IGRdrawedge", "cannot get edge attr");
        return -1;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_LP];
    if (ap->value && ap->value[0]) {
        if (RIparseattr (ap->value, RI_OP_POS, &op) == -1) {
            SUwarning (0, "IGRdrawedge", "cannot get label pos");
            return -1;
        }
        x = op.u.point.x, y = RIY (rip, op.u.point.y);
    } else
        x = y = 0;
    ap = &UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_LWIDTH];
    if (ap->value && ap->value[0]) {
        if (RIparseattr (ap->value, RI_OP_LEN, &op) == -1) {
            SUwarning (0, "IGRdrawedge", "cannot get label width");
            return -1;
        }
        w = op.u.length.l;
    } else
        w = 0;
    ap = &UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_LHEIGHT];
    if (ap->value && ap->value[0]) {
        if (RIparseattr (ap->value, RI_OP_LEN, &op) == -1) {
            SUwarning (0, "IGRdrawedge", "cannot get label height");
            return -1;
        }
        h = op.u.length.l;
    } else
        h = 0;
    x -= w / 2, y -= h / 2;
    ap = &UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_COLOR];
    lncolor = (ap->value) ? ap->value : "black";
    ap = &UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_FILLCOLOR];
    flcolor = (ap->value) ? ap->value : NULL;
    ap = &UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_FONTCOLOR];
    fncolor = (ap->value) ? ap->value : "black";
    ap = &UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_FONTNAME];
    fnname = (ap->value) ? ap->value : "Times-Roman";
    ap = &UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_FONTSIZE];
    fnsize = (ap->value) ? ap->value : "8";
    if (!flcolor)
        flcolor = lncolor;

    RIaddop (rip, NULL); // reset
    UTlockcolor = FALSE;
    if (lncolor && lncolor[0]) {
        memset (&op, 0, sizeof (RIop_t));
        op.type = RI_OP_c;
        op.u.color.t =lncolor;
        if (RIaddop (rip, &op) == -1) {
            SUwarning (0, "IGRdrawedge", "cannot add c op");
            return -1;
        }
    }
    if (flcolor && flcolor[0]) {
        memset (&op, 0, sizeof (RIop_t));
        op.type = RI_OP_C;
        op.u.color.t = flcolor;
        if (RIaddop (rip, &op) == -1) {
            SUwarning (0, "IGRdrawedge", "cannot add C op");
            return -1;
        }
    }
    if (fncolor && fncolor[0]) {
        memset (&op, 0, sizeof (RIop_t));
        op.type = RI_OP_FC;
        op.u.color.t = fncolor;
        if (RIaddop (rip, &op) == -1) {
            SUwarning (0, "IGRdrawedge", "cannot add FC op");
            return -1;
        }
    }
    for (ai = UT_ATTR_MINDRAW; ai <= UT_ATTR_MAXDRAW; ai++) {
        ap = &UTattrgroups[UT_ATTRGROUP_EDGE][ai];
        if (!ap->value)
            continue;
        if (ai == UT_ATTR_DRAW && opn > 0) {
            if (RIparseop (rip, ap->value, FALSE) == -1) {
                SUwarning (0, "IGRdrawedge", "cannot parse ops %d", ai);
                continue;
            }
            RIsetviewport (rip, TRUE, x + MW, y - MW, w, h);
            for (opi = 0; opi < opn; opi++) {
                if (RIaddop (rip, &ops[opi]) == -1) {
                    SUwarning (0, "IGRdrawedge", "cannot add em op");
                    return -1;
                }
            }
            RIsetviewport (rip, TRUE, MW, -MW, width - MW, height - MW);
        } else {
            if (RIparseop (rip, ap->value, FALSE) == -1) {
                SUwarning (0, "IGRdrawedge", "cannot parse ops %d", ai);
                continue;
            }
        }
    }
    if (RIexecop (
        rip, NULL, 0, NULL, lncolor, flcolor, fncolor, fnname, fnsize
    ) == -1)
        SUwarning (0, "IGRdrawedge", "cannot draw ops %d", ai);

    return 0;
}

/* utilities */

static int getattrs (Agraph_t *gp, Agnode_t *np, Agedge_t *ep) {
    UTattr_t *ap;
    int ai;
    Agsym_t *sp;

    if (gp) {
        for (ai = 0; ai < UT_ATTR_SIZE; ai++) {
            ap = &UTattrgroups[UT_ATTRGROUP_GRAPH][ai];
            ap->value = NULL;
            sp = gvsyms[UT_ATTRGROUP_GRAPH][ai];
            if (!ap->name || !sp)
                continue;
            ap->value = agxget (gp, sp->index);
        }
    }
    if (np) {
        for (ai = 0; ai < UT_ATTR_SIZE; ai++) {
            ap = &UTattrgroups[UT_ATTRGROUP_NODE][ai];
            ap->value = NULL;
            sp = gvsyms[UT_ATTRGROUP_NODE][ai];
            if (!ap->name || !sp)
                continue;
            ap->value = agxget (np, sp->index);
        }
    }
    if (ep) {
        for (ai = 0; ai < UT_ATTR_SIZE; ai++) {
            ap = &UTattrgroups[UT_ATTRGROUP_EDGE][ai];
            ap->value = NULL;
            sp = gvsyms[UT_ATTRGROUP_EDGE][ai];
            if (!ap->name || !sp)
                continue;
            ap->value = agxget (ep, sp->index);
        }
    }

    return 0;
}

static int cmp (const void *a, const void *b) {
    IGRcc_t *ap, *bp;

    ap = *(IGRcc_t **) a;
    bp = *(IGRcc_t **) b;
    if (ap->prc != bp->prc)
        return - (ap->prc - bp->prc);
    return - (ap->src - bp->src);
}
