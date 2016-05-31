#pragma prototyped

#include <ast.h>
#include <swift.h>
#include "pmem.h"
#include "eeval.h"
#include "vgce.h"

alarm_t *insertalarm (VG_alarm_t *datap) {
    alarm_t *alarmp;
    int vari;
    char *s1, *s2, *s3;

    if (datap->s_severity < 0)
        datap->s_severity = 0;
    if (datap->s_state < VG_ALARM_N_STATE_NONE)
        datap->s_state = VG_ALARM_N_STATE_NONE;
    if (!(alarmp = PMalloc (sizeof (alarm_t)))) {
        SUwarning (0, "insertalarm", "cannot allocate alarm");
        return NULL;
    }
    memset (alarmp, 0, sizeof (alarm_t));
    alarmp->sn = rootp->alarmsn++;
    if (rootp->alarmsn == INT_MAX)
        rootp->alarmsn = 0;
    rootp->alarmn++;
    memcpy (&alarmp->data, datap, sizeof (VG_alarm_t));

    alarmp->vars = NULL;
    alarmp->varn = 0;
    if (alarmp->data.s_variables[0])
        for (s1 = alarmp->data.s_variables; s1; )
            alarmp->varn++, s1 = strstr (s1, "\t\t"), s1 = (s1 ? s1 + 2 : NULL);
    if (!(alarmp->vars = PMalloc (sizeof (var_t) * alarmp->varn))) {
        SUwarning (0, "insertalarm", "cannot allocate var array");
        rootp->alarmn--;
        PMfree (alarmp);
        return NULL;
    }
    memset (alarmp->vars, 0, sizeof (var_t) * alarmp->varn);
    if (alarmp->data.s_variables[0]) {
        for (vari = 0, s1 = alarmp->data.s_variables; s1; vari++) {
            if ((s2 = strstr (s1, "\t\t")))
                *s2 = 0;
            s3 = strchr (s1, '=');
            *s3 = 0;
            alarmp->vars[vari].nam = s1;
            if (strncmp (s1, "num_", 4) == 0) {
                if (strchr (s1, '.')) {
                    alarmp->vars[vari].val.type = EE_VAL_TYPE_REAL;
                    alarmp->vars[vari].val.u.f = atof (s3 + 1);
                } else {
                    alarmp->vars[vari].val.type = EE_VAL_TYPE_INTEGER;
                    alarmp->vars[vari].val.u.i = atoi (s3 + 1);
                }
            } else {
                alarmp->vars[vari].val.type = EE_VAL_TYPE_CSTRING;
                alarmp->vars[vari].val.u.s = s3 + 1;
            }
            if (s2)
                s2 += 2;
            s1 = s2;
        }
    }

    VG_warning (0, "DATA INFO", "inserted alarm sn=%d", alarmp->sn);

    return alarmp;
}

int removealarm (alarm_t *alarmp) {
    int sn;

    if (alarmp->rc > 0) {
        SUwarning (0, "removealarm", "cannot remove alarm with active refs");
        return -1;
    }
    sn = alarmp->sn;
    rootp->alarmn--;

    if (!PMfree (alarmp->vars)) {
        SUwarning (0, "removealarm", "cannot free var array");
        return -1;
    }

    if (!PMfree (alarmp)) {
        SUwarning (0, "removealarm", "cannot free alarm");
        return -1;
    }

    VG_warning (0, "DATA INFO", "removed alarm sn=%d", sn);

    return 0;
}

#define FINDVAR { \
    if (id == -1) { \
        for (vari = 0; vari < alarmp->varn; vari++) \
            if (strcmp (alarmp->vars[vari].nam, np + 7) == 0) \
                break; \
    } else { \
        for (vari = 0; vari < alarmp->varn; vari++) \
            if (alarmp->vars[vari].id == id) \
                break; \
    } \
    if (vari == alarmp->varn) { \
        SUwarning ( \
            0, "findalarmvar", "cannot find var (%d/%s) in array", id, np \
        ); \
        return -1; \
    } \
    varp = &alarmp->vars[vari]; \
}

int getalarmvar (alarm_t *alarmp, int id, char *np, EEval_t *vp, void *vcxtp) {
    context_t *cxtp;
    var_t *varp;
    int vari;

    cxtp = vcxtp;
    if (cxtp->eei >= CE_EXPR_FTICKET && cxtp->eei != CE_EXPR_EMIT) {
        SUwarning (0, "getalarmvar", "illegal alarm var request");
        return -1;
    }

    switch (id) {
    case CE_VAR_ALARM_ALARMID:
        vp->type = EE_VAL_TYPE_CSTRING, vp->u.s = &alarmp->data.s_alarmid[0];
        return 0;
    case CE_VAR_ALARM_LEVEL1:
        vp->type = EE_VAL_TYPE_CSTRING, vp->u.s = &alarmp->data.s_level1[0];
        return 0;
    case CE_VAR_ALARM_OBJECT1:
        vp->type = EE_VAL_TYPE_CSTRING, vp->u.s = &alarmp->data.s_id1[0];
        return 0;
    case CE_VAR_ALARM_LEVEL2:
        vp->type = EE_VAL_TYPE_CSTRING, vp->u.s = &alarmp->data.s_level2[0];
        return 0;
    case CE_VAR_ALARM_OBJECT2:
        vp->type = EE_VAL_TYPE_CSTRING, vp->u.s = &alarmp->data.s_id2[0];
        return 0;
    case CE_VAR_ALARM_TEXT:
        vp->type = EE_VAL_TYPE_CSTRING; vp->u.s = &alarmp->data.s_text[0];
        return 0;
    case CE_VAR_ALARM_COMMENT:
        vp->type = EE_VAL_TYPE_CSTRING, vp->u.s = &alarmp->data.s_comment[0];
        return 0;
    case CE_VAR_ALARM_STATE:
        vp->type = EE_VAL_TYPE_CSTRING;
        vp->u.s = statemap[alarmp->data.s_state];
        return 0;
    case CE_VAR_ALARM_SMODE:
        vp->type = EE_VAL_TYPE_INTEGER, vp->u.i = alarmp->data.s_smode;
        return 0;
    case CE_VAR_ALARM_TMODE:
        vp->type = EE_VAL_TYPE_INTEGER, vp->u.i = alarmp->data.s_tmode;
        return 0;
    case CE_VAR_ALARM_TIME:
        vp->type = EE_VAL_TYPE_INTEGER, vp->u.i = alarmp->data.s_timeissued;
        return 0;
    case CE_VAR_ALARM_SEVERITY:
        vp->type = EE_VAL_TYPE_INTEGER, vp->u.i = alarmp->data.s_severity;
        return 0;
    default:
        FINDVAR;
        *vp = varp->val;
        return 0;
    }
    return -1;
}

int setalarmvar (alarm_t *alarmp, int id, char *np, EEval_t *vp, void *vcxtp) {
    context_t *cxtp;

    cxtp = vcxtp;
    if (cxtp->eei >= CE_EXPR_FTICKET) {
        SUwarning (0, "setalarmvar", "illegal alarm var request");
        return -1;
    }

    switch (id) {
    case CE_VAR_ALARM_ALARMID:
        SUwarning (0, "setalarmvar", "attempt to set read-only var %s", np);
        return 0;
    }
    return -1;
}

int alarmhasvars (alarm_t *alarmp, var_t *vars, int varn) {
    var_t *varp;
    int vari, varj;

    for (vari = 0; vari < varn; vari++) {
        varp = &vars[vari];
        if (varp->id != -1 || strncmp (varp->nam, "_alarm_", 7) != 0)
            continue;
        for (varj = 0; varj < alarmp->varn; varj++)
            if (strcmp (varp->nam + 7, alarmp->vars[varj].nam) == 0)
                break;
        if (varj == alarmp->varn)
            return CE_VAL_FALSE;
    }
    return CE_VAL_TRUE;
}

void printalarms (int lod) {
    if (lod <= 1)
        return;

    sfprintf (sfstderr, "alarms: n=%d\n", rootp->alarmn);
}

void printalarm (alarm_t *alarmp, int lod) {
    var_t *varp;
    int vari;

    if (lod <= 2)
        return;

    if (lod <= 3)
        sfprintf (sfstderr, "alarm: sn=%d rc=%d\n", alarmp->sn, alarmp->rc);
    else
        sfprintf (
            sfstderr, "alarm: sn=%d rc=%d text=%s\n",
            alarmp->sn, alarmp->rc, alarmp->data.s_text
        );

    if (lod <= 3)
        return;

    for (vari = 0; vari < alarmp->varn; vari++) {
        varp = &alarmp->vars[vari];
        sfprintf (
            sfstderr, "  var: id=%2d name=%-16s", varp->id, varp->nam
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
        }
    }
}
