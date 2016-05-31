#pragma prototyped

#include <ast.h>
#include <swift.h>
#include <time.h>
#include "pmem.h"
#include "eeval.h"
#include "vgce.h"

static void phelper (void);

int CEinit (
    char *pfile, char *rfile, int newflag, int compactflag,
    int alarmdatasize, int firstcci, char *ccidprefix
) {
    struct stat sb;
    time_t t;
    int i;

    rootp = NULL;
    if (EEinit () == -1) {
        SUwarning (0, "CEinit", "cannot init EE");
        return -1;
    }
    if (PMinit (pfile, 5 * 1024 * 1024, phelper, newflag, compactflag) == -1) {
        SUwarning (0, "CEinit", "cannot init PM");
        return -1;
    }
    if (!(rootp = PMgetanchor ())) {
        if (!(rootp = PMalloc (sizeof (root_t)))) {
            SUwarning (0, "CEinit", "cannot allocate root");
            return -1;
        }
        PMsetanchor (rootp);
        memset (rootp, 0, sizeof (root_t));

        rootp->mark = 1;
        rootp->rules = NULL;
        rootp->rulel = rootp->rulen = 0;
        if (!(rootp->ccs = PMalloc (sizeof (cc_t) * 64 * 1024))) {
            SUwarning (0, "CEinit", "cannot allocate cc list");
            return -1;
        }
        memset (&rootp->ccs[0], 0, sizeof (cc_t) * 64 * 1024);
        rootp->ccn = 64 * 1024;
        rootp->ccm = 0;
        if (firstcci <= -1 || firstcci >= rootp->ccn) {
            if (firstcci != -1)
                SUwarning (0, "CEinit", "firstcci out of bounds");
            rootp->ccl = 0;
        } else
            rootp->ccl = firstcci;
        if (strlen (ccidprefix) >= VG_alarm_ccid_L - 7) {
            SUwarning (0, "CEinit", "ccid prefix too long");
            ccidprefix = "CCID";
        }
        strcpy (rootp->ccidprefix, ccidprefix);
        rootp->alarmdatasize = alarmdatasize;
        rootp->alarmn = 0;
        rootp->alarmsn = 0;
        rootp->currtime = 0;
    } else {
        if (strcmp (rootp->ccidprefix, ccidprefix) != 0) {
            if (strlen (ccidprefix) >= VG_alarm_ccid_L - 7) {
                SUwarning (0, "CEinit", "ccid prefix too long");
                ccidprefix = "CCID";
            }
            memset (rootp->ccidprefix, 0, VG_alarm_ccid_L);
            strcpy (rootp->ccidprefix, ccidprefix);
        }
    }
    if (initrules () == -1) {
        SUwarning (0, "CEinit", "cannot init rules");
        return -1;
    }
    for (t = time (NULL), i = 0; i < 30; i++) {
        if (stat (rfile, &sb) == -1) {
            SUwarning (0, "CEinit", "cannot stat rule file");
            continue;
        }
        if (
            sb.st_mtime != 0 &&
            sb.st_mtime == rootp->sb.st_mtime && sb.st_size == rootp->sb.st_size
        )
            break;
        if (t - sb.st_mtime < 5) {
            SUwarning (1, "CEinit", "waiting for rules file to stabilize");
            sleep (1);
            t++;
            continue;
        }
        if (loadrules (rfile) == -1) {
            SUwarning (0, "CEinit", "cannot read rules");
            return -1;
        }
        rootp->sb = sb;
        break;
    }
    rootp->mark++;
    if (rootp->mark == INT_MAX)
        rootp->mark = 1;

    return 0;
}

int CEterm (char *pfile) {
    if (termrules () == -1) {
        SUwarning (0, "CEterm", "cannot term rules");
        return -1;
    }
    if (PMterm (pfile) == -1) {
        SUwarning (0, "CEterm", "cannot term PM");
        return -1;
    }
    if (EEterm () == -1) {
        SUwarning (0, "CEterm", "cannot term EE");
        return -1;
    }
    return 0;
}

static void phelper (void) {
    rule_t *rulep;
    int rulei;
    cc_t *ccp;
    int cci;
    match_t *matchp;
    int matchi;
    alarm_t *alarmp;
    int alarmi;
    var_t *varp;
    int vari;
    int vali;
    int eei;

    PMptrwrite (&rootp->rules);
    for (rulei = 0; rulei < rootp->rulel; rulei++) {
        rulep = &rootp->rules[rulei];
        if (!rulep->id)
            continue;
        PMptrwrite (&rulep->id);
        PMptrwrite (&rulep->descr);
        for (eei = 0; eei < CE_EXPR_SIZE; eei++) {
            if (!rulep->eeflags[eei])
                continue;
            PMptrwrite (&rulep->ees[eei].codep);
            PMptrwrite (&rulep->ees[eei].vars);
            for (vari = 0; vari < rulep->ees[eei].varn; vari++)
                PMptrwrite (&rulep->ees[eei].vars[vari].nam);
            PMptrwrite (&rulep->ees[eei].vals);
            for (vali = 0; vali < rulep->ees[eei].valn; vali++)
                if (EE_VAL_TYPE_ISSTRING (rulep->ees[eei].vals[vali]))
                    PMptrwrite (&rulep->ees[eei].vals[vali].u.s);
        }
        PMptrwrite (&rulep->vars);
        for (vari = 0; vari < rulep->varn; vari++) {
            varp = &rulep->vars[vari];
            PMptrwrite (&varp->nam);
            if (EE_VAL_TYPE_ISSTRING (varp->val) && varp->val.u.s)
                PMptrwrite (&varp->val.u.s);
        }
    }

    PMptrwrite (&rootp->ccs);
    for (cci = 0; cci < rootp->ccn; cci++) {
        ccp = &rootp->ccs[cci];
        if (!ccp->inuse)
            continue;
        PMptrwrite (&ccp->matchs);
        for (matchi = 0; matchi < ccp->matchn; matchi++) {
            matchp = &ccp->matchs[matchi];
            if (!matchp->inuse)
                continue;
            PMptrwrite (&matchp->alarms);
            for (alarmi = 0; alarmi < matchp->alarml; alarmi++) {
                alarmp = matchp->alarms[alarmi];
                PMptrwrite (&matchp->alarms[alarmi]);
                if (alarmp->mark == rootp->mark)
                    continue;
                alarmp->mark = rootp->mark;
                PMptrwrite (&alarmp->vars);
                for (vari = 0; vari < alarmp->varn; vari++) {
                    varp = &alarmp->vars[vari];
                    PMptrwrite (&varp->nam);
                    if (EE_VAL_TYPE_ISSTRING (varp->val) && varp->val.u.s)
                        PMptrwrite (&varp->val.u.s);
                }
            }
            PMptrwrite (&matchp->vars);
            for (vari = 0; vari < matchp->varn; vari++) {
                varp = &matchp->vars[vari];
                PMptrwrite (&varp->nam);
                if (EE_VAL_TYPE_ISSTRING (varp->val) && varp->val.u.s)
                    PMptrwrite (&varp->val.u.s);
            }
        }
    }
}
