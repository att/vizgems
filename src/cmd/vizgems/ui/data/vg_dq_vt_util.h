#ifndef _VG_DQ_VT_UTIL_INCLUDE
#define _VG_DQ_VT_UTIL_INCLUDE

#include <ast.h>
#include <vmalloc.h>
#include <swift.h>
#include <graphviz/graphvizconfig.h>
#include <graphviz/geom.h>
#include <graphviz/gvc.h>
#include <graphviz/graph.h>
#include <graphviz/gd.h>
#include <graphviz/gdfontt.h>
#include <graphviz/gdfonts.h>
#include <graphviz/gdfontmb.h>
#include <graphviz/gdfontl.h>
#include <graphviz/gdfontg.h>
#include <graphviz/gdfont4.h>
#include <graphviz/gdfont5.h>
#include <graphviz/gdfont6.h>
#include <graphviz/gdfont7.h>
#include <graphviz/gdfont8.h>
#include <graphviz/gdfont9.h>
#include <graphviz/gdfont10.h>
#include <graphviz/gdfont11.h>
#include <graphviz/gdfont12.h>
#include <graphviz/gdfont13.h>
#include <graphviz/gdfont14.h>
#include <graphviz/gdfont15.h>
#include <graphviz/gdfont16.h>
#include <graphviz/gdfont17.h>
#include <graphviz/gdfont18.h>
#include <graphviz/gdfont19.h>
#include <graphviz/gdfont20.h>
#include "vg_hdr.h"

#define SZ_level  VG_inv_node_level_L
#define SZ_id     VG_inv_node_id_L
#define SZ_val    VG_inv_node_val_L
#define SZ_skey   VG_stat_key_L
#define SZ_sunit  VG_stat_unit_L
#define SZ_slabel VG_stat_label_L

/* UT */

#define UT_ATTRGROUP_MAIN   0
#define UT_ATTRGROUP_GRAPH  1
#define UT_ATTRGROUP_NODE   2
#define UT_ATTRGROUP_EDGE   3
#define UT_ATTRGROUP_TITLE  4
#define UT_ATTRGROUP_METRIC 5
#define UT_ATTRGROUP_ALARM  6
#define UT_ATTRGROUP_XAXIS  7
#define UT_ATTRGROUP_YAXIS  8
#define UT_ATTRGROUP_SIZE 9

#define UT_ATTR_AVGLINEWIDTH  0
#define UT_ATTR_BB            1
#define UT_ATTR_BGCOLOR       2
#define UT_ATTR_CAPCOLOR      3
#define UT_ATTR_COLOR         4
#define UT_ATTR_TMODENAMES    5
#define UT_ATTR_SEVNAMES      6
#define UT_ATTR_SEVMAP        7
#define UT_ATTR_COORD         8
#define UT_ATTR_FILLCOLOR     9
#define UT_ATTR_FLAGS        10
#define UT_ATTR_FONTCOLOR    11
#define UT_ATTR_FONTNAME     12
#define UT_ATTR_FONTSIZE     13
#define UT_ATTR_HEIGHT       14
#define UT_ATTR_LABEL        15
#define UT_ATTR_LHEIGHT      16
#define UT_ATTR_LINEWIDTH    17
#define UT_ATTR_LP           18
#define UT_ATTR_LWIDTH       19
#define UT_ATTR_POS          20
#define UT_ATTR_WIDTH        21
#define UT_ATTR_HDR          22
#define UT_ATTR_ROW          23
#define UT_ATTR_INFO         24
#define UT_ATTR_TIMEFORMAT   25
#define UT_ATTR_TIMELABEL    26
#define UT_ATTR_DRAW         27
#define UT_ATTR_HDRAW        28
#define UT_ATTR_HLDRAW       29
#define UT_ATTR_LDRAW        30
#define UT_ATTR_TDRAW        31
#define UT_ATTR_TLDRAW       32
#define UT_ATTR_MINDRAW 27
#define UT_ATTR_MAXDRAW 32
#define UT_ATTR_SIZE 33

typedef struct UTattr_s {
    char *name, *value;
} UTattr_t;

/* RI */

typedef struct RIbb_s {
    int x, y, w, h;
} RIbb_t;

typedef struct RIcolor_s {
    Dtlink_t link;
    /* begin key */
    char *name;
    /* end key */
    int id;
} RIcolor_t;

typedef struct RIcss_s {
    Dtlink_t link;
    /* begin key */
    char *key;
    /* end key */
    char *str, *tid;
} RIcss_t;

#define RI_FONT_0 0
#define RI_FONT_1 1
#define RI_FONT_2 2
#define RI_FONT_3 3
#define RI_FONT_4 4
#define RI_FONT_5 5
#define RI_FONT_6 6
#define RI_FONT_7 7
#define RI_FONT_8 8
#define RI_FONT_9 9
#define RI_FONT_10 10
#define RI_FONT_11 11
#define RI_FONT_12 12
#define RI_FONT_13 13
#define RI_FONT_14 14
#define RI_FONT_15 15
#define RI_FONT_16 16
#define RI_FONT_SIZE 17

#define RI_FONT_NONE 0
#define RI_FONT_BI   1
#define RI_FONT_TTF  2

typedef struct RIfont_s {
    int type;
    int size;
    char *file;
    gdFont *font;
} RIfont_t;

#define RI_OP_NOOP      0
#define RI_OP_E         1
#define RI_OP_e         2
#define RI_OP_P         3
#define RI_OP_p         4
#define RI_OP_L         5
#define RI_OP_B         6
#define RI_OP_b         7
#define RI_OP_T         8
#define RI_OP_F         9
#define RI_OP_FI       10
#define RI_OP_C        11
#define RI_OP_c        12
#define RI_OP_FC       13
#define RI_OP_S        14
#define RI_OP_I        15
#define RI_OP_BB       16
#define RI_OP_POS      17
#define RI_OP_SIZ      18
#define RI_OP_LEN      19
#define RI_OP_LW       20
#define RI_OP_CLIP     21
#define RI_OP_COPYC    22
#define RI_OP_COPYc    23
#define RI_OP_COPYFC   24
#define RI_OP_HTMLTBB  25
#define RI_OP_HTMLTBE  26
#define RI_OP_HTMLTBCB 27
#define RI_OP_HTMLTBCE 28
#define RI_OP_HTMLTRB  29
#define RI_OP_HTMLTRE  30
#define RI_OP_HTMLTDB  31
#define RI_OP_HTMLTDE  32
#define RI_OP_HTMLTDT  33
#define RI_OP_HTMLAB   34
#define RI_OP_HTMLAE   35
#define RI_OP_HTMLCOM  36

#define RI_STYLE_NONE      0
#define RI_STYLE_DOTTED    1
#define RI_STYLE_DASHED    2
#define RI_STYLE_LINEWIDTH 3

typedef struct RIop_s {
    int type;
    union {
        struct {
            gdPoint o, s;
            int ba, ea;
        } rect;
        struct {
            int pi, pn;
        } poly;
        struct {
            gdPoint p;
            int jx, jy, w, h, n;
            char *t;
        } text;
        struct {
            int s, n;
            char *t;
            RIfont_t f;
            int w, h;
        } font;
        struct {
            int n;
            char *t;
            int locked;
        } color;
        struct {
            int n;
            char *t;
            int style;
        } style;
        struct {
            int x, y;
        } point;
        struct {
            int l;
        } length;
        struct {
            gdImage *img;
            int rx, ry, ix, iy, iw, ih;
        } img;
        struct {
            char *lab, *fn, *fs, *bg, *cl, *url, *tid;
        } htb;
        struct {
            char *fn, *fs, *bg, *cl, *tid;
        } htr;
        struct {
            char *lab;
            char *fn, *fs, *bg, *cl, *tid;
            int j, cn;
        } htd;
        struct {
            char *url, *info, *fn, *fs, *bg, *cl, *tid;
        } ha;
        struct {
            char *str;
        } hcom;
    } u;
} RIop_t;

#define RI_AREA_AREA  1
#define RI_AREA_EMBED 2
#define RI_AREA_ALL   (1 | 2)

#define RI_AREA_RECT  1
#define RI_AREA_STYLE 2

typedef struct RIarea_s {
    int mode, type;
    union {
        struct {
            int x1, y1, x2, y2;
        } rect;
        struct {
            char *fn, *fs, *flcl, *lncl, *fncl;
        } style;
    } u;
    int pass;
    char *obj, *url, *info;
} RIarea_t;

typedef gdPoint RIpoint_t;

#define RI_TYPE_IMAGE  0
#define RI_TYPE_CANVAS 1
#define RI_TYPE_TABLE  2

typedef struct RI_s {
    int type;
    char *fprefix;
    int findex;
    int w, h;
    RIop_t *ops;
    int opn, opm, cflag;
    RIpoint_t *points;
    int pointn, pointm;
    RIpoint_t *sppoints;
    int sppointn, sppointm;
    RIarea_t *areas;
    int arean, aream;
    Sfio_t *tocfp;
    Dt_t *cdict;
    union {
        struct {
            int slicen;
            gdImage *img;
            int vpx, vpy, vpw, vph;
            int bgid, fgid;
        } image;
        struct {
            Sfio_t *fp;
            int vpx, vpy, vpw, vph;
        } canvas;
        struct {
            Sfio_t *fp, *cssfp;
            Dt_t *cssdict;
        } table;
    } u;
    Vmalloc_t *vm;
} RI_t;

#define RIY(p, y) ((int) ((p)->h - (y)))

/* EM */

#define EM_MARGIN_W 2
#define EM_MARGIN_H 2

#define EM_DIR_H 1
#define EM_DIR_V 2

typedef struct EMbb_s {
    int w, h;
    RIop_t op;
} EMbb_t;

typedef struct EMimage_s {
    Dtlink_t link;
    /* begin key */
    char *file;
    /* end key */
    RIop_t op;
} EMimage_t;

#define EM_TYPE_I 1
#define EM_TYPE_S 2

typedef struct EMrec_s {
    char *id, *im;
    int type;
    union {
        struct {
            char *file;
            int x1, x2, y1, y2;
        } i;
        struct {
            char *fn, *fs, *flcl, *lncl, *fncl;
        } s;
    } u;
} EMrec_t;

#define EM_DOMAIN_NODE 1
#define EM_DOMAIN_EDGE 2

typedef struct EMnode_s {
    Dtlink_t link;
    /* begin key */
    char level[SZ_level];
    char id[SZ_id];
    /* end key */
    EMrec_t *recs;
    int recn, recm;
} EMnode_t;

typedef struct EMedge_s {
    Dtlink_t link;
    /* begin key */
    char level1[SZ_level];
    char id1[SZ_id];
    char level2[SZ_level];
    char id2[SZ_id];
    /* end key */
    EMrec_t *recs;
    int recn, recm;
} EMedge_t;

/* IGR */

typedef struct IGRcc_s {
    Dtlink_t link;
    /* begin key */
    char level[SZ_level];
    char id[SZ_id];
    /* end key */
    int done;
    struct IGRcc_s *lccp;
    int prc, src;
    Agraph_t *gp;
    int opi, opn;
} IGRcc_t;

typedef struct IGRcl_s {
    Dtlink_t link;
    /* begin key */
    char level[SZ_level];
    char id[SZ_id];
    /* end key */
    int rc;
    short nclass;
    Agraph_t *gp;
    int opi, opn;
} IGRcl_t;

typedef struct IGRrk_s {
    Dtlink_t link;
    /* begin key */
    char level[SZ_level];
    char id[SZ_id];
    /* end key */
    int rc;
    Agraph_t *gp;
    int opi, opn;
} IGRrk_t;

typedef struct IGRnd_s {
    Dtlink_t link;
    /* begin key */
    char level[SZ_level];
    char id[SZ_id];
    /* end key */
    struct IGRcc_s **ccs, *ccp;
    int ccn;
    struct IGRcl_s **cls, *clp;
    int cln;
    struct IGRrk_s **rks, *rkp;
    int rkn;
    short nclass;
    Agraph_t *gp, *rkgp;
    Agnode_t *np;
    int opi, opn;
    int w, h, havewh;
} IGRnd_t;

typedef struct IGRed_s {
    Dtlink_t link;
    /* begin key */
    struct IGRnd_s *ndp1, *ndp2;
    /* end key */
    Agedge_t *ep;
    int opi, opn;
} IGRed_t;

/* IGD */

typedef struct IGDcc_s {
    Dtlink_t link;
    /* begin key */
    char level[SZ_level];
    char id[SZ_id];
    /* end key */
    int rc;
    int opi, opn;
} IGDcc_t;

typedef struct IGDcl_s {
    Dtlink_t link;
    /* begin key */
    char level[SZ_level];
    char id[SZ_id];
    /* end key */
    int rc;
    int bmh1, bmh2, bmh3;
    int lw;
    char *cl, *fn, *fs, *lab;
    int opi, opn;
    char *obj, *url;
} IGDcl_t;

typedef struct IGDnd_s {
    Dtlink_t link;
    /* begin key */
    char level[SZ_level];
    char id[SZ_id];
    /* end key */
    struct IGDcc_s **ccs, *ccp;
    int ccn;
    struct IGDcl_s **cls, *clp;
    int cln;
    short nclass;
    int bmx, bmy, bmw, bmh;
    int lw;
    char *cl, *fcl, *fn, *fs, *lab, *info;
    int opi, opn;
} IGDnd_t;

/* IMP */

typedef struct IMPlabel_s {
    char *name;
    float llx, lly;
    int bmx, bmy;
} IMPlabel_t;

typedef struct IMPpoint_s {
    float llx, lly;
    int bmx, bmy;
} IMPpoint_t;

typedef struct IMPpoly_s {
    char *name;
    int pn;
    IMPpoint_t *ps;
} IMPpoly_t;

typedef struct IMPgeom_s {
    int labeln;
    IMPlabel_t *labels;
    int polyn;
    IMPpoly_t *polys;
    float llx1, lly1, llx2, lly2;
} IMPgeom_t;

typedef struct IMPnd_s {
    Dtlink_t link;
    /* begin key */
    char level[SZ_level];
    char id[SZ_id];
    /* end key */
    short nclass;
    int havecoord;
    float llx, lly;
    int bmx, bmy, bmw, bmh, ovw, ovh;
    int lw;
    char *bg, *cl, *fn, *fs, *info;
    int opi, opn;
    int next, done;
} IMPnd_t;

typedef struct IMPed_s {
    Dtlink_t link;
    /* begin key */
    struct IMPnd_s *ndp1, *ndp2;
    /* end key */
    int lw;
    char *cl, *fn, *fs, *info;
    int opi, opn;
} IMPed_t;

/* IMPLL */

typedef struct IMPLLnd_s {
    Dtlink_t link;
    /* begin key */
    char level[SZ_level];
    char id[SZ_id];
    /* end key */
    short nclass;
    int havecoord;
    float llx, lly;
    int w, h;
    int lw;
    char *bg, *cl, *fn, *fs, *info;
    int opi, opn;
} IMPLLnd_t;

typedef struct IMPLLed_s {
    Dtlink_t link;
    /* begin key */
    struct IMPLLnd_s *ndp1, *ndp2;
    /* end key */
    int lw;
    char *cl, *fn, *fs, *info;
    int opi, opn;
} IMPLLed_t;

/* ITB */

typedef struct ITBcell_s {
    char *str;
    int j, sa;
    int i;
    float f;
} ITBcell_t;

typedef struct ITBnd_s {
    Dtlink_t link;
    /* begin key */
    char level[SZ_level];
    char id[SZ_id];
    /* end key */
    short nclass;
    char *lab, *hdr, *row, *bg, *cl, *fn, *fs;
    ITBcell_t *cells;
    int celln;
} ITBnd_t;

typedef struct ITBed_s {
    Dtlink_t link;
    /* begin key */
    struct ITBnd_s *ndp1, *ndp2;
    /* end key */
    char *lab, *hdr, *row, *bg, *cl, *fn, *fs;
    ITBcell_t *cells;
    int celln;
} ITBed_t;

/* AGD */

#define AGD_TMODE_YTICKET 0
#define AGD_TMODE_DTICKET 1
#define AGD_TMODE_NTICKET 2
#define AGD_TMODE_SIZE 3

typedef struct AGDalarm_s {
    Dtlink_t link;
    /* begin key */
    char ilevel1[SZ_level];
    char iid1[SZ_id];
    char ilevel2[SZ_level];
    char iid2[SZ_id];
    /* end key */
    int *cns[AGD_TMODE_SIZE];
    int *havecns[AGD_TMODE_SIZE];
    int havetms[AGD_TMODE_SIZE];
    int sgid;
} AGDalarm_t;

typedef struct AGDsevattr_s {
    char *name;
    char *bg[AGD_TMODE_SIZE], *cl[AGD_TMODE_SIZE], used[AGD_TMODE_SIZE];
    int x, lx, rx;
} AGDsevattr_t;

/* ATB */

#define ATB_TMODE_YTICKET 0
#define ATB_TMODE_DTICKET 1
#define ATB_TMODE_NTICKET 2
#define ATB_TMODE_SIZE 3

typedef struct ATBcell_s {
    char *str;
    int j, sa;
    int i;
    float f;
} ATBcell_t;

typedef struct ATBnd_s {
    Dtlink_t link;
    /* begin key */
    int index;
    /* end key */
    char ilevel[SZ_level];
    char iid[SZ_id];
    short nclass;
    int tmi, sevi;
    char *lab, *hdr, *row, *url, *fn, *fs, *scl;
    ATBcell_t *cells;
    int celln;
} ATBnd_t;

typedef struct ATBed_s {
    Dtlink_t link;
    /* begin key */
    int index;
    /* end key */
    char ilevel1[SZ_level];
    char iid1[SZ_id];
    char ilevel2[SZ_level];
    char iid2[SZ_id];
    short nclass1, nclass2;
    int tmi, sevi;
    char *lab, *hdr, *row, *url, *fn, *fs, *scl;
    ATBcell_t *cells;
    int celln;
} ATBed_t;

typedef struct ATBsevattr_s {
    char *name;
    char *bg[AGD_TMODE_SIZE], *cl[AGD_TMODE_SIZE], used[AGD_TMODE_SIZE];
} ATBsevattr_t;

/* AST */

#define AST_TMODE_YTICKET 0
#define AST_TMODE_DTICKET 1
#define AST_TMODE_NTICKET 2
#define AST_TMODE_SIZE 3

typedef struct ASTalarm_s {
    Dtlink_t link;
    /* begin key */
    char ilevel1[SZ_level];
    char iid1[SZ_id];
    char ilevel2[SZ_level];
    char iid2[SZ_id];
    /* end key */
    int *cns[AST_TMODE_SIZE];
    int *havecns[AST_TMODE_SIZE];
    int havetms[AST_TMODE_SIZE];
    int sgid;
} ASTalarm_t;

typedef struct ASTsevattr_s {
    char *name;
    char *bg[AST_TMODE_SIZE], *cl[AST_TMODE_SIZE], used[AGD_TMODE_SIZE];
} ASTsevattr_t;

/* SCH */

#define SCH_MTYPE_MEAN 0
#define SCH_MTYPE_CURR 1
#define SCH_MTYPE_PROJ 2
#define SCH_MTYPE_SIZE 3

typedef struct SCHmetric_s {
    Dtlink_t link;
    /* begin key */
    char ilevel[SZ_level];
    char iid[SZ_id];
    char key[SZ_skey];
    /* end key */
    char level[SZ_level];
    char id[SZ_id];
    int oc1, oc2, oci;
    char ostr1[SZ_skey], ostr2[SZ_skey], ostri[SZ_skey];
    char unit[SZ_sunit], label[SZ_slabel], deflabel[SZ_slabel];
    char invlabel[SZ_val];
    char **units, **labels;
    float *vals[SCH_MTYPE_SIZE];
    int *havevals[SCH_MTYPE_SIZE], *marks;
    int havemtypes[SCH_MTYPE_SIZE];
    float minvals[SCH_MTYPE_SIZE], maxvals[SCH_MTYPE_SIZE];
    int havecapv;
    float capv;
    int aggrmode;
    int instinlabels;
    char *color;
    int metricorder, chartid, lx, ly;
} SCHmetric_t;

typedef struct SCHorder_s {
    char *str;
    int len, c1, c2, ci;
} SCHorder_t;

typedef struct SCHframe_s {
    char *label;
    int level, x, lx, rx;
    int mark;
} SCHframe_t;

/* STB */

#define STB_MTYPE_MEAN 0
#define STB_MTYPE_CURR 1
#define STB_MTYPE_PROJ 2
#define STB_MTYPE_SIZE 3

typedef struct STBmetricval_s {
    int mti, framei;
    float val;
} STBmetricval_t;

typedef struct STBmetric_s {
    Dtlink_t link;
    /* begin key */
    char ilevel[SZ_level];
    char iid[SZ_id];
    char key[SZ_skey];
    /* end key */
    char level[SZ_level];
    char id[SZ_id];
    int oc1, oc2, oci;
    char ostr1[SZ_skey], ostr2[SZ_skey], ostri[SZ_skey];
    char unit[SZ_sunit], label[SZ_slabel], deflabel[SZ_slabel];
    char invlabel[SZ_val];
    char **units, **labels;
    float *vals[STB_MTYPE_SIZE];
    int *havevals[STB_MTYPE_SIZE], havemvs[STB_MTYPE_SIZE], *marks;
    int havemtypes[STB_MTYPE_SIZE];
    STBmetricval_t *mvs;
    int mvi, mvn;
    int havecapv;
    float capv;
    int instinlabels;
    int metricorder, tabid;
    char *attr, *url, *obj;
} STBmetric_t;

typedef struct STBorder_s {
    char *str;
    int len, c1, c2, ci;
} STBorder_t;

typedef struct STBframe_s {
    char *label;
    time_t t;
} STBframe_t;

_BEGIN_EXTERNS_ /* public data */

#if _BLD_vg_dq_vt_util && defined(__EXPORT__)
#define extern extern __EXPORT__
#endif
#if !_BLD_vg_dq_vt_util && defined(__IMPORT__)
#define extern extern __IMPORT__
#endif

extern UTattr_t *UTattrs[UT_ATTRGROUP_SIZE];
extern int UTattrn[UT_ATTRGROUP_SIZE], UTattrm[UT_ATTRGROUP_SIZE];
extern UTattr_t UTattrgroups[UT_ATTRGROUP_SIZE][UT_ATTR_SIZE];
extern char **UTtoks;
extern int UTtokn, UTtokm, UTlockcolor;

extern int RImaxtsflag;

extern RIop_t *EMops;
extern int EMopn, EMopm;

extern int IGRccexists, IGRclexists, IGRrkexists, IGRndexists, IGRedexists;
extern IGRcc_t **IGRccs;
extern int IGRccn;
extern IGRcl_t **IGRcls;
extern int IGRcln;
extern IGRrk_t **IGRrks;
extern int IGRrkn;
extern IGRnd_t **IGRnds, **IGRwnds;
extern int IGRndn;
extern IGRed_t **IGReds;
extern int IGRedn;

extern IGDcc_t **IGDccs;
extern int IGDccn;
extern IGDcl_t **IGDcls;
extern int IGDcln;
extern IGDnd_t **IGDnds, **IGDwnds;
extern int IGDndn;

extern IMPnd_t **IMPnds;
extern int IMPndn;
extern IMPed_t **IMPeds;
extern int IMPedn;

extern IMPLLnd_t **IMPLLnds;
extern int IMPLLndn;
extern IMPLLed_t **IMPLLeds;
extern int IMPLLedn;

extern ITBnd_t **ITBnds;
extern int ITBndn;
extern ITBed_t **ITBeds;
extern int ITBedn;

extern AGDalarm_t **AGDalarms;
extern int AGDalarmn;

extern ATBnd_t **ATBnds;
extern int ATBndn;
extern ATBed_t **ATBeds;
extern int ATBedn;

extern ASTalarm_t **ASTalarms;
extern int ASTalarmn;

extern int SCHmetricexists, SCHchartn;
extern SCHmetric_t **SCHmetrics;
extern int SCHmetricn;

extern int STBmetricexists, STBtabn;
extern STBmetric_t **STBmetrics;
extern int STBmetricn;

#undef extern
_END_EXTERNS_

_BEGIN_EXTERNS_ /* public functions */
#if _BLD_vg_dq_vt_util && defined(__EXPORT__)
#define extern  __EXPORT__
#endif

extern int UTinit (void);
extern int UTsplitattr (int, char *);
extern int UTgetattrs (int);
extern int UTaddclip (RI_t *, int, int, int, int);
extern int UTaddlw (RI_t *, int);
extern int UTaddbox (RI_t *, int, int, int, int, char *, int);
extern int UTaddline (RI_t *, int, int, int, int, char *);
extern int UTaddpolybegin (RI_t *, char *, int);
extern int UTaddpolypoint (RI_t *, int, int);
extern int UTaddpolyend (RI_t *);
extern int UTaddtext (
    RI_t *, char *, char *, char *, char *,
    int, int, int, int, int, int, int, int *, int *
);
extern int UTaddtablebegin (RI_t *, char *, char *, char *, char *);
extern int UTaddtableend (RI_t *);
extern int UTaddtablecaption (
    RI_t *, char *, char *, char *, char *, char *, char *, char *
);
extern int UTaddrowbegin (RI_t *, char *, char *, char *, char *);
extern int UTaddrowend (RI_t *);
extern int UTaddcellbegin (RI_t *, char *, char *, char *, char *, int, int);
extern int UTaddcellend (RI_t *);
extern int UTaddcelltext (RI_t *, char *);
extern int UTaddlinkbegin (
    RI_t *, char *, char *, char *, char *, char *, char *
);
extern int UTaddlinkend (RI_t *);
extern int UTsplitstr (char *);

extern int RIinit (char *, char *, int);
extern RI_t *RIopen (int, char *, int, int, int);
extern int RIclose (RI_t *);
extern int RIaddarea (RI_t *, RIarea_t *);
extern char *RIaddcss (RI_t *, char *, char *, char *, char *, char *);
extern int RIaddtoc (RI_t *, int, char *);
extern int RIparseattr (char *, int, RIop_t *);
extern int RIparseop (RI_t *, char *, int);
extern int RIaddop (RI_t *, RIop_t *);
extern int RIaddpt (RI_t *, int, int);
extern int RIaddpts (RI_t *, int *, int *, int);
extern int RIgettextsize (char *, char *, char *, RIop_t *);
extern int RIsetviewport (RI_t *, int, int, int, int, int);
extern int RIexecop (
    RI_t *, RIop_t *, int, char *, char *, char *, char *, char *, char *
);
extern int RIloadimage (char *, RIop_t *);

extern int EMinit (char *);
extern int EMparsenode (char *, char *, char *, int, int *, int *, EMbb_t *);
extern int EMparseedge (
    char *, char *, char *, char *, char *, int, int *, int *, EMbb_t *
);

extern int IGRinit (char *);
extern IGRcc_t *IGRinsertcc (char *, char *);
extern IGRcc_t *IGRlinkcc2cc (IGRcc_t *, IGRcc_t *);
extern IGRcl_t *IGRinsertcl (char *, char *, short);
extern IGRrk_t *IGRinsertrk (char *, char *);
extern IGRnd_t *IGRinsertnd (char *, char *, short);
extern IGRnd_t *IGRfindnd (char *, char *);
extern IGRnd_t *IGRlinknd2cc (IGRnd_t *, IGRcc_t *);
extern IGRnd_t *IGRunlinknd2cc (IGRnd_t *, IGRcc_t *);
extern IGRnd_t *IGRlinknd2cl (IGRnd_t *, IGRcl_t *);
extern IGRnd_t *IGRunlinknd2cl (IGRnd_t *, IGRcl_t *);
extern IGRnd_t *IGRlinknd2rk (IGRnd_t *, IGRrk_t *);
extern IGRnd_t *IGRunlinknd2rk (IGRnd_t *, IGRrk_t *);
extern IGRed_t *IGRinserted (IGRnd_t *, IGRnd_t *);
extern int IGRflatten (void);
extern int IGRreset (char *);
extern int IGRlayout (char *);
extern int IGRnewgraph (IGRcc_t *, char *);
extern int IGRnewccgraph (Agraph_t *, IGRcc_t *, char *);
extern int IGRnewclgraph (Agraph_t *, IGRcl_t *, char *);
extern int IGRnewrkgraph (Agraph_t *, IGRrk_t *, char *);
extern int IGRnewnode (Agraph_t *, IGRnd_t *, char *);
extern int IGRnewedge (Agraph_t *, IGRed_t *, char *);
extern int IGRbegindraw (char *, int, RIop_t *, int, char *, char *);
extern int IGRenddraw (void);
extern int IGRdrawsgraph (Agraph_t *, RIop_t *, int, char *, char *);
extern int IGRdrawnode (
    Agnode_t *, RIop_t *, int, int, int, int, char *, char *
);
extern int IGRdrawedge (Agedge_t *, RIop_t *, int, char *, char *);

extern int IGDinit (int, char *);
extern IGDcc_t *IGDinsertcc (char *, char *);
extern IGDcl_t *IGDinsertcl (char *, char *, char *, char *, char *);
extern IGDnd_t *IGDinsertnd (char *, char *, short, char *);
extern IGDnd_t *IGDlinknd2cc (IGDnd_t *, IGDcc_t *);
extern IGDnd_t *IGDlinknd2cl (IGDnd_t *, IGDcl_t *);
extern int IGDflatten (void);
extern int IGDbegindraw (char *, int, int, char *, char *, char *, char *);
extern int IGDenddraw (void);
extern int IGDdrawnode (IGDnd_t *, char *, char *, int);

extern int IMPinit (char *, char *);
extern IMPnd_t *IMPinsertnd (char *, char *, short, char *);
extern IMPnd_t *IMPfindnd (char *, char *);
extern IMPed_t *IMPinserted (IMPnd_t *, IMPnd_t *, char *);
extern int IMPflatten (void);
extern int IMPbegindraw (char *, int, char *, char *, char *, char *);
extern int IMPenddraw (void);
extern int IMPdrawnode (IMPnd_t *, char *, char *);
extern int IMPdrawedge (IMPed_t *, char *, char *);

extern int IMPLLinit (char *);
extern IMPLLnd_t *IMPLLinsertnd (char *, char *, short, char *);
extern IMPLLnd_t *IMPLLfindnd (char *, char *);
extern IMPLLed_t *IMPLLinserted (IMPLLnd_t *, IMPLLnd_t *, char *);
extern int IMPLLflatten (void);
extern int IMPLLbegindraw (
    char *, int, char *gmattr, char *tlattr, char *tlurl, char *tlobj
);
extern int IMPLLenddraw (void);
extern int IMPLLdrawnode (IMPLLnd_t *, char *, char *, char *, int, int);
extern int IMPLLdrawedge (IMPLLed_t *, char *, char *, int);

extern int ITBinit (char *);
extern ITBnd_t *ITBinsertnd (char *, char *, short, char *);
extern ITBnd_t *ITBfindnd (char *, char *);
extern ITBed_t *ITBinserted (ITBnd_t *, ITBnd_t *, char *);
extern int ITBflatten (char *, char *);
extern int ITBsetupprint (char *, char *);
extern int ITBbeginnd (char *, int);
extern int ITBendnd (void);
extern int ITBprintnd (int, ITBnd_t *, char *, char *);
extern int ITBbegined (char *, int);
extern int ITBended (void);
extern int ITBprinted (int, ITBed_t *, char *, char *);

extern int AGDinit (int, int, char *);
extern AGDalarm_t *AGDinsert (
    char *, char *, char *, char *, char *, char *, char *, char *,
    int, int, int
);
extern int AGDflatten (void);
extern int AGDsetupdraw (char *, char *, char *, char *, char *);
extern int AGDfinishdraw (char *);
extern int AGDdrawgrid (
    char *, int, int, char *, char *, char *, char *, char *, char *
);

extern int ATBinit (int, char *, char *);
extern ATBnd_t *ATBinsertnd (
    char *, char *, short, int, int, int, char *, char *, char *
);
extern ATBed_t *ATBinserted (
    char *, char *, char *, char *, short, short, int, int, int, char *, char *,
    char *
);
extern int ATBflatten (char *, char *);
extern int ATBsetupprint (char *, char *);
extern int ATBbeginnd (char *, int);
extern int ATBendnd (char *);
extern int ATBprintnd (int, ATBnd_t *, char *);
extern int ATBbegined (char *, int);
extern int ATBended (char *);
extern int ATBprinted (int, ATBed_t *, char *);

extern int ASTinit (int, int);
extern ASTalarm_t *ASTinsert (
    char *, char *, char *, char *, char *, char *, char *, char *,
    int, int, int
);
extern int ASTflatten (void);
extern int ASTsetupprint (char *, char *, char *);
extern int ASTfinishprint (char *);
extern int ASTprintstyle (char *, int, int, char *);

extern int SCHinit (int, int, char *, char *);
extern SCHmetric_t *SCHinsert (
    char *, char *, char *, char *, char *, int, float, char *, char *
);
extern int SCHattachdata (SCHmetric_t *, char *, char *, int, float, int);
extern int SCHflatten (char *, char *, char *, char *);
extern int SCHsetupdraw (char *, char *, char *, char *);
extern int SCHdrawchart (
    char *, int, int, int, SCHmetric_t **, int,
    char *, char *, char *, char *, char *, char *, char *, char *
);

extern int STBinit (int, int, char *, char *);
extern STBmetric_t *STBinsert (
    char *, char *, char *, char *, char *, int, float, char *, char *, int
);
extern int STBattachdata (STBmetric_t *, char *, char *, int, float);
extern int STBattachattr (STBmetric_t *, char *, char *, char *);
extern int STBflatten (char *, char *, char *);
extern int STBsetupprint (char *, char *);
extern int STBprinttab (
    char *, int, int, int, STBmetric_t **, int, char *, char *
);

#undef extern
_END_EXTERNS_

#endif
