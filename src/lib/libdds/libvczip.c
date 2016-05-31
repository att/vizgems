#pragma prototyped

#include <ast.h>
#include <sfdisc.h>
#include <vcodex.h>
#include <vmalloc.h>
#include <tok.h>
#include <sys/types.h>
#include <swift.h>
#include <dds.h>
#include <dds_int.h>

#define MAGIC1 "HDR1"
#define MAGIC2 "HDR2"
#define MAGIC3 "HDR3"
#define MAGICSIZE 4
#define HDRSIZE 12

typedef struct vcm_t {
    char *np, *ap;
    Vcmethod_t *mp;
} vcm_t;

static ssize_t vcziread (Sfio_t *, void *, size_t, Sfdisc_t *);
static ssize_t vcziwrite (Sfio_t *, const void *, size_t, Sfdisc_t *);
static Sfoff_t vcziseek (Sfio_t *, Sfoff_t, int, Sfdisc_t *);
static int vcziexcept (Sfio_t *, int, void *, Sfdisc_t *);
static ssize_t vczoread (Sfio_t *, void *, size_t, Sfdisc_t *);
static ssize_t vczowrite (Sfio_t *, const void *, size_t, Sfdisc_t *);
static Sfoff_t vczoseek (Sfio_t *, Sfoff_t, int, Sfdisc_t *);
static int vczoexcept (Sfio_t *, int, void *, Sfdisc_t *);
static int rddir (vczi_t *, Sfdisc_t *);
static int wrdir (vczo_t *, Sfdisc_t *);
static int rdheader (Sfio_t *, Sfdisc_t *, ssize_t *, ssize_t *);
static int wrheader (Sfio_t *, Sfdisc_t *, ssize_t, ssize_t);

static int growiwin (vczi_t *, int);
static int growowin (vczo_t *, int);
static int growdata (vczi_t *, int);
static int parsevczstr (Vmalloc_t *, char *, vcm_t **);
static Vcmethod_t *maps2m (char *);

static char rdbbuf[10000], rtbbuf[10000];

Sfoff_t DDSvczsize (DDSheader_t *hdrp) {
    vczi_t *ip;

    if (!(ip = hdrp->vczp)) {
        SUwarning (1, "DDSvczsize", "cannot find compression info");
        return -1;
    }

    if (ip->rendoff == -1) {
        SUwarning (1, "DDSvczsize", "real end offset not set");
        return -1;
    }

    return ip->rendoff;
}

ssize_t DDSvczread (
    DDSheader_t *hdrp, char *memp, void *buf, Sfoff_t off, size_t len
) {
    vczi_t *ip;
    char *currp;
    size_t currlen, rest;
    vczwin_t *vczwinp;
    int vczwini;
    void *lastblockfix;

    ip = (vczi_t *) hdrp->vczp;

    if (off >= ip->endoff)
        return 0;

    for (ip->vczwini = 0; ip->vczwini < ip->vczwinl; ip->vczwini++) {
        vczwinp = &ip->vczwins[ip->vczwini];
        if (off >= vczwinp->uoff + vczwinp->usize)
            continue;
        if (!vczwinp->datap) {
            if (++ip->vczwinc % 100 == 0) {
                for (vczwini = 0; vczwini < ip->vczwinn; vczwini++) {
                    if (!ip->vczwins[vczwini].datap)
                        continue;
                    if (ip->vczwins[vczwini].uflag)
                        vmfree (ip->vm, ip->vczwins[vczwini].datap);
                    ip->vczwins[vczwini].datap = NULL;
                    ip->vczwins[vczwini].datan = 0;
                }
                if (vcbuffer (ip->vcp, NULL, -1, -1)) {
                    SUwarning (1, "DDSvczread", "cannot free vczip buffer");
                    return -1;
                }
            }
            if (vczwinp->uflag) {
                if (!(vczwinp->datap = vmalloc (ip->vm, vczwinp->csize))) {
                    SUwarning (1, "DDSvczread", "cannot allocate space");
                    return -1;
                }
                vczwinp->datan = vczwinp->csize;
                memcpy (
                    vczwinp->datap, memp + vczwinp->coff + HDRSIZE,
                    vczwinp->csize
                );
            } else {
                if ((vczwinp->datan = vcapply (
                    ip->vcp, memp + vczwinp->coff + HDRSIZE,
                    vczwinp->csize, &vczwinp->datap
                )) == -1) {
                    if (
                        ip->vczwini == ip->vczwinl - 1 && ip->vczwinl >= 2 &&
                        lastblockfix == 0
                    ) {
                        /*
                            there was a bug in versions up to 2011.01.10
                            where the last buffer of a file was not compressed
                            independently, so this fix decompresses the
                            previous buffer and tries again which should
                            set up the right state for the last buffer
                        */
                        if (vcapply (
                            ip->vcp,
                            memp + ip->vczwins[ip->vczwinl - 2].coff + HDRSIZE,
                            ip->vczwins[ip->vczwinl - 2].csize, &lastblockfix
                        ) != -1)
                            if ((vczwinp->datan = vcapply (
                                ip->vcp, memp + vczwinp->coff + HDRSIZE,
                                vczwinp->csize, &vczwinp->datap
                            )) != -1)
                                goto lastblockfix;
                    }
                    SUwarning (
                        1, "DDSvczread", "cannot uncompress %d bytes",
                        vczwinp->csize
                    );
                    return -1;
                }
            }
        }
lastblockfix:
        vczwinp->datal = off - vczwinp->uoff;
        break;
    }

    currp = buf, currlen = len;
    while (currlen > 0) {
        if (ip->vczwini == ip->vczwinm)
            break;
        if (
            ip->vczwini == -1 ||
            ip->vczwins[ip->vczwini].datal == ip->vczwins[ip->vczwini].datan
        ) {
            if (ip->vczwini + 1 == ip->vczwinm)
                break;
            ip->vczwini++;
            ip->vczwins[ip->vczwini].datal = 0;
            vczwinp = &ip->vczwins[ip->vczwini];

            if (!vczwinp->datap) {
                if (vczwinp->uflag) {
                    if (!(vczwinp->datap = vmalloc (ip->vm, vczwinp->csize))) {
                        SUwarning (1, "DDSvczread", "cannot allocate space");
                        return -1;
                    }
                    vczwinp->datan = vczwinp->csize;
                    memcpy (
                        vczwinp->datap, memp + vczwinp->coff + HDRSIZE,
                        vczwinp->csize
                    );
                } else {
                    if ((vczwinp->datan = vcapply (
                        ip->vcp, memp + vczwinp->coff + HDRSIZE,
                        vczwinp->csize, &vczwinp->datap
                    )) == -1) {
                        SUwarning (
                            1, "DDSvczread", "cannot uncompress %d bytes",
                            vczwinp->csize
                        );
                        return -1;
                    }
                }
            }
            vczwinp->datal = 0;
        }
        vczwinp = &ip->vczwins[ip->vczwini];
        rest = vczwinp->datan - vczwinp->datal;
        rest = (currlen <= rest) ? currlen : rest;
        memcpy (currp, vczwinp->datap + vczwinp->datal, rest);
        vczwinp->datal += rest;
        currp += rest, currlen -= rest;
    }

    return len - currlen;
}

char *_ddsvczgenrdb (DDSschema_t *schemap) {
    int fieldi;
    int coff, noff;
    char *s;

    strcpy (rdbbuf, "rdb.schema=[");
    s = rdbbuf + strlen (rdbbuf);
    coff = 0;
    for (fieldi = 0; fieldi < schemap->fieldn; fieldi++) {
        noff = (
            fieldi == schemap->fieldn - 1
        ) ? schemap->recsize : schemap->fields[fieldi + 1].off;
        sfsprintf (
            s, 10000 - (s - rdbbuf), "%s%d",
            (fieldi == 0) ? "" : ",", noff - coff
        );
        s += strlen (s);
        coff = noff;
    }
    strcat (s, "],table,mtf,rle.0,huffman");
    return &rdbbuf[0];
}

char *_ddsvczgenrtb (DDSschema_t *schemap) {
    int fieldi;
    int coff, noff;
    char *s;

    strcpy (rtbbuf, "rtable.schema=[");
    s = rtbbuf + strlen (rtbbuf);
    coff = 0;
    for (fieldi = 0; fieldi < schemap->fieldn; fieldi++) {
        noff = (
            fieldi == schemap->fieldn - 1
        ) ? schemap->recsize : schemap->fields[fieldi + 1].off;
        sfsprintf (
            s, 10000 - (s - rtbbuf), "%s%d",
            (fieldi == 0) ? "" : ",", noff - coff
        );
        s += strlen (s);
        coff = noff;
    }
    strcat (s, "],mtf,rle.0,huffman");
    return &rtbbuf[0];
}

vczi_t *_ddsvczopeni (Sfio_t *fp, char *vczipspec) {
    Vmalloc_t *vm;
    vczi_t *ip;
    vcm_t *vcms;
    int vcmn, vcmi;
    char *s1, *s2;

    vm = NULL;
    ip = NULL;
    if (
        !(vm = vmopen (Vmdcsbrk, Vmbest, 0)) ||
        !(ip = vmalloc (vm, sizeof (vczi_t)))
    ) {
        SUwarning (1, "ddsvczopeni", "cannot allocate disc space");
        goto abortddsvczopeni;
    }
    memset (ip, 0, sizeof (vczi_t));
    ip->vm = vm;
    ip->fp = fp;
    ip->vczwini = ip->vczwinm = -1;
    ip->baseoff = ip->ccoff = ip->cuoff = sfseek (ip->fp, 0, SEEK_CUR);
    ip->endoff = ip->rendoff = -1;

    if (!(vcmn = parsevczstr (ip->vm, vczipspec, &vcms))) {
        SUwarning (1, "ddsvczopeni", "cannot parse spec %s", vczipspec);
        goto abortddsvczopeni;
    }
    ip->vcp = NULL;
    for (vcmi = vcmn - 1; vcmi >= 0; vcmi--) {
        if (strcmp (vcms[vcmi].np, "rtable") == 0) {
            if (
                (s1 = strstr (vcms[vcmi].ap, "schema")) &&
                (s1 = strchr (s1, '[')) && (s2 = strchr (s1, ']'))
            ) {
                if (!(ip->rtschemap = vmstrdup (vm, s1))) {
                    SUwarning (
                        1, "ddsvczopeni", "cannot allocate rtschema %s:%s",
                        vcms[vcmi].np, vcms[vcmi].ap
                    );
                    goto abortddsvczopeni;
                }
                s2 = strchr (ip->rtschemap, ']');
                *(s2 + 1) = 0;
            }
        }
        if (!(ip->vcp = vcopen (
            NULL, vcms[vcmi].mp, vcms[vcmi].ap, ip->vcp,
            VC_DECODE | VC_CLOSECODER
        ))) {
            SUwarning (
                1, "ddsvczopeni", "cannot open method %s:%s",
                vcms[vcmi].np, vcms[vcmi].ap
            );
            goto abortddsvczopeni;
        }
    }

    ip->disc.readf = vcziread;
    ip->disc.writef = vcziwrite;
    ip->disc.seekf = vcziseek;
    ip->disc.exceptf = vcziexcept;
    if (sfdisc (ip->fp, (Sfdisc_t *) ip) != (Sfdisc_t *) ip) {
        SUwarning (1, "ddsvczopeni", "sfdisc failed");
        goto abortddsvczopeni;
    }
    if (ip->baseoff != -1 && sfseek (
        ip->fp, ip->baseoff, SEEK_SET
    ) != ip->baseoff) {
        SUwarning (1, "ddsvczopeni", "sfseek failed");
        goto abortddsvczopeni;
    }
    return ip;

abortddsvczopeni:
    if (vm)
        vmclose (vm);
    return NULL;
}

int _ddsvczclosei (Sfio_t *fp, vczi_t *ip) {
    sfdisc (fp, SF_POPDISC);
    if (ip->vcp)
        vcclose (ip->vcp);
    vmclose (ip->vm);
    return 0;
}

vczo_t *_ddsvczopeno (Sfio_t *fp, char *vczopspec, int recsize) {
    Vmalloc_t *vm;
    vczo_t *op;
    vcm_t *vcms;
    int vcmn, vcmi;
    int datan;
    char *s1, *s2;

    vm = NULL;
    op = NULL;
    if (
        !(vm = vmopen (Vmdcsbrk, Vmbest, 0)) ||
        !(op = vmalloc (vm, sizeof (vczo_t)))
    ) {
        SUwarning (1, "ddsvczopeno", "cannot allocate disc space");
        goto abortddsvczopeno;
    }
    memset (op, 0, sizeof (vczo_t));
    op->vm = vm;
    op->fp = fp;

    if (!(vcmn = parsevczstr (op->vm, vczopspec, &vcms))) {
        SUwarning (1, "ddsvczopeno", "cannot parse spec %s", vczopspec);
        goto abortddsvczopeno;
    }
    op->vcp = NULL;
    for (vcmi = vcmn - 1; vcmi >= 0; vcmi--) {
        if (strcmp (vcms[vcmi].np, "rtable") == 0) {
            if (
                (s1 = strstr (vcms[vcmi].ap, "schema")) &&
                (s1 = strchr (s1, '[')) && (s2 = strchr (s1, ']'))
            ) {
                if (!(op->rtschemap = vmstrdup (vm, s1))) {
                    SUwarning (
                        1, "ddsvczopeno", "cannot allocate rtschema %s:%s",
                        vcms[vcmi].np, vcms[vcmi].ap
                    );
                    goto abortddsvczopeno;
                }
                s2 = strchr (op->rtschemap, ']');
                *(s2 + 1) = 0;
            }
        }
        if (!(op->vcp = vcopen (
            NULL, vcms[vcmi].mp, vcms[vcmi].ap, op->vcp,
            VC_ENCODE | VC_CLOSECODER
        ))) {
            SUwarning (
                1, "ddsvczopeno", "cannot open method %s:%s",
                vcms[vcmi].np, vcms[vcmi].ap
            );
            goto abortddsvczopeno;
        }
    }

    datan = 10 * recsize * recsize;
    if (datan < 4 * 1024 * 1024)
        datan = recsize * (4 * 1024 * 1024 / recsize);
    else if (datan > 4 * 1024 * 1024)
        datan = recsize * (4 * 1024 * 1024 / recsize);

    if (!(op->vczwin.datap = vmalloc (vm, datan))) {
        SUwarning (1, "ddsvczopeno", "cannot allocate disc space");
        goto abortddsvczopeno;
    }
    op->vczwin.datan = datan;

    op->disc.readf = vczoread;
    op->disc.writef = vczowrite;
    op->disc.seekf = NULL; // vczoseek;
    op->disc.exceptf = vczoexcept;
    if (sfdisc (op->fp, (Sfdisc_t *) op) != (Sfdisc_t *) op) {
        SUwarning (1, "ddsvczopeno", "sfdisc failed");
        goto abortddsvczopeno;
    }
    return op;

abortddsvczopeno:
    if (vm)
        vmclose (vm);
    return NULL;
}

int _ddsvczcloseo (Sfio_t *fp, vczo_t *op) {
    sfdisc (fp, SF_POPDISC);
    if (op->vcp)
        vcclose (op->vcp);
    vmclose (op->vm);
    return 0;
}

static ssize_t vcziread (
    Sfio_t *fp, void *buf, size_t len, Sfdisc_t *discp
) {
    vczi_t *ip;
    ssize_t csize, usize;
    char *currp;
    size_t currlen, rest;
    int ret;
    vczwin_t *vczwinp;
    int vczwini;
    Sfoff_t coff, uoff;

    ip = (vczi_t *) discp;
    if (ip->recflag)
        return sfrd (fp, buf, len, discp);

    ip->recflag = TRUE;
    currp = buf, currlen = len;
    while (currlen > 0) {
        if (ip->cuoff < ip->baseoff) {
            rest = ip->baseoff - ip->cuoff;
            rest = (currlen <= rest) ? currlen : rest;
            currp += rest, currlen -= rest;
            ip->cuoff += rest;
            continue;
        }
        if (ip->vczwinm >= 0 && ip->vczwini == ip->vczwinm)
            break;
        if (
            ip->vczwini == -1 ||
            ip->vczwins[ip->vczwini].datal == ip->vczwins[ip->vczwini].datan
        ) {
            if (ip->vczwinm >= 0 && ip->vczwini + 1 == ip->vczwinm)
                break;
            if (ip->vczwini + 1 == ip->vczwinn) {
                if (growiwin (ip, 2 * (ip->vczwinn + 1)) == -1) {
                    SUwarning (1, "vcziread", "cannot grow vczwin");
                    return -1;
                }
            }
            if (ip->vczwini + 1 < ip->vczwinl) {
                ip->vczwini++;
                ip->vczwins[ip->vczwini].datal = 0;
            } else {
                if (ip->vczwini == -1)
                    coff = ip->baseoff, uoff = ip->baseoff;
                else {
                    coff = (
                        ip->vczwins[ip->vczwini].coff + HDRSIZE +
                        ip->vczwins[ip->vczwini].csize
                    );
                    uoff = (
                        ip->vczwins[ip->vczwini].uoff +
                        ip->vczwins[ip->vczwini].usize
                    );
                }
                if (ip->baseoff != -1 && ip->ccoff != coff && sfsk (
                    fp, coff, SEEK_SET, discp
                ) != coff) {
                    SUwarning (
                        1, "vcziread", "cannot seek to header %d", ip->vczwini
                    );
                    return -1;
                }
                if ((ret = rdheader (fp, discp, &csize, &usize)) == -1) {
                    SUwarning (1, "vcziread", "cannot read header");
                    return -1;
                }
                if (ret == 1) {
                    ip->vczwinm = ip->vczwinl;
                    ip->endoff = ip->cuoff;
                    break;
                }
                ip->vczwini++, ip->vczwinl++;
                ip->vczwins[ip->vczwini].datal = 0;
                ip->vczwins[ip->vczwini].coff = coff;
                ip->vczwins[ip->vczwini].uoff = uoff;
                if (csize == 0) {
                    ip->vczwins[ip->vczwini].csize = usize;
                    ip->vczwins[ip->vczwini].uflag = TRUE;
                } else
                    ip->vczwins[ip->vczwini].csize = csize;
                ip->vczwins[ip->vczwini].usize = usize;
                if (ip->vczwins[ip->vczwini].csize > ip->datan && growdata (
                    ip, ip->vczwins[ip->vczwini].csize
                ) == -1) {
                    SUwarning (1, "vcziread", "cannot grow data");
                    return -1;
                }
                if (ip->baseoff != -1)
                    ip->ccoff = vczwinp->coff + HDRSIZE;
            }
            vczwinp = &ip->vczwins[ip->vczwini];

            if (!vczwinp->datap) {
                for (vczwini = 0; vczwini < ip->vczwinn; vczwini++) {
                    if (!ip->vczwins[vczwini].datap)
                        continue;
                    if (ip->vczwins[vczwini].uflag)
                        vmfree (ip->vm, ip->vczwins[vczwini].datap);
                    ip->vczwins[vczwini].datap = NULL;
                    ip->vczwins[vczwini].datan = 0;
                }
#ifdef NOTUSED
                if (ip->rtschemap && (vcsetmtarg (
                    ip->vcp, "schema", "[1]", 0
                ) == -1 || vcsetmtarg (
                    ip->vcp, "schema", ip->rtschemap, 0
                ) == -1)) {
                    SUwarning (
                        1, "vcziread", "cannot reset schema to %s",
                        ip->rtschemap
                    );
                    return -1;
                }
#endif
                if (vcbuffer (ip->vcp, NULL, -1, -1)) {
                    SUwarning (1, "vcziread", "cannot free vczip buffer");
                    return -1;
                }
                if (
                    ip->baseoff != -1 && ip->ccoff != vczwinp->coff + HDRSIZE &&
                    sfsk (
                        fp, vczwinp->coff + HDRSIZE, SEEK_SET, discp
                    ) != vczwinp->coff + HDRSIZE
                ) {
                    SUwarning (
                        1, "vcziread", "cannot seek past header %d", ip->vczwini
                    );
                    return -1;
                }
                if (vczwinp->uflag) {
                    if (!(vczwinp->datap = vmalloc (ip->vm, vczwinp->csize))) {
                        SUwarning (1, "vcziread", "cannot allocate space");
                        return -1;
                    }
                    vczwinp->datan = vczwinp->csize;
                    if (sfrd (
                        fp, vczwinp->datap, vczwinp->csize, discp
                    ) != vczwinp->csize) {
                        SUwarning (1, "vcziread", "cannot read window data");
                        return -1;
                    }
                } else {
                    if (sfrd (
                        fp, ip->datap, vczwinp->csize, discp
                    ) != vczwinp->csize) {
                        SUwarning (1, "vcziread", "cannot read window data");
                        return -1;
                    }
                    if ((vczwinp->datan = vcapply (
                        ip->vcp, ip->datap, vczwinp->csize, &vczwinp->datap
                    )) == -1) {
                        SUwarning (
                            1, "vcziread", "cannot uncompress %d bytes",
                            vczwinp->csize
                        );
                        return -1;
                    }
                }
                if (ip->baseoff != -1)
                    ip->ccoff = vczwinp->coff + HDRSIZE + vczwinp->csize;
            }
            vczwinp->datal = 0;
            if (ip->baseoff != -1)
                ip->cuoff = vczwinp->uoff;
        }
        vczwinp = &ip->vczwins[ip->vczwini];
        rest = vczwinp->datan - vczwinp->datal;
        rest = (currlen <= rest) ? currlen : rest;
        memcpy (currp, vczwinp->datap + vczwinp->datal, rest);
        vczwinp->datal += rest;
        currp += rest, currlen -= rest;
        if (ip->baseoff != -1)
            ip->cuoff += rest;
    }

    ip->recflag = FALSE;
    return len - currlen;
}

static ssize_t vcziwrite (
    Sfio_t *fp, const void *buf, size_t len, Sfdisc_t *discp
) {
    return -1;
}

static Sfoff_t vcziseek (
    Sfio_t *fp, Sfoff_t off, int type, Sfdisc_t *discp
) {
    vczi_t *ip;
    vczwin_t *vczwinp;
    Sfoff_t toff;

    ip = (vczi_t *) discp;
    if (ip->recflag)
        return sfsk (fp, off, type, discp);

    if (ip->baseoff == -1)
        return -1;

    ip->recflag = TRUE;
    switch (type) {
    case SEEK_SET:
        toff = off;
        break;
    case SEEK_CUR:
        ip->cuoff += off;
        toff = ip->cuoff;
        break;
    case SEEK_END:
        if (ip->endoff == -1 && rddir (ip, discp) == -1) {
            SUwarning (1, "vcziseek", "cannot read dir");
            return -1;
        }
        toff = ip->endoff + off;
        break;
    }

    if (toff <= ip->baseoff) {
        if (ip->ccoff != toff && sfsk (fp, toff, SEEK_SET, discp) != toff) {
            SUwarning (1, "vcziread", "cannot seek inside main header");
            return -1;
        }
        ip->vczwini = -1;
        ip->cuoff = ip->ccoff = toff;
        ip->recflag = FALSE;
        return toff;
    }

    if (ip->endoff == -1 && rddir (ip, discp) == -1) {
        SUwarning (1, "vcziseek", "cannot read dir");
        return -1;
    }

    for (ip->vczwini = 0; ip->vczwini < ip->vczwinl; ip->vczwini++) {
        vczwinp = &ip->vczwins[ip->vczwini];
        if (toff >= vczwinp->uoff + vczwinp->usize)
            continue;
        if (!vczwinp->datap) {
            if (ip->ccoff != vczwinp->coff + HDRSIZE && sfsk (
                fp, vczwinp->coff + HDRSIZE, SEEK_SET, discp
            ) != vczwinp->coff + HDRSIZE) {
                SUwarning (
                    1, "vcziread", "cannot seek past header %d", ip->vczwini
                );
                return -1;
            }
            if (vczwinp->uflag) {
                if (!(vczwinp->datap = vmalloc (ip->vm, vczwinp->csize))) {
                    SUwarning (1, "vcziseek", "cannot allocate space");
                    return -1;
                }
                vczwinp->datan = vczwinp->csize;
                if (sfrd (
                    fp, vczwinp->datap, vczwinp->csize, discp
                ) != vczwinp->csize) {
                    SUwarning (1, "vcziseek", "cannot read window data");
                    return -1;
                }
            } else {
                if (sfrd (
                    fp, ip->datap, vczwinp->csize, discp
                ) != vczwinp->csize) {
                    SUwarning (1, "vcziseek", "cannot read window data");
                    return -1;
                }
                if ((vczwinp->datan = vcapply (
                    ip->vcp, ip->datap, vczwinp->csize, &vczwinp->datap
                )) == -1) {
                    SUwarning (
                        1, "vcziseek", "cannot uncompress %d bytes",
                        vczwinp->csize
                    );
                    return -1;
                }
            }
            ip->ccoff = vczwinp->coff + HDRSIZE + vczwinp->csize;
        }
        vczwinp->datal = toff - vczwinp->uoff;
        break;
    }
    ip->cuoff = toff;

    ip->recflag = FALSE;
    return toff;
}

static int vcziexcept (
    Sfio_t *fp, int type, void *datap, Sfdisc_t *discp
) {
    if (type == SF_FINAL || type == SF_ATEXIT)
        return _ddsvczclosei (fp, (vczi_t *) discp);
    return 0;
}

static ssize_t vczoread (
    Sfio_t *fp, void *buf, size_t len, Sfdisc_t *discp
) {
    return -1;
}

static ssize_t vczowrite (
    Sfio_t *fp, const void *buf, size_t len, Sfdisc_t *discp
) {
    vczo_t *op;
    Vcchar_t *datap;
    ssize_t datan;
    const char *currp;
    size_t currlen, rest;

    op = (vczo_t *) discp;
    if (op->recflag)
        return sfwr (fp, buf, len, discp);

    op->recflag = TRUE;
    currp = buf, currlen = len;
    while (currlen > 0) {
        if (op->vczwin.datal == op->vczwin.datan) {
            if (op->rtschemap && (vcsetmtarg (
                op->vcp, "schema", "[1]", 0
            ) == -1 || vcsetmtarg (
                op->vcp, "schema", op->rtschemap, 0
            ) == -1)) {
                SUwarning (
                    1, "vczowrite", "cannot reset schema to %s", op->rtschemap
                );
                return -1;
            }

            if ((datan = vcapply (
                op->vcp, op->vczwin.datap, op->vczwin.datal, &datap
            )) == -1) {
                SUwarning (
                    1, "vczowrite", "cannot compress %d bytes", op->vczwin.datal
                );
                return -1;
            }
            if ((rest = vcundone (op->vcp)) > 0) {
                memmove (
                    op->vczwin.datap,
                    op->vczwin.datap + (op->vczwin.datal - rest), rest
                );
            }

            if (datan > 0) {
                if (op->vczwinl + 1 >= op->vczwinn) {
                    if (growowin (op, 2 * (op->vczwinl + 1)) == -1) {
                        SUwarning (1, "vczowrite", "cannot grow vczwin");
                        return -1;
                    }
                }
                op->vczwins[op->vczwinl].csize = datan;
                op->vczwins[op->vczwinl++].usize = op->vczwin.datal - rest;
                if (wrheader (
                    fp, discp, datan, op->vczwin.datal - rest
                ) == -1) {
                    SUwarning (1, "vczowrite", "cannot write header");
                    return -1;
                }
                if (sfwr (fp, datap, datan, discp) != datan) {
                    SUwarning (
                        1, "vczowrite", "cannot write %d compressed bytes",
                        datan
                    );
                    return -1;
                }
                if (vcbuffer (op->vcp, NULL, -1, -1)) {
                    SUwarning (1, "vczowrite", "cannot free vczip buffer");
                    return -1;
                }
            }
            op->vczwin.datal = rest;

            if (op->vczwin.datal == op->vczwin.datan) {
                if (op->vczwinl + 1 >= op->vczwinn) {
                    if (growowin (op, 2 * (op->vczwinl + 1)) == -1) {
                        SUwarning (1, "vczowrite", "cannot grow vczwin");
                        return -1;
                    }
                }
                op->vczwins[op->vczwinl].csize = 0;
                op->vczwins[op->vczwinl++].usize = op->vczwin.datal;
                if (wrheader (fp, discp, 0, op->vczwin.datal) == -1) {
                    SUwarning (1, "vczowrite", "cannot write header");
                    return -1;
                }
                if (sfwr (
                    fp, op->vczwin.datap, op->vczwin.datal, discp
                ) != op->vczwin.datal) {
                    SUwarning (
                        1, "vczowrite", "cannot write %d uncompressed bytes",
                        op->vczwin.datal
                    );
                    return -1;
                }
                op->vczwin.datal = 0;
            }
        }
        rest = op->vczwin.datan - op->vczwin.datal;
        rest = (currlen <= rest) ? currlen : rest;
        memcpy (op->vczwin.datap + op->vczwin.datal, currp, rest);
        op->vczwin.datal += rest;
        currp += rest, currlen -= rest;
    }
    op->recflag = FALSE;
    return len;
}

static Sfoff_t vczoseek (
    Sfio_t *fp, Sfoff_t off, int type, Sfdisc_t *discp
) {
    return -1;
}

static int vczoexcept (
    Sfio_t *fp, int type, void *vp, Sfdisc_t *discp
) {
    vczo_t *op;
    Vcchar_t *datap;
    ssize_t datan;
    size_t rest;

    op = (vczo_t *) discp;
    if (op->finished)
        return 0;
    if (
        type == SF_FINAL || type == SF_DPOP ||
        type == SF_ATEXIT || type == SF_CLOSING
    ) {
        op->finished = TRUE;
        op->recflag = TRUE;
        if (op->vczwin.datal > 0) {
            if (op->rtschemap && (vcsetmtarg (
                op->vcp, "schema", "[1]", 0
            ) == -1 || vcsetmtarg (
                op->vcp, "schema", op->rtschemap, 0
            ) == -1)) {
                SUwarning (
                    1, "vczoexcept", "cannot reset schema to %s", op->rtschemap
                );
                return -1;
            }

            if ((datan = vcapply (
                op->vcp, op->vczwin.datap, op->vczwin.datal, &datap
            )) == -1) {
                SUwarning (
                    1, "vczoexcept", "cannot compress %d bytes",
                    op->vczwin.datal
                );
                return -1;
            }
            if ((rest = vcundone (op->vcp)) > 0) {
                memmove (
                    op->vczwin.datap,
                    op->vczwin.datap + (op->vczwin.datal - rest), rest
                );
            }

            if (datan > 0) {
                if (op->vczwinl + 1 >= op->vczwinn) {
                    if (growowin (op, 2 * (op->vczwinl + 1)) == -1) {
                        SUwarning (1, "vczoexcept", "cannot grow vczwin");
                        return -1;
                    }
                }
                op->vczwins[op->vczwinl].csize = datan;
                op->vczwins[op->vczwinl++].usize = op->vczwin.datal - rest;
                if (wrheader (
                    fp, discp, datan, op->vczwin.datal - rest
                ) == -1) {
                    SUwarning (1, "vczoexcept", "cannot write header");
                    return -1;
                }
                if (sfwr (fp, datap, datan, discp) != datan) {
                    SUwarning (
                        1, "vczoexcept", "cannot write %d compressed bytes",
                        datan
                    );
                    return -1;
                }
                if (vcbuffer (op->vcp, NULL, -1, -1)) {
                    SUwarning (1, "vczoexcept", "cannot free vczip buffer");
                    return -1;
                }
            }
            op->vczwin.datal = rest;

            if (op->vczwin.datal > 0) {
                if (op->vczwinl + 1 >= op->vczwinn) {
                    if (growowin (op, 2 * (op->vczwinl + 1)) == -1) {
                        SUwarning (1, "vczoexcept", "cannot grow vczwin");
                        return -1;
                    }
                }
                op->vczwins[op->vczwinl].csize = 0;
                op->vczwins[op->vczwinl++].usize = op->vczwin.datal;
                if (wrheader (fp, discp, 0, op->vczwin.datal) == -1) {
                    SUwarning (1, "vczoexcept", "cannot write header");
                    return -1;
                }
                if (sfwr (
                    fp, op->vczwin.datap, op->vczwin.datal, discp
                ) != op->vczwin.datal) {
                    SUwarning (
                        1, "vczoexcept", "cannot write %d uncompressed bytes",
                        op->vczwin.datal
                    );
                    return -1;
                }
                op->vczwin.datal = 0;
            }
        }
        if (wrdir (op, discp) == -1) {
            SUwarning (1, "vczoexcept", "cannot write dir");
            return -1;
        }
        op->recflag = FALSE;
        if (type == SF_FINAL || type == SF_ATEXIT)
            return _ddsvczcloseo (fp, (vczo_t *) discp);
    }
    return 0;
}

static int rddir (vczi_t *ip, Sfdisc_t *discp) {
    unsigned char hdr[MAGICSIZE], buf[8];
    Sfoff_t endoff, off, coff, uoff;
    ssize_t csize, usize;
    int vczwini, vczwinl;

    if ((endoff = sfsk (ip->fp, 0, SEEK_END, discp)) == -1) {
        SUwarning (1, "rddir", "cannot seek to EOF");
        return -1;
    }
    ip->rendoff = endoff;
    off = endoff - (MAGICSIZE + 4);
    if (sfsk (ip->fp, off, SEEK_SET, discp) != off) {
        SUwarning (1, "rddir", "cannot seek to header 3");
        return -1;
    }
    if (sfrd (ip->fp, hdr, MAGICSIZE, discp) != MAGICSIZE) {
        SUwarning (1, "rddir", "cannot read header 3");
        return -1;
    }
    if (memcmp (hdr, MAGIC3, MAGICSIZE) != 0) {
        SUwarning (1, "rddir", "bad header 3");
        return -1;
    }
    if (sfrd (ip->fp, buf, 4, discp) != 4) {
        SUwarning (1, "rddir", "cannot read dir length");
        return -1;
    }
    vczwinl = (
        buf[0] * 256 * 256 * 256 + buf[1] * 256 * 256 +
        buf[2] * 256 + buf[3]
    );
    if (vczwinl >= ip->vczwinn) {
        if (growiwin (ip, vczwinl) == -1) {
            SUwarning (1, "rddir", "cannot grow vczwin");
            return -1;
        }
    }
    off = endoff - (vczwinl * 2 + 3) * 4;
    if (sfsk (ip->fp, off, SEEK_SET, discp) != off) {
        SUwarning (1, "rddir", "cannot seek to header 2");
        return -1;
    }
    if (sfrd (ip->fp, hdr, MAGICSIZE, discp) != MAGICSIZE) {
        SUwarning (1, "rddir", "cannot read header 2");
        return -1;
    }
    if (memcmp (hdr, MAGIC2, MAGICSIZE) != 0) {
        SUwarning (1, "rddir", "bad header 2");
        return -1;
    }
    coff = ip->baseoff, uoff = ip->baseoff;
    for (vczwini = 0; vczwini < vczwinl; vczwini++) {
        if (sfrd (ip->fp, buf, 8, discp) != 8) {
            SUwarning (1, "rddir", "cannot read dir data %d", vczwini);
            return -1;
        }
        ip->vczwins[vczwini].coff = coff;
        ip->vczwins[vczwini].uoff = uoff;
        csize = (
            buf[0] * 256 * 256 * 256 + buf[1] * 256 * 256 +
            buf[2] * 256 + buf[3]
        );
        usize = (
            buf[4] * 256 * 256 * 256 + buf[5] * 256 * 256 +
            buf[6] * 256 + buf[7]
        );
        if (csize == 0) {
            ip->vczwins[vczwini].uflag = TRUE;
            csize = usize;
        }
        ip->vczwins[vczwini].csize = csize;
        ip->vczwins[vczwini].usize = usize;
        ip->vczwins[vczwini].datal = 0;
        coff += HDRSIZE + csize;
        uoff += usize;
        if (csize > ip->datan && growdata (ip, csize) == -1) {
            SUwarning (1, "rddir", "cannot grow data");
            return -1;
        }
    }
    ip->ccoff = ip->cuoff = ip->baseoff;
    ip->endoff = uoff;
    ip->vczwini = -1, ip->vczwinl = ip->vczwinm = vczwinl;
    if (sfsk (ip->fp, ip->ccoff, SEEK_SET, discp) != ip->ccoff) {
        SUwarning (1, "rddir", "cannot seek to baseoff");
        return -1;
    }
    return 0;
}

static int wrdir (vczo_t *op, Sfdisc_t *discp) {
    unsigned char buf[8];
    int vczwini, n;

    if (sfwr (op->fp, MAGIC2, MAGICSIZE, discp) != MAGICSIZE) {
        SUwarning (1, "wrdir", "cannot write header 2");
        return -1;
    }
    for (vczwini = 0; vczwini < op->vczwinl; vczwini++) {
        n = op->vczwins[vczwini].csize;
        buf[0] = n / (256 * 256 * 256);
        buf[1] = (n / (256 * 256)) % 256;
        buf[2] = (n / 256) % 256;
        buf[3] = n % 256;
        n = op->vczwins[vczwini].usize;
        buf[4] = n / (256 * 256 * 256);
        buf[5] = (n / (256 * 256)) % 256;
        buf[6] = (n / 256) % 256;
        buf[7] = n % 256;
        if (sfwr (op->fp, buf, 8, discp) != 8) {
            SUwarning (1, "wrdir", "cannot write dir data %d", vczwini);
            return -1;
        }
    }
    if (sfwr (op->fp, MAGIC3, MAGICSIZE, discp) != MAGICSIZE) {
        SUwarning (1, "wrdir", "cannot write header 3");
        return -1;
    }
    n = op->vczwinl;
    buf[0] = n / (256 * 256 * 256);
    buf[1] = (n / (256 * 256)) % 256;
    buf[2] = (n / 256) % 256;
    buf[3] = n % 256;
    if (sfwr (op->fp, buf, 4, discp) != 4) {
        SUwarning (1, "wrdir", "cannot write dir length");
        return -1;
    }

    return 0;
}

static int rdheader (
    Sfio_t *fp, Sfdisc_t *discp, ssize_t *csizep, ssize_t *usizep
) {
    unsigned char hdr[MAGICSIZE], buf[8];
    int ret;

    if ((ret = sfrd (fp, hdr, MAGICSIZE, discp)) != MAGICSIZE) {
        if (ret == 0)
            return 1;
        SUwarning (1, "rdheader", "cannot read header");
        return -1;
    }
    if (memcmp (hdr, MAGIC1, MAGICSIZE) != 0) {
        if (memcmp (hdr, MAGIC2, MAGICSIZE) == 0)
            return 1;
        SUwarning (1, "rdheader", "bad header");
        return -1;
    }
    if ((ret = sfrd (fp, buf, 8, discp)) != 8) {
        SUwarning (1, "rdheader", "cannot read data");
        return -1;
    }

    *csizep = (
        buf[0] * 256 * 256 * 256 + buf[1] * 256 * 256 +
        buf[2] * 256 + buf[3]
    );
    *usizep = (
        buf[4] * 256 * 256 * 256 + buf[5] * 256 * 256 +
        buf[6] * 256 + buf[7]
    );
    return 0;
}

static int wrheader (
    Sfio_t *fp, Sfdisc_t *discp, ssize_t csize, ssize_t usize
) {
    unsigned char buf[8];

    buf[0] = csize / (256 * 256 * 256);
    buf[1] = (csize / (256 * 256)) % 256;
    buf[2] = (csize / 256) % 256;
    buf[3] = csize % 256;
    buf[4] = usize / (256 * 256 * 256);
    buf[5] = (usize / (256 * 256)) % 256;
    buf[6] = (usize / 256) % 256;
    buf[7] = usize % 256;
    if (sfwr (fp, MAGIC1, MAGICSIZE, discp) != MAGICSIZE) {
        SUwarning (1, "wrheader", "cannot write header");
        return -1;
    }
    if (sfwr (fp, buf, 8, discp) != 8) {
        SUwarning (1, "wrheader", "cannot write data");
        return -1;
    }
    return 0;
}

static int growiwin (vczi_t *ip, int newn) {
    if (newn <= ip->vczwinn)
        return 0;

    if (!(ip->vczwins = vmresize (
        ip->vm, ip->vczwins, newn * sizeof (vczwin_t), VM_RSCOPY
    ))) {
        SUwarning (1, "growiwin", "cannot grow vczwin");
        return -1;
    }
    memset (
        ip->vczwins + ip->vczwinn, 0, (newn - ip->vczwinn) * sizeof (vczwin_t)
    );
    ip->vczwinn = newn;
    return 0;
}

static int growowin (vczo_t *op, int newn) {
    if (newn <= op->vczwinn)
        return 0;

    if (!(op->vczwins = vmresize (
        op->vm, op->vczwins, newn * sizeof (vczwin_t), VM_RSCOPY
    ))) {
        SUwarning (1, "growowin", "cannot grow vczwin");
        return -1;
    }
    memset (
        op->vczwins + op->vczwinn, 0, (newn - op->vczwinn) * sizeof (vczwin_t)
    );
    op->vczwinn = newn;
    return 0;
}

static int growdata (vczi_t *ip, int newn) {
    if (newn <= ip->datan)
        return 0;

    if (!(ip->datap = vmresize (ip->vm, ip->datap, newn, VM_RSCOPY))) {
        SUwarning (1, "growdata", "cannot grow data");
        return -1;
    }
    ip->datan = newn;
    return 0;
}

static int parsevczstr (Vmalloc_t *vm, char *str, vcm_t **vcmsp) {
    vcm_t *vcms;
    int vcmn;
    char *s1, *s2, *s3;

    vcms = NULL, vcmn = 0;
    for (s1 = str; s1; ) {
        if ((s2 = strchr (s1, ','))) {
            if ((s3 = strchr (s1, ']')) && s3 > s2)
                s2 = strchr (s3, ',');
            if (s2)
                *s2 = 0;
        }
        if ((s3 = strchr (s1, '.')))
            *s3++ = 0;
        if (!(vcms = vmresize (
            vm, vcms, (vcmn + 1) * sizeof (vcm_t), VM_RSCOPY
        ))) {
            SUwarning (1, "parsevczstr", "cannot grow vcms");
            return -1;
        }
        vcms[vcmn].np = vcms[vcmn].ap = NULL, vcms[vcmn].mp = NULL;
        if (
            !(vcms[vcmn].mp = maps2m (s1)) ||
            !(vcms[vcmn].np = vmstrdup (vm, s1)) ||
            (s3 && !(vcms[vcmn].ap = vmstrdup (vm, s3)))
        ) {
            if (s3)
                *--s3 = '.';
            if (s2)
                *s2 = ',';
            SUwarning (1, "parsevczstr", "cannot add method %s", s1);
            return -1;
        }
        vcmn++;
        if (s3)
            *--s3 = '.';
        if (s2)
            *s2++ = ',';
        s1 = s2;
    }
    *vcmsp = vcms;
    return vcmn;
}

static Vcmethod_t *maps2m (char *str) {
#if _AST_VERSION > 20060000L
    if (strcmp (str, "rdb") == 0)
        return vcgetmeth ("rdb", 0);
    if (strcmp (str, "rtable") == 0)
        return vcgetmeth ("rtable", 0);
    if (strcmp (str, "table") == 0)
        return vcgetmeth ("table", 0);
#else
    if (strcmp (str, "rdb") == 0)
        return Vcrdb;
    if (strcmp (str, "table") == 0)
        return Vctable;
#endif
    if (strcmp (str, "mtf") == 0)
        return Vcmtf;
    if (strcmp (str, "rle") == 0)
        return Vcrle;
    if (strcmp (str, "huffman") == 0)
        return Vchuffman;
    if (strcmp (str, "huffgroup") == 0)
        return Vchuffgroup;
    return NULL;
}

