#pragma prototyped

#include <ast.h>
#include <tm.h>
#include <regex.h>
#include <swift.h>
#ifndef VG_DEFS_MAIN
#include "vg_hdr.h"
#endif

#define S_level1 VG_alarm_level1_N
#define S_id1 VG_alarm_id1_N
#define S_key VG_inv_node_key_N
#define S_val VG_inv_node_val_N

#define STYLE_SMS  1
#define STYLE_TEXT 2
#define STYLE_HTML 3

typedef struct email_s {
    char *objstr, *idstr, *textstr;
    int objreflag, nameflag, idreflag, textreflag;
    int objcn, idcn, textcn;
    regex_t objcode, idcode, textcode;
    int tmode;
    int sevnum;
    time_t st, et, sdate, edate;
    int repeat;
    char *faddr, *taddr, *subject;
    int style;
    char *account;
    int ini;
} email_t;

#ifndef EMAILLISTONLY
typedef struct inv_s {
    char *account, level[S_level1], *idre;
    regex_t code;
    int acctype, reflag, compflag;
} inv_t;
#endif

extern int emailload (char *, char *);
extern email_t *emailfindfirst (char *, char *, char *, char *, int, int, int);
extern email_t *emailfindnext (char *, char *, char *, char *, int, int, int);
extern email_t *emaillistfirst (char *, char *);
extern email_t *emaillistnext (char *, char *);
