#pragma prototyped

#include <ast.h>
#include <vmalloc.h>
#include <swift.h>
#include "pmem.h"
#include "eeval.h"
#include "vgce.h"

static var_t *tvars;
static int tvarn;

static char *emap[] = {
    "FMATCH",  /* FMATCH  */
    "SMATCH",  /* SMATCH  */
    "INSERT",  /* INSERT  */
    "REMOVE",  /* REMOVE  */
    "FTICKET", /* FTICKET */
    "STICKET", /* STICKET */
    "PRUNE",   /* PRUNE   */
    "EMIT",    /* EMIT    */
    NULL,
};

int comprules (int, int);

static int mergevars (rule_t *rulep);

int initrules (void) {
    if (!(tvars = vmalloc (Vmheap, 100 * sizeof (var_t)))) {
        SUwarning (1, "initrules", "cannot allocate tvars");
        return -1;
    }
    tvarn = 100;
    return 0;
}

int termrules (void) {
    vmfree (Vmheap, tvars), tvarn = 0;
    return 0;
}

int loadrules (char *file) {
    Sfio_t *fp;
    char *line, *s;
    int ln;
    int state;
    rule_t *rulep;
    int rulei, rulej, rulek;
    int eei, eel;

    if (!(fp = sfopen (NULL, file, "r"))) {
        SUwarning (0, "loadrules", "cannot open rules file");
        return -1;
    }

    ln = 0, state = 0, rulep = NULL;
    while ((line = sfgetr (fp, '\n', 1))) {
        ln++;
        for (s = line; *s; s++)
            if (*s == '#' || (*s != ' ' && *s != '\t'))
                break;
        if (*s == 0 || *s == '#')
            continue;
        if (strcmp (line, "BEGIN") == 0) {
            if (state != 0) {
                SUwarning (
                    0, "loadrules", "BEGIN tag out of order, line: %d", ln
                );
                return -1;
            }
            if ((rulei = insertrule ()) == -1) {
                SUwarning (0, "loadrules", "cannot insert rule");
                return -1;
            }
            rulep = &rootp->rules[rulei];
            rulep->mark = rootp->mark;
            state = 1;
        } else if (strcmp (line, "END") == 0) {
            if (state != 1 || !rulep) {
                SUwarning (
                    0, "loadrules", "END tag out of order, line: %d", ln
                );
                return -1;
            }
            if (!rulep->id || !rulep->eeflags[CE_EXPR_FMATCH]) {
                SUwarning (0, "loadrules", "incomplete rule");
                if (removerule (rulei) == -1) {
                    SUwarning (0, "loadrules", "cannot remove rule");
                    return -1;
                }
            }
            if (mergevars (rulep) == -1) {
                SUwarning (0, "loadrules", "cannot generate merged var list");
                return -1;
            }
            VG_warning (0, "RULE INFO", "loaded rule %s", rulep->id);
            for (rulej = 0; rulej < rootp->rulel; rulej++) {
                if (
                    rulei == rulej ||
                    !rootp->rules[rulej].id || !rootp->rules[rulei].id
                )
                    continue;
                if (strcmp (rulep->id, rootp->rules[rulej].id) != 0)
                    continue;
                if (comprules (rulei, rulej)) {
                    VG_warning (
                        0, "RULE INFO", "keeping rule %s",
                        rootp->rules[rulej].id
                    );
                    rulek = rulei;
                    rootp->rules[rulej].mark = rootp->mark;
                } else {
                    VG_warning (
                        0, "RULE INFO", "replacing rule %s",
                        rootp->rules[rulej].id
                    );
                    rulek = rulej;
                }
                if (removematchesbyrule (rulek) || removerule (rulek) == -1) {
                    SUwarning (0, "loadrules", "cannot remove old rule");
                    return -1;
                }
            }
            state = 0;
            rulep = NULL;
        } else if (strncmp (line, "ID ", 3) == 0) {
            if (state != 1 || !rulep) {
                SUwarning (0, "loadrules", "ID tag unexpected, line: %d", ln);
                return -1;
            }
            if (!(rulep->id = PMstrdup (line + 3))) {
                SUwarning (0, "loadrules", "cannot allocate id");
                return -1;
            }
        } else if (strncmp (line, "DESCRIPTION ", 12) == 0) {
            if (state != 1 || !rulep) {
                SUwarning (
                    0, "loadrules", "COMMENT tag unexpected, line: %d", ln
                );
                return -1;
            }
            if (!(rulep->descr = PMstrdup (line + 12))) {
                SUwarning (0, "loadrules", "cannot allocate comment");
                return -1;
            }
        } else {
            for (eei = 0; eei < CE_EXPR_SIZE; eei++) {
                eel = strlen (emap[eei]);
                if (strncmp (line, emap[eei], eel) == 0 && line[eel] == ' ') {
                    if (state != 1 || !rulep || rulep->eeflags[eei]) {
                        SUwarning (
                            0, "loadrules", "%s tag unexpected", emap[eei]
                        );
                        return -1;
                    }
                    if (EEparse (
                        line + eel + 1, varmap, &rulep->ees[eei]
                    ) == -1) {
                        SUwarning (
                            0, "loadrules", "cannot parse %s tag", emap[eei]
                        );
                        return -1;
                    }
                    rulep->eeflags[eei] = TRUE;
                    break;
                }
            }
            if (eei == CE_EXPR_SIZE)
                SUwarning (
                    0, "loadrules", "unknown tag, line %d: %s", ln, line
                );
        }
    }
    sfclose (fp);
    for (rulej = 0; rulej < rootp->rulel; rulej++) {
        if (!rootp->rules[rulej].id)
            continue;
        if (rootp->rules[rulej].mark == rootp->mark)
            continue;
        VG_warning (
            0, "RULE INFO", "removing rule %s", rootp->rules[rulej].id
        );
        if (removematchesbyrule (rulej) || removerule (rulej) == -1) {
            SUwarning (0, "loadrules", "cannot remove old rule");
            return -1;
        }
    }
    packrules ();
    VG_warning (0, "RULE INFO", "total rules %d", rootp->rulel);
    return 0;
}

int comprules (int ri1, int ri2) {
    rule_t *rp1, *rp2;
    EE_t *eep1, *eep2;
    int eei;
    EEvar_t *varp1, *varp2;
    int vari;
    EEval_t *valp1, *valp2;
    int vali;
    var_t *vp1, *vp2;
    int vi;

    rp1 = &rootp->rules[ri1], rp2 = &rootp->rules[ri2];

    if (strcmp (rp1->id, rp2->id) != 0)
        return FALSE;

    if (strcmp (rp1->descr, rp2->descr) != 0)
        return FALSE;

    for (eei = 0; eei < CE_EXPR_SIZE; eei++) {
        if (rp1->eeflags[eei] != rp2->eeflags[eei])
            return FALSE;
        if (!rp1->eeflags[eei])
            continue;
        eep1 = &rp1->ees[eei], eep2 = &rp2->ees[eei];
        if (eep1->coden != eep2->coden)
            return FALSE;
        if (memcmp (
            eep1->codep, eep2->codep, eep1->coden * sizeof (EEcode_t)
        ) != 0)
            return FALSE;
        if (eep1->varn != eep2->varn)
            return FALSE;
        for (vari = 0; vari < eep1->varn; vari++) {
            varp1 = &eep2->vars[vari], varp2 = &eep2->vars[vari];
            if (varp1->id != varp2->id)
                return FALSE;
            if (strcmp (varp1->nam, varp2->nam) != 0)
                return FALSE;
        }
        for (vali = 0; vali < eep1->valn; vali++) {
            valp1 = &eep2->vals[vali], valp2 = &eep2->vals[vali];
            if (valp1->type != valp2->type)
                return FALSE;
            switch (valp1->type) {
            case EE_VAL_TYPE_NULL:
                break;
            case EE_VAL_TYPE_INTEGER:
                if (valp1->u.i != valp2->u.i)
                    return FALSE;
                break;
            case EE_VAL_TYPE_REAL:
                if (valp1->u.f != valp2->u.f)
                    return FALSE;
                break;
            case EE_VAL_TYPE_CSTRING:
            case EE_VAL_TYPE_RSTRING:
                if (strcmp (valp1->u.s, valp2->u.s) != 0)
                    return FALSE;
                break;
            }
        }
        if (rp1->varn != rp2->varn)
            return FALSE;
        for (vi = 0; vi < rp1->varn; vi++) {
            vp1 = &rp2->vars[vi], vp2 = &rp2->vars[vi];
            if (vp1->id != vp2->id)
                return FALSE;
            if (strcmp (vp1->nam, vp2->nam) != 0)
                return FALSE;
        }
    }
    return TRUE;
}

void printrules (int lod) {
    int rulei;

    if (lod <= 1)
        return;

    sfprintf (sfstderr, "rules: %d\n", rootp->rulel);

    if (lod <= 2)
        return;

    for (rulei = 0; rulei < rootp->rulel; rulei++)
        printrule (rulei, lod);
}

int insertrule (void) {
    int rulei;

    for (rulei = 0; rulei < rootp->rulen; rulei++)
        if (!rootp->rules[rulei].id)
            break;
    if (rulei == rootp->rulen) {
        if (!(rootp->rules = PMresize (
            rootp->rules, sizeof (rule_t) * (rootp->rulen + CE_ROOT_RULEINCR)
        ))) {
            SUwarning (0, "insertrule", "cannot grow rule array");
            return -1;
        }
        memset (
            &rootp->rules[rootp->rulen], 0, sizeof (rule_t) * CE_ROOT_RULEINCR
        );
        rootp->rulen += CE_ROOT_RULEINCR;
    }
    if (rootp->rulel < rulei + 1)
        rootp->rulel = rulei + 1;

    memset (&rootp->rules[rulei], 0, sizeof (rule_t));

    VG_warning (0, "RULE INFO", "inserted rule id=%d", rulei);

    return rulei;
}

int removerule (int rulei) {
    rule_t *rulep;
    int eei;

    rulep = &rootp->rules[rulei];
    if (rulep->rc > 0) {
        SUwarning (0, "removerule", "cannot remove rule with active refs");
        return -1;
    }
    for (eei = 0; eei < CE_EXPR_SIZE; eei++) {
        if (!rulep->eeflags[eei])
            continue;
        if (!PMfree (rulep->ees[eei].codep) || !PMfree (rulep->ees[eei].vars)) {
            SUwarning (0, "removerule", "cannot free code and vars");
            return -1;
        }
    }
    if (rulep->vars && !PMfree (rulep->vars)) {
        SUwarning (0, "removerule", "cannot free var array");
        return -1;
    }
    memset (rulep, 0, sizeof (rule_t));

    for (
        ; rootp->rulel > 0 && !rootp->rules[rootp->rulel - 1].id; rootp->rulel--
    )
        ;

    VG_warning (0, "RULE INFO", "removed rule id=%d", rulei);

    return 0;
}

void packrules (void) {
    int rulei, rulej, rulel;

    rulel = rootp->rulel;

    for (rulei = 0; rulei < rootp->rulel; rulei++) {
        if (rootp->rules[rulei].id)
            continue;
        for (rulej = rulei + 1; rulej < rootp->rulel; rulej++) {
            if (rootp->rules[rulej].id && rootp->rules[rulej].rc == 0) {
                rootp->rules[rulei] = rootp->rules[rulej];
                memset (&rootp->rules[rulej], 0, sizeof (rule_t));
                VG_warning (
                    0, "RULE INFO", "moved rule from id=%d to %d", rulej, rulei
                );
                break;
            }
        }
    }

    for (
        ; rootp->rulel > 0 && !rootp->rules[rootp->rulel - 1].id; rootp->rulel--
    )
        ;

    if (rulel != rootp->rulel)
        VG_warning (
            0, "RULE INFO", "packed rules from %d to %d", rulel, rootp->rulel
        );
}

void printrule (int rulei, int lod) {
    rule_t *rulep;
    int eei;

    if (lod <= 2)
        return;

    rulep = &rootp->rules[rulei];
    if (!rulep->id)
        return;
    sfprintf (sfstderr, "RULE %d {\n", rulei);
    sfprintf (sfstderr, "  ID = %s\n", rulep->id);
    sfprintf (sfstderr, "  DESCRIPTION = %s\n", rulep->descr);
    for (eei = 0; eei < CE_EXPR_SIZE; eei++) {
        if (!rulep->eeflags[eei])
            continue;
        sfprintf (sfstderr, "  EXPR %s {\n", emap[eei]);
        EEprint (&rulep->ees[eei], 4);
        sfprintf (sfstderr, "  }\n");
    }
    sfprintf (sfstderr, "}\n");
}

static int mergevars (rule_t *rulep) {
    int eei, vari, varj, vark;

    vari = 0;
    for (eei = 0; eei < CE_EXPR_SIZE; eei++) {
        if (!rulep->eeflags[eei])
            continue;
        for (varj = 0; varj < rulep->ees[eei].varn; varj++) {
            for (vark = 0; vark < vari; vark++) {
                if (
                    tvars[vark].id == rulep->ees[eei].vars[varj].id &&
                    strcmp (
                        tvars[vark].nam, rulep->ees[eei].vars[varj].nam
                    ) == 0
                )
                    break;
            }
            if (vark == vari) {
                if (vari >= tvarn) {
                    if (!(tvars = vmresize (
                        Vmheap, tvars, (tvarn + 10) * sizeof (var_t), VM_RSCOPY
                    ))) {
                        SUwarning (1, "mergevars", "cannot resize tvar array");
                        return -1;
                    }
                    tvarn += 10;
                }
                tvars[vari].id = rulep->ees[eei].vars[varj].id;
                tvars[vari++].nam = rulep->ees[eei].vars[varj].nam;
            }
        }
    }
    rulep->varn = vari;
    if (!(rulep->vars = PMalloc (sizeof (var_t) * rulep->varn))) {
        SUwarning (0, "mergevars", "cannot allocate var array");
        return -1;
    }
    memset (rulep->vars, 0, sizeof (var_t) * rulep->varn);
    for (vari = 0; vari < rulep->varn; vari++) {
        rulep->vars[vari].id = tvars[vari].id;
        rulep->vars[vari].nam = tvars[vari].nam;
        memset (&rulep->vars[vari].val, 0, sizeof (EEval_t));
    }

    return 0;
}
