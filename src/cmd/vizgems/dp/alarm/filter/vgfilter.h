#pragma prototyped

#include <ast.h>
#include <tm.h>
#include <regex.h>
#include <swift.h>

#define S_level1 VG_alarm_level1_N
#define S_id1 VG_alarm_id1_N
#define S_key VG_inv_node_key_N
#define S_val VG_inv_node_val_N

typedef struct filter_s {
    char *objstr, *idstr, *textstr;
    int objreflag, nameflag, idreflag, textreflag;
    int objcn, idcn, textcn;
    regex_t objcode, idcode, textcode;
    int tm, sm;
    int sevnum;
    time_t st, et, sdate, edate;
    int repeat;
    char *com;
    char *account;
    int ini;
} filter_t;

#ifndef FILTERLISTONLY
typedef struct inv_s {
    char *account, level[S_level1], *idre;
    regex_t code;
    int acctype, reflag, compflag;
} inv_t;
#endif

extern int filterload (char *, char *);
extern filter_t *filterfind (char *, char *, char *, char *, char *, int);
extern filter_t *filterlistfirst (char *, char **, int);
extern filter_t *filterlistnext (char *, char **, int);
