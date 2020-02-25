#pragma prototyped

#include <ast.h>
#include <vmalloc.h>
#include <swift.h>
#include "pmem.h"

typedef struct segnoff_s {
    short d1, d2;
    int segi1, off1;
    int segi2, off2;
} segnoff_t;

static PMarena_t *arenap;
static unsigned char *anchorp;

static PMsize_t newsegmentsize;

static PMptrfunc ptrfunc;
static Sfio_t *ptrfp;

static segnoff_t *segnoffs;
static int segnoffl, segnoffn;

static int PMptrread (int, int);
static int cmp (const void *, const void *);
static int cmp2 (const void *, const void *);

int PMinit (
    char *file, PMsize_t size, PMptrfunc func, int newflag, int compactflag
) {
    Sfio_t *fp;
    PMsegment_t *segp;
    int segi;
    char vstr[8];
    int rtn;

    if (size <= 2 * PM_ALIGN_UNIT) {
        SUwarning (
            0, "PMinit",
            "segment size %d <= 2 * alignment size %d", size, PM_ALIGN_UNIT
        );
        return -1;
    }
    newsegmentsize = size;
    ptrfunc = func;
    if (!(arenap = vmalloc (Vmheap, sizeof (PMarena_t)))) {
        SUwarning (0, "PMinit", "cannot allocate arena");
        return -1;
    }
    arenap->segments = NULL, arenap->segmentn = 0;
    anchorp = NULL;
    if (newflag)
        return 0;
    if (!(fp = sfopen (NULL, file, "r"))) {
        SUwarning (0, "PMinit", "cannot open state file %s", file);
        return -1;
    }
    if (sfread (fp, vstr, 4) != 4) {
        SUwarning (0, "PMinit", "cannot read version number");
        return -1;
    }
    if (strncmp (vstr, PM_VERSION, 4) != 0) {
        SUwarning (0, "PMinit", "wrong version number - ignoring state");
        return 0;
    }
    if (sfread (fp, &arenap->segmentn, sizeof (int)) != sizeof (int)) {
        SUwarning (0, "PMinit", "cannot read num of segments");
        return -1;
    }
    if (!(arenap->segments = vmresize (
        Vmheap, arenap->segments, sizeof (PMsegment_t) * arenap->segmentn,
        VM_RSCOPY
    ))) {
        SUwarning (0, "PMinit", "cannot allocate segment list");
        return -1;
    }
    for (segi = 0; segi < arenap->segmentn; segi++) {
        segp = &arenap->segments[segi];
        segp->mem = NULL;
        segp->end = segp->size = 0;
        memset (segp->slots, 0, sizeof (PMoff_t) * PM_SLOT_N);
        if (
            sfread (fp, &segp->size, sizeof (PMsize_t)) != sizeof (PMsize_t) ||
            sfread (fp, &segp->end, sizeof (PMoff_t)) != sizeof (PMoff_t)
        ) {
            SUwarning (0, "PMinit", "cannot read segment %d size, end", segi);
            return -1;
        }
        if (!(segp->mem = vmalloc (Vmheap, segp->size))) {
            SUwarning (0, "PMinit", "cannot allocate segment %d memory", segi);
            return -1;
        }
        if (sfread (
            fp, segp->slots, sizeof (int) * PM_SLOT_N
        ) != sizeof (int) * PM_SLOT_N) {
            SUwarning (0, "PMinit", "cannot read segment %d slots", segi);
            return -1;
        }
        if (sfread (fp, segp->mem, segp->size) != segp->size) {
            SUwarning (0, "PMinit", "cannot read segment %d memory", segi);
            return -1;
        }
    }
    ptrfp = fp;
    if (PMptrread (TRUE, compactflag) == -1) {
        SUwarning (0, "PMinit", "cannot read anchor data");
        return -1;
    }
    while ((rtn = PMptrread (FALSE, compactflag)) >= 0)
        ;
    if (rtn == -1) {
        SUwarning (0, "PMinit", "cannot read pointer data");
        return -1;
    }
    ptrfp = NULL;
    sfclose (fp);
    if (compactflag)
        return PMcompact ();

    return 0;
}

int PMterm (char *file) {
    Sfio_t *fp;
    PMsegment_t *segp;
    int segi;
    char file2[1024];

    if (!file)
        return 0;
    if (!anchorp) {
        SUwarning (0, "PMterm", "anchor not set");
        return -1;
    }
    sfsprintf (file2, 1024, "%s.tmp", file);
    if (!(fp = sfopen (NULL, file2, "w"))) {
        SUwarning (0, "PMterm", "cannot open state file %s", file2);
        return -1;
    }
    if (sfwrite (fp, PM_VERSION, 4) != 4) {
        SUwarning (0, "PMterm", "cannot write version number");
        return -1;
    }
    if (sfwrite (fp, &arenap->segmentn, sizeof (int)) != sizeof (int)) {
        SUwarning (0, "PMterm", "cannot write num of segments");
        return -1;
    }
    for (segi = 0; segi < arenap->segmentn; segi++) {
        segp = &arenap->segments[segi];
        if (
            sfwrite (fp, &segp->size, sizeof (PMsize_t)) != sizeof (PMsize_t) ||
            sfwrite (fp, &segp->end, sizeof (PMoff_t)) != sizeof (PMoff_t)
        ) {
            SUwarning (0, "PMterm", "cannot write segment %d size, end", segi);
            return -1;
        }
        if (sfwrite (
            fp, segp->slots, sizeof (int) * PM_SLOT_N
        ) != sizeof (int) * PM_SLOT_N) {
            SUwarning (0, "PMterm", "cannot write segment %d slots", segi);
            return -1;
        }
        if (sfwrite (fp, segp->mem, segp->size) != segp->size) {
            SUwarning (0, "PMterm", "cannot write segment %d memory", segi);
            return -1;
        }
    }
    ptrfp = fp;
    PMptrwrite (NULL);
    if (ptrfunc)
        (*ptrfunc) ();
    ptrfp = NULL;
    sfclose (fp);
    if (rename (file2, file) == -1) {
        SUwarning (0, "PMterm", "cannot rename state file %s", file2);
        return -1;
    }
    return 0;
}

void *PMalloc (PMsize_t size) {
    PMsegment_t *segp;
    int segi;
    int sloti;
    PMsize_t asize, fsize;
    PMoff_t off, poff;

    if (size == 0)
        size = 1;
    asize = PM_ALIGN_SIZE (size);
    sloti = PM_SLOT_FROMSIZE (asize);
    fsize = PM_ITEM_FULLSIZE (asize);
    segi = 0;

again:
    for (; segi < arenap->segmentn; segi++) {
        segp = &arenap->segments[segi];
        if (segp->size - segp->end >= fsize) {
            off = segp->end;
            segp->end += fsize;
            PM_ITEM_SETMAGIC (segp->mem, off);
            PM_ITEM_NEXT (segp->mem, off) = segp->slots[sloti];
            PM_ITEM_SIZE (segp->mem, off) = asize;
            segp->slots[sloti] = off;
        }
        if (PM_SLOT_ISFIXED (sloti)) {
            poff = -1;
            if ((off = segp->slots[sloti]) != 0)
                goto found;
        } else {
            for (
                poff = -1, off = segp->slots[sloti];
                off != 0; off = PM_ITEM_NEXT (segp->mem, off)
            ) {
                if (PM_ITEM_SIZE (segp->mem, off) >= asize)
                    goto found;
                poff = off;
            }
        }
    }
    if (fsize + PM_ALIGN_UNIT > newsegmentsize) {
        SUwarning (0, "PMalloc", "extending segment size to fit alloc request");
        newsegmentsize = 1 * (fsize + PM_ALIGN_UNIT);
    }
    if (!(arenap->segments = vmresize (
        Vmheap, arenap->segments, sizeof (PMsegment_t) * (arenap->segmentn + 1),
        VM_RSCOPY
    ))) {
        SUwarning (0, "PMalloc", "cannot allocate segment");
        return NULL;
    }
    segp = &arenap->segments[arenap->segmentn];
    arenap->segmentn++;
    if (!(segp->mem = vmalloc (Vmheap, newsegmentsize))) {
        SUwarning (0, "PMalloc", "cannot allocate new segment memory");
        return NULL;
    }
    segp->size = newsegmentsize;
    segp->end = PM_ALIGN_UNIT;
    memset (segp->slots, 0, sizeof (PMoff_t) * PM_SLOT_N);
    segi = arenap->segmentn - 1;
    goto again;

found:
    if (poff == -1)
        segp->slots[sloti] = PM_ITEM_NEXT (segp->mem, off);
    else
        PM_ITEM_NEXT (segp->mem, poff) = PM_ITEM_NEXT (segp->mem, off);
    PM_ITEM_USIZE (segp->mem, off) = asize;
    return segp->mem + off + sizeof (PMhdr_t);
}

void *PMresize (void *oldp, PMsize_t newsize) {
    unsigned char *newp;
    PMsegment_t *segp;
    int segi;
    PMsize_t asize, oldsize;
    PMoff_t off;

    asize = PM_ALIGN_SIZE (newsize);
    off = 0, oldsize = 0;
    for (segi = 0; segi < arenap->segmentn; segi++) {
        segp = &arenap->segments[segi];
        if (
            oldp < segp->mem + sizeof (PMhdr_t) ||
            oldp >= segp->mem + segp->size
        )
            continue;
        off = (unsigned char *) oldp - segp->mem - sizeof (PMhdr_t);
        oldsize = PM_ITEM_SIZE (segp->mem, off);
        break;
    }
    if (oldp && off == 0) {
        SUwarning (0, "PMresize", "cannot find old memory");
        return NULL;
    }
    if (asize <= oldsize) {
        PM_ITEM_USIZE (segp->mem, off) = asize;
        return oldp;
    }
    if (!(newp = PMalloc (newsize))) {
        SUwarning (0, "PMresize", "cannot allocate new memory");
        return NULL;
    }
    if (oldp) {
        memcpy (newp, oldp, oldsize);
        if (!PMfree (oldp)) {
            SUwarning (0, "PMresize", "cannot free old memory");
            return NULL;
        }
    }
    return newp;
}

void *PMstrdup (char *str) {
    char *p;

    if (!(p = PMalloc (strlen (str) + 1))) {
        SUwarning (0, "PMresize", "cannot allocate new memory");
        return NULL;
    }
    strcpy (p, str);
    return p;
}

void *PMfree (void *p) {
    PMsegment_t *segp;
    int segi;
    int sloti;
    PMsize_t asize;
    PMoff_t off;

    for (segi = 0; segi < arenap->segmentn; segi++) {
        segp = &arenap->segments[segi];
        if (p < segp->mem + sizeof (PMhdr_t) || p >= segp->mem + segp->size)
            continue;
        off = (unsigned char *) p - segp->mem - sizeof (PMhdr_t);
        asize = PM_ITEM_SIZE (segp->mem, off);
        sloti = PM_SLOT_FROMSIZE (asize);
        PM_ITEM_NEXT (segp->mem, off) = segp->slots[sloti];
        segp->slots[sloti] = off;
        return p;
    }
    SUwarning (0, "PMfree", "cannot find memory");
    return NULL;
}

void PMsetanchor (void *p) {
    anchorp = p;
}

void *PMgetanchor (void) {
    return anchorp;
}

int PMcompact (void) {
    segnoff_t *segnoffp, **segnoffs2;
    int segnoffi, segnoffj;
    PMsegment_t *osegp2, *nsegp;
    int newbasesegi, osegi, osegi2, nsegi;
    int ooff2, noff;
    unsigned char *op2, *np, *np1, *np2;
    PMsize_t asize, fsize;

    if (!(segnoffs2 = vmalloc (Vmheap, sizeof (segnoff_t *) * segnoffl))) {
        SUwarning (0, "PMcompact", "cannot allocate segment2 list");
        return -1;
    }
    for (segnoffi = 0; segnoffi < segnoffl; segnoffi++)
        segnoffs2[segnoffi] = &segnoffs[segnoffi];

    newbasesegi = arenap->segmentn;
    for (osegi = 0; osegi < arenap->segmentn; osegi++)
        if (newsegmentsize < arenap->segments[osegi].size)
            newsegmentsize = arenap->segments[osegi].size;

    if (!(arenap->segments = vmresize (
        Vmheap, arenap->segments, sizeof (PMsegment_t) * (arenap->segmentn + 1),
        VM_RSCOPY
    ))) {
        SUwarning (0, "PMcompact", "cannot allocate segments");
        return -1;
    }
    nsegp = &arenap->segments[arenap->segmentn];
    nsegi = arenap->segmentn++;
    if (!(nsegp->mem = vmalloc (Vmheap, newsegmentsize))) {
        SUwarning (0, "PMcompact", "cannot allocate new segment memory");
        return -1;
    }
    nsegp->size = newsegmentsize;
    nsegp->end = PM_ALIGN_UNIT;
    memset (nsegp->slots, 0, sizeof (PMoff_t) * PM_SLOT_N);

    qsort (segnoffs, segnoffl, sizeof (segnoff_t), cmp);
    qsort (segnoffs2, segnoffl, sizeof (segnoff_t *), cmp2);
    segnoffj = 0;
    for (segnoffi = 0; segnoffi < segnoffl; ) {
        segnoffp = &segnoffs[segnoffi];
        if (segnoffp->d2 == 1) {
            segnoffi++;
            continue;
        }
        osegi2 = segnoffp->segi2;
        ooff2 = segnoffp->off2;
        osegp2 = &arenap->segments[osegi2];
        op2 = osegp2->mem + ooff2 + sizeof (PMhdr_t);
        if (!PM_ITEM_ISMAGIC (osegp2->mem, ooff2)) {
            SUwarning (
                0, "PMcompact", "cannot handle pointer %d,%d", osegi2, ooff2
            );
            segnoffi++;
            continue;
        }
        asize = PM_ITEM_USIZE (osegp2->mem, ooff2);
        fsize = PM_ITEM_FULLSIZE (asize);
        if (nsegp->size - nsegp->end < fsize) {
            if (!(arenap->segments = vmresize (
                Vmheap, arenap->segments,
                sizeof (PMsegment_t) * (arenap->segmentn + 1), VM_RSCOPY
            ))) {
                SUwarning (0, "PMcompact", "cannot allocate segments");
                return -1;
            }
            nsegp = &arenap->segments[arenap->segmentn];
            nsegi = arenap->segmentn++;
            if (!(nsegp->mem = vmalloc (Vmheap, newsegmentsize))) {
                SUwarning (
                    0, "PMcompact", "cannot allocate new segment memory"
                );
                return -1;
            }
            nsegp->size = newsegmentsize;
            nsegp->end = PM_ALIGN_UNIT;
            memset (nsegp->slots, 0, sizeof (PMoff_t) * PM_SLOT_N);
        }
        noff = nsegp->end;
        np = nsegp->mem + noff + sizeof (PMhdr_t);
        nsegp->end += fsize;
        PM_ITEM_SETMAGIC (nsegp->mem, noff);
        PM_ITEM_SIZE (nsegp->mem, noff) = asize;
        PM_ITEM_USIZE (nsegp->mem, noff) = asize;
        memcpy (np, op2, asize);
        for (; segnoffi < segnoffl; segnoffi++) {
            segnoffp = &segnoffs[segnoffi];
            if (
                segnoffp->segi2 != osegi2 ||
                segnoffp->off2 < ooff2 || segnoffp->off2 >= ooff2 + fsize
            )
                break;
            segnoffp->segi2 = nsegi - newbasesegi;
            segnoffp->off2 = noff + (segnoffp->off2 - ooff2);
            segnoffp->d2 = 1;
            np2 = nsegp->mem + segnoffp->off2 + sizeof (PMhdr_t);
            if (segnoffp->segi1 == -1)
                continue;
            if (
                segnoffp->segi1 == osegi2 &&
                segnoffp->off1 >= ooff2 && segnoffp->off1 < ooff2 + fsize
            ) {
                segnoffp->segi1 = nsegi - newbasesegi;
                segnoffp->off1 = noff + (segnoffp->off1 - ooff2);
                segnoffp->d1 = 1;
            }
            if (segnoffp->d1 == 1)
                np1 = arenap->segments[
                    segnoffp->segi1 + newbasesegi
                ].mem + segnoffp->off1 + sizeof (PMhdr_t);
            else
                np1 = arenap->segments[
                    segnoffp->segi1
                ].mem + segnoffp->off1 + sizeof (PMhdr_t);
            *(unsigned char **) np1 = np2;
        }
        for (; segnoffj < segnoffl; ) {
            segnoffp = segnoffs2[segnoffj];
            if (segnoffp->segi1 == -1) {
                segnoffj++;
                continue;
            }
            if (segnoffp->d1 == 1) {
                segnoffj++;
                continue;
            }
            if (segnoffp->segi1 < osegi2 || (
                segnoffp->segi1 == osegi2 && segnoffp->off1 < ooff2
            )) {
                SUwarning (0, "PMcompact", "found p1 without p2");
                return -1;
            }
            if (
                segnoffp->segi1 != osegi2 ||
                segnoffp->off1 < ooff2 || segnoffp->off1 >= ooff2 + fsize
            )
                break;
            segnoffp->segi1 = nsegi - newbasesegi;
            segnoffp->off1 = noff + (segnoffp->off1 - ooff2);
            segnoffp->d1 = 1;
            segnoffj++;
        }
    }
    for (segnoffi = 0; segnoffi < segnoffl; segnoffi++) {
        segnoffp = &segnoffs[segnoffi];
        if (segnoffp->d2 == 0 ||segnoffp->d2 == 0)
            abort ();
    }
    for (osegi = 0; osegi < newbasesegi; osegi++)
        vmfree (Vmheap, arenap->segments[osegi].mem);
    for (osegi = newbasesegi; osegi < arenap->segmentn; osegi++)
        arenap->segments[osegi - newbasesegi] = arenap->segments[osegi];
    arenap->segmentn -= newbasesegi;
    anchorp = arenap->segments[
        segnoffs[0].segi2
    ].mem + segnoffs[0].off2 + sizeof (PMhdr_t);
    vmfree (Vmheap, segnoffs2);
    return 0;
}

int PMptrread (int isanchor, int save) {
    PMsegment_t *segp;
    int segi;
    PMoff_t off;
    void *p1, *p2;
    int rtn;

    if (save && segnoffl >= segnoffn) {
        if (!(segnoffs = vmresize (
            Vmheap, segnoffs, sizeof (segnoff_t) * (segnoffn + 1024),
            VM_RSCOPY
        ))) {
            SUwarning (0, "PMptrread", "cannot allocate segment list");
            return -1;
        }
        segnoffn += 1024;
    }

    if (isanchor) {
        if (save) {
            segnoffs[segnoffl].d1 = 0;
            segnoffs[segnoffl].d2 = 0;
            segnoffs[segnoffl].segi1 = -1;
            segnoffs[segnoffl].off1 = 0;
        }
        p1 = &anchorp;
        goto step2;
    }

    if (
        (rtn = sfread (ptrfp, &segi, sizeof (int))) != sizeof (int) ||
        sfread (ptrfp, &off, sizeof (int)) != sizeof (int)
    ) {
        if (rtn == 0)
            return -2;
        SUwarning (0, "PMptrread", "cannot read p1");
        return -1;
    }
    if (
        segi < 0 || segi >= arenap->segmentn ||
        off >= (segp = &arenap->segments[segi])->size
    ) {
        SUwarning (0, "PMptrread", "bad data for p1");
        return -1;
    }
    segp = &arenap->segments[segi];
    p1 = segp->mem + off + sizeof (PMhdr_t);
    if (save) {
        segnoffs[segnoffl].d1 = 0;
        segnoffs[segnoffl].d2 = 0;
        segnoffs[segnoffl].segi1 = segi;
        segnoffs[segnoffl].off1 = off;
    }

step2:
    if (
        sfread (ptrfp, &segi, sizeof (int)) != sizeof (int) ||
        sfread (ptrfp, &off, sizeof (int)) != sizeof (int)
    ) {
        SUwarning (0, "PMptrread", "cannot read p2");
        return -1;
    }
    if (
        segi < 0 || segi >= arenap->segmentn ||
        off >= (segp = &arenap->segments[segi])->size
    ) {
        SUwarning (0, "PMptrread", "bad data for p2");
        return -1;
    }
    segp = &arenap->segments[segi];
    p2 = segp->mem + off + sizeof (PMhdr_t);
    if (save) {
        segnoffs[segnoffl].segi2 = segi;
        segnoffs[segnoffl++].off2 = off;
    }
    *(unsigned char **) p1 = p2;

    return 0;
}

int PMptrwrite (void *p1) {
    PMsegment_t *segp;
    int segi;
    PMoff_t off;
    void *p2;

    if (!p1) {
        p2 = anchorp;
        goto step2;
    }
    if (!(p2 = *(unsigned char **) p1))
        return 0;

    for (segi = 0; segi < arenap->segmentn; segi++) {
        segp = &arenap->segments[segi];
        if (p1 < segp->mem + sizeof (PMhdr_t) || p1 >= segp->mem + segp->size)
            continue;
        off = (unsigned char *) p1 - segp->mem - sizeof (PMhdr_t);
        if (
            sfwrite (ptrfp, &segi, sizeof (int)) != sizeof (int) ||
            sfwrite (ptrfp, &off, sizeof (int)) != sizeof (int)
        ) {
            SUwarning (0, "PMptrwrite", "cannot write p1");
            return -1;
        }
        goto step2;
    }
    SUwarning (0, "PMptrwrite", "cannot map pointer1 %x", p1);
    return -1;

step2:
    for (segi = 0; segi < arenap->segmentn; segi++) {
        segp = &arenap->segments[segi];
        if (p2 < segp->mem + sizeof (PMhdr_t) || p2 >= segp->mem + segp->size)
            continue;
        off = (unsigned char *) p2 - segp->mem - sizeof (PMhdr_t);
        if (
            sfwrite (ptrfp, &segi, sizeof (int)) != sizeof (int) ||
            sfwrite (ptrfp, &off, sizeof (int)) != sizeof (int)
        ) {
            SUwarning (0, "PMptrwrite", "cannot write p2");
            return -1;
        }
        goto done;
    }
    SUwarning (0, "PMptrwrite", "cannot map pointer2 %x", p2);
    return -1;

done:
    return 0;
}

static int cmp (const void *a, const void *b) {
    segnoff_t *ap, *bp;

    ap = (segnoff_t *) a;
    bp = (segnoff_t *) b;
    if (ap->segi2 != bp->segi2)
        return ap->segi2 - bp->segi2;
    return ap->off2 - bp->off2;
}

static int cmp2 (const void *a, const void *b) {
    segnoff_t *ap, *bp;

    ap = *(segnoff_t **) a;
    bp = *(segnoff_t **) b;
    if (ap->segi1 != bp->segi1)
        return ap->segi1 - bp->segi1;
    return ap->off1 - bp->off1;
}
