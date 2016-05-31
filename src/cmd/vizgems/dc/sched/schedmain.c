#pragma prototyped

#include <ast.h>
#include <tm.h>
#include <option.h>
#include <errno.h>
#include <swift.h>
#include "sched.h"

static const char usage[] =
"[-1p1?\n@(#)$Id: vgsched (AT&T Labs Research) 2006-03-09 $\n]"
USAGE_LICENSE
"[+NAME?vgsched - execute the VizGEMS collection schedule]"
"[+DESCRIPTION?\bvgsched\b execute the VizGEMS collection schedule."
" \bvgsched\b reads a collection schedule and executes the actions at the"
" appropriate times."
"]"
"[100:ld?specifies the name of the log directory."
" \bvgsched\b will create a log file per hour in that directory."
" The default is \b\".\"\b."
" \bvgsched\b will create a new log file every hour."
"]:[logdir]"
"[101:ef?specifies the name of the exit file."
" \bvgsched\b checks for that file every 1 minute."
" If the file is there \bvgsched\b will exit."
"]:[exitfile]"
"[102:tf?specifies the name of the time diff file."
" \bvgsched\b checks for that file every 1 hour."
" If the file is there \bvgsched\b will read a value that indicates how"
" far off the local clock is from the server clock."
" \bvgsched\b will adjust all times it reports by that amount."
"]:[timefile]"
"[103:j?specifies the maximum number of parallel jobs."
" The default is 16."
"]#[maxjobs]"
"[104:sc?specifies the interval in seconds for checking if the schedule file"
" has been updated."
" The default is 60 secs."
"]#[seconds]"
"[105:li?specifies the maximum number of collection intervals to wait before"
" killing an unresponsive collection process."
" The default is 2 intervals."
"]#[seconds]"
"[106:rt?specifies which timestamp to use on the records."
" The default is to use the scheduled time and the option is to use the"
" current time."
"]:[(schedule|current)]"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"\n"
"\n schedule \n"
"\n"
;

schedroot_t *schedrootp;

sigdata_t *sigdatas;
int sigdatan;

int maxjobn, curjobn, rectime;

int main (int argc, char **argv) {
    int norun;
    char *datadir, *logdir, *exitfile, *timefile;
    char buf[128];

    struct timeval tv;
    time_t ct, pct, lt, tt, dt, et, st, ft;
    int nt, pnt, sc, li, ret;

    logdir = ".", exitfile = NULL, timefile = NULL, tt = dt = st = ft = 0;
    maxjobn = 16, sc = 60, li = 2;
    rectime = RECTIME_SCHEDULE;
    norun = 0;
    for (;;) {
        switch (optget (argv, usage)) {
        case -100:
            logdir = opt_info.arg;
            continue;
        case -101:
            exitfile = opt_info.arg;
            continue;
        case -102:
           timefile = opt_info.arg;
            continue;
        case -103:
            maxjobn = opt_info.num;
            continue;
        case -104:
            sc = opt_info.num;
            continue;
        case -105:
            li = opt_info.num;
            continue;
        case -106:
            if (strcmp (opt_info.arg, "schedule") == 0)
                rectime = RECTIME_SCHEDULE;
            else if (strcmp (opt_info.arg, "current") == 0)
                rectime = RECTIME_CURRENT;
            continue;
        case -999:
            SUwarnlevel++;
            continue;
        case '?':
            SUusage (0, "vgsched", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "vgsched", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;
    if (argc > 1)
        SUerror ("vgsched", "too many arguments");

    if ((sigdatan = maxjobn * 2) < 100)
        sigdatan = 100;
    if (!(sigdatas = vmresize (
        Vmheap, NULL, sigdatan * sizeof (sigdata_t), VM_RSZERO
    )))
        SUerror ("vgsched", "cannot allocate sigdatas");

    schedrootp = NULL;
    curjobn = 0;
    lt = et = tt = st = ft = ct = time (NULL);
    if (!(datadir = getenv ("VG_DSCOPESDIR")))
        SUerror ("vgsched", "cannot find data directory");
    if (logdir && schedulelog (logdir, lt) == -1)
        SUerror ("vgsched", "cannot open log file");
    if (exitfile && access (exitfile, R_OK) == 0)
        return 0;
    if (timefile && scheduletimediff (timefile, &dt) == -1)
        SUerror ("vgsched", "cannot read time diff file");
    ct -= 5;
    tmfmt (buf, 128, "%Y/%m/%d %H:%M:%S", &ct);
    sfprintf (sfstderr, "%s running\n", buf);

    if (scheduleload (argv[0], buf) == -1)
        SUerror ("vgsched", "cannot load schedule %s", argv[0]);
    pnt = -1;
    while ((nt = scheduleupdate (ct)) >= 0) {
        SUwarning (1, "msg", "sleeping for %d", nt);
        if (nt > 15)
            nt = 15;
        if (nt == 0 && pnt == 0)
            tv.tv_sec = 1, tv.tv_usec = 0;
        else
            tv.tv_sec = nt, tv.tv_usec = 0;
        pnt = nt;
        ret = select (FD_SETSIZE, NULL, NULL, NULL, &tv);
        if (ret < 0 && errno != EINTR)
            SUerror ("vgsched", "cannot select %d", errno);
        tmfmt (buf, 128, "%Y/%m/%d %H:%M:%S", &ct);
        if (schedulereap (datadir, buf, ct, li) == -1)
            SUerror ("vgsched", "cannot reap jobs");
        pct = ct, ct = time (NULL);
        if (ct < pct) {
            sfprintf (sfstderr, "dst change\n");
            break;
        }
        if (scheduleexec (datadir, ct, dt, buf) == -1)
            SUerror ("vgsched", "cannot exec jobs");
        if (exitfile && ct > et + 30) {
            if (access (exitfile, R_OK) == 0)
                break;
            et = ct;
        }
        if (sc > 0 && ct > st + sc) {
            if (scheduleload (argv[0], buf) == -1)
                SUerror ("vgsched", "cannot load schedule %s", argv[1]);
            st = ct;
        }
        if (logdir && ct > lt + 60 * 60) {
            if (schedulelog (logdir, ct) == -1)
                SUerror ("vgsched", "cannot open log file");
            lt = ct;
        }
        if (timefile && ct > tt + 60 * 60) {
            if (scheduletimediff (timefile, &dt) == -1)
                SUerror ("vgsched", "cannot read time diff file");
            tt = ct;
        }
        if (ct > ft + 5 * 60) {
            if (schedulets (datadir, ct) == -1)
                SUerror ("vgsched", "cannot touch timestamp files");
            ft = ct;
        }
    }
    tmfmt (buf, 128, "%Y/%m/%d %H:%M:%S", &ct);
    sfprintf (sfstderr, "%s exiting\n", buf);
    return 0;
}
