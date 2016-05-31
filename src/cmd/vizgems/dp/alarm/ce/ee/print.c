#pragma prototyped
/* Lefteris Koutsofios - AT&T Labs Research */

#include <ast.h>
#include <swift.h>
#include "eeval.h"
#include "eeval_int.h"

static int indent;
#define INDINC(i) (indent += (i))
#define INDDEC(i) (indent -= (i))

static void codepr (EEcode_t *, int);
static void prv (EEvar_t *);
static void prs (char *);
static void pri (int);
static void prd (float);
static void prnl (int);

void EEpprint (EE_t *eep, int initialindent) {
    int vari;

    indent = initialindent;
    prnl (FALSE);
    for (vari = 0; vari < eep->varn; vari++)
        prv (&eep->vars[vari]), prnl (TRUE);
    codepr (eep->codep, 0);
    indent = 0;
    prnl (TRUE);
}

static void codepr (EEcode_t *cp, int ci) {
    int ct, ct1;
    int ci1, ci2;

    switch ((ct = EEcgettype (cp, ci))) {
    case C_ASSIGN:
        codepr (cp, (ci1 = EEcgetfp (cp, ci)));
        prs (" = ");
        codepr (cp, EEcgetnext (cp, ci1));
        break;
    case C_OR:
    case C_AND:
    case C_EQ:
    case C_NE:
    case C_LT:
    case C_LE:
    case C_GT:
    case C_GE:
    case C_PLUS:
    case C_MINUS:
    case C_MUL:
    case C_DIV:
    case C_MOD:
        codepr (cp, (ci1 = EEcgetfp (cp, ci)));
        switch (ct) {
        case C_OR:    prs (" || ");  break;
        case C_AND:   prs (" && ");  break;
        case C_EQ:    prs (" == "); break;
        case C_NE:    prs (" != "); break;
        case C_LT:    prs (" < ");  break;
        case C_LE:    prs (" <= "); break;
        case C_GT:    prs (" > ");  break;
        case C_GE:    prs (" >= "); break;
        case C_PLUS:  prs (" + ");  break;
        case C_MINUS: prs (" - ");  break;
        case C_MUL:   prs (" * ");  break;
        case C_DIV:   prs (" / ");  break;
        case C_MOD:   prs (" % ");  break;
        }
        codepr (cp, EEcgetnext (cp, ci1));
        break;
    case C_NOT:
        prs ("!");
        codepr (cp, EEcgetfp (cp, ci));
        break;
    case C_UMINUS:
        prs ("-");
        codepr (cp, EEcgetfp (cp, ci));
        break;
    case C_PEXPR:
        prs ("(");
        codepr (cp, EEcgetfp (cp, ci));
        prs (")");
        break;
    case C_INTEGER:
        pri (EEcgetinteger (cp, ci));
        break;
    case C_REAL:
        prd (EEcgetreal (cp, ci));
        break;
    case C_CSTRING:
    case C_RSTRING:
        prs ("\""), prs (EEcgetstring (cp, ci)), prs ("\"");
        break;
    case C_VAR:
        ci1 = EEcgetfp (cp, ci);
        prs (EEcgetstring (cp, ci1));
        break;
    case C_STMT:
        ci1 = EEcgetfp (cp, ci);
        if (ci1 == C_NULL) {
            prs (";");
            break;
        }
        if (EEcgetnext (cp, ci1) == C_NULL) {
            codepr (cp, ci1);
            ct1 = EEcgettype (cp, ci1);
            if (!(ct1 >= C_STMT && ct1 <= C_IF))
                prs (";");
        } else {
            prs (" {");
            INDINC (2);
            for (; ci1 != C_NULL; ci1 = EEcgetnext (cp, ci1)) {
                prnl (TRUE);
                codepr (cp, ci1);
            }
            INDDEC (2);
            prnl (TRUE);
            prs ("}");
        }
        break;
    case C_IF:
        ci1 = EEcgetfp (cp, ci);
        prs ("if (");
        codepr (cp, ci1);
        prs (")");
        ci1 = EEcgetnext (cp, ci1);
        ci2 = EEcgetfp (cp, ci1);
        if (ci2 == C_NULL || EEcgetnext (cp, ci2) == C_NULL) {
            INDINC (2);
            prnl (TRUE);
            codepr (cp, ci1);
            INDDEC (2);
        } else {
            codepr (cp, ci1);
        }
        ci1 = EEcgetnext (cp, ci1);
        if (ci1 == C_NULL)
            break;
        if (ci2 == C_NULL || EEcgetnext (cp, ci2) == C_NULL) {
            prnl (TRUE);
            prs ("else");
        } else {
            prs (" else");
        }
        ci2 = EEcgetfp (cp, ci1);
        if (ci2 == C_NULL || EEcgetnext (cp, ci2) == C_NULL) {
            INDINC (2);
            prnl (TRUE);
            codepr (cp, ci1);
            INDDEC (2);
        } else {
            codepr (cp, ci1);
        }
        break;
    default:
        SUwarning (0, "codepr", "unknown code type: %d", ct);
    }
}

static void prv (EEvar_t *varp) {
    sfprintf (sfstderr, "name = %-16s id = %d", varp->nam, varp->id);
}

static void prs (char *s) {
    sfprintf (sfstderr, "%s", s);
}

static void pri (int i) {
    sfprintf (sfstderr, "%d", i);
}

static void prd (float f) {
    sfprintf (sfstderr, "%f", f);
}

static void prnl (int nlflag) {
    int i;

    if (nlflag)
        sfprintf (sfstderr, "\n");
    for (i = 0; i < indent; i++)
        sfprintf (sfstderr, " ");
}
