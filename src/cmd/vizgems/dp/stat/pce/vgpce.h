#pragma prototyped

#ifndef VGPCE_H
#define VGPCE_H

#include <ast.h>
#include <cdt.h>
#include <regex.h>
#include <swift.h>
#include "vg_hdr.h"

#define SZ_level VG_stat_level_L
#define SZ_id VG_stat_id_L
#define SZ_key VG_stat_key_L
#define SZ_label VG_stat_label_L

#define epsilon 1E-6
#define ALMOSTZERO(v) ((v) >= -epsilon && (v) <= epsilon)

extern int currtime, curri;
extern int secperframe;
extern char *ftmode, *rtmode;
#define TIME2I(t) ((t) / secperframe)

#define RRTYPE_CLEAR  1
#define RRTYPE_ACTUAL 2
#define RRTYPE_SIGMA  3

#define RROP_NONE 1
#define RROP_INCL 2
#define RROP_EXCL 3

typedef struct rulerange_s {
    int type, minop, maxop;
    float minval, maxval;
    int sev;
    int m, n;
    int freq, ival;
    struct alarmdata_s *alarmdatap;
} rulerange_t;

#define ROP_NONE 1
#define ROP_INCL 2
#define ROP_EXCL 3

typedef struct rule_s {
    int minop, maxop;
    float minval, maxval;
    rulerange_t *ranges;
    int rangen;
    rulerange_t *clearrangep;
    int maxn;
} rule_t;

typedef struct proffile_s {
    char *meanfile, *sdevfile;
} proffile_t;

extern proffile_t *proffiles;
extern int proffilen;

typedef struct profdata_s {
    int haveprof, havemean, havesdev;
    float mean, sdev;
} profdata_t;

typedef struct alarmdata_s {
    int i;
    int sev;
#if 0
    float curr, mean, sdev;
#endif
    float val;
    struct alarmdata_s *nextp;
} alarmdata_t;

typedef struct alarmdatalist_s {
    alarmdata_t *alarmdatas;
    int alarmdatam, alarmdatan;
} alarmdatalist_t;

#define ADL_SIZE 10000
extern alarmdatalist_t *alarmdatalists, *alarmdatalistp;
extern int alarmdatalistn;

#define ASAM_CLEAR 0
#define ASAM_ALARM 1

typedef struct alarmstate_s {
    int mode, time, i, sev, rtime;
} alarmstate_t;

typedef struct node_s {
    Dtlink_t link;
    /* begin key */
    char level[SZ_level], id[SZ_id], key[SZ_key];
    /* end key */

    char label[SZ_label];
    rule_t *rulep;
    int rulemaxn;
    profdata_t *profdatas;
    int profdatan;
    alarmdata_t *firstalarmdatap, *lastalarmdatap;
    int alarmdatan;
    int mark;
    alarmstate_t alarmstate;
} node_t;

extern int nodemark;

extern int ruleinit (void);
extern int ruleterm (void);
extern rule_t *ruleinsert (char *);
extern int ruleremove (rule_t *);
extern rulerange_t *rulematch (rule_t *, float, float, float, float *);

extern int nodeinit (char *);
extern int nodeterm (void);
extern node_t *nodeinsert (char *, char *, char *, char *);
extern node_t *nodefind (char *, char *, char *);
extern int noderemove (node_t *);
extern int nodeinsertrule (node_t *, rule_t *);
extern profdata_t *nodeloadprofdata (node_t *, int);
extern int nodeinsertalarmdata (node_t *, int, int, float);
extern int nodeupdatealarms (node_t *);
extern int allnodesupdatealarms (void);
extern int nodeprune (node_t *);
extern int allnodesprune (void);
extern int nodeloadstate (char *);
extern int nodesavestate (char *);

extern int alarminit (void);
extern int alarmterm (void);
extern alarmdata_t *alarmnew (void);
extern alarmdata_t *alarmmatchrange (alarmdata_t *, rulerange_t *);
extern int alarmemit (alarmdata_t *, rulerange_t *, node_t *, int);

#endif /* VGPCE_H */
