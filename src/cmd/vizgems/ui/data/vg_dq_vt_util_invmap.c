#include <ast.h>
#include <ast_float.h>
#include <tok.h>
#include <swift.h>
#include "vg_dq_vt_util.h"

IMPnd_t **IMPnds;
int IMPndn;
IMPed_t **IMPeds;
int IMPedn;

static char *pagemode;
static int pageindex, pagei, pagem, pageemit;
static char pagebuf[1000];

static Dt_t *nddict;
static Dtdisc_t nddisc = {
    sizeof (Dtlink_t), SZ_level + SZ_id, 0, NULL, NULL, NULL, NULL, NULL, NULL
};
static Dt_t *eddict;
static Dtdisc_t eddisc = {
    sizeof (Dtlink_t), sizeof (IMPnd_t *) * 2,
    0, NULL, NULL, NULL, NULL, NULL, NULL
};

#define LL2BM(llx, lly, bmx, bmy) ( \
    bmx = ((llx - llx1) / (llx2 - llx1)) * bmw, \
    bmy = bmh - ((lly - lly1) / (lly2 - lly1)) * bmh \
)
#define GMMARGIN 10
#define NCMARGIN 10

static IMPgeom_t geom;
static int width, height;
static float focusx1, focusx2, focusy1, focusy2;
static char *geomdir;

static RI_t *rip;

static int load (float, float, float, float);
static int adjustpass (int);
static int adjcmp (const void *, const void *);

int IMPinit (char *dir, char *pm) {
    if (!(nddict = dtopen (&nddisc, Dtset))) {
        SUwarning (0, "IMPinit", "cannot create nddict");
        return -1;
    }
    if (!(eddict = dtopen (&eddisc, Dtset))) {
        SUwarning (0, "IMPinit", "cannot create eddict");
        return -1;
    }
    IMPnds = NULL;
    IMPndn = 0;
    IMPeds = NULL;
    IMPedn = 0;

    pagemode = pm;
    pageindex = pagei = pagem = 0;
    if (pagemode && pagemode[0] && strcmp (pagemode, "all") != 0)
        pagem = atoi (pagemode);
    if (pagem < 0)
        pagem = 0;

    UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_BGCOLOR].name = "bgcolor";
    UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_WIDTH].name = "width";
    UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_HEIGHT].name = "height";
    UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_FLAGS].name = "flags";
    UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_COLOR].name = "color";
    UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_LINEWIDTH].name = "linewidth";
    UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_FONTNAME].name = "fontname";
    UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_FONTSIZE].name = "fontsize";
    UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_FONTCOLOR].name = "fontcolor";

    UTattrgroups[UT_ATTRGROUP_TITLE][UT_ATTR_LABEL].name = "label";
    UTattrgroups[UT_ATTRGROUP_TITLE][UT_ATTR_COLOR].name = "color";
    UTattrgroups[UT_ATTRGROUP_TITLE][UT_ATTR_FONTNAME].name = "fontname";
    UTattrgroups[UT_ATTRGROUP_TITLE][UT_ATTR_FONTSIZE].name = "fontsize";
    UTattrgroups[UT_ATTRGROUP_TITLE][UT_ATTR_INFO].name = "info";

    UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_BGCOLOR].name = "bgcolor";
    UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_COLOR].name = "color";
    UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_LINEWIDTH].name = "linewidth";
    UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_FONTNAME].name = "fontname";
    UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_FONTSIZE].name = "fontsize";
    UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_COORD].name = "coord";
    UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_LABEL].name = "label";
    UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_WIDTH].name = "width";
    UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_HEIGHT].name = "height";
    UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_INFO].name = "info";

    UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_COLOR].name = "color";
    UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_LINEWIDTH].name = "linewidth";
    UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_FONTNAME].name = "fontname";
    UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_FONTSIZE].name = "fontsize";
    UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_LABEL].name = "label";
    UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_INFO].name = "info";

    focusx1 = focusy1 = FLT_LONG_MAX;
    focusx2 = focusy2 = FLT_LONG_MIN;

    if (!(geomdir = vmstrdup (Vmheap, dir))) {
        SUwarning (0, "IMPinit", "cannot copy geomdir");
        return -1;
    }

    return 0;
}

IMPnd_t *IMPinsertnd (char *level, char *id, short nclass, char *attrstr) {
    IMPnd_t nd, *ndp, *ndmem;
    UTattr_t *ap;
    EMbb_t bb;
    char *s1;

    memcpy (nd.level, level, SZ_level);
    memcpy (nd.id, id, SZ_id);
    if ((ndp = dtsearch (nddict, &nd)))
        return ndp;

    if (!(ndmem = vmalloc (Vmheap, sizeof (IMPnd_t)))) {
        SUwarning (0, "IMPinsertnd", "cannot allocate ndmem");
        return NULL;
    }
    memset (ndmem, 0, sizeof (IMPnd_t));
    memcpy (ndmem->level, level, SZ_level);
    memcpy (ndmem->id, id, SZ_id);
    if ((ndp = dtinsert (nddict, ndmem)) != ndmem) {
        SUwarning (0, "IMPinsertnd", "cannot insert nd");
        vmfree (Vmheap, ndmem);
        return NULL;
    }
    ndp->nclass = nclass;

    if (UTsplitattr (UT_ATTRGROUP_NODE, attrstr) == -1) {
        SUwarning (0, "IMPinsertnd", "cannot parse attrstr");
        return NULL;
    }
    if (UTgetattrs (UT_ATTRGROUP_NODE) == -1) {
        SUwarning (0, "IMPinsertnd", "cannot get node attr");
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
    if (
        ap->value &&
        strcmp (ap->name, "label") == 0 &&
        strncmp (ap->value, "EMBED:", 6) == 0
    ) {
        if (EMparsenode (
            ndp->level, ndp->id, ap->value + 6, EM_DIR_V,
            &ndp->opi, &ndp->opn, &bb
        ) == -1) {
            SUwarning (0, "IMPinsertnd", "cannot parse node EM contents");
            return NULL;
        }
        if (ndp->bmw < bb.w)
            ndp->bmw = bb.w;
        if (ndp->bmh < bb.h)
            ndp->bmh = bb.h;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_LINEWIDTH];
    ndp->lw = (ap->value && ap->value[0]) ? atoi (ap->value) : 1;
    ap = &UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_BGCOLOR];
    if (!(ndp->bg = vmstrdup (
        Vmheap, (ap->value && ap->value[0]) ? ap->value : "white"
    ))) {
        SUwarning (0, "IMPinsertnd", "cannot copy node bgcolor");
        return NULL;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_COLOR];
    if (!(ndp->cl = vmstrdup (
        Vmheap, (ap->value && ap->value[0]) ? ap->value : "black"
    ))) {
        SUwarning (0, "IMPinsertnd", "cannot copy node color");
        return NULL;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_FONTNAME];
    if (!(ndp->fn = vmstrdup (
        Vmheap, (ap->value && ap->value[0]) ? ap->value : "abc"
    ))) {
        SUwarning (0, "IMPinsertnd", "cannot copy node font name");
        return NULL;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_FONTSIZE];
    if (!(ndp->fs = vmstrdup (
        Vmheap, (ap->value && ap->value[0]) ? ap->value : "10"
    ))) {
        SUwarning (0, "IMPinsertnd", "cannot copy node font size");
        return NULL;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_INFO];
    if (!(ndp->info = vmstrdup (
        Vmheap, (ap->value && ap->value[0]) ? ap->value : id
    ))) {
        SUwarning (0, "IMPinsertnd", "cannot copy node info");
        return NULL;
    }

    ndp->havecoord = FALSE;
    ap = &UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_COORD];
    if (ap->value && ap->value[0] && (s1 = strchr (ap->value, ' '))) {
        *s1 = 0;
        ndp->llx = atof (ap->value), ndp->lly = atof (s1 + 1);
        *s1 = ' ';
        ndp->havecoord = TRUE;
    }

    return ndp;
}

IMPnd_t *IMPfindnd (char *level, char *id) {
    IMPnd_t nd;

    memcpy (nd.level, level, SZ_level);
    memcpy (nd.id, id, SZ_id);
    return dtsearch (nddict, &nd);
}

IMPed_t *IMPinserted (IMPnd_t *ndp1, IMPnd_t *ndp2, char *attrstr) {
    IMPed_t ed, *edp, *edmem;
    UTattr_t *ap;
    EMbb_t bb;

    ed.ndp1 = ndp1;
    ed.ndp2 = ndp2;
    if ((edp = dtsearch (eddict, &ed)))
        return edp;

    if (!(edmem = vmalloc (Vmheap, sizeof (IMPed_t)))) {
        SUwarning (0, "IMPinserted", "cannot allocate edmem");
        return NULL;
    }
    memset (edmem, 0, sizeof (IMPed_t));
    edmem->ndp1 = ndp1, edmem->ndp2 = ndp2;
    if ((edp = dtinsert (eddict, edmem)) != edmem) {
        SUwarning (0, "IMPinserted", "cannot insert ed");
        vmfree (Vmheap, edmem);
        return NULL;
    }

    if (UTsplitattr (UT_ATTRGROUP_EDGE, attrstr) == -1) {
        SUwarning (0, "IMPinserted", "cannot parse attrstr");
        return NULL;
    }
    if (UTgetattrs (UT_ATTRGROUP_EDGE) == -1) {
        SUwarning (0, "IMPinserted", "cannot get edge attr");
        return NULL;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_LABEL];
    if (
        ap->value &&
        strcmp (ap->name, "label") == 0 &&
        strncmp (ap->value, "EMBED:", 6) == 0
    ) {
        if (EMparseedge (
            ndp1->level, ndp1->id, ndp2->level, ndp2->id,
            ap->value + 6, EM_DIR_V, &edp->opi, &edp->opn, &bb
        ) == -1) {
            SUwarning (0, "IMPinserted", "cannot parse edge EM contents");
            return NULL;
        }
    }
    ap = &UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_LINEWIDTH];
    edp->lw = (ap->value && ap->value[0]) ? atoi (ap->value) : 1;
    ap = &UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_COLOR];
    if (!(edp->cl = vmstrdup (
        Vmheap, (ap->value && ap->value[0]) ? ap->value : "black"
    ))) {
        SUwarning (0, "IMPinserted", "cannot copy edge color");
        return NULL;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_FONTNAME];
    if (!(edp->fn = vmstrdup (
        Vmheap, (ap->value && ap->value[0]) ? ap->value : "abc"
    ))) {
        SUwarning (0, "IMPinserted", "cannot copy edge font name");
        return NULL;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_FONTSIZE];
    if (!(edp->fs = vmstrdup (
        Vmheap, (ap->value && ap->value[0]) ? ap->value : "10"
    ))) {
        SUwarning (0, "IMPinserted", "cannot copy edge font size");
        return NULL;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_INFO];
    if (!(edp->info = vmstrdup (
        Vmheap, (ap->value && ap->value[0]) ? ap->value : ""
    ))) {
        SUwarning (0, "IMPinserted", "cannot copy edge info");
        return NULL;
    }

    return edp;
}

int IMPflatten (void) {
    IMPnd_t *ndp;
    int ndi;
    IMPed_t *edp;
    int edi;

    IMPndn = dtsize (nddict);
    if (!(IMPnds = vmalloc (Vmheap, sizeof (IMPnd_t *) * IMPndn))) {
        SUwarning (0, "IMPflatten", "cannot allocate IMPnds array");
        return -1;
    }
    for (ndi = 0, ndp = dtfirst (nddict); ndp; ndp = dtnext (nddict, ndp))
        IMPnds[ndi++] = ndp;

    IMPedn = dtsize (eddict);
    if (!(IMPeds = vmalloc (Vmheap, sizeof (IMPed_t *) * IMPedn))) {
        SUwarning (0, "IMPflatten", "caenot allocate IMPeds array");
        return -1;
    }
    for (edi = 0, edp = dtfirst (eddict); edp; edp = dtnext (eddict, edp))
        IMPeds[edi++] = edp;

    return 0;
}

int IMPbegindraw (
    char *fprefix, int findex,
    char *gmattr, char *tlattr, char *tlurl, char *tlobj
) {
    IMPnd_t *ndp;
    int ndi;
    IMPlabel_t *labelp;
    int labeli;
    IMPpoly_t *polyp;
    int polyi;
    IMPpoint_t *pp;
    int pi;
    float fdx, fdy, rx, llx1, lly1, llx2, lly2;
    int minbmw, maxbmw, maxbmh, bmx, bmy, bmw, bmh;
    int tlxoff, tlyoff, tlxsiz, tlysiz;
    int ncxoff, ncyoff, ncxsiz, ncysiz;
    int gmxoff, gmyoff, gmxsiz, gmysiz;
    UTattr_t *ap;
    int focusflag, noadjflag;
    char *gmbg, *gmcl, *gmfn, *gmfs, *gmfcl;
    int gmlw;
    char *tllab, *tlcl, *tlfn, *tlfs, *tlinfo;
    int tlshowtitle;
    int havenc, ncmaxh, gmndn, mpassi, apassi, apassn;
    RIop_t top;
    RIarea_t a;
    int w, h, dx, dy;

    if (IMPndn == 0)
        return 0;

    tlshowtitle = FALSE;

    if (UTsplitattr (UT_ATTRGROUP_MAIN, gmattr) == -1) {
        SUwarning (0, "IMPbegindraw", "cannot parse attrstr");
        return -1;
    }
    if (UTgetattrs (UT_ATTRGROUP_MAIN) == -1) {
        SUwarning (0, "IMPbegindraw", "cannot get invmap attr");
        return -1;
    }
    if (UTsplitattr (UT_ATTRGROUP_TITLE, tlattr) == -1) {
        SUwarning (0, "IMPbegindraw", "cannot parse attrstr");
        return -1;
    }
    if (UTgetattrs (UT_ATTRGROUP_TITLE) == -1) {
        SUwarning (0, "IMPbegindraw", "cannot get title attr");
        return -1;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_FLAGS];
    focusflag = (ap->value && strstr (ap->value, "focus")) ? TRUE : FALSE;
    noadjflag = (ap->value && strstr (ap->value, "noadjust")) ? TRUE : FALSE;
    ap = &UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_WIDTH];
    maxbmw = maxbmh = 0;
    if (ap->value && ap->value[0])
        maxbmw = atoi (ap->value) - 2 * GMMARGIN;
    if (maxbmw <= 0)
        maxbmw = 1000;
    minbmw = maxbmw / 2.0;
    ap = &UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_HEIGHT];
    if (ap->value && ap->value[0])
        maxbmh = atoi (ap->value) - 2 * GMMARGIN;
    if (maxbmh <= 0)
        maxbmh = 600;

    focusx1 = focusy1 = FLT_LONG_MAX;
    focusx2 = focusy2 = FLT_LONG_MIN;
    for (ndi = 0, gmndn = 0; ndi < IMPndn; ndi++) {
        ndp = IMPnds[ndi];
        if (!ndp->havecoord)
            continue;
        if (focusx1 > ndp->llx)
            focusx1 = ndp->llx;
        if (focusy1 > ndp->lly)
            focusy1 = ndp->lly;
        if (focusx2 < ndp->llx)
            focusx2 = ndp->llx;
        if (focusy2 < ndp->lly)
            focusy2 = ndp->lly;
        gmndn++;
    }
    if (gmndn > 0) {
        if (load (focusx1, focusy1, focusx2, focusy2) == -1) {
            SUwarning (0, "IMPbegindraw", "cannot load geom file");
            return -1;
        }
        focusx1 = focusy1 = FLT_LONG_MAX;
        focusx2 = focusy2 = FLT_LONG_MIN;
        for (ndi = 0, gmndn = 0; ndi < IMPndn; ndi++) {
            ndp = IMPnds[ndi];
            if (!ndp->havecoord)
                continue;
            if (
                ndp->llx < geom.llx1 || ndp->llx > geom.llx2 ||
                ndp->lly < geom.lly1 || ndp->lly > geom.lly2
            ) {
                ndp->havecoord = FALSE;
                continue;
            }
            if (focusx1 > ndp->llx)
                focusx1 = ndp->llx;
            if (focusy1 > ndp->lly)
                focusy1 = ndp->lly;
            if (focusx2 < ndp->llx)
                focusx2 = ndp->llx;
            if (focusy2 < ndp->lly)
                focusy2 = ndp->lly;
            gmndn++;
        }
    }
    if (gmndn == 0) {
        geom.polyn = 0;
        focusx1 = geom.llx1 = -0.001, focusx2 = geom.llx2 = 0.001;
        focusy1 = geom.lly1 = -0.001, focusy2 = geom.lly2 = 0.001;
        bmw = maxbmw, bmh = 0;
        llx1 = geom.llx1, lly1 = geom.lly1;
        llx2 = geom.llx2, lly2 = geom.lly2;
    } else {
        if (focusx1 == focusx2)
            focusx1 -= 1, focusx2 += 1;
        if (focusy1 == focusy2)
            focusy1 -= 0.5, focusy2 += 0.5;

        if (focusflag) {
            fdx = focusx2 - focusx1, fdy = focusy2 - focusy1;
            rx = fdx / (geom.llx2 - geom.llx1);
            bmw = minbmw + (maxbmw - minbmw) * rx;
            bmh = maxbmw * fdy / fdx;
            llx1 = focusx1 - 0.05 * fdx, llx2 = focusx2 + 0.05 * fdx;
            lly1 = focusy1 - 0.025 * fdy, lly2 = focusy2 + 0.025 * fdy;
        } else {
            fdx = geom.llx2 - geom.llx1, fdy = geom.lly2 - geom.lly1;
            rx = 1.0;
            bmw = maxbmw;
            bmh = maxbmw * fdy / fdx;
            llx1 = geom.llx1, lly1 = geom.lly1;
            llx2 = geom.llx2, lly2 = geom.lly2;
        }

        for (mpassi = 0; ; mpassi++) {
            for (ndi = 0; ndi < IMPndn; ndi++) {
                ndp = IMPnds[ndi];
                if (ndp->havecoord)
                    LL2BM (ndp->llx, ndp->lly, ndp->bmx, ndp->bmy);
            }
            if (noadjflag)
                break;
            switch (mpassi) {
            case 0:  apassn =  2; break;
            case 1:  apassn =  5; break;
            case 2:  apassn = 10; break;
            case 3:  apassn = 20; break;
            default: apassn = 40; break;
            }
            for (apassi = 0; apassi < apassn; apassi++)
                if (adjustpass (apassi) == 0)
                    break;
            if (apassi < apassn || mpassi >= 4 || bmw > 3200)
                break;
            bmw = (mpassi + 1) * maxbmw;
            bmh = (mpassi + 1) * maxbmw * fdy / fdx;
        }
    }

    tlxoff = tlyoff = 0, tlxsiz = tlysiz = 0;
    gmxoff = gmyoff = 0, gmxsiz = gmysiz = 0;
    ncxoff = ncyoff = 0, ncxsiz = ncysiz = 0;

    havenc = FALSE;
    gmxsiz = bmw, gmysiz = bmh;
    for (ncmaxh = 0, ndi = 0; ndi < IMPndn; ndi++) {
        ndp = IMPnds[ndi];
        if (!ndp->havecoord) {
            if (ncmaxh < ndp->bmh)
                ncmaxh = ndp->bmh;
        }
    }
    for (ndi = 0; ndi < IMPndn; ndi++) {
        ndp = IMPnds[ndi];
        if (!ndp->havecoord) {
            if (ncxsiz + NCMARGIN + ndp->bmw > gmxsiz)
                ncxsiz = 0, ncysiz += NCMARGIN + ncmaxh;
            ndp->bmx = ncxsiz, ndp->bmy = ncysiz;
            ncxsiz += NCMARGIN + ndp->bmw;
            havenc = TRUE;
            continue;
        }
        if (gmxoff > ndp->bmx - ndp->bmw / 2.0)
            gmxoff = ndp->bmx - ndp->bmw / 2.0;
        if (gmyoff > ndp->bmy - ndp->bmh / 2.0)
            gmyoff = ndp->bmy - ndp->bmh / 2.0;
        if (gmxsiz < ndp->bmx + ndp->bmw / 2.0)
            gmxsiz = ndp->bmx + ndp->bmw / 2.0;
        if (gmysiz < ndp->bmy + ndp->bmh / 2.0)
            gmysiz = ndp->bmy + ndp->bmh / 2.0;
    }
    if (havenc)
        ncysiz += ncmaxh;
    dx = - gmxoff, dy = - gmyoff;
    gmxsiz += dx, gmysiz += dy, ncxsiz = gmxsiz;

    tlxoff = 0, tlyoff = 0, tlxsiz = 0, tlysiz = 0;
    ap = &UTattrgroups[UT_ATTRGROUP_TITLE][UT_ATTR_LABEL];
    if (ap->value && ap->value[0]) {
        tllab = ap->value;
        ap = &UTattrgroups[UT_ATTRGROUP_TITLE][UT_ATTR_COLOR];
        tlcl = (ap->value) ? ap->value : "black";
        ap = &UTattrgroups[UT_ATTRGROUP_TITLE][UT_ATTR_FONTNAME];
        tlfn = (ap->value) ? ap->value : "Times-Roman";
        ap = &UTattrgroups[UT_ATTRGROUP_TITLE][UT_ATTR_FONTSIZE];
        tlfs = (ap->value) ? ap->value : "8";
        if (RIgettextsize ("W", tlfn, tlfs, &top) == -1) {
            SUwarning (0, "IMPbegindraw", "cannot get title text size");
            return -1;
        }
        tlysiz = top.u.font.h;
        tlshowtitle = TRUE;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_TITLE][UT_ATTR_INFO];
    tlinfo = ap->value;

    tlxsiz = gmxsiz + 2 * GMMARGIN;
    gmxoff = GMMARGIN, gmyoff = tlyoff + tlysiz + GMMARGIN;
    ncxoff = GMMARGIN, ncyoff = gmyoff + gmysiz + GMMARGIN;

    width = tlxsiz, height = ncyoff + ncysiz;

    for (ndi = 0; ndi < IMPndn; ndi++) {
        ndp = IMPnds[ndi];
        if (ndp->havecoord) {
            ndp->bmx = gmxoff + dx + ndp->bmx - ndp->bmw / 2.0;
            ndp->bmy = gmyoff + dy + ndp->bmy - ndp->bmh / 2.0;
        } else {
            ndp->bmx = ncxoff + ndp->bmx;
            ndp->bmy = ncyoff + ndp->bmy;
        }
    }

    if (!(rip = RIopen (RI_TYPE_IMAGE, fprefix, findex, width, height))) {
        SUwarning (0, "IMPbegindraw", "cannot open image file #%d", findex);
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
        gmbg = ap->value;
        if (UTaddbox (rip, 0, 0, width, height, gmbg, TRUE) == -1) {
            SUwarning (0, "IMPbegindraw", "cannot add gmbg box");
            return -1;
        }
    }
    ap = &UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_COLOR];
    gmcl = (ap->value && ap->value[0]) ? ap->value : "black";
    ap = &UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_LINEWIDTH];
    gmlw = (ap->value && ap->value[0]) ? atoi (ap->value) : 1;
    ap = &UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_FONTNAME];
    gmfn = (ap->value) ? ap->value : "Times-Roman";
    ap = &UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_FONTSIZE];
    gmfs = (ap->value) ? ap->value : "8";
    ap = &UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_FONTCOLOR];
    gmfcl = (ap->value) ? ap->value : "black";

    if (tlshowtitle) {
        if (UTaddtext (
            rip, tllab, tlfn, tlfs, tlcl, tlxoff, tlyoff,
            tlxoff + tlxsiz, tlyoff + tlysiz, 0, -1, FALSE, &w, &h
        ) == -1) {
            SUwarning (0, "IMPbegindraw", "cannot add title text");
            return -1;
        }
        if (pageemit)
            sfsprintf (pagebuf, 1000, "%s", tllab);
    }

    if (pageemit)
        RIaddtoc (rip, ++pageindex, pagebuf);

    if (UTaddclip (rip, gmxoff, gmyoff, gmxsiz, gmysiz) == -1) {
        SUwarning (0, "IMPbegindraw", "cannot add clip rect");
        return -1;
    }
    if (UTaddlw (rip, gmlw) == -1) {
        SUwarning (0, "IMPbegindraw", "cannot add node line width");
        return -1;
    }
    for (polyi = 0; polyi < geom.polyn; polyi++) {
        polyp = &geom.polys[polyi];
        if (!polyp->name || polyp->pn < 1)
            continue;
        if (UTaddpolybegin (rip, gmcl, RI_STYLE_NONE) == -1) {
            SUwarning (0, "IMPbegindraw", "cannot add poly begin");
            return -1;
        }
        for (pi = 0; pi < polyp->pn; pi++) {
            pp = &polyp->ps[pi];
            LL2BM (pp->llx, pp->lly, bmx, bmy);
            if (UTaddpolypoint (
                rip, gmxoff + dx + bmx, gmyoff + dy + bmy
            ) == -1) {
                SUwarning (0, "IMPbegindraw", "cannot add poly point");
                return -1;
            }
        }
        if (UTaddpolyend (rip) == -1) {
            SUwarning (0, "IMPbegindraw", "cannot add poly end");
            return -1;
        }
    }
    for (labeli = 0; labeli < geom.labeln; labeli++) {
        labelp = &geom.labels[labeli];
        if (!labelp->name)
            continue;
        LL2BM (labelp->llx, labelp->lly, bmx, bmy);
        if (UTaddtext (
            rip, labelp->name, gmfn, gmfs, gmfcl,
            gmxoff + dx + bmx, gmyoff + dy + bmy,
            gmxoff + dx + bmx, gmyoff + dy + bmy, 0, 0, FALSE, &w, &h
        ) == -1) {
            SUwarning (0, "IMPbegindraw", "cannot add label text");
            return -1;
        }
    }
    if (UTaddclip (rip, 0, 0, width, height) == -1) {
        SUwarning (0, "IMPbegindraw", "cannot add clip rect");
        return -1;
    }

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
            SUwarning (0, "IMPbegindraw", "cannot add area");
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
            SUwarning (0, "IMPbegindraw", "cannot add area");
    }

    RIexecop (
        rip, NULL, 0, NULL, "black", "black", "black", "Times-Roman", "10"
    );

    return 0;
}

int IMPenddraw (void) {
    if (geom.polyn == 0 && IMPndn == 0)
        return 0;

    if (RIclose (rip) == -1) {
        SUwarning (0, "IMPenddraw", "cannot save image");
        return -1;
    }

    return 0;
}

int IMPdrawnode (IMPnd_t *ndp, char *url, char *obj) {
    RIarea_t a;
    int opi;

    if (geom.polyn == 0 && IMPndn == 0)
        return 0;

    RIaddop (rip, NULL); // reset
    UTlockcolor = FALSE;

    if (UTaddlw (rip, ndp->lw) == -1) {
        SUwarning (0, "IMPdrawnode", "cannot add node line width");
        return -1;
    }

    if (UTaddbox (
        rip, ndp->bmx, ndp->bmy, ndp->bmx + ndp->bmw, ndp->bmy + ndp->bmh,
        ndp->bg, TRUE
    ) == -1) {
        SUwarning (0, "IMPdrawnode", "cannot add node bgbox");
        return -1;
    }

    RIsetviewport (rip, TRUE, ndp->bmx, ndp->bmy, ndp->bmw, ndp->bmh);
    for (opi = 0; opi < ndp->opn; opi++) {
        if (RIaddop (rip, &EMops[ndp->opi + opi]) == -1) {
            SUwarning (0, "IMPdrawnode", "cannot add em op");
            return -1;
        }
    }
    RIsetviewport (rip, FALSE, ndp->bmx, ndp->bmy, ndp->bmw, ndp->bmh);
    UTlockcolor = TRUE;

    if (UTaddbox (
        rip, ndp->bmx, ndp->bmy, ndp->bmx + ndp->bmw, ndp->bmy + ndp->bmh,
        ndp->cl, FALSE
    ) == -1) {
        SUwarning (0, "IMPdrawnode", "cannot add node box");
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
            SUwarning (0, "IMPdrawnode", "cannot add node draw area");
    }

    RIexecop (
        rip, NULL, 0, NULL, "black", "black", "black", "Times-Roman", "10"
    );

    return 0;
}

int IMPdrawedge (IMPed_t *edp, char *url, char *obj) {
    int opi;

    if (geom.polyn == 0 && IMPndn == 0)
        return 0;

    RIaddop (rip, NULL); // reset

    if (!edp->ndp1->havecoord || !edp->ndp2->havecoord)
        return 0;

    if (UTaddlw (rip, edp->lw) == -1) {
        SUwarning (0, "IMPdrawedge", "cannot add edge line width");
        return -1;
    }

    for (opi = 0; opi < edp->opn; opi++) {
        if (RIaddop (rip, &EMops[edp->opi + opi]) == -1) {
            SUwarning (0, "IMPdrawedge", "cannot add em op");
            return -1;
        }
    }

    if (UTaddline (
        rip, edp->ndp1->bmx + edp->ndp1->bmw / 2.0,
        edp->ndp1->bmy + edp->ndp1->bmh / 2.0,
        edp->ndp2->bmx + edp->ndp2->bmw / 2.0,
        edp->ndp2->bmy + edp->ndp2->bmh / 2.0, edp->cl
    ) == -1) {
        SUwarning (0, "IMPdrawedge", "cannot add edge line");
        return -1;
    }

    RIexecop (
        rip, NULL, 0, NULL, "black", "black", "black", "Times-Roman", "10"
    );

    return 0;
}

static int load (float llx1, float lly1, float llx2, float lly2) {
    Sfio_t *fp;
    char *line, *s1, *s2, *s3, *s4, file[PATH_MAX];
    float maxiarea, iarea, maxgarea, garea;
    float bbx1, bby1, bbx2, bby2, cx1, cy1, cx2, cy2;
    IMPlabel_t *labelp;
    int labeli;
    IMPpoly_t *polyp;
    int polyi;
    IMPpoint_t *pp;
    int pi;

    file[0] = 0;
    maxiarea = -1.0;
    maxgarea = 0.0;
    if (llx1 == llx2)
        llx1 -= 0.1, llx2 += 0.1;
    if (lly1 == lly2)
        lly1 -= 0.1, lly2 += 0.1;
    if (!(fp = sfopen (NULL, sfprints ("%s/geom.dir", geomdir), "r"))) {
        SUwarning (0, "load", "cannot open geom.dir file");
        return -1;
    }
    while ((line = sfgetr (fp, '\n', 1))) {
        if (
            !(s1 = strchr (line, ' ')) || !(s2 = strchr (s1 + 1, ' ')) ||
            !(s3 = strchr (s2 + 1, ' ')) || !(s4 = strchr (s3 + 1, ' '))
        ) {
            SUwarning (0, "load", "bad bbox line %s", line);
            return -1;
        }
        *s1++ = 0, *s2++ = 0, *s3++ = 0, *s4++ = 0;
        bbx1 = atof (s1), bby1 = atof (s2);
        bbx2 = atof (s3), bby2 = atof (s4);
        if (llx1 > bbx2)
            cx1 = cx2 = bbx2;
        else if (llx2 < bbx1)
            cx1 = cx2 = bbx1;
        else {
            cx1 = (llx1 < bbx1) ? bbx1 : llx1;
            cx2 = (llx2 > bbx2) ? bbx2 : llx2;
        }
        if (lly1 > bby2)
            cy1 = cy2 = bby2;
        else if (lly2 < bby1)
            cy1 = cy2 = bby1;
        else {
            cy1 = (lly1 < bby1) ? bby1 : lly1;
            cy2 = (lly2 > bby2) ? bby2 : lly2;
        }
        iarea = (cx2 - cx1) * (cy2 - cy1);
        garea = (bbx2 - bbx1) * (bby2 - bby1);
        if (maxiarea < iarea) {
            maxiarea = iarea;
            maxgarea = garea;
            strcpy (file, line);
        } else if (maxiarea == iarea && maxgarea > garea) {
            maxiarea = iarea;
            maxgarea = garea;
            strcpy (file, line);
        }
    }
    sfclose (fp);

    if (!file[0]) {
        SUwarning (0, "load", "cannot find geometry file");
        return -1;
    }

    if (!(fp = sfopen (NULL, sfprints ("%s/%s", geomdir, file), "r"))) {
        SUwarning (0, "load", "cannot open geom file %s", file);
        return -1;
    }

    geom.llx1 = geom.lly1 = FLT_LONG_MAX;
    geom.llx2 = geom.lly2 = FLT_LONG_MIN;

    if (!(line = sfgetr (fp, '\n', 1))) {
        SUwarning (0, "load", "empty geom file");
        return -1;
    }
    geom.labeln = atoi (line);
    if (!(geom.labels = vmalloc (Vmheap, sizeof (IMPlabel_t) * geom.labeln))) {
        SUwarning (0, "load", "cannot allocate labels array");
        return -1;
    }
    for (labeli = 0; labeli < geom.labeln; labeli++) {
        labelp = &geom.labels[labeli];
        if (!(line = sfgetr (fp, '\n', 1))) {
            SUwarning (0, "load", "missing label name");
            return -1;
        }
        if (!(labelp->name = vmstrdup (Vmheap, line))) {
            SUwarning (0, "load", "cannot copy label name");
            return -1;
        }
        if (!(line = sfgetr (fp, '\n', 1))) {
            SUwarning (0, "load", "missing label point %d", labeli);
            return -1;
        }
        if (!(s1 = strchr (line, ' '))) {
            SUwarning (0, "load", "bad coord line %s", line);
            return -1;
        }
        *s1++ = 0;
        labelp->llx = atof (line), labelp->lly = atof (s1);
        if (geom.llx1 > labelp->llx)
            geom.llx1 = labelp->llx;
        if (geom.lly1 > labelp->lly)
            geom.lly1 = labelp->lly;
        if (geom.llx2 < labelp->llx)
            geom.llx2 = labelp->llx;
        if (geom.lly2 < labelp->lly)
            geom.lly2 = labelp->lly;
    }

    if (!(line = sfgetr (fp, '\n', 1))) {
        SUwarning (0, "load", "empty geom file");
        return -1;
    }
    geom.polyn = atoi (line);
    if (!(geom.polys = vmalloc (Vmheap, sizeof (IMPpoly_t) * geom.polyn))) {
        SUwarning (0, "load", "cannot allocate polys array");
        return -1;
    }
    for (polyi = 0; polyi < geom.polyn; polyi++) {
        polyp = &geom.polys[polyi];
        if (!(line = sfgetr (fp, '\n', 1))) {
            SUwarning (0, "load", "missing polygon name");
            return -1;
        }
        if (!(polyp->name = vmstrdup (Vmheap, line))) {
            SUwarning (0, "load", "cannot copy polygon name");
            return -1;
        }
        if (!(line = sfgetr (fp, '\n', 1))) {
            SUwarning (0, "load", "missing polygon size");
            return -1;
        }
        polyp->pn = atoi (line);
        if (!(polyp->ps = vmalloc (Vmheap, sizeof (IMPpoint_t) * polyp->pn))) {
            SUwarning (0, "load", "cannot allocate points");
            return -1;
        }
        for (pi = 0; pi < polyp->pn; pi++) {
            pp = &polyp->ps[pi];
            if (!(line = sfgetr (fp, '\n', 1))) {
                SUwarning (0, "load", "missing point %d %d", polyi, pp);
                return -1;
            }
            if (!(s1 = strchr (line, ' '))) {
                SUwarning (0, "load", "bad coord line %s", line);
                return -1;
            }
            *s1++ = 0;
            pp->llx = atof (line), pp->lly = atof (s1);
            if (geom.llx1 > pp->llx)
                geom.llx1 = pp->llx;
            if (geom.lly1 > pp->lly)
                geom.lly1 = pp->lly;
            if (geom.llx2 < pp->llx)
                geom.llx2 = pp->llx;
            if (geom.lly2 < pp->lly)
                geom.lly2 = pp->lly;
        }
    }

    sfclose (fp);
    if (geom.llx1 == geom.llx2)
        geom.llx1 -= 4, geom.llx2 += 4;
    if (geom.lly1 == geom.lly2)
        geom.lly1 -= 4, geom.lly2 += 4;

    return 0;
}

static int adjustpass (int mode) {
    IMPnd_t *ndp1, *ndp2;
    int ndi1, ndi2;
    int ndn;
    int ny, dw, dh;

    qsort (IMPnds, IMPndn, sizeof (IMPnd_t *), adjcmp);

    for (ndi1 = 0; ndi1 < IMPndn; ndi1++) {
        IMPnds[ndi1]->next = -1, IMPnds[ndi1]->done = 0;
        IMPnds[ndi1]->ovw = IMPnds[ndi1]->ovh = 0;
    }

    for (ndn = 0, ndi1 = 0; ndi1 < IMPndn - 1; ndi1++) {
        ndp1 = IMPnds[ndi1];
        if (!ndp1->havecoord)
            continue;
        for (ndi2 = ndi1 + 1; ndi2 < IMPndn; ndi2++) {
            ndp2 = IMPnds[ndi2];
            if (!ndp2->havecoord)
                continue;
            if (ndp1->bmy + ndp1->bmh / 2 <= ndp2->bmy - ndp2->bmh / 2)
                continue;
            if (ndp1->bmx - ndp1->bmw / 2 >= ndp2->bmx + ndp2->bmw / 2)
                continue;
            if (ndp1->bmx + ndp1->bmw / 2 <= ndp2->bmx - ndp2->bmw / 2)
                continue;
            ndp1->next = ndi2;
            ndp1->ovw = (
                ndp1->bmx + ndp1->bmw / 2
            ) - (ndp2->bmx - ndp2->bmw / 2);
            ndp1->ovh = (
                ndp1->bmy + ndp1->bmh / 2
            ) - (ndp2->bmy - ndp2->bmh / 2);
            ndn++;
            break;
        }
    }

    if (ndn == 0)
        return 0;

    switch (mode) {
    case 0:
        for (ndi1 = 0; ndi1 < IMPndn; ndi1++) {
            ndp1 = IMPnds[ndi1];
            if (ndp1->done || ndp1->next == -1)
                continue;
            ndp2 = IMPnds[ndp1->next];
            if (ndp1->bmy - ndp2->bmy < -3 || ndp1->bmy - ndp2->bmy > 3)
                continue;
            dw = ndp1->ovw;
            ndp1->bmx -= (dw / 1.95);
            ndp2->bmx += (dw / 1.95);
            ndp1->done = 1;
            ndp2->done = 1;
        }
        break;
    default:
        for (ndi1 = 0; ndi1 < IMPndn; ndi1++) {
            ndp1 = IMPnds[ndi1];
            if (ndp1->done || ndp1->next == -1)
                continue;
            for (ndn = 0, dh = 0, ndi2 = ndi1; ndi2 != -1; ndi2 = ndp2->next) {
                ndp2 = IMPnds[ndi2];
                ndp2->done = 1;
                ndn++;
                dh += ndp2->ovh;
            }
            ndp1->bmy -= (dh / 1.5);
            ny = ndp1->bmy + ndp1->bmh / 2;
            for (
                ndn = 0, dh = 0, ndi2 = ndp1->next;
                ndi2 != -1; ndi2 = ndp2->next
            ) {
                ndp2 = IMPnds[ndi2];
                ndp2->bmy = ny + ndp2->bmh / 2.0;
                ny = ndp2->bmy + ndp2->bmh / 2.0;
            }
        }
        break;
    }

    return 1;
}

static int adjcmp (const void *a, const void *b) {
    IMPnd_t *ap, *bp;

    ap = *(IMPnd_t **) a;
    bp = *(IMPnd_t **) b;
    if (ap->bmy != bp->bmy)
        return -(bp->bmy - ap->bmy);
    return bp->bmx - ap->bmx;
}
