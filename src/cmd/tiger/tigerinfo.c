#pragma prototyped

#include <ast.h>
#include <option.h>
#include <swift.h>
#include "tiger.h"

static const char usage[] =
"[-1p1s1D?\n@(#)$Id: tigerinfo (AT&T Labs Research) 2001-09-24 $\n]"
USAGE_LICENSE
"[+NAME?tigerinfo - print information about a TIGER dataset]"
"[+DESCRIPTION?\btigerinfo\b prints the number of vertices, edges, and"
" polygons in the specified dataset."
"]"
"[100:i?specifies the directory containing the input files."
" The default is the current directory."
"]:[inputdir]"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"\n"
"[+SEE ALSO?\btiger\b(1)]"
;

int main (int argc, char **argv) {
    int norun;
    char *startbrk, *endbrk;
    char *indir;

    startbrk = (char *) sbrk (0);
    indir = ".";
    norun = 0;
    for (;;) {
        switch (optget (argv, usage)) {
        case -100:
            indir = opt_info.arg;
            continue;
        case -999:
            SUwarnlevel++;
            continue;
        case '?':
            SUusage (0, "tigerinfo", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "tigerinfo", opt_info.arg);
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
        SUerror ("tigerinfo", "loaddata failed");
    SUwarnlevel++;
    printstats ();
    SUwarnlevel--;
    endbrk = (char *) sbrk (0);
    SUmessage (1, "tigerinfo", "memory usage %d", endbrk - startbrk);
    return 0;
}
