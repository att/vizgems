#pragma prototyped

#include <ast.h>
#include <errno.h>
#include <tm.h>
#include <sys/types.h>
#include <swift.h>
#include <xml.h>
#include "sched.h"

int schedulelog (char *logdir, time_t t) {
    char logfile[PATH_MAX], buf[1024];

    tmfmt (buf, 1024, "%Y%m%d-%H%M%S", &t);
    sfsprintf (logfile, PATH_MAX, "%s/schedule.%s.log", logdir, buf);
    if (!sfopen (sfstderr, logfile, "w")) {
        SUwarning (0, "schedulelog", "cannot create logfile %s", logfile);
        return -1;
    }
    return 0;
}
