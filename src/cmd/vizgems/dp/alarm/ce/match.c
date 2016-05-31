#pragma prototyped

#include <ast.h>
#include <swift.h>
#include "pmem.h"
#include "eeval.h"
#include "vgce.h"

static int getvar (EE_t *, int, EEval_t *, void *);
static int setvar (EE_t *, int, EEval_t *, void *);

int insertmatches (VG_alarm_t *adatap) {
    alarm_t *alarmp;
    context_t context;
    cc_t *ccp;
    int cci, ccj;
    match_t match, *matchp;
    int matchi;
    rule_t *rulep;
    int rulei;
    var_t *varp;
    int vari;
    int res, fn, sn, mn;

    fn = sn = mn = 0;
    memset (adatap->s_ccid, 0, VG_alarm_ccid_L);

    if (!(alarmp = insertalarm (adatap))) {
        SUwarning (0, "insertmatches", "failed to insert alarm");
        return -1;
    }

    memset (&match, 0, sizeof (match_t));
    context.matchp = &match;
    context.alarmp = alarmp;
    context.eei = CE_EXPR_SMATCH;
    context.peval = TRUE;

    for (rulei = 0; rulei < rootp->rulel; rulei++) {
        rulep = &rootp->rules[rulei];
        if (!rulep->id)
            continue;
        if (!(rulep->vres = alarmhasvars (alarmp, rulep->vars, rulep->varn)))
            continue;
        match.rulei = rulei;
        match.vars = rulep->vars;
        match.varn = rulep->varn;
        if (runee (&context, &rulep->sres) == -1) {
            SUwarning (0, "insertmatches", "cannot eval partial SMATCH");
            return -1;
        }
        sn += ((rulep->sres == CE_VAL_TRUE) ? 1 : 0);
        for (vari = 0; vari < rulep->varn; vari++) {
            varp = &rulep->vars[vari];
            if (EE_VAL_TYPE_ISSTRING (varp->val) && varp->val.u.s && !PMfree (
                varp->val.u.s
            )) {
                SUwarning (0, "insertmatches", "cannot free string value");
                return -1;
            }
            memset (&varp->val, 0, sizeof (EEval_t));
        }
    }

    context.alarmp = alarmp;
    context.peval = FALSE;

    for (cci = 0, ccj = -1; cci < rootp->ccn; cci++) {
        ccp = &rootp->ccs[cci];
        if (!ccp->inuse)
            continue;
        for (matchi = 0; matchi < ccp->matchn; matchi++) {
            matchp = &ccp->matchs[matchi];
            if (!matchp->inuse)
                continue;
            rulep = &rootp->rules[matchp->rulei];
            if (rulep->sres != CE_VAL_TRUE)
                continue;
            context.matchp = matchp;
            context.eei = CE_EXPR_SMATCH;
            if (runee (&context, &res) == -1) {
                SUwarning (0, "insertmatches", "cannot eval full SMATCH");
                return -1;
            }
            if (res != CE_VAL_TRUE)
                continue;
            if (insertmatchalarm (cci, matchp, alarmp) == -1) {
                SUwarning (0, "insertmatches", "cannot insert match alarm");
                return -1;
            }
            if (alarmp->data.s_ccid[0] == 0) {
                ccj = cci;
                sfsprintf (
                    alarmp->data.s_ccid, VG_alarm_ccid_L,
                    "%s.%d", rootp->ccidprefix, cci
                );
                strcpy (adatap->s_ccid, alarmp->data.s_ccid);
            }
            context.eei = CE_EXPR_INSERT;
            if (runee (&context, NULL) == -1) {
                SUwarning (0, "insertmatches", "cannot eval INSERT");
                return -1;
            }
            rulep->sres = CE_VAL_TRUE + 1;
            mn++;
        }
    }

    memset (&match, 0, sizeof (match_t));
    context.matchp = &match;
    context.alarmp = alarmp;
    context.eei = CE_EXPR_FMATCH;
    context.peval = FALSE;

    for (rulei = 0; rulei < rootp->rulel; rulei++) {
        rulep = &rootp->rules[rulei];
        if (!rulep->id)
            continue;
        if (rulep->sres == CE_VAL_TRUE + 1)
            continue;
        if (!rulep->vres)
            continue;
        match.rulei = rulei;
        match.vars = rulep->vars;
        match.varn = rulep->varn;
        if (runee (&context, &rulep->fres) == -1) {
            SUwarning (0, "insertmatches", "cannot eval full FMATCH");
            return -1;
        }
        fn += ((rulep->fres == CE_VAL_TRUE) ? 1 : 0);
    }

    context.alarmp = alarmp;
    context.eei = CE_EXPR_INSERT;
    context.peval = FALSE;

    if (fn > 0) {
        if ((cci = ccj) == -1)
            if ((cci = insertcc ()) == -1) {
                SUwarning (0, "insertmatches", "cannot insert cc");
                return -1;
            }
        for (rulei = 0; rulei < rootp->rulel; rulei++) {
            rulep = &rootp->rules[rulei];
            if (!rulep->id)
                continue;
            if (rulep->fres != CE_VAL_TRUE)
                continue;
            if (!(matchp = insertmatch (cci, rulei))) {
                SUwarning (0, "insertmatches", "cannot insert match");
                return -1;
            }
            if (insertmatchalarm (cci, matchp, alarmp) == -1) {
                SUwarning (0, "insertmatches", "cannot insert match alarm");
                return -1;
            }
            if (alarmp->data.s_ccid[0] == 0) {
                sfsprintf (
                    alarmp->data.s_ccid, VG_alarm_ccid_L,
                    "%s.%d", rootp->ccidprefix, cci
                );
                strcpy (adatap->s_ccid, alarmp->data.s_ccid);
            }
            context.matchp = matchp;
            if (runee (&context, NULL) == -1) {
                SUwarning (0, "insertmatches", "cannot eval INSERT");
                return -1;
            }
            mn++;
        }
    }

    for (rulei = 0; rulei < rootp->rulel; rulei++) {
        rulep = &rootp->rules[rulei];
        if (!rulep->id)
            continue;
        rulep->fres = 0;
        rulep->sres = 0;
        rulep->vres = 0;
        for (vari = 0; vari < rulep->varn; vari++) {
            varp = &rulep->vars[vari];
            if (EE_VAL_TYPE_ISSTRING (varp->val) && varp->val.u.s && !PMfree (
                varp->val.u.s
            )) {
                SUwarning (0, "insertmatches", "cannot free string value");
                return -1;
            }
            memset (&varp->val, 0, sizeof (EEval_t));
        }
    }

    VG_warning (
        0, "DATA INFO", "alarm sn=%d rules=%d/%d matches=%d",
        alarmp->sn, fn, sn, mn
    );

    if (mn == 0 && removealarm (alarmp) == -1) {
        SUwarning (0, "insertmatches", "failed to remove alarm");
        return -1;
    }

    return mn; /* number of additions */
}

int removematchesbytime (void) {
    context_t context;
    cc_t *ccp;
    int cci;
    match_t *matchp;
    int matchi;
    alarm_t *alarmp;
    int alarmi;
    EEval_t v;
    int res;

    for (cci = 0; cci < rootp->ccn; cci++) {
        ccp = &rootp->ccs[cci];
        if (!ccp->inuse)
            continue;
        for (matchi = 0; matchi < ccp->matchn; matchi++) {
            matchp = &ccp->matchs[matchi];
            if (!matchp->inuse)
                continue;
            context.alarmp = NULL;
            context.eei = CE_EXPR_PRUNE;
            context.peval = FALSE;
            context.matchp = matchp;
            v.type = EE_VAL_TYPE_INTEGER, v.u.i = 0;
            if (setmatchvar (
                matchp, CE_VAR_MATCH_PTIME, varmap[CE_VAR_MATCH_PTIME],
                &v, &context
            ) == -1) {
                SUwarning (
                    0, "removematchesbytime", "cannot reset match ptime"
                );
                return -1;
            }
            if (runee (&context, &res) == -1) {
                SUwarning (0, "removematchesbytime", "cannot eval PRUNE");
                return -1;
            }
            if (res == CE_VAL_PRUNEALARM) {
                if (getmatchvar (
                    matchp, CE_VAR_MATCH_PTIME, varmap[CE_VAR_MATCH_PTIME],
                    &v, &context
                ) == -1) {
                    SUwarning (
                        0, "removematchesbytime", "cannot get match ptime"
                    );
                    return -1;
                }
                if (v.type != EE_VAL_TYPE_INTEGER || v.u.i <= 0) {
                    SUwarning (
                        0, "removematchesbytime", "bad fatime %d", v.u.i
                    );
                    continue;
                }
                if (v.u.i <= matchp->fatime)
                    continue;
            } else if (res == CE_VAL_PRUNEMATCH)
                v.u.i = INT_MAX;
            else
                continue;

            matchp->fatime = INT_MAX;
            matchp->latime = 0;
            for (alarmi = matchp->alarml - 1; alarmi >= 0; alarmi--) {
                alarmp = matchp->alarms[alarmi];
                if (alarmp->data.s_timeissued >= v.u.i) {
                    if (matchp->fatime > alarmp->data.s_timeissued)
                        matchp->fatime = alarmp->data.s_timeissued;
                    if (matchp->latime < alarmp->data.s_timeissued)
                        matchp->latime = alarmp->data.s_timeissued;
                    continue;
                }
                context.matchp = matchp;
                context.alarmp = alarmp;
                context.eei = CE_EXPR_REMOVE;
                context.peval = FALSE;
                if (runee (&context, NULL) == -1) {
                    SUwarning (0, "removematchesbytime", "cannot eval REMOVE");
                    return -1;
                }
                if (removematchalarm (cci, matchp, alarmi) == -1) {
                    SUwarning (
                        0, "removematchesbytime", "cannot remove match alarm"
                    );
                    return -1;
                }
                if (alarmp->rc > 0)
                    continue;
                if (removealarm (alarmp) == -1) {
                    SUwarning (0, "removematchesbytime", "cannot remove alarm");
                    return -1;
                }
            }
            if (matchp->alarml == 0 && removematch (cci, matchp) == -1) {
                SUwarning (0, "removematchesbytime", "cannot remove match");
                return -1;
            }
        }
        if (ccp->matchm == 0 && removecc (cci) == -1) {
            SUwarning (0, "insertmatches", "cannot remove cc");
            return -1;
        }
    }

    return 0;
}

int removematchesbyrule (int rulei) {
    context_t context;
    cc_t *ccp;
    int cci;
    match_t *matchp;
    int matchi;
    alarm_t *alarmp;
    int alarmi;

    for (cci = 0; cci < rootp->ccn; cci++) {
        ccp = &rootp->ccs[cci];
        if (!ccp->inuse)
            continue;
        for (matchi = 0; matchi < ccp->matchn; matchi++) {
            matchp = &ccp->matchs[matchi];
            if (!matchp->inuse || matchp->rulei != rulei)
                continue;

            matchp->fatime = INT_MAX;
            matchp->latime = 0;
            for (alarmi = matchp->alarml - 1; alarmi >= 0; alarmi--) {
                alarmp = matchp->alarms[alarmi];
                context.matchp = matchp;
                context.alarmp = alarmp;
                context.eei = CE_EXPR_REMOVE;
                context.peval = FALSE;
                if (runee (&context, NULL) == -1) {
                    SUwarning (0, "removematchesbyrule", "cannot eval REMOVE");
                    return -1;
                }
                if (removematchalarm (cci, matchp, alarmi) == -1) {
                    SUwarning (
                        0, "removematchesbyrule", "cannot remove match alarm"
                    );
                    return -1;
                }
                if (alarmp->rc > 0)
                    continue;
                if (removealarm (alarmp) == -1) {
                    SUwarning (0, "removematchesbyrule", "cannot remove alarm");
                    return -1;
                }
            }
            if (matchp->alarml == 0 && removematch (cci, matchp) == -1) {
                SUwarning (0, "removematchesbyrule", "cannot remove match");
                return -1;
            }
        }
        if (ccp->matchm == 0 && removecc (cci) == -1) {
            SUwarning (0, "insertmatches", "cannot remove cc");
            return -1;
        }
    }

    return 0;
}

int removematchesbyclear (VG_alarm_t *adatap, char mode) {
    char ccidstr[100], *s1;
    char *lvl, *obj, *aid;
    int tim;
    char *ccidprefix, *cciddot;
    int ccidid;
    context_t context;
    cc_t *ccp;
    int cci;
    match_t *matchp;
    int matchi;
    alarm_t *alarmp;
    int alarmi;

    lvl = obj = aid = "", tim = -123456, ccidprefix = NULL, ccidid = -123456;
    switch (mode) {
    case 'a':
        lvl = adatap->s_level1;
        obj = adatap->s_id1;
        tim = adatap->s_timecleared;
        break;
    case 'i':
        lvl = adatap->s_level1;
        obj = adatap->s_id1;
        aid = adatap->s_alarmid;
        break;
    case 'c':
        strcpy (ccidstr, adatap->s_ccid);
        ccidprefix = ccidstr;
        if (!(cciddot = strrchr (ccidstr, '.'))) {
            SUwarning (0, "removematchesbyclear", "cannot parse: %s", ccidstr);
            return 0;
        }
        *cciddot++ = 0;
        ccidid = strtol (cciddot, &s1, 10);
        if (s1 && *s1) {
            SUwarning (0, "removematchesbyclear", "cannot parse: %s", cciddot);
            return 0;
        }
        break;
    case 'o':
        lvl = adatap->s_level1;
        obj = adatap->s_id1;
        break;
    case 'A':
        break;
    }

    if (ccidprefix && strcmp (rootp->ccidprefix, ccidprefix) != 0)
        return 0;

    for (cci = 0; cci < rootp->ccn; cci++) {
        ccp = &rootp->ccs[cci];
        if (!ccp->inuse)
            continue;
        if (ccidid != -123456 && cci != ccidid)
            continue;
        for (matchi = 0; matchi < ccp->matchn; matchi++) {
            matchp = &ccp->matchs[matchi];
            if (!matchp->inuse)
                continue;
            matchp->fatime = INT_MAX;
            matchp->latime = 0;
            for (alarmi = matchp->alarml - 1; alarmi >= 0; alarmi--) {
                alarmp = matchp->alarms[alarmi];
                if ((
                    tim != -123456 && alarmp->data.s_timeissued != tim
                ) || (
                    obj[0] && strcmp (alarmp->data.s_id1, obj) != 0
                ) || (
                    aid[0] && strcmp (alarmp->data.s_alarmid, aid) != 0
                ) || (
                    lvl[0] && strcmp (alarmp->data.s_level1, lvl) != 0
                )) {
                    if (matchp->fatime > alarmp->data.s_timeissued)
                        matchp->fatime = alarmp->data.s_timeissued;
                    if (matchp->latime < alarmp->data.s_timeissued)
                        matchp->latime = alarmp->data.s_timeissued;
                    continue;
                }
                context.matchp = matchp;
                context.alarmp = alarmp;
                context.eei = CE_EXPR_REMOVE;
                context.peval = FALSE;
                if (runee (&context, NULL) == -1) {
                    SUwarning (0, "removematchesbyclear", "cannot eval REMOVE");
                    return -1;
                }
                if (removematchalarm (cci, matchp, alarmi) == -1) {
                    SUwarning (
                        0, "removematchesbyclear", "cannot remove match alarm"
                    );
                    return -1;
                }
                if (alarmp->rc > 0)
                    continue;
                if (removealarm (alarmp) == -1) {
                    SUwarning (
                        0, "removematchesbyclear", "cannot remove alarm"
                    );
                    return -1;
                }
            }
            if (matchp->alarml == 0 && removematch (cci, matchp) == -1) {
                SUwarning (0, "removematchesbyclear", "cannot remove match");
                return -1;
            }
        }
        if (ccp->matchm == 0 && removecc (cci) == -1) {
            SUwarning (0, "removematchesbyclear", "cannot remove cc");
            return -1;
        }
    }

    return 0;
}

match_t *insertmatch (int cci, int rulei) {
    match_t *matchp;
    rule_t *rulep;
    int vari;

    if (!(matchp = insertccmatch (cci))) {
        SUwarning (0, "insertmatch", "cannot insert cc match");
        return NULL;
    }

    matchp->rulei = rulei;
    rulep = &rootp->rules[rulei];
    rulep->rc++;

    if (!(matchp->alarms = PMalloc (sizeof (alarm_t *) * CE_MATCH_ALARMINCR))) {
        SUwarning (0, "insertmatch", "cannot allocate alarm array");
        return NULL;
    }
    memset (&matchp->alarms[0], 0, sizeof (alarm_t *) * CE_MATCH_ALARMINCR);
    matchp->alarmn = CE_MATCH_ALARMINCR;
    matchp->alarml = 0;

    if (!(matchp->vars = PMalloc (sizeof (var_t) * rulep->varn))) {
        SUwarning (0, "insertmatch", "cannot allocate var array");
        return NULL;
    }
    memset (&matchp->vars[0], 0, sizeof (var_t) * rulep->varn);
    matchp->varn = rulep->varn;
    for (vari = 0; vari < rulep->varn; vari++) {
        matchp->vars[vari] = rulep->vars[vari];
        if (EE_VAL_TYPE_ISSTRING (matchp->vars[vari].val) && !(
            matchp->vars[vari].val.u.s = PMstrdup (matchp->vars[vari].val.u.s)
        )) {
            SUwarning (0, "insertmatch", "cannot copy string value");
            return NULL;
        }
    }

    matchp->ttime = 0;
    matchp->fatime = INT_MAX;
    matchp->latime = 0;

    VG_warning (
        0, "DATA INFO", "inserted match cc=%d rule=%s", cci, rulep->id
    );

    return matchp;
}

int removematch (int cci, match_t *matchp) {
    rule_t *rulep;
    var_t *varp;
    int vari;

    for (vari = 0; vari < matchp->varn; vari++) {
        varp = &matchp->vars[vari];
        if (EE_VAL_TYPE_ISSTRING (varp->val) && varp->val.u.s && !PMfree (
            varp->val.u.s
        )) {
            SUwarning (0, "removematch", "cannot free string value");
            return -1;
        }
        memset (&varp->val, 0, sizeof (EEval_t));
    }
    if (!PMfree (matchp->vars)) {
        SUwarning (0, "removematch", "cannot free var array");
        return -1;
    }

    if (!PMfree (matchp->alarms)) {
        SUwarning (0, "removematch", "cannot free alarm array");
        return -1;
    }

    rulep = &rootp->rules[matchp->rulei];
    rulep->rc--;

    if (removeccmatch (cci, matchp) == -1) {
        SUwarning (0, "removematch", "cannot remove cc match");
        return -1;
    }

    VG_warning (
        0, "DATA INFO", "removed match cc=%d rule=%s", cci, rulep->id
    );

    return 0;
}

int insertmatchalarm (int cci, match_t *matchp, alarm_t *alarmp) {
    if (matchp->alarml == matchp->alarmn) {
        if (!(matchp->alarms = PMresize (
            matchp->alarms,
            sizeof (alarm_t *) * (matchp->alarmn + CE_MATCH_ALARMINCR)
        ))) {
            SUwarning (0, "insertmatchalarm", "cannot grow alarm array");
            return -1;
        }
        memset (
            &matchp->alarms[matchp->alarmn], 0,
            sizeof (alarm_t *) * CE_MATCH_ALARMINCR
        );
        matchp->alarmn += CE_MATCH_ALARMINCR;
    }
    matchp->alarms[matchp->alarml++] = alarmp;
    if (matchp->fatime > alarmp->data.s_timeissued)
        matchp->fatime = alarmp->data.s_timeissued;
    if (matchp->latime < alarmp->data.s_timeissued)
        matchp->latime = alarmp->data.s_timeissued;
    if (matchp->maxtmode < VG_ALARM_N_MODE_MAX - alarmp->data.s_tmode)
        matchp->maxtmode = VG_ALARM_N_MODE_MAX - alarmp->data.s_tmode;
    alarmp->rc++;

    VG_warning (
        0, "DATA INFO", "inserted match alarm sn=%d cc=%d rule=%s",
        alarmp->sn, cci, rootp->rules[matchp->rulei].id
    );

    return 0;
}

int removematchalarm (int cci, match_t *matchp, int alarmi) {
    int sn;

    sn = matchp->alarms[alarmi]->sn;
    matchp->alarms[alarmi]->rc--;
    matchp->alarms[alarmi] = matchp->alarms[--matchp->alarml];
    matchp->alarms[matchp->alarml] = NULL;

    VG_warning (
        0, "DATA INFO", "removed match alarm sn=%d cc=%d rule=%s",
        sn, cci, rootp->rules[matchp->rulei].id
    );

    return 0;
}

#define FINDVAR { \
    if (id == -1) { \
        for (vari = 0; vari < matchp->varn; vari++) \
            if (strcmp (matchp->vars[vari].nam, np) == 0) \
                break; \
    } else { \
        for (vari = 0; vari < matchp->varn; vari++) \
            if (matchp->vars[vari].id == id) \
                break; \
    } \
    if (vari == matchp->varn) { \
        SUwarning ( \
            0, "findmatchvar", "cannot find var (%d/%s) in array", id, np \
        ); \
        return -1; \
    } \
    varp = &matchp->vars[vari]; \
}

int getmatchvar (match_t *matchp, int id, char *np, EEval_t *vp, void *vcxtp) {
    context_t *cxtp;

    var_t *varp;
    int vari;

    cxtp = vcxtp;
    switch (id) {
    case CE_VAR_MATCH_FALSE:
        vp->type = EE_VAL_TYPE_INTEGER, vp->u.i = CE_VAL_FALSE;
        return 0;
    case CE_VAR_MATCH_TRUE:
        vp->type = EE_VAL_TYPE_INTEGER, vp->u.i = CE_VAL_TRUE;
        return 0;
    case CE_VAR_MATCH_PRUNEMATCH:
        vp->type = EE_VAL_TYPE_INTEGER, vp->u.i = CE_VAL_PRUNEMATCH;
        return 0;
    case CE_VAR_MATCH_PRUNEALARM:
        vp->type = EE_VAL_TYPE_INTEGER, vp->u.i = CE_VAL_PRUNEALARM;
        return 0;
    case CE_VAR_MATCH_RESULT:
        FINDVAR;
        *vp = varp->val;
        return 0;
    case CE_VAR_MATCH_CTIME:
        vp->type = EE_VAL_TYPE_INTEGER, vp->u.i = rootp->currtime;
        return 0;
    case CE_VAR_MATCH_TTIME:
        vp->type = EE_VAL_TYPE_INTEGER, vp->u.i = matchp->ttime;
        return 0;
    case CE_VAR_MATCH_PTIME:
        FINDVAR;
        *vp = varp->val;
        return 0;
    case CE_VAR_MATCH_FATIME:
        if (cxtp->peval) {
            vp->type = EE_VAL_TYPE_NULL, vp->u.i = 0;
            return 0;
        }
        vp->type = EE_VAL_TYPE_INTEGER, vp->u.i = matchp->fatime;
        return 0;
    case CE_VAR_MATCH_LATIME:
        if (cxtp->peval) {
            vp->type = EE_VAL_TYPE_NULL, vp->u.i = 0;
            return 0;
        }
        vp->type = EE_VAL_TYPE_INTEGER, vp->u.i = matchp->latime;
        return 0;
    case CE_VAR_MATCH_FIELDS:
        FINDVAR;
        *vp = varp->val;
        return 0;
    default:
        if (cxtp->peval) {
            vp->type = EE_VAL_TYPE_NULL, vp->u.i = 0;
            return 0;
        }
        FINDVAR;
        *vp = varp->val;
        return 0;
    }
    return -1;
}

int setmatchvar (match_t *matchp, int id, char *np, EEval_t *vp, void *vcxtp) {
    var_t *varp;
    int vari;

    switch (id) {
    case CE_VAR_MATCH_FALSE:
    case CE_VAR_MATCH_TRUE:
    case CE_VAR_MATCH_PRUNEMATCH:
    case CE_VAR_MATCH_PRUNEALARM:
        SUwarning (0, "setmatchvar", "attempt to set read-only var %s", np);
        return 0;
    case CE_VAR_MATCH_RESULT:
        FINDVAR;
        varp->val = *vp;
        return 0;
    case CE_VAR_MATCH_CTIME:
    case CE_VAR_MATCH_TTIME:
    case CE_VAR_MATCH_FATIME:
    case CE_VAR_MATCH_LATIME:
        SUwarning (0, "setmatchvar", "attempt to set read-only var %s", np);
        return 0;
    case CE_VAR_MATCH_PTIME:
        FINDVAR;
        varp->val = *vp;
        return 0;
    case CE_VAR_MATCH_FIELDS:
        FINDVAR;
        if (EE_VAL_TYPE_ISSTRING (varp->val) && varp->val.u.s && !PMfree (
            varp->val.u.s
        )) {
            SUwarning (0, "setmatchvar", "cannot free string value");
            systemerror = TRUE;
            return -1;
        }
        varp->val = *vp;
        if (EE_VAL_TYPE_ISSTRING (varp->val) && !(
            varp->val.u.s = PMstrdup (varp->val.u.s)
        )) {
            SUwarning (0, "setmatchvar", "cannot copy string value");
            systemerror = TRUE;
            return -1;
        }
        return 0;
    default:
        FINDVAR;
        if (EE_VAL_TYPE_ISSTRING (varp->val) && varp->val.u.s && !PMfree (
            varp->val.u.s
        )) {
            SUwarning (0, "setmatchvar", "cannot free string value");
            systemerror = TRUE;
            return -1;
        }
        varp->val = *vp;
        if (EE_VAL_TYPE_ISSTRING (varp->val) && !(
            varp->val.u.s = PMstrdup (varp->val.u.s)
        )) {
            SUwarning (0, "setmatchvar", "cannot copy string value");
            systemerror = TRUE;
            return -1;
        }
        return 0;
    }
    return -1;
}

int runee (context_t *cxtp, int *res) {
    rule_t *rulep;
    EEval_t v;

    rulep = &rootp->rules[cxtp->matchp->rulei];
    if (!rulep->eeflags[cxtp->eei])
        return FALSE;
    if (res) {
        v.type = EE_VAL_TYPE_INTEGER, v.u.i = 0;
        if (setmatchvar (
            cxtp->matchp, CE_VAR_MATCH_RESULT, varmap[CE_VAR_MATCH_RESULT],
            &v, cxtp
        ) == -1) {
            SUwarning (0, "runee", "cannot reset result value");
            return -1;
        }
    }
    switch (EEeval (
        &rulep->ees[cxtp->eei], getvar, setvar, cxtp, cxtp->peval
    )) {
    case -1:
        SUwarning (0, "runee", "evaluation system error");
        return -1;
    case 1:
        SUwarning (0, "runee", "evaluation failed");
        break;
    }
    if (res) {
        if (getmatchvar (
            cxtp->matchp, CE_VAR_MATCH_RESULT, varmap[CE_VAR_MATCH_RESULT],
            &v, cxtp
        ) == -1) {
            SUwarning (0, "runee", "cannot get result value");
            return -1;
        }
        *res = 0;
        if (v.type == EE_VAL_TYPE_INTEGER)
            *res = v.u.i;
    }
    return 0;
}

void printmatch (match_t *matchp, int lod) {
    rule_t *rulep;
    var_t *varp;
    int vari;
    alarm_t *alarmp;
    int alarmi;

    if (lod <= 1)
        return;

    rulep = &rootp->rules[matchp->rulei];

    sfprintf (
        sfstderr, "  match: rule=%s alarmn=%d/%d (%d-%d) varn=%d\n",
        rulep->id, matchp->alarml, matchp->alarmn,
        matchp->fatime, matchp->latime, matchp->varn
    );

    if (lod <= 2)
        return;

    for (vari = 0; vari < matchp->varn; vari++) {
        varp = &matchp->vars[vari];
        sfprintf (
            sfstderr, "    var: id=%2d name=%-16s", varp->id, varp->nam
        );
        switch (varp->val.type) {
        case EE_VAL_TYPE_NULL:
            sfprintf (sfstderr, " val=NULL\n");
            break;
        case EE_VAL_TYPE_INTEGER:
            sfprintf (sfstderr, " val=INT (%d)\n", varp->val.u.i);
            break;
        case EE_VAL_TYPE_REAL:
            sfprintf (sfstderr, " val=REAL (%f)\n", varp->val.u.f);
            break;
        case EE_VAL_TYPE_CSTRING:
            sfprintf (sfstderr, " val=CSTRING (%s)\n", varp->val.u.s);
            break;
        case EE_VAL_TYPE_RSTRING:
            sfprintf (sfstderr, " val=RSTRING (%s)\n", varp->val.u.s);
            break;
        default:
            if ((
                varp->id >= CE_VAR_ALARM_FIRST &&
                varp->id <= CE_VAR_ALARM_LAST
            ) || strncmp (varp->nam, "_alarm_", 7) == 0)
                sfprintf (sfstderr, " val=ALARMDATA\n");
            else
                sfprintf (sfstderr, " val=UNKNOWN\n");
            break;
        }
    }

    if (lod <= 3)
        return;

    for (alarmi = 0; alarmi < matchp->alarmn; alarmi++) {
        alarmp = matchp->alarms[alarmi];
        if (!alarmp)
            continue;
        sfprintf (sfstderr, "    alarm: i=%d sn=%d\n", alarmi, alarmp->sn);
    }
}

static int getvar (EE_t *eep, int vari, EEval_t *vp, void *vcxtp) {
    rule_t *rulep;
    context_t *cxtp;
    EEvar_t *varp;

    cxtp = vcxtp;
    rulep = &rootp->rules[cxtp->matchp->rulei];
    if (eep != &rulep->ees[cxtp->eei]) {
        SUwarning (0, "getvar", "code pointer missmatch");
        return -1;
    }

    if (vari < 0 || vari >= eep->varn) {
        SUwarning (0, "getvar", "variable id out of range: %d", vari);
        return -1;
    }

    varp = &eep->vars[vari];
    if (varp->id >= CE_VAR_ALARM_FIRST && varp->id <= CE_VAR_ALARM_LAST)
        return getalarmvar (cxtp->alarmp, varp->id, varp->nam, vp, vcxtp);
    else if (varp->id >= CE_VAR_MATCH_FIRST && varp->id <= CE_VAR_MATCH_LAST)
        return getmatchvar (cxtp->matchp, varp->id, varp->nam, vp, vcxtp);

    if (strncmp (varp->nam, "_alarm_", 7) == 0)
        return getalarmvar (cxtp->alarmp, varp->id, varp->nam, vp, vcxtp);
    return getmatchvar (cxtp->matchp, varp->id, varp->nam, vp, vcxtp);
}

static int setvar (EE_t *eep, int vari, EEval_t *vp, void *vcxtp) {
    context_t *cxtp;
    rule_t *rulep;
    EEvar_t *varp;

    cxtp = vcxtp;
    rulep = &rootp->rules[cxtp->matchp->rulei];
    if (eep != &rulep->ees[cxtp->eei]) {
        SUwarning (0, "setvar", "code pointer missmatch");
        return -1;
    }

    if (vari < 0 || vari >= eep->varn) {
        SUwarning (0, "setvar", "variable id out of range: %d", vari);
        return -1;
    }

    varp = &eep->vars[vari];
    if (varp->id >= CE_VAR_ALARM_FIRST && varp->id <= CE_VAR_ALARM_LAST)
        return setalarmvar (cxtp->alarmp, varp->id, varp->nam, vp, vcxtp);
    else if (varp->id >= CE_VAR_MATCH_FIRST && varp->id <= CE_VAR_MATCH_LAST)
        return setmatchvar (cxtp->matchp, varp->id, varp->nam, vp, vcxtp);

    if (strncmp (varp->nam, "_alarm_", 7) == 0)
        return setalarmvar (cxtp->alarmp, varp->id, varp->nam, vp, vcxtp);
    return setmatchvar (cxtp->matchp, varp->id, varp->nam, vp, vcxtp);
}
