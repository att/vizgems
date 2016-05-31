#pragma prototyped

#include <ast.h>
#include <regex.h>
#include <swift.h>

typedef struct vid_s {
    int vid;
    char *value;
} vid_t;

typedef struct var_s {
    char *name;
    int type;
    union {
        char *lit;
        int vid;
    } u;
} var_t;

typedef struct alarm_s {
    char *id;
    char *textstr, *textrestr;
    char *textlcss;
    int textlcsn;
    regex_t textcode;
    char *objstr, *objrestr;
    char *objlcss;
    int objlcsn;
    regex_t objcode;
    int statenum;
    int sevnum;
    vid_t *vids;
    int vidl, vidn;
    var_t *vars;
    int varl, varn;
    char *unique;
    int tm, sm;
    char *com;
} alarm_t;

extern int alarmdbload (char *);
extern alarm_t *alarmdbfind (char *, char *, char *);
extern char *alarmdbvars (alarm_t *);
