#pragma prototyped

#include <ast.h>
#include <option.h>
#include <tok.h>
#include <vmalloc.h>
#include <swift.h>
#include <aggr.h>
#include <aggr_int.h>

static const char usage[] =
"[-1p1s1D?\n@(#)$Id: aggrreaggr (AT&T Labs Research) 2001-09-24 $\n]"
USAGE_LICENSE
"[+NAME?aggrreaggr - re-aggregate AGGR data]"
"[+DESCRIPTION?\baggrreaggr\b aggregates data from an AGGR input file"
" and writes the results in a new AGGR file.]"
"[100:o?specifies the name of the output file."
" The default name is \breaggr.aggr\b."
"]:[file]"
"[101:cl?specifies a string describing the output dataset."
" Operations that combine multiple AGGR files will only work across files with"
" the same description."
" The default class is \baggrreaggr\b."
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
"[114:ascmap?specifies the file name to read keys from in ascii form."
" There is no default; either this or the \bbinmap\b option must be specified."
" The file name may be \b-\b to have the data read from standard input."
" The format of this data should be lines containing the in and out keys"
" separated by a space."
" Quoting may be used for keys that have spaces."
"]:[file]"
"[115:binmap?specifies the file name to read keys from in binary form."
" There is no default; either this or the \bascmap\b option must be specified."
" The file name may be \b-\b to have the data read from standard input."
"]:[file]"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"\n"
"\n[ file ]\n"
"\n"
"[+SEE ALSO?\baggr\b(1), \baggr\b(3)]"
;

static unsigned char *copy (AGGRfile_t *, unsigned char *);

int main (int argc, char **argv) {
    int norun;
    char *mapnamep, mapmode, *onamep, *classp;
    int keytype, keylen, valtype, vallen, dictincr, itemincr;
    AGGRfile_t *iafp, *oafp;
    char file[PATH_MAX];
    Sfio_t *fp;
    AGGRreaggr_t *reaggrs;
    int reaggrn, reaggrl;
    int count, rejcount;
    char *line, *s;
    unsigned char *ikeyp, *okeyp;
    char *avs[10];
    int avn;
    int ret;

    if (AGGRinit () == -1)
        SUerror ("aggrreaggr", "init failed");
    mapnamep = NULL, mapmode = 0;
    onamep = "reaggr.aggr";
    classp = "aggrreaggr";
    keytype = AGGR_TYPE_STRING;
    keylen = -1;
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
        case -102:
            if (AGGRparsetype (opt_info.arg, &keytype, &keylen) == -1)
                SUerror ("aggrreaggr", "bad argument for -kt option");
            continue;
        case -103:
            if (AGGRparsetype (opt_info.arg, &valtype, &vallen) == -1)
                SUerror ("aggrreaggr", "bad argument for -vt option");
            continue;
        case -104:
            dictincr = opt_info.num;
            continue;
        case -105:
            itemincr = opt_info.num;
            continue;
        case -114:
            mapnamep = opt_info.arg;
            mapmode = 'a';
            continue;
        case -115:
            mapnamep = opt_info.arg;
            mapmode = 'b';
            continue;
        case -999:
            SUwarnlevel++;
            continue;
        case '?':
            SUusage (0, "aggrreaggr", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "aggrreaggr", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;
    if (argc != 1)
        SUerror ("aggrreaggr", "no input file name specified");
    if (!mapnamep)
        SUerror ("aggrreaggr", "no map file name specified");
    if (!(iafp = AGGRopen (argv[0], AGGR_RWMODE_RD, 1, TRUE)))
        SUerror ("aggrreaggr", "open failed");
    if (valtype == -1) {
        valtype = AGGR_TYPE_FLOAT;
        vallen = (
            iafp->hdr.vallen / AGGRtypelens[iafp->hdr.valtype]
        ) * AGGRtypelens[valtype];
    }
    if (!(oafp = AGGRcreate (
        onamep, 1, 1, keytype, valtype, classp,
        keylen, vallen, dictincr, itemincr, 1, TRUE
    )))
        SUerror ("aggrreaggr", "create failed");
    if (strcmp (mapnamep, "-") == 0)
        fp = sfstdin;
    else {
        if (access (mapnamep, R_OK) == 0)
            strcpy (file, mapnamep);
        else if (!pathaccess (
            getenv ("PATH"), "../lib/aggr", mapnamep, R_OK, file, PATH_MAX
        ))
            SUerror ("aggrreaggr", "cannot find map file");
        if (!(fp = sfopen (NULL, file, "r")))
            SUerror ("aggrreaggr", "sfopen failed");
    }
    sfsetbuf (fp, NULL, 1048576);
    if (iafp->ad.keylen != -1)
        if (!(ikeyp = vmalloc (Vmheap, iafp->ad.keylen)))
            SUerror ("aggrreaggr", "vmalloc failed");
    if (oafp->ad.keylen != -1)
        if (!(okeyp = vmalloc (Vmheap, oafp->ad.keylen)))
            SUerror ("aggrreaggr", "vmalloc failed");
    if (!(reaggrs = vmalloc (Vmheap, 100 * sizeof (AGGRreaggr_t))))
        SUerror ("aggrreaggr", "vmalloc failed");
    reaggrn = 100, reaggrl = 0;
    count = 0, rejcount = 0;
    switch (mapmode) {
    case 'a':
        while ((line = sfgetr (fp, '\n', 1))) {
            count++;
            if ((avn = tokscan (line, &s, " %v ", avs, 10)) < 1 || avn > 2) {
                SUwarning (1, "aggrreaggr", "bad line %d", count);
                rejcount++;
                continue;
            }
            if (reaggrl >= reaggrn) {
                if (!(reaggrs = vmresize (
                    Vmheap, reaggrs, reaggrn * 2 * sizeof (AGGRreaggr_t),
                    VM_RSCOPY
                )))
                    SUerror ("aggrreaggr", "vmresize failed");
                reaggrn *= 2;
            }
            if (_aggrdictscankey (iafp, avs[0], &ikeyp) == -1) {
                SUwarning (1, "aggrreaggr", "bad key at line %d", count);
                rejcount++;
                continue;
            }
            reaggrs[reaggrl].okeyp = copy (iafp, ikeyp);
            if (_aggrdictscankey (oafp, avs[1], &okeyp) == -1) {
                SUwarning (1, "aggrreaggr", "bad key at line %d", count);
                rejcount++;
                continue;
            }
            reaggrs[reaggrl].nkeyp = copy (oafp, okeyp);
            if (avn == 3)
                reaggrs[reaggrl].nitemi = atoi (avs[2]);
            else
                reaggrs[reaggrl].nitemi = -1;
            reaggrl++;
            if (count % 10000 == 0)
                SUmessage (
                    1, "aggrreaggr", "processed %d lines, dropped %d",
                    count, rejcount
                );
        }
        break;
    case 'b':
        while (1) {
            if (reaggrl >= reaggrn) {
                if (!(reaggrs = vmresize (
                    Vmheap, reaggrs, reaggrn * 2 * sizeof (AGGRreaggr_t),
                    VM_RSCOPY
                )))
                    SUerror ("aggrreaggr", "vmresize failed");
                reaggrn *= 2;
            }
            if (iafp->ad.keylen == -1) {
                if (!(ikeyp = (unsigned char *) sfgetr (fp, 0, 1))) {
                    if (sfvalue (fp) != 0)
                        SUerror ("aggrreaggr", "bad entry %d", reaggrl);
                    else
                        break;
                }
            } else {
                if ((ret = sfread (fp, ikeyp, iafp->ad.keylen)) == 0)
                    break;
                else if (ret != iafp->ad.keylen)
                    SUerror ("aggrreaggr", "bad entry %d", reaggrl);
                SWAPKEY (ikeyp, iafp->hdr.keytype, iafp->ad.keylen);
            }
            reaggrs[reaggrl].okeyp = copy (iafp, ikeyp);
            if (oafp->ad.keylen == -1) {
                if (!(okeyp = (unsigned char *) sfgetr (fp, 0, 1))) {
                    if (sfvalue (fp) != 0)
                        SUerror ("aggrreaggr", "bad entry %d", reaggrl);
                    else
                        break;
                }
            } else {
                if ((ret = sfread (fp, okeyp, oafp->ad.keylen)) == 0)
                    break;
                else if (ret != oafp->ad.keylen)
                    SUerror ("aggrreaggr", "bad entry %d", reaggrl);
            }
            reaggrs[reaggrl].nkeyp = copy (oafp, okeyp);
            count++;
            reaggrs[reaggrl].nitemi = -1;
            reaggrl++;
            if (count % 10000 == 0)
                SUmessage (
                    1, "aggrreaggr", "processed %d lines, dropped %d",
                    count, rejcount
                );
        }
        break;
    }
    SUmessage (
        rejcount > 0 ? 0 : 1,
        "aggrreaggr", "processed %d lines, dropped %d", count, rejcount
    );
    if (AGGRreaggr (iafp, reaggrs, reaggrl, oafp) == -1)
        SUerror ("aggrreaggr", "reaggr failed");
    if (AGGRminmax (oafp, TRUE) == -1)
        SUerror ("aggrreaggr", "minmax failed");
    if (AGGRclose (oafp) == -1)
        SUerror ("aggrreaggr", "close failed");
    if (AGGRterm () == -1)
        SUerror ("aggrreaggr", "term failed");
    return 0;
}

static unsigned char *copy (AGGRfile_t *afp, unsigned char *ikeyp) {
    int len;
    unsigned char *okeyp;

    if (afp->ad.keylen == -1)
        len = strlen ((char *) ikeyp) + 1;
    else
        len = afp->ad.keylen;
    if (!(okeyp = vmalloc (Vmheap, len)))
        SUerror ("aggrreaggr", "vmalloc failed");
    memcpy (okeyp, ikeyp, len);
    return okeyp;
}
