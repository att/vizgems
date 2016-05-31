#pragma prototyped

#include <ast.h>
#include <regex.h>
#include <swift.h>

#define S_level VG_inv_node_level_N
#define S_id VG_inv_node_id_N
#define S_key VG_inv_node_key_N
#define S_val VG_inv_node_val_N

typedef struct threshold_s {
    char *idstr;
    int idreflag, nameflag;
    int idcn;
    regex_t idcode;
    char *sname, *sstatus, *svalue;
    char *account;
    int ini;
    char *cid;
} threshold_t;

#ifndef THRESHOLDLISTONLY
typedef struct inv_s {
    char *account, level[S_level], *idre;
    regex_t code;
    int acctype, reflag, compflag;
} inv_t;
#endif

extern int thresholdload (char *, char *);
extern threshold_t *thresholdfindfirst (char *, char *);
extern threshold_t *thresholdfindnext (char *, char *);
extern threshold_t *thresholdlistfirst (char *, char *);
extern threshold_t *thresholdlistnext (char *, char *);
