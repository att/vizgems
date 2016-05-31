#pragma prototyped

#include <ast.h>
#include <option.h>
#include <proc.h>
#include <swift.h>

static const char usage[] =
"[-1p1s1D?\n@(#)$Id: suinewpgrp (AT&T Labs Research) 2001-09-24 $\n]"
USAGE_LICENSE
"[+NAME?suinewpgrp - run specified job in a new process group]"
"[+DESCRIPTION?\bsuinewpgrp\b runs the specified job in a new process group."
" This is used by SWIFT CGI scripts to make it easy to kill all processes"
" of a CGI execution when a timer expires."
"]"
"[102:log?specifies the log file to use."
" The default is to print log entries on stderr."
"]:[logfile]"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"\n"
"\n\"command arguments\"\n"
"\n"
"[+SEE ALSO?\bsuidserver\b(1)]"
;

int main (int argc, char **argv) {
    int norun;
    char *logfile, *cmd;
    Sfio_t *fp;

    logfile = NULL;
    norun = 0;
    for (;;) {
        switch (optget (argv, usage)) {
        case -102:
            logfile = opt_info.arg;
            continue;
        case -999:
            SUwarnlevel++;
            continue;
        case '?':
            SUusage (0, "suinewpgrp", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "suinewpgrp", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;
    if (argc < 1)
        SUerror ("suinewpgrp", "usage: %s", optusage (NULL));

    cmd = argv[0];
    if (logfile) {
        if (!(fp = sfopen (NULL, logfile, "w")))
            SUerror ("suinewpgrp", "failed to open log file");
    } else
        fp = sfstderr;
    sfprintf (fp, "%d\n", getpid ());
    if (logfile)
        sfclose (fp);
    if (!procopen (cmd, argv, NULL, NULL, PROC_OVERLAY | PROC_SESSION))
        SUerror ("suinewpgrp", "procopen failed");
    return 0;
}
