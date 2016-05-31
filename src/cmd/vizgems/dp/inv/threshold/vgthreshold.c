#pragma prototyped

#include <ast.h>
#include <regex.h>
#include <vmalloc.h>
#include <swift.h>
#include <ctype.h>
#include "vg_hdr.h"
#include "vgthreshold_pub.h"
#include "sl_level_map.c"
#include "sl_inv_map1.c"
#include "sl_inv_nodeattr.c"

static threshold_t *thresholds;
static int thresholdn, thresholdl, thresholdm;

static inv_t *ins;
static int inn, inl;

static char *psname;

static char clevel[S_level], cpkey[S_key], nkey[S_key], vgnam[S_val];

#define ICMPFLAGS (REG_NULL | REG_EXTENDED | REG_LEFT | REG_RIGHT)
#define ICMP(p, i, s) (regexec (&p[i].code, s, 0, NULL, ICMPFLAGS) == 0)

static threshold_t *thresholdfind (char *, char *);
static threshold_t *thresholdlist (char *, char *);
static int cmp (const void *, const void *);

int thresholdload (char *thresholdfile, char *accountfile) {
    Sfio_t *ffp, *afp;
    char *line;
    int ln;
    char *s1, *s2, *s3, *s4, *s5;
    char *s;
    threshold_t *thresholdp;
    inv_t *inp;
    int ini, inj;

    strcpy (clevel, "c");

    sl_level_mapopen (getenv ("LEVELMAPFILE"));
    sl_inv_map1open (getenv ("INVMAPFILE"));
    sl_inv_nodeattropen (getenv ("INVNODEATTRFILE"));
    M1I (TRUE);
    memset (cpkey, 0, S_key);
    strcpy (cpkey, "custpriv");
    memset (nkey, 0, S_key);
    strcpy (nkey, "name");

    ins = NULL;
    inn = inl = 0;

    if (!(afp = sfopen (NULL, accountfile, "r"))) {
        SUwarning (0, "thresholdload", "cannot open account data file");
        return -1;
    }
    while ((line = sfgetr (afp, '\n', 1))) {
        if (!(s1 = strchr (line, '|'))) {
            SUwarning (0, "thresholdload", "bad line: %s", line);
            return -1;
        }
        *s1++ = 0;
        if (!(s2 = strchr (s1, '|'))) {
            SUwarning (0, "thresholdload", "bad line: %s", s1);
            return -1;
        }
        *s2++ = 0;
        if (!(s3 = strchr (s2, '|'))) {
            SUwarning (0, "thresholdload", "bad line: %s", s2);
            return -1;
        }
        *s3++ = 0;

        if (inl == inn) {
            if (!(ins = vmresize (
                Vmheap, ins, sizeof (inv_t) * (inn + 128), VM_RSCOPY
            ))) {
                SUwarning (0, "thresholdload", "cannot grow ins");
                inl = inn = 0;
                return -1;
            }
            inn += 128;
        }
        inp = &ins[inl++];
        memset (inp, 0, sizeof (inv_t));

        if (!(inp->account = vmstrdup (Vmheap, line))) {
            SUwarning (
                0, "thresholdload", "cannot allocate account id: %s", line
            );
            return -1;
        }
        strncpy (inp->level, s2, S_level);
        if (!(inp->idre = vmstrdup (Vmheap, s3))) {
            SUwarning (0, "thresholdload", "cannot allocate id: %s", s3);
            return -1;
        }
        inp->acctype = atoi (s1);

        if (strcmp (s3, "__ALL__") != 0)
            for (s = s3; *s; s++)
                if (!isalnum (*s) && *s != '_') {
                    inp->reflag = TRUE;
                    break;
                }
    }
    sfclose (afp);

    thresholds = NULL;
    thresholdn = thresholdl = 0;
    if (!(ffp = sfopen (NULL, thresholdfile, "r"))) {
        SUwarning (0, "thresholdload", "cannot open threshold file");
        return -1;
    }
    ln = 0;
    while ((line = sfgetr (ffp, '\n', 1))) {
        ln++;
        s1 = line;
        if (!(s2 = strstr (s1, "|"))) {
            SUwarning (0, "thresholdload", "missing stat name, line: %d", ln);
            continue;
        }
        *s2 = 0, s2++;
        if (!(s3 = strstr (s2, "|"))) {
            SUwarning (0, "thresholdload", "missing stat status, line: %d", ln);
            continue;
        }
        *s3 = 0, s3++;
        if (!(s4 = strstr (s3, "|"))) {
            SUwarning (0, "thresholdload", "missing stat value, line: %d", ln);
            continue;
        }
        *s4 = 0, s4++;
        if (!(s5 = strstr (s4, "|"))) {
            SUwarning (0, "thresholdload", "missing account, line: %d", ln);
            continue;
        }
        *s5 = 0, s5++;

        for (ini = 0, inj = -2; ini < inl; ini++) {
            inp = &ins[ini];
            if (
                s5[0] == inp->account[0] && strcmp (s5, inp->account) == 0
            ) {
                inj = ini;
                if (strcmp (inp->idre, "__ALL__") == 0)
                    inj = -1;
                else
                    break;
            } else if (inj != -2)
                break;
        }
        if (inj == -2) {
            SUwarning (0, "thresholdload", "account not found: %s", s5);
            continue;
        }

        if (thresholdl == thresholdn) {
            if (!(thresholds = vmresize (
                Vmheap, thresholds, sizeof (threshold_t) * (thresholdn + 128),
                VM_RSCOPY
            ))) {
                SUwarning (0, "thresholdload", "cannot grow thresholds");
                thresholdl = thresholdn = 0;
                return -1;
            }
            thresholdn += 128;
        }
        thresholdp = &thresholds[thresholdl];
        memset (thresholdp, 0, sizeof (threshold_t));

        VG_urldec (s1, s1, strlen (s1) + 1);
        if (strncasecmp (s1, "name=", 5) == 0)
            thresholdp->nameflag = TRUE, s1 += 5;
        else
            thresholdp->nameflag = FALSE;
        if (!*s1)
            s1 = ".*";
        thresholdp->idreflag = FALSE;
        thresholdp->idcn = 0;
        for (s = s1; *s; s++) {
            if (!isalnum (*s) && *s != '_')
                thresholdp->idreflag = TRUE;
            else
                thresholdp->idcn++;
        }
        if (!(thresholdp->idstr = vmstrdup (Vmheap, s1))) {
            SUwarning (0, "thresholdload", "cannot allocate id str");
            return -1;
        }
        if (thresholdp->idreflag) {
            if (regcomp (
                &thresholdp->idcode, thresholdp->idstr,
                REG_NULL | REG_EXTENDED | REG_LEFT | REG_RIGHT | REG_ICASE
            ) != 0) {
                SUwarning (
                    0, "thresholdload",
                    "failed to compile regex for idect, line: %d", ln
                );
                continue;
            }
        }

        if (!(thresholdp->sname = vmstrdup (Vmheap, s2))) {
            SUwarning (0, "thresholdload", "cannot allocate stat name");
            return -1;
        }

        if (!(thresholdp->sstatus = vmstrdup (Vmheap, s3))) {
            SUwarning (0, "thresholdload", "cannot allocate stat status");
            return -1;
        }

        if (!(thresholdp->svalue = vmstrdup (Vmheap, s4))) {
            SUwarning (0, "thresholdload", "cannot allocate stat value");
            return -1;
        }

        if (!(thresholdp->account = vmstrdup (Vmheap, s5))) {
            SUwarning (0, "thresholdload", "cannot allocate account str");
            return -1;
        }
        thresholdp->ini = inj;

        thresholdl++;
    }
    qsort (thresholds, thresholdl, sizeof (threshold_t), cmp);
    sfclose (ffp);
    return 0;
}

threshold_t *thresholdfindfirst (char *level, char *id) {
    thresholdm = 0;
    psname = NULL;
    return thresholdfind (level, id);
}

threshold_t *thresholdfindnext (char *level, char *id) {
    return thresholdfind (level, id);
}

static threshold_t *thresholdfind (char *level, char *id) {
    threshold_t *thresholdp;
    inv_t *inp;
    int ini;
    sl_inv_nodeattr_t *cpval, *nval;
    int nflag;
    char *m1p;

    vgnam[0] = 0, nflag = FALSE;

again:
    for (; thresholdm < thresholdl; thresholdm++) {
        thresholdp = &thresholds[thresholdm];

        if (thresholdp->nameflag && !nflag) {
            if ((nval = sl_inv_nodeattrfind (level, id, nkey))) {
                strcpy (vgnam, nval->sl_val);
                nflag = TRUE;
            }
        }
        if (thresholdp->idreflag) {
            if (thresholdp->nameflag) {
                if (regexec (
                    &thresholdp->idcode, vgnam, 0, NULL,
                    REG_NULL | REG_EXTENDED | REG_LEFT | REG_RIGHT | REG_ICASE
                ) != 0)
                    continue;
            } else {
                if (regexec (
                    &thresholdp->idcode, id, 0, NULL,
                    REG_NULL | REG_EXTENDED | REG_LEFT | REG_RIGHT | REG_ICASE
                ) != 0)
                    continue;
            }
        } else {
            if (thresholdp->nameflag) {
                if (strcasecmp (thresholdp->idstr, vgnam) != 0)
                    continue;
            } else {
                if (strcasecmp (thresholdp->idstr, id)  != 0)
                    continue;
            }
        }

match:
        for (ini = thresholdp->ini; ini > -1 && ini < inl; ini++) {
            inp = &ins[ini];
            if (
                thresholdp->account[0] != inp->account[0] ||
                strcmp (thresholdp->account, inp->account) != 0
            )
                break;
            if (inp->idre[0] == '_' && strcmp (inp->idre, "__ALL__") == 0)
                continue;
            for (
                m1p = M1F (level, id, inp->level); m1p;
                m1p = M1N (level, id, inp->level)
            ) {
                if (inp->reflag) {
                    if (!inp->compflag && regcomp (
                        &inp->code, inp->idre, ICMPFLAGS
                    ) != 0) {
                        SUwarning (
                            0, "thresholdfind",
                            "cannot compile regex %s", inp->idre
                        );
                        return NULL;
                    }
                    inp->compflag = TRUE;
                    if (ICMP (ins, ini, m1p))
                        break;
                } else {
                    if (strcmp (inp->idre, m1p) == 0)
                        break;
                }
            }
            if (!m1p)
                goto nomatch;
            if (inp->acctype == VG_ACCOUNT_TYPE_ATT)
                continue;
            if (!(cpval = sl_inv_nodeattrfind (level, id, cpkey)))
                continue;
            if (!strchr (cpval->sl_val, 'T'))
                goto nomatch;
        }
        SUwarning (
            1, "thresholdfind", "matched\naccount=%s\nid=%s\n",
            thresholdp->account, id
        );
        if (!(m1p = M1F (level, id, clevel)))
            goto nomatch;
        thresholdp->cid = m1p;
        if (psname && strcmp (psname, thresholdp->sname) == 0)
            goto nomatch;
        psname = thresholdp->sname;
        thresholdm++;
        return thresholdp;
    }
    return NULL;

nomatch:
    if (++thresholdm < thresholdl)
        goto again;

    return NULL;
}

threshold_t *thresholdlistfirst (char *level, char *obj) {
    thresholdm = 0;
    psname = NULL;
    return thresholdlist (level, obj);
}

threshold_t *thresholdlistnext (char *level, char *obj) {
    return thresholdlist (level, obj);
}

static threshold_t *thresholdlist (char *level, char *obj) {
    threshold_t *thresholdp;
    inv_t *inp;
    int ini;
    sl_inv_nodeattr_t *cpval, *nval;
    int nflag;
    char lv[VG_alarm_level1_L], id[VG_alarm_id1_L];
    char *m1p;

    vgnam[0] = 0, nflag = FALSE;
    memset (lv, 0, VG_alarm_level1_L), strcpy (lv, level);
    memset (id, 0, VG_alarm_id1_L), strcpy (id, obj);

again:
    for (; thresholdm < thresholdl; thresholdm++) {
        thresholdp = &thresholds[thresholdm];

        if (thresholdp->nameflag && !nflag) {
            if ((nval = sl_inv_nodeattrfind (level, obj, nkey))) {
                strcpy (vgnam, nval->sl_val);
                nflag = TRUE;
            }
        }
        if (thresholdp->idreflag) {
            if (thresholdp->nameflag) {
                if (regexec (
                    &thresholdp->idcode, vgnam, 0, NULL,
                    REG_NULL | REG_EXTENDED | REG_LEFT | REG_RIGHT | REG_ICASE
                ) != 0)
                    continue;
            } else {
                if (regexec (
                    &thresholdp->idcode, obj, 0, NULL,
                    REG_NULL | REG_EXTENDED | REG_LEFT | REG_RIGHT | REG_ICASE
                ) != 0)
                    continue;
            }
        } else {
            if (thresholdp->nameflag) {
                if (strcasecmp (thresholdp->idstr, vgnam) != 0)
                    continue;
            } else {
                if (strcasecmp (thresholdp->idstr, obj) != 0)
                    continue;
            }
        }

        for (ini = thresholdp->ini; ini > -1 && ini < inl; ini++) {
            inp = &ins[ini];
            if (
                thresholdp->account[0] != inp->account[0] ||
                strcmp (thresholdp->account, inp->account) != 0
            )
                break;
            if (inp->idre[0] == '_' && strcmp (inp->idre, "__ALL__") == 0)
                continue;
            for (
                m1p = M1F (lv, id, inp->level); m1p;
                m1p = M1N (lv, id, inp->level)
            ) {
                if (inp->reflag) {
                    if (!inp->compflag && regcomp (
                        &inp->code, inp->idre, ICMPFLAGS
                    ) != 0) {
                        SUwarning (
                            0, "thresholdlist",
                            "cannot compile regex %s", inp->idre
                        );
                        return NULL;
                    }
                    inp->compflag = TRUE;
                    if (ICMP (ins, ini, m1p))
                        break;
                } else {
                    if (strcmp (inp->idre, m1p) == 0)
                        break;
                }
            }
            if (!m1p)
                goto nomatch;
            if (inp->acctype == VG_ACCOUNT_TYPE_ATT)
                continue;
            if (!(cpval = sl_inv_nodeattrfind (lv, id, cpkey)))
                continue;
            if (!strchr (cpval->sl_val, 'T'))
                goto nomatch;
        }
        if (psname && strcmp (psname, thresholdp->sname) == 0)
            goto nomatch;
        psname = thresholdp->sname;
        thresholdm++;
        return thresholdp;
    }
    return NULL;

nomatch:
    if (++thresholdm < thresholdl)
        goto again;

    return NULL;
}

static int cmp (const void *a, const void *b) {
    threshold_t *ap, *bp;
    int ret;

    ap = (threshold_t *) a;
    bp = (threshold_t *) b;
    if ((ret = strcmp (ap->sname, bp->sname)) != 0)
        return ret;
    return bp->idcn - ap->idcn;
}
