#pragma prototyped

#ifndef _DDS_INCLUDE
#define _DDS_INCLUDE

#include <ast.h>
#include <vmalloc.h>
#include <cdt.h>
#include <recsort.h>
#include <aggr.h>

typedef enum {
    DDS_FIELD_TYPE_CHAR = 1,
    DDS_FIELD_TYPE_SHORT = 2,
    DDS_FIELD_TYPE_INT = 3,
    DDS_FIELD_TYPE_LONG = 4,
    DDS_FIELD_TYPE_LONGLONG = 5,
    DDS_FIELD_TYPE_FLOAT = 6,
    DDS_FIELD_TYPE_DOUBLE = 7
} DDSfieldtype_t;

typedef struct DDSfield_t {
    char *name;
    DDSfieldtype_t type;
    int isunsigned;
    int n, off;
} DDSfield_t;

typedef struct DDSschema_t {
    char *name;
    DDSfield_t *fields;
    int fieldn;
    char *preload;
    char *include;
    char *recbuf;
    int recsize;
    Vmalloc_t *vm;
} DDSschema_t;

typedef struct DDSheader_t {
    unsigned int contents;
    char *schemaname;
    DDSschema_t *schemap;
    char *vczspec;
    void *vczp;
} DDSheader_t;

#define DDS_HDR_NAME 1
#define DDS_HDR_DESC 2

typedef struct DDSexpr_t {
    char *vars;
    char *libs;
    char *begin;
    char *end;
    char *loop;
} DDSexpr_t;

typedef int (*DDSloaderinitfunc) (void);
typedef int (*DDSloadertermfunc) (void);
typedef int (*DDSloaderworkfunc) (char **, int, char *, int, int);

typedef struct DDSloader_t {
    void *handle;
    DDSloaderinitfunc init;
    DDSloadertermfunc term;
    DDSloaderworkfunc work;
} DDSloader_t;

typedef int (*DDSsorterinitfunc) (void);
typedef int (*DDSsortertermfunc) (void);
typedef Rsdefkey_f DDSsorterworkfunc;

typedef struct DDSsorter_t {
    void *handle;
    Rs_t *rsp;
    Rsdisc_t rsdisc;
    DDSsorterinitfunc init;
    DDSsortertermfunc term;
    DDSsorterworkfunc work;
} DDSsorter_t;

typedef int (*DDSfilterinitfunc) (void);
typedef int (*DDSfiltertermfunc) (void);
typedef int (*DDSfilterworkfunc) (char *, int, long long *, long long);

typedef struct DDSfilter_t {
    void *handle;
    DDSfilterinitfunc init;
    DDSfiltertermfunc term;
    DDSfilterworkfunc work;
} DDSfilter_t;

typedef int (*DDSsplitterinitfunc) (
    char *, void *(*) (char *), void *(*) (char *), int (*) (void *)
);
typedef int (*DDSsplittertermfunc) (void);
typedef void *(*DDSsplitterworkfunc) (char *, int);

typedef struct DDSsplitter_t {
    void *handle;
    DDSsplitterinitfunc init;
    DDSsplittertermfunc term;
    DDSsplitterworkfunc work;
} DDSsplitter_t;

typedef int (*DDSgrouperinitfunc) (
    char *, void *(*) (char *), void *(*) (char *), int (*) (void *)
);
typedef int (*DDSgroupertermfunc) (void);
typedef void *(*DDSgrouperworkfunc) (char *, int);

typedef struct DDSgrouper_t {
    void *handle;
    DDSgrouperinitfunc init;
    DDSgroupertermfunc term;
    DDSgrouperworkfunc work;
} DDSgrouper_t;

typedef int (*DDSextractorinitfunc) (void);
typedef int (*DDSextractortermfunc) (void);
typedef int (*DDSextractorworkfunc) (char *, int, char *, int);

typedef struct DDSextractor_t {
    void *handle;
    DDSextractorinitfunc init;
    DDSextractortermfunc term;
    DDSextractorworkfunc work;
    DDSschema_t *schemap;
} DDSextractor_t;

typedef int (*DDSconverterinitfunc) (void);
typedef int (*DDSconvertertermfunc) (void);
typedef int (*DDSconverterworkfunc) (
    char *, int, DDSschema_t *, char *, int, DDSschema_t *,
    long long *, long long,
    int (*) (Sfio_t *, void *, DDSschema_t *, size_t), Sfio_t *
);

typedef struct DDSconverter_t {
    void *handle;
    DDSconverterinitfunc init;
    DDSconvertertermfunc term;
    DDSconverterworkfunc work;
} DDSconverter_t;

typedef int (*DDScounterinitfunc) (Dt_t **, int *);
typedef int (*DDScountertermfunc) (void);
typedef int (*DDScounterworkfunc) (char *, int, int);

typedef struct DDScounter_t {
    void *handle;
    DDScounterinitfunc init;
    DDScountertermfunc term;
    DDScounterworkfunc work;
    DDSschema_t *schemap;
} DDScounter_t;

typedef int (*DDSaggregatorinitfunc) (AGGRfile_t *);
typedef int (*DDSaggregatortermfunc) (AGGRfile_t *);
typedef int (*DDSaggregatorworkfunc) (char *, int, AGGRfile_t *);

typedef struct DDSaggregator_t {
    void *handle;
    DDSaggregatorinitfunc init;
    DDSaggregatortermfunc term;
    DDSaggregatorworkfunc work;
} DDSaggregator_t;

typedef int (*DDSprinterinitfunc) (void);
typedef int (*DDSprintertermfunc) (void);
typedef int (*DDSprinterworkfunc) (char *, int, Sfio_t *);

typedef struct DDSprinter_t {
    void *handle;
    DDSprinterinitfunc init;
    DDSprintertermfunc term;
    DDSprinterworkfunc work;
} DDSprinter_t;

#if _BLD_dds && defined(__EXPORT__)
#define extern  __EXPORT__
#endif

extern int DDSinit (void);
extern int DDSterm (void);

extern DDSschema_t *DDScreateschema (char *, DDSfield_t *, int, char *, char *);
extern int DDSdestroyschema (DDSschema_t *);
extern DDSschema_t *DDSloadschema (char *, Sfio_t *, int *);
extern int DDSsaveschema (char *, Sfio_t *, DDSschema_t *, int *);
extern DDSfield_t *DDSfindfield (DDSschema_t *, char *);

extern int DDSreadheader (Sfio_t *, DDSheader_t *);
extern int DDSwriteheader (Sfio_t *, DDSheader_t *);

extern int DDSparseexpr (char *, char *, DDSexpr_t *);

extern int DDSreadrecord (Sfio_t *, void *, DDSschema_t *);
extern int DDSwriterecord (Sfio_t *, void *, DDSschema_t *);
extern int DDSswaprecord (void *, DDSschema_t *);
extern int DDSreadrawrecord (Sfio_t *, void *, DDSschema_t *);
extern int DDSwriterawrecord (Sfio_t *, void *, DDSschema_t *);

extern DDSloader_t *DDScreateloader (DDSschema_t *, char *, char *);
extern char *DDScompileloader (DDSschema_t *, char *, char *, char *);
extern DDSloader_t *DDSloadloader (DDSschema_t *, char *);
extern int DDSdestroyloader (DDSloader_t *);

extern DDSsorter_t *DDScreatesorter (
    DDSschema_t *, char *, int, char *, char *, int
);
extern char *DDScompilesorter (
    DDSschema_t *, char *, char *, char *, char *, int
);
extern DDSsorter_t *DDSloadsorter (DDSschema_t *, char *, int);
extern int DDSdestroysorter (DDSsorter_t *);

extern DDSfilter_t *DDScreatefilter (
    DDSschema_t *, DDSexpr_t *, char *, char *
);
extern char *DDScompilefilter (
    DDSschema_t *, DDSexpr_t *, char *, char *, char *
);
extern DDSfilter_t *DDSloadfilter (DDSschema_t *, char *);
extern int DDSdestroyfilter (DDSfilter_t *);

extern DDSsplitter_t *DDScreatesplitter (DDSschema_t *, char *, char *, char *);
extern char *DDScompilesplitter (DDSschema_t *, char *, char *, char *, char *);
extern DDSsplitter_t *DDSloadsplitter (DDSschema_t *, char *);
extern int DDSdestroysplitter (DDSsplitter_t *);

extern DDSgrouper_t *DDScreategrouper (
    DDSschema_t *, DDSexpr_t *, char *, char *
);
extern char *DDScompilegrouper (
    DDSschema_t *, DDSexpr_t *, char *, char *, char *
);
extern DDSgrouper_t *DDSloadgrouper (DDSschema_t *, char *);
extern int DDSdestroygrouper (DDSgrouper_t *);

extern DDSextractor_t *DDScreateextractor (
    DDSschema_t *, char *, char *, char *, char *
);
extern char *DDScompileextractor (
    DDSschema_t *, char *, char *, char *, char *, char *
);
extern DDSextractor_t *DDSloadextractor (DDSschema_t *, char *);
extern int DDSdestroyextractor (DDSextractor_t *);

extern DDSconverter_t *DDScreateconverter (
    DDSschema_t *, DDSschema_t *, DDSexpr_t *, char *, char *
);
extern char *DDScompileconverter (
    DDSschema_t *, DDSschema_t *, DDSexpr_t *, char *, char *, char *
);
extern DDSconverter_t *DDSloadconverter (DDSschema_t *, DDSschema_t *, char *);
extern int DDSdestroyconverter (DDSconverter_t *);

extern DDScounter_t *DDScreatecounter (
    DDSschema_t *, char *, char *, char *, char *
);
extern char *DDScompilecounter (
    DDSschema_t *, char *, char *, char *, char *, char *
);
extern DDScounter_t *DDSloadcounter (DDSschema_t *, char *);
extern int DDSdestroycounter (DDScounter_t *);

extern DDSaggregator_t *DDScreateaggregator (
    DDSschema_t *, DDSexpr_t *, int, char *, char *
);
extern char *DDScompileaggregator (
    DDSschema_t *, DDSexpr_t *, int, char *, char *, char *
);
extern DDSaggregator_t *DDSloadaggregator (DDSschema_t *, char *);
extern int DDSdestroyaggregator (DDSaggregator_t *);

extern DDSprinter_t *DDScreateprinter (
    DDSschema_t *, DDSexpr_t *, char *, char *
);
extern char *DDScompileprinter (
    DDSschema_t *, DDSexpr_t *, char *, char *, char *
);
extern DDSprinter_t *DDSloadprinter (DDSschema_t *, char *);
extern int DDSdestroyprinter (DDSprinter_t *);

extern Sfoff_t DDSvczsize (DDSheader_t *);
extern ssize_t DDSvczread (DDSheader_t *, char *, void *, Sfoff_t, size_t);

#undef extern
_END_EXTERNS_

#ifdef __EXPORT__
#define DDS_EXPORT  __EXPORT__
#else
#define DDS_EXPORT
#endif

#endif /* _DDS_INCLUDE */
