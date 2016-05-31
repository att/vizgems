#pragma prototyped

#include <ast.h>
#include <swift.h>

typedef struct EEcode_s {
    int ctype;
    int fp;
    int next;
    short vari, vali;
    union {
        char s[1];
        float f;
        int i;
    } u;
} EEcode_t;

typedef struct EEvar_s {
    int id;  /* -1 for user variables, >= 0 for system variables */
    char *nam;
} EEvar_t;

/* must match C_NULL C_INTEGER C_REAL C_CSTRING C_RSTRING */
#define EE_VAL_TYPE_NULL    -1
#define EE_VAL_TYPE_INTEGER  2
#define EE_VAL_TYPE_REAL     3
#define EE_VAL_TYPE_CSTRING  4
#define EE_VAL_TYPE_RSTRING  5

#define EE_VAL_TYPE_ISNUMBER(v) ( \
    (v).type == EE_VAL_TYPE_INTEGER || (v).type == EE_VAL_TYPE_REAL \
)
#define EE_VAL_TYPE_ISSTRING(v) ( \
    (v).type == EE_VAL_TYPE_CSTRING || (v).type == EE_VAL_TYPE_RSTRING \
)

typedef struct EEval_s {
    int type;
    union {
        char *s;
        float f;
        int i;
    } u;
} EEval_t;

typedef struct EE_s {
    EEcode_t *codep;
    int coden;
    EEvar_t *vars;
    int varn;
    EEval_t *vals;
    int valn;
} EE_t;

typedef int (*EEgetvar) (EE_t *, int, EEval_t *, void *);
typedef int (*EEsetvar) (EE_t *, int, EEval_t *, void *);

extern int systemerror;

extern int EEinit (void);
extern int EEterm (void);
extern int EEparse (char *, char **, EE_t *);
extern void EEprint (EE_t *, int);
extern int EEeval (EE_t *, EEgetvar, EEsetvar, void *, int);
