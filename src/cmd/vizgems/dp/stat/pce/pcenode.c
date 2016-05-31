#pragma prototyped

#include <ast.h>
#include <cdt.h>
#include <vmalloc.h>
#include <swift.h>
#include "vgpce.h"
#include "sl_mean.c"
#include "sl_sdev.c"

proffile_t *proffiles;
int proffilen;

int nodemark;

static Dt_t *nodedict;

static Dtdisc_t nodedisc = {
    sizeof (Dtlink_t), SZ_level + SZ_id + SZ_key, 0,
    NULL, NULL, NULL, NULL, NULL, NULL
};

static char cmeanfile[PATH_MAX], csdevfile[PATH_MAX];

int nodeinit (char *proffile) {
    Sfio_t *fp;
    proffile_t *pfp;
    int pfi;
    char *line, *s1, *s2;

    if (!(nodedict = dtopen (&nodedisc, Dtset))) {
        SUwarning (0, "nodeinit", "node dtopen failed");
        return -1;
    }
    proffiles = NULL;
    proffilen = 0;
    nodemark = 0;

    if (!(fp = sfopen (NULL, proffile, "r"))) {
        SUwarning (0, "nodeinit", "open failed for file %s", proffile);
        return -1;
    }
    while ((line = sfgetr (fp, '\n', 1))) {
        if (!(s1 = strchr (line, '|')) || !(s2 = strchr (s1 + 1, '|'))) {
            SUwarning (0, "nodeinit", "bad proffile line %s", line);
            continue;
        }
        *s1++ = 0, *s2++ = 0;
        pfi = atoi (line);
        if (pfi < 0) {
            SUwarning (0, "nodeinit", "bad proffile index %d", pfi);
            continue;
        }
        if (pfi >= proffilen) {
            if (!(proffiles = vmresize (
                Vmheap, proffiles, (pfi + 1) * sizeof (proffile_t), VM_RSCOPY
            ))) {
                SUwarning (0, "nodeinit", "cannot resize proffiles");
                return -1;
            }
            memset (
                &proffiles[proffilen], 0,
                (pfi + 1 - proffilen) * sizeof (proffile_t)
            );
            proffilen = pfi + 1;
        }
        pfp = &proffiles[pfi];
        if (
            !(pfp->meanfile = vmstrdup (Vmheap, s1)) ||
            !(pfp->sdevfile = vmstrdup (Vmheap, s2))
        ) {
            SUwarning (0, "nodeinit", "cannot copy files %s/%s", s1, s2);
            return -1;
        }
    }
    sfclose (fp);

    return 0;
}

int nodeterm (void) {
    node_t *np;
    int pfi;

    for (pfi = 0; pfi < proffilen; pfi++) {
        if (proffiles[pfi].meanfile)
            vmfree (Vmheap, proffiles[pfi].meanfile);
        if (proffiles[pfi].sdevfile)
            vmfree (Vmheap, proffiles[pfi].sdevfile);
    }
    vmfree (Vmheap, proffiles);
    proffilen = 0;

    while ((np = dtfirst (nodedict)))
        noderemove (np);
    if (dtclose (nodedict) == -1) {
        SUwarning (0, "termgraph", "node dtclose failed");
        return -1;
    }

    return 0;
}

node_t *nodeinsert (char *level, char *id, char *key, char *label) {
    node_t nodemem, *np;

    memset (&nodemem, 0, sizeof (node_t));
    memccpy (&nodemem.level, level, 0, SZ_level);
    memccpy (&nodemem.id, id, 0, SZ_id);
    memccpy (&nodemem.key, key, 0, SZ_key);
    if (!(np = dtsearch (nodedict, &nodemem))) {
        if (!(np = vmalloc (Vmheap, sizeof (node_t)))) {
            SUwarning (0, "nodeinsert", "node malloc failed");
            return NULL;
        }
        memset (np, 0, sizeof (node_t));
        memccpy (np->level, level, 0, SZ_level);
        memccpy (np->id, id, 0, SZ_id);
        memccpy (np->key, key, 0, SZ_key);
        if (dtinsert (nodedict, np) != np) {
            SUwarning (0, "nodeinsert", "node dtinsert failed");
            return NULL;
        }
    }
    memcpy (np->label, label, SZ_label);

    return np;
}

node_t *nodefind (char *level, char *id, char *key) {
    node_t nodemem;

    memset (&nodemem, 0, sizeof (node_t));
    memccpy (&nodemem.level, level, 0, SZ_level);
    memccpy (&nodemem.id, id, 0, SZ_id);
    memccpy (&nodemem.key, key, 0, SZ_key);
    return dtsearch (nodedict, &nodemem);
}

int noderemove (node_t *np) {
    if (np->profdatas)
        vmfree (Vmheap, np->profdatas);
    if (np->rulep)
        ruleremove (np->rulep);
    dtdelete (nodedict, np);
    vmfree (Vmheap, np);
    return 0;
}

int nodeinsertrule (node_t *np, rule_t *rp) {
    np->rulep = rp;
    np->rulemaxn = rp->maxn;
    return 0;
}

profdata_t *nodeloadprofdata (node_t *np, int framei) {
    sl_mean_t *mp;
    sl_sdev_t *sp;

    if (framei >= 0 && framei < np->profdatan && np->profdatas)
        if (np->profdatas[framei].haveprof)
            return &np->profdatas[framei];

    if (!proffiles[framei].meanfile || !proffiles[framei].sdevfile)
        return NULL;

    if (!np->profdatas) {
        if (!(np->profdatas = vmalloc (
            Vmheap, proffilen * sizeof (profdata_t)
        ))) {
            SUwarning (0, "nodeloadprofdata", "cannot allocate profdatas");
            return NULL;
        }
        np->profdatan = proffilen;
        memset (np->profdatas, 0, proffilen * sizeof (profdata_t));
    }

    if (cmeanfile[0])
        if (strcmp (cmeanfile, proffiles[framei].meanfile) != 0)
            sl_meanclose (), cmeanfile[0] = 0;
    if (!cmeanfile[0]) {
        if (sl_meanopen (proffiles[framei].meanfile) != 0) {
            vmfree (Vmheap, proffiles[framei].meanfile);
            proffiles[framei].meanfile = NULL;
        } else
            strcpy (cmeanfile, proffiles[framei].meanfile);
    }
    if (csdevfile[0])
        if (strcmp (csdevfile, proffiles[framei].sdevfile) != 0)
            sl_sdevclose (), csdevfile[0] = 0;
    if (!csdevfile[0]) {
        if (sl_sdevopen (proffiles[framei].sdevfile) != 0) {
            vmfree (Vmheap, proffiles[framei].sdevfile);
            proffiles[framei].sdevfile = NULL;
        } else
            strcpy (csdevfile, proffiles[framei].sdevfile);
    }
    if (!cmeanfile[0] || !csdevfile[0])
        return NULL;

    for (
        mp = sl_meanfindfirst (np->level, np->id, np->key); mp;
        mp = sl_meanfindnext (np->level, np->id, np->key)
    ) {
        if (mp->sl_frame >= 0 && mp->sl_frame < proffilen) {
            np->profdatas[mp->sl_frame].mean = mp->sl_val;
            np->profdatas[mp->sl_frame].havemean = TRUE;
            if (np->profdatas[mp->sl_frame].havesdev)
                np->profdatas[mp->sl_frame].haveprof = TRUE;
        }
    }
    for (
        sp = sl_sdevfindfirst (np->level, np->id, np->key); sp;
        sp = sl_sdevfindnext (np->level, np->id, np->key)
    ) {
        if (sp->sl_frame >= 0 && sp->sl_frame < proffilen) {
            np->profdatas[sp->sl_frame].sdev = sp->sl_val;
            np->profdatas[sp->sl_frame].havesdev = TRUE;
            if (np->profdatas[sp->sl_frame].havemean)
                np->profdatas[sp->sl_frame].haveprof = TRUE;
        }
    }

    if (np->profdatas[framei].haveprof)
        return &np->profdatas[framei];
    return NULL;
}

int nodeinsertalarmdata (node_t *np, int i, int sev, float val) {
    alarmdata_t *adp, *nadp;

    if (!(adp = alarmnew ())) {
        SUwarning (0, "nodeinsertalarmdata", "cannot get new alarm");
        return -1;
    }
    np->alarmdatan++;
    adp->i = i, adp->sev = sev;
    adp->val = val;
    adp->nextp = NULL;

    if (!np->firstalarmdatap)
        np->firstalarmdatap = np->lastalarmdatap = adp;
    else if (np->firstalarmdatap->i > adp->i)
        adp->nextp = np->firstalarmdatap, np->firstalarmdatap = adp;
    else {
        for (nadp = np->firstalarmdatap; nadp->nextp; nadp = nadp->nextp) {
            if (nadp->nextp->i > adp->i) {
                adp->nextp = nadp->nextp, nadp->nextp = adp;
                break;
            }
        }
        if (!nadp->nextp)
            nadp->nextp = adp, np->lastalarmdatap = adp;
    }
    np->mark = nodemark;

    return 0;
}

int nodeupdatealarms (node_t *np) {
    rulerange_t *rrp;
    int rri, minrri;
    int mini;
    alarmdata_t *adp, *lastadp;
    int newalarms;

    newalarms = FALSE;
    if (np->mark < nodemark || !np->rulep)
        goto skipnew;

    for (rri = 0; rri < np->rulep->rangen; rri++) {
        rrp = &np->rulep->ranges[rri];
        rrp->alarmdatap = alarmmatchrange (np->firstalarmdatap, rrp);
    }
    for (;;) {
        mini = INT_MAX, minrri = -1;
        for (rri = 0; rri < np->rulep->rangen; rri++) {
            rrp = &np->rulep->ranges[rri];
            if (rrp->alarmdatap && mini > rrp->alarmdatap->i)
                mini = rrp->alarmdatap->i, minrri = rri;
        }
        if (minrri != -1) {
            rrp = &np->rulep->ranges[minrri];
            if (alarmemit (rrp->alarmdatap, rrp, np, FALSE) == 0)
                newalarms = TRUE;
            for (rri = 0; rri < np->rulep->rangen; rri++) {
                rrp = &np->rulep->ranges[rri];
                if (rrp->alarmdatap && rrp->alarmdatap->i <= mini)
                    rrp->alarmdatap = alarmmatchrange (
                        rrp->alarmdatap->nextp, rrp
                    );
            }
        } else
            break;
    }

skipnew:
    if (
        newalarms || !np->rulep || np->alarmstate.mode != ASAM_ALARM ||
        np->alarmstate.time + np->alarmstate.rtime > currtime
    )
        return 0;

    lastadp = NULL;
    for (rri = 0; rri < np->rulep->rangen; rri++) {
        rrp = &np->rulep->ranges[rri];
        if (rrp->type != RRTYPE_CLEAR && rrp->sev == np->alarmstate.sev) {
            lastadp = alarmmatchrange (np->firstalarmdatap, rrp);
            for (adp = lastadp; adp; adp = adp->nextp)
                if (adp->sev == np->alarmstate.sev)
                    lastadp = adp;
            break;
        }
    }
    if (lastadp && rrp)
        alarmemit (lastadp, rrp, np, TRUE);
    return 0;
}

int allnodesupdatealarms (void) {
    node_t *np;

    for (np = dtfirst (nodedict); np; np = dtnext (nodedict, np)) {
        if (nodeupdatealarms (np) == -1) {
            SUwarning (
                0, "allnodesupdatealarms",
                "cannot update node alarms for %s:%s:%s",
                np->level, np->id, np->key
            );
            break;
        }
    }
    return 0;
}

int nodeprune (node_t *np) {
    while (
        np->firstalarmdatap && np->firstalarmdatap->i <= curri - np->rulemaxn
    ) {
        if (np->lastalarmdatap == np->firstalarmdatap)
            np->lastalarmdatap = NULL;
        np->firstalarmdatap = np->firstalarmdatap->nextp;
        np->alarmdatan--;
    }
    if (np->alarmstate.mode == ASAM_CLEAR) {
        while (np->firstalarmdatap && np->firstalarmdatap->sev == -1) {
            if (np->lastalarmdatap == np->firstalarmdatap)
                np->lastalarmdatap = NULL;
            np->firstalarmdatap = np->firstalarmdatap->nextp;
            np->alarmdatan--;
        }
    }
    if (np->alarmdatan > 0)
        np->mark = nodemark;

    return 0;
}

int allnodesprune (void) {
    node_t *np;

    for (np = dtfirst (nodedict); np; np = dtnext (nodedict, np)) {
        if (nodeprune (np) == -1) {
            SUwarning (
                0, "allnodesprune",
                "cannot prune %s:%s:%s", np->level, np->id, np->key
            );
            break;
        }
    }
    return 0;
}

int nodeloadstate (char *statefile) {
    Sfio_t *fp;
    node_t *np;
    char *line, *s1, *s2, *s3, *s4, *s5, *s6, *s7, *s8, *s9, *s10;;

    if (!(fp = sfopen (NULL, statefile, "r"))) {
        SUwarning (0, "nodeloadstate", "open failed for file %s", statefile);
        return 0;
    }
    np = NULL;
    while ((line = sfgetr (fp, '\n', 1))) {
        if (strncmp (line, "N|", 2) == 0) {
            s1 = line + 2;
            if (
                !(s2 = strchr (s1, '|')) || !(s3 = strchr (s2 + 1, '|')) ||
                !(s4 = strchr (s3 + 1, '|')) || !(s5 = strchr (s4 + 1, '|')) ||
                !(s6 = strchr (s5 + 1, '|')) || !(s7 = strchr (s6 + 1, '|')) ||
                !(s8 = strchr (s7 + 1, '|')) || !(s9 = strchr (s8 + 1, '|')) ||
                !(s10 = strchr (s9 + 1, '|'))
            ) {
                SUwarning (0, "nodeloadstate", "bad line %s", line);
                return -1;
            }
            *s2++ = 0, *s3++ = 0, *s4++ = 0, *s5++ = 0;
            *s6++ = 0, *s7++ = 0, *s8++ = 0, *s9++ = 0, *s10++ = 0;
            if (!(np = nodeinsert (s1, s2, s3, s10))) {
                SUwarning (0, "nodeloadstate", "cannot insert node");
                return -1;
            }
            np->rulemaxn = atoi (s4);
            np->alarmstate.mode = atoi (s5);
            np->alarmstate.time = atoi (s6);
            np->alarmstate.rtime = atoi (s7);
            np->alarmstate.i = atoi (s8);
            np->alarmstate.sev = atoi (s9);
        } else if (strncmp (line, "A|", 2) == 0) {
            if (!np) {
                SUwarning (0, "nodeloadstate", "A record before N record");
                return -1;
            }
            s1 = line + 2;
            if (!(s2 = strchr (s1, '|')) || !(s3 = strchr (s2 + 1, '|'))) {
                SUwarning (0, "nodeloadstate", "bad line %s", line);
                return -1;
            }
            *s2++ = 0, *s3++ = 0;
            if (nodeinsertalarmdata (
                np, atoi (s1), atoi (s2), atof (s3)
            ) == -1) {
                SUwarning (0, "nodeloadstate", "cannot insert node alarm data");
                return -1;
            }
        }
    }
    return 0;
}

int nodesavestate (char *statefile) {
    Sfio_t *fp;
    node_t *np;
    alarmdata_t *adp;
    char file[PATH_MAX];

    sfsprintf (file, PATH_MAX, "%s.tmp", statefile);
    if (!(fp = sfopen (NULL, file, "w"))) {
        SUwarning (0, "nodesavestate", "open failed for file %s", file);
        return -1;
    }
    for (np = dtfirst (nodedict); np; np = dtnext (nodedict, np)) {
        if (np->mark < nodemark)
            continue;
        sfprintf (
            fp, "N|%s|%s|%s|%d|%d|%d|%d|%d|%d|%s\n",
            np->level, np->id, np->key, np->rulemaxn,
            np->alarmstate.mode, np->alarmstate.time, np->alarmstate.rtime,
            np->alarmstate.i, np->alarmstate.sev, np->label
        );
        for (adp = np->firstalarmdatap; adp; adp = adp->nextp) {
            sfprintf (fp, "A|%d|%d|%f\n", adp->i, adp->sev, adp->val);
        }
    }
    sfclose (fp);
    if (rename (file, statefile) == -1) {
        SUwarning (0, "nodesavestate", "cannot rename state file %s", file);
        return -1;
    }

    return 0;
}
