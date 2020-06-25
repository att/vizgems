#pragma prototyped

#include <ast.h>
#include <swift.h>
#include "pmem.h"
#include "eeval.h"
#include "vgce.h"

extern char *currdate;

static Sfio_t *fp3, *fp4;

static int createticket (int, match_t *);
static int updateticket (int, match_t *);
static int emitrecord (context_t *);

int updatetickets (void) {
    cc_t *ccp;
    int cci;
    match_t *matchp;
    int matchi;
    context_t context;
    int res;

    context.alarmp = NULL;
    context.peval = FALSE;

    for (cci = 0; cci < rootp->ccn; cci++) {
        ccp = &rootp->ccs[cci];
        if (!ccp->inuse)
            continue;
        if (!ccp->ticketed) {
            context.eei = CE_EXPR_FTICKET;
            for (matchi = 0; matchi < ccp->matchn; matchi++) {
                matchp = &ccp->matchs[matchi];
                if (!matchp->inuse)
                    continue;
                if (
                    matchp->maxtmode ==
                    VG_ALARM_N_MODE_MAX - VG_ALARM_N_MODE_KEEP
                )
                    res = CE_VAL_TRUE;
                else {
                    context.matchp = matchp;
                    if (runee (&context, &res) == -1) {
                        SUwarning (0, "updatetickets", "cannot eval FTICKET");
                        return -1;
                    }
                }
                if (res == CE_VAL_TRUE) {
                    if (createticket (cci, matchp) == -1) {
                        SUwarning (0, "updatetickets", "cannot create ticket");
                        return -1;
                    }
                    ccp->ticketed = TRUE;
                    ccp->ttime = rootp->currtime;
                    for (matchi = 0; matchi < ccp->matchn; matchi++)
                        if (ccp->matchs[matchi].inuse)
                            ccp->matchs[matchi].ttime = ccp->ttime;
                    break;
                }
            }
        } else {
            context.eei = CE_EXPR_STICKET;
            for (matchi = 0; matchi < ccp->matchn; matchi++) {
                matchp = &ccp->matchs[matchi];
                if (!matchp->inuse)
                    continue;
                context.matchp = matchp;
                if (runee (&context, &res) == -1) {
                    SUwarning (0, "updatetickets", "cannot eval STICKET");
                    return -1;
                }
                if (res == CE_VAL_TRUE) {
                    if (updateticket (cci, matchp) == -1) {
                        SUwarning (0, "updatetickets", "cannot update ticket");
                        return -1;
                    }
                    ccp->ttime = rootp->currtime;
                    for (matchi = 0; matchi < ccp->matchn; matchi++)
                        if (ccp->matchs[matchi].inuse)
                            ccp->matchs[matchi].ttime = ccp->ttime;
                    break;
                }
            }
        }
    }

    return 0;
}

int emitrecords (void) {
    cc_t *ccp;
    int cci;
    match_t *matchp;
    int matchi;
    context_t context;
    int res;

    context.alarmp = NULL;
    context.peval = FALSE;

    for (cci = 0; cci < rootp->ccn; cci++) {
        ccp = &rootp->ccs[cci];
        if (!ccp->inuse)
            continue;
        context.eei = CE_EXPR_EMIT;
        for (matchi = 0; matchi < ccp->matchn; matchi++) {
            matchp = &ccp->matchs[matchi];
            if (!matchp->inuse)
                continue;
            context.matchp = matchp;
            if (runee (&context, &res) == -1) {
                SUwarning (0, "emitrecords", "cannot eval EMIT");
                return 0;
            }
            if (res == CE_VAL_TRUE) {
                if (emitrecord (&context) == -1) {
                    SUwarning (0, "emitrecords", "cannot emit record");
                    return 0;
                }
                break;
            }
        }
    }

    return 0;
}

static int createticket (int cci, match_t *matchp) {
    alarm_t *alarmp;
    int alarmi;
    int maxsev, maxalarmi;
    long maxtime;
    char buf[2][4000], *tid1;
    int anchor;

    maxsev = 999, maxalarmi = -1, maxtime = -1;
    for (alarmi = 0; alarmi < matchp->alarml; alarmi++) {
        alarmp = matchp->alarms[alarmi];
        if (alarmp->data.s_severity < maxsev)
            maxsev = alarmp->data.s_severity, maxalarmi = alarmi;
        else if (
            alarmp->data.s_severity == maxsev &&
            alarmp->data.s_timeissued > maxtime
        )
            maxalarmi = alarmi;
        if (alarmp->data.s_timeissued > maxtime)
            maxtime = alarmp->data.s_timeissued;
    }
    if (maxalarmi == -1) {
        SUwarning (0, "createticket", "cannot find alarm record");
        return -1;
    }
    alarmp = matchp->alarms[maxalarmi];

    if (!fp3 && !(fp3 = sfnew (NULL, NULL, 4096, 3, SF_WRITE))) {
        SUwarning (0, "createticket", "open of fd 3 failed");
        return -1;
    }
    if (!fp4 && !(fp4 = sfnew (NULL, NULL, 4096, 4, SF_WRITE))) {
        SUwarning (0, "createticket", "open of fd 4 failed");
        return -1;
    }
    if (alarmp->data.s_origmsg[0])
        tid1 = alarmp->data.s_origmsg;
    else
        tid1 = alarmp->data.s_id1;
    anchor = (alarmp->data.s_text[0] == '+' || alarmp->data.s_text[0] == '-');
    sfprintf (
        fp4,
        "severity=%s object=%s msg_text=\"%s%s%s%s%s"
        " CORR EVENT\"\n",
        sevmaps[alarmp->data.s_severity].name, tid1,
        !anchor ? "+-USER> MSGKEY=none " : "",
        !anchor ? (
            alarmp->data.s_type == VG_ALARM_N_TYPE_ALARM ? ":DOWN " : ":UP "
        ) : "",
        alarmp->data.s_text, alarmp->data.s_comment[0] ? " COMMENT=" : "",
        alarmp->data.s_comment
    );
    VG_urlenc (alarmp->data.s_text, buf[0], 4000);
    VG_urlenc (alarmp->data.s_comment, buf[1], 4000);
    sfprintf (
        fp3,
        "<alarm>"
        "<v>%s</v><sid>%s</sid><aid>%s</aid>"
        "<ccid>%s</ccid><st>%s</st><sm>%s</sm>"
        "<vars>%s</vars>"
        "<di>%s</di><hi>%s</hi>"
        "<tc>%d</tc><ti>%d</ti>"
        "<tp>%s</tp><so>%s</so><pm>%s</pm>"
        "<lv1>%s</lv1><id1>%s</id1><lv2>%s</lv2><id2>%s</id2>"
        "<tm>%s</tm><sev>%d</sev>"
        "<txt>%s</txt><com>%s</com><origmsg>%s</origmsg>"
        "</alarm>\n",
        VG_S_VERSION, alarmp->data.s_scope, alarmp->data.s_alarmid,
        alarmp->data.s_ccid,
        statemap[alarmp->data.s_state], modemap[alarmp->data.s_smode],
        alarmp->data.s_variables, alarmp->data.s_dateissued,
        alarmp->data.s_hourissued,
        alarmp->data.s_timecleared, (
            alarmp->data.s_timecleared == 0
        ) ? rootp->currtime : alarmp->data.s_timeissued,
        typemap[alarmp->data.s_type], VG_ALARM_S_SO_OVERRIDE,
        pmodemap[VG_ALARM_N_PMODE_PASSTHROUGH],
        alarmp->data.s_level1, alarmp->data.s_id1,
        alarmp->data.s_level2, alarmp->data.s_id2,
        ftmode, alarmp->data.s_severity,
        buf[0], buf[1], alarmp->data.s_origmsg
    );
    sfsync (fp3);
    sfsync (fp4);
    VG_warning (
        0, "DATA INFO", "created ticket cc=%d rule=%s", cci,
        rootp->rules[matchp->rulei].id
    );
    return 0;
}

static int updateticket (int cci, match_t *matchp) {
    alarm_t *alarmp;
    int alarmi;
    int maxsev, maxalarmi;
    long maxtime;
    char buf[2][4000], *tid1;
    int anchor;

    maxsev = 999, maxalarmi = -1, maxtime = -1;
    for (alarmi = 0; alarmi < matchp->alarml; alarmi++) {
        alarmp = matchp->alarms[alarmi];
        if (alarmp->data.s_severity < maxsev)
            maxsev = alarmp->data.s_severity, maxalarmi = alarmi;
        else if (
            alarmp->data.s_severity == maxsev &&
            alarmp->data.s_timeissued > maxtime
        )
            maxalarmi = alarmi;
        if (alarmp->data.s_timeissued > maxtime)
            maxtime = alarmp->data.s_timeissued;
    }
    if (maxalarmi == -1) {
        SUwarning (0, "updateticket", "cannot find alarm record");
        return -1;
    }
    alarmp = matchp->alarms[maxalarmi];

    if (!fp3 && !(fp3 = sfnew (NULL, NULL, 4096, 3, SF_WRITE))) {
        SUwarning (0, "updateticket", "open of fd 3 failed");
        return -1;
    }
    if (!fp4 && !(fp4 = sfnew (NULL, NULL, 4096, 4, SF_WRITE))) {
        SUwarning (0, "updateticket", "open of fd 4 failed");
        return -1;
    }
    if (alarmp->data.s_origmsg[0])
        tid1 = alarmp->data.s_origmsg;
    else
        tid1 = alarmp->data.s_id1;
    anchor = (alarmp->data.s_text[0] == '+' || alarmp->data.s_text[0] == '-');
    sfprintf (
        fp4,
        "severity=%s object=%s msg_text=\"%s%s%s%s%s"
        " CORR EVENT UPDATE\"\n",
        sevmaps[alarmp->data.s_severity].name, tid1,
        !anchor ? "+-USER> MSGKEY=none " : "",
        !anchor ? (
            alarmp->data.s_type == VG_ALARM_N_TYPE_ALARM ? ":DOWN " : ":UP "
        ) : "",
        alarmp->data.s_text, alarmp->data.s_comment[0] ? " COMMENT=" : "",
        alarmp->data.s_comment
    );
    VG_urlenc (alarmp->data.s_text, buf[0], 4000);
    VG_urlenc (alarmp->data.s_comment, buf[1], 4000);
    sfprintf (
        fp3,
        "<alarm>"
        "<v>%s</v><sid>%s</sid><aid>%s</aid>"
        "<ccid>%s</ccid><st>%s</st><sm>%s</sm>"
        "<vars>%s</vars>"
        "<di>%s</di><hi>%s</hi>"
        "<tc>%d</tc><ti>%d</ti>"
        "<tp>%s</tp><so>%s</so><pm>%s</pm>"
        "<lv1>%s</lv1><id1>%s</id1><lv2>%s</lv2><id2>%s</id2>"
        "<tm>%s</tm><sev>%d</sev>"
        "<txt>%s (repeat)</txt><com>%s</com><origmsg>%s</origmsg>"
        "</alarm>\n",
        VG_S_VERSION, alarmp->data.s_scope, alarmp->data.s_alarmid,
        alarmp->data.s_ccid,
        statemap[alarmp->data.s_state], modemap[alarmp->data.s_smode],
        alarmp->data.s_variables, alarmp->data.s_dateissued,
        alarmp->data.s_hourissued,
        alarmp->data.s_timecleared, (
            alarmp->data.s_timecleared == 0
        ) ? rootp->currtime : alarmp->data.s_timeissued,
        typemap[alarmp->data.s_type], VG_ALARM_S_SO_OVERRIDE,
        pmodemap[VG_ALARM_N_PMODE_PASSTHROUGH],
        alarmp->data.s_level1, alarmp->data.s_id1,
        alarmp->data.s_level2, alarmp->data.s_id2,
        rtmode, alarmp->data.s_severity,
        buf[0], buf[1], alarmp->data.s_origmsg
    );
    sfsync (fp3);
    sfsync (fp4);
    VG_warning (
        0, "DATA INFO", "updated ticket cc=%d rule=%s", cci,
        rootp->rules[matchp->rulei].id
    );
    return 0;
}

int simpleticket (VG_alarm_t *datap) {
    char *tid1;
    int anchor;

    if (!fp4 && !(fp4 = sfnew (NULL, NULL, 4096, 4, SF_WRITE))) {
        SUwarning (0, "createticket", "open of fd 4 failed");
        return -1;
    }
    if (datap->s_origmsg[0])
        tid1 = datap->s_origmsg;
    else
        tid1 = datap->s_id1;
    anchor = (datap->s_text[0] == '+' || datap->s_text[0] == '-');
    sfprintf (
        fp4,
        "severity=%s object=%s msg_text=\"%s%s%s%s%s\"\n",
        sevmaps[datap->s_severity].name, tid1,
        !anchor ? "+-USER> MSGKEY=none " : "",
        !anchor ? (
            datap->s_type == VG_ALARM_N_TYPE_ALARM ? ":DOWN " : ":UP "
        ) : "",
        datap->s_text, datap->s_comment[0] ? " COMMENT=" : "",
        datap->s_comment
    );
    sfsync (fp4);
    VG_warning (
        0, "DATA INFO", "created simple ticket text=%s", datap->s_text
    );
    return 0;
}

int closeanyticket (VG_alarm_t *datap) {
    char *tid1;
    int anchor;

    if (!fp4 && !(fp4 = sfnew (NULL, NULL, 4096, 4, SF_WRITE))) {
        SUwarning (0, "closeanyticket", "open of fd 4 failed");
        return -1;
    }
    if (datap->s_origmsg[0])
        tid1 = datap->s_origmsg;
    else
        tid1 = datap->s_id1;
    anchor = (datap->s_text[0] == '+' || datap->s_text[0] == '-');
    if (datap->s_ccid[0]) {
        sfprintf (
            fp4,
            "severity=%s object=%s msg_text=\"%s%s%s%s%s"
            " CORR EVENT CLOSE\"\n",
            sevmaps[datap->s_severity].name, tid1,
            !anchor ? "+-USER> MSGKEY=none " : "",
            !anchor ? (
                datap->s_type == VG_ALARM_N_TYPE_ALARM ? ":DOWN " : ":UP "
            ) : "",
            datap->s_text, datap->s_comment[0] ? " COMMENT=" : "",
            datap->s_comment
        );
    } else {
        sfprintf (
            fp4,
            "severity=%s object=%s msg_text=\"%s%s%s%s%s\"\n",
            sevmaps[datap->s_severity].name, tid1,
            !anchor ? "+-USER> MSGKEY=none " : "",
            !anchor ? (
                datap->s_type == VG_ALARM_N_TYPE_ALARM ? ":DOWN " : ":UP "
            ) : "",
            datap->s_text, datap->s_comment[0] ? " COMMENT=" : "",
            datap->s_comment
        );
    }
    sfsync (fp4);
    VG_warning (0, "DATA INFO", "closed ticket obj=%s", tid1);
    return 0;
}

#define FKV_AID          0
#define FKV_CCID         1
#define FKV_STATE        2
#define FKV_SMODE        3
#define FKV_VARIABLES    4
#define FKV_TIMECLEARED  5
#define FKV_TIMEISSUED   6
#define FKV_TYPE         7
#define FKV_SORTORDER    8
#define FKV_PMODE        9
#define FKV_LV1         10
#define FKV_ID1         11
#define FKV_LV2         12
#define FKV_ID2         13
#define FKV_TMODE       14
#define FKV_SEVERITY    15
#define FKV_TEXT        16
#define FKV_COMMENT     17
#define FKV_ORIGMSG     18
typedef struct fieldkv_s {
    char *key, *val;
} fieldkv_t;
static struct fieldkv_s fieldkvs[] = {
    { "aid",         NULL },
    { "ccid",        NULL },
    { "state",       NULL },
    { "smode",       NULL },
    { "variables",   NULL },
    { "timecleared", NULL },
    { "timeissued",  NULL },
    { "type",        NULL },
    { "sortorder",   NULL },
    { "pmode",       NULL },
    { "lv1",         NULL },
    { "id1",         NULL },
    { "lv2",         NULL },
    { "id2",         NULL },
    { "tmode",       NULL },
    { "severity",    NULL },
    { "text",        NULL },
    { "comment",     NULL },
    { "origmsg",     NULL },
    { NULL,          NULL },
};
#define FKVVAL(i, d) (fieldkvs[i].val ? fieldkvs[i].val : (d))

static int emitrecord (context_t *cxtp) {
    alarm_t *alarmp;
    int alarmi;
    int maxalarmi;
    long maxtime;
    EEval_t v;
    fieldkv_t *fkvp;
    int fkvi;
    char buf[5][4000], *s1, *s2, *s3, *s4, quote, *sp, *ep;
    int vmi, id, n;

    if (!fp3 && !(fp3 = sfnew (NULL, NULL, 4096, 3, SF_WRITE))) {
        SUwarning (0, "emitrecord", "open of fd 3 failed");
        return -1;
    }

    maxalarmi = -1, maxtime = -1;
    for (alarmi = 0; alarmi < cxtp->matchp->alarml; alarmi++) {
        alarmp = cxtp->matchp->alarms[alarmi];
        if (alarmp->data.s_timeissued > maxtime) {
            maxalarmi = alarmi;
            maxtime = alarmp->data.s_timeissued;
        }
    }
    if (maxalarmi == -1) {
        SUwarning (0, "emitrecord", "cannot find alarm record");
        return -1;
    }
    alarmp = cxtp->matchp->alarms[maxalarmi];

    for (fkvi = 0; fieldkvs[fkvi].key; fkvi++)
        fieldkvs[fkvi].val = NULL;
    if (getmatchvar (
        cxtp->matchp, CE_VAR_MATCH_FIELDS, varmap[CE_VAR_MATCH_FIELDS],
        &v, cxtp
    ) == -1) {
        SUwarning (0, "emitrecord", "cannot get fields value");
        return -1;
    }
    if (v.type == EE_VAL_TYPE_RSTRING)
        strncpy (buf[0], v.u.s, 4000);
    else {
        SUwarning (0, "emitrecord", "cannot get fields data");
        return -1;
    }
    buf[1][0] = 0, sp = &buf[1][0], ep = sp + 4000 - 1;
    for (s1 = buf[0]; s1; ) {
        if ((s2 = strchr (s1, '|')))
            *s2++ = 0;
        if (!(s3 = strchr (s1, '='))) {
            SUwarning (0, "emitrecord", "cannot find key-value separator");
            return -1;
        }
        *s3++ = 0;
        for (fkvi = 0, fkvp = NULL; fieldkvs[fkvi].key; fkvi++) {
            if (strcmp (s1, fieldkvs[fkvi].key) == 0) {
                fkvp = &fieldkvs[fkvi];
                break;
            }
        }
        if (!fkvp) {
            SUwarning (0, "emitrecord", "cannot find variable %s", s1);
            return -1;
        }
        fkvp->val = sp;
        while (s3) {
            if (*s3 == '\'')
                quote = '\'', s3++;
            else if (*s3 == '"')
                quote = '"', s3++;
            else
                quote = 0;
            if (quote) {
                if (!(s4 = strchr (s3, quote))) {
                    SUwarning (0, "emitrecord", "cannot find quote separator");
                    return -1;
                }
                *s4++ = 0;
                n = sfsprintf (sp, ep - sp, "%s", s3), sp += n;
                if (*s4 == ',')
                    *s4++ = 0;
                else
                    s4 = NULL;
            } else {
                if ((s4 = strchr (s3, ',')))
                    *s4++ = 0;
                if (strncmp (s3, "_alarm_", 7) == 0) {
                    id = -1;
                    for (
                        vmi = CE_VAR_ALARM_FIRST;
                        vmi <= CE_VAR_ALARM_LAST; vmi++
                    ) {
                        if (strcmp (varmap[vmi], s3) == 0) {
                            id = vmi;
                            break;
                        }
                    }
                    if (getalarmvar (alarmp, id, s3, &v, cxtp) == -1) {
                        SUwarning (0, "emitrecord", "cannot get aval %s", s3);
                        return -1;
                    }
                } else {
                    id = -1;
                    for (
                        vmi = CE_VAR_MATCH_FIRST;
                        vmi <= CE_VAR_MATCH_LAST; vmi++
                    ) {
                        if (strcmp (varmap[vmi], s3) == 0) {
                            id = vmi;
                            break;
                        }
                    }
                    if (getmatchvar (cxtp->matchp, id, s3, &v, cxtp) == -1) {
                        SUwarning (0, "emitrecord", "cannot get mval %s", s3);
                        return -1;
                    }
                }
                switch (v.type) {
                case EE_VAL_TYPE_NULL:
                    n = sfsprintf (sp, ep - sp, "NULL");
                    break;
                case EE_VAL_TYPE_INTEGER:
                    n = sfsprintf (sp, ep - sp, "%d", v.u.i);
                    break;
                case EE_VAL_TYPE_REAL:
                    n = sfsprintf (sp, ep - sp, "%f", v.u.f);
                    break;
                case EE_VAL_TYPE_CSTRING:
                    n = sfsprintf (sp, ep - sp, "%s", v.u.s);
                    break;
                case EE_VAL_TYPE_RSTRING:
                    n = sfsprintf (sp, ep - sp, "%s", v.u.s);
                    break;
                default:
                    SUwarning (0, "emitrecord", "bad value type for: %s", s3);
                    return -1;
                    break;
                }
                sp += n;
            }
            s3 = s4;
        }
        s1 = s2;
        *sp++ = 0;
    }

    VG_urlenc (FKVVAL (FKV_TEXT, ""), buf[2], 4000);
    VG_urlenc (FKVVAL (FKV_COMMENT, ""), buf[3], 4000);
    sfsprintf (buf[4], 4000, "%ld", rootp->currtime);
    sfprintf (
        fp3,
        "<alarm>"
        "<v>%s</v><sid></sid><aid>%s</aid>"
        "<ccid>%s</ccid><st>%s</st><sm>%s</sm>"
        "<vars>%s</vars>"
        "<di></di><hi></hi>"
        "<tc>%s</tc><ti>%s</ti>"
        "<tp>%s</tp><so>%s</so><pm>%s</pm>"
        "<lv1>%s</lv1><id1>%s</id1><lv2>%s</lv2><id2>%s</id2>"
        "<tm>%s</tm><sev>%s</sev>"
        "<txt>%s</txt><com>%s</com><origmsg>%s</origmsg>"
        "</alarm>\n",
        VG_S_VERSION, FKVVAL (FKV_AID, ""),
        FKVVAL (FKV_CCID, alarmp->data.s_ccid),
        FKVVAL (FKV_STATE, ""), FKVVAL (FKV_SMODE, ""),
        FKVVAL (FKV_VARIABLES, ""),
        FKVVAL (FKV_TIMECLEARED, "0"), FKVVAL (FKV_TIMEISSUED, buf[4]),
        FKVVAL (FKV_TYPE, ""), FKVVAL (FKV_SORTORDER, ""),
        FKVVAL (FKV_PMODE, ""),
        FKVVAL (FKV_LV1, ""), FKVVAL (FKV_ID1, ""),
        FKVVAL (FKV_LV2, ""), FKVVAL (FKV_ID2, ""),
        FKVVAL (FKV_TMODE, ""), FKVVAL (FKV_SEVERITY, ""),
        buf[2], buf[3], FKVVAL (FKV_ORIGMSG, "")
    );
    sfsync (fp3);

    return 0;
}
