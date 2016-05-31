#pragma prototyped

#include <ast.h>
#include <option.h>
#include <tok.h>
#include <vmalloc.h>
#include <swift.h>
#include <aggr.h>
#include <aggr_int.h>

static const char usage[] =
"[-1p1s1D?\n@(#)$Id: aggrmean (AT&T Labs Research) 2001-09-24 $\n]"
USAGE_LICENSE
"[+NAME?aggrmean - compute means and standard deviations for AGGR data]"
"[+DESCRIPTION?\baggrmean\b computes the mean and standard deviation of the"
" item values in the AGGR input files.]"
"[100:o?specifies the names of the output files."
" There should be two output files, the first for the mean values and the"
" second for the standard deviation values."
" The default names are \bmean.aggr\b and \bstdev.aggr\b."
" The argument must be quoted: \a-o 'a.aggr b.aggr'."
"]:['file1 file2']"
"[101:cl?specifies a string describing the output dataset."
" Operations that combine multiple AGGR files will only work across files with"
" the same description."
" The default class is \baggrmean\b."
"]:[class]"
"[103:vt?specifies the data type of the value fields."
" The main type name may be following by a colon and a number, specifying how"
" many elements of that type make up the value, e.g. \bint:5\b would mean that"
" the value is a sequence of 5 integers."
" Such a value should appear in the input stream as 5 integers separated by"
" colons: 10:22:3:1:1."
" The default type is \bfloat\b."
"]:[(char|short|int|llong|float|double)[::n]]]"
"[104:di?specifies the increment step for adding new keys to the dictionary."
" The default value is \b1\b."
" Higher values may improve performance when loading large data files since"
" they result in less frequent rearranging of the output files."
"]#[n]"
"[105:ii?specifies the increment step for adding new items to the file."
" The default value is \b1\b."
" Higher values may improve performance when loading large data files since"
" they result in less frequent rearranging of the output files."
"]#[n]"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"\n"
"\n[ file1 file2 ... ]\n"
"\n"
"[+SEE ALSO?\baggr\b(1), \baggr\b(3)]"
;

int main (int argc, char **argv) {
    int norun;
    char *mnamep, *dnamep, *classp;
    int valtype, vallen, dictincr, itemincr;
    char *avs[10], *s;
    AGGRfile_t **iafps, *mafp, *dafp;
    int iafpn, iafpi;

    if (AGGRinit () == -1)
        SUerror ("aggrmean", "init failed");
    mnamep = "mean.aggr", dnamep = "stdev.aggr";
    classp = "aggrmean";
    valtype = -1, vallen = -1;
    dictincr = 1, itemincr = 1;
    norun = 0;
    for (;;) {
        switch (optget (argv, usage)) {
        case -100:
            if (tokscan (opt_info.arg, &s, " %v ", avs, 10) != 2)
                SUerror ("aggrmean", "bad argument for -o option");
            mnamep = avs[0], dnamep = avs[1];
            continue;
        case -101:
            classp = opt_info.arg;
            continue;
        case -103:
            if (AGGRparsetype (opt_info.arg, &valtype, &vallen) == -1)
                SUerror ("aggrmean", "bad argument for -vt option");
            continue;
        case -104:
            dictincr = opt_info.num;
            continue;
        case -105:
            itemincr = opt_info.num;
            continue;
        case -999:
            SUwarnlevel++;
            continue;
        case '?':
            SUusage (0, "aggrmean", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "aggrmean", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;
    if (argc < 2)
        SUerror ("aggrmean", "not enough input files specified");
    if (!(iafps = vmalloc (Vmheap, sizeof (AGGRfile_t *) * argc)))
        SUerror ("aggrmean", "vmalloc failed");
    iafpn = argc;
    for (iafpi = 0; iafpi < iafpn; iafpi++) {
        if (!(iafps[iafpi] = AGGRopen (argv[iafpi], AGGR_RWMODE_RD, 1, TRUE)))
            SUerror ("aggrmean", "open failed");
    }
    if (valtype == -1) {
        valtype = AGGR_TYPE_FLOAT;
        vallen = (
            iafps[0]->hdr.vallen / AGGRtypelens[iafps[0]->hdr.valtype]
        ) * AGGRtypelens[valtype];
    }
    if (!(mafp = AGGRcreate (
        mnamep, 1, 1, iafps[0]->hdr.keytype, valtype, classp,
        iafps[0]->ad.keylen, vallen, dictincr, itemincr, 1, TRUE
    )))
        SUerror ("aggrmean", "create failed");
    if (!(dafp = AGGRcreate (
        dnamep, 1, 1, iafps[0]->hdr.keytype, valtype, classp,
        iafps[0]->ad.keylen, vallen, dictincr, itemincr, 1, TRUE
    )))
        SUerror ("aggrmean", "create failed");
    if (AGGRmean (iafps, iafpn, mafp, dafp) == -1)
        SUerror ("aggrmean", "mean failed");
    if (AGGRminmax (mafp, TRUE) == -1)
        SUerror ("aggrmean", "minmax failed");
    if (AGGRminmax (dafp, TRUE) == -1)
        SUerror ("aggrmean", "minmax failed");
    if (AGGRclose (mafp) == -1)
        SUerror ("aggrmean", "close failed");
    if (AGGRclose (dafp) == -1)
        SUerror ("aggrmean", "close failed");
    if (AGGRterm () == -1)
        SUerror ("aggrmean", "term failed");
    return 0;
}
