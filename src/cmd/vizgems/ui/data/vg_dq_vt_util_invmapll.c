#include <ast.h>
#include <ast_float.h>
#include <tok.h>
#include <swift.h>
#include "vg_dq_vt_util.h"

IMPLLnd_t **IMPLLnds;
int IMPLLndn;
IMPLLed_t **IMPLLeds;
int IMPLLedn;

static char *pagemode;
static int pageindex, pagei, pagem, pageemit;

static Dt_t *nddict;
static Dtdisc_t nddisc = {
    sizeof (Dtlink_t), SZ_level + SZ_id, 0, NULL, NULL, NULL, NULL, NULL, NULL
};
static Dt_t *eddict;
static Dtdisc_t eddisc = {
    sizeof (Dtlink_t), sizeof (IMPLLnd_t *) * 2,
    0, NULL, NULL, NULL, NULL, NULL, NULL
};

static int width, height;
static float focusx1, focusx2, focusy1, focusy2;
static Sfio_t *hfp, *lfp;

static RI_t *rip;

int IMPLLinit (char *pm) {
    if (!(nddict = dtopen (&nddisc, Dtset))) {
        SUwarning (0, "IMPLLinit", "cannot create nddict");
        return -1;
    }
    if (!(eddict = dtopen (&eddisc, Dtset))) {
        SUwarning (0, "IMPLLinit", "cannot create eddict");
        return -1;
    }
    IMPLLnds = NULL;
    IMPLLndn = 0;
    IMPLLeds = NULL;
    IMPLLedn = 0;

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

    hfp = NULL;

    return 0;
}

IMPLLnd_t *IMPLLinsertnd (char *level, char *id, short nclass, char *attrstr) {
    IMPLLnd_t nd, *ndp, *ndmem;
    UTattr_t *ap;
    EMbb_t bb;
    char *s1;

    memcpy (nd.level, level, SZ_level);
    memcpy (nd.id, id, SZ_id);
    if ((ndp = dtsearch (nddict, &nd)))
        return ndp;

    if (!(ndmem = vmalloc (Vmheap, sizeof (IMPLLnd_t)))) {
        SUwarning (0, "IMPLLinsertnd", "cannot allocate ndmem");
        return NULL;
    }
    memset (ndmem, 0, sizeof (IMPLLnd_t));
    memcpy (ndmem->level, level, SZ_level);
    memcpy (ndmem->id, id, SZ_id);
    if ((ndp = dtinsert (nddict, ndmem)) != ndmem) {
        SUwarning (0, "IMPLLinsertnd", "cannot insert nd");
        vmfree (Vmheap, ndmem);
        return NULL;
    }
    ndp->nclass = nclass;

    if (UTsplitattr (UT_ATTRGROUP_NODE, attrstr) == -1) {
        SUwarning (0, "IMPLLinsertnd", "cannot parse attrstr");
        return NULL;
    }
    if (UTgetattrs (UT_ATTRGROUP_NODE) == -1) {
        SUwarning (0, "IMPLLinsertnd", "cannot get node attr");
        return NULL;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_WIDTH];
    if (ap->value && ap->value[0])
        ndp->w = atoi (ap->value);
    else
        ndp->w = 100;
    ap = &UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_HEIGHT];
    if (ap->value && ap->value[0])
        ndp->h = atoi (ap->value);
    else
        ndp->h = 50;
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
            SUwarning (0, "IMPLLinsertnd", "cannot parse node EM contents");
            return NULL;
        }
        if (ndp->w < bb.w)
            ndp->w = bb.w;
        if (ndp->h < bb.h)
            ndp->h = bb.h;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_LINEWIDTH];
    ndp->lw = (ap->value && ap->value[0]) ? atoi (ap->value) : 1;
    ap = &UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_BGCOLOR];
    if (!(ndp->bg = vmstrdup (
        Vmheap, (ap->value && ap->value[0]) ? ap->value : "white"
    ))) {
        SUwarning (0, "IMPLLinsertnd", "cannot copy node bgcolor");
        return NULL;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_COLOR];
    if (!(ndp->cl = vmstrdup (
        Vmheap, (ap->value && ap->value[0]) ? ap->value : "black"
    ))) {
        SUwarning (0, "IMPLLinsertnd", "cannot copy node color");
        return NULL;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_FONTNAME];
    if (!(ndp->fn = vmstrdup (
        Vmheap, (ap->value && ap->value[0]) ? ap->value : "abc"
    ))) {
        SUwarning (0, "IMPLLinsertnd", "cannot copy node font name");
        return NULL;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_FONTSIZE];
    if (!(ndp->fs = vmstrdup (
        Vmheap, (ap->value && ap->value[0]) ? ap->value : "10"
    ))) {
        SUwarning (0, "IMPLLinsertnd", "cannot copy node font size");
        return NULL;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_INFO];
    if (!(ndp->info = vmstrdup (
        Vmheap, (ap->value && ap->value[0]) ? ap->value : id
    ))) {
        SUwarning (0, "IMPLLinsertnd", "cannot copy node info");
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

IMPLLnd_t *IMPLLfindnd (char *level, char *id) {
    IMPLLnd_t nd;

    memcpy (nd.level, level, SZ_level);
    memcpy (nd.id, id, SZ_id);
    return dtsearch (nddict, &nd);
}

IMPLLed_t *IMPLLinserted (IMPLLnd_t *ndp1, IMPLLnd_t *ndp2, char *attrstr) {
    IMPLLed_t ed, *edp, *edmem;
    UTattr_t *ap;
    EMbb_t bb;

    ed.ndp1 = ndp1;
    ed.ndp2 = ndp2;
    if ((edp = dtsearch (eddict, &ed)))
        return edp;

    if (!(edmem = vmalloc (Vmheap, sizeof (IMPLLed_t)))) {
        SUwarning (0, "IMPLLinserted", "cannot allocate edmem");
        return NULL;
    }
    memset (edmem, 0, sizeof (IMPLLed_t));
    edmem->ndp1 = ndp1, edmem->ndp2 = ndp2;
    if ((edp = dtinsert (eddict, edmem)) != edmem) {
        SUwarning (0, "IMPLLinserted", "cannot insert ed");
        vmfree (Vmheap, edmem);
        return NULL;
    }

    if (UTsplitattr (UT_ATTRGROUP_EDGE, attrstr) == -1) {
        SUwarning (0, "IMPLLinserted", "cannot parse attrstr");
        return NULL;
    }
    if (UTgetattrs (UT_ATTRGROUP_EDGE) == -1) {
        SUwarning (0, "IMPLLinserted", "cannot get edge attr");
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
            SUwarning (0, "IMPLLinserted", "cannot parse edge EM contents");
            return NULL;
        }
    }
    ap = &UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_LINEWIDTH];
    edp->lw = (ap->value && ap->value[0]) ? atoi (ap->value) : 1;
    ap = &UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_COLOR];
    if (!(edp->cl = vmstrdup (
        Vmheap, (ap->value && ap->value[0]) ? ap->value : "black"
    ))) {
        SUwarning (0, "IMPLLinserted", "cannot copy edge color");
        return NULL;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_FONTNAME];
    if (!(edp->fn = vmstrdup (
        Vmheap, (ap->value && ap->value[0]) ? ap->value : "abc"
    ))) {
        SUwarning (0, "IMPLLinserted", "cannot copy edge font name");
        return NULL;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_FONTSIZE];
    if (!(edp->fs = vmstrdup (
        Vmheap, (ap->value && ap->value[0]) ? ap->value : "10"
    ))) {
        SUwarning (0, "IMPLLinserted", "cannot copy edge font size");
        return NULL;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_INFO];
    if (!(edp->info = vmstrdup (
        Vmheap, (ap->value && ap->value[0]) ? ap->value : ""
    ))) {
        SUwarning (0, "IMPLLinserted", "cannot copy edge info");
        return NULL;
    }

    return edp;
}

int IMPLLflatten (void) {
    IMPLLnd_t *ndp;
    int ndi;
    IMPLLed_t *edp;
    int edi;

    IMPLLndn = dtsize (nddict);
    if (!(IMPLLnds = vmalloc (Vmheap, sizeof (IMPLLnd_t *) * IMPLLndn))) {
        SUwarning (0, "IMPLLflatten", "cannot allocate IMPLLnds array");
        return -1;
    }
    for (ndi = 0, ndp = dtfirst (nddict); ndp; ndp = dtnext (nddict, ndp))
        IMPLLnds[ndi++] = ndp;

    IMPLLedn = dtsize (eddict);
    if (!(IMPLLeds = vmalloc (Vmheap, sizeof (IMPLLed_t *) * IMPLLedn))) {
        SUwarning (0, "IMPLLflatten", "caenot allocate IMPLLeds array");
        return -1;
    }
    for (edi = 0, edp = dtfirst (eddict); edp; edp = dtnext (eddict, edp))
        IMPLLeds[edi++] = edp;

    return 0;
}

int IMPLLbegindraw (
    char *fprefix, int findex,
    char *gmattr, char *tlattr, char *tlurl, char *tlobj
) {
    IMPLLnd_t *ndp;
    int ndi;
    UTattr_t *ap;
    int focusflag;
    char *gmbg, *gmcl, *gmfn, *gmfs, *gmfcl;
    int gmlw;
    char *tllab, *tlcl, *tlfn, *tlfs, *tlinfo;
    int tlshowtitle;
    Sfio_t *fp;

    if (IMPLLndn == 0)
        return 0;

    tlshowtitle = FALSE;

    if (UTsplitattr (UT_ATTRGROUP_MAIN, gmattr) == -1) {
        SUwarning (0, "IMPLLbegindraw", "cannot parse attrstr");
        return -1;
    }
    if (UTgetattrs (UT_ATTRGROUP_MAIN) == -1) {
        SUwarning (0, "IMPLLbegindraw", "cannot get invmap attr");
        return -1;
    }
    if (UTsplitattr (UT_ATTRGROUP_TITLE, tlattr) == -1) {
        SUwarning (0, "IMPLLbegindraw", "cannot parse attrstr");
        return -1;
    }
    if (UTgetattrs (UT_ATTRGROUP_TITLE) == -1) {
        SUwarning (0, "IMPLLbegindraw", "cannot get title attr");
        return -1;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_FLAGS];
    focusflag = (ap->value && strstr (ap->value, "focus")) ? TRUE : FALSE;
    ap = &UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_WIDTH];
    width = height = 0;
    if (ap->value && ap->value[0])
        width = atoi (ap->value);
    if (width <= 0)
        width = 1000;
    ap = &UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_HEIGHT];
    if (ap->value && ap->value[0])
        height = atoi (ap->value);
    if (height <= 0)
        height = 600;

    focusx1 = focusy1 = FLT_LONG_MAX;
    focusx2 = focusy2 = FLT_LONG_MIN;
    for (ndi = 0; ndi < IMPLLndn; ndi++) {
        ndp = IMPLLnds[ndi];
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
    }

    ap = &UTattrgroups[UT_ATTRGROUP_TITLE][UT_ATTR_LABEL];
    if (ap->value && ap->value[0]) {
        tllab = ap->value;
        ap = &UTattrgroups[UT_ATTRGROUP_TITLE][UT_ATTR_COLOR];
        tlcl = (ap->value) ? ap->value : "black";
        ap = &UTattrgroups[UT_ATTRGROUP_TITLE][UT_ATTR_FONTNAME];
        tlfn = (ap->value) ? ap->value : "Times-Roman";
        ap = &UTattrgroups[UT_ATTRGROUP_TITLE][UT_ATTR_FONTSIZE];
        tlfs = (ap->value) ? ap->value : "8";
        tlshowtitle = TRUE;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_TITLE][UT_ATTR_INFO];
    tlinfo = ap->value;

    ap = &UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_BGCOLOR];
    gmbg = (ap->value && ap->value[0]) ? ap->value : "white";
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

    sfprintf (
        sfstdout, "%s.%d.lllist\n%s.%d.html\n%s.%d.css\n",
        fprefix, findex, fprefix, findex, fprefix, findex
    );
    if (pagem > 0 && pagei++ % pagem == 0) {
        sfprintf (sfstdout, "%s.%d.toc\n", fprefix, findex);
        if (!(fp = sfopen (
            NULL, sfprints ("%s.%d.toc", fprefix, findex), "w"
        ))) {
            SUwarning (0, "IMPLLbegindraw", "cannot open toc file");
            return -1;
        }
        sfprintf (fp, "map %d", pagei);
        sfclose (fp);
    }

    if (!(fp = sfopen (
        NULL, sfprints ("%s.%d.css", fprefix, findex), "w"
    ))) {
        SUwarning (0, "IMPLLbegindraw", "cannot open css file");
        return -1;
    }
    sfprintf (fp, "<link rel='stylesheet' href='SWIFTLLCSS'/>\n");
    sfprintf (fp, "<script src='SWIFTLLJS'></script>\n");
    sfclose (fp);

    if (!(lfp = sfopen (
        NULL, sfprints ("%s.%d.lllist", fprefix, findex
    ), "w"))) {
        SUwarning (0, "IMPLLbegindraw", "cannot open altlist file");
        return -1;
    }

    if (!(hfp = sfopen (NULL, sfprints ("%s.%d.html", fprefix, findex), "w"))) {
        SUwarning (0, "IMPLLbegindraw", "cannot open html file");
        return -1;
    }

    if (tlshowtitle) {
        sfprintf (
            hfp,
            "<div class=page style='font-family:%s; font-size:%d; color:%s'>"
            "</div>\n"
        );
        if ((tlinfo && tlinfo[0]) || (tlurl && tlurl[0])) {
            if (tlinfo && tlinfo[0])
                sfprintf (
                    hfp, "<a class=page href='%s' title='%s'>%s</a>",
                    tlurl, tlinfo, tllab
                );
            else if (tlobj && tlobj[0])
                sfprintf (
                    hfp, "<a class=page href='%s' title='%s'>%s</a>",
                    tlurl, tlobj, tllab
                );
            else
                sfprintf (hfp, "<a class=page href='%s'>%s</a>", tlurl, tllab);
        } else {
            sfprintf (hfp, "%s", tllab);
        }
        sfprintf (hfp, "</div");
    }

    sfprintf (
        hfp,
        "<div class=page id=map%d style='width:%dpx; height:%dpx'></div>\n",
        findex, width, height
    );
    sfprintf (hfp, "<script>\n");
    sfprintf (hfp, "var map%d = L.map('map%d')", findex, findex);
    if (focusflag) {
        if (focusx2 - focusx1 > 0.00001 && focusy2 - focusy1 > 0.00001)
            sfprintf (
                hfp, ".fitBounds([[%f,%f],[%f,%f]]);\n",
                focusy1, focusx1, focusy2, focusx2
            );
        else
            sfprintf (
                hfp, ".setView([%f,%f], 12);\n",
                (focusy1 + focusy2) / 2.0, (focusx1 + focusx2) / 2.0
            );
    } else {
        sfprintf (
            hfp, ".setView([%f,%f], 4);\n",
            (focusy1 + focusy2) / 2.0, (focusx1 + focusx2) / 2.0
        );
    }
    sfprintf (
        hfp,
        "L.tileLayer('SWIFTLLTILESERVER', {"
        " maxZoom:18, attribution: 'SWIFTLLTILEATTR' "
        "}).addTo(map%d);\n",
        findex
    );

    return 0;
}

int IMPLLenddraw (void) {
    if (hfp) {
        sfprintf (hfp, "</script>\n");
        sfclose (hfp);
    }
    if (lfp)
        sfclose (lfp);

    return 0;
}

int IMPLLdrawnode (
    IMPLLnd_t *ndp, char *url, char *obj, char *fprefix, int findex, int iindex
) {
    RIarea_t a;
    int opi;

    if (!(rip = RIopen (RI_TYPE_IMAGE, fprefix, iindex, ndp->w, ndp->h))) {
        SUwarning (0, "IMPLLbegindraw", "cannot open image file #%d", iindex);
        return -1;
    }

    RIaddop (rip, NULL); // reset

    UTlockcolor = FALSE;
    if (UTaddlw (rip, ndp->lw) == -1) {
        SUwarning (0, "IMPLLdrawnode", "cannot add node line width");
        return -1;
    }
    if (UTaddbox (rip, 0, 0, ndp->w, ndp->h, ndp->bg, TRUE) == -1) {
        SUwarning (0, "IMPLLdrawnode", "cannot add node bgbox");
        return -1;
    }
    RIsetviewport (rip, TRUE, 0, 0, ndp->w, ndp->h);
    for (opi = 0; opi < ndp->opn; opi++) {
        if (RIaddop (rip, &EMops[ndp->opi + opi]) == -1) {
            SUwarning (0, "IMPLLdrawnode", "cannot add em op");
            return -1;
        }
    }
    RIsetviewport (rip, FALSE, 0, 0, ndp->w, ndp->h);
    UTlockcolor = TRUE;
    if (UTaddbox (rip, 0, 0, ndp->w, ndp->h, ndp->cl, FALSE) == -1) {
        SUwarning (0, "IMPLLdrawnode", "cannot add node box");
        return -1;
    }
    UTlockcolor = FALSE;

    if (url && url[0]) {
        a.mode = RI_AREA_AREA;
        a.type = RI_AREA_RECT;
        a.u.rect.x1 = 0;
        a.u.rect.y1 = 0;
        a.u.rect.x2 = ndp->w - 1;
        a.u.rect.y2 = ndp->h - 1;
        a.pass = 0;
        a.obj = obj;
        a.url = url;
        a.info = ndp->info;
        if (RIaddarea (rip, &a) == -1)
            SUwarning (0, "IMPLLdrawnode", "cannot add node draw area");
    }

    RIexecop (
        rip, NULL, 0, NULL, "black", "black", "black", "Times-Roman", "10"
    );

    if (RIclose (rip) == -1) {
        SUwarning (0, "IMPLLdrawnode", "cannot save image");
        return -1;
    }

    if (ndp->havecoord) {
        sfprintf (
            hfp,
            "L.popup({ closeButton:false, closeOnClick:false, minWidth:10 })"
            ".setLatLng([%f,%f]).setContent('SWLLP%s.%dPLLWS').addTo(map%d);\n",
            ndp->lly, ndp->llx, fprefix, iindex, findex
        );
        sfprintf (lfp, "%s.%d 1 %s\n", fprefix, iindex, url);
    } else {
        sfprintf (lfp, "%s.%d 0 %s\n", fprefix, iindex, url);
    }

    return 0;
}

int IMPLLdrawedge (IMPLLed_t *edp, char *url, char *obj, int findex) {
    if (!edp->ndp1->havecoord || !edp->ndp2->havecoord)
        return 0;

    sfprintf (
        hfp,
        "L.polyline([[%f,%f],[%f,%f]],{color:black}).addTo(map%d);\n",
        edp->ndp1->lly, edp->ndp1->llx, edp->ndp2->lly, edp->ndp2->llx, findex
    );

    return 0;
}
