
#include <ast.h>
#include <option.h>
#include <stdio.h>
#include <swift.h>
#define VG_DEFS_MAIN
#include "vg_dq_vt_util.h"

static const char usage[] =
"[-1p1?\n@(#)$Id: vg_dq_vt_tool (AT&T Labs Research) 2014-05-20 $\n]"
USAGE_LICENSE
"[+NAME?vg_dq_vt_tool - VG visual tool graphic generator]"
"[+DESCRIPTION?\bvg_dq_vt_tool\b reads lines containing rendering directives"
" and generates an image from these directives."
"]"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"[+SEE ALSO?\bdds\b(1), \bdds\b(3)]"
;

#define CHECK_ARGN(line, got, need) { \
    if (got != need) { \
        SUwarning ( \
            0, "vg_dq_vt_tool", "argument error, got: %d need: %d - %s", \
            got, need, line \
        ); \
        continue; \
    } \
}
#define CHECK_ARGN_POLY1(line, got) { \
    if (got < 6) { \
        SUwarning ( \
            0, "vg_dq_vt_tool", \
            "argument error, got: %d need at least 6 - %s", got, line \
        ); \
        continue; \
    } \
    if ((got - 4) % 2 != 0) { \
        SUwarning ( \
            0, "vg_dq_vt_tool", \
            "argument error, got: %d need a multiple of 2 - %s", got, line \
        ); \
        continue; \
    } \
}
#define CHECK_ARGN_POLY2(line, got) { \
    if (got < 5) { \
        SUwarning ( \
            0, "vg_dq_vt_tool", \
            "argument error, got: %d need at least 5 - %s", got, line \
        ); \
        continue; \
    } \
    if ((got - 3) % 2 != 0) { \
        SUwarning ( \
            0, "vg_dq_vt_tool", \
            "argument error, got: %d need a multiple of 2 - %s", got, line \
        ); \
        continue; \
    } \
}
#define CHECK_ARGN_POLY3(line, got) { \
    int n; \
    if (got < 6) { \
        SUwarning ( \
            0, "vg_dq_vt_tool", \
            "argument error, got: %d need at least 6 - %s", got, line \
        ); \
        continue; \
    } \
    n = ((got - 4) - 2) / 6; \
    if ((got - 4) != n * 6 + 2) { \
        SUwarning ( \
            0, "vg_dq_vt_tool", \
            "argument error, got: %d need a multiple of 6+2 - %s", got, line \
        ); \
        continue; \
    } \
}

#define X(x) (x)
#define Y(y) (ch - ((y) + 1))

#define ADD_OP(rip, op) { \
    if (RIaddop (rip, &op) == -1) { \
        SUwarning (0, "vg_dq_vt_tool", "addop failed for op %d", op.type); \
        continue; \
    } \
}

#define ADD_PT(pi, rip, x, y) { \
    if ((pi = RIaddpt (rip, x, y)) == -1) { \
        SUwarning (0, "vg_dq_vt_tool", "addpt failed for %d/%d", x, y); \
        continue; \
    } \
}

int main (int argc, char **argv) {
    int norun;

    RI_t *rip;
    RIop_t op, top;
    RIarea_t a;
    char *toks[10000];
    int tokm, toki;
    char cprefix[PATH_MAX];
    int cindex, cw, ch;
    char buf[100], *line, *s, *str, *fn, *fs;
    int pagen, ritype, x1, y1, x2, y2, xj, yj, w, h, b, e, pi, pj, fflag;

    pagen = 0;
    rip = NULL;
    norun = 0;
    for (;;) {
        switch (optget (argv, usage)) {
        case -999:
            SUwarnlevel++;
            continue;
        case '?':
            SUusage (0, "vg_dq_vt_tool", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "vg_dq_vt_tool", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;

    if (RIinit ("tool", "tool", 10000000) == -1)
        SUerror ("vg_dq_vt_tool", "init failed");
    rip = NULL;
    while ((line = sfgetr (sfstdin, '\n', 1))) {
        if ((tokm = tokscan (line, &s, " %v ", toks, 10000)) < 1) {
            SUwarning (0, "vg_dq_vt_tool", "cannot parse line %s", line);
            continue;
        }
        if (strcmp (toks[0], "open") == 0) {
            CHECK_ARGN (line, tokm, 6); // does a 'continue' on error
            if (strcmp (toks[1], "image") == 0)
                ritype = RI_TYPE_IMAGE;
            else if (strcmp (toks[1], "table") == 0)
                ritype = RI_TYPE_TABLE;
            else
                SUerror ("vg_dq_vt_tool", "unknown type %s", toks[1]);
            strncpy (cprefix, toks[2], PATH_MAX);
            cindex = atoi (toks[3]);
            cw = atoi (toks[4]);
            ch = atoi (toks[5]);
            if (!(rip = RIopen (ritype, cprefix, cindex, cw, ch)))
                SUerror (
                    "vg_dq_vt_tool", "open failed for %d %s.%d %d/%d",
                    ritype, cprefix, cindex, cw, ch
                );
            RIaddop (rip, NULL);
            pagen = 0;
            continue;
        } else if (strcmp (toks[0], "close") == 0) {
            CHECK_ARGN (line, tokm, 1); // does a 'continue' on error
            RIexecop (
                rip, NULL, 0, NULL, "black", "black", "black",
                "Times-Roman", "10"
            );
            if (RIclose (rip) == -1)
                SUerror (
                    "vg_dq_vt_tool", "close failed for %d %s.%d %d/%d",
                    ritype, cprefix, cindex, cw, ch
                );
            rip = NULL;
            continue;
        } else if (!rip) {
            SUwarning (0, "vg_dq_vt_tool", "skipping operation %s", line);
            continue;
        }
        if (strcmp (toks[0], "clip") == 0) {
            CHECK_ARGN (line, tokm, 5); // does a 'continue' on error
            x1 = X (atoi (toks[1]));
            y1 = Y (atoi (toks[2]));
            w = atoi (toks[3]);
            h = atoi (toks[4]);

            memset (&op, 0, sizeof (RIop_t));
            op.type = RI_OP_CLIP;
            op.u.rect.o.x = x1;
            op.u.rect.o.y = y1 - h;
            op.u.rect.s.x = w;
            op.u.rect.s.y = h;
            ADD_OP (rip, op);
        } else if (strcmp (toks[0], "box") == 0) {
            CHECK_ARGN (line, tokm, 8); // does a 'continue' on error
            x1 = X (atoi (toks[1]));
            y1 = Y (atoi (toks[2]));
            x2 = X (atoi (toks[3]));
            y2 = Y (atoi (toks[4]));
            fflag = (strcmp (toks[6], "true") == 0) ? TRUE : FALSE;

            memset (&op, 0, sizeof (RIop_t));
            op.type = RI_OP_LW;
            op.u.length.l = atoi (toks[5]);
            ADD_OP (rip, op);

            memset (&op, 0, sizeof (RIop_t));
            op.type = fflag ? RI_OP_C : RI_OP_c;
            op.u.color.t = toks[7];
            ADD_OP (rip, op);

            ADD_PT (pi, rip, x1, y1);
            ADD_PT (pj, rip, x2, y1);
            ADD_PT (pj, rip, x2, y2);
            ADD_PT (pj, rip, x1, y2);
            memset (&op, 0, sizeof (RIop_t));
            op.type = fflag ? RI_OP_P : RI_OP_p;
            op.u.poly.pi = pi, op.u.poly.pn = pj - pi + 1;
            ADD_OP (rip, op);
        } else if (strcmp (toks[0], "ellipse") == 0) {
            CHECK_ARGN (line, tokm, 10); // does a 'continue' on error
            x1 = X (atoi (toks[1]));
            y1 = Y (atoi (toks[2]));
            w = atoi (toks[3]);
            h = atoi (toks[4]);
            b = atoi (toks[5]);
            e = atoi (toks[6]);
            fflag = (strcmp (toks[8], "true") == 0) ? TRUE : FALSE;

            memset (&op, 0, sizeof (RIop_t));
            op.type = RI_OP_LW;
            op.u.length.l = atoi (toks[7]);
            ADD_OP (rip, op);

            memset (&op, 0, sizeof (RIop_t));
            op.type = fflag ? RI_OP_C : RI_OP_c;
            op.u.color.t = toks[9];
            ADD_OP (rip, op);

            memset (&op, 0, sizeof (RIop_t));
            op.type = fflag ? RI_OP_E : RI_OP_e;
            op.u.rect.o.x = x1, op.u.rect.o.y = y1;
            op.u.rect.s.x = w, op.u.rect.s.y = h;
            op.u.rect.ba = b, op.u.rect.ea = e;
            ADD_OP (rip, op);
        } else if (strcmp (toks[0], "polygon") == 0) {
            CHECK_ARGN_POLY1 (line, tokm); // does a 'continue' on error
            fflag = (strcmp (toks[tokm - 2], "true") == 0) ? TRUE : FALSE;

            memset (&op, 0, sizeof (RIop_t));
            op.type = RI_OP_LW;
            op.u.length.l = atoi (toks[tokm - 3]);
            ADD_OP (rip, op);

            memset (&op, 0, sizeof (RIop_t));
            op.type = fflag ? RI_OP_C : RI_OP_c;
            op.u.color.t = toks[tokm - 1];
            ADD_OP (rip, op);

            for (toki = 1; toki < tokm - 3; toki += 2) {
                x1 = X (atoi (toks[toki]));
                y1 = Y (atoi (toks[toki + 1]));
                if (toki == 1) {
                    ADD_PT (pi, rip, x1, y1);
                } else {
                    ADD_PT (pj, rip, x1, y1);
                }
            }
            memset (&op, 0, sizeof (RIop_t));
            op.type = fflag ? RI_OP_P : RI_OP_p;
            op.u.poly.pi = pi, op.u.poly.pn = pj - pi + 1;
            ADD_OP (rip, op);
        } else if (strcmp (toks[0], "polyline") == 0) {
            CHECK_ARGN_POLY2 (line, tokm); // does a 'continue' on error

            memset (&op, 0, sizeof (RIop_t));
            op.type = RI_OP_LW;
            op.u.length.l = atoi (toks[tokm - 2]);
            ADD_OP (rip, op);

            memset (&op, 0, sizeof (RIop_t));
            op.type = RI_OP_c;
            op.u.color.t = toks[tokm - 1];
            ADD_OP (rip, op);

            for (toki = 1; toki < tokm - 2; toki += 2) {
                x1 = X (atoi (toks[toki]));
                y1 = Y (atoi (toks[toki + 1]));
                if (toki == 1) {
                    ADD_PT (pi, rip, x1, y1);
                } else {
                    ADD_PT (pj, rip, x1, y1);
                }
            }
            memset (&op, 0, sizeof (RIop_t));
            op.type = RI_OP_L;
            op.u.poly.pi = pi, op.u.poly.pn = pj - pi + 1;
            ADD_OP (rip, op);
        } else if (strcmp (toks[0], "splinegon") == 0) {
            CHECK_ARGN_POLY3 (line, tokm); // does a 'continue' on error
            fflag = (strcmp (toks[tokm - 2], "true") == 0) ? TRUE : FALSE;

            memset (&op, 0, sizeof (RIop_t));
            op.type = RI_OP_LW;
            op.u.length.l = atoi (toks[tokm - 3]);
            ADD_OP (rip, op);

            memset (&op, 0, sizeof (RIop_t));
            op.type = fflag ? RI_OP_C : RI_OP_c;
            op.u.color.t = toks[tokm - 1];
            ADD_OP (rip, op);

            for (toki = 1; toki < tokm - 3; toki += 2) {
                x1 = X (atoi (toks[toki]));
                y1 = Y (atoi (toks[toki + 1]));
                if (toki == 1) {
                    ADD_PT (pi, rip, x1, y1);
                } else {
                    ADD_PT (pj, rip, x1, y1);
                }
            }
            memset (&op, 0, sizeof (RIop_t));
            op.type = fflag ? RI_OP_b : RI_OP_B;
            op.u.poly.pi = pi, op.u.poly.pn = pj - pi + 1;
            ADD_OP (rip, op);
        } else if (strcmp (toks[0], "text") == 0) {
            CHECK_ARGN (line, tokm, 12); // does a 'continue' on error
            str = toks[1];
            fn = toks[2];
            fs = toks[3];
            x1 = X (atoi (toks[4]));
            y1 = Y (atoi (toks[5]));
            x2 = X (atoi (toks[6]));
            y2 = Y (atoi (toks[7]));
            xj = atoi (toks[8]);
            yj = atoi (toks[9]);
            fflag = (strcmp (toks[10], "true") == 0) ? TRUE : FALSE;

again:
            if (RIgettextsize (str, fn, fs, &top) == -1) {
                SUwarning (
                    0, "vg_dq_vt_tool",
                    "cannot get text size - %s - %s - %s", str, fn, fs
                );
                continue;
            }
            if (fflag && (
                top.u.font.w > (x2 - x1) + 1 || top.u.font.h > (y1 - y2) + 1)
            ) {
                if (top.u.font.s > 4) {
                    sfsprintf (buf, 100, "%d", --top.u.font.s);
                    fs = buf;
                    goto again;
                }
            }

            memset (&op, 0, sizeof (RIop_t));
            op.type = RI_OP_FC;
            op.u.color.t = toks[11];
            ADD_OP (rip, op);

            memset (&op, 0, sizeof (RIop_t));
            op.type = RI_OP_F;
            op.u.font.t = fn;
            op.u.font.s = atoi (fs);
            op.u.font.f = top.u.font.f;
            op.u.font.w = top.u.font.w;
            op.u.font.h = top.u.font.h;
            ADD_OP (rip, op);

            memset (&op, 0, sizeof (RIop_t));
            op.type = RI_OP_T;
            switch (xj) {
            case -1: op.u.text.p.x = x1;            break;
            case 0:  op.u.text.p.x = (x1 + x2) / 2; break;
            case 1:  op.u.text.p.x = x2;            break;
            }
            op.u.text.jx = xj;
            switch (yj) {
            case -1: op.u.text.p.y = y2;            break;
            case 0:  op.u.text.p.y = (y2 + y1) / 2; break;
            case 1:  op.u.text.p.y = y1;            break;
            }
            op.u.text.jy = yj;
            op.u.text.w = top.u.font.w;
            op.u.text.h = top.u.font.h;
            op.u.text.t = str;
            ADD_OP (rip, op);
        } else if (strcmp (toks[0], "image") == 0) {
            CHECK_ARGN (line, tokm, 4); // does a 'continue' on error
            x1 = X (atoi (toks[2]));
            y1 = Y (atoi (toks[3]));

            memset (&op, 0, sizeof (RIop_t));
            if (RIloadimage (toks[1], &op) == -1) {
                SUwarning (0, "vg_dq_vt_tool", "cannot load image %s", toks[1]);
                continue;
            }
            op.u.img.rx = x1, op.u.img.ry = y1 - op.u.img.ih;
            ADD_OP (rip, op);
        } else if (strcmp (toks[0], "page") == 0) {
            CHECK_ARGN (line, tokm, 2); // does a 'continue' on error

            if (RIaddtoc (rip, ++pagen, toks[1]) == -1) {
                SUwarning (0, "vg_dq_vt_tool", "cannot add page %s", toks[1]);
                continue;
            }
        } else if (strcmp (toks[0], "area") == 0) {
            CHECK_ARGN (line, tokm, 8); // does a 'continue' on error
            x1 = X (atoi (toks[1]));
            y1 = Y (atoi (toks[2]));
            x2 = X (atoi (toks[3]));
            y2 = Y (atoi (toks[4]));

            a.mode = RI_AREA_AREA;
            a.type = RI_AREA_RECT;
            a.u.rect.x1 = x1;
            a.u.rect.y1 = y2;
            a.u.rect.x2 = x2;
            a.u.rect.y2 = y1;
            a.pass = 0;
            a.obj = toks[5];
            a.url = toks[6][0] ? toks[6] : "javascript:void(0)";
            a.info = toks[7][0] ? toks[7] : NULL;
            if (RIaddarea (rip, &a) == -1) {
                SUwarning (
                    0, "vg_dq_vt_tool",
                    "cannot add area %d %d %d %d - %s - %s - %s",
                    x1, y1, x2, y2, toks[5], toks[6], toks[7]
                );
                continue;
            }
        } else if (strcmp (toks[0], "tablebegin") == 0) {
            CHECK_ARGN (line, tokm, 5); // does a 'continue' on error

            memset (&op, 0, sizeof (RIop_t));
            op.type = RI_OP_HTMLTBB;
            op.u.htb.lab = NULL;
            op.u.htb.fn = toks[1];
            op.u.htb.fs = toks[2];
            op.u.htb.bg = toks[3];
            op.u.htb.cl = toks[4];
            ADD_OP (rip, op);
        } else if (strcmp (toks[0], "tableend") == 0) {
            CHECK_ARGN (line, tokm, 1); // does a 'continue' on error

            memset (&op, 0, sizeof (RIop_t));
            op.type = RI_OP_HTMLTBE;
            ADD_OP (rip, op);
        } else if (strcmp (toks[0], "caption") == 0) {
            CHECK_ARGN (line, tokm, 8); // does a 'continue' on error

            memset (&op, 0, sizeof (RIop_t));
            op.type = RI_OP_HTMLTBCB;
            op.u.htb.lab = toks[1];
            op.u.htb.fn = toks[2];
            op.u.htb.fs = toks[3];
            op.u.htb.bg = toks[4];
            op.u.htb.cl = toks[5];
            ADD_OP (rip, op);

            if (toks[6][0]) {
                memset (&op, 0, sizeof (RIop_t));
                op.type = RI_OP_HTMLAB;
                op.u.ha.url = toks[6];
                op.u.ha.info = toks[7];
                op.u.ha.fn = toks[2];
                op.u.ha.fs = toks[3];
                op.u.ha.bg = toks[4];
                op.u.ha.cl = toks[5];
                ADD_OP (rip, op);

                memset (&op, 0, sizeof (RIop_t));
                op.type = RI_OP_HTMLAE;
                ADD_OP (rip, op);
            }

            memset (&op, 0, sizeof (RIop_t));
            op.type = RI_OP_HTMLTBCE;
            ADD_OP (rip, op);
        } else if (strcmp (toks[0], "rowbegin") == 0) {
            CHECK_ARGN (line, tokm, 5); // does a 'continue' on error

            memset (&op, 0, sizeof (RIop_t));
            op.type = RI_OP_HTMLTRB;
            op.u.htr.fn = toks[1];
            op.u.htr.fs = toks[2];
            op.u.htr.bg = toks[3];
            op.u.htr.cl = toks[4];
            ADD_OP (rip, op);
        } else if (strcmp (toks[0], "rowend") == 0) {
            CHECK_ARGN (line, tokm, 1); // does a 'continue' on error

            memset (&op, 0, sizeof (RIop_t));
            op.type = RI_OP_HTMLTRE;
            ADD_OP (rip, op);
        } else if (strcmp (toks[0], "cell") == 0) {
            CHECK_ARGN (line, tokm, 10); // does a 'continue' on error

            memset (&op, 0, sizeof (RIop_t));
            op.type = RI_OP_HTMLTDB;
            op.u.htd.fn = toks[2];
            op.u.htd.fs = toks[3];
            op.u.htd.bg = toks[4];
            op.u.htd.cl = toks[5];
            op.u.htd.j = atoi (toks[6]);
            op.u.htd.cn = atoi (toks[7]);
            ADD_OP (rip, op);

            if (toks[8][0]) {
                memset (&op, 0, sizeof (RIop_t));
                op.type = RI_OP_HTMLAB;
                op.u.ha.url = toks[8];
                op.u.ha.info = toks[9];
                op.u.ha.fn = toks[2];
                op.u.ha.fs = toks[3];
                op.u.ha.bg = toks[4];
                op.u.ha.cl = toks[5];
                ADD_OP (rip, op);
            }

            memset (&op, 0, sizeof (RIop_t));
            op.type = RI_OP_HTMLTDT;
            op.u.htd.lab = toks[1];
            ADD_OP (rip, op);

            if (toks[8][0]) {
                memset (&op, 0, sizeof (RIop_t));
                op.type = RI_OP_HTMLAE;
                ADD_OP (rip, op);
            }

            memset (&op, 0, sizeof (RIop_t));
            op.type = RI_OP_HTMLTDE;
            ADD_OP (rip, op);
        } else {
            SUwarning (0, "vg_dq_vt_tool", "unknown directive %s", line);
        }
    }
}
