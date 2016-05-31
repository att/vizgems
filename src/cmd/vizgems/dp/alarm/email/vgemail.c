#pragma prototyped

#include <ast.h>
#include <tm.h>
#include <regex.h>
#include <vmalloc.h>
#include <swift.h>
#include <ctype.h>
#include "vgemail_pub.h"
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

static n2i_t stylemap[] = {
    { "sms",  STYLE_SMS  },
    { "text", STYLE_TEXT },
    { "html", STYLE_HTML },
    { NULL,   0          },
};

static char remap[256];

static email_t *emails;
static int emailn, emaill, emailm;

static inv_t *ins;
static int inn, inl;

static char cpkey[S_key], nkey[S_key], vgnam[S_val];

#define ICMPFLAGS (REG_NULL | REG_EXTENDED | REG_LEFT | REG_RIGHT)
#define ICMP(p, i, s) (regexec (&p[i].code, s, 0, NULL, ICMPFLAGS) == 0)

static email_t *emailfind (char *, char *, char *, char *, int, int, int);
static email_t *emaillist (char *, char *);
static int mapn2i (n2i_t *, char *);
static int cmp (const void *, const void *);

int emailload (char *emailfile, char *accountfile) {
    Sfio_t *efp, *afp;
    char *line;
    int ln;
    char *s1, *s2, *s3, *s4, *s5, *s6, *s7, *s8;
    char *s9, *s10, *s11, *s12, *s13, *s14, *s15, *s;
    email_t *emailp;
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

    if (!(afp = sfopen (NULL, accountfile, "r")))
        SUerror ("emailload", "cannot open account data file");
    while ((line = sfgetr (afp, '\n', 1))) {
        if (!(s1 = strchr (line, '|'))) {
            SUwarning (0, "emailload", "bad line: %s", line);
            return -1;
        }
        *s1++ = 0;
        if (!(s2 = strchr (s1, '|'))) {
            SUwarning (0, "emailload", "bad line: %s", s1);
            return -1;
        }
        *s2++ = 0;
        if (!(s3 = strchr (s2, '|'))) {
            SUwarning (0, "emailload", "bad line: %s", s2);
            return -1;
        }
        *s3++ = 0;

        if (inl == inn) {
            if (!(ins = vmresize (
                Vmheap, ins, sizeof (inv_t) * (inn + 128), VM_RSCOPY
            ))) {
                SUwarning (0, "emailload", "cannot grow ins");
                inl = inn = 0;
                return -1;
            }
            inn += 128;
        }
        inp = &ins[inl++];
        memset (inp, 0, sizeof (inv_t));

        if (!(inp->account = vmstrdup (Vmheap, line))) {
            SUwarning (0, "emailload", "cannot allocate account id: %s", line);
            return -1;
        }
        strncpy (inp->level, s2, S_level1);
        if (!(inp->idre = vmstrdup (Vmheap, s3))) {
            SUwarning (0, "emailload", "cannot allocate id: %s", s3);
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

    emails = NULL;
    emailn = emaill = 0;

    if (!(efp = sfopen (NULL, emailfile, "r"))) {
        SUwarning (0, "emailload", "cannot open email data file");
        return -1;
    }
    ln = 0;
    while ((line = sfgetr (efp, '\n', 1))) {
        ln++;
        s1 = line;
        if (!(s2 = strstr (s1, "|+|"))) {
            SUwarning (0, "emailload", "missing alarm id, line: %d", ln);
            continue;
        }
        *s2 = 0, s2 += 3;
        if (!(s3 = strstr (s2, "|+|"))) {
            SUwarning (0, "emailload", "missing msg_text, line: %d", ln);
            continue;
        }
        *s3 = 0, s3 += 3;
        if (!(s4 = strstr (s3, "|+|"))) {
            SUwarning (0, "emailload", "missing ticket mode, line: %d", ln);
            continue;
        }
        *s4 = 0, s4 += 3;
        if (!(s5 = strstr (s4, "|+|"))) {
            SUwarning (0, "emailload", "missing severity, line: %d", ln);
            continue;
        }
        *s5 = 0, s5 += 3;
        if (!(s6 = strstr (s5, "|+|"))) {
            SUwarning (0, "emailload", "missing start time, line: %d", ln);
            continue;
        }
        *s6 = 0, s6 += 3;
        if (!(s7 = strstr (s6, "|+|"))) {
            SUwarning (0, "emailload", "missing end time, line: %d", ln);
            continue;
        }
        *s7 = 0, s7 += 3;
        if (!(s8 = strstr (s7, "|+|"))) {
            SUwarning (0, "emailload", "missing start date, line: %d", ln);
            continue;
        }
        *s8 = 0, s8 += 3;
        if (!(s9 = strstr (s8, "|+|"))) {
            SUwarning (0, "emailload", "missing end date, line: %d", ln);
            continue;
        }
        *s9 = 0, s9 += 3;
        if (!(s10 = strstr (s9, "|+|"))) {
            SUwarning (0, "emailload", "missing repeat mode, line: %d", ln);
            continue;
        }
        *s10 = 0, s10 += 3;
        if (!(s11 = strstr (s10, "|+|"))) {
            SUwarning (0, "emailload", "missing from address, line: %d", ln);
            continue;
        }
        *s11 = 0, s11 += 3;
        if (!(s12 = strstr (s11, "|+|"))) {
            SUwarning (0, "emailload", "missing to addresses, line: %d", ln);
            continue;
        }
        *s12 = 0, s12 += 3;
        if (!(s13 = strstr (s12, "|+|"))) {
            SUwarning (0, "emailload", "missing style, line: %d", ln);
            continue;
        }
        *s13 = 0, s13 += 3;
        if (!(s14 = strstr (s13, "|+|"))) {
            SUwarning (0, "emailload", "missing subject, line: %d", ln);
            continue;
        }
        *s14 = 0, s14 += 3;
        if (!(s15 = strstr (s14, "|+|"))) {
            SUwarning (0, "emailload", "missing account, line: %d", ln);
            continue;
        }
        *s15 = 0, s15 += 3;

        for (ini = 0, inj = -2; ini < inl; ini++) {
            inp = &ins[ini];
            if (
                s15[0] == inp->account[0] && strcmp (s15, inp->account) == 0
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
            SUwarning (0, "emailload", "account not found: %s", s15);
            continue;
        }

        if (emaill == emailn) {
            if (!(emails = vmresize (
                Vmheap, emails, sizeof (email_t) * (emailn + 128), VM_RSCOPY
            ))) {
                SUwarning (0, "emailload", "cannot grow emails");
                emaill = emailn = 0;
                return -1;
            }
            emailn += 128;
        }
        emailp = &emails[emaill];
        memset (emailp, 0, sizeof (email_t));

        if (strncasecmp (s1, "name=", 5) == 0)
            emailp->nameflag = TRUE, s1 += 5;
        else
            emailp->nameflag = FALSE;
        if (!*s1)
            s1 = ".*";
        emailp->objreflag = FALSE;
        emailp->objcn = 0;
        for (s = s1; *s; s++) {
            if (!isalnum (*s) && *s != '_')
                emailp->objreflag = TRUE;
            else
                emailp->objcn++;
        }
        if (!(emailp->objstr = vmstrdup (Vmheap, s1))) {
            SUwarning (0, "emailload", "cannot allocate obj str");
            return -1;
        }
        if (emailp->objreflag) {
            if (regcomp (
                &emailp->objcode, emailp->objstr,
                REG_NULL | REG_EXTENDED | REG_LEFT | REG_RIGHT | REG_ICASE
            ) != 0) {
                SUwarning (
                    0, "emailload",
                    "failed to compile regex for object, line: %d", ln
                );
                continue;
            }
        }

        if (!*s2)
            s2 = ".*";
        emailp->idreflag = FALSE;
        emailp->idcn = 0;
        for (s = s2; *s; s++) {
            if (!isalnum (*s) && *s != '_')
                emailp->idreflag = TRUE;
            else
                emailp->idcn++;
        }
        if (!(emailp->idstr = vmstrdup (Vmheap, s2))) {
            SUwarning (0, "emailload", "malloc failed for idstr");
            return -1;
        }
        if (emailp->idreflag) {
            if (regcomp (
                &emailp->idcode, emailp->idstr,
                REG_NULL | REG_EXTENDED | REG_LEFT | REG_RIGHT
            ) != 0) {
                SUwarning (
                    0, "emailload",
                    "failed to compile regex for alarm id, line: %d", ln
                );
                continue;
            }
        }

        if (!*s3)
            s3 = ".*";
        emailp->textreflag = FALSE;
        emailp->textcn = 0;
        for (s = s3; *s; s++) {
            if (remap[*s] && (s == s3 || *(s - 1) != '\\'))
                *s = remap[*s];
            else if (*s == '*' && (s == s3 || *(s - 1) != '.'))
                *s = '.';
            else if (*s == '\\' && *(s + 1) == 0)
                *s = '.';
            if (!isalnum (*s) && *s != '_')
                emailp->textreflag = TRUE;
            else
                emailp->textcn++;
        }
        if (!(emailp->textstr = vmstrdup (Vmheap, s3))) {
            SUwarning (0, "emailload", "cannot allocate text str");
            return -1;
        }
        if (emailp->textreflag) {
            if (regcomp (
                &emailp->textcode, emailp->textstr, REG_NULL | REG_EXTENDED
            ) != 0) {
                SUwarning (
                    0, "emailload",
                    "failed to compile regex for message text, line: %d", ln
                );
                continue;
            }
        }

        emailp->tmode = -1;
        if (*s4 && (emailp->tmode = mapn2i (modemap, s4)) == -1) {
            SUwarning (
                0, "emailload", "cannot map ticket mode %s", s4
            );
            continue;
        }

        emailp->sevnum = -1;
        if (*s5 && (emailp->sevnum = sevmapfind (s5)) == -1) {
            SUwarning (
                0, "emailload", "cannot map severity %s", s5
            );
            continue;
        }

        emailp->st = strtol (s6, &s, 10);
        if (s == s6 || (*s != 0 && *s != ' ')) {
            SUwarning (0, "emailload", "no or bad start time %s", s6);
            continue;
        }
        emailp->et = strtol (s7, &s, 10);
        if (s == s7 || (*s != 0 && *s != ' ')) {
            SUwarning (0, "emailload", "no or bad end time %s", s7);
            continue;
        }

        emailp->sdate = strtol (s8, &s, 10);
        if (s == s8 || (*s != 0 && *s != ' ')) {
            SUwarning (0, "emailload", "no or bad start date %s", s8);
            continue;
        }
        if (strcmp (s9, "indefinitely") == 0)
            emailp->edate = 0;
        else {
            emailp->edate = strtol (s9, &s, 10);
            if (s == s9 || (*s != 0 && *s != ' ')) {
                SUwarning (0, "emailload", "no or bad end date %s", s9);
                continue;
            }
        }

        if ((emailp->repeat = mapn2i (repmap, s10)) == -1) {
            SUwarning (
                0, "emailload", "cannot map repeat mode %s", s10
            );
            continue;
        }

        if (!(emailp->faddr = vmstrdup (Vmheap, s11))) {
            SUwarning (0, "emailload", "cannot allocate from address str");
            return -1;
        }

        if (!(emailp->taddr = vmstrdup (Vmheap, s12))) {
            SUwarning (0, "emailload", "cannot allocate to address str");
            return -1;
        }

        if ((emailp->style = mapn2i (stylemap, s13)) == -1) {
            SUwarning (0, "emailload", "cannot allocate style str");
            continue;
        }

        if (!(emailp->subject = vmstrdup (Vmheap, s14))) {
            SUwarning (0, "emailload", "cannot allocate subject str");
            return -1;
        }

        if (!(emailp->account = vmstrdup (Vmheap, s15))) {
            SUwarning (0, "emailload", "cannot allocate account str");
            return -1;
        }
        emailp->ini = inj;

        emaill++;
    }
    qsort (emails, emaill, sizeof (email_t), cmp);
    sfclose (efp);

    return 0;
}

email_t *emailfindfirst (
    char *level, char *obj, char *id, char *text, int tmode, int sev, int tim
) {
    emailm = 0;
    return emailfind (level, obj, id, text, tmode, sev, tim);
}

email_t *emailfindnext (
    char *level, char *obj, char *id, char *text, int tmode, int sev, int tim
) {
    return emailfind (level, obj, id, text, tmode, sev, tim);
}

static email_t *emailfind (
    char *level, char *obj, char *id, char *text, int tmode, int sev, int tim
) {
    email_t *emailp;
    inv_t *inp;
    int ini;
    sl_inv_nodeattr_t *cpval, *nval;
    int nflag;
    time_t t, ct, st, et, dt;
    Tm_t tm1, tm2, tmc, *tmp;
    char *m1p;

    vgnam[0] = 0, nflag = FALSE;
    ct = tim;
    for (; emailm < emaill; emailm++) {
        emailp = &emails[emailm];

        if (emailp->tmode != -1 && emailp->tmode != tmode)
            continue;

        if (emailp->sevnum != -1 && emailp->sevnum != sev)
            continue;

        if (emailp->nameflag && !nflag) {
            if ((nval = sl_inv_nodeattrfind (level, obj, nkey))) {
                strcpy (vgnam, nval->sl_val);
                nflag = TRUE;
            }
        }
        if (emailp->objreflag) {
            if (emailp->nameflag) {
                if (regexec (
                    &emailp->objcode, vgnam, 0, NULL,
                    REG_NULL | REG_EXTENDED | REG_LEFT | REG_RIGHT | REG_ICASE
                ) != 0)
                    continue;
            } else {
                if (regexec (
                    &emailp->objcode, obj, 0, NULL,
                    REG_NULL | REG_EXTENDED | REG_LEFT | REG_RIGHT | REG_ICASE
                ) != 0)
                    continue;
            }
        } else {
            if (emailp->nameflag) {
                if (strcasecmp (emailp->objstr, vgnam) != 0)
                    continue;
            } else {
                if (strcasecmp (emailp->objstr, obj) != 0)
                    continue;
            }
        }

        if (emailp->idreflag) {
            if (regexec (
                &emailp->idcode, id, 0, NULL,
                REG_NULL | REG_EXTENDED | REG_LEFT | REG_RIGHT
            ) != 0)
                continue;
        } else {
            if (strcmp (emailp->idstr, id) != 0)
                continue;
        }

        if (emailp->textreflag) {
            if (regexec (
                &emailp->textcode, text, 0, NULL,
                REG_NULL | REG_EXTENDED
            ) != 0)
                continue;
        } else {
            if (!strstr (text, emailp->textstr))
                continue;
        }

        st = emailp->st * 60;
        et = emailp->et * 60;
        dt = et - st;

        if (emailp->repeat == VG_ALARM_N_REP_ONCE) {
            if (emailp->edate == emailp->sdate && dt < 0)
                dt += (24 * 60 * 60);
            if (ct < emailp->sdate + st)
                continue;
            if (emailp->edate > 0 && ct > emailp->edate + st + dt)
                continue;
        } else {
            if (dt < 0)
                dt += (24 * 60 * 60);
            if (ct < emailp->sdate + st || (
                emailp->edate > 0 && ct > emailp->edate + st + dt
            ))
                continue;
            t = emailp->sdate + st;
            tmp = tmmake (&t), tm1 = *tmp;
            t = emailp->sdate + st + dt;
            tmp = tmmake (&t), tm2 = *tmp;
            tmp = tmmake (&ct), tmc = *tmp;
            t = tmc.tm_hour * 3600 + tmc.tm_min * 60;
            switch (emailp->repeat) {
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
        for (ini = emailp->ini; ini > -1 && ini < inl; ini++) {
            inp = &ins[ini];
            if (
                emailp->account[0] != inp->account[0] ||
                strcmp (emailp->account, inp->account) != 0
            )
                break;
            if (inp->idre[0] == '_' && strcmp (inp->idre, "__ALL__") == 0)
                continue;
            for (
                m1p = M1F (level, obj, inp->level); m1p;
                m1p = M1N (level, obj, inp->level)
            ) {
                if (inp->reflag) {
                    if (!inp->compflag && regcomp (
                        &inp->code, inp->idre, ICMPFLAGS
                    ) != 0) {
                        SUwarning (
                            0, "emailfind",
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
            if (!(cpval = sl_inv_nodeattrfind (level, obj, cpkey)))
                continue;
            if (!strchr (cpval->sl_val, 'E'))
                goto nomatch;
        }
        SUwarning (
            1, "emailfind", "matched\naccount=%s\nobj=%s\nmsg=%s\n",
            emailp->account, obj, text
        );
        emailm++;
        return emailp;

nomatch:
        ;
    }
    return NULL;
}

email_t *emaillistfirst (char *level, char *obj) {
    emailm = 0;
    return emaillist (level, obj);
}

email_t *emaillistnext (char *level, char *obj) {
    return emaillist (level, obj);
}

static email_t *emaillist (char *level, char *obj) {
    email_t *emailp;
    inv_t *inp;
    int ini;
    sl_inv_nodeattr_t *cpval, *nval;
    int nflag;
    char lv[VG_alarm_level1_L], id[VG_alarm_id1_L];
    char *m1p;

    vgnam[0] = 0, nflag = FALSE;
    memset (lv, 0, VG_alarm_level1_L), strcpy (lv, level);
    memset (id, 0, VG_alarm_id1_L), strcpy (id, obj);
    for (; emailm < emaill; emailm++) {
        emailp = &emails[emailm];

        if (emailp->nameflag && !nflag) {
            if ((nval = sl_inv_nodeattrfind (level, obj, nkey))) {
                strcpy (vgnam, nval->sl_val);
                nflag = TRUE;
            }
        }
        if (emailp->objreflag) {
            if (emailp->nameflag) {
                if (regexec (
                    &emailp->objcode, vgnam, 0, NULL,
                    REG_NULL | REG_EXTENDED | REG_LEFT | REG_RIGHT | REG_ICASE
                ) != 0)
                    continue;
            } else {
                if (regexec (
                    &emailp->objcode, obj, 0, NULL,
                    REG_NULL | REG_EXTENDED | REG_LEFT | REG_RIGHT | REG_ICASE
                ) != 0)
                    continue;
            }
        } else {
            if (emailp->nameflag) {
                if (strcasecmp (emailp->objstr, vgnam) != 0)
                    continue;
            } else {
                if (strcasecmp (emailp->objstr, obj) != 0)
                    continue;
            }
        }

        for (ini = emailp->ini; ini > -1 && ini < inl; ini++) {
            inp = &ins[ini];
            if (
                emailp->account[0] != inp->account[0] ||
                strcmp (emailp->account, inp->account) != 0
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
                            0, "emaillist",
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
            if (!strchr (cpval->sl_val, 'E'))
                goto nomatch;
        }
        emailm++;
        return emailp;

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
    email_t *ap, *bp;

    ap = (email_t *) a;
    bp = (email_t *) b;
    if (ap->objcn != bp->objcn)
        return bp->objcn - ap->objcn;
    if (ap->idcn != bp->idcn)
        return bp->idcn - ap->idcn;
    return bp->textcn - ap->textcn;
}
