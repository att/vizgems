#pragma prototyped

#ifndef _TIGER_INCLUDE
#define _TIGER_INCLUDE

#include <ast.h>
#include <cdt.h>

#define T_EATTR_ZIP        1
#define T_EATTR_NPANXXLOC  2
#define T_EATTR_STATE      3
#define T_EATTR_COUNTY     4
#define T_EATTR_CTBNA      5
#define T_EATTR_BLKG       6
#define T_EATTR_BLK        7
#define T_EATTR_BLKS       8
#define T_EATTR_COUNTRY    9
#define T_EATTR_CFCC      10

typedef struct string_t {
    Dtlink_t link;
    /* begin key */
    char *str;
    /* end key */
    int id, refcount;
} string_t;

typedef struct xy_t {
    int x, y;
} xy_t;

typedef struct edgedata_t {
    int tlid;
    int nameid;
    int zipl, zipr;
    int npanxxlocl, npanxxlocr;
    short countyl, countyr;
    int ctbnal, ctbnar;
    short blkl, blkr;
    char statel, stater;
    char blksl, blksr;
    xy_t xy0, xy1;
    short cfccid;
    char onesided;
} edgedata_t;

typedef struct polydata_t {
    int cenid;
    int polyid;
} polydata_t;

typedef struct edge2polydata_t {
    int tlid;
    int cenidl, cenidr;
    int polyidl, polyidr;
} edge2polydata_t;

typedef struct vertex_t {
    Dtlink_t link;
    /* begin key */
    xy_t xy;
    /* end key */
    struct edge_t **edgeps;
    int edgepn, edgepl;
} vertex_t;

typedef struct edge_t {
    Dtlink_t link;
    /* begin key */
    int tlid;
    /* end key */
    int onesided;
    string_t *sp;
    short cfccid;
    int zipl, zipr;
    int npanxxlocl, npanxxlocr;
    short countyl, countyr;
    int ctbnal, ctbnar;
    short blkl, blkr;
    char statel, stater;
    char blksl, blksr;
    struct vertex_t *v0p, *v1p;
    int v0i, v1i;
    struct poly_t *p0p, *p1p;
    int p0i, p1i;
    xy_t *xys;
    int xyn;
    int mark;
} edge_t;

typedef struct poly_t {
    Dtlink_t link;
    /* begin key */
    int cenid;
    int polyid;
    /* end key */
    struct edge_t **edgeps;
    int edgepn, edgepl;
    int mark;
} poly_t;

extern Dt_t *stringdict, *vertexdict, *edgedict, *polydict;

/* from tigerutils.c */
extern int mergeonesidededges (void);
extern int removedegree2vertices (int);
extern int mergepolysbyattr (int);
extern int buildpolysbyattr (int);
extern int removezeroedges (void);
extern int extractbyattr (int, int);
extern int simplifyxys (float);
extern void printps (int);
extern int cfccstr2id (char *);
extern int cenidstr2id (char *);
extern char *cfccid2str (int);
extern char *cenidid2str (int);

/* from tigerfiles.c */
extern int loaddata (char *);
extern int loadattrboundaries (char *, int);
extern int savedata (char *);

/* from tigerrecords.c */
extern int initrecords (void);
extern string_t *createstring (char *);
extern string_t *findstring (char *);
extern int destroystring (string_t *);
extern vertex_t *createvertex (xy_t);
extern vertex_t *findvertex (xy_t);
extern int destroyvertex (vertex_t *);
extern edge_t *createedge (edgedata_t *, char *);
extern edge_t *findedge (int);
extern int attachshapexys (edge_t *, xy_t *, int);
extern int destroyedge (edge_t *);
extern int linkedgenvertices (edge_t *, vertex_t *, vertex_t *);
extern int unlinkedgenvertices (edge_t *);
extern int spliceedges (vertex_t *);
extern poly_t *createpoly (polydata_t *);
extern poly_t *findpoly (int, int);
extern int destroypoly (poly_t *);
extern int linkedgenpolys (edge2polydata_t *);
extern int unlinkedgenpolys (edge_t *);
extern int unlinkpolynedges (poly_t *);
extern int orderedges (poly_t *);
extern int mergeedges (edge_t *, edge_t *);
extern int mergepolys (edge_t *);
extern int checkrecords (void);
extern void printrecords (void);
extern void printstats (void);

#endif
