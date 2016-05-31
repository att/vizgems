#pragma prototyped

#include <ast.h>
#include <cdt.h>
#include <vmalloc.h>
#include <swift.h>
#include "vggce.h"
#include "sl_inv_nodeattr.c"
#include "sl_inv_edge.c"
#include "sl_inv_cc2nd.c"

Dt_t *ccdict;
Dt_t *nddict;
int ccmark, ndmark;

static Dtdisc_t nddisc = {
    sizeof (Dtlink_t), SZ_level + SZ_id, 0, NULL, NULL, NULL, NULL, NULL, NULL
};
static Dtdisc_t ccdisc = {
    sizeof (Dtlink_t), SZ_level + SZ_id, 0, NULL, NULL, NULL, NULL, NULL, NULL
};

int initgraph (void) {
    if (!(ccdict = dtopen (&ccdisc, Dtset))) {
        SUwarning (0, "initgraph", "cc dtopen failed");
        return -1;
    }

    if (!(nddict = dtopen (&nddisc, Dtset))) {
        SUwarning (0, "initgraph", "nd dtopen failed");
        return -1;
    }

    sl_inv_nodeattropen (getenv ("INVNODEATTRFILE"));
    sl_inv_edgeopen (getenv ("INVEDGEFILE"));
    sl_inv_cc2ndopen (getenv ("INVCC2NDFILE"));

    ccmark = 0;

    return 0;
}

int termgraph (void) {
    if (dtclose (ccdict) == -1) {
        SUwarning (0, "termgraph", "cc dtclose failed");
        return -1;
    }

    if (dtclose (nddict) == -1) {
        SUwarning (0, "termgraph", "nd dtclose failed");
        return -1;
    }

    return 0;
}

cc_t *insertcc (char *level, char *id, int fullflag) {
    cc_t ccmem, *ccp;
    nd_t *ndp;
    int ndpi;
    int svcmi;
    sl_inv_cc2nd_t *cc2ndp;

    memcpy (&ccmem.level, level, SZ_level);
    memcpy (&ccmem.id, id, SZ_id);
    if (!(ccp = dtsearch (ccdict, &ccmem))) {
        if (!(ccp = vmalloc (Vmheap, sizeof (cc_t)))) {
            SUwarning (0, "insertcc", "cc malloc failed");
            return NULL;
        }
        memset (ccp, 0, sizeof (cc_t));
        memcpy (ccp->level, level, SZ_level);
        memcpy (ccp->id, id, SZ_id);
        if (dtinsert (ccdict, ccp) != ccp) {
            SUwarning (0, "insertcc", "cc dtinsert failed");
            return NULL;
        }
    }
    ccp->mark = ccmark;

    if (ccp->fullflag || !fullflag)
        return ccp;

    for (
        cc2ndp = sl_inv_cc2ndfindfirst (level, id); cc2ndp;
        cc2ndp = sl_inv_cc2ndfindnext (level, id)
    ) {
        if (ccp->ndpm >= ccp->ndpn) {
            if (!(ccp->ndps = vmresize (
                Vmheap, ccp->ndps, (ccp->ndpn + 10) * sizeof (nd_t *), VM_RSCOPY
            ))) {
                SUwarning (0, "insertcc", "ndps resize failed");
                return NULL;
            }
            ccp->ndpn += 10;
        }
        if (!(ccp->ndps[ccp->ndpm] = insertnd (
            cc2ndp->sl_ndlevel, cc2ndp->sl_ndid, ccp, TRUE
        ))) {
            SUwarning (0, "insertcc", "insertnd failed");
            return NULL;
        }
        ccp->ndps[ccp->ndpm]->index = ccp->ndpm++;
    }
    ccp->ndsti = 0;
    if (!(ccp->ndsts = vmalloc (Vmheap, ccp->ndpm * sizeof (nd_t *)))) {
        SUwarning (0, "insertcc", "ndsts malloc failed");
        return NULL;
    }
    memset (ccp->ndsts, 0, ccp->ndpm * sizeof (nd_t *));

    svcmark++;
    for (ndpi = 0; ndpi < ccp->ndpm; ndpi++) {
        ndp = ccp->ndps[ndpi];
        if (insertndedges (ndp) == -1) {
            SUwarning (0, "insertcc", "insertndedges failed");
            return NULL;
        }
        for (svcmi = 0; svcmi < ndp->svcmm; svcmi++) {
            if (ndp->svcms[svcmi].svcp->mark == svcmark)
                continue;
            ndp->svcms[svcmi].svcp->mark = svcmark;
            if (ccp->svcpm >= ccp->svcpn) {
                if (!(ccp->svcps = vmresize (
                    Vmheap, ccp->svcps, (ccp->svcpn + 10) * sizeof (svc_t *),
                    VM_RSCOPY
                ))) {
                    SUwarning (0, "insertcc", "svcps resize failed");
                    return NULL;
                }
                ccp->svcpn += 10;
            }
            ccp->svcps[ccp->svcpm++] = ndp->svcms[svcmi].svcp;
        }
    }

    ccp->fullflag = TRUE;

    return ccp;
}

cc_t *findcc (char *level, char *id) {
    cc_t ccmem;

    memcpy (&ccmem.level, level, SZ_level);
    memcpy (&ccmem.id, id, SZ_id);
    return dtsearch (ccdict, &ccmem);
}

nd_t *insertnd (char *level, char *id, cc_t *ccp, int fullflag) {
    nd_t ndmem, *ndp;
    svcm_t *svcmp;
    int svcmi;
    int role;
    char *name;
    sl_inv_nodeattr_t ina, *inap;

    memcpy (&ndmem.level, level, SZ_level);
    memcpy (&ndmem.id, id, SZ_id);
    if (!(ndp = dtsearch (nddict, &ndmem))) {
        if (!(ndp = vmalloc (Vmheap, sizeof (nd_t)))) {
            SUwarning (0, "insertnd", "nd malloc failed");
            return NULL;
        }
        memset (ndp, 0, sizeof (nd_t));
        memcpy (ndp->level, level, SZ_level);
        memcpy (ndp->id, id, SZ_id);
        if (dtinsert (nddict, ndp) != ndp) {
            SUwarning (0, "insertnd", "nd dtinsert failed");
            return NULL;
        }
    }
    ndp->mark = ndmark;

    if (ndp->fullflag || !fullflag)
        return ndp;

    ndp->ccp = ccp;
    for (svcmi = 0; ; svcmi++) {
        memset (ina.sl_key, 0, sizeof (ina.sl_key));
        sfsprintf (ina.sl_key, sizeof (ina.sl_key), "gce_service%d", svcmi);
        if (!(inap = sl_inv_nodeattrfind (level, id, ina.sl_key)))
            break;

        if (strncmp (inap->sl_val, "prod:", 5) == 0)
            role = SVC_ROLE_PROD, name = inap->sl_val + 5;
        else if (strncmp (inap->sl_val, "cons:", 5) == 0)
            role = SVC_ROLE_CONS, name = inap->sl_val + 5;
        else
            continue;
        if (ndp->svcmm >= ndp->svcmn) {
            if (!(ndp->svcms = vmresize (
                Vmheap, ndp->svcms, (ndp->svcmn + 4) * sizeof (svcm_t),
                VM_RSCOPY
            ))) {
                SUwarning (0, "insertnd", "svcms resize failed");
                return NULL;
            }
            ndp->svcmn += 4;
        }
        svcmp = &ndp->svcms[ndp->svcmm++];
        memset (svcmp, 0, sizeof (svcm_t));
        if (!(svcmp->svcp = insertsvc (name))) {
            SUwarning (0, "insertnd", "insertsvc failed");
            return NULL;
        }
        svcmp->role = role;
    }

    ndp->fullflag = TRUE;

    return ndp;
}

nd_t *findnd (char *level, char *id) {
    nd_t ndmem;

    memcpy (&ndmem.level, level, SZ_level);
    memcpy (&ndmem.id, id, SZ_id);
    return dtsearch (nddict, &ndmem);
}

int insertndedges (nd_t *ndp) {
    sl_inv_edge_t *iep;
    ed_t *edp;

    ndp->edn = 0;
    for (
        iep = sl_inv_edgefindfirst (ndp->level, ndp->id); iep;
        iep = sl_inv_edgefindnext (ndp->level, ndp->id)
    ) {
        if (ndp->edm >= ndp->edn) {
            if (!(ndp->eds = vmresize (
                Vmheap, ndp->eds, (ndp->edn + 16) * sizeof (ed_t), VM_RSCOPY
            ))) {
                SUwarning (0, "insertndedges", "eds resize failed");
                return -1;
            }
            ndp->edn += 16;
        }
        edp = &ndp->eds[ndp->edm++];
        memset (edp, 0, sizeof (ed_t));
        if (!(edp->ndp = findnd (iep->sl_level2, iep->sl_id2))) {
            SUwarning (0, "insertndedges", "findnd failed");
            return -1;
        }
    }

    return 0;
}

int insertndevent (nd_t *ndp, ev_t *evp) {
    if (ndp->evpm >= ndp->evpn) {
        if (!(ndp->evps = vmresize (
            Vmheap, ndp->evps, (ndp->evpn + 16) * sizeof (ev_t *), VM_RSCOPY
        ))) {
            SUwarning (0, "insertndevent", "evps resize failed");
            return -1;
        }
        ndp->evpn += 16;
    }
    ndp->evps[ndp->evpm++] = evp;

    return 0;
}
