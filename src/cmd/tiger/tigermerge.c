#pragma prototyped

#include <ast.h>
#include <option.h>
#include <swift.h>
#include "tiger.h"

static const char usage[] =
"[-1p1s1D?\n@(#)$Id: tigermerge (AT&T Labs Research) 2001-09-24 $\n]"
USAGE_LICENSE
"[+NAME?tigermerge - merge polygons in a TIGER dataset]"
"[+DESCRIPTION?\btigermerge\b merges adjacent polygons that share the same"
" attribute value into one."
" If an edge has the same left and right values for the specified attribute,"
" that edge is eliminated and the polygons on its two sides are merged"
" together."
"]"
"[100:i?specifies the directory containing the input files."
" The default is the current directory."
"]:[inputdir]"
"[101:o?specifies the directory for the output files."
" The default is the current directory."
"]:[outputdir]"
"[200:sm?specifies the splice mode for vertices of degree two."
" When a vertex only has 2 edges connected to it, the two edges can be merged"
" into one and the old vertex becomes a shape vertex in the new edge."
" This reduces the number of edges and speeds up several algorithms."
" A value of \b0\b disables splicing."
" A value of \b1\b (default) excludes boundary edges."
" A value of \b2\b operates on all edges."
"]#[(0|1|2)]"
"[300:a?specifies the attribute to use."
" When \bzip\b or \bnpanxxloc\b are specified, the corresponding attributes"
" are compared."
" When any other attribute is specified, the comparison involves all the"
" attributes down to that level."
" For example, if \bblkg\b is specified, the \bstate\b, \bcounty\b, \bctbna\b,"
" and \bblkg\b attributes must match for an edge to be eliminated."
" The default is \bstate\b."
"]:[(state|county|ctbna|blkg|blk|blks|zip|npanxxloc)]"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"\n"
"[+SEE ALSO?\btiger\b(1)]"
;

int main (int argc, char **argv) {
    int norun;
    char *startbrk, *endbrk;
    char *indir, *outdir;
    int attr, splicemode;

    startbrk = (char *) sbrk (0);
    indir = ".";
    outdir = ".";
    attr = T_EATTR_STATE;
    splicemode = 1;
    norun = 0;
    for (;;) {
        switch (optget (argv, usage)) {
        case -100:
            indir = opt_info.arg;
            continue;
        case -101:
            outdir = opt_info.arg;
            continue;
        case -200:
            splicemode = opt_info.num;
            continue;
        case -300:
            if (strcmp (opt_info.arg, "zip") == 0)
                attr = T_EATTR_ZIP;
            else if (strcmp (opt_info.arg, "npanxxloc") == 0)
                attr = T_EATTR_NPANXXLOC;
            else if (strcmp (opt_info.arg, "state") == 0)
                attr = T_EATTR_STATE;
            else if (strcmp (opt_info.arg, "county") == 0)
                attr = T_EATTR_COUNTY;
            else if (strcmp (opt_info.arg, "ctbna") == 0)
                attr = T_EATTR_CTBNA;
            else if (strcmp (opt_info.arg, "blkg") == 0)
                attr = T_EATTR_BLKG;
            else if (strcmp (opt_info.arg, "blk") == 0)
                attr = T_EATTR_BLK;
            else if (strcmp (opt_info.arg, "blks") == 0)
                attr = T_EATTR_BLKS;
            else if (strcmp (opt_info.arg, "country") == 0)
                attr = T_EATTR_COUNTRY;
            else
                SUerror ("tigermerge", "bad argument for -a option");
            continue;
        case -999:
            SUwarnlevel++;
            continue;
        case '?':
            SUusage (0, "tigermerge", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "tigermerge", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;

    initrecords ();
    if (loadattrboundaries (indir, attr) == -1)
        SUerror ("tigermerge", "loadattrboundaries failed");
    if (splicemode > 0 && removedegree2vertices (splicemode) == -1)
        SUerror ("tigermerge", "removedegree2vertices failed");
    if (buildpolysbyattr (attr) == -1)
        SUerror ("tigermerge", "buildpolysbyattr failed");
    savedata (outdir);
    printstats ();
    endbrk = (char *) sbrk (0);
    SUmessage (1, "tigermerge", "memory usage %d", endbrk - startbrk);
    return 0;
}
