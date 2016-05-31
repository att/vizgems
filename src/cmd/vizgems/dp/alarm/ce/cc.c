#pragma prototyped

#include <ast.h>
#include <swift.h>
#include "pmem.h"
#include "eeval.h"
#include "vgce.h"

int insertcc (void) {
    cc_t *ccp;
    int cci;

    for (cci = rootp->ccl; cci < rootp->ccn; cci++)
        if (!rootp->ccs[cci].inuse)
            goto found;
    for (cci = 0; cci < rootp->ccl; cci++)
        if (!rootp->ccs[cci].inuse)
            goto found;
    SUwarning (0, "insertcc", "extending cc array");
    if (!(rootp->ccs = PMresize (
        rootp->ccs, sizeof (cc_t) * (rootp->ccn + CE_ROOT_CCINCR)
    ))) {
        SUwarning (0, "insertcc", "cannot grow cc array");
        return -1;
    }
    memset (&rootp->ccs[rootp->ccn], 0, sizeof (cc_t) * CE_ROOT_CCINCR);
    cci = rootp->ccn;
    rootp->ccn += CE_ROOT_CCINCR;

found:
    rootp->ccl = cci + 1;
    memset (&rootp->ccs[cci], 0, sizeof (cc_t));
    if (rootp->ccl > rootp->ccn)
        rootp->ccl = 0;
    ccp = &rootp->ccs[cci];
    ccp->inuse = TRUE;

    if (!(ccp->matchs = PMalloc (sizeof (match_t) * CE_CC_MATCHINCR))) {
        SUwarning (0, "insertcc", "cannot allocate match array");
        return -1;
    }
    memset (&ccp->matchs[0], 0, sizeof (match_t) * CE_CC_MATCHINCR);
    ccp->matchn = CE_CC_MATCHINCR;

    rootp->ccm++;

    VG_warning (0, "DATA INFO", "inserted cc cc=%d", cci);

    return cci;
}

int removecc (int cci) {
    cc_t *ccp;

    ccp = &rootp->ccs[cci];
    if (!PMfree (ccp->matchs)) {
        SUwarning (0, "removecc", "cannot free match array");
        return -1;
    }

    memset (ccp, 0, sizeof (cc_t));

    rootp->ccm--;

    VG_warning (0, "DATA INFO", "removed cc cc=%d", cci);

    return 0;
}

match_t *insertccmatch (int cci) {
    cc_t *ccp;
    match_t *matchp;
    int matchi;

    ccp = &rootp->ccs[cci];
    for (matchi = 0; matchi < ccp->matchn; matchi++)
        if (!ccp->matchs[matchi].inuse)
            break;
    if (matchi == ccp->matchn) {
        if (!(ccp->matchs = PMresize (
            ccp->matchs, sizeof (match_t) * (ccp->matchn + CE_CC_MATCHINCR)
        ))) {
            SUwarning (0, "insertccmatch", "cannot grow match array");
            return NULL;
        }
        memset (
            &ccp->matchs[ccp->matchn], 0, sizeof (match_t) * CE_CC_MATCHINCR
        );
        ccp->matchn += CE_CC_MATCHINCR;
    } else
        memset (&ccp->matchs[matchi], 0, sizeof (match_t));

    matchp = &ccp->matchs[matchi];
    matchp->inuse = TRUE;

    ccp->matchm++;

    return matchp;
}

int removeccmatch (int cci, match_t *matchp) {
    cc_t *ccp;
    int matchi;

    ccp = &rootp->ccs[cci];
    for (matchi = 0; matchi < ccp->matchn; matchi++)
        if (ccp->matchs[matchi].inuse && matchp == &ccp->matchs[matchi])
            break;
    if (matchi == ccp->matchn) {
        SUwarning (0, "removeccmatch", "cannot find match");
        return -1;
    }

    memset (matchp, 0, sizeof (match_t));

    ccp->matchm--;

    return 0;
}

void printccs (int lod) {
    cc_t *ccp;
    int cci;
    match_t *matchp;
    int matchi;

    if (lod <= 1)
        return;

    sfprintf (
        sfstderr, "ccs: m=%d l=%d n=%d\n", rootp->ccm, rootp->ccl, rootp->ccn
    );

    if (lod <= 2)
        return;

    for (cci = 0; cci < rootp->ccl; cci++) {
        ccp = &rootp->ccs[cci];
        if (!ccp->inuse)
            continue;
        sfprintf (
            sfstderr, "  cc: cc=%d n=%d/%d\n", cci, ccp->matchm, ccp->matchn
        );
        for (matchi = 0; matchi < ccp->matchn; matchi++) {
            matchp = &ccp->matchs[matchi];
            if (!matchp->inuse)
                continue;
            printmatch (matchp, lod);
        }
    }
}
