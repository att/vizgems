#pragma prototyped

#include <ast.h>
#include <option.h>
#include <vmalloc.h>
#include <swift.h>
#include <aggr.h>
#include <aggr_int.h>

static const char usage[] =
"[-1p1s1D?\n@(#)$Id: aggrcat (AT&T Labs Research) 2001-09-24 $\n]"
USAGE_LICENSE
"[+NAME?aggrcat - concatenate AGGR files]"
"[+DESCRIPTION?\baggrcat\b concatenates multiple AGGR files into a new file."
" The input files should all be of the same class, key, and value types."
" The dictionary of the output file will be the union of all the input"
" dictionaries."
" Items from the input files are combined based on their dictionary keys."
" When a key-item combination exists in only some of the input files, its"
" value in all the other files is assumed to be zero.]"
"[100:o?specifies the name of the output file."
" The default name is \bcat.aggr\b."
"]:[file]"
"[101:cl?specifies a string describing the output dataset."
" The default class is \baggrload\b."
"]:[class]"
"[103:vt?specifies the data type of the value fields."
" The main type name may be following by a colon and a number, specifying how"
" many elements of that type make up the value, e.g. \bint:5\b would mean that"
" the value is a sequence of 5 integers."
" Such a value should appear in the input stream as 5 integers separated by"
" colons: 10:22:3:1:1."
" The default type is \bfloat\b."
" When data is converted from \bfloat\b or \bdouble\b to an integer format,"
" a roundoff error is accumulated."
" As each new item is processed, the current roundoff error is added to the"
" floating point value (up to but not including +/-1.0)."
" This value is then converted to an integer and its deviation from the"
" original floating point value is added to the roundoff error."
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
"\n[ file ]\n"
"\n"
"[+SEE ALSO?\baggr\b(1), \baggr\b(3)]"
;

int main (int argc, char **argv) {
    int norun;
    char *onamep, *classp;
    int valtype, vallen, dictincr, itemincr;
    AGGRfile_t **iafps, *oafp;
    int iafpn, iafpi;

    if (AGGRinit () == -1)
        SUerror ("aggrcat", "init failed");
    onamep = "cat.aggr";
    classp = "aggrcat";
    valtype = -1, vallen = -1;
    dictincr = 1, itemincr = 1;
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
                SUerror ("aggrcat", "bad argument for -vt option");
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
            SUusage (0, "aggrcat", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "aggrcat", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;
    if (argc < 1)
        SUerror ("aggrcat", "no input file names specified");
    if (!(iafps = vmalloc (Vmheap, sizeof (AGGRfile_t *) * argc)))
        SUerror ("aggrcat", "vmalloc failed");
    iafpn = argc;
    for (iafpi = 0; iafpi < iafpn; iafpi++)
        if (!(iafps[iafpi] = AGGRopen (argv[iafpi], AGGR_RWMODE_RD, 1, TRUE)))
            SUerror ("aggrcat", "open failed");
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
        SUerror ("aggrcat", "create failed");
    if (AGGRcat (iafps, iafpn, oafp) == -1)
        SUerror ("aggrcat", "cat failed");
    if (AGGRminmax (oafp, TRUE) == -1)
        SUerror ("aggrcat", "minmax failed");
    if (AGGRclose (oafp) == -1)
        SUerror ("aggrcat", "close failed");
    if (AGGRterm () == -1)
        SUerror ("aggrcat", "term failed");
    return 0;
}
