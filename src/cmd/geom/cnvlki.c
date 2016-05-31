#pragma prototyped

#include <ast.h>
#include <option.h>
#include <tok.h>
#include <cdt.h>
#include <swift.h>

static const char usage[] =
"[-1p1s1D?\n@(#)$Id: cnvlki (AT&T Labs Research) 2001-09-24 $\n]"
USAGE_LICENSE
"[+NAME?cnvlki - convert LKI data to format for ascii2geom]"
"[+DESCRIPTION?\bcnvlki\b converts geometry data from LKI to the ascii format"
" used by \bascii2geom\b."
"]"
"[100:i?specifies the input file."
" The default is \bascii.lki\b."
"]:[inputfile]"
"[101:o?specifies the output file."
" The default is \bascii.coords\b."
"]:[outputfile]"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"\n"
"[+SEE ALSO?\btiger\b(1)]"
;

typedef struct item_t {
    Dtlink_t link;
    /* begin key */
    char *id;
    /* end key */
    char *coords;
} item_t;

#define ITEMINCR 1000

static Dtdisc_t itemdisc = {
    sizeof (Dtlink_t), -1, 0, NULL, NULL, NULL, NULL, NULL, NULL
};

static Dt_t *itemdict;

static item_t **items;
static int itemm, itemn;

static int count;

static int loaditems (char *);
static int saveitemcoords (char *);
static int additem (char *, char *, int);
static char *getline (Sfio_t *);
static void badline (char *);

int main (int argc, char **argv) {
    int norun;
    char *infile, *outfile;

    infile = "ascii.lki";
    outfile = "ascii.coords";
    norun = 0;
    for (;;) {
        switch (optget (argv, usage)) {
        case -100:
            infile = opt_info.arg;
            continue;
        case -101:
            outfile = opt_info.arg;
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

    if (!(items = malloc (sizeof (item_t *) * ITEMINCR)))
        SUerror ("cnvlki", "malloc failed for items");
    itemm = ITEMINCR;
    itemn = 0;
    if (!(itemdict = dtopen (&itemdisc, Dtset))) {
        SUerror ("cnvlki", "dtopen failed for itemdict");
        return -1;
    }
    if (loaditems (infile) == -1)
        SUerror ("cnvlki", "loaditems failed");
    if (saveitemcoords (outfile) == -1)
        SUerror ("cnvlki", "saveitemcoords failed");
    return 0;
}

static int loaditems (char *file) {
    Sfio_t *bfp;
    int blen;
    char *bp;
    Sfio_t *fp;
    char *s, *line, *name;
    double x, y, fx, fy;
    int polyn, polyi, pointn, pointi;

    SUmessage (1, "loaditems", "loading file %s", file);
    if (!(bfp = sftmp (1024 * 1024)))
        SUerror ("cnvlki", "cannot open buffer channel");
    if (!(fp = sfopen (NULL, file, "r"))) {
        SUwarning (1, "cnvlki", "open failed for file %s", file);
        return -1;
    }
    while ((line = getline (fp))) {
        if (line[0] == 0)
            continue;
        if (!strstr (line, "BEGINOUTLINE")) {
            SUwarning (1, "cnvlki", "missing BEGINOUTLINE in file %s", file);
            badline (line);
            continue;
        }
        if (!(line = getline (fp))) {
            SUwarning (1, "cnvlki", "missing name in file %s", file);
            badline (line);
            continue;
        }
        if (!(name = strdup (line))) {
            SUwarning (1, "cnvlki", "name copy failed");
            return -1;
        }
        if (!(line = getline (fp))) {
            SUwarning (1, "cnvlki", "missing npolys in file %s", file);
            badline (line);
            continue;
        }
        polyn = atoi (line);
        for (polyi = 0; polyi < polyn; polyi++) {
            if (polyi > 0)
                sfprintf (bfp, " | |");
            if (!(line = getline (fp))) {
                SUwarning (1, "cnvlki", "missing npoints in file %s", file);
                badline (line);
                continue;
            }
            if (!(s = strchr (line, ' '))) {
                pointn = atoi (line);
                if (!(line = getline (fp))) {
                    SUwarning (1, "cnvlki", "missing line in file %s", file);
                    badline (line);
                    continue;
                }
            } else {
                *s = 0;
                pointn = atoi (line);
            }
            pointn--;
            for (pointi = 0; pointi < pointn; pointi++) {
                if (!(line = getline (fp))) {
                    SUwarning (1, "cnvlki", "missing point in file %s", file);
                    badline (line);
                    continue;
                }
                if (tokscan (line, &s, "%f %f\n", &x, &y) < 2) {
                    SUwarning (1, "cnvlki", "bad line #%d: %s", count, line);
                    continue;
                }
                if (pointi == 0)
                    fx = x, fy = y;
                sfprintf (bfp, " %f %f", x, y);
            }
            sfprintf (bfp, " %f %f", fx, fy);
        }
        if (!(line = getline (fp))) {
            SUwarning (1, "cnvlki", "missing ENDOUTLINE in file %s", file);
            badline (line);
            continue;
        }
        if (!strstr (line, "ENDOUTLINE")) {
            SUwarning (1, "cnvlki", "bad line in file %s, %s", file, line);
            continue;
        }
        if ((blen = sfseek (bfp, 0, SEEK_CUR)) == -1) {
            SUwarning (1, "cnvlki", "sfseek get failed");
            continue;
        }
        if (sfseek (bfp, 0, SEEK_SET) == -1) {
            SUwarning (1, "cnvlki", "sfseek set failed");
            continue;
        }
        if (!(bp = sfreserve (bfp, blen, 0)) || sfvalue (bfp) < blen) {
            SUwarning (1, "cnvlki", "sfreserve failed");
            continue;
        }
        if (additem (name, bp, blen) == -1) {
            SUwarning (1, "cnvlki", "additem failed");
            return -1;
        }
        if (sfseek (bfp, 0, SEEK_SET) == -1) {
            SUwarning (1, "cnvlki", "sfseek set failed");
            continue;
        }
    }
    sfclose (fp);
    return 0;
}

static int saveitemcoords (char *file) {
    Sfio_t *fp;
    int itemi;

    SUmessage (1, "cnvlki", "saving file %s", file);
    if (!(fp = sfopen (NULL, file, "w"))) {
        SUwarning (1, "cnvlki", "open failed for %s", file);
        return -1;
    }
    for (itemi = 0; itemi < itemn; itemi++)
        sfprintf (
            fp, "\"%s\" \"%s\" \"\"\n",
            items[itemi]->id, items[itemi]->coords
        );
    sfclose (fp);
    return 0;
}

static int additem (char *id, char *coords, int coordn) {
    item_t *itemmem, *itemp;

    /* add to item dict */
    if (!(itemmem = malloc (sizeof (item_t)))) {
        SUwarning (1, "cnvlki", "malloc failed for itemmem");
        return -1;
    }
    itemmem->id = strdup (id);
    if (!(itemp = dtinsert (itemdict, itemmem))) {
        SUwarning (1, "cnvlki", "dtinsert failed for item %s", itemmem->id);
        return -1;
    } else if (itemp != itemmem) {
        SUwarning (1, "cnvlki", "duplicate entry for %s", itemmem->id);
        free (itemmem);
    } else {
        if (!(itemp->coords = malloc (coordn + 1))) {
            SUwarning (1, "cnvlki", "malloc failed for coords");
            return -1;
        }
        memcpy (itemp->coords, coords, coordn);
        itemp->coords[coordn] = 0;
        if (itemn == itemm) {
            if (!(items = realloc (
                items, sizeof (item_t *) * (itemm + ITEMINCR)
            ))) {
                SUwarning (1, "cnvlki", "realloc failed for items");
                return -1;
            }
            itemm += ITEMINCR;
        }
        items[itemn] = itemp;
        itemn++;
    }
    return 0;
}

static char *getline (Sfio_t *fp) {
    char *line;
    int n;

    if (!(line = sfgetr (fp, '\n', 1)))
        return NULL;
    count++;
    if (line[strlen (line) - 1] == '\r')
        line[strlen (line) - 1] = 0;
    while (line[0] == ' ')
        line++;
    for (n = strlen (line); n > 0 && line[n - 1] == ' '; n--)
        line[n - 1] = 0;
    return line;
}

static void badline (char *line) {
    SUwarning (1, "cnvlki", "bad line #%d: %s", count, line);
}
