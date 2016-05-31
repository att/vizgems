#pragma prototyped

#include <ast.h>
#include <option.h>
#include <tok.h>
#include <swift.h>
#include <aggr.h>
#include <aggr_int.h>

static const char usage[] =
"[-1p1s1D?\n@(#)$Id: aggrmap (AT&T Labs Research) 2001-09-24 $\n]"
USAGE_LICENSE
"[+NAME?aggrmap - generate or search an AGGR map file]"
"[+DESCRIPTION?\baggrmap\b generates or searches AGGR map files."
" AGGR map files are ascii or binary files of pairs of keys."
" \baggrmap\b can convert map files between ascii and binary formats."
" In this case the map data is read from standard input and written"
" on standard output."
" \baggrmap\b can also search a map file; given a stream of keys, \baggrmap\b"
" can search either the first or second column of keys for matches."
" When a match is found, the key in the opposite column is emitted."
" In this case the map data is read from the file specified on the command"
" line."
" The key data is read from standard input and the matching keys are written on"
" standard output.]"
"[134:in?specifies the format of the input data."
" \basc\b indicates ascii format while \bbin\b binary."
" The default format is \basc\b."
"]:[(asc|bin):oneof]{[+asc][+bin]}"
"[135:out?specifies the format of the output data."
" \basc\b indicates ascii format while \bbin\b binary."
" The default format is \basc\b."
"]:[(asc|bin):oneof]{[+asc][+bin]}"
"[136:search1?specifies the format of the data in the map file to be searched."
" It also specifies that the keys read from standard input are to be searched"
" for in the first column of keys in the map file."
" \basc\b indicates ascii format while \bbin\b binary."
" The default format is \basc\b."
"]:[(asc|bin):oneof]{[+asc][+bin]}"
"[137:search2?specifies the format of the data in the map file to be searched."
" It also specifies that the keys read from standard input are to be searched"
" for in the second column of keys in the map file."
" \basc\b indicates ascii format while \bbin\b binary."
" The default format is \basc\b."
"]:[(asc|bin):oneof]{[+asc][+bin]}"
"[138:kt1?specifies the data type of the first key field in the map file."
" The main type name may be following by a colon and a number, specifying how"
" many elements of that type make up the key, e.g. \bint:5\b would mean that"
" the key is a sequence of 5 integers."
" Such a key should appear in the input stream as 5 integers separated by"
" colons: 10:22:3:1:1."
" The default type is \bstring\b."
"]:[(char|short|int|llong|float|double|string)[::n]]]"
"[139:kt2?specifies the data type of the second key field in the map file."
" The main type name may be following by a colon and a number, specifying how"
" many elements of that type make up the key, e.g. \bint:5\b would mean that"
" the key is a sequence of 5 integers."
" Such a key should appear in the input stream as 5 integers separated by"
" colons: 10:22:3:1:1."
" The default type is \bstring\b."
"]:[(char|short|int|llong|float|double|string)[::n]]]"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"\n"
"\n[ file ]\n"
"\n"
"[+SEE ALSO?\baggr\b(1), \baggr\b(3)]"
;

#define CONVERT 1
#define SEARCH1 2
#define SEARCH2 3

typedef struct kk_t {
    Dtlink_t link;
    struct kk_t *nextp;
} kk_t;

Dt_t *kkdict;

static Dtdisc_t kkdisc = {
    sizeof (Dtlink_t) + sizeof (struct kk_t *), -1, 0,
    NULL, NULL, NULL, NULL, NULL, NULL
};

static void insert (int, unsigned char *, int, unsigned char *, int);

int main (int argc, char **argv) {
    int norun;
    char inmode, outmode, searchmode;
    int keytype1, keylen1, keytype2, keylen2, keylen3, keylen4;
    int opmode;
    char *searchfile;
    char *line, *s, *avs[10];
    int avn;
    AGGRfile_t dummy1, dummy2, dummy3, dummy4;
    unsigned char *key1p, *key2p, *key3p, *key4p;
    int ret;
    kk_t *kkp;
    int l0, l1;
    Sfio_t *sfp;

    if (AGGRinit () == -1)
        SUerror ("aggrmap", "init failed");
    inmode = outmode = 'a';
    keytype1 = AGGR_TYPE_STRING;
    keylen1 = -1;
    keytype2 = AGGR_TYPE_STRING;
    keylen2 = -1;
    opmode = CONVERT;
    avn = -1;
    norun = 0;
    for (;;) {
        switch (optget (argv, usage)) {
        case -134:
            if (strcmp (opt_info.arg, "asc") == 0)
                inmode = 'a';
            else if (strcmp (opt_info.arg, "bin") == 0)
                inmode = 'b';
            continue;
        case -135:
            if (strcmp (opt_info.arg, "asc") == 0)
                outmode = 'a';
            else if (strcmp (opt_info.arg, "bin") == 0)
                outmode = 'b';
            continue;
        case -136:
            if (strcmp (opt_info.arg, "asc") == 0)
                searchmode = 'a';
            else if (strcmp (opt_info.arg, "bin") == 0)
                searchmode = 'b';
            opmode = SEARCH1;
            continue;
        case -137:
            if (strcmp (opt_info.arg, "asc") == 0)
                searchmode = 'a';
            else if (strcmp (opt_info.arg, "bin") == 0)
                searchmode = 'b';
            opmode = SEARCH2;
            continue;
        case -138:
            if (AGGRparsetype (opt_info.arg, &keytype1, &keylen1) == -1)
                SUerror ("aggrmap", "bad argument for -kt1 option");
            continue;
        case -139:
            if (AGGRparsetype (opt_info.arg, &keytype2, &keylen2) == -1)
                SUerror ("aggrmap", "bad argument for -kt2 option");
            continue;
        case -999:
            SUwarnlevel++;
            continue;
        case '?':
            SUusage (0, "aggrmap", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "aggrmap", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;
    if (opmode == SEARCH1 || opmode == SEARCH2)
        if (argc != 1)
            SUerror ("aggrmap", "map file argument missing");
        else
            searchfile = argv[0];
    sfsetbuf (sfstdin, NULL, 1048576);
    sfsetbuf (sfstdout, NULL, 1048576);
    dummy1.ad.keylen = keylen1;
    dummy1.hdr.keytype = keytype1;
    dummy2.ad.keylen = keylen2;
    dummy2.hdr.keytype = keytype2;
    if (keylen1 != -1)
        if (!(key1p = vmalloc (Vmheap, keylen1)))
            SUerror ("aggrmap", "vmalloc failed");
    if (keylen2 != -1)
        if (!(key2p = vmalloc (Vmheap, keylen2)))
            SUerror ("aggrmap", "vmalloc failed");
    if (opmode == SEARCH1 || opmode == SEARCH2) {
        if (opmode == SEARCH1) {
            kkdisc.size = (keylen1 == -1) ? 0 : keylen1;
            dummy3 = dummy1, dummy4 = dummy2;
            keylen3 = keylen1, keylen4 = keylen2;
        } else {
            kkdisc.size = (keylen2 == -1) ? 0 : keylen2;
            dummy3 = dummy2, dummy4 = dummy1;
            keylen3 = keylen2, keylen4 = keylen1;
        }
        if (keylen3 != -1)
            if (!(key3p = vmalloc (Vmheap, keylen3)))
                SUerror ("aggrmap", "vmalloc failed");
        if (!(kkdict = dtopen (&kkdisc, Dtset)))
            SUerror ("aggrmap", "cannot open dictionary");
        if (!(sfp = sfopen (NULL, searchfile, "r")))
            SUerror ("aggrmap", "cannot open %s", searchfile);
        switch (searchmode) {
        case 'a':
            while ((line = sfgetr (sfp, '\n', 1))) {
                if (
                    (avn = tokscan (line, &s, " %v ", avs, 10)) < 1 || avn > 2
                ) {
                    SUwarning (1, "aggrmap", "bad line");
                    continue;
                }
                if (_aggrdictscankey (&dummy1, avs[0], &key1p) == -1) {
                    SUwarning (1, "aggrmap", "bad key");
                    continue;
                }
                if (_aggrdictscankey (&dummy2, avs[1], &key2p) == -1) {
                    SUwarning (1, "aggrmap", "bad key");
                    continue;
                }
                insert (opmode, key1p, keylen1, key2p, keylen2);
            }
            break;
        case 'b':
            while (1) {
                if (keylen1 == -1) {
                    if (!(key1p = (unsigned char *) sfgetr (sfp, 0, 1))) {
                        if (sfvalue (sfp) != 0)
                            SUerror ("aggrmap", "bad entry");
                        else
                            break;
                    }
                } else {
                    if ((ret = sfread (sfp, key1p, keylen1)) == 0)
                        break;
                    else if (ret != keylen1)
                        SUerror ("aggrmap", "bad entry");
                }
                if (keylen2 == -1) {
                    if (!(key2p = (unsigned char *) sfgetr (sfp, 0, 1))) {
                        if (sfvalue (sfp) != 0)
                            SUerror ("aggrmap", "bad entry");
                        else
                            break;
                    }
                } else {
                    if ((ret = sfread (sfp, key2p, keylen2)) == 0)
                        break;
                    else if (ret != keylen2)
                        SUerror ("aggrmap", "bad entry");
                }
                SWAPKEY (key1p, keytype1, keylen1);
                SWAPKEY (key2p, keytype2, keylen2);
                insert (opmode, key1p, keylen1, key2p, keylen2);
            }
            break;
        }
        sfclose (sfp);
        switch (inmode) {
        case 'a':
            while ((line = sfgetr (sfstdin, '\n', 1))) {
                if ((avn = tokscan (line, &s, " %v ", avs, 10)) != 1) {
                    SUwarning (1, "aggrmap", "bad line");
                    continue;
                }
                if (_aggrdictscankey (&dummy3, avs[0], &key3p) == -1) {
                    SUwarning (1, "aggrmap", "bad key");
                    continue;
                }
                if (!(kkp = dtmatch (kkdict, key3p)))
                    continue;
                l0 = sizeof (kk_t);
                l1 = (keylen3 == -1) ? strlen ((char *) key3p) + 1 : keylen3;
                for (; kkp; kkp = kkp->nextp) {
                    key4p = (unsigned char *) kkp + l0 + l1;
                    switch (outmode) {
                    case 'a':
                        _aggrdictprintkey (&dummy4, sfstdout, key4p);
                        sfputc (sfstdout, '\n');
                        break;
                    case 'b':
                        if (keylen4 == -1)
                            sfputr (sfstdout, (char *) key4p, 0);
                        else {
                            SWAPKEY (key4p, keytype2, keylen4);
                            sfwrite (sfstdout, key4p, keylen4);
                        }
                        break;
                    }
                }
            }
            break;
        case 'b':
            while (1) {
                if (keylen3 == -1) {
                    if (!(key3p = (unsigned char *) sfgetr (sfstdin, 0, 1))) {
                        if (sfvalue (sfstdin) != 0)
                            SUerror ("aggrmap", "bad entry");
                        else
                            break;
                    }
                } else {
                    if ((ret = sfread (sfstdin, key3p, keylen3)) == 0)
                        break;
                    else if (ret != keylen3)
                        SUerror ("aggrmap", "bad entry");
                }
                SWAPKEY (key3p, keytype1, keylen3);
                if (!(kkp = dtmatch (kkdict, key3p)))
                    continue;
                l0 = sizeof (kk_t);
                l1 = (keylen3 == -1) ? strlen ((char *) key3p) + 1 : keylen3;
                for (; kkp; kkp = kkp->nextp) {
                    key4p = (unsigned char *) kkp + l0 + l1;
                    switch (outmode) {
                    case 'a':
                        _aggrdictprintkey (&dummy4, sfstdout, key4p);
                        sfputc (sfstdout, '\n');
                        break;
                    case 'b':
                        if (keylen4 == -1)
                            sfputr (sfstdout, (char *) key4p, 0);
                        else
                            sfwrite (sfstdout, key4p, keylen4);
                        break;
                    }
                }
            }
            break;
        }
        return 0;
    }
    switch (inmode) {
        case 'a':
            while ((line = sfgetr (sfstdin, '\n', 1))) {
                if (
                    (avn = tokscan (line, &s, " %v ", avs, 10)) < 1 || avn > 2
                ) {
                    SUwarning (1, "aggrmap", "bad line");
                    continue;
                }
                if (_aggrdictscankey (&dummy1, avs[0], &key1p) == -1) {
                    SUwarning (1, "aggrmap", "bad key");
                    continue;
                }
                if (_aggrdictscankey (&dummy2, avs[1], &key2p) == -1) {
                    SUwarning (1, "aggrmap", "bad key");
                    continue;
                }
                switch (outmode) {
                case 'a':
                    _aggrdictprintkey (&dummy1, sfstdout, key1p);
                    sfputc (sfstdout, ' ');
                    _aggrdictprintkey (&dummy2, sfstdout, key2p);
                    sfputc (sfstdout, '\n');
                    break;
                case 'b':
                    if (keylen1 == -1)
                        sfputr (sfstdout, (char *) key1p, 0);
                    else {
                        SWAPKEY (key1p, keytype1, keylen1);
                        sfwrite (sfstdout, key1p, keylen1);
                    }
                    if (keylen2 == -1)
                        sfputr (sfstdout, (char *) key2p, 0);
                    else {
                        SWAPKEY (key2p, keytype2, keylen2);
                        sfwrite (sfstdout, key2p, keylen2);
                    }
                    break;
                }
            }
            break;
        case 'b':
            while (1) {
                if (keylen1 == -1) {
                    if (!(key1p = (unsigned char *) sfgetr (sfstdin, 0, 1))) {
                        if (sfvalue (sfstdin) != 0)
                            SUerror ("aggrmap", "bad entry");
                        else
                            break;
                    }
                } else {
                    if ((ret = sfread (sfstdin, key1p, keylen1)) == 0)
                        break;
                    else if (ret != keylen1)
                        SUerror ("aggrmap", "bad entry");
                }
                if (keylen2 == -1) {
                    if (!(key2p = (unsigned char *) sfgetr (sfstdin, 0, 1))) {
                        if (sfvalue (sfstdin) != 0)
                            SUerror ("aggrmap", "bad entry");
                        else
                            break;
                    }
                } else {
                    if ((ret = sfread (sfstdin, key2p, keylen2)) == 0)
                        break;
                    else if (ret != keylen2)
                        SUerror ("aggrmap", "bad entry");
                }
                switch (outmode) {
                case 'a':
                    SWAPKEY (key1p, keytype1, keylen1);
                    _aggrdictprintkey (&dummy1, sfstdout, key1p);
                    sfputc (sfstdout, ' ');
                    SWAPKEY (key2p, keytype2, keylen2);
                    _aggrdictprintkey (&dummy2, sfstdout, key2p);
                    sfputc (sfstdout, '\n');
                    break;
                case 'b':
                    if (keylen1 == -1)
                        sfputr (sfstdout, (char *) key1p, 0);
                    else
                        sfwrite (sfstdout, key1p, keylen1);
                    if (keylen2 == -1)
                        sfputr (sfstdout, (char *) key2p, 0);
                    else
                        sfwrite (sfstdout, key2p, keylen2);
                    break;
                }
            }
            break;
        }
    return 0;
}

static void insert (
    int mode, unsigned char *k1p, int k1l, unsigned char *k2p, int k2l
) {
    kk_t *kkp, *kkmem;
    int l0, l1, l2;

    l0 = sizeof (kk_t);
    l1 = (k1l == -1) ? strlen ((char *) k1p) + 1 : k1l;
    l2 = (k2l == -1) ? strlen ((char *) k2p) + 1 : k2l;
    if (!(kkmem = vmalloc (Vmheap, l0 + l1 + l2)))
        SUerror ("aggrmap", "vmalloc failed");
    if (mode == SEARCH1) {
        memcpy ((char *) kkmem + l0, k1p, l1);
        memcpy ((char *) kkmem + l0 + l1, k2p, l2);
    } else {
        memcpy ((char *) kkmem + l0, k2p, l2);
        memcpy ((char *) kkmem + l0 + l2, k1p, l1);
    }
    kkmem->nextp = NULL;
    if (!(kkp = dtinsert (kkdict, kkmem)))
        SUerror ("aggrmap", "insert failed");
    else if (kkp != kkmem)
        kkmem->nextp = kkp->nextp, kkp->nextp = kkmem;
}
