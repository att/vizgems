#pragma prototyped

#include <ast.h>
#include <swift.h>

#define PM_VERSION "   2"

typedef unsigned int PMoff_t;
typedef unsigned int PMsize_t;

#define PM_ALIGN_UNIT sizeof (double)
#define PM_ALIGN_SIZE(s) ( \
    (((s) + PM_ALIGN_UNIT - 1) / PM_ALIGN_UNIT) * PM_ALIGN_UNIT \
)
#define PM_ITEM_FULLSIZE(s) ((s) + sizeof (PMhdr_t))
#define PM_ITEM_SETMAGIC(p, o) (\
    ((PMhdr_t *) ((p) + (o)))->magic1 = PM_MAGIC1, \
    ((PMhdr_t *) ((p) + (o)))->magic2 = PM_MAGIC2 \
)
#define PM_ITEM_ISMAGIC(p, o) (\
    ((PMhdr_t *) ((p) + (o)))->magic1 == PM_MAGIC1 && \
    ((PMhdr_t *) ((p) + (o)))->u.size <= ((PMhdr_t *) ((p) + (o)))->size && \
    ((PMhdr_t *) ((p) + (o)))->magic2 == PM_MAGIC2 \
)
#define PM_ITEM_SIZE(p, o) (((PMhdr_t *) ((p) + (o)))->size)
#define PM_ITEM_NEXT(p, o) (((PMhdr_t *) ((p) + (o)))->u.next)
#define PM_ITEM_USIZE(p, o) (((PMhdr_t *) ((p) + (o)))->u.size)

#define PM_MAGIC1 123546789
#define PM_MAGIC2 987645321

typedef struct PMhdr_s {
    unsigned int magic1;
    union {
        PMoff_t next;
        PMsize_t size;
    } u;
    PMsize_t size;
    unsigned int magic2;
} PMhdr_t;

#define PM_SLOT_N 1025
#define PM_SLOT_FROMSIZE(s) ( \
    (((s) / PM_ALIGN_UNIT) < PM_SLOT_N) ? \
    ((s) / PM_ALIGN_UNIT) : PM_SLOT_N - 1 \
)
#define PM_SLOT_ISFIXED(s) ((s < PM_SLOT_N - 1) ? TRUE : FALSE)

typedef struct PMsegment_s {
    unsigned char *mem;
    PMoff_t end;
    PMsize_t size;
    PMoff_t slots[PM_SLOT_N];
} PMsegment_t;

typedef struct PMarena_s {
    PMsegment_t *segments;
    int segmentn;
} PMarena_t;

typedef void (*PMptrfunc) (void);

extern int PMinit (char *, PMsize_t, PMptrfunc, int, int);
extern int PMterm (char *);

extern void *PMalloc (PMsize_t);
extern void *PMresize (void *, PMsize_t);
extern void *PMstrdup (char *);
extern void *PMfree (void *);

extern void PMsetanchor (void *);
extern void *PMgetanchor (void);

extern int PMcompact (void);

extern int PMptrwrite (void *);
