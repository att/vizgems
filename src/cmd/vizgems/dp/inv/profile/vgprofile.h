#pragma prototyped

#include <ast.h>
#include <regex.h>
#include <swift.h>

#define S_level VG_inv_node_level_N
#define S_id VG_inv_node_id_N
#define S_key VG_inv_node_key_N
#define S_val VG_inv_node_val_N

typedef struct profile_s {
    char *idstr;
    int idreflag, nameflag;
    int idcn;
    regex_t idcode;
    char *sname, *sstatus, *svalue;
    char *account;
    int ini;
} profile_t;

typedef struct default_s {
    Dtlink_t link;
    /* begin key */
    char *slst;
    /* end key */

    char *sname, *svalue;
} default_t;

typedef struct sname_s {
    Dtlink_t link;
    /* begin key */
    char *sname;
    /* end key */
} sname_t;

#ifndef PROFILELISTONLY
typedef struct inv_s {
    char *account, level[S_level], *idre;
    regex_t code;
    int acctype, reflag, compflag;
} inv_t;
#endif

extern int profileload (char *, char *, char *);
extern profile_t *profilefindfirst (char *, char *);
extern profile_t *profilefindnext (char *, char *);
extern default_t *defaultfindfirst (char *, char *);
extern default_t *defaultfindnext (char *, char *);
extern profile_t *profilelistfirst (char *, char *);
extern profile_t *profilelistnext (char *, char *);
extern char *svalueparse (char *, char *, char *, char *);
