#pragma prototyped

#include <ast.h>
#include <option.h>
#include <tok.h>
#include <cdt.h>
#include <math.h>
#include <ctype.h>
#include <swift.h>

static const char usage[] =
"[-1p1s1D?\n@(#)$Id: cnvnpanxx (AT&T Labs Research) 2001-09-24 $\n]"
USAGE_LICENSE
"[+NAME?cnvnpanxx - convert NPANXX data to format for ascii2geom]"
"[+DESCRIPTION?\bcnvnpanxx\b uses an NPANXX coordinate file to generate"
" geometry in the format used by \bascii2geom\b."
"]"
"[100:i?specifies the input file."
" The default is \bnpanxx\b."
"]:[inputfile]"
"[101:o?specifies the output directory."
" The default is \b.\b."
"]:[outputdir]"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"\n"
"[+SEE ALSO?\btiger\b(1)]"
;

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

typedef struct npa_t {
    Dtlink_t link;
    /* begin key */
    int npa;
    /* end key */
    int npai;
} npa_t;

#define NPAINCR 1000

typedef struct npanxx_t {
    Dtlink_t link;
    /* begin key */
    int npanxx;
    /* end key */
    int npanxxi;
    char state[3];
} npanxx_t;

#define NPANXXINCR 100000

typedef struct loc_t {
    Dtlink_t link;
    /* begin key */
    ushort v, h;
    /* end key */
    int loci;
    int x, y;
    npanxx_t **npanxxs;
    int npanxxn;
} loc_t;

#define LOCINCR 100000

static Dtdisc_t npadisc = {
    sizeof (Dtlink_t), sizeof (int), 0, NULL, NULL, NULL, NULL, NULL, NULL
};

static Dtdisc_t npanxxdisc = {
    sizeof (Dtlink_t), sizeof (int), 0, NULL, NULL, NULL, NULL, NULL, NULL
};

static Dtdisc_t locdisc = {
    sizeof (Dtlink_t), 2 * sizeof (short), 0, NULL, NULL, NULL, NULL, NULL, NULL
};

static Dt_t *npadict, *npanxxdict, *locdict;

static npa_t **npas;
static int npam, npan;

static npanxx_t **npanxxs;
static int npanxxm, npanxxn;

static loc_t **locs;
static int locm, locn;

static int loadnpanxx (char *);
static int savelocmap (char *);
static int savenpacoords (char *);
static int saveloccoords (char *);
static int savelocstates (char *);
static int additem (char *, char *, char *, char *);
static void calcll (int, int, int *, int *);

int main (int argc, char **argv) {
    int norun;
    char *npanxxfile, *outdir;

    npanxxfile = "npanxx";
    outdir = ".";
    norun = 0;
    for (;;) {
        switch (optget (argv, usage)) {
        case -100:
            npanxxfile = opt_info.arg;
            continue;
        case -101:
            outdir = opt_info.arg;
            continue;
        case -999:
            SUwarnlevel++;
            continue;
        case '?':
            SUusage (0, "cnvlki", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "cnvlki", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;

    if (!(locs = malloc (sizeof (loc_t *) * LOCINCR)))
        SUerror ("cnvnpanxx", "malloc failed for locs");
    locm = LOCINCR;
    locn = 0;
    if (!(npanxxs = malloc (sizeof (npanxx_t *) * NPANXXINCR)))
        SUerror ("cnvnpanxx", "malloc failed for npanxxs");
    npanxxm = NPANXXINCR;
    npanxxn = 0;
    if (!(npas = malloc (sizeof (npa_t) * NPAINCR)))
        SUerror ("cnvnpanxx", "malloc failed for npas");
    npam = NPAINCR;
    npan = 0;
    if (!(npadict = dtopen (&npadisc, Dtset))) {
        SUerror ("cnvnpanxx", "dtopen failed for npadict");
        return -1;
    }
    if (!(npanxxdict = dtopen (&npanxxdisc, Dtset))) {
        SUerror ("cnvnpanxx", "dtopen failed for npanxxdict");
        return -1;
    }
    if (!(locdict = dtopen (&locdisc, Dtset))) {
        SUerror ("cnvnpanxx", "dtopen failed for locdict");
        return -1;
    }
    if (loadnpanxx (npanxxfile) == -1)
        SUerror ("cnvnpanxx", "loadnpanxx failed");
    if (savelocmap (outdir) == -1)
        SUerror ("cnvnpanxx", "savelocorder failed");
    if (savenpacoords (outdir) == -1)
        SUerror ("cnvnpanxx", "savenpacoords failed");
    if (saveloccoords (outdir) == -1)
        SUerror ("cnvnpanxx", "saveloccoords failed");
    if (savelocstates (outdir) == -1)
        SUerror ("cnvnpanxx", "savelocstates failed");
    return 0;
}

static int loadnpanxx (char *file) {
    Sfio_t *fp;
    char *s, *line, *avs[10];

    SUmessage (1, "loadnpanxx", "loading file %s", file);
    if (!(fp = sfopen (NULL, file, "r"))) {
        SUwarning (1, "loadnpanxx", "open failed for file %s", file);
        return -1;
    }
    while ((line = sfgetr (fp, '\n', 1))) {
        if (line[strlen (line) - 1] == '\r')
            line[strlen (line) - 1] = 0;
        if (tokscan (
            line, &s, "%s:%s:%s:%s\n", &avs[0], &avs[1], &avs[2], &avs[3]
        ) != 4) {
            SUwarning (1, "loadnpanxx", "bad line in file %s, %s", file, line);
            continue;
        }
        if (additem (avs[0], avs[1], avs[2], avs[3]) == -1) {
            SUwarning (1, "loadnpanxx", "additem failed");
            return -1;
        }
    }
    sfclose (fp);
    return 0;
}

static int savelocmap (char *dir) {
    Sfio_t *fp;
    loc_t *locp;
    int loci;
    int npanxxi;

    SUmessage (1, "savelocmap", "saving file %s/npanxxloc.map", dir);
    if (!(fp = sfopen (NULL, sfprints ("%s/npanxx2loc.map", dir), "w"))) {
        SUwarning (
            1, "savelocmap", "open failed for %s/npanxxloc.map", dir
        );
        return -1;
    }
    sfsetbuf (fp, NULL, 1048576);
    for (loci = 0; loci < locn; loci++) {
        locp = locs[loci];
        for (npanxxi = 0; npanxxi < locp->npanxxn; npanxxi++)
            sfprintf (fp, "%06d %d\n", locp->npanxxs[npanxxi]->npanxx, loci);
    }
    sfclose (fp);
    return 0;
}

static double xs[1000], ys[1000];
static int ns[1000];

static int savenpacoords (char *dir) {
    Sfio_t *fp;
    loc_t *locp;
    int loci;
    int npanxxi;
    npa_t *npap;
    int npai, npa;

    SUmessage (1, "savenpacoords", "saving file %s/npa.coords", dir);
    if (!(fp = sfopen (NULL, sfprints ("%s/npa.coords", dir), "w"))) {
        SUwarning (1, "savenpacoords", "open failed for %s/npa.coords", dir);
        return -1;
    }
    sfsetbuf (fp, NULL, 1048576);
    for (loci = 0; loci < locn; loci++) {
        locp = locs[loci];
        for (npanxxi = 0; npanxxi < locp->npanxxn; npanxxi++) {
            npa = locp->npanxxs[npanxxi]->npanxx / 1000;
            xs[npa] += locp->x, ys[npa] += locp->y, ns[npa]++;
        }
    }
    for (npai = 0; npai < npan; npai++) {
        npap = npas[npai];
        if (ns[npap->npa] > 0)
            sfprintf (
                fp, "%03d %d %d\n", npap->npa,
                (int) (xs[npap->npa] / (double) ns[npap->npa]),
                (int) (ys[npap->npa] / (double) ns[npap->npa])
            );
    }
    sfclose (fp);
    return 0;
}

static int saveloccoords (char *dir) {
    Sfio_t *fp;
    loc_t *locp;
    int loci;

    SUmessage (1, "saveloccoords", "saving file %s/npanxxloc.coords", dir);
    if (!(fp = sfopen (NULL, sfprints ("%s/npanxxloc.coords", dir), "w"))) {
        SUwarning (
            1, "saveloccoords", "open failed for %s/npanxxloc.coords", dir
        );
        return -1;
    }
    sfsetbuf (fp, NULL, 1048576);
    for (loci = 0; loci < locn; loci++) {
        locp = locs[loci];
        sfprintf (fp, "%d \"%d %d\" \"\"\n", loci, locp->x, locp->y);
    }
    sfclose (fp);
    return 0;
}

static int savelocstates (char *dir) {
    Sfio_t *fp;
    loc_t *locp;
    int loci;

    SUmessage (1, "savelocstates", "saving file %s/npanxxloc.states", dir);
    if (!(fp = sfopen (NULL, sfprints ("%s/npanxxloc.states", dir), "w"))) {
        SUwarning (
            1, "savelocstates", "open failed for %s/npanxxloc.states", dir
        );
        return -1;
    }
    sfsetbuf (fp, NULL, 1048576);
    for (loci = 0; loci < locn; loci++) {
        locp = locs[loci];
        sfprintf (fp, "%d %s\n", loci, locp->npanxxs[0]->state);
    }
    sfclose (fp);
    return 0;
}

static int additem (char *npanxx, char *clli, char *v, char *h) {
    npanxx_t *npanxxmem, *npanxxp;
    npa_t *npamem, *npap;
    loc_t *locmem, *locp;
    int vn, hn;

    if ((vn = atoi (v)) == 0 || (hn = atoi (h)) == 0) {
        SUwarning (
            1, "additem", "missing v & h coords for npanxx", "%s", npanxx
        );
        return 1;
    }
    if (strlen (npanxx) < 6) {
        SUwarning (1, "additem", "bad npanxx", "%s", npanxx);
        return 1;
    }
    if (strlen (clli) < 6) {
        SUwarning (1, "additem", "missing clli for npanxx", "%s", clli);
        return 1;
    }

    /* add to npanxx dict */
    if (!(npanxxmem = malloc (sizeof (npanxx_t)))) {
        SUwarning (1, "additem", "malloc failed npanxxmem");
        return -1;
    }
    npanxxmem->npanxx = atoi (npanxx);
    if (!(npanxxp = dtinsert (npanxxdict, npanxxmem))) {
        SUwarning (1, "additem", "dtinsert failed for %06d", npanxxmem->npanxx);
        return -1;
    } else  if (npanxxp != npanxxmem) {
        SUwarning (1, "additem", "duplicate entry for %06d", npanxxmem->npanxx);
        free (npanxxmem);
        return 1;
    } else {
        if (npanxxn == npanxxm) {
            if (!(npanxxs = realloc (
                npanxxs, sizeof (npanxx_t *) * (npanxxm + NPANXXINCR)
            ))) {
                SUwarning (1, "additem", "realloc failed for npanxxs");
                return -1;
            }
            npanxxm += NPANXXINCR;
        }
        npanxxs[npanxxn] = npanxxp;
        npanxxp->state[0] = tolower (clli[4]);
        npanxxp->state[1] = tolower (clli[5]);
        npanxxp->state[2] = 0;
        npanxxp->npanxxi = npanxxn++;
    }

    /* add to npa dict */
    if (!(npamem = malloc (sizeof (npa_t)))) {
        SUwarning (1, "additem", "malloc failed for npamem");
        return -1;
    }
    npamem->npa = atoi (npanxx) / 1000;
    if (!(npap = dtinsert (npadict, npamem))) {
        SUwarning (1, "additem", "dtinsert failed for %03d", npamem->npa);
        return -1;
    } else  if (npap != npamem) {
        free (npamem);
    } else {
        if (npan == npam) {
            if (!(npas = realloc (
                npas, sizeof (npa_t *) * (npam + NPAINCR)
            ))) {
                SUwarning (1, "additem", "realloc failed for npas");
                return -1;
            }
            npam += NPAINCR;
        }
        npas[npan] = npap;
        npap->npai = npan++;
    }

    /* add to loc dict */
    if (!(locmem = malloc (sizeof (loc_t)))) {
        SUwarning (1, "additem", "malloc failed for locmem");
        return -1;
    }
    locmem->v = vn, locmem->h = hn;
    if (!(locp = dtinsert (locdict, locmem))) {
        SUwarning (1, "additem", "dtinsert failed for %d %d", vn, hn);
        return -1;
    } else  if (locp != locmem) {
        SUwarning (1, "additem", "duplicate entry for %d %d", vn, hn);
        free (locmem);
        if (!(locp->npanxxs = realloc (
            locp->npanxxs, (locp->npanxxn + 1) * sizeof (npanxx_t *)
        ))) {
            SUwarning (1, "additem", "realloc failed for npanxxs");
            return -1;
        }
        locp->npanxxn++;
    } else {
        if (locn == locm) {
            if (!(locs = realloc (
                locs, sizeof (loc_t *) * (locm + LOCINCR)
            ))) {
                SUwarning (1, "additem", "realloc failed for locs");
                return -1;
            }
            locm += LOCINCR;
        }
        locs[locn] = locp;
        locp->loci = locn++;
        if (!(locp->npanxxs = malloc (sizeof (npanxx_t *)))) {
            SUwarning (1, "additem", "malloc failed for npanxxs");
            return -1;
        }
        locp->npanxxn = 1;
        calcll (locp->v, locp->h, &locp->x, &locp->y);
    }
    locp->npanxxs[locp->npanxxn - 1] = npanxxp;
    return 0;
}

#define RAD2DEG(a) (180 * (a) / M_PI)

double a = 0.000077939;
double b = 0.000018571;

double vs = 6363.235;
double hs = 2250.7;
double C = .9155698;
double D = 0.4;

#define sign(x) ((x) > 0 ? 1 : (x) < 0 ? - 1 : 0)

static void calcll (int vn, int hn, int *xp, int *yp) {
    double cosC, sinC;
    double vhv, vhh, h, v, E, cosE, sinE, W, w;
    double p, cosP, sinP, K_, K_2, K, e;

    cosC = cos (C);
    sinC = sin (C);
    vhv = vn - vs;
    vhh = hn - hs;
    h = a * vhh + b * vhv;
    v = b * vhh - a * vhv;
    E = hypot (v, h);
    cosE = cos (E);
    sinE = sin (E);
    W = sqrt (E * E - 2 * D * h + D * D);
    w = acos ((cos (W) - cosE * cos (D)) / (sinE * sin (D))) * sign (v);
    p = 1.263992 - w;
    cosP = cosC * cosE + sinC * sinE * cos (p);
    sinP = sqrt (1 - cosP * cosP);
    K_ = M_PI / 2 - acos (cosP);
    K_2 = K_ * K_;
    K = K_ * (
        1.0056772 + K_2 * (-.0034423 + K_2 * (.0007140 + K_2 * (-.0000777)))
    );
    e = acos ((cosE - cosP * cosC) / (sinP * sinC));
    if (p < 0 || p > M_PI)
        e = -e;
    *yp = RAD2DEG (K) * 1000000.0;
    *xp = RAD2DEG (- (e + 1.4425887)) * 1000000.0;
}
