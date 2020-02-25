#pragma prototyped

#include <ast.h>
#include <option.h>
#include <swift.h>
#include "tiger.h"

static const char usage[] =
"[-1p1s1D?\n@(#)$Id: tigerextract (AT&T Labs Research) 2001-09-24 $\n]"
USAGE_LICENSE
"[+NAME?tigerextract - extract elements from a TIGER dataset]"
"[+DESCRIPTION?\btigerextract\b extracts geometry elements from a dataset"
" based on the specified attribute and attribute value."
" Edges that have the specified value in either of their sides are kept,"
" all other edges eliminated."
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
"[300:a?specifies the attribute and attribute value to use."
" The default is \bstate=34\b."
"]:[(state|county|ctbna|blkg|blk|blks|zip|npanxxloc)=value]"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"\n"
"[+SEE ALSO?\btiger\b(1)]"
;

int main (int argc, char **argv) {
    int norun;
    char *startbrk, *endbrk;
    char *indir, *outdir;
    int attr, val, splicemode;
    char *kp, *vp;

    startbrk = (char *) sbrk (0);
    indir = ".";
    outdir = ".";
    attr = T_EATTR_STATE;
    val = 34;
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
            if (!(kp = strdup (opt_info.arg)))
                SUerror ("tigerextract", "cannot copy argument value for -a");
            if (!(vp = strchr (kp, '=')))
                SUerror ("tigerextract", "bad argument for -a option");
            *vp++ = 0;
            if (strcmp (kp, "zip") == 0)
                attr = T_EATTR_ZIP;
            else if (strcmp (kp, "npanxxloc") == 0)
                attr = T_EATTR_NPANXXLOC;
            else if (strcmp (kp, "state") == 0)
                attr = T_EATTR_STATE;
            else if (strcmp (kp, "county") == 0)
                attr = T_EATTR_COUNTY;
            else if (strcmp (kp, "ctbna") == 0)
                attr = T_EATTR_CTBNA;
            else if (strcmp (kp, "blkg") == 0)
                attr = T_EATTR_BLKG;
            else if (strcmp (kp, "blk") == 0)
                attr = T_EATTR_BLK;
            else if (strcmp (kp, "blks") == 0)
                attr = T_EATTR_BLKS;
            else if (strcmp (kp, "country") == 0)
                attr = T_EATTR_COUNTRY;
            else if (strcmp (kp, "cfcc") == 0)
                attr = T_EATTR_CFCC;
            else
                SUerror ("tigerextract", "bad argument for -a option");
            if (attr == T_EATTR_CFCC)
                val = (vp[0] - 'A') * 256 + atoi (&vp[1]) * 10;
            else
                val = atoi (vp);
            continue;
        case -999:
            SUwarnlevel++;
            continue;
        case '?':
            SUusage (0, "tigerextract", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "tigerextract", opt_info.arg);
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
        SUerror ("tigerextract", "loaddata failed");
    if (mergeonesidededges () == -1)
        SUerror ("tigerextract", "mergeonesidededges failed");
    if (extractbyattr (attr, val) == -1)
        SUerror ("tigerextract", "extractbyattr failed");
    if (splicemode > 0 && removedegree2vertices (splicemode) == -1)
        SUerror ("tigerextract", "removedegree2vertices failed");
    savedata (outdir);
    printstats ();
    endbrk = (char *) sbrk (0);
    SUmessage (1, "tigerextract", "memory usage %d", endbrk - startbrk);
    return 0;
}
