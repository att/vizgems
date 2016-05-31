#include <ast.h>
#include <ast_float.h>
#include <tok.h>
#include <swift.h>
#include "vg_dq_vt_util.h"

IGDcc_t **IGDccs;
int IGDccn;
IGDcl_t **IGDcls;
int IGDcln;
IGDnd_t **IGDnds, **IGDwnds;
int IGDndn;

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
static Dt_t *nddict;
static Dtdisc_t nddisc = {
    sizeof (Dtlink_t), SZ_level + SZ_id, 0, NULL, NULL, NULL, NULL, NULL, NULL
};

#define MARGIN 5

static int prefwidth, width, height;

static int *bmxs, *bmws;

static RI_t *rip;

static int layout (int, int, int, int, int *, int *);
static int cmp (const void *, const void *);

int IGDinit (int screenwidth, char *pm) {
    if (!(ccdict = dtopen (&ccdisc, Dtset))) {
        SUwarning (0, "IGDinit", "cannot create ccdict");
        return -1;
    }
    if (!(cldict = dtopen (&cldisc, Dtset))) {
        SUwarning (0, "IGDinit", "cannot create cldict");
        return -1;
    }
    if (!(nddict = dtopen (&nddisc, Dtset))) {
        SUwarning (0, "IGDinit", "cannot create nddict");
        return -1;
    }
    IGDccs = NULL;
    IGDccn = 0;
    IGDcls = NULL;
    IGDcln = 0;
    IGDnds = IGDwnds = NULL;
    IGDndn = 0;

    pagemode = pm;
    pageindex = pagei = pagem = 0;
    if (pagemode && pagemode[0] && strcmp (pagemode, "all") != 0)
        pagem = atoi (pagemode);
    if (pagem < 0)
        pagem = 0;

    UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_BGCOLOR].name = "bgcolor";

    UTattrgroups[UT_ATTRGROUP_GRAPH][UT_ATTR_COLOR].name = "color";
    UTattrgroups[UT_ATTRGROUP_GRAPH][UT_ATTR_FONTNAME].name = "fontname";
    UTattrgroups[UT_ATTRGROUP_GRAPH][UT_ATTR_FONTSIZE].name = "fontsize";
    UTattrgroups[UT_ATTRGROUP_GRAPH][UT_ATTR_LABEL].name = "label";
    UTattrgroups[UT_ATTRGROUP_GRAPH][UT_ATTR_LINEWIDTH].name = "linewidth";

    UTattrgroups[UT_ATTRGROUP_TITLE][UT_ATTR_LABEL].name = "label";
    UTattrgroups[UT_ATTRGROUP_TITLE][UT_ATTR_COLOR].name = "color";
    UTattrgroups[UT_ATTRGROUP_TITLE][UT_ATTR_FONTNAME].name = "fontname";
    UTattrgroups[UT_ATTRGROUP_TITLE][UT_ATTR_FONTSIZE].name = "fontsize";
    UTattrgroups[UT_ATTRGROUP_TITLE][UT_ATTR_INFO].name = "info";

    UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_COLOR].name = "color";
    UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_FILLCOLOR].name = "fillcolor";
    UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_WIDTH].name = "width";
    UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_HEIGHT].name = "height";
    UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_LINEWIDTH].name = "linewidth";
    UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_FONTNAME].name = "fontname";
    UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_FONTSIZE].name = "fontsize";
    UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_LABEL].name = "label";
    UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_INFO].name = "info";

    prefwidth = screenwidth;

    return 0;
}

IGDcc_t *IGDinsertcc (char *level, char *id) {
    IGDcc_t cc, *ccp, *ccmem;

    memcpy (cc.level, level, SZ_level);
    memcpy (cc.id, id, SZ_id);
    if (!(ccp = dtsearch (ccdict, &cc))) {
        if (!(ccmem = vmalloc (Vmheap, sizeof (IGDcc_t)))) {
            SUwarning (0, "IGDinsertcc", "cannot allocate ccmem");
            return NULL;
        }
        memset (ccmem, 0, sizeof (IGDcc_t));
        memcpy (ccmem->level, level, SZ_level);
        memcpy (ccmem->id, id, SZ_id);
        if ((ccp = dtinsert (ccdict, ccmem)) != ccmem) {
            SUwarning (0, "IGDinsertcc", "cannot insert cc");
            vmfree (Vmheap, ccmem);
            return NULL;
        }
    }
    return ccp;
}

IGDcl_t *IGDinsertcl (
    char *level, char *id, char *attrstr, char *obj, char *url
) {
    IGDcl_t cl, *clp, *clmem;
    UTattr_t *ap;

    memcpy (cl.level, level, SZ_level);
    memcpy (cl.id, id, SZ_id);
    if (!(clp = dtsearch (cldict, &cl))) {
        if (!(clmem = vmalloc (Vmheap, sizeof (IGDcl_t)))) {
            SUwarning (0, "IGDinsertcl", "cannot allocate clmem");
            return NULL;
        }
        memset (clmem, 0, sizeof (IGDcl_t));
        memcpy (clmem->level, level, SZ_level);
        memcpy (clmem->id, id, SZ_id);
        if ((clp = dtinsert (cldict, clmem)) != clmem) {
            SUwarning (0, "IGDinsertcl", "cannot insert cl");
            vmfree (Vmheap, clmem);
            return NULL;
        }
    }

    if (!(clp->obj = vmstrdup (Vmheap, obj ? obj : ""))) {
        SUwarning (0, "IGDinsertcl", "cannot copy obj");
        return NULL;
    }
    if (!(clp->url = vmstrdup (Vmheap, url ? url : ""))) {
        SUwarning (0, "IGDinsertcl", "cannot copy url");
        return NULL;
    }

    if (UTsplitattr (UT_ATTRGROUP_GRAPH, attrstr) == -1) {
        SUwarning (0, "IGDinsertcl", "cannot parse attrstr");
        return NULL;
    }
    if (UTgetattrs (UT_ATTRGROUP_GRAPH) == -1) {
        SUwarning (0, "IGDinsertcl", "cannot get node attr");
        return NULL;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_GRAPH][UT_ATTR_LABEL];
    if (!(clp->lab = vmstrdup (
        Vmheap, (ap->value && ap->value[0]) ? ap->value : ""
    ))) {
        SUwarning (0, "IGDinsertcl", "cannot copy node label");
        return NULL;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_GRAPH][UT_ATTR_LINEWIDTH];
    clp->lw = (ap->value && ap->value[0]) ? atoi (ap->value) : 1;
    ap = &UTattrgroups[UT_ATTRGROUP_GRAPH][UT_ATTR_COLOR];
    if (!(clp->cl = vmstrdup (
        Vmheap, (ap->value && ap->value[0]) ? ap->value : "black"
    ))) {
        SUwarning (0, "IGDinsertcl", "cannot copy node color");
        return NULL;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_GRAPH][UT_ATTR_FONTNAME];
    if (!(clp->fn = vmstrdup (
        Vmheap, (ap->value && ap->value[0]) ? ap->value : "abc"
    ))) {
        SUwarning (0, "IGDinsertcl", "cannot copy node font name");
        return NULL;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_GRAPH][UT_ATTR_FONTSIZE];
    if (!(clp->fs = vmstrdup (
        Vmheap, (ap->value && ap->value[0]) ? ap->value : "10"
    ))) {
        SUwarning (0, "IGDinsertcl", "cannot copy node font size");
        return NULL;
    }

    return clp;
}

IGDnd_t *IGDinsertnd (char *level, char *id, short nclass, char *attrstr) {
    IGDnd_t nd, *ndp, *ndmem;
    UTattr_t *ap;
    EMbb_t bb;

    memcpy (nd.level, level, SZ_level);
    memcpy (nd.id, id, SZ_id);
    if ((ndp = dtsearch (nddict, &nd)))
        return ndp;

    if (!(ndmem = vmalloc (Vmheap, sizeof (IGDnd_t)))) {
        SUwarning (0, "IGDinsertnd", "cannot allocate ndmem");
        return NULL;
    }
    memset (ndmem, 0, sizeof (IGDnd_t));
    memcpy (ndmem->level, level, SZ_level);
    memcpy (ndmem->id, id, SZ_id);
    if ((ndp = dtinsert (nddict, ndmem)) != ndmem) {
        SUwarning (0, "IGDinsertnd", "cannot insert nd");
        vmfree (Vmheap, ndmem);
        return NULL;
    }
    ndp->nclass = nclass;

    if (UTsplitattr (UT_ATTRGROUP_NODE, attrstr) == -1) {
        SUwarning (0, "IGDinsertnd", "cannot parse attrstr");
        return NULL;
    }
    if (UTgetattrs (UT_ATTRGROUP_NODE) == -1) {
        SUwarning (0, "IGDinsertnd", "cannot get node attr");
        return NULL;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_WIDTH];
    if (ap->value && ap->value[0])
        ndp->bmw = atoi (ap->value);
    else
        ndp->bmw = 100;
    ap = &UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_HEIGHT];
    if (ap->value && ap->value[0])
        ndp->bmh = atoi (ap->value);
    else
        ndp->bmh = 50;
    ap = &UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_LABEL];
    if (!(ndp->lab = vmstrdup (
        Vmheap, (ap->value && ap->value[0]) ? ap->value : id
    ))) {
        SUwarning (0, "IGDinsertnd", "cannot copy node label");
        return NULL;
    }
    if (
        ap->value &&
        strcmp (ap->name, "label") == 0 &&
        strncmp (ap->value, "EMBED:", 6) == 0
    ) {
        if (EMparsenode (
            ndp->level, ndp->id, ap->value + 6, EM_DIR_V,
            &ndp->opi, &ndp->opn, &bb
        ) == -1) {
            SUwarning (0, "IGDinsertnd", "cannot parse node EM contents");
            return NULL;
        }
        if (ndp->bmw < bb.w)
            ndp->bmw = bb.w;
        if (ndp->bmh < bb.h)
            ndp->bmh = bb.h;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_LINEWIDTH];
    ndp->lw = (ap->value && ap->value[0]) ? atoi (ap->value) : 1;
    ap = &UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_COLOR];
    if (!(ndp->cl = vmstrdup (
        Vmheap, (ap->value && ap->value[0]) ? ap->value : "black"
    ))) {
        SUwarning (0, "IGDinsertnd", "cannot copy node color");
        return NULL;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_FILLCOLOR];
    if (!(ndp->fcl = vmstrdup (
        Vmheap, (ap->value && ap->value[0]) ? ap->value : ""
    ))) {
        SUwarning (0, "IGDinsertnd", "cannot copy node fillcolor");
        return NULL;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_FONTNAME];
    if (!(ndp->fn = vmstrdup (
        Vmheap, (ap->value && ap->value[0]) ? ap->value : "abc"
    ))) {
        SUwarning (0, "IGDinsertnd", "cannot copy node font name");
        return NULL;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_FONTSIZE];
    if (!(ndp->fs = vmstrdup (
        Vmheap, (ap->value && ap->value[0]) ? ap->value : "10"
    ))) {
        SUwarning (0, "IGDinsertnd", "cannot copy node font size");
        return NULL;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_INFO];
    if (ap->value && !(ndp->info = vmstrdup (Vmheap, ap->value))) {
        SUwarning (0, "IGDinsertnd", "cannot copy node info");
        return NULL;
    }

    return ndp;
}

IGDnd_t *IGDlinknd2cc (IGDnd_t *ndp, IGDcc_t *ccp) {
    int cci;

    for (cci = 0; cci < ndp->ccn; cci++)
        if (ndp->ccs[cci] == ccp)
            return ndp;

    if (!(ndp->ccs = vmresize (
        Vmheap, ndp->ccs, (ndp->ccn + 1) * sizeof (IGDcc_t *), VM_RSCOPY
    ))) {
        SUwarning (0, "IGDlinknd2cc", "cannot grow ccs array");
        return NULL;
    }
    ndp->ccs[ndp->ccn++] = ccp;
    ccp->rc++;
    return ndp;
}

IGDnd_t *IGDlinknd2cl (IGDnd_t *ndp, IGDcl_t *clp) {
    int cli;

    for (cli = 0; cli < ndp->cln; cli++)
        if (ndp->cls[cli] == clp)
            return ndp;

    if (!(ndp->cls = vmresize (
        Vmheap, ndp->cls, (ndp->cln + 1) * sizeof (IGDcl_t *), VM_RSCOPY
    ))) {
        SUwarning (0, "IGDlinknd2cl", "cannot grow cls array");
        return NULL;
    }
    ndp->cls[ndp->cln++] = clp;
    clp->rc++;
    return ndp;
}

int IGDflatten (void) {
    IGDcc_t *ccp;
    int cci, maxcci;
    IGDcl_t *clp;
    int cli, maxcli;
    IGDnd_t *ndp;
    int ndi;
    int maxrc;

    IGDccn = dtsize (ccdict);
    if (!(IGDccs = vmalloc (Vmheap, sizeof (IGDcc_t *) * IGDccn))) {
        SUwarning (0, "IGDflatten", "cannot allocate ccs array");
        return -1;
    }
    for (cci = 0, ccp = dtfirst (ccdict); ccp; ccp = dtnext (ccdict, ccp))
        IGDccs[cci++] = ccp;

    IGDcln = dtsize (cldict);
    if (!(IGDcls = vmalloc (Vmheap, sizeof (IGDcl_t *) * IGDcln))) {
        SUwarning (0, "IGDflatten", "cannot allocate IGDcls array");
        return -1;
    }
    for (cli = 0, clp = dtfirst (cldict); clp; clp = dtnext (cldict, clp))
        IGDcls[cli++] = clp;

    IGDndn = dtsize (nddict);
    if (!(IGDnds = vmalloc (Vmheap, sizeof (IGDnd_t *) * IGDndn))) {
        SUwarning (0, "IGDflatten", "cannot allocate IGDnds array");
        return -1;
    }
    for (ndi = 0, ndp = dtfirst (nddict); ndp; ndp = dtnext (nddict, ndp)) {
        IGDnds[ndi++] = ndp;
        maxrc = -1, maxcci = -1;
        for (cci = 0; cci < ndp->ccn; cci++)
            if (maxrc < ndp->ccs[cci]->rc)
                maxrc = ndp->ccs[cci]->rc, maxcci = cci;
        if (maxcci > -1)
            ndp->ccp = ndp->ccs[maxcci];
        maxrc = -1, maxcli = -1;
        for (cli = 0; cli < ndp->cln; cli++)
            if (maxrc < ndp->cls[cli]->rc)
                maxrc = ndp->cls[cli]->rc, maxcli = cli;
        if (maxcli > -1)
            ndp->clp = ndp->cls[maxcli];
    }
    if (!(IGDwnds = vmalloc (Vmheap, sizeof (IGDnd_t *) * IGDndn))) {
        SUwarning (0, "IGDflatten", "cannot allocate IGDwnds array");
        return -1;
    }
    qsort (IGDnds, IGDndn, sizeof (IGDnd_t *), cmp);

    if (!(bmws = vmalloc (Vmheap, sizeof (int) * IGDndn))) {
        SUwarning (0, "IGDflatten", "cannot allocate bmws array");
        return -1;
    }
    if (!(bmxs = vmalloc (Vmheap, sizeof (int) * IGDndn))) {
        SUwarning (0, "IGDflatten", "cannot allocate bmxs array");
        return -1;
    }

    return 0;
}

int IGDbegindraw (
    char *fprefix, int findex, int ndn,
    char *igattr, char *tlattr, char *tlurl, char *tlobj
) {
    IGDcl_t *clp;
    IGDnd_t *ndp;
    int ndi, ndk, ndl;
    int tlxoff, tlyoff, tlxsiz, tlysiz;
    int igxoff, igyoff, igxsiz, igysiz;
    UTattr_t *ap;
    char *igbg;
    char *tllab, *tlcl, *tlfn, *tlfs, *tlinfo;
    int tlshowtitle;
    RIop_t top;
    RIarea_t a;
    int w, h;

    tlshowtitle = FALSE;

    if (UTsplitattr (UT_ATTRGROUP_MAIN, igattr) == -1) {
        SUwarning (0, "IGDbegindraw", "cannot parse attrstr");
        return -1;
    }
    if (UTgetattrs (UT_ATTRGROUP_MAIN) == -1) {
        SUwarning (0, "IGDbegindraw", "cannot get invgrid attr");
        return -1;
    }
    if (UTsplitattr (UT_ATTRGROUP_TITLE, tlattr) == -1) {
        SUwarning (0, "IGDbegindraw", "cannot parse attrstr");
        return -1;
    }
    if (UTgetattrs (UT_ATTRGROUP_TITLE) == -1) {
        SUwarning (0, "IGDbegindraw", "cannot get title attr");
        return -1;
    }

    width = height = 0;

    tlxoff = tlyoff = 0, tlxsiz = tlysiz = 0;
    igxoff = igyoff = 0, igxsiz = igysiz = 0;

    ap = &UTattrgroups[UT_ATTRGROUP_TITLE][UT_ATTR_LABEL];
    if (ap->value && ap->value[0]) {
        tllab = ap->value;
        ap = &UTattrgroups[UT_ATTRGROUP_TITLE][UT_ATTR_COLOR];
        tlcl = (ap->value) ? ap->value : "black";
        ap = &UTattrgroups[UT_ATTRGROUP_TITLE][UT_ATTR_FONTNAME];
        tlfn = (ap->value) ? ap->value : "Times-Roman";
        ap = &UTattrgroups[UT_ATTRGROUP_TITLE][UT_ATTR_FONTSIZE];
        tlfs = (ap->value) ? ap->value : "8";
        if (RIgettextsize (tllab, tlfn, tlfs, &top) == -1) {
            SUwarning (0, "IGDbegindraw", "cannot get title text size");
            return -1;
        }
        tlxoff = MARGIN;
        tlxsiz = top.u.font.w;
        tlysiz = top.u.font.h;
        height += tlysiz + MARGIN;
        tlshowtitle = TRUE;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_TITLE][UT_ATTR_INFO];
    tlinfo = ap->value;

    igxoff = MARGIN, igyoff = (height > 0) ? height : MARGIN;
    for (ndi = 0, clp = NULL; ndi < ndn; ndi++) {
        ndp = IGDwnds[ndi];
        if (!clp)
            clp = ndp->clp, ndk = ndi;
        if (ndi == ndn - 1 || clp != IGDwnds[ndi + 1]->clp) {
            ndl = ndi;
            layout (ndk, ndl, igxoff, igyoff + igysiz, &w, &h);
            clp = NULL;
            if (igxsiz < w)
                igxsiz = w;
            igysiz += h;
        }
    }
    width = igxoff + igxsiz + MARGIN;
    if (width < tlxoff + tlxsiz + MARGIN)
        width = tlxoff + tlxsiz + MARGIN;
    height = igyoff + igysiz + MARGIN;

    tlxsiz = width - MARGIN;

    if (!(rip = RIopen (RI_TYPE_IMAGE, fprefix, findex, width, height))) {
        SUwarning (0, "IGDbegindraw", "cannot open image file #%d", findex);
        return -1;
    }

    pageemit = 0;
    if (pagem > 0 && pagei++ % pagem == 0) {
        pageemit = 1;
        sfsprintf (pagebuf, 1000, "image %d", pagei);
    }

    RIaddop (rip, NULL); // reset

    ap = &UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_BGCOLOR];
    if (ap->value) {
        igbg = ap->value;
        if (UTaddbox (rip, 0, 0, width, height, igbg, TRUE) == -1) {
            SUwarning (0, "IGDbegindraw", "cannot add igbg box");
            return -1;
        }
    }

    if (tlshowtitle) {
        if (UTaddtext (
            rip, tllab, tlfn, tlfs, tlcl, tlxoff, tlyoff,
            tlxoff + tlxsiz, tlyoff + tlysiz, 0, -1, FALSE, &w, &h
        ) == -1) {
            SUwarning (0, "IGDbegindraw", "cannot add title text");
            return -1;
        }
        if (pageemit)
            sfsprintf (pagebuf, 1000, "%s", tllab);
    }

    if (pageemit)
        RIaddtoc (rip, ++pageindex, pagebuf);

    if (tlshowtitle && tlurl && tlurl[0] && tlobj && tlobj[0]) {
        a.mode = RI_AREA_AREA;
        a.type = RI_AREA_RECT;
        a.u.rect.x1 = tlxoff;
        a.u.rect.y1 = tlyoff;
        a.u.rect.x2 = tlxoff + tlxsiz - 1;
        a.u.rect.y2 = tlyoff + tlysiz - 1;
        a.pass = 0;
        a.obj = tlobj;
        a.url = tlurl;
        a.info = tlinfo;
        if (RIaddarea (rip, &a) == -1)
            SUwarning (0, "IGDbegindraw", "cannot add area");
    }

    if (tlobj && tlobj[0]) {
        a.mode = RI_AREA_EMBED;
        a.type = RI_AREA_RECT;
        a.u.rect.x1 = 0, a.u.rect.y1 = 0;
        a.u.rect.x2 = width - 1, a.u.rect.y2 = height - 1;
        a.pass = 0;
        a.obj = tlobj;
        a.url = NULL;
        a.info = NULL;
        if (RIaddarea (rip, &a) == -1)
            SUwarning (0, "IGDbegindraw", "cannot add area");
    }

    RIexecop (
        rip, NULL, 0, NULL, "black", "black", "black", "Times-Roman", "10"
    );

    return 0;
}

int IGDenddraw (void) {
    if (RIclose (rip) == -1) {
        SUwarning (0, "IGDenddraw", "cannot save image");
        return -1;
    }

    return 0;
}

int IGDdrawnode (IGDnd_t *ndp, char *url, char *obj, int hdrflag) {
    IGDcl_t *clp;
    RIarea_t a;
    RIop_t op;
    int opi;
    int w, h;

    RIaddop (rip, NULL); // reset
    UTlockcolor = TRUE;

    if (hdrflag) {
        clp = ndp->clp;
        if (clp && clp->lab[0]) {
            if (UTaddlw (rip, clp->lw) == -1) {
                SUwarning (0, "IGDdrawnode", "cannot add node line width");
                return -1;
            }
            if (UTaddbox (
                rip, MARGIN / 2, clp->bmh1, width - MARGIN / 2, clp->bmh2,
                clp->cl, FALSE
            ) == -1) {
                SUwarning (0, "IGDdrawnode", "cannot add node box");
                return -1;
            }
            if (UTaddlw (rip, 1) == -1) {
                SUwarning (0, "IGDdrawnode", "cannot add node line width");
                return -1;
            }
            if (UTaddtext (
                rip, clp->lab, clp->fn, clp->fs, clp->cl,
                0, clp->bmh1 + MARGIN / 2,
                width, clp->bmh1 + MARGIN / 2, 0, -1, FALSE, &w, &h
            ) == -1) {
                SUwarning (0, "IGDdrawnode", "cannot add title text");
                return -1;
            }
            if (clp->url && clp->url[0]) {
                a.mode = RI_AREA_AREA;
                a.type = RI_AREA_RECT;
                a.u.rect.x1 = 0;
                a.u.rect.y1 = clp->bmh1;
                a.u.rect.x2 = a.u.rect.x1 + width;
                a.u.rect.y2 = clp->bmh3;
                a.pass = 0;
                a.obj = clp->obj;
                a.url = clp->url;
                a.info = clp->lab;
                if (RIaddarea (rip, &a) == -1)
                    SUwarning (0, "IGDdrawnode", "cannot add node draw area");
            }
        }
    }

    if (UTaddlw (rip, ndp->lw) == -1) {
        SUwarning (0, "IGDdrawnode", "cannot add node line width");
        return -1;
    }

    UTlockcolor = FALSE;
    if (ndp->fcl[0] && UTaddbox (
        rip, ndp->bmx, ndp->bmy, ndp->bmx + ndp->bmw, ndp->bmy + ndp->bmh,
        ndp->fcl, TRUE
    ) == -1) {
        SUwarning (0, "IGDdrawnode", "cannot add node fill box");
        return -1;
    }
    if (ndp->cl[0]) {
        memset (&op, 0, sizeof (RIop_t));
        op.type = RI_OP_c;
        op.u.color.t = ndp->cl;
        if (RIaddop (rip, &op) == -1) {
            SUwarning (0, "IGDdrawnode", "cannot add c op");
            return -1;
        }
        memset (&op, 0, sizeof (RIop_t));
        op.type = RI_OP_FC;
        op.u.color.t = ndp->cl;
        if (RIaddop (rip, &op) == -1) {
            SUwarning (0, "IGDdrawnode", "cannot add FC op");
            return -1;
        }
    }

    RIsetviewport (rip, TRUE, ndp->bmx, ndp->bmy, ndp->bmw, ndp->bmh);
    for (opi = 0; opi < ndp->opn; opi++) {
        if (RIaddop (rip, &EMops[ndp->opi + opi]) == -1) {
            SUwarning (0, "IGDdrawnode", "cannot add em op");
            return -1;
        }
    }
    RIsetviewport (rip, FALSE, ndp->bmx, ndp->bmy, ndp->bmw, ndp->bmh);

    UTlockcolor = TRUE;
    if (UTaddbox (
        rip, ndp->bmx, ndp->bmy, ndp->bmx + ndp->bmw, ndp->bmy + ndp->bmh,
        ndp->cl, FALSE
    ) == -1) {
        SUwarning (0, "IGDdrawnode", "cannot add node box");
        return -1;
    }
    UTlockcolor = FALSE;

    if (url && url[0]) {
        a.mode = RI_AREA_AREA;
        a.type = RI_AREA_RECT;
        a.u.rect.x1 = ndp->bmx;
        a.u.rect.y1 = ndp->bmy;
        a.u.rect.x2 = ndp->bmx + ndp->bmw - 1;
        a.u.rect.y2 = ndp->bmy + ndp->bmh - 1;
        a.pass = 0;
        a.obj = obj;
        a.url = url;
        a.info = ndp->info;
        if (RIaddarea (rip, &a) == -1)
            SUwarning (0, "IGDdrawnode", "cannot add node draw area");
    }

    RIexecop (
        rip, NULL, 0, NULL, "black", "black", "black", "Times-Roman", "10"
    );

    return 0;
}

static int layout (int ndk, int ndl, int xoff, int yoff, int *wp, int *hp) {
    IGDcl_t *clp;
    int ndi;
    int n, i, j;
    int minw, maxh;
    RIop_t top;

    *wp = 0, *hp = 0;
    clp = IGDwnds[ndk]->clp;

    if (clp && clp->lab[0]) {
        if (RIgettextsize (clp->lab, clp->fn, clp->fs, &top) == -1) {
            SUwarning (0, "layout", "cannot get title text size");
            return -1;
        }
        clp->bmh1 = yoff;
        clp->bmh2 = yoff;
        yoff += top.u.font.h + MARGIN;
        clp->bmh3 = yoff;
        *hp += top.u.font.h + MARGIN;
    }

    for (maxh = 0, minw = IGDwnds[ndk]->bmw, ndi = ndk; ndi <= ndl; ndi++) {
        if (minw > IGDwnds[ndi]->bmw)
            minw = IGDwnds[ndi]->bmw;
        if (maxh < IGDwnds[ndi]->bmh)
            maxh = IGDwnds[ndi]->bmh;
    }
    if (minw == 0 || (n = prefwidth / minw) < 1)
        n = 1;
    if (n > ndl - ndk + 1)
        n = ndl - ndk + 1;

    for (;;) {
        for (i = 0; i < n; i++)
            bmws[i] = 0;
        for (ndi = ndk; ndi <= ndl; ndi++) {
            i = (ndi - ndk) % n;
            if (bmws[i] < IGDwnds[ndi]->bmw)
                bmws[i] = IGDwnds[ndi]->bmw;
        }
        for (*wp = 0, i = 0; i < n; i++) {
            bmxs[i] = *wp;
            *wp += MARGIN + bmws[i];
        }
        if (*wp <= prefwidth || n == 1) {
            for (ndi = ndk; ndi <= ndl; ndi++) {
                i = (ndi - ndk) % n;
                j = (ndi - ndk) / n;
                IGDwnds[ndi]->bmx = xoff + bmxs[i];
                IGDwnds[ndi]->bmy = yoff + j * maxh + j * MARGIN;
            }
            *hp += IGDwnds[ndl]->bmy + maxh + MARGIN - yoff;
            break;
        }
        n--;
    }

    if (clp && clp->lab[0]) {
        clp->bmh2 += *hp;
        *hp += MARGIN;
        if (*wp < top.u.font.w)
            *wp = top.u.font.w;
    }

    return 0;
}

static int cmp (const void *a, const void *b) {
    IGDnd_t *ap, *bp;
    int ret;

    ap = *(IGDnd_t **) a;
    bp = *(IGDnd_t **) b;
    if (
        ap->clp && bp->clp && ap->clp != bp->clp && ap->clp->lab && bp->clp->lab
    )
        if ((ret = strcasecmp (ap->clp->lab, bp->clp->lab)) != 0)
            return ret;
    if ((ret = ap->nclass - bp->nclass) != 0)
        return ret;
    if ((ret = strcasecmp (ap->lab, bp->lab)) != 0)
        return ret;
    if ((ret = strcmp (ap->id, bp->id)) != 0)
        return ret;
    return strcmp (ap->level, bp->level);
}
