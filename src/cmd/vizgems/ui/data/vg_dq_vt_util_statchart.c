#include <ast.h>
#include <ast_float.h>
#include <tok.h>
#include <swift.h>
#include "vg_dq_vt_util.h"
#include "vg_statmap.c"

#define MAXPIX 10 * 1024 * 1024

int SCHmetricexists, SCHchartn;

SCHmetric_t **SCHmetrics;
int SCHmetricn;

static int chartsperimage, width, height, showcap, showperc;

static SCHorder_t *orders;
static int ordern, orderm;

static SCHframe_t *frames;
static int framen;

static char *pagemode;
static int pageindex, pagei, pagem, pageemit;
static char pagebuf[1000];

static Dt_t *metricdict;
static Dtdisc_t metricdisc = {
    sizeof (Dtlink_t), SZ_level + SZ_id + SZ_skey,
    0, NULL, NULL, NULL, NULL, NULL, NULL
};

#define KEY_ASSET 0
#define KEY_C1    1
#define KEY_C2    2
#define KEY_CI    3
#define KEYSIZE   4
static int sortkeys[KEYSIZE], groupkeys[KEYSIZE];
static int sortkeyn, groupkeyn;

static char frameunit[1024];

static char **bgs;
static int bgn, bgm, bgi;

static RI_t *rip;

static int parseorder (char *, int *, int *);
static int metriccmp (const void *, const void *);
static int sortcmp (const void *, const void *);
static int groupcmp (SCHmetric_t *, SCHmetric_t *);

int SCHinit (int frn, int on, char *ofile, char *pm) {
    Sfio_t *fp;
    char *line, *s1, *s2, *s3;

    if (!(metricdict = dtopen (&metricdisc, Dtset))) {
        SUwarning (0, "SCHinit", "cannot create metricdict");
        return -1;
    }
    SCHmetrics = NULL;
    SCHmetricn = 0;

    pagemode = pm;
    pageindex = pagei = pagem = 0;
    if (pagemode && pagemode[0] && strcmp (pagemode, "all") != 0)
        pagem = atoi (pagemode);
    if (pagem < 0)
        pagem = 0;

    UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_BGCOLOR].name = "bgcolor";
    UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_WIDTH].name = "width";
    UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_HEIGHT].name = "height";

    UTattrgroups[UT_ATTRGROUP_TITLE][UT_ATTR_LABEL].name = "label";
    UTattrgroups[UT_ATTRGROUP_TITLE][UT_ATTR_COLOR].name = "color";
    UTattrgroups[UT_ATTRGROUP_TITLE][UT_ATTR_FONTNAME].name = "fontname";
    UTattrgroups[UT_ATTRGROUP_TITLE][UT_ATTR_FONTSIZE].name = "fontsize";
    UTattrgroups[UT_ATTRGROUP_TITLE][UT_ATTR_INFO].name = "info";

    UTattrgroups[UT_ATTRGROUP_METRIC][UT_ATTR_FLAGS].name = "flags";
    UTattrgroups[UT_ATTRGROUP_METRIC][UT_ATTR_LABEL].name = "label";
    UTattrgroups[UT_ATTRGROUP_METRIC][UT_ATTR_COLOR].name = "color";
    UTattrgroups[UT_ATTRGROUP_METRIC][UT_ATTR_CAPCOLOR].name = "capcolor";
    UTattrgroups[UT_ATTRGROUP_METRIC][UT_ATTR_LINEWIDTH].name = "linewidth";
    {
        UTattrgroups[UT_ATTRGROUP_METRIC][UT_ATTR_AVGLINEWIDTH].name =
        "avglinewidth";
    }
    UTattrgroups[UT_ATTRGROUP_METRIC][UT_ATTR_FONTNAME].name = "fontname";
    UTattrgroups[UT_ATTRGROUP_METRIC][UT_ATTR_FONTSIZE].name = "fontsize";

    UTattrgroups[UT_ATTRGROUP_XAXIS][UT_ATTR_FLAGS].name = "flags";
    UTattrgroups[UT_ATTRGROUP_XAXIS][UT_ATTR_LABEL].name = "label";
    UTattrgroups[UT_ATTRGROUP_XAXIS][UT_ATTR_COLOR].name = "color";
    UTattrgroups[UT_ATTRGROUP_XAXIS][UT_ATTR_FONTNAME].name = "fontname";
    UTattrgroups[UT_ATTRGROUP_XAXIS][UT_ATTR_FONTSIZE].name = "fontsize";

    UTattrgroups[UT_ATTRGROUP_YAXIS][UT_ATTR_FLAGS].name = "flags";
    UTattrgroups[UT_ATTRGROUP_YAXIS][UT_ATTR_COLOR].name = "color";
    UTattrgroups[UT_ATTRGROUP_YAXIS][UT_ATTR_FONTNAME].name = "fontname";
    UTattrgroups[UT_ATTRGROUP_YAXIS][UT_ATTR_FONTSIZE].name = "fontsize";

    framen = frn;

    orderm = 0;
    ordern = on;
    if (ordern > 0) {
        if (!(orders = vmalloc (Vmheap, sizeof (SCHorder_t) * ordern))) {
            SUwarning (0, "SCHinit", "cannot allocate orders");
            return -1;
        }
        memset (orders, 0, sizeof (SCHorder_t) * ordern);
        if (!(fp = sfopen (NULL, ofile, "r"))) {
            SUwarning (0, "SCHinit", "cannot open stat order file");
            return -1;
        }
        while ((line = sfgetr (fp, '\n', 1))) {
            if (!(s1 = strchr (line, '|'))) {
                SUwarning (0, "SCHinit", "bad line: %s", line);
                break;
            }
            *s1++ = 0;
            if (!(s2 = strchr (s1, '|'))) {
                SUwarning (0, "SCHinit", "bad line: %s", s1);
                break;
            }
            *s2++ = 0;
            if (!(s3 = strchr (s2, '|'))) {
                SUwarning (0, "SCHinit", "bad line: %s", s2);
                break;
            }
            *s3++ = 0;
            if (!(orders[orderm].str = vmstrdup (Vmheap, line))) {
                SUwarning (0, "SCHinit", "cannot copy name: %s", line);
                break;
            }
            orders[orderm].len = strlen (line);
            orders[orderm].c1 = atoi (s1);
            orders[orderm].c2 = atoi (s2);
            orders[orderm].ci = atoi (s3);
            orderm++;
        }
        sfclose (fp);
    }
    if (orderm != ordern)
        SUwarning (0, "SCHinit", "incomplete order list");

    return 0;
}

SCHmetric_t *SCHinsert (
    char *ilevel, char *iid, char *level, char *id, char *key,
    int framei, float val, char *unit, char *label
) {
    SCHmetric_t m, *mp, *mmem;
    int mti;
    SCHorder_t *op;
    int oi;
    char keystr[SZ_skey], *c1p, *c2p, *cip;

    memcpy (m.ilevel, ilevel, SZ_level);
    memcpy (m.iid, iid, SZ_id);
    memcpy (m.key, key, SZ_skey);

    SCHmetricexists = TRUE;
    if (!(mp = dtsearch (metricdict, &m))) {
        SCHmetricexists = FALSE;
        if (!(mmem = vmalloc (Vmheap, sizeof (SCHmetric_t)))) {
            SUwarning (0, "SCHinsert", "cannot allocate mmem");
            return NULL;
        }
        memset (mmem, 0, sizeof (SCHmetric_t));
        memcpy (mmem->ilevel, ilevel, SZ_level);
        memcpy (mmem->iid, iid, SZ_id);
        memcpy (mmem->level, level, SZ_level);
        memcpy (mmem->id, id, SZ_id);
        memcpy (mmem->key, key, SZ_skey);
        if ((mp = dtinsert (metricdict, mmem)) != mmem) {
            SUwarning (0, "SCHinsert", "cannot insert metric");
            vmfree (Vmheap, mmem);
            return NULL;
        }
        for (mti = 0; mti < SCH_MTYPE_SIZE; mti++) {
            if (!(mp->vals[mti] = vmalloc (
                Vmheap, framen * sizeof (float)
            ))) {
                SUwarning (0, "SCHinsert", "cannot allocate vals array");
                return NULL;
            }
            memset (mp->vals[mti], 0, framen * sizeof (float));
            if (!(mp->havevals[mti] = vmalloc (
                Vmheap, framen * sizeof (int)
            ))) {
                SUwarning (0, "SCHinsert", "cannot allocate havevals array");
                return NULL;
            }
            memset (mp->havevals[mti], 0, framen * sizeof (int));
            mp->minvals[mti] = FLT_MAX, mp->maxvals[mti] = FLT_MIN;
        }
        if (!(mp->marks = vmalloc (Vmheap, framen * sizeof (int)))) {
            SUwarning (0, "SCHinsert", "cannot allocate marks array");
            return NULL;
        }

        strcpy (keystr, key);
        for (oi = 0; oi < orderm; oi++)
            if (strncmp (keystr, orders[oi].str, orders[oi].len) == 0)
                break;
        op = (oi == orderm) ? NULL : &orders[oi];
        c1p = keystr;
        if ((cip = strchr (keystr, '.')))
            *cip++ = 0;
        if ((c2p = strchr (keystr, '_')))
            *c2p++ = 0;
        if (op) {
            mp->oc1 = op->c1;
            mp->oc2 = op->c2;
            mp->oci = op->ci;
        } else
            mp->oc1 = mp->oc2 = mp->oci = INT_MAX;
        if (c1p)
            strcpy (mp->ostr1, c1p);
        if (c2p)
            strcpy (mp->ostr2, c2p);
        if (cip)
            strcpy (mp->ostri, cip);
        mp->aggrmode = VG_STATMAP_AGGRMODE_AVG; // hack, attachdata will set
        mp->instinlabels = FALSE;
    }
    if (framei < 0 || framei >= framen) {
        SUwarning (
            0, "SCHinsert", "frame: %d out of range: %d-%d", framei, 0, framen
        );
        return NULL;
    }
    if (label[0] == '_') {
        if (strcmp (label, "___mean___") == 0)
            mti = SCH_MTYPE_MEAN;
        else if (strcmp (label, "___proj___") == 0)
            mti = SCH_MTYPE_PROJ;
    } else
        mti = SCH_MTYPE_CURR;
    switch (mp->aggrmode) {
    case VG_STATMAP_AGGRMODE_AVG:
        mp->vals[mti][framei] += val;
        mp->havevals[mti][framei]++;
        break;
    case VG_STATMAP_AGGRMODE_SUM:
        mp->vals[mti][framei] += val;
        mp->havevals[mti][framei] = 1;
        break;
    case VG_STATMAP_AGGRMODE_MIN:
        if (mp->havevals[mti][framei] == 0)
            mp->vals[mti][framei] = val;
        else if (mp->vals[mti][framei] > val)
            mp->vals[mti][framei] = val;
        mp->havevals[mti][framei] = 1;
        break;
    case VG_STATMAP_AGGRMODE_MAX:
        if (mp->havevals[mti][framei] == 0)
            mp->vals[mti][framei] = val;
        else if (mp->vals[mti][framei] < val)
            mp->vals[mti][framei] = val;
        mp->havevals[mti][framei] = 1;
        break;
    }
    mp->havemtypes[mti] = TRUE;
    if (unit[0]) {
        if (!mp->unit[0])
            memcpy (mp->unit, unit, SZ_sunit);
        else if (strcmp (unit, mp->unit) != 0) {
            if (!mp->units && !(mp->units = vmalloc (
                Vmheap, framen * sizeof (char *)
            ))) {
                SUwarning (0, "SCHinsert", "cannot allocate units array");
                return NULL;
            }
            memset (mp->units, 0, framen * sizeof (char *));
            if (!(mp->units[framei] = vmstrdup (Vmheap, unit))) {
                SUwarning (0, "SCHinsert", "cannot copy unit");
                return NULL;
            }
        }
    }
    if (mti == SCH_MTYPE_CURR && label[0]) {
        if (strchr (label, '('))
            mp->instinlabels = TRUE;
        if (!mp->label[0])
            memcpy (mp->label, label, SZ_slabel);
        else if (strcmp (label, mp->label) != 0) {
            if (!mp->labels) {
                if (!(mp->labels = vmalloc (
                    Vmheap, framen * sizeof (char *)
                ))) {
                    SUwarning (0, "SCHinsert", "cannot allocate labels array");
                    return NULL;
                }
                memset (mp->labels, 0, framen * sizeof (char *));
            }
            if (!(mp->labels[framei] = vmstrdup (Vmheap, label))) {
                SUwarning (0, "SCHinsert", "cannot copy label");
                return NULL;
            }
        }
    }
    val = mp->vals[mti][framei] / mp->havevals[mti][framei];
    if (mp->minvals[mti] > val)
        mp->minvals[mti] = val;
    if (mp->maxvals[mti] < val)
        mp->maxvals[mti] = val;
    return mp;
}

int SCHattachdata (
    SCHmetric_t *mp, char *deflabel, char *invlabel, int havecapv, float capv,
    int amode
) {
    if (deflabel)
        memcpy (mp->deflabel, deflabel, SZ_slabel);
    if (invlabel)
        memcpy (mp->invlabel, invlabel, SZ_val);
    mp->havecapv = havecapv, mp->capv = capv;
    mp->aggrmode = amode;
    return 0;
}

int SCHflatten (
    char *metricorder, char *sortorder, char *grouporder, char *displaymode
) {
    SCHmetric_t *mp;
    int mi, mj, mpan, maxmpan, toki;

    SCHchartn = 0;
    if ((SCHmetricn = dtsize (metricdict)) == 0)
        return 0;

    if (UTsplitstr (metricorder) == -1) {
        SUwarning (0, "SCHflatten", "cannot parse metric order");
        return -1;
    }
    maxmpan = (UTtokm > 1) ? atoi (UTtoks[0]) : -1;
    if (parseorder (sortorder, sortkeys, &sortkeyn) == -1) {
        SUwarning (0, "SCHflatten", "cannot parse sort keys");
        return -1;
    }
    if (parseorder (grouporder, groupkeys, &groupkeyn) == -1) {
        SUwarning (0, "SCHflatten", "cannot parse group keys");
        return -1;
    }
    showcap = showperc = FALSE;
    if (strcmp (displaymode, "actual") == 0)
        showcap = TRUE, showperc = FALSE;
    else if (strcmp (displaymode, "perc") == 0)
        showcap = TRUE, showperc = TRUE;

    if (!(SCHmetrics = vmalloc (Vmheap, sizeof (SCHmetric_t *) * SCHmetricn))) {
        SUwarning (0, "SCHflatten", "cannot allocate metrics array");
        return -1;
    }
    for (
        mi = 0, mp = dtfirst (metricdict); mp; mp = dtnext (metricdict, mp)
    ) {
        SCHmetrics[mi++] = mp;
        if (maxmpan == -1)
            continue;
        for (toki = 1; toki < UTtokm; toki++) {
            if (strstr (mp->key, UTtoks[toki]) == mp->key) {
                mp->metricorder = toki;
                break;
            }
        }
        if (toki == UTtokm)
            mp->metricorder = INT_MAX;
    }
    if (maxmpan > -1) {
        qsort (SCHmetrics, SCHmetricn, sizeof (SCHmetric_t *), metriccmp);
        mpan = 0;
        for (mi = mj = 0; mi < SCHmetricn; mi++) {
            if (
                mi == 0 || strcmp (
                    SCHmetrics[mi]->ilevel, SCHmetrics[mi - 1]->ilevel
                ) != 0 ||
                strcmp (SCHmetrics[mi]->iid, SCHmetrics[mi - 1]->iid) != 0
            )
                mpan = 0;
            if (mpan >= maxmpan)
                continue;
            SCHmetrics[mj++] = SCHmetrics[mi];
            mpan++;
        }
        SCHmetricn = mj;
    }
    qsort (SCHmetrics, SCHmetricn, sizeof (SCHmetric_t *), sortcmp);
    SCHchartn = 1;
    if (SCHmetricn > 0)
        SCHmetrics[0]->chartid = SCHchartn - 1;
    for (mi = 1; mi < SCHmetricn; mi++) {
        if (groupcmp (SCHmetrics[mi - 1], SCHmetrics[mi]) != 0)
            SCHchartn++;
        SCHmetrics[mi]->chartid = SCHchartn - 1;
    }

    return 0;
}

int SCHsetupdraw (
    char *rendermode, char *scattr, char *xaattr, char *yaattr
) {
    UTattr_t *ap;
    int framei;
    char *labelstr, *s1, *s2, *s3, *s4;
    int imagen;

    if (SCHchartn == 0)
        return 0;

    if (UTsplitattr (UT_ATTRGROUP_MAIN, scattr) == -1) {
        SUwarning (0, "SCHsetupdraw", "cannot parse scattr");
        return -1;
    }
    if (UTgetattrs (UT_ATTRGROUP_MAIN) == -1) {
        SUwarning (0, "SCHsetupdraw", "cannot get statchart attr");
        return -1;
    }
    if (UTsplitattr (UT_ATTRGROUP_XAXIS, xaattr) == -1) {
        SUwarning (0, "SCHsetupdraw", "cannot parse xaattr");
        return -1;
    }
    if (UTgetattrs (UT_ATTRGROUP_XAXIS) == -1) {
        SUwarning (0, "SCHsetupdraw", "cannot get xaxis attr");
        return -1;
    }
    if (UTsplitattr (UT_ATTRGROUP_YAXIS, yaattr) == -1) {
        SUwarning (0, "SCHsetupdraw", "cannot parse yaattr");
        return -1;
    }
    if (UTgetattrs (UT_ATTRGROUP_YAXIS) == -1) {
        SUwarning (0, "SCHsetupdraw", "cannot get yaxis attr");
        return -1;
    }

    if (!(frames = vmalloc (Vmheap, sizeof (SCHframe_t) * framen))) {
        SUwarning (0, "SCHsetupdraw", "cannot allocate frames");
        return -1;
    }
    memset (frames, 0, sizeof (SCHframe_t) * framen);
    ap = &UTattrgroups[UT_ATTRGROUP_XAXIS][UT_ATTR_LABEL];
    labelstr = (ap->value) ? ap->value : "frame|0:1:0";
    if (!(s1 = strchr (labelstr, '|')))
        s1 = labelstr;
    else {
        *s1++ = 0;
        sfsprintf (frameunit, 1024, "%s", labelstr);
    }
    for (; s1; s1 = s2) {
        if ((s2 = strchr (s1, '|')))
            *s2++ = 0;
        if (!(s3 = strchr (s1, ':')) || !(s4 = strchr (s3 + 1, ':'))) {
            SUwarning (0, "SCHsetupdraw", "bad frame label: %s", s1);
            break;
        }
        *s3++ = 0, *s4++ = 0;
        framei = atoi (s1);
        if (framei < 0 || framei >= framen)
            continue;
        frames[framei].level = atoi (s3);
        if (!(frames[framei].label = vmstrdup (Vmheap, s4))) {
            SUwarning (0, "SCHsetupdraw", "cannot copy label: %s", s4);
            break;
        }
    }

    ap = &UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_WIDTH];
    width = (ap->value && ap->value[0]) ? atoi (ap->value) : 400;
    ap = &UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_HEIGHT];
    height = (ap->value && ap->value[0]) ? atoi (ap->value) : 100;

    if (strcmp (rendermode, "embed") == 0) {
        if ((chartsperimage = MAXPIX / (width * height)) < 1)
            chartsperimage = 1;
        else if (chartsperimage > SCHchartn)
            chartsperimage = SCHchartn;
        imagen = (SCHchartn + chartsperimage - 1) / chartsperimage;
        chartsperimage = (SCHchartn + imagen - 1) / imagen;
    } else
        chartsperimage = 1;

    return 0;
}

int SCHdrawchart (
    char *fprefix, int findex, int ci, int cn,
    SCHmetric_t **ms, int mn, char *tlattr, char *mtattr,
    char *churl, char *tlurl, char *mturl, char *xaurl, char *yaurl, char *obj
) {
    int mi, mj, mk, ml, mti;
    float minvals[SCH_MTYPE_SIZE], maxvals[SCH_MTYPE_SIZE], chminval, chmaxval;
    int xoff, yoff, xsiz, ysiz;
    int tlxoff, tlyoff, tlxsiz, tlysiz;
    int mtxoff, mtyoff, mtxsiz, mtysiz;
    int xaxoff, xayoff, xaxsiz, xaysiz;
    int yaxoff, yayoff, yaxsiz, yaysiz;
    int chxoff, chyoff, chxsiz, chysiz;
    UTattr_t *ap;
    char *chbg;
    char *tllab, *tlcl, *tlfn, *tlfs, *tlinfo;
    int tlshowtitle;
    char *mtflags, *mtstr, *mtlab, *mtcapcl, *mtavgcl, *mtcl;
    char *mtfn, *mtfs, *mtbgstr;
    int mtrown, mtrowi, mtroww, mtrowh;
    int mtlabmode, mtshowcap, mtshowperc, mtshowquarts, mtshowlabel, mtdrawmode;
    int mtavgstyle, mthaveavg, mtavgx, mtavgy;
    float mtmaxcap;
    int mtlw, mtavglw;
    char *xaflags, *xacl, *xafn, *xafs;
    int xashowunits, xashowvals, xashowaxis;
    char *yaflags, *yacl, *yafn, *yafs, yainfo[256];
    char *yafmt, yaunitlab[1024], yamaxlab[1024], yaminlab[1024];
    int yashowunits, yashowvals, yashowaxis;
    SCHframe_t *fp;
    int level, framei, framej, framek, framel, lflag, sflag, gflag, toki, pflag;
    RIop_t top;
    RIarea_t a;
    void *tokp;
    char c, *s1, *s2;
    int x, y, x1, y1, x2, y2, w, h;

    if (mn < 1)
        return 0;

    if (ci % chartsperimage == 0) {
        if (!(rip = RIopen (
            RI_TYPE_IMAGE, fprefix, findex, width, chartsperimage * height
        ))) {
            SUwarning (0, "SCHdrawchart", "cannot open image file #%d", findex);
            return -1;
        }
    }

    pageemit = 0;
    if (pagem > 0 && pagei++ % pagem == 0) {
        pageemit = 1;
        sfsprintf (pagebuf, 1000, "image %d", pagei);
    }

    mtshowcap = showcap, mtshowperc = showperc;
    mtmaxcap = FLT_MIN;
    for (mi = 0; mi < mn; mi++) {
        if (!showcap || !ms[mi]->havecapv) {
            mtshowcap = mtshowperc = FALSE;
            break;
        }
        if (mtmaxcap < ms[mi]->capv)
            mtmaxcap = ms[mi]->capv;
    }
    if (mtmaxcap < 0.0)
        mtmaxcap = 1.0;

    chminval = FLT_MAX, chmaxval = FLT_MIN;
    for (mti = 0; mti < SCH_MTYPE_SIZE; mti++) {
        minvals[mti] = FLT_MAX, maxvals[mti] = FLT_MIN;
        for (mi = 0; mi < mn; mi++) {
            if (!ms[mi]->havemtypes[mti])
                continue;
            if (minvals[mti] > ms[mi]->minvals[mti])
                minvals[mti] = ms[mi]->minvals[mti];
            if (maxvals[mti] < ms[mi]->maxvals[mti])
                maxvals[mti] = ms[mi]->maxvals[mti];
            if (mtshowcap && ms[mi]->havecapv) {
                if (chminval > ms[mi]->capv)
                    chminval = ms[mi]->capv;
                if (chmaxval < ms[mi]->capv)
                    chmaxval = ms[mi]->capv;
            }
        }
        if (chminval > minvals[mti])
            chminval = minvals[mti];
        if (chmaxval < maxvals[mti])
            chmaxval = maxvals[mti];
    }
    if (chminval > 0.0)
        chminval = 0.0;
    if (chmaxval <= chminval)
        chmaxval = chminval + 1.0;

    xoff = 0, yoff = (ci % chartsperimage) * height;
    xsiz = width, ysiz = height;

    tlxoff = xoff, tlyoff = yoff, tlxsiz = xsiz, tlysiz = 0;
    mtxoff = xoff, mtyoff = tlyoff + tlysiz, mtxsiz = xsiz, mtysiz = 0;
    chxoff = xoff, chyoff = mtyoff + mtysiz, chxsiz = xsiz, chysiz = ysiz;
    xaxoff = xoff, xayoff = chyoff + chysiz, xaxsiz = xsiz, xaysiz = 0;
    yaxoff = xoff, yayoff = mtyoff + mtysiz, yaxsiz = 0, yaysiz = ysiz;

    tlshowtitle = FALSE;
    mtshowlabel = FALSE;
    xashowunits = xashowvals = xashowaxis = FALSE;
    yashowunits = yashowvals = yashowaxis = FALSE;

    RIaddop (rip, NULL); // reset

    if (UTsplitattr (UT_ATTRGROUP_TITLE, tlattr) == -1) {
        SUwarning (0, "SCHdrawchart", "cannot parse tlattr");
        return -1;
    }
    if (UTgetattrs (UT_ATTRGROUP_TITLE) == -1) {
        SUwarning (0, "SCHdrawchart", "cannot get title attr");
        return -1;
    }
    if (UTsplitattr (UT_ATTRGROUP_METRIC, mtattr) == -1) {
        SUwarning (0, "SCHdrawchart", "cannot parse mtattr");
        return -1;
    }
    if (UTgetattrs (UT_ATTRGROUP_METRIC) == -1) {
        SUwarning (0, "SCHdrawchart", "cannot get metric attr");
        return -1;
    }

    ap = &UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_BGCOLOR];
    if (ap->value) {
        chbg = ap->value;
        if (UTaddbox (
            rip, xoff, yoff, xoff + xsiz, yoff + ysiz, chbg, TRUE
        ) == -1) {
            SUwarning (0, "SCHdrawchart", "cannot add chbg box");
            return -1;
        }
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
        if (UTaddtext (
            rip, tllab, tlfn, tlfs, tlcl, tlxoff, tlyoff,
            tlxoff + tlxsiz, tlyoff + tlysiz, 0, -1, FALSE, &w, &h
        ) == -1) {
            SUwarning (0, "SCHdrawchart", "cannot add title text");
            return -1;
        }
        tlysiz += h;
        mtyoff += h, chyoff += h, chysiz -= h, yayoff += h, yaysiz -= h;
        tlshowtitle = TRUE;
        if (pageemit)
            sfsprintf (pagebuf, 1000, "%s", tllab);
    }
    ap = &UTattrgroups[UT_ATTRGROUP_TITLE][UT_ATTR_INFO];
    tlinfo = ap->value;

    ap = &UTattrgroups[UT_ATTRGROUP_METRIC][UT_ATTR_COLOR];
    if (ap->value && ap->value[0])
        tokp = tokopen (ap->value, 1), mtavgcl = tokread (tokp);
    else
        tokp = NULL, mtavgcl = NULL;
    mtavgstyle = RI_STYLE_NONE;
    if (mtavgcl) {
        if (mtavgcl[0] == '_') {
            if (strcmp (mtavgcl, "_dotted_") == 0)
                mtavgstyle = RI_STYLE_DOTTED;
            else if (strcmp (mtavgcl, "_dashed_") == 0)
                mtavgstyle = RI_STYLE_DASHED;
        }
        if (!(mtavgcl = vmstrdup (Vmheap, (mtavgcl) ? mtavgcl : "black"))) {
            SUwarning (0, "SCHdrawchart", "cannot copy metric bg color");
            return -1;
        }
        mtcl = tokread (tokp);
    } else
        mtcl = NULL;
    mthaveavg = FALSE;
    for (mi = 0; mi < mn; mi++) {
        if (!(ms[mi]->color = vmstrdup (Vmheap, (mtcl) ? mtcl : "black"))) {
            SUwarning (0, "SCHdrawchart", "cannot copy metric color");
            return -1;
        }
        if (
            ms[mi]->havemtypes[SCH_MTYPE_MEAN] ||
            ms[mi]->havemtypes[SCH_MTYPE_PROJ]
        )
            mthaveavg = TRUE;
        if (tokp && mtcl)
            mtcl = tokread (tokp);
    }
    if (tokp)
        tokclose (tokp);

    ap = &UTattrgroups[UT_ATTRGROUP_METRIC][UT_ATTR_LINEWIDTH];
    if (ap->value && ap->value[0])
        mtlw = atoi (ap->value);
    else
        mtlw = 1;
    ap = &UTattrgroups[UT_ATTRGROUP_METRIC][UT_ATTR_AVGLINEWIDTH];
    if (ap->value && ap->value[0])
        mtavglw = atoi (ap->value);
    else
        mtavglw = 1;

    if (mtshowcap) {
        ap = &UTattrgroups[UT_ATTRGROUP_METRIC][UT_ATTR_CAPCOLOR];
        mtcapcl = (ap->value) ? ap->value : "black";
    }

    mtlabmode = 1;
    mtshowquarts = FALSE;
    mtrown = 1;
    mtdrawmode = 1;
    ap = &UTattrgroups[UT_ATTRGROUP_METRIC][UT_ATTR_FLAGS];
    if (ap->value && ap->value[0]) {
        mtflags = ap->value;
        tokp = tokopen (mtflags, 1);
        while ((mtstr = tokread (tokp))) {
            if (strncmp (mtstr, "label_lines:", 12) == 0)
                mtrown = atoi (mtstr + 12);
            else if (strcmp (mtstr, "label_inchart") == 0)
                mtlabmode = 2;
            else if (strcmp (mtstr, "draw_lines") == 0)
                mtdrawmode = 1;
            else if (strcmp (mtstr, "draw_multilines") == 0)
                mtdrawmode = 2;
            else if (strcmp (mtstr, "draw_impulses") == 0)
                mtdrawmode = 3;
            else if (strncmp (mtstr, "quartiles:", 10) == 0) {
                mtshowquarts = TRUE;
                mtbgstr = mtstr + 10;
                if (UTsplitstr (mtbgstr) == -1) {
                    SUwarning (
                        0, "SCHdrawchart", "cannot parse metric bgcolor"
                    );
                    return -1;
                }
                if (UTtokn > bgn) {
                    if (!(bgs = vmresize (
                        Vmheap, bgs, UTtokn * sizeof (char *), VM_RSCOPY
                    ))) {
                        SUwarning (0, "SCHdrawchart", "cannot grow bgs array");
                        return -1;
                    }
                    bgn = UTtokn;
                }
                for (toki = bgm = 0; toki < UTtokm; toki++)
                    bgs[bgm++] = UTtoks[toki];
                bgi = 0;
            }
        }
        tokclose (tokp);
    }
    if (mtrown < 1)
        mtrown = 1;

    mtlab = NULL;
    ap = &UTattrgroups[UT_ATTRGROUP_METRIC][UT_ATTR_LABEL];
    if (mtlabmode == 1 && ap->value && ap->value[0]) {
        mtlab = ap->value;
        if (*mtlab == '\001')
            mtlab++;
        ap = &UTattrgroups[UT_ATTRGROUP_METRIC][UT_ATTR_FONTNAME];
        mtfn = (ap->value) ? ap->value : "Times-Roman";
        ap = &UTattrgroups[UT_ATTRGROUP_METRIC][UT_ATTR_FONTSIZE];
        mtfs = (ap->value) ? ap->value : "8";
        mtroww = mtrowh = 0;
        mtrowi = 0;
        mk = (mthaveavg && mtavgstyle == RI_STYLE_NONE) ? mn + 1 : mn;
        ml = mk;
        for (mi = mj = 0, s1 = mtlab; mi < mk && s1 && mtrowi < mtrown; mi++) {
            if (mi < mn)
                ms[mi]->lx = mtroww, ms[mi]->ly = 0;
            else
                mtavgx = mtroww, mtavgy = 0;
            if ((s2 = strchr (s1, '\001')))
                c = *s2, *s2 = 0;
            if (RIgettextsize (s1, mtfn, mtfs, &top) == -1) {
                SUwarning (0, "SCHdrawchart", "cannot get metric text size");
                return -1;
            }
            if (s2)
                *s2++ = c;
            s1 = (mi + 1 != mn) ? s2 : (
                ms[mi]->havemtypes[SCH_MTYPE_MEAN]
            ) ? "Avg" : "Proj";
            if (mtrowh < top.u.font.h)
                mtrowh = top.u.font.h;
            if ((
                mtroww > 0 && mtroww + top.u.font.w > mtxsiz
            ) || mi == mk - 1) {
                ml = (
                    mtroww > 0 && mtroww + top.u.font.w > mtxsiz
                ) ? mi : mk;
                if (mtroww > 0 && mtroww + top.u.font.w > mtxsiz)
                    top.u.font.w = 0;
                for (; mj < ml; mj++) {
                    if (mj < mn) {
                        ms[mj]->lx += mtxoff + (
                            mtxsiz - (mtroww + top.u.font.w)
                        ) / 2;
                        ms[mj]->ly += mtyoff + mtysiz;
                    } else {
                        mtavgx += mtxoff + (
                            mtxsiz - (mtroww + top.u.font.w)
                        ) / 2;
                        mtavgy += mtyoff + mtysiz;
                    }
                }
                mj = mi;
                mtysiz += mtrowh;
                chyoff += mtrowh, chysiz -= mtrowh;
                yayoff += mtrowh, yaysiz -= mtrowh;
                mtroww = mtrowh = 0;
                mtrowi++;
            }
            mtroww += top.u.font.w + 5;
        }
        chyoff++, chysiz--, yayoff++, yaysiz--;
        for (mj = 0, s1 = mtlab; mj < ml && s1; mj++) {
            if ((s2 = strchr (s1, '\001')))
                c = *s2, *s2 = 0;
            if (mj < mn) {
                if (UTaddtext (
                    rip, s1, mtfn, mtfs, ms[mj]->color,
                    ms[mj]->lx, ms[mj]->ly, ms[mj]->lx, ms[mj]->ly,
                    -1, -1, FALSE, &w, &h
                ) == -1) {
                    SUwarning (
                        0, "SCHdrawchart", "cannot draw metric text %d/%d",
                        mj, mn
                    );
                    return -1;
                }
            } else {
                if (UTaddtext (
                    rip, s1, mtfn, mtfs, mtavgcl,
                    mtavgx, mtavgy, mtavgx, mtavgy, -1, -1, FALSE, &w, &h
                ) == -1) {
                    SUwarning (
                        0, "SCHdrawchart", "cannot draw avg metric text"
                    );
                    return -1;
                }
            }
            if (pageemit == 1 && mj != mn)
                sfsprintf (pagebuf, 1000, "%s", s1), pageemit = 2;
            if (s2)
                *s2++ = ' ';
            s1 = (mj + 1 != mn) ? s2 : (
                ms[mj]->havemtypes[SCH_MTYPE_MEAN]
            ) ? "Avg" : "Proj";
        }
        mtshowlabel = TRUE;
    }

    if (pageemit)
        RIaddtoc (rip, ++pageindex, pagebuf);

    ap = &UTattrgroups[UT_ATTRGROUP_XAXIS][UT_ATTR_FLAGS];
    if (ap->value && ap->value[0]) {
        xaflags = ap->value;
        if (strstr (xaflags, "units"))
            xashowunits = TRUE;
        if (strstr (xaflags, "values"))
            xashowvals = TRUE;
        if (strstr (xaflags, "axis"))
            xashowaxis = TRUE;
        if (xashowunits || xashowvals || xashowaxis) {
            ap = &UTattrgroups[UT_ATTRGROUP_XAXIS][UT_ATTR_COLOR];
            xacl = (ap->value) ? ap->value : "black";
            ap = &UTattrgroups[UT_ATTRGROUP_XAXIS][UT_ATTR_FONTNAME];
            xafn = (ap->value) ? ap->value : "Times-Roman";
            ap = &UTattrgroups[UT_ATTRGROUP_XAXIS][UT_ATTR_FONTSIZE];
            xafs = (ap->value) ? ap->value : "8";
            if (xashowunits || xashowvals) {
                if (RIgettextsize ("0", xafn, xafs, &top) == -1) {
                    SUwarning (0, "SCHdrawchart", "cannot get xaxis text size");
                    return -1;
                }
                h = (
                    (xashowunits ? 1 : 0) + (xashowvals ? 1 : 0)
                ) * top.u.font.h + 2;
                xayoff -= h, xaysiz = h, chysiz -= h, yaysiz -= h;
            }
        }
    }

    ap = &UTattrgroups[UT_ATTRGROUP_YAXIS][UT_ATTR_FLAGS];
    if (ap->value && ap->value[0]) {
        yaflags = ap->value;
        if (strstr (yaflags, "units"))
            yashowunits = TRUE;
        if (strstr (yaflags, "values"))
            yashowvals = TRUE;
        if (strstr (yaflags, "axis"))
            yashowaxis = TRUE;
        if (yashowunits || yashowvals || yashowaxis) {
            ap = &UTattrgroups[UT_ATTRGROUP_YAXIS][UT_ATTR_COLOR];
            yacl = (ap->value) ? ap->value : "black";
            ap = &UTattrgroups[UT_ATTRGROUP_YAXIS][UT_ATTR_FONTNAME];
            yafn = (ap->value) ? ap->value : "Times-Roman";
            ap = &UTattrgroups[UT_ATTRGROUP_YAXIS][UT_ATTR_FONTSIZE];
            yafs = (ap->value) ? ap->value : "8";
            w = 0;
            if (yashowunits) {
                if (mtshowperc)
                    strcpy (yaunitlab, "%");
                else
                    strcpy (yaunitlab, ms[0]->unit);
                if (RIgettextsize (yaunitlab, yafn, yafs, &top) == -1) {
                    SUwarning (0, "SCHdrawchart", "cannot get yaxis text size");
                    return -1;
                }
                if (w < top.u.font.w + 1)
                    w = top.u.font.w + 1;
            }
            if (yashowvals) {
                if (mtshowperc) {
                    yafmt = "%.2f";
                    if (strcmp (ms[0]->unit, "%") == 0 && chmaxval > 100.0)
                        sfsprintf (yamaxlab, 1024, yafmt, chmaxval);
                    else
                        sfsprintf (yamaxlab, 1024, yafmt, 100.0);
                    sfsprintf (yaminlab, 1024, yafmt, 0.0);
                } else {
                    yafmt = (chmaxval < 10.0) ? "%.3f" : (
                        (chmaxval < 100.0) ? "%.2f" : "%.0f"
                    );
                    sfsprintf (yamaxlab, 1024, yafmt, chmaxval);
                    sfsprintf (yaminlab, 1024, yafmt, chminval);
                }
                if (RIgettextsize (yamaxlab, yafn, yafs, &top) == -1) {
                    SUwarning (0, "SCHdrawchart", "cannot get yaxis text size");
                    return -1;
                }
                if (w < top.u.font.w + 1)
                    w = top.u.font.w + 1;
                if (RIgettextsize (yaminlab, yafn, yafs, &top) == -1) {
                    SUwarning (0, "SCHdrawchart", "cannot get yaxis text size");
                    return -1;
                }
                if (w < top.u.font.w + 1)
                    w = top.u.font.w + 1;
            }
            yaxsiz = w, chxoff += w, chxsiz -= w, xaxoff += w, xaxsiz -= w;
        }
    }
    if (minvals[SCH_MTYPE_CURR] == FLT_MAX)
        sfsprintf (yainfo, 256, "no current data");
    else if (mtshowperc)
        sfsprintf (
            yainfo, 256, "range: %.2f - %.2f %s",
            100.0 * (minvals[SCH_MTYPE_CURR] / mtmaxcap),
            100.0 * (maxvals[SCH_MTYPE_CURR] / mtmaxcap), yaunitlab
        );
    else
        sfsprintf (
            yainfo, 256, "range: %.3f - %.3f %s",
            minvals[SCH_MTYPE_CURR], maxvals[SCH_MTYPE_CURR], yaunitlab
        );

    chxoff += 1, chxsiz -= 2, xaxoff += 1, xaxsiz -= 2;

    if (xashowaxis) {
        if (UTaddline (
            rip, xaxoff, xayoff - 1, xaxoff + xaxsiz, xayoff - 1, xacl
        ) == -1) {
            SUwarning (0, "SCHdrawchart", "cannot add xaxis line");
            return -1;
        }
    }
    if (xashowvals) {
        for (framei = 0; framei < framen; framei++) {
            frames[framei].mark = 0;
            if (framen == 1)
                frames[framei].x = xaxoff;
            else
                frames[framei].x = xaxoff + xaxsiz * (
                    (double) framei / (framen - 1)
                );
            if (!frames[framei].label)
                continue;
            if (RIgettextsize (
                frames[framei].label, xafn, xafs, &top
            ) == -1) {
                SUwarning (0, "SCHdrawchart", "cannot get xaxis text size");
                return -1;
            }
            frames[framei].lx = frames[framei].x - 1.1 * (top.u.font.w) / 2;
            frames[framei].rx = frames[framei].lx + 1.1 * top.u.font.w;
            if (frames[framei].lx < 0)
                frames[framei].label = NULL;
            if (frames[framei].rx > xaxoff + xaxsiz)
                frames[framei].label = NULL;
        }
        for (level = 1, lflag = TRUE, sflag = TRUE; lflag && sflag; level++) {
            lflag = FALSE, sflag = FALSE;
            for (framei = 0; framei < framen; framei++) {
                fp = &frames[framei];
                if (fp->mark == 0)
                    sflag = TRUE;
                if (!fp->label)
                    continue;
                if (fp->level == level + 1)
                    lflag = TRUE;
                if (fp->level != level)
                    continue;
                if (fp->mark > 0)
                    continue;
                gflag = TRUE;
                for (framek = framei - 1; framek >= 0; framek--) {
                    if (frames[framek].x < fp->lx)
                        break;
                    if (frames[framek].mark > 0) {
                        gflag = FALSE;
                        break;
                    }
                }
                for (framel = framei + 1; framel < framen; framel++) {
                    if (frames[framel].x > fp->rx)
                        break;
                    if (frames[framel].mark > 0) {
                        gflag = FALSE;
                        break;
                    }
                }
                if (!gflag)
                    continue;
                if (framek < 0)
                    framek = 0;
                if (framel > framen - 1)
                    framel = framen - 1;
                for (framej = framek; framej <= framel; framej++)
                    frames[framej].mark = 2;
                frames[framei].mark = 1;
            }
        }
        for (framei = 0; framei < framen; framei++) {
            fp = &frames[framei];
            if (!fp->label || fp->mark != 1)
                continue;
            if (UTaddtext (
                rip, fp->label, xafn, xafs, xacl,
                fp->lx, xayoff, fp->rx, xayoff, 0, -1, FALSE, &w, &h
            ) == -1) {
                SUwarning (0, "SCHdrawchart", "cannot add xaxis text");
                return -1;
            }
        }
    } else {
        for (framei = 0; framei < framen; framei++) {
            frames[framei].mark = 0;
            if (framen == 1)
                frames[framei].x = xaxoff;
            else
                frames[framei].x = xaxoff + xaxsiz * (
                    (double) framei / (framen - 1)
                );
        }
    }
    if (xashowunits) {
        if (UTaddtext (
            rip, frameunit, xafn, xafs, xacl,
            xaxoff, xayoff - 1, xaxoff + xaxsiz, xayoff + xaysiz - 1,
            0, 1, FALSE, &w, &h
        ) == -1) {
            SUwarning (0, "SCHdrawchart", "cannot add xaxis text");
            return -1;
        }
    }

    if (yashowaxis) {
        if (UTaddline (
            rip, yaxoff + yaxsiz, yayoff, yaxoff + yaxsiz, yayoff + yaysiz, yacl
        ) == -1) {
            SUwarning (0, "SCHdrawchart", "cannot add yaxis line");
            return -1;
        }
    }
    if (yashowvals) {
        if (UTaddtext (
            rip, yaminlab, yafn, yafs, yacl,
            yaxoff, yayoff + yaysiz, yaxoff + yaxsiz, yayoff + yaysiz,
            1, 1, FALSE, &w, &h
        ) == -1) {
            SUwarning (0, "SCHdrawchart", "cannot add yaxis text");
            return -1;
        }
        if (UTaddtext (
            rip, yamaxlab, yafn, yafs, yacl,
            yaxoff, yayoff, yaxoff + yaxsiz, yayoff,
            1, -1, FALSE, &w, &h
        ) == -1) {
            SUwarning (0, "SCHdrawchart", "cannot add yaxis text");
            return -1;
        }
    }
    if (yashowunits) {
        if (UTaddtext (
            rip, yaunitlab, yafn, yafs, yacl, yaxoff, yayoff,
            yaxoff + yaxsiz, yayoff + yaysiz, 1, 0, FALSE, &w, &h
        ) == -1) {
            SUwarning (0, "SCHdrawchart", "cannot add yaxis text");
            return -1;
        }
    }

    if (mtshowquarts) {
        if (UTaddbox (
            rip, chxoff, chyoff + 0.75 * chysiz,
            chxoff + chxsiz, chyoff + 1.0 * chysiz - 1, bgs[bgi++ % bgm], TRUE
        ) == -1 || UTaddbox (
            rip, chxoff, chyoff + 0.5 * chysiz,
            chxoff + chxsiz, chyoff + 0.75 * chysiz, bgs[bgi++ % bgm], TRUE
        ) == -1 || UTaddbox (
            rip, chxoff, chyoff + 0.25 * chysiz,
            chxoff + chxsiz, chyoff + 0.5 * chysiz, bgs[bgi++ % bgm], TRUE
        ) == -1 || UTaddbox (
            rip, chxoff, chyoff + 0.0 * chysiz + 1,
            chxoff + chxsiz, chyoff + 0.25 * chysiz, bgs[bgi++ % bgm], TRUE
        ) == -1) {
            SUwarning (0, "SCHdrawchart", "cannot add quartile boxe");
            return -1;
        }
    }

    ap = &UTattrgroups[UT_ATTRGROUP_METRIC][UT_ATTR_LABEL];
    if (mtlabmode == 2 && ap->value && ap->value[0]) {
        mtlab = ap->value;
        if (*mtlab == '\001')
            mtlab++;
        ap = &UTattrgroups[UT_ATTRGROUP_METRIC][UT_ATTR_FONTNAME];
        mtfn = (ap->value) ? ap->value : "Times-Roman";
        ap = &UTattrgroups[UT_ATTRGROUP_METRIC][UT_ATTR_FONTSIZE];
        mtfs = (ap->value) ? ap->value : "8";
        mtroww = mtrowh = 0;
        mtrowi = 0;
        mtxoff = chxoff, mtyoff = chyoff, mtxsiz = chxsiz, mtysiz = 0;
        mk = (mthaveavg && mtavgstyle == RI_STYLE_NONE) ? mn + 1 : mn;
        ml = mk;
        for (mi = mj = 0, s1 = mtlab; mi < mk && s1 && mtrowi < mtrown; mi++) {
            if (mi < mn)
                ms[mi]->lx = mtroww, ms[mi]->ly = 0;
            else
                mtavgx = mtroww, mtavgy = 0;
            if ((s2 = strchr (s1, '\001')))
                c = *s2, *s2 = 0;
            if (RIgettextsize (s1, mtfn, mtfs, &top) == -1) {
                SUwarning (0, "SCHdrawchart", "cannot get metric text size");
                return -1;
            }
            if (s2)
                *s2++ = c;
            s1 = (mi + 1 != mn) ? s2 : (
                ms[mi]->havemtypes[SCH_MTYPE_MEAN]
            ) ? "Avg" : "Proj";
            if (mtrowh < top.u.font.h)
                mtrowh = top.u.font.h;
            if ((
                mtroww > 0 && mtroww + top.u.font.w > mtxsiz
            ) || mi == mk - 1) {
                ml = (
                    mtroww > 0 && mtroww + top.u.font.w > mtxsiz
                ) ? mi : mk;
                if (mtroww > 0 && mtroww + top.u.font.w > mtxsiz)
                    top.u.font.w = 0;
                for (; mj < ml; mj++) {
                    if (mj < mn) {
                        ms[mj]->lx += mtxoff + (
                            mtxsiz - (mtroww + top.u.font.w)
                        ) / 2;
                        ms[mj]->ly += mtyoff + mtysiz;
                    } else {
                        mtavgx += mtxoff + (
                            mtxsiz - (mtroww + top.u.font.w)
                        ) / 2;
                        mtavgy += mtyoff + mtysiz;
                    }
                }
                mj = mi;
                mtysiz += mtrowh;
                mtroww = mtrowh = 0;
                mtrowi++;
            }
            mtroww += top.u.font.w + 5;
        }
        for (mj = 0, s1 = mtlab; mj < ml && s1; mj++) {
            if ((s2 = strchr (s1, '\001')))
                c = *s2, *s2 = 0;
            if (mj < mn) {
                if (UTaddtext (
                    rip, s1, mtfn, mtfs, ms[mj]->color,
                    ms[mj]->lx, ms[mj]->ly, ms[mj]->lx, ms[mj]->ly,
                    -1, -1, FALSE, &w, &h
                ) == -1) {
                    SUwarning (
                        0, "SCHdrawchart", "cannot draw metric text %d/%d",
                        mj, mn
                    );
                    return -1;
                }
            } else {
                if (UTaddtext (
                    rip, s1, mtfn, mtfs, mtavgcl,
                    mtavgx, mtavgy, mtavgx, mtavgy, -1, -1, FALSE, &w, &h
                ) == -1) {
                    SUwarning (
                        0, "SCHdrawchart", "cannot draw avg metric text"
                    );
                    return -1;
                }
            }
            if (pageemit == 1 && mj != mn)
                sfsprintf (pagebuf, 1000, "%s", s1), pageemit = 2;
            if (s2)
                *s2++ = ' ';
            s1 = (mj + 1 != mn) ? s2 : (
                ms[mj]->havemtypes[SCH_MTYPE_MEAN]
            ) ? "Avg" : "Proj";
        }
    }

    for (mi = 0; mi < mn; mi++) {
        if (mtshowcap && ms[mi]->havecapv) {
            y = chyoff + chysiz * (
                1.0 - (ms[mi]->capv - chminval) / (chmaxval - chminval)
            );
            if (UTaddline (rip, chxoff, y, chxoff + chxsiz, y, mtcapcl) == -1) {
                SUwarning (0, "SCHdrawchart", "cannot add capacity line");
                return -1;
            }
        }
        for (mti = 0; mti < SCH_MTYPE_SIZE; mti++) {
            if (!ms[mi]->havemtypes[mti])
                continue;
            if (UTaddlw (rip, (mti != SCH_MTYPE_CURR) ? mtavglw : mtlw) == -1) {
                SUwarning (0, "SCHdrawchart", "cannot add line width");
                return -1;
            }
            if (mtavgstyle != RI_STYLE_NONE)
                mtavgcl = ms[mi]->color;
            if (mtdrawmode == 1 || mtdrawmode == 2) {
                if (mtdrawmode == 2)
                    pflag = FALSE;
                if (UTaddpolybegin (
                    rip, (mti != SCH_MTYPE_CURR) ? mtavgcl : ms[mi]->color,
                    (mti != SCH_MTYPE_CURR) ? mtavgstyle : RI_STYLE_NONE
                ) == -1) {
                    SUwarning (0, "SCHdrawchart", "cannot add poly begin");
                    return -1;
                }
                for (framei = 0; framei < framen; framei++) {
                    if (!ms[mi]->havevals[mti][framei]) {
                        if (mtdrawmode == 2 && pflag) {
                            if (UTaddpolyend (rip) == -1) {
                                SUwarning (
                                    0, "SCHdrawchart", "cannot add poly end"
                                );
                                return -1;
                            }
                            if (UTaddpolybegin (
                                rip, (
                                    mti != SCH_MTYPE_CURR
                                ) ? mtavgcl : ms[mi]->color, (
                                    mti != SCH_MTYPE_CURR
                                ) ? mtavgstyle : RI_STYLE_NONE
                            ) == -1) {
                                SUwarning (
                                    0, "SCHdrawchart", "cannot add poly begin"
                                );
                                return -1;
                            }
                        }
                        pflag = FALSE;
                        continue;
                    }
                    x = frames[framei].x;
                    y = chyoff + chysiz * (1.0 - (
                        ms[mi]->vals[mti][framei] /
                        ms[mi]->havevals[mti][framei] -
                        chminval
                    ) / (chmaxval - chminval));
                    if (UTaddpolypoint (rip, x, y) == -1) {
                        SUwarning (0, "SCHdrawchart", "cannot add poly point");
                        return -1;
                    }
                    if (mtdrawmode == 2)
                        pflag = TRUE;
                }
                if (UTaddpolyend (rip) == -1) {
                    SUwarning (0, "SCHdrawchart", "cannot add poly end");
                    return -1;
                }
            } else if (mtdrawmode == 3) {
                for (framei = 0; framei < framen; framei++) {
                    if (!ms[mi]->havevals[mti][framei])
                        continue;
                    x1 = frames[framei].x + mi;
                    y1 = chyoff + chysiz;
                    x2 = frames[framei].x + mi;
                    y2 = chyoff + chysiz * (1.0 - (
                        ms[mi]->vals[mti][framei] /
                        ms[mi]->havevals[mti][framei] -
                        chminval
                    ) / (chmaxval - chminval));
                    if (UTaddline (
                        rip, x1, y1, x2, y2,
                        (mti != SCH_MTYPE_CURR) ? mtavgcl : ms[mi]->color
                    ) == -1) {
                        SUwarning (0, "SCHdrawchart", "cannot draw impulse");
                        return -1;
                    }
                }
            }
            if (UTaddlw (rip, 1) == -1) {
                SUwarning (0, "SCHdrawchart", "cannot add line width");
                return -1;
            }
        }
    }

    if (tlshowtitle && ((tlinfo && tlinfo[0]) || (tlurl && tlurl[0]))) {
        a.mode = RI_AREA_AREA;
        a.type = RI_AREA_RECT;
        a.u.rect.x1 = tlxoff;
        a.u.rect.y1 = tlyoff;
        a.u.rect.x2 = tlxoff + tlxsiz - 1;
        a.u.rect.y2 = tlyoff + tlysiz - 1;
        a.pass = 0;
        a.obj = obj;
        a.url = (tlurl && tlurl[0]) ? tlurl : "javascript:void(0)";
        a.info = (tlinfo && tlinfo[0]) ? tlinfo : NULL;
        if (RIaddarea (rip, &a) == -1)
            SUwarning (0, "SCHdrawchart", "cannot add area");
    }

    if (mtshowlabel && ((mtlab && mtlab[0]) || (mturl && mturl[0]))) {
        a.mode = RI_AREA_AREA;
        a.type = RI_AREA_RECT;
        a.u.rect.x1 = mtxoff;
        a.u.rect.y1 = mtyoff;
        a.u.rect.x2 = mtxoff + mtxsiz - 1;
        a.u.rect.y2 = mtyoff + mtysiz - 1;
        a.pass = 0;
        a.obj = obj;
        a.url = (mturl && mturl[0]) ? mturl : "javascript:void(0)";
        a.info = (mtlab && mtlab[0]) ? mtlab : NULL;
        if (RIaddarea (rip, &a) == -1)
            SUwarning (0, "SCHdrawchart", "cannot add area");
    }

    if ((xashowunits || xashowvals || xashowaxis)) {
        a.mode = RI_AREA_AREA;
        a.type = RI_AREA_RECT;
        a.u.rect.x1 = xaxoff;
        a.u.rect.y1 = xayoff;
        a.u.rect.x2 = xaxoff + xaxsiz - 1;
        a.u.rect.y2 = xayoff + xaysiz - 1;
        a.pass = 0;
        a.obj = obj;
        a.url = (xaurl && xaurl[0]) ? xaurl : "javascript:void(0)";
        a.info = frameunit;
        if (RIaddarea (rip, &a) == -1)
            SUwarning (0, "SCHdrawchart", "cannot add area");
    }

    if ((yashowunits || yashowvals || yashowaxis)) {
        a.mode = RI_AREA_AREA;
        a.type = RI_AREA_RECT;
        a.u.rect.x1 = yaxoff;
        a.u.rect.y1 = yayoff;
        a.u.rect.x2 = yaxoff + yaxsiz - 1;
        a.u.rect.y2 = yayoff + yaysiz - 1;
        a.pass = 0;
        a.obj = obj;
        a.url = (yaurl && yaurl[0]) ? yaurl : "javascript:void(0)";
        a.info = yainfo;
        if (RIaddarea (rip, &a) == -1)
            SUwarning (0, "SCHdrawchart", "cannot add area");
    }

    if ((mtlab && mtlab[0]) || (churl && churl[0])) {
        a.mode = (churl && churl[0]) ? RI_AREA_ALL : RI_AREA_EMBED;
        a.type = RI_AREA_RECT;
        a.u.rect.x1 = xoff;
        a.u.rect.y1 = yoff;
        a.u.rect.x2 = xoff + xsiz - 1;
        a.u.rect.y2 = yoff + ysiz - 1;
        a.pass = 0;
        a.obj = obj;
        a.url = (churl && churl[0]) ? churl : "javascript:void(0)";
        a.info = (mtlab && mtlab[0]) ? mtlab : NULL;
        if (RIaddarea (rip, &a) == -1)
            SUwarning (0, "SCHdrawchart", "cannot add area");
    }

    RIexecop (
        rip, NULL, 0, NULL, "black", "black", "black", "Times-Roman", "10"
    );

    if ((ci + 1) % chartsperimage == 0 || ci == cn - 1) {
        if (RIclose (rip) == -1) {
            SUwarning (0, "SCHdrawchart", "cannot save image");
            return -1;
        }
    }
    return 0;
}

static int parseorder (char *str, int *keyps, int *keypn) {
    char *s1, *s2, c;

    *keypn = 0;
    for (s1 = str; s1; s1 = s2) {
        if (*keypn >= KEYSIZE) {
            SUwarning (0, "parseorder", "too many keys");
            break;
        }
        if ((s2 = strchr (s1, ' ')))
            c = *s2, *s2 = 0;
        if (strcmp (s1, "asset") == 0)
            keyps[(*keypn)++] = KEY_ASSET;
        else if (strcmp (s1, "c1") == 0)
            keyps[(*keypn)++] = KEY_C1;
        else if (strcmp (s1, "c2") == 0)
            keyps[(*keypn)++] = KEY_C2;
        else if (strcmp (s1, "ci") == 0)
            keyps[(*keypn)++] = KEY_CI;
        else
            SUwarning (0, "parseorder", "unknown key: %s", s1);
        if (s2)
            *s2++ = c;
    }
    return 0;
}

static int metriccmp (const void *a, const void *b) {
    SCHmetric_t *ap, *bp;
    int ret;

    ap = *(SCHmetric_t **) a;
    bp = *(SCHmetric_t **) b;

    if ((ret = strcmp (ap->ilevel, bp->ilevel)) != 0)
        return ret;
    if ((ret = strcmp (ap->iid, bp->iid)) != 0)
        return ret;
    if ((ret = ap->metricorder - bp->metricorder) != 0)
        return ret;
    if ((ret = strcmp (ap->key, bp->key)) != 0)
        return ret;
    return 0;
}

static int sortcmp (const void *a, const void *b) {
    SCHmetric_t *ap, *bp;
    int ret, keyi;

    ap = *(SCHmetric_t **) a;
    bp = *(SCHmetric_t **) b;

    for (keyi = 0; keyi < sortkeyn; keyi++) {
        switch (sortkeys[keyi]) {
        case KEY_ASSET:
            if ((ret = strcmp (ap->ilevel, bp->ilevel)) != 0)
                return ret;
            if ((ret = strcmp (ap->iid, bp->iid)) != 0)
                return ret;
            break;
        case KEY_C1:
            if (ap->oc1 != bp->oc1)
                return (ap->oc1 - bp->oc1);
            break;
        case KEY_C2:
            if (ap->oc2 != bp->oc2)
                return (ap->oc2 - bp->oc2);
            break;
        case KEY_CI:
            if (ap->oci != bp->oci)
                return (ap->oci - bp->oci);
            break;
        }
    }
    for (keyi = 0; keyi < sortkeyn; keyi++) {
        switch (sortkeys[keyi]) {
        case KEY_C1:
            if ((ret = strcmp (ap->ostr1, bp->ostr1)) != 0)
                return ret;
            break;
        case KEY_C2:
            if ((ret = strcmp (ap->ostr2, bp->ostr2)) != 0)
                return ret;
            break;
        case KEY_CI:
            if ((ret = strcmp (ap->ostri, bp->ostri)) != 0)
                return ret;
            break;
        }
    }
    return 0;
}

static int groupcmp (SCHmetric_t *ap, SCHmetric_t *bp) {
    int ret, keyi;

    for (keyi = 0; keyi < groupkeyn; keyi++) {
        switch (groupkeys[keyi]) {
        case KEY_ASSET:
            if ((ret = strcmp (ap->ilevel, bp->ilevel)) != 0)
                return ret;
            if ((ret = strcmp (ap->iid, bp->iid)) != 0)
                return ret;
            break;
        case KEY_C1:
            if (ap->oc1 != bp->oc1)
                return (ap->oc1 - bp->oc1);
            break;
        case KEY_C2:
            if (ap->oc2 != bp->oc2)
                return (ap->oc2 - bp->oc2);
            break;
        case KEY_CI:
            if (ap->oci != bp->oci)
                return (ap->oci - bp->oci);
            break;
        }
    }
    for (keyi = 0; keyi < groupkeyn; keyi++) {
        switch (groupkeys[keyi]) {
        case KEY_C1:
            if ((ret = strcmp (ap->ostr1, bp->ostr1)) != 0)
                return ret;
            break;
        case KEY_C2:
            if ((ret = strcmp (ap->ostr2, bp->ostr2)) != 0)
                return ret;
            break;
        case KEY_CI:
            if ((ret = strcmp (ap->ostri, bp->ostri)) != 0)
                return ret;
            break;
        }
    }
    return 0;
}
