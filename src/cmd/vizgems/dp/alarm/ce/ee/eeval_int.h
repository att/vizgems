#pragma prototyped

#include <ast.h>
#include <swift.h>

/* from lex.c */

#define L_SEMI      0
#define L_ASSIGN    1
#define L_OR        2
#define L_AND       3
#define L_EQ        4
#define L_NE        5
#define L_LT        6
#define L_LE        7
#define L_GT        8
#define L_GE        9
#define L_PLUS     10
#define L_MINUS    11
#define L_MUL      12
#define L_DIV      13
#define L_MOD      14
#define L_NOT      15
#define L_STRING   16
#define L_NUMBER   17
#define L_ID       18
#define L_LP       19
#define L_RP       20
#define L_LCB      21
#define L_RCB      22
#define L_COLON    23
#define L_COMMA    24
#define L_IF       25
#define L_ELSE     26
#define L_EOF      27
#define L_SIZE     28

typedef char *EElname_t;

extern int EEltok;
extern char EElstrtok[];
extern EElname_t EElnames[];

extern void EElsetstr (char *);
extern char *EElgetstr (void);
extern void EElgtok (void);

/* from code.c */

#define C_NULL     -1
#define C_CODE      0
#define C_ASSIGN    1
#define C_INTEGER   2
#define C_REAL      3
#define C_CSTRING   4
#define C_RSTRING   5
#define C_OR        6
#define C_AND       7
#define C_EQ        8
#define C_NE        9
#define C_LT       10
#define C_LE       11
#define C_GT       12
#define C_GE       13
#define C_PLUS     14
#define C_MINUS    15
#define C_MUL      16
#define C_DIV      17
#define C_MOD      18
#define C_UMINUS   19
#define C_NOT      20
#define C_PEXPR    21
#define C_VAR      22
#define C_STMT     23
#define C_IF       24
#define C_SIZE     25

extern int EEcinit (void);
extern int EEcterm (void);

extern int EEcbegin (char **);
extern int EEcend (EE_t *);

extern int EEcnew (int);
extern int EEcinteger (int);
extern int EEcreal (float);
extern int EEcstring (char *, int);

extern void EEcsetfp (int, int);
extern void EEcsetnext (int, int);
extern void EEcsetvari (int, int);
extern void EEcsetvali (int, int);

extern int EEcaddvar (int);
extern int EEcaddval (int);

#define EEcgettype(cp, ci)    cp[ci].ctype
#define EEcgetfp(cp, ci)      cp[ci].fp
#define EEcgetnext(cp, ci)    cp[ci].next
#define EEcgetvari(cp, ci)    cp[ci].vari
#define EEcgetvali(cp, ci)    cp[ci].vali
#define EEcgetinteger(cp, ci) cp[ci].u.i
#define EEcgetreal(cp, ci)    cp[ci].u.f
#define EEcgetstring(cp, ci)  ((char *) &cp[ci].u.s)
#define EEcgetaddr(cp, ci)    cp[ci]

/* from parse.c */

extern int EEpinit (void);
extern int EEpterm (void);
extern int EEpunit (char *, char **, EE_t *);

/* from print.c */

extern void EEpprint (EE_t *, int);

/* from parse.c */

extern int EEeinit (void);
extern int EEeterm (void);
extern int EEeunit (EE_t *, EEgetvar, EEsetvar, void *, int);
