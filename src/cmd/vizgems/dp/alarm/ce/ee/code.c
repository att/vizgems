#pragma prototyped
/* Lefteris Koutsofios - AT&T Labs Research */

#include <ast.h>
#include <vmalloc.h>
#include <swift.h>
#include <ctype.h>
#include "pmem.h"
#include "eeval.h"
#include "eeval_int.h"

static EEcode_t *codep;
static int coden, codei;
#define CODEINCR 1000
#define CODESIZE sizeof (EEcode_t)

static EEvar_t *vars;
static int varn, vari;
#define VARINCR 10
#define VARSIZE sizeof (EEvar_t)

static EEval_t *vals;
static int valn, vali;
#define VALINCR 10
#define VALSIZE sizeof (EEval_t)

static int stringoffset;

static char **currdict;

int EEcinit (void) {
    EEcode_t c;

    if (!(codep = vmalloc (Vmheap, CODEINCR * CODESIZE))) {
        SUwarning (0, "EEcinit", "cannot allocate codep");
        return -1;
    }
    coden = CODEINCR;
    codei = 0;

    if (!(vars = vmalloc (Vmheap, VARINCR * VARSIZE))) {
        SUwarning (0, "EEcinit", "cannot allocate vars");
        return -1;
    }
    varn = VARINCR;
    vari = 0;

    if (!(vals = vmalloc (Vmheap, VALINCR * VALSIZE))) {
        SUwarning (0, "EEcinit", "cannot allocate vals");
        return -1;
    }
    valn = VALINCR;
    vali = 0;

    stringoffset = (char *) &c.u.s[0] - (char *) &c + 1;
    /* the + 1 above accounts for the null character */

    return 0;
}

int EEcterm (void) {
    vmfree (Vmheap, codep);
    codep = NULL;
    coden = codei = 0;

    vmfree (Vmheap, vars);
    vars = NULL;
    varn = vari = 0;

    vmfree (Vmheap, vals);
    vals = NULL;
    valn = vali = 0;

    return 0;
}

int EEcbegin (char **dict) {
    currdict = dict;
    codei = 0;
    vari = 0;
    vali = 0;

    return 0;
}

int EEcend (EE_t *eep) {
    int codej, size;

    if (!(eep->codep = PMalloc (codei * CODESIZE))) {
        SUwarning (0, "EEcend", "cannot allocate code array");
        return -1;
    }
    memcpy (eep->codep, codep, codei * CODESIZE);
    eep->coden = codei;

    if (!(eep->vars = PMalloc (vari * sizeof (EEvar_t)))) {
        SUwarning (0, "EEcend", "cannot allocate var array");
        return -1;
    }
    memcpy (eep->vars, vars, vari * sizeof (EEvar_t));
    eep->varn = vari;

    if (!(eep->vals = PMalloc (vali * sizeof (EEval_t)))) {
        SUwarning (0, "EEcend", "cannot allocate val array");
        return -1;
    }
    memcpy (eep->vals, vals, vali * sizeof (EEval_t));
    eep->valn = vali;

    for (codej = 0; codej < codei; ) {
        if (
            eep->codep[codej].ctype != C_CSTRING &&
            eep->codep[codej].ctype != C_RSTRING
        ) {
            codej++;
            continue;
        }
        if (eep->codep[codej].vari >= 0)
            eep->vars[eep->codep[codej].vari].nam = &eep->codep[codej].u.s[0];
        else if (eep->codep[codej].vali >= 0)
            eep->vals[eep->codep[codej].vali].u.s = &eep->codep[codej].u.s[0];
        size = (strlen (
            &eep->codep[codej].u.s[0]
        ) + stringoffset + CODESIZE - 1) / CODESIZE;
        codej += size;
    }

    return 0;
}

int EEcnew (int ctype) {
    int codej;

    if (codei >= coden) {
        if (!(codep = vmresize (
            Vmheap, codep, (coden + CODEINCR) * CODESIZE, VM_RSCOPY
        ))) {
            SUwarning (0, "EEcnew", "cannot resize code array");
            return -1;
        }
        coden += CODEINCR;
    }
    codej = codei++;
    memset (&codep[codej], 0, sizeof (EEcode_t));
    codep[codej].ctype = ctype;
    codep[codej].fp = C_NULL;
    codep[codej].next = C_NULL;
    codep[codej].vari = codep[codej].vali = -1;

    return codej;
}

int EEcinteger (int i) {
    int codej;

    if (codei >= coden) {
        if (!(codep = vmresize (
            Vmheap, codep, (coden + CODEINCR) * CODESIZE, VM_RSCOPY
        ))) {
            SUwarning (0, "EEcinteger", "cannot resize code array");
            return -1;
        }
        coden += CODEINCR;
    }
    codej = codei++;
    memset (&codep[codej], 0, sizeof (EEcode_t));
    codep[codej].ctype = C_INTEGER;
    codep[codej].fp = C_NULL;
    codep[codej].next = C_NULL;
    codep[codej].vari = codep[codej].vali = -1;
    codep[codej].u.i = i;

    return codej;
}

int EEcreal (float f) {
    int codej;

    if (codei >= coden) {
        if (!(codep = vmresize (
            Vmheap, codep, (coden + CODEINCR) * CODESIZE, VM_RSCOPY
        ))) {
            SUwarning (0, "EEcreal", "cannot resize code array");
            return -1;
        }
        coden += CODEINCR;
    }
    codej = codei++;
    memset (&codep[codej], 0, sizeof (EEcode_t));
    codep[codej].ctype = C_REAL;
    codep[codej].fp = C_NULL;
    codep[codej].next = C_NULL;
    codep[codej].vari = codep[codej].vali = -1;
    codep[codej].u.f = f;

    return codej;
}

int EEcstring (char *s, int recheck) {
    int codej, size, incr;
    char *s1;

    size = (strlen (s) + stringoffset + CODESIZE - 1) / CODESIZE;
    if (codei + size > coden) {
        incr = (size > CODEINCR) ? size : CODEINCR;
        if (!(codep = vmresize (
            Vmheap, codep, (coden + incr) * CODESIZE, VM_RSCOPY
        ))) {
            SUwarning (0, "EEcnew", "cannot resize code array");
            return -1;
        }
        coden += incr;
    }
    codej = codei, codei += size;
    memset (&codep[codej], 0, sizeof (EEcode_t) * size);
    codep[codej].ctype = C_CSTRING;
    if (recheck)
        for (s1 = s; *s1; s1++)
            if (!isalnum (*s1) && *s1 != '_') {
                codep[codej].ctype = C_RSTRING;
                break;
            }
    codep[codej].fp = C_NULL;
    codep[codej].next = C_NULL;
    codep[codej].vari = codep[codej].vali = -1;
    strcpy ((char *) &codep[codej].u.s[0], s);

    return codej;
}

void EEcsetfp (int codej, int codek) {
    codep[codej].fp = codek;
}

void EEcsetnext (int codej, int codek) {
    codep[codej].next = codek;
}

void EEcsetvari (int codej, int vari) {
    codep[codej].vari = vari;
}

void EEcsetvali (int codej, int vali) {
    codep[codej].vali = vali;
}

int EEcaddvar (int codej) {
    int varj, id;
    char *np;

    np = (char *) &codep[codej].u.s[0];

    for (varj = 0; varj < vari; varj++) {
        if (strcmp (vars[varj].nam, np) == 0)
            return varj;
    }
    if (vari >= varn) {
        if (!(vars = vmresize (
            Vmheap, vars, (varn + VARINCR) * VARSIZE, VM_RSCOPY
        ))) {
            SUwarning (0, "EEcaddvar", "cannot resize var array");
            return -1;
        }
        varn += VARINCR;
    }
    varj = vari++;
    vars[varj].id = -1;
    for (id = 0; currdict[id]; id++) {
        if (strcmp (currdict[id], np) == 0) {
            vars[varj].id = id;
            break;
        }
    }
    vars[varj].nam = np;

    return varj;
}

int EEcaddval (int codej) {
    int valj;

    for (valj = 0; valj < vali; valj++) {
        if (vals[valj].type != codep[codej].ctype)
            continue;
        switch (codep[codej].ctype) {
        case C_INTEGER:
            if (vals[valj].u.i == codep[codej].u.i)
                return valj;
            break;
        case C_REAL:
            if (vals[valj].u.f == codep[codej].u.f)
                return valj;
            break;
        case C_CSTRING:
        case C_RSTRING:
            if (strcmp (vals[valj].u.s, &codep[codej].u.s[0]) == 0)
                return valj;
            break;
        }
    }

    if (vali >= valn) {
        if (!(vals = vmresize (
            Vmheap, vals, (valn + VALINCR) * VALSIZE, VM_RSCOPY
        ))) {
            SUwarning (0, "EEcaddval", "cannot resize val array");
            return -1;
        }
        valn += VALINCR;
    }
    valj = vali++;
    vals[valj].type = codep[codej].ctype;
    switch (codep[codej].ctype) {
    case C_INTEGER: vals[valj].u.i = codep[codej].u.i;     break;
    case C_REAL:    vals[valj].u.f = codep[codej].u.f;     break;
    case C_CSTRING: vals[valj].u.s = &codep[codej].u.s[0]; break;
    case C_RSTRING: vals[valj].u.s = &codep[codej].u.s[0]; break;
    }

    return valj;
}
