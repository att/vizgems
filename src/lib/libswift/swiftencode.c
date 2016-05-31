#pragma prototyped

#include <ast.h>
#include <option.h>
#include <swift.h>

static const char usage[] =
"[-1p1s1D?\n@(#)$Id: file (AT&T Labs Research) 1996-03-10 $\n]"
USAGE_LICENSE
"[+NAME?swiftencode - encode a string]"
"[+DESCRIPTION?\bswiftencode\b encodes a string using the specified key "
"and the HMAC algorithm. "
"The key must be encoded using the swiftkeygen program.]"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"\n"
"\n[ 'key' 'string' ]\n"
"\n"
"[+NOTES?both the key and the string must be quoted if they contain spaces "
"or other word separators.]"
"[+SEE ALSO?\bswiftkeygen\b(1)]"
;

int main (int argc, char **argv) {
    int norun;
    unsigned char dkey[10000], encbuf[40];

    norun = 0;
    for (;;) {
        switch (optget (argv, usage)) {
        case -999:
            SUwarnlevel++;
            continue;
        case '?':
            SUusage (0, "swiftencode", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "swiftencode", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;
    if (argc != 2) {
        SUusage (0, "swiftencode", optusage (NULL));
        return 1;
    }
    SUdecodekey ((unsigned char *) argv[0], (unsigned char *) dkey);
    SUhmac (
        (unsigned char *) argv[1], strlen (argv[1]),
        dkey, strlen ((char *) dkey), encbuf
    );
    sfprintf (sfstdout, "%s\n", encbuf);
    return 0;
}
