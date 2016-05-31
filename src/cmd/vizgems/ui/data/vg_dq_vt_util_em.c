#include <ast.h>
#include <swift.h>
#include "xml.h"
#include "vg_dq_vt_util.h"

RIop_t *EMops;
int EMopn, EMopm;

static Dt_t *imagedict;
static Dtdisc_t imagedisc = {
    sizeof (Dtlink_t), -1,
    0, NULL, NULL, NULL, NULL, NULL, NULL
};
static Dt_t *nodedict;
static Dtdisc_t nodedisc = {
    sizeof (Dtlink_t), SZ_level + SZ_id,
    0, NULL, NULL, NULL, NULL, NULL, NULL
};
static Dt_t *edgedict;
static Dtdisc_t edgedisc = {
    sizeof (Dtlink_t), 2 * (SZ_level + SZ_id),
    0, NULL, NULL, NULL, NULL, NULL, NULL
};

static int load (char *, char *);
static int recparse (
    XMLnode_t *, EMrec_t *, int, EMrec_t *, int, int, EMbb_t *
);

int EMinit (char *file) {
    Sfio_t *fp;
    char *line, *s1;

    EMops = NULL;
    EMopn = EMopm = 0;
    if (!(imagedict = dtopen (&imagedisc, Dtset))) {
        SUwarning (0, "EMinit", "cannot create imagedict");
        return -1;
    }
    if (!(nodedict = dtopen (&nodedisc, Dtset))) {
        SUwarning (0, "EMinit", "cannot create nodedict");
        return -1;
    }
    if (!(edgedict = dtopen (&edgedisc, Dtset))) {
        SUwarning (0, "EMinit", "cannot create edgedict");
        return -1;
    }

    if (!(fp = sfopen (NULL, file, "r")))
        SUerror ("EMinit", "cannot open file %s", file);
    while ((line = sfgetr (fp, '\n', 1))) {
        if (!(s1 = strchr (line, '|'))) {
            SUwarning (0, "EMinit", "bad line: %s", line);
            continue;
        }
        *s1++ = 0;
        if (load (line, s1) == -1)
            SUerror ("EMinit", "cannot load embed file %s", line);
    }
    sfclose (fp);

    return 0;
}

int EMparsenode (
    char *level, char *id, char *str, int dir, int *opip, int *opnp, EMbb_t *bbp
) {
    EMnode_t n, *np;
    XMLnode_t *xp;
    EMrec_t *recs, *drecs;
    int recm, drecm;

    if (!(xp = XMLreadstring (str, "em", TRUE, Vmheap))) {
        SUwarning (0, "EMparsenode", "cannot parse xml spec");
        return 0;
    }
    memset (&n, 0, sizeof (EMnode_t));
    memcpy (n.level, level, SZ_level);
    memcpy (n.id, id, SZ_id);
    if ((np = dtsearch (nodedict, &n)))
        recs = np->recs, recm = np->recm;
    else
        recs = NULL, recm = 0;
    memset (&n, 0, sizeof (EMnode_t));
    strncpy (n.level, "_D_", SZ_level);
    strncpy (n.id, "__DEFAULT__", SZ_id);
    if ((np = dtsearch (nodedict, &n)))
        drecs = np->recs, drecm = np->recm;
    else
        drecs = NULL, drecm = 0;
    *opip = EMopm;
    if (recparse (xp, recs, recm, drecs, drecm, dir, bbp) == -1) {
        SUwarning (0, "EMparsenode", "cannot parse spec");
        return -1;
    }
    *opnp = EMopm - *opip;
    return 0;
}

int EMparseedge (
    char *level1, char *id1, char *level2, char *id2,
    char *str, int dir, int *opip, int *opnp, EMbb_t *bbp
) {
    EMedge_t e, *ep;
    XMLnode_t *xp;
    EMrec_t *recs, *drecs;
    int recm, drecm;

    if (!(xp = XMLreadstring (str, "em", TRUE, Vmheap))) {
        SUwarning (0, "EMparseedge", "cannot parse xml spec");
        return 0;
    }
    memset (&e, 0, sizeof (EMedge_t));
    memcpy (e.level1, level1, SZ_level);
    memcpy (e.id1, id1, SZ_id);
    memcpy (e.level2, level2, SZ_level);
    memcpy (e.id2, id2, SZ_id);
    if ((ep = dtsearch (edgedict, &e)))
        recs = ep->recs, recm = ep->recm;
    else
        recs = NULL, recm = 0;
    memset (&e, 0, sizeof (EMedge_t));
    memcpy (e.level1, "_D_", SZ_level);
    memcpy (e.id1, "__DEFAULT__", SZ_id);
    memcpy (e.level2, "_D_", SZ_level);
    memcpy (e.id2, "__DEFAULT__", SZ_id);
    if ((ep = dtsearch (edgedict, &e)))
        drecs = ep->recs, drecm = ep->recm;
    else
        drecs = NULL, drecm = 0;
    *opip = EMopm;
    if (recparse (xp, recs, recm, drecs, drecm, dir, bbp) == -1) {
        SUwarning (0, "EMparseedge", "cannot parse spec");
        return -1;
    }
    *opnp = EMopm - *opip;
    return 0;
}

static int load (char *id, char *file) {
    EMnode_t n, *np, *nmem;
    EMedge_t e, *ep, *emem;
    EMrec_t *rp, **rsp;
    int *rnp, *rmp;
    Sfio_t *fp;
    char *line, *s1, *s2, *s3, *s4, *s5, *s6, *s7;

    if (!(fp = sfopen (NULL, file, "r"))) {
        SUwarning (0, "load", "cannot open embed info file");
        return 0;
    }
    while ((line = sfgetr (fp, '\n', 1))) {
        np = NULL;
        ep = NULL;
        if (!(s1 = strchr (line, '|'))) {
            SUwarning (0, "load", "bad line(1): %s", line);
            continue;
        }
        *s1++ = 0;
        if (line[0] == 'N' || line[0] == 'n') {
            if (!(s2 = strchr (s1, '|')) || !(s3 = strchr (s2 + 1, '|'))) {
                SUwarning (0, "load", "bad line(2): %s", s1);
                continue;
            }
            *s2++ = 0, *s3++ = 0;
            memset (&n, 0, sizeof (EMnode_t));
            strcpy (n.level, s1);
            strcpy (n.id, s2);
            if (!(np = dtsearch (nodedict, &n))) {
                if (!(nmem = vmalloc (Vmheap, sizeof (EMnode_t)))) {
                    SUwarning (0, "load", "cannot allocate nmem");
                    return -1;
                }
                memset (nmem, 0, sizeof (EMnode_t));
                strcpy (nmem->level, s1);
                strcpy (nmem->id, s2);
                if (!(np = dtinsert (nodedict, nmem))) {
                    SUwarning (0, "load", "cannot insert node");
                    vmfree (Vmheap, nmem);
                    return -1;
                }
            }
            s1 = s3;
            rsp = &np->recs, rnp = &np->recn, rmp = &np->recm;
        } else if (line[0] == 'E' || line[0] == 'e') {
            if (!(s2 = strchr (s1, '|')) || !(s3 = strchr (s2 + 1, '|'))) {
                SUwarning (0, "load", "bad line(3): %s", s1);
                continue;
            }
            *s2++ = *s3++ = 0;
            if (!(s4 = strchr (s3, '|')) || !(s5 = strchr (s4 + 1, '|'))) {
                SUwarning (0, "load", "bad line(4): %s", s3);
                continue;
            }
            *s4++ = 0, *s5++ = 0;
            memset (&e, 0, sizeof (EMedge_t));
            strcpy (e.level1, s1);
            strcpy (e.id1, s2);
            strcpy (e.level2, s3);
            strcpy (e.id2, s4);
            if (!(ep = dtsearch (edgedict, &e))) {
                if (!(emem = vmalloc (Vmheap, sizeof (EMedge_t)))) {
                    SUwarning (0, "load", "cannot allocate emem");
                    return -1;
                }
                memset (emem, 0, sizeof (EMedge_t));
                strcpy (emem->level1, s1);
                strcpy (emem->id1, s2);
                strcpy (emem->level2, s3);
                strcpy (emem->id2, s4);
                if (!(ep = dtinsert (edgedict, emem))) {
                    SUwarning (0, "load", "cannot insert edge");
                    vmfree (Vmheap, emem);
                    return -1;
                }
            }
            s1 = s5;
            rsp = &ep->recs, rnp = &ep->recn, rmp = &ep->recm;
        }
        if (!(s2 = strchr (s1, '|'))) {
            SUwarning (0, "load", "bad line(5): %s", s1);
            continue;
        }
        *s2++ = 0;
        if (*rmp >= *rnp) {
            if (!(*rsp = vmresize (
                Vmheap, *rsp, (*rnp + 5) * sizeof (EMrec_t), VM_RSCOPY
            ))) {
                SUwarning (0, "load", "cannot grow recs array");
                return -1;
            }
            *rnp += 5;
        }
        rp = &(*rsp)[*rmp], (*rmp)++;
        if (!(rp->id = vmstrdup (Vmheap, id))) {
            SUwarning (0, "load", "cannot copy id");
            return -1;
        }
        if (s1[0] == 'I' || s1[0] == 'i') {
            if (!(s3 = strchr (s2, '|'))) {
                SUwarning (0, "load", "bad line(6): %s", s2);
                continue;
            }
            if (!(s4 = strchr (s3 + 1, '|')) || !(s5 = strchr (s4 + 1, '|'))) {
                SUwarning (0, "load", "bad line(7): %s", s3);
                continue;
            }
            if (!(s6 = strchr (s5 + 1, '|')) || !(s7 = strchr (s6 + 1, '|'))) {
                SUwarning (0, "load", "bad line(8): %s", s5);
                continue;
            }
            *s3++ = *s4++ = *s5++ = *s6++ = *s7++ = 0;
            rp->type = EM_TYPE_I;
            if (!(rp->u.i.file = vmstrdup (Vmheap, s2))) {
                SUwarning (0, "load", "cannot copy file");
                return -1;
            }
            rp->u.i.x1 = atoi (s3), rp->u.i.y1 = atoi (s4);
            rp->u.i.x2 = atoi (s5), rp->u.i.y2 = atoi (s6);
        } else if (s1[0] == 'S' || s1[0] == 's') {
            if (!(s3 = strchr (s2, '|'))) {
                SUwarning (0, "load", "bad line(9): %s", s2);
                continue;
            }
            if (!(s4 = strchr (s3 + 1, '|')) || !(s5 = strchr (s4 + 1, '|'))) {
                SUwarning (0, "load", "bad line(10): %s", s3);
                continue;
            }
            if (!(s6 = strchr (s5 + 1, '|')) || !(s7 = strchr (s6 + 1, '|'))) {
                SUwarning (0, "load", "bad line(11): %s", s5);
                continue;
            }
            *s3++ = *s4++ = *s5++ = *s6++ = *s7++ = 0;
            rp->type = EM_TYPE_S;
            if (
                !(rp->u.s.fn = vmstrdup (Vmheap, s2)) ||
                !(rp->u.s.fs = vmstrdup (Vmheap, s3)) ||
                !(rp->u.s.flcl = vmstrdup (Vmheap, s4)) ||
                !(rp->u.s.lncl = vmstrdup (Vmheap, s5)) ||
                !(rp->u.s.fncl = vmstrdup (Vmheap, s6))
            ) {
                SUwarning (0, "load", "cannot copy style");
                return -1;
            }
        }
        if (!(rp->im = vmstrdup (Vmheap, s7))) {
            SUwarning (0, "load", "cannot copy image map");
            return -1;
        }
    }
    sfclose (fp);
    return 0;
}

static int recparse (
    XMLnode_t *xp, EMrec_t *crecs, int crecn, EMrec_t *drecs, int drecn,
    int dir, EMbb_t *bbp
) {
    EMrec_t *recs, *recp;
    int reci, recj, recn, ri;
    RIop_t *opp, top;
    int opi, opk, opl;
    EMbb_t cbb;
    EMimage_t *ip, *imem;
    XMLnode_t *cp, *pp, *tp, *fnp, *fsp, *clp;
    char *fs, *fn, *cl;
    int ci;
    int dw, dh;

    if (xp->type == XML_TYPE_TEXT) {
        SUwarning (0, "recparse", "called with text node");
        return -1;
    }

    opk = EMopm;
    bbp->w = bbp->h = 0;
    for (ci = 0; ci < xp->nodem; ci++) {
        cp = xp->nodes[ci];
        if (cp->type == XML_TYPE_TEXT)
            continue;
        if (strcmp (cp->tag, "v") == 0 || strcmp (cp->tag, "h") == 0) {
            opl = EMopm;
            if (recparse (
                cp, crecs, crecn, drecs, drecn,
                (cp->tag[0] == 'v') ? EM_DIR_V : EM_DIR_H, &cbb
            ) == -1) {
                SUwarning (0, "recparse", "cannot parse v list");
                return -1;
            }
            if (dir == EM_DIR_H) {
                for (opi = opl; opi < EMopm; opi++) {
                    if (EMops[opi].type == RI_OP_I) {
                        EMops[opi].u.img.rx += bbp->w;
                        EMops[opi].u.img.ry += (bbp->h - cbb.h) / 2;
                    } else if (EMops[opi].type == RI_OP_T) {
                        EMops[opi].u.text.p.x += bbp->w;
                        EMops[opi].u.text.p.y += (bbp->h - cbb.h) / 2;
                    }
                }
                bbp->w += cbb.w;
                if (bbp->h < cbb.h) {
                    dh = cbb.h - bbp->h;
                    bbp->h = cbb.h;
                    for (opi = opk; opi < EMopm; opi++) {
                        if (EMops[opi].type == RI_OP_I)
                            EMops[opi].u.img.ry += dh / 2;
                        else if (EMops[opi].type == RI_OP_T)
                            EMops[opi].u.text.p.y += dh / 2;
                    }
                }
            } else {
                for (opi = opl; opi < EMopm; opi++) {
                    if (EMops[opi].type == RI_OP_I) {
                        EMops[opi].u.img.ry += bbp->h;
                        EMops[opi].u.img.rx += (bbp->w - cbb.w) / 2;
                    } else if (EMops[opi].type == RI_OP_T) {
                        EMops[opi].u.text.p.y += bbp->h;
                        EMops[opi].u.text.p.x += (bbp->w - cbb.w) / 2;
                    }
                }
                bbp->h += cbb.h;
                if (bbp->w < cbb.w) {
                    dw = cbb.w - bbp->w;
                    bbp->w = cbb.w;
                    for (opi = opk; opi < EMopm; opi++) {
                        if (EMops[opi].type == RI_OP_I)
                            EMops[opi].u.img.rx += dw / 2;
                        else if (EMops[opi].type == RI_OP_T)
                            EMops[opi].u.text.p.x += dw / 2;
                    }
                }
            }
        } else if (strcmp (cp->tag, "rimg") == 0) {
            if (!(tp = XMLfind (cp, "path", XML_TYPE_TEXT, -1, TRUE))) {
                SUwarning (0, "recparse", "cannot find rimg/path");
                continue;
            }
            if (!(ip = dtmatch (imagedict, tp->text))) {
                if (!(imem = vmalloc (Vmheap, sizeof (EMimage_t)))) {
                    SUwarning (0, "recparse", "cannot allocate imem");
                    return -1;
                }
                memset (imem, 0, sizeof (EMimage_t));
                if (!(imem->file = vmstrdup (Vmheap, tp->text))) {
                    SUwarning (0, "recparse", "cannot copy image file");
                    return -1;
                }
                if (!(ip = dtinsert (imagedict, imem))) {
                    SUwarning (0, "recparse", "cannot insert image");
                    vmfree (Vmheap, imem->file);
                    vmfree (Vmheap, imem);
                    return -1;
                }
                if (RIloadimage (ip->file, &ip->op) == -1) {
                    SUwarning (0, "recparse", "cannot load image");
                    continue;
                }
            }

            if (ip->op.type == RI_OP_NOOP) {
                if ((pp = XMLfind (cp, "alt", XML_TYPE_TAG, -1, TRUE))) {
                    if (recparse (
                        pp, crecs, crecn, drecs, drecn, dir, &cbb
                    ) == -1) {
                        SUwarning (0, "recparse", "cannot parse alt tag");
                        return -1;
                    }
                    if (dir == EM_DIR_H) {
                        bbp->w += cbb.w;
                        if (bbp->h < cbb.h) {
                            dh = cbb.h - bbp->h;
                            bbp->h = cbb.h;
                        }
                    } else {
                        bbp->h += cbb.h;
                        if (bbp->w < cbb.w) {
                            dw = cbb.w - bbp->w;
                            bbp->w = cbb.w;
                        }
                    }
                }
            } else {
                if (EMopm >= EMopn) {
                    if (!(EMops = vmresize (
                        Vmheap, EMops, (EMopn + 100) * sizeof (RIop_t),
                        VM_RSCOPY
                    ))) {
                        SUwarning (0, "recparse", "cannot grow EMops array");
                        return -1;
                    }
                    EMopn += 100;
                }
                opp = &EMops[EMopm++];
                *opp = ip->op;
                if (dir == EM_DIR_H) {
                    opp->u.img.rx = bbp->w + EM_MARGIN_W / 2;
                    opp->u.img.ry = (bbp->h - opp->u.img.ih) / 2;
                    bbp->w += opp->u.img.iw + EM_MARGIN_W;
                    if (bbp->h < opp->u.img.ih + EM_MARGIN_H) {
                        dh = opp->u.img.ih + EM_MARGIN_H - bbp->h;
                        bbp->h = opp->u.img.ih + EM_MARGIN_H;
                        for (opi = opk; opi < EMopm; opi++) {
                            if (EMops[opi].type == RI_OP_I)
                                EMops[opi].u.img.ry += dh / 2;
                            else if (EMops[opi].type == RI_OP_T)
                                EMops[opi].u.text.p.y += dh / 2;
                        }
                    }
                } else {
                    opp->u.img.rx = (bbp->w - opp->u.img.iw) / 2;
                    opp->u.img.ry = bbp->h + EM_MARGIN_H / 2;
                    if (bbp->w < opp->u.img.iw + EM_MARGIN_W) {
                        dw = opp->u.img.iw + EM_MARGIN_W - bbp->w;
                        bbp->w = opp->u.img.iw + EM_MARGIN_W;
                        for (opi = opk; opi < EMopm; opi++) {
                            if (EMops[opi].type == RI_OP_I)
                                EMops[opi].u.img.rx += dw / 2;
                            else if (EMops[opi].type == RI_OP_T)
                                EMops[opi].u.text.p.x += dw / 2;
                        }
                    }
                    bbp->h += opp->u.img.ih + EM_MARGIN_H;
                }
            }
        } else if (strcmp (cp->tag, "qimg") == 0) {
            if (!(tp = XMLfind (cp, "path", XML_TYPE_TEXT, -1, TRUE))) {
                SUwarning (0, "recparse", "cannot find qimg/path");
                continue;
            }
            for (ri = 0; ri < 2; ri++) {
                if (ri == 0)
                    recs = crecs, recn = crecn;
                else
                    recs = drecs, recn = drecn;
                recj = -1;
                for (reci = 0; reci < recn; reci++) {
                    recp = &recs[reci];
                    if (strcmp (recp->id, tp->text) != 0)
                        continue;

                    if (recp->type == EM_TYPE_I) {
                        recj = reci;
                        if (!(ip = dtmatch (imagedict, recp->u.i.file))) {
                            if (!(imem = vmalloc (
                                Vmheap, sizeof (EMimage_t)
                            ))) {
                                SUwarning (
                                    0, "recparse", "cannot allocate imem"
                                );
                                return -1;
                            }
                            memset (imem, 0, sizeof (EMimage_t));
                            if (!(imem->file = vmstrdup (
                                Vmheap, recp->u.i.file
                            ))) {
                                SUwarning (
                                    0, "recparse", "cannot copy image file"
                                );
                                return -1;
                            }
                            if (!(ip = dtinsert (imagedict, imem))) {
                                SUwarning (
                                    0, "recparse", "cannot insert image"
                                );
                                vmfree (Vmheap, imem->file);
                                vmfree (Vmheap, imem);
                                return -1;
                            }
                            if (RIloadimage (ip->file, &ip->op) == -1) {
                                SUwarning (0, "recparse", "cannot load image");
                                continue;
                            }
                        }
                        if (EMopm >= EMopn) {
                            if (!(EMops = vmresize (
                                Vmheap, EMops, (EMopn + 100) * sizeof (RIop_t),
                                VM_RSCOPY
                            ))) {
                                SUwarning (
                                    0, "recparse", "cannot grow EMops array"
                                );
                                return -1;
                            }
                            EMopn += 100;
                        }
                        opp = &EMops[EMopm++];
                        *opp = ip->op;
                        opp->u.img.ix = recp->u.i.x1;
                        opp->u.img.iy = recp->u.i.y1;
                        opp->u.img.iw = recp->u.i.x2 - recp->u.i.x1 + 1;
                        opp->u.img.ih = recp->u.i.y2 - recp->u.i.y1 + 1;
                        if (dir == EM_DIR_H) {
                            opp->u.img.rx = bbp->w + EM_MARGIN_W / 2;
                            opp->u.img.ry = (bbp->h - opp->u.img.ih) / 2;
                            bbp->w += opp->u.img.iw + EM_MARGIN_W;
                            if (bbp->h < opp->u.img.ih + EM_MARGIN_H) {
                                dh = opp->u.img.ih + EM_MARGIN_H - bbp->h;
                                bbp->h = opp->u.img.ih + EM_MARGIN_H;
                                for (opi = opk; opi < EMopm; opi++) {
                                    if (EMops[opi].type == RI_OP_I)
                                        EMops[opi].u.img.ry += dh / 2;
                                    else if (EMops[opi].type == RI_OP_T)
                                        EMops[opi].u.text.p.y += dh / 2;
                                }
                            }
                        } else {
                            opp->u.img.rx = (bbp->w - opp->u.img.iw) / 2;
                            opp->u.img.ry = bbp->h + EM_MARGIN_H / 2;
                            if (bbp->w < opp->u.img.iw + EM_MARGIN_W) {
                                dw = opp->u.img.iw + EM_MARGIN_W - bbp->w;
                                bbp->w = opp->u.img.iw + EM_MARGIN_W;
                                for (opi = opk; opi < EMopm; opi++) {
                                    if (EMops[opi].type == RI_OP_I)
                                        EMops[opi].u.img.rx += dw / 2;
                                    else if (EMops[opi].type == RI_OP_T)
                                        EMops[opi].u.text.p.x += dw / 2;
                                }
                            }
                            bbp->h += opp->u.img.ih + EM_MARGIN_H;
                        }
                    } else if (recp->type == EM_TYPE_S) {
                        recj = reci;
                        if (EMopm + 3 >= EMopn) {
                            if (!(EMops = vmresize (
                                Vmheap, EMops, (EMopn + 100) * sizeof (RIop_t),
                                VM_RSCOPY
                            ))) {
                                SUwarning (
                                    0, "recparse", "cannot grow EMops array"
                                );
                                return -1;
                            }
                            EMopn += 100;
                        }
                        if (recp->u.s.lncl && recp->u.s.lncl[0]) {
                            opp = &EMops[EMopm++];
                            opp->type = RI_OP_COPYc;
                            opp->u.color.n = 1;
                            opp->u.color.t = recp->u.s.lncl;
                        }
                        if (recp->u.s.fncl && recp->u.s.fncl[0]) {
                            opp = &EMops[EMopm++];
                            opp->type = RI_OP_COPYFC;
                            opp->u.color.n = 1;
                            opp->u.color.t = recp->u.s.fncl;
                        }
                        if (recp->u.s.flcl && recp->u.s.flcl[0]) {
                            opp = &EMops[EMopm++];
                            opp->type = RI_OP_COPYC;
                            opp->u.color.n = 1;
                            opp->u.color.t = recp->u.s.flcl;
                        }
                    }
                }
                if (recj != -1)
                    break;
            }
        } else if (strcmp (cp->tag, "label") == 0) {
            if (!(fnp = XMLfind (cp, "fontname", XML_TYPE_TEXT, -1, TRUE)))
                fn = "abc";
            else
                fn = fnp->text;
            if (!(fsp = XMLfind (cp, "fontsize", XML_TYPE_TEXT, -1, TRUE)))
                fs = "10";
            else
                fs = fsp->text;
            if (!(clp = XMLfind (cp, "color", XML_TYPE_TEXT, -1, TRUE)))
                cl = NULL;
            else
                cl = clp->text;
            if (!(tp = XMLfind (cp, "text", XML_TYPE_TEXT, -1, TRUE)))
                continue;
            if (RIgettextsize (tp->text, fn, fs, &top) == -1) {
                SUwarning (0, "recparse", "cannot get text size");
                return -1;
            }
            top.u.font.w += 8, top.u.font.h += 4;
            if (EMopm + 3 >= EMopn) {
                if (!(EMops = vmresize (
                    Vmheap, EMops, (EMopn + 100) * sizeof (RIop_t), VM_RSCOPY
                ))) {
                    SUwarning (0, "recparse", "cannot grow EMops array");
                    return -1;
                }
                EMopn += 100;
            }
            opp = &EMops[EMopm++];
            *opp = top;
            opp->type = RI_OP_F;
            if (cl) {
                opp = &EMops[EMopm++];
                memset (opp, 0, sizeof (RIop_t));
                opp->type = RI_OP_FC;
                if (!(opp->u.color.t = vmstrdup (Vmheap, cl))) {
                    SUwarning (0, "recparse", "cannot copy color");
                    return -1;
                }
            }
            opp = &EMops[EMopm++];
            memset (opp, 0, sizeof (RIop_t));
            opp->type = RI_OP_T;
            opp->u.text.jx = 0;
            opp->u.text.jy = -1;
            opp->u.text.w = top.u.font.w - 8;
            opp->u.text.h = top.u.font.h - 4;
            if (!(opp->u.text.t = vmstrdup (Vmheap, tp->text))) {
                SUwarning (0, "recparse", "cannot copy text");
                return -1;
            }
            if (dir == EM_DIR_H) {
                opp->u.text.p.x = bbp->w + top.u.font.w / 2 + 4;
                opp->u.text.p.y = (bbp->h - top.u.font.h) / 2 + 2;
                bbp->w += top.u.font.w + EM_MARGIN_W;
                if (bbp->h < top.u.font.h + EM_MARGIN_H) {
                    dh = top.u.font.h + EM_MARGIN_H - bbp->h;
                    bbp->h = top.u.font.h + EM_MARGIN_H;
                    for (opi = opk; opi < EMopm; opi++) {
                        if (EMops[opi].type == RI_OP_I)
                            EMops[opi].u.img.ry += dh / 2;
                        else if (EMops[opi].type == RI_OP_T)
                            EMops[opi].u.text.p.y += dh / 2;
                    }
                }
            } else {
                opp->u.text.p.x = bbp->w / 2 + 0;
                opp->u.text.p.y = bbp->h + EM_MARGIN_H / 2 + 2;
                if (bbp->w < top.u.font.w + EM_MARGIN_W) {
                    dw = top.u.font.w + EM_MARGIN_W - bbp->w;
                    bbp->w = top.u.font.w + EM_MARGIN_W;
                    for (opi = opk; opi < EMopm; opi++) {
                        if (EMops[opi].type == RI_OP_I)
                            EMops[opi].u.img.rx += dw / 2;
                        else if (EMops[opi].type == RI_OP_T)
                            EMops[opi].u.text.p.x += dw / 2;
                    }
                }
                bbp->h += top.u.font.h + EM_MARGIN_H;
            }
        }
    }

    return 0;
}
