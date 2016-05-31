#pragma prototyped

#include <ast.h>
#include <cdt.h>
#include <regex.h>
#include <vmalloc.h>
#include <swift.h>
#include <ctype.h>
#include "vg_hdr.h"
#include "vgprofile_pub.h"
#define VG_STATMAP_MAIN
#include "vg_statmap.c"
#include "sl_level_map.c"
#include "sl_inv_map1.c"
#include "sl_inv_nodeattr.c"

static profile_t *profiles;
static int profilen, profilel, profilem;

static Dt_t *defaultdict;
static Dtdisc_t defaultdisc = {
    sizeof (Dtlink_t), -1, 0, NULL, NULL, NULL, NULL, NULL, NULL
};

static inv_t *ins;
static int inn, inl;

static char *psname;

static char clevel[S_level], cpkey[S_key], nkey[S_key], vgnam[S_val];
static char skey1[S_key], skey2[S_key];
static char slkey[S_key], stkey[S_key], slst[S_val * 2 + 1];

#define ICMPFLAGS (REG_NULL | REG_EXTENDED | REG_LEFT | REG_RIGHT)
#define ICMP(p, i, s) (regexec (&p[i].code, s, 10, NULL, ICMPFLAGS) == 0)

static Dt_t *snamedict;
static Dtdisc_t snamedisc = {
    sizeof (Dtlink_t), -1, 0, NULL, NULL, NULL, NULL, NULL, NULL
};

static profile_t *profilefind (char *, char *);
static default_t *defaultfind (char *, char *);
static profile_t *profilelist (char *, char *);
static int cmp (const void *, const void *);
static int snamereset (void);
static int snameinsert (char *);
static int snamefind (char *);

int profileload (char *profilefile, char *defaultfile, char *accountfile) {
    Sfio_t *ffp, *afp;
    char *line;
    int ln;
    char *s1, *s2, *s3, *s4, *s5, *s6, *s7, *s8, *s9, *s10, *s11;
    char *s;
    default_t *defaultp;
    profile_t *profilep;
    inv_t *inp;
    int ini, inj;

    strcpy (clevel, "c");

    if (statmapload (getenv ("STATMAPFILE"), FALSE, FALSE, FALSE) == -1) {
        SUwarning (0, "profileload", "cannot load statmap file");
        return -1;
    }
    sl_level_mapopen (getenv ("LEVELMAPFILE"));
    sl_inv_map1open (getenv ("INVMAPFILE"));
    sl_inv_nodeattropen (getenv ("INVNODEATTRFILE"));
    M1I (TRUE);
    memset (cpkey, 0, S_key), strcpy (cpkey, "custpriv");
    memset (slkey, 0, S_key), strcpy (slkey, "scope_servicelevel");
    memset (stkey, 0, S_key), strcpy (stkey, "scope_systype");
    memset (nkey, 0, S_key), strcpy (nkey, "name");

    if (!(defaultdict = dtopen (&defaultdisc, Dtobag))) {
        SUwarning (0, "profileload", "default dtopen failed");
        return -1;
    }
    if (!(snamedict = dtopen (&snamedisc, Dtset))) {
        SUwarning (0, "profileload", "sname dtopen failed");
        return -1;
    }

    ins = NULL;
    inn = inl = 0;

    if (!(afp = sfopen (NULL, accountfile, "r"))) {
        SUwarning (0, "profileload", "cannot open account data file");
        return -1;
    }
    while ((line = sfgetr (afp, '\n', 1))) {
        if (!(s1 = strchr (line, '|'))) {
            SUwarning (0, "profileload", "bad line: %s", line);
            return -1;
        }
        *s1++ = 0;
        if (!(s2 = strchr (s1, '|'))) {
            SUwarning (0, "profileload", "bad line: %s", s1);
            return -1;
        }
        *s2++ = 0;
        if (!(s3 = strchr (s2, '|'))) {
            SUwarning (0, "profileload", "bad line: %s", s2);
            return -1;
        }
        *s3++ = 0;

        if (inl == inn) {
            if (!(ins = vmresize (
                Vmheap, ins, sizeof (inv_t) * (inn + 128), VM_RSCOPY
            ))) {
                SUwarning (0, "profileload", "cannot grow ins");
                inl = inn = 0;
                return -1;
            }
            inn += 128;
        }
        inp = &ins[inl++];
        memset (inp, 0, sizeof (inv_t));

        if (!(inp->account = vmstrdup (Vmheap, line))) {
            SUwarning (
                0, "profileload", "cannot allocate account id: %s", line
            );
            return -1;
        }
        strncpy (inp->level, s2, S_level);
        if (!(inp->idre = vmstrdup (Vmheap, s3))) {
            SUwarning (0, "profileload", "cannot allocate id: %s", s3);
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

    profiles = NULL;
    profilen = profilel = 0;
    if (!(ffp = sfopen (NULL, profilefile, "r"))) {
        SUwarning (0, "profileload", "cannot open profile file");
        return -1;
    }
    ln = 0;
    while ((line = sfgetr (ffp, '\n', 1))) {
        ln++;
        s1 = line;
        if (!(s2 = strstr (s1, "|"))) {
            SUwarning (0, "profileload", "missing stat name, line: %d", ln);
            continue;
        }
        *s2 = 0, s2++;
        if (!(s3 = strstr (s2, "|"))) {
            SUwarning (0, "profileload", "missing stat status, line: %d", ln);
            continue;
        }
        *s3 = 0, s3++;
        if (!(s4 = strstr (s3, "|"))) {
            SUwarning (0, "profileload", "missing stat value, line: %d", ln);
            continue;
        }
        *s4 = 0, s4++;
        if (!(s5 = strstr (s4, "|"))) {
            SUwarning (0, "profileload", "missing account, line: %d", ln);
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
            SUwarning (0, "profileload", "account not found: %s", s5);
            continue;
        }

        if (profilel == profilen) {
            if (!(profiles = vmresize (
                Vmheap, profiles, sizeof (profile_t) * (profilen + 128),
                VM_RSCOPY
            ))) {
                SUwarning (0, "profileload", "cannot grow profiles");
                profilel = profilen = 0;
                return -1;
            }
            profilen += 128;
        }
        profilep = &profiles[profilel];
        memset (profilep, 0, sizeof (profile_t));

        VG_urldec (s1, s1, strlen (s1) + 1);
        if (strncasecmp (s1, "name=", 5) == 0)
            profilep->nameflag = TRUE, s1 += 5;
        else
            profilep->nameflag = FALSE;
        if (!*s1)
            s1 = ".*";
        profilep->idreflag = FALSE;
        profilep->idcn = 0;
        for (s = s1; *s; s++) {
            if (!isalnum (*s) && *s != '_')
                profilep->idreflag = TRUE;
            else
                profilep->idcn++;
        }
        if (!(profilep->idstr = vmstrdup (Vmheap, s1))) {
            SUwarning (0, "profileload", "cannot allocate id str");
            return -1;
        }
        if (profilep->idreflag) {
            if (regcomp (
                &profilep->idcode, profilep->idstr,
                REG_NULL | REG_EXTENDED | REG_LEFT | REG_RIGHT | REG_ICASE
            ) != 0) {
                SUwarning (
                    0, "profileload",
                    "failed to compile regex for idect, line: %d", ln
                );
                continue;
            }
        }

        if (!(profilep->sname = vmstrdup (Vmheap, s2))) {
            SUwarning (0, "profileload", "cannot allocate stat name");
            return -1;
        }

        if (!(profilep->sstatus = vmstrdup (Vmheap, s3))) {
            SUwarning (0, "profileload", "cannot allocate stat status");
            return -1;
        }

        if (!(profilep->svalue = vmstrdup (Vmheap, s4))) {
            SUwarning (0, "profileload", "cannot allocate stat value");
            return -1;
        }

        if (!(profilep->account = vmstrdup (Vmheap, s5))) {
            SUwarning (0, "profileload", "cannot allocate account str");
            return -1;
        }
        profilep->ini = inj;

        profilel++;
    }
    qsort (profiles, profilel, sizeof (profile_t), cmp);
    sfclose (ffp);

    if (!defaultfile)
        return 0;

    if (!(ffp = sfopen (NULL, defaultfile, "r"))) {
        SUwarning (0, "profileload", "cannot open default file");
        return -1;
    }
    ln = 0;
    while ((line = sfgetr (ffp, '\n', 1))) {
        ln++;
        if (line[0] == '#')
            continue;
        s1 = line;
        if (
            !(s2 = strchr (s1, '|')) || !(s3 = strchr (s2 + 1, '|')) ||
            !(s4 = strchr (s3 + 1, '|')) || !(s5 = strchr (s4 + 1, '|')) ||
            !(s6 = strchr (s5 + 1, '|')) || !(s7 = strchr (s6 + 1, '|')) ||
            !(s8 = strchr (s7 + 1, '|')) || !(s9 = strchr (s8 + 1, '|')) ||
            !(s10 = strchr (s9 + 1, '|')) || !(s11 = strchr (s10 + 1, '|'))
        )
            continue;
        *s2++ = 0, *s3++ = 0, *s4++ = 0, *s5++ = 0, *s6++ = 0;
        *s7++ = 0, *s8++ = 0, *s9++ = 0, *s10++ = 0, *s11++ = 0;
        if (!*s1 || !*s2 || !*s4 || !*s10)
            continue;
        if (!(defaultp = vmalloc (Vmheap, sizeof (default_t)))) {
            SUwarning (0, "profileload", "default malloc failed");
            return -1;
        }
        memset (defaultp, 0, sizeof (default_t));
        if (!(defaultp->slst = vmalloc (
            Vmheap, strlen (s1) + strlen (s2) + 2
        ))) {
            SUwarning (0, "profileload", "default malloc failed");
            return -1;
        }
        defaultp->slst[0] = 0;
        strcat (defaultp->slst, s1);
        strcat (defaultp->slst, ":");
        strcat (defaultp->slst, s2);
        if (!(defaultp->sname = vmstrdup (Vmheap, s4))) {
            SUwarning (0, "profileload", "cannot copy default sname");
            continue;
        }
        if (!(defaultp->svalue = vmstrdup (Vmheap, s10))) {
            SUwarning (0, "profileload", "cannot copy default svalue");
            continue;
        }
        if (!dtinsert (defaultdict, defaultp)) {
            SUwarning (0, "profileload", "default dtinsert failed");
            vmfree (Vmheap, defaultp->slst);
            vmfree (Vmheap, defaultp);
            continue;
        }
    }
    sfclose (ffp);

    return 0;
}

profile_t *profilefindfirst (char *level, char *id) {
    profilem = 0;
    psname = NULL;
    snamereset ();
    return profilefind (level, id);
}

profile_t *profilefindnext (char *level, char *id) {
    return profilefind (level, id);
}

static profile_t *profilefind (char *level, char *id) {
    profile_t *profilep;
    inv_t *inp;
    int ini;
    sl_inv_nodeattr_t *nap, *nval;
    int nflag;
    char *m1p;

    vgnam[0] = 0, nflag = FALSE;

again:
    for (; profilem < profilel; profilem++) {
        profilep = &profiles[profilem];

        if (profilep->nameflag && !nflag) {
            if ((nval = sl_inv_nodeattrfind (level, id, nkey))) {
                strcpy (vgnam, nval->sl_val);
                nflag = TRUE;
            }
        }
        if (profilep->idreflag) {
            if (profilep->nameflag) {
                if (regexec (
                    &profilep->idcode, vgnam, 0, NULL,
                    REG_NULL | REG_EXTENDED | REG_LEFT | REG_RIGHT | REG_ICASE
                ) != 0)
                    continue;
            } else {
                if (regexec (
                    &profilep->idcode, id, 0, NULL,
                    REG_NULL | REG_EXTENDED | REG_LEFT | REG_RIGHT | REG_ICASE
                ) != 0)
                    continue;
            }
        } else {
            if (profilep->nameflag) {
                if (strcasecmp (profilep->idstr, vgnam) != 0)
                    continue;
            } else {
                if (strcasecmp (profilep->idstr, id) != 0)
                    continue;
            }
        }

match:
        for (ini = profilep->ini; ini > -1 && ini < inl; ini++) {
            inp = &ins[ini];
            if (
                profilep->account[0] != inp->account[0] ||
                strcmp (profilep->account, inp->account) != 0
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
                            0, "profilefind",
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
            if (!(nap = sl_inv_nodeattrfind (level, id, cpkey)))
                continue;
            if (!strchr (nap->sl_val, 'T'))
                goto nomatch;
        }
        SUwarning (
            1, "profilefind", "matched\naccount=%s\nid=%s\n",
            profilep->account, id
        );
        if (snameinsert (profilep->sname) == -1)
            SUwarning (0, "profilefind", "cannot insert sname");
        if (psname && strcmp (psname, profilep->sname) == 0)
            goto nomatch;
        psname = profilep->sname;
        profilem++;
        return profilep;
    }
    return NULL;

nomatch:
    if (++profilem < profilel)
        goto again;

    return NULL;
}

static default_t *defaultp, *defaultpp, *defaultnp;

default_t *defaultfindfirst (char *level, char *id) {
    sl_inv_nodeattr_t *nap;

    defaultp = NULL;
    if (!(nap = sl_inv_nodeattrfind (level, id, slkey)))
        return NULL;
    slst[0] = 0;
    strcat (slst, nap->sl_val);
    strcat (slst, ":");
    if (!(nap = sl_inv_nodeattrfind (level, id, stkey)))
        return NULL;
    strcat (slst, nap->sl_val);
    defaultpp = defaultnp = dtmatch (defaultdict, slst);

    return defaultfind (level, id);
}

default_t *defaultfindnext (char *level, char *id) {
    return defaultfind (level, id);
}

static default_t *defaultfind (char *level, char *id) {
again:
    if (!defaultp) {
        defaultp = defaultpp;
        goto match;
    }
    if (defaultpp) {
        defaultp = defaultpp = dtprev (defaultdict, defaultpp);
        if (defaultp && strcmp (defaultp->slst, slst) == 0)
            goto match;
        defaultpp = NULL;
    }
    if (defaultnp) {
        defaultp = defaultnp = dtnext (defaultdict, defaultnp);
        if (defaultp && strcmp (defaultp->slst, slst) == 0)
            goto match;
        defaultnp = NULL;
    }
    return NULL;

match:
    if (defaultp && snamefind (defaultp->sname) == 0)
        goto again;
    return defaultp;

}

profile_t *profilelistfirst (char *level, char *id) {
    profilem = 0;
    psname = NULL;
    return profilelist (level, id);
}

profile_t *profilelistnext (char *level, char *id) {
    return profilelist (level, id);
}

static profile_t *profilelist (char *level, char *obj) {
    profile_t *profilep;
    inv_t *inp;
    int ini;
    sl_inv_nodeattr_t *nap, *nval;
    int nflag;
    char lv[S_level], id[S_id];
    char *m1p;

    vgnam[0] = 0, nflag = FALSE;
    memset (lv, 0, S_level), strcpy (lv, level);
    memset (id, 0, S_id), strcpy (id, obj);

again:
    for (; profilem < profilel; profilem++) {
        profilep = &profiles[profilem];

        if (profilep->nameflag && !nflag) {
            if ((nval = sl_inv_nodeattrfind (level, obj, nkey))) {
                strcpy (vgnam, nval->sl_val);
                nflag = TRUE;
            }
        }
        if (profilep->idreflag) {
            if (profilep->nameflag) {
                if (regexec (
                    &profilep->idcode, vgnam, 0, NULL,
                    REG_NULL | REG_EXTENDED | REG_LEFT | REG_RIGHT | REG_ICASE
                ) != 0)
                    continue;
            } else {
                if (regexec (
                    &profilep->idcode, obj, 0, NULL,
                    REG_NULL | REG_EXTENDED | REG_LEFT | REG_RIGHT | REG_ICASE
                ) != 0)
                    continue;
            }
        } else {
            if (profilep->nameflag) {
                if (strcasecmp (profilep->idstr, vgnam) != 0)
                    continue;
            } else {
                if (strcasecmp (profilep->idstr, obj) != 0)
                    continue;
            }
        }

        for (ini = profilep->ini; ini > -1 && ini < inl; ini++) {
            inp = &ins[ini];
            if (
                profilep->account[0] != inp->account[0] ||
                strcmp (profilep->account, inp->account) != 0
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
                            0, "profilelist",
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
            if (!(nap = sl_inv_nodeattrfind (lv, id, cpkey)))
                continue;
            if (!strchr (nap->sl_val, 'T'))
                goto nomatch;
        }
        if (psname && strcmp (psname, profilep->sname) == 0)
            goto nomatch;
        psname = profilep->sname;
        profilem++;
        return profilep;
    }
    return NULL;

nomatch:
    if (++profilem < profilel)
        goto again;

    return NULL;
}

static char *svaluebuf;
static int svaluelen;
static char *specbuf;
static int speclen, specind;
static char **ws;
static int wn, wm;
#define COPY(d) { \
    char *_s; \
    for (_s = d; _s && *_s; _s++) \
        specbuf[specind++] = *_s; \
    specbuf[specind] = 0; \
}

char *svalueparse (char *level, char *id, char *sname, char *svalue) {
    sl_inv_nodeattr_t *nap;
    int havecap;
    float capval, percval;
    int smi, svl, wi;
    char valbuf[20], *s1, *s2;

    havecap = FALSE;
    if (strchr (svalue, '%')) {
        memset (skey1, 0, S_key);
        strcat (skey1, "si_sz_");
        strcat (skey1, sname);
        memset (skey2, 0, S_key);
        strcat (skey2, "scopeinv_size_");
        strcat (skey2, sname);
        if ((nap = sl_inv_nodeattrfind (level, id, skey1)))
            capval = atof (nap->sl_val), havecap = TRUE;
        else if ((nap = sl_inv_nodeattrfind (level, id, skey2)))
            capval = atof (nap->sl_val), havecap = TRUE;
        else if ((smi = statmapfind (sname)) != -1 && statmaps[smi].havemaxv)
            capval = statmaps[smi].maxv, havecap = TRUE;
    }

    if ((svl = strlen (svalue) + 1) > svaluelen) {
        if (!(svaluebuf = vmresize (Vmheap, svaluebuf, svl, VM_RSMOVE))) {
            SUwarning (0, "svalueparse", "cannot grow svaluebuf");
            svaluelen = 0;
            return NULL;
        }
        svaluelen = svl;
        if (!(specbuf = vmresize (Vmheap, specbuf, svl * 20 + 16, VM_RSMOVE))) {
            SUwarning (0, "svalueparse", "cannot grow specbuf");
            return NULL;
        }
        speclen = svl * 20 + 16;
    }
    strcpy (svaluebuf, svalue);

    wm = 0;
    for (s1 = svaluebuf; s1; s1 = s2) {
        if ((s2 = strchr (s1, ':')))
            *s2++ = 0;
        if (wm >= wn) {
            if (!(ws = vmresize (
                Vmheap, ws, (wn + 32) * sizeof (char *), VM_RSCOPY
            ))) {
                SUwarning (0, "svalueparse", "cannot grow ws");
                return NULL;
            }
            wn += 32;
        }
        ws[wm++] = s1;
    }

    specbuf[0] = 0;
    specind = 0;
    for (wi = 0; wi < wm; ) {
        if (strcmp (ws[wi], "VR") == 0) {
            if (wi + 4 >= wm) {
                SUwarning (0, "svalueparse", "bad VR spec: %s", svalue);
                return NULL;
            }
            COPY (ws[wi]); COPY (":");     // VR
            COPY (ws[wi + 1]); COPY (":"); // e,i,n
            if (!(s1 = strchr (ws[wi + 2], '%'))) {
                COPY (ws[wi + 2]); COPY (":"); // val
            } else if (havecap) {
                *s1 = 0;
                percval = atof (ws[wi + 2]);
                sfsprintf (valbuf, 20, "%f", capval * (percval / 100.0));
                COPY (valbuf); COPY (":"); // val
            } else {
                SUwarning (
                    0, "svalueparse",
                    "percent value without capacity: %s:%s %s/%s",
                    level, id, sname, svalue
                );
                return NULL;
            }
            COPY (ws[wi + 3]); COPY (":"); // e,i,n
            if (!(s1 = strchr (ws[wi + 4], '%'))) {
                COPY (ws[wi + 4]); COPY (";"); // val
            } else if (havecap) {
                *s1 = 0;
                percval = atof (ws[wi + 4]);
                sfsprintf (valbuf, 20, "%f", capval * (percval / 100.0));
                COPY (valbuf); COPY (";"); // val
            } else {
                SUwarning (
                    0, "svalueparse",
                    "percent value without capacity: %s:%s %s/%s",
                    level, id, sname, svalue
                );
                return NULL;
            }
            wi += 5;
        } else if (strcmp (ws[wi], "ACTUAL") == 0) {
            if (wi + 7 >= wm) {
                SUwarning (0, "svalueparse", "bad ACTUAL spec: %s", svalue);
                return NULL;
            }
            COPY (ws[wi]); COPY (":");     // ACTUAL
            COPY (ws[wi + 1]); COPY (":"); // e,i,n
            if (!(s1 = strchr (ws[wi + 2], '%'))) {
                COPY (ws[wi + 2]); COPY (":"); // val
            } else if (havecap) {
                *s1 = 0;
                percval = atof (ws[wi + 2]);
                sfsprintf (valbuf, 20, "%f", capval * (percval / 100.0));
                COPY (valbuf); COPY (":"); // val
            } else {
                SUwarning (
                    0, "svalueparse",
                    "percent value without capacity: %s:%s %s/%s",
                    level, id, sname, svalue
                );
                return NULL;
            }
            COPY (ws[wi + 3]); COPY (":"); // e,i,n
            if (!(s1 = strchr (ws[wi + 4], '%'))) {
                COPY (ws[wi + 4]); COPY (":"); // val
            } else if (havecap) {
                *s1 = 0;
                percval = atof (ws[wi + 4]);
                sfsprintf (valbuf, 20, "%f", capval * (percval / 100.0));
                COPY (valbuf); COPY (":"); // val
            } else {
                SUwarning (
                    0, "svalueparse",
                    "percent value without capacity: %s:%s %s/%s",
                    level, id, sname, svalue
                );
                return NULL;
            }
            COPY (ws[wi + 5]); COPY (":"); // sev
            COPY (ws[wi + 6]); COPY (":"); // m
            COPY (ws[wi + 7]); COPY (":"); // n
            COPY (ws[wi + 8]); COPY (":"); // freq
            COPY (ws[wi + 9]); COPY (";"); // ival
            wi += 10;
        } else if (strcmp (ws[wi], "SIGMA") == 0) {
            if (wi + 7 >= wm) {
                SUwarning (0, "svalueparse", "bad SIGMA spec: %s", svalue);
                return NULL;
            }
            COPY (ws[wi]); COPY (":");     // SIGMA
            COPY (ws[wi + 1]); COPY (":"); // e,i,n
            COPY (ws[wi + 2]); COPY (":"); // val
            COPY (ws[wi + 3]); COPY (":"); // e,i,n
            COPY (ws[wi + 4]); COPY (":"); // val
            COPY (ws[wi + 5]); COPY (":"); // sev
            COPY (ws[wi + 6]); COPY (":"); // m
            COPY (ws[wi + 7]); COPY (":"); // n
            COPY (ws[wi + 8]); COPY (":"); // freq
            COPY (ws[wi + 9]); COPY (";"); // ival
            wi += 10;
        } else if (strcmp (ws[wi], "CLEAR") == 0) {
            if (wi + 3 >= wm) {
                SUwarning (0, "svalueparse", "bad CLEAR spec: %s", svalue);
                return NULL;
            }
            COPY (ws[wi]); COPY (":");     // CLEAR
            COPY (ws[wi + 1]); COPY (":"); // sev
            COPY (ws[wi + 2]); COPY (":"); // m
            COPY (ws[wi + 3]); COPY (";"); // n
            wi += 4;
        } else {
            SUwarning (0, "svalueparse", "bad spec: %s", svalue);
            return NULL;
        }
    }
    if (specbuf[specind - 1] == ';')
        specbuf[specind - 1] = 0;

    return specbuf;
}

static int cmp (const void *a, const void *b) {
    profile_t *ap, *bp;
    int ret;

    ap = (profile_t *) a;
    bp = (profile_t *) b;
    if ((ret = strcmp (ap->sname, bp->sname)) != 0)
        return ret;
    return bp->idcn - ap->idcn;
}

static int snamereset (void) {
    sname_t *snamep;

    while ((snamep = dtfirst (snamedict))) {
        dtdelete (snamedict, snamep);
        vmfree (Vmheap, snamep->sname);
        vmfree (Vmheap, snamep);
    }
    return 0;
}

static int snameinsert (char *snamestr) {
    sname_t *snamep;

    if (dtmatch (snamedict, snamestr))
        return 0;

    if (!(snamep = vmalloc (Vmheap, sizeof (sname_t)))) {
        SUwarning (0, "snameinsert", "sname malloc failed");
        return -1;
    }
    memset (snamep, 0, sizeof (sname_t));
    if (!(snamep->sname = vmstrdup (Vmheap, snamestr))) {
        SUwarning (0, "snameinsert", "cannot copy sname value");
        return -1;
    }
    if (dtinsert (snamedict, snamep) != snamep) {
        SUwarning (0, "snameinsert", "sname dtinsert failed");
        vmfree (Vmheap, snamep->sname);
        vmfree (Vmheap, snamep);
        return -1;
    }
    return 0;
}

static int snamefind (char *snamestr) {
    char *s;

    if (dtmatch (snamedict, snamestr))
        return 0;
    if ((s = strchr (snamestr, '.'))) {
        strcpy (skey1, snamestr);
        skey1[(s - snamestr)] = 0;
        if (dtmatch (snamedict, skey1))
            return 0;
    }
    return -1;
}
