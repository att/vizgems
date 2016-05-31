#pragma prototyped

#include <ast.h>
#include <swift.h>
#include "sched.h"

int scheduletimediff (char *timefile, time_t *tp) {
    Sfio_t *fp;
    char *line, *s;

    *tp = 0;
    if (!(fp = sfopen (NULL, timefile, "r"))) {
        SUwarning (1, "scheduletimediff", "cannot open timefile %s", timefile);
        return 0;
    }
    if (!(line = sfgetr (fp, '\n', 1))) {
        SUwarning (1, "scheduletimediff", "cannot read timefile %s", timefile);
        sfclose (fp);
        return 0;
    }
    *tp = strtol (line, &s, 10);
    if (s && *s) {
        SUwarning (0, "scheduletimediff", "cannot calculate offset %s", line);
        *tp = 0;
        sfclose (fp);
        return -1;
    }
    sfclose (fp);
    return 0;
}
