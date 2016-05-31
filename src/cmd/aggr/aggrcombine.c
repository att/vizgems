#pragma prototyped

#include <ast.h>
#include <option.h>
#include <sys/resource.h>
#include <vmalloc.h>
#include <tok.h>
#include <swift.h>
#include <aggr.h>
#include <aggr_int.h>

static const char usage[] =
"[-1p1s1D?\n@(#)$Id: aggrcombine (AT&T Labs Research) 2001-09-24 $\n]"
USAGE_LICENSE
"[+NAME?aggrcombine - compute arithmetic operations on AGGR files]"
"[+DESCRIPTION?\baggrcombine\b computes an arithmetic combination of the input"
" AGGR files and stores the result in the output file."
" The input files should all have the same key and value type and should also"
" have the same number of frames but they may have different dictionaries."
" The dictionary of the output file will be the union of all the input"
" dictionaries."
" Items from the input files are combined based on their dictionary keys."
" When a key-item combination exists in only some of the input files, its"
" value in all the other files is assumed to be zero.]"
"[100:o?specifies the name of the output file."
" The default name is \bcombine.aggr\b."
"]:[file]"
"[101:cl?specifies a string describing the output dataset."
" \baggrcombine\b will only work across files with the same description."
" The default class is \baggrcombine\b."
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
"[109:add?performs addition."
" This is the default."
"]"
"[110:sub?performs subtraction."
" Only two input files may be specified."
"]"
"[111:mul?performs multiplication."
"]"
"[112:div?performs division."
" Only two input files may be specified."
"]"
"[127:min?specifies that values are combined by picking the minimum value."
"]"
"[128:max?specifies that values are combined by picking the maximum value."
"]"
"[113:wavg?performs averaging."
" The argument must be a sequence of numbers (quoted to avoid word splitting)."
" These numbers correspond to the set of input files, in order."
" Each output value is the sum: (n1 * v1) + (n2 * v2) + ..., where v1, v2, ..."
" are the corresponding values in the input files."
"]:['n1 ...']"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"\n"
"\n[ file ]\n"
"\n"
"[+SEE ALSO?\baggr\b(1), \baggr\b(3)]"
;

int main (int argc, char **argv) {
    int norun;
    char *onamep, *classp;
    int valtype, vallen, dictincr, itemincr, oper;
    char *wavgstr;
    char *wavgs[10000], *s;
    float *ws;
    AGGRfile_t **iafps, *oafp;
    int iafpn, iafpi;
    struct rlimit rl;

    if (AGGRinit () == -1)
        SUerror ("aggrcombine", "init failed");
    onamep = "combine.aggr";
    classp = "aggrcombine";
    valtype = -1, vallen = -1;
    dictincr = 1, itemincr = 1;
    oper = AGGR_COMBINE_OPER_ADD;
    wavgstr = NULL;
    ws = NULL;
    norun = 0;
    for (;;) {
        switch (optget (argv, usage)) {
        case -100:
            onamep = opt_info.arg;
            continue;
        case -101:
            classp = opt_info.arg;
            continue;
        case -103:
            if (AGGRparsetype (opt_info.arg, &valtype, &vallen) == -1)
                SUerror ("aggrcombine", "bad argument for -vt option");
            continue;
        case -104:
            dictincr = opt_info.num;
            continue;
        case -105:
            itemincr = opt_info.num;
            continue;
        case -109:
            oper = AGGR_COMBINE_OPER_ADD;
            continue;
        case -110:
            oper = AGGR_COMBINE_OPER_SUB;
            continue;
        case -111:
            oper = AGGR_COMBINE_OPER_MUL;
            continue;
        case -112:
            oper = AGGR_COMBINE_OPER_DIV;
            continue;
        case -127:
            oper = AGGR_COMBINE_OPER_MIN;
            continue;
        case -128:
            oper = AGGR_COMBINE_OPER_MAX;
            continue;
        case -113:
            oper = AGGR_COMBINE_OPER_WAVG;
            wavgstr = opt_info.arg;
            continue;
        case -999:
            SUwarnlevel++;
            continue;
        case '?':
            SUusage (0, "aggrcombine", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "aggrcombine", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;
    if (argc < 1)
        SUerror ("aggrcombine", "no input file names specified");
    if (getrlimit (RLIMIT_NOFILE, &rl) != -1) {
        rl.rlim_cur = rl.rlim_max;
        setrlimit (RLIMIT_NOFILE, &rl);
    }
    if (!(iafps = vmalloc (Vmheap, sizeof (AGGRfile_t *) * argc)))
        SUerror ("aggrcombine", "vmalloc failed");
    iafpn = argc;
    if (oper == AGGR_COMBINE_OPER_WAVG) {
        if (tokscan (wavgstr, &s, " %v ", wavgs, 10000) != iafpn)
            SUerror (
                "aggrcombine", "wrong number of arguments for -wavg option"
            );
        if (!(ws = vmalloc (Vmheap, sizeof (float) * iafpn)))
            SUerror ("aggrcombine", "vmalloc failed");
        for (iafpi = 0; iafpi < iafpn; iafpi++)
            ws[iafpi] = atof (wavgs[iafpi]);
    }
    for (iafpi = 0; iafpi < iafpn; iafpi++) {
        if (!(iafps[iafpi] = AGGRopen (argv[iafpi], AGGR_RWMODE_RD, 1, TRUE)))
            SUerror ("aggrcombine", "open failed");
    }
    if (valtype == -1) {
        valtype = AGGR_TYPE_FLOAT;
        vallen = (
            iafps[0]->hdr.vallen / AGGRtypelens[iafps[0]->hdr.valtype]
        ) * AGGRtypelens[valtype];
    }
    if (!(oafp = AGGRcreate (
        onamep, 1, 1, iafps[0]->hdr.keytype, valtype, classp,
        iafps[0]->ad.keylen, vallen, dictincr, itemincr, 1, TRUE
    )))
        SUerror ("aggrcombine", "create failed");
    if (AGGRcombine (iafps, iafpn, oper, ws, oafp) == -1)
        SUerror ("aggrcombine", "combine failed");
    if (AGGRminmax (oafp, TRUE) == -1)
        SUerror ("aggrcombine", "minmax failed");
    if (AGGRclose (oafp) == -1)
        SUerror ("aggrcombine", "close failed");
    if (AGGRterm () == -1)
        SUerror ("aggrcombine", "term failed");
    return 0;
}
