#pragma prototyped

#include <ast.h>
#include <option.h>
#include <swift.h>
#include <dds.h>
#include <time.h>
#define VG_DEFS_MAIN
#define VG_SEVMAP_MAIN
#include "vgpce.h"
#include "sl_profile.c"

static const char usage[] =
"[-1p1?\n@(#)$Id: vgpce (AT&T Labs Research) 2011-02-01 $\n]"
USAGE_LICENSE
"[+NAME?vgpce - VG statistical alarm engine]"
"[+DESCRIPTION?\bvgpce\b reads statistics records in DDS format from standard"
" input, and uses the specified set of rules to determine if alarms need to be"
" generated."
"]"
"[100:af?specifies the file containing the alarm generation rules."
" It must be in DDS format with schema vg_profile.schema"
"]:[alarmfile]"
"[101:pf?specifies the file containing the pathnames of the profile files."
" It must be an ascii file with format: frameno|meanfile_path|sdevfile_path"
"]:[proffile]"
"[102:sf?specifies the file containing the state from the previous execution"
"]:[statefile]"
"[103:date?specifies the date (YYYY.MM.DD) that stats records must have to be"
" processed."
" Other alarms are passed through without any processing."
"]:[date]"
"[200:new?specifies that a new state file should be created."
"]"
"[201:check?loads and checks all the support files without doing any other"
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

int currtime, curri;
int secperframe;
char *ftmode, *rtmode;

int main (int argc, char **argv) {
    int norun;
    char *alarmfile, *proffile, *statefile, *currdate;
    int newflag, checkflag;

    Sfio_t *ifp, *ofp;
    int osm;
    DDSschema_t *schemap;
    DDSheader_t hdr;
    int ret;

    VG_stat_t sdata;
    node_t *pnp, *np;
    rule_t *rp;
    rulerange_t *rrp;
    profdata_t *pdp;
    sl_profile_t *parp;
    float dval;
    char skey[SZ_key], *s;

    schemap = NULL;
    osm = DDS_HDR_NAME | DDS_HDR_DESC;
    alarmfile = NULL;
    proffile = NULL;
    statefile = NULL;
    currdate = NULL;
    newflag = FALSE;
    checkflag = FALSE;
    norun = 0;
    for (;;) {
        switch (optget (argv, usage)) {
        case -100:
            alarmfile = opt_info.arg;
            continue;
        case -101:
            proffile = opt_info.arg;
            continue;
        case -102:
            statefile = opt_info.arg;
            continue;
        case -103:
            currdate = opt_info.arg;
            continue;
        case -200:
            newflag = TRUE;
            continue;
        case -999:
            SUwarnlevel++;
            continue;
        case '?':
            SUusage (0, "vgpce", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "vgpce", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;

    ifp = ofp = NULL;
    if (
        !alarmfile || !proffile || !statefile ||
        !currdate || strlen (currdate) != 10
    ) {
        SUwarning (0, "vgpce", "missing arguments");
        goto failsafe1;
    }

    DDSinit ();
    if (!(ifp = SUsetupinput (sfstdin, 1048576))) {
        SUwarning (0, "vgpce", "setupinput failed");
        goto failsafe1;
    }
    if (!(ofp = SUsetupoutput (NULL, sfstdout, 1048576))) {
        SUwarning (0, "vgpce", "setupoutput failed");
        goto failsafe1;
    }
    if ((ret = DDSreadheader (ifp, &hdr)) == -1) {
        SUwarning (0, "vgpce", "DDSreadheader failed");
        goto failsafe1;
    }
    if (ret == -2)
        goto done;
    if (ret == 0) {
        if (hdr.schemap)
            schemap = hdr.schemap;
    }
    if (!schemap) {
        SUwarning (0, "vgpce", "DDSloadschema failed");
        goto failsafe1;
    }
    if (!hdr.schemap && schemap)
        hdr.schemap = schemap;
    hdr.contents = osm;
    hdr.vczspec = "";
#ifdef EMIT
    if (osm && DDSwriteheader (ofp, &hdr) == -1) {
        SUwarning (0, "vgpce", "DDSwriteheader failed");
        goto failsafe1;
    }
#endif

    s = getenv ("DEFAULTTMODE");
    if (!(ftmode = vmstrdup (Vmheap, s ? s : VG_ALARM_S_MODE_KEEP))) {
        SUwarning (0, "vgpce", "cannot copy tmode");
        goto failsafe1;
    }
    if ((rtmode = strchr (ftmode, ':')))
        *rtmode++ = 0;
    else
        rtmode = ftmode;
    currtime = 0;
    secperframe = atoi (getenv ("STATINTERVAL"));
    if (sl_profileopen (alarmfile) != 0) {
        SUwarning (0, "vgpce", "cannot load alarm spec file");
        goto failsafe1;
    }
    if (ruleinit () == -1) {
        SUwarning (0, "vgpce", "ruleinit failed");
        goto failsafe1;
    }
    if (alarminit () == -1) {
        SUwarning (0, "vgpce", "alarminit failed");
        goto failsafe1;
    }
    if (nodeinit (proffile) == -1) {
        SUwarning (0, "vgpce", "nodeinit failed");
        goto failsafe1;
    }
    if (checkflag)
        goto done;
    if (!newflag && nodeloadstate (statefile) == -1) {
        SUwarning (0, "vgpce", "loadstate failed");
        goto failsafe1;
    }
    nodemark++;

    pnp = NULL;
    while ((
        ret = DDSreadrecord (ifp, &sdata, schemap)
    ) == sizeof (VG_stat_t)) {
        if (!DATEMATCH (currdate, sdata.s_dateissued))
            goto write;

        if (currtime < sdata.s_timeissued) {
            currtime = sdata.s_timeissued;
            curri = TIME2I (currtime);
        }

        np = NULL;
        rp = NULL;
        if (
            pnp && strcmp (sdata.s_level, pnp->level) == 0 &&
            strcmp (sdata.s_id, pnp->id) == 0 &&
            strcmp (sdata.s_key, pnp->key) == 0
        ) {
            np = pnp;
            rp = pnp->rulep;
        }
        if (!rp) {
            if ((parp = sl_profilefind (
                sdata.s_level, sdata.s_id, sdata.s_key
            )))
                if (!(rp = ruleinsert (parp->sl_spec)))
                    goto write;
        }
        if (!rp && (s = strchr (sdata.s_key, '.'))) {
            memset (skey, 0, SZ_key);
            memcpy (skey, sdata.s_key, s - sdata.s_key);
            if (!(parp = sl_profilefind (
                sdata.s_level, sdata.s_id, skey
            )))
                goto write;
            if (!(rp = ruleinsert (parp->sl_spec)))
                goto write;
        }
        if (!np) {
            if (!(np = nodeinsert (
                sdata.s_level, sdata.s_id, sdata.s_key, sdata.s_label
            ))) {
                SUwarning (0, "vgpce", "cannot insert node");
                goto failsafe2;
            }
        }
        pnp = np;
        if (rp != np->rulep) {
            if (np->rulep)
                ruleremove (np->rulep), np->rulep = NULL;
            if (nodeinsertrule (np, rp) == -1) {
                SUwarning (0, "vgpce", "cannot insert rule");
                goto failsafe2;
            }
        }
        if (!(pdp = nodeloadprofdata (np, sdata.s_frame)))
            goto write;
        rrp = rulematch (rp, sdata.s_val, pdp->mean, pdp->sdev, &dval);
        if (np->alarmdatan > 0 || rrp) {
            if (nodeinsertalarmdata (
                np, TIME2I (sdata.s_timeissued), (rrp) ? rrp->sev : -1, dval
            ) != 0)
                goto write;
        }

write:
#ifdef EMIT
        if (DDSwriterecord (ofp, &sdata, schemap) != sizeof (VG_stat_t)) {
            SUwarning (0, "vgpce", "cannot write stat");
            goto failsafe1;
        }
#else
        ;
#endif
    }

    if (allnodesupdatealarms () == -1)
        SUwarning (0, "vgpce", "cannot update all node alarms");

    nodemark++;
    if (allnodesprune () == -1)
        SUwarning (0, "vgpce", "cannot prune all nodes");

    if (nodesavestate (statefile) == -1)
        SUwarning (0, "vgpce", "cannot save node state");

done:
    DDSterm ();
    nodeterm ();
    alarmterm ();
    ruleterm ();
    return 0;

failsafe2:
#ifdef EMIT
    if (DDSwriterecord (ofp, &sdata, schemap) != sizeof (VG_stat_t))
        SUwarning (0, "vgpce", "cannot write stat");
#else
        ;
#endif

failsafe1:
    SUwarning (0, "vgpce", "IN FAILSAFE MODE");
#ifdef EMIT
    if (sfmove (ifp, ofp, -1, -1) == -1)
        SUwarning (0, "vgpce", "cannot copy data");
#else
        ;
#endif

    return 101;
}
