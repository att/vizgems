#pragma prototyped

#include <ast.h>
#include <option.h>
#include <swift.h>
#include <dds.h>
#include <time.h>
#define VG_DEFS_MAIN
#define VG_SEVMAP_MAIN
#include "vggce.h"
#include "sl_inv_nd2cc.c"

static const char usage[] =
"[-1p1?\n@(#)$Id: vggce (AT&T Labs Research) 2001-09-24 $\n]"
USAGE_LICENSE
"[+NAME?vggce - VG alarm graph correlation engine]"
"[+DESCRIPTION?\bvggce\b reads alarm records in DDS format from standard"
" input, and uses the specified set of rules to calculate the impact these"
" alarms have on various services."
" \bvggce\b searches the inventory tables for two kinds of service roles:"
" producers and consumers."
" When one of the rules indicates that an alarm impacts a service, it"
" calculates if the connectivity between each consumer and all the producers"
" has been affected."
" If it has, \bvggce\b generates an alarm indicating partial or total loss"
" of service."
" When the service is restored, \bvggce\b clears that alarm."
"]"
"[100:rf?specifies the file containing the correlation rules."
"]:[rulesfile]"
"[101:sf?specifies the state file."
"]:[statefile]"
"[102:date?specifies the date (YYYY.MM.DD) that alarms must have to be"
" processed."
" Other alarms are passed through without any processing."
"]:[date]"
"[200:new?specifies that a new state file should be created."
"]"
"[201:checkrules?loads and checks the rules file without doing any other"
" processing."
"]"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"[+SEE ALSO?\bdds\b(1), \bdds\b(3)]"
;

#define DATEMATCH(d1, d2) ( \
    d1[9] == d2[9] && d1[8] == d2[8] && d1[7] == d2[7] && d1[6] == d2[6] && \
    d1[5] == d2[5] && d1[4] == d2[4] && d1[3] == d2[3] && d1[2] == d2[2] && \
    d1[1] == d2[1] && d1[0] == d2[0] \
)

char *tmodemap[] = {
    VG_ALARM_S_MODE_NONE,  /* VG_ALARM_N_MODE_NONE */
    VG_ALARM_S_MODE_KEEP,  /* VG_ALARM_N_MODE_KEEP */
    VG_ALARM_S_MODE_DEFER, /* VG_ALARM_N_MODE_DEFER */
    VG_ALARM_S_MODE_DROP,  /* VG_ALARM_N_MODE_DROP  */
    NULL
};

int currtime;
char *ftmode, *rtmode;

int main (int argc, char **argv) {
    int norun;
    char *rulesfile, *statefile;
    int newflag, checkflag;

    Sfio_t *ifp, *ofp;
    int osm;
    DDSschema_t *schemap;
    DDSheader_t hdr;
    int ret;

    VG_alarm_t adata;
    ruleev_t *ruleevp;
    cc_t *ccp;
    nd_t *ndp;
    ev_t *evp;
    sl_inv_nd2cc_t *nd2ccp;
    char *currdate, *s;
    int maxalarmkeepmin;

    schemap = NULL;
    osm = DDS_HDR_NAME | DDS_HDR_DESC;
    rulesfile = NULL;
    statefile = NULL;
    currdate = NULL;
    newflag = FALSE;
    checkflag = FALSE;
    norun = 0;
    for (;;) {
        switch (optget (argv, usage)) {
        case -100:
            rulesfile = opt_info.arg;
            continue;
        case -101:
            statefile = opt_info.arg;
            continue;
        case -102:
            currdate = opt_info.arg;
            continue;
        case -200:
            newflag = TRUE;
            continue;
        case -201:
            checkflag = TRUE;
            continue;
        case -999:
            SUwarnlevel++;
            continue;
        case '?':
            SUusage (0, "vggce", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "vggce", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;

    ifp = ofp = NULL;
    if (!rulesfile || !statefile || !currdate || strlen (currdate) != 10) {
        SUwarning (0, "vggce", "missing arguments");
        goto failsafe1;
    }

    DDSinit ();
    if (!(ifp = SUsetupinput (sfstdin, 1048576))) {
        SUwarning (0, "vggce", "setupinput failed");
        goto failsafe1;
    }
    if (!(ofp = SUsetupoutput (NULL, sfstdout, 1048576))) {
        SUwarning (0, "vggce", "setupoutput failed");
        goto failsafe1;
    }
    if ((ret = DDSreadheader (ifp, &hdr)) == -1) {
        SUwarning (0, "vggce", "DDSreadheader failed");
        goto failsafe1;
    }
    if (ret == -2)
        goto done;
    if (ret == 0) {
        if (hdr.schemap)
            schemap = hdr.schemap;
    }
    if (!schemap) {
        SUwarning (0, "vggce", "DDSloadschema failed");
        goto failsafe1;
    }
    if (!hdr.schemap && schemap)
        hdr.schemap = schemap;
    hdr.contents = osm;
    hdr.vczspec = "";
    if (osm && DDSwriteheader (ofp, &hdr) == -1) {
        SUwarning (0, "vggce", "DDSwriteheader failed");
        goto failsafe1;
    }

    s = getenv ("DEFAULTTMODE");
    if (!(ftmode = vmstrdup (Vmheap, s ? s : VG_ALARM_S_MODE_KEEP))) {
        SUwarning (0, "vggce", "cannot copy tmode");
        goto failsafe1;
    }
    if ((rtmode = strchr (ftmode, ':')))
        *rtmode++ = 0;
    else
        rtmode = ftmode;

    currtime = 0;
    maxalarmkeepmin = atoi (getenv ("MAXALARMKEEPMIN"));
    sl_inv_nd2ccopen (getenv ("INVND2CCFILE"));

    if (sevmapload (getenv ("SEVMAPFILE")) == -1) {
        SUwarning (0, "vggce", "cannot load sevmap file");
        goto failsafe1;
    }
    if (initgraph () == -1) {
        SUwarning (0, "vggce", "initgraph failed");
        goto failsafe1;
    }
    if (initcorr () == -1) {
        SUwarning (0, "vggce", "initcorr failed");
        goto failsafe1;
    }
    if (loadrules (rulesfile) == -1) {
        SUwarning (0, "vggce", "loadrules failed");
        goto failsafe1;
    }
    if (checkflag)
        goto done;
    if (!newflag && loadevstate (statefile) == -1) {
        SUwarning (0, "vggce", "loadstate failed");
        goto failsafe1;
    }

    ccmark++;
    evmark++;

    while ((
        ret = DDSreadrecord (ifp, &adata, schemap)
    ) == sizeof (VG_alarm_t)) {
        VG_warning (0, "GCE INFO", "read alarm %s", adata.s_text);

        if (
            !DATEMATCH (currdate, adata.s_dateissued) ||
            adata.s_sortorder != VG_ALARM_N_SO_NORMAL ||
            adata.s_pmode != VG_ALARM_N_PMODE_PROCESS
        )
            goto write;

        if (currtime < adata.s_timeissued)
            currtime = adata.s_timeissued;

        if (!(ruleevp = matchruleev (&adata)))
            goto write;

        if (!(nd2ccp = sl_inv_nd2ccfind (adata.s_level1, adata.s_id1)))
            goto write;

        if (!(ccp = insertcc (nd2ccp->sl_cclevel, nd2ccp->sl_ccid, TRUE))) {
            SUwarning (0, "vggce", "cannot insert cc %s", nd2ccp->sl_ccid);
            goto failsafe1;
        }
        if (ccp->svcpm < 1)
            goto write;

        if (!(ndp = findnd (adata.s_level1, adata.s_id1))) {
            SUwarning (0, "vggce", "cannot find nd %s", adata.s_id1);
            goto failsafe1;
        }

        if (!(evp = insertev (
            adata.s_timeissued, adata.s_type, ccp, ndp, ruleevp
        ))) {
            SUwarning (0, "vggce", "cannot insert ev");
            goto failsafe1;
        }
        if (evp->svcpm < 1)
            goto write;

        if (insertndevent (ndp, evp) == -1) {
            SUwarning (0, "vggce", "cannot insert ev in nd");
            goto failsafe1;
        }

write:
        if (DDSwriterecord (ofp, &adata, schemap) != sizeof (VG_alarm_t)) {
            SUwarning (0, "vggce", "cannot write alarm");
            goto failsafe1;
        }
        VG_warning (0, "GCE INFO", "wrote alarm");
    }

    ndmark++;

    if (pruneevs (currtime - 60 * maxalarmkeepmin, 1) == -1) {
        SUwarning (0, "vggce", "cannot prune evs (1)");
        goto failsafe1;
    }

    calcsvcvalues (SVC_MODE_NOEV, -1, -1);
    calcsvcvalues (SVC_MODE_OLDEV, 0, 0);
    calcsvcvalues (SVC_MODE_NEWEV, 0, 1);

    if (updatealarms (SVC_MODE_OLDEV, SVC_MODE_NEWEV, 1) == -1) {
        SUwarning (0, "vggce", "cannot update alarms (1)");
        goto failsafe1;
    }

    evmark++;
//    ccmark++;

    if (pruneevs (currtime - 60 * maxalarmkeepmin, 2) == -1) {
        SUwarning (0, "vggce", "cannot prune evs");
        goto failsafe1;
    }

    calcsvcvalues (SVC_MODE_PRUNE, 2, 2);

    if (updatealarms (SVC_MODE_NEWEV, SVC_MODE_PRUNE, 2) == -1) {
        SUwarning (0, "vggce", "cannot update alarms (2)");
        goto failsafe1;
    }

    if (saveevstate (statefile) == -1) {
        SUwarning (0, "vggce", "savestate failed");
        goto failsafe1;
    }

    VG_warning (0, "GCE INFO", "events=%d", evpm);

    if (ret != 0)
        SUerror ("vggce", "failed to read full record");

done:
    DDSterm ();
    return 0;

failsafe1:
    SUwarning (0, "vggce", "IN FAILSAFE MODE");
    if (sfmove (ifp, ofp, -1, -1) == -1)
        SUwarning (0, "vggce", "cannot copy data");
    return 101;
}
