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

extern int rectime;

static int sigsinited;
static int launchfailures;

static char *shellpath;

static int launch (char *, schedule_t *, time_t, time_t);

static void sigchldhandler (int);

int scheduleexec (char *datadir, time_t ct, time_t dt, char *ts) {
    schedule_t *sp;
    int si;
    int ret;

    for (si = 0; si < schedrootp->schedulen; si++) {
        sp = schedrootp->sorder[si];
        if (sp->running || sp->expired || !sp->scheduled || ct < sp->nt)
            continue;

        if (sp->gp && sp->gp->cur >= sp->gp->max) {
            if (sp->nt + sp->ival < ct) {
                sfprintf (
                    sfstderr, "%s incomplete group %d/%d/%d\n",
                    ts, (si + 1), schedrootp->schedulen, sp->gp->max
                );
            }
            continue;
        }

        if (curjobn > maxjobn) {
            if (sp->nt + sp->ival < ct) {
                sfprintf (
                    sfstderr, "%s incomplete %d/%d\n",
                    ts, (si + 1), schedrootp->schedulen
                );
                break;
            }
            continue;
        }

        sfprintf (sfstderr, "%s exec %s\n", ts, sp->jp->id);
        if ((ret = launch (datadir, sp, ct, dt)) == -1) {
            SUwarning (0, "scheduleexec", "cannot launch %s", sp->jp->id);
            return -1;
        } else if (ret == 1) {
            sfprintf (sfstderr, "%s waiting to launch %s\n", ts, sp->jp->id);
            if (launchfailures++ > 100) {
                SUwarning (0, "scheduleexec", "too many launch failures");
                return -1;
            }
            break;
        }
        sp->scheduled = FALSE, sp->running = TRUE, sp->late = FALSE;
        sp->nt = ct;
        launchfailures = 0;
        curjobn++;
        if (sp->gp)
            sp->gp->cur++;
    }

    return 0;
}

static int launch (char *datadir, schedule_t *sp, time_t ct, time_t dt) {
    Sfio_t *fp;
    char dir[PATH_MAX], file[PATH_MAX], jobid[PATH_MAX], cmd[1000], *s;
    char *argv[10];

    if (!sigsinited) {
        sigset (SIGCHLD, sigchldhandler);
        sigset (SIGPIPE, SIG_IGN);
        sigsinited = TRUE;
    }

    if (!shellpath) {
        if (!(shellpath = getenv ("SHELL")))
            shellpath = "/bin/ksh";
    }

    strcpy (jobid, sp->jp->id);
    for (s = jobid; *s; s++)
        if (*s == '/')
            *s = '_';
    sfsprintf (dir, PATH_MAX, "%s/data/%s", datadir, jobid);
    if (access (dir, R_OK | W_OK | X_OK) == -1) {
        if (mkdir (dir, 0777) == -1) {
            SUwarning (1, "launch", "cannot create dir %s", dir);
            return -1;
        }
    }

    sfsprintf (file, PATH_MAX, "%s/spec.sh", dir);
    if (!(fp = sfopen (NULL, file, "w"))) {
        if (errno == EMFILE)
            return 1;
        SUwarning (1, "launch", "cannot open spec file %s", file);
        return -1;
    }
    XMLwriteksh (fp, sp->np, 0, TRUE);
    sfclose (fp);

    sfsprintf (
        cmd, 1000, "vg_collector %s %s %s %ld %ld collector.log",
        dir, jobid, "spec.sh",
        (rectime == RECTIME_SCHEDULE) ? sp->nt + dt : ct + dt, sp->ival
    );

    argv[0] = "ksh", argv[1] = "-c", argv[2] = cmd, argv[3] = NULL;
    if ((sp->pid = spawnveg (shellpath, argv, NULL, 1)) == -1) {
        SUwarning (1, "launch", "spawn failed");
        return 1;
    }

    return 0;
}

static void sigchldhandler (int data) {
    sigdata_t *sdp;
    int sdi;
    pid_t pid;
    int status;

    while ((pid = waitpid (-1, &status, WNOHANG)) > 0 && !WIFSTOPPED (status)) {
        for (sdi = 0; sdi < sigdatan; sdi++) {
            sdp = &sigdatas[sdi];
            if (!sdp->inuse) {
                sdp->pid = pid;
                if (WIFEXITED (status))
                    sdp->status = WEXITSTATUS (status);
                else
                    sdp->status = -1;
                sdp->mark = 0;
                sdp->inuse = TRUE;
                break;
            }
        }
        if (sdi == sigdatan)
            SUwarning (0, "sigchld", "out of sig data space");
    }
}
