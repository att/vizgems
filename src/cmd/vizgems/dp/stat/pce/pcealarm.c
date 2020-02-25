#pragma prototyped

#include <ast.h>
#include <vmalloc.h>
#include <swift.h>
#include "vgpce.h"

alarmdatalist_t *alarmdatalists, *alarmdatalistp;
int alarmdatalistn;

int alarminit (void) {
    alarmdatalists = NULL;
    alarmdatalistp = NULL;
    alarmdatalistn = 0;

    return 0;
}

int alarmterm (void) {
    int adli;

    for (adli = 0; adli < alarmdatalistn; adli++) {
        if (alarmdatalists[adli].alarmdatas) {
            vmfree (Vmheap, alarmdatalists[adli].alarmdatas);
            alarmdatalists[adli].alarmdatas = NULL;
            alarmdatalists[adli].alarmdatan = 0;
        }
    }
    if (alarmdatalists)
        vmfree (Vmheap, alarmdatalists);

    return 0;
}

alarmdata_t *alarmnew (void) {
    alarmdata_t *adp;

    if (
        !alarmdatalistp ||
        alarmdatalistp->alarmdatam == alarmdatalistp->alarmdatan
    ) {
        if (!(alarmdatalists = vmresize (
            Vmheap, alarmdatalists,
            (alarmdatalistn + 1) * sizeof (alarmdatalist_t), VM_RSCOPY
        ))) {
            SUwarning (0, "alarmnew", "cannot grow alarmdatalists");
            return NULL;
        }
        if (!(alarmdatalists[alarmdatalistn].alarmdatas = vmalloc (
            Vmheap, ADL_SIZE * sizeof (alarmdata_t)
        ))) {
            SUwarning (0, "alarmnew", "cannot allocate alarmdatas");
            return NULL;
        }
        alarmdatalists[alarmdatalistn].alarmdatam = 0;
        alarmdatalists[alarmdatalistn].alarmdatan = ADL_SIZE;
        alarmdatalistp = &alarmdatalists[alarmdatalistn];
        alarmdatalistn++;
    }
    adp = &alarmdatalistp->alarmdatas[alarmdatalistp->alarmdatam++];
    memset (adp, 0, sizeof (alarmdata_t));
    return adp;
}

alarmdata_t *alarmmatchrange (alarmdata_t *fadp, rulerange_t *rrp) {
    alarmdata_t *adp, *padp;
    int n, m;

    n = m = 0;
    for (adp = padp = fadp; adp; adp = adp->nextp) {
        if (n == rrp->n) {
            if ((rrp->type == RRTYPE_CLEAR && padp->sev == -1) || (
                rrp->type != RRTYPE_CLEAR && padp->sev != -1 &&
                padp->sev <= rrp->sev
            ))
                m--;
            n--;
            padp = padp->nextp;
        }
        if ((rrp->type == RRTYPE_CLEAR && adp->sev == -1) || (
            rrp->type != RRTYPE_CLEAR && adp->sev != -1 &&
            adp->sev <= rrp->sev
        ))
            m++;
        n++;
        if (m >= rrp->m)
            return adp;
    }
    return NULL;
}

int alarmemit (alarmdata_t *adp, rulerange_t *rrp, node_t *np, int rflag) {
    int tc, ti;
    char *ststr, *tpstr, *tpstr2, txtstr[1024], rangestr[128];

    static Sfio_t *fp3;

    if (!rflag && (
        (rrp->type == RRTYPE_CLEAR && np->alarmstate.mode == ASAM_CLEAR) || (
            rrp->type != RRTYPE_CLEAR && np->alarmstate.mode == ASAM_ALARM &&
            np->alarmstate.sev <= rrp->sev
        )
    ))
        return 1;

    if (!fp3 && !(fp3 = sfnew (NULL, NULL, 4096, 3, SF_WRITE))) {
        SUwarning (0, "alarmemit", "open of fd 3 failed");
        return -1;
    }

    if (rrp->type == RRTYPE_CLEAR) {
        ststr = VG_ALARM_S_STATE_UP;
        tpstr = VG_ALARM_S_TYPE_CLEAR;
        tpstr2 = "CLEAR";
        sfsprintf (
            txtstr, 1024, "Profile Alarm Cleared (%s)", np->label
        );
        ti = currtime;
        tc = adp->i * secperframe;
        np->alarmstate.mode = ASAM_CLEAR;
        np->alarmstate.time = adp->i * secperframe;
        np->alarmstate.rtime = 0;
        np->alarmstate.i = adp->i;
        np->alarmstate.sev = rrp->sev;
    } else {
        ststr = VG_ALARM_S_STATE_DOWN;
        tpstr = VG_ALARM_S_TYPE_ALARM;
        tpstr2 = "ALARM";
        if (rrp->minop == RROP_NONE) {
            if (rrp->maxop == RROP_NONE)
                rangestr[0] = 0;
            else if (rrp->maxop == RROP_INCL)
                sfsprintf (rangestr, 128, "<= %.2f", rrp->maxval);
            else
                sfsprintf (rangestr, 128, "< %.2f", rrp->maxval);
        } else if (rrp->minop == RROP_INCL) {
            if (rrp->maxop == RROP_NONE)
                sfsprintf (rangestr, 128, ">= %.2f", rrp->minval);
            else if (rrp->maxop == RROP_INCL)
                sfsprintf (
                    rangestr, 128, "in range [%.2f,%.2f]",
                    rrp->minval, rrp->maxval
                );
            else
                sfsprintf (
                    rangestr, 128, "in range [%.2f,%.2f)",
                    rrp->minval, rrp->maxval
                );
        } else {
            if (rrp->maxop == RROP_NONE)
                sfsprintf (rangestr, 128, "> %.2f", rrp->minval);
            else if (rrp->maxop == RROP_INCL)
                sfsprintf (
                    rangestr, 128, "in range (%.2f,%.2f]",
                    rrp->minval, rrp->maxval
                );
            else
                sfsprintf (
                    rangestr, 128, "in range (%.2f,%.2f)",
                    rrp->minval, rrp->maxval
                );
        }
        sfsprintf (
            txtstr, 1024, "Profile Alarm (%s=%.2f %s) %s for %d/%d cycles%s",
            np->label, adp->val, rrp->type == RRTYPE_SIGMA ? "sigma" : "actual",
            rangestr, rrp->m, rrp->n, rflag ? " (repeat)" : ""
        );
        ti = adp->i * secperframe;
        tc = 0;
        np->alarmstate.mode = ASAM_ALARM;
        np->alarmstate.time = adp->i * secperframe;
        np->alarmstate.rtime = rrp->ival / rrp->freq;
        np->alarmstate.i = adp->i;
        np->alarmstate.sev = rrp->sev;
    }

    VG_warning (
       0, "PCE INFO",
       "%sissued %s for %s:%s:%s type=%s sev=%d",
       rflag ? "re" : "", tpstr2, np->level, np->id, np->key, tpstr, rrp->sev
    );
    sfprintf (
        fp3,
        "<alarm>"
        "<v>%s</v><sid>PCE</sid><aid>%s</aid>"
        "<ccid></ccid><st>%s</st><sm>%s</sm>"
        "<vars></vars><di></di><hi></hi>"
        "<tc>%d</tc><ti>%d</ti>"
        "<tp>%s</tp><so>%s</so><pm>%s</pm>"
        "<lv1>%s</lv1><id1>%s</id1><lv2></lv2><id2></id2>"
        "<tm>%s</tm><sev>%d</sev>"
        "<txt>VG %s PCE %s</txt><com></com>"
        "<origmsg></origmsg>"
        "</alarm>\n",
        VG_S_VERSION, np->key,
        ststr, VG_ALARM_S_MODE_KEEP,
        tc, ti,
        tpstr, VG_ALARM_S_SO_NORMAL, VG_ALARM_S_PMODE_PROCESS,
        np->level, np->id,
        rflag ? rtmode : ftmode, rrp->sev,
        tpstr2, txtstr
    );

    return 0;
}
