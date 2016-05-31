#include <ast.h>
#include <ast_float.h>
#include <tok.h>
#include <swift.h>
#include <ctype.h>
#include "vg_dq_vt_util.h"

UTattr_t *UTattrs[UT_ATTRGROUP_SIZE];
int UTattrn[UT_ATTRGROUP_SIZE], UTattrm[UT_ATTRGROUP_SIZE];
UTattr_t UTattrgroups[UT_ATTRGROUP_SIZE][UT_ATTR_SIZE];

char **UTtoks;
int UTtokn, UTtokm, UTlockcolor;

static char *attrstr;
static int attrlen;

int UTinit (void) {
    memset (UTattrs, 0, UT_ATTRGROUP_SIZE * sizeof (UTattr_t *));
    memset (UTattrn, 0, UT_ATTRGROUP_SIZE * sizeof (int));
    memset (UTattrm, 0, UT_ATTRGROUP_SIZE * sizeof (int));
    memset (
        UTattrgroups, 0, UT_ATTRGROUP_SIZE * UT_ATTR_SIZE * sizeof (UTattr_t)
    );
    UTlockcolor = FALSE;

    return 0;
}

int UTsplitattr (int groupi, char *str) {
    UTattr_t *attrp;
    char *s1, *s2, *s3, c;
    int len, escmode;

    if (groupi < 0 || groupi >= UT_ATTRGROUP_SIZE) {
        SUwarning (0, "UTsplitattr", "bad attr group: %d", groupi);
        return 0;
    }

    UTattrm[groupi] = 0;

    if (!str || !str[0])
        return 0;

    if ((len = strlen (str) + 1) >= attrlen) {
        if (!(attrstr = vmresize (Vmheap, attrstr, len, VM_RSCOPY))) {
            SUwarning (0, "UTsplitattr", "cannot grow attrstr");
            return -1;
        }
        attrlen = len;
    }
    memcpy (attrstr, str, len);

    s1 = s2 = s3 = NULL;
    for (s1 = attrstr; *s1; ) {
        while (isspace (*s1))
            s1++;
        if (!*s1)
            break;
        for (s2 = s1; !isspace (*s2) && *s2 != '=' && *s2 != 0; s2++)
            ;
        if (*s2 == 0 || s1 == s2) {
            SUwarning (0, "UTsplitattr", "cannot parse attr string(1)");
            return 0;
        }
        if (*s2 != '=') {
            *s2++ = 0;
            while (isspace (*s2))
                s2++;
            if (*s2 != '=') {
                SUwarning (0, "UTsplitattr", "cannot parse attr string(2)");
                return 0;
            }
        } else
            *s2++ = 0;
        if (*s2 == 0) {
            SUwarning (0, "UTsplitattr", "cannot parse attr string(3)");
            return 0;
        }
        for (; isspace (*s2) && *s2 != 0; s2++)
            ;
        if (*s2 == 0) {
            SUwarning (0, "UTsplitattr", "cannot parse attr string(4)");
            return 0;
        }
        if (*s2 == '"' || *s2 == '\'') {
            escmode = FALSE;
            for (c = *s2, s3 = s2 + 1; *s3 != c && *s3 != 0; s3++) {
                if (*s3 == '\\' && *(s3 + 1) == c)
                    escmode = TRUE, s3++;
            }
            if (*s3 == 0) {
                SUwarning (0, "UTsplitattr", "cannot parse attr string(5)");
                return 0;
            }
            s2++;
            *s3++ = 0;
            if (escmode)
                VG_simpledec (s2, s2, s3 - s2, "'\"");
        } else {
            for (s3 = s2; !isspace (*s3) && *s3 != 0; s3++)
                ;
            if (*s3 != 0)
                *s3++ = 0;
        }

        if (UTattrm[groupi] >= UTattrn[groupi]) {
            if (!(UTattrs[groupi] = vmresize (
                Vmheap, UTattrs[groupi],
                (UTattrn[groupi] + 100) * sizeof (UTattr_t), VM_RSCOPY
            ))) {
                SUwarning (0, "UTsplitattr", "cannot grow attrs array");
                return -1;
            }
            UTattrn[groupi] += 100;
        }
        attrp = &UTattrs[groupi][UTattrm[groupi]++];
        if (
            !(attrp->name = vmstrdup (Vmheap, s1)) ||
            !(attrp->value = vmstrdup (Vmheap, s2))
        ) {
            SUwarning (0, "UTsplitattr", "cannot copy name and value");
            return -1;
        }

        s1 = s3;
    }
    return 0;
}

int UTgetattrs (int groupi) {
    UTattr_t *ap;
    int ai, aj;

    if (groupi < 0 || groupi >= UT_ATTRGROUP_SIZE) {
        SUwarning (0, "UTgetattrs", "bad attr group: %d", groupi);
        return 0;
    }

    for (ai = 0; ai < UT_ATTR_SIZE; ai++) {
        ap = &UTattrgroups[groupi][ai];
        ap->value = NULL;
        for (aj = 0; aj < UTattrm[groupi]; aj++) {
            if (ap->name && strcmp (UTattrs[groupi][aj].name, ap->name) == 0) {
                ap->value = UTattrs[groupi][aj].value;
                break;
            }
        }
    }

    return 0;
}

int UTaddclip (RI_t *rip, int x, int y, int w, int h) {
    RIop_t op;

    memset (&op, 0, sizeof (RIop_t));
    op.type = RI_OP_CLIP;
    op.u.rect.o.x = x, op.u.rect.o.y = y;
    op.u.rect.s.x = w, op.u.rect.s.y = h;
    if (RIaddop (rip, &op) == -1) {
        SUwarning (0, "UTaddclip", "cannot add clip rect");
        return -1;
    }
    return 0;
}

int UTaddlw (RI_t *rip, int lw) {
    RIop_t op;

    memset (&op, 0, sizeof (RIop_t));
    op.type = RI_OP_LW;
    op.u.length.l = lw;
    if (RIaddop (rip, &op) == -1) {
        SUwarning (0, "UTaddlw", "cannot add line width");
        return -1;
    }
    return 0;
}

int UTaddbox (RI_t *rip, int x1, int y1, int x2, int y2, char *cl, int fflag) {
    RIop_t op;
    int pi, pj;

    if (x2 > x1)
        x2--;
    if (y2 > y1)
        y2--;
    memset (&op, 0, sizeof (RIop_t));
    op.type = (fflag) ? RI_OP_C : RI_OP_c;
    op.u.color.t = cl;
    op.u.color.locked = UTlockcolor;
    if (RIaddop (rip, &op) == -1) {
        SUwarning (0, "UTaddbox", "cannot add color");
        return -1;
    }
    if (
        (pi = RIaddpt (rip, x1, y1)) == -1 || RIaddpt (rip, x2, y1) == -1 ||
        RIaddpt (rip, x2, y2) == -1 || (pj = RIaddpt (rip, x1, y2)) == -1
    ) {
        SUwarning (0, "UTaddbox", "cannot add points");
        return -1;
    }
    memset (&op, 0, sizeof (RIop_t));
    op.type = (fflag) ? RI_OP_P : RI_OP_p;
    op.u.poly.pi = pi, op.u.poly.pn = pj - pi + 1;
    if (RIaddop (rip, &op) == -1) {
        SUwarning (0, "UTaddbox", "cannot add poly");
        return -1;
    }
    return 0;
}

int UTaddline (RI_t *rip, int x1, int y1, int x2, int y2, char *cl) {
    RIop_t op;
    int pi, pj;

    memset (&op, 0, sizeof (RIop_t));
    op.type = RI_OP_c;
    op.u.color.t = cl;
    op.u.color.locked = UTlockcolor;
    if (RIaddop (rip, &op) == -1) {
        SUwarning (0, "UTaddline", "cannot add color");
        return -1;
    }
    if (
        (pi = RIaddpt (rip, x1, y1)) == -1 ||
        (pj = RIaddpt (rip, x2, y2)) == -1
    ) {
        SUwarning (0, "UTaddline", "cannot add points");
        return -1;
    }
    memset (&op, 0, sizeof (RIop_t));
    op.type = RI_OP_L;
    op.u.poly.pi = pi, op.u.poly.pn = pj - pi + 1;
    if (RIaddop (rip, &op) == -1) {
        SUwarning (0, "UTaddline", "cannot add line");
        return -1;
    }
    return 0;
}

static int pxs[512], pys[512];
static int pxyi;
static int polypi, polypj;

int UTaddpolybegin (RI_t *rip, char *cl, int style) {
    RIop_t op;

    memset (&op, 0, sizeof (RIop_t));
    op.type = RI_OP_c;
    op.u.color.t = cl;
    op.u.color.locked = UTlockcolor;
    if (RIaddop (rip, &op) == -1) {
        SUwarning (0, "UTaddpolybegin", "cannot add color");
        return -1;
    }
    memset (&op, 0, sizeof (RIop_t));
    op.type = RI_OP_S;
    op.u.style.t = "";
    op.u.style.style = style;
    if (RIaddop (rip, &op) == -1) {
        SUwarning (0, "UTaddpolybegin", "cannot add style");
        return -1;
    }
    polypi = polypj = -1;
    pxyi = 0;
    return 0;
}

int UTaddpolypoint (RI_t *rip, int x, int y) {
    int pi;

    if (pxyi >= 512) {
        if ((pi = RIaddpts (rip, pxs, pys, pxyi)) == -1) {
            SUwarning (0, "UTaddpolypoint", "cannot add points");
            return -1;
        }
        if (polypi == -1)
            polypi = pi;
        polypj = pi + pxyi - 1;
        pxyi = 0;
    }
    pxs[pxyi] = x, pys[pxyi] = y;
    pxyi++;
    return 0;
}

int UTaddpolyend (RI_t *rip) {
    RIop_t op;
    int pi;

    if (pxyi > 0) {
        if ((pi = RIaddpts (rip, pxs, pys, pxyi)) == -1) {
            SUwarning (0, "UTaddpolyend", "cannot add points");
            return -1;
        }
        if (polypi == -1)
            polypi = pi;
        polypj = pi + pxyi - 1;
    }

    if (polypj == -1)
        return 0;

    memset (&op, 0, sizeof (RIop_t));
    op.type = RI_OP_L;
    op.u.poly.pi = polypi, op.u.poly.pn = polypj - polypi + 1;
    if (RIaddop (rip, &op) == -1) {
        SUwarning (0, "UTaddpolyend", "cannot add poly");
        return -1;
    }
    return 0;
}

int UTaddtext (
    RI_t *rip, char *str, char *fn, char *fs, char *cl,
    int x1, int y1, int x2, int y2, int xj, int yj, int fflag, int *wp, int *hp
) {
    RIop_t op, top;
    char buf[100];

again:
    if (RIgettextsize (str, fn, fs, &top) == -1) {
        SUwarning (0, "UTaddtext", "cannot get text size");
        return -1;
    }
    memset (&op, 0, sizeof (RIop_t));
    op.type = RI_OP_FC;
    op.u.color.t = cl;
    op.u.color.locked = UTlockcolor;
    if (RIaddop (rip, &op) == -1) {
        SUwarning (0, "UTaddtext", "cannot add color");
        return -1;
    }
    memset (&op, 0, sizeof (RIop_t));
    op.type = RI_OP_F;
    op.u.font.t = fn;
    op.u.font.s = atoi (fs);
    op.u.font.f = top.u.font.f;
    op.u.font.w = top.u.font.w;
    op.u.font.h = top.u.font.h;
    if (RIaddop (rip, &op) == -1) {
        SUwarning (0, "UTaddtext", "cannot add font");
        return -1;
    }
    memset (&op, 0, sizeof (RIop_t));
    op.type = RI_OP_T;
    switch (xj) {
    case -1: op.u.text.p.x = x1;            break;
    case 0:  op.u.text.p.x = (x1 + x2) / 2; break;
    case 1:  op.u.text.p.x = x2;            break;
    }
    op.u.text.jx = xj;
    switch (yj) {
    case -1: op.u.text.p.y = y1;            break;
    case 0:  op.u.text.p.y = (y1 + y2) / 2; break;
    case 1:  op.u.text.p.y = y2;            break;
    }
    op.u.text.jy = yj;
    op.u.text.w = top.u.font.w;
    op.u.text.h = top.u.font.h;
    op.u.text.t = str;
    if (fflag && (op.u.text.w > (x2 - x1) + 1 || op.u.text.h > (y2 - y1) + 1)) {
        if (top.u.font.s > 4) {
            sfsprintf (buf, 100, "%d", --top.u.font.s);
            fs = buf;
            goto again;
        }
    }
    if (RIaddop (rip, &op) == -1) {
        SUwarning (0, "UTaddtext", "cannot add text");
        return -1;
    }
    *wp = top.u.font.w, *hp = top.u.font.h;

    return 0;
}

int UTaddtablebegin (RI_t *rip, char *fn, char *fs, char *bg, char *cl) {
    RIop_t op;

    memset (&op, 0, sizeof (RIop_t));
    op.type = RI_OP_HTMLTBB;
    op.u.htb.lab = NULL;
    op.u.htb.fn = fn;
    op.u.htb.fs = fs;
    op.u.htb.bg = bg;
    op.u.htb.cl = cl;
    if (RIaddop (rip, &op) == -1) {
        SUwarning (0, "UTaddtablebegin", "cannot add begin");
        return -1;
    }

    return 0;
}

int UTaddtableend (RI_t *rip) {
    RIop_t op;

    memset (&op, 0, sizeof (RIop_t));
    op.type = RI_OP_HTMLTBE;
    if (RIaddop (rip, &op) == -1) {
        SUwarning (0, "UTaddtableend", "cannot add end");
        return -1;
    }

    return 0;
}

int UTaddtablecaption (
    RI_t *rip, char *str, char *fn, char *fs, char *bg, char *cl,
    char *url, char *info
) {
    RIop_t op;

    memset (&op, 0, sizeof (RIop_t));
    op.type = RI_OP_HTMLTBCB;
    op.u.htb.lab = str;
    op.u.htb.fn = fn;
    op.u.htb.fs = fs;
    op.u.htb.bg = bg;
    op.u.htb.cl = cl;
    if (RIaddop (rip, &op) == -1) {
        SUwarning (0, "UTaddtablecaption", "cannot add caption");
        return -1;
    }

    if (url && url[0]) {
        memset (&op, 0, sizeof (RIop_t));
        op.type = RI_OP_HTMLAB;
        op.u.ha.url = url;
        op.u.ha.info = info;
        op.u.ha.fn = fn;
        op.u.ha.fs = fs;
        op.u.ha.bg = bg;
        op.u.ha.cl = cl;
        if (RIaddop (rip, &op) == -1) {
            SUwarning (0, "UTaddtablecaption", "cannot add link begin");
            return -1;
        }
        memset (&op, 0, sizeof (RIop_t));
        op.type = RI_OP_HTMLAE;
        if (RIaddop (rip, &op) == -1) {
            SUwarning (0, "UTaddtablecaption", "cannot add link end");
            return -1;
        }
    }

    memset (&op, 0, sizeof (RIop_t));
    op.type = RI_OP_HTMLTBCE;
    if (RIaddop (rip, &op) == -1) {
        SUwarning (0, "UTaddtablecaption", "cannot add caption");
        return -1;
    }

    return 0;
}

int UTaddrowbegin (RI_t *rip, char *fn, char *fs, char *bg, char *cl) {
    RIop_t op;

    memset (&op, 0, sizeof (RIop_t));
    op.type = RI_OP_HTMLTRB;
    op.u.htr.fn = fn;
    op.u.htr.fs = fs;
    op.u.htr.bg = bg;
    op.u.htr.cl = cl;
    if (RIaddop (rip, &op) == -1) {
        SUwarning (0, "UTaddrowbegin", "cannot add row begin");
        return -1;
    }

    return 0;
}

int UTaddrowend (RI_t *rip) {
    RIop_t op;

    memset (&op, 0, sizeof (RIop_t));
    op.type = RI_OP_HTMLTRE;
    if (RIaddop (rip, &op) == -1) {
        SUwarning (0, "UTaddrowend", "cannot add row end");
        return -1;
    }

    return 0;
}

int UTaddcellbegin (
    RI_t *rip, char *fn, char *fs, char *bg, char *cl, int j, int cn
) {
    RIop_t op;

    memset (&op, 0, sizeof (RIop_t));
    op.type = RI_OP_HTMLTDB;
    op.u.htd.fn = fn;
    op.u.htd.fs = fs;
    op.u.htd.bg = bg;
    op.u.htd.cl = cl;
    op.u.htd.j = j;
    op.u.htd.cn = cn;
    if (RIaddop (rip, &op) == -1) {
        SUwarning (0, "UTaddcellbegin", "cannot add cell begin");
        return -1;
    }

    return 0;
}

int UTaddcellend (RI_t *rip) {
    RIop_t op;

    memset (&op, 0, sizeof (RIop_t));
    op.type = RI_OP_HTMLTDE;
    if (RIaddop (rip, &op) == -1) {
        SUwarning (0, "UTaddcellend", "cannot add cell end");
        return -1;
    }

    return 0;
}

int UTaddcelltext (RI_t *rip, char *str) {
    RIop_t op;

    memset (&op, 0, sizeof (RIop_t));
    op.type = RI_OP_HTMLTDT;
    op.u.htd.lab = str;
    if (RIaddop (rip, &op) == -1) {
        SUwarning (0, "UTaddcelltext", "cannot add cell text");
        return -1;
    }

    return 0;
}

int UTaddlinkbegin (
    RI_t *rip, char *url, char *info, char *fn, char *fs, char *bg, char *cl
) {
    RIop_t op;

    memset (&op, 0, sizeof (RIop_t));
    op.type = RI_OP_HTMLAB;
    op.u.ha.url = url;
    op.u.ha.info = info;
    op.u.ha.fn = fn;
    op.u.ha.fs = fs;
    op.u.ha.bg = bg;
    op.u.ha.cl = cl;
    if (RIaddop (rip, &op) == -1) {
        SUwarning (0, "UTaddlinkbegin", "cannot add link begin");
        return -1;
    }

    return 0;
}

int UTaddlinkend (RI_t *rip) {
    RIop_t op;

    memset (&op, 0, sizeof (RIop_t));
    op.type = RI_OP_HTMLAE;
    if (RIaddop (rip, &op) == -1) {
        SUwarning (0, "UTaddlinkend", "cannot add link end");
        return -1;
    }

    return 0;
}

int UTsplitstr (char *str) {
    char *s1, *s2;

    UTtokm = 0;
    for (s1 = str; s1; s1 = s2) {
        if ((s2 = strchr (s1, '|')))
            *s2++ = 0;
        if (UTtokm >= UTtokn) {
            if (!(UTtoks = vmresize (
                Vmheap, UTtoks, (UTtokn + 100) * sizeof (char *), VM_RSCOPY
            ))) {
                SUwarning (0, "UTsplitstr", "cannot grow UTtoks array");
                return -1;
            }
            UTtokn += 100;
        }
        UTtoks[UTtokm++] = s1;
    }

    return 0;
}
