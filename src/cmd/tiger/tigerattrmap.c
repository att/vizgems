#pragma prototyped

#include <ast.h>
#include <option.h>
#include <swift.h>
#include "tiger.h"

static const char usage[] =
"[-1p1s1D?\n@(#)$Id: tigerattrmap (AT&T Labs Research) 2001-09-24 $\n]"
USAGE_LICENSE
"[+NAME?tigerattrmap - print a mapping between attributes of a TIGER dataset]"
"[+DESCRIPTION?\btigerattrmap\b loads a dataset and prints a mapping between"
" the \afrom\a and the \ato\a attributes in this dataset."
" For example, it can print how zip codes map to counties."
"]"
"[100:i?specifies the directory containing the input files."
" The default is the current directory."
"]:[inputdir]"
"[300:fa?specifies the \afrom\a attribute."
" The default is \bstate\b."
"]:[(state|county|ctbna|blkg|blk|blks|zip|npanxxloc)]"
"[301:ta?specifies the \ato\a attribute."
" The default is \bcounty\b."
"]:[(state|county|ctbna|blkg|blk|blks|zip|npanxxloc)]"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"\n"
"[+SEE ALSO?\btiger\b(1)]"
;

static int printattrmap (int, int);

int main (int argc, char **argv) {
    int norun;
    char *startbrk, *endbrk;
    char *indir;
    int fromattr, toattr;

    startbrk = (char *) sbrk (0);
    indir = ".";
    fromattr = T_EATTR_STATE;
    toattr = T_EATTR_COUNTY;
    norun = 0;
    for (;;) {
        switch (optget (argv, usage)) {
        case -100:
            indir = opt_info.arg;
            continue;
        case -300:
            if (strcmp (opt_info.arg, "zip") == 0)
                fromattr = T_EATTR_ZIP;
            else if (strcmp (opt_info.arg, "npanxxloc") == 0)
                fromattr = T_EATTR_NPANXXLOC;
            else if (strcmp (opt_info.arg, "state") == 0)
                fromattr = T_EATTR_STATE;
            else if (strcmp (opt_info.arg, "county") == 0)
                fromattr = T_EATTR_COUNTY;
            else if (strcmp (opt_info.arg, "ctbna") == 0)
                fromattr = T_EATTR_CTBNA;
            else if (strcmp (opt_info.arg, "blkg") == 0)
                fromattr = T_EATTR_BLKG;
            else if (strcmp (opt_info.arg, "blk") == 0)
                fromattr = T_EATTR_BLK;
            else if (strcmp (opt_info.arg, "blks") == 0)
                fromattr = T_EATTR_BLKS;
            else if (strcmp (opt_info.arg, "country") == 0)
                fromattr = T_EATTR_COUNTRY;
            else
                SUerror ("tigerattrmap", "bad argument for -fa option");
            continue;
        case -301:
            if (strcmp (opt_info.arg, "zip") == 0)
                toattr = T_EATTR_ZIP;
            else if (strcmp (opt_info.arg, "npanxxloc") == 0)
                toattr = T_EATTR_NPANXXLOC;
            else if (strcmp (opt_info.arg, "state") == 0)
                toattr = T_EATTR_STATE;
            else if (strcmp (opt_info.arg, "county") == 0)
                toattr = T_EATTR_COUNTY;
            else if (strcmp (opt_info.arg, "ctbna") == 0)
                toattr = T_EATTR_CTBNA;
            else if (strcmp (opt_info.arg, "blkg") == 0)
                toattr = T_EATTR_BLKG;
            else if (strcmp (opt_info.arg, "blk") == 0)
                toattr = T_EATTR_BLK;
            else if (strcmp (opt_info.arg, "blks") == 0)
                toattr = T_EATTR_BLKS;
            else if (strcmp (opt_info.arg, "country") == 0)
                toattr = T_EATTR_COUNTRY;
            else
                SUerror ("tigerattrmap", "bad argument for -ta option");
            continue;
        case -999:
            SUwarnlevel++;
            continue;
        case '?':
            SUusage (0, "tigerattrmap", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "tigerattrmap", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;

    initrecords ();
    if (loaddata (indir) == -1)
        SUerror ("tigerattrmap", "loaddata failed");
    if (printattrmap (fromattr, toattr) == -1)
        SUerror ("tigerattrmap", "printattrmap failed");
    printstats ();
    endbrk = (char *) sbrk (0);
    SUmessage (1, "tigerattrmap", "memory usage %d", endbrk - startbrk);
    return 0;
}

static int printattrmap (int fromattr, int toattr) {
    edge_t *ep;
    char *fs, *ts;
    int zip, npanxxloc, ctbna;
    short county, blk;
    char state, blks;
    int i;

    SUmessage (1, "printattrmap", "printing attribute map");
    for (
        ep = (edge_t *) dtflatten (edgedict); ep;
        ep = (edge_t *) dtlink (edgedict, ep)
    ) {
        for (i = 0; i < 2; i++) {
            if (i == 0) {
                blks = ep->blksl;
                blk = ep->blkl;
                ctbna = ep->ctbnal;
                county = ep->countyl;
                state = ep->statel;
                zip = ep->zipl;
                npanxxloc = ep->npanxxlocl;
            } else {
                blks = ep->blksr;
                blk = ep->blkr;
                ctbna = ep->ctbnar;
                county = ep->countyr;
                state = ep->stater;
                zip = ep->zipr;
                npanxxloc = ep->npanxxlocr;
            }
            fs = NULL;
            switch (fromattr) {
            case T_EATTR_BLKS:
                if ((state > 0) && (county > 0) && (ctbna > 0) && (blk > 0))
                    fs = sfprints (
                        "%02d%03d%06d%03d%c", state, county, ctbna, blk, blks
                    );
                break;
            case T_EATTR_BLK:
                if ((state > 0) && (county > 0) && (ctbna > 0) && (blk > 0))
                    fs = sfprints (
                        "%02d%03d%06d%03d", state, county, ctbna, blk
                    );
                break;
            case T_EATTR_BLKG:
                if ((state > 0) && (county > 0) && (ctbna > 0) && (blk > 0))
                    fs = sfprints (
                        "%02d%03d%06d%d", state, county, ctbna, blk / 100
                    );
                break;
            case T_EATTR_CTBNA:
                if ((state > 0) && (county > 0) && (ctbna > 0))
                    fs = sfprints ("%02d%03d%06d", state, county, ctbna);
                break;
            case T_EATTR_COUNTY:
                if ((state > 0) && (county > 0))
                    fs = sfprints ("%02d%03d", state, county);
                break;
            case T_EATTR_STATE:
                if ((state > 0))
                    fs = sfprints ("%02d", state);
                break;
            case T_EATTR_ZIP:
                if ((zip > 0))
                    fs = sfprints ("%05d", zip);
                break;
            case T_EATTR_NPANXXLOC:
                if ((npanxxloc > -1))
                    fs = sfprints ("%d", npanxxloc);
                break;
            case T_EATTR_COUNTRY:
                fs = "USA";
                break;
            }
            if (!fs)
                continue;
            fs = strdup (fs);
            ts = NULL;
            switch (toattr) {
            case T_EATTR_BLKS:
                if ((state > 0) && (county > 0) && (ctbna > 0) && (blk > 0))
                    ts = sfprints (
                        "%02d%03d%06d%03d%c", state, county, ctbna, blk, blks
                    );
                break;
            case T_EATTR_BLK:
                if ((state > 0) && (county > 0) && (ctbna > 0) && (blk > 0))
                    ts = sfprints (
                        "%02d%03d%06d%03d", state, county, ctbna, blk
                    );
                break;
            case T_EATTR_BLKG:
                if ((state > 0) && (county > 0) && (ctbna > 0) && (blk > 0))
                    ts = sfprints (
                        "%02d%03d%06d%d", state, county, ctbna, blk / 100
                    );
                break;
            case T_EATTR_CTBNA:
                if ((state > 0) && (county > 0) && (ctbna > 0))
                    ts = sfprints ("%02d%03d%06d", state, county, ctbna);
                break;
            case T_EATTR_COUNTY:
                if ((state > 0) && (county > 0))
                    ts = sfprints ("%02d%03d", state, county);
                break;
            case T_EATTR_STATE:
                if ((state > 0))
                    ts = sfprints ("%02d", state);
                break;
            case T_EATTR_ZIP:
                if ((zip > 0))
                    ts = sfprints ("%05d", zip);
                break;
            case T_EATTR_NPANXXLOC:
                if ((npanxxloc > -1))
                    ts = sfprints ("%d", npanxxloc);
                break;
            case T_EATTR_COUNTRY:
                ts = "USA";
                break;
            }
            if (!ts)
                continue;
            ts = strdup (ts);
            sfprintf (sfstdout, "%s %s\n", fs, ts);
            free (fs), free (ts);
        }
    }
    return 0;
}
