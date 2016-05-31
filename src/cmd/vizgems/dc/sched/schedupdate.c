#pragma prototyped

#include <ast.h>
#include <tm.h>
#include <swift.h>
#include <xml.h>
#include "sched.h"

#define SWD(a, b) ((a).tm_wday == (b).tm_wday)
#define SMD(a, b) ((a).tm_mday == (b).tm_mday)

int scheduleupdate (time_t ct) {
    time_t minnt, soff, eoff, nt, cbod, coff, noff, tt;
    Tm_t tm1, tm2, tmc, *tmp;
    int n;
    schedule_t *sp;
    int si;

    minnt = INT_MAX;
    for (si = 0; si < schedrootp->schedulen; si++) {
        sp = &schedrootp->schedules[si];
        if (sp->expired)
            continue;
        if (sp->scheduled) {
            if (minnt > sp->nt)
                minnt = sp->nt;
            continue;
        }

        soff = sp->soff;
        eoff = sp->eoff;
        nt = 0;
        if (sp->eoff < sp->soff)
            eoff += (24 * 60 * 60);
        if (ct > sp->ebod + eoff) {
            sp->expired = TRUE;
            continue;
        }
        if (sp->mode == 1) {
            if ((n = (ct - (sp->sbod + soff) + sp->ival - 1) / sp->ival) < 0)
                n = 0;
            nt = sp->sbod + soff + n * sp->ival;
            while (nt <= sp->nt)
                nt += sp->ival;
        } else {
            tt = sp->sbod + soff;
            tmp = tmmake (&tt), tm1 = *tmp;
            tt = sp->sbod + eoff;
            tmp = tmmake (&tt), tm2 = *tmp;
            tmp = tmmake (&ct), tmc = *tmp;
            coff = tmc.tm_hour * 3600 + tmc.tm_min * 60 + tmc.tm_sec;
            cbod = ct - coff;
            while (cbod < sp->ebod) {
                if ((n = (coff - soff + sp->ival - 1) / sp->ival) < 0)
                    n = 0;
                nt = cbod + soff + n * sp->ival;
                while (nt <= sp->nt)
                    nt += sp->ival;
                noff = nt - cbod;
                switch (sp->mode) {
                case 2:
                    if (noff >= soff && noff <= eoff)
                        goto found;
                    tmc.tm_mday++;
                    tmc.tm_hour = tmc.tm_min = tmc.tm_sec = 0;
                    tt = tmtime (&tmc, TM_LOCALZONE);
                    coff = 0, cbod = tt - coff;
                    break;
                case 3:
                    if (
                        (SWD (tmc, tm1) && noff >= soff && noff <= eoff) ||
                        (SWD (tmc, tm2) && noff <= sp->eoff)
                    )
                        goto found;
                    tmc.tm_mday += 7;
                    tmc.tm_hour = tmc.tm_min = tmc.tm_sec = 0;
                    tt = tmtime (&tmc, TM_LOCALZONE);
                    coff = 0, cbod = tt - coff;
                    break;
                case 4:
                    if (
                        (SMD (tmc, tm1) && noff >= soff && noff <= eoff) ||
                        (SMD (tmc, tm2) && noff <= sp->eoff)
                    )
                        goto found;
                    tmc.tm_mon++;
                    tmc.tm_hour = tmc.tm_min = tmc.tm_sec = 0;
                    tt = tmtime (&tmc, TM_LOCALZONE);
                    coff = 0, cbod = tt - coff;
                    break;
                default:
                    SUwarning (
                        0, "scheduleupdate", "unknown mode %d", sp->mode
                    );
                    return -1;
                }
            }
            nt = 0;
        }
found:
        if (nt == 0) {
            SUwarning (
                0, "scheduleupdate", "failed to schedule id %s", sp->jp->id
            );
            sp->expired = TRUE;
            continue;
        }
        if (nt > sp->ebod + eoff) {
            sp->expired = TRUE;
            continue;
        }
        sp->nt = nt, sp->scheduled = TRUE;
        if (minnt > sp->nt)
            minnt = sp->nt;
    }
    if (minnt == INT_MAX)
        return -1;

    return (minnt > ct) ? minnt - ct : 0;
}
