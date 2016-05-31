#pragma prototyped

#include <ast.h>
#include <vmalloc.h>
#include <swift.h>
#include "vggce.h"

Dt_t *svcdict;
int svcmark;

ev_t **evps;
int evpn, evpm;
int evmark;

ruleev_t *ruleevs;
int ruleevn, ruleevm;
rulesvc_t *rulesvcs;
int rulesvcn, rulesvcm;

static Dtdisc_t svcdisc = {
    sizeof (Dtlink_t), -1, 0, NULL, NULL, NULL, NULL, NULL, NULL
};
#define REFLAGS (REG_NULL | REG_EXTENDED | REG_LEFT | REG_RIGHT)
#define REFLAGSI (REG_NULL | REG_EXTENDED | REG_LEFT | REG_RIGHT | REG_ICASE)

static Sfio_t *fp3;

int initcorr (void) {
    if (!(svcdict = dtopen (&svcdisc, Dtset))) {
        SUwarning (0, "initcorr", "svc dtopen failed");
        return -1;
    }
    svcmark = 0;
    evmark = 0;

    return 0;
}

int termcorr (void) {
    if (dtclose (svcdict) == -1) {
        SUwarning (0, "termcorr", "svc dtclose failed");
        return -1;
    }

    return 0;
}

int updatealarms (int mode1, int mode2, int toff) {
    cc_t *ccp;
    nd_t *ndp;
    int ndpi;
    svcm_t *svcmp;
    int svcmi;
    rulesvc_t *rulesvcp;
    rulesev_t *sevp;
    int sevi;
    int tc, ti;
    float ratio;
    char *ststr, *tpstr, *tpstr2, txtstr[1024];

    if (!fp3 && !(fp3 = sfnew (NULL, NULL, 4096, 3, SF_WRITE))) {
        SUwarning (0, "updatealarms", "open of fd 3 failed");
        return -1;
    }

    for (ccp = dtfirst (ccdict); ccp; ccp = dtnext (ccdict, ccp)) {
        if (ccp->mark != ccmark || !ccp->fullflag)
            continue;

        for (ndpi = 0; ndpi < ccp->ndpm; ndpi++) {
            ndp = ccp->ndps[ndpi];
            for (svcmi = 0; svcmi < ndp->svcmm; svcmi++) {
                svcmp = &ndp->svcms[svcmi];
                if (svcmp->role != SVC_ROLE_CONS)
                    continue;
                if (svcmp->vs[SVC_MODE_NOEV] == 0)
                    continue;
                if (svcmp->vs[mode1] == svcmp->vs[mode2])
                    continue;
                if (!(rulesvcp = matchrulesvc (svcmp->svcp->name)))
                    continue;

                ratio = 100.0 * (
                    (float) svcmp->vs[mode2] / svcmp->vs[SVC_MODE_NOEV]
                );
                for (sevi = 0; sevi < rulesvcp->sevn; sevi++) {
                    sevp = &rulesvcp->sevs[sevi];
                    if (sevp->ratio >= ratio) {
                        if (
                            svcmp->vs[mode2] == svcmp->vs[SVC_MODE_NOEV]
                        ) {
                            tc = currtime + toff, ti = currtime + toff;
                            ststr = VG_ALARM_S_STATE_UP;
                            tpstr = VG_ALARM_S_TYPE_CLEAR;
                            tpstr2 = "CLEAR";
                            sfsprintf (
                                txtstr, 1024,
                                "%s service fully restored",
                                svcmp->svcp->name
                            );
                        } else if (svcmp->vs[mode2] == 0) {
                            tc = 0, ti = currtime + toff;
                            ststr = VG_ALARM_S_STATE_DOWN;
                            tpstr = VG_ALARM_S_TYPE_ALARM;
                            tpstr2 = "ALARM";
                            sfsprintf (
                                txtstr, 1024,
                                "%s service fully down",
                                svcmp->svcp->name
                            );
                        } else {
                            tc = 0, ti = currtime + toff;
                            ststr = VG_ALARM_S_STATE_DEG;
                            tpstr = VG_ALARM_S_TYPE_ALARM;
                            tpstr2 = "ALARM";
                            sfsprintf (
                                txtstr, 1024,
                                "%s service degraded (%.2f%% up)",
                                 svcmp->svcp->name, ratio
                            );
                        }
                        VG_warning (
                           0, "GCE INFO",
                           "issued alarm for %s:%s type=%s sev=%d",
                           ndp->level, ndp->id, tpstr, sevp->sev
                        );
                        sfprintf (
                            fp3,
                            "<alarm>"
                            "<v>%s</v><sid>GCE</sid><aid>svc.%s</aid>"
                            "<ccid></ccid><st>%s</st><sm>%s</sm>"
                            "<vars></vars><di></di><hi></hi>"
                            "<tc>%d</tc><ti>%d</ti>"
                            "<tp>%s</tp><so>%s</so><pm>%s</pm>"
                            "<lv1>%s</lv1><id1>%s</id1><lv2></lv2><id2></id2>"
                            "<tm>%s</tm><sev>%d</sev>"
                            "<txt>VG %s GCE %s</txt><com>%s</com>"
                            "<origmsg></origmsg>"
                            "</alarm>\n",
                            VG_S_VERSION, svcmp->svcp->name,
                            ststr, VG_ALARM_S_MODE_KEEP,
                            tc, ti,
                            tpstr,
                            VG_ALARM_S_SO_NORMAL, VG_ALARM_S_PMODE_PROCESS,
                            ndp->level, ndp->id,
                            ftmode, sevp->sev,
                            tpstr2, txtstr, rulesvcp->comstr
                        );

                        break;
                    }
                }
            }
        }
    }
    return 0;
}

int calcsvcvalues (int mode, int minevmark, int maxevmark) {
    cc_t *ccp;
    nd_t *ndp, *ndstp;
    int ndpi;
    ed_t *edp;
    svc_t *svcp;
    int svcpi;
    svcm_t *svcmp;
    int svcmi;
    ev_t *evp;
    int evpi;
    int edi;

    for (ccp = dtfirst (ccdict); ccp; ccp = dtnext (ccdict, ccp)) {
        if (ccp->mark != ccmark)
            continue;
        if (!ccp->fullflag && insertcc (ccp->level, ccp->id, TRUE) != ccp) {
            SUwarning (0, "calcsvcvalues", "insertcc failed");
            return -1;
        }

        for (svcpi = 0; svcpi < ccp->svcpm; svcpi++) {
            svcp = ccp->svcps[svcpi];
            ndmark++;
            for (ndpi = 0; ndpi < ccp->ndpm; ndpi++) {
                ndp = ccp->ndps[ndpi];
                ndp->v = 0;
                ndp->state = ND_STATE_UNUSED;
                ndp->downflag = FALSE;
                if (mode != SVC_MODE_NOEV) {
                    for (evpi = 0; evpi < ndp->evpm; evpi++) {
                        evp = ndp->evps[evpi];
                        if (evp->mark < minevmark || evp->mark > maxevmark)
                            continue;
                        for (svcpi = 0; svcpi < evp->svcpm; svcpi++) {
                            if (evp->svcps[svcpi] == svcp) {
                                if (evp->type == VG_ALARM_N_TYPE_CLEAR)
                                    ndp->downflag = FALSE;
                                else
                                    ndp->downflag = TRUE;
                                break;
                            }
                        }
                    }
                }
                for (svcmi = 0; svcmi < ndp->svcmm; svcmi++) {
                    svcmp = &ndp->svcms[svcmi];
                    if (svcmp->svcp == svcp) {
                        svcmp->vs[mode] = 0;
                        ndp->mark = ndmark;
                        ndp->svcmp = svcmp;
                        break;
                    }
                }
            }

            for (ndpi = 0; ndpi < ccp->ndpm; ndpi++) {
                ndp = ccp->ndps[ndpi];
                if (ndp->mark != ndmark || ndp->downflag)
                    continue;
                svcmp = &ndp->svcms[svcmi];
                if (svcmp->role != SVC_ROLE_PROD)
                    continue;
                ccp->ndsti = 0;
                ccp->ndsts[ccp->ndsti++] = ndp;
                ndp->state = ND_STATE_ONSTACK;
                while (ccp->ndsti > 0) {
                    ndstp = ccp->ndsts[ccp->ndsti - 1];
                    if (ndstp->state == ND_STATE_ONPATH) {
                        ndstp->state = ND_STATE_UNUSED;
                        ccp->ndsti--;
                        continue;
                    }
                    ndstp->state = ND_STATE_ONPATH;
                    for (edi = 0; edi < ndstp->edm; edi++) {
                        edp = &ndstp->eds[edi];
                        if (edp->ndp->downflag)
                            continue;
                        edp->ndp->v++;
                        if (edp->ndp->mark == ndmark)
                            edp->ndp->svcmp->vs[mode] = edp->ndp->v;
                        if (edp->ndp->state != ND_STATE_UNUSED)
                            continue;
                        ccp->ndsts[ccp->ndsti++] = edp->ndp;
                        edp->ndp->state = ND_STATE_ONSTACK;
                    }
                }
            }
            if (SUwarnlevel > 0) {
                for (ndpi = 0; ndpi < ccp->ndpm; ndpi++) {
                    ndp = ccp->ndps[ndpi];
                    if (ndp->mark != ndmark)
                        continue;
                    if (ndp->svcmp->role == SVC_ROLE_CONS)
                        SUwarning (
                            0, "calcsvcvalues",
                            "state s: %s n: %s m: %d v: %d %d %d %d %d",
                            svcp->name, ndp->id, mode, ndp->v,
                            ndp->svcmp->vs[0], ndp->svcmp->vs[1],
                            ndp->svcmp->vs[2], ndp->svcmp->vs[3]
                        );
                }
            }
        }
    }
    return 0;
}

svc_t *insertsvc (char *name) {
    svc_t *svcp;

    if ((svcp = dtmatch (svcdict, name)))
        return svcp;

    if (!(svcp = vmalloc (Vmheap, sizeof (svc_t)))) {
        SUwarning (0, "insertsvc", "svc malloc failed");
        return NULL;
    }
    memset (svcp, 0, sizeof (svc_t));
    if (!(svcp->name = vmstrdup (Vmheap, name))) {
        SUwarning (0, "insertsvc", "name strdup failed");
        return NULL;
    }
    if (dtinsert (svcdict, svcp) != svcp) {
        SUwarning (0, "insertsvc", "svc dtinsert failed");
        return NULL;
    }

    return svcp;
}

int pruneevs (int mintime, int mode) {
    ev_t *evp;
    int evpi;

    for (evpi = 0; evpi < evpm; evpi++) {
        evp = evps[evpi];
        if (mode == 2) {
            if (evp->time >= mintime)
                evp->mark = evmark;
        } else {
            if (evp->time < mintime)
                evp->ccp->mark = ccmark;
        }
    }

    return 0;
}

ev_t *insertev (int time, int type, cc_t *ccp, nd_t *ndp, ruleev_t *ruleevp) {
    ev_t *evp;
    svc_t *svcp;
    int svcpi;

    if (evpm >= evpn) {
        if (!(evps = vmresize (
            Vmheap, evps, (evpn + 100) * sizeof (ev_t *), VM_RSCOPY
        ))) {
            SUwarning (0, "insertev", "evs resize failed");
            return NULL;
        }
        evpn += 100;
    }
    if (!(evps[evpm] = vmalloc (Vmheap, sizeof (ev_t)))) {
        SUwarning (0, "insertev", "ev malloc failed");
        return NULL;
    }
    evp = evps[evpm++];
    memset (evp, 0, sizeof (ev_t));
    evp->time = time, evp->type = type;
    evp->ccp = ccp, evp->ndp = ndp, evp->ruleevp = ruleevp;
    if (ccp->fullflag) {
        for (svcpi = 0; svcpi < ccp->svcpm; svcpi++) {
            svcp = ccp->svcps[svcpi];
            if (regexec (&ruleevp->svcre, svcp->name, 0, NULL, REFLAGS) != 0)
                continue;
            if (evp->svcpm >= evp->svcpn) {
                if (!(evp->svcps = vmresize (
                    Vmheap, evp->svcps, (evp->svcpn + 4) * sizeof (svc_t *),
                    VM_RSCOPY
                ))) {
                    SUwarning (0, "insertev", "svcps resize failed");
                    return NULL;
                }
                evp->svcpn += 4;
            }
            evp->svcps[evp->svcpm++] = svcp;
        }
    }

    evp->mark = evmark;

    return evp;
}

int insertevservice (ev_t *evp, char *svcstr) {
    if (evp->svcpm >= evp->svcpn) {
        if (!(evp->svcps = vmresize (
            Vmheap, evp->svcps, (evp->svcpn + 4) * sizeof (svc_t *), VM_RSCOPY
        ))) {
            SUwarning (0, "insertevservice", "svcps resize failed");
            return -1;
        }
        evp->svcpn += 4;
    }
    if (!(evp->svcps[evp->svcpm++] = insertsvc (svcstr))) {
        SUwarning (0, "insertevservice", "insertsvc failed");
        return -1;
    }

    return 0;
}

int loadevstate (char *file) {
    Sfio_t *fp;
    ev_t *evp;
    char *line, *s1, *s2, *s3, *s4, *s5, *s6, *s7;
    int time, type;
    char cclevel[SZ_level], ccid[SZ_id];
    char ndlevel[SZ_level], ndid[SZ_id];
    cc_t *ccp;
    nd_t *ndp;
    ruleev_t *ruleevp;

    if (!(fp = sfopen (NULL, file, "r"))) {
        SUwarning (0, "loadevstate", "open failed for file %s", file);
        return -1;
    }
    evp = NULL;
    while ((line = sfgetr (fp, '\n', 1))) {
        if (strncmp (line, "EV1|", 4) == 0) {
            s1 = line + 4;
            if (
                !(s2 = strchr (s1, '|')) || !(s3 = strchr (s2 + 1, '|')) ||
                !(s4 = strchr (s3 + 1, '|')) || !(s5 = strchr (s4 + 1, '|')) ||
                !(s6 = strchr (s5 + 1, '|')) || !(s7 = strchr (s6 + 1, '|'))
            ) {
                SUwarning (0, "loadevstate", "bad line %s", line);
                return -1;
            }
            *s2++ = 0, *s3++ = 0, *s4++ = 0, *s5++ = 0, *s6++ = 0, *s7++ = 0;
            time = atoi (s1);
            type = atoi (s2);
            memset (cclevel, 0, SZ_level);
            memset (ccid, 0, SZ_id);
            strcpy (cclevel, s3), strcpy (ccid, s4);
            memset (ndlevel, 0, SZ_level);
            memset (ndid, 0, SZ_id);
            strcpy (ndlevel, s5), strcpy (ndid, s6);
            if (!(ccp = insertcc (cclevel, ccid, FALSE))) {
                SUwarning (0, "loadevstate", "insertcc failed");
                return -1;
            }
            if (!(ndp = insertnd (ndlevel, ndid, ccp, FALSE))) {
                SUwarning (0, "loadevstate", "insertnd failed");
                return -1;
            }
            if (!(ruleevp = findruleev (s7))) {
                SUwarning (0, "loadevstate", "findruleev failed");
                return -1;
            }
            if (!(evp = insertev (time, type, ccp, ndp, ruleevp))) {
                SUwarning (0, "loadevstate", "insertev failed");
                return -1;
            }
            if (insertndevent (ndp, evp) == -1) {
                SUwarning (0, "loadevstate", "insertndevent failed");
                return -1;
            }
            if (currtime < time)
                currtime = time;
        } else if (strncmp (line, "EV2|", 4) == 0) {
            if (!evp) {
                SUwarning (0, "loadevstate", "EV2 record before EV1");
                return -1;
            }
            s1 = line + 4;
            if (insertevservice (evp, s1) == -1) {
                SUwarning (0, "loadevstate", "insertevservice failed");
                return -1;
            }
        }
    }
    sfclose (fp);

    return 0;
}

int saveevstate (char *file) {
    Sfio_t *fp;
    ev_t *evp;
    int evpi;
    int svcpi;

    if (!(fp = sfopen (NULL, file, "w"))) {
        SUwarning (0, "saveevstate", "open failed for file %s", file);
        return 0;
    }
    for (evpi = 0; evpi < evpm; evpi++) {
        evp = evps[evpi];
        if (evp->mark != evmark)
            continue;

        sfprintf (
            fp, "EV1|%d|%d|%s|%s|%s|%s|%s\n", evp->time, evp->type,
            evp->ccp->level, evp->ccp->id, evp->ndp->level, evp->ndp->id,
            evp->ruleevp->name
        );
        for (svcpi = 0; svcpi < evp->svcpm; svcpi++)
            sfprintf (fp, "EV2|%s\n", evp->svcps[svcpi]->name);
    }
    sfclose (fp);

    return 0;
}

ruleev_t *matchruleev (VG_alarm_t *alarmp) {
    ruleev_t *ruleevp;
    int ruleevi;

    for (ruleevi = 0; ruleevi < ruleevm; ruleevi++) {
        ruleevp = &ruleevs[ruleevi];
        if (
            regexec (
                &ruleevp->aidre, alarmp->s_alarmid, 0, NULL, REFLAGS
            ) == 0 &&
            regexec (&ruleevp->idre, alarmp->s_id1, 0, NULL, REFLAGS) == 0 &&
            regexec (&ruleevp->txtre, alarmp->s_text, 0, NULL, REFLAGS) == 0 &&
            regexec (
                &ruleevp->tmodere, tmodemap[alarmp->s_tmode], 0, NULL, REFLAGS
            ) == 0 &&
            regexec (
                &ruleevp->sevre, sevmaps[alarmp->s_severity].name,
                0, NULL, REFLAGSI
            ) == 0
        )
            return ruleevp;
    }

    return NULL;
}

ruleev_t *findruleev (char *name) {
    int ruleevi;

    for (ruleevi = 0; ruleevi < ruleevm; ruleevi++)
        if (strcmp (ruleevs[ruleevi].name, name) == 0)
            return &ruleevs[ruleevi];
    return NULL;
}

rulesvc_t *matchrulesvc (char *name) {
    rulesvc_t *rulesvcp;
    int rulesvci;

    for (rulesvci = 0; rulesvci < rulesvcm; rulesvci++) {
        rulesvcp = &rulesvcs[rulesvci];
        if (regexec (&rulesvcp->svcre, name, 0, NULL, REFLAGS) == 0)
            return rulesvcp;
    }

    return NULL;
}

int loadrules (char *file) {
    Sfio_t *fp;
    char *line, *s1, *s2, *s3, *s4, *s5, *s6, *s7;
    int tmodei;
    int sevi;
    ruleev_t *ruleevp;
    rulesvc_t *rulesvcp;

    if (!(fp = sfopen (NULL, file, "r"))) {
        SUwarning (0, "loadrules", "open failed for file %s", file);
        return -1;
    }
    while ((line = sfgetr (fp, '\n', 1))) {
        if (line[0] == '#')
            continue;

        if (strncmp (line, "RULEEV|+|", 9) == 0) {
            s1 = line + 9;
            if (
                !(s2 = strstr (s1, "|+|")) ||
                !(s3 = strstr (s2 + 3, "|+|")) ||
                !(s4 = strstr (s3 + 3, "|+|")) ||
                !(s5 = strstr (s4 + 3, "|+|")) ||
                !(s6 = strstr (s5 + 3, "|+|")) ||
                !(s7 = strstr (s6 + 3, "|+|"))
            ) {
                SUwarning (0, "loadrules", "bad ev line %s", line);
                return -1;
            }
            if (ruleevm >= ruleevn) {
                if (!(ruleevs = vmresize (
                    Vmheap, ruleevs, (ruleevn + 4) * sizeof (ruleev_t),
                    VM_RSCOPY
                ))) {
                    SUwarning (0, "loadrules", "ruleevs resize failed");
                    return -1;
                }
                ruleevn += 4;
            }
            ruleevp = &ruleevs[ruleevm++];
            memset (ruleevp, 0, sizeof (ruleev_t));
            *s2 = 0, *s3 = 0, *s4 = 0, *s5 = 0, *s6 = 0, *s7 = 0;
            s2 += 3, s3 += 3, s4 += 3, s5 += 3, s6 += 3, s7 += 3;
            if (
                !(ruleevp->name = vmstrdup (Vmheap, s1)) ||
                !(ruleevp->aidstr = vmstrdup (Vmheap, s2)) ||
                !(ruleevp->idstr = vmstrdup (Vmheap, s3)) ||
                !(ruleevp->txtstr = vmstrdup (Vmheap, s4)) ||
                !(ruleevp->tmodestr = vmstrdup (Vmheap, s5)) ||
                !(ruleevp->sevstr = vmstrdup (Vmheap, s6)) ||
                !(ruleevp->svcstr = vmstrdup (Vmheap, s7))
            ) {
                SUwarning (0, "loadrules", "strdups failed");
                return -1;
            }
            if (
                regcomp (&ruleevp->aidre, ruleevp->aidstr, REFLAGS) != 0 ||
                regcomp (&ruleevp->idre, ruleevp->idstr, REFLAGS) != 0 ||
                regcomp (&ruleevp->txtre, ruleevp->txtstr, REFLAGS) != 0 ||
                regcomp (&ruleevp->tmodere, ruleevp->tmodestr, REFLAGS) != 0 ||
                regcomp (&ruleevp->sevre, ruleevp->sevstr, REFLAGSI) != 0 ||
                regcomp (&ruleevp->svcre, ruleevp->svcstr, REFLAGS) != 0
            ) {
                SUwarning (0, "loadrules", "recomps failed");
                return -1;
            }
        } else if (strncmp (line, "RULESVC|+|", 10) == 0) {
            s1 = line + 10;
            if (
                !(s2 = strstr (s1, "|+|")) ||
                !(s3 = strstr (s2 + 3, "|+|")) ||
                !(s4 = strstr (s3 + 3, "|+|"))
            ) {
                SUwarning (0, "loadrules", "bad svc line %s", line);
                return -1;
            }
            if (rulesvcm >= rulesvcn) {
                if (!(rulesvcs = vmresize (
                    Vmheap, rulesvcs, (rulesvcn + 4) * sizeof (rulesvc_t),
                    VM_RSCOPY
                ))) {
                    SUwarning (0, "loadrules", "rulesvcs resize failed");
                    return -1;
                }
                rulesvcn += 4;
            }
            rulesvcp = &rulesvcs[rulesvcm++];
            memset (rulesvcp, 0, sizeof (rulesvc_t));
            *s2 = 0, *s3 = 0, *s4 = 0;
            s2 += 3, s3 += 3, s4 += 3;
            if (!(rulesvcp->svcstr = vmstrdup (Vmheap, s1))) {
                SUwarning (0, "loadrules", "svcstr strdup failed");
                return -1;
            }
            if (regcomp (&rulesvcp->svcre, rulesvcp->svcstr, REFLAGS) != 0) {
                SUwarning (0, "loadrules", "recomps failed");
                return -1;
            }

            for (tmodei = 0; tmodemap[tmodei]; tmodei++)
                if (strcmp (tmodemap[tmodei], s2) == 0)
                    break;
            if (!tmodemap[tmodei]) {
                SUwarning (0, "loadrules", "unknown tmode %s", s2);
                return -1;
            }
            rulesvcp->tmode = tmodei;

            for (rulesvcp->sevn = 0, s5 = s3; s5; s5 = strchr (s5 + 1, ':'))
                rulesvcp->sevn++;
            if (rulesvcp->sevn % 2 != 0) {
                SUwarning (0, "loadrules", "bad sev spec: %s", s3);
                return -1;
            }
            rulesvcp->sevn = rulesvcp->sevn / 2;
            if (!(rulesvcp->sevs = vmalloc (
                Vmheap, rulesvcp->sevn * sizeof (rulesev_t)
            ))) {
                SUwarning (0, "loadrules", "sevs malloc failed");
                return -1;
            }
            for (sevi = 0, s5 = s3; sevi < rulesvcp->sevn; sevi++) {
                if ((s5 = strchr (s3, ':')))
                    *s5++ = 0;
                rulesvcp->sevs[sevi].ratio = strtod (s3, &s6);
                if (s6 && *s6) {
                    SUwarning (0, "loadrules", "ratio copy failed");
                    return -1;
                }
                s3 = s5;
                if ((s5 = strchr (s3, ':')))
                    *s5++ = 0;
                if ((rulesvcp->sevs[sevi].sev = sevmapfind (s3)) == -1) {
                    SUwarning (0, "loadrules", "bad severity: %s", s3);
                    return -1;
                }
                s3 = s5;
            }

            if (!(rulesvcp->comstr = vmstrdup (Vmheap, s4))) {
                SUwarning (0, "loadrules", "comstr strdup failed");
                return -1;
            }
        } else {
            SUwarning (0, "loadrules", "bad line %s", line);
            return -1;
        }
    }
    sfclose (fp);

    return 0;
}
