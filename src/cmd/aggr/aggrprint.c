#pragma prototyped

#include <ast.h>
#include <option.h>
#include <vmalloc.h>
#include <swift.h>
#include <aggr.h>
#include <aggr_int.h>

static const char usage[] =
"[-1p1s1D?\n@(#)$Id: aggrprint (AT&T Labs Research) 2001-09-24 $\n]"
USAGE_LICENSE
"[+NAME?aggrprint - print the contents of an AGGR file]"
"[+DESCRIPTION?\baggrprint\b prints the contents of an AGGR file.]"
"[116:dict?specifies that the keys of the dictionary should also be printed."
" The default action is not to print the dictionary."
"]"
"[117:nodata?specifies that no data should be printed."
" The default action is to print the data."
"]"
"[118:key?specifies that only the values for key \bkey\b should be printed."
" The key must be specified in ascii and must be quoted to avoid word"
" splitting."
" The default action is to print all entries."
"]:[key]"
"[119:item?specifies that only the values for item \bitem\b should be printed."
" The default action is to print all items."
"]#[item]"
"[120:frame?specifies that only the values for frame \bframe\b should be"
" printed."
" The default action is to print data for all frames."
"]#[frame]"
"[121:raw?specifies that the output should be in binary format."
" The default action is to print data in ascii."
"]"
"[122:alwayslocked?specifies that the input file should remain locked from the"
" time it is open until the time that it is closed."
" The default action is to lock the file while reading each frame, then unlock"
" the file and output the frame."
" The danger with this is that the input file may get modified halfway through"
" the printing operation."
" In that case the output may be a mixture of the old and the new data."
" The danger with keeping the file always locked is that if the output of this"
" tool is piped to a pager like \bmore\b, the input file may remain locked for"
" an extended period of time while the user pages through the output."
" If there is some automated process attempting to update the file, it will get"
" stuck."
"]"
"[123:V?increases the amount of info being output."
" The default action is to print just the values."
" The first \bV\b will add item ids to the output."
" The second \bV\b will also add key ids to the output."
"]"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"\n"
"\n[ file ]\n"
"\n"
"[+SEE ALSO?\baggr\b(1), \baggr\b(3)]"
;

int main (int argc, char **argv) {
    int norun;
    int printdict, printdata;
    char *onlykeyp;
    int onlyitemi, onlyframei;
    int verbose, rawmode, alwayslocked;
    AGGRfile_t *afp;
    int argi;
    unsigned char *keyp;

    if (AGGRinit () == -1)
        SUerror ("aggrprint", "init failed");
    printdict = FALSE;
    printdata = TRUE;
    onlykeyp = NULL;
    onlyitemi = -1;
    onlyframei = -1;
    verbose = 0;
    rawmode = FALSE;
    alwayslocked = FALSE;
    norun = 0;
    for (;;) {
        switch (optget (argv, usage)) {
        case -116:
            printdict = TRUE;
            continue;
        case -117:
            printdata = FALSE;
            continue;
        case -118:
            onlykeyp = opt_info.arg;
            continue;
        case -119:
            onlyitemi = opt_info.num;
            continue;
        case -120:
            onlyframei = opt_info.num;
            continue;
        case -121:
            rawmode = TRUE;
            continue;
        case -122:
            alwayslocked = TRUE;
            continue;
        case -123:
            verbose++;
            continue;
        case -999:
            SUwarnlevel++;
            continue;
        case '?':
            SUusage (0, "aggrprint", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "aggrprint", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;
    if (argc < 1)
        SUerror ("aggrprint", "no input file names specified");
    for (argi = 0; argi < argc; argi++) {
        if (!(afp = AGGRopen (argv[argi], AGGR_RWMODE_RD, 1, alwayslocked)))
            SUerror ("aggrprint", "open failed");
        if (onlykeyp) {
            if (afp->ad.keylen != -1) {
                if (!(keyp = vmalloc (Vmheap, afp->ad.keylen)))
                    SUerror ("aggrprint", "vmalloc failed");
                if (_aggrdictscankey (afp, onlykeyp, &keyp) == -1) {
                    SUwarning (1, "aggrprint", "bad key");
                    vmfree (Vmheap, keyp);
                    keyp = NULL;
                }
            } else
                keyp = (unsigned char *) onlykeyp;
            if (!keyp || (onlyitemi = AGGRlookup (afp, keyp)) == -1)
                onlyitemi = -1;
            if (afp->ad.keylen != -1)
                vmfree (Vmheap, keyp);
            if (onlyitemi == -1)
                continue;
        }
        if (AGGRprint (
            afp, printdict, printdata, onlyitemi, onlyframei, verbose, rawmode
        ) == -1)
            SUerror ("aggrprint", "print failed");
        AGGRclose (afp);
    }
    if (AGGRterm () == -1)
        SUerror ("aggrprint", "term failed");
    return 0;
}
