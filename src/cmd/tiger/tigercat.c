#pragma prototyped

#include <ast.h>
#include <option.h>
#include <swift.h>
#include "tiger.h"

static const char usage[] =
"[-1p1s1D?\n@(#)$Id: tigercat (AT&T Labs Research) 2001-09-24 $\n]"
USAGE_LICENSE
"[+NAME?tigercat - concatenate several TIGER datasets into one]"
"[+DESCRIPTION?\btigercat\b reads one or more TIGER datasets and concatenates"
" them into one."
"]"
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
"[202:rz?specifies that edges with zero values in the CTBNA fields should be"
" eliminated."
" The default is to keep such edges."
"]"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"\n"
"[+SEE ALSO?\btiger\b(1)]"
;

int main (int argc, char **argv) {
    int norun;
    char *startbrk, *endbrk;
    char *outdir;
    int removezero, splicemode;
    float digits;

    startbrk = (char *) sbrk (0);
    outdir = ".";
    removezero = 0;
    splicemode = 1;
    digits = -1;
    norun = 0;
    for (;;) {
        switch (optget (argv, usage)) {
        case -101:
            outdir = opt_info.arg;
            continue;
        case -200:
            splicemode = opt_info.num;
            continue;
        case -201:
            if ((digits = atof (opt_info.arg)) < 0)
                digits = -1;
            continue;
        case -202:
            removezero = 1;
            continue;
        case -999:
            SUwarnlevel++;
            continue;
        case '?':
            SUusage (0, "tigercat", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "tigercat", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;

    initrecords ();
    while (argc) {
        SUmessage (1, "tigercat", "loading directory %s", argv[0]);
        if (loaddata (argv[0]) == -1 || mergeonesidededges () == -1)
            SUerror ("tigercat", "loaddata failed for directory %s", argv[0]);
        argc--, argv++;
    }
    if (removezero > 0 && removezeroedges () == -1)
        SUerror ("tigercat", "removezeroedges failed");
    if (splicemode > 0 && removedegree2vertices (splicemode) == -1)
        SUerror ("tigercat", "removedegree2vertices failed");
    if (digits >= 0 && simplifyxys (digits) == -1)
        SUerror ("tigercat", "simplifyxys failed");
    savedata (outdir);
    printstats ();
    endbrk = (char *) sbrk (0);
    SUmessage (1, "tigercat", "memory usage %d", endbrk - startbrk);
    return 0;
}
