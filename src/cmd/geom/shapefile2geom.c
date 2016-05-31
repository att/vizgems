#include <ast.h>
#include <math.h>
#include <option.h>
#include <vmalloc.h>
#include <swift.h>
#include "geom.h"

static const char usage[] =
"[-1p1s1D?\n@(#)$Id: shapefile2geom (AT&T Labs Research) 2010-02-04 $\n]"
USAGE_LICENSE
"[+NAME?shapefile2geom - generate SWIFT geometry from shapefiles]"
"[+DESCRIPTION?\bshapefile2geom\b generates SWIFT geometry files from files"
" in the shapefile format."
" It can generate up to four types of geometry: points, vertical lines,"
" polygon outlines, and filled polygons."
"]"
"[100:i?specifies the prefix of the input files."
" The default is \bshapefile\b."
"]:[inputprefix]"
"[101:o?specifies the prefix for the output files."
" The default is \bgeom\b."
"]:[outputprefix]"
"[201:p?specifies the accuracy level for edge thining."
" This reduces the number of shape vertices in each edge."
" Shape vertex that are \aclose\a to the straight line between the edge"
" endpoints are eliminated."
" The argument to this option specifies the accuracy level when determining"
" closeness."
" This is a floating point number with the range: \b0-9\b."
" Higher numbers result in more shape edges qualifing for elimination."
" A value of \b-1\b (default) disables this processing."
"]:[level]"
"[303:cl?specifies the class name of the geometry files."
" The default is \bascii\b."
"]:[classname]"
"[306:bb?specifies that bounding box information should be printed out."
"]"
"[600:np?disables the generation of the points geometry."
"]"
"[601:nl?disables the generation of the vertical lines geometry."
"]"
"[602:nP?disables the generation of the polygon outlines geometry."
"]"
"[603:nt?disables the generation of the filled polygons geometry."
"]"
"[700:dbf?specifies the name of database field to use as the geometry id."
" The default is to use the first field, when the database file is present"
" and an ascending number when not."
"]:[fieldname]"
"[701:ddh?dumps the header of the database file (.dbf file)."
"]"
"[702:dda?dumps the header and contents of the database file (.dbf file)."
"]"
"[703:nodb?specifies that the database file should not be used."
"]"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"\n"
"[+SEE ALSO?\btiger\b(1)]"
;

#define CP1(dp, di, sp, si) ( \
    *((unsigned char *) (dp) + di) = *((unsigned char *) (sp) + si) \
)

#if SWIFT_LITTLEENDIAN == 1
#define BCP1(d, s) ( \
    CP1 (d, 0, s, 0) \
)
#define BCP2(d, s) ( \
    CP1 (d, 0, s, 1), CP1 (d, 1, s, 0) \
)
#define BCP4(d, s) ( \
    CP1 (d, 0, s, 3), CP1 (d, 1, s, 2), CP1 (d, 2, s, 1), CP1 (d, 3, s, 0) \
)
#define BCP8(d, s) ( \
    CP1 (d, 0, s, 7), CP1 (d, 1, s, 6), CP1 (d, 2, s, 5), CP1 (d, 3, s, 4), \
    CP1 (d, 4, s, 3), CP1 (d, 5, s, 2), CP1 (d, 6, s, 1), CP1 (d, 7, s, 0) \
)
#define LCP1(d, s) ( \
    CP1 (d, 0, s, 0) \
)
#define LCP2(d, s) ( \
    CP1 (d, 0, s, 0), CP1 (d, 1, s, 1) \
)
#define LCP4(d, s) ( \
    CP1 (d, 0, s, 0), CP1 (d, 1, s, 1), CP1 (d, 2, s, 2), CP1 (d, 3, s, 3) \
)
#define LCP8(d, s) ( \
    CP1 (d, 0, s, 0), CP1 (d, 1, s, 1), CP1 (d, 2, s, 2), CP1 (d, 3, s, 3), \
    CP1 (d, 4, s, 4), CP1 (d, 5, s, 5), CP1 (d, 6, s, 6), CP1 (d, 7, s, 7) \
)
#else
#define LCP1(d, s) ( \
    CP1 (d, 0, s, 0) \
)
#define LCP2(d, s) ( \
    CP1 (d, 0, s, 1), CP1 (d, 1, s, 0) \
)
#define LCP4(d, s) ( \
    CP1 (d, 0, s, 3), CP1 (d, 1, s, 2), CP1 (d, 2, s, 1), CP1 (d, 3, s, 0) \
)
#define LCP8(d, s) ( \
    CP1 (d, 0, s, 7), CP1 (d, 1, s, 6), CP1 (d, 2, s, 5), CP1 (d, 3, s, 4), \
    CP1 (d, 4, s, 3), CP1 (d, 5, s, 2), CP1 (d, 6, s, 1), CP1 (d, 7, s, 0) \
)
#define BCP1(d, s) ( \
    CP1 (d, 0, s, 0) \
)
#define BCP2(d, s) ( \
    CP1 (d, 0, s, 0), CP1 (d, 1, s, 1) \
)
#define BCP4(d, s) ( \
    CP1 (d, 0, s, 0), CP1 (d, 1, s, 1), CP1 (d, 2, s, 2), CP1 (d, 3, s, 3) \
)
#define BCP8(d, s) ( \
    CP1 (d, 0, s, 0), CP1 (d, 1, s, 1), CP1 (d, 2, s, 2), CP1 (d, 3, s, 3), \
    CP1 (d, 4, s, 4), CP1 (d, 5, s, 5), CP1 (d, 6, s, 6), CP1 (d, 7, s, 7) \
)
#endif

#ifndef MAXFLOAT
#ifdef HUGE
#define MAXFLOAT HUGE
#else
#define MAXFLOAT 3.40282347e+38F
#endif
#endif

#define TNULL         0
#define TPOINT        1
#define TPOLYLINE     3
#define TPOLYGON      5
#define TMULTIPOINT   8
#define TPOINTZ      11
#define TPOLYLINEZ   13
#define TPOLYGONZ    15
#define TMULTIPOINTZ 18
#define TPOINTM      21
#define TPOLYLINEM   23
#define TPOLYGONM    25
#define TMULTIPOINTM 28
#define TMULTIPATCH  31

#define MPTTRISTRIP  0
#define MPTTRIFAN    1
#define MPTOUTERRING 2
#define MPTINNERRING 3
#define MPTFIRSTRING 4
#define MPTRING      5

typedef struct shp_hdr_s {
    int fcode;
    int j1, j2, j3, j4, j5;
    int flen;
    int version;
    int stype;
    double bbx1, bby1, bbx2, bby2, bbz1, bbz2, bbm1, bbm2;
} shp_hdr_t;

static shp_hdr_t shphdr;
static Sfio_t *shpfp;
static char *shprecstr;
static int shpreclen;

typedef struct dbf_field_t {
    char *name;
    short type, off, len;
    char *value;
} dbf_field_t;

typedef struct dbf_hdr_t {
    int recn;
    short recsize, hdrsize;
    dbf_field_t *fields;
    int fieldn;
} dbf_hdr_t;

static dbf_hdr_t dbfhdr;
static Sfio_t *dbffp;
static int dbfnamefieldi;
static char *dbfrecstr;

#define ISCCW 1
#define ISCW  2
#define ISON  3

#define POINTINCR 100
#define INDINCR     1
#define FLAGINCR  100
#define TRIINCR    10
#define LABELINCR  10

typedef struct point_t {
    float x, y, z, tx, ty;
} point_t;

typedef struct tri_t {
    point_t a, b, c;
} tri_t;

typedef struct item_t {
    Dtlink_t link;
    /* begin key */
    char *name;
    /* end key */
    point_t *points;
    int pointm, pointn;
    int *flags;
    int flagm, flagn;
    int *inds;
    int indm, indn;
    tri_t *tris;
    int trim, trin;
    GEOMlabel_t *labels;
    int labelm, labeln;
    int tpointn;
} item_t;

static Dtdisc_t itemdisc = {
    sizeof (Dtlink_t), -1, 0, NULL, NULL, NULL, NULL, NULL, NULL
};

static Dt_t *itemdict;
static item_t **itemps;
static int itempn;

static double *ts;
static int tn;

static int shpreadhdr (void);
static int shploadrec (char *);
static int dbfreadhdr (void);
static int dbfdumphdr (void);
static int dbfdumprec (void);
static int dbffindfield (char *);
static char *dbfreadfield (void);

static int add2points (item_t *, double, double, double);
static int add2inds (item_t *, int);

static int simplifypoints (float digits);
static void recsimplify (item_t *, int, int, int, double *, double, int *);
static int gentris (void);
static int savepoints (char *, char *);
static int savelines (char *, char *);
static int savepolys (char *, char *);
static int savetris (char *, char *);

static int maxdist (point_t *, int, int, double *, double);
static double dist (point_t, point_t);
static int triangulate (item_t *, int, int);
static int rectriangulate (item_t *itemp, point_t *, int);
static int ccw (point_t, point_t, point_t);
static int isdiagonal (int, int, point_t *, int);
static int intersects (point_t, point_t, point_t, point_t);
static int between (point_t, point_t, point_t);

static tri_t *gettri (item_t *, point_t, point_t, point_t);

static int printbbox (void);

int main (int argc, char **argv) {
    int norun;
    char *iprefix, *oprefix, *classp;
    int bboxflag;
    int savemask;
    float digits;
    char *dbfindexfield;
    int dumpdbf, nodbf;
    char idbuf[100], *idstr;
    int reci, ret, extrarecs;
    item_t *itemp;
    int itemi;

    iprefix = "shapefile";
    oprefix = "geom";
    classp = "shapefile";
    bboxflag = 0;
    savemask = 15;
    digits = -1;
    dbfindexfield = NULL;
    dumpdbf = 0;
    nodbf = FALSE;
    norun = 0;
    for (;;) {
        switch (optget (argv, usage)) {
        case -100:
            iprefix = opt_info.arg;
            continue;
        case -101:
            oprefix = opt_info.arg;
            continue;
        case -201:
            if ((digits = atof (opt_info.arg)) < 0)
                digits = -1;
            continue;
        case -303:
            classp = opt_info.arg;
            continue;
        case -306:
            bboxflag = 1;
            continue;
        case -600:
            savemask &= 14;
            continue;
        case -601:
            savemask &= 13;
            continue;
        case -602:
            savemask &= 11;
            continue;
        case -603:
            savemask &= 7;
            continue;
        case -700:
            dbfindexfield = opt_info.arg;
            continue;
        case -701:
            dumpdbf = 1;
            continue;
        case -702:
            dumpdbf = 2;
            continue;
        case -703:
            nodbf = TRUE;
            continue;
        case -999:
            SUwarnlevel++;
            continue;
        case '?':
            SUusage (0, "shapefile2geom", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "shapefile2geom", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;

    if (!(itemdict = dtopen (&itemdisc, Dtset)))
        SUerror ("shapefile2geom", "cannot create item dictionary");

    if (dumpdbf > 0) {
        if (nodbf)
            return 0;

        if (!(dbffp = sfopen (NULL, sfprints ("%s.dbf", iprefix), "r")))
            SUerror ("shapefile2geom", "cannot open %s.dbf", iprefix);
        dbfreadhdr ();
        dbfdumphdr ();
        if (dumpdbf == 2)
            while (dbfdumprec () == 1)
                ;
        sfclose (dbffp);
        return 0;
    }

    if (!(shpfp = sfopen (NULL, sfprints ("%s.shp", iprefix), "r")))
        SUerror ("shapefile2geom", "cannot open %s.shp", iprefix);
    if (!nodbf && !(dbffp = sfopen (NULL, sfprints ("%s.dbf", iprefix), "r")))
        SUerror ("shapefile2geom", "cannot open %s.dbf", iprefix);

    if (shpreadhdr () == -1)
        SUerror ("shapefile2geom", "cannot read shp header");
    if (!nodbf && dbfreadhdr () == -1)
        SUerror ("shapefile2geom", "cannot read dbf header");
    if (!nodbf && dbfindexfield && dbffindfield (dbfindexfield) == -1)
        SUerror ("shapefile2geom", "cannot find dbf field %s", dbfindexfield);

    reci = 0;
    if (nodbf) {
        for ( ;; ) {
            sfsprintf (idbuf, 100, "%d", reci);
            if ((ret = shploadrec (idbuf)) == -1)
                SUerror ("shapefile2geom", "cannot read shp record %d", reci);
            if (ret == 0)
                break;
            reci++;
        }
    } else {
        while ((idstr = dbfreadfield ())) {
            if ((ret = shploadrec (idstr)) == -1)
                SUerror ("shapefile2geom", "cannot read shp record %d", reci);
            if (ret == 0) {
                SUwarning (0, "shapefile2geom", "EOF for shp record %d", reci);
                break;
            }
            reci++;
        }
        extrarecs = 0;
        for ( ;; ) {
            sfsprintf (idbuf, 100, "%d", reci);
            if ((ret = shploadrec (idbuf)) == -1)
                SUerror ("shapefile2geom", "cannot read shp record %d", reci);
            if (ret == 0)
                break;
            if (extrarecs == 0) {
                SUwarning (
                    0, "shapefile2geom", "first extra shp record %d", reci
                );
                extrarecs = 1;
            }
            reci++;
        }
        if (extrarecs == 1)
            SUwarning (0, "shapefile2geom", "last extra shp record %d", reci);
    }
    itempn = dtsize (itemdict);
    if (!(itemps = vmalloc (Vmheap, sizeof (item_t *) * itempn)))
        SUerror ("shapefile2geom", "cannot allocate itemps");
    for (
        itemi = 0, itemp = (item_t *) dtflatten (itemdict); itemp;
        itemp = (item_t *) dtlink (itemdict, itemp)
    )
        itemps[itemi++] = itemp;

    if (digits >= 0 && simplifypoints (digits) == -1)
        SUerror ("shapefile2geom", "simplifypoints failed");
    if (bboxflag)
        printbbox ();
    if ((savemask & 1) && savepoints (oprefix, classp) == -1)
        SUerror ("shapefile2geom", "save failed for points");
    if ((savemask & 2) && savelines (oprefix, classp) == -1)
        SUerror ("shapefile2geom", "save failed for lines");
    if ((savemask & 4) && savepolys (oprefix, classp) == -1)
        SUerror ("shapefile2geom", "save failed for polys");
    if ((savemask & 8) && gentris () == -1)
        SUerror ("shapefile2geom", "gentris failed");
    if ((savemask & 8) && savetris (oprefix, classp) == -1)
        SUerror ("shapefile2geom", "save failed for tris");

    return 0;
}

static int shpreadhdr (void) {
    char hdrstr[100];

    if (sfread (shpfp, hdrstr, 100) != 100) {
        SUwarning (0, "shpreadhdr", "cannot read file header");
        return -1;
    }

    BCP4 (&shphdr.fcode, hdrstr + 0);
    BCP4 (&shphdr.flen, hdrstr + 24);
    LCP4 (&shphdr.version, hdrstr + 28);
    LCP4 (&shphdr.stype, hdrstr + 32);
    LCP8 (&shphdr.bbx1, hdrstr + 36);
    LCP8 (&shphdr.bby1, hdrstr + 44);
    LCP8 (&shphdr.bbx2, hdrstr + 52);
    LCP8 (&shphdr.bby2, hdrstr + 60);
    LCP8 (&shphdr.bbz1, hdrstr + 68);
    LCP8 (&shphdr.bbz2, hdrstr + 76);
    LCP8 (&shphdr.bbm1, hdrstr + 84);
    LCP8 (&shphdr.bbm2, hdrstr + 92);

    return 0;
}

static int shploadrec (char *idstr) {
    char hdrstr[8];
    int recn, reclen, rectype, ret;
    item_t *itemmem, *itemp;
    double x, y, z;
    int partn, parti, pointn, pointi, pointj;

    if ((ret = sfread (shpfp, hdrstr, 8)) != 8) {
        if (ret == 0)
            return 0;
        SUwarning (0, "shploadrec", "cannot read record header");
        return -1;
    }
    BCP4 (&recn, hdrstr + 0);
    BCP4 (&reclen, hdrstr + 4);
    reclen *= 2;

    if (shpreclen < reclen) {
        if (!(shprecstr = vmresize (
            Vmheap, shprecstr, reclen, VM_RSMOVE
        ))) {
            SUwarning (0, "shploadrec", "cannot grow shprecstr");
            return -1;
        }
        shpreclen = reclen;
    }
    if (sfread (shpfp, shprecstr, reclen) != reclen) {
        SUwarning (0, "shploadrec", "cannot read record data");
        return -1;
    }

    if (!(itemmem = vmalloc (Vmheap, sizeof (item_t)))) {
        SUwarning (0, "shploadrec", "cannot allocate itemmem");
        return -1;
    }
    if (!(itemmem->name = vmstrdup (Vmheap, idstr))) {
        SUwarning (0, "shploadrec", "cannot copy idstr");
        return -1;
    }
    if (!(itemp = dtinsert (itemdict, itemmem)) || itemp != itemmem) {
        SUwarning (0, "shploadrec", "cannot insert itemp");
        return -1;
    }
    itemp->pointn = itemp->pointm = 0, itemp->points = NULL;
    itemp->flagn = itemp->flagm = 0, itemp->flags = NULL;
    itemp->indn = itemp->indm = 0, itemp->inds = NULL;
    itemp->trin = itemp->trim = 0, itemp->tris = NULL;
    itemp->labeln = itemp->labelm = 0, itemp->labels = NULL;
    itemp->tpointn = 0;

    LCP4 (&rectype, shprecstr + 0);
    switch (rectype) {
    case TNULL:
        break;
    case TPOINT:
    case TPOINTM:
    case TPOINTZ:
        LCP8 (&x, shprecstr + 4);
        LCP8 (&y, shprecstr + 12);
        if (rectype == TPOINTZ)
            LCP8 (&z, shprecstr + 20);
        else
            z = 0.0;
        if (add2points (itemp, x, y, z) == -1) {
            SUwarning (0, "shploadrec", "point - cannot add point");
            return -1;
        }
        if (add2inds (itemp, FALSE) == -1) {
            SUwarning (0, "shploadrec", "point - cannot add to ind");
            return -1;
        }
        if (add2inds (itemp, TRUE) == -1) {
            SUwarning (0, "shploadrec", "point - cannot grow ind");
            return -1;
        }
        break;
    case TMULTIPOINT:
    case TMULTIPOINTM:
    case TMULTIPOINTZ:
        LCP4 (&pointn, shprecstr + 36);
        for (pointi = 0; pointi < pointn; pointi++) {
            LCP8 (&x, shprecstr + 40 + pointi * 16);
            LCP8 (&y, shprecstr + 40 + pointi * 16 + 8);
            if (rectype == TMULTIPOINTZ)
                LCP8 (&z, shprecstr + 40 + pointn * 16 + 16 + pointi * 8);
            else
                z = 0.0;
            if (add2points (itemp, x, y, z) == -1) {
                SUwarning (0, "shploadrec", "mpoint - cannot add point");
                return -1;
            }
            if (add2inds (itemp, FALSE) == -1) {
                SUwarning (0, "shploadrec", "mpoint - cannot add to ind");
                return -1;
            }
            if (add2inds (itemp, TRUE) == -1) {
                SUwarning (0, "shploadrec", "mpoint - cannot grow ind");
                return -1;
            }
        }
        break;
    case TPOLYLINE:
    case TPOLYLINEM:
    case TPOLYLINEZ:
    case TPOLYGON:
    case TPOLYGONM:
    case TPOLYGONZ:
        LCP4 (&partn, shprecstr + 36);
        LCP4 (&pointn, shprecstr + 40);
        parti = 0;
        LCP4 (&pointj, shprecstr + 44 + parti * 4);
        for (pointi = 0; pointi < pointn; pointi++) {
            if (pointi == pointj) {
                if (pointj > 0 && add2inds (itemp, TRUE) == -1) {
                    SUwarning (0, "shploadrec", "poly - cannot grow ind");
                    return -1;
                }
                if (++parti < partn)
                    LCP4 (&pointj, shprecstr + 44 + parti * 4);
            }
            LCP8 (&x, shprecstr + 44 + partn * 4 + pointi * 16);
            LCP8 (&y, shprecstr + 44 + partn * 4 + pointi * 16 + 8);
            if (rectype == TPOLYGONZ || rectype == TPOLYLINEZ)
                LCP8 (
                    &z,
                    shprecstr + 44 + partn * 4 + pointn * 16 + 16 + pointi * 8
                );
            else
                z = 0.0;
            if (add2points (itemp, x, y, z) == -1) {
                SUwarning (0, "shploadrec", "poly - cannot add point");
                return -1;
            }
            if (add2inds (itemp, FALSE) == -1) {
                SUwarning (0, "shploadrec", "poly - cannot add to ind");
                return -1;
            }
        }
        if (partn > 0 && add2inds (itemp, TRUE) == -1) {
            SUwarning (0, "shploadrec", "poly - cannot grow ind");
            return -1;
        }
        break;
    case TMULTIPATCH:
        // this isn't exactly right for tristrips and trifans
        LCP8 (&partn, shprecstr + 36);
        LCP8 (&pointn, shprecstr + 40);
        parti = 0;
        LCP4 (&pointj, shprecstr + 44 + parti * 4);
        for (pointi = 0; pointi < pointn; pointi++) {
            if (pointi == pointj) {
                if (pointj > 0 && add2inds (itemp, TRUE) == -1) {
                    SUwarning (0, "shploadrec", "poly - cannot grow ind");
                    return -1;
                }
                if (++parti < partn)
                    LCP4 (&pointj, shprecstr + 44 + parti * 4);
            }
            LCP8 (&x, shprecstr + 44 + partn * 8 + pointi * 16);
            LCP8 (&y, shprecstr + 44 + partn * 8 + pointi * 16 + 8);
            LCP8 (
                &z,
                shprecstr + 44 + partn * 8 + pointn * 16 + 16 + pointi * 8
            );
            if (add2points (itemp, x, y, z) == -1) {
                SUwarning (0, "shploadrec", "poly - cannot add point");
                return -1;
            }
            if (add2inds (itemp, FALSE) == -1) {
                SUwarning (0, "shploadrec", "poly - cannot add to ind");
                return -1;
            }
        }
        if (partn > 0 && add2inds (itemp, TRUE) == -1) {
            SUwarning (0, "shploadrec", "poly - cannot grow ind");
            return -1;
        }
        break;
    }

    return 1;
}

static int dbfreadhdr (void) {
    char hdrstr[32], namestr[16];
    int fieldi, off;

    if (sfread (dbffp, hdrstr, 32) != 32) {
        SUwarning (1, "dbfreadhdr", "cannot read header");
        return -1;
    }
    LCP4 (&dbfhdr.recn, hdrstr + 4);
    LCP2 (&dbfhdr.hdrsize, hdrstr + 8);
    LCP2 (&dbfhdr.recsize, hdrstr + 10);
    dbfhdr.fieldn = dbfhdr.hdrsize / 32 - 1;

    if (!(dbfhdr.fields = vmalloc (
        Vmheap, sizeof (dbf_field_t) * dbfhdr.fieldn
    ))) {
        SUwarning (0, "dbfreadhdr", "cannot allocate header fields");
        return -1;
    }
    off = 0;
    for (fieldi = 0; fieldi < dbfhdr.fieldn; fieldi++) {
        if (sfread (dbffp, hdrstr, 32) != 32) {
            SUwarning (0, "dbfreadhdr", "cannot read header field %d", fieldi);
            return -1;
        }
        memcpy (namestr, hdrstr, 12);
        namestr[10] = 0;
        dbfhdr.fields[fieldi].name = strdup (namestr);
        dbfhdr.fields[fieldi].type = hdrstr[11];
        dbfhdr.fields[fieldi].off = off;
        dbfhdr.fields[fieldi].len = hdrstr[16];
        off += dbfhdr.fields[fieldi].len;
        if (!(dbfhdr.fields[fieldi].value = vmalloc (
            Vmheap, dbfhdr.fields[fieldi].len + 1
        ))) {
            SUwarning (0, "dbfreadhdr", "cannot allocate field %d", fieldi);
            return -1;
        }
    }
    if (sfseek (dbffp, dbfhdr.hdrsize, SEEK_SET) != dbfhdr.hdrsize) {
        SUwarning (0, "dbfreadhdr", "cannot seek to first data record");
        return -1;
    }

    if (!(dbfrecstr = vmalloc (Vmheap, dbfhdr.recsize + 1))) {
        SUwarning (0, "dbfreadhdr", "cannot allocate record string");
        return -1;
    }

    return 0;
}

static int dbfdumphdr (void) {
    int fieldi;

    sfprintf (
        sfstdout, "recn=%d\nrecsize=%d\nfieldn=%d\n",
        dbfhdr.recn, dbfhdr.recsize, dbfhdr.fieldn
    );
    for (fieldi = 0; fieldi < dbfhdr.fieldn; fieldi++)
        sfprintf (
            sfstdout, "field %d name=%s type=%d size=%d\n",
            fieldi, dbfhdr.fields[fieldi].name,
            dbfhdr.fields[fieldi].type, dbfhdr.fields[fieldi].len
        );

    return 0;
}

static int dbfdumprec (void) {
    int fieldi, ret;

    if ((ret = sfread (dbffp, dbfrecstr, dbfhdr.recsize)) != dbfhdr.recsize) {
        if (ret == 0)
            return 0;
        SUwarning (0, "dbfdumprec", "cannot read record");
        return -1;
    }

    sfprintf (sfstdout, "record");
    for (fieldi = 0; fieldi < dbfhdr.fieldn; fieldi++) {
        memcpy (
            dbfhdr.fields[fieldi].value,
            dbfrecstr + dbfhdr.fields[fieldi].off + 1, dbfhdr.fields[fieldi].len
        );
        dbfhdr.fields[fieldi].value[dbfhdr.fields[fieldi].len] = 0;
        sfprintf (sfstdout, "|%s", dbfhdr.fields[fieldi].value);
    }
    sfprintf (sfstdout, "\n");

    return 1;
}

static int dbffindfield (char *name) {
    int fieldi;

    dbfnamefieldi = -1;
    for (fieldi = 0; fieldi < dbfhdr.fieldn; fieldi++) {
        if (strcasecmp (name, dbfhdr.fields[fieldi].name) == 0) {
            dbfnamefieldi = fieldi;
            break;
        }
    }
    if (dbfnamefieldi == -1) {
        SUwarning (0, "dbffindfield", "cannot find field %s", name);
        return -1;
    }

    return 0;
}

static char *dbfreadfield (void) {
    int fieldi;

    if (sfread (dbffp, dbfrecstr, dbfhdr.recsize) != dbfhdr.recsize)
        return 0;

    for (fieldi = 0; fieldi < dbfhdr.fieldn; fieldi++) {
        memcpy (
            dbfhdr.fields[fieldi].value,
            dbfrecstr + dbfhdr.fields[fieldi].off + 1, dbfhdr.fields[fieldi].len
        );
        dbfhdr.fields[fieldi].value[dbfhdr.fields[fieldi].len] = 0;
    }

    return dbfhdr.fields[dbfnamefieldi].value;
}

static int add2points (item_t *itemp, double x, double y, double z) {
    if (itemp->pointn >= itemp->pointm) {
        if (!(itemp->points = vmresize (
            Vmheap, itemp->points,
            sizeof (point_t) * (itemp->pointm + POINTINCR), VM_RSCOPY
        ))) {
            SUwarning (0, "add2points", "cannot grow points");
            return -1;
        }
        itemp->pointm += POINTINCR;
    }
    itemp->points[itemp->pointn].x = x;
    itemp->points[itemp->pointn].y = y;
    itemp->points[itemp->pointn].z = 0.0;
    itemp->pointn++;

    return 0;
}

static int add2inds (item_t *itemp, int newind) {
    if (itemp->indn >= itemp->indm) {
        if (!(itemp->inds = vmresize (
            Vmheap, itemp->inds,
            sizeof (int) * (itemp->indm + INDINCR), VM_RSCOPY
        ))) {
            SUwarning (0, "add2inds", "cannot grow inds");
            return -1;
        }
        memset (itemp->inds + itemp->indm, 0, sizeof (int) * INDINCR);
        itemp->indm += INDINCR;
    }
    if (newind)
        itemp->indn++;
    else
        itemp->inds[itemp->indn]++;

    return 0;
}

static int simplifypoints (float digits) {
    item_t *itemp;
    int itempi;
    int pointi, pointj, pointk, pointn, indi;
    int count;
    double acc;

    SUmessage (1, "simplifypoints", "simplifying shape xys");
    if (digits < 0) {
        SUwarning (
            1, "simplifypoints", "negative number of digits (%d)", digits
        );
        return -1;
    }
    count = 0;
    acc = 0.0;
    if (digits > 0.0)
        acc = pow (10.0, (digits - 1.0)) / 1000000.0;
    for (itempi = 0; itempi < itempn; itempi++) {
        itemp = itemps[itempi];
        if (itemp->pointn == 0)
            continue;
        for (pointi = pointj = 0, indi = 0; indi < itemp->indn; indi++) {
            if (tn < itemp->inds[indi]) {
                if (!(ts = realloc (
                    ts, itemp->inds[indi] * sizeof (double)
                ))) {
                    SUwarning (1, "simplifypoints", "realloc failed");
                    return -1;
                }
                tn = itemp->inds[indi];
            }
            for (pointk = 0; pointk < itemp->inds[indi] - 1; pointk++)
                ts[pointk] = dist (
                    itemp->points[pointj + pointk],
                    itemp->points[pointj + pointk + 1]
                );
            pointn = 0;
            recsimplify (
                itemp, pointi, pointj, itemp->inds[indi], ts, acc, &pointn
            );
            if (itemp->flagn > 0)
                itemp->flags[pointi + pointn] = (
                    itemp->flags[pointj + itemp->inds[indi] - 1]
                );
            itemp->points[pointi + pointn++] = (
                itemp->points[pointj + itemp->inds[indi] - 1]
            );
            count += (itemp->inds[indi] - pointn);
            SUmessage (2, "simplifypoints", "eliminated %d points", count);
            pointj += itemp->inds[indi];
            itemp->pointn -= (itemp->inds[indi] - pointn);
            itemp->inds[indi] = pointn;
            pointi += pointn;
        }
    }
    SUmessage (1, "simplifypoints", "eliminated %d points", count);
    return 0;
}

static void recsimplify (
    item_t *itemp, int pointi, int pointj, int pointn, double *ts, double mind,
    int *pointnp
) {
    int pointm;

    if ((pointm = maxdist (itemp->points, pointj, pointn, ts, mind)) == -1) {
        if (itemp->flagn > 0)
            itemp->flags[pointi + *pointnp] = itemp->flags[pointj];
        itemp->points[pointi + *pointnp] = itemp->points[pointj], (*pointnp)++;
        return;
    }
    if (pointm > pointj)
        recsimplify (
            itemp, pointi, pointj, pointm - pointj + 1, ts, mind, pointnp
        );
    if (pointm - pointj + 1 < pointn)
        recsimplify (
            itemp, pointi, pointm, pointn - pointm + pointj, ts, mind, pointnp
        );
}

static int gentris (void) {
    item_t *itemp;
    int itempi;
    int indi, pointi, msgstep;

    if ((msgstep = itempn / 10) == 0)
        msgstep = 1;
    for (itempi = 0; itempi < itempn; itempi++) {
        itemp = itemps[itempi];
        if (itempi % msgstep == 0)
            SUmessage (
                1, "gentris", "triangulating item %d/%d", itempi, itempn
            );
        for (pointi = 0, indi = 0; indi < itemp->indn; indi++) {
            triangulate (itemp, pointi, itemp->inds[indi]);
            pointi += itemp->inds[indi];
        }
    }
    return 0;
}

static int savepoints (char *prefix, char *classp) {
    GEOM_t geom;
    char *fname;
    Sfio_t *fp;
    item_t *itemp;
    int itempi, itemn;
    int dictl;
    int pointi;
    int labeli, labeln;
    double x, y;

    for (
        itemn = 0, itempi = 0, dictl = 0, labeln = 0; itempi < itempn; itempi++
    ) {
        itemp = itemps[itempi];
        if (itemp->pointn == 0)
            continue;
        itemn++;
        dictl += strlen (itemp->name) + 1;
        labeln += itemp->labeln;
    }
    if (
        !(geom.items = malloc (sizeof (char *) * itemn)) ||
        !(geom.points = malloc (sizeof (GEOMpoint_t) * itemn)) ||
        !(geom.p2is = malloc (sizeof (int) * itemn))
    ) {
        SUwarning (1, "savepoints", "malloc failed");
        return -1;
    }
    geom.labels = NULL;
    geom.labeln = labeln;
    if (
        labeln > 0 && !(geom.labels = malloc (sizeof (GEOMlabel_t) * labeln))
    ) {
        SUwarning (1, "savepoints", "malloc failed");
        return -1;
    }
    geom.name = prefix;
    geom.type = GEOM_TYPE_POINTS;
    geom.class = classp;
    for (
        itempi = 0, geom.itemn = geom.pointn = geom.p2in = geom.labeln = 0;
        itempi < itempn; itempi++
    ) {
        itemp = itemps[itempi];
        if (itemp->pointn == 0)
            continue;
        geom.p2is[geom.p2in++] = geom.itemn;
        for (x = y = 0.0, pointi = 0; pointi < itemp->pointn; pointi++)
            x += itemp->points[pointi].x, y += itemp->points[pointi].y;
        x /= itemp->pointn, y /= itemp->pointn;
        geom.points[geom.pointn][0] = x;
        geom.points[geom.pointn][1] = y;
        geom.points[geom.pointn++][2] = 0.0;
        for (labeli = 0; labeli < itemp->labeln; labeli++) {
            itemp->labels[labeli].itemi = itempi;
            geom.labels[geom.labeln++] = itemp->labels[labeli];
        }
        geom.items[geom.itemn++] = (unsigned char *) itemp->name;
    }
    geom.dict = NULL;
    geom.dictlen = dictl;
    geom.keytype = GEOM_KEYTYPE_STRING;
    geom.keylen = -1;
    geom.colors = NULL;
    geom.colorn = -1;
    geom.inds = NULL;
    geom.indn = -1;
    geom.flags = NULL;
    geom.flagn = -1;
    geom.tpoints = NULL;
    geom.tpointn = -1;

    fname = strdup (sfprints ("%s_points.single", prefix));
    SUmessage (1, "savepoints", "saving file %s", fname);
    if (!(fp = sfopen (NULL, fname, "w"))) {
        SUwarning (1, "savepoints", "open failed for file %s", fname);
        return -1;
    }
    if (GEOMsave (fp, &geom) == -1) {
        SUwarning (1, "savepoints", "save failed for file %s", fname);
        return -1;
    }
    sfclose (fp);
    return 0;
}

static int savelines (char *prefix, char *classp) {
    GEOM_t geom;
    char *fname;
    Sfio_t *fp;
    item_t *itemp;
    int itempi, itemn;
    int dictl;
    int pointi;
    int labeli, labeln;
    double x, y;

    for (
        itemn = 0, itempi = 0, dictl = 0, labeln = 0; itempi < itempn; itempi++
    ) {
        itemp = itemps[itempi];
        if (itemp->pointn == 0)
            continue;
        itemn++;
        dictl += strlen (itemp->name) + 1;
        labeln += itemp->labeln;
    }
    if (
        !(geom.items = malloc (sizeof (char *) * itemn)) ||
        !(geom.points = malloc (sizeof (GEOMpoint_t) * 2 * itemn)) ||
        !(geom.p2is = malloc (sizeof (int) * 2 * itemn)) ||
        !(geom.flags = malloc (sizeof (int) * 2 * itemn))
    ) {
        SUwarning (1, "savelines", "malloc failed");
        return -1;
    }
    geom.labels = NULL;
    geom.labeln = labeln;
    if (
        labeln > 0 && !(geom.labels = malloc (sizeof (GEOMlabel_t) * labeln))
    ) {
        SUwarning (1, "savepoints", "malloc failed");
        return -1;
    }
    geom.name = prefix;
    geom.type = GEOM_TYPE_LINES;
    geom.class = classp;
    for (
        itempi = 0,
        geom.itemn = geom.pointn = geom.p2in = geom.flagn = geom.labeln = 0;
        itempi < itempn; itempi++
    ) {
        itemp = itemps[itempi];
        if (itemp->pointn == 0)
            continue;
        geom.p2is[geom.p2in++] = geom.itemn;
        geom.p2is[geom.p2in++] = geom.itemn;
        geom.flags[geom.flagn++] = 8;
        geom.flags[geom.flagn++] = 12;
        for (x = y = 0.0, pointi = 0; pointi < itemp->pointn; pointi++)
            x += itemp->points[pointi].x, y += itemp->points[pointi].y;
        x /= itemp->pointn, y /= itemp->pointn;
        geom.points[geom.pointn][0] = x;
        geom.points[geom.pointn][1] = y;
        geom.points[geom.pointn++][2] = 0.0;
        geom.points[geom.pointn][0] = x;
        geom.points[geom.pointn][1] = y;
        geom.points[geom.pointn++][2] = 1.0;
        for (labeli = 0; labeli < itemp->labeln; labeli++) {
            itemp->labels[labeli].itemi = itempi;
            geom.labels[geom.labeln++] = itemp->labels[labeli];
        }
        geom.items[geom.itemn++] = (unsigned char *) itemp->name;
    }
    geom.dict = NULL;
    geom.dictlen = dictl;
    geom.keytype = GEOM_KEYTYPE_STRING;
    geom.keylen = -1;
    geom.colors = NULL;
    geom.colorn = -1;
    geom.inds = NULL;
    geom.indn = -1;
    geom.tpoints = NULL;
    geom.tpointn = -1;

    fname = strdup (sfprints ("%s_lines.single", prefix));
    SUmessage (1, "savelines", "saving file %s", fname);
    if (!(fp = sfopen (NULL, fname, "w"))) {
        SUwarning (1, "savelines", "open failed for file %s", fname);
        return -1;
    }
    if (GEOMsave (fp, &geom) == -1) {
        SUwarning (1, "savelines", "save failed for file %s", fname);
        return -1;
    }
    sfclose (fp);
    return 0;
}

static int savepolys (char *prefix, char *classp) {
    GEOM_t geom;
    char *fname;
    Sfio_t *fp;
    item_t *itemp;
    int itempi, itemn;
    int dictl;
    int indi, indn;
    int pointi, pointn;
    int labeli, labeln;

    for (
        pointn = indn = itemn = 0, itempi = 0, dictl = 0, labeln = 0;
        itempi < itempn; itempi++
    ) {
        itemp = itemps[itempi];
        if (itemp->pointn == 0)
            continue;
        itemn++;
        pointn += itemps[itempi]->pointn;
        indn += itemps[itempi]->indn;
        dictl += strlen (itemp->name) + 1;
        labeln += itemp->labeln;
    }
    if (
        !(geom.items = malloc (sizeof (char *) * itemn)) ||
        !(geom.points = malloc (sizeof (GEOMpoint_t) * pointn)) ||
        !(geom.inds = malloc (sizeof (int) * indn)) ||
        !(geom.p2is = malloc (sizeof (int) * pointn))
    ) {
        SUwarning (1, "savepoints", "malloc failed");
        return -1;
    }
    geom.labels = NULL;
    geom.labeln = labeln;
    if (
        labeln > 0 && !(geom.labels = malloc (sizeof (GEOMlabel_t) * labeln))
    ) {
        SUwarning (1, "savepoints", "malloc failed");
        return -1;
    }
    geom.name = prefix;
    geom.type = GEOM_TYPE_LINESTRIPS;
    geom.class = classp;
    for (
        itempi = 0,
        geom.itemn = geom.pointn = geom.indn = geom.p2in = geom.labeln = 0;
        itempi < itempn; itempi++
    ) {
        itemp = itemps[itempi];
        if (itemp->pointn == 0)
            continue;
        for (indi = 0; indi < itemp->indn; indi++)
            geom.inds[geom.indn++] = itemp->inds[indi];
        for (pointi = 0; pointi < itemp->pointn; pointi++) {
            geom.p2is[geom.p2in++] = geom.itemn;
            geom.points[geom.pointn][0] = itemp->points[pointi].x;
            geom.points[geom.pointn][1] = itemp->points[pointi].y;
            geom.points[geom.pointn++][2] = 0.0;
        }
        for (labeli = 0; labeli < itemp->labeln; labeli++) {
            itemp->labels[labeli].itemi = itempi;
            geom.labels[geom.labeln++] = itemp->labels[labeli];
        }
        geom.items[geom.itemn++] = (unsigned char *) itemp->name;
    }
    geom.dict = NULL;
    geom.dictlen = dictl;
    geom.keytype = GEOM_KEYTYPE_STRING;
    geom.keylen = -1;
    geom.colors = NULL;
    geom.colorn = -1;
    geom.flags = NULL;
    geom.flagn = -1;
    geom.tpoints = NULL;
    geom.tpointn = -1;

    fname = strdup (sfprints ("%s_polys.single", prefix));
    SUmessage (1, "savepolys", "saving file %s", fname);
    if (!(fp = sfopen (NULL, fname, "w"))) {
        SUwarning (1, "savepolys", "open failed for file %s", fname);
        return -1;
    }
    if (GEOMsave (fp, &geom) == -1) {
        SUwarning (1, "savepolys", "save failed for file %s", fname);
        return -1;
    }
    sfclose (fp);
    return 0;
}

static int savetris (char *prefix, char *classp) {
    GEOM_t geom;
    char *fname;
    Sfio_t *fp;
    item_t *itemp;
    int itempi, itemn;
    int dictl;
    int pointn, havetpoints;
    int labeli, labeln;
    int trii;

    for (
        pointn = itemn = 0, itempi = 0, dictl = 0, labeln = 0, havetpoints = 0;
        itempi < itempn; itempi++
    ) {
        itemp = itemps[itempi];
        if (itemp->trin == 0)
            continue;
        itemn++;
        pointn += 3 * itemps[itempi]->trin;
        dictl += strlen (itemp->name) + 1;
        labeln += itemp->labeln;
        if (itemp->tpointn > 0)
            havetpoints = 1;
    }
    if (
        !(geom.items = malloc (sizeof (char *) * itemn)) ||
        !(geom.points = malloc (sizeof (GEOMpoint_t) * pointn)) ||
        !(geom.p2is = malloc (sizeof (int) * pointn))
    ) {
        SUwarning (1, "savetris", "malloc failed");
        return -1;
    }
    geom.labels = NULL;
    geom.labeln = labeln;
    if (
        labeln > 0 && !(geom.labels = malloc (sizeof (GEOMlabel_t) * labeln))
    ) {
        SUwarning (1, "savepoints", "malloc failed");
        return -1;
    }
    geom.tpoints = NULL;
    geom.tpointn = 0;
    if (havetpoints > 0) {
        if (!(geom.tpoints = malloc (sizeof (GEOMtpoint_t) * pointn))) {
            SUwarning (1, "savepoints", "malloc failed");
            return -1;
        }
        geom.tpointn = pointn;
    }
    geom.name = prefix;
    geom.type = GEOM_TYPE_TRIS;
    geom.class = classp;
    for (
        itempi = 0, geom.itemn = geom.pointn = geom.p2in = geom.labeln = 0;
        itempi < itempn; itempi++
    ) {
        itemp = itemps[itempi];
        if (itemp->trin == 0)
            continue;
        for (trii = 0; trii < itemp->trin; trii++) {
            geom.p2is[geom.p2in++] = geom.itemn;
            geom.p2is[geom.p2in++] = geom.itemn;
            geom.p2is[geom.p2in++] = geom.itemn;
            if (havetpoints) {
                geom.tpoints[geom.pointn][0] = itemp->tris[trii].a.tx;
                geom.tpoints[geom.pointn][1] = itemp->tris[trii].a.ty;
            }
            geom.points[geom.pointn][0] = itemp->tris[trii].a.x;
            geom.points[geom.pointn][1] = itemp->tris[trii].a.y;
            geom.points[geom.pointn++][2] = 0.0;
            if (havetpoints) {
                geom.tpoints[geom.pointn][0] = itemp->tris[trii].b.tx;
                geom.tpoints[geom.pointn][1] = itemp->tris[trii].b.ty;
            }
            geom.points[geom.pointn][0] = itemp->tris[trii].b.x;
            geom.points[geom.pointn][1] = itemp->tris[trii].b.y;
            geom.points[geom.pointn++][2] = 0.0;
            if (havetpoints) {
                geom.tpoints[geom.pointn][0] = itemp->tris[trii].c.tx;
                geom.tpoints[geom.pointn][1] = itemp->tris[trii].c.ty;
            }
            geom.points[geom.pointn][0] = itemp->tris[trii].c.x;
            geom.points[geom.pointn][1] = itemp->tris[trii].c.y;
            geom.points[geom.pointn++][2] = 0.0;
        }
        for (labeli = 0; labeli < itemp->labeln; labeli++) {
            itemp->labels[labeli].itemi = itempi;
            geom.labels[geom.labeln++] = itemp->labels[labeli];
        }
        geom.items[geom.itemn++] = (unsigned char *) itemp->name;
    }
    geom.dict = NULL;
    geom.dictlen = dictl;
    geom.keytype = GEOM_KEYTYPE_STRING;
    geom.keylen = -1;
    geom.colors = NULL;
    geom.colorn = -1;
    geom.inds = NULL;
    geom.indn = -1;
    geom.flags = NULL;
    geom.flagn = -1;

    fname = strdup (sfprints ("%s_tris.single", prefix));
    SUmessage (1, "savetris", "saving file %s", fname);
    if (!(fp = sfopen (NULL, fname, "w"))) {
        SUwarning (1, "savetris", "open failed for file %s", fname);
        return -1;
    }
    if (GEOMsave (fp, &geom) == -1) {
        SUwarning (1, "savetris", "save failed for file %s", fname);
        return -1;
    }
    sfclose (fp);
    return 0;
}

/* co-linear test utilities */

static int maxdist (
    point_t *points, int pointj, int pointn, double *ts, double mind
) {
    double totl, l, t, dx, dy, d;
    point_t pointt;
    int pointi, mini;

    if (pointn < 2)
        return -1;
    dx = points[pointj + pointn - 1].x - points[pointj].x;
    dy = points[pointj + pointn - 1].y - points[pointj].y;
    for (totl = 0.0, pointi = 0; pointi < pointn - 1; pointi++)
        totl += ts[pointj + pointi];
    mini = -1;
    for (l = 0.0, d = 0.0, pointi = 0; pointi < pointn - 1; pointi++) {
        l += ts[pointj + pointi];
        t = l / totl;
        pointt.x = points[pointj].x + dx * t;
        pointt.y = points[pointj].y + dy * t;
        if ((d = dist (points[pointj + pointi + 1], pointt)) > mind)
            mind = d, mini = pointj + pointi + 1;
    }
    return mini;
}

static double dist (point_t pointa, point_t pointb) {
    double ax, ay, bx, by;

    ax = pointa.x, ay = pointa.y;
    bx = pointb.x, by = pointb.y;
    return sqrt (
        (bx - ax) * (bx - ax) + (by - ay) * (by - ay)
    );
}

/* triangulation functions */

static int triangulate (item_t *itemp, int pointf, int pointn) {
    point_t *points;
    int pointi, pointj, mini;
    double minx;
    point_t p1, p2, p3;

    points = &itemp->points[pointf];
    /* make sure polygon is CCW */
    for (pointi = 0, pointj = 0; pointj < pointn; pointi++) {
        points[pointi] = points[pointj++];
        while (pointj < pointn && ccw (
            points[pointi], points[pointj], points[(pointj + 1) % pointn]
        ) == ISON)
            pointj++;
    }
    pointn = pointi;
    for (pointi = 0, minx = MAXFLOAT, mini = -1; pointi < pointn; pointi++)
        if (minx > points[pointi].x)
            minx = points[pointi].x, mini = pointi;
    p2 = points[mini];
    p1 = points[((mini == 0) ? pointn - 1: mini - 1)];
    p3 = points[((mini == pointn - 1) ? 0 : mini + 1)];
#if 0
    if (
        ((p1.x == p2.x && p2.x == p3.x) && (p3.y > p2.y)) ||
        ccw (p1, p2, p3) != ISCCW
    ) {
#else
    if (
        ((p1.x == p2.x && p2.x == p3.x) && (p3.y > p2.y)) ||
        ccw (p1, p2, p3) == ISCW
    ) {
#endif
        for (pointi = pointn - 1; pointi >= 0; pointi--) {
            if ((pointj = pointn - pointi - 1) >= pointi)
                break;
            p1 = points[pointi];
            points[pointi] = points[pointj];
            points[pointj] = p1;
        }
    }
    rectriangulate (itemp, points, pointn);
    return 0;
}

static int rectriangulate (item_t *itemp, point_t *pp, int pn) {
    int i, ip1, ip2;
    int ccwres;

again:
    if (pn > 3) {
        for (i = 0; i < pn; i++) {
            ip1 = (i + 1) % pn;
            ip2 = (i + 2) % pn;
            if ((ccwres = ccw (pp[i], pp[ip1], pp[ip2])) == ISCW)
                continue;
            if (ccwres == ISON) {
                for (i = ip1; i < pn - 1; i++)
                    pp[i] = pp[i + 1];
                pn--;
                goto again;
            } else if (isdiagonal (i, ip2, pp, pn)) {
                if (!gettri (itemp, pp[i], pp[ip1], pp[ip2])) {
                    SUwarning (1, "rectriangulate", "gettri failed (1)");
                    return -1;
                }
                for (i = ip1; i < pn - 1; i++)
                    pp[i] = pp[i + 1];
                rectriangulate (itemp, pp, pn - 1);
                return 0;
            }
        }
        SUwarning (0, "rectriangulate", "failed for %d points", pn);
        for (i = 0; i < pn; i++)
            SUwarning (0, "rectriangulate", "%d %f %f", i, pp[i].x, pp[i].y);
    } else if (pn == 3) {
        if (!gettri (itemp, pp[0], pp[1], pp[2]) == -1) {
            SUwarning (1, "rectriangulate", "gettri failed (2)");
            return -1;
        }
    }
    return 0;
}

static int ccw (point_t p1, point_t p2, point_t p3) {
    double d;

    d = ((p1.y - p2.y) * (p3.x - p2.x)) - ((p3.y - p2.y) * (p1.x - p2.x));
    return (d > 0) ? ISCCW : ((d < 0) ? ISCW : ISON);
}

/* check if (i, i + 2) is a diagonal */
static int isdiagonal (int i, int ip2, point_t *pp, int pn) {
    int ip1, im1, j, jp1;

    /* neighborhood test */
    ip1 = (i + 1) % pn;
    im1 = (i + pn - 1) % pn;
    if (ccw (pp[im1], pp[i], pp[ip1]) == ISCCW)
        if (ccw (pp[i], pp[ip2], pp[im1]) != ISCCW)
            return FALSE;

    /* check against all other edges */
    for (j = 0; j < pn; j++) {
        jp1 = (j + 1) % pn;
        if (!((j == i) || (jp1 == i) || (j == ip2) || (jp1 == ip2)))
            if (intersects (pp[i], pp[ip2], pp[j], pp[jp1]))
                return FALSE;
    }
    return TRUE;
}

/* line to line intersection */
static int intersects (point_t pa, point_t pb, point_t pc, point_t pd) {
    int ccw1, ccw2, ccw3, ccw4;

    if (
        ccw (pa, pb, pc) == ISON || ccw (pa, pb, pd) == ISON ||
        ccw (pc, pd, pa) == ISON || ccw (pc, pd, pb) == ISON
    ) {
        if (
            between (pa, pb, pc) || between (pa, pb, pd) ||
            between (pc, pd, pa) || between (pc, pd, pb)
        )
            return TRUE;
    } else {
        ccw1 = (ccw (pa, pb, pc) == ISCCW) ? 1 : 0;
        ccw2 = (ccw (pa, pb, pd) == ISCCW) ? 1 : 0;
        ccw3 = (ccw (pc, pd, pa) == ISCCW) ? 1 : 0;
        ccw4 = (ccw (pc, pd, pb) == ISCCW) ? 1 : 0;
        return (ccw1 ^ ccw2) && (ccw3 ^ ccw4);
    }
    return FALSE;
}

static int between (point_t pa, point_t pb, point_t pc) {
    point_t pba, pca;

    pba.x = pb.x - pa.x, pba.y = pb.y - pa.y;
    pca.x = pc.x - pa.x, pca.y = pc.y - pa.y;
    if (ccw (pa, pb, pc) != ISON)
        return FALSE;
    return (
        pca.x * pba.x + pca.y * pba.y >= 0) &&
        (pca.x * pca.x + pca.y * pca.y <= pba.x * pba.x + pba.y * pba.y
    );
}

/* allocation functions */

static tri_t *gettri (item_t *itemp, point_t a, point_t b, point_t c) {
    tri_t *trip;

    if (itemp->trin == itemp->trim) {
        if (!itemp->tris) {
            if (!(itemp->tris = malloc (sizeof (tri_t) * TRIINCR))) {
                SUwarning (1, "gettri", "malloc failed");
                return NULL;
            }
        } else {
            if (!(itemp->tris = realloc (
                itemp->tris, sizeof (tri_t) * (itemp->trim + TRIINCR)
            ))) {
                SUwarning (1, "gettri", "realloc failed");
                return NULL;
            }
        }
        itemp->trim += TRIINCR;
    }
    trip = &itemp->tris[itemp->trin++];
    if (ccw (a, b, c) == ISCCW)
        trip->a = a, trip->b = b, trip->c = c;
    else
        trip->a = c, trip->b = b, trip->c = b;
    return trip;
}

static int printbbox (void) {
    point_t minp, maxp, p;
    item_t *itemp;
    int itempi;
    int pointi;

    minp.x = minp.y = 999999;
    maxp.x = maxp.y = -999999;
    for (itempi = 0; itempi < itempn; itempi++) {
        itemp = itemps[itempi];
        if (itemp->pointn == 0)
            continue;
        for (pointi = 0; pointi < itemp->pointn; pointi++) {
            p = itemp->points[pointi];
            if (minp.x > p.x)
                minp.x = p.x;
            if (minp.y > p.y)
                minp.y = p.y;
            if (maxp.x < p.x)
                maxp.x = p.x;
            if (maxp.y < p.y)
                maxp.y = p.y;
        }
    }
    sfprintf (
        sfstdout, "bbox: %f %f 0 %f %f 0\n", minp.x, minp.y, maxp.x, maxp.y
    );
    return 0;
}
