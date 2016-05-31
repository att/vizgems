#pragma prototyped

#include <ast.h>
#include <option.h>
#include <swift.h>
#include <dds.h>
#include <time.h>
#define VG_DEFS_MAIN
#define VG_SEVMAP_MAIN
#include "eeval.h"
#include "vgce.h"

static const char usage[] =
"[-1p1?\n@(#)$Id: vgce (AT&T Labs Research) 2001-09-24 $\n]"
USAGE_LICENSE
"[+NAME?vgce - VG alarm correlation engine]"
"[+DESCRIPTION?\bvgce\b reads alarm records in DDS format from standard"
" input, and uses the specified set of rules to group alarms into collections"
" of related alarms."
" \bvgce\b processes each record that it reads, assigns a CCID to it (CCID is"
" a correlation group id), and outputs the record in the same DDS format, but"
" with the \bccid\b field of the record filled out."
" If the alarm record is not of a known alarm type (its alarmid field is not"
" set), the CCID value will be -1 and the record will not be correlated to any"
" other records."
" \bvgce\b will also generate ticketing records."
" These records are in the ascii format used by RuniGEMS."
" A ticketing record will be generated when either (1) a group of alarms"
" satisfies the ticketing requirements of at least one of the rules used to"
" generate the group, or (2) when an alarm with ticket mode =="
" VG_ALARM_N_MODE_KEEP is received and is attached to a correlation group"
" that had not been ticketed before."
" Alarms with ticket mode == VG_ALARM_S_MODE_DEFER will not generate tickets"
" except when condition (1) is met."
" Tickets of alarms that are not correlated (ie their alarmid field is not set)"
" are the standard RuniGEMS tickets, while correlation tickets have extra text"
" in their \btext\b field that identifies the correlation group they"
" belong to."
" \bvgce\b uses a permanent storage library to speed up startup time."
"]"
"[100:rf?specifies the file containing the correlation rules."
"]:[rulesfile]"
"[101:sf?specifies the state file for the tool's permanent storage feature."
" The file must exist, unless the \b-new\b option is also specified."
"]:[statefile]"
"[103:date?specifies the date (YYYY.MM.DD) that alarms must have to be"
" processed."
" Other alarms are passed through without any processing."
"]:[date]"
"[104:prefix?specifies the prefix to attach to CCIDs."
" The final CCID string will have the form \bprefix.number\b."
" This is useful when several correlation engines forward their data to a"
" single server for display."
" The default prefix is \bCCID\b."
"]:[prefix]"
"[200:compact?specifies that the permanent storage area should be compacted."
"]"
"[201:flush?specifies that the permanent storage area should be emptied."
"]"
"[202:new?specifies that a new permanent storage area should be created."
" The argument specifies the first CCID to use in assigning ids to groups."
"]#[firstccid]"
"[203:update?specifies that all current correlation groups should be checked"
" and any groups that are no longer active should be removed."
"]"
"[204:checkrules?loads and checks the rules file without doing any other"
" processing."
"]"
"[205:dump?specifies that the correlation shate should be dumped to stderr."
"]"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"[+SEE ALSO?\bdds\b(1), \bdds\b(3)]"
;

#define DATEMATCH(d1, d2) ( \
    d1[9] == d2[9] && d1[8] == d2[8] && d1[7] == d2[7] && d1[6] == d2[6] && \
    d1[5] == d2[5] && d1[4] == d2[4] && d1[3] == d2[3] && d1[2] == d2[2] && \
    d1[1] == d2[1] && d1[0] == d2[0] \
)

root_t *rootp;

char *varmap[] = {
    "_alarm_alarmid",  /* CE_VAR_ALARM_ALARMID  */
    "_alarm_state",    /* CE_VAR_ALARM_STATE    */
    "_alarm_smode",    /* CE_VAR_ALARM_SMODE    */
    "_alarm_tmode",    /* CE_VAR_ALARM_TMODE    */
    "_alarm_level",    /* CE_VAR_ALARM_LEVEL1   */
    "_alarm_object",   /* CE_VAR_ALARM_OBJECT1  */
    "_alarm_level2",   /* CE_VAR_ALARM_LEVEL2   */
    "_alarm_object2",  /* CE_VAR_ALARM_OBJECT2  */
    "_alarm_time",     /* CE_VAR_ALARM_TIME     */
    "_alarm_severity", /* CE_VAR_ALARM_SEVERITY */
    "_alarm_text",     /* CE_VAR_ALARM_TEXT     */
    "_alarm_comment",  /* CE_VAR_ALARM_COMMENT  */

    "FALSE",      /* CE_VAR_MATCH_FALSE      */
    "TRUE",       /* CE_VAR_MATCH_TRUE       */
    "PRUNEMATCH", /* CE_VAR_MATCH_PRUNEMATCH */
    "PRUNEALARM", /* CE_VAR_MATCH_PRUNEMATCH */

    "_result",         /* CE_VAR_MATCH_RESULT */
    "_currtime",       /* CE_VAR_MATCH_CTIME  */
    "_tickettime",     /* CE_VAR_MATCH_TTIME  */
    "_prunetime",      /* CE_VAR_MATCH_PTIME  */
    "_firstalarmtime", /* CE_VAR_MATCH_FATIME */
    "_lastalarmtime",  /* CE_VAR_MATCH_LATIME */
    "_fields",         /* CE_VAR_MATCH_FIELDS */

    NULL,
};

char *statemap[] = {
    VG_ALARM_S_STATE_NONE, /* VG_ALARM_N_STATE_NONE */
    VG_ALARM_S_STATE_INFO, /* VG_ALARM_N_STATE_INFO */
    VG_ALARM_S_STATE_DEG,  /* VG_ALARM_N_STATE_DEG  */
    VG_ALARM_S_STATE_DOWN, /* VG_ALARM_N_STATE_DOWN */
    VG_ALARM_S_STATE_UP,   /* VG_ALARM_N_STATE_UP   */
};

char *modemap[] = {
    VG_ALARM_S_MODE_NONE,  /* VG_ALARM_N_MODE_NONE  */
    VG_ALARM_S_MODE_KEEP,  /* VG_ALARM_N_MODE_KEEP  */
    VG_ALARM_S_MODE_DEFER, /* VG_ALARM_N_MODE_DEFER */
    VG_ALARM_S_MODE_DROP,  /* VG_ALARM_N_MODE_DROP  */
};

char *pmodemap[] = {
    VG_ALARM_S_PMODE_NONE,        /* VG_ALARM_N_PMODE_NONE        */
    VG_ALARM_S_PMODE_PROCESS,     /* VG_ALARM_N_PMODE_PROCESS     */
    VG_ALARM_S_PMODE_PASSTHROUGH, /* VG_ALARM_N_PMODE_PASSTHROUGH */
};

char *typemap[] = {
    VG_ALARM_S_TYPE_NONE,  /* VG_ALARM_N_TYPE_NONE  */
    VG_ALARM_S_TYPE_ALARM, /* VG_ALARM_N_TYPE_ALARM */
    VG_ALARM_S_TYPE_CLEAR, /* VG_ALARM_N_TYPE_CLEAR */
};

char *ftmode, *rtmode;

char *currdate;

int main (int argc, char **argv) {
    int norun;
    char *rulesfile, *statefile, *ccidprefix, *s;
    int compactflag, flushflag, updateflag, newflag, checkflag, dumpflag;
    int firstcci;

    Sfio_t *ifp, *ofp;
    int osm;
    DDSschema_t *schemap;
    DDSheader_t hdr;
    int ret;

    VG_alarm_t adata;
    int clearmode, mi, mn;

    schemap = NULL;
    osm = DDS_HDR_NAME | DDS_HDR_DESC;
    rulesfile = NULL;
    statefile = NULL;
    ccidprefix = "CCID";
    currdate = NULL;
    compactflag = FALSE;
    flushflag = FALSE;
    newflag = FALSE;
    firstcci = -1;
    updateflag = FALSE;
    checkflag = FALSE;
    dumpflag = FALSE;
    norun = 0;
    for (;;) {
        switch (optget (argv, usage)) {
        case -100:
            rulesfile = opt_info.arg;
            continue;
        case -101:
            statefile = opt_info.arg;
            continue;
        case -103:
            currdate = opt_info.arg;
            continue;
        case -104:
            ccidprefix = opt_info.arg;
            continue;
        case -200:
            compactflag = TRUE;
            continue;
        case -201:
            flushflag = TRUE;
            continue;
        case -202:
            newflag = TRUE;
            firstcci = opt_info.num;
            continue;
        case -203:
            updateflag = TRUE;
            continue;
        case -204:
            newflag = TRUE;
            checkflag = TRUE;
            continue;
        case -205:
            dumpflag = TRUE;
            continue;
        case -999:
            SUwarnlevel++;
            continue;
        case '?':
            SUusage (0, "vgce", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "vgce", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;
    if (!rulesfile || !statefile || !currdate || strlen (currdate) != 10) {
        SUwarning (0, "vgce", "missing arguments");
        return -1;
    }

    if (sevmapload (getenv ("SEVMAPFILE")) == -1)
        SUerror ("vgce", "cannot load sevmap file");
    DDSinit ();
    if (!(ifp = SUsetupinput (sfstdin, 1048576)))
        SUerror ("vgce", "setupinput failed");
    if (!(ofp = SUsetupoutput (NULL, sfstdout, 1048576)))
        SUerror ("vgce", "setupoutput failed");
    if ((ret = DDSreadheader (ifp, &hdr)) == -1) {
        SUwarning (0, "vgce", "DDSreadheader failed");
        goto failsafe1;
    }
    if (ret == -2)
        goto done;
    if (ret == 0) {
        if (hdr.schemap)
            schemap = hdr.schemap;
    }
    if (!schemap) {
        SUwarning (0, "vgce", "DDSloadschema failed");
        goto failsafe1;
    }
    if (!hdr.schemap && schemap)
        hdr.schemap = schemap;
    hdr.contents = osm;
    hdr.vczspec = "";
    if (osm && DDSwriteheader (ofp, &hdr) == -1) {
        SUwarning (0, "vgce", "DDSwriteheader failed");
        goto failsafe1;
    }

    s = getenv ("DEFAULTTMODE");
    if (!(ftmode = vmstrdup (Vmheap, s ? s : VG_ALARM_S_MODE_KEEP))) {
        SUwarning (0, "vgce", "cannot copy tmode");
        goto failsafe1;
    }
    if ((rtmode = strchr (ftmode, ':')))
        *rtmode++ = 0;
    else
        rtmode = ftmode;

    if (CEinit (
        statefile, rulesfile, newflag, compactflag,
        sizeof (VG_alarm_t), firstcci, ccidprefix
    ) == -1) {
        SUwarning (0, "vgce", "cannot init CE");
        if (rootp && rootp->rulel > 0)
            goto failsafe2;
        goto failsafe1;
    }

    if (schemap->recsize != sizeof (VG_alarm_t)) {
        SUwarning (
            0, "vgce", "size mismatch for alarm record: DDS=%d - VG=%d",
            schemap->recsize, sizeof (VG_alarm_t)
        );
        goto failsafe3;
    }
    if (rootp->alarmdatasize != sizeof (VG_alarm_t)) {
        SUwarning (
            0, "vgce", "size changed from last invocation: OLD=%d - NEW=%d",
            rootp->alarmdatasize, sizeof (VG_alarm_t)
        );
        goto failsafe3;
    }

    if (checkflag)
        goto done;

    if (dumpflag) {
        printalarms (999);
        printrules (999);
        printccs (999);
        goto done;
    }

    mn = 0;
    while ((ret = DDSreadrecord (
        ifp, &adata, schemap
    )) == rootp->alarmdatasize) {
        VG_warning (0, "DATA INFO", "read alarm %s", adata.s_text);
        mi = 0;
        if (rootp->currtime < adata.s_timeissued)
            rootp->currtime = adata.s_timeissued;

        if (adata.s_type == VG_ALARM_N_TYPE_CLEAR) {
            clearmode = 0;
            if (strcmp (adata.s_id1, "__ce_svc_upd__") == 0) {
                VG_warning (0, "DATA INFO", "HB alarm");
                if (updatetickets () == -1) {
                    SUwarning (0, "vgce", "cannot update tickets");
                    goto failsafe2;
                }
                if (removematchesbytime () == -1) {
                    SUwarning (0, "vgce", "cannot remove matches");
                    goto failsafe2;
                }
                if (emitrecords () == -1) {
                    SUwarning (0, "vgce", "cannot emit records");
                    goto failsafe2;
                }
                goto write;
            } else if (strcmp (adata.s_id1, "__all_clear__") == 0)
                clearmode = 'A';
            else if (adata.s_ccid[0])
                clearmode = 'c';
            else if (adata.s_alarmid[0] && adata.s_id1[0])
                clearmode = 'i';
            else if (adata.s_timecleared > 0 && adata.s_id1[0])
                clearmode = 'a';
            else if (adata.s_id1[0])
                clearmode = 'o';
            if (clearmode) {
                VG_warning (0, "DATA INFO", "clear alarm");
                if (removematchesbyclear (&adata, clearmode) == -1) {
                    SUwarning (0, "vgce", "cannot remove matches");
                    goto failsafe2;
                }
                if (
                    adata.s_tmode != VG_ALARM_N_MODE_DROP &&
                    closeanyticket (&adata) == -1
                ) {
                    SUwarning (0, "vgce", "cannot clear ticket");
                    goto failsafe2;
                }
                goto write;
            }
        }
        if (
            DATEMATCH (currdate, adata.s_dateissued) &&
            adata.s_sortorder == VG_ALARM_N_SO_NORMAL &&
            adata.s_pmode == VG_ALARM_N_PMODE_PROCESS &&
            adata.s_tmode != VG_ALARM_N_MODE_DROP
        ) {
            mi = 0;
            if (adata.s_alarmid[0] && !adata.s_ccid[0]) {
                if ((mi = insertmatches (&adata)) == -1) {
                    SUwarning (0, "vgce", "cannot insert matches");
                    goto failsafe2;
                }
                mn += mi;
                if (mi > 0)
                    adata.s_tmode = VG_ALARM_N_MODE_DEFER;
            }
            if (mi == 0 && adata.s_tmode != VG_ALARM_N_MODE_DROP) {
                if (simpleticket (&adata) == -1)
                    SUwarning (0, "vgce", "cannot create simple ticket");
            }
        } else
            VG_warning (0, "DATA INFO", "pass-through alarm");

write:
        if (adata.s_smode != VG_ALARM_N_MODE_KEEP)
            continue;

        if (DDSwriterecord (ofp, &adata, schemap) != rootp->alarmdatasize) {
            SUwarning (0, "vgce", "cannot write alarm");
            goto failsafe1;
        }
        VG_warning (0, "DATA INFO", "wrote alarm");
    }

    if (mn > 0 || updateflag) {
        if (updatetickets () == -1) {
            SUwarning (0, "vgce", "cannot update tickets");
            goto failsafe3;
        }
        if (removematchesbytime () == -1) {
            SUwarning (0, "vgce", "cannot remove matches");
            goto failsafe3;
        }
        if (emitrecords () == -1) {
            SUwarning (0, "vgce", "cannot emit records");
            goto failsafe3;
        }
    }

    if (flushflag) {
        rootp->currtime = INT_MAX;
        if (updatetickets () == -1)
            SUwarning (0, "vgce", "cannot update tickets");
        if (removematchesbytime () == -1)
            SUwarning (0, "vgce", "cannot remove matches");
    }

    VG_warning (
        0, "CORR INFO", "alarms=%d ccs=%d", rootp->alarmn, rootp->ccm
    );

    printalarms (SUwarnlevel);
    printrules (SUwarnlevel);
    printccs (SUwarnlevel);

    if (CEterm (statefile) == -1)
        SUerror ("vgce", "cannot term CE");

    if (ret != 0)
        SUerror ("vgce", "failed to read full record");

done:
    DDSterm ();
    return 0;

failsafe1:
    SUwarning (0, "vgce", "IN FAILSAFE MODE 1");
    if (sfmove (ifp, ofp, -1, -1) == -1)
        SUwarning (0, "vgce", "cannot copy data");
    return 101;

failsafe2:
    SUwarning (0, "vgce", "IN FAILSAFE MODE 2");
    if (DDSwriterecord (ofp, &adata, schemap) != rootp->alarmdatasize)
        SUwarning (0, "vgce", "cannot write alarm");
    while ((ret = DDSreadrecord (
        ifp, &adata, schemap
    )) == rootp->alarmdatasize) {
        if (adata.s_tmode != VG_ALARM_N_MODE_DROP) {
            if (simpleticket (&adata) == -1)
                SUwarning (0, "vgce", "cannot create simple ticket");
        }
        if (DDSwriterecord (ofp, &adata, schemap) != rootp->alarmdatasize) {
            SUwarning (0, "vgce", "cannot write alarm");
            break;
        }
    }

failsafe3:
    SUwarning (0, "vgce", "IN FAILSAFE MODE 3");
    rootp->currtime = INT_MAX;
    if (updatetickets () == -1)
        SUwarning (0, "vgce", "cannot update tickets");
    if (removematchesbytime () == -1)
        SUwarning (0, "vgce", "cannot remove matches");
    return 103;

}
