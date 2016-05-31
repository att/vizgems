#pragma prototyped

#include <ast.h>
#include <tm.h>
#include <regex.h>
#include <vmalloc.h>
#include <swift.h>
#include <ctype.h>
#include "vg_hdr.h"
#include "vgfilter_pub.h"
#include "vg_sevmap.c"
#include "sl_level_map.c"
#include "sl_inv_map1.c"
#include "sl_inv_nodeattr.c"

typedef struct n2i_s {
    char *name;
    int id;
} n2i_t;

static n2i_t modemap[] = {
    { VG_ALARM_S_MODE_DROP,  VG_ALARM_N_MODE_DROP  },
    { VG_ALARM_S_MODE_KEEP,  VG_ALARM_N_MODE_KEEP  },
    { VG_ALARM_S_MODE_DEFER, VG_ALARM_N_MODE_DEFER },
    { NULL,                  0                     },
};

static n2i_t repmap[] = {
    { VG_ALARM_S_REP_ONCE,    VG_ALARM_N_REP_ONCE    },
    { VG_ALARM_S_REP_DAILY,   VG_ALARM_N_REP_DAILY   },
    { VG_ALARM_S_REP_WEEKLY,  VG_ALARM_N_REP_WEEKLY  },
    { VG_ALARM_S_REP_MONTHLY, VG_ALARM_N_REP_MONTHLY },
    { NULL,                   0                      },
};

static char remap[256];

static filter_t *filters;
static int filtern, filterl, filterm;

static inv_t *ins;
static int inn, inl;

static char cpkey[S_key], nkey[S_key], vgnam[S_val];

#define ICMPFLAGS (REG_NULL | REG_EXTENDED | REG_LEFT | REG_RIGHT)
#define ICMP(p, i, s) (regexec (&p[i].code, s, 0, NULL, ICMPFLAGS) == 0)

static filter_t *filterlist (char *, char **, int);
static int mapn2i (n2i_t *, char *);
static int cmp (const void *, const void *);

int filterload (char *filterfile, char *accountfile) {
    Sfio_t *ffp, *afp;
    char *line;
    int ln;
    char *s1, *s2, *s3, *s4, *s5, *s6, *s7, *s8, *s9, *s10, *s11, *s12, *s13;
    char *s;
    filter_t *filterp;
    inv_t *inp;
    int ini, inj;

    sl_level_mapopen (getenv ("LEVELMAPFILE"));
    sl_inv_map1open (getenv ("INVMAPFILE"));
    sl_inv_nodeattropen (getenv ("INVNODEATTRFILE"));
    M1I (TRUE);
    memset (cpkey, 0, S_key);
    strcpy (cpkey, "custpriv");
    memset (nkey, 0, S_key);
    strcpy (nkey, "name");

    remap['['] = '.';
    remap[']'] = '.';
    remap['('] = '.';
    remap[')'] = '.';
    remap['{'] = '.';
    remap['}'] = '.';
    remap['^'] = '.';
    remap['$'] = '.';
    remap['|'] = '.';
    remap['\\'] = '.';

    ins = NULL;
    inn = inl = 0;

    if (!(afp = sfopen (NULL, accountfile, "r"))) {
        SUwarning (0, "filterload", "cannot open account data file");
        return -1;
    }
    while ((line = sfgetr (afp, '\n', 1))) {
        if (!(s1 = strchr (line, '|'))) {
            SUwarning (0, "filterload", "bad line: %s", line);
            return -1;
        }
        *s1++ = 0;
        if (!(s2 = strchr (s1, '|'))) {
            SUwarning (0, "filterload", "bad line: %s", s1);
            return -1;
        }
        *s2++ = 0;
        if (!(s3 = strchr (s2, '|'))) {
            SUwarning (0, "filterload", "bad line: %s", s2);
            return -1;
        }
        *s3++ = 0;

        if (inl == inn) {
            if (!(ins = vmresize (
                Vmheap, ins, sizeof (inv_t) * (inn + 128), VM_RSCOPY
            ))) {
                SUwarning (0, "filterload", "cannot grow ins");
                inl = inn = 0;
                return -1;
            }
            inn += 128;
        }
        inp = &ins[inl++];
        memset (inp, 0, sizeof (inv_t));

        if (!(inp->account = vmstrdup (Vmheap, line))) {
            SUwarning (0, "filterload", "cannot allocate account id: %s", line);
            return -1;
        }
        strncpy (inp->level, s2, S_level1);
        if (!(inp->idre = vmstrdup (Vmheap, s3))) {
            SUwarning (0, "filterload", "cannot allocate id: %s", s3);
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

    filters = NULL;
    filtern = filterl = 0;
    if (!(ffp = sfopen (NULL, filterfile, "r"))) {
        SUwarning (0, "filterload", "cannot open filter file");
        return -1;
    }
    ln = 0;
    while ((line = sfgetr (ffp, '\n', 1))) {
        ln++;
        s1 = line;
        if (!(s2 = strstr (s1, "|+|"))) {
            SUwarning (0, "filterload", "missing alarm id, line: %d", ln);
            continue;
        }
        *s2 = 0, s2 += 3;
        if (!(s3 = strstr (s2, "|+|"))) {
            SUwarning (0, "filterload", "missing msg_text, line: %d", ln);
            continue;
        }
        *s3 = 0, s3 += 3;
        if (!(s4 = strstr (s3, "|+|"))) {
            SUwarning (0, "filterload", "missing start time, line: %d", ln);
            continue;
        }
        *s4 = 0, s4 += 3;
        if (!(s5 = strstr (s4, "|+|"))) {
            SUwarning (0, "filterload", "missing end time, line: %d", ln);
            continue;
        }
        *s5 = 0, s5 += 3;
        if (!(s6 = strstr (s5, "|+|"))) {
            SUwarning (0, "filterload", "missing start date, line: %d", ln);
            continue;
        }
        *s6 = 0, s6 += 3;
        if (!(s7 = strstr (s6, "|+|"))) {
            SUwarning (0, "filterload", "missing end date, line: %d", ln);
            continue;
        }
        *s7 = 0, s7 += 3;
        if (!(s8 = strstr (s7, "|+|"))) {
            SUwarning (0, "filterload", "missing repeat mode, line: %d", ln);
            continue;
        }
        *s8 = 0, s8 += 3;
        if (!(s9 = strstr (s8, "|+|"))) {
            SUwarning (0, "filterload", "missing ticket mode, line: %d", ln);
            continue;
        }
        *s9 = 0, s9 += 3;
        if (!(s10 = strstr (s9, "|+|"))) {
            SUwarning (0, "filterload", "missing viz mode, line: %d", ln);
            continue;
        }
        *s10 = 0, s10 += 3;
        if (!(s11 = strstr (s10, "|+|"))) {
            SUwarning (0, "filterload", "missing severity, line: %d", ln);
            continue;
        }
        *s11 = 0, s11 += 3;
        if (!(s12 = strstr (s11, "|+|"))) {
            SUwarning (0, "filterload", "missing comment, line: %d", ln);
            continue;
        }
        *s12 = 0, s12 += 3;
        if (!(s13 = strstr (s12, "|+|"))) {
            SUwarning (0, "filterload", "missing account, line: %d", ln);
            continue;
        }
        *s13 = 0, s13 += 3;

        for (ini = 0, inj = -2; ini < inl; ini++) {
            inp = &ins[ini];
            if (
                s13[0] == inp->account[0] && strcmp (s13, inp->account) == 0
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
            SUwarning (0, "filterload", "account not found: %s", s13);
            continue;
        }

        if (filterl == filtern) {
            if (!(filters = vmresize (
                Vmheap, filters, sizeof (filter_t) * (filtern + 128), VM_RSCOPY
            ))) {
                SUwarning (0, "filterload", "cannot grow filters");
                filterl = filtern = 0;
                return -1;
            }
            filtern += 128;
        }
        filterp = &filters[filterl];
        memset (filterp, 0, sizeof (filter_t));
        filterp->tm = filterp->sm = -1;
        filterp->sevnum = -1;

        if (strncasecmp (s1, "name=", 5) == 0)
            filterp->nameflag = TRUE, s1 += 5;
        else
            filterp->nameflag = FALSE;
        if (!*s1)
            s1 = ".*";
        filterp->objreflag = FALSE;
        filterp->objcn = 0;
        for (s = s1; *s; s++) {
            if (!isalnum (*s) && *s != '_')
                filterp->objreflag = TRUE;
            else
                filterp->objcn++;
        }
        if (!(filterp->objstr = vmstrdup (Vmheap, s1))) {
            SUwarning (0, "filterload", "cannot allocate obj str");
            return -1;
        }
        if (filterp->objreflag) {
            if (regcomp (
                &filterp->objcode, filterp->objstr,
                REG_NULL | REG_EXTENDED | REG_LEFT | REG_RIGHT | REG_ICASE
            ) != 0) {
                SUwarning (
                    0, "filterload",
                    "failed to compile regex for object, line: %d", ln
                );
                continue;
            }
        }

        if (!*s2)
            s2 = ".*";
        filterp->idreflag = FALSE;
        filterp->idcn = 0;
        for (s = s2; *s; s++) {
            if (!isalnum (*s) && *s != '_')
                filterp->idreflag = TRUE;
            else
                filterp->idcn++;
        }
        if (!(filterp->idstr = vmstrdup (Vmheap, s2))) {
            SUwarning (0, "filterload", "malloc failed for idstr");
            return -1;
        }
        if (filterp->idreflag) {
            if (regcomp (
                &filterp->idcode, filterp->idstr,
                REG_NULL | REG_EXTENDED | REG_LEFT | REG_RIGHT
            ) != 0) {
                SUwarning (
                    0, "filterload",
                    "failed to compile regex for alarm id, line: %d", ln
                );
                continue;
            }
        }

        if (!*s3)
            s3 = ".*";
        filterp->textreflag = FALSE;
        filterp->textcn = 0;
        for (s = s3; *s; s++) {
            if (remap[*s] && (s == s3 || *(s - 1) != '\\'))
                *s = remap[*s];
            else if (*s == '*' && (s == s3 || *(s - 1) != '.'))
                *s = '.';
            else if (*s == '\\' && *(s + 1) == 0)
                *s = '.';
            if (!isalnum (*s) && *s != '_')
                filterp->textreflag = TRUE;
            else
                filterp->textcn++;
        }
        if (!(filterp->textstr = vmstrdup (Vmheap, s3))) {
            SUwarning (0, "filterload", "cannot allocate text str");
            return -1;
        }
        if (filterp->textreflag) {
            if (regcomp (
                &filterp->textcode, filterp->textstr, REG_NULL | REG_EXTENDED
            ) != 0) {
                SUwarning (
                    0, "filterload",
                    "failed to compile regex for message text, line: %d", ln
                );
                continue;
            }
        }

        filterp->st = strtol (s4, &s, 10);
        if (s == s4 || (*s != 0 && *s != ' ')) {
            SUwarning (0, "filterload", "no or bad start time %s", s4);
            continue;
        }
        filterp->et = strtol (s5, &s, 10);
        if (s == s5 || (*s != 0 && *s != ' ')) {
            SUwarning (0, "filterload", "no or bad end time %s", s5);
            continue;
        }

        filterp->sdate = strtol (s6, &s, 10);
        if (s == s6 || (*s != 0 && *s != ' ')) {
            SUwarning (0, "filterload", "no or bad start date %s", s6);
            continue;
        }
        if (strcmp (s7, "indefinitely") == 0)
            filterp->edate = 0;
        else {
            filterp->edate = strtol (s7, &s, 10);
            if (s == s7 || (*s != 0 && *s != ' ')) {
                SUwarning (0, "filterload", "no or bad end date %s", s7);
                continue;
            }
        }

        if ((filterp->repeat = mapn2i (repmap, s8)) == -1) {
            SUwarning (
                0, "filterload", "cannot map repeat mode %s", s8
            );
            continue;
        }

        if ((filterp->tm = mapn2i (modemap, s9)) == -1) {
            SUwarning (
                0, "filterload", "cannot map ticket mode %s", s9
            );
            continue;
        }
        if ((filterp->sm = mapn2i (modemap, s10)) == -1) {
            SUwarning (
                0, "filterload", "cannot map viz mode %s", s10
            );
            continue;
        }

        if (*s11 && (filterp->sevnum = sevmapfind (s11)) == -1) {
            SUwarning (
                0, "filterload", "cannot map severity %s", s11
            );
            continue;
        }

        if (!(filterp->com = vmstrdup (Vmheap, s12))) {
            SUwarning (0, "filterload", "cannot allocate comment str");
            return -1;
        }

        if (!(filterp->account = vmstrdup (Vmheap, s13))) {
            SUwarning (0, "filterload", "cannot allocate account str");
            return -1;
        }
        filterp->ini = inj;

        filterl++;
    }
    qsort (filters, filterl, sizeof (filter_t), cmp);
    sfclose (ffp);
    return 0;
}

filter_t *filterfind (
    char *level, char *vgobj, char *tkobj, char *id, char *text, int tim
) {
    filter_t *filterp;
    int filteri;
    inv_t *inp;
    int ini;
    sl_inv_nodeattr_t *cpval, *nval;
    int vmatch, tmatch, nflag;
    time_t t, ct, st, et, dt;
    Tm_t tm1, tm2, tmc, *tmp;
    char *m1p;

    vgnam[0] = 0, nflag = FALSE;
    ct = tim;
    for (filteri = 0; filteri < filterl; filteri++) {
        filterp = &filters[filteri];

        if (filterp->nameflag && !nflag) {
            if ((nval = sl_inv_nodeattrfind (level, vgobj, nkey))) {
                strcpy (vgnam, nval->sl_val);
                nflag = TRUE;
            }
        }
        if (filterp->objreflag) {
            if (filterp->nameflag) {
                vmatch = (regexec (
                    &filterp->objcode, vgnam, 0, NULL,
                    REG_NULL | REG_EXTENDED | REG_LEFT | REG_RIGHT | REG_ICASE
                ) == 0);
            } else {
                vmatch = (regexec (
                    &filterp->objcode, vgobj, 0, NULL,
                    REG_NULL | REG_EXTENDED | REG_LEFT | REG_RIGHT | REG_ICASE
                ) == 0);
            }
            if (vgobj == tkobj)
                tmatch = vmatch;
            else
                tmatch = (regexec (
                    &filterp->objcode, tkobj, 0, NULL,
                    REG_NULL | REG_EXTENDED | REG_LEFT | REG_RIGHT | REG_ICASE
                ) == 0);
            if (!tmatch && !vmatch)
                continue;
        } else {
            if (filterp->nameflag)
                vmatch = (strcasecmp (filterp->objstr, vgnam) == 0);
            else
                vmatch = (strcasecmp (filterp->objstr, vgobj) == 0);
            if (vgobj == tkobj)
                tmatch = vmatch;
            else
                tmatch = (strcasecmp (filterp->objstr, tkobj) == 0);
            if (!tmatch && !vmatch)
                continue;
        }

        if (filterp->idreflag) {
            if (regexec (
                &filterp->idcode, id, 0, NULL,
                REG_NULL | REG_EXTENDED | REG_LEFT | REG_RIGHT
            ) != 0)
                continue;
        } else {
            if (strcmp (filterp->idstr, id) != 0)
                continue;
        }

        if (filterp->textreflag) {
            if (regexec (
                &filterp->textcode, text, 0, NULL,
                REG_NULL | REG_EXTENDED
            ) != 0)
                continue;
        } else {
            if (!strstr (text, filterp->textstr))
                continue;
        }

        st = filterp->st * 60;
        et = filterp->et * 60;
        dt = et - st;

        if (filterp->repeat == VG_ALARM_N_REP_ONCE) {
            if (filterp->edate == filterp->sdate && dt < 0)
                dt += (24 * 60 * 60);
            if (ct < filterp->sdate + st)
                continue;
            if (filterp->edate > 0 && ct > filterp->edate + st + dt)
                continue;
        } else {
            if (dt < 0)
                dt += (24 * 60 * 60);
            if (ct < filterp->sdate + st || (
                filterp->edate > 0 && ct > filterp->edate + st + dt
            ))
                continue;
            t = filterp->sdate + st;
            tmp = tmmake (&t), tm1 = *tmp;
            t = filterp->sdate + st + dt;
            tmp = tmmake (&t), tm2 = *tmp;
            tmp = tmmake (&ct), tmc = *tmp;
            t = tmc.tm_hour * 3600 + tmc.tm_min * 60;
            switch (filterp->repeat) {
            case VG_ALARM_N_REP_DAILY:
                if (et >= st) {
                    if (t >= st && t <= st + dt)
                        goto match;
                } else if (t >= st || t <= et)
                    goto match;
                break;
            case VG_ALARM_N_REP_WEEKLY:
                if (tmc.tm_wday == tm1.tm_wday && t >= st && t <= st + dt)
                    goto match;
                else if (
                    tm1.tm_wday != tm2.tm_wday &&
                    tmc.tm_wday == tm2.tm_wday && t <= et
                )
                    goto match;
                break;
            case VG_ALARM_N_REP_MONTHLY:
                if (tmc.tm_mday == tm1.tm_mday && t >= st && t <= st + dt)
                    goto match;
                else if (
                    tm1.tm_mday != tm2.tm_mday &&
                    tmc.tm_mday == tm2.tm_mday && t <= et
                )
                    goto match;
                break;
            }
            continue;
        }

match:
        for (ini = filterp->ini; ini > -1 && ini < inl; ini++) {
            inp = &ins[ini];
            if (
                filterp->account[0] != inp->account[0] ||
                strcmp (filterp->account, inp->account) != 0
            )
                break;
            if (inp->idre[0] == '_' && strcmp (inp->idre, "__ALL__") == 0)
                continue;
            for (
                m1p = M1F (level, vgobj, inp->level); m1p;
                m1p = M1N (level, vgobj, inp->level)
            ) {
                if (inp->reflag) {
                    if (!inp->compflag && regcomp (
                        &inp->code, inp->idre, ICMPFLAGS
                    ) != 0) {
                        SUwarning (
                            0, "filterfind",
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
            if (!(cpval = sl_inv_nodeattrfind (level, vgobj, cpkey)))
                continue;
            if (!strchr (cpval->sl_val, 'F'))
                goto nomatch;
        }
        SUwarning (
            1, "filterfind", "matched\naccount=%s\nobj=%s\nmsg=%s\n",
            filterp->account, vgobj, text
        );
        return filterp;

nomatch:
        ;
    }
    return NULL;
}

filter_t *filterlistfirst (char *level, char *objs[], int objn) {
    filterm = 0;
    return filterlist (level, objs, objn);
}

filter_t *filterlistnext (char *level, char *objs[], int objn) {
    return filterlist (level, objs, objn);
}

static filter_t *filterlist (char *level, char *objs[], int objn) {
    filter_t *filterp;
    inv_t *inp;
    int ini;
    sl_inv_nodeattr_t *cpval, *nval;
    char lv[VG_alarm_level1_L], id[VG_alarm_id1_L];
    int obji;
    char *m1p;

    memset (lv, 0, VG_alarm_level1_L), strcpy (lv, level);
    memset (id, 0, VG_alarm_id1_L), strcpy (id, objs[0]);

    for (; filterm < filterl; filterm++) {
        filterp = &filters[filterm];

        if (filterp->objreflag) {
            for (obji = 0; obji < objn; obji++) {
                if (filterp->nameflag) {
                    nval = sl_inv_nodeattrfind (level, objs[obji], nkey);
                    if (nval && regexec (
                        &filterp->objcode, nval->sl_val, 0, NULL,
                        REG_NULL | REG_EXTENDED | REG_LEFT | REG_RIGHT |
                        REG_ICASE
                    ) == 0)
                        break;
                } else {
                    if (regexec (
                        &filterp->objcode, objs[obji], 0, NULL,
                        REG_NULL | REG_EXTENDED | REG_LEFT | REG_RIGHT |
                        REG_ICASE
                    ) == 0)
                        break;
                }
            }
            if (obji == objn)
                continue;
        } else {
            for (obji = 0; obji < objn; obji++) {
                if (filterp->nameflag) {
                    nval = sl_inv_nodeattrfind (level, objs[obji], nkey);
                    if (nval && strcasecmp (filterp->objstr, nval->sl_val) == 0)
                        break;
                } else
                    if (strcasecmp (filterp->objstr, objs[obji]) == 0)
                        break;
            }
            if (obji == objn)
                continue;
        }

        for (ini = filterp->ini; ini > -1 && ini < inl; ini++) {
            inp = &ins[ini];
            if (
                filterp->account[0] != inp->account[0] ||
                strcmp (filterp->account, inp->account) != 0
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
                            0, "filterlist",
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
            if (!strchr (cpval->sl_val, 'F'))
                goto nomatch;
        }
        filterm++;
        return filterp;

nomatch:
        ;
    }
    return NULL;
}

static int mapn2i (n2i_t *maps, char *name) {
    int mapi;

    for (mapi = 0; maps[mapi].name; mapi++)
        if (strcasecmp (maps[mapi].name, name) == 0)
            return maps[mapi].id;
    return -1;
}

static int cmp (const void *a, const void *b) {
    filter_t *ap, *bp;

    ap = (filter_t *) a;
    bp = (filter_t *) b;
    if (ap->objcn != bp->objcn)
        return bp->objcn - ap->objcn;
    if (ap->idcn != bp->idcn)
        return bp->idcn - ap->idcn;
    return bp->textcn - ap->textcn;
}
