#pragma prototyped

#include <ast.h>
#include <option.h>
#include <vmalloc.h>
#include <tok.h>
#include <swift.h>
#include <float.h>
#include <aggr.h>
#include <aggr_int.h>

static const char usage[] =
"[-1p1s1D?\n@(#)$Id: aggrload (AT&T Labs Research) 2001-09-24 $\n]"
USAGE_LICENSE
"[+NAME?aggrload - create an AGGR file from ascii data]"
"[+DESCRIPTION?\baggrload\b generates one or more AGGR files from ascii data."
" An AGGR file is a two dimensional array of numeric values."
" One of the dimensions uses a dictionary while the other one is indexed."
" The indexed dimension is referred to as `frames'."
" If no file is specified, data is read from standard input."
" By default the first item in each line is the AGGR key for all the"
" remaining data values on that line."
" \aaggrload\a will create a separate AGGR file for each column of values."
" An input line of the form: `abc 1 10 33'  will insert an item with key: abc"
" in each of the 3 output files."
" The item will have the value 1 in the first file,"
" 10 in the second, and 33 in the third."
" In the default case \aaggrload\a will place all items in a single frame,"
" frame #0."
" If the \a-fr\a option is specified, then each line must start with a numeric"
" value specifying the id of the frame."
" The input line: `10 abc 1 10 33' will work as before except that all 3 items"
" will be inserted in frame #10 of the corresponding output file.]"
"[100:o?specifies the names of the output files."
" There should be a file per column of data specified in the input stream"
" (except for the frame and key columns)."
" The default action is to create a single output file named \bdata.aggr\b."
" If there are multiple file names the argument must be quoted:"
" \a-o 'a.aggr b.aggr'."
"]:['file1 ...']"
"[101:cl?specifies a string describing the output dataset."
" Operations that combine multiple AGGR files will only work across files with"
" the same description."
" The default class is \baggrload\b."
"]:[class]"
"[102:kt?specifies the data type of the key fields."
" The main type name may be following by a colon and a number, specifying how"
" many elements of that type make up the key, e.g. \bint:5\b would mean that"
" the key is a sequence of 5 integers."
" Such a key should appear in the input stream as 5 integers separated by"
" colons: 10:22:3:1:1."
" The default type is \bstring\b."
"]:[(char|short|int|llong|float|double|string)[::n]]]"
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
"[106:ql?specifies the length of the queue for adding new items to the file."
" The default value is \b1,000,000\b."
" Higher values may improve performance when loading large data files since"
" they result in less frequent rearranging of the output files."
"]#[n]"
"[107:fr?specifies that the input file contains the frame id as the first"
" column on each line."
" The default action is to insert all values in frame #0."
"]"
"[108:avg?specifies how to handle data values when the same key (or key-frame)"
" appears multiple times."
" The default action is to sum these values."
" This option specifies that these values should be averaged."
"]"
"[145:fn?specifies a list of frame levels and names."
" The argument must be a sequence of triplets: frame level string."
" For example, the argument \b\"0 0 'abc' 1 0 'def'\"\b will set the level 0"
" labels for frames 0 and 1 to the corresponding strings."
"]:[fnames]"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"\n"
"\n[ file ]\n"
"\n"
"[+SEE ALSO?\baggr\b(1), \baggr\b(3)]"
;

int main (int argc, char **argv) {
    int norun;
    char *onamep, *classp;
    int keytype, keylen, valtype, vallen;
    int avgmode, hasframei;
    int dictincr, itemincr, queuen;
    AGGRfname_t *fnames;
    int fnamen;
    char *avs[10000];
    int avn, avi, avj;
    char *s;
    Sfio_t *fp;
    AGGRfile_t **oafps, *avgafp;
    int oafpn, oafpi;
    int fieldn;
    int count, rejcount;
    char *line;
    unsigned char *keyp, *valp;
    AGGRkv_t kv, avgkv;
    AGGRunit_t vu;
    int framei;
    int itemi;
    int *ip;
    void *datap;
    unsigned char *cp;
    int cl, vl;
    double val, minval, maxval;

    if (AGGRinit () == -1)
        SUerror ("aggrload", "init failed");
    onamep = "data.aggr";
    classp = "aggrload";
    keytype = AGGR_TYPE_STRING;
    keylen = -1;
    valtype = AGGR_TYPE_FLOAT, vallen = AGGRtypelens[valtype];
    avgmode = 0;
    hasframei = FALSE;
    dictincr = 1, itemincr = 1;
    queuen = 1000000;
    fnames = NULL, fnamen = 0;
    avn = -1;
    norun = 0;
    for (;;) {
        switch (optget (argv, usage)) {
        case -100:
            if ((avn = tokscan (opt_info.arg, &s, " %v ", avs, 10000)) < 1)
                SUerror ("aggrload", "bad argument for -o option");
            continue;
        case -101:
            classp = opt_info.arg;
            continue;
        case -102:
            if (AGGRparsetype (opt_info.arg, &keytype, &keylen) == -1)
                SUerror ("aggrload", "bad argument for -kt option");
            continue;
        case -103:
            if (AGGRparsetype (opt_info.arg, &valtype, &vallen) == -1)
                SUerror ("aggrload", "bad argument for -vt option");
            continue;
        case -104:
            dictincr = opt_info.num;
            continue;
        case -105:
            itemincr = opt_info.num;
            continue;
        case -106:
            queuen = opt_info.num;
            continue;
        case -107:
            hasframei = 1;
            continue;
        case -108:
            avgmode = 1;
            continue;
        case -145:
            if (AGGRparsefnames (opt_info.arg, &fnames, &fnamen) == -1)
                SUerror ("aggrload", "bad argument for -fn option");
            continue;
        case -999:
            SUwarnlevel++;
            continue;
        case '?':
            SUusage (0, "aggrload", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "aggrload", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;
    if (argc == 0)
        fp = sfstdin;
    else if (argc > 1)
        SUerror ("aggrload", "too many arguments");
    else if (!(fp = sfopen (NULL, argv[0], "r")))
        SUerror ("aggrload", "sfopen failed");
    kv.oper = avgkv.oper = AGGR_APPEND_OPER_ADD;
    vu.u.i = 1;
    if (avn == -1) {
        if (avgmode && !(avgafp = AGGRcreate (
            sfprints ("%s.counts", onamep), 1, 1, keytype, AGGR_TYPE_INT,
            "temp avg file", keylen, sizeof (int),
            dictincr, itemincr, queuen, FALSE
        )))
            SUerror ("aggrload", "create failed");
        if (!(oafps = vmresize (
            Vmheap, NULL, sizeof (AGGRfile_t *), VM_RSZERO
        )))
            SUerror ("aggrload", "vmresize failed");
        oafpn = 1;
        if (!(oafps[0] = AGGRcreate (
            onamep, 1, 1, keytype, valtype, classp,
            keylen, vallen, dictincr, itemincr, queuen, TRUE
        )))
            SUerror ("aggrload", "create failed");
        if (fnamen > 0) {
            if (AGGRfnames (oafps[0], fnames, fnamen) == -1)
            SUerror ("aggrload", "setfnames failed");
        }
    } else {
        if (avgmode && !(avgafp = AGGRcreate (
            sfprints ("%s.counts", avs[0]), 1, 1, keytype, AGGR_TYPE_INT,
            "temp avg file", keylen, sizeof (int),
            dictincr, itemincr, queuen, FALSE
        )))
            SUerror ("aggrload", "create failed");
        if (!(oafps = vmresize (
            Vmheap, NULL, avn * sizeof (AGGRfile_t *), VM_RSZERO
        )))
            SUerror ("aggrload", "vmresize failed");
        oafpn = avn;
        for (oafpi = 0; oafpi < oafpn; oafpi++) {
            if (!(oafps[oafpi] = AGGRcreate (
                avs[oafpi], 1, 1, keytype, valtype, classp,
                keylen, vallen, dictincr, itemincr, queuen, TRUE
            )))
                SUerror ("aggrload", "create failed");
            if (fnamen > 0) {
                if (AGGRfnames (oafps[oafpi], fnames, fnamen) == -1)
                SUerror ("aggrload", "setfnames failed");
            }
        }
    }
    if ((keylen = oafps[0]->ad.keylen) != -1)
        if (!(keyp = vmalloc (Vmheap, keylen)))
            SUerror ("aggrload", "vmalloc failed");
    if (!(valp = vmalloc (Vmheap, vallen)))
        SUerror ("aggrload", "vmalloc failed");
    fieldn = oafpn + 1 + (hasframei ? 1 : 0);
    count = 0, rejcount = 0;
    while ((line = sfgetr (fp, '\n', 1))) {
        count++;
        if ((avn = tokscan (line, &s, " %v ", avs, 10000)) != fieldn) {
            SUwarning (1, "aggrload", "bad line %d", count);
            rejcount++;
            continue;
        }
        avi = 0;
        if (hasframei)
            kv.framei = atoi (avs[avi++]);
        else
            kv.framei = 0;
        if (_aggrdictscankey (oafps[0], avs[avi], &keyp) == -1) {
            SUwarning (1, "aggrload", "bad key at line %d", count);
            rejcount++;
            continue;
        }
        kv.keyp = keyp;
        avi++;
        for (avj = avi; avj < avn; avj++) {
            if (_aggrscanval (oafps[0], avs[avj], &valp) == -1) {
                SUwarning (1, "aggrload", "bad val at line %d", count);
                rejcount++;
                continue;
            }
            kv.valp = valp;
            kv.itemi = -1;
            AGGRappend (oafps[avj - avi], &kv, TRUE);
            if (avgmode) {
                avgkv = kv;
                avgkv.valp = (unsigned char *) &vu.u.i;
                avgkv.itemi = -1;
                AGGRappend (avgafp, &avgkv, TRUE);
            }
        }
        if (count % 10000 == 0)
            SUmessage (
                1, "aggrload", "processed %d lines, dropped %d", count, rejcount
            );
    }
    if (avgmode) {
        for (oafpi = 0; oafpi < oafpn; oafpi++) {
            if (AGGRflush (oafps[oafpi]) == -1)
                SUerror ("aggrload", "flush failed");
            oafps[oafpi]->hdr.minval = DBL_MAX;
            oafps[oafpi]->hdr.maxval = DBL_MIN;
        }
        for (framei = 0; framei < avgafp->hdr.framen; framei++) {
            if (!(ip = AGGRget (avgafp, framei)))
                SUerror ("aggrload", "get failed");
            for (oafpi = 0; oafpi < oafpn; oafpi++) {
                minval = oafps[oafpi]->hdr.minval;
                maxval = oafps[oafpi]->hdr.maxval;
                if (!(datap = AGGRget (oafps[oafpi], framei)))
                    SUerror ("aggrload", "get failed");
                cp = datap;
                cl = AGGRtypelens[oafps[oafpi]->hdr.valtype];
                val = 0.0;
                switch (oafps[oafpi]->hdr.valtype) {
                case AGGR_TYPE_CHAR:
                    for (itemi = 0; itemi < avgafp->ad.itemn; itemi++) {
                        for (vl = 0; vl < oafps[oafpi]->hdr.vallen; vl += cl) {
                            if (ip[itemi] == 0)
                                continue;
                            val = (CVAL (&cp[vl]) /= ip[itemi]);
                        }
                        cp += oafps[oafpi]->hdr.vallen;
                    }
                    break;
                case AGGR_TYPE_SHORT:
                    for (itemi = 0; itemi < avgafp->ad.itemn; itemi++) {
                        for (vl = 0; vl < oafps[oafpi]->hdr.vallen; vl += cl) {
                            if (ip[itemi] == 0)
                                continue;
                            val = (SVAL (&cp[vl]) /= ip[itemi]);
                        }
                        cp += oafps[oafpi]->hdr.vallen;
                    }
                    break;
                case AGGR_TYPE_INT:
                    for (itemi = 0; itemi < avgafp->ad.itemn; itemi++) {
                        for (vl = 0; vl < oafps[oafpi]->hdr.vallen; vl += cl) {
                            if (ip[itemi] == 0)
                                continue;
                            val = (IVAL (&cp[vl]) /= ip[itemi]);
                        }
                        cp += oafps[oafpi]->hdr.vallen;
                    }
                    break;
                case AGGR_TYPE_LLONG:
                    for (itemi = 0; itemi < avgafp->ad.itemn; itemi++) {
                        for (vl = 0; vl < oafps[oafpi]->hdr.vallen; vl += cl) {
                            if (ip[itemi] == 0)
                                continue;
                            val = (LVAL (&cp[vl]) /= ip[itemi]);
                        }
                        cp += oafps[oafpi]->hdr.vallen;
                    }
                    break;
                case AGGR_TYPE_FLOAT:
                    for (itemi = 0; itemi < avgafp->ad.itemn; itemi++) {
                        for (vl = 0; vl < oafps[oafpi]->hdr.vallen; vl += cl) {
                            if (ip[itemi] == 0)
                                continue;
                            val = (FVAL (&cp[vl]) /= ip[itemi]);
                        }
                        cp += oafps[oafpi]->hdr.vallen;
                    }
                    break;
                case AGGR_TYPE_DOUBLE:
                    for (itemi = 0; itemi < avgafp->ad.itemn; itemi++) {
                        for (vl = 0; vl < oafps[oafpi]->hdr.vallen; vl += cl) {
                            if (ip[itemi] == 0)
                                continue;
                            val = (DVAL (&cp[vl]) /= ip[itemi]);
                        }
                        cp += oafps[oafpi]->hdr.vallen;
                    }
                    break;
                }
                if (minval > val)
                    minval = val;
                if (maxval < val)
                    maxval = val;
                if (AGGRrelease (oafps[oafpi]) == -1)
                    SUerror ("aggrload", "release failed");
                oafps[oafpi]->hdr.minval = minval;
                oafps[oafpi]->hdr.maxval = maxval;
            }
            if (AGGRrelease (avgafp) == -1)
                SUerror ("aggrload", "release failed");
        }
        if (avgmode && AGGRdestroy (avgafp) == -1)
            SUerror ("aggrload", "destroy failed");
    }
    SUmessage (
        rejcount > 0 ? 0 : 1,
        "aggrload", "processed %d lines, dropped %d", count, rejcount
    );
    for (oafpi = 0; oafpi < oafpn; oafpi++) {
        if (avgmode && AGGRminmax (oafps[oafpi], FALSE) == -1)
            SUerror ("aggrload", "minmax failed");
        if (AGGRclose (oafps[oafpi]) == -1)
            SUerror ("aggrload", "close failed");
    }
    if (AGGRterm () == -1)
        SUerror ("aggrload", "term failed");
    return 0;
}
