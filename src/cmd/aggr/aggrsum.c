#pragma prototyped

#include <ast.h>
#include <option.h>
#include <vmalloc.h>
#include <swift.h>
#include <aggr.h>
#include <aggr_int.h>

static const char usage[] =
"[-1p1s1D?\n@(#)$Id: aggrsum (AT&T Labs Research) 2001-09-24 $\n]"
USAGE_LICENSE
"[+NAME?aggrsum - sum up all frames or all items in an AGGR file]"
"[+DESCRIPTION?\baggrsum\b sums up either all the items in a frame, or all the"
" frames for an item, into one value."
" It reduces a two-dimensional dataset to a one-dimensional summary.]"
"[100:o?specifies the name of the output file."
" The default name is \bsum.aggr\b."
"]:[file]"
"[101:cl?specifies a string describing the output dataset."
" Operations that combine multiple AGGR files will only work across files with"
" the same description."
" The default class is \baggrsum\b."
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
"[124:frames?specifies that values for all frames for each item should be"
" combined into one value per item."
"]"
"[125:items?specifies that values for all items in a frame should be combined"
" into one value per frame."
"]"
"[126:add?specifies that values are combined by adding them up."
" This is the default action."
"]"
"[127:min?specifies that values are combined by picking the minimum value."
"]"
"[128:max?specifies that values are combined by picking the maximum value."
"]"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"\n"
"\n[ file ]\n"
"\n"
"[+SEE ALSO?\baggr\b(1), \baggr\b(3)]"
;

int main (int argc, char **argv) {
    int norun;
    char *onamep, *classp;
    int valtype, vallen, dictincr, itemincr, dir, oper;
    AGGRfile_t *iafp, *oafp;

    if (AGGRinit () == -1)
        SUerror ("aggrsum", "init failed");
    onamep = "sum.aggr";
    classp = "aggrsum";
    valtype = -1, vallen = -1;
    dictincr = 1, itemincr = 1;
    dir = AGGR_SUM_DIR_FRAMES;
    oper = AGGR_SUM_OPER_ADD;
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
                SUerror ("aggrsum", "bad argument for -vt option");
            continue;
        case -104:
            dictincr = opt_info.num;
            continue;
        case -105:
            itemincr = opt_info.num;
            continue;
        case -124:
            dir = AGGR_SUM_DIR_FRAMES;
            continue;
        case -125:
            dir = AGGR_SUM_DIR_ITEMS;
            continue;
        case -126:
            oper = AGGR_SUM_OPER_ADD;
            continue;
        case -127:
            oper = AGGR_SUM_OPER_MIN;
            continue;
        case -128:
            oper = AGGR_SUM_OPER_MAX;
            continue;
        case -999:
            SUwarnlevel++;
            continue;
        case '?':
            SUusage (0, "aggrsum", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "aggrsum", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;
    if (argc != 1)
        SUerror ("aggrsum", "no input file name specified");
    if (!(iafp = AGGRopen (argv[0], AGGR_RWMODE_RD, 1, TRUE)))
        SUerror ("aggrsum", "open failed");
    if (valtype == -1) {
        valtype = AGGR_TYPE_FLOAT;
        vallen = (
            iafp->hdr.vallen / AGGRtypelens[iafp->hdr.valtype]
        ) * AGGRtypelens[valtype];
    }
    if (!(oafp = AGGRcreate (
        onamep, 1, 1, iafp->hdr.keytype, valtype, classp,
        iafp->ad.keylen, vallen, dictincr, itemincr, 1, TRUE
    )))
        SUerror ("aggrsum", "create failed");
    if (AGGRsum (iafp, dir, oper, oafp) == -1)
        SUerror ("aggrsum", "sum failed");
    if (AGGRminmax (oafp, TRUE) == -1)
        SUerror ("aggrsum", "minmax failed");
    if (AGGRclose (oafp) == -1)
        SUerror ("aggrsum", "close failed");
    if (AGGRterm () == -1)
        SUerror ("aggrsum", "term failed");
    return 0;
}
