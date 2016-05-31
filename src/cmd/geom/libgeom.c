#pragma prototyped

#include <ast.h>
#include <tok.h>
#include <cdt.h>
#include <swift.h>
#include "geom.h"

#if SWIFT_LITTLEENDIAN == 1
static int __i;
static GEOMpoint_t __pt;
static GEOMcolor_t __cr;
static int __rtn;
static unsigned char *__cp, __c;
#define SWI(i) ( \
    __cp = (unsigned char *) &(i), \
    __c = __cp[0], __cp[0] = __cp[3], __cp[3] = __c, \
    __c = __cp[1], __cp[1] = __cp[2], __cp[2] = __c \
)
#define SWF(f) ( \
    __cp = (unsigned char *) &(f), \
    __c = __cp[0], __cp[0] = __cp[3], __cp[3] = __c, \
    __c = __cp[1], __cp[1] = __cp[2], __cp[2] = __c \
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
#define RDXY(f, pt) ( \
    __rtn = sfread ((f), &(pt), sizeof (GEOMtpoint_t)), \
    SWF (pt[0]), SWF (pt[1]), __rtn \
)
#define WRXY(f, pt) ( \
    __pt[0] = (pt)[0], __pt[1] = (pt)[1], \
    SWF (__pt[0]), SWF (__pt[1]), \
    sfwrite ((f), &__pt, sizeof (GEOMtpoint_t)) \
)
#define RDXYZ(f, pt) ( \
    __rtn = sfread ((f), &(pt), sizeof (GEOMpoint_t)), \
    SWF (pt[0]), SWF (pt[1]), SWF (pt[2]), __rtn \
)
#define WRXYZ(f, pt) ( \
    __pt[0] = (pt)[0], __pt[1] = (pt)[1], __pt[2] = (pt)[2], \
    SWF (__pt[0]), SWF (__pt[1]), SWF (__pt[2]), \
    sfwrite ((f), &__pt, sizeof (GEOMpoint_t)) \
)
#define RDRGBA(f, cr) ( \
    __rtn = sfread ((f), &(cr), sizeof (GEOMcolor_t)), \
    SWF (cr[0]), SWF (cr[1]), SWF (cr[2]), SWF (cr[3]), __rtn \
)
#define WRRGBA(f, cr) ( \
    __cr[0] = (cr)[0], __cr[1] = (cr)[1], \
    __cr[2] = (cr)[2], __cr[3] = (cr)[3], \
    SWF (__cr[0]), SWF (__cr[1]), SWF (__cr[2]), SWF (__cr[3]), \
    sfwrite ((f), &__cr, sizeof (GEOMcolor_t)) \
)
#else
#define SWI(i) (i)
#define SWF(f) (f)
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
#define RDXY(f, pt) ( \
    sfread ((f), &(pt), sizeof (GEOMtpoint_t)) \
)
#define WRXY(f, pt) ( \
    sfwrite ((f), &(pt), sizeof (GEOMtpoint_t)) \
)
#define RDXYZ(f, pt) ( \
    sfread ((f), &(pt), sizeof (GEOMpoint_t)) \
)
#define WRXYZ(f, pt) ( \
    sfwrite ((f), &(pt), sizeof (GEOMpoint_t)) \
)
#define RDRGBA(f, cr) ( \
    sfread ((f), &(cr), sizeof (GEOMcolor_t)) \
)
#define WRRGBA(f, cr) ( \
    sfwrite ((f), &(cr), sizeof (GEOMcolor_t)) \
)
#endif

static char *__s;
#define RDLABEL(f, s) ( \
    __s = sfgetr ((f), 0, SF_STRING), \
    s = (*alloc) (NULL, 1, strlen (__s) + 1), \
    strcpy (s, __s), s == NULL ? -1 : 0 \
)
#define WRLABEL(f, s) ( \
    sfputr ((f), (s), 0) \
)

static char *typemap[] = {
    /* GEOM_TYPE_POINTS     */  "points",
    /* GEOM_TYPE_LINES      */  "lines",
    /* GEOM_TYPE_LINESTRIPS */  "linestrips",
    /* GEOM_TYPE_POLYS      */  "polys",
    /* GEOM_TYPE_TRIS       */  "tris",
    /* GEOM_TYPE_QUADS      */  "quads",
                                NULL,
};

static char *keytypemap[] = {
    /* GEOM_KEYTYPE_CHAR   */  "char",
    /* GEOM_KEYTYPE_SHORT  */  "short",
    /* GEOM_KEYTYPE_INT    */  "int",
    /* GEOM_KEYTYPE_LLONG  */  "llong",
    /* GEOM_KEYTYPE_FLOAT  */  "float",
    /* GEOM_KEYTYPE_DOUBLE */  "double",
    /* GEOM_KEYTYPE_STRING */  "string",
                               NULL,
};

static char *sdup (char *, void *(*) (void *, size_t, int));

GEOM_t *GEOMload (Sfio_t *fp, void *(*alloc) (void *, size_t, int)) {
    GEOM_t *geomp;
    char *line;
    int i;
    unsigned char *keyp;
    int itemi;
    int sections;
    int pointi, colori, indi, p2ii, flagi, labeli, tpointi;

    if (!(geomp = (*alloc) (NULL, sizeof (GEOM_t), 1)))
        goto abortload;
    geomp->points = NULL;
    geomp->pointn = -1;
    geomp->colors = NULL;
    geomp->colorn = -1;
    geomp->inds = NULL;
    geomp->indn = -1;
    geomp->p2is = NULL;
    geomp->p2in = -1;
    geomp->flags = NULL;
    geomp->flagn = -1;
    geomp->labels = NULL;
    geomp->labeln = -1;
    geomp->tpoints = NULL;
    geomp->tpointn = -1;
    if (
        !(line = sfgetr (fp, 0, 1)) ||
        !(geomp->name = sdup (line, alloc)) ||
        !(line = sfgetr (fp, 0, 1))
    )
        goto abortload;
    for (i = 0; typemap[i]; i++)
        if (strcmp (line, typemap[i]) == 0)
            break;
    if (!typemap[i])
        goto abortload;
    geomp->type = i;
    if (
        !(line = sfgetr (fp, 0, 1)) ||
        !(geomp->class = sdup (line, alloc)) ||
        RDI (fp, geomp->itemn) == -1 ||
        !(geomp->items = alloc (NULL, sizeof (char *), geomp->itemn)) ||
        !(RDI (fp, geomp->dictlen)) ||
        !(geomp->dict = alloc (NULL, sizeof (char), geomp->dictlen)) ||
        !(line = sfgetr (fp, 0, 1))
    )
        goto abortload;
    geomp->dictlen--;
    for (i = 0; keytypemap[i]; i++)
        if (strcmp (line, keytypemap[i]) == 0)
            break;
    if (!keytypemap[i])
        goto abortload;
    geomp->keytype = i;
    if (
        RDI (fp, geomp->keylen) == -1 ||
        sfread (fp, geomp->dict, geomp->dictlen + 1) != geomp->dictlen + 1
    )
        goto abortload;
    keyp = geomp->dict;
    if (geomp->keylen == -1) {
        for (itemi = 0; itemi < geomp->itemn; itemi++) {
            geomp->items[itemi] = keyp;
            while (*keyp++ != 0)
                ;
        }
    } else {
        for (itemi = 0; itemi < geomp->itemn; itemi++) {
            geomp->items[itemi] = keyp;
            SWI (geomp->items[itemi]);
            keyp += geomp->keylen;
        }
    }
    if (RDI (fp, sections) == -1)
        goto abortload;
    if (sections & GEOM_SECTION_POINTS) {
        if (
            RDI (fp, geomp->pointn) == -1 ||
            !(geomp->points = alloc (NULL, sizeof (GEOMpoint_t), geomp->pointn))
        )
            goto abortload;
        for (pointi = 0; pointi < geomp->pointn; pointi++)
            if (RDXYZ (fp, geomp->points[pointi]) == -1)
                goto abortload;
    }
    if (sections & GEOM_SECTION_COLORS) {
        if (
            RDI (fp, geomp->colorn) == -1 ||
            !(geomp->colors = alloc (NULL, sizeof (GEOMcolor_t), geomp->colorn))
        )
            goto abortload;
        for (colori = 0; colori < geomp->colorn; colori++)
            if (RDRGBA (fp, geomp->colors[colori]) == -1)
                goto abortload;
    }
    if (sections & GEOM_SECTION_INDS) {
        if (
            RDI (fp, geomp->indn) == -1 ||
            !(geomp->inds = alloc (NULL, sizeof (int), geomp->indn))
        )
            goto abortload;
        for (indi = 0; indi < geomp->indn; indi++)
            if (RDI (fp, geomp->inds[indi]) == -1)
                goto abortload;
    }
    if (sections & GEOM_SECTION_P2IS) {
        if (
            RDI (fp, geomp->p2in) == -1 ||
            !(geomp->p2is = alloc (NULL, sizeof (int), geomp->p2in))
        )
            goto abortload;
        for (p2ii = 0; p2ii < geomp->p2in; p2ii++)
            if (RDI (fp, geomp->p2is[p2ii]) == -1)
                goto abortload;
    }
    if (sections & GEOM_SECTION_FLAGS) {
        if (
            RDI (fp, geomp->flagn) == -1 ||
            !(geomp->flags = alloc (NULL, sizeof (int), geomp->flagn))
        )
            goto abortload;
        for (flagi = 0; flagi < geomp->flagn; flagi++)
            if (RDI (fp, geomp->flags[flagi]) == -1)
                goto abortload;
    }
    if (sections & GEOM_SECTION_LABELS) {
        if (
            RDI (fp, geomp->labeln) == -1 ||
            !(geomp->labels = alloc (NULL, sizeof (GEOMlabel_t), geomp->labeln))
        )
            goto abortload;
        for (labeli = 0; labeli < geomp->labeln; labeli++) {
            if (RDI (fp, geomp->labels[labeli].itemi) == -1)
                goto abortload;
            if (RDXYZ (fp, geomp->labels[labeli].o) == -1)
                goto abortload;
            if (RDXYZ (fp, geomp->labels[labeli].c) == -1)
                goto abortload;
            if (RDLABEL (fp, geomp->labels[labeli].label) == -1)
                goto abortload;
        }
    }
    if (sections & GEOM_SECTION_TEXPTS) {
        if (
            RDI (fp, geomp->tpointn) == -1 ||
            !(geomp->tpoints = alloc (
                NULL, sizeof (GEOMtpoint_t), geomp->tpointn
            ))
        )
            goto abortload;
        for (tpointi = 0; tpointi < geomp->tpointn; tpointi++)
            if (RDXY (fp, geomp->tpoints[tpointi]) == -1)
                goto abortload;
    }
    return geomp;

abortload:
    SUwarning (1, "GEOMload", "load failed");
    return NULL;
}

int GEOMsave (Sfio_t *fp, GEOM_t *geomp) {
    int dictlen;
    int itemi;
    int sections;
    int pointi, colori, indi, p2ii, flagi, labeli, tpointi;

    dictlen = geomp->dictlen + 1;
    if (
        sfputr (fp, geomp->name, 0) == -1 ||
        sfputr (fp, typemap[geomp->type], 0) == -1 ||
        sfputr (fp, geomp->class, 0) == -1 ||
        WRI (fp, geomp->itemn) == -1 ||
        WRI (fp, dictlen) == -1 ||
        sfputr (fp, keytypemap[geomp->keytype], 0) == -1 ||
        WRI (fp, geomp->keylen) == -1
    )
        goto abortsave;
    if (geomp->keylen == -1) {
        for (itemi = 0; itemi < geomp->itemn; itemi++)
            if (sfputr (fp, (char *) geomp->items[itemi], 0) == -1)
                goto abortsave;
    } else {
        for (itemi = 0; itemi < geomp->itemn; itemi++)
            if (WRI (fp, * (int *) geomp->items[itemi]) == -1)
                goto abortsave;
    }
    sections = 0;
    if (geomp->points)
        sections |= GEOM_SECTION_POINTS;
    if (geomp->colors)
        sections |= GEOM_SECTION_COLORS;
    if (geomp->inds)
        sections |= GEOM_SECTION_INDS;
    if (geomp->p2is)
        sections |= GEOM_SECTION_P2IS;
    if (geomp->flags)
        sections |= GEOM_SECTION_FLAGS;
    if (geomp->labels)
        sections |= GEOM_SECTION_LABELS;
    if (geomp->tpoints)
        sections |= GEOM_SECTION_TEXPTS;
    if (
        sfputr (fp, "", 0) == -1 ||
        WRI (fp, sections) == -1
    )
        goto abortsave;
    if (sections & GEOM_SECTION_POINTS) {
        if (WRI (fp, geomp->pointn) == -1)
            goto abortsave;
        for (pointi = 0; pointi < geomp->pointn; pointi++)
            if (WRXYZ (fp, geomp->points[pointi]) == -1)
                goto abortsave;
    }
    if (sections & GEOM_SECTION_COLORS) {
        if (WRI (fp, geomp->colorn) == -1)
            goto abortsave;
        for (colori = 0; colori < geomp->colorn; colori++)
            if (WRRGBA (fp, geomp->colors[colori]) == -1)
                goto abortsave;
    }
    if (sections & GEOM_SECTION_INDS) {
        if (WRI (fp, geomp->indn) == -1)
            goto abortsave;
        for (indi = 0; indi < geomp->indn; indi++)
            if (WRI (fp, geomp->inds[indi]) == -1)
                goto abortsave;
    }
    if (sections & GEOM_SECTION_P2IS) {
        if (WRI (fp, geomp->p2in) == -1)
            goto abortsave;
        for (p2ii = 0; p2ii < geomp->p2in; p2ii++)
            if (WRI (fp, geomp->p2is[p2ii]) == -1)
                goto abortsave;
    }
    if (sections & GEOM_SECTION_FLAGS) {
        if (WRI (fp, geomp->flagn) == -1)
            goto abortsave;
        for (flagi = 0; flagi < geomp->flagn; flagi++)
            if (WRI (fp, geomp->flags[flagi]) == -1)
                goto abortsave;
    }
    if (sections & GEOM_SECTION_LABELS) {
        if (WRI (fp, geomp->labeln) == -1)
            goto abortsave;
        for (labeli = 0; labeli < geomp->labeln; labeli++) {
            if (WRI (fp, geomp->labels[labeli].itemi) == -1)
                goto abortsave;
            if (WRXYZ (fp, geomp->labels[labeli].o) == -1)
                goto abortsave;
            if (WRXYZ (fp, geomp->labels[labeli].c) == -1)
                goto abortsave;
            if (WRLABEL (fp, geomp->labels[labeli].label) == -1)
                goto abortsave;
        }
    }
    if (sections & GEOM_SECTION_TEXPTS) {
        if (WRI (fp, geomp->tpointn) == -1)
            goto abortsave;
        for (tpointi = 0; tpointi < geomp->tpointn; tpointi++)
            if (WRXY (fp, geomp->tpoints[tpointi]) == -1)
                goto abortsave;
    }
    return 0;

abortsave:
    SUwarning (1, "GEOMsave", "save failed");
    return -1;
}

static char *sdup (char *s, void *(*alloc) (void *, size_t, int)) {
    char *dups;

    if (!s) {
        SUwarning (1, "sdup", "NULL pointer");
        return NULL;
    }
    if (!(dups = alloc (NULL, sizeof (char), strlen (s) + 1))) {
        SUwarning (1, "sdup", "alloc failed for '%s'", s);
        return NULL;
    }
    strcpy (dups, s);
    return dups;
}
