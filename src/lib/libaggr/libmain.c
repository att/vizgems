#pragma prototyped

#include <ast.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <vmalloc.h>
#include <cdt.h>
#include <tok.h>
#include <math.h>
#include <float.h>
#include <swift.h>
#define _AGGR_PRIVATE
#include <aggr.h>
#include <aggr_int.h>
#include "key.h"

#define HDR_LEN sizeof (AGGRhdr_t)

#define CMP(a, b) ( \
    a->framei != b->framei ? a->framei - b->framei : a->itemi - b->itemi \
)

#define ALIGN(v, a) ((((v) + (a) - 1) / (a)) * (a))

#define DICTUNIT (sizeof (double))
#define DICTLEN(afp) ( \
    ALIGN (afp->ad.byten + 6 * sizeof (int), afp->hdr.dictincr) \
)

#define DATALEN(afp) ( \
    (long long) afp->hdr.itemn * afp->hdr.framen * afp->hdr.vallen \
)

#define FILELEN(afp) (HDR_LEN + afp->hdr.dictlen + afp->hdr.datalen)

#define DOOP1(op, l, r) switch (op) { \
case APPEND_OPER_ADD: \
    (l) += (r); \
    break; \
case APPEND_OPER_SUB: \
    (l) -= (r); \
    break; \
case APPEND_OPER_MUL: \
    (l) *= (r); \
    break; \
case APPEND_OPER_DIV: \
    if ((r) != 0) \
        (l) /= (r); \
    else \
        (l) = 0; \
    break; \
case APPEND_OPER_MIN: \
    if ((l) > (r)) \
        (l) = (r); \
    break; \
case APPEND_OPER_MAX: \
    if ((l) < (r)) \
        (l) = (r); \
    break; \
case APPEND_OPER_SET: \
    (l) = (r); \
    break; \
}

static Vmalloc_t *aggrvm;
static Void_t *aggrheapmem (Vmalloc_t *, Void_t *, size_t, size_t, Vmdisc_t *);
static Vmdisc_t aggrheap = { aggrheapmem, NULL, 1024 * 1024 };

static int typelens[] = {
    /* CHAR   0 */ sizeof (char),
    /* SHORT  1 */ sizeof (short),
    /* INT    2 */ sizeof (int),
    /* LLONG  3 */ sizeof (long long),
    /* FLOAT  4 */ sizeof (float),
    /* DOUBLE 5 */ sizeof (double),
    /* STRING 6 */ -1
};

int *AGGRtypelens;

static char *typestrs[] = {
    /* CHAR   0 */ "char",
    /* SHORT  1 */ "short",
    /* INT    2 */ "int",
    /* LLONG  3 */ "long long",
    /* FLOAT  4 */ "float",
    /* DOUBLE 5 */ "double",
    /* STRING 6 */ "string"
};

static Dt_t *afdict;
static Dtdisc_t afdisc = {
    sizeof (Dtlink_t), -1, 0, NULL, NULL, NULL, NULL, NULL, NULL
};

static Dt_t *asdict;
static Dtdisc_t asdisc = {
    sizeof (Dtlink_t), -1, 0, NULL, NULL, NULL, NULL, NULL, NULL
};

#define MSG_4 4
#define MSG_8 8
#define MSG_S -1

static int dictinit (AGGRfile_t *);
static int dictterm (AGGRfile_t *);
static int dictread (AGGRfile_t *);
static int dictwrite (AGGRfile_t *);
static void dictdump (AGGRfile_t *, int);
static int dictgrowstack (AGGRfile_t *);

static int hdrread (AGGRfile_t *);
static int hdrwrite (AGGRfile_t *);

static int localopen (AGGRfile_t *, int);
static int localclose (AGGRfile_t *, int);
static int clientopen (AGGRfile_t *, int);
static int clientclose (AGGRfile_t *, int);
static int localsetlock (AGGRfile_t *);
static int localunsetlock (AGGRfile_t *);
static int clientsetlock (AGGRfile_t *);
static int clientunsetlock (AGGRfile_t *);
static int localensurelen (AGGRfile_t *, AGGRoff_t, int);
static int clientensurelen (AGGRfile_t *, AGGRoff_t, int);
static int localtruncate (AGGRfile_t *, AGGRoff_t);
static int clienttruncate (AGGRfile_t *, AGGRoff_t);
static int localsetmap (AGGRfile_t *, AGGRmap_t *);
static int localunsetmap (AGGRfile_t *, AGGRmap_t *);
static int clientsetmap (AGGRfile_t *, AGGRmap_t *);
static int clientunsetmap (AGGRfile_t *, AGGRmap_t *);

static int msgread (AGGRserver_t *);
static int msgwrite (AGGRserver_t *);
static int msgget (AGGRserver_t *, void *, int, int);
static int msgput (AGGRserver_t *, void *, int, int);

static AGGRfuncs_t localfuncs = {
    localopen, localclose,
    localsetlock, localunsetlock,
    localensurelen, localtruncate,
    localsetmap, localunsetmap
};
static AGGRfuncs_t clientfuncs = {
    clientopen, clientclose,
    clientsetlock, clientunsetlock,
    clientensurelen, clienttruncate,
    clientsetmap, clientunsetmap
};

int AGGRinit (void) {
    AGGRtypelens = typelens;
    if (!(aggrvm = vmopen (&aggrheap, Vmbest, VMFLAGS))) {
        SUwarning (1, "AGGRinit", "vmopen failed");
        return -1;
    }
    if (!(afdict = dtopen (&afdisc, Dtset))) {
        SUwarning (1, "AGGRinit", "dtopen failed");
        return -1;
    }
    if (!(asdict = dtopen (&asdisc, Dtset))) {
        SUwarning (1, "AGGRinit", "dtopen failed");
        return -1;
    }
    _aggrcombineinit ();
    _aggrcatinit ();
    _aggrsuminit ();
    _aggrreaggrinit ();
    _aggrextractinit ();
    _aggrmeaninit ();
    _aggrtopninit ();
    return 0;
}

int AGGRterm (void) {
    AGGRfile_t *afp;

    while ((afp = dtfirst (afdict)))
        AGGRclose (afp);
    dtclose (asdict);
    dtclose (afdict);
    vmclose (aggrvm);
    return 0;
}

AGGRfile_t *AGGRcreate (
    char *namep, int itemn, int framen, char keytype, char valtype,
    char *classp, int keylen, int vallen, int dictincr, int itemincr,
    int queuen, int alwayslocked
) {
    AGGRfile_t *afmemp, *afp;

    if (keytype != TYPE_STRING && keylen == -1)
        keylen = typelens[keytype];
    if (vallen == -1)
        vallen = typelens[valtype];
    if (
        !namep || itemn < 0 || framen < 0 || valtype >= TYPE_MAXID ||
        !classp || strlen (classp) + 1 > AGGR_CLASS_LEN ||
        keylen < -1 || keylen == 0 || vallen < -1 || vallen == 0 ||
        dictincr < 1 || itemincr < 1 || queuen < 1
    ) {
        SUwarning (
            1, "AGGRcreate", "bad arguments: %x %d %d %d %x %d %d %d %d %d",
            namep, itemn, framen, valtype, classp, keylen, vallen,
            dictincr, itemincr, queuen
        );
        return NULL;
    }
    dictincr = ALIGN (dictincr, DICTUNIT);
    if (itemn < itemincr)
        itemn = itemincr;
    if (dtmatch (afdict, namep)) {
        SUwarning (1, "AGGRcreate", "file %s in use", namep);
        return NULL;
    }
    afmemp = afp = NULL;
    if (
        !(afmemp = vmalloc (aggrvm, sizeof (AGGRfile_t))) ||
        !(afmemp->namep = vmstrdup (aggrvm, namep)) ||
        !(afp = dtinsert (afdict, afmemp)) || afp != afmemp
    )
        goto abortAGGRcreate;
    afp->fd = -1, afp->rwmode = AGGR_RWMODE_RW;
    afp->alwayslocked = alwayslocked, afp->lockednow = FALSE;
    afp->serverp = NULL;
    afp->cbufs = NULL, afp->cbufn = afp->cbufl = afp->cbufo = 0;
    /* setup header after opening the file ... */
    afp->ad.vm = NULL;
    afp->mapactive = FALSE;
    afp->map.rp = afp->map.p = NULL;
    afp->map.roff = afp->map.off = 0;
    afp->map.rlen = afp->map.len = 0;
    afp->map.datap = NULL;
    afp->map.datan = 0;
    afp->queues = NULL, afp->qops = NULL;
    afp->queuel = 0, afp->queuen = queuen;
    afp->qframes = NULL;
    afp->qframen = afp->qitemn = afp->qiteml = 0;
    afp->qmaps = NULL;
    afp->qmapn = 0;
    afp->qmaxiteml = 1;
    afp->dicthaschanged = FALSE;
    if (strncmp (namep, "http://", 7) == 0)
        afp->funcs = clientfuncs;
    else
        afp->funcs = localfuncs;
    if (dictinit (afp) == -1)
        goto abortAGGRcreate;
    afp->ad.keylen = keylen;
    if ((*afp->funcs.open) (afp, TRUE) == -1)
        goto abortAGGRcreate;
    if ((*afp->funcs.setlock) (afp) == -1)
        goto abortAGGRcreate;
    afp->hdr.version = -1;
    if (hdrread (afp) != -1)
        afp->hdr.dictsn++, afp->hdr.datasn++;
    else
        afp->hdr.dictsn = 1, afp->hdr.datasn = 1;
    if ((*afp->funcs.truncate) (afp, 0) == -1)
        goto abortAGGRcreate;
    memcpy (afp->hdr.magic, AGGR_MAGIC, AGGR_MAGIC_LEN);
    afp->hdr.version = AGGR_VERSION;
    afp->hdr.itemn = itemn, afp->hdr.framen = framen;
    afp->hdr.keytype = keytype;
    afp->hdr.valtype = valtype;
    afp->hdr.dictincr = dictincr;
    afp->hdr.itemincr = itemincr;
    afp->hdr.vallen = vallen;
    afp->hdr.dictlen = DICTLEN (afp);
    afp->hdr.datalen = DATALEN (afp);
    afp->hdr.minval = DBL_MAX, afp->hdr.maxval = DBL_MIN;
    memset (afp->hdr.class, 0, AGGR_CLASS_LEN);
    strcpy (afp->hdr.class, classp);
    if (
        hdrwrite (afp) == -1 || dictwrite (afp) == -1 ||
        (*afp->funcs.ensurelen) (
            afp, afp->hdr.dictlen + afp->hdr.datalen, TRUE
        ) == -1
    )
        goto abortAGGRcreate;
    if (!afp->alwayslocked && (*afp->funcs.unsetlock) (afp) == -1)
        goto abortAGGRcreate;
    if (!(afp->queues = vmalloc (aggrvm, queuen * vallen)))
        goto abortAGGRcreate;
    if (!(afp->qops = vmalloc (aggrvm, queuen * sizeof (char))))
        goto abortAGGRcreate;
    return afp;

abortAGGRcreate:
    SUwarning (1, "AGGRcreate", "create failed for file %s", namep);
    SUwarning (1, "AGGRcreate", "afp = %x", afp);
    if (afp) {
        SUwarning (1, "AGGRcreate", "afp->lockednow = %d", afp->lockednow);
        if (afp->lockednow > 0)
            (*afp->funcs.unsetlock) (afp);
        SUwarning (1, "AGGRcreate", "afp->fd = %d", afp->fd);
        if (afp->fd != -1)
            close (afp->fd);
        SUwarning (
            1, "AGGRcreate", "afp->queues = %x, afp->queuen = %d",
            afp->queues, afp->queuen
        );
        if (afp->queues)
            vmfree (aggrvm, afp->queues);
        SUwarning (1, "AGGRcreate", "afp->qops = %x", afp->qops);
        if (afp->qops)
            vmfree (aggrvm, afp->qops);
        SUwarning (1, "AGGRcreate", "afp->ad.vm = %x", afp->ad.vm);
        if (afp->ad.vm)
            dictterm (afp);
        if (afp == afmemp)
            dtdelete (afdict, afp);
    }
    SUwarning (1, "AGGRcreate", "afmemp = %x", afmemp);
    if (afmemp) {
        SUwarning (1, "AGGRcreate", "afmemp->namep = %x", afmemp->namep);
        if (afmemp->namep)
            vmfree (aggrvm, afmemp->namep);
        vmfree (aggrvm, afmemp);
    }
    return NULL;
}

int AGGRdestroy (AGGRfile_t *afp) {
    int qframei;

    if (!afp) {
        SUwarning (1, "AGGRdestroy", "bad argument: %x", afp);
        return -1;
    }
    if (afp->mapactive && AGGRrelease (afp) == -1)
        SUwarning (0, "AGGRdestroy", "release failed");
    if (afp->queuel > 0 && AGGRflush (afp) == -1)
        SUwarning (0, "AGGRdestroy", "flush failed");
    if (afp->alwayslocked && (*afp->funcs.unsetlock) (afp) == -1)
        SUwarning (0, "AGGRdestroy", "unsetlock failed");
    if (afp->fd != -1 && (*afp->funcs.close) (afp, TRUE) == -1)
        SUwarning (0, "AGGRdestroy", "close failed");
    if (afp->serverp && AGGRserverrelease (afp->serverp) == -1)
        SUwarning (0, "AGGRdestroy", "serverrelease failed");
    if (afp->qframes) {
        for (qframei = 0; qframei < afp->qframen; qframei++)
            vmfree (aggrvm, afp->qframes[qframei]);
        vmfree (aggrvm, afp->qframes);
    }
    if (afp->qmaps)
        vmfree (aggrvm, afp->qmaps);
    if (afp->queues)
        vmfree (aggrvm, afp->queues);
    if (afp->qops)
        vmfree (aggrvm, afp->qops);
    if (afp->ad.vm)
        dictterm (afp);
    dtdelete (afdict, afp);
    if (afp->namep)
        vmfree (aggrvm, afp->namep);
    vmfree (aggrvm, afp);
    return 0;
}

AGGRfile_t *AGGRopen (char *namep, int rwmode, int queuen, int alwayslocked) {
    AGGRfile_t *afmemp, *afp;

    if (
        (rwmode != AGGR_RWMODE_RD && rwmode != AGGR_RWMODE_RW) || queuen < 1 ||
        (alwayslocked != TRUE && alwayslocked != FALSE)
    ) {
        SUwarning (
            1, "AGGRopen", "bad arguments: %d %d %d",
            rwmode, queuen, alwayslocked
        );
        return NULL;
    }
    if (dtmatch (afdict, namep)) {
        SUwarning (1, "AGGRopen", "file %s in use", namep);
        return NULL;
    }
    afmemp = afp = NULL;
    if (
        !(afmemp = vmalloc (aggrvm, sizeof (AGGRfile_t))) ||
        !(afmemp->namep = vmstrdup (aggrvm, namep)) ||
        !(afp = dtinsert (afdict, afmemp)) || afp != afmemp
    )
        goto abortAGGRopen;
    afp->fd = -1, afp->rwmode = rwmode;
    afp->alwayslocked = alwayslocked, afp->lockednow = FALSE;
    afp->serverp = NULL;
    afp->cbufs = NULL, afp->cbufn = afp->cbufl = afp->cbufo = 0;
    /* setup header after opening the file ... */
    afp->ad.vm = NULL;
    afp->mapactive = FALSE;
    afp->map.rp = afp->map.p = NULL;
    afp->map.roff = afp->map.off = 0;
    afp->map.rlen = afp->map.len = 0;
    afp->map.datap = NULL;
    afp->map.datan = 0;
    afp->queues = NULL, afp->qops = NULL;
    afp->queuel = 0, afp->queuen = queuen;
    afp->qframes = NULL;
    afp->qframen = afp->qitemn = afp->qiteml = 0;
    afp->qmaps = NULL;
    afp->qmapn = 0;
    afp->qmaxiteml = 1;
    afp->dicthaschanged = FALSE;
    if (strncmp (namep, "http://", 7) == 0)
        afp->funcs = clientfuncs;
    else
        afp->funcs = localfuncs;
    if (dictinit (afp) == -1)
        goto abortAGGRopen;
    if ((*afp->funcs.open) (
        afp, (rwmode == AGGR_RWMODE_RW) ? TRUE : FALSE
    ) == -1)
        goto abortAGGRopen;
    afp->hdr.version = -1;
    if ((*afp->funcs.setlock) (afp) == -1 || hdrread (afp) == -1)
        goto abortAGGRopen;
    if (dictread (afp) == -1 || afp->ad.itemn > afp->hdr.itemn)
        goto abortAGGRopen;
    if (afp->alwayslocked && afp->rwmode == AGGR_RWMODE_RW) {
        afp->hdr.datasn++;
        if (hdrwrite (afp) == -1)
            goto abortAGGRopen;
    }
    if (!afp->alwayslocked && (*afp->funcs.unsetlock) (afp) == -1)
        goto abortAGGRopen;
    if (!(afp->queues = vmalloc (aggrvm, queuen * afp->hdr.vallen)))
        goto abortAGGRopen;
    if (!(afp->qops = vmalloc (aggrvm, queuen * sizeof (char))))
        goto abortAGGRopen;
    return afp;

abortAGGRopen:
    SUwarning (1, "AGGRopen", "open failed for file %s", namep);
    SUwarning (1, "AGGRopen", "afp = %x", afp);
    if (afp) {
        SUwarning (1, "AGGRopen", "afp->lockednow = %d", afp->lockednow);
        if (afp->lockednow > 0)
            (*afp->funcs.unsetlock) (afp);
        SUwarning (1, "AGGRopen", "afp->fd = %d", afp->fd);
        if (afp->fd != -1)
            close (afp->fd);
        SUwarning (
            1, "AGGRopen", "afp->queues = %x, afp->queuen = %d",
            afp->queues, afp->queuen
        );
        if (afp->queues)
            vmfree (aggrvm, afp->queues);
        SUwarning (1, "AGGRopen", "afp->qops = %x", afp->qops);
        if (afp->qops)
            vmfree (aggrvm, afp->qops);
        SUwarning (1, "AGGRopen", "afp->ad.vm = %x", afp->ad.vm);
        if (afp->ad.vm)
            dictterm (afp);
        if (afp == afmemp)
            dtdelete (afdict, afp);
    }
    SUwarning (1, "AGGRopen", "afmemp = %x", afmemp);
    if (afmemp) {
        SUwarning (1, "AGGRopen", "afmemp->namep = %x", afmemp->namep);
        if (afmemp->namep)
            vmfree (aggrvm, afmemp->namep);
        vmfree (aggrvm, afmemp);
    }
    return NULL;
}

int AGGRclose (AGGRfile_t *afp) {
    int qframei;

    if (!afp) {
        SUwarning (1, "AGGRclose", "bad argument: %x", afp);
        return -1;
    }
    if (afp->mapactive && AGGRrelease (afp) == -1)
        SUwarning (0, "AGGRclose", "release failed");
    if (afp->queuel > 0 && AGGRflush (afp) == -1)
        SUwarning (0, "AGGRclose", "flush failed");
    if (afp->alwayslocked && (*afp->funcs.unsetlock) (afp) == -1)
        SUwarning (0, "AGGRclose", "unsetlock failed");
    if (afp->fd != -1 && (*afp->funcs.close) (afp, FALSE) == -1)
        SUwarning (0, "AGGRclose", "close failed");
    if (afp->serverp && AGGRserverrelease (afp->serverp) == -1)
        SUwarning (0, "AGGRclose", "serverrelease failed");
    if (afp->qframes) {
        for (qframei = 0; qframei < afp->qframen; qframei++)
            vmfree (aggrvm, afp->qframes[qframei]);
        vmfree (aggrvm, afp->qframes);
    }
    if (afp->qmaps)
        vmfree (aggrvm, afp->qmaps);
    if (afp->queues)
        vmfree (aggrvm, afp->queues);
    if (afp->qops)
        vmfree (aggrvm, afp->qops);
    if (afp->ad.vm)
        dictterm (afp);
    dtdelete (afdict, afp);
    if (afp->namep)
        vmfree (aggrvm, afp->namep);
    vmfree (aggrvm, afp);
    return 0;
}

int AGGRlockmode (AGGRfile_t *afp, int alwayslocked) {
    if (afp->alwayslocked == alwayslocked)
        return 0;
    if (afp->alwayslocked) {
        if ((*afp->funcs.unsetlock) (afp) == -1) {
            SUwarning (0, "AGGRlockmode", "unsetlock failed");
            return -1;
        }
    } else {
        if ((*afp->funcs.setlock) (afp) == -1 || hdrread (afp) == -1) {
            SUwarning (0, "AGGRlockmode", "setlock/hdrread failed");
            return -1;
        }
    }
    afp->alwayslocked = alwayslocked;
    return 0;
}

int AGGRprint (
    AGGRfile_t *afp, int printdict, int printdata,
    int onlyitemi, int onlyframei, int verbose, int rawmode
) {
    int itemi, fitemi, litemi, framei, fframei, lframei;
    void *datap, *cachep;
    unsigned char **revmap;
    int fnamei, len;
    int sn;
#if SWIFT_LITTLEENDIAN == 1
    AGGRunit_t ku, vu;
    unsigned char *kcp;
    int kcl, kl, vcl, vl;
#endif
    unsigned char *vcp;

    if (onlyitemi >= 0)
        fitemi = litemi = onlyitemi;
    else
        fitemi = 0, litemi = afp->ad.itemn - 1;
    if (onlyframei >= 0)
        fframei = lframei = onlyframei;
    else
        fframei = 0, lframei = afp->hdr.framen - 1;
    if (rawmode) {
        if (printdict)
            SUwarning (
                1, "AGGRprint", "dictionary printing not available in raw mode"
            );
        if (printdata) {
            if (verbose > 1) {
                if (!(revmap = vmalloc (
                    aggrvm, afp->ad.itemn * sizeof (char *)
                ))) {
                    SUwarning (1, "AGGRprint", "vmalloc failed");
                    return -1;
                }
                _aggrdictreverse (afp, revmap);
            }
            for (framei = fframei; framei <= lframei; framei++) {
                if (!(datap = AGGRget (afp, framei))) {
                    SUwarning (1, "AGGRprint", "get failed");
                    return -1;
                }
                vcp = (unsigned char *) datap + fitemi * afp->hdr.vallen;
                for (itemi = fitemi; itemi <= litemi; itemi++) {
                    if (verbose > 0) {
#if SWIFT_LITTLEENDIAN == 1
                        ku.u.i = framei;
                        SW4 (&ku.u.i);
                        sfwrite (sfstdout, &ku.u.i, sizeof (int));
#else
                        sfwrite (sfstdout, &framei, sizeof (int));
#endif
                    }
                    if (verbose > 1) {
                        if (afp->ad.keylen == -1)
                            len = strlen ((char *) revmap[itemi]) + 1;
                        else
                            len = afp->ad.keylen;
#if SWIFT_LITTLEENDIAN == 1
                        kcp = revmap[itemi];
                        kcl = typelens[afp->hdr.keytype];
                        switch (afp->hdr.keytype) {
                        case AGGR_TYPE_CHAR:
                            for (kl = 0; kl < len; kl += kcl) {
                                ku.u.c = CVAL (kcp + kl);
                                sfwrite (sfstdout, &ku.u.c, kcl);
                            }
                            break;
                        case AGGR_TYPE_SHORT:
                            for (kl = 0; kl < len; kl += kcl) {
                                ku.u.s = SVAL (kcp + kl);
                                SW2 (&ku.u.s);
                                sfwrite (sfstdout, &ku.u.s, kcl);
                            }
                            break;
                        case AGGR_TYPE_INT:
                            for (kl = 0; kl < len; kl += kcl) {
                                ku.u.i = IVAL (kcp + kl);
                                SW4 (&ku.u.i);
                                sfwrite (sfstdout, &ku.u.i, kcl);
                            }
                            break;
                        case AGGR_TYPE_LLONG:
                            for (kl = 0; kl < len; kl += kcl) {
                                ku.u.l = LVAL (kcp + kl);
                                SW8 (&ku.u.l);
                                sfwrite (sfstdout, &ku.u.l, kcl);
                            }
                            break;
                        case AGGR_TYPE_FLOAT:
                            for (kl = 0; kl < len; kl += kcl) {
                                ku.u.f = FVAL (kcp + kl);
                                SW4 (&ku.u.f);
                                sfwrite (sfstdout, &ku.u.f, kcl);
                            }
                            break;
                        case AGGR_TYPE_DOUBLE:
                            for (kl = 0; kl < len; kl += kcl) {
                                ku.u.d = DVAL (kcp + kl);
                                SW8 (&ku.u.d);
                                sfwrite (sfstdout, &ku.u.d, kcl);
                            }
                            break;
                        case AGGR_TYPE_STRING:
                            sfwrite (sfstdout, kcp, len);
                            break;
                        }
#else
                        sfwrite (sfstdout, revmap[itemi], len);
#endif
                    }
                    len = afp->hdr.vallen;
#if SWIFT_LITTLEENDIAN == 1
                    vcl = typelens[afp->hdr.valtype];
                    switch (afp->hdr.valtype) {
                    case AGGR_TYPE_CHAR:
                        for (vl = 0; vl < len; vl += vcl) {
                            vu.u.c = CVAL (vcp + vl);
                            sfwrite (sfstdout, &vu.u.c, vcl);
                        }
                        break;
                    case AGGR_TYPE_SHORT:
                        for (vl = 0; vl < len; vl += vcl) {
                            vu.u.s = SVAL (vcp + vl);
                            SW2 (&vu.u.s);
                            sfwrite (sfstdout, &vu.u.s, vcl);
                        }
                        break;
                    case AGGR_TYPE_INT:
                        for (vl = 0; vl < len; vl += vcl) {
                            vu.u.i = IVAL (vcp + vl);
                            SW4 (&vu.u.i);
                            sfwrite (sfstdout, &vu.u.i, vcl);
                        }
                        break;
                    case AGGR_TYPE_LLONG:
                        for (vl = 0; vl < len; vl += vcl) {
                            vu.u.l = LVAL (vcp + vl);
                            SW8 (&vu.u.l);
                            sfwrite (sfstdout, &vu.u.l, vcl);
                        }
                        break;
                    case AGGR_TYPE_FLOAT:
                        for (vl = 0; vl < len; vl += vcl) {
                            vu.u.f = FVAL (vcp + vl);
                            SW4 (&vu.u.f);
                            sfwrite (sfstdout, &vu.u.f, vcl);
                        }
                        break;
                    case AGGR_TYPE_DOUBLE:
                        for (vl = 0; vl < len; vl += vcl) {
                            vu.u.d = DVAL (vcp + vl);
                            SW8 (&vu.u.d);
                            sfwrite (sfstdout, &vu.u.d, vcl);
                        }
                        break;
                    }
#else
                    sfwrite (sfstdout, vcp, len);
#endif
                    vcp += len;
                }
                if (AGGRrelease (afp) == -1) {
                    SUwarning (1, "AGGRprint", "release failed");
                    return -1;
                }
            }
        }
    } else { /* ascii mode */
        if (verbose > 1) {
            sfprintf (sfstdout, "file: %s\n", afp->namep);
            sfprintf (sfstdout, "dictsn: %d\n", afp->hdr.dictsn);
            sfprintf (sfstdout, "datasn: %d\n", afp->hdr.datasn);
            sfprintf (sfstdout, "itemn: %d\n", afp->hdr.itemn);
            sfprintf (sfstdout, "framen: %d\n", afp->hdr.framen);
            sfprintf (sfstdout, "keytype: %s\n", typestrs[afp->hdr.keytype]);
            sfprintf (sfstdout, "valtype: %s\n", typestrs[afp->hdr.valtype]);
            sfprintf (sfstdout, "dictlen: %lld\n", afp->hdr.dictlen);
            sfprintf (sfstdout, "datalen: %lld\n", afp->hdr.datalen);
            sfprintf (sfstdout, "dictincr: %d\n", afp->hdr.dictincr);
            sfprintf (sfstdout, "itemincr: %d\n", afp->hdr.itemincr);
            sfprintf (sfstdout, "minvalue: %f\n", afp->hdr.minval);
            sfprintf (sfstdout, "maxvalue: %f\n", afp->hdr.maxval);
            sfprintf (sfstdout, "class: %s\n", afp->hdr.class);
            sfprintf (sfstdout, "keylen: %d\n", afp->ad.keylen);
            sfprintf (sfstdout, "vallen: %d\n", afp->hdr.vallen);
            sfprintf (sfstdout, "keyn: %d\n", afp->ad.keyn);
            sfprintf (sfstdout, "byten: %lld\n", afp->ad.byten);
            sfprintf (sfstdout, "dictitemn: %d\n", afp->ad.itemn);
        }
        if (printdict)
            dictdump (afp, onlyitemi);
        if (printdata) {
            if (verbose > 1) {
                if (!(revmap = vmalloc (
                    aggrvm, afp->ad.itemn * sizeof (char *)
                ))) {
                    SUwarning (1, "AGGRprint", "alloc failed");
                    return -1;
                }
                _aggrdictreverse (afp, revmap);
            }
            if (!(cachep = vmalloc (aggrvm, (
                litemi - fitemi + 1
            ) * afp->hdr.vallen))) {
                SUwarning (1, "AGGRprint", "alloc failed");
                return -1;
            }
            sn = afp->hdr.dictsn;
            for (framei = fframei; framei <= lframei; framei++) {
                if (verbose > 1) {
                    sfprintf (sfstdout, "frame# %d", framei);
                    for (fnamei = 0; fnamei < afp->ad.fnamen; fnamei++)
                        if (afp->ad.fnames[fnamei].framei == framei)
                            sfprintf (
                                sfstdout, " %d \"%s\"",
                                afp->ad.fnames[fnamei].level,
                                afp->ad.fnames[fnamei].name
                            );
                    sfprintf (sfstdout, "\n");
                }
                if (!(datap = AGGRget (afp, framei))) {
                    SUwarning (1, "AGGRprint", "get failed");
                    return -1;
                }
                if (afp->hdr.dictsn != sn) {
                    SUwarning (1, "AGGRprint", "file changed");
                    vmfree (aggrvm, cachep);
                    return -1;
                }
                memcpy (cachep, (char *) datap + fitemi * afp->hdr.vallen, (
                    litemi - fitemi + 1
                ) * afp->hdr.vallen);
                vcp = cachep;
                if (AGGRrelease (afp) == -1) {
                    SUwarning (1, "AGGRprint", "release failed");
                    return -1;
                }
                len = afp->hdr.vallen;
                for (itemi = fitemi; itemi <= litemi; itemi++) {
                    if (verbose > 1) {
                        sfprintf (sfstdout, "item# %d ", itemi);
                        _aggrdictprintkey (afp, sfstdout, revmap[itemi]);
                        sfprintf (sfstdout, " ");
                    } else if (verbose > 0)
                        sfprintf (sfstdout, "%d ", itemi);
                    _aggrprintval (afp, sfstdout, vcp);
                    sfprintf (sfstdout, "\n");
                    vcp += len;
                }
            }
            vmfree (aggrvm, cachep);
            if (verbose > 1)
                vmfree (aggrvm, revmap);
        }
    }
    return 0;
}

int AGGRgrow (AGGRfile_t *afp, int itemn, int framen) {
    AGGRhdr_t hdr;

    if (!afp || itemn < afp->hdr.itemn || framen < afp->hdr.framen) {
        SUwarning (
            1, "AGGRgrow", "bad arguments: %x %d %d", afp, itemn, framen
        );
        return -1;
    }
    if (afp->rwmode != AGGR_RWMODE_RW) {
        SUwarning (1, "AGGRgrow", "cannot grow read-only file");
        return -1;
    }
    if (afp->mapactive) {
        SUwarning (1, "AGGRgrow", "frame locked");
        return -1;
    }
    if (afp->queuel > 0 && AGGRflush (afp) == -1) {
        SUwarning (1, "AGGRgrow", "flash failed");
        return -1;
    }
    if (!afp->alwayslocked) {
        hdr = afp->hdr;
        if ((*afp->funcs.setlock) (afp) == -1 || hdrread (afp) == -1) {
            SUwarning (1, "AGGRgrow", "setlock/hdrread failed");
            goto abortAGGRgrow;
        }
        if (afp->hdr.dictsn != hdr.dictsn) {
            SUwarning (1, "AGGRgrow", "dictionary changed");
            if (
                dictterm (afp) == -1 || dictinit (afp) == -1 ||
                dictread (afp) == -1
            ) {
                SUwarning (1, "AGGRgrow", "dictread failed");
                goto abortAGGRgrow;
            }
        }
        if (afp->hdr.datasn != hdr.datasn)
            SUwarning (1, "AGGRgrow", "data changed");
        if (itemn < afp->hdr.itemn || framen < afp->hdr.framen) {
            SUwarning (1, "AGGRgrow", "cannot shrink file");
            goto abortAGGRgrow;
        }
    }
    if (_aggrgrow (afp, itemn, framen) == -1) {
        SUwarning (1, "AGGRgrow", "grow failed");
        goto abortAGGRgrow;
    }
    if (!afp->alwayslocked) {
        if ((*afp->funcs.unsetlock) (afp) == -1) {
            SUwarning (1, "AGGRgrow", "unsetlock failed");
            return -1;
        }
    }
    return 0;

abortAGGRgrow:
    if (!afp->alwayslocked && afp->lockednow)
        (*afp->funcs.unsetlock) (afp);
    return -1;
}

int AGGRappend (AGGRfile_t *afp, AGGRkv_t *kvp, int insertflag) {
    AGGRqueue_t *queuep, *qopp;
    int qframei, qitemn, qitemi, qmapn, qmapi, itemi, cl, vl;

    if (!afp || !kvp || kvp->oper < 0 || kvp->oper >= APPEND_OPER_MAXID) {
        SUwarning (
            1, "AGGRappend", "bad arguments: %x %x", afp, kvp
        );
        return -1;
    }
    if (afp->rwmode != AGGR_RWMODE_RW) {
        SUwarning (1, "AGGRappend", "cannot append to read-only file");
        return -1;
    }
    if (afp->mapactive) {
        SUwarning (1, "AGGRappend", "a frame is active");
        return -1;
    }
    if (afp->queuel == afp->queuen || afp->qiteml >= afp->qmaxiteml) {
        if (AGGRflush (afp) == -1) {
            SUwarning (1, "AGGRappend", "flush failed");
            return -1;
        }
    }
    if (!kvp->keyp && kvp->itemi >= 0) {
        itemi = kvp->itemi;
    } else {
        switch (afp->ad.keylen) {
        case -1:
            itemi = _aggrdictlookupvar (
                afp, kvp->keyp, kvp->itemi, insertflag
            );
            break;
        default:
            itemi = _aggrdictlookupfixed (
                afp, kvp->keyp, kvp->itemi, insertflag
            );
            break;
        }
    }
    if (itemi == -1) {
        SUwarning (1, "AGGRappend", "dictlookup failed");
        return -1;
    }
    kvp->itemi = itemi;
    if (kvp->itemi >= afp->qmapn) {
        if (afp->queuen > 10000)
            qmapn = ALIGN (kvp->itemi + 1, afp->queuen / 100);
        else
            qmapn = ALIGN (kvp->itemi + 1, 1000);
        if (!(afp->qmaps = vmresize (
            aggrvm, afp->qmaps, qmapn * sizeof (int), VM_RSCOPY
        ))) {
            SUwarning (1, "AGGRappend", "vmresize failed");
            return -1;
        }
        for (qmapi = afp->qmapn; qmapi < qmapn; qmapi++)
            afp->qmaps[qmapi] = -1;
        afp->qmapn = qmapn;
    }
    if ((qitemi = afp->qmaps[kvp->itemi]) == -1) {
        if (afp->qiteml + 1 > afp->qitemn) {
            if (afp->queuen > 10000)
                qitemn = ALIGN (afp->qiteml + 1, afp->queuen / 100);
            else
                qitemn = ALIGN (afp->qiteml + 1, 1000);
            for (qframei = 0; qframei < afp->qframen; qframei++) {
                if (!(afp->qframes[qframei] = vmresize (
                    aggrvm, afp->qframes[qframei],
                    qitemn * sizeof (AGGRqueue_t *), VM_RSCOPY
                ))) {
                    SUwarning (1, "AGGRappend", "vmresize failed");
                    return -1;
                }
                memset (
                    &afp->qframes[qframei][afp->qitemn], 0,
                    (qitemn - afp->qitemn) * sizeof (AGGRqueue_t *)
                );
            }
            afp->qitemn = qitemn;
        }
        qitemi = afp->qmaps[kvp->itemi] = afp->qiteml++;
    }
    if (kvp->framei >= afp->qframen) {
        if (!(afp->qframes = vmresize (
            aggrvm, afp->qframes,
            (kvp->framei + 1) * sizeof (AGGRqueue_t **), VM_RSCOPY
        ))) {
            SUwarning (1, "AGGRappend", "vmresize failed");
            return -1;
        }
        for (qframei = afp->qframen; qframei < kvp->framei + 1; qframei++) {
            if (!(afp->qframes[qframei] = vmresize (
                aggrvm, NULL,
                afp->qitemn * sizeof (AGGRqueue_t *), VM_RSCOPY
            ))) {
                SUwarning (1, "AGGRappend", "vmresize failed");
                return -1;
            }
            memset (
                &afp->qframes[qframei][0], 0,
                (afp->qitemn) * sizeof (AGGRqueue_t *)
            );
        }
        afp->qframen = kvp->framei + 1;
        if ((afp->qmaxiteml = (afp->queuen * 6) / afp->qframen) < 1000)
            afp->qmaxiteml = 1000;
    }
    cl = typelens[afp->hdr.valtype];
    if (!(queuep = afp->qframes[kvp->framei][qitemi])) {
        queuep = &afp->queues[afp->hdr.vallen * afp->queuel++];
        qopp = &afp->qops[(queuep - &afp->queues[0]) / afp->hdr.vallen];
        afp->qframes[kvp->framei][qitemi] = queuep;
        switch (afp->hdr.valtype) {
        case TYPE_CHAR:
            for (vl = 0; vl < afp->hdr.vallen; vl += cl)
                CVAL (&queuep[vl]) = CVAL (&kvp->valp[vl]);
            break;
        case TYPE_SHORT:
            for (vl = 0; vl < afp->hdr.vallen; vl += cl)
                SVAL (&queuep[vl]) = SVAL (&kvp->valp[vl]);
            break;
        case TYPE_INT:
            for (vl = 0; vl < afp->hdr.vallen; vl += cl)
                IVAL (&queuep[vl]) = IVAL (&kvp->valp[vl]);
            break;
        case TYPE_LLONG:
            for (vl = 0; vl < afp->hdr.vallen; vl += cl)
                LVAL (&queuep[vl]) = LVAL (&kvp->valp[vl]);
            break;
        case TYPE_FLOAT:
            for (vl = 0; vl < afp->hdr.vallen; vl += cl)
                FVAL (&queuep[vl]) = FVAL (&kvp->valp[vl]);
            break;
        case TYPE_DOUBLE:
            for (vl = 0; vl < afp->hdr.vallen; vl += cl)
                DVAL (&queuep[vl]) = DVAL (&kvp->valp[vl]);
            break;
        }
        *qopp = kvp->oper;
    } else {
        qopp = &afp->qops[(queuep - &afp->queues[0]) / afp->hdr.vallen];
        switch (afp->hdr.valtype) {
        case TYPE_CHAR:
            for (vl = 0; vl < afp->hdr.vallen; vl += cl)
                DOOP1 (kvp->oper, CVAL (&queuep[vl]), CVAL (&kvp->valp[vl]));
            break;
        case TYPE_SHORT:
            for (vl = 0; vl < afp->hdr.vallen; vl += cl)
                DOOP1 (kvp->oper, SVAL (&queuep[vl]), SVAL (&kvp->valp[vl]));
            break;
        case TYPE_INT:
            for (vl = 0; vl < afp->hdr.vallen; vl += cl)
                DOOP1 (kvp->oper, IVAL (&queuep[vl]), IVAL (&kvp->valp[vl]));
            break;
        case TYPE_LLONG:
            for (vl = 0; vl < afp->hdr.vallen; vl += cl)
                DOOP1 (kvp->oper, LVAL (&queuep[vl]), LVAL (&kvp->valp[vl]));
            break;
        case TYPE_FLOAT:
            for (vl = 0; vl < afp->hdr.vallen; vl += cl)
                DOOP1 (kvp->oper, FVAL (&queuep[vl]), FVAL (&kvp->valp[vl]));
            break;
        case TYPE_DOUBLE:
            for (vl = 0; vl < afp->hdr.vallen; vl += cl)
                DOOP1 (kvp->oper, DVAL (&queuep[vl]), DVAL (&kvp->valp[vl]));
            break;
        }
        *qopp = kvp->oper;
    }
    return 0;
}

int AGGRflush (AGGRfile_t *afp) {
    AGGRhdr_t hdr;
    int itemn, framen;
    AGGRqueue_t *queuep, *qopp;
    int qframei, qmapi;
    unsigned char *cp;
    int cl, vl;
    double val;

    if (!afp) {
        SUwarning (1, "AGGRflush", "bad argument: %x", afp);
        return -1;
    }
    if (afp->queuel == 0)
        return 0;
    if (!afp->alwayslocked) {
        hdr = afp->hdr;
        if ((*afp->funcs.setlock) (afp) == -1 || hdrread (afp) == -1) {
            SUwarning (1, "AGGRflush", "setlock/hdrread failed");
            goto abortAGGRflush;
        }
        if (afp->hdr.dictsn != hdr.dictsn) {
            SUwarning (1, "AGGRflush", "dictionary changed");
            goto abortAGGRflush;
        }
        if (afp->hdr.datasn != hdr.datasn)
            SUwarning (1, "AGGRflush", "data changed");
    }
    if ((framen = afp->qframen) < afp->hdr.framen)
        framen = afp->hdr.framen;
    if (
        afp->dicthaschanged || afp->ad.itemn != afp->hdr.itemn ||
        framen != afp->hdr.framen
    ) {
        if (afp->ad.itemn > afp->hdr.itemn)
            itemn = afp->ad.itemn;
        else
            itemn = afp->hdr.itemn;
        if (_aggrgrow (afp, itemn, framen) == -1) {
            SUwarning (1, "AGGRflush", "grow failed");
            goto abortAGGRflush;
        }
    }
    afp->map.len = afp->hdr.itemn * afp->hdr.vallen;
    cl = typelens[afp->hdr.valtype];
    for (qframei = 0; qframei < afp->qframen; qframei++) {
        for (qmapi = 0, queuep = NULL; qmapi < afp->qmapn; qmapi++) {
            if (afp->qmaps[qmapi] == -1)
                continue;
            if ((queuep = afp->qframes[qframei][afp->qmaps[qmapi]]))
                break;
        }
        if (!queuep)
            continue;
        afp->map.off = (
            HDR_LEN + afp->hdr.dictlen +
            (long long) afp->hdr.itemn * qframei * afp->hdr.vallen
        );
        if ((*afp->funcs.setmap) (afp, &afp->map) == -1) {
            SUwarning (1, "AGGRflush", "setmap failed");
            goto abortAGGRflush;
        }
#if SWIFT_LITTLEENDIAN == 1
        switch (afp->hdr.valtype) {
        case AGGR_TYPE_SHORT:
            for (vl = 0; vl < afp->map.len; vl += cl)
                SW2 (afp->map.p + vl);
            break;
        case AGGR_TYPE_INT:
            for (vl = 0; vl < afp->map.len; vl += cl)
                SW4 (afp->map.p + vl);
            break;
        case AGGR_TYPE_LLONG:
            for (vl = 0; vl < afp->map.len; vl += cl)
                SW8 (afp->map.p + vl);
            break;
        case AGGR_TYPE_FLOAT:
            for (vl = 0; vl < afp->map.len; vl += cl)
                SW4 (afp->map.p + vl);
            break;
        case AGGR_TYPE_DOUBLE:
            for (vl = 0; vl < afp->map.len; vl += cl)
                SW8 (afp->map.p + vl);
            break;
        }
#endif
        cl = typelens[afp->hdr.valtype];
        for (qmapi = 0; qmapi < afp->qmapn; qmapi++) {
            if (afp->qmaps[qmapi] == -1)
                continue;
            if (!(queuep = afp->qframes[qframei][afp->qmaps[qmapi]]))
                continue;
            cp = (unsigned char *) afp->map.p + qmapi * afp->hdr.vallen;
            qopp = &afp->qops[(queuep - &afp->queues[0]) / afp->hdr.vallen];
            val = 0.0;
            switch (afp->hdr.valtype) {
            case TYPE_CHAR:
                for (vl = 0; vl < afp->hdr.vallen; vl += cl)
                    DOOP1 (*qopp, CVAL (&cp[vl]), CVAL (&queuep[vl]));
                val = CVAL (&cp[0]);
                break;
            case TYPE_SHORT:
                for (vl = 0; vl < afp->hdr.vallen; vl += cl)
                    DOOP1 (*qopp, SVAL (&cp[vl]), SVAL (&queuep[vl]));
                val = SVAL (&cp[0]);
                break;
            case TYPE_INT:
                for (vl = 0; vl < afp->hdr.vallen; vl += cl)
                    DOOP1 (*qopp, IVAL (&cp[vl]), IVAL (&queuep[vl]));
                val = IVAL (&cp[0]);
                break;
            case TYPE_LLONG:
                for (vl = 0; vl < afp->hdr.vallen; vl += cl)
                    DOOP1 (*qopp, LVAL (&cp[vl]), LVAL (&queuep[vl]));
                val = LVAL (&cp[0]);
                break;
            case TYPE_FLOAT:
                for (vl = 0; vl < afp->hdr.vallen; vl += cl)
                    DOOP1 (*qopp, FVAL (&cp[vl]), FVAL (&queuep[vl]));
                val = FVAL (&cp[0]);
                break;
            case TYPE_DOUBLE:
                for (vl = 0; vl < afp->hdr.vallen; vl += cl)
                    DOOP1 (*qopp, DVAL (&cp[vl]), DVAL (&queuep[vl]));
                val = DVAL (&cp[0]);
                break;
            }
            if (val < afp->hdr.minval)
                afp->hdr.minval = val;
            if (val > afp->hdr.maxval)
                afp->hdr.maxval = val;
            afp->qframes[qframei][afp->qmaps[qmapi]] = NULL;
        }
#if SWIFT_LITTLEENDIAN == 1
        if (afp->rwmode == AGGR_RWMODE_RW && afp->map.len > 0) {
            switch (afp->hdr.valtype) {
            case AGGR_TYPE_SHORT:
                for (vl = 0; vl < afp->map.len; vl += cl)
                    SW2 (afp->map.p + vl);
                break;
            case AGGR_TYPE_INT:
                for (vl = 0; vl < afp->map.len; vl += cl)
                    SW4 (afp->map.p + vl);
                break;
            case AGGR_TYPE_LLONG:
                for (vl = 0; vl < afp->map.len; vl += cl)
                    SW8 (afp->map.p + vl);
                break;
            case AGGR_TYPE_FLOAT:
                for (vl = 0; vl < afp->map.len; vl += cl)
                    SW4 (afp->map.p + vl);
                break;
            case AGGR_TYPE_DOUBLE:
                for (vl = 0; vl < afp->map.len; vl += cl)
                    SW8 (afp->map.p + vl);
                break;
            }
        }
#endif
        if ((*afp->funcs.unsetmap) (afp, &afp->map) == -1) {
            SUwarning (1, "AGGRflush", "unsetmap failed");
            goto abortAGGRflush;
        }
    }
    for (qmapi = 0; qmapi < afp->qmapn; qmapi++)
        afp->qmaps[qmapi] = -1;
    afp->qiteml = 0;
    afp->queuel = 0;
    afp->hdr.datasn++;
    if (hdrwrite (afp) == -1) {
        SUwarning (1, "AGGRflush", "hdrwrite failed");
        goto abortAGGRflush;
    }
    if (!afp->alwayslocked) {
        if ((*afp->funcs.unsetlock) (afp) == -1) {
            SUwarning (1, "AGGRflush", "unsetlock failed");
            return -1;
        }
    }
    return 0;

abortAGGRflush:
    if (!afp->alwayslocked && afp->lockednow)
        (*afp->funcs.unsetlock) (afp);
    return -1;
}

int AGGRlookup (AGGRfile_t *afp, unsigned char *keyp) {
    if (!afp || !keyp) {
        SUwarning (1, "AGGRlookup", "bad arguments: %x %x", afp, keyp);
        return -1;
    }
    switch (afp->ad.keylen) {
    case -1:
        return _aggrdictlookupvar (afp, keyp, -1, FALSE);
    default:
        return _aggrdictlookupfixed (afp, keyp, -1, FALSE);
    }
}

int AGGRfnames (AGGRfile_t *afp, AGGRfname_t *fnames, int fnamen) {
    int fnamei, len;

    if (!afp || !fnames || fnamen < 0) {
        SUwarning (
            1, "AGGRfnames", "bad arguments: %x %x %d", afp, fnames, fnamen
        );
        return -1;
    }
    len = 0;
    for (fnamei = 0; fnamei < afp->ad.fnamen; fnamei++) {
        len -= strlen (afp->ad.fnames[fnamei].name) + 1;
        vmfree (afp->ad.vm, afp->ad.fnames[fnamei].name);
        len -= 2 * sizeof (int);
    }
    if (afp->ad.fnames)
        vmfree (afp->ad.vm, afp->ad.fnames);
    if (!(afp->ad.fnames = vmalloc (
        afp->ad.vm, fnamen * sizeof (AGGRfname_t)
    ))) {
        SUwarning (1, "AGGRfnames", "malloc failed for fnames");
        return -1;
    }
    afp->ad.fnamen = fnamen;
    for (fnamei = 0; fnamei < fnamen; fnamei++) {
        afp->ad.fnames[fnamei].framei = fnames[fnamei].framei;
        afp->ad.fnames[fnamei].level = fnames[fnamei].level;
        if (!(afp->ad.fnames[fnamei].name = vmstrdup (
            afp->ad.vm, fnames[fnamei].name
        ))) {
            SUwarning (1, "AGGRfnames", "malloc failed for name %d", fnamei);
            return -1;
        }
        len += strlen (afp->ad.fnames[fnamei].name) + 1;
        len += 2 * sizeof (int);
    }
    afp->ad.byten += len;
    return 0;
}

int AGGRminmax (AGGRfile_t *afp, int computeflag) {
    double val, minval, maxval;
    AGGRhdr_t hdr;
    int rwmode;
    int framei, itemi;
    void *datap;
    unsigned char *cp;

    rwmode = -1;
    if (!afp) {
        SUwarning (1, "AGGRminmax", "bad argument: %x", afp);
        return -1;
    }
    if (afp->mapactive) {
        SUwarning (1, "AGGRminmax", "a frame is active");
        return -1;
    }
    if (afp->queuel > 0 && AGGRflush (afp) == -1) {
        SUwarning (1, "AGGRminmax", "flash failed");
        return -1;
    }
    if (!computeflag)
        minval = afp->hdr.minval, maxval = afp->hdr.maxval;
    else
        minval = DBL_MAX, maxval = DBL_MIN;
    if (!afp->alwayslocked) {
        hdr = afp->hdr;
        if ((*afp->funcs.setlock) (afp) == -1 || hdrread (afp) == -1) {
            SUwarning (1, "AGGRminmax", "setlock/hdrread failed");
            goto abortAGGRminmax;
        }
        if (afp->hdr.dictsn != hdr.dictsn || afp->hdr.datasn != hdr.datasn) {
            SUwarning (1, "AGGRminmax", "dictionary changed");
            goto abortAGGRminmax;
        }
    }
    if (computeflag) {
        rwmode = afp->rwmode, afp->rwmode = AGGR_RWMODE_RD;
        for (framei = 0; framei < afp->hdr.framen; framei++) {
            if (!(datap = AGGRget (afp, framei))) {
                SUwarning (1, "AGGRminmax", "get failed");
                goto abortAGGRminmax;
            }
            cp = (unsigned char *) datap;
            switch (afp->hdr.valtype) {
            case AGGR_TYPE_CHAR:
                for (itemi = 0; itemi < afp->ad.itemn; itemi++) {
                    if ((val = SVAL (cp + itemi * afp->hdr.vallen)) < minval)
                        minval = val;
                    if (val > maxval)
                        maxval = val;
                }
                break;
            case AGGR_TYPE_SHORT:
                for (itemi = 0; itemi < afp->ad.itemn; itemi++) {
                    if ((val = SVAL (cp + itemi * afp->hdr.vallen)) < minval)
                        minval = val;
                    if (val > maxval)
                        maxval = val;
                }
                break;
            case AGGR_TYPE_INT:
                for (itemi = 0; itemi < afp->ad.itemn; itemi++) {
                    if ((val = IVAL (cp + itemi * afp->hdr.vallen)) < minval)
                        minval = val;
                    if (val > maxval)
                        maxval = val;
                }
                break;
            case AGGR_TYPE_LLONG:
                for (itemi = 0; itemi < afp->ad.itemn; itemi++) {
                    if ((val = LVAL (cp + itemi * afp->hdr.vallen)) < minval)
                        minval = val;
                    if (val > maxval)
                        maxval = val;
                }
                break;
            case AGGR_TYPE_FLOAT:
                for (itemi = 0; itemi < afp->ad.itemn; itemi++) {
                    if ((val = FVAL (cp + itemi * afp->hdr.vallen)) < minval)
                        minval = val;
                    if (val > maxval)
                        maxval = val;
                }
                break;
            case AGGR_TYPE_DOUBLE:
                for (itemi = 0; itemi < afp->ad.itemn; itemi++) {
                    if ((val = DVAL (cp + itemi * afp->hdr.vallen)) < minval)
                        minval = val;
                    if (val > maxval)
                        maxval = val;
                }
                break;
            }
            if (AGGRrelease (afp) == -1) {
                SUwarning (1, "AGGRminmax", "release failed");
                goto abortAGGRminmax;
            }
        }
        afp->rwmode = rwmode;
    }
    afp->hdr.minval = minval, afp->hdr.maxval = maxval;
    afp->hdr.datasn++;
    if (hdrwrite (afp) == -1) {
        SUwarning (1, "AGGRminmax", "hdrwrite failed");
        goto abortAGGRminmax;
    }
    if (!afp->alwayslocked) {
        if ((*afp->funcs.unsetlock) (afp) == -1) {
            SUwarning (1, "AGGRminmax", "unsetlock failed");
            return -1;
        }
    }
    return 0;

abortAGGRminmax:
    if (rwmode != -1)
        afp->rwmode = rwmode;
    if (!afp->alwayslocked && afp->lockednow)
        (*afp->funcs.unsetlock) (afp);
    return -1;
}

void *AGGRget (AGGRfile_t *afp, int framei) {
    AGGRhdr_t hdr;
#if SWIFT_LITTLEENDIAN == 1
    int cl, vl;
#endif

    if (!afp || framei < 0 || framei >= afp->hdr.framen) {
        SUwarning (1, "AGGRget", "bad arguments: %x %d", afp, framei);
        return NULL;
    }
    if (afp->mapactive) {
        SUwarning (1, "AGGRget", "a frame is active");
        return NULL;
    }
    if (afp->queuel > 0 && AGGRflush (afp) == -1) {
        SUwarning (1, "AGGRget", "flush failed");
        return NULL;
    }
    if (!afp->alwayslocked) {
        hdr = afp->hdr;
        if ((*afp->funcs.setlock) (afp) == -1 || hdrread (afp) == -1) {
            SUwarning (1, "AGGRget", "setlock/hdrread failed");
            goto abortAGGRget;
        }
        if (afp->hdr.dictsn != hdr.dictsn) {
            SUwarning (1, "AGGRget", "dictionary changed");
            if (
                dictterm (afp) == -1 || dictinit (afp) == -1 ||
                dictread (afp) == -1
            ) {
                SUwarning (1, "AGGRget", "dictread failed");
                goto abortAGGRget;
            }
        }
        if (afp->hdr.datasn != hdr.datasn)
            SUwarning (1, "AGGRget", "data changed");
    }
    if (!afp->alwayslocked && afp->rwmode == AGGR_RWMODE_RW) {
        afp->hdr.datasn++;
        if (hdrwrite (afp) == -1) {
            SUwarning (1, "AGGRget", "hdrwrite failed");
            return NULL;
        }
    }
    afp->map.off = (
        HDR_LEN + afp->hdr.dictlen +
        (long long) afp->hdr.itemn * framei * afp->hdr.vallen
    );
    afp->map.len = afp->hdr.itemn * afp->hdr.vallen;
    if ((*afp->funcs.setmap) (afp, &afp->map) == -1) {
        SUwarning (1, "AGGRget", "setmap failed");
        if (!afp->alwayslocked)
            (*afp->funcs.unsetlock) (afp);
        return NULL;
    }
#if SWIFT_LITTLEENDIAN == 1
    cl = typelens[afp->hdr.valtype];
    switch (afp->hdr.valtype) {
    case AGGR_TYPE_SHORT:
        for (vl = 0; vl < afp->map.len; vl += cl)
            SW2 (afp->map.p + vl);
        break;
    case AGGR_TYPE_INT:
        for (vl = 0; vl < afp->map.len; vl += cl)
            SW4 (afp->map.p + vl);
        break;
    case AGGR_TYPE_LLONG:
        for (vl = 0; vl < afp->map.len; vl += cl)
            SW8 (afp->map.p + vl);
        break;
    case AGGR_TYPE_FLOAT:
        for (vl = 0; vl < afp->map.len; vl += cl)
            SW4 (afp->map.p + vl);
        break;
    case AGGR_TYPE_DOUBLE:
        for (vl = 0; vl < afp->map.len; vl += cl)
            SW8 (afp->map.p + vl);
        break;
    }
#endif
    return afp->map.p;

abortAGGRget:
    if (!afp->alwayslocked && afp->lockednow)
        (*afp->funcs.unsetlock) (afp);
    return NULL;
}

int AGGRrelease (AGGRfile_t *afp) {
#if SWIFT_LITTLEENDIAN == 1
    int cl, vl;
#endif

    if (!afp) {
        SUwarning (1, "AGGRrelease", "bad argument: %x", afp);
        return -1;
    }
    if (!afp->mapactive) {
        SUwarning (1, "AGGRrelease", "no frame to release");
        return -1;
    }
#if SWIFT_LITTLEENDIAN == 1
    if (afp->rwmode == AGGR_RWMODE_RW && afp->map.len > 0) {
        cl = typelens[afp->hdr.valtype];
        switch (afp->hdr.valtype) {
        case AGGR_TYPE_SHORT:
            for (vl = 0; vl < afp->map.len; vl += cl)
                SW2 (afp->map.p + vl);
            break;
        case AGGR_TYPE_INT:
            for (vl = 0; vl < afp->map.len; vl += cl)
                SW4 (afp->map.p + vl);
            break;
        case AGGR_TYPE_LLONG:
            for (vl = 0; vl < afp->map.len; vl += cl)
                SW8 (afp->map.p + vl);
            break;
        case AGGR_TYPE_FLOAT:
            for (vl = 0; vl < afp->map.len; vl += cl)
                SW4 (afp->map.p + vl);
            break;
        case AGGR_TYPE_DOUBLE:
            for (vl = 0; vl < afp->map.len; vl += cl)
                SW8 (afp->map.p + vl);
            break;
        }
    }
#endif
    if ((*afp->funcs.unsetmap) (afp, &afp->map) == -1) {
        SUwarning (1, "AGGRrelease", "unsetmap failed");
        if (!afp->alwayslocked)
            (*afp->funcs.unsetlock) (afp);
        return -1;
    }
    if (!afp->alwayslocked) {
        if ((*afp->funcs.unsetlock) (afp) == -1) {
            SUwarning (1, "AGGRrelease", "unsetlock failed");
            return -1;
        }
    }
    return 0;
}

int AGGRserverstart (int *portp) {
    int sfd, slen, rtn;
    struct sockaddr_in sin;

    if ((sfd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
        SUwarning (1, "AGGRserverstart", "cannot create socket");
        return -1;
    }
    do {
        sin.sin_family = AF_INET;
        sin.sin_port = htons ((*portp)++);
        sin.sin_addr.s_addr = INADDR_ANY;
        slen = sizeof (sin);
    } while (
        (rtn = bind (sfd, (struct sockaddr *) &sin, slen)) == -1 &&
        errno == EADDRINUSE
    );
    if (rtn == -1) {
        SUwarning (1, "AGGRserverstart", "cannot bind socket");
        return -1;
    }
    if (listen (sfd, 5) < 0) {
        SUwarning (1, "AGGRserverstart", "cannot listen on socket");
        return -1;
    }
    (*portp)--;
    sfprintf (sfstdout, "port = %d\n", *portp);
    sfsync (NULL);
    return sfd;
}

int AGGRserverstop (int sfd) {
    return close (sfd);
}

AGGRserver_t *AGGRserverget (char *namep) {
    AGGRserver_t *asmemp, *asp;
    char host[PATH_MAX], *portp;
    int port;
    struct hostent *hp;
    struct sockaddr_in sin;
    int cfd;

    strcpy (host, namep + 7);
    *(strchr (host, '/')) = 0;
    if ((asp = dtmatch (asdict, host))) {
        asp->refcount++;
        return asp;
    }
    asmemp = asp = NULL;
    if (
        !(asmemp = vmalloc (aggrvm, sizeof (AGGRserver_t))) ||
        !(asmemp->namep = vmstrdup (aggrvm, namep)) ||
        !(asp = dtinsert (asdict, asmemp)) || asp != asmemp
    ) {
        SUwarning (1, "AGGRserverget", "cannot insert file");
        return NULL;
    }
    if ((portp = strchr (host, ':')))
        port = atoi (portp + 1), *portp = 0;
    else
        port = 22301;
    if (!(hp = gethostbyname (host))) {
        SUwarning (1, "AGGRserverget", "cannot find host %s", host);
        return NULL;
    }
    memset ((char *) &sin, 1, sizeof (sin));
    memcpy ((char *) &sin.sin_addr, hp->h_addr, hp->h_length);
    sin.sin_family = hp->h_addrtype;
    sin.sin_port = htons (port);
    if ((cfd = socket (hp->h_addrtype, SOCK_STREAM, 0)) < 0) {
        SUwarning (1, "AGGRserverget", "cannot create socket");
        return NULL;
    }
    if (connect (cfd, (struct sockaddr *) &sin, sizeof (sin)) < 0) {
        SUwarning (1, "AGGRserverget", "cannot connect");
        return NULL;
    }
    asp->socket = cfd, asp->refcount = 1;
    asp->msgs = NULL, asp->msgn = asp->msgi = asp->msgl = 0;
    if (SUauth (SU_AUTH_CLIENT, cfd, "SWIFTASERVER", SWIFTAUTHKEY) == -1) {
        SUwarning (1, "AGGRserverget", "authentication failed");
        return NULL;
    }
    return asp;
}

int AGGRserverrelease (AGGRserver_t *asp) {
    if (--asp->refcount == 0) {
        close (asp->socket);
        dtdelete (asdict, asp);
        vmfree (aggrvm, asp->namep);
        vmfree (aggrvm, asp);
    }
    return 0;
}

int AGGRserve (AGGRserver_t *asp) {
    int cmd, rtn;
    AGGRfile_t af;
    char name[PATH_MAX];
    AGGRfile_t *afp;
    int afi;
    int createflag, destroyflag, growflag;
    int fd;
    AGGRoff_t off, len;

    static AGGRfile_t *afs;
    static int afn;

    while (msgread (asp) > 0) {
        if (msgget (asp, &cmd, MSG_4, TRUE) == -1) {
            SUwarning (1, "AGGRserve", "read of command id failed");
            return -1;
        }
        SUwarning (
            1, "AGGRserve", "received command %d from client %d",
            cmd, asp->socket
        );
        switch (cmd) {
        case AGGR_CMDcreate:
        case AGGR_CMDopen:
            af.namep = name;
            af.lockednow = FALSE;
            af.mapactive = FALSE;
            createflag = (cmd == AGGR_CMDcreate ? TRUE : FALSE);
            if (
                msgget (asp, af.namep, MSG_S, FALSE) == -1 ||
                msgget (asp, &af.rwmode, MSG_4, TRUE) == -1 ||
                (rtn = localopen (&af, createflag)) == -1 ||
                msgput (asp, &rtn, MSG_4, TRUE) == -1 || msgwrite (asp) == -1
            ) {
                SUwarning (1, "AGGRserve", "create/open failed");
                return -1;
            }
            if (af.fd >= afn) {
                if (!(afs = vmresize (
                    aggrvm, afs, (af.fd + 1) * sizeof (AGGRfile_t), VM_RSMOVE
                ))) {
                    SUwarning (1, "AGGRserve", "vmresize for afs failed");
                    return -1;
                }
                for (afi = afn; afi < af.fd + 1; afi++)
                    afs[afi].fd = -1;
                afn =af.fd + 1;
            }
            afs[af.fd] = af;
            afp = &afs[af.fd];
            afp->map.datap = NULL, afp->map.datan = 0;
            SUwarning (
                1, "AGGRserve", "%d: %s name %s mode %d <> rtn %d", asp->socket,
                (createflag ? "create" : "open"), afp->namep, afp->rwmode, rtn
            );
            break;
        case AGGR_CMDdestroy:
        case AGGR_CMDclose:
            destroyflag = (cmd == AGGR_CMDdestroy ? TRUE : FALSE);
            if (msgget (asp, &fd, MSG_4, TRUE) == -1 || (afs[fd].fd == -1)) {
                SUwarning (1, "AGGRserve", "destroy/close failed");
                return -1;
            }
            afp = &afs[af.fd];
            if (
                (rtn = localclose (afp, destroyflag)) == -1 ||
                msgput (asp, &rtn, MSG_4, TRUE) == -1 ||  msgwrite (asp) == -1
            ) {
                SUwarning (1, "AGGRserve", "destroy/close failed");
                return -1;
            }
            SUwarning (
                1, "AGGRserve", "%d: %s file %d <> rtn %d", asp->socket,
                (destroyflag ? "destroy" : "close"), fd, rtn
            );
            break;
        case AGGR_CMDsetlock:
            if (msgget (asp, &fd, MSG_4, TRUE) == -1 || (afs[fd].fd == -1)) {
                SUwarning (1, "AGGRserve", "setlock failed");
                return -1;
            }
            afp = &afs[af.fd];
            if (localsetlock (afp) == -1) {
                SUwarning (1, "AGGRserve", "setlock failed");
                return -1;
            }
            SUwarning (1, "AGGRserve", "%d: setlock file %d", asp->socket, fd);
            break;
        case AGGR_CMDunsetlock:
            if (msgget (asp, &fd, MSG_4, TRUE) == -1 || (afs[fd].fd == -1)) {
                SUwarning (1, "AGGRserve", "unsetlock failed");
                return -1;
            }
            afp = &afs[af.fd];
            if (localunsetlock (afp) == -1) {
                SUwarning (1, "AGGRserve", "unsetlock failed");
                return -1;
            }
            SUwarning (
                1, "AGGRserve", "%d: unsetlock file %d", asp->socket, fd
            );
            break;
        case AGGR_CMDensurelen:
            if (msgget (asp, &fd, MSG_4, TRUE) == -1 || (afs[fd].fd == -1)) {
                SUwarning (1, "AGGRserve", "ensurelen failed");
                return -1;
            }
            afp = &afs[af.fd];
            if (
                msgget (asp, &len, MSG_8, TRUE) == -1 ||
                msgget (asp, &growflag, MSG_4, TRUE) == -1 ||
                (rtn = localensurelen (afp, len, growflag)) == -1 ||
                msgput (asp, &rtn, MSG_4, TRUE) == -1 || msgwrite (asp) == -1
            ) {
                SUwarning (1, "AGGRserve", "ensurelen failed");
                return -1;
            }
            SUwarning (
                1, "AGGRserve",
                "%d: ensurelen file %d len %lld flag %d <> rtn %d",
                asp->socket, fd, len, growflag, rtn
            );
            break;
        case AGGR_CMDtruncate:
            if (msgget (asp, &fd, MSG_4, TRUE) == -1 || (afs[fd].fd == -1)) {
                SUwarning (1, "AGGRserve", "truncate failed");
                return -1;
            }
            afp = &afs[af.fd];
            if (
                msgget (asp, &len, MSG_8, TRUE) == -1 ||
                localtruncate (afp, len) == -1
            ) {
                SUwarning (1, "AGGRserve", "truncate failed");
                return -1;
            }
            SUwarning (
                1, "AGGRserve", "%d: truncate file %d len %lld",
                asp->socket, fd, len
            );
            break;
        case AGGR_CMDsetmap:
            if (msgget (asp, &fd, MSG_4, TRUE) == -1 || (afs[fd].fd == -1)) {
                SUwarning (1, "AGGRserve", "setmap failed");
                return -1;
            }
            afp = &afs[af.fd];
            afp->mapactive = FALSE;
            if (
                msgget (asp, &afp->map.off, MSG_8, TRUE) == -1 ||
                msgget (asp, &afp->map.len, MSG_8, TRUE) == -1 ||
                (rtn = localsetmap (afp, &afp->map)) == -1 ||
                msgput (asp, &rtn, MSG_4, TRUE) == -1 ||
                msgput (asp, afp->map.p, afp->map.len, FALSE) == -1 ||
                msgwrite (asp) == -1
            ) {
                SUwarning (1, "AGGRserve", "setmap failed");
                return -1;
            }
            SUwarning (
                1, "AGGRserve",
                "%d: setmap file %d off %lld len %lld <> rtn %d",
                asp->socket, fd, afs[fd].map.off, afs[fd].map.len, rtn
            );
            break;
        case AGGR_CMDunsetmap:
            if (msgget (asp, &fd, MSG_4, TRUE) == -1 || (afs[fd].fd == -1)) {
                SUwarning (1, "AGGRserve", "unsetmap failed");
                return -1;
            }
            afp = &afs[af.fd];
            afp->mapactive = FALSE;
            afp->map.off = -1;
            if (
                msgget (asp, &off, MSG_8, TRUE) == -1 ||
                msgget (asp, &afp->map.len, MSG_8, TRUE) == -1 ||
                localsetmap (afp, &afp->map) == -1
            ) {
                SUwarning (1, "AGGRserve", "unsetmap failed");
                return -1;
            }
            afp->mapactive = TRUE;
            afp->map.off = off;
            if (
                msgget (asp, afp->map.p, afp->map.len, FALSE) == -1 ||
                (rtn = localunsetmap (afp, &afp->map)) == -1 ||
                msgput (asp, &rtn, MSG_4, TRUE) == -1 || msgwrite (asp) == -1
            ) {
                SUwarning (1, "AGGRserve", "unsetmap failed");
                return -1;
            }
            SUwarning (
                1, "AGGRserve",
                "%d: unsetmap file %d off %lld len %lld <> rtn %d",
                asp->socket, fd, afs[fd].map.off, afs[fd].map.len, rtn
            );
            break;
        default:
            SUwarning (1, "AGGRserve", "unknown command id %d", cmd);
            break;
        }
    }
    return 0;
}

int AGGRparsetype (char *name, int *typep, int *lenp) {
    char *s;

    s = name;
    if (strncmp (name, "char", 4) == 0)
        *typep = AGGR_TYPE_CHAR, s += 4;
    else if (strncmp (name, "short", 5) == 0)
        *typep = AGGR_TYPE_SHORT, s += 5;
    else if (strncmp (name, "int", 3) == 0)
        *typep = AGGR_TYPE_INT, s += 3;
    else if (strncmp (name, "llong", 5) == 0)
        *typep = AGGR_TYPE_LLONG, s += 5;
    else if (strncmp (name, "float", 5) == 0)
        *typep = AGGR_TYPE_FLOAT, s += 5;
    else if (strncmp (name, "double", 6) == 0)
        *typep = AGGR_TYPE_DOUBLE, s += 6;
    else if (strncmp (name, "string", 6) == 0)
        *typep = AGGR_TYPE_STRING, s += 6;
    else {
        SUwarning (1, "AGGRparsetype", "unknown type %s", name);
        return -1;
    }
    if (!lenp && *typep == AGGR_TYPE_STRING) {
        SUwarning (1, "AGGRparsetype", "cannot have string as value type");
        return -1;
    }
    if (*s == ':') {
        if ((*lenp = atoi (s + 1) * typelens[*typep]) <= 0) {
            SUwarning (1, "AGGRparsetype", "non-positive number of elements");
            return -1;
        }
        return 0;
    }
    if (lenp)
        *lenp = typelens[*typep];
    return 0;
}

int AGGRparsefnames (char *names, AGGRfname_t **fnamesp, int *fnamenp) {
    AGGRfname_t *fnames;
    int fnamen, fnamei;
    char *tp, *s;
    int l;
    char c;

    fnamen = strlen (names) / 6 + 16;
    if (!(fnames = vmalloc (Vmheap, fnamen * sizeof (AGGRfname_t)))) {
        SUwarning (1, "AGGRparsefnames", "malloc failed for fnames");
        return -1;
    }
    fnamei = 0;
    tp = tokopen (names, 1);
    while ((s = tokread (tp))) {
        fnames[fnamei].framei = atoi (s);
        if (!(s = tokread (tp))) {
            SUwarning (1, "AGGRparsefnames", "bad frame name string");
            return -1;
        }
        fnames[fnamei].level = atoi (s);
        if (!(s = tokread (tp))) {
            SUwarning (1, "AGGRparsefnames", "bad frame name string");
            return -1;
        }
        l = strlen (s), c = 0;
        if (s[0] == '"' && s[l - 1] == '"')
            c = '"', s[l - 1] = 0, s++;
        else if (s[0] == '\'' && s[l - 1] == '\'')
            c = '\'', s[l - 1] = 0, s++;
        if (!(fnames[fnamei].name = vmstrdup (Vmheap, s))) {
            SUwarning (
                1, "AGGRparsefnames", "malloc failed for name %d", fnamei
            );
            return -1;
        }
        if (c > 0)
            s--, s[0] = s[l - 1] = c;
        fnamei++;
    }
    tokclose (tp);
    *fnamesp = fnames, *fnamenp = fnamei;
    return 0;
}

int _aggrgrow (AGGRfile_t *afp, int itemn, int framen) {
    int datahaschanged, movedata, framei;
    long long dictlen;
    AGGRsize_t olen;

    datahaschanged = FALSE;
    dictlen = DICTLEN (afp);
    movedata = FALSE;
    if (itemn == -1)
        itemn = (
            afp->hdr.itemn > afp->ad.itemn ? afp->hdr.itemn : afp->ad.itemn
        );
    itemn = ALIGN (itemn, afp->hdr.itemincr);
    if (framen == -1)
        framen = afp->hdr.framen;
    if (
        (afp->dicthaschanged && dictlen > afp->hdr.dictlen) ||
        itemn > afp->hdr.itemn
    )
        movedata = TRUE;
    if ((*afp->funcs.ensurelen) (
        afp, dictlen + (long long) itemn * framen * afp->hdr.vallen, TRUE
    ) == -1) {
        SUwarning (1, "grow", "ensurelen failed");
        return -1;
    }
    if (framen > afp->hdr.framen) {
        for (framei = framen - 1; framei >= afp->hdr.framen; framei--) {
            afp->map.off = -1;
            afp->map.len = itemn * afp->hdr.vallen;
            if ((*afp->funcs.setmap) (afp, &afp->map) == -1) {
                SUwarning (1, "grow", "setmap failed");
                return -1;
            }
            memset (afp->map.p, 0, afp->map.len);
            afp->map.off = (
                HDR_LEN + dictlen + (long long) itemn * framei * afp->hdr.vallen
            );
            if ((*afp->funcs.unsetmap) (afp, &afp->map) == -1) {
                SUwarning (1, "grow", "unsetmap failed");
                return -1;
            }
        }
        datahaschanged = TRUE;
    }
    if (movedata) {
        for (framei = afp->hdr.framen - 1; framei >= 0; framei--) {
            afp->map.off = (
                HDR_LEN + afp->hdr.dictlen +
                (long long) afp->hdr.itemn * framei * afp->hdr.vallen
            );
            afp->map.len = itemn * afp->hdr.vallen;
            if ((*afp->funcs.setmap) (afp, &afp->map) == -1) {
                SUwarning (1, "grow", "setmap failed");
                return -1;
            }
            olen = afp->hdr.itemn * afp->hdr.vallen;
            memset (afp->map.p + olen, 0, afp->map.len - olen);
            afp->map.off = (
                HDR_LEN + dictlen + (long long) itemn * framei * afp->hdr.vallen
            );
            if ((*afp->funcs.unsetmap) (afp, &afp->map) == -1) {
                SUwarning (1, "grow", "unsetmap failed");
                return -1;
            }
        }
        datahaschanged = TRUE;
    }
    afp->hdr.dictlen = dictlen;
    if (afp->dicthaschanged && dictwrite (afp) == -1) {
        SUwarning (1, "grow", "dictwrite failed");
        return -1;
    }
    if (afp->dicthaschanged)
        afp->hdr.dictsn++, afp->dicthaschanged = FALSE;
    if (datahaschanged)
        afp->hdr.datasn++;
    afp->hdr.itemn = itemn, afp->hdr.framen = framen;
    afp->hdr.datalen = DATALEN (afp);
    if (hdrwrite (afp) == -1) {
        SUwarning (1, "grow", "hdrwrite failed");
        return -1;
    }
    return 0;
}

/* dictionary functions */

static int dictinit (AGGRfile_t *afp) {
    if (!(afp->ad.vm = vmopen (&aggrheap, Vmbest, VMFLAGS))) {
        SUwarning (1, "dictinit", "vmopen failed");
        return -1;
    }
    afp->ad.keylen = -2, afp->ad.keyn = 0, afp->ad.byten = 0, afp->ad.itemn = 0;
    afp->ad.fnames = NULL, afp->ad.fnamen = 0;
    afp->ad.rootp = NULL;
    if (!(afp->ad.stacks = vmalloc (afp->ad.vm, 10 * sizeof (AGGRstack_t)))) {
        SUwarning (1, "dictinit", "vmalloc failed");
        return -1;
    }
    afp->ad.stackn = 10, afp->ad.stackl = 0;
    return 0;
}

static int dictterm (AGGRfile_t *afp) {
    vmclose (afp->ad.vm);
    return 0;
}

int _aggrdictlookupvar (
    AGGRfile_t *afp, unsigned char *keyp, int itemi, int insertflag
) {
    AGGRtree_t node, *lnodep, *rnodep, *tnodep, *rootp, *newnodep;
    int order;

    if ((rootp = afp->ad.rootp)) {
        node.nodeps[0] = node.nodeps[1] = NULL;
        lnodep = rnodep = &node;
        for (;;) {
            if ((order = strcmp ((char *) keyp, (char *) rootp->keyp)) == 0)
                break;
            if (order < 0) {
                if (!rootp->nodeps[0])
                    break;
                if ((order = strcmp (
                    (char *) keyp, (char *) rootp->nodeps[0]->keyp
                )) < 0) {
                    tnodep = rootp->nodeps[0];
                    rootp->nodeps[0] = tnodep->nodeps[1];
                    tnodep->nodeps[1] = rootp;
                    rootp = tnodep;
                    if (!rootp->nodeps[0])
                        break;
                }
                rnodep->nodeps[0] = rootp;
                rnodep = rootp;
                rootp = rootp->nodeps[0];
            } else {
                if (!rootp->nodeps[1])
                    break;
                if ((order = strcmp (
                    (char *) keyp, (char *) rootp->nodeps[1]->keyp
                )) > 0) {
                    tnodep = rootp->nodeps[1];
                    rootp->nodeps[1] = tnodep->nodeps[0];
                    tnodep->nodeps[0] = rootp;
                    rootp = tnodep;
                    if (!rootp->nodeps[1])
                        break;
                }
                lnodep->nodeps[1] = rootp;
                lnodep = rootp;
                rootp = rootp->nodeps[1];
            }
        }
        lnodep->nodeps[1] = rootp->nodeps[0];
        rnodep->nodeps[0] = rootp->nodeps[1];
        rootp->nodeps[0] = node.nodeps[1];
        rootp->nodeps[1] = node.nodeps[0];
        afp->ad.rootp = rootp;
    }
    if (rootp && (order = strcmp ((char *) keyp, (char *) rootp->keyp)) == 0)
        return rootp->itemi;
    if (!insertflag)
        return -1;
    if (
        !(newnodep = vmalloc (afp->ad.vm, sizeof (AGGRtree_t))) ||
        !(newnodep->keyp = (unsigned char *) vmstrdup (
            afp->ad.vm, (char *) keyp
        ))
    ) {
        SUwarning (1, "dictlookupvar", "vmalloc/vmstrdup failed");
        return -1;
    }
    afp->ad.byten += strlen ((char *) keyp) + 1 + sizeof (int);
    if (itemi == -1)
        newnodep->itemi = afp->ad.itemn++;
    else {
        newnodep->itemi = itemi;
        if (itemi >= afp->ad.itemn)
            afp->ad.itemn = itemi + 1;
    }
    if (!rootp)
        newnodep->nodeps[0] = newnodep->nodeps[1] = NULL;
    else if (order < 0) {
        newnodep->nodeps[0] = rootp->nodeps[0];
        newnodep->nodeps[1] = rootp;
        rootp->nodeps[0] = NULL;
    } else {
        newnodep->nodeps[1] = rootp->nodeps[1];
        newnodep->nodeps[0] = rootp;
        rootp->nodeps[1] = NULL;
    }
    afp->ad.rootp = newnodep;
    afp->ad.keyn++;
    afp->dicthaschanged = TRUE;
    return newnodep->itemi;
}

int _aggrdictlookupfixed (
    AGGRfile_t *afp, unsigned char *keyp, int itemi, int insertflag
) {
    AGGRtree_t node, *lnodep, *rnodep, *tnodep, *rootp, *newnodep;
    int order;

    if ((rootp = afp->ad.rootp)) {
        node.nodeps[0] = node.nodeps[1] = NULL;
        lnodep = rnodep = &node;
        for (;;) {
            if ((order = memcmp (keyp, rootp->keyp, afp->ad.keylen)) == 0)
                break;
            if (order < 0) {
                if (!rootp->nodeps[0])
                    break;
                if ((order = memcmp (
                    keyp, rootp->nodeps[0]->keyp, afp->ad.keylen
                )) < 0) {
                    tnodep = rootp->nodeps[0];
                    rootp->nodeps[0] = tnodep->nodeps[1];
                    tnodep->nodeps[1] = rootp;
                    rootp = tnodep;
                    if (!rootp->nodeps[0])
                        break;
                }
                rnodep->nodeps[0] = rootp;
                rnodep = rootp;
                rootp = rootp->nodeps[0];
            } else {
                if (!rootp->nodeps[1])
                    break;
                if ((order = memcmp (
                    keyp, rootp->nodeps[1]->keyp, afp->ad.keylen
                )) > 0) {
                    tnodep = rootp->nodeps[1];
                    rootp->nodeps[1] = tnodep->nodeps[0];
                    tnodep->nodeps[0] = rootp;
                    rootp = tnodep;
                    if (!rootp->nodeps[1])
                        break;
                }
                lnodep->nodeps[1] = rootp;
                lnodep = rootp;
                rootp = rootp->nodeps[1];
            }
        }
        lnodep->nodeps[1] = rootp->nodeps[0];
        rnodep->nodeps[0] = rootp->nodeps[1];
        rootp->nodeps[0] = node.nodeps[1];
        rootp->nodeps[1] = node.nodeps[0];
        afp->ad.rootp = rootp;
    }
    if (rootp && (order = memcmp (keyp, rootp->keyp, afp->ad.keylen)) == 0)
        return rootp->itemi;
    if (!insertflag)
        return -1;
    if (
        !(newnodep = vmalloc (afp->ad.vm, sizeof (AGGRtree_t))) ||
        !(newnodep->keyp = vmalloc (afp->ad.vm, afp->ad.keylen))
    ) {
        SUwarning (1, "dictlookupfixed", "vmalloc failed");
        return -1;
    }
    memcpy (newnodep->keyp, keyp, afp->ad.keylen);
    afp->ad.byten += afp->ad.keylen + sizeof (int);
    if (itemi == -1)
        newnodep->itemi = afp->ad.itemn++;
    else {
        newnodep->itemi = itemi;
        if (itemi >= afp->ad.itemn)
            afp->ad.itemn = itemi + 1;
    }
    if (!rootp)
        newnodep->nodeps[0] = newnodep->nodeps[1] = NULL;
    else if (order < 0) {
        newnodep->nodeps[0] = rootp->nodeps[0];
        newnodep->nodeps[1] = rootp;
        rootp->nodeps[0] = NULL;
    } else {
        newnodep->nodeps[1] = rootp->nodeps[1];
        newnodep->nodeps[0] = rootp;
        rootp->nodeps[1] = NULL;
    }
    afp->ad.rootp = newnodep;
    afp->ad.keyn++;
    afp->dicthaschanged = TRUE;
    return newnodep->itemi;
}

int _aggrdictmerge (
    AGGRfile_t **iafps, int iafpn, merge_t *merges, AGGRfile_t *oafp
) {
    int keylen;
    AGGRfile_t *iafp;
    int iafpi;
    merge_t *mergep;
    AGGRstack_t *stackp;
    unsigned char *minkeyp;
    int itemi;

    keylen = iafps[0]->ad.keylen;
    for (iafpi = 0; iafpi < iafpn; iafpi++) {
        iafp = iafps[iafpi];
        mergep = &merges[iafpi];
        iafp->ad.stackl = 0;
        if (iafp->ad.rootp) {
            stackp = &iafp->ad.stacks[iafp->ad.stackl++];
            stackp->nodep = iafp->ad.rootp;
            stackp->state = 0;
        }
        if (mergep->maps)
            for (itemi = 0; itemi < iafp->ad.itemn; itemi++)
                mergep->maps[itemi] = -1;
    }
    for (;;) {
        for (iafpi = 0; iafpi < iafpn; iafpi++) {
            iafp = iafps[iafpi];
            mergep = &merges[iafpi];
            if (iafp->ad.stackl == 0) {
                mergep->keyp = NULL;
                continue;
            }
again:
            if (
                iafp->ad.stackl >= iafp->ad.stackn && dictgrowstack (iafp) == -1
            ) {
                SUwarning (1, "dictmerge", "dictgrowstack failed");
                return -1;
            }
            stackp = &iafp->ad.stacks[iafp->ad.stackl - 1];
            if (stackp->state == 0) {
                if (stackp->nodep->nodeps[0]) {
                    iafp->ad.stacks[iafp->ad.stackl].nodep = (
                        stackp->nodep->nodeps[0]
                    );
                    iafp->ad.stacks[iafp->ad.stackl++].state = 0;
                }
                stackp->state++;
                goto again;
            }
            mergep->keyp = stackp->nodep->keyp;
        }
        minkeyp = NULL;
        for (iafpi = 0; iafpi < iafpn; iafpi++) {
            mergep = &merges[iafpi];
            if (!mergep->keyp)
                continue;
            if (!minkeyp)
                minkeyp = mergep->keyp;
            else {
                if ((
                    keylen == -1 ? strcmp (
                        (char *) minkeyp, (char *) mergep->keyp
                    ) : memcmp (minkeyp, mergep->keyp, keylen)
                ) > 0)
                    minkeyp = mergep->keyp;
            }
        }
        if (!minkeyp)
            break;
        switch (keylen) {
        case -1:
            itemi = _aggrdictlookupvar (oafp, minkeyp, -1, TRUE);
            break;
        default:
            itemi = _aggrdictlookupfixed (oafp, minkeyp, -1, TRUE);
            break;
        }
        if (itemi == -1) {
            SUwarning (1, "dictmerge", "dictlookup failed");
            return -1;
        }
        for (iafpi = 0; iafpi < iafpn; iafpi++) {
            iafp = iafps[iafpi];
            mergep = &merges[iafpi];
            if (!mergep->keyp)
                continue;
            if ((
                keylen == -1 ? strcmp (
                    (char *) minkeyp, (char *) mergep->keyp
                ) : memcmp (minkeyp, mergep->keyp, keylen)
            ) == 0) {
                stackp = &iafp->ad.stacks[iafp->ad.stackl - 1];
                if (mergep->maps)
                    mergep->maps[stackp->nodep->itemi] = itemi;
                if (stackp->nodep->nodeps[1]) {
                    stackp->nodep = stackp->nodep->nodeps[1];
                    stackp->state = 0;
                } else
                    iafp->ad.stackl--;
            }
        }
    }
    return 0;
}

int _aggrdictcopy (AGGRfile_t *iafp, AGGRfile_t *oafp) {
    AGGRstack_t *stackp;
    int stackl;
    int itemi;

    if (!iafp->ad.rootp)
        return 0;
    stackl = 0;
    stackp = &iafp->ad.stacks[stackl++];
    stackp->nodep = iafp->ad.rootp;
    stackp->state = 0;
    while (stackl > 0) {
        if (stackl >= iafp->ad.stackn && dictgrowstack (iafp) == -1) {
            SUwarning (1, "dictcopy", "dictgrowstack failed");
            return -1;
        }
        stackp = &iafp->ad.stacks[stackl - 1];
        if (stackp->state == 0) {
            if (stackp->nodep->nodeps[0]) {
                iafp->ad.stacks[stackl].nodep = stackp->nodep->nodeps[0];
                iafp->ad.stacks[stackl++].state = 0;
            }
            stackp->state++;
            continue;
        }
        if (iafp->ad.keylen == -1)
            itemi = _aggrdictlookupvar (
                oafp, stackp->nodep->keyp, stackp->nodep->itemi, TRUE
            );
        else
            itemi = _aggrdictlookupfixed (
                oafp, stackp->nodep->keyp, stackp->nodep->itemi, TRUE
            );
        if (itemi != stackp->nodep->itemi) {
            SUwarning (1, "dictcopy", "lookup failed");
            return -1;
        }
        if (stackp->nodep->nodeps[1]) {
            stackp->nodep = stackp->nodep->nodeps[1];
            stackp->state = 0;
        } else
            stackl--;
    }
    return 0;
}

static int dictread (AGGRfile_t *afp) {
    unsigned char *strp;
    AGGRtree_t *treep;
    int treei, fi, mi, li;
    unsigned char *keystrp;
    AGGRstack_t *stackp;
    int stackl;
    int fnamei, len;

    afp->map.off = HDR_LEN;
    afp->map.len = afp->hdr.dictlen;
    if ((*afp->funcs.setmap) (afp, &afp->map) == -1) {
        SUwarning (1, "dictread", "setmap failed");
        return -1;
    }
    strp = afp->map.p;
    SW4 (strp), memcpy (&afp->ad.keylen, strp, sizeof (int));
    strp += sizeof (int);
    SW4 (strp), memcpy (&afp->ad.keyn, strp, sizeof (int));
    strp += sizeof (int);
    if (afp->hdr.version < 5) {
        int byten;
        SW4 (strp), memcpy (&byten, strp, sizeof (int));
        strp += sizeof (int);
        afp->ad.byten = byten;
    } else {
        SW8 (strp), memcpy (&afp->ad.byten, strp, sizeof (long long));
        strp += sizeof (long long);
    }
    SW4 (strp), memcpy (&afp->ad.itemn, strp, sizeof (int));
    strp += sizeof (int);
    if (afp->hdr.version < 4)
        afp->ad.fnamen = 0;
    else {
        SW4 (strp), memcpy (&afp->ad.fnamen, strp, sizeof (int));
        strp += sizeof (int);
    }
    if (!(afp->ad.fnames = vmalloc (
        afp->ad.vm, afp->ad.fnamen * sizeof (AGGRfname_t)
    ))) {
        SUwarning (1, "dictread", "malloc failed for fnames");
        return -1;
    }
    if (afp->ad.byten > 0) {
        for (fnamei = 0; fnamei < afp->ad.fnamen; fnamei++) {
            SW4 (strp);
            memcpy (&afp->ad.fnames[fnamei].framei, strp, sizeof (int));
            strp += sizeof (int);
            SW4 (strp);
            memcpy (&afp->ad.fnames[fnamei].level, strp, sizeof (int));
            strp += sizeof (int);
            afp->ad.fnames[fnamei].name = vmstrdup (afp->ad.vm, (char *) strp);
            strp += strlen ((char *) strp) + 1;
        }
        if (!(treep = vmalloc (
            afp->ad.vm,
            afp->ad.keyn * (sizeof (AGGRtree_t) - sizeof (int)) + afp->ad.byten
        ))) {
            SUwarning (1, "dictread", "vmalloc failed");
            return -1;
        }
        keystrp = (unsigned char *) (treep + afp->ad.keyn);
        stackl = 0;
        stackp = &afp->ad.stacks[stackl++];
        stackp->fi = 0, stackp->li = afp->ad.keyn - 1;
        while (stackl > 0) {
            if (stackl + 2 > afp->ad.stackn && dictgrowstack (afp) == -1) {
                SUwarning (1, "dictread", "dictgrowstack failed");
                return -1;
            }
            stackp = &afp->ad.stacks[--stackl];
            fi = stackp->fi, li = stackp->li;
            mi = (li + fi) / 2;
            treep[mi].nodeps[0] = treep[mi].nodeps[1] = NULL;
            if (mi - fi > 0) {
                afp->ad.stacks[stackl].fi = fi;
                afp->ad.stacks[stackl++].li = mi - 1;
                treep[mi].nodeps[0] = &treep[(mi - 1 + fi) / 2];
            }
            if (li - mi > 0) {
                afp->ad.stacks[stackl].fi = mi + 1;
                afp->ad.stacks[stackl++].li = li;
                treep[mi].nodeps[1] = &treep[(li + mi + 1) / 2];
            }
        }
        for (treei = 0; treei < afp->ad.keyn; treei++) {
            treep[treei].keyp = keystrp;
            if (afp->ad.keylen == -1)
                len = strlen ((char *) strp) + 1;
            else
                len = afp->ad.keylen;
            memcpy (treep[treei].keyp, strp, len);
            keystrp += len, strp += len;
            memcpy (&treep[treei].itemi, strp, sizeof (int));
#if SWIFT_LITTLEENDIAN == 1
            switch (afp->hdr.keytype) {
                int kl;
            case AGGR_TYPE_SHORT:
                for (kl = 0; kl < len; kl += typelens[AGGR_TYPE_SHORT])
                    SW2 (treep[treei].keyp + kl);
                break;
            case AGGR_TYPE_INT:
                for (kl = 0; kl < len; kl += typelens[AGGR_TYPE_INT])
                    SW4 (treep[treei].keyp + kl);
                break;
            case AGGR_TYPE_LLONG:
                for (kl = 0; kl < len; kl += typelens[AGGR_TYPE_LLONG])
                    SW8 (treep[treei].keyp + kl);
                break;
            case AGGR_TYPE_FLOAT:
                for (kl = 0; kl < len; kl += typelens[AGGR_TYPE_FLOAT])
                    SW4 (treep[treei].keyp + kl);
                break;
            case AGGR_TYPE_DOUBLE:
                for (kl = 0; kl < len; kl += typelens[AGGR_TYPE_DOUBLE])
                    SW8 (treep[treei].keyp + kl);
                break;
            }
            SW4 (&treep[treei].itemi);
#endif
            strp += sizeof (int);
        }
        afp->ad.rootp = treep + (afp->ad.keyn - 1) / 2;
    }
    afp->map.len = 0;
    if ((*afp->funcs.unsetmap) (afp, &afp->map) == -1) {
        SUwarning (1, "dictread", "unsetmap failed");
        return -1;
    }
    return 0;
}

static int dictwrite (AGGRfile_t *afp) {
    unsigned char *strp;
    AGGRstack_t *stackp;
    int stackl;
    int fnamei, len;

    if ((*afp->funcs.ensurelen) (afp, afp->hdr.dictlen, TRUE) == -1) {
        SUwarning (1, "dictwrite", "ensurelen failed");
        return -1;
    }
    afp->map.off = -1;
    afp->map.len = afp->hdr.dictlen;
    if ((*afp->funcs.setmap) (afp, &afp->map) == -1) {
        SUwarning (1, "dictwrite", "setmap failed");
        return -1;
    }
    strp = afp->map.p;
    memcpy (strp, &afp->ad.keylen, sizeof (int)), SW4 (strp);
    strp += sizeof (int);
    memcpy (strp, &afp->ad.keyn, sizeof (int)), SW4 (strp);
    strp += sizeof (int);
    memcpy (strp, &afp->ad.byten, sizeof (long long)), SW8 (strp);
    strp += sizeof (long long);
    memcpy (strp, &afp->ad.itemn, sizeof (int)), SW4 (strp);
    strp += sizeof (int);
    memcpy (strp, &afp->ad.fnamen, sizeof (int)), SW4 (strp);
    strp += sizeof (int);
    for (fnamei = 0; fnamei < afp->ad.fnamen; fnamei++) {
        memcpy (strp, &afp->ad.fnames[fnamei].framei, sizeof (int)), SW4 (strp);
        strp += sizeof (int);
        memcpy (strp, &afp->ad.fnames[fnamei].level, sizeof (int)), SW4 (strp);
        strp += sizeof (int);
        len = strlen (afp->ad.fnames[fnamei].name) + 1;
        memcpy (strp, afp->ad.fnames[fnamei].name, len);
        strp += len;
    }
    if (afp->ad.rootp) {
        stackl = 0;
        stackp = &afp->ad.stacks[stackl++];
        stackp->nodep = afp->ad.rootp;
        stackp->state = 0;
        while (stackl > 0) {
            if (stackl >= afp->ad.stackn && dictgrowstack (afp) == -1) {
                SUwarning (1, "dictwrite", "dictgrowstack failed");
                return -1;
            }
            stackp = &afp->ad.stacks[stackl - 1];
            if (stackp->state == 0) {
                if (stackp->nodep->nodeps[0]) {
                    afp->ad.stacks[stackl].nodep = stackp->nodep->nodeps[0];
                    afp->ad.stacks[stackl++].state = 0;
                }
                stackp->state++;
                continue;
            }
            if (afp->ad.keylen == -1)
                len = strlen ((char *) stackp->nodep->keyp) + 1;
            else
                len = afp->ad.keylen;
            memcpy (strp, stackp->nodep->keyp, len);
            memcpy (strp + len, &stackp->nodep->itemi, sizeof (int));
#if SWIFT_LITTLEENDIAN == 1
            switch (afp->hdr.keytype) {
                int kl;
            case AGGR_TYPE_SHORT:
                for (kl = 0; kl < len; kl += typelens[AGGR_TYPE_SHORT])
                    SW2 (strp + kl);
                break;
            case AGGR_TYPE_INT:
                for (kl = 0; kl < len; kl += typelens[AGGR_TYPE_INT])
                    SW4 (strp + kl);
                break;
            case AGGR_TYPE_LLONG:
                for (kl = 0; kl < len; kl += typelens[AGGR_TYPE_LLONG])
                    SW8 (strp + kl);
                break;
            case AGGR_TYPE_FLOAT:
                for (kl = 0; kl < len; kl += typelens[AGGR_TYPE_FLOAT])
                    SW4 (strp + kl);
                break;
            case AGGR_TYPE_DOUBLE:
                for (kl = 0; kl < len; kl += typelens[AGGR_TYPE_DOUBLE])
                    SW8 (strp + kl);
                break;
            }
            SW4 (strp + len);
#endif
            strp += len + sizeof (int);
            if (stackp->nodep->nodeps[1]) {
                stackp->nodep = stackp->nodep->nodeps[1];
                stackp->state = 0;
            } else
                stackl--;
        }
    }
    while (strp - afp->map.p < afp->map.len)
        *strp++ = 0;
    afp->map.off = HDR_LEN;
    if ((*afp->funcs.unsetmap) (afp, &afp->map) == -1) {
        SUwarning (1, "dictwrite", "unsetmap failed");
        return -1;
    }
    return 0;
}

void _aggrdictreverse (AGGRfile_t *afp, unsigned char **revmap) {
    int itemi;
    AGGRstack_t *stackp;
    int stackl;

    for (itemi = 0; itemi < afp->ad.itemn; itemi++)
        revmap[itemi] = NULL;
    if (afp->ad.rootp) {
        stackl = 0;
        stackp = &afp->ad.stacks[stackl++];
        stackp->nodep = afp->ad.rootp;
        stackp->state = 0;
        while (stackl > 0) {
            if (stackl >= afp->ad.stackn && dictgrowstack (afp) == -1) {
                SUwarning (1, "dictreverse", "dictgrowstack failed");
                return;
            }
            stackp = &afp->ad.stacks[stackl - 1];
            if (stackp->state == 0) {
                if (stackp->nodep->nodeps[0]) {
                    afp->ad.stacks[stackl].nodep = stackp->nodep->nodeps[0];
                    afp->ad.stacks[stackl++].state = 0;
                }
                stackp->state++;
                continue;
            }
            revmap[stackp->nodep->itemi] = stackp->nodep->keyp;
            if (stackp->nodep->nodeps[1]) {
                stackp->nodep = stackp->nodep->nodeps[1];
                stackp->state = 0;
            } else
                stackl--;
        }
    }
}

static void dictdump (AGGRfile_t *afp, int onlyitemi) {
    AGGRstack_t *stackp;
    int stackl;

    if (afp->ad.rootp) {
        stackl = 0;
        stackp = &afp->ad.stacks[stackl++];
        stackp->nodep = afp->ad.rootp;
        stackp->state = 0;
        while (stackl > 0) {
            if (stackl >= afp->ad.stackn && dictgrowstack (afp) == -1) {
                SUwarning (1, "dictdump", "dictgrowstack failed");
                return;
            }
            stackp = &afp->ad.stacks[stackl - 1];
            if (stackp->state == 0) {
                if (stackp->nodep->nodeps[0]) {
                    afp->ad.stacks[stackl].nodep = stackp->nodep->nodeps[0];
                    afp->ad.stacks[stackl++].state = 0;
                }
                stackp->state++;
                continue;
            }
            if (onlyitemi == -1 || stackp->nodep->itemi == onlyitemi) {
                _aggrdictprintkey (afp, sfstdout, stackp->nodep->keyp);
                sfprintf (sfstdout, " -> %d\n", stackp->nodep->itemi);
            }
            if (stackp->nodep->nodeps[1]) {
                stackp->nodep = stackp->nodep->nodeps[1];
                stackp->state = 0;
            } else
                stackl--;
        }
    }
}

int _aggrdictscankey (AGGRfile_t *afp, char *sp, unsigned char **keypp) {
    AGGRunit_t ku;
    unsigned char *keyp;
    int keyi, typel, keyl;
    char *s;

    keyi = 0, keyl = afp->ad.keylen;
    typel = typelens[afp->hdr.keytype];
    switch (afp->hdr.keytype) {
    case AGGR_TYPE_CHAR:
        for (
            keyp = *keypp, keyi = 0; sp && keyi < keyl; sp = s, keyi += typel
        ) {
            if ((s = strchr (sp, ':')))
                *s++ = 0;
            ku.u.c = atoi (sp);
            memcpy (keyp + keyi, &ku.u.c, typel);
        }
        break;
    case AGGR_TYPE_SHORT:
        for (
            keyp = *keypp, keyi = 0; sp && keyi < keyl; sp = s, keyi += typel
        ) {
            if ((s = strchr (sp, ':')))
                *s++ = 0;
            ku.u.s = atoi (sp);
            memcpy (keyp + keyi, &ku.u.s, typel);
        }
        break;
    case AGGR_TYPE_INT:
        for (
            keyp = *keypp, keyi = 0; sp && keyi < keyl; sp = s, keyi += typel
        ) {
            if ((s = strchr (sp, ':')))
                *s++ = 0;
            ku.u.i = atoi (sp);
            memcpy (keyp + keyi, &ku.u.i, typel);
        }
        break;
    case AGGR_TYPE_LLONG:
        for (
            keyp = *keypp, keyi = 0; sp && keyi < keyl; sp = s, keyi += typel
        ) {
            if ((s = strchr (sp, ':')))
                *s++ = 0;
            ku.u.l = strtoll (sp, NULL, 10);
            memcpy (keyp + keyi, &ku.u.l, typel);
        }
        break;
    case AGGR_TYPE_FLOAT:
        for (
            keyp = *keypp, keyi = 0; sp && keyi < keyl; sp = s, keyi += typel
        ) {
            if ((s = strchr (sp, ':')))
                *s++ = 0;
            ku.u.f = atof (sp);
            memcpy (keyp + keyi, &ku.u.f, typel);
        }
        break;
    case AGGR_TYPE_DOUBLE:
        for (
            keyp = *keypp, keyi = 0; sp && keyi < keyl; sp = s, keyi += typel
        ) {
            if ((s = strchr (sp, ':')))
                *s++ = 0;
            ku.u.d = atof (sp);
            memcpy (keyp + keyi, &ku.u.d, typel);
        }
        break;
    case AGGR_TYPE_STRING:
        *keypp = (unsigned char *) sp;
        break;
    }
    if (keyl != -1) {
        if (keyi != keyl) {
            SUwarning (1, "dictscankey", "not enough elements in key");
            return -1;
        } else if (sp) {
            SUwarning (1, "dictscankey", "too many elements in key");
            return -1;
        }
    }
    return 0;
}

void _aggrdictprintkey (AGGRfile_t *afp, Sfio_t *fp, unsigned char *keyp) {
    AGGRunit_t ku;
    int keyi, typel, keyl;

    if (!keyp) {
        sfprintf (fp, "\"\"");
        return;
    }
    keyl = afp->ad.keylen;
    typel = typelens[afp->hdr.keytype];
    switch (afp->hdr.keytype) {
    case TYPE_CHAR:
        for (keyi = 0; keyi < keyl; keyi += typel) {
            memcpy (&ku.u.c, keyp + keyi, typel);
            sfprintf (fp, "%s%d", (keyi > 0) ? ":" : "", ku.u.c);
        }
        break;
    case TYPE_SHORT:
        for (keyi = 0; keyi < keyl; keyi += typel) {
            memcpy (&ku.u.s, keyp + keyi, typel);
            sfprintf (fp, "%s%d", (keyi > 0) ? ":" : "", ku.u.s);
        }
        break;
    case TYPE_INT:
        for (keyi = 0; keyi < keyl; keyi += typel) {
            memcpy (&ku.u.i, keyp + keyi, typel);
            sfprintf (fp, "%s%d", (keyi > 0) ? ":" : "", ku.u.i);
        }
        break;
    case TYPE_LLONG:
        for (keyi = 0; keyi < keyl; keyi += typel) {
            memcpy (&ku.u.l, keyp + keyi, typel);
            sfprintf (fp, "%s%d", (keyi > 0) ? ":" : "", ku.u.l);
        }
        break;
    case TYPE_FLOAT:
        for (keyi = 0; keyi < keyl; keyi += typel) {
            memcpy (&ku.u.f, keyp + keyi, typel);
            sfprintf (fp, "%s%f", (keyi > 0) ? ":" : "", ku.u.f);
        }
        break;
    case TYPE_DOUBLE:
        for (keyi = 0; keyi < keyl; keyi += typel) {
            memcpy (&ku.u.d, keyp + keyi, typel);
            sfprintf (fp, "%s%f", (keyi > 0) ? ":" : "", ku.u.d);
        }
        break;
    case TYPE_STRING:
        if (keyl == -1) {
            sfprintf (fp, "\"%s\"", keyp);
        } else {
            sfprintf (fp, "\"");
            sfwrite (fp, keyp, keyl);
            sfprintf (fp, "\"");
        }
        break;
    }
}

static int dictgrowstack (AGGRfile_t *afp) {
    if (!(afp->ad.stacks = vmresize (
        afp->ad.vm, afp->ad.stacks,
        afp->ad.stackn * 2 * sizeof (AGGRstack_t), VM_RSCOPY
    ))) {
        SUwarning (1, "dictgrowstack", "vmresize failed");
        return -1;
    }
    afp->ad.stackn *= 2;
    return 0;
}

/* value conversion functions */

int _aggrscanval (AGGRfile_t *afp, char *sp, unsigned char **valpp) {
    AGGRunit_t vu;
    unsigned char *valp;
    int vali, typel, vall;
    char *s;

    vali = 0, vall = afp->hdr.vallen;
    typel = typelens[afp->hdr.valtype];
    switch (afp->hdr.valtype) {
    case AGGR_TYPE_CHAR:
        for (
            valp = *valpp, vali = 0; sp && vali < vall; sp = s, vali += typel
        ) {
            if ((s = strchr (sp, ':')))
                *s++ = 0;
            vu.u.c = atoi (sp);
            memcpy (valp + vali, &vu.u.c, typel);
        }
        break;
    case AGGR_TYPE_SHORT:
        for (
            valp = *valpp, vali = 0; sp && vali < vall; sp = s, vali += typel
        ) {
            if ((s = strchr (sp, ':')))
                *s++ = 0;
            vu.u.s = atoi (sp);
            memcpy (valp + vali, &vu.u.s, typel);
        }
        break;
    case AGGR_TYPE_INT:
        for (
            valp = *valpp, vali = 0; sp && vali < vall; sp = s, vali += typel
        ) {
            if ((s = strchr (sp, ':')))
                *s++ = 0;
            vu.u.i = atoi (sp);
            memcpy (valp + vali, &vu.u.i, typel);
        }
        break;
    case AGGR_TYPE_LLONG:
        for (
            valp = *valpp, vali = 0; sp && vali < vall; sp = s, vali += typel
        ) {
            if ((s = strchr (sp, ':')))
                *s++ = 0;
            vu.u.l = strtoll (sp, NULL, 10);
            memcpy (valp + vali, &vu.u.l, typel);
        }
        break;
    case AGGR_TYPE_FLOAT:
        for (
            valp = *valpp, vali = 0; sp && vali < vall; sp = s, vali += typel
        ) {
            if ((s = strchr (sp, ':')))
                *s++ = 0;
            vu.u.f = atof (sp);
            memcpy (valp + vali, &vu.u.f, typel);
        }
        break;
    case AGGR_TYPE_DOUBLE:
        for (
            valp = *valpp, vali = 0; sp && vali < vall; sp = s, vali += typel
        ) {
            if ((s = strchr (sp, ':')))
                *s++ = 0;
            vu.u.d = atof (sp);
            memcpy (valp + vali, &vu.u.d, typel);
        }
        break;
    }
    if (vali != vall) {
        SUwarning (1, "scanval", "not enough elements in val");
        return -1;
    } else if (sp) {
        SUwarning (1, "scanval", "too many elements in val");
        return -1;
    }
    return 0;
}

void _aggrprintval (AGGRfile_t *afp, Sfio_t *fp, unsigned char *valp) {
    AGGRunit_t vu;
    int vali, typel, vall;

    if (!valp) {
        sfprintf (fp, "\"\"");
        return;
    }
    vall = afp->hdr.vallen;
    typel = typelens[afp->hdr.valtype];
    switch (afp->hdr.valtype) {
    case TYPE_CHAR:
        for (vali = 0; vali < vall; vali += typel) {
            memcpy (&vu.u.c, valp + vali, typel);
            sfprintf (fp, "%s%d", (vali > 0) ? ":" : "", vu.u.c);
        }
        break;
    case TYPE_SHORT:
        for (vali = 0; vali < vall; vali += typel) {
            memcpy (&vu.u.s, valp + vali, typel);
            sfprintf (fp, "%s%d", (vali > 0) ? ":" : "", vu.u.s);
        }
        break;
    case TYPE_INT:
        for (vali = 0; vali < vall; vali += typel) {
            memcpy (&vu.u.i, valp + vali, typel);
            sfprintf (fp, "%s%d", (vali > 0) ? ":" : "", vu.u.i);
        }
        break;
    case TYPE_LLONG:
        for (vali = 0; vali < vall; vali += typel) {
            memcpy (&vu.u.l, valp + vali, typel);
            sfprintf (fp, "%s%d", (vali > 0) ? ":" : "", vu.u.l);
        }
        break;
    case TYPE_FLOAT:
        for (vali = 0; vali < vall; vali += typel) {
            memcpy (&vu.u.f, valp + vali, typel);
            sfprintf (fp, "%s%f", (vali > 0) ? ":" : "", vu.u.f);
        }
        break;
    case TYPE_DOUBLE:
        for (vali = 0; vali < vall; vali += typel) {
            memcpy (&vu.u.d, valp + vali, typel);
            sfprintf (fp, "%s%f", (vali > 0) ? ":" : "", vu.u.d);
        }
        break;
    }
}

/* header functions */

static int hdrread (AGGRfile_t *afp) {
    char magic[AGGR_MAGIC_LEN];
    int version;

    static int dictsn, datasn;

    if ((*afp->funcs.ensurelen) (afp, 0, FALSE) <= -1) {
        SUwarning (1, "hdrread", "ensurelen failed");
        return -1;
    }
    afp->map.off = 0;
    afp->map.len = HDR_LEN;
    if ((*afp->funcs.setmap) (afp, &afp->map) == -1) {
        SUwarning (1, "hdrread", "setmap failed");
        return -1;
    }
    if (afp->rwmode == AGGR_RWMODE_RD && afp->hdr.version != -1)
        dictsn = afp->hdr.dictsn, datasn = afp->hdr.datasn;
    else
        dictsn = datasn = -1;
    memcpy (magic, afp->map.p, AGGR_MAGIC_LEN);
    if (strncmp (magic, AGGR_MAGIC, AGGR_MAGIC_LEN) != 0) {
        SUwarning (1, "hdrread", "not an AGGR file");
        return -1;
    }
    memcpy (&version, afp->map.p + AGGR_MAGIC_LEN, sizeof (int));
#if SWIFT_LITTLEENDIAN == 1
    SW4 (&version);
#endif
    if (version != AGGR_VERSION) {
        if (version == 4) {
            if (afp->rwmode != AGGR_RWMODE_RD) {
                SUwarning (
                    1, "hdrread", "cannot open version 4 file for write"
                );
                return -1;
            }
            memset (&afp->hdr, 0, sizeof (AGGRhdr_t));
            memcpy (
                &afp->hdr, afp->map.p,
                AGGR_MAGIC_LEN +
                5 * sizeof (int) + 2 * sizeof (short)
            );
            memcpy (
                (char *) &afp->hdr + AGGR_MAGIC_LEN +
                5 * sizeof (int) + 2 * sizeof (short) + sizeof (int),
                afp->map.p + AGGR_MAGIC_LEN +
                5 * sizeof (int) + 2 * sizeof (short),
                sizeof (int)
            );
            memcpy (
                (char *) &afp->hdr + AGGR_MAGIC_LEN +
                5 * sizeof (int) + 2 * sizeof (short) +
                sizeof (long long) + sizeof (int),
                afp->map.p + AGGR_MAGIC_LEN +
                5 * sizeof (int) + 2 * sizeof (short) + sizeof (int),
                sizeof (int)
            );
            memcpy (
                (char *) &afp->hdr + AGGR_MAGIC_LEN +
                5 * sizeof (int) + 2 * sizeof (short) + 2 * sizeof (long long),
                afp->map.p + AGGR_MAGIC_LEN +
                5 * sizeof (int) + 2 * sizeof (short) + 2 * sizeof (int),
                HDR_LEN - (
                    AGGR_MAGIC_LEN +
                    5 * sizeof (int) + 2 * sizeof (short) +
                    2 * sizeof (long long)
                )
            );
        } else if (version == 3) {
            if (afp->rwmode != AGGR_RWMODE_RD) {
                SUwarning (
                    1, "hdrread", "cannot open version 3 file for write"
                );
                return -1;
            }
            memset (&afp->hdr, 0, sizeof (AGGRhdr_t));
            memcpy (
                &afp->hdr, afp->map.p,
                AGGR_MAGIC_LEN +
                5 * sizeof (int) + 2 * sizeof (short)
            );
            memcpy (
                (char *) &afp->hdr + AGGR_MAGIC_LEN +
                5 * sizeof (int) + 2 * sizeof (short) + sizeof (int),
                afp->map.p + AGGR_MAGIC_LEN +
                5 * sizeof (int) + 2 * sizeof (short),
                sizeof (int)
            );
            memcpy (
                (char *) &afp->hdr + AGGR_MAGIC_LEN +
                5 * sizeof (int) + 2 * sizeof (short) +
                sizeof (long long) + sizeof (int),
                afp->map.p + AGGR_MAGIC_LEN +
                5 * sizeof (int) + 2 * sizeof (short) + sizeof (int),
                sizeof (int)
            );
            memcpy (
                (char *) &afp->hdr + AGGR_MAGIC_LEN +
                5 * sizeof (int) + 2 * sizeof (short) + 2 * sizeof (long long),
                afp->map.p + AGGR_MAGIC_LEN +
                5 * sizeof (int) + 2 * sizeof (short) + 2 * sizeof (int),
                2 * sizeof (int) + 2 * sizeof (double)
            );
            afp->hdr.vallen = 0;
            memcpy (
                (char *) &afp->hdr + AGGR_MAGIC_LEN +
                8 * sizeof (int) + 2 * sizeof (short) +
                2 * sizeof (long long) + 2 * sizeof (double),
                afp->map.p + AGGR_MAGIC_LEN +
                9 * sizeof (int) + 2 * sizeof (short) + 2 * sizeof (double),
                HDR_LEN - (
                    AGGR_MAGIC_LEN +
                    8 * sizeof (int) + 2 * sizeof (short) +
                    2 * sizeof (long long) + 2 * sizeof (double)
                )
            );
        } else {
            SUwarning (0, "hdrread", "incompatible version");
            return -1;
        }
    } else
        memcpy (&afp->hdr, afp->map.p, HDR_LEN);
#if SWIFT_LITTLEENDIAN == 1
    SW4 (&afp->hdr.version);
    SW4 (&afp->hdr.dictsn), SW4 (&afp->hdr.datasn);
    SW4 (&afp->hdr.itemn), SW4 (&afp->hdr.framen);
    SW2 (&afp->hdr.keytype), SW2 (&afp->hdr.valtype);
    SW4 (&afp->hdr.vallen);
    SW8 (&afp->hdr.dictlen), SW8 (&afp->hdr.datalen);
    SW4 (&afp->hdr.dictincr), SW4 (&afp->hdr.itemincr);
    SW8 (&afp->hdr.minval), SW8 (&afp->hdr.maxval);
#endif
    if (afp->hdr.version == 3)
        afp->hdr.vallen = typelens[afp->hdr.valtype];
    if (afp->rwmode == AGGR_RWMODE_RD)
        if (dictsn != afp->hdr.dictsn || datasn != afp->hdr.datasn)
            afp->cbufl = 0;
    afp->map.len = 0;
    if ((*afp->funcs.unsetmap) (afp, &afp->map) == -1) {
        SUwarning (1, "hdrread", "unsetmap failed");
        return -1;
    }
    return 0;
}

static int hdrwrite (AGGRfile_t *afp) {
#if SWIFT_LITTLEENDIAN == 1
    AGGRhdr_t *swhdrp;
#endif

    if ((*afp->funcs.ensurelen) (afp, 0, TRUE) == -1) {
        SUwarning (1, "hdrwrite", "ensurelen failed");
        return -1;
    }
    afp->map.off = -1;
    afp->map.len = HDR_LEN;
    if ((*afp->funcs.setmap) (afp, &afp->map) == -1) {
        SUwarning (1, "hdrwrite", "setmap failed");
        return -1;
    }
    memcpy (afp->map.p, &afp->hdr, HDR_LEN);
#if SWIFT_LITTLEENDIAN == 1
    swhdrp = (AGGRhdr_t *) afp->map.p;
    SW4 (&swhdrp->version);
    SW4 (&swhdrp->dictsn), SW4 (&swhdrp->datasn);
    SW4 (&swhdrp->itemn), SW4 (&swhdrp->framen);
    SW2 (&swhdrp->keytype), SW2 (&swhdrp->valtype);
    SW4 (&swhdrp->vallen);
    SW8 (&swhdrp->dictlen), SW8 (&swhdrp->datalen);
    SW4 (&swhdrp->dictincr), SW4 (&swhdrp->itemincr);
    SW8 (&swhdrp->minval), SW8 (&swhdrp->maxval);
#endif
    afp->map.off = 0;
    if ((*afp->funcs.unsetmap) (afp, &afp->map) == -1) {
        SUwarning (1, "hdrwrite", "unsetmap failed");
        return -1;
    }
    return 0;
}

/* file operations */

static int localopen (AGGRfile_t *afp, int createflag) {
    int openmode;

    openmode = (afp->rwmode == AGGR_RWMODE_RD ? O_RDONLY : O_RDWR);
    if ((afp->fd = open (afp->namep, openmode)) > -1)
        return afp->fd;
    SUwarning (1, "localopen", "open failed");
    if (!createflag)
        return -1;
    if ((afp->fd = open (afp->namep, O_RDWR | O_CREAT | O_EXCL, 0666)) == -1) {
        SUwarning (1, "localopen", "create failed");
        return -1;
    }
    return afp->fd;
}

static int localclose (AGGRfile_t *afp, int destroyflag) {
    if (afp->fd != -1 && close (afp->fd) == -1)
        SUwarning (0, "localclose", "close failed");
    if (destroyflag && unlink (afp->namep) == -1)
        SUwarning (0, "localclose", "unlink failed");
    return 0;
}

static int clientopen (AGGRfile_t *afp, int createflag) {
    int cmd, rtn;
    char *namep;

    if (!(afp->serverp = AGGRserverget (afp->namep))) {
        SUwarning (1, "clientopen", "server get failed");
        return -1;
    }
    cmd = (createflag ? AGGR_CMDcreate : AGGR_CMDopen);
    namep = strchr (afp->namep, '/') + 2;
    namep = strchr (namep, '/');
    if (
        msgput (afp->serverp, &cmd, MSG_4, TRUE) == -1 ||
        msgput (afp->serverp, namep, MSG_S, FALSE) == -1 ||
        msgput (afp->serverp, &afp->rwmode, MSG_4, TRUE) == -1 ||
        msgwrite (afp->serverp) == -1 ||
        msgread (afp->serverp) == -1 ||
        msgget (afp->serverp, &rtn, MSG_4, TRUE) == -1 || rtn == -1
    ) {
        SUwarning (1, "clientopen", "create/open failed");
        return -1;
    }
    afp->fd = rtn;
    return afp->fd;
}

static int clientclose (AGGRfile_t *afp, int destroyflag) {
    int cmd, rtn;

    cmd = (destroyflag ? AGGR_CMDdestroy : AGGR_CMDclose);
    if (
        msgput (afp->serverp, &cmd, MSG_4, TRUE) == -1 ||
        msgput (afp->serverp, &afp->fd, MSG_4, TRUE) == -1 ||
        msgwrite (afp->serverp) == -1 ||
        msgread (afp->serverp) == -1 ||
        msgget (afp->serverp, &rtn, MSG_4, TRUE) == -1 || rtn == -1
    ) {
        SUwarning (1, "clientclose", "destroy/close failed");
        return -1;
    }
    return rtn;
}

static int localsetlock (AGGRfile_t *afp) {
    struct flock lock;

    if (afp->lockednow > 0) {
        SUwarning (1, "localsetlock", "file already locked");
        return -1;
    }
#ifndef LOCKBROKEN
    lock.l_type = (afp->rwmode == AGGR_RWMODE_RW ? F_WRLCK : F_RDLCK);
    lock.l_whence = 0;
    lock.l_start = 0;
    lock.l_len = 0;
    if (fcntl (afp->fd, F_SETLKW, &lock) == -1) {
        SUwarning (1, "localsetlock", "lock failed");
        return -1;
    }
#else
    if (flock (afp->fd, LOCK_EX) == -1) {
        SUwarning (1, "localsetlock", "lock failed");
        return -1;
    }
#endif
    afp->lockednow = TRUE;
    return 0;
}

static int localunsetlock (AGGRfile_t *afp) {
    struct flock lock;

#ifndef LOCKBROKEN
    lock.l_type = F_UNLCK;
    lock.l_whence = 0;
    lock.l_start = 0;
    lock.l_len = 0;
    if (fcntl (afp->fd, F_SETLKW, &lock) == -1) {
        SUwarning (1, "localunsetlock", "unlock failed");
        return -1;
    }
#else
    if (flock (afp->fd, LOCK_UN) == -1) {
        SUwarning (1, "localunsetlock", "unlock failed");
        return -1;
    }
#endif
    afp->lockednow = FALSE;
    return 0;
}

static int clientsetlock (AGGRfile_t *afp) {
    int cmd;

    if (afp->lockednow > 0) {
        SUwarning (1, "clientsetlock", "file already locked");
        return -1;
    }
    cmd = AGGR_CMDsetlock;
    if (
        msgput (afp->serverp, &cmd, MSG_4, TRUE) == -1 ||
        msgput (afp->serverp, &afp->fd, MSG_4, TRUE) == -1 ||
        msgwrite (afp->serverp) == -1
    ) {
        SUwarning (1, "clientsetlock", "setlock failed");
        return -1;
    }
    afp->lockednow = TRUE;
    return 0;
}

static int clientunsetlock (AGGRfile_t *afp) {
    int cmd;

    cmd = AGGR_CMDunsetlock;
    if (
        msgput (afp->serverp, &cmd, MSG_4, TRUE) == -1 ||
        msgput (afp->serverp, &afp->fd, MSG_4, TRUE) == -1 ||
        msgwrite (afp->serverp) == -1
    ) {
        SUwarning (1, "clientunsetlock", "unsetlock failed");
        return -1;
    }
    afp->lockednow = FALSE;
    return 0;
}

static int localensurelen (AGGRfile_t *afp, AGGRoff_t len, int growflag) {
    AGGRoff_t clen;

    static char c;

    len += HDR_LEN;
    if ((clen = lseek (afp->fd, 0, SEEK_END)) == -1) {
        SUwarning (1, "localensurelen", "lseek failed");
        return -1;
    }
    if (clen >= len)
        return 0;
    if (!growflag)
        return -2;
    if (
        lseek (afp->fd, len - 1, SEEK_SET) == -1 || write (afp->fd, &c, 1) == -1
    ) {
        SUwarning (1, "localensurelen", "lseek/write failed");
        return -1;
    }
    return 0;
}

static int clientensurelen (AGGRfile_t *afp, AGGRoff_t len, int growflag) {
    int cmd, rtn;

    cmd = AGGR_CMDensurelen;
    if (
        msgput (afp->serverp, &cmd, MSG_4, TRUE) == -1 ||
        msgput (afp->serverp, &afp->fd, MSG_4, TRUE) == -1 ||
        msgput (afp->serverp, &len, MSG_8, TRUE) == -1 ||
        msgput (afp->serverp, &growflag, MSG_4, TRUE) == -1 ||
        msgwrite (afp->serverp) == -1 ||
        msgread (afp->serverp) == -1 ||
        msgget (afp->serverp, &rtn, MSG_4, TRUE) == -1 || rtn == -1
    ) {
        SUwarning (1, "clientensurelen", "ensurelen failed");
        return -1;
    }
    return rtn;
}

static int localtruncate (AGGRfile_t *afp, AGGRoff_t len) {
    if (ftruncate (afp->fd, len) == -1) {
        SUwarning (1, "localtruncate", "truncate failed");
        return -1;
    }
    return 0;
}

static int clienttruncate (AGGRfile_t *afp, AGGRoff_t len) {
    int cmd;

    cmd = AGGR_CMDtruncate;
    if (
        msgput (afp->serverp, &cmd, MSG_4, TRUE) == -1 ||
        msgput (afp->serverp, &afp->fd, MSG_4, TRUE) == -1 ||
        msgput (afp->serverp, &len, MSG_8, TRUE) == -1 ||
        msgwrite (afp->serverp) == -1
    ) {
        SUwarning (1, "clienttruncate", "truncate failed");
        return -1;
    }
    return 0;
}

static int localsetmap (AGGRfile_t *afp, AGGRmap_t *mapp) {
    long long n, l, ret;

    if (afp->mapactive)
        SUwarning (0, "localsetmap", "nested localsetmap call");
    if (mapp->datan < mapp->len) {
        if (!(mapp->datap = vmresize (
            aggrvm, mapp->datap, mapp->len, VM_RSMOVE
        ))) {
            SUwarning (1, "localsetmap", "vmresize failed");
            return -1;
        }
        mapp->datan = mapp->len;
    }
    mapp->p = mapp->datap;
    if (mapp->off == -1) {
        afp->mapactive = TRUE;
        return 0;
    }
    if (lseek (afp->fd, mapp->off, SEEK_SET) != mapp->off) {
        SUwarning (1, "localsetmap", "lseek failed");
        return -1;
    }
    n = mapp->len, l = 0;
    while ((ret = read (afp->fd, mapp->p + l, n - l)) > 0)
        if ((l += ret) == n)
            break;
    if (ret < 0) {
        SUwarning (1, "localsetmap", "read failed");
        return -1;
    }
    afp->mapactive = TRUE;
    return 0;
}

static int localunsetmap (AGGRfile_t *afp, AGGRmap_t *mapp) {
    long long n, l, ret;

    if (!afp->mapactive)
        SUwarning (0, "localunsetmap", "illegal localunsetmap call");
    if (afp->rwmode != AGGR_RWMODE_RW || mapp->len <= 0) {
        afp->mapactive = FALSE;
        return 0;
    }
    if (lseek (afp->fd, mapp->off, SEEK_SET) != mapp->off) {
        SUwarning (1, "localunsetmap", "lseek failed");
        return -1;
    }
    n = mapp->len, l = 0;
    while ((ret = write (afp->fd, mapp->p + l, n - l)) > 0)
        if ((l += ret) == n)
            break;
    if (ret < 0) {
        SUwarning (1, "localunsetmap", "write failed");
        return -1;
    }
    afp->mapactive = FALSE;
    return 0;
}

static int clientsetmap (AGGRfile_t *afp, AGGRmap_t *mapp) {
    int cmd, rtn;

    cmd = AGGR_CMDsetmap;
    if (afp->mapactive)
        SUwarning (0, "clientsetmap", "nested clientsetmap call");
    if (mapp->datan < mapp->len) {
        if (!(mapp->datap = vmresize (
            aggrvm, mapp->datap, mapp->len, VM_RSMOVE
        ))) {
            SUwarning (1, "clientsetmap", "vmresize failed");
            return -1;
        }
        mapp->datan = mapp->len;
    }
    mapp->p = mapp->datap;
    if (mapp->off == -1) {
        afp->mapactive = TRUE;
        return 0;
    }
    if (afp->rwmode == AGGR_RWMODE_RD) {
        if (
            mapp->off >= afp->cbufo &&
            mapp->off - afp->cbufo + mapp->len < afp->cbufl
        ) {
            memcpy (mapp->p, afp->cbufs + mapp->off - afp->cbufo, mapp->len);
            afp->mapactive = TRUE;
            return 0;
        }
        if (afp->cbufn < mapp->len) {
            afp->cbufn = (mapp->len > 4000000 ? mapp->len : 4000000);
            if (!(afp->cbufs = vmresize (aggrvm, afp->cbufs, afp->cbufn, 0))) {
                SUwarning (1, "clientsetmap", "vmresize failed");
                return -1;
            }
            afp->cbufl = 0, afp->cbufo = -1;
        }
        if ((afp->cbufl = afp->cbufn) + mapp->off > FILELEN (afp))
            afp->cbufl = FILELEN (afp) - mapp->off;
        afp->cbufo = mapp->off;
        if (
            msgput (afp->serverp, &cmd, MSG_4, TRUE) == -1 ||
            msgput (afp->serverp, &afp->fd, MSG_4, TRUE) == -1 ||
            msgput (afp->serverp, &mapp->off, MSG_8, TRUE) == -1 ||
            msgput (afp->serverp, &afp->cbufl, MSG_8, TRUE) == -1 ||
            msgwrite (afp->serverp) == -1 ||
            msgread (afp->serverp) == -1 ||
            msgget (afp->serverp, &rtn, MSG_4, TRUE) == -1 || rtn == -1 ||
            msgget (afp->serverp, afp->cbufs, afp->cbufl, FALSE) == -1
        ) {
            SUwarning (1, "clientsetmap", "setmap failed");
            return -1;
        }
        memcpy (mapp->datap, afp->cbufs, mapp->len);
        afp->mapactive = TRUE;
        return 0;
    }
    if (
        msgput (afp->serverp, &cmd, MSG_4, TRUE) == -1 ||
        msgput (afp->serverp, &afp->fd, MSG_4, TRUE) == -1 ||
        msgput (afp->serverp, &mapp->off, MSG_8, TRUE) == -1 ||
        msgput (afp->serverp, &mapp->len, MSG_8, TRUE) == -1 ||
        msgwrite (afp->serverp) == -1 ||
        msgread (afp->serverp) == -1 ||
        msgget (afp->serverp, &rtn, MSG_4, TRUE) == -1 || rtn == -1 ||
        msgget (afp->serverp, mapp->p, mapp->len, FALSE) == -1
    ) {
        SUwarning (1, "clientsetmap", "setmap failed");
        return -1;
    }
    afp->mapactive = TRUE;
    return rtn;
}

static int clientunsetmap (AGGRfile_t *afp, AGGRmap_t *mapp) {
    int cmd, rtn;

    cmd = AGGR_CMDunsetmap;
    if (!afp->mapactive)
        SUwarning (0, "clientunsetmap", "illegal clientunsetmap call");
    if (afp->rwmode != AGGR_RWMODE_RW || mapp->len <= 0) {
        afp->mapactive = FALSE;
        return 0;
    }
    if (
        msgput (afp->serverp, &cmd, MSG_4, TRUE) == -1 ||
        msgput (afp->serverp, &afp->fd, MSG_4, TRUE) == -1 ||
        msgput (afp->serverp, &mapp->off, MSG_8, TRUE) == -1 ||
        msgput (afp->serverp, &mapp->len, MSG_8, TRUE) == -1 ||
        msgput (afp->serverp, mapp->p, mapp->len, FALSE) == -1 ||
        msgwrite (afp->serverp) == -1 ||
        msgread (afp->serverp) == -1 ||
        msgget (afp->serverp, &rtn, MSG_4, TRUE) == -1 || rtn == -1
    ) {
        SUwarning (1, "clientunsetmap", "unsetmap failed");
        return -1;
    }
    afp->mapactive = FALSE;
    return rtn;
}

/* server functions */

static int msgread (AGGRserver_t *asp) {
    unsigned char s[4];
    int l, n, ret;

    n = 4, l = 0;
    while ((ret = read (asp->socket, (char *) s + l, n - l)) > 0)
        if ((l += ret) == n)
            break;
    if (ret < 0) {
        SUwarning (1, "msgread", "read of size failed");
        return -1;
    }
    if (l == 0)
        return 0;
    if (l != 4)
        return -1;
    n = s[0] * 16777216 + s[1] * 65536 + s[2] * 256 + s[3];
    if (n > asp->msgn) {
        if (!(asp->msgs = vmresize (aggrvm, asp->msgs, n, VM_RSCOPY))) {
            SUwarning (1, "msgread", "vmresize failed");
            return -1;
        }
        asp->msgn = n;
    }
    asp->msgl = asp->msgi = l = 0;
    while ((ret = read (asp->socket, (char *) asp->msgs + l, n - l)) > 0)
        if ((l += ret) == n)
            break;
    if (ret < 0) {
        SUwarning (1, "msgread", "read of body failed");
        return -1;
    }
    asp->msgl = l;
    return (l == n ? n : -1);
}

static int msgwrite (AGGRserver_t *asp) {
    unsigned char s[4];
    int l, n, ret;

    n = asp->msgl;
    s[0] = n / 16777216, n %= 16777216;
    s[1] = n / 65536, n %= 65536;
    s[2] = n / 256, n %= 256;
    s[3] = n;
    n = 4, l = 0;
    while ((ret = write (asp->socket, (char *) s + l, n - l)) > 0)
        if ((l += ret) == n)
            break;
    if (ret < 0) {
        SUwarning (1, "msgwrite", "write of size failed");
        return -1;
    }
    n = asp->msgl;
    asp->msgl = asp->msgi = l = 0;
    while ((ret = write (asp->socket, (char *) asp->msgs + l, n - l)) > 0)
        if ((l += ret) == n)
            break;
    if (ret < 0) {
        SUwarning (1, "msgwrite", "write of body failed");
        return -1;
    }
    return n;
}

static int msgget (AGGRserver_t *asp, void *p, int n, int numflag) {
    unsigned char *cp;

    if (n > -1) {
        if (asp->msgi + n > asp->msgl) {
            SUwarning (1, "msgget", "out of data");
            return -1;
        }
        memcpy (p, asp->msgs + asp->msgi, n);
#if SWIFT_LITTLEENDIAN == 1
        if (numflag)
            switch (n) {
            case 2: SW2 (p); break;
            case 4: SW4 (p); break;
            case 8: SW8 (p); break;
            }
#endif
        asp->msgi += n;
    } else {
        cp = p;
        while (asp->msgi != asp->msgl)
            if ((*cp++ = asp->msgs[asp->msgi++]) == 0)
                break;
        n = cp - (unsigned char *) p;
    }
    return n;
}

static int msgput (AGGRserver_t *asp, void *p, int n, int numflag) {
    if (asp->msgi != 0)
        asp->msgi = 0, asp->msgl = 0;
    if (n == -1)
        n = strlen (p) + 1;
    if (asp->msgl + n > asp->msgn) {
        if (!(asp->msgs = vmresize (
            aggrvm, asp->msgs, asp->msgn + n, VM_RSCOPY
        ))) {
            SUwarning (1, "msgput", "vmresize failed");
            return -1;
        }
        asp->msgn += n;
    }
    memcpy (asp->msgs + asp->msgl, p, n);
#if SWIFT_LITTLEENDIAN == 1
    if (numflag)
        switch (n) {
        case 2: SW2 (asp->msgs + asp->msgl); break;
        case 4: SW4 (asp->msgs + asp->msgl); break;
        case 8: SW8 (asp->msgs + asp->msgl); break;
        }
#endif
    asp->msgl += n;
    return n;
}

static Void_t *aggrheapmem (
    Vmalloc_t *vm, Void_t *caddr, size_t csize, size_t nsize, Vmdisc_t *disc
) {
    if (csize == 0)
        return vmalloc (Vmheap, nsize);
    else if (nsize == 0)
        return (vmfree (Vmheap, caddr) >= 0) ? caddr : NULL;
    else
        return vmresize (Vmheap, caddr, nsize, 0);
}
