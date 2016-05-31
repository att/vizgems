#pragma prototyped

#include <ast.h>
#include <option.h>
#include <tok.h>
#include <swift.h>
#include <aggr.h>
#include <aggr_int.h>

static const char usage[] =
"[-1p1s1D?\n@(#)$Id: aggrkeys (AT&T Labs Research) 2001-09-24 $\n]"
USAGE_LICENSE
"[+NAME?aggrkeys - convert AGGR keys between binary and ascii formats]"
"[+DESCRIPTION?\baggrkeys\b converts AGGR keys between binary and ascii"
" formats.]"
"[134:in?specifies the format of the input data."
" \basc\b indicates ascii format while \bbin\b binary."
" The default format is \basc\b."
"]:[(asc|bin):oneof]{[+asc][+bin]}"
"[135:out?specifies the format of the output data."
" \basc\b indicates ascii format while \bbin\b binary."
" The default format is \basc\b."
"]:[(asc|bin):oneof]{[+asc][+bin]}"
"[138:kt?specifies the data type of the keys."
" The main type name may be following by a colon and a number, specifying how"
" many elements of that type make up the key, e.g. \bint:5\b would mean that"
" the key is a sequence of 5 integers."
" Such a key should appear in the input stream as 5 integers separated by"
" colons: 10:22:3:1:1."
" The default type is \bstring\b."
"]:[(char|short|int|llong|float|double|string)[::n]]]"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"[+SEE ALSO?\baggr\b(1), \baggr\b(3)]"
;

int main (int argc, char **argv) {
    int norun;
    char inmode, outmode;
    int keytype, keylen;
    char *line, *s, *avs[10];
    AGGRfile_t dummy;
    unsigned char *keyp;
    int ret;

    if (AGGRinit () == -1)
        SUerror ("aggrkeys", "init failed");
    inmode = outmode = 'a';
    keytype = AGGR_TYPE_STRING;
    keylen = -1;
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
        case -138:
            if (AGGRparsetype (opt_info.arg, &keytype, &keylen) == -1)
                SUerror ("aggrkeys", "bad argument for -kt option");
            continue;
        case -999:
            SUwarnlevel++;
            continue;
        case '?':
            SUusage (0, "aggrkeys", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "aggrkeys", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;
    sfsetbuf (sfstdin, NULL, 1048576);
    sfsetbuf (sfstdout, NULL, 1048576);
    dummy.ad.keylen = keylen;
    dummy.hdr.keytype = keytype;
    if (keylen != -1)
        if (!(keyp = vmalloc (Vmheap, keylen)))
            SUerror ("aggrkeys", "vmalloc failed");
    switch (inmode) {
    case 'a':
        while ((line = sfgetr (sfstdin, '\n', 1))) {
            if (tokscan (line, &s, " %v ", avs, 10) != 1) {
                SUwarning (1, "aggrkeys", "bad line");
                continue;
            }
            if (_aggrdictscankey (&dummy, avs[0], &keyp) == -1) {
                SUwarning (1, "aggrkeys", "bad key");
                continue;
            }
            switch (outmode) {
            case 'a':
                _aggrdictprintkey (&dummy, sfstdout, keyp);
                sfputc (sfstdout, '\n');
                break;
            case 'b':
                if (keylen == -1)
                    sfputr (sfstdout, (char *) keyp, 0);
                else {
                    SWAPKEY (keyp, keytype, keylen);
                    sfwrite (sfstdout, keyp, keylen);
                }
                break;
            }
        }
        break;
    case 'b':
        while (1) {
            if (keylen == -1) {
                if (!(keyp = (unsigned char *) sfgetr (sfstdin, 0, 1))) {
                    if (sfvalue (sfstdin) != 0)
                        SUerror ("aggrkeys", "bad entry");
                    else
                        break;
                }
            } else {
                if ((ret = sfread (sfstdin, keyp, keylen)) == 0)
                    break;
                else if (ret != keylen)
                    SUerror ("aggrkeys", "bad entry");
            }
            switch (outmode) {
            case 'a':
                SWAPKEY (keyp, keytype, keylen);
                _aggrdictprintkey (&dummy, sfstdout, keyp);
                sfputc (sfstdout, '\n');
                break;
            case 'b':
                if (keylen == -1)
                    sfputr (sfstdout, (char *) keyp, 0);
                else
                    sfwrite (sfstdout, keyp, keylen);
                break;
            }
        }
        break;
    }
    return 0;
}
