#pragma prototyped

#include <ast.h>
#include <vmalloc.h>
#include <swift.h>
#include <ctype.h>
#include "xml.h"

#define nc(p, fp) ( \
    ((p > iep) ? (p = fillibuf (fp)) : p), *p++ \
)
#define IBUFSIZE 10240
static char ibuf[IBUFSIZE];
static char *ibp, *iep, *icp;

#define XBUFINCR 10240
static char *xbuf;
static int xn, xi;

static char *fillibuf (Sfio_t *);

static int growxbuf (void);

XMLnode_t *XMLflattenread (Sfio_t *ifp, int iwsflag, Vmalloc_t *vm) {
    int xftbi, xftei, xltbi, xltei, xj, xk, c, done;

    if (!ibp)
        ibp = ibuf, iep = ibuf + IBUFSIZE - 1, icp = iep + 1;

    xi = 0;
    while ((c = nc (icp, ifp)) != '<') {
        if (c == 0)
            return 0;
        if (isspace (c))
            continue;
    }
    xftbi = xi;
    if (xi >= xn && growxbuf () == -1)
        return NULL;
    xbuf[xi++] = c;

    while ((c = nc (icp, ifp)) != '>') {
        if (c == 0) {
            SUwarning (0, "XMLflattenread", "EOF before close of first tag");
            return NULL;
        }
        if (xi >= xn && growxbuf () == -1)
            return NULL;
        xbuf[xi++] = c;
    }
    xftei = xi;
    if (xi >= xn && growxbuf () == -1)
        return NULL;
    xbuf[xi++] = c;

    done = FALSE;
    while (!done) {
        xltbi = xltei = -1;
        while ((c = nc (icp, ifp)) != '>') {
            if (c == 0) {
                SUwarning (0, "XMLflattenread", "EOF before close of tag");
                return NULL;
            }
            if (c == '<')
                xltbi = xi;
            if (xi >= xn && growxbuf () == -1)
                return NULL;
            xbuf[xi++] = c;
        }
        xltei = xi;
        if (xi >= xn && growxbuf () == -1)
            return NULL;
        xbuf[xi++] = c;
        if (xltbi == -1 || xbuf[xltbi + 1] != '/')
            continue;
        done = TRUE;
        for (
            xj = xftbi + 1, xk = xltbi + 2;
            xj < xftei && xk < xltei; xj++, xk++
        ) {
            if (xbuf[xj] != xbuf[xk]) {
                done = FALSE;
                break;
            }
        }
    }

    if (!done) {
        SUwarning (0, "XMLflattenread", "cannot find complete record");
        return NULL;
    }

    if (xi >= xn && growxbuf () == -1)
        return NULL;
    xbuf[xi++] = 0;

    return XMLreadstring (xbuf, "flat", iwsflag, vm);
}

int XMLflattenwrite (Sfio_t *fp, XMLnode_t *nodep, char *rectag, int srflag) {
    XMLnode_t *fnp, *nip, *njp;
    int ni, nj;

    if (nodep->type != XML_TYPE_TAG) {
        SUwarning (0, "XMLflattenwrite", "cannot operate on text node");
        return -1;
    }
    if (nodep->nodem != 1) {
        SUwarning (0, "XMLflattenwrite", "cannot operate on multichild nodes");
        return -1;
    }

    nodep = nodep->nodes[0];
    fnp = NULL;
    if (rectag && rectag[0] != 0) {
        for (ni = 0; ni < nodep->nodem; ni++) {
            nip = nodep->nodes[ni];
            if (nip->type == XML_TYPE_TAG && strcmp (nip->tag, rectag) == 0) {
                fnp = nip;
                break;
            }
        }
    }

    if (fnp) {
        for (nj = 0; nj < fnp->nodem; nj++) {
            if (srflag)
                sfprintf (fp, "%s=(", nodep->tag);
            else
                sfprintf (fp, "(");
            njp = fnp->nodes[nj];
            njp->indexplusone = 1;
            for (ni = 0; ni < nodep->nodem; ni++) {
                nip = nodep->nodes[ni];
                if (nip == fnp)
                    continue;
                XMLwriteksh (fp, nip, 1, TRUE);
            }
            XMLwriteksh (fp, njp, 1, TRUE);
            sfprintf (fp, ")\n");
        }
    } else {
        if (srflag)
            sfprintf (fp, "%s=(", nodep->tag);
        else
            sfprintf (fp, "(");
        for (ni = 0; ni < nodep->nodem; ni++) {
            nip = nodep->nodes[ni];
            XMLwriteksh (fp, nip, 1, TRUE);
        }
        sfprintf (fp, ")\n");
    }

    return 0;
}

static char *fillibuf (Sfio_t *ifp) {
    ssize_t ilen;

    if ((ilen = sfread (ifp, ibuf, IBUFSIZE)) == -1) {
        SUwarning (0, "fillibuf", "cannot fill ibuf");
        ibuf[0] = 0;
        return ibuf;
    }

    if (ilen < IBUFSIZE)
        ibuf[ilen] = 0;

    return ibuf;
}

static int growxbuf (void) {
    if (!(xbuf = vmresize (Vmheap, xbuf, (xn + XBUFINCR), VM_RSCOPY))) {
        SUwarning (0, "growxbuf", "cannot grow xbuf");
        return -1;
    }
    xn += XBUFINCR;
    return 0;
}
