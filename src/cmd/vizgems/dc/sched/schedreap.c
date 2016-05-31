#pragma prototyped

#include <ast.h>
#include <errno.h>
#include <vmalloc.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <swift.h>
#include <xml.h>
#include "sched.h"

static unsigned int mark;

static int postlog (char *, char *, schedule_t *);

int schedulereap (char *datadir, char *ts, time_t ct, int li) {
    schedule_t *sp;
    int si;
    sigdata_t *sdp;
    int sdi, sdm;

    curjobn = 0;
    if (++mark == 0)
        mark = 1;
    // mark which entries existed before the main loop started
    for (sdi = sdm = 0; sdi < sigdatan; sdi++) {
        sdp = &sigdatas[sdi];
        if (sdp->inuse)
            sdm = sdi + 1, sdp->mark = mark;
    }
    for (si = 0; si < schedrootp->schedulen; si++) {
        sp = &schedrootp->schedules[si];
        if (sp->gp && sp->gp->mark != mark)
            sp->gp->mark = mark, sp->gp->cur = 0;
        if (!sp->running)
            continue;

        for (sdi = 0; sdi < sdm; sdi++) {
            sdp = &sigdatas[sdi];
            if (sdp->inuse && sdp->pid == sp->pid) {
                if (postlog (datadir, ts, sp) == -1)
                    SUwarning (
                        0, "schedulereap", "cannot read log %s", sp->jp->id
                    );
                if (sdp->status == 0)
                    sfprintf (sfstderr, "%s reap %s\n", ts, sp->jp->id);
                else
                    sfprintf (
                        sfstderr, "%s reap %s (%d)\n",
                        ts, sp->jp->id, sdp->status
                    );
                sp->running = FALSE, sp->late = FALSE, sp->pid = 0;
                sdp->inuse = FALSE;
                break;
            }
        }
        if (sp->running) {
            if (!sp->late && ct > sp->nt + li * sp->ival) {
                sfprintf (sfstderr, "%s late %s\n", ts, sp->jp->id);
                sp->late = TRUE;
                if (kill (-sp->pid, SIGKILL) == -1) {
                    if (postlog (datadir, ts, sp) == -1)
                        SUwarning (
                            0, "schedulereap", "cannot read log %s", sp->jp->id
                        );
                    sfprintf (
                        sfstderr, "%s reap %s (%d)\n",
                        ts, sp->jp->id, -2
                    );
                    sp->running = FALSE, sp->late = FALSE, sp->pid = 0;
                }
            }
        }
        if (sp->running) {
            curjobn++;
            if (sp->gp)
                sp->gp->cur++;
        }
    }
    // if an entry existed before the main loop started but matched nothing
    // it must be for a job that got deleted, so just ignore
    for (sdi = 0; sdi < sdm; sdi++) {
        sdp = &sigdatas[sdi];
        if (sdp->inuse && sdp->mark == mark)
            sdp->inuse = FALSE;
    }

    SUwarning (1, "schedulereap", "active jobs %d", curjobn);

    return 0;
}

static int postlog (char *datadir, char *ts, schedule_t *sp) {
    char file[PATH_MAX], jobid[PATH_MAX], *s;
    Sfio_t *fp;

    strcpy (jobid, sp->jp->id);
    for (s = jobid; *s; s++)
        if (*s == '/')
            *s = '_';
    sfsprintf (
        file, PATH_MAX, "%s/data/%s/collector.log", datadir, jobid
    );
    if (!(fp = sfopen (NULL, file, "r"))) {
        SUwarning (1, "launch", "cannot open log file %s", file);
        return -1;
    }
    while ((s = sfgetr (fp, '\n', 1)))
        sfprintf (sfstderr, "%s %s: %s\n", ts, sp->jp->id, s);
    sfclose (fp);

    return 0;
}
