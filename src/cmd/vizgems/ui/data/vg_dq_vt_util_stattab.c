#include <ast.h>
#include <ast_float.h>
#include <tok.h>
#include <tm.h>
#include <swift.h>
#include "vg_dq_vt_util.h"

int STBmetricexists, STBtabn;

STBmetric_t **STBmetrics;
int STBmetricn;

static char *pagemode;
static int pageindex, pagei, pagem;

static STBorder_t *orders;
static int ordern, orderm;

static STBframe_t *frames;
static int framen;

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

static char **bgs, **cls;
static int bgn, bgm, bgi, cln, clm, cli;

static RI_t *rip;

static int parseorder (char *, int *, int *);
static int metriccmp (const void *, const void *);
static int sortcmp (const void *, const void *);
static int groupcmp (STBmetric_t *, STBmetric_t *);

int STBinit (int frn, int on, char *ofile, char *pm) {
    Sfio_t *fp;
    char *line, *s1, *s2, *s3;

    if (!(metricdict = dtopen (&metricdisc, Dtset))) {
        SUwarning (0, "STBinit", "cannot create metricdict");
        return -1;
    }
    STBmetrics = NULL;
    STBmetricn = 0;

    pagemode = pm;
    pageindex = pagei = pagem = 0;
    if (pagemode && pagemode[0] && strcmp (pagemode, "all") != 0)
        pagem = atoi (pagemode);
    if (pagem < 0)
        pagem = 0;

    UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_BGCOLOR].name = "bgcolor";
    UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_COLOR].name = "color";
    UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_FONTNAME].name = "fontname";
    UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_FONTSIZE].name = "fontsize";
    UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_TIMEFORMAT].name = "timeformat";
    UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_TIMELABEL].name = "timelabel";

    UTattrgroups[UT_ATTRGROUP_TITLE][UT_ATTR_LABEL].name = "label";

    UTattrgroups[UT_ATTRGROUP_METRIC][UT_ATTR_LABEL].name = "label";
    UTattrgroups[UT_ATTRGROUP_METRIC][UT_ATTR_BGCOLOR].name = "bgcolor";
    UTattrgroups[UT_ATTRGROUP_METRIC][UT_ATTR_COLOR].name = "color";
    UTattrgroups[UT_ATTRGROUP_METRIC][UT_ATTR_FONTNAME].name = "fontname";
    UTattrgroups[UT_ATTRGROUP_METRIC][UT_ATTR_FONTSIZE].name = "fontsize";

    framen = frn;
    if (!(frames = vmalloc (Vmheap, sizeof (STBframe_t) * framen))) {
        SUwarning (0, "STBinit", "cannot allocate frames");
        return -1;
    }
    memset (frames, 0, sizeof (STBframe_t) * framen);

    orderm = 0;
    ordern = on;
    if (ordern > 0) {
        if (!(orders = vmalloc (Vmheap, sizeof (STBorder_t) * ordern))) {
            SUwarning (0, "STBinit", "cannot allocate orders");
            return -1;
        }
        memset (orders, 0, sizeof (STBorder_t) * ordern);
        if (!(fp = sfopen (NULL, ofile, "r"))) {
            SUwarning (0, "STBinit", "cannot open stat order file");
            return -1;
        }
        while ((line = sfgetr (fp, '\n', 1))) {
            if (!(s1 = strchr (line, '|'))) {
                SUwarning (0, "STBinit", "bad line: %s", line);
                break;
            }
            *s1++ = 0;
            if (!(s2 = strchr (s1, '|'))) {
                SUwarning (0, "STBinit", "bad line: %s", s1);
                break;
            }
            *s2++ = 0;
            if (!(s3 = strchr (s2, '|'))) {
                SUwarning (0, "STBinit", "bad line: %s", s2);
                break;
            }
            *s3++ = 0;
            if (!(orders[orderm].str = vmstrdup (Vmheap, line))) {
                SUwarning (0, "STBinit", "cannot copy name: %s", line);
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
        SUwarning (0, "STBinit", "incomplete order list");

    return 0;
}

STBmetric_t *STBinsert (
    char *ilevel, char *iid, char *level, char *id, char *key,
    int framei, float val, char *unit, char *label, int timeissued
) {
    STBmetric_t m, *mp, *mmem;
    STBmetricval_t *mvp;
    int mti;
    STBorder_t *op;
    int oi;
    char keystr[SZ_skey], *c1p, *c2p, *cip;

    memcpy (m.ilevel, ilevel, SZ_level);
    memcpy (m.iid, iid, SZ_id);
    memcpy (m.key, key, SZ_skey);

    STBmetricexists = TRUE;
    if (!(mp = dtsearch (metricdict, &m))) {
        STBmetricexists = FALSE;
        if (!(mmem = vmalloc (Vmheap, sizeof (STBmetric_t)))) {
            SUwarning (0, "STBinsert", "cannot allocate mmem");
            return NULL;
        }
        memset (mmem, 0, sizeof (STBmetric_t));
        memcpy (mmem->ilevel, ilevel, SZ_level);
        memcpy (mmem->iid, iid, SZ_id);
        memcpy (mmem->level, level, SZ_level);
        memcpy (mmem->id, id, SZ_id);
        memcpy (mmem->key, key, SZ_skey);
        if ((mp = dtinsert (metricdict, mmem)) != mmem) {
            SUwarning (0, "STBinsert", "cannot insert metric");
            vmfree (Vmheap, mmem);
            return NULL;
        }
        for (mti = 0; mti < STB_MTYPE_SIZE; mti++) {
            if (!(mp->vals[mti] = vmalloc (
                Vmheap, framen * sizeof (float)
            ))) {
                SUwarning (0, "STBinsert", "cannot allocate vals array");
                return NULL;
            }
            memset (mp->vals[mti], 0, framen * sizeof (float));
            if (!(mp->havevals[mti] = vmalloc (
                Vmheap, framen * sizeof (int)
            ))) {
                SUwarning (0, "STBinsert", "cannot allocate havevals array");
                return NULL;
            }
            memset (mp->havevals[mti], 0, framen * sizeof (int));
        }
        if (!(mp->marks = vmalloc (Vmheap, framen * sizeof (int)))) {
            SUwarning (0, "STBinsert", "cannot allocate marks array");
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
        mp->instinlabels = FALSE;
    }
    if (framei < 0 || framei >= framen) {
        SUwarning (
            0, "STBinsert", "frame: %d out of range: %d-%d", framei, 0, framen
        );
        return NULL;
    }
    if (label[0] == '_') {
        if (strcmp (label, "___mean___") == 0)
            mti = STB_MTYPE_MEAN;
        else if (strcmp (label, "___proj___") == 0)
            mti = STB_MTYPE_PROJ;
    } else
        mti = STB_MTYPE_CURR;
    if (mp->havevals[mti][framei] >= 1) {
        if (mp->mvi + 1 >= mp->mvn) {
            if (!(mp->mvs = vmresize (
                Vmheap, mp->mvs, (mp->mvn + 10) * sizeof (STBmetricval_t),
                VM_RSCOPY
            ))) {
                SUwarning (0, "STBinsert", "cannot grow mvs array");
                return NULL;
            }
            mp->mvn += 10;
        }
        if (mp->havevals[mti][framei] == 1) {
            mvp = &mp->mvs[mp->mvi++];
            mvp->mti = mti, mvp->framei = framei;
            mvp->val = mp->vals[mti][framei];
        }
        mvp = &mp->mvs[mp->mvi++];
        mvp->mti = mti, mvp->framei = framei, mvp->val = val;
        mp->havemvs[mti]++;
    }
    mp->vals[mti][framei] += val;
    mp->havevals[mti][framei]++;
    mp->havemtypes[mti] = TRUE;
    if (unit[0]) {
        if (!mp->unit[0])
            memcpy (mp->unit, unit, SZ_sunit);
        else if (strcmp (unit, mp->unit) != 0) {
            if (!mp->units && !(mp->units = vmalloc (
                Vmheap, framen * sizeof (char *)
            ))) {
                SUwarning (0, "STBinsert", "cannot allocate units array");
                return NULL;
            }
            memset (mp->units, 0, framen * sizeof (char *));
            if (!(mp->units[framei] = vmstrdup (Vmheap, unit))) {
                SUwarning (0, "STBinsert", "cannot copy unit");
                return NULL;
            }
        }
    }
    if (mti == STB_MTYPE_CURR && label[0]) {
        if (strchr (label, '('))
            mp->instinlabels = TRUE;
        if (!mp->label[0])
            memcpy (mp->label, label, SZ_slabel);
        else if (strcmp (label, mp->label) != 0) {
            if (!mp->labels) {
                if (!(mp->labels = vmalloc (
                    Vmheap, framen * sizeof (char *)
                ))) {
                    SUwarning (0, "STBinsert", "cannot allocate labels array");
                    return NULL;
                }
                memset (mp->labels, 0, framen * sizeof (char *));
            }
            if (!(mp->labels[framei] = vmstrdup (Vmheap, label))) {
                SUwarning (0, "STBinsert", "cannot copy label");
                return NULL;
            }
        }
    }
    if (frames[framei].t == 0)
        frames[framei].t = timeissued;

    return mp;
}

int STBattachdata (
    STBmetric_t *mp, char *deflabel, char *invlabel, int havecapv, float capv
) {
    if (deflabel)
        memcpy (mp->deflabel, deflabel, SZ_slabel);
    if (invlabel)
        memcpy (mp->invlabel, invlabel, SZ_val);
    mp->havecapv = havecapv, mp->capv = capv;
    return 0;
}

int STBattachattr (
    STBmetric_t *mp, char *attr, char *url, char *obj
) {
    if (!(mp->attr = vmstrdup (Vmheap, attr))) {
        SUwarning (0, "STBattachattr", "cannot copy attributes");
        return -1;
    }
    if (!(mp->url = vmstrdup (Vmheap, url))) {
        SUwarning (0, "STBattachattr", "cannot copy url");
        return -1;
    }
    if (!(mp->obj = vmstrdup (Vmheap, obj))) {
        SUwarning (0, "STBattachattr", "cannot copy object");
        return -1;
    }
    return 0;
}

int STBflatten (char *metricorder, char *sortorder, char *grouporder) {
    STBmetric_t *mp;
    int mi, mj, mpan, maxmpan, toki;

    STBtabn = 0;
    if ((STBmetricn = dtsize (metricdict)) == 0)
        return 0;

    if (UTsplitstr (metricorder) == -1) {
        SUwarning (0, "STBflatten", "cannot parse metric order");
        return -1;
    }
    maxmpan = (UTtokm > 1) ? atoi (UTtoks[0]) : -1;
    if (parseorder (sortorder, sortkeys, &sortkeyn) == -1) {
        SUwarning (0, "STBflatten", "cannot parse sort keys");
        return -1;
    }
    if (parseorder (grouporder, groupkeys, &groupkeyn) == -1) {
        SUwarning (0, "STBflatten", "cannot parse group keys");
        return -1;
    }

    if (!(STBmetrics = vmalloc (Vmheap, sizeof (STBmetric_t *) * STBmetricn))) {
        SUwarning (0, "STBflatten", "cannot allocate metrics array");
        return -1;
    }
    for (
        mi = 0, mp = dtfirst (metricdict); mp; mp = dtnext (metricdict, mp)
    ) {
        STBmetrics[mi++] = mp;
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
        qsort (STBmetrics, STBmetricn, sizeof (STBmetric_t *), metriccmp);
        mpan = 0;
        for (mi = mj = 0; mi < STBmetricn; mi++) {
            if (
                mi == 0 || strcmp (
                    STBmetrics[mi]->ilevel, STBmetrics[mi - 1]->ilevel
                ) != 0 ||
                strcmp (STBmetrics[mi]->iid, STBmetrics[mi - 1]->iid) != 0
            )
                mpan = 0;
            if (mpan >= maxmpan)
                continue;
            STBmetrics[mj++] = STBmetrics[mi];
            mpan++;
        }
        STBmetricn = mj;
    }
    qsort (STBmetrics, STBmetricn, sizeof (STBmetric_t *), sortcmp);
    STBtabn = 1;
    if (STBmetricn > 0)
        STBmetrics[0]->tabid = STBtabn - 1;
    for (mi = 1; mi < STBmetricn; mi++) {
        if (groupcmp (STBmetrics[mi - 1], STBmetrics[mi]) != 0)
            STBtabn++;
        STBmetrics[mi]->tabid = STBtabn - 1;
    }

    return 0;
}

int STBsetupprint (char *rendermode, char *stattr) {
    UTattr_t *ap;
    int framei;
    char buf[1024], *tfmtstr;

    if (STBtabn == 0)
        return 0;

    if (UTsplitattr (UT_ATTRGROUP_MAIN, stattr) == -1) {
        SUwarning (0, "STBsetupprint", "cannot parse stattr");
        return -1;
    }
    if (UTgetattrs (UT_ATTRGROUP_MAIN) == -1) {
        SUwarning (0, "STBsetupprint", "cannot get stattab attr");
        return -1;
    }

    ap = &UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_TIMEFORMAT];
    tfmtstr = (ap->value) ? ap->value : "%Y/%m/%d-%H:%M";
    for (framei = 0; framei < framen; framei++) {
        if (frames[framei].t == 0)
            continue;
        tmfmt (buf, 1024, tfmtstr, &frames[framei].t);
        if (!(frames[framei].label = vmstrdup (Vmheap, buf))) {
            SUwarning (0, "STBsetupprint", "cannot copy label: %s", buf);
            return -1;
        }
    }

    return 0;
}

int STBprinttab (
    char *fprefix, int findex, int ci, int cn, STBmetric_t **ms, int mn,
    char *tlattr, char *tlurl
) {
    STBmetric_t *mp;
    int mi;
    STBmetricval_t *mvp;
    int mvi, mvm, mvl;
    UTattr_t *ap;
    char *stbg, *stcl, *stfn, *stfs;
    char *tllab;
    char *mtlab;
    char buf[1024], vbuf[4][1024], *bgstr, *clstr, *fnstr, *fsstr;
    int framei, toki;

    if (mn < 1)
        return 0;

    if (!(rip = RIopen (RI_TYPE_TABLE, fprefix, findex, 0, 0))) {
        SUwarning (0, "STBprinttab", "cannot open table file");
        return -1;
    }

    pageindex = pagei = 0;

    RIaddop (rip, NULL); // reset

    ap = &UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_BGCOLOR];
    stbg = (ap->value && ap->value[0]) ? ap->value : "white";
    ap = &UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_COLOR];
    stcl = (ap->value && ap->value[0]) ? ap->value : "black";
    ap = &UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_FONTNAME];
    stfn = (ap->value && ap->value[0]) ? ap->value : "Times-Roman";
    ap = &UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_FONTSIZE];
    stfs = (ap->value && ap->value[0]) ? ap->value : "8";

    if (UTaddtablebegin (rip, stfn, stfs, stbg, stcl) == -1) {
        SUwarning (0, "STBprinttab", "cannot add table begin");
        return -1;
    }

    if (UTsplitattr (UT_ATTRGROUP_TITLE, tlattr) == -1) {
        SUwarning (0, "STBprinttab", "cannot parse tlattr");
        return -1;
    }
    if (UTgetattrs (UT_ATTRGROUP_TITLE) == -1) {
        SUwarning (0, "STBprinttab", "cannot get title attr");
        return -1;
    }

    ap = &UTattrgroups[UT_ATTRGROUP_TITLE][UT_ATTR_LABEL];
    tllab = (ap->value && ap->value[0]) ? ap->value : "";
    if (UTaddtablecaption (
        rip, tllab, stfn, stfs, stbg, stcl, tlurl, NULL
    ) == -1) {
        SUwarning (0, "STBprinttab", "cannot add caption");
        return -1;
    }

    if (UTaddrowbegin (rip, stfn, stfs, stbg, stcl) == -1) {
        SUwarning (0, "STBprinttab", "cannot add row begin");
        return -1;
    }
    if (UTaddcellbegin (rip, stfn, stfs, stbg, stcl, 1, 1) == -1) {
        SUwarning (0, "STBprinttab", "cannot add time cell begin");
        return -1;
    }
    if (UTaddcelltext (rip, "Time") == -1) {
        SUwarning (0, "STBprinttab", "cannot add time cell");
        return -1;
    }
    if (UTaddcellend (rip) == -1) {
        SUwarning (0, "STBprinttab", "cannot add time cell end");
        return -1;
    }
    for (mi = 0; mi < mn; mi++) {
        mp = ms[mi];
        if (UTsplitattr (UT_ATTRGROUP_METRIC, mp->attr) == -1) {
            SUwarning (0, "STBprinttab", "cannot parse metric attr");
            return -1;
        }
        if (UTgetattrs (UT_ATTRGROUP_METRIC) == -1) {
            SUwarning (0, "STBprinttab", "cannot get metric attr");
            return -1;
        }
        ap = &UTattrgroups[UT_ATTRGROUP_METRIC][UT_ATTR_LABEL];
        mtlab = (ap->value && ap->value[0]) ? ap->value : "";
        cn = 2;
        if (mp->havecapv && mp->capv != 0.0 && strcmp (mp->unit, "%") != 0)
            cn++;
        if (mp->labels)
            cn++;
        if (mp->havemvs[STB_MTYPE_CURR] > 0)
            cn++;
        if (UTaddcellbegin (rip, stfn, stfs, stbg, stcl, 1, cn) == -1) {
            SUwarning (0, "STBprinttab", "cannot add header cell begin");
            return -1;
        }
        if (mp->url[0])
            if (UTaddlinkbegin (
                rip, mp->url, NULL, stfn, stfs, stbg, stcl
            ) == -1) {
                SUwarning (0, "STBprinttab", "cannot add data link begin");
                return -1;
            }

        if (UTaddcelltext (rip, mtlab) == -1) {
            SUwarning (0, "STBprinttab", "cannot add header cell");
            return -1;
        }
        if (mp->url[0])
            if (UTaddlinkend (rip) == -1) {
                SUwarning (0, "STBprinttab", "cannot add data link end");
                return -1;
            }
        if (UTaddcellend (rip) == -1) {
            SUwarning (0, "STBprinttab", "cannot add header cell end");
            return -1;
        }
    }
    if (UTaddrowend (rip) == -1) {
        SUwarning (0, "STBprinttab", "cannot add row end");
        return -1;
    }

    if (UTaddrowbegin (rip, stfn, stfs, stbg, stcl) == -1) {
        SUwarning (0, "STBprinttab", "cannot add row begin");
        return -1;
    }
    if (UTaddcellbegin (rip, stfn, stfs, stbg, stcl, 1, 1) == -1) {
        SUwarning (0, "STBprinttab", "cannot add time cell begin");
        return -1;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_TIMELABEL];
    if (UTaddcelltext (rip, (ap->value) ? ap->value : "") == -1) {
        SUwarning (0, "STBprinttab", "cannot add time cell");
        return -1;
    }
    if (UTaddcellend (rip) == -1) {
        SUwarning (0, "STBprinttab", "cannot add time cell end");
        return -1;
    }
    for (mi = 0; mi < mn; mi++) {
        mp = ms[mi];
        if (UTaddcellbegin (rip, stfn, stfs, stbg, stcl, 1, 1) == -1) {
            SUwarning (0, "STBprinttab", "cannot add header cell begin");
            return -1;
        }
        if (mp->unit[0])
            sfsprintf (buf, 1024, "Values (%s)", mp->unit);
        else
            sfsprintf (buf, 1024, "Values");
        if (UTaddcelltext (rip, buf) == -1) {
            SUwarning (0, "STBprinttab", "cannot add header cell");
            return -1;
        }
        if (UTaddcellend (rip) == -1) {
            SUwarning (0, "STBprinttab", "cannot add header cell end");
            return -1;
        }
        if (mp->havecapv && mp->capv != 0.0 && strcmp (mp->unit, "%") != 0) {
            if (UTaddcellbegin (rip, stfn, stfs, stbg, stcl, 1, 1) == -1) {
                SUwarning (0, "STBprinttab", "cannot add header cell begin");
                return -1;
            }
            if (UTaddcelltext (rip, "% Cap.") == -1) {
                SUwarning (0, "STBprinttab", "cannot add header cell");
                return -1;
            }
            if (UTaddcellend (rip) == -1) {
                SUwarning (0, "STBprinttab", "cannot add header cell end");
                return -1;
            }
        }
        if (UTaddcellbegin (rip, stfn, stfs, stbg, stcl, 1, 1) == -1) {
            SUwarning (0, "STBprinttab", "cannot add header cell begin");
            return -1;
        }
        if (UTaddcelltext (rip, (
            mp->havemtypes[STB_MTYPE_MEAN]
        ) ? "Avg" : "Proj") == -1) {
            SUwarning (0, "STBprinttab", "cannot add header cell");
            return -1;
        }
        if (UTaddcellend (rip) == -1) {
            SUwarning (0, "STBprinttab", "cannot add header cell end");
            return -1;
        }
        if (mp->labels) {
            if (UTaddcellbegin (rip, stfn, stfs, stbg, stcl, 1, 1) == -1) {
                SUwarning (0, "STBprinttab", "cannot add header cell begin");
                return -1;
            }
            if (UTaddcelltext (rip, "Labels") == -1) {
                SUwarning (0, "STBprinttab", "cannot add header cell");
                return -1;
            }
            if (UTaddcellend (rip) == -1) {
                SUwarning (0, "STBprinttab", "cannot add header cell end");
                return -1;
            }
        }
        if (mp->havemvs[STB_MTYPE_CURR]) {
            if (UTaddcellbegin (rip, stfn, stfs, stbg, stcl, 1, 1) == -1) {
                SUwarning (0, "STBprinttab", "cannot add header cell begin");
                return -1;
            }
            if (UTaddcelltext (rip, "Raw Data") == -1) {
                SUwarning (0, "STBprinttab", "cannot add header cell");
                return -1;
            }
            if (UTaddcellend (rip) == -1) {
                SUwarning (0, "STBprinttab", "cannot add header cell end");
                return -1;
            }
        }
    }
    if (UTaddrowend (rip) == -1) {
        SUwarning (0, "STBprinttab", "cannot add row end");
        return -1;
    }

    ap = &UTattrgroups[UT_ATTRGROUP_METRIC][UT_ATTR_BGCOLOR];
    bgstr = (ap->value && ap->value[0]) ? ap->value : "white";
    ap = &UTattrgroups[UT_ATTRGROUP_METRIC][UT_ATTR_COLOR];
    clstr = (ap->value && ap->value[0]) ? ap->value : "black";
    ap = &UTattrgroups[UT_ATTRGROUP_METRIC][UT_ATTR_FONTNAME];
    fnstr = (ap->value && ap->value[0]) ? ap->value : "Times-Roman";
    ap = &UTattrgroups[UT_ATTRGROUP_METRIC][UT_ATTR_FONTSIZE];
    fsstr = (ap->value && ap->value[0]) ? ap->value : "10";

    if (UTsplitstr (bgstr) == -1) {
        SUwarning (0, "STBprinttab", "cannot parse metric bgcolor");
        return -1;
    }
    if (UTtokn > bgn) {
        if (!(bgs = vmresize (
            Vmheap, bgs, UTtokn * sizeof (char *), VM_RSCOPY
        ))) {
            SUwarning (0, "STBprinttab", "cannot grow bgs array");
            return -1;
        }
        bgn = UTtokn;
    }
    for (toki = bgm = 0; toki < UTtokm; toki++)
        bgs[bgm++] = UTtoks[toki];
    bgi = 0;
    if (UTsplitstr (clstr) == -1) {
        SUwarning (0, "STBprinttab", "cannot parse metric color");
        return -1;
    }
    if (UTtokn > cln) {
        if (!(cls = vmresize (
            Vmheap, cls, UTtokn * sizeof (char *), VM_RSCOPY
        ))) {
            SUwarning (0, "STBprinttab", "cannot grow cls array");
            return -1;
        }
        cln = UTtokn;
    }
    for (toki = clm = 0; toki < UTtokm; toki++)
        cls[clm++] = UTtoks[toki];
    cli = 0;

    for (framei = 0; framei < framen; framei++) {
        if (!frames[framei].label)
            continue;

        if (pagem > 0 && pagei++ % pagem == 0)
            RIaddtoc (rip, ++pageindex, frames[framei].label);
        if (UTaddrowbegin (
            rip, fnstr, fsstr, bgs[bgi % bgm], cls[cli % clm]
        ) == -1) {
            SUwarning (0, "STBprinttab", "cannot add row begin");
            return -1;
        }
        if (UTaddcellbegin (
            rip, fnstr, fsstr, bgs[bgi % bgm], cls[cli % clm], 1, 1
        ) == -1) {
            SUwarning (0, "STBprinttab", "cannot add time cell begin");
            return -1;
        }
        if (UTaddcelltext (rip, frames[framei].label) == -1) {
            SUwarning (0, "STBprinttab", "cannot add time cell");
            return -1;
        }
        if (UTaddcellend (rip) == -1) {
            SUwarning (0, "STBprinttab", "cannot add time cell end");
            return -1;
        }
        for (mi = 0; mi < mn; mi++) {
            mp = ms[mi];
            if (UTaddcellbegin (
                rip, fnstr, fsstr, bgs[bgi % bgm], cls[cli % clm], 1, 1
            ) == -1) {
                SUwarning (0, "STBprinttab", "cannot add data cell begin");
                return -1;
            }
            if (mp->havevals[STB_MTYPE_CURR][framei])
                sfsprintf (
                    vbuf[0], 1024, "%.3f", (
                        mp->vals[STB_MTYPE_CURR][framei] /
                        mp->havevals[STB_MTYPE_CURR][framei]
                    )
                );
            else
                vbuf[0][0] = 0;
            if (UTaddcelltext (rip, vbuf[0]) == -1) {
                SUwarning (0, "STBprinttab", "cannot add data cell");
                return -1;
            }
            if (UTaddcellend (rip) == -1) {
                SUwarning (0, "STBprinttab", "cannot add data cell end");
                return -1;
            }
            if (
                mp->havecapv && mp->capv != 0.0 && strcmp (mp->unit, "%") != 0
            ) {
                if (UTaddcellbegin (
                    rip, fnstr, fsstr, bgs[bgi % bgm], cls[cli % clm], 1, 1
                ) == -1) {
                    SUwarning (0, "STBprinttab", "cannot add data cell begin");
                    return -1;
                }
                if (mp->havevals[STB_MTYPE_CURR][framei])
                    sfsprintf (
                        vbuf[1], 1024, "%.2f", 100.0 * (
                            mp->vals[STB_MTYPE_CURR][framei] /
                            mp->havevals[STB_MTYPE_CURR][framei]
                        ) / mp->capv
                    );
                else
                    vbuf[1][0] = 0;
                if (UTaddcelltext (rip, vbuf[1]) == -1) {
                    SUwarning (0, "STBprinttab", "cannot add data cell");
                    return -1;
                }
                if (UTaddcellend (rip) == -1) {
                    SUwarning (0, "STBprinttab", "cannot add data cell end");
                    return -1;
                }
            }
            if (UTaddcellbegin (
                rip, fnstr, fsstr, bgs[bgi % bgm], cls[cli % clm], 1, 1
            ) == -1) {
                SUwarning (0, "STBprinttab", "cannot add data cell begin");
                return -1;
            }
            if (mp->havevals[STB_MTYPE_MEAN][framei])
                sfsprintf (
                    vbuf[2], 1024, "%.3f", mp->vals[STB_MTYPE_MEAN][framei]
                );
            else if (mp->havevals[STB_MTYPE_PROJ][framei])
                sfsprintf (
                    vbuf[2], 1024, "%.3f", mp->vals[STB_MTYPE_PROJ][framei]
                );
            else
                vbuf[2][0] = 0;
            if (UTaddcelltext (rip, vbuf[2]) == -1) {
                SUwarning (0, "STBprinttab", "cannot add data cell");
                return -1;
            }
            if (UTaddcellend (rip) == -1) {
                SUwarning (0, "STBprinttab", "cannot add data cell end");
                return -1;
            }
            if (mp->labels) {
                if (UTaddcellbegin (
                    rip, fnstr, fsstr, bgs[bgi % bgm], cls[cli % clm], 1, 1
                ) == -1) {
                    SUwarning (0, "STBprinttab", "cannot add data cell begin");
                    return -1;
                }
                if (UTaddcelltext (
                    rip, mp->labels[framei] ? mp->labels[framei] : mp->label
                ) == -1) {
                    SUwarning (0, "STBprinttab", "cannot add data cell");
                    return -1;
                }
                if (UTaddcellend (rip) == -1) {
                    SUwarning (0, "STBprinttab", "cannot add data cell end");
                    return -1;
                }
            }
            if (mp->havemvs[STB_MTYPE_CURR] > 0) {
                if (UTaddcellbegin (
                    rip, fnstr, fsstr, bgs[bgi % bgm], cls[cli % clm], 1, 1
                ) == -1) {
                    SUwarning (0, "STBprinttab", "cannot add data cell begin");
                    return -1;
                }
                mvm = 0, mvl = 0;
                vbuf[3][0] = 0;
                for (mvi = 0; mvi < mp->mvi; mvi++) {
                    mvp = &mp->mvs[mvi];
                    if (mvp->mti != STB_MTYPE_CURR || mvp->framei != framei)
                        continue;
                    if (1024 - mvl > 0)
                        sfsprintf (
                            vbuf[3] + mvl, 1024 - mvl, "%s%.3f",
                            (mvm == 0) ? "" : ", ", mvp->val
                        );
                    mvl += sfslen ();
                    mvm++;
                }
                if (vbuf[3][0] == 0)
                    strcpy (vbuf[3], vbuf[0]);
                if (UTaddcelltext (rip, vbuf[3]) == -1) {
                    SUwarning (0, "STBprinttab", "cannot add data cell");
                    return -1;
                }
                if (UTaddcellend (rip) == -1) {
                    SUwarning (0, "STBprinttab", "cannot add data cell end");
                    return -1;
                }
            }
        }
        if (UTaddrowend (rip) == -1) {
            SUwarning (0, "STBprinttab", "cannot add row end");
            return -1;
        }
        bgi++, cli++;
    }

    if (pagem > 0)
        RIaddtoc (rip, 123456789, NULL);

    if (UTaddtableend (rip) == -1) {
        SUwarning (0, "STBprinttab", "cannot add table eed");
        return -1;
    }

    RIexecop (rip, NULL, 0, "black", "black", NULL, NULL, "Times-Roman", "10");

    if (RIclose (rip) == -1) {
        SUwarning (0, "STBprinttab", "cannot close table file");
        return -1;
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
    STBmetric_t *ap, *bp;
    int ret;

    ap = *(STBmetric_t **) a;
    bp = *(STBmetric_t **) b;

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
    STBmetric_t *ap, *bp;
    int ret, keyi;

    ap = *(STBmetric_t **) a;
    bp = *(STBmetric_t **) b;

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

static int groupcmp (STBmetric_t *ap, STBmetric_t *bp) {
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
