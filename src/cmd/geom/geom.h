#pragma prototyped

#ifndef _GEOM_INCLUDE
#define _GEOM_INCLUDE

#include <ast.h>
#include <cdt.h>

typedef float GEOMpoint_t[3];
typedef float GEOMcolor_t[4];
typedef float GEOMtpoint_t[2];
typedef struct GEOMlabel_s {
    int itemi;
    GEOMpoint_t o, c;
    char *label;
} GEOMlabel_t;

typedef struct GEOM_t {
    char *name;
    int type;
    char *class;
    unsigned char **items, *dict;
    int itemn, dictlen, keytype, keylen;
    int sections;
    GEOMpoint_t *points;
    int pointn;
    GEOMcolor_t *colors;
    int colorn;
    int *inds;
    int indn;
    int *p2is;
    int p2in;
    int *flags;
    int flagn;
    GEOMlabel_t *labels;
    int labeln;
    GEOMtpoint_t *tpoints;
    int tpointn;
} GEOM_t;

#define GEOM_TYPE_POINTS     0
#define GEOM_TYPE_LINES      1
#define GEOM_TYPE_LINESTRIPS 2
#define GEOM_TYPE_POLYS      3
#define GEOM_TYPE_TRIS       4
#define GEOM_TYPE_QUADS      5

/* DANGER: the following macros must match the ones in aggr.h */
#define GEOM_KEYTYPE_CHAR   0
#define GEOM_KEYTYPE_SHORT  1
#define GEOM_KEYTYPE_INT    2
#define GEOM_KEYTYPE_LLONG  3
#define GEOM_KEYTYPE_FLOAT  4
#define GEOM_KEYTYPE_DOUBLE 5
#define GEOM_KEYTYPE_STRING 6

#define GEOM_SECTION_POINTS  1
#define GEOM_SECTION_COLORS  2
#define GEOM_SECTION_INDS    4
#define GEOM_SECTION_P2IS    8
#define GEOM_SECTION_FLAGS  16
#define GEOM_SECTION_LABELS 32
#define GEOM_SECTION_TEXPTS 64

GEOM_t *GEOMload (Sfio_t *, void *(*) (void *, size_t, int));
int GEOMsave (Sfio_t *, GEOM_t *);

#endif
