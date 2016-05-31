#pragma prototyped

#include <ast.h>
#include <swift.h>
#include "eeval.h"
#include "eeval_int.h"

#define GTOKIFEQ(t) { \
    if (EEltok == (t)) \
        EElgtok (); \
    else \
        SUwarning ( \
            0, "parser", "expected: '%s', found: '%s', string: %s", \
            EElnames[t], EElnames[EEltok], EElgetstr () \
        ); \
}

static int pstmt (void);
static int pifst (void);
static int pexpr (void);
static int getop (int, int);
static int pexpi (int);
static int pexp5 (void);
static int pexp6 (void);
static int pexp7 (void);
static int pcons (void);
static int pvar (void);

int EEpinit (void) {
    return 0;
}

int EEpterm (void) {
    return 0;
}

int EEpunit (char *codestr, char **dict, EE_t *eep) {
    char *reststr;

    EElsetstr (codestr);
    EEcbegin (dict);

    while (EEltok == L_SEMI)
        EElgtok ();
    if (EEltok == L_EOF)
        return -1;

    if (pstmt () == -1) {
        SUwarning (0, "EEpunit", "parsing failed for string %s", codestr);
        return -1;
    }
    if ((reststr = EElgetstr ()) && *reststr != 0) {
        SUwarning (
            0, "EEpunit", "extra characters at end of string: %s", reststr
        );
        return -1;
    }

    return EEcend (eep);
}

static int pstmt (void) {
    int si, si0, si1;

    if ((si = EEcnew (C_STMT)) == -1) {
        SUwarning (0, "pstmt", "cannot create statement");
        return -1;
    }
    switch (EEltok) {
    case L_SEMI:
        EEcsetfp (si, C_NULL);
        EElgtok ();
        break;
    case L_LCB:
        EElgtok ();
        if (EEltok == L_RCB) {
            EEcsetfp (si, C_NULL);
        } else {
            if ((si1 = pstmt ()) == -1) {
                SUwarning (0, "pstmt", "cannot create compound statement");
                return -1;
            }
            EEcsetfp (si, si1);
            si0 = si1;
            while (EEltok != L_RCB) {
                if ((si1 = pstmt ()) == -1) {
                    SUwarning (0, "pstmt", "cannot create compound statement");
                    return -1;
                }
                EEcsetnext (si0, si1);
                si0 = si1;
            }
        }
        EElgtok ();
        break;
    case L_IF:
        if ((si0 = pifst ()) == -1) {
            SUwarning (0, "pstmt", "cannot create if statement");
            return -1;
        }
        EEcsetfp (si, si0);
        break;
    default:
        if ((si0 = pexpr ()) == -1) {
            SUwarning (0, "pstmt", "cannot create expression");
            return -1;
        }
        EEcsetfp (si, si0);
        GTOKIFEQ (L_SEMI);
    }
    return si;
}

static int pifst (void) {
    int isi, ii, ti, ei;

    if ((isi = EEcnew (C_IF)) == -1) {
        SUwarning (0, "pifst", "cannot create statement");
        return -1;
    }
    EElgtok ();
    GTOKIFEQ (L_LP);
    if ((ii = pexpr ()) == -1) {
        SUwarning (0, "pifst", "cannot create expression");
        return -1;
    }
    EEcsetfp (isi, ii);
    GTOKIFEQ (L_RP);
    if ((ti = pstmt ()) == -1) {
        SUwarning (0, "pifst", "cannot create then statement");
        return -1;
    }
    EEcsetnext (ii, ti);
    if (EEltok == L_ELSE) {
        EElgtok ();
        if ((ei = pstmt ()) == -1) {
            SUwarning (0, "pifst", "cannot create else statement");
            return -1;
        }
        EEcsetnext (ti, ei);
    }
    return isi;
}

static int pexpr (void) {
    int ai, ei0, ei1;

    if ((ei0 = pexpi (0)) == -1) {
        SUwarning (0, "pexpr", "cannot create lhs expression");
        return -1;
    }
    if (EEltok != C_ASSIGN)
        return ei0;

    if ((ai = EEcnew (C_ASSIGN)) == -1) {
        SUwarning (0, "pexpr", "cannot create rhs code");
        return -1;
    }
    EEcsetfp (ai, ei0);
    EElgtok ();
    if ((ei1 = pexpr ()) == -1) {
        SUwarning (0, "pexpr", "cannot create rhs expression");
        return -1;
    }
    EEcsetnext (ei0, ei1);
    return ai;
}

static int lextab[][7] = {
    {   L_OR,       0,     0,    0,    0,    0, 0 },
    {  L_AND,       0,     0,    0,    0,    0, 0 },
    {   L_EQ,    L_NE,  L_LT, L_LE, L_GT, L_GE, 0 },
    { L_PLUS, L_MINUS,     0,    0,    0,    0, 0 },
    {  L_MUL,   L_DIV, L_MOD,    0,    0,    0, 0 },
    {      0,       0,     0,    0,    0,    0, 0 }
};

static int parsetab[][7] = {
    {   C_OR,       0,     0,    0,    0,    0, 0 },
    {  C_AND,       0,     0,    0,    0,    0, 0 },
    {   C_EQ,    C_NE,  C_LT, C_LE, C_GT, C_GE, 0 },
    { C_PLUS, C_MINUS,     0,    0,    0,    0, 0 },
    {  C_MUL,   C_DIV, C_MOD,    0,    0,    0, 0 },
    {      0,       0,     0,    0,    0,    0, 0 }
};

static int getop (int t, int i) {
    int j;

    for (j = 0; lextab[i][j] != 0; j++)
        if (t == lextab[i][j])
            return parsetab[i][j];
    return -1;
}

static int pexpi (int k) {
    int ei0, ei1, ei2, ptok;

    if (lextab[k][0] == 0)
        return pexp5 ();

    if ((ei0 = pexpi (k + 1)) == -1) {
        SUwarning (0, "pexpi", "cannot create first expression");
        return -1;
    }
    while ((ptok = getop (EEltok, k)) != -1) {
        if ((ei1 = EEcnew (ptok)) == -1) {
            SUwarning (0, "pexpi", "cannot create code");
            return -1;
        }
        EEcsetfp (ei1, ei0);
        EElgtok ();
        if ((ei2 = pexpi (k + 1)) == -1) {
            SUwarning (0, "pexpi", "cannot create next expression");
            return -1;
        }
        EEcsetnext (ei0, ei2);
        ei0 = ei1;
    }
    return ei0;
}

static int pexp5 (void) {
    int ei0, ei1;

    if (EEltok == L_MINUS) {
        if ((ei0 = EEcnew (C_UMINUS)) == -1) {
            SUwarning (0, "pexp5", "cannot create code");
            return -1;
        }
        EElgtok ();
        if ((ei1 = pexp5 ()) == -1) {
            SUwarning (0, "pexp5", "cannot create expression");
            return -1;
        }
        EEcsetfp (ei0, ei1);
        return ei0;
    }
    return pexp6 ();
}

static int pexp6 (void) {
    int ei0, ei1;

    if (EEltok == L_NOT) {
        if ((ei0 = EEcnew (C_NOT)) == -1) {
            SUwarning (0, "pexp6", "cannot create code");
            return -1;
        }
        EElgtok ();
        if ((ei1 = pexp6 ()) == -1) {
            SUwarning (0, "pexp6", "cannot create expression");
            return -1;
        }
        EEcsetfp (ei0, ei1);
        return ei0;
    }
    return pexp7 ();
}

static int pexp7 (void) {
    int ei0, ei1;

    switch (EEltok) {
    case L_LP:
        if ((ei0 = EEcnew (C_PEXPR)) == -1) {
            SUwarning (0, "pexp7", "cannot create code");
            return -1;
        }
        EElgtok ();
        if ((ei1 = pexpr ()) == -1) {
            SUwarning (0, "pexp7", "cannot create expression");
            return -1;
        }
        GTOKIFEQ (L_RP);
        EEcsetfp (ei0, ei1);
        break;
    case L_STRING:
    case L_NUMBER:
        if ((ei0 = pcons ()) == -1) {
            SUwarning (0, "pexp7", "cannot create constant expression");
            return -1;
        }
        break;
    case L_ID:
        if ((ei0 = pvar ()) == -1) {
            SUwarning (0, "pexp7", "cannot create variable expression");
            return -1;
        }
        break;
    default:
        SUwarning (
            0, "pexp7",
            "unexpected token: %s, string: %s", EElnames[EEltok], EElgetstr ()
        );
    }
    return ei0;
}

static int pcons (void) {
    int ci, vali;
    float f;

    switch (EEltok) {
    case L_NUMBER:
        f = atof (EElstrtok);
        if ((
            ci = (f == (float) (int) f) ? EEcinteger ((int) f) : EEcreal (f)
        ) == -1) {
            SUwarning (0, "pcons", "cannot create number expression");
            return -1;
        }
        break;
    case L_STRING:
        if ((ci = EEcstring (EElstrtok, TRUE)) == -1) {
            SUwarning (0, "pcons", "cannot create string expression");
            return -1;
        }
        break;
    default:
        SUwarning (0, "pcons", "expected scalar, found: %s", EElnames[EEltok]);
        return -1;
    }
    if ((vali = EEcaddval (ci)) == -1) {
        SUwarning (0, "pcons", "cannot add constant");
        return -1;
    }
    EEcsetvali (ci, vali);
    EElgtok ();
    return ci;
}

static int pvar (void) {
    int vi, ci, vari;

    if ((vi = EEcnew (C_VAR)) == -1) {
        SUwarning (0, "pvar", "cannot create variable code");
        return -1;
    }
    if ((ci = EEcstring (EElstrtok, FALSE)) == -1) {
        SUwarning (0, "pvar", "cannot create variable string");
        return -1;
    }
    EEcsetfp (vi, ci);
    if ((vari = EEcaddvar (ci)) == -1) {
        SUwarning (0, "pvar", "cannot add variable");
        return -1;
    }
    EEcsetvari (ci, vari);
    EElgtok ();
    return vi;
}
