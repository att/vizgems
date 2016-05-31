#pragma prototyped

#include <ast.h>
#include <option.h>
#include <vmalloc.h>
#include <swift.h>
#include <aggr.h>
#include <aggr_int.h>

static const char usage[] =
"[-1p1s1D?\n@(#)$Id: aggrtopn (AT&T Labs Research) 2001-09-24 $\n]"
USAGE_LICENSE
"[+NAME?aggrtopn - compute the top maximum or minimum items in an AGGR file]"
"[+DESCRIPTION?\baggrtopn\b computes and prints either the top maximum or top"
" minimum items in an AGGR file along with their values.]"
"[129:n?specifies how many top numbers to print."
" The default value is \b10\b."
"]#[n]"
"[130:ffr?specifies to search for top values starting from frame \bframe\b."
" The default action is to start from the first frame."
"]#[frame]"
"[131:lfr?specifies to search for top values ending with frame \bframe\b."
" The default action is to end with the last frame."
"]#[frame]"
"[132:min?specifies to search for the top minimum values."
" The default action is to search for maximum values."
"]"
"[133:max?specifies to search for the top maximum values."
"]"
"[123:V?increases the amount of info being output."
" The default action is to print the frame id, item id, and value for each"
" item."
" The first \bV\b will also add key ids to the output."
"]"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"\n"
"\n[ file ]\n"
"\n"
"[+SEE ALSO?\baggr\b(1), \baggr\b(3)]"
;

int main (int argc, char **argv) {
    int norun;
    int oper, kvn;
    AGGRfile_t *iafp;
    AGGRkv_t *kvs;
    int kvi;
    int fframei, lframei;
    int verbose;
    unsigned char **revmap;

    if (AGGRinit () == -1)
        SUerror ("aggrtopn", "init failed");
    oper = AGGR_TOPN_OPER_MAX;
    kvn = 10;
    fframei = lframei = -1;
    verbose = 0;
    norun = 0;
    for (;;) {
        switch (optget (argv, usage)) {
        case -129:
            kvn = opt_info.num;
            continue;
        case -130:
            fframei = opt_info.num;
            continue;
        case -131:
            lframei = opt_info.num;
            continue;
        case -132:
            oper = AGGR_TOPN_OPER_MIN;
            continue;
        case -133:
            oper = AGGR_TOPN_OPER_MAX;
            continue;
        case -123:
            verbose++;
            continue;
        case -999:
            SUwarnlevel++;
            continue;
        case '?':
            SUusage (0, "aggrtopn", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "aggrtopn", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;
    if (argc != 1)
        SUerror ("aggrtopn", "no input file name specified");
    if (!(iafp = AGGRopen (argv[0], AGGR_RWMODE_RD, 1, TRUE)))
        SUerror ("aggrtopn", "open failed");
    if (!(kvs = vmalloc (Vmheap, kvn * sizeof (AGGRkv_t))))
        SUerror ("aggrtopn", "vmalloc failed");
    for (kvi = 0; kvi < kvn; kvi++)
        if (!(kvs[kvi].valp = vmalloc (Vmheap, iafp->hdr.vallen)))
            SUerror ("aggrtopn", "vmalloc failed");
    if (fframei == -1)
        fframei = 0;
    if (lframei == -1)
        lframei = iafp->hdr.framen - 1;
    if (AGGRtopn (iafp, fframei, lframei, kvs, kvn, oper) == -1)
        SUerror ("aggrtopn", "topn failed");
    if (verbose > 1) {
        if (!(revmap = vmalloc (Vmheap, iafp->ad.itemn * sizeof (char *))))
            SUerror ("aggrtopn", "vmalloc failed");
        _aggrdictreverse (iafp, revmap);
    }
    for (kvi = 0; kvi < kvn; kvi++) {
        if (kvs[kvi].framei == -1)
            continue;
        sfprintf (sfstdout, "%d %d ", kvs[kvi].framei, kvs[kvi].itemi);
        if (verbose > 1) {
            _aggrdictprintkey (iafp, sfstdout, revmap[kvs[kvi].itemi]);
            sfprintf (sfstdout, " ");
        }
        _aggrprintval (iafp, sfstdout, kvs[kvi].valp);
        sfprintf (sfstdout, "\n");
    }
    if (AGGRterm () == -1)
        SUerror ("aggrtopn", "term failed");
    return 0;
}
