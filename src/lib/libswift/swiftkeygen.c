#pragma prototyped

#include <ast.h>
#include <option.h>
#include <swift.h>

static const char usage[] =
"[-1p1s1D?\n@(#)$Id: file (AT&T Labs Research) 1996-03-10 $\n]"
USAGE_LICENSE
"[+NAME?swiftkeygen - encode / decode a key]"
"[+DESCRIPTION?\bswiftkeygen\b encodes or decodes a key. "
"The encoded key can be used to encrypt strings using the program "
"swiftencode. "
"The default is to encode the string.]"
"[100:d?decode the string.]"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"\n"
"\n[ string ]\n"
"\n"
"[+SEE ALSO?\bswiftencode\b(1)]"
;

int main (int argc, char **argv) {
    int mode;
    int norun;
    unsigned char buf[10000];

    mode = 1;
    norun = 0;
    for (;;) {
        switch (optget (argv, usage)) {
        case -100:
            mode = 2;
            continue;
        case -999:
            SUwarnlevel++;
            continue;
        case '?':
            SUusage (0, "swiftkeygen", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "swiftkeygen", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;
    if (argc != 1) {
        SUusage (0, "swiftkeygen", optusage (NULL));
        return 1;
    }
    if (mode == 1)
        SUencodekey ((unsigned char *) argv[0], buf);
    else
        SUdecodekey ((unsigned char *) argv[0], buf);
    sfprintf (sfstdout, "%s\n", buf);
    return 0;
}
