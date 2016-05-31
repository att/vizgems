#include <ast.h>
#include <ast_float.h>
#include <tok.h>
#include <swift.h>
#include <ctype.h>
#include "vg_dq_vt_util.h"

ITBnd_t **ITBnds;
int ITBndn;
ITBed_t **ITBeds;
int ITBedn;

static char *pagemode;
static int pageindex, pagei, pagem;

static Dt_t *nddict;
static Dtdisc_t nddisc = {
    sizeof (Dtlink_t), SZ_level + SZ_id, 0, NULL, NULL, NULL, NULL, NULL, NULL
};
static Dt_t *eddict;
static Dtdisc_t eddisc = {
    sizeof (Dtlink_t), sizeof (ITBnd_t *) * 2,
    0, NULL, NULL, NULL, NULL, NULL, NULL
};

static char **bgs, **cls;
static int bgn, bgm, bgi, cln, clm, cli;

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

static RI_t *rip;

static int parseorder (char *);
static int ndcmp (const void *, const void *);
static int edcmp (const void *, const void *);

int ITBinit (char *pm) {
    if (!(nddict = dtopen (&nddisc, Dtset))) {
        SUwarning (0, "ITBinit", "cannot create nddict");
        return -1;
    }
    if (!(eddict = dtopen (&eddisc, Dtset))) {
        SUwarning (0, "ITBinit", "cannot create eddict");
        return -1;
    }
    ITBnds = NULL;
    ITBndn = 0;
    ITBeds = NULL;
    ITBedn = 0;

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

    UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_BGCOLOR].name = "bgcolor";
    UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_COLOR].name = "color";
    UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_FONTNAME].name = "fontname";
    UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_FONTSIZE].name = "fontsize";
    UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_LABEL].name = "label";
    UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_HDR].name = "hdr";
    UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_ROW].name = "row";

    UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_BGCOLOR].name = "bgcolor";
    UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_COLOR].name = "color";
    UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_FONTNAME].name = "fontname";
    UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_FONTSIZE].name = "fontsize";
    UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_LABEL].name = "label";
    UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_HDR].name = "hdr";
    UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_ROW].name = "row";

    return 0;
}

ITBnd_t *ITBinsertnd (char *level, char *id, short nclass, char *attrstr) {
    ITBnd_t nd, *ndp, *ndmem;
    UTattr_t *ap;
    char *tokp, jc, sc;
    int toki;

    memcpy (nd.level, level, SZ_level);
    memcpy (nd.id, id, SZ_id);
    if ((ndp = dtsearch (nddict, &nd)))
        return ndp;

    if (!(ndmem = vmalloc (Vmheap, sizeof (ITBnd_t)))) {
        SUwarning (0, "ITBinsertnd", "cannot allocate ndmem");
        return NULL;
    }
    memset (ndmem, 0, sizeof (ITBnd_t));
    memcpy (ndmem->level, level, SZ_level);
    memcpy (ndmem->id, id, SZ_id);
    if ((ndp = dtinsert (nddict, ndmem)) != ndmem) {
        SUwarning (0, "ITBinsertnd", "cannot insert nd");
        vmfree (Vmheap, ndmem);
        return NULL;
    }
    ndp->nclass = nclass;

    if (UTsplitattr (UT_ATTRGROUP_NODE, attrstr) == -1) {
        SUwarning (0, "ITBinsertnd", "cannot parse attrstr");
        return NULL;
    }
    if (UTgetattrs (UT_ATTRGROUP_NODE) == -1) {
        SUwarning (0, "ITBinsertnd", "cannot get node attr");
        return NULL;
    }

    ap = &UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_BGCOLOR];
    if (!(ndp->bg = vmstrdup (
        Vmheap, (ap->value && ap->value[0]) ? ap->value : "white"
    ))) {
        SUwarning (0, "ITBinsertnd", "cannot copy node bgcolor");
        return NULL;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_COLOR];
    if (!(ndp->cl = vmstrdup (
        Vmheap, (ap->value && ap->value[0]) ? ap->value : "black"
    ))) {
        SUwarning (0, "ITBinsertnd", "cannot copy node color");
        return NULL;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_FONTNAME];
    if (!(ndp->fn = vmstrdup (
        Vmheap, (ap->value && ap->value[0]) ? ap->value : "abc"
    ))) {
        SUwarning (0, "ITBinsertnd", "cannot copy node font name");
        return NULL;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_FONTSIZE];
    if (!(ndp->fs = vmstrdup (
        Vmheap, (ap->value && ap->value[0]) ? ap->value : "10"
    ))) {
        SUwarning (0, "ITBinsertnd", "cannot copy node font size");
        return NULL;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_LABEL];
    if (!(ndp->lab = vmstrdup (
        Vmheap, (ap->value && ap->value[0]) ? ap->value : ""
    ))) {
        SUwarning (0, "ITBinsertnd", "cannot copy node lab");
        return NULL;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_HDR];
    if (!(ndp->hdr = vmstrdup (
        Vmheap, (ap->value && ap->value[0]) ? ap->value : ""
    ))) {
        SUwarning (0, "ITBinsertnd", "cannot copy node hdr");
        return NULL;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_NODE][UT_ATTR_ROW];
    if (!(ndp->row = vmstrdup (
        Vmheap, (ap->value && ap->value[0]) ? ap->value : ""
    ))) {
        SUwarning (0, "ITBinsertnd", "cannot copy node row");
        return NULL;
    }
    if (UTsplitstr (ap->value) == -1) {
        SUwarning (0, "ITBinsertnd", "cannot parse node row");
        return NULL;
    }
    if (!(ndp->cells = vmalloc (Vmheap, UTtokm * sizeof (ITBcell_t)))) {
        SUwarning (0, "ITBinsertnd", "cannot allocate cells array");
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
            SUwarning (0, "ITBinsertnd", "cannot copy cell string");
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

    return ndp;
}

ITBnd_t *ITBfindnd (char *level, char *id) {
    ITBnd_t nd;

    memcpy (nd.level, level, SZ_level);
    memcpy (nd.id, id, SZ_id);
    return dtsearch (nddict, &nd);
}

ITBed_t *ITBinserted (ITBnd_t *ndp1, ITBnd_t *ndp2, char *attrstr) {
    ITBed_t ed, *edp, *edmem;
    UTattr_t *ap;
    char *tokp, jc, sc;
    int toki;

    ed.ndp1 = ndp1;
    ed.ndp2 = ndp2;
    if ((edp = dtsearch (eddict, &ed)))
        return edp;

    if (!(edmem = vmalloc (Vmheap, sizeof (ITBed_t)))) {
        SUwarning (0, "ITBinserted", "cannot allocate edmem");
        return NULL;
    }
    memset (edmem, 0, sizeof (ITBed_t));
    edmem->ndp1 = ndp1, edmem->ndp2 = ndp2;
    if ((edp = dtinsert (eddict, edmem)) != edmem) {
        SUwarning (0, "ITBinserted", "cannot insert ed");
        vmfree (Vmheap, edmem);
        return NULL;
    }

    if (UTsplitattr (UT_ATTRGROUP_EDGE, attrstr) == -1) {
        SUwarning (0, "ITBinserted", "cannot parse attrstr");
        return NULL;
    }
    if (UTgetattrs (UT_ATTRGROUP_EDGE) == -1) {
        SUwarning (0, "ITBinserted", "cannot get edge attr");
        return NULL;
    }

    if (UTsplitattr (UT_ATTRGROUP_EDGE, attrstr) == -1) {
        SUwarning (0, "ITBinserted", "cannot parse attrstr");
        return NULL;
    }
    if (UTgetattrs (UT_ATTRGROUP_EDGE) == -1) {
        SUwarning (0, "ITBinserted", "cannot get edge attr");
        return NULL;
    }

    ap = &UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_BGCOLOR];
    if (!(edp->bg = vmstrdup (
        Vmheap, (ap->value && ap->value[0]) ? ap->value : "white"
    ))) {
        SUwarning (0, "ITBinserted", "cannot copy edge bgcolor");
        return NULL;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_COLOR];
    if (!(edp->cl = vmstrdup (
        Vmheap, (ap->value && ap->value[0]) ? ap->value : "black"
    ))) {
        SUwarning (0, "ITBinserted", "cannot copy edge color");
        return NULL;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_FONTNAME];
    if (!(edp->fn = vmstrdup (
        Vmheap, (ap->value && ap->value[0]) ? ap->value : "abc"
    ))) {
        SUwarning (0, "ITBinserted", "cannot copy edge font name");
        return NULL;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_FONTSIZE];
    if (!(edp->fs = vmstrdup (
        Vmheap, (ap->value && ap->value[0]) ? ap->value : "10"
    ))) {
        SUwarning (0, "ITBinserted", "cannot copy edge font size");
        return NULL;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_LABEL];
    if (!(edp->lab = vmstrdup (
        Vmheap, (ap->value && ap->value[0]) ? ap->value : ""
    ))) {
        SUwarning (0, "ITBinserted", "cannot copy edge lab");
        return NULL;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_HDR];
    if (!(edp->hdr = vmstrdup (
        Vmheap, (ap->value && ap->value[0]) ? ap->value : ""
    ))) {
        SUwarning (0, "ITBinserted", "cannot copy edge hdr");
        return NULL;
    }
    ap = &UTattrgroups[UT_ATTRGROUP_EDGE][UT_ATTR_ROW];
    if (!(edp->row = vmstrdup (
        Vmheap, (ap->value && ap->value[0]) ? ap->value : ""
    ))) {
        SUwarning (0, "ITBinserted", "cannot copy edge row");
        return NULL;
    }
    if (UTsplitstr (ap->value) == -1) {
        SUwarning (0, "ITBinserted", "cannot parse edge row");
        return NULL;
    }
    if (!(edp->cells = vmalloc (Vmheap, UTtokm * sizeof (ITBcell_t)))) {
        SUwarning (0, "ITBinserted", "cannot allocate cells array");
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
            SUwarning (0, "ITBinserted", "cannot copy cell str");
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

    return edp;
}

int ITBflatten (char *sorder, char *surl) {
    ITBnd_t *ndp;
    int ndi;
    ITBed_t *edp;
    int edi;

    if (parseorder (sorder) == -1) {
        SUwarning (0, "ITBflatten", "cannot parse sort keys");
        return -1;
    }

    if (!(sorturl = vmstrdup (Vmheap, surl))) {
        SUwarning (0, "ITBflatten", "cannot copy sorturl");
        return -1;
    }

    ITBndn = dtsize (nddict);
    if (!(ITBnds = vmalloc (Vmheap, sizeof (ITBnd_t *) * ITBndn))) {
        SUwarning (0, "ITBflatten", "cannot allocate ITBnds array");
        return -1;
    }
    for (ndi = 0, ndp = dtfirst (nddict); ndp; ndp = dtnext (nddict, ndp))
        ITBnds[ndi++] = ndp;
    qsort (ITBnds, ITBndn, sizeof (ITBnd_t *), ndcmp);

    ITBedn = dtsize (eddict);
    if (!(ITBeds = vmalloc (Vmheap, sizeof (ITBed_t *) * ITBedn))) {
        SUwarning (0, "ITBflatten", "caenot allocate ITBeds array");
        return -1;
    }
    for (edi = 0, edp = dtfirst (eddict); edp; edp = dtnext (eddict, edp))
        ITBeds[edi++] = edp;
    qsort (ITBeds, ITBedn, sizeof (ITBed_t *), edcmp);

    return 0;
}

int ITBsetupprint (char *rendermode, char *itattr) {
    if (UTsplitattr (UT_ATTRGROUP_MAIN, itattr) == -1) {
        SUwarning (0, "ITBsetupprint", "cannot parse itattr");
        return -1;
    }
    if (UTgetattrs (UT_ATTRGROUP_MAIN) == -1) {
        SUwarning (0, "ITBsetupprint", "cannot get invtab attr");
        return -1;
    }

    return 0;
}

int ITBbeginnd (char *fprefix, int findex) {
    UTattr_t *ap;
    char *itbg, *itcl, *itfn, *itfs;

    if (!(rip = RIopen (RI_TYPE_TABLE, fprefix, findex, 0, 0))) {
        SUwarning (0, "ITBbeginnd", "cannot open table file");
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
        SUwarning (0, "ITBbeginnd", "cannot add table begin");
        return -1;
    }

    pageindex = pagei = 0;

    return 0;
}

int ITBendnd (void) {
    if (pagem > 0)
        RIaddtoc (rip, 123456789, NULL);

    if (UTaddtableend (rip) == -1) {
        SUwarning (0, "ITBendnd", "cannot add table end");
        return -1;
    }

    RIexecop (rip, NULL, 0, "black", "black", NULL, NULL, "Times-Roman", "10");

    if (RIclose (rip) == -1) {
        SUwarning (0, "ITBendnd", "cannot close table file");
        return -1;
    }

    return 0;
}

int ITBprintnd (int hdrflag, ITBnd_t *ndp, char *url, char *obj) {
    UTattr_t *ap;
    char *itbg, *itcl, *itfn, *itfs;
    EMbb_t bb;
    char *tokp, jc, *ustr, *tstr;
    int j;
    ITBcell_t *cellp;
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
            SUwarning (0, "ITBprintnd", "cannot add caption");
            return -1;
        }
        if (UTsplitstr (ndp->hdr) == -1) {
            SUwarning (0, "ITBprintnd", "cannot parse node header");
            return -1;
        }
        if (UTaddrowbegin (rip, itfn, itfs, itbg, itcl) == -1) {
            SUwarning (0, "ITBprintnd", "cannot add row begin");
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
                SUwarning (0, "ITBprintnd", "cannot add data cell begin");
                return -1;
            }
            if (sorturl && sorturl[0]) {
                if (UTaddlinkbegin (
                    rip, sfprints ("%s&invtab_sortorder=%d", sorturl, toki),
                    "sort table by this field",
                    itfn, itfs, itbg, itcl
                ) == -1) {
                    SUwarning (0, "ITBprintnd", "cannot add hdr link begin");
                    return -1;
                }
            }
            if (UTaddcelltext (rip, tokp) == -1) {
                SUwarning (0, "ITBprintnd", "cannot add hdr cell");
                return -1;
            }
            if (sorturl && sorturl[0]) {
                if (UTaddlinkend (rip) == -1) {
                    SUwarning (0, "ITBprintnd", "cannot add hdr link end");
                    return -1;
                }
            }
            if (UTaddcellend (rip) == -1) {
                SUwarning (0, "ITBprintnd", "cannot add data cell end");
                return -1;
            }
        }
        if (UTaddrowend (rip) == -1) {
            SUwarning (0, "ITBprintnd", "cannot add row end");
            return -1;
        }

        if (UTsplitstr (ndp->bg) == -1) {
            SUwarning (0, "ITBprintnd", "cannot parse node bgcolor");
            return -1;
        }
        if (UTtokn > bgn) {
            if (!(bgs = vmresize (
                Vmheap, bgs, UTtokn * sizeof (char *), VM_RSCOPY
            ))) {
                SUwarning (0, "ITBprintnd", "cannot grow bgs array");
                return -1;
            }
            bgn = UTtokn;
        }
        for (toki = bgm = 0; toki < UTtokm; toki++)
            bgs[bgm++] = UTtoks[toki];
        bgi = 0;
        if (UTsplitstr (ndp->cl) == -1) {
            SUwarning (0, "ITBprintnd", "cannot parse node color");
            return -1;
        }
        if (UTtokn > cln) {
            if (!(cls = vmresize (
                Vmheap, cls, UTtokn * sizeof (char *), VM_RSCOPY
            ))) {
                SUwarning (0, "ITBprintnd", "cannot grow cls array");
                return -1;
            }
            cln = UTtokn;
        }
        for (toki = clm = 0; toki < UTtokm; toki++)
            cls[clm++] = UTtoks[toki];
        cli = 0;
    }

    if (UTsplitstr (url) == -1) {
        SUwarning (0, "ITBprintnd", "cannot parse node urls");
        return -1;
    }
    if (pagem > 0 && pagei++ % pagem == 0)
        RIaddtoc (rip, ++pageindex, (ndp->celln > 0) ? ndp->cells[0].str : "");
    if (UTaddrowbegin (
        rip, ndp->fn, ndp->fs, bgs[bgi % bgm], cls[cli % clm]
    ) == -1) {
        SUwarning (0, "ITBprintnd", "cannot add row begin");
        return -1;
    }
    for (celli = 0; celli < ndp->celln; celli++) {
        cellp = &ndp->cells[celli];
        if (UTaddcellbegin (
            rip, ndp->fn, ndp->fs, bgs[bgi % bgm], cls[cli % clm], cellp->j, 1
        ) == -1) {
            SUwarning (0, "ITBprintnd", "cannot add data cell begin");
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
                rip, ustr, tstr,
                ndp->fn, ndp->fs, bgs[bgi % bgm], cls[cli % clm]
            ) == -1) {
                SUwarning (0, "ITBprintnd", "cannot add data link begin");
                return -1;
            }
        }
        if (strncmp (cellp->str, "EMBED:", 6) != 0) {
            if (UTaddcelltext (rip, cellp->str) == -1) {
                SUwarning (0, "ITBprintnd", "cannot add data cell");
                return -1;
            }
        } else {
            if (EMparsenode (
                ndp->level, ndp->id, cellp->str, EM_DIR_V, &opi, &opn, &bb
            ) == -1) {
                SUwarning (0, "ITBprintnd", "cannot parse node EM contents");
                return -1;
            }
            for (opj = 0; opj < opn; opj++) {
                if (RIaddop (rip, &EMops[opi + opj]) == -1) {
                    SUwarning (0, "ITBprintnd", "cannot add data cell op");
                    return -1;
                }
            }
        }
        if (celli < UTtokm && UTtoks[celli][0]) {
            if (UTaddlinkend (rip) == -1) {
                SUwarning (0, "ITBprintnd", "cannot add data link end");
                return -1;
            }
        }
        if (UTaddcellend (rip) == -1) {
            SUwarning (0, "ITBprintnd", "cannot add data cell end");
            return -1;
        }
    }
    if (UTaddrowend (rip) == -1) {
        SUwarning (0, "ITBprintnd", "cannot add row end");
        return -1;
    }
    bgi++, cli++;

    return 0;
}

int ITBbegined (char *fprefix, int findex) {
    UTattr_t *ap;
    char *itbg, *itcl, *itfn, *itfs;

    if (!(rip = RIopen (RI_TYPE_TABLE, fprefix, findex, 0, 0))) {
        SUwarning (0, "ITBbegined", "cannot open table file");
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
        SUwarning (0, "ITBbegined", "cannot add table begin");
        return -1;
    }

    pageindex = pagei = 0;

    return 0;
}

int ITBended (void) {
    if (pagem > 0)
        RIaddtoc (rip, 123456789, NULL);

    if (UTaddtableend (rip) == -1) {
        SUwarning (0, "ITBended", "cannot add table eed");
        return -1;
    }

    RIexecop (rip, NULL, 0, "black", "black", NULL, NULL, "Times-Roman", "10");

    if (RIclose (rip) == -1) {
        SUwarning (0, "ITBended", "cannot close table file");
        return -1;
    }

    return 0;
}

int ITBprinted (int hdrflag, ITBed_t *edp, char *url, char *obj) {
    UTattr_t *ap;
    char *itbg, *itcl, *itfn, *itfs;
    EMbb_t bb;
    char *tokp, jc, *ustr, *tstr;
    int j;
    ITBcell_t *cellp;
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
            SUwarning (0, "ITBprinted", "cannot add caption");
            return -1;
        }
        if (UTsplitstr (edp->hdr) == -1) {
            SUwarning (0, "ITBprinted", "cannot parse edge header");
            return -1;
        }
        if (UTaddrowbegin (rip, itfn, itfs, itbg, itcl) == -1) {
            SUwarning (0, "ITBprinted", "cannot add row begin");
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
                SUwarning (0, "ITBprinted", "cannot add data cell begin");
                return -1;
            }
            if (sorturl && sorturl[0]) {
                if (UTaddlinkbegin (
                    rip, sfprints ("%s&invtab_sortorder=%d", sorturl, toki),
                    "sort table by this field",
                    itfn, itfs, itbg, itcl
                ) == -1) {
                    SUwarning (0, "ITBprinted", "cannot add hdr link begin");
                    return -1;
                }
            }
            if (UTaddcelltext (rip, tokp) == -1) {
                SUwarning (0, "ITBprinted", "cannot add hdr cell");
                return -1;
            }
            if (sorturl && sorturl[0]) {
                if (UTaddlinkend (rip) == -1) {
                    SUwarning (0, "ITBprinted", "cannot add hdr link end");
                    return -1;
                }
            }
            if (UTaddcellend (rip) == -1) {
                SUwarning (0, "ITBprinted", "cannot add data cell end");
                return -1;
            }
        }
        if (UTaddrowend (rip) == -1) {
            SUwarning (0, "ITBprinted", "cannot add row end");
            return -1;
        }

        if (UTsplitstr (edp->bg) == -1) {
            SUwarning (0, "ITBprinted", "cannot parse edge bgcolor");
            return -1;
        }
        if (UTtokn > bgn) {
            if (!(bgs = vmresize (
                Vmheap, bgs, UTtokn * sizeof (char *), VM_RSCOPY
            ))) {
                SUwarning (0, "ITBprinted", "cannot grow ops array");
                return -1;
            }
            bgn = UTtokn;
        }
        for (toki = bgm = 0; toki < UTtokm; toki++)
            bgs[bgm++] = UTtoks[toki];
        bgi = 0;
        if (UTsplitstr (edp->cl) == -1) {
            SUwarning (0, "ITBprinted", "cannot parse edge color");
            return -1;
        }
        if (UTtokn > cln) {
            if (!(cls = vmresize (
                Vmheap, cls, UTtokn * sizeof (char *), VM_RSCOPY
            ))) {
                SUwarning (0, "ITBprinted", "cannot grow ops array");
                return -1;
            }
            cln = UTtokn;
        }
        for (toki = clm = 0; toki < UTtokm; toki++)
            cls[clm++] = UTtoks[toki];
        cli = 0;
    }

    if (UTsplitstr (url) == -1) {
        SUwarning (0, "ITBprintnd", "cannot parse edge urls");
        return -1;
    }
    if (pagem > 0 && pagei++ % pagem == 0)
        RIaddtoc (rip, ++pageindex, (edp->celln > 0) ? edp->cells[0].str : "");
    if (UTaddrowbegin (
        rip, edp->fn, edp->fs, bgs[bgi % bgm], cls[cli % clm]
    ) == -1) {
        SUwarning (0, "ITBprinted", "cannot add row begin");
        return -1;
    }
    for (celli = 0; celli < edp->celln; celli++) {
        cellp = &edp->cells[celli];
        if (UTaddcellbegin (
            rip, edp->fn, edp->fs, bgs[bgi % bgm], cls[cli % clm], cellp->j, 1
        ) == -1) {
            SUwarning (0, "ITBprinted", "cannot add data cell begin");
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
                rip, ustr, tstr,
                edp->fn, edp->fs, bgs[bgi % bgm], cls[cli % clm]
            ) == -1) {
                SUwarning (0, "ITBprinted", "cannot add data link begin");
                return -1;
            }
        }
        if (strncmp (cellp->str, "EMBED:", 6) == 0) {
            if (UTaddcelltext (rip, cellp->str) == -1) {
                SUwarning (0, "ITBprinted", "cannot add data cell");
                return -1;
            }
        } else {
            if (EMparseedge (
                edp->ndp1->level, edp->ndp1->id,
                edp->ndp2->level, edp->ndp2->id,
                cellp->str, EM_DIR_V, &opi, &opn, &bb
            ) == -1) {
                SUwarning (0, "ITBprinted", "cannot parse edge EM contents");
                return -1;
            }
            for (opj = 0; opi < opn; opj++) {
                if (RIaddop (rip, &EMops[opi + opj]) == -1) {
                    SUwarning (0, "ITBprinted", "cannot add data cell op");
                    return -1;
                }
            }
        }
        if (celli < UTtokm && UTtoks[celli][0]) {
            if (UTaddlinkend (rip) == -1) {
                SUwarning (0, "ITBprinted", "cannot add data link end");
                return -1;
            }
        }
        if (UTaddcellend (rip) == -1) {
            SUwarning (0, "ITBprinted", "cannot add data cell end");
            return -1;
        }
    }
    if (UTaddrowend (rip) == -1) {
        SUwarning (0, "ITBprinted", "cannot add row end");
        return -1;
    }
    bgi++, cli++;

    return 0;
}

static int parseorder (char *str) {
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
            SUwarning (0, "parseorder", "unknown key: %s", s1);
            continue;
        }

        if (!*s1 || *s1 == 'f')
            dir = DIR_FWD;
        else if (*s1 == 'r')
            dir = DIR_REV;
        else {
            SUwarning (0, "parseorder", "unknown key: %s", s1);
            continue;
        }

        if (sortkeym >= sortkeyn) {
            if (!(sortkeys = vmresize (
                Vmheap, sortkeys, (sortkeyn + 10) * sizeof (sortkey_t),
                VM_RSCOPY
            ))) {
                SUwarning (0, "parseorder", "cannot grow sortkeys array");
                return -1;
            }
            sortkeyn += 10;
        }
        skp = &sortkeys[sortkeym++];
        skp->key = key, skp->ci = ci; skp->dir = dir;
    }

    return 0;
}

static int ndcmp (const void *a, const void *b) {
    ITBnd_t *ap, *bp;
    sortkey_t *skp;
    int ski, sa;
    ITBcell_t *cpa, *cpb;
    int ci;
    int ret;
    float fret;

    ap = *(ITBnd_t **) a;
    bp = *(ITBnd_t **) b;

    if ((ret = ap->nclass - bp->nclass) != 0)
        return ret;
    if (sortkeym == 0) {
        if ((ret = strcmp (ap->level, bp->level)) != 0)
            return ret;
        if ((ret = strcmp (ap->id, bp->id)) != 0)
            return ret;
    } else {
        for (ski = 0; ski < sortkeym; ski++) {
            skp = &sortkeys[ski];
            switch (skp->key) {
            case KEY_IDENT:
                if ((ret = strcmp (ap->level, bp->level)) != 0)
                    return (skp->dir == DIR_FWD ? ret : -ret);
                if ((ret = strcmp (ap->id, bp->id)) != 0)
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
    ITBed_t *ap, *bp;
    sortkey_t *skp;
    int ski, sa;
    ITBcell_t *cpa, *cpb;
    int ci;
    int ret;
    float fret;

    ap = *(ITBed_t **) a;
    bp = *(ITBed_t **) b;

    if ((ret = ap->ndp1->nclass - bp->ndp1->nclass) != 0)
        return ret;
    if ((ret = ap->ndp2->nclass - bp->ndp2->nclass) != 0)
        return ret;
    if (sortkeym == 0) {
        if ((ret = strcmp (ap->ndp1->level, bp->ndp1->level)) != 0)
            return ret;
        if ((ret = strcmp (ap->ndp1->id, bp->ndp1->id)) != 0)
            return ret;
        if ((ret = strcmp (ap->ndp2->level, bp->ndp2->level)) != 0)
            return ret;
        if ((ret = strcmp (ap->ndp2->id, bp->ndp2->id)) != 0)
            return ret;
    } else {
        for (ski = 0; ski < sortkeym; ski++) {
            skp = &sortkeys[ski];
            switch (skp->key) {
            case KEY_IDENT:
                if ((ret = strcmp (ap->ndp1->level, bp->ndp1->level)) != 0)
                    return (skp->dir == DIR_FWD ? ret : -ret);
                if ((ret = strcmp (ap->ndp1->id, bp->ndp1->id)) != 0)
                    return (skp->dir == DIR_FWD ? ret : -ret);
                if ((ret = strcmp (ap->ndp2->level, bp->ndp2->level)) != 0)
                    return (skp->dir == DIR_FWD ? ret : -ret);
                if ((ret = strcmp (ap->ndp2->id, bp->ndp2->id)) != 0)
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
