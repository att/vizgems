#include <ast.h>
#include <ast_float.h>
#include <tok.h>
#include <swift.h>
#include <ctype.h>
#include "vg_dq_vt_util.h"

ATBnd_t **ATBnds;
int ATBndn;
ATBed_t **ATBeds;
int ATBedn;

static char *pagemode;
static int pageindex, pagei, pagem;

static int aindex;

static Dt_t *nddict;
static Dtdisc_t nddisc = {
    sizeof (Dtlink_t), sizeof (int), 0, NULL, NULL, NULL, NULL, NULL, NULL
};
static Dt_t *eddict;
static Dtdisc_t eddisc = {
    sizeof (Dtlink_t), sizeof (int), 0, NULL, NULL, NULL, NULL, NULL, NULL
};

#define SORTAS_STR 1
#define SORTAS_INT 2
#define SORTAS_FLT 3
#define KEY_IDENT 1
#define KEY_CELL  2
#define DIR_FWD 1
#define DIR_REV 2
typedef struct sortkey_s {
    int key, ci, dir;
} sortkey_t;

static sortkey_t *sortkeys;
static int sortkeym, sortkeyn;
static char *sorturl;

#define GROUP_FIRST 1
#define GROUP_LAST  2
#define GROUP_COUNT 3
typedef struct groupkey_s {
    int key, ci;
} groupkey_t;

static groupkey_t *groupkeys;
static int groupkeym, groupkeyn, groupmode;
static char *groupkeybuf, *groupstrbuf;
static int groupkeylen, groupstrlen;

typedef struct ndgroup_s {
    Dtlink_t link;
    /* begin key */
    char *key;
    /* end key */
    int n;
    ATBnd_t *ndp;
} ndgroup_t;
typedef struct edgroup_s {
    Dtlink_t link;
    /* begin key */
    char *key;
    /* end key */
    int n;
    ATBed_t *edp;
} edgroup_t;

static Dt_t *ndgroupdict;
static Dtdisc_t ndgroupdisc = {
    sizeof (Dtlink_t), -1, 0, NULL, NULL, NULL, NULL, NULL, NULL
};
static Dt_t *edgroupdict;
static Dtdisc_t edgroupdisc = {
    sizeof (Dtlink_t), -1, 0, NULL, NULL, NULL, NULL, NULL, NULL
};

static ATBsevattr_t *sevattrs;
static int sevn, clearsevi;

static int tmodemap[VG_ALARM_N_MODE_MAX + 1] = {
    ATB_TMODE_NTICKET,
    ATB_TMODE_YTICKET,
    ATB_TMODE_DTICKET,
    ATB_TMODE_NTICKET,
};
static char *tmodenames[ATB_TMODE_SIZE] = {
    "ticketed", "deferred", "filtered"
};
static int tmodetrans[ATB_TMODE_SIZE] = {
    0, 1, 2
};

static RI_t *rip;

static int parsesorder (char *);
static int parsegorder (char *);
static int ndcmp (const void *, const void *);
static int edcmp (const void *, const void *);

int ATBinit (int sn, char *pm, char *go) {
    if (!(nddict = dtopen (&nddisc, Dtset))) {
        SUwarning (0, "ATBinit", "cannot create nddict");
        return -1;
    }
    if (!(eddict = dtopen (&eddisc, Dtset))) {
        SUwarning (0, "ATBinit", "cannot create eddict");
        return -1;
    }
    ATBnds = NULL;
    ATBndn = 0;
    ATBeds = NULL;
    ATBedn = 0;

    sevn = sn + 1, clearsevi = sn;
    pagemode = pm;
    pageindex = pagei = pagem = 0;
    if (pagemode && pagemode[0] && strcmp (pagemode, "all") != 0)
        pagem = atoi (pagemode);
    if (pagem < 0)
        pagem = 0;

    if (parsegorder (go) == -1) {
        SUwarning (0, "ATBinit", "cannot parse group keys");
        return -1;
    }
    if (!(ndgroupdict = dtopen (&ndgroupdisc, Dtset))) {
        SUwarning (0, "ATBinit", "cannot create ndgroupdict");
        return -1;
    }
    if (!(edgroupdict = dtopen (&edgroupdisc, Dtset))) {
        SUwarning (0, "ATBinit", "cannot create edgroupdict");
        return -1;
    }

    UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_BGCOLOR].name = "bgcolor";
    UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_COLOR].name = "color";
    UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_FONTNAME].name = "fontname";
    UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_FONTSIZE].name = "fontsize";
    UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_TMODENAMES].name = "tmodenames";
    UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_SEVNAMES].name = "sevnames";
    UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_SEVMAP].name = "sevmap";
    UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_FLAGS].name = "flags";

    UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_FONTNAME].name = "fontname";
    UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_FONTSIZE].name = "fontsize";
    UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_LABEL].name = "label";
    UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_HDR].name = "hdr";
    UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_ROW].name = "row";

    UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_FONTNAME].name = "fontname";
    UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_FONTSIZE].name = "fontsize";
    UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_LABEL].name = "label";
    UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_HDR].name = "hdr";
    UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_ROW].name = "row";

    return 0;
}

ATBnd_t *ATBinsertnd (
    char *level, char *id, short nclass, int type, int tmode, int sevi,
    char *attrstr, char *scl, char *url
) {
    ATBnd_t *ndp, *ndmem;
    UTattr_t *ap;
    groupkey_t *gkp;
    int gki;
    ndgroup_t *ndgmem, *ndgp;
    char *tokp, jc, sc;
    int toki, ci;
    char *p;
    int n, l;

    if (UTsplitattr (UT_ATTRGROUP_NODE, attrstr) == -1) {
        SUwarning (0, "ATBinsertnd", "cannot parse attrstr");
        return NULL;
    }
    if (UTgetattrs (UT_ATTRGROUP_NODE) == -1) {
        SUwarning (0, "ATBinsertnd", "cannot get node attr");
        return NULL;
    }

    if (groupkeym > 0) {
        ap = &UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_ROW];
        l = strlen (ap->value) + 1;
        if (l > groupstrlen) {
            if (!(groupstrbuf = vmresize (Vmheap, groupstrbuf, l, VM_RSMOVE))) {
                SUwarning (0, "ATBinsertnd", "cannot grow groupstrbuf");
                return NULL;
            }
            groupstrlen = l;
        }
        strcpy (groupstrbuf, ap->value);
        if (UTsplitstr (groupstrbuf) == -1) {
            SUwarning (0, "ATBinsertnd", "cannot parse node row for grouping");
            return NULL;
        }
        l = 2 * l + 1;
        if (l < SZ_level + SZ_id + 2)
            l = SZ_level + SZ_id + 2;
        if (l > groupkeylen) {
            if (!(groupkeybuf = vmresize (Vmheap, groupkeybuf, l, VM_RSMOVE))) {
                SUwarning (0, "ATBinsertnd", "cannot grow groupkeybuf");
                return NULL;
            }
            groupkeylen = l;
        }
        groupkeybuf[0] = 0;
        p = groupkeybuf, n = groupkeylen;
        for (gki = 0; gki < groupkeym; gki++) {
            gkp = &groupkeys[gki];
            switch (gkp->key) {
            case KEY_IDENT:
                strncat (p, "|", n);
                p += 1, n -= 1;
                strncat (p, level, n);
                l = strlen (level);
                p += l, n -= l;
                strncat (p, "|", n);
                p += 1, n -= 1;
                strncat (p, id, n);
                l = strlen (id);
                p += l, n -= l;
                break;
            case KEY_CELL:
                if (gkp->ci >= 0 && gkp->ci < UTtokm) {
                    tokp = UTtoks[gkp->ci];
                    if ((
                        tolower (*tokp) == 'l' || tolower (*tokp) == 'r' ||
                        tolower (*tokp) == 'c'
                    ) && (
                        tolower (*(tokp + 1)) == 'f' ||
                        tolower (*(tokp + 1)) == 'i' ||
                        tolower (*(tokp + 1)) == 's'
                    ) && *(tokp + 2) == ':')
                        tokp += 3;
                    strncat (p, "|", n);
                    p += 1, n -= 1;
                    strncat (p, tokp, n);
                    l = strlen (tokp);
                    p += l, n -= l;
                }
                break;
            }
        }
        if (!(ndgp = dtmatch (ndgroupdict, groupkeybuf))) {
            if (!(ndgmem = vmalloc (Vmheap, sizeof (ndgroup_t)))) {
                SUwarning (0, "ATBinsertnd", "cannot allocate ndgmem");
                return NULL;
            }
            memset (ndgmem, 0, sizeof (ndgroup_t));
            if (!(ndgmem->key = vmstrdup (Vmheap, groupkeybuf))) {
                SUwarning (0, "ATBinsertnd", "cannot copy ndgroup key");
                return NULL;
            }
            if ((ndgp = dtinsert (ndgroupdict, ndgmem)) != ndgmem) {
                SUwarning (0, "ATBinsertnd", "cannot insert ndgroup");
                return NULL;
            }
            ndgp->n = 1;
        }
        if (ndgp->ndp) {
            if (groupmode == GROUP_COUNT) {
                ndgp->n++;
                return ndgp->ndp;
            }
            if (groupmode == GROUP_FIRST)
                return ndgp->ndp;
            if (groupmode == GROUP_LAST) {
                if (!(ndp = dtmatch (nddict, &ndgp->ndp->index))) {
                    SUwarning (
                        0, "ATBinsertnd", "cannot find nd %d", ndgp->ndp->index
                    );
                    return NULL;
                }
                if (ndp->scl)
                    vmfree (Vmheap, ndp->scl);
                if (ndp->fn)
                    vmfree (Vmheap, ndp->fn);
                if (ndp->fs)
                    vmfree (Vmheap, ndp->fs);
                if (ndp->lab)
                    vmfree (Vmheap, ndp->lab);
                if (ndp->hdr)
                    vmfree (Vmheap, ndp->hdr);
                if (ndp->row)
                    vmfree (Vmheap, ndp->row);
                if (ndp->url)
                    vmfree (Vmheap, ndp->url);
                for (ci = 0; ci < ndp->celln; ci++)
                    vmfree (Vmheap, ndp->cells[ci].str);
                if (ndp->cells)
                    vmfree (Vmheap, ndp->cells);
                dtdelete (nddict, ndp);
                vmfree (Vmheap, ndp);
                ndgp->ndp = NULL;
            }
        }
    }

    if (!(ndmem = vmalloc (Vmheap, sizeof (ATBnd_t)))) {
        SUwarning (0, "ATBinsertnd", "cannot allocate ndmem");
        return NULL;
    }
    memset (ndmem, 0, sizeof (ATBnd_t));
    ndmem->index = aindex++;
    if ((ndp = dtinsert (nddict, ndmem)) != ndmem) {
        SUwarning (0, "ATBinsertnd", "cannot insert nd");
        vmfree (Vmheap, ndmem);
        return NULL;
    }
    memcpy (ndp->ilevel, level, SZ_level);
    memcpy (ndp->iid, id, SZ_id);
    ndp->nclass = nclass;
    if (sevi < 0 || sevi >= sevn) {
        SUwarning (
            0, "ATBinsertnd", "sevi: %d out of range: %d-%d", sevi, 0, sevn
        );
        return NULL;
    }
    if (tmode < 0 || tmode > VG_ALARM_N_MODE_MAX) {
        SUwarning (
            0, "ATBinsertnd", "tmode: %d out of range: %d-%d",
            tmode, 0, VG_ALARM_N_MODE_MAX
        );
        return NULL;
    }
    ndp->tmi = tmodemap[tmode];
    if (type == VG_ALARM_N_TYPE_CLEAR)
        ndp->sevi = clearsevi;
    else
        ndp->sevi = sevi;

    if (scl && scl[0] && !(ndp->scl = vmstrdup (Vmheap, scl))) {
        SUwarning (0, "ATBinsertnd", "cannot copy node special color");
        return NULL;
    }

    ap = &UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_FONTNAME];
    if (!(ndp->fn = vmstrdup (
        Vmheap, (ap->value && ap->value[0]) ? ap->value : "abc"
    ))) {
        SUwarning (0, "ATBinsertnd", "cannot copy node font name");
        return NULL;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_FONTSIZE];
    if (!(ndp->fs = vmstrdup (
        Vmheap, (ap->value && ap->value[0]) ? ap->value : "10"
    ))) {
        SUwarning (0, "ATBinsertnd", "cannot copy node font size");
        return NULL;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_LABEL];
    if (!(ndp->lab = vmstrdup (
        Vmheap, (ap->value && ap->value[0]) ? ap->value : ""
    ))) {
        SUwarning (0, "ATBinsertnd", "cannot copy node lab");
        return NULL;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_HDR];
    if (!(ndp->hdr = vmstrdup (
        Vmheap, (ap->value && ap->value[0]) ? ap->value : ""
    ))) {
        SUwarning (0, "ATBinsertnd", "cannot copy node hdr");
        return NULL;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_ROW];
    if (!(ndp->row = vmstrdup (
        Vmheap, (ap->value && ap->value[0]) ? ap->value : ""
    ))) {
        SUwarning (0, "ATBinsertnd", "cannot copy node row");
        return NULL;
    }
    if (UTsplitstr (ap->value) == -1) {
        SUwarning (0, "ATBinsertnd", "cannot parse node row");
        return NULL;
    }
    if (!(ndp->cells = vmalloc (Vmheap, UTtokm * sizeof (ATBcell_t)))) {
        SUwarning (0, "ATBinsertnd", "cannot allocate cells array");
        return NULL;
    }
    ndp->celln = UTtokm;
    for (toki = 0; toki < UTtokm; toki++) {
        tokp = UTtoks[toki];
        if ((
            tolower (*tokp) == 'l' || tolower (*tokp) == 'r' ||
            tolower (*tokp) == 'c'
        ) && (
            tolower (*(tokp + 1)) == 'f' || tolower (*(tokp + 1)) == 'i' ||
            tolower (*(tokp + 1)) == 's'
        ) && *(tokp + 2) == ':') {
            jc = tolower (*tokp), sc = tolower (*(tokp + 1)), tokp += 3;
        } else {
            jc = 'l', sc = 's';
        }
        if (!(ndp->cells[toki].str = vmstrdup (Vmheap, tokp))) {
            SUwarning (0, "ATBinsertnd", "cannot copy cell string");
            return NULL;
        }
        switch (jc) {
        case 'l': ndp->cells[toki].j = -1; break;
        case 'c': ndp->cells[toki].j = 0;  break;
        case 'r': ndp->cells[toki].j = 1;  break;
        }
        switch (sc) {
        case 's':
            ndp->cells[toki].sa = SORTAS_STR;
            break;
        case 'i':
            ndp->cells[toki].sa = SORTAS_INT;
            ndp->cells[toki].i = atoi (tokp);
            break;
        case 'f':
            ndp->cells[toki].sa = SORTAS_FLT;
            ndp->cells[toki].f = atof (tokp);
            break;
        }
    }
    if (!(ndp->url = vmstrdup (Vmheap, url))) {
        SUwarning (0, "ATBinsertnd", "cannot copy node url");
        return NULL;
    }

    if (groupkeym > 0)
        ndgp->ndp = ndp;

    return ndp;
}

ATBed_t *ATBinserted (
    char *level1, char *id1, char *level2, char *id2,
    short nclass1, short nclass2, int type, int tmode, int sevi, char *attrstr,
    char *scl, char *url
) {
    ATBed_t *edp, *edmem;
    UTattr_t *ap;
    groupkey_t *gkp;
    int gki;
    edgroup_t *edgmem, *edgp;
    char *tokp, jc, sc;
    int toki, ci;
    char *p;
    int n, l;

    if (UTsplitattr (UT_ATTRGROUP_EDGE, attrstr) == -1) {
        SUwarning (0, "ATBinserted", "cannot parse attrstr");
        return NULL;
    }
    if (UTgetattrs (UT_ATTRGROUP_EDGE) == -1) {
        SUwarning (0, "ATBinserted", "cannot get node attr");
        return NULL;
    }

    if (groupkeym > 0) {
        ap = &UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_ROW];
        l = strlen (ap->value) + 1;
        if (l > groupstrlen) {
            if (!(groupstrbuf = vmresize (Vmheap, groupstrbuf, l, VM_RSMOVE))) {
                SUwarning (0, "ATBinserted", "cannot grow groupstrbuf");
                return NULL;
            }
            groupstrlen = l;
        }
        strcpy (groupstrbuf, ap->value);
        if (UTsplitstr (groupstrbuf) == -1) {
            SUwarning (0, "ATBinserted", "cannot parse node row for grouping");
            return NULL;
        }
        l = 2 * l + 1;
        if (l < 2 * (SZ_level + SZ_id + 2))
            l = 2 * (SZ_level + SZ_id + 2);
        if (l > groupkeylen) {
            if (!(groupkeybuf = vmresize (Vmheap, groupkeybuf, l, VM_RSMOVE))) {
                SUwarning (0, "ATBinserted", "cannot grow groupkeybuf");
                return NULL;
            }
            groupkeylen = l;
        }
        groupkeybuf[0] = 0;
        p = groupkeybuf, n = groupkeylen;
        for (gki = 0; gki < groupkeym; gki++) {
            gkp = &groupkeys[gki];
            switch (gkp->key) {
            case KEY_IDENT:
                strncat (p, "|", n);
                p += 1, n -= 1;
                strncat (p, level1, n);
                l = strlen (level1);
                p += l, n -= l;
                strncat (p, "|", n);
                p += 1, n -= 1;
                strncat (p, id1, n);
                l = strlen (id1);
                p += l, n -= l;
                strncat (p, "|", n);
                p += 1, n -= 1;
                strncat (p, level2, n);
                l = strlen (level2);
                p += l, n -= l;
                strncat (p, "|", n);
                p += 1, n -= 1;
                strncat (p, id2, n);
                l = strlen (id2);
                p += l, n -= l;
                break;
            case KEY_CELL:
                if (gkp->ci >= 0 && gkp->ci < UTtokm) {
                    tokp = UTtoks[gkp->ci];
                    if ((
                        tolower (*tokp) == 'l' || tolower (*tokp) == 'r' ||
                        tolower (*tokp) == 'c'
                    ) && (
                        tolower (*(tokp + 1)) == 'f' ||
                        tolower (*(tokp + 1)) == 'i' ||
                        tolower (*(tokp + 1)) == 's'
                    ) && *(tokp + 2) == ':')
                        tokp += 3;
                    strncat (p, "|", n);
                    p += 1, n -= 1;
                    strncat (p, tokp, n);
                    l = strlen (tokp);
                    p += l, n -= l;
                }
                break;
            }
        }
        if (!(edgp = dtmatch (edgroupdict, groupkeybuf))) {
            if (!(edgmem = vmalloc (Vmheap, sizeof (edgroup_t)))) {
                SUwarning (0, "ATBinserted", "cannot allocate edgmem");
                return NULL;
            }
            memset (edgmem, 0, sizeof (edgroup_t));
            if (!(edgmem->key = vmstrdup (Vmheap, groupkeybuf))) {
                SUwarning (0, "ATBinserted", "cannot copy edgroup key");
                return NULL;
            }
            if ((edgp = dtinsert (edgroupdict, edgmem)) != edgmem) {
                SUwarning (0, "ATBinserted", "cannot insert edgroup");
                return NULL;
            }
            edgp->n = 1;
        }
        if (edgp->edp) {
            if (groupmode == GROUP_COUNT) {
                edgp->n++;
                return edgp->edp;
            }
            if (groupmode == GROUP_FIRST)
                return edgp->edp;
            if (groupmode == GROUP_LAST) {
                if (!(edp = dtmatch (eddict, &edgp->edp->index))) {
                    SUwarning (
                        0, "ATBinserted", "cannot find ed %d", edgp->edp->index
                    );
                    return NULL;
                }
                if (edp->scl)
                    vmfree (Vmheap, edp->scl);
                if (edp->fn)
                    vmfree (Vmheap, edp->fn);
                if (edp->fs)
                    vmfree (Vmheap, edp->fs);
                if (edp->lab)
                    vmfree (Vmheap, edp->lab);
                if (edp->hdr)
                    vmfree (Vmheap, edp->hdr);
                if (edp->row)
                    vmfree (Vmheap, edp->row);
                if (edp->url)
                    vmfree (Vmheap, edp->url);
                for (ci = 0; ci < edp->celln; ci++)
                    vmfree (Vmheap, edp->cells[ci].str);
                if (edp->cells)
                    vmfree (Vmheap, edp->cells);
                dtdelete (eddict, edp);
                vmfree (Vmheap, edp);
                edgp->edp = NULL;
            }
        }
    }

    if (!(edmem = vmalloc (Vmheap, sizeof (ATBed_t)))) {
        SUwarning (0, "ATBinserted", "cannot allocate edmem");
        return NULL;
    }
    memset (edmem, 0, sizeof (ATBed_t));
    edmem->index = aindex++;
    if ((edp = dtinsert (eddict, edmem)) != edmem) {
        SUwarning (0, "ATBinserted", "cannot insert ed");
        vmfree (Vmheap, edmem);
        return NULL;
    }
    memcpy (edp->ilevel1, level1, SZ_level);
    memcpy (edp->iid1, id1, SZ_id);
    memcpy (edp->ilevel2, level2, SZ_level);
    memcpy (edp->iid2, id2, SZ_id);
    edp->nclass1 = nclass1;
    edp->nclass2 = nclass2;
    if (sevi < 0 || sevi >= sevn) {
        SUwarning (
            0, "ATBinserted", "sevi: %d out of range: %d-%d", sevi, 0, sevn
        );
        return NULL;
    }
    if (tmode < 0 || tmode > VG_ALARM_N_MODE_MAX) {
        SUwarning (
            0, "ATBinserted", "tmode: %d out of range: %d-%d",
            tmode, 0, VG_ALARM_N_MODE_MAX
        );
        return NULL;
    }
    edp->tmi = tmodemap[tmode];
    if (type == VG_ALARM_N_TYPE_CLEAR)
        edp->sevi = clearsevi;
    else
        edp->sevi = sevi;

    if (scl && scl[0] && !(edp->scl = vmstrdup (Vmheap, scl))) {
        SUwarning (0, "ATBinserted", "cannot copy edge special color");
        return NULL;
    }

    ap = &UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_FONTNAME];
    if (!(edp->fn = vmstrdup (
        Vmheap, (ap->value && ap->value[0]) ? ap->value : "abc"
    ))) {
        SUwarning (0, "ATBinserted", "cannot copy edge font name");
        return NULL;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_FONTSIZE];
    if (!(edp->fs = vmstrdup (
        Vmheap, (ap->value && ap->value[0]) ? ap->value : "10"
    ))) {
        SUwarning (0, "ATBinserted", "cannot copy edge font size");
        return NULL;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_LABEL];
    if (!(edp->lab = vmstrdup (
        Vmheap, (ap->value && ap->value[0]) ? ap->value : ""
    ))) {
        SUwarning (0, "ATBinserted", "cannot copy edge lab");
        return NULL;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_HDR];
    if (!(edp->hdr = vmstrdup (
        Vmheap, (ap->value && ap->value[0]) ? ap->value : ""
    ))) {
        SUwarning (0, "ATBinserted", "cannot copy edge hdr");
        return NULL;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_ROW];
    if (!(edp->row = vmstrdup (
        Vmheap, (ap->value && ap->value[0]) ? ap->value : ""
    ))) {
        SUwarning (0, "ATBinserted", "cannot copy edge row");
        return NULL;
    }
    if (UTsplitstr (ap->value) == -1) {
        SUwarning (0, "ATBinserted", "cannot parse edge row");
        return NULL;
    }
    if (!(edp->cells = vmalloc (Vmheap, UTtokm * sizeof (ATBcell_t)))) {
        SUwarning (0, "ATBinserted", "cannot allocate cells array");
        return NULL;
    }
    edp->celln = UTtokm;
    for (toki = 0; toki < UTtokm; toki++) {
        tokp = UTtoks[toki];
        if ((
            tolower (*tokp) == 'l' || tolower (*tokp) == 'r' ||
            tolower (*tokp) == 'c'
        ) && (
            tolower (*(tokp + 1)) == 'f' || tolower (*(tokp + 1)) == 'i' ||
            tolower (*(tokp + 1)) == 's'
        ) && *(tokp + 2) == ':') {
            jc = tolower (*tokp), sc = tolower (*(tokp + 1)), tokp += 3;
        } else {
            jc = 'l', sc = 's';
        }
        if (!(edp->cells[toki].str = vmstrdup (Vmheap, tokp))) {
            SUwarning (0, "ATBinserted", "cannot copy cell str");
            return NULL;
        }
        switch (jc) {
        case 'l': edp->cells[toki].j = -1; break;
        case 'c': edp->cells[toki].j = 0;  break;
        case 'r': edp->cells[toki].j = 1;  break;
        }
        switch (sc) {
        case 's':
            edp->cells[toki].sa = SORTAS_STR;
            break;
        case 'i':
            edp->cells[toki].sa = SORTAS_INT;
            edp->cells[toki].i = atoi (tokp);
            break;
        case 'f':
            edp->cells[toki].sa = SORTAS_FLT;
            edp->cells[toki].f = atof (tokp);
            break;
        }
    }
    if (!(edp->url = vmstrdup (Vmheap, url))) {
        SUwarning (0, "ATBinserted", "cannot copy edge url");
        return NULL;
    }

    if (groupkeym > 0)
        edgp->edp = edp;

    return edp;
}

int ATBflatten (char *sorder, char *surl) {
    ATBnd_t *ndp;
    int ndi;
    ATBed_t *edp;
    int edi;
    char numbuf[11], *s1;
    int ci;
    ndgroup_t *ndgp;
    edgroup_t *edgp;

    for (
        ndgp = dtfirst (ndgroupdict); ndgp;
        ndgp = dtnext (ndgroupdict, ndgp)
    ) {
        if (!(ndp = ndgp->ndp))
            continue;
        for (ci = 0; ci < ndp->celln; ci++) {
            if (!(s1 = strstr (ndp->cells[ci].str, "XGC_GC_GCX")))
                continue;
            sfsprintf (
                numbuf, 11, "%-10d", (groupmode == GROUP_COUNT) ? ndgp->n : 1
            );
            memcpy (s1, numbuf, 10);
        }
    }
    for (
        edgp = dtfirst (edgroupdict); edgp;
        edgp = dtnext (edgroupdict, edgp)
    ) {
        if (!(edp = edgp->edp))
            continue;
        for (ci = 0; ci < edp->celln; ci++) {
            if (!(s1 = strstr (edp->cells[ci].str, "XGC_GC_GCX")))
                continue;
            sfsprintf (
                numbuf, 11, "%-10d", (groupmode == GROUP_COUNT) ? edgp->n : 1
            );
            memcpy (s1, numbuf, 10);
        }
    }

    if (parsesorder (sorder) == -1) {
        SUwarning (0, "ATBflatten", "cannot parse sort keys");
        return -1;
    }

    if (!(sorturl = vmstrdup (Vmheap, surl))) {
        SUwarning (0, "ATBflatten", "cannot copy sorturl");
        return -1;
    }

    ATBndn = dtsize (nddict);
    if (!(ATBnds = vmalloc (Vmheap, sizeof (ATBnd_t *) * ATBndn))) {
        SUwarning (0, "ATBflatten", "cannot allocate ATBnds array");
        return -1;
    }
    for (ndi = 0, ndp = dtfirst (nddict); ndp; ndp = dtnext (nddict, ndp))
        ATBnds[ndi++] = ndp;
    qsort (ATBnds, ATBndn, sizeof (ATBnd_t *), ndcmp);

    ATBedn = dtsize (eddict);
    if (!(ATBeds = vmalloc (Vmheap, sizeof (ATBed_t *) * ATBedn))) {
        SUwarning (0, "ATBflatten", "caenot allocate ATBeds array");
        return -1;
    }
    for (edi = 0, edp = dtfirst (eddict); edp; edp = dtnext (eddict, edp))
        ATBeds[edi++] = edp;
    qsort (ATBeds, ATBedn, sizeof (ATBed_t *), edcmp);

    return 0;
}

int ATBsetupprint (char *rendermode, char *itattr) {
    UTattr_t *ap;
    char *namestr, *mapstr, *stflags, *ststr, *s1, *s2, *s3, *s4, *s5;
    int tmodei, sevi, sevattrk, sevattrl;
    int tmi, tmk, tml, ftmi, ttmi;
    void *tokp;

    if (UTsplitattr (UT_ATTRGROUP_MAIN, itattr) == -1) {
        SUwarning (0, "ATBsetupprint", "cannot parse itattr");
        return -1;
    }
    if (UTgetattrs (UT_ATTRGROUP_MAIN) == -1) {
        SUwarning (0, "ATBsetupprint", "cannot get alarmtab attr");
        return -1;
    }

    if (!(sevattrs = vmalloc (Vmheap, sizeof (ATBsevattr_t) * sevn))) {
        SUwarning (0, "ATBsetupprint", "cannot allocate sevattrs");
        return -1;
    }
    memset (sevattrs, 0, sizeof (ATBsevattr_t) * sevn);
    ap = &UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_TMODENAMES];
    namestr = (ap->value) ? ap->value : NULL;
    for (s1 = namestr; s1 && s1[0]; s1 = s2) {
        if ((s2 = strchr (s1, '|')))
            *s2++ = 0;
        if (!(s3 = strchr (s1, ':'))) {
            SUwarning (0, "ATBsetupprint", "bad tmodename label: %s", s1);
            break;
        }
        *s3++ = 0;
        tmodei = -1;
        if (strcmp (s1, "ticketed") == 0)
            tmodei = ATB_TMODE_YTICKET;
        else if (strcmp (s1, "deferred") == 0)
            tmodei = ATB_TMODE_DTICKET;
        else if (strcmp (s1, "filtered") == 0)
            tmodei = ATB_TMODE_NTICKET;
        if (tmodei < 0 || tmodei >= ATB_TMODE_SIZE)
            continue;
        if (!(tmodenames[tmodei] = vmstrdup (Vmheap, s3))) {
            SUwarning (0, "ATBsetupprint", "cannot copy label: %s", s3);
            break;
        }
    }
    ap = &UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_SEVNAMES];
    namestr = (ap->value) ? ap->value : NULL;
    for (s1 = namestr; s1 && s1[0]; s1 = s2) {
        if ((s2 = strchr (s1, '|')))
            *s2++ = 0;
        if (!(s3 = strchr (s1, ':'))) {
            SUwarning (0, "ATBsetupprint", "bad sevname label: %s", s1);
            break;
        }
        *s3++ = 0;
        sevi = -1;
        if (isdigit (*s1))
            sevi = atoi (s1);
        else if (strcmp (s1, "clear") == 0)
            sevi = clearsevi;
        if (sevi < 0 || sevi >= sevn)
            continue;
        if (!(sevattrs[sevi].name = vmstrdup (Vmheap, s3))) {
            SUwarning (0, "ATBsetupprint", "cannot copy label: %s", s3);
            break;
        }
    }
    ap = &UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_SEVMAP];
    mapstr = (ap->value) ? ap->value : "1";
    for (s1 = mapstr; s1; s1 = s2) {
        if ((s2 = strchr (s1, '|')))
            *s2++ = 0;
        if (
            !(s3 = strchr (s1, ':')) || !(s4 = strchr (s3 + 1, ':')) ||
            !(s5 = strchr (s4 + 1, ':'))
        ) {
            SUwarning (0, "ATBsetupprint", "bad sevmap label: %s", s1);
            break;
        }
        *s3++ = 0, *s4++ = 0, *s5++ = 0;
        sevattrk = -1;
        if (isdigit (*s1))
            sevattrk = sevattrl = atoi (s1);
        else if (strcmp (s1, "any") == 0)
            sevattrk = 0, sevattrl = sevn - 1;
        if (sevattrk < 0 || sevattrk >= sevn)
            continue;
        tmi = -1;
        if (strcmp (s3, "ticketed") == 0)
            tmk = tml = ATB_TMODE_YTICKET;
        else if (strcmp (s3, "deferred") == 0)
            tmk = tml = ATB_TMODE_DTICKET;
        else if (strcmp (s3, "filtered") == 0)
            tmk = tml = ATB_TMODE_NTICKET;
        else if (strcmp (s3, "all") == 0)
            tmk = 0, tml = ATB_TMODE_SIZE - 1;
        if (tmk < 0 || tmk >= ATB_TMODE_SIZE)
            continue;
        for (sevi = sevattrk; sevi <= sevattrl; sevi++) {
            for (tmi = tmk; tmi <= tml; tmi++) {
                if (!(sevattrs[sevi].bg[tmi] = vmstrdup (Vmheap, s4))) {
                    SUwarning (0, "ATBsetupprint", "cannot copy bg: %s", s4);
                    break;
                }
                if (!(sevattrs[sevi].cl[tmi] = vmstrdup (Vmheap, s5))) {
                    SUwarning (0, "ATBsetupprint", "cannot copy color: %s", s5);
                    break;
                }
            }
        }
    }

    ap = &UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_FLAGS];
    stflags = (ap->value && ap->value[0]) ? ap->value : "";
    tokp = tokopen (stflags, 1);
    while ((ststr = tokread (tokp))) {
        if (strncmp (ststr, "map_", 4) == 0) {
            s1 = ststr + 4;
            if (!(s2 = strchr (s1, '_')))
                continue;
            *s2++ = 0;
            if (strcmp (s1, "ticketed") == 0)
                ftmi = ATB_TMODE_YTICKET;
            else if (strcmp (s1, "deferred") == 0)
                ftmi = ATB_TMODE_DTICKET;
            else if (strcmp (s1, "filtered") == 0)
                ftmi = ATB_TMODE_NTICKET;
            else
                continue;
            if (strcmp (s2, "ticketed") == 0)
                ttmi = ATB_TMODE_YTICKET;
            else if (strcmp (s2, "deferred") == 0)
                ttmi = ATB_TMODE_DTICKET;
            else if (strcmp (s2, "filtered") == 0)
                ttmi = ATB_TMODE_NTICKET;
            else
                continue;
            tmodetrans[ftmi] = ttmi;
        } else if (strncmp (ststr, "hide_", 5) == 0) {
            s1 = ststr + 5;
            if (strcmp (s1, "ticketed") == 0)
                ftmi = ATB_TMODE_YTICKET;
            else if (strcmp (s1, "deferred") == 0)
                ftmi = ATB_TMODE_DTICKET;
            else if (strcmp (s1, "filtered") == 0)
                ftmi = ATB_TMODE_NTICKET;
            else
                continue;
            tmodetrans[ftmi] = -1;
        }
    }
    tokclose (tokp);

    return 0;
}

int ATBbeginnd (char *fprefix, int findex) {
    UTattr_t *ap;
    char *itbg, *itcl, *itfn, *itfs;

    if (!(rip = RIopen (RI_TYPE_TABLE, fprefix, findex, 0, 0))) {
        SUwarning (0, "ATBbeginnd", "cannot open table file");
        return -1;
    }

    RIaddop (rip, NULL); // reset

    ap = &UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_BGCOLOR];
    itbg = (ap->value && ap->value[0]) ? ap->value : "white";
    ap = &UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_COLOR];
    itcl = (ap->value && ap->value[0]) ? ap->value : "black";
    ap = &UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_FONTNAME];
    itfn = (ap->value && ap->value[0]) ? ap->value : "Times-Roman";
    ap = &UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_FONTSIZE];
    itfs = (ap->value && ap->value[0]) ? ap->value : "8";

    if (UTaddtablebegin (rip, itfn, itfs, itbg, itcl) == -1) {
        SUwarning (0, "ATBbeginnd", "cannot add table begin");
        return -1;
    }

    pageindex = pagei = 0;

    return 0;
}

int ATBendnd (char *fprefix) {
    Sfio_t *fp;
    int sevi, tmi;

    if (pagem > 0)
        RIaddtoc (rip, 123456789, NULL);

    if (UTaddtableend (rip) == -1) {
        SUwarning (0, "ATBendnd", "cannot add table end");
        return -1;
    }

    RIexecop (rip, NULL, 0, "black", "black", NULL, NULL, "Times-Roman", "10");

    if (RIclose (rip) == -1) {
        SUwarning (0, "ATBendnd", "cannot close table file");
        return -1;
    }

    if (!(fp = sfopen (NULL, sfprints ("%s.info", fprefix), "w"))) {
        SUwarning (0, "ATBendnd", "cannot open info file");
        return -1;
    }
    sfprintf (
        fp,
        "<table class=sdiv>"
        "<caption class=sdiv style='font-size:80%%'>color map</caption>\n"
    );
    for (tmi = 0; tmi <= ATB_TMODE_SIZE; tmi++) {
        for (sevi = 1; sevi < sevn; sevi++)
            if (sevattrs[sevi].used[tmi] > 0)
                break;
        if (sevi >= sevn - 1)
            continue;
        sfprintf (fp, "<tr class=sdiv>\n");
        sfprintf (
            fp,
            "<td class=sdiv style='font-size:80%%'>&nbsp;%s:&nbsp;</td>\n",
            tmodenames[tmi]
        );
        for (sevi = 1; sevi < sevn; sevi++) {
            if (!sevattrs[sevi].bg[tmi] || !sevattrs[sevi].cl[tmi]) {
                sfprintf (fp, "<td class=sdiv></td>\n");
                continue;
            }
            sfprintf (
                fp,
                "<td class=sdiv "
                "style='background-color:%s; color:%s; font-size:80%%'>"
                "&nbsp;%s&nbsp;"
                "</td>\n",
                sevattrs[sevi].bg[tmi] ? sevattrs[sevi].bg[tmi] : "",
                sevattrs[sevi].cl[tmi] ? sevattrs[sevi].cl[tmi] : "",
                sevattrs[sevi].name ? sevattrs[sevi].name : ""
            );
        }
        sfprintf (fp, "</tr>\n");
    }
    sfprintf (fp, "</table>\n");
    if (sfclose (fp) == -1) {
        SUwarning (0, "ATBendnd", "cannot cloas info file");
        return -1;
    }

    return 0;
}

int ATBprintnd (int hdrflag, ATBnd_t *ndp, char *obj) {
    UTattr_t *ap;
    char *itbg, *itcl, *itfn, *itfs;
    EMbb_t bb;
    sortkey_t *skp;
    int ski;
    char *tokp, jc, *dir, *ustr, *tstr;
    int j;
    ATBcell_t *cellp;
    int celli;
    int toki, opi, opj, opn;

    if (hdrflag) {
        ap = &UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_BGCOLOR];
        itbg = (ap->value && ap->value[0]) ? ap->value : "white";
        ap = &UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_COLOR];
        itcl = (ap->value && ap->value[0]) ? ap->value : "black";
        ap = &UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_FONTNAME];
        itfn = (ap->value && ap->value[0]) ? ap->value : "Times-Roman";
        ap = &UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_FONTSIZE];
        itfs = (ap->value && ap->value[0]) ? ap->value : "8";

        if (UTaddtablecaption (
            rip, ndp->lab, itfn, itfs, itbg, itcl, NULL, NULL
        ) == -1) {
            SUwarning (0, "ATBprintnd", "cannot add caption");
            return -1;
        }
        if (UTsplitstr (ndp->hdr) == -1) {
            SUwarning (0, "ATBprintnd", "cannot parse node header");
            return -1;
        }
        if (UTaddrowbegin (rip, itfn, itfs, itbg, itcl) == -1) {
            SUwarning (0, "ATBprintnd", "cannot add row begin");
            return -1;
        }
        for (toki = 0; toki < UTtokm; toki++) {
            tokp = UTtoks[toki];
            if ((
                tolower (*tokp) == 'l' || tolower (*tokp) == 'r' ||
                tolower (*tokp) == 'c'
            ) && *(tokp + 1) == ':')
                jc = tolower (*tokp), tokp += 2;
            else
                jc = 'l';
            switch (jc) {
            case 'r': j = 1;  break;
            case 'c': j = 0;  break;
            default:  j = -1; break;
            }
            if (UTaddcellbegin (rip, itfn, itfs, itbg, itcl, j, 1) == -1) {
                SUwarning (0, "ATBprintnd", "cannot add hdr cell begin");
                return -1;
            }
            if (sorturl && sorturl[0]) {
                dir = "f";
                for (ski = 0; ski < sortkeym; ski++) {
                    skp = &sortkeys[ski];
                    if (skp->key == KEY_CELL && skp->ci == toki) {
                        dir = (skp->dir == DIR_FWD) ? "r" : "f";
                        break;
                    }
                }
                if (UTaddlinkbegin (
                    rip,
                    sfprints ("%s&alarmtab_sortorder=%d%s", sorturl, toki, dir),
                    "sort table by this field",
                    itfn, itfs, itbg, itcl
                ) == -1) {
                    SUwarning (0, "ATBprintnd", "cannot add hdr link begin");
                    return -1;
                }
            }
            if (UTaddcelltext (rip, tokp) == -1) {
                SUwarning (0, "ATBprintnd", "cannot add hdr cell");
                return -1;
            }
            if (sorturl && sorturl[0]) {
                if (UTaddlinkend (rip) == -1) {
                    SUwarning (0, "ATBprintnd", "cannot add data link end");
                    return -1;
                }
            }
            if (UTaddcellend (rip) == -1) {
                SUwarning (0, "ATBprintnd", "cannot add hdr cell end");
                return -1;
            }
        }
        if (UTaddrowend (rip) == -1) {
            SUwarning (0, "ATBprintnd", "cannot add row end");
            return -1;
        }
    }

    if (tmodetrans[ndp->tmi] == -1)
        return 0;

    if (UTsplitstr (ndp->url) == -1) {
        SUwarning (0, "ATBprintnd", "cannot parse node urls");
        return -1;
    }
    sevattrs[ndp->sevi].used[tmodetrans[ndp->tmi]] = 1;
    if (pagem > 0 && pagei++ % pagem == 0)
        RIaddtoc (rip, ++pageindex, (ndp->celln > 0) ? ndp->cells[0].str : "");
    if (UTaddrowbegin (
        rip, ndp->fn, ndp->fs,
        ndp->scl ? ndp->scl : sevattrs[ndp->sevi].bg[tmodetrans[ndp->tmi]],
        sevattrs[ndp->sevi].cl[tmodetrans[ndp->tmi]]
    ) == -1) {
        SUwarning (0, "ATBprintnd", "cannot add row begin");
        return -1;
    }
    for (celli = 0; celli < ndp->celln; celli++) {
        cellp = &ndp->cells[celli];
        if (UTaddcellbegin (
            rip, ndp->fn, ndp->fs,
            ndp->scl ? ndp->scl : sevattrs[ndp->sevi].bg[tmodetrans[ndp->tmi]],
            sevattrs[ndp->sevi].cl[tmodetrans[ndp->tmi]], cellp->j, 1
        ) == -1) {
            SUwarning (0, "ATBprintnd", "cannot add data cell begin");
            return -1;
        }
        if (celli < UTtokm && UTtoks[celli][0]) {
            if ((tstr = strstr (UTtoks[celli], "TITLE:"))) {
                if (tstr > UTtoks[celli] && tstr[-1] == ':')
                    tstr[-1] = 0;
                tstr += 6;
            }
            if ((ustr = strstr (UTtoks[celli], "URL:"))) {
                if (ustr > UTtoks[celli] && ustr[-1] == ':')
                    ustr[-1] = 0;
                ustr += 4;
            }
            if (!ustr) {
                if (tstr)
                    ustr = NULL;
                else
                    ustr = UTtoks[celli];
            }
            if (UTaddlinkbegin (
                rip, ustr, tstr, ndp->fn, ndp->fs,
                sevattrs[ndp->sevi].bg[tmodetrans[ndp->tmi]],
                sevattrs[ndp->sevi].cl[tmodetrans[ndp->tmi]]
            ) == -1) {
                SUwarning (0, "ATBprintnd", "cannot add data link begin");
                return -1;
            }
        }
        if (strncmp (cellp->str, "EMBED:", 6) != 0) {
            if (UTaddcelltext (rip, cellp->str) == -1) {
                SUwarning (0, "ATBprintnd", "cannot add data cell");
                return -1;
            }
        } else {
            if (EMparsenode (
                ndp->ilevel, ndp->iid, cellp->str, EM_DIR_V, &opi, &opn, &bb
            ) == -1) {
                SUwarning (0, "ATBprintnd", "cannot parse node EM contents");
                return -1;
            }
            for (opj = 0; opj < opn; opj++) {
                if (RIaddop (rip, &EMops[opi + opj]) == -1) {
                    SUwarning (0, "ATBprintnd", "cannot add data cell op");
                    return -1;
                }
            }
        }
        if (celli < UTtokm && UTtoks[celli][0]) {
            if (UTaddlinkend (rip) == -1) {
                SUwarning (0, "ATBprintnd", "cannot add data link end");
                return -1;
            }
        }
        if (UTaddcellend (rip) == -1) {
            SUwarning (0, "ATBprintnd", "cannot add data cell end");
            return -1;
        }
    }
    if (UTaddrowend (rip) == -1) {
        SUwarning (0, "ATBprintnd", "cannot add row end");
        return -1;
    }

    return 0;
}

int ATBbegined (char *fprefix, int findex) {
    UTattr_t *ap;
    char *itbg, *itcl, *itfn, *itfs;

    if (!(rip = RIopen (RI_TYPE_TABLE, fprefix, findex, 0, 0))) {
        SUwarning (0, "ATBbegined", "cannot open table file");
        return -1;
    }

    RIaddop (rip, NULL); // reset

    ap = &UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_BGCOLOR];
    itbg = (ap->value && ap->value[0]) ? ap->value : "white";
    ap = &UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_COLOR];
    itcl = (ap->value && ap->value[0]) ? ap->value : "black";
    ap = &UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_FONTNAME];
    itfn = (ap->value && ap->value[0]) ? ap->value : "Times-Roman";
    ap = &UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_FONTSIZE];
    itfs = (ap->value && ap->value[0]) ? ap->value : "8";

    if (UTaddtablebegin (rip, itfn, itfs, itbg, itcl) == -1) {
        SUwarning (0, "ATBbegined", "cannot add table begin");
        return -1;
    }

    pageindex = pagei = 0;

    return 0;
}

int ATBended (char *fprefix) {
    Sfio_t *fp;
    int sevi, tmi;

    if (pagem > 0)
        RIaddtoc (rip, 123456789, NULL);

    if (UTaddtableend (rip) == -1) {
        SUwarning (0, "ATBended", "cannot add table eed");
        return -1;
    }

    RIexecop (rip, NULL, 0, "black", "black", NULL, NULL, "Times-Roman", "10");

    if (RIclose (rip) == -1) {
        SUwarning (0, "ATBended", "cannot close table file");
        return -1;
    }

    if (!(fp = sfopen (NULL, sfprints ("%s.info", fprefix), "w"))) {
        SUwarning (0, "ATBended", "cannot open info file");
        return -1;
    }
    sfprintf (
        fp,
        "<table class=sdiv>"
        "<caption class=sdiv style='font-size:80%%'>color map</caption>\n"
    );
    for (tmi = 0; tmi <= ATB_TMODE_SIZE; tmi++) {
        for (sevi = 1; sevi < sevn; sevi++)
            if (sevattrs[sevi].used[tmi] > 0)
                break;
        if (sevi >= sevn - 1)
            continue;
        sfprintf (fp, "<tr class=sdiv>\n");
        sfprintf (
            fp,
            "<td class=sdiv style='font-size:80%%'>&nbsp;%s:&nbsp;</td>\n",
            tmodenames[tmi]
        );
        for (sevi = 1; sevi < sevn; sevi++) {
            if (!sevattrs[sevi].bg[tmi] || !sevattrs[sevi].cl[tmi]) {
                sfprintf (fp, "<td class=sdiv></td>\n");
                continue;
            }
            sfprintf (
                fp,
                "<td class=sdiv "
                "style='background-color:%s; color:%s; font-size:80%%'>"
                "&nbsp;%s&nbsp;"
                "</td>\n",
                sevattrs[sevi].bg[tmi] ? sevattrs[sevi].bg[tmi] : "",
                sevattrs[sevi].cl[tmi] ? sevattrs[sevi].cl[tmi] : "",
                sevattrs[sevi].name ? sevattrs[sevi].name : ""
            );
        }
    }
    sfprintf (fp, "</table>\n");
    if (sfclose (fp) == -1) {
        SUwarning (0, "ATBended", "cannot cloas info file");
        return -1;
    }
    return 0;
}

int ATBprinted (int hdrflag, ATBed_t *edp, char *obj) {
    UTattr_t *ap;
    char *itbg, *itcl, *itfn, *itfs;
    EMbb_t bb;
    sortkey_t *skp;
    int ski;
    char *tokp, jc, *dir, *ustr, *tstr;
    int j;
    ATBcell_t *cellp;
    int celli;
    int toki, opi, opj, opn;

    if (hdrflag) {
        ap = &UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_BGCOLOR];
        itbg = (ap->value && ap->value[0]) ? ap->value : "white";
        ap = &UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_COLOR];
        itcl = (ap->value && ap->value[0]) ? ap->value : "black";
        ap = &UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_FONTNAME];
        itfn = (ap->value && ap->value[0]) ? ap->value : "Times-Roman";
        ap = &UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_FONTSIZE];
        itfs = (ap->value && ap->value[0]) ? ap->value : "8";

        if (UTaddtablecaption (
            rip, edp->lab, itfn, itfs, itbg, itcl, NULL, NULL
        ) == -1) {
            SUwarning (0, "ATBprinted", "cannot add caption");
            return -1;
        }
        if (UTsplitstr (edp->hdr) == -1) {
            SUwarning (0, "ATBprinted", "cannot parse edge header");
            return -1;
        }
        if (UTaddrowbegin (rip, itfn, itfs, itbg, itcl) == -1) {
            SUwarning (0, "ATBprinted", "cannot add row begin");
            return -1;
        }
        for (toki = 0; toki < UTtokm; toki++) {
            tokp = UTtoks[toki];
            if ((
                tolower (*tokp) == 'l' || tolower (*tokp) == 'r' ||
                tolower (*tokp) == 'c'
            ) && *(tokp + 1) == ':')
                jc = tolower (*tokp), tokp += 2;
            else
                jc = 'l';
            switch (jc) {
            case 'r': j = 1;  break;
            case 'c': j = 0;  break;
            default:  j = -1; break;
            }
            if (UTaddcellbegin (rip, itfn, itfs, itbg, itcl, j, 1) == -1) {
                SUwarning (0, "ATBprinted", "cannot add data cell begin");
                return -1;
            }
            if (sorturl && sorturl[0]) {
                dir = "f";
                for (ski = 0; ski < sortkeym; ski++) {
                    skp = &sortkeys[ski];
                    if (skp->key == KEY_CELL && skp->ci == toki) {
                        dir = (skp->dir == DIR_FWD) ? "r" : "f";
                        break;
                    }
                }
                if (UTaddlinkbegin (
                    rip,
                    sfprints ("%s&alarmtab_sortorder=%d%s", sorturl, toki, dir),
                    "sort table by this field",
                    itfn, itfs, itbg, itcl
                ) == -1) {
                    SUwarning (0, "ATBprinted", "cannot add hdr link begin");
                    return -1;
                }
            }
            if (UTaddcelltext (rip, tokp) == -1) {
                SUwarning (0, "ATBprinted", "cannot add hdr cell");
                return -1;
            }
            if (sorturl && sorturl[0]) {
                if (UTaddlinkend (rip) == -1) {
                    SUwarning (0, "ATBprinted", "cannot add hdr link end");
                    return -1;
                }
            }
            if (UTaddcellend (rip) == -1) {
                SUwarning (0, "ATBprinted", "cannot add data cell end");
                return -1;
            }
        }
        if (UTaddrowend (rip) == -1) {
            SUwarning (0, "ATBprinted", "cannot add row end");
            return -1;
        }
    }

    if (tmodetrans[edp->tmi] == -1)
        return 0;

    if (UTsplitstr (edp->url) == -1) {
        SUwarning (0, "ATBprintnd", "cannot parse edge urls");
        return -1;
    }
    if (pagem > 0 && pagei++ % pagem == 0)
        RIaddtoc (rip, ++pageindex, (edp->celln > 0) ? edp->cells[0].str : "");
    if (UTaddrowbegin (
        rip, edp->fn, edp->fs,
        edp->scl ? edp->scl : sevattrs[edp->sevi].bg[tmodetrans[edp->tmi]],
        sevattrs[edp->sevi].cl[tmodetrans[edp->tmi]]
    ) == -1) {
        SUwarning (0, "ATBprinted", "cannot add row begin");
        return -1;
    }
    for (celli = 0; celli < edp->celln; celli++) {
        cellp = &edp->cells[celli];
        if (UTaddcellbegin (
            rip, edp->fn, edp->fs,
            edp->scl ? edp->scl : sevattrs[edp->sevi].bg[tmodetrans[edp->tmi]],
            sevattrs[edp->sevi].cl[tmodetrans[edp->tmi]], cellp->j, 1
        ) == -1) {
            SUwarning (0, "ATBprinted", "cannot add data cell begin");
            return -1;
        }
        if (celli < UTtokm && UTtoks[celli][0]) {
            if ((tstr = strstr (UTtoks[celli], "TITLE:"))) {
                if (tstr > UTtoks[celli] && tstr[-1] == ':')
                    tstr[-1] = 0;
                tstr += 6;
            }
            if ((ustr = strstr (UTtoks[celli], "URL:"))) {
                if (ustr > UTtoks[celli] && ustr[-1] == ':')
                    ustr[-1] = 0;
                ustr += 4;
            }
            if (!ustr) {
                if (tstr)
                    ustr = NULL;
                else
                    ustr = UTtoks[celli];
            }
            if (UTaddlinkbegin (
                rip, ustr, tstr, edp->fn, edp->fs,
                sevattrs[edp->sevi].bg[tmodetrans[edp->tmi]],
                sevattrs[edp->sevi].cl[tmodetrans[edp->tmi]]
            ) == -1) {
                SUwarning (0, "ATBprinted", "cannot add data link begin");
                return -1;
            }
        }
        if (strncmp (cellp->str, "EMBED:", 6) == 0) {
            if (UTaddcelltext (rip, cellp->str) == -1) {
                SUwarning (0, "ATBprinted", "cannot add data cell");
                return -1;
            }
        } else {
            if (EMparseedge (
                edp->ilevel1, edp->iid1, edp->ilevel2, edp->iid2,
                cellp->str, EM_DIR_V, &opi, &opn, &bb
            ) == -1) {
                SUwarning (0, "ATBprinted", "cannot parse edge EM contents");
                return -1;
            }
            for (opj = 0; opi < opn; opj++) {
                if (RIaddop (rip, &EMops[opi + opj]) == -1) {
                    SUwarning (0, "ATBprinted", "cannot add data cell op");
                    return -1;
                }
            }
        }
        if (celli < UTtokm && UTtoks[celli][0]) {
            if (UTaddlinkend (rip) == -1) {
                SUwarning (0, "ATBprinted", "cannot add data link end");
                return -1;
            }
        }
        if (UTaddcellend (rip) == -1) {
            SUwarning (0, "ATBprinted", "cannot add data cell end");
            return -1;
        }
    }
    if (UTaddrowend (rip) == -1) {
        SUwarning (0, "ATBprinted", "cannot add row end");
        return -1;
    }

    return 0;
}

static int parsesorder (char *str) {
    sortkey_t *skp;
    char *s1, *s2;
    int key, ci, dir;

    sortkeym = 0;
    for (s1 = str; s1; s1 = s2) {
        if ((s2 = strchr (s1, ' ')))
            *s2++ = 0;

        ci = 0;
        if (*s1 == 'i')
            key = KEY_IDENT, s1++;
        else if (isdigit (*s1)) {
            key = KEY_CELL;
            ci = *s1++ - '0';
            while (isdigit (*s1))
                ci = ci * 10 + *s1++ - '0';
        } else {
            SUwarning (0, "parsesorder", "unknown key: %s", s1);
            continue;
        }

        if (!*s1 || *s1 == 'f')
            dir = DIR_FWD;
        else if (*s1 == 'r')
            dir = DIR_REV;
        else {
            SUwarning (0, "parsesorder", "unknown dir: %s", s1);
            continue;
        }

        if (sortkeym >= sortkeyn) {
            if (!(sortkeys = vmresize (
                Vmheap, sortkeys, (sortkeyn + 10) * sizeof (sortkey_t),
                VM_RSCOPY
            ))) {
                SUwarning (0, "parsesorder", "cannot grow sortkeys array");
                return -1;
            }
            sortkeyn += 10;
        }
        skp = &sortkeys[sortkeym++];
        skp->key = key, skp->ci = ci; skp->dir = dir;
    }

    return 0;
}

static int parsegorder (char *str) {
    groupkey_t *gkp;
    char *s1, *s2;
    int key, ci;

    groupkeym = 0;
    groupmode = GROUP_FIRST;
    for (s1 = str; s1 && *s1; s1 = s2) {
        if ((s2 = strchr (s1, ' ')))
            *s2++ = 0;

        ci = 0;
        if (*s1 == 'u' || *s1 == 'f') {
            groupmode = GROUP_FIRST, s1++;
            continue;
        } else if (*s1 == 'l') {
            groupmode = GROUP_LAST, s1++;
            continue;
        } else if (*s1 == 'c') {
            groupmode = GROUP_COUNT, s1++;
            continue;
        } else if (*s1 == 'i')
            key = KEY_IDENT, s1++;
        else if (isdigit (*s1)) {
            key = KEY_CELL;
            ci = *s1++ - '0';
            while (isdigit (*s1))
                ci = ci * 10 + *s1++ - '0';
        } else {
            SUwarning (0, "parsegorder", "unknown token: %s", s1);
            continue;
        }

        if (groupkeym >= groupkeyn) {
            if (!(groupkeys = vmresize (
                Vmheap, groupkeys, (groupkeyn + 10) * sizeof (groupkey_t),
                VM_RSCOPY
            ))) {
                SUwarning (0, "parsegorder", "cannot grow groupkeys array");
                return -1;
            }
            groupkeyn += 10;
        }
        gkp = &groupkeys[groupkeym++];
        gkp->key = key, gkp->ci = ci;
    }

    return 0;
}

static int ndcmp (const void *a, const void *b) {
    ATBnd_t *ap, *bp;
    sortkey_t *skp;
    int ski, sa;
    ATBcell_t *cpa, *cpb;
    int ci;
    int ret;
    float fret;

    ap = *(ATBnd_t **) a;
    bp = *(ATBnd_t **) b;

    if ((ret = ap->nclass - bp->nclass) != 0)
        return ret;
    if (sortkeym == 0) {
        if ((ret = strcmp (ap->ilevel, bp->ilevel)) != 0)
            return ret;
        if ((ret = strcmp (ap->iid, bp->iid)) != 0)
            return ret;
    } else {
        for (ski = 0; ski < sortkeym; ski++) {
            skp = &sortkeys[ski];
            switch (skp->key) {
            case KEY_IDENT:
                if ((ret = strcmp (ap->ilevel, bp->ilevel)) != 0)
                    return (skp->dir == DIR_FWD ? ret : -ret);
                if ((ret = strcmp (ap->iid, bp->iid)) != 0)
                    return (skp->dir == DIR_FWD ? ret : -ret);
                break;
            case KEY_CELL:
                if ((ci = skp->ci) >= 0 && ci < ap->celln && ci < bp->celln) {
                    cpa = &ap->cells[ci];
                    cpb = &bp->cells[ci];
                    if ((sa = cpa->sa) != cpb->sa)
                        sa = SORTAS_STR;
                    switch (sa) {
                    case SORTAS_STR:
                        if ((ret = strcmp (cpa->str, cpb->str)) != 0)
                            return (skp->dir == DIR_FWD ? ret : -ret);
                        break;
                    case SORTAS_INT:
                        if ((ret = (cpa->i - cpb->i)) != 0)
                            return (skp->dir == DIR_FWD ? ret : -ret);
                        break;
                    case SORTAS_FLT:
                        if ((fret = (cpa->f - cpb->f)) != 0) {
                            ret = (fret < 0.0) ? -1 : ((fret > 0.0) ? 1 : 0);
                            return (skp->dir == DIR_FWD ? ret : -ret);
                        }
                        break;
                    }
                }
                break;
            }
        }
    }

    return 0;
}

static int edcmp (const void *a, const void *b) {
    ATBed_t *ap, *bp;
    sortkey_t *skp;
    int ski, sa;
    ATBcell_t *cpa, *cpb;
    int ci;
    int ret;
    float fret;

    ap = *(ATBed_t **) a;
    bp = *(ATBed_t **) b;

    if ((ret = ap->nclass1 - bp->nclass1) != 0)
        return ret;
    if ((ret = ap->nclass2 - bp->nclass2) != 0)
        return ret;
    if (sortkeym == 0) {
        if ((ret = strcmp (ap->ilevel1, bp->ilevel1)) != 0)
            return ret;
        if ((ret = strcmp (ap->iid1, bp->iid1)) != 0)
            return ret;
        if ((ret = strcmp (ap->ilevel2, bp->ilevel2)) != 0)
            return ret;
        if ((ret = strcmp (ap->iid2, bp->iid2)) != 0)
            return ret;
    } else {
        for (ski = 0; ski < sortkeym; ski++) {
            skp = &sortkeys[ski];
            switch (skp->key) {
            case KEY_IDENT:
                if ((ret = strcmp (ap->ilevel1, bp->ilevel1)) != 0)
                    return (skp->dir == DIR_FWD ? ret : -ret);
                if ((ret = strcmp (ap->iid1, bp->iid1)) != 0)
                    return (skp->dir == DIR_FWD ? ret : -ret);
                if ((ret = strcmp (ap->ilevel2, bp->ilevel2)) != 0)
                    return (skp->dir == DIR_FWD ? ret : -ret);
                if ((ret = strcmp (ap->iid2, bp->iid2)) != 0)
                    return (skp->dir == DIR_FWD ? ret : -ret);
                break;
            case KEY_CELL:
                if ((ci = skp->ci) >= 0 && ci < ap->celln && ci < bp->celln) {
                    cpa = &ap->cells[ci];
                    cpb = &bp->cells[ci];
                    if ((sa = cpa->sa) != cpb->sa)
                        sa = SORTAS_STR;
                    switch (sa) {
                    case SORTAS_STR:
                        if ((ret = strcmp (cpa->str, cpb->str)) != 0)
                            return (skp->dir == DIR_FWD ? ret : -ret);
                        break;
                    case SORTAS_INT:
                        if ((ret = (cpa->i - cpb->i)) != 0)
                            return (skp->dir == DIR_FWD ? ret : -ret);
                        break;
                    case SORTAS_FLT:
                        if ((fret = (cpa->f - cpb->f)) != 0) {
                            ret = (fret < 0.0) ? -1 : ((fret > 0.0) ? 1 : 0);
                            return (skp->dir == DIR_FWD ? ret : -ret);
                        }
                        break;
                    }
                }
                break;
            }
        }
    }

    return 0;
}
