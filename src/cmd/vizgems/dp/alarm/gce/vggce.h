#pragma prototyped

#ifndef VGGCE_H
#define VGGCE_H

#include <ast.h>
#include <cdt.h>
#include <regex.h>
#include <swift.h>
#include "vg_hdr.h"
#include "vg_sevmap.c"

#define SZ_level VG_alarm_level1_L
#define SZ_id VG_alarm_id1_L

typedef struct cc_s {
    Dtlink_t link;
    /* begin key */
    char level[SZ_level], id[SZ_id];
    /* end key */

    int fullflag;
    struct nd_s **ndps, **ndsts;
    int ndpn, ndpm;
    int ndsti;
    struct svc_s **svcps;
    int svcpn, svcpm;

    int mark;
} cc_t;

#define ND_STATE_UNUSED  0
#define ND_STATE_ONSTACK 1
#define ND_STATE_ONPATH  2
typedef struct nd_s {
    Dtlink_t link;
    /* begin key */
    char level[SZ_level], id[SZ_id];
    /* end key */

    int index, state, downflag;
    int fullflag;
    struct svcm_s *svcms, *svcmp;
    int svcmn, svcmm;
    struct ed_s *eds;
    int edn, edm;
    struct cc_s *ccp;
    struct ev_s **evps;
    int evpn, evpm;
    int v;

    int mark;
} nd_t;

typedef struct ed_s {
    struct nd_s *ndp;
} ed_t;

#define SVC_ROLE_PROD 1
#define SVC_ROLE_CONS 2
#define SVC_MODE_NOEV  0
#define SVC_MODE_OLDEV 1
#define SVC_MODE_NEWEV 2
#define SVC_MODE_PRUNE 3
#define SVC_MODE_SIZE  4
typedef struct svcm_s {
    struct svc_s *svcp;
    int role;
    int vs[SVC_MODE_SIZE];
} svcm_t;

typedef struct svc_s {
    Dtlink_t link;
    /* begin key */
    char *name;
    /* end key */

    int mark;
} svc_t;

typedef struct ev_s {
    int time, type;
    struct ruleev_s *ruleevp;
    struct cc_s *ccp;
    struct nd_s *ndp;
    struct svc_s **svcps;
    int svcpn, svcpm;

    int mark;
} ev_t;

typedef struct rulesev_s {
    float ratio;
    int sev;
} rulesev_t;

typedef struct rulesvc_s {
    char *svcstr;
    regex_t svcre;
    int tmode;
    struct rulesev_s *sevs;
    int sevn;
    char *comstr;
} rulesvc_t;

typedef struct ruleev_s {
    char *name;
    char *aidstr, *idstr, *txtstr, *tmodestr, *sevstr;
    regex_t aidre, idre, txtre, tmodere, sevre;
    char *svcstr;
    regex_t svcre;
} ruleev_t;

extern char *tmodemap[];
extern int currtime;
extern char *ftmode, *rtmode;

extern Dt_t *ccdict;
extern Dt_t *nddict;
extern int ccmark, ndmark;

extern Dt_t *svcdict;
extern int svcmark;

extern ev_t **evps;
extern int evpn, evpm;
extern int evmark;

extern ruleev_t *ruleevs;
extern int ruleevn, ruleevm;
extern rulesvc_t *rulesvcs;
extern int rulesvcn, rulesvcm;

extern int initgraph (void);
extern int termgraph (void);
extern cc_t *insertcc (char *, char *, int);
extern cc_t *findcc (char *, char *);
extern nd_t *insertnd (char *, char *, cc_t *, int);
extern nd_t *findnd (char *, char *);
extern int insertndedges (nd_t *);
extern int insertndevent (nd_t *, ev_t *);

extern int initcorr (void);
extern int termcorr (void);
extern int updatealarms (int, int, int);
extern int calcsvcvalues (int, int, int);
extern svc_t *insertsvc (char *);
extern int pruneevs (int, int);
extern ev_t *insertev (int, int, cc_t *, nd_t *, ruleev_t *);
extern int insertevservice (ev_t *, char *);
extern int loadevstate (char *);
extern int saveevstate (char *);
extern ruleev_t *matchruleev (VG_alarm_t *);
extern ruleev_t *findruleev (char *);
extern rulesvc_t *matchrulesvc (char *);
extern int loadrules (char *);

#endif /* VGGCE_H */
