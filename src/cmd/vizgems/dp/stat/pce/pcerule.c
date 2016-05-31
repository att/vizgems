#pragma prototyped

#include <ast.h>
#include <vmalloc.h>
#include <swift.h>
#include "vgpce.h"

static char *rulestr;
static int rulelen;

int ruleinit (void) {
    return 0;
}

int ruleterm (void) {
    if (rulestr)
        vmfree (Vmheap, rulestr);

    return 0;
}

rule_t *ruleinsert (char *spec) {
    rule_t *rp;
    rulerange_t *rrp;
    char *s1, *s2, *s3, *s4, *s5, *s6, *s7, *s8, *s9, *s10, *s11;
    int rn, crn, vrn;
    int maxn;
    int len;

    for (s1 = spec, crn = 0, vrn = 0, rn = 0; s1; ) {
        if (strstr (s1, "CLEAR") == s1)
           vrn++;
        if (strstr (s1, "VR") == s1)
           crn++;
        if ((s1 = strchr (s1, ';')))
            s1++, rn++;
    }
    if (spec[0])
        rn++;
    if (rn == 0)
        return NULL;
    if (crn == 0)
        rn++; // add a default CLEAR
    if (vrn == 0)
        rn--; // no value range spec

    if ((len = strlen (spec) + 1) > rulelen) {
        if (!(rulestr = vmresize (Vmheap, rulestr, len, VM_RSMOVE))) {
            SUwarning (0, "ruleinsert", "cannot grow rulestr");
            rulelen = 0;
            return NULL;
        }
        rulelen = len;
    }
    strcpy (rulestr, spec);

    if (!(rp = vmalloc (Vmheap, sizeof (rule_t)))) {
        SUwarning (0, "ruleinsert", "cannot allocate rule");
        return NULL;
    }
    memset (rp, 0, sizeof (rule_t));
    if (!(rp->ranges = vmalloc (Vmheap, rn * sizeof (rulerange_t)))) {
        SUwarning (0, "ruleinsert", "cannot allocate ranges");
        return NULL;
    }
    memset (rp->ranges, 0, rn * sizeof (rulerange_t));
    rrp = &rp->ranges[0];
    maxn = 0;
    rp->minop = RROP_NONE, rp->maxop = RROP_NONE;
    for (s1 = rulestr; s1; s1 = s2) {
        if ((s2 = strchr (s1, ';')))
            *s2++ = 0;
        if (strstr (s1, "VR") == s1) {
            if (
                !(s3 = strchr (s1, ':')) || !(s4 = strchr (s3 + 1, ':')) ||
                !(s5 = strchr (s4 + 1, ':')) || !(s6 = strchr (s5 + 1, ':'))
            ) {
                SUwarning (0, "ruleinsert", "bad VR spec %s", s1);
                continue;
            }
            *s3++ = 0, *s4++ = 0, *s5++ = 0, *s6++ = 0;
            if (s3[0] == 'n')
                rp->minop = ROP_NONE;
            else if (s3[0] == 'i')
                rp->minop = ROP_INCL;
            else if (s3[0] == 'e')
                rp->minop = ROP_EXCL;
            else {
                SUwarning (0, "ruleinsert", "bad min val op %s", s3);
                continue;
            }
            rp->minval = atof (s4);
            if (s5[0] == 'n')
                rp->maxop = ROP_NONE;
            else if (s5[0] == 'i')
                rp->maxop = ROP_INCL;
            else if (s5[0] == 'e')
                rp->maxop = ROP_EXCL;
            else {
                SUwarning (0, "ruleinsert", "bad max val op %s", s5);
                continue;
            }
            rp->maxval = atof (s6);
        } else if (strstr (s1, "CLEAR") == s1) {
            if (
                !(s3 = strchr (s1, ':')) || !(s4 = strchr (s3 + 1, ':')) ||
                !(s5 = strchr (s4 + 1, ':'))
            ) {
                SUwarning (0, "ruleinsert", "bad CLEAR spec %s", s1);
                continue;
            }
            *s3++ = 0, *s4++ = 0, *s5++ = 0;
            rrp->type = RRTYPE_CLEAR;
            rrp->minop = RROP_NONE;
            rrp->maxop = RROP_NONE;
            rrp->sev = atoi (s3);
            rrp->m = atoi (s4);
            rrp->n = atoi (s5);
            if (maxn < rrp->n)
                maxn = rrp->n;
            if (!rp->clearrangep)
                rp->clearrangep = rrp;
            rp->rangen++;
            rrp = &rp->ranges[rp->rangen];
        } else if (strstr (s1, "SIGMA") == s1) {
            if (
                !(s3 = strchr (s1, ':')) || !(s4 = strchr (s3 + 1, ':')) ||
                !(s5 = strchr (s4 + 1, ':')) || !(s6 = strchr (s5 + 1, ':')) ||
                !(s7 = strchr (s6 + 1, ':')) || !(s8 = strchr (s7 + 1, ':')) ||
                !(s9 = strchr (s8 + 1, ':')) || !(s10 = strchr (s9 + 1, ':')) ||
                !(s11 = strchr (s10 + 1, ':'))
            ) {
                SUwarning (0, "ruleinsert", "bad ALARM SIGMA spec %s", s1);
                continue;
            }
            *s3++ = 0, *s4++ = 0, *s5++ = 0, *s6++ = 0;
            *s7++ = 0, *s8++ = 0, *s9++ = 0, *s10++ = 0, *s11++ = 0;
            rrp->type = RRTYPE_SIGMA;
            if (s3[0] == 'n')
                rrp->minop = RROP_NONE;
            else if (s3[0] == 'i')
                rrp->minop = RROP_INCL;
            else if (s3[0] == 'e')
                rrp->minop = RROP_EXCL;
            else {
                SUwarning (0, "ruleinsert", "bad min sigma op %s", s3);
                continue;
            }
            rrp->minval = atof (s4);
            if (s5[0] == 'n')
                rrp->maxop = RROP_NONE;
            else if (s5[0] == 'i')
                rrp->maxop = RROP_INCL;
            else if (s5[0] == 'e')
                rrp->maxop = RROP_EXCL;
            else {
                SUwarning (0, "ruleinsert", "bad max sigma op %s", s5);
                continue;
            }
            rrp->maxval = atof (s6);
            rrp->sev = atoi (s7);
            rrp->m = atoi (s8);
            rrp->n = atoi (s9);
            rrp->freq = atoi (s10);
            rrp->ival = atoi (s11);
            if (rrp->freq != 1) {
                if (rrp->freq == 0)
                    rrp->freq = 1;
                rrp->ival /= rrp->freq;
                if (rrp->ival < 15)
                    rrp->ival = 15;
            }
            if (maxn < rrp->n)
                maxn = rrp->n;
            rp->rangen++;
            rrp = &rp->ranges[rp->rangen];
        } else if (strstr (s1, "ACTUAL") == s1) {
            if (
                !(s3 = strchr (s1, ':')) || !(s4 = strchr (s3 + 1, ':')) ||
                !(s5 = strchr (s4 + 1, ':')) || !(s6 = strchr (s5 + 1, ':')) ||
                !(s7 = strchr (s6 + 1, ':')) || !(s8 = strchr (s7 + 1, ':')) ||
                !(s9 = strchr (s8 + 1, ':')) || !(s10 = strchr (s9 + 1, ':')) ||
                !(s11 = strchr (s10 + 1, ':'))
            ) {
                SUwarning (0, "ruleinsert", "bad ALARM ACTUAL spec %s", s1);
                continue;
            }
            *s3++ = 0, *s4++ = 0, *s5++ = 0, *s6++ = 0;
            *s7++ = 0, *s8++ = 0, *s9++ = 0, *s10++ = 0, *s11++ = 0;
            rrp->type = RRTYPE_ACTUAL;
            if (s3[0] == 'n')
                rrp->minop = RROP_NONE;
            else if (s3[0] == 'i')
                rrp->minop = RROP_INCL;
            else if (s3[0] == 'e')
                rrp->minop = RROP_EXCL;
            else {
                SUwarning (0, "ruleinsert", "bad min actual op %s", s3);
                continue;
            }
            rrp->minval = atof (s4);
            if (s5[0] == 'n')
                rrp->maxop = RROP_NONE;
            else if (s5[0] == 'i')
                rrp->maxop = RROP_INCL;
            else if (s5[0] == 'e')
                rrp->maxop = RROP_EXCL;
            else {
                SUwarning (0, "ruleinsert", "bad max actual op %s", s5);
                continue;
            }
            rrp->maxval = atof (s6);
            rrp->sev = atoi (s7);
            rrp->m = atoi (s8);
            rrp->n = atoi (s9);
            rrp->freq = atoi (s10);
            rrp->ival = atoi (s11);
            if (rrp->freq != 1) {
                if (rrp->freq == 0)
                    rrp->freq = 1;
                rrp->ival /= rrp->freq;
                if (rrp->ival < 15)
                    rrp->ival = 15;
            }
            if (maxn < rrp->n)
                maxn = rrp->n;
            rp->rangen++;
            rrp = &rp->ranges[rp->rangen];
        } else {
            SUwarning (0, "ruleinsert", "bad spec type %s", s1);
            continue;
        }
    }
    if (maxn == 0)
        maxn = 1;
    if (crn == 0) {
        rrp->type = RRTYPE_CLEAR;
        rrp->minop = RROP_NONE;
        rrp->maxop = RROP_NONE;
        rrp->sev = 5;
        rrp->m = maxn;
        rrp->n = maxn;
        rp->clearrangep = rrp;
    }
    rp->maxn = maxn;

    return rp;
}

int ruleremove (rule_t *rp) {
    if (rp->ranges)
        vmfree (Vmheap, rp->ranges);
    vmfree (Vmheap, rp);
    return 0;
}

rulerange_t *rulematch (
    rule_t *rp, float curr, float mean, float sdev, float *vp
) {
    rulerange_t *rrp;
    int rri;
    float sigma, actual;

    if (
        (rp->minop == ROP_INCL && curr < rp->minval) ||
        (rp->minop == ROP_EXCL && curr <= rp->minval) ||
        (rp->maxop == ROP_INCL && curr > rp->maxval) ||
        (rp->maxop == ROP_EXCL && curr >= rp->maxval)
    )
        return NULL;
    if (!ALMOSTZERO (sdev))
        sigma = (curr - mean) / sdev;
    else if (!ALMOSTZERO (mean))
        sigma = 100.0 * (curr - mean) / mean;
    else
        sigma = (0.001 + curr) / 0.001;
    if (sigma < 0.0)
        sigma = -sigma;
    actual = curr - mean;
    if (actual < 0.0)
        actual = -actual;
    *vp = sigma;

    for (rri = 0; rri < rp->rangen; rri++) {
        rrp = &rp->ranges[rri];
        if (rrp == rp->clearrangep)
            continue;
        if (rrp->type == RRTYPE_SIGMA && (
            (rrp->minop == RROP_INCL && sigma >= rrp->minval) ||
            (rrp->minop == RROP_EXCL && sigma > rrp->minval) ||
            rrp->minop == RROP_NONE
        )) {
            *vp = sigma;
            return rrp;
        }
        if (rrp->type == RRTYPE_ACTUAL && (
            (rrp->minop == RROP_INCL && actual >= rrp->minval) ||
            (rrp->minop == RROP_EXCL && actual > rrp->minval) ||
            rrp->minop == RROP_NONE
        )) {
            *vp = actual;
            return rrp;
        }
    }
    return NULL;
}
