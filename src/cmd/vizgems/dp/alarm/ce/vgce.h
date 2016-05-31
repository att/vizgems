#pragma prototyped

#include <ast.h>
#include <swift.h>
#include "vg_hdr.h"
#include "vg_sevmap.c"

#define CE_VAR_ALARM_FIRST     0
#define CE_VAR_ALARM_ALARMID   0
#define CE_VAR_ALARM_STATE     1
#define CE_VAR_ALARM_SMODE     2
#define CE_VAR_ALARM_TMODE     3
#define CE_VAR_ALARM_LEVEL1    4
#define CE_VAR_ALARM_OBJECT1   5
#define CE_VAR_ALARM_LEVEL2    6
#define CE_VAR_ALARM_OBJECT2   7
#define CE_VAR_ALARM_TIME      8
#define CE_VAR_ALARM_SEVERITY  9
#define CE_VAR_ALARM_TEXT     10
#define CE_VAR_ALARM_COMMENT  11
#define CE_VAR_ALARM_LAST     11

#define CE_VAR_MATCH_FIRST      12
#define CE_VAR_MATCH_FALSE      12
#define CE_VAR_MATCH_TRUE       13
#define CE_VAR_MATCH_PRUNEMATCH 14
#define CE_VAR_MATCH_PRUNEALARM 15
#define CE_VAR_MATCH_RESULT     16
#define CE_VAR_MATCH_CTIME      17
#define CE_VAR_MATCH_TTIME      18
#define CE_VAR_MATCH_PTIME      19
#define CE_VAR_MATCH_FATIME     20
#define CE_VAR_MATCH_LATIME     21
#define CE_VAR_MATCH_FIELDS     22
#define CE_VAR_MATCH_LAST       22

#define CE_VAL_FALSE      0
#define CE_VAL_TRUE       1
#define CE_VAL_PRUNEMATCH 2
#define CE_VAL_PRUNEALARM 3

typedef struct var_s {
    int id;
    char *nam;
    EEval_t val;
} var_t;

typedef struct alarm_s {
    int sn, rc;
    VG_alarm_t data;
    var_t *vars; // vars[vari].u.s points in alarm data - do no free
    int varn;
    int mark;
} alarm_t;

/* do not change the numbering of these: */
#define CE_EXPR_FMATCH  0
#define CE_EXPR_SMATCH  1
#define CE_EXPR_INSERT  2
#define CE_EXPR_REMOVE  3
#define CE_EXPR_FTICKET 4
#define CE_EXPR_STICKET 5
#define CE_EXPR_PRUNE   6
#define CE_EXPR_EMIT    7
#define CE_EXPR_SIZE    8

typedef struct rule_s {
    int rc;
    char *id;
    char *descr;
    EE_t ees[CE_EXPR_SIZE];
    int eeflags[CE_EXPR_SIZE];
    var_t *vars; // vars[vari].u.s is allocated and must be freed
    int varn;
    int fres, sres, vres;
    int mark;
} rule_t;

typedef struct match_s {
    int inuse;
    int rulei;
    alarm_t **alarms;
    int alarml, alarmn;
    var_t *vars;
    int varn;
    int ttime; // parent cc_t's ticket time
    int fatime, latime; // oldest / newest alarm timestamps
    int maxtmode;
} match_t;
#define CE_MATCH_ALARMINCR 256

typedef struct cc_s {
    int inuse;
    int ticketed;
    int ttime;
    match_t *matchs;
    int matchm, matchn;
} cc_t;
#define CE_CC_MATCHINCR 2

typedef struct root_s {
    rule_t *rules;
    int rulel, rulen;
    cc_t *ccs;
    int ccl, ccm, ccn;
    char ccidprefix[VG_alarm_ccid_L];
    int alarmdatasize, alarmn, alarmsn;
    int currtime;
    int mark;
    struct stat sb;
} root_t;
#define CE_ROOT_RULEINCR 16
#define CE_ROOT_CCINCR 1024

typedef struct context_s {
    match_t *matchp;
    alarm_t *alarmp;
    int eei;
    int peval;
} context_t;

extern root_t *rootp;

extern char *varmap[];
extern char *statemap[], *typemap[], *modemap[], *pmodemap[];
extern char *ftmode, *rtmode;

/* misc.c */
extern int CEinit (char *, char *, int, int, int, int, char *);
extern int CEterm (char *);

/* alarms.c */
extern alarm_t *insertalarm (VG_alarm_t *);
extern int removealarm (alarm_t *);
extern int getalarmvar (alarm_t *, int, char *, EEval_t *, void *);
extern int setalarmvar (alarm_t *, int, char *, EEval_t *, void *);
extern int alarmhasvars (alarm_t *, var_t *, int);
extern void printalarms (int);
extern void printalarm (alarm_t *, int);

/* rules.c */
extern int initrules (void);
extern int termrules (void);
extern int loadrules (char *);
extern int insertrule (void);
extern int removerule (int);
extern void packrules (void);
extern void printrules (int);
extern void printrule (int, int);

/* match.c */
extern int insertmatches (VG_alarm_t *);
extern int removematchesbytime (void);
extern int removematchesbyrule (int);
extern int removematchesbyclear (VG_alarm_t *, char);
extern match_t *insertmatch (int, int);
extern int removematch (int, match_t *);
extern int insertmatchalarm (int, match_t *, alarm_t *);
extern int removematchalarm (int, match_t *, int);
extern int getmatchvar (match_t *, int, char *, EEval_t *, void *);
extern int setmatchvar (match_t *, int, char *, EEval_t *, void *);
extern int runee (context_t *, int *);
extern void printmatch (match_t *, int);

/* cc.c */
extern int insertcc (void);
extern int removecc (int);
extern match_t *insertccmatch (int);
extern int removeccmatch (int, match_t *);
extern void printccs (int);

/* ticket.c */
extern int updatetickets (void);
extern int emitrecords (void);
extern int simpleticket (VG_alarm_t *);
extern int closeanyticket (VG_alarm_t *);
