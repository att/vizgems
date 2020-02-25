#pragma prototyped

#include <ast.h>
#include <tm.h>
#include <vmalloc.h>
#include <swift.h>
#include <xml.h>
#include <ctype.h>
#include "sched.h"

#ifdef FEATURE_NO64FS
typedef off_t off64_t;
#define stat64 stat
#endif

typedef struct n2i_s {
    char *name;
    int id;
} n2i_t;

static n2i_t modemap[] = {
    { "once",    1 },
    { "daily",   2 },
    { "weekly",  3 },
    { "monthly", 4 },
    { NULL,      0 },
};

static Void_t *myheapmem (Vmalloc_t *, Void_t *, size_t, size_t, Vmdisc_t *);
static Vmdisc_t myheap = { myheapmem, NULL, 1024 * 1024 };
static Vmalloc_t *vm;
static time_t mt;

static int mapn2i (n2i_t *, char *);

static Void_t *dtmem (Dt_t *, Void_t *, size_t, Dtdisc_t *);

static Dtdisc_t jobdisc = {
    sizeof (Dtlink_t), -1, 0, NULL, NULL, NULL, NULL, dtmem, NULL
};
static Dtdisc_t grpdisc = {
    sizeof (Dtlink_t), -1, 0, NULL, NULL, NULL, NULL, dtmem, NULL
};

static int cmp (const void *, const void *);

int scheduleload (char *name, char *ts) {
    Vmalloc_t *pvm;
    XMLnode_t *xmlroot;
    Sfio_t *fp;
    schedroot_t *srp;
    schedule_t *sp;
    job_t *jp, *pjp;
    grp_t *gp, *pgp;
    int sl, gmax;
    XMLnode_t *np, *np1, *np2, *np3, *jnp;
    int ni;
    char sdbuf[100], edbuf[100];
    char *sts[3 + 1], *ets[3 + 1];
    int stn, etn;
    char *s1, *js;
    int count, ival;
    struct stat64 sb;

    if (stat64 (name, &sb) == -1) {
        SUwarning (0, "scheduleload", "cannot stat schedule file %s", name);
        return 0;
    }

    SUwarning (
        1, "scheduleload", "in load %ld %ld %d",
        mt, sb.st_mtime, mt <= sb.st_mtime
    );
    if (mt > 0 && mt >= sb.st_mtime)
        return 0;

    sfprintf (sfstderr, "%s loading schedule file %s\n", ts, name);

    pvm = vm;
#ifdef _UWIN
    if (!(vm = vmopen (Vmdcsbrk, Vmbest, 0))) {
#else
    if (!(vm = vmopen (&myheap, Vmbest, 0))) {
#endif
        SUwarning (0, "scheduleload", "cannot create region");
        return -1;
    }

    if (!(fp = sfopen (NULL, name, "r"))) {
        SUwarning (0, "scheduleload", "cannot open schedule file");
        return -1;
    }
    if (!(xmlroot = XMLreadfile (fp, "cfg", TRUE, vm))) {
        SUwarning (0, "scheduleload", "cannot load schedule file");
        return -1;
    }

    if (!(srp = vmalloc (vm, sizeof (schedroot_t)))) {
        SUwarning (0, "scheduleload", "cannot allocate scheddata root");
        return -1;
    }
    if (!(srp->schedules = vmresize (
        vm, NULL, sizeof (schedule_t) * (xmlroot->nodem), VM_RSZERO
    ))) {
        SUwarning (0, "scheduleload", "cannot allocate schedules array");
        return -1;
    }
    if (!(srp->sorder = vmresize (
        vm, NULL, sizeof (schedule_t *) * (xmlroot->nodem), VM_RSZERO
    ))) {
        SUwarning (0, "scheduleload", "cannot allocate sorder array");
        return -1;
    }
    if (!(srp->jobdict = dtopen (&jobdisc, Dtset))) {
        SUwarning (0, "scheduleload", "cannot create jobdict");
        return -1;
    }
    if (!(srp->grpdict = dtopen (&grpdisc, Dtset))) {
        SUwarning (0, "scheduleload", "cannot create grpdict");
        return -1;
    }

    sl = 0;
    for (ni = 0; ni < xmlroot->nodem; ni++) {
        if (xmlroot->nodes[ni]->type == XML_TYPE_TEXT)
            continue;

        np = xmlroot->nodes[ni];
        if (strcmp (np->tag, "cfg") != 0)
            continue;

        if (!(jnp = XMLfind (np, "jobid", XML_TYPE_TEXT, -1, TRUE))) {
            SUwarning (0, "scheduleload", "spec %d: cannot find jobid tag", ni);
            continue;
        }
        if (!jnp->text[0]) {
            SUwarning (0, "scheduleload", "spec %d: empty jobid", ni);
            continue;
        }
        js = jnp->text;

        sp = &srp->schedules[sl];
        srp->sorder[sl] = &srp->schedules[sl];
        sp->np = np;

        if (!(np1 = XMLfind (np, "schedule", XML_TYPE_TAG, -1, TRUE))) {
            SUwarning (0, "scheduleload", "%s: cannot find schedule tag", js);
            continue;
        }

        if (!(np2 = XMLfind (np1, "sdate", XML_TYPE_TEXT, -1, TRUE))) {
            SUwarning (0, "scheduleload", "%s: cannot find sdate tag", js);
            continue;
        }
        sfsprintf (sdbuf, 100, "%s-00:00:00", np2->text);
        sp->sbod = tmdate (sdbuf, &s1, NULL);
        if (*s1) {
            SUwarning (
                0, "scheduleload", "%s: cannot parse start date: %s", js, sdbuf
            );
            continue;
        }

        if (!(np2 = XMLfind (np1, "edate", XML_TYPE_TEXT, -1, TRUE))) {
            SUwarning (0, "scheduleload", "%s: cannot find edate tag", js);
            continue;
        }
        sfsprintf (edbuf, 100, "%s-00:00:00", np2->text);
        sp->ebod = tmdate (edbuf, &s1, NULL);
        if (*s1) {
            SUwarning (
                0, "scheduleload", "%s: cannot parse end date: %s", js, edbuf
            );
            continue;
        }

        if (!(np2 = XMLfind (np1, "stime", XML_TYPE_TEXT, -1, TRUE))) {
            SUwarning (0, "scheduleload", "%s: cannot find stime tag", js);
            continue;
        }
        stn = 0, sts[stn++] = np2->text;
        for (s1 = np2->text; *s1; ) {
            if (*s1 == ':') {
                *s1++ = 0;
                if (*s1 && stn < 4)
                    sts[stn++] = s1;
            } else if (!isdigit (*s1)) {
                stn = -1;
                break;
            } else
                s1++;
        }
        if (stn < 1 || stn > 3) {
            SUwarning (0, "scheduleload", "%s: bad start time", js);
            continue;
        }
        switch (stn) {
        case 1:
            sp->soff = atoi (sts[0]) * 60 * 60;
            break;
        case 2:
            sp->soff = atoi (sts[0]) * 60 * 60;
            sp->soff += atoi (sts[1]) * 60;
            break;
        case 3:
            sp->soff = atoi (sts[0]) * 60 * 60;
            sp->soff += atoi (sts[1]) * 60;
            sp->soff += atoi (sts[2]);
            break;
        }

        if (!(np2 = XMLfind (np1, "etime", XML_TYPE_TEXT, -1, TRUE))) {
            SUwarning (0, "scheduleload", "%s: cannot find etime tag", js);
            continue;
        }
        etn = 0, ets[etn++] = np2->text;
        for (s1 = np2->text; *s1; ) {
            if (*s1 == ':') {
                *s1++ = 0;
                if (*s1 && etn < 4)
                    ets[etn++] = s1;
            } else if (!isdigit (*s1)) {
                etn = -1;
                break;
            } else
                s1++;
        }
        if (etn < 1 || etn > 3) {
            SUwarning (0, "scheduleload", "%s: bad end time", js);
            continue;
        }
        switch (etn) {
        case 1:
            sp->eoff = atoi (ets[0]) * 60 * 60;
            break;
        case 2:
            sp->eoff = atoi (ets[0]) * 60 * 60;
            sp->eoff += atoi (ets[1]) * 60;
            break;
        case 3:
            sp->eoff = atoi (ets[0]) * 60 * 60;
            sp->eoff += atoi (ets[1]) * 60;
            sp->eoff += atoi (ets[2]);
            break;
        }

        if (!(np2 = XMLfind (np1, "mode", XML_TYPE_TEXT, -1, TRUE))) {
            SUwarning (0, "scheduleload", "%s: cannot find mode tag", js);
            continue;
        }
        if ((sp->mode = mapn2i (modemap, np2->text)) == -1) {
            SUwarning (
                0, "scheduleload", "%s: cannot map mode: %s", js, np2->text
            );
            continue;
        }

        if (!(np2 = XMLfind (np1, "freq", XML_TYPE_TAG, -1, TRUE))) {
            SUwarning (0, "scheduleload", "%s: cannot find freq tag", js);
            continue;
        }
        if (!(np3 = XMLfind (np2, "count", XML_TYPE_TEXT, -1, TRUE))) {
            SUwarning (0, "scheduleload", "%s: cannot find count tag", js);
            continue;
        }
        count = strtol (np3->text, &s1, 10);
        if (*s1) {
            SUwarning (
                0, "scheduleload", "%s: cannot parse count: %s", js, np3->text
            );
            continue;
        }
        if (!(np3 = XMLfind (np2, "ival", XML_TYPE_TEXT, -1, TRUE))) {
            SUwarning (0, "scheduleload", "%s: cannot find count tag", js);
            continue;
        }
        ival = strtol (np3->text, &s1, 10);
        if (*s1) {
            SUwarning (
                0, "scheduleload", "%s: cannot parse ival: %s", js, np3->text
            );
            continue;
        }
        if (count == 0)
            sp->ival = 0;
        else
            if ((sp->ival = ival / count) <= 0)
                sp->ival = 1;

        if (!(np1 = XMLfind (np, "vars", XML_TYPE_TAG, -1, TRUE))) {
            SUwarning (0, "scheduleload", "%s: cannot find vars tag", js);
            continue;
        }
        sp->vnp = np1;

        if (!(np1 = XMLfind (np, "alarms", XML_TYPE_TAG, -1, TRUE))) {
            SUwarning (0, "scheduleload", "%s: cannot find alarms tag", js);
            continue;
        }
        sp->anp = np1;

        if (!(np1 = XMLfind (np, "jobid", XML_TYPE_TEXT, -1, TRUE))) {
            SUwarning (0, "scheduleload", "%s: cannot find jobid tag", js);
            continue;
        }
        if (!np1->text[0]) {
            SUwarning (0, "scheduleload", "%s: empty jobid", js);
            continue;
        }
        if (dtmatch (srp->jobdict, np1->text)) {
            SUwarning (
                0, "scheduleload", "%s: duplicate jobid %s", js, np1->text
            );
            continue;
        }
        if (!(jp = vmalloc (vm, sizeof (job_t)))) {
            SUwarning (0, "scheduleload", "%s: cannot allocate jp", js);
            continue;
        }
        memset (jp, 0, sizeof (job_t));
        if (!(jp->id = vmstrdup (vm, np1->text))) {
            SUwarning (0, "scheduleload", "%s: cannot copy job id", js);
            continue;
        }
        if (dtinsert (srp->jobdict, jp) != jp) {
            SUwarning (0, "scheduleload", "%s: cannot insert job", js);
            continue;
        }
        sp->jp = jp, jp->sp = sp;

        gp = NULL;
        if ((np1 = XMLfind (np, "group", XML_TYPE_TAG, -1, TRUE))) {
            if (!(np2 = XMLfind (np1, "id", XML_TYPE_TEXT, -1, TRUE))) {
                SUwarning (
                    0, "scheduleload", "%s: cannot find group id tag", js
                );
                continue;
            }
            if (!np2->text[0]) {
                SUwarning (0, "scheduleload", "%s: empty group id", js);
                continue;
            }
            if (!(np3 = XMLfind (np1, "max", XML_TYPE_TEXT, -1, TRUE))) {
                SUwarning (
                    0, "scheduleload", "%s: cannot find group max tag", js
                );
                continue;
            }
            if (!np3->text[0]) {
                SUwarning (0, "scheduleload", "%s: empty group max", js);
                continue;
            }
            if ((gmax = atoi (np3->text)) < 1) {
                SUwarning (0, "scheduleload", "%s: grpmax less than 1", js);
                continue;
            }
            if (!(gp = dtmatch (srp->grpdict, np2->text))) {
                if (!(gp = vmalloc (vm, sizeof (grp_t)))) {
                    SUwarning (0, "scheduleload", "%s: cannot allocate gp", js);
                    continue;
                }
                memset (gp, 0, sizeof (grp_t));
                if (!(gp->id = vmstrdup (vm, np2->text))) {
                    SUwarning (0, "scheduleload", "%s: cannot copy grp id", js);
                    continue;
                }
                if (dtinsert (srp->grpdict, gp) != gp) {
                    SUwarning (0, "scheduleload", "%s: cannot insert grp", js);
                    continue;
                }
            }
            if (gp->max < gmax)
                gp->max = gmax;
        }
        sp->gp = gp;

        sp->scheduled = FALSE;

        /* copy state from previous schedule */
        if (schedrootp && (pjp = dtmatch (schedrootp->jobdict, jp->id))) {
            jp->sp->nt = pjp->sp->nt;
            jp->sp->expired = pjp->sp->expired;
            jp->sp->scheduled = pjp->sp->scheduled;
            jp->sp->running = pjp->sp->running;
            jp->sp->late = pjp->sp->late;
            jp->sp->pid = pjp->sp->pid;
        }
        if (schedrootp && gp && (pgp = dtmatch (schedrootp->grpdict, gp->id))) {
            gp->cur = pgp->cur;
        }

        sl++;
    }
    sfclose (fp);
    srp->schedulen = sl;
    qsort (srp->sorder, sl, sizeof (schedule_t *), cmp);

    schedrootp = srp;

    if (pvm)
        vmclose (pvm), pvm = NULL;

    mt = sb.st_mtime;

    sfprintf (
        sfstderr, "%s loaded schedule file %s %d entries\n", ts, name, sl
    );

    return 0;
}

static int mapn2i (n2i_t *maps, char *name) {
    int mapi;

    for (mapi = 0; maps[mapi].name; mapi++)
        if (strcasecmp (maps[mapi].name, name) == 0)
            return maps[mapi].id;
    return -1;
}

static Void_t *dtmem (Dt_t *dt, Void_t *addr, size_t size, Dtdisc_t *disc) {
    if (addr) {
        if (size == 0) {
            vmfree (vm, addr);
            return NULL;
        } else
            return vmresize (vm, addr, size, VM_RSCOPY);
    } else
        return (size > 0) ? vmalloc (vm, size) : NULL;
}

static Void_t *myheapmem (
    Vmalloc_t *vm, Void_t *caddr, size_t csize, size_t nsize, Vmdisc_t *disc
) {
    if (csize == 0)
        return vmalloc (Vmheap, nsize);
    else if (nsize == 0)
        return (vmfree (Vmheap, caddr) >= 0) ? caddr : NULL;
    else
        return vmresize (Vmheap, caddr, nsize, 0);
}

static int cmp (const void *a, const void *b) {
    schedule_t *asp, *bsp;

    asp = *(schedule_t **) a;
    bsp = *(schedule_t **) b;
    if (bsp->ival != asp->ival)
        return asp->ival - bsp->ival;
    if (!asp->gp && !bsp->gp)
        return 0;
    if (asp->gp && !bsp->gp)
        return -1;
    if (!asp->gp && bsp->gp)
        return 1;
    return bsp->gp->max - asp->gp->max;
}
