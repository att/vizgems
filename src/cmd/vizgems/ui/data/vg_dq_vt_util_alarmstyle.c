#include <ast.h>
#include <ast_float.h>
#include <tok.h>
#include <swift.h>
#include <ctype.h>
#include "vg_dq_vt_util.h"

ASTalarm_t **ASTalarms;
int ASTalarmn;

static int width, height;

static Dt_t *alarmdict;
static Dtdisc_t alarmdisc = {
    sizeof (Dtlink_t), 2 * (SZ_level + SZ_id),
    0, NULL, NULL, NULL, NULL, NULL, NULL
};

static int tmodemap[VG_ALARM_N_MODE_MAX + 1] = {
    AST_TMODE_NTICKET,
    AST_TMODE_YTICKET,
    AST_TMODE_DTICKET,
    AST_TMODE_NTICKET,
};
static char *tmodenames[ATB_TMODE_SIZE] = {
    "ticketed", "deferred", "filtered"
};
static int tmodetrans[AST_TMODE_SIZE] = {
    0, 1, 2
};

static ASTsevattr_t *sevattrs;
static int sevn, clearsevi;

static int openmode;

static RI_t *rip;

static int sortcmp (const void *, const void *);

int ASTinit (int sn, int mode) {

    if (!(alarmdict = dtopen (&alarmdisc, Dtset))) {
        SUwarning (0, "ASTinit", "cannot create alarmdict");
        return -1;
    }
    ASTalarms = NULL;
    ASTalarmn = 0;

    sevn = sn + 1, clearsevi = sn;
    openmode = mode;

    UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_BGCOLOR].name = "bgcolor";
    UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_COLOR].name = "color";
    UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_FLAGS].name = "flags";
    UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_TMODENAMES].name = "tmodenames";
    UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_SEVNAMES].name = "sevnames";

    UTattrgroups[UT_ATTRGROUP_ALARM][UT_ATTR_COLOR].name = "color";

    return 0;
}

ASTalarm_t *ASTinsert (
    char *ilevel1, char *iid1, char *ilevel2, char *iid2,
    char *level1, char *id1, char *level2, char *id2,
    int type, int tmode, int sevi
) {
    ASTalarm_t al, *alp, *almem;
    int tmi;

    memcpy (al.ilevel1, ilevel1, SZ_level);
    memcpy (al.iid1, iid1, SZ_id);
    memcpy (al.ilevel2, ilevel2, SZ_level);
    memcpy (al.iid2, iid2, SZ_id);

    if (!(alp = dtsearch (alarmdict, &al))) {
        if (!(almem = vmalloc (Vmheap, sizeof (ASTalarm_t)))) {
            SUwarning (0, "ASTinsert", "cannot allocate almem");
            return NULL;
        }
        memset (almem, 0, sizeof (ASTalarm_t));
        memcpy (almem->ilevel1, ilevel1, SZ_level);
        memcpy (almem->iid1, iid1, SZ_id);
        memcpy (almem->ilevel2, ilevel2, SZ_level);
        memcpy (almem->iid2, iid2, SZ_id);
        if ((alp = dtinsert (alarmdict, almem)) != almem) {
            SUwarning (0, "ASTinsert", "cannot insert alarm");
            vmfree (Vmheap, almem);
            return NULL;
        }
        for (tmi = 0; tmi < AST_TMODE_SIZE; tmi++) {
            if (!(alp->cns[tmi] = vmalloc (Vmheap, sevn * sizeof (int)))) {
                SUwarning (0, "ASTinsert", "cannot allocate cns array");
                return NULL;
            }
            memset (alp->cns[tmi], 0, sevn * sizeof (int));
            if (!(alp->havecns[tmi] = vmalloc (Vmheap, sevn * sizeof (int)))) {
                SUwarning (0, "ASTinsert", "cannot allocate havecns array");
                return NULL;
            }
        }
    }
    if (tmode == -123 && sevi == -123)
        return alp; // called to create black default style

    if (sevi < 0 || sevi >= sevn) {
        SUwarning (
            0, "ASTinsert", "sevi: %d out of range: %d-%d", sevi, 0, sevn
        );
        return NULL;
    }
    if (tmode < 0 || tmode > VG_ALARM_N_MODE_MAX) {
        SUwarning (
            0, "ASTinsert", "tmode: %d out of range: %d-%d",
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

int ASTflatten (void) {
    ASTalarm_t *alp;
    int ali;

    if ((ASTalarmn = dtsize (alarmdict)) == 0)
        return 0;

    if (!(ASTalarms = vmalloc (Vmheap, sizeof (ASTalarm_t *) * ASTalarmn))) {
        SUwarning (0, "ASTflatten", "cannot allocate alarms array");
        return -1;
    }
    for (
        ali = 0, alp = dtfirst (alarmdict); alp;
        alp = dtnext (alarmdict, alp)
    )
        ASTalarms[ali++] = alp;
    qsort (ASTalarms, ASTalarmn, sizeof (ASTalarm_t *), sortcmp);

    return 0;
}

int ASTsetupprint (
    char *rendermode, char *sgattr, char *alattr
) {
    UTattr_t *ap;
    int sevi, sevattrk, sevattrl, tmodei;
    int tmi, tmk, tml, ftmi, ttmi;
    char *namestr, *colorstr, *sgflags, *sgstr, *s1, *s2, *s3, *s4, *s5;
    void *tokp;

    if (ASTalarmn == 0)
        return 0;

    if (UTsplitattr (UT_ATTRGROUP_MAIN, sgattr) == -1) {
        SUwarning (0, "ASTsetupprint", "cannot parse sgattr");
        return -1;
    }
    if (UTgetattrs (UT_ATTRGROUP_MAIN) == -1) {
        SUwarning (0, "ASTsetupprint", "cannot get alarmstyle attr");
        return -1;
    }
    if (UTsplitattr (UT_ATTRGROUP_ALARM, alattr) == -1) {
        SUwarning (0, "ASTsetupprint", "cannot parse alattr");
        return -1;
    }
    if (UTgetattrs (UT_ATTRGROUP_ALARM) == -1) {
        SUwarning (0, "ASTsetupprint", "cannot get alarm attr");
        return -1;
    }

    width = 10;
    height = 10;
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
                ftmi = AST_TMODE_YTICKET;
            else if (strcmp (s1, "deferred") == 0)
                ftmi = AST_TMODE_DTICKET;
            else if (strcmp (s1, "filtered") == 0)
                ftmi = AST_TMODE_NTICKET;
            else
                continue;
            if (strcmp (s2, "ticketed") == 0)
                ttmi = AST_TMODE_YTICKET;
            else if (strcmp (s2, "deferred") == 0)
                ttmi = AST_TMODE_DTICKET;
            else if (strcmp (s2, "filtered") == 0)
                ttmi = AST_TMODE_NTICKET;
            else
                continue;
            tmodetrans[ftmi] = ttmi;
        } else if (strncmp (sgstr, "hide_", 5) == 0) {
            s1 = sgstr + 5;
            if (strcmp (s1, "ticketed") == 0)
                ftmi = AST_TMODE_YTICKET;
            else if (strcmp (s1, "deferred") == 0)
                ftmi = AST_TMODE_DTICKET;
            else if (strcmp (s1, "filtered") == 0)
                ftmi = AST_TMODE_NTICKET;
            else
                continue;
            tmodetrans[ftmi] = -1;
        }
    }
    tokclose (tokp);

    if (!(sevattrs = vmalloc (Vmheap, sizeof (ASTsevattr_t) * sevn))) {
        SUwarning (0, "ASTsetupprint", "cannot allocate sevattrs");
        return -1;
    }
    memset (sevattrs, 0, sizeof (ASTsevattr_t) * sevn);
    ap = &UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_TMODENAMES];
    namestr = (ap->value) ? ap->value : NULL;
    for (s1 = namestr; s1 && s1[0]; s1 = s2) {
        if ((s2 = strchr (s1, '|')))
            *s2++ = 0;
        if (!(s3 = strchr (s1, ':'))) {
            SUwarning (0, "ASTsetupprint", "bad tmodename label: %s", s1);
            break;
        }
        *s3++ = 0;
        tmodei = -1;
        if (strcmp (s1, "ticketed") == 0)
            tmodei = AST_TMODE_YTICKET;
        else if (strcmp (s1, "deferred") == 0)
            tmodei = AST_TMODE_DTICKET;
        else if (strcmp (s1, "filtered") == 0)
            tmodei = AST_TMODE_NTICKET;
        if (tmodei < 0 || tmodei >= AST_TMODE_SIZE)
            continue;
        if (!(tmodenames[tmodei] = vmstrdup (Vmheap, s3))) {
            SUwarning (0, "ASTsetupprint", "cannot copy label: %s", s3);
            break;
        }
    }
    ap = &UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_SEVNAMES];
    namestr = (ap->value) ? ap->value : NULL;
    for (s1 = namestr; s1 && s1[0]; s1 = s2) {
        if ((s2 = strchr (s1, '|')))
            *s2++ = 0;
        if (!(s3 = strchr (s1, ':'))) {
            SUwarning (0, "ASTsetupprint", "bad sevname label: %s", s1);
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
            SUwarning (0, "ASTsetupprint", "cannot copy label: %s", s3);
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
            SUwarning (0, "ASTsetupprint", "bad sevname label: %s", s1);
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
            tmk = tml = AST_TMODE_YTICKET;
        else if (strcmp (s3, "deferred") == 0)
            tmk = tml = AST_TMODE_DTICKET;
        else if (strcmp (s3, "filtered") == 0)
            tmk = tml = AST_TMODE_NTICKET;
        else if (strcmp (s3, "all") == 0)
            tmk = 0, tml = AST_TMODE_SIZE - 1;
        if (tmk < 0 || tmk >= AST_TMODE_SIZE)
            continue;
        for (sevi = sevattrk; sevi <= sevattrl; sevi++) {
            for (tmi = tmk; tmi <= tml; tmi++) {
                if (!(sevattrs[sevi].bg[tmi] = vmstrdup (Vmheap, s4))) {
                    SUwarning (0, "ASTsetupprint", "cannot copy bg: %s", s4);
                    break;
                }
                if (!(sevattrs[sevi].cl[tmi] = vmstrdup (Vmheap, s5))) {
                    SUwarning (0, "ASTsetupprint", "cannot copy color: %s", s5);
                    break;
                }
            }
        }
    }

    return 0;
}

int ASTfinishprint (char *fprefix) {
    Sfio_t *fp;
    int sevi, tmi;

    if (!(fp = sfopen (NULL, sfprints ("%s.info", fprefix), "w"))) {
        SUwarning (0, "ATBfinishprint", "cannot open info file");
        return -1;
    }
    sfprintf (
        fp,
        "<table class=sdiv>"
        "<caption class=sdiv style='font-size:80%%'>color map</caption>\n"
    );
    for (tmi = 0; tmi <= AST_TMODE_SIZE; tmi++) {
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
        SUwarning (0, "ATBfinishprint", "cannot close info file");
        return -1;
    }
    return 0;
}

int ASTprintstyle (
    char *fprefix, int findex, int ali, char *obj
) {
    ASTalarm_t *alp;
    UTattr_t *ap;
    char *sgbg, *sgcl;
    char *bg, *cl;
    int sum, tmi, tmj, sevi, sevm;
    RIarea_t a;

    sevm = sevn - (openmode ? 1 : 0);
    if (sevm < 2)
        return 0;

    alp = ASTalarms[ali];

    if (ali == 0) {
        if (!(rip = RIopen (RI_TYPE_IMAGE, fprefix, findex, width, height))) {
            SUwarning (0, "ASTprintstyle", "cannot open file #%d", findex);
            return -1;
        }
    }

    RIaddop (rip, NULL); // reset

    ap = &UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_BGCOLOR];
    sgbg = (ap->value && ap->value[0]) ? ap->value : "white";
    ap = &UTattrgroups[UT_ATTRGROUP_MAIN][UT_ATTR_COLOR];
    sgcl = (ap->value && ap->value[0]) ? ap->value : "black";

    sum = 0;
    for (tmi = 0; tmi < AST_TMODE_SIZE; tmi++) {
        if (tmodetrans[tmi] != tmi)
            continue;
        for (sevi = 1; sevi < sevm; sevi++) {
            for (tmj = 0; tmj < AST_TMODE_SIZE; tmj++) {
                if (tmodetrans[tmj] != tmi)
                    continue;
                sevattrs[sevi].used[tmj]++;
                sum += alp->cns[tmj][sevi];
            }
            if (sum > 0)
                break;
        }
        if (sum > 0)
            break;
    }
    if (sum > 0)
        bg = sevattrs[sevi].bg[tmi], cl = sevattrs[sevi].cl[tmi];
    else
        bg = sgbg, cl = sgcl;

    if (obj && obj[0]) {
        a.mode = RI_AREA_EMBED;
        a.type = RI_AREA_STYLE;
        a.u.style.fn = a.u.style.fs = "";
        a.u.style.flcl = bg;
        a.u.style.lncl = bg;
        a.u.style.fncl = cl;
        a.pass = 0;
        a.obj = obj;
        a.url = NULL;
        a.info = NULL;
        if (RIaddarea (rip, &a) == -1)
            SUwarning (0, "ASTprintstyle", "cannot add area");
    }

    RIexecop (
        rip, NULL, 0, NULL, "black", "black", "black", "Times-Roman", "10"
    );

    if (ali == ASTalarmn - 1) {
        if (RIclose (rip) == -1) {
            SUwarning (0, "ASTprintstyle", "cannot save image");
            return -1;
        }
    }

    return 0;
}

static int sortcmp (const void *a, const void *b) {
    ASTalarm_t *ap, *bp;
    int ret;

    ap = *(ASTalarm_t **) a;
    bp = *(ASTalarm_t **) b;

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
