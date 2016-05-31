#pragma prototyped

#ifndef _AGGR_INT_INCLUDE
#define _AGGR_INT_INCLUDE

#include <ast.h>
#include <swift.h>
#define _AGGR_PRIVATE
#include <aggr.h>

#ifdef _BLD_DEBUG
#define VMFLAGS VM_TRACE
#else
#define VMFLAGS 0
#endif

#if SWIFT_LITTLEENDIAN == 1
static unsigned char *__cp, __c;
#define SW2(sp) ( \
    __cp = (unsigned char *) (sp), \
    __c = __cp[0], __cp[0] = __cp[1], __cp[1] = __c \
)
#define SW4(ip) ( \
    __cp = (unsigned char *) (ip), \
    __c = __cp[0], __cp[0] = __cp[3], __cp[3] = __c, \
    __c = __cp[1], __cp[1] = __cp[2], __cp[2] = __c \
)
#define SW8(dp) ( \
    __cp = (unsigned char *) (dp), \
    __c = __cp[0], __cp[0] = __cp[7], __cp[7] = __c, \
    __c = __cp[1], __cp[1] = __cp[6], __cp[6] = __c, \
    __c = __cp[2], __cp[2] = __cp[5], __cp[5] = __c, \
    __c = __cp[3], __cp[3] = __cp[4], __cp[4] = __c \
)
#define SWAPKEY(k, t, l) do {\
    switch (t) {\
        int o;\
    case AGGR_TYPE_SHORT:\
        for (o = 0; o < l; o += 2)\
            SW2 (k + o);\
        break;\
    case AGGR_TYPE_INT:\
        for (o = 0; o < l; o += 4)\
            SW4 (k + o);\
        break;\
    case AGGR_TYPE_LLONG:\
        for (o = 0; o < l; o += 8)\
            SW8 (k + o);\
        break;\
    case AGGR_TYPE_FLOAT:\
        for (o = 0; o < l; o += 4)\
            SW4 (k + o);\
        break;\
    case AGGR_TYPE_DOUBLE:\
        for (o = 0; o < l; o += 8)\
            SW8 (k + o);\
        break;\
    }\
} while (0)
#else
#define SW2(ip) (ip)
#define SW4(sp) (sp)
#define SW8(dp) (dp)
#define SWAPKEY(k, t, l)
#endif

#define AGGR_CMDcreate     1
#define AGGR_CMDdestroy    2
#define AGGR_CMDopen       3
#define AGGR_CMDclose      4
#define AGGR_CMDsetlock    5
#define AGGR_CMDunsetlock  6
#define AGGR_CMDensurelen  7
#define AGGR_CMDtruncate   8
#define AGGR_CMDsetmap     9
#define AGGR_CMDunsetmap  10

#define TYPE_CHAR   AGGR_TYPE_CHAR
#define TYPE_SHORT  AGGR_TYPE_SHORT
#define TYPE_INT    AGGR_TYPE_INT
#define TYPE_LLONG  AGGR_TYPE_LLONG
#define TYPE_FLOAT  AGGR_TYPE_FLOAT
#define TYPE_DOUBLE AGGR_TYPE_DOUBLE
#define TYPE_STRING AGGR_TYPE_STRING
#define TYPE_MAXID  AGGR_TYPE_MAXID

#define COMB_OPER_ADD   AGGR_COMBINE_OPER_ADD
#define COMB_OPER_SUB   AGGR_COMBINE_OPER_SUB
#define COMB_OPER_MUL   AGGR_COMBINE_OPER_MUL
#define COMB_OPER_DIV   AGGR_COMBINE_OPER_DIV
#define COMB_OPER_MIN   AGGR_COMBINE_OPER_MIN
#define COMB_OPER_MAX   AGGR_COMBINE_OPER_MAX
#define COMB_OPER_WAVG  AGGR_COMBINE_OPER_WAVG
#define COMB_OPER_MAXID AGGR_COMBINE_OPER_MAXID

#define SUM_DIR_FRAMES AGGR_SUM_DIR_FRAMES
#define SUM_DIR_ITEMS  AGGR_SUM_DIR_ITEMS
#define SUM_DIR_MAXID  AGGR_SUM_DIR_MAXID

#define SUM_OPER_ADD   AGGR_SUM_OPER_ADD
#define SUM_OPER_MIN   AGGR_SUM_OPER_MIN
#define SUM_OPER_MAX   AGGR_SUM_OPER_MAX
#define SUM_OPER_MAXID AGGR_SUM_OPER_MAXID

#define TOPN_OPER_MIN   AGGR_TOPN_OPER_MIN
#define TOPN_OPER_MAX   AGGR_TOPN_OPER_MAX
#define TOPN_OPER_MAXID AGGR_TOPN_OPER_MAXID

#define APPEND_OPER_ADD   AGGR_APPEND_OPER_ADD
#define APPEND_OPER_SUB   AGGR_APPEND_OPER_SUB
#define APPEND_OPER_MUL   AGGR_APPEND_OPER_MUL
#define APPEND_OPER_DIV   AGGR_APPEND_OPER_DIV
#define APPEND_OPER_MIN   AGGR_APPEND_OPER_MIN
#define APPEND_OPER_MAX   AGGR_APPEND_OPER_MAX
#define APPEND_OPER_SET   AGGR_APPEND_OPER_SET
#define APPEND_OPER_MAXID AGGR_APPEND_OPER_MAXID

#define CVAL AGGR_CVAL
#define SVAL AGGR_SVAL
#define IVAL AGGR_IVAL
#define LVAL AGGR_LVAL
#define FVAL AGGR_FVAL
#define DVAL AGGR_DVAL

typedef struct merge_t {
    int *maps;
    unsigned char *keyp;
    void *cachep, *datap;
} merge_t;

typedef int (*combfunc_t) (
    AGGRfile_t **, int, merge_t *, float *, AGGRfile_t *
);
typedef int (*catfunc_t) (AGGRfile_t **, int, merge_t *, AGGRfile_t *);
typedef int (*sumfunc_t) (AGGRfile_t *, int, AGGRfile_t *);
typedef int (*reaggrfunc_t) (AGGRfile_t *, merge_t *, AGGRfile_t *);
typedef int (*extractfunc_t) (AGGRfile_t *, merge_t *, AGGRfile_t *);
typedef int (*meanfunc_t) (
    AGGRfile_t **, int, merge_t *, AGGRfile_t *, AGGRfile_t *
);
typedef int (*topnfunc_t) (AGGRfile_t *, int, int, AGGRkv_t *, int);

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

#undef __PUBLIC_DATA__

#if _BLD_aggr && defined(__EXPORT__)
#define extern  __EXPORT__
#endif

extern int _aggrgrow (AGGRfile_t *, int, int);

extern int _aggrdictlookupvar (AGGRfile_t *, unsigned char *, int, int);
extern int _aggrdictlookupfixed (AGGRfile_t *, unsigned char *, int, int);
extern int _aggrdictmerge (AGGRfile_t **, int, merge_t *, AGGRfile_t *);
extern int _aggrdictcopy (AGGRfile_t *, AGGRfile_t *);
extern void _aggrdictreverse (AGGRfile_t *, unsigned char **);
extern int _aggrdictscankey (AGGRfile_t *, char *, unsigned char **);
extern void _aggrdictprintkey (AGGRfile_t *, Sfio_t *, unsigned char *);

extern int _aggrscanval (AGGRfile_t *, char *, unsigned char **);
extern void _aggrprintval (AGGRfile_t *, Sfio_t *, unsigned char *);

extern void _aggrcombineinit (void);
extern int _aggrcombinerun (
    AGGRfile_t **, int, merge_t *, AGGRfile_t *, int, int, int, float *
);
extern void _aggrcatinit (void);
extern int _aggrcatrun (AGGRfile_t **, int, merge_t *, AGGRfile_t *, int, int);
extern void _aggrsuminit (void);
extern int _aggrsumrun (AGGRfile_t *, int, AGGRfile_t *, int, int, int);
extern void _aggrreaggrinit (void);
extern int _aggrreaggrrun (AGGRfile_t *, merge_t *, AGGRfile_t *, int, int);
extern void _aggrextractinit (void);
extern int _aggrextractrun (AGGRfile_t *, merge_t *, AGGRfile_t *, int, int);
extern void _aggrmeaninit (void);
extern int _aggrmeanrun (
    AGGRfile_t **, int, merge_t *, AGGRfile_t *, AGGRfile_t *, int, int
);
extern void _aggrtopninit (void);
extern int _aggrtopnrun (AGGRfile_t *, int, int, AGGRkv_t *, int, int, int);

#undef extern
_END_EXTERNS_

#endif /* _AGGR_INT_INCLUDE */
