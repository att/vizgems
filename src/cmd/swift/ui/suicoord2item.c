#pragma prototyped

#include <ast.h>
#include <option.h>
#include <swift.h>
#include <geom.h>

static const char usage[] =
"[-1p1s1D?\n@(#)$Id: suicoord2item (AT&T Labs Research) 2001-09-24 $\n]"
USAGE_LICENSE
"[+NAME?suicoord2item - find geometry items inside a bounding box]"
"[+DESCRIPTION?\bsuicoord2item\b reads a SWIFT geometry file and outputs all"
" the items in that file whose coordinates are inside the specified bounding"
" box."
"]"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"\n"
"\nclass x1 y1 z1 x2 y2 z2 file\n"
"\n"
"[+SEE ALSO?\bgeomprint\b(1)]"
;

#define POINTINCUBE(pp, min, max) ( \
    ((*pp)[0] >= min[0] && (*pp)[0] <= max[0]) && \
    ((*pp)[1] >= min[1] && (*pp)[1] <= max[1]) && \
    ((*pp)[2] >= min[2] && (*pp)[2] <= max[2]) \
)

static void *alloc (void *, size_t, int);

int main (int argc, char **argv) {
    int norun;
    char *classp, *fname;
    GEOMpoint_t min, max;
    float t;
    Sfio_t *fp;
    GEOM_t *geomp;
    GEOMpoint_t *pointp;
    int pointi, itemi;

    norun = 0;
    for (;;) {
        switch (optget (argv, usage)) {
        case -999:
            SUwarnlevel++;
            continue;
        case '?':
            SUusage (0, "suicoord2item", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "suicoord2item", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;
    if (argc < 8)
        SUerror ("suicoord2item", "usage: %s", optusage (NULL));

    classp = argv[0];
    min[0] = atof (argv[1]);
    min[1] = atof (argv[2]);
    min[2] = atof (argv[3]);
    max[0] = atof (argv[4]);
    max[1] = atof (argv[5]);
    max[2] = atof (argv[6]);
    fname = argv[7];
    if (!(fp = sfopen (NULL, fname, "r")))
        SUerror ("suicoord2item", "cannot allocate map space");
    if (!(geomp = GEOMload (fp, alloc)))
        SUerror ("suicoord2item", "cannot allocate map space");
    sfclose (fp);
    if (classp[0] && strcmp (geomp->class, classp) != 0)
        exit (0);
    if (min[0] > max[0])
        t = min[0], min[0] = max[0], max[0] = t;
    if (min[1] > max[1])
        t = min[1], min[1] = max[1], max[1] = t;
    if (min[2] > max[2])
        t = min[2], min[2] = max[2], max[2] = t;
    for (pointi = 0; pointi < geomp->pointn; pointi++) {
        pointp = &geomp->points[pointi];
        if (POINTINCUBE (pointp, min, max)) {
            itemi = geomp->p2is[pointi];
            if (geomp->items[itemi]) {
                switch (geomp->keytype) {
                case GEOM_KEYTYPE_CHAR:
                    sfprintf (
                        sfstdout, "%c\n", * (char *) &geomp->items[itemi]
                    );
                    break;
                case GEOM_KEYTYPE_SHORT:
                    sfprintf (
                        sfstdout, "%d\n", * (short *) &geomp->items[itemi]
                    );
                    break;
                case GEOM_KEYTYPE_INT:
                    sfprintf (
                        sfstdout, "%d\n", * (int *) &geomp->items[itemi]
                    );
                    break;
                case GEOM_KEYTYPE_LLONG:
                    sfprintf (
                        sfstdout, "%lld\n", * (long long *) &geomp->items[itemi]
                    );
                    break;
                case GEOM_KEYTYPE_FLOAT:
                    sfprintf (
                        sfstdout, "%f\n", * (float *) &geomp->items[itemi]
                    );
                    break;
                case GEOM_KEYTYPE_DOUBLE:
                    sfprintf (
                        sfstdout, "%f\n", * (double *) &geomp->items[itemi]
                    );
                    break;
                case GEOM_KEYTYPE_STRING:
                    sfprintf (sfstdout, "%s\n", geomp->items[itemi]);
                    break;
                }
                geomp->items[itemi] = NULL;
            }
        }
    }
    return 0;
}

static void *alloc (void *oldp, size_t itemsize, int n) {
    void *p;

    if (!oldp)
        p = malloc (itemsize * n);
    else
        p = realloc (oldp, itemsize * n);
    if (!p) {
        SUwarning (
            1, "alloc", "alloc failed for (%x, %d, %d)", oldp, itemsize, n
        );
        return NULL;
    }
    return p;
}
