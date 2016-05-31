#pragma prototyped

#include <ast.h>
#include <swift.h>
#include <xml.h>
#include <cdt.h>

typedef struct job_t {
    Dtlink_t link;
    /* begin key */
    char *id;
    /* end key */
    struct schedule_s *sp;
} job_t;

typedef struct grp_t {
    Dtlink_t link;
    /* begin key */
    char *id;
    /* end key */
    int max, cur;
    unsigned int mark;
} grp_t;

typedef struct schedule_s {
    time_t sbod, ebod, soff, eoff, ival;
    int mode;
    XMLnode_t *np, *vnp, *anp;
    job_t *jp;
    grp_t *gp;

    time_t nt;
    int expired, scheduled, running, late;
    pid_t pid;
} schedule_t;

typedef struct schedroot_s {
    schedule_t *schedules, **sorder;
    int schedulen;

    Dt_t *jobdict;
    Dt_t *grpdict;
} schedroot_t;

extern schedroot_t *schedrootp;

typedef struct sigdata_s {
    char inuse;
    pid_t pid;
    int status;
    unsigned int mark;
} sigdata_t;

extern sigdata_t *sigdatas;
extern int sigdatan;

extern int maxjobn, curjobn;

#define RECTIME_SCHEDULE 1
#define RECTIME_CURRENT  2

int scheduleload (char *, char *);
int scheduleupdate (time_t);
int scheduleexec (char *, time_t, time_t, char *);
int schedulereap (char *, char *, time_t, int);
int schedulelog (char *, time_t);
int scheduletimediff (char *, time_t *);
int schedulets (char *, time_t);
