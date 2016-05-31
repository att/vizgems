#pragma prototyped

#include <ast.h>
#include <vmalloc.h>
#include <cdt.h>
#include <swift.h>
#include "tiger.h"

#if SWIFT_LITTLEENDIAN == 1
static int __i;
static short __s;
static edgedata_t __ed;
static xy_t __xy;
static polydata_t __pd;
static edge2polydata_t __e2pd;
static int __rtn;
static char *__cp, __c;
#define SWI(i) ( \
    __cp = (char *) &(i), \
    __c = __cp[0], __cp[0] = __cp[3], __cp[3] = __c, \
    __c = __cp[1], __cp[1] = __cp[2], __cp[2] = __c \
)
#define SWS(s) ( \
    __cp = (char *) &(s), \
    __c = __cp[0], __cp[0] = __cp[1], __cp[1] = __c \
)
#define RDI(f, i) ( \
    __rtn = sfread ((f), &(i), sizeof (int)), SWI (i), __rtn \
)
#define WRI(f, i) ( \
    __i = (i), SWI (__i), sfwrite ((f), &__i, sizeof (int)) \
)
#define RDS(f, s) ( \
    __rtn = sfread ((f), &(s), sizeof (short)), SWS (s), __rtn \
)
#define WRS(f, s) ( \
    __s = (s), SWS (__s), sfwrite ((f), &__s, sizeof (short)) \
)
#define RDED(f, ed) ( \
    __rtn = sfread ((f), &(ed), sizeof (edgedata_t)), \
    SWI (ed.tlid), \
    SWI (ed.nameid), \
    SWI (ed.zipl), SWI (ed.zipr), \
    SWI (ed.npanxxlocl), SWI (ed.npanxxlocr), \
    SWS (ed.countyl), SWS (ed.countyr), \
    SWI (ed.ctbnal), SWI (ed.ctbnar), \
    SWS (ed.blkl), SWS (ed.blkr), \
    SWI (ed.xy0.x), SWI (ed.xy0.y), \
    SWI (ed.xy1.x), SWI (ed.xy1.y), \
    SWS (ed.cfccid), __rtn \
)
#define WRED(f, ed) ( \
    __ed = (ed), \
    SWI (__ed.tlid), \
    SWI (__ed.nameid), \
    SWI (__ed.zipl), SWI (__ed.zipr), \
    SWI (__ed.npanxxlocl), SWI (__ed.npanxxlocr), \
    SWS (__ed.countyl), SWS (__ed.countyr), \
    SWI (__ed.ctbnal), SWI (__ed.ctbnar), \
    SWS (__ed.blkl), SWS (__ed.blkr), \
    SWI (__ed.xy0.x), SWI (__ed.xy0.y), \
    SWI (__ed.xy1.x), SWI (__ed.xy1.y), \
    SWS (__ed.cfccid), \
    sfwrite ((f), &__ed, sizeof (edgedata_t)) \
)
#define RDXY(f, xy) ( \
    __rtn = sfread ((f), &(xy), sizeof (xy_t)), \
    SWI (xy.x),  SWI (xy.y), __rtn \
)
#define WRXY(f, xy) ( \
    __xy = (xy), \
    SWI (__xy.x), SWI (__xy.y), \
    sfwrite ((f), &__xy, sizeof (xy_t)) \
)
#define RDPD(f, pd) ( \
    __rtn = sfread ((f), &(pd), sizeof (polydata_t)), \
    SWI (pd.cenid), SWI (pd.polyid), __rtn \
)
#define WRPD(f, pd) ( \
    __pd = (pd), \
    SWI (__pd.cenid), SWI (__pd.polyid), \
    sfwrite ((f), &__pd, sizeof (polydata_t)) \
)
#define RDE2PD(f, e2pd) ( \
    __rtn = sfread ((f), &(e2pd), sizeof (edge2polydata_t)), \
    SWI (e2pd.tlid), \
    SWI (e2pd.cenidl), SWI (e2pd.cenidr), \
    SWI (e2pd.polyidl), SWI (e2pd.polyidr), __rtn \
)
#define WRE2PD(f, e2pd) ( \
    __e2pd = (e2pd), \
    SWI (__e2pd.tlid), \
    SWI (__e2pd.cenidl), SWI (__e2pd.cenidr), \
    SWI (__e2pd.polyidl), SWI (__e2pd.polyidr), \
    sfwrite ((f), &__e2pd, sizeof (edge2polydata_t)) \
)
#else
#define SWI(i) (i)
#define SWS(s) (s)
#define RDI(f, i) ( \
    sfread ((f), &(i), sizeof (int)) \
)
#define WRI(f, i) ( \
    sfwrite ((f), &(i), sizeof (int)) \
)
#define RDS(f, s) ( \
    sfread ((f), &(s), sizeof (short)) \
)
#define WRS(f, s) ( \
    sfwrite ((f), &(s), sizeof (short)) \
)
#define RDED(f, ed) ( \
    sfread ((f), &(ed), sizeof (edgedata_t)) \
)
#define WRED(f, ed) ( \
    sfwrite ((f), &(ed), sizeof (edgedata_t)) \
)
#define RDXY(f, xy) ( \
    sfread ((f), &(xy), sizeof (xy_t)) \
)
#define WRXY(f, xy) ( \
    sfwrite ((f), &(xy), sizeof (xy_t)) \
)
#define RDPD(f, pd) ( \
    sfread ((f), &(pd), sizeof (polydata_t)) \
)
#define WRPD(f, pd) ( \
    sfwrite ((f), &(pd), sizeof (polydata_t)) \
)
#define RDE2PD(f, e2pd) ( \
    sfread ((f), &(e2pd), sizeof (edge2polydata_t)) \
)
#define WRE2PD(f, e2pd) ( \
    sfwrite ((f), &(e2pd), sizeof (edge2polydata_t)) \
)
#endif

int loaddata (char *dir) {
    Sfio_t *fp;
    char **sps;
    int spn, spi;
    char *ss;
    int sn;
    edgedata_t ed;
    edge_t *ep;
    polydata_t pd;
    edge2polydata_t e2pd;
    xy_t xy;
    int xyi, xyn;

    sps = NULL;
    ss = NULL;
    /* read strings */
    if (!(fp = sfopen (NULL, sfprints ("%s/strings.list", dir), "r")))
        goto abortloaddata;
    sfsetbuf (fp, NULL, 1048576);
    if (
        RDI (fp, spn) <= 0 ||
        (spn > 0 && !(sps = vmalloc (Vmheap, spn * sizeof (char *))))
    )
        goto abortloaddata;
    for (spi = 0; spi < spn; spi++)
        if (RDI (fp, sps[spi]) <= 0)
            goto abortloaddata;
    if (
        RDI (fp, sn) <= 0 ||
        (sn > 0 && !(ss = vmalloc (Vmheap, sn * sizeof (char)))) ||
        sfread (fp, ss, sn * sizeof (char)) == -1
    )
        goto abortloaddata;
    for (spi = 0; spi < spn; spi++)
        sps[spi] = ss + (int) sps[spi];
    sfclose (fp);
    /* read edges */
    if (!(fp = sfopen (NULL, sfprints ("%s/edges.list", dir), "r")))
        goto abortloaddata;
    sfsetbuf (fp, NULL, 1048576);
    while (RDED (fp, ed) > 0) {
        if (!(ep = createedge (&ed, sps[ed.nameid])))
            goto abortloaddata;
        if (ep->onesided == 0 && ed.onesided != 0) {
            if (RDI (fp, xyn) <= 0)
                goto abortloaddata;
            if (xyn == 0)
                continue;
            for (xyi = 0; xyi < xyn; xyi++)
                if (RDXY (fp, xy) <= 0)
                    goto abortloaddata;
            continue;
        }
        if (RDI (fp, ep->xyn) <= 0)
            goto abortloaddata;
        if (ep->xyn == 0)
            continue;
        if (!(ep->xys = vmalloc (Vmheap, ep->xyn * sizeof (xy_t))))
            goto abortloaddata;
        for (xyi = 0; xyi < ep->xyn; xyi++)
            if (RDXY (fp, ep->xys[xyi]) <= 0)
                goto abortloaddata;
    }
    sfclose (fp);
    if (!(fp = sfopen (NULL, sfprints ("%s/polys.list", dir), "r")))
        goto abortloaddata;
    sfsetbuf (fp, NULL, 1048576);
    while (RDPD (fp, pd) > 0) {
        if (!createpoly (&pd))
            goto abortloaddata;
    }
    sfclose (fp);
    if (!(fp = sfopen (NULL, sfprints ("%s/edge2polys.list", dir), "r")))
        goto abortloaddata;
    sfsetbuf (fp, NULL, 1048576);
    while (RDE2PD (fp, e2pd) > 0) {
        if (linkedgenpolys (&e2pd) == -1)
            goto abortloaddata;
    }
    sfclose (fp);
    vmfree (Vmheap, sps);
    vmfree (Vmheap, ss);
    return 0;

abortloaddata:
    SUwarning (1, "loaddata", "load failed for directory %s", dir);
    if (fp)
        sfclose (fp);
    if (sps)
        vmfree (Vmheap, sps);
    if (ss)
        vmfree (Vmheap, ss);
    return -1;
}

int loadattrboundaries (char *dir, int attr) {
    Sfio_t *fp;
    char **sps;
    int spn, spi;
    char *ss;
    int sn;
    edgedata_t ed;
    edge_t *ep;
    int v, xyn, xyi;

    sps = NULL;
    ss = NULL;
    /* read strings */
    if (!(fp = sfopen (NULL, sfprints ("%s/strings.list", dir), "r")))
        goto abortloadattrboundaries;
    sfsetbuf (fp, NULL, 1048576);
    if (
        RDI (fp, spn) <= 0 ||
        (spn > 0 && !(sps = vmalloc (Vmheap, spn * sizeof (char *))))
    )
        goto abortloadattrboundaries;
    for (spi = 0; spi < spn; spi++)
        if (RDI (fp, sps[spi]) <= 0)
            goto abortloadattrboundaries;
    if (
        RDI (fp, sn) <= 0 ||
        (sn > 0 && !(ss = vmalloc (Vmheap, sn * sizeof (char)))) ||
        sfread (fp, ss, sn * sizeof (char)) == -1
    )
        goto abortloadattrboundaries;
    for (spi = 0; spi < spn; spi++)
        sps[spi] = ss + (int) sps[spi];
    sfclose (fp);
    /* read edges */
    if (!(fp = sfopen (NULL, sfprints ("%s/edges.list", dir), "r")))
        goto abortloadattrboundaries;
    sfsetbuf (fp, NULL, 1048576);
    while (RDED (fp, ed) > 0) {
        if (ed.blkl < 100)
            ed.blkl = 0;
        if (ed.blkr < 100)
            ed.blkr = 0;
        switch (attr) {
        case T_EATTR_BLKS:
            v = (
                (ed.blksl == ed.blksr) && (ed.blkl == ed.blkr) &&
                (ed.ctbnal == ed.ctbnar) &&
                (ed.countyl == ed.countyr) && (ed.statel == ed.stater)
            );
            if (
                (ed.blkl == ed.blkr && ed.blkl == 0) &&
                (ed.ctbnal == ed.ctbnar && ed.ctbnal == 0)
            )
                v = 1;
            break;
        case T_EATTR_BLK:
            v = (
                (ed.blkl == ed.blkr) && (ed.ctbnal == ed.ctbnar) &&
                (ed.countyl == ed.countyr) && (ed.statel == ed.stater)
            );
            if (
                (ed.blkl == ed.blkr && ed.blkl == 0) &&
                (ed.ctbnal == ed.ctbnar && ed.ctbnal == 0)
            )
                v = 1;
            break;
        case T_EATTR_BLKG:
            v = (
                (ed.blkl / 100 == ed.blkr / 100) &&
                (ed.ctbnal == ed.ctbnar) &&
                (ed.countyl == ed.countyr) && (ed.statel == ed.stater)
            );
            if (
                (ed.blkl == ed.blkr && ed.blkl == 0) &&
                (ed.ctbnal == ed.ctbnar && ed.ctbnal == 0)
            )
                v = 1;
            break;
        case T_EATTR_CTBNA:
            v = (
                (ed.ctbnal == ed.ctbnar) &&
                (ed.countyl == ed.countyr) && (ed.statel == ed.stater)
            );
            if ((ed.ctbnal == ed.ctbnar && ed.ctbnal == 0))
                v = 1;
            break;
        case T_EATTR_COUNTY:
            v = (ed.countyl == ed.countyr) && (ed.statel == ed.stater);
            break;
        case T_EATTR_STATE:
            v = (ed.statel == ed.stater);
            break;
        case T_EATTR_ZIP:
            v = (ed.zipl == ed.zipr);
            break;
        case T_EATTR_NPANXXLOC:
            v = (ed.npanxxlocl == ed.npanxxlocr);
            break;
        case T_EATTR_COUNTRY:
            v = 1;
            break;
        }
        if (!v || ed.onesided) {
            if (!(ep = createedge (&ed, sps[ed.nameid])))
                goto abortloadattrboundaries;
            if (RDI (fp, ep->xyn) <= 0)
                goto abortloadattrboundaries;
            if (ep->xyn == 0)
                continue;
            if (!(ep->xys = vmalloc (Vmheap, ep->xyn * sizeof (xy_t))))
                goto abortloadattrboundaries;
            for (xyi = 0; xyi < ep->xyn; xyi++)
                if (RDXY (fp, ep->xys[xyi]) <= 0)
                    goto abortloadattrboundaries;
        } else {
            if (RDI (fp, xyn) <= 0)
                goto abortloadattrboundaries;
            if (xyn == 0)
                continue;
            if (sfseek (fp, xyn * sizeof (xy_t), 1) <= 0)
                goto abortloadattrboundaries;
        }
    }
    sfclose (fp);
    vmfree (Vmheap, sps);
    vmfree (Vmheap, ss);
    return 0;

abortloadattrboundaries:
    SUwarning (1, "loadattrboundaries", "load failed for directory %s", dir);
    if (fp)
        sfclose (fp);
    if (sps)
        vmfree (Vmheap, sps);
    if (ss)
        vmfree (Vmheap, ss);
    return -1;
}

int savedata (char *dir) {
    Sfio_t *fp;
    string_t *sp;
    char **sps;
    int spn, spi;
    int sl;
    edgedata_t ed;
    edge_t *ep;
    int xyi;
    polydata_t pd;
    poly_t *pp;
    edge2polydata_t e2pd;

    sps = NULL;
    if (!(sps = vmalloc (
        Vmheap, (spn = dtsize (stringdict)) * sizeof (string_t)
    )))
        goto abortsavedata;
    for (
        spi = 0, sp = (string_t *) dtflatten (stringdict); sp;
        sp = (string_t *) dtlink (stringdict, sp)
    )
        sps[(sp->id = spi++)] = sp->str;
    /* write strings */
    if (!(fp = sfopen (NULL, sfprints ("%s/strings.list", dir), "w")))
        goto abortsavedata;
    sfsetbuf (fp, NULL, 1048576);
    if (WRI (fp, spn) <= 0)
        goto abortsavedata;
    for (spi = 0, sl = 0; spi < spn; sl += strlen (sps[spi++]) + 1)
        if (WRI (fp, sl) <= 0)
            goto abortsavedata;
    if (WRI (fp, sl) <= 0)
        goto abortsavedata;
    for (spi = 0; spi < spn; spi++)
        if (sfputr (fp, sps[spi], 0) <= 0)
            goto abortsavedata;
    sfclose (fp);
    /* write edges */
    if (!(fp = sfopen (NULL, sfprints ("%s/edges.list", dir), "w")))
        goto abortsavedata;
    sfsetbuf (fp, NULL, 1048576);
    for (
        ep = (edge_t *) dtflatten (edgedict); ep;
        ep = (edge_t *) dtlink (edgedict, ep)
    ) {
        if (!ep->onesided && (!ep->p0p || !ep->p1p)) {
            ep->onesided = TRUE;
            if (!ep->p0p) {
                ep->zipl = 0;
                ep->npanxxlocl = -1;
                ep->statel = 0;
                ep->countyl = 0;
                ep->ctbnal = 0;
                ep->blkl = 0;
                ep->blksl = 32;
            }
            if (!ep->p1p) {
                ep->zipr = 0;
                ep->npanxxlocr = -1;
                ep->stater = 0;
                ep->countyr = 0;
                ep->ctbnar = 0;
                ep->blkr = 0;
                ep->blksr = 32;
            }
        }
        ed.tlid = ep->tlid;
        ed.onesided = ep->onesided;
        ed.nameid = ep->sp->id;
        ed.cfccid = ep->cfccid;
        ed.zipl = ep->zipl, ed.zipr = ep->zipr;
        ed.npanxxlocl = ep->npanxxlocl, ed.npanxxlocr = ep->npanxxlocr;
        ed.statel = ep->statel, ed.stater = ep->stater;
        ed.countyl = ep->countyl, ed.countyr = ep->countyr;
        ed.ctbnal = ep->ctbnal, ed.ctbnar = ep->ctbnar;
        ed.blkl = ep->blkl, ed.blkr = ep->blkr;
        ed.blksl = ep->blksl, ed.blksr = ep->blksr;
        ed.xy0.x = ep->v0p->xy.x, ed.xy0.y = ep->v0p->xy.y;
        ed.xy1.x = ep->v1p->xy.x, ed.xy1.y = ep->v1p->xy.y;
        if (WRED (fp, ed) <= 0)
            goto abortsavedata;
        if (WRI (fp, ep->xyn) <= 0)
            goto abortsavedata;
        if (ep->xyn > 0) {
            for (xyi = 0; xyi < ep->xyn; xyi++)
                if (WRXY (fp, ep->xys[xyi]) <= 0)
                    goto abortsavedata;
        }
    }
    sfclose (fp);
    if (!(fp = sfopen (NULL, sfprints ("%s/polys.list", dir), "w")))
        goto abortsavedata;
    sfsetbuf (fp, NULL, 1048576);
    for (
        pp = (poly_t *) dtflatten (polydict); pp;
        pp = (poly_t *) dtlink (polydict, pp)
    ) {
        pd.cenid = pp->cenid;
        pd.polyid = pp->polyid;
        if (WRPD (fp, pd) <= 0)
            goto abortsavedata;
    }
    sfclose (fp);
    if (!(fp = sfopen (NULL, sfprints ("%s/edge2polys.list", dir), "w")))
        goto abortsavedata;
    sfsetbuf (fp, NULL, 1048576);
    for (
        ep = (edge_t *) dtflatten (edgedict); ep;
        ep = (edge_t *) dtlink (edgedict, ep)
    ) {
        e2pd.tlid = ep->tlid;
        if (ep->p0p)
            e2pd.cenidl = ep->p0p->cenid, e2pd.polyidl = ep->p0p->polyid;
        else
            e2pd.cenidl = 0, e2pd.polyidl = 0;
        if (ep->p1p)
            e2pd.cenidr = ep->p1p->cenid, e2pd.polyidr = ep->p1p->polyid;
        else
            e2pd.cenidr = 0, e2pd.polyidr = 0;
        if (WRE2PD (fp, e2pd) <= 0)
            goto abortsavedata;
    }
    sfclose (fp);
    vmfree (Vmheap, sps);
    return 0;

abortsavedata:
    SUwarning (1, "savedata", "save failed for directory %s", dir);
    if (fp)
        sfclose (fp);
    if (sps)
        vmfree (Vmheap, sps);
    return -1;
}
