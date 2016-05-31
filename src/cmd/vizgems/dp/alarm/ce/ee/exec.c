#pragma prototyped
/* Lefteris Koutsofios - AT&T Labs Research */

#include <ast.h>
#include <vmalloc.h>
#include <regex.h>
#include <swift.h>
#include "eeval.h"
#include "eeval_int.h"

typedef struct recache_s {
    char *sp;
    regex_t recode;
} recache_t;

static recache_t *recs;
static int recl, recn;
#define RECINCR 1000
#define RECSIZE sizeof (recache_t)

static EEgetvar getvarcb;
static EEsetvar setvarcb;
static void *cbcontext;

static int partialmode;

static EEval_t valn, valt, valf;

static int eeval (EE_t *, int, EEval_t *);

static int getvar (EE_t *, int, EEval_t *);
static int setvar (EE_t *, int, EEval_t *);

static int boolop (EEval_t *, EEval_t *);
static int orderop (EEval_t *, int, EEval_t *, EEval_t *);
static int arithop (EEval_t *, int, EEval_t *, EEval_t *);

int EEeinit (void) {
    if (!(recs = vmalloc (Vmheap, RECINCR * RECSIZE))) {
        SUwarning (0, "EEeinit", "cannot allocate recs");
        return -1;
    }
    recn = RECINCR;
    recl = 0;

    valn.type = C_NULL;
    valn.u.i = 0;
    valf.type = C_INTEGER;
    valf.u.i = FALSE;
    valt.type = C_INTEGER;
    valt.u.i = TRUE;

    return 0;
}

int EEeterm (void) {
    vmfree (Vmheap, recs);
    recn = recl = 0;
    return 0;
}

int EEeunit (
    EE_t *eep, EEgetvar getfunc, EEsetvar setfunc, void *context, int peval
) {
    EEval_t v;

    if (!eep || !getfunc || !setfunc || !context) {
        SUwarning (0, "EEunit", "null parameters");
        return -1;
    }

    setvarcb = setfunc;
    getvarcb = getfunc;
    cbcontext = context;
    partialmode = peval;
    systemerror = FALSE;
    if (eeval (eep, 0, &v) == -1) {
        if (systemerror) {
            SUwarning (0, "EEunit", "evaluation system error");
            return -1;
        } else {
            SUwarning (0, "EEunit", "evaluation failed");
            return 1;
        }
    }

    return 0;
}

static int eeval (EE_t *eep, int ci, EEval_t *ovp) {
    EEcode_t *cp;
    int ctype;
    int ci1, ci2;
    EEval_t v1, v2;
    int isfalse;

    cp = eep->codep;
tailrec:
    ctype = EEcgettype (cp, ci);
    switch (ctype) {
    case C_ASSIGN:
        ci1 = EEcgetfp (cp, ci);
        if (eeval (eep, EEcgetnext (cp, ci1), ovp) == -1) {
            SUwarning (0, "eeval", "assign rhs eval failed");
            return -1;
        }
        if (setvar (eep, EEcgetvari (cp, EEcgetfp (cp, ci1)), ovp) == -1) {
            SUwarning (0, "eeval", "setvar failed");
            return -1;
        }
        return 0;
    case C_OR:
    case C_AND:
    case C_NOT:
        ci1 = EEcgetfp (cp, ci);
        if (eeval (eep, ci1, &v1) == -1) {
            SUwarning (0, "eeval", "boolean l eval failed");
            return -1;
        }
        switch (ctype) {
        case C_OR:
            if (boolop (&v1, ovp) == -1) {
                SUwarning (0, "eeval", "OR l boolop failed");
                return -1;
            }
            if (ovp->type == C_INTEGER && ovp->u.i == valt.u.i)
                return 0;

            isfalse = (ovp->type == C_INTEGER && ovp->u.i == valf.u.i);
            if (eeval (eep, EEcgetnext (cp, ci1), &v2) == -1) {
                SUwarning (0, "eeval", "OR r eval failed");
                return -1;
            }
            if (boolop (&v2, ovp) == -1) {
                SUwarning (0, "eeval", "OR r boolop failed");
                return -1;
            }
            if (partialmode)
                if (isfalse && ovp->type == C_INTEGER && ovp->u.i == valf.u.i)
                    *ovp = valf;
                else
                    *ovp = valt;
            break;
        case C_AND:
            if (boolop (&v1, ovp) == -1) {
                SUwarning (0, "eeval", "AND l boolop failed");
                return -1;
            }
            if (ovp->type == C_INTEGER && ovp->u.i == valf.u.i)
                return 0;

            if (eeval (eep, EEcgetnext (cp, ci1), &v2) == -1) {
                SUwarning (0, "eeval", "AND r eval failed");
                return -1;
            }
            if (boolop (&v2, ovp) == -1) {
                SUwarning (0, "eeval", "AND r boolop failed");
                return -1;
            }
            if (partialmode)
                if (ovp->type == C_NULL)
                    *ovp = valt;
            break;
        case C_NOT:
            if (boolop (&v1, ovp) == -1) {
                SUwarning (0, "eeval", "NOT boolop failed");
                return -1;
            }
            if (partialmode && ovp->type == C_NULL)
                *ovp = valt;
            else if (ovp->u.i == valf.u.i)
                *ovp = valt;
            else
                *ovp = valf;
            break;
        }
        return 0;
    case C_EQ:
    case C_NE:
    case C_LT:
    case C_LE:
    case C_GT:
    case C_GE:
        ci1 = EEcgetfp (cp, ci);
        if (eeval (eep, ci1, &v1) == -1) {
            SUwarning (0, "eeval", "order l eval failed");
            return -1;
        }
        if (eeval (eep, EEcgetnext (cp, ci1), &v2) == -1) {
            SUwarning (0, "eeval", "order r eval failed");
            return -1;
        }
        if (orderop (&v1, ctype, &v2, ovp) == -1) {
            SUwarning (0, "eeval", "orderop failed");
            return -1;
        }
        if (partialmode && ovp->type == C_NULL)
            *ovp = valt;
        return 0;
    case C_PLUS:
    case C_MINUS:
    case C_MUL:
    case C_DIV:
    case C_MOD:
    case C_UMINUS:
        ci1 = EEcgetfp (cp, ci);
        if ((v1.type = EEcgettype (cp, ci1)) == C_INTEGER)
            v1.u.i = EEcgetinteger (cp, ci1);
        else if (v1.type == C_REAL)
            v1.u.f = EEcgetreal (cp, ci1);
        else if (eeval (eep, ci1, &v1) == -1) {
            SUwarning (0, "eeval", "arithmetic l eval failed");
            return -1;
        }
        if (ctype == C_UMINUS) {
            if (arithop (&v1, ctype, &v1, ovp) == -1) {
                SUwarning (0, "eeval", "UMINUS arithop failed");
                return -1;
            }
            return 0;
        }

        ci1 = EEcgetnext (cp, ci1);
        if ((v2.type = EEcgettype (cp, ci1)) == C_INTEGER)
            v2.u.i = EEcgetinteger (cp, ci1);
        else if (v2.type == C_REAL)
            v2.u.f = EEcgetreal (cp, ci1);
        else if (eeval (eep, ci1, &v2) == -1) {
            SUwarning (0, "eeval", "arithmetic r eval failed");
            return -1;
        }
        if (arithop (&v1, ctype, &v2, ovp) == -1) {
            SUwarning (0, "eeval", "arithop failed");
            return -1;
        }
        return 0;
    case C_PEXPR:
        ci = EEcgetfp (cp, ci);
        goto tailrec;
    case C_INTEGER:
    case C_REAL:
    case C_CSTRING:
    case C_RSTRING:
        *ovp = eep->vals[EEcgetvali (cp, ci)];
        return 0;
    case C_VAR:
        return getvar (eep, EEcgetvari (cp, EEcgetfp (cp, ci)), ovp);
    case C_STMT:
        for (ci1 = EEcgetfp (cp, ci); ci1 != C_NULL; )
            if ((ci2 = EEcgetnext (cp, ci1)) != C_NULL) {
                if (eeval (eep, ci1, ovp) == -1) {
                    SUwarning (0, "eeval", "statement eval failed");
                    return -1;
                }
                *ovp = valn;
                ci1 = ci2;
            } else {
                ci = ci1;
                goto tailrec;
            }
        break;
    case C_IF:
        ci1 = EEcgetfp (cp, ci);
        if (eeval (eep, ci1, &v1) == -1) {
            SUwarning (0, "eeval", "IF eval failed");
            return -1;
        }
        if (boolop (&v1, ovp) == -1) {
            SUwarning (0, "eeval", "IF boolop failed");
            return -1;
        }
        if (ovp->u.i == valt.u.i) {
            ci = EEcgetnext (cp, ci1);
            goto tailrec;
        } else if ((ci = EEcgetnext (cp, EEcgetnext (cp, ci1))) != C_NULL)
            goto tailrec;
        break;
    default:
        SUwarning (0, "eeval", "unknown program token type %d", ctype);
        systemerror = TRUE;
        return -1;
    }
    return 0;
}

static int getvar (EE_t *eep, int vari, EEval_t *vp) {
    return (*getvarcb) (eep, vari, vp, cbcontext);
}

static int setvar (EE_t *eep, int vari, EEval_t *vp) {
    return (*setvarcb) (eep, vari, vp, cbcontext);
}

static int boolop (EEval_t *ivp, EEval_t *ovp) {
    switch (ivp->type) {
    case C_NULL:
        SUwarning (0, "boolop", "NULL value");
        if (partialmode) {
            *ovp = valn;
            return 0;
        }
        return -1;
    case C_INTEGER:
        *ovp = (ivp->u.i == 0) ? valf : valt;
        break;
    case C_REAL:
        *ovp = (ivp->u.f == 0.0) ? valf : valt;
        break;
    case C_CSTRING:
    case C_RSTRING:
        *ovp = (ivp->u.s == NULL) ? valf : valt;
        break;
    default:
        SUwarning (0, "boolop", "unknown type");
        return -1;
    }
    return 0;
}

#define TT(t1, t2) ((t1) * 10000 + (t2))

static int orderop (EEval_t *ivp1, int op, EEval_t *ivp2, EEval_t *ovp) {
    char *rs, *cs;
    recache_t *recp;
    int reci;
    int r;

    if (ivp1->type == C_NULL || ivp2->type == C_NULL) {
        if (partialmode) {
            *ovp = valn;
            return 0;
        } else {
            SUwarning (0, "orderop", "NULL values");
            return -1;
        }
    }

    switch (TT (ivp1->type, ivp2->type)) {
    case TT(C_INTEGER, C_INTEGER):
        r = (ivp1->u.i == ivp2->u.i) ? 0 : ((ivp1->u.i < ivp2->u.i) ? -1 : 1);
        break;
    case TT(C_INTEGER, C_REAL):
        r = (ivp1->u.i == ivp2->u.f) ? 0 : ((ivp1->u.i < ivp2->u.f) ? -1 : 1);
        break;
    case TT(C_REAL, C_INTEGER):
        r = (ivp1->u.f == ivp2->u.i) ? 0 : ((ivp1->u.f < ivp2->u.i) ? -1 : 1);
        break;
    case TT(C_REAL, C_REAL):
        r = (ivp1->u.f == ivp2->u.f) ? 0 : ((ivp1->u.f < ivp2->u.f) ? -1 : 1);
        break;
    case TT(C_CSTRING, C_CSTRING):
        r = strcmp (ivp1->u.s, ivp2->u.s);
        break;
    case TT(C_CSTRING, C_RSTRING):
    case TT(C_RSTRING, C_CSTRING):
        if (op != C_EQ && op != C_NE) {
            SUwarning (0, "orderop", "only == and != acceptable for regex ops");
            return -1;
        }
        if (ivp1->type == C_CSTRING)
            cs = ivp1->u.s, rs = ivp2->u.s;
        else
            cs = ivp2->u.s, rs = ivp1->u.s;
        for (reci = 0; reci < recl; reci++)
            if (recs[reci].sp == rs)
                break;
        if (reci == recl) {
            if (recl >= recn) {
                if (!(recs = vmresize (
                    Vmheap, recs, (recn + RECINCR) * RECSIZE, VM_RSCOPY
                ))) {
                    SUwarning (0, "orderop", "cannot resize rec array");
                    systemerror = TRUE;
                    return -1;
                }
                recn += RECINCR;
            }
            recl++;
            recp = &recs[reci];
            SUwarning (1, "orderop", "compiling regex for %s", rs);
            if (regcomp (
                &recp->recode, rs,
                REG_NULL | REG_EXTENDED | REG_LEFT | REG_RIGHT
            ) != 0) {
                SUwarning (0, "orderop", "failed to compile regex for %s", rs);
                return -1;
            }
            recp->sp = rs;
        } else
            recp = &recs[reci];
        r = regexec (
            &recp->recode, cs, 0, NULL,
            REG_NULL | REG_EXTENDED | REG_LEFT | REG_RIGHT
        );
        break;
    default:
        SUwarning (
            0, "orderop", "incompatible operand types %d,%d",
            ivp1->type, ivp2->type
        );
        return -1;
    }

    switch (op) {
    case C_EQ: *ovp = (r == 0) ? valt : valf; break;
    case C_NE: *ovp = (r != 0) ? valt : valf; break;
    case C_LT: *ovp = (r <  0) ? valt : valf; break;
    case C_LE: *ovp = (r <= 0) ? valt : valf; break;
    case C_GT: *ovp = (r >  0) ? valt : valf; break;
    case C_GE: *ovp = (r >= 0) ? valt : valf; break;
    default:
        SUwarning (0, "orderop", "unknown operand %d", op);
        return -1;
    }
    return 0;
}

static int arithop (EEval_t *ivp1, int op, EEval_t *ivp2, EEval_t *ovp) {
    double f1, f2, f3;

    switch (ivp1->type) {
    case C_INTEGER: f1 = ivp1->u.i; break;
    case C_REAL:    f1 = ivp1->u.f; break;
    case C_CSTRING: f1 = 0.0;       break;
    case C_RSTRING: f1 = 0.0;       break;
    case C_NULL:
        if (partialmode) {
            *ovp = valn;
            return 0;
        }
        SUwarning (0, "arithop", "NULL lvalue");
        return -1;
    case 0:
        f1 = 0.0;
        break;
    default:
        SUwarning (0, "arithop", "unknown ltype %d", ivp1->type);
        return -1;
    }

    if (op == C_UMINUS) {
        f3 = -f1;
        goto result;
    }

    switch (ivp2->type) {
    case C_INTEGER: f2 = ivp2->u.i; break;
    case C_REAL:    f2 = ivp2->u.f; break;
    case C_CSTRING: f2 = 0.0;       break;
    case C_RSTRING: f2 = 0.0;       break;
    case C_NULL:
        if (partialmode) {
            *ovp = valn;
            return 0;
        }
        SUwarning (0, "arithop", "NULL rvalue");
        return -1;
    case 0:
        f2 = 0.0;
        break;
    default:
        SUwarning (0, "arithop", "unknown rtype %d", ivp2->type);
        return -1;
    }

    switch (op) {
    case C_PLUS:  f3 = f1 + f2;             break;
    case C_MINUS: f3 = f1 - f2;             break;
    case C_MUL:   f3 = f1 * f2;             break;
    case C_DIV:   f3 = f1 / f2;             break;
    case C_MOD:   f3 = (int) f1 % (int) f2; break;
    default:
        SUwarning (0, "arithop", "unknown operand %d", op);
        return -1;
    }

result:
    if (f3 == (double) (int) f3)
        ovp->type = C_INTEGER, ovp->u.i = f3;
    else
        ovp->type = C_REAL, ovp->u.f = f3;
    return 0;
}
