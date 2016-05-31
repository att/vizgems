#pragma prototyped

#ifndef _AGGR_INCLUDE
#define _AGGR_INCLUDE

#include <ast.h>
#include <vmalloc.h>
#include <cdt.h>

typedef long long AGGRoff_t;
typedef unsigned long long AGGRsize_t;

#define AGGR_TYPE_CHAR   0
#define AGGR_TYPE_SHORT  1
#define AGGR_TYPE_INT    2
#define AGGR_TYPE_LLONG  3
#define AGGR_TYPE_FLOAT  4
#define AGGR_TYPE_DOUBLE 5
#define AGGR_TYPE_STRING 6 /* ONLY FOR KEYS */
#define AGGR_TYPE_MAXID  7

#define AGGR_MAGIC "AGGRFILE"
#define AGGR_MAGIC_LEN 8
#define AGGR_VERSION 5
#define AGGR_CLASS_LEN 948

typedef struct AGGRhdr_t {
    char magic[AGGR_MAGIC_LEN];
    int version;
    unsigned int dictsn, datasn;
    int itemn, framen;
    short keytype, valtype;
    long long dictlen, datalen;
    int dictincr, itemincr;
    double minval, maxval;
    int vallen;
    char class[AGGR_CLASS_LEN];
} AGGRhdr_t;

typedef struct AGGRtree_t {
    unsigned char *keyp;
    int itemi;
    struct AGGRtree_t *nodeps[2];
} AGGRtree_t;

#ifdef _AGGR_PRIVATE
typedef struct AGGRstack_t {
    int fi, li;
    AGGRtree_t *nodep;
    int state;
} AGGRstack_t;
#endif

typedef struct AGGRfname_t {
    int framei, level;
    char *name;
} AGGRfname_t;

typedef struct AGGRdict_t {
    Vmalloc_t *vm;
    int keylen, keyn, itemn; /* itemn <= hdr.itemn */
    long long byten;
    AGGRtree_t *rootp;
    AGGRfname_t *fnames;
    int fnamen;
#ifdef _AGGR_PRIVATE
    AGGRstack_t *stacks;
    int stackn, stackl;
#endif
} AGGRdict_t;

#ifdef _AGGR_PRIVATE
typedef struct AGGRmap_t {
    unsigned char *rp, *p;
    AGGRoff_t roff, off;
    AGGRsize_t rlen, len;
    unsigned char *datap;
    AGGRsize_t datan;
} AGGRmap_t;
#endif

typedef struct AGGRunit_t {
    union {
        char c;
        short s;
        int i;
        long long l;
        float f;
        double d;
        char *strp;
    } u;
} AGGRunit_t;

typedef struct AGGRkv_t {
    int framei;
    unsigned char *keyp;
    unsigned char *valp;
    int itemi;
    int oper;
} AGGRkv_t;

#ifdef _AGGR_PRIVATE
typedef unsigned char AGGRqueue_t;
#endif

typedef struct AGGRreaggr_t {
    unsigned char *okeyp, *nkeyp;
    int nitemi;
} AGGRreaggr_t;

typedef struct AGGRextract_t {
    unsigned char *keyp;
    int nitemi;
} AGGRextract_t;

#ifdef _AGGR_PRIVATE
typedef struct AGGRserver_t {
    Dtlink_t link;
    /* begin key */
    char *namep;
    /* end key */
    int socket;
    int refcount;
    char *msgs;
    int msgi, msgl, msgn;
} AGGRserver_t;

typedef struct AGGRfile_t *dummy;

typedef struct AGGRfuncs_t {
    int (*open) (struct AGGRfile_t *, int);
    int (*close) (struct AGGRfile_t *, int);
    int (*setlock) (struct AGGRfile_t *);
    int (*unsetlock) (struct AGGRfile_t *);
    int (*ensurelen) (struct AGGRfile_t *, AGGRoff_t, int);
    int (*truncate) (struct AGGRfile_t *, AGGRoff_t);
    int (*setmap) (struct AGGRfile_t *, struct AGGRmap_t *);
    int (*unsetmap) (struct AGGRfile_t *, struct AGGRmap_t *);
} AGGRfuncs_t;
#endif

#define AGGR_RWMODE_RD 1
#define AGGR_RWMODE_RW 2

typedef struct AGGRfile_t {
    Dtlink_t link;
    /* begin key */
    char *namep;
    /* end key */
    int fd, rwmode;
    int alwayslocked, lockednow;
    AGGRhdr_t hdr;
    AGGRdict_t ad;
#ifdef _AGGR_PRIVATE
    AGGRmap_t map;
    int mapactive;
    AGGRqueue_t *queues, *qops;
    int queuel, queuen;
    AGGRqueue_t ***qframes;
    int qframen, qitemn, qiteml, qmaxiteml;
    int *qmaps;
    int qmapn;
    int dicthaschanged;
    AGGRserver_t *serverp;
    unsigned char *cbufs;
    AGGRoff_t cbufo;
    AGGRsize_t cbufn, cbufl;
    AGGRfuncs_t funcs;
#endif
} AGGRfile_t;

#define AGGR_COMBINE_OPER_ADD   0
#define AGGR_COMBINE_OPER_SUB   1
#define AGGR_COMBINE_OPER_MUL   2
#define AGGR_COMBINE_OPER_DIV   3
#define AGGR_COMBINE_OPER_MIN   4
#define AGGR_COMBINE_OPER_MAX   5
#define AGGR_COMBINE_OPER_WAVG  6
#define AGGR_COMBINE_OPER_MAXID 7

#define AGGR_SUM_DIR_FRAMES 0
#define AGGR_SUM_DIR_ITEMS  1
#define AGGR_SUM_DIR_MAXID  2

#define AGGR_SUM_OPER_ADD   0
#define AGGR_SUM_OPER_MIN   1
#define AGGR_SUM_OPER_MAX   2
#define AGGR_SUM_OPER_MAXID 3

#define AGGR_TOPN_OPER_MIN   0
#define AGGR_TOPN_OPER_MAX   1
#define AGGR_TOPN_OPER_MAXID 2

#define AGGR_APPEND_OPER_ADD   0
#define AGGR_APPEND_OPER_SUB   1
#define AGGR_APPEND_OPER_MUL   2
#define AGGR_APPEND_OPER_DIV   3
#define AGGR_APPEND_OPER_MIN   4
#define AGGR_APPEND_OPER_MAX   5
#define AGGR_APPEND_OPER_SET   6
#define AGGR_APPEND_OPER_MAXID 7

#define AGGR_CVAL(p) (* (char *) (p))
#define AGGR_SVAL(p) (* (short *) (p))
#define AGGR_IVAL(p) (* (int *) (p))
#define AGGR_LVAL(p) (* (long long *) (p))
#define AGGR_FVAL(p) (* (float *) (p))
#define AGGR_DVAL(p) (* (double *) (p))

_BEGIN_EXTERNS_ /* public data */

#if _BLD_aggr && defined(__EXPORT__)
#define __PUBLIC_DATA__ __EXPORT__
#else
#if !_BLD_aggr && defined(__IMPORT__)
#define __PUBLIC_DATA__ __IMPORT__
#else
#define __PUBLIC_DATA__
#endif
#endif

extern __PUBLIC_DATA__ int *AGGRtypelens;

#undef __PUBLIC_DATA__

#if _BLD_aggr && defined(__EXPORT__)
#define extern  __EXPORT__
#endif

extern int AGGRinit (void);
extern int AGGRterm (void);
extern AGGRfile_t *AGGRcreate (
    char *, int, int, char, char, char *, int, int, int, int, int, int
);
extern int AGGRdestroy (AGGRfile_t *);
extern AGGRfile_t *AGGRopen (char *, int, int, int);
extern int AGGRclose (AGGRfile_t *);
extern int AGGRlockmode (AGGRfile_t *, int);
extern int AGGRcombine (AGGRfile_t **, int, int, float *, AGGRfile_t *);
extern int AGGRcat (AGGRfile_t **, int, AGGRfile_t *);
extern int AGGRsum (AGGRfile_t *, int, int, AGGRfile_t *);
extern int AGGRreaggr (AGGRfile_t *, AGGRreaggr_t *, int, AGGRfile_t *);
extern int AGGRextract (AGGRfile_t *, AGGRextract_t *, int, AGGRfile_t *);
extern int AGGRmean (AGGRfile_t **, int, AGGRfile_t *, AGGRfile_t *);
extern int AGGRtopn (AGGRfile_t *, int, int, AGGRkv_t *, int, int);
extern int AGGRprint (AGGRfile_t *, int, int, int, int, int, int);
extern int AGGRgrow (AGGRfile_t *, int, int);
extern int AGGRappend (AGGRfile_t *, AGGRkv_t *, int);
extern int AGGRflush (AGGRfile_t *);
extern int AGGRlookup (AGGRfile_t *, unsigned char *);
extern int AGGRfnames (AGGRfile_t *, AGGRfname_t *, int);
extern int AGGRminmax (AGGRfile_t *, int);
extern void *AGGRget (AGGRfile_t *, int);
extern int AGGRrelease (AGGRfile_t *);
extern int AGGRserverstart (int *);
extern int AGGRserverstop (int);
extern int AGGRparsetype (char *, int *, int *);
extern int AGGRparsefnames (char *, AGGRfname_t **, int *);
#ifdef _AGGR_PRIVATE
extern AGGRserver_t *AGGRserverget (char *);
extern int AGGRserverrelease (AGGRserver_t *);
extern int AGGRserve (AGGRserver_t *);
#endif

#undef extern
_END_EXTERNS_

#endif /* _AGGR_INCLUDE */
