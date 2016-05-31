#pragma prototyped

#include <ast.h>
#include <option.h>
#include <tok.h>
#include <vmalloc.h>
#include <swift.h>
#include <aggr.h>
#include <aggr_int.h>

static const char usage[] =
"[-1p1s1D?\n@(#)$Id: aggrextract (AT&T Labs Research) 2001-09-24 $\n]"
USAGE_LICENSE
"[+NAME?aggrextract - extract a subset of items from an AGGR file]"
"[+DESCRIPTION?\baggrextract\b extracts specific items from an input"
" AGGR file and copies them to a new AGGR file.]"
"[100:o?specifies the name of the output file."
" The default name is \bextract.aggr\b."
"]:[file]"
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
"[114:asckeys?specifies the file name to read keys from in ascii form."
" There is no default; either this or the \bbinkeys\b option must be specified."
" The file name may be \b-\b to have the data read from standard input."
"]:[file]"
"[115:binkeys?specifies the file name to read keys from in binary form."
" There is no default; either this or the \basckeys\b option must be specified."
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
    char *keynamep, keymode, *onamep;
    int valtype, vallen, dictincr, itemincr;
    AGGRfile_t *iafp, *oafp;
    char file[PATH_MAX];
    Sfio_t *fp;
    AGGRextract_t *extracts;
    int extractn, extractl;
    int count, rejcount;
    char *line, *s;
    unsigned char *keyp;
    int keylen;
    char *avs[10];
    int avn;
    int ret;

    if (AGGRinit () == -1)
        SUerror ("aggrextract", "init failed");
    keynamep = NULL, keymode = 0;
    onamep = "extract.aggr";
    valtype = -1, vallen = -1;
    dictincr = 1, itemincr = 1;
    norun = 0;
    for (;;) {
        switch (optget (argv, usage)) {
        case -100:
            onamep = opt_info.arg;
            continue;
        case -103:
            if (AGGRparsetype (opt_info.arg, &valtype, &vallen) == -1)
                SUerror ("aggrextract", "bad argument for -vt option");
            continue;
        case -104:
            dictincr = opt_info.num;
            continue;
        case -105:
            itemincr = opt_info.num;
            continue;
        case -114:
            keynamep = opt_info.arg;
            keymode = 'a';
            continue;
        case -115:
            keynamep = opt_info.arg;
            keymode = 'b';
            continue;
        case -999:
            SUwarnlevel++;
            continue;
        case '?':
            SUusage (0, "aggrextract", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "aggrextract", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;
    if (argc != 1)
        SUerror ("aggrextract", "no input file name specified");
    if (!keynamep)
        SUerror ("aggrextract", "no map file name specified");
    if (!(iafp = AGGRopen (argv[0], AGGR_RWMODE_RD, 1, TRUE)))
        SUerror ("aggrextract", "open failed");
    if (valtype == -1) {
        valtype = AGGR_TYPE_FLOAT;
        vallen = (
            iafp->hdr.vallen / AGGRtypelens[iafp->hdr.valtype]
        ) * AGGRtypelens[valtype];
    }
    if (!(oafp = AGGRcreate (
        onamep, 1, 1, iafp->hdr.keytype, valtype, iafp->hdr.class,
        iafp->ad.keylen, vallen, dictincr, itemincr, 1, TRUE
    )))
        SUerror ("aggrextract", "create failed");
    if (strcmp (keynamep, "-") == 0)
        fp = sfstdin;
    else {
        if (access (keynamep, R_OK) == 0)
            strcpy (file, keynamep);
        else if (!pathaccess (
            getenv ("PATH"), "../lib/aggr", keynamep, R_OK, file, PATH_MAX
        ))
            SUerror ("aggrextract", "cannot find map file");
        if (!(fp = sfopen (NULL, file, "r")))
            SUerror ("aggrextract", "sfopen failed");
    }
    sfsetbuf (fp, NULL, 1048576);
    if ((keylen = iafp->ad.keylen) != -1)
        if (!(keyp = vmalloc (Vmheap, keylen)))
            SUerror ("aggrextract", "vmalloc failed");
    if (!(extracts = vmalloc (Vmheap, 100 * sizeof (AGGRextract_t))))
        SUerror ("aggrextract", "vmalloc failed");
    extractn = 100, extractl = 0;
    count = 0, rejcount = 0;
    switch (keymode) {
    case 'a':
        while ((line = sfgetr (fp, '\n', 1))) {
            count++;
            if ((avn = tokscan (line, &s, " %v ", avs, 10)) < 1 || avn > 2) {
                SUwarning (1, "aggrextract", "bad line %d", count);
                rejcount++;
                continue;
            }
            if (extractl >= extractn) {
                if (!(extracts = vmresize (
                    Vmheap, extracts, (extractn + 100) * sizeof (AGGRextract_t),
                    VM_RSCOPY
                )))
                    SUerror ("aggrextract", "vmresize failed");
                extractn += 100;
            }
            if (_aggrdictscankey (iafp, avs[0], &keyp) == -1) {
                SUwarning (1, "aggrextract", "bad key at line %d", count);
                rejcount++;
                continue;
            }
            extracts[extractl].keyp = copy (iafp, keyp);
            if (avn == 2)
                extracts[extractl].nitemi = atoi (avs[1]);
            else
                extracts[extractl].nitemi = -1;
            extractl++;
            if (count % 10000 == 0)
                SUmessage (
                    1, "aggrextract", "processed %d lines, dropped %d",
                    count, rejcount
                );
        }
        break;
    case 'b':
        while (1) {
            if (extractl >= extractn) {
                if (!(extracts = vmresize (
                    Vmheap, extracts, (extractn + 100) * sizeof (AGGRextract_t),
                    VM_RSCOPY
                )))
                    SUerror ("aggrextract", "vmresize failed");
                extractn += 100;
            }
            if (keylen == -1) {
                if (!(keyp = (unsigned char *) sfgetr (fp, 0, 1))) {
                    if (sfvalue (fp) != 0)
                        SUerror ("aggrextract", "bad entry %d", extractl);
                    else
                        break;
                }
            } else {
                if ((ret = sfread (fp, keyp, keylen)) == 0)
                    break;
                else if (ret != keylen)
                    SUerror ("aggrreaggr", "bad entry %d", extractl);
                SWAPKEY (keyp, iafp->hdr.keytype, iafp->ad.keylen);
            }
            extracts[extractl].keyp = copy (iafp, keyp);
            count++;
            extracts[extractl].nitemi = -1;
            extractl++;
            if (count % 10000 == 0)
                SUmessage (
                    1, "aggrextract", "processed %d lines, dropped %d",
                    count, rejcount
                );
        }
        break;
    }
    SUmessage (
        rejcount > 0 ? 0 : 1,
        "aggrextract", "processed %d lines, dropped %d", count, rejcount
    );
    if (AGGRextract (iafp, extracts, extractl, oafp) == -1)
        SUerror ("aggrextract", "extract failed");
    if (AGGRminmax (oafp, TRUE) == -1)
        SUerror ("aggrextract", "minmax failed");
    if (AGGRclose (oafp) == -1)
        SUerror ("aggrextract", "close failed");
    if (AGGRterm () == -1)
        SUerror ("aggrextract", "term failed");
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
        SUerror ("aggrextract", "vmalloc failed");
    memcpy (okeyp, ikeyp, len);
    return okeyp;
}
