#pragma prototyped

#include <ast.h>
#include <option.h>
#include <swift.h>
#include "tiger.h"

static const char usage[] =
"[-1p1s1D?\n@(#)$Id: tigerprint (AT&T Labs Research) 2001-09-24 $\n]"
USAGE_LICENSE
"[+NAME?tigerprint - dump contents of a TIGER dataset]"
"[+DESCRIPTION?\btigerprint\b generates an ascii dump of the contents of the"
" specified dataset, or a postscript picture of it."
"]"
"[100:i?specifies the directory containing the input files."
" The default is the current directory."
"]:[inputdir]"
"[400:ps1?generates postscript output of the endpoints of the edges in the"
" dataset."
"]"
"[401:ps2?generates postscript output that includes the shape vertices in the"
" dataset."
"]"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"\n"
"[+SEE ALSO?\btiger\b(1)]"
;

int main (int argc, char **argv) {
    int norun;
    char *startbrk, *endbrk;
    char *indir;
    int psflag, psmode;

    startbrk = (char *) sbrk (0);
    indir = ".";
    psflag = FALSE;
    norun = 0;
    for (;;) {
        switch (optget (argv, usage)) {
        case -100:
            indir = opt_info.arg;
            continue;
        case -400:
            psflag = TRUE;
            psmode = 0;
            continue;
        case -401:
            psflag = TRUE;
            psmode = 1;
            continue;
        case -999:
            SUwarnlevel++;
            continue;
        case '?':
            SUusage (0, "tigerprint", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "tigerprint", opt_info.arg);
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
        SUerror ("tigerprint", "loaddata failed");
    if (psflag)
        printps (psmode);
    else
        printrecords ();
    printstats ();
    endbrk = (char *) sbrk (0);
    SUmessage (1, "tigerprint", "memory usage %d", endbrk - startbrk);
    return 0;
}
