#include <ast.h>
#include <ast_float.h>
#include <tok.h>
#include <swift.h>
#include <ctype.h>
#include "vg_dq_vt_util.h"

#define MAXPIX 10 * 1024 * 1024

AGDalarm_t **AGDalarms;
int AGDalarmn;

static int alarmgridsperimage, width, height;

static char *pagemode;
static int pageindex, pagei, pagem, pageemit;
static char pagebuf[1000];

static Dt_t *alarmdict;
static Dtdisc_t alarmdisc = {
    sizeof (Dtlink_t), 2 * (SZ_level + SZ_id),
    0, NULL, NULL, NULL, NULL, NULL, NULL
};

static int tmodemap[VG_ALARM_N_MODE_MAX + 1] = {
    AGD_TMODE_NTICKET,
    AGD_TMODE_YTICKET,
    AGD_TMODE_DTICKET,
    AGD_TMODE_NTICKET,
};
static char *tmodenames[AGD_TMODE_SIZE] = {
    "ticketed", "deferred", "filtered"
};
static int tmodetrans[AGD_TMODE_SIZE] = {
    0, 1, 2
};
static int tmodeyoffs[AGD_TMODE_SIZE];

static AGDsevattr_t *sevattrs;
static int sevn, clearsevi;

static int openmode;

static RI_t *rip;

static int sortcmp (const void *, const void *);

int AGDinit (int sn, int mode, char *pm) {

    if (!(alarmdict = dtopen (&alarmdisc, Dtset))) {
        SUwarning (0, "AGDinit", "cannot create alarmdict");
        return -1;
    }
    AGDalarms = NULL;
    AGDalarmn = 0;

    sevn = sn + 1, clearsevi = sn;
    openmode = mode;

    pagemode = pm;
    pageindex = pagei = pagem = 0;
    if (pagemode && pagemode[0] && strcmp (pagemode, "all") != 0)
        pagem = atoi (pagemode);
    if (pagem < 0)
        pagem = 0;

    UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_BGCOLOR].name = "bgcolor";
    UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_COLOR].name = "color";
    UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_FLAGS].name = "flags";
    UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_WIDTH].name = "width";
    UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_HEIGHT].name = "height";

    UTattrgroups[UT_ATTRGROUP_TITLE][UT_ATTR_LABEL].name = "label";
    UTattrgroups[UT_ATTRGROUP_TITLE][UT_ATTR_COLOR].name = "color";
    UTattrgroups[UT_ATTRGROUP_TITLE][UT_ATTR_FONTNAME].name = "fontname";
    UTattrgroups[UT_ATTRGROUP_TITLE][UT_ATTR_FONTSIZE].name = "fontsize";
    UTattrgroups[UT_ATTRGROUP_TITLE][UT_ATTR_INFO].name = "info";

    UTattrgroups[UT_ATTRGROUP_ALARM][UT_ATTR_FLAGS].name = "flags";
    UTattrgroups[UT_ATTRGROUP_ALARM][UT_ATTR_COLOR].name = "color";
    UTattrgroups[UT_ATTRGROUP_ALARM][UT_ATTR_LINEWIDTH].name = "linewidth";
    UTattrgroups[UT_ATTRGROUP_ALARM][UT_ATTR_FONTNAME].name = "fontname";
    UTattrgroups[UT_ATTRGROUP_ALARM][UT_ATTR_FONTSIZE].name = "fontsize";

    UTattrgroups[UT_ATTRGROUP_XAXIS][UT_ATTR_FLAGS].name = "flags";
    UTattrgroups[UT_ATTRGROUP_XAXIS][UT_ATTR_LABEL].name = "label";
    UTattrgroups[UT_ATTRGROUP_XAXIS][UT_ATTR_COLOR].name = "color";
    UTattrgroups[UT_ATTRGROUP_XAXIS][UT_ATTR_FONTNAME].name = "fontname";
    UTattrgroups[UT_ATTRGROUP_XAXIS][UT_ATTR_FONTSIZE].name = "fontsize";

    UTattrgroups[UT_ATTRGROUP_YAXIS][UT_ATTR_FLAGS].name = "flags";
    UTattrgroups[UT_ATTRGROUP_YAXIS][UT_ATTR_LABEL].name = "label";
    UTattrgroups[UT_ATTRGROUP_YAXIS][UT_ATTR_COLOR].name = "color";
    UTattrgroups[UT_ATTRGROUP_YAXIS][UT_ATTR_FONTNAME].name = "fontname";
    UTattrgroups[UT_ATTRGROUP_YAXIS][UT_ATTR_FONTSIZE].name = "fontsize";

    return 0;
}

AGDalarm_t *AGDinsert (
    char *ilevel1, char *iid1, char *ilevel2, char *iid2,
    char *level1, char *id1, char *level2, char *id2,
    int type, int tmode, int sevi
) {
    AGDalarm_t al, *alp, *almem;
    int tmi;

    memcpy (al.ilevel1, ilevel1, SZ_level);
    memcpy (al.iid1, iid1, SZ_id);
    memcpy (al.ilevel2, ilevel2, SZ_level);
    memcpy (al.iid2, iid2, SZ_id);

    if (!(alp = dtsearch (alarmdict, &al))) {
        if (!(almem = vmalloc (Vmheap, sizeof (AGDalarm_t)))) {
            SUwarning (0, "AGDinsert", "cannot allocate almem");
            return NULL;
        }
        memset (almem, 0, sizeof (AGDalarm_t));
        memcpy (almem->ilevel1, ilevel1, SZ_level);
        memcpy (almem->iid1, iid1, SZ_id);
        memcpy (almem->ilevel2, ilevel2, SZ_level);
        memcpy (almem->iid2, iid2, SZ_id);
        if ((alp = dtinsert (alarmdict, almem)) != almem) {
            SUwarning (0, "AGDinsert", "cannot insert alarm");
            vmfree (Vmheap, almem);
            return NULL;
        }
        for (tmi = 0; tmi < AGD_TMODE_SIZE; tmi++) {
            if (!(alp->cns[tmi] = vmalloc (Vmheap, sevn * sizeof (int)))) {
                SUwarning (0, "AGDinsert", "cannot allocate cns array");
                return NULL;
            }
            memset (alp->cns[tmi], 0, sevn * sizeof (int));
            if (!(alp->havecns[tmi] = vmalloc (Vmheap, sevn * sizeof (int)))) {
                SUwarning (0, "AGDinsert", "cannot allocate havecns array");
                return NULL;
            }
        }
    }
    if (tmode == -123 && sevi == -123)
        return alp; // called to create blank default grid

    if (sevi < 0 || sevi >= sevn) {
        SUwarning (
            0, "AGDinsert", "sevi: %d out of range: %d-%d", sevi, 0, sevn
        );
        return NULL;
    }
    if (tmode < 0 || tmode > VG_ALARM_N_MODE_MAX) {
        SUwarning (
            0, "AGDinsert", "tmode: %d out of range: %d-%d",
            tmode, 0, VG_ALARM_N_MODE_MAX
        );
        return NULL;
    }
    tmi = tmodemap[tmode];
    if (type == VG_ALARM_N_TYPE_CLEAR)
        sevi = clearsevi;
    alp->cns[tmi][sevi]++;
    alp->havecns[tmi][sevi]++;
    alp->havetms[tmi] = TRUE;

    return alp;
}

int AGDflatten (void) {
    AGDalarm_t *alp;
    int ali;

    if ((AGDalarmn = dtsize (alarmdict)) == 0)
        return 0;

    if (!(AGDalarms = vmalloc (Vmheap, sizeof (AGDalarm_t *) * AGDalarmn))) {
        SUwarning (0, "AGDflatten", "cannot allocate alarms array");
        return -1;
    }
    for (
        ali = 0, alp = dtfirst (alarmdict); alp;
        alp = dtnext (alarmdict, alp)
    )
        AGDalarms[ali++] = alp;
    qsort (AGDalarms, AGDalarmn, sizeof (AGDalarm_t *), sortcmp);

    return 0;
}

int AGDsetupdraw (
    char *rendermode, char *sgattr, char *alattr, char *xaattr, char *yaattr
) {
    UTattr_t *ap;
    int sevi, sevattrk, sevattrl;
    int tmi, tmk, tml, ftmi, ttmi;
    char *labelstr, *colorstr, *sgflags, *sgstr, *s1, *s2, *s3, *s4, *s5;
    int imagen;
    void *tokp;

    if (AGDalarmn == 0)
        return 0;

    if (UTsplitattr (UT_ATTRGROUP_MAIN, sgattr) == -1) {
        SUwarning (0, "AGDsetupdraw", "cannot parse sgattr");
        return -1;
    }
    if (UTgetattrs (UT_ATTRGROUP_MAIN) == -1) {
        SUwarning (0, "AGDsetupdraw", "cannot get alarmgrid attr");
        return -1;
    }
    if (UTsplitattr (UT_ATTRGROUP_ALARM, alattr) == -1) {
        SUwarning (0, "AGDsetupdraw", "cannot parse alattr");
        return -1;
    }
    if (UTgetattrs (UT_ATTRGROUP_ALARM) == -1) {
        SUwarning (0, "AGDsetupdraw", "cannot get alarm attr");
        return -1;
    }
    if (UTsplitattr (UT_ATTRGROUP_XAXIS, xaattr) == -1) {
        SUwarning (0, "AGDsetupdraw", "cannot parse xaattr");
        return -1;
    }
    if (UTgetattrs (UT_ATTRGROUP_XAXIS) == -1) {
        SUwarning (0, "AGDsetupdraw", "cannot get xaxis attr");
        return -1;
    }
    if (UTsplitattr (UT_ATTRGROUP_YAXIS, yaattr) == -1) {
        SUwarning (0, "AGDsetupdraw", "cannot parse yaattr");
        return -1;
    }
    if (UTgetattrs (UT_ATTRGROUP_YAXIS) == -1) {
        SUwarning (0, "AGDsetupdraw", "cannot get yaxis attr");
        return -1;
    }

    ap = &UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_WIDTH];
    width = (ap->value && ap->value[0]) ? atoi (ap->value) : 400;
    ap = &UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_HEIGHT];
    height = (ap->value && ap->value[0]) ? atoi (ap->value) : 100;
    ap = &UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_FLAGS];
    sgflags = (ap->value && ap->value[0]) ? ap->value : "";
    tokp = tokopen (sgflags, 1);
    while ((sgstr = tokread (tokp))) {
        if (strncmp (sgstr, "map_", 4) == 0) {
            s1 = sgstr + 4;
            if (!(s2 = strchr (s1, '_')))
                continue;
            *s2++ = 0;
            if (strcmp (s1, "ticketed") == 0)
                ftmi = AGD_TMODE_YTICKET;
            else if (strcmp (s1, "deferred") == 0)
                ftmi = AGD_TMODE_DTICKET;
            else if (strcmp (s1, "filtered") == 0)
                ftmi = AGD_TMODE_NTICKET;
            else
                continue;
            if (strcmp (s2, "ticketed") == 0)
                ttmi = AGD_TMODE_YTICKET;
            else if (strcmp (s2, "deferred") == 0)
                ttmi = AGD_TMODE_DTICKET;
            else if (strcmp (s2, "filtered") == 0)
                ttmi = AGD_TMODE_NTICKET;
            else
                continue;
            tmodetrans[ftmi] = ttmi;
        } else if (strncmp (sgstr, "hide_", 5) == 0) {
            s1 = sgstr + 5;
            if (strcmp (s1, "ticketed") == 0)
                ftmi = AGD_TMODE_YTICKET;
            else if (strcmp (s1, "deferred") == 0)
                ftmi = AGD_TMODE_DTICKET;
            else if (strcmp (s1, "filtered") == 0)
                ftmi = AGD_TMODE_NTICKET;
            else
                continue;
            tmodetrans[ftmi] = -1;
        }
    }
    tokclose (tokp);

    if (!(sevattrs = vmalloc (Vmheap, sizeof (AGDsevattr_t) * sevn))) {
        SUwarning (0, "AGDsetupdraw", "cannot allocate sevattrs");
        return -1;
    }
    memset (sevattrs, 0, sizeof (AGDsevattr_t) * sevn);
    ap = &UTattrgroups[UT_ATTRGROUP_XAXIS][UT_ATTR_LABEL];
    labelstr = (ap->value) ? ap->value : NULL;
    for (s1 = labelstr; s1 && *s1; s1 = s2) {
        if ((s2 = strchr (s1, '|')))
            *s2++ = 0;
        if (!(s3 = strchr (s1, ':'))) {
            SUwarning (0, "AGDsetupdraw", "bad sevname label: %s", s1);
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
            SUwarning (0, "AGDsetupdraw", "cannot copy label: %s", s3);
            break;
        }
    }
    ap = &UTattrgroups[UT_ATTRGROUP_YAXIS][UT_ATTR_LABEL];
    labelstr = (ap->value && ap->value[0]) ? ap->value : NULL;
    for (s1 = labelstr; s1; s1 = s2) {
        if ((s2 = strchr (s1, '|')))
            *s2++ = 0;
        if (!(s3 = strchr (s1, ':'))) {
            SUwarning (0, "AGDsetupdraw", "bad ticketmode label: %s", s1);
            break;
        }
        *s3++ = 0;
        tmi = -1;
        if (isdigit (*s1))
            tmi = atoi (s1) - 1;
        if (tmi < 0 || tmi >= AGD_TMODE_SIZE)
            continue;
        if (!(tmodenames[tmi] = vmstrdup (Vmheap, s3))) {
            SUwarning (0, "AGDsetupdraw", "cannot copy label: %s", s3);
            break;
        }
    }
    ap = &UTattrgroups[UT_ATTRGROUP_ALARM][UT_ATTR_COLOR];
    colorstr = (ap->value) ? ap->value : "1";
    for (s1 = colorstr; s1; s1 = s2) {
        if ((s2 = strchr (s1, '|')))
            *s2++ = 0;
        if (
            !(s3 = strchr (s1, ':')) || !(s4 = strchr (s3 + 1, ':')) ||
            !(s5 = strchr (s4 + 1, ':'))
        ) {
            SUwarning (0, "AGDsetupdraw", "bad sevname label: %s", s1);
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
            tmk = tml = AGD_TMODE_YTICKET;
        else if (strcmp (s3, "deferred") == 0)
            tmk = tml = AGD_TMODE_DTICKET;
        else if (strcmp (s3, "filtered") == 0)
            tmk = tml = AGD_TMODE_NTICKET;
        else if (strcmp (s3, "all") == 0)
            tmk = 0, tml = AGD_TMODE_SIZE - 1;
        if (tmk < 0 || tmk >= AGD_TMODE_SIZE)
            continue;
        for (sevi = sevattrk; sevi <= sevattrl; sevi++) {
            for (tmi = tmk; tmi <= tml; tmi++) {
                if (!(sevattrs[sevi].bg[tmi] = vmstrdup (Vmheap, s4))) {
                    SUwarning (0, "AGDsetupdraw", "cannot copy bg: %s", s4);
                    break;
                }
                if (!(sevattrs[sevi].cl[tmi] = vmstrdup (Vmheap, s5))) {
                    SUwarning (0, "AGDsetupdraw", "cannot copy color: %s", s5);
                    break;
                }
            }
        }
    }

    if (strcmp (rendermode, "embed") == 0) {
        if ((alarmgridsperimage = MAXPIX / (width * height)) < 1)
            alarmgridsperimage = 1;
        else if (alarmgridsperimage > AGDalarmn)
            alarmgridsperimage = AGDalarmn;
        imagen = (AGDalarmn + alarmgridsperimage - 1) / alarmgridsperimage;
        alarmgridsperimage = (AGDalarmn + imagen - 1) / imagen;
    } else
        alarmgridsperimage = 1;

    return 0;
}

int AGDfinishdraw (char *fprefix) {
    Sfio_t *fp;
    int sevi, tmi;

    if (!(fp = sfopen (NULL, sfprints ("%s.info", fprefix), "w"))) {
        SUwarning (0, "AGDfinishdraw", "cannot open info file");
        return -1;
    }
    sfprintf (
        fp,
        "<table class=sdiv>"
        "<caption class=sdiv style='font-size:80%%'>color map</caption>\n"
    );
    for (tmi = 0; tmi <= AGD_TMODE_SIZE; tmi++) {
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
        SUwarning (0, "AGDfinishdraw", "cannot cloas info file");
        return -1;
    }
    return 0;
}

int AGDdrawgrid (
    char *fprefix, int findex, int ali, char *tlattr,
    char *sgurl, char *tlurl, char *xaurl, char *yaurl, char *obj
) {
    AGDalarm_t *alp;
    int xoff, yoff, xsiz, ysiz;
    int tlxoff, tlyoff, tlxsiz, tlysiz;
    int xaxoff, xayoff, xaxsiz, xaysiz;
    int yaxoff, yayoff, yaxsiz, yaysiz;
    int sgxoff, sgyoff, sgxsiz, sgysiz;
    UTattr_t *ap;
    char *sgbg, *sgcl;
    char *tllab, *tlcl, *tlfn, *tlfs, *tlinfo;
    int tlshowtitle;
    char *alflags, *alfn, *alfs;
    int alw, alh, allw, alx1, alx2, aly1, aly2;
    int alshowoutline, alshowfilled, alshowtext, fflag;
    char *xaflags, *xacl, *xafn, *xafs;
    int xashowvals;
    char *yaflags, *yamaxstr, *yacl, *yafn, *yafs;
    int yalen, yamaxlen, yashowvals;
    int sum, tmi, tmj, tmn, sevi, sevm;
    char countbuf[1024];
    RIop_t top;
    RIarea_t a;
    int w, h;

    sevm = sevn - (openmode ? 1 : 0);
    if (sevm < 2)
        return 0;

    alp = AGDalarms[ali];

    if (ali % alarmgridsperimage == 0) {
        if (!(rip = RIopen (
            RI_TYPE_IMAGE, fprefix, findex, width, alarmgridsperimage * height
        ))) {
            SUwarning (0, "AGDdrawgrid", "cannot open image #%d", findex);
            return -1;
        }
    }

    pageemit = 0;
    if (pagem > 0 && pagei++ % pagem == 0) {
        pageemit = 1;
        sfsprintf (pagebuf, 1000, "image %d", pagei);
    }

    xoff = 0, yoff = (ali % alarmgridsperimage) * height;
    xsiz = width, ysiz = height;

    tlxoff = xoff, tlyoff = yoff, tlxsiz = xsiz, tlysiz = 0;
    sgxoff = xoff, sgyoff = tlyoff + tlysiz, sgxsiz = xsiz, sgysiz = ysiz;
    xaxoff = xoff, xayoff = sgyoff + sgysiz, xaxsiz = xsiz, xaysiz = 0;
    yaxoff = xoff, yayoff = tlyoff + tlysiz, yaxsiz = 0, yaysiz = ysiz;

    tlshowtitle = FALSE;
    alshowoutline = alshowfilled = alshowtext = FALSE;
    xashowvals = FALSE;
    yashowvals = FALSE;

    RIaddop (rip, NULL); // reset

    if (UTsplitattr (UT_ATTRGROUP_TITLE, tlattr) == -1) {
        SUwarning (0, "AGDdrawgrid", "cannot parse tlattr");
        return -1;
    }
    if (UTgetattrs (UT_ATTRGROUP_TITLE) == -1) {
        SUwarning (0, "AGDdrawgrid", "cannot get title attr");
        return -1;
    }

    ap = &UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_BGCOLOR];
    if (ap->value) {
        sgbg = ap->value;
        if (UTaddbox (
            rip, xoff, yoff, xoff + xsiz, yoff + ysiz, sgbg, TRUE
        ) == -1) {
            SUwarning (0, "AGDdrawgrid", "cannot add sgbg box");
            return -1;
        }
    }
    ap = &UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_COLOR];
    sgcl = (ap->value && ap->value[0]) ? ap->value : "black";

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
            rip, tllab, tlfn, tlfs, tlcl,
            tlxoff, tlyoff, tlxoff + tlxsiz, tlyoff, 0, -1, FALSE, &w, &h
        ) == -1) {
            SUwarning (0, "AGDdrawgrid", "cannot add title text");
            return -1;
        }
        tlysiz += h, sgyoff += h, sgysiz -= h, yayoff += h, yaysiz -= h;
        tlshowtitle = TRUE;
        if (pageemit)
            sfsprintf (pagebuf, 1000, "%s", tllab);
    }
    ap = &UTattrgroups[UT_ATTRGROUP_TITLE][UT_ATTR_INFO];
    tlinfo = ap->value;

    if (pageemit)
        RIaddtoc (rip, ++pageindex, pagebuf);

    ap = &UTattrgroups[UT_ATTRGROUP_ALARM][UT_ATTR_LINEWIDTH];
    allw = (ap->value && ap->value[0]) ? atoi (ap->value) : 1;
    ap = &UTattrgroups[UT_ATTRGROUP_ALARM][UT_ATTR_FLAGS];
    if (ap->value && ap->value[0]) {
        alflags = ap->value;
        if (strstr (alflags, "outline"))
            alshowoutline = TRUE;
        if (strstr (alflags, "filled"))
            alshowfilled = TRUE;
        if (strstr (alflags, "text"))
            alshowtext = TRUE;
        if (alshowoutline || alshowfilled || alshowtext) {
            ap = &UTattrgroups[UT_ATTRGROUP_ALARM][UT_ATTR_FONTNAME];
            alfn = (ap->value) ? ap->value : "Times-Roman";
            ap = &UTattrgroups[UT_ATTRGROUP_ALARM][UT_ATTR_FONTSIZE];
            alfs = (ap->value) ? ap->value : "8";
        }
    }

    ap = &UTattrgroups[UT_ATTRGROUP_XAXIS][UT_ATTR_FLAGS];
    if (ap->value && ap->value[0]) {
        xaflags = ap->value;
        if (strstr (xaflags, "values"))
            xashowvals = TRUE;
        if (xashowvals) {
            ap = &UTattrgroups[UT_ATTRGROUP_XAXIS][UT_ATTR_COLOR];
            xacl = (ap->value) ? ap->value : "black";
            ap = &UTattrgroups[UT_ATTRGROUP_XAXIS][UT_ATTR_FONTNAME];
            xafn = (ap->value) ? ap->value : "Times-Roman";
            ap = &UTattrgroups[UT_ATTRGROUP_XAXIS][UT_ATTR_FONTSIZE];
            xafs = (ap->value) ? ap->value : "8";
            if (RIgettextsize ("W", xafn, xafs, &top) == -1) {
                SUwarning (0, "AGDdrawgrid", "cannot get xaxis text size");
                return -1;
            }
            h = top.u.font.h;
            xayoff -= h, xaysiz = h, sgysiz -= h, yaysiz -= h;
        }
    }

    ap = &UTattrgroups[UT_ATTRGROUP_YAXIS][UT_ATTR_FLAGS];
    if (ap->value && ap->value[0]) {
        yaflags = ap->value;
        if (strstr (yaflags, "values"))
            yashowvals = TRUE;
        if (yashowvals) {
            ap = &UTattrgroups[UT_ATTRGROUP_YAXIS][UT_ATTR_COLOR];
            yacl = (ap->value) ? ap->value : "black";
            ap = &UTattrgroups[UT_ATTRGROUP_YAXIS][UT_ATTR_FONTNAME];
            yafn = (ap->value) ? ap->value : "Times-Roman";
            ap = &UTattrgroups[UT_ATTRGROUP_YAXIS][UT_ATTR_FONTSIZE];
            yafs = (ap->value) ? ap->value : "8";
            yamaxlen = 0, yamaxstr = "";
            for (tmi = 0; tmi < AGD_TMODE_SIZE; tmi++) {
                if (tmodetrans[tmi] != tmi)
                    continue;
                yalen = strlen (tmodenames[tmi]);
                if (yamaxlen < yalen)
                    yamaxlen = yalen, yamaxstr = tmodenames[tmi];
            }
            if (RIgettextsize (yamaxstr, yafn, yafs, &top) == -1) {
                SUwarning (0, "AGDdrawgrid", "cannot get yaxis text size");
                return -1;
            }
            w = top.u.font.w + 1;
            yaxsiz = w, sgxoff += w, sgxsiz -= w, xaxoff += w, xaxsiz -= w;
        }
    }

    sgxoff += 1, sgxsiz -= 1, xaxoff += 1, xaxsiz -= 1;

    alw = (double) sgxsiz / (sevm - 1);
    if (xashowvals) {
        for (sevi = 1; sevi < sevm; sevi++) {
            sevattrs[sevi].x = xaxoff + alw * (0.5 + sevi - 1);
            if (UTaddtext (
                rip, sevattrs[sevi].name, xafn, xafs, xacl,
                sevattrs[sevi].x - alw / 2.0, xayoff,
                sevattrs[sevi].x + alw / 2.0, xayoff, 0, -1, FALSE, &w, &h
            ) == -1) {
                SUwarning (0, "AGDdrawgrid", "cannot add xaxis text");
                return -1;
            }
        }
    } else {
        for (sevi = 1; sevi < sevm; sevi++)
            sevattrs[sevi].x = xaxoff + alw * (0.5 + sevi - 1);
    }

    for (tmi = tmn = 0; tmi < AGD_TMODE_SIZE; tmi++) {
        if (tmodetrans[tmi] != tmi)
            continue;
        tmn++;
    }

    alh = (double) sgysiz / tmn;
    if (yashowvals) {
        for (tmi = tmj = 0; tmi < AGD_TMODE_SIZE; tmi++) {
            if (tmodetrans[tmi] != tmi)
                continue;
            tmodeyoffs[tmi] = yayoff + yaysiz - alh * (0.5 + tmn - tmj - 1);
            if (UTaddtext (
                rip, tmodenames[tmi], yafn, yafs, yacl,
                yaxoff, tmodeyoffs[tmi] - alh / 2.0,
                yaxoff + yaxsiz, tmodeyoffs[tmi] + alh / 2.0, 1, 0, FALSE,
                &w, &h
            ) == -1) {
                SUwarning (0, "AGDdrawgrid", "cannot add yaxis text");
                return -1;
            }
            tmj++;
        }
    } else {
        for (tmi = tmj = 0; tmi < AGD_TMODE_SIZE; tmi++) {
            if (tmodetrans[tmi] != tmi)
                continue;
            tmodeyoffs[tmi] = yayoff + yaysiz - alh * (0.5 + tmn - tmj - 1);
            tmj++;
        }
    }

    for (tmi = 0; tmi < AGD_TMODE_SIZE; tmi++) {
        if (tmodetrans[tmi] != tmi)
            continue;
        for (sevi = 1; sevi < sevm; sevi++) {
            sevattrs[sevi].used[tmi]++;
            sum = 0;
            for (tmj = 0; tmj < AGD_TMODE_SIZE; tmj++) {
                if (tmodetrans[tmj] != tmi)
                    continue;
                sum += alp->cns[tmj][sevi];
            }
            fflag = FALSE;
            if (alshowfilled || alshowoutline || alshowtext) {
                alx1 = sevattrs[sevi].x - alw / 2.0;
                aly1 = tmodeyoffs[tmi] - alh / 2.0;
                alx2 = alx1 + alw;
                aly2 = aly1 + alh;
                if (alx1 < xoff)
                    alx1 = xoff;
                if (aly1 < yoff)
                    aly1 = yoff;
                if (alx2 > xoff + xsiz)
                    alx2 = xoff + xsiz;
                if (aly2 > yoff + ysiz)
                    aly2 = yoff + ysiz;
            }
            if (alshowfilled || alshowoutline) {
                if (UTaddlw (rip, allw) == -1) {
                    SUwarning (0, "AGDdrawgrid", "cannot add line width");
                    return -1;
                }
                if (alshowfilled && alshowoutline)
                    fflag = (sum > 0) ? TRUE : FALSE;
                else
                    fflag = alshowfilled;
                if (UTaddbox (
                    rip, alx1, aly1, alx2 - 1, aly2 - 1,
                    sevattrs[sevi].bg[tmi], fflag
                ) == -1) {
                    SUwarning (0, "AGDdrawgrid", "cannot add alarm box");
                    return -1;
                }
            }
            if (alshowtext) {
                sfsprintf (countbuf, 1024, "%d", sum);
                RImaxtsflag = FALSE;
                if (UTaddtext (
                    rip, countbuf, alfn, alfs,
                    (sum > 0 || fflag) ? sevattrs[sevi].cl[tmi] : sgcl,
                    alx1, aly1, alx2 - 1, aly2 - 1, 0, 0, TRUE, &w, &h
                ) == -1) {
                    RImaxtsflag = TRUE;
                    SUwarning (0, "AGDdrawgrid", "cannot add alarm text");
                    return -1;
                }
                RImaxtsflag = TRUE;
            }
        }
    }

    if (tlshowtitle && tlurl && tlurl[0]) {
        a.mode = RI_AREA_AREA;
        a.type = RI_AREA_RECT;
        a.u.rect.x1 = tlxoff;
        a.u.rect.y1 = tlyoff;
        a.u.rect.x2 = tlxoff + tlxsiz - 1;
        a.u.rect.y2 = tlyoff + tlysiz - 1;
        a.pass = 0;
        a.obj = obj;
        a.url = tlurl;
        a.info = tlinfo;
        if (RIaddarea (rip, &a) == -1)
            SUwarning (0, "AGDdrawgrid", "cannot add area");
    }

    if (xashowvals && xaurl && xaurl[0]) {
        a.mode = RI_AREA_AREA;
        a.type = RI_AREA_RECT;
        a.u.rect.x1 = xaxoff;
        a.u.rect.y1 = xayoff;
        a.u.rect.x2 = xaxoff + xaxsiz - 1;
        a.u.rect.y2 = xayoff + xaysiz - 1;
        a.pass = 0;
        a.obj = obj;
        a.url = xaurl;
        a.info = NULL;
        if (RIaddarea (rip, &a) == -1)
            SUwarning (0, "AGDdrawgrid", "cannot add area");
    }

    if (yashowvals && yaurl && yaurl[0]) {
        a.mode = RI_AREA_AREA;
        a.type = RI_AREA_RECT;
        a.u.rect.x1 = yaxoff;
        a.u.rect.y1 = yayoff;
        a.u.rect.x2 = yaxoff + yaxsiz - 1;
        a.u.rect.y2 = yayoff + yaysiz - 1;
        a.pass = 0;
        a.obj = obj;
        a.url = yaurl;
        a.info = NULL;
        if (RIaddarea (rip, &a) == -1)
            SUwarning (0, "AGDdrawgrid", "cannot add area");
    }

    if (sgurl && sgurl[0]) {
        a.mode = RI_AREA_AREA;
        a.type = RI_AREA_RECT;
        a.u.rect.x1 = sgxoff;
        a.u.rect.y1 = sgyoff;
        a.u.rect.x2 = sgxoff + sgxsiz - 1;
        a.u.rect.y2 = sgyoff + sgysiz - 1;
        a.pass = 0;
        a.obj = obj;
        a.url = sgurl;
        a.info = NULL;
        if (RIaddarea (rip, &a) == -1)
            SUwarning (0, "AGDdrawgrid", "cannot add area");
    }

    if (obj && obj[0]) {
        a.mode = RI_AREA_EMBED;
        a.type = RI_AREA_RECT;
        a.u.rect.x1 = xoff;
        a.u.rect.y1 = yoff;
        a.u.rect.x2 = xoff + xsiz - 1;
        a.u.rect.y2 = yoff + ysiz - 1;
        a.pass = 0;
        a.obj = obj;
        a.url = NULL;
        a.info = NULL;
        if (RIaddarea (rip, &a) == -1)
            SUwarning (0, "AGDdrawgrid", "cannot add area");
    }

    RIexecop (
        rip, NULL, 0, NULL, "black", "black", "black", "Times-Roman", "10"
    );

    if ((ali + 1) % alarmgridsperimage == 0 || ali == AGDalarmn - 1) {
        if (RIclose (rip) == -1) {
            SUwarning (0, "AGDdrawgrid", "cannot save image");
            return -1;
        }
    }

    return 0;
}

static int sortcmp (const void *a, const void *b) {
    AGDalarm_t *ap, *bp;
    int ret;

    ap = *(AGDalarm_t **) a;
    bp = *(AGDalarm_t **) b;

    if ((ret = strcmp (ap->ilevel1, bp->ilevel1)) != 0)
        return ret;
    if ((ret = strcmp (ap->iid1, bp->iid1)) != 0)
        return ret;
    if ((ret = strcmp (ap->ilevel2, bp->ilevel2)) != 0)
        return ret;
    if ((ret = strcmp (ap->iid2, bp->iid2)) != 0)
        return ret;
    return 0;
}
