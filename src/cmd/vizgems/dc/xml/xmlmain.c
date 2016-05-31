#pragma prototyped

#include <ast.h>
#include <option.h>
#include <vmalloc.h>
#include <swift.h>
#include "xml.h"

static const char usage[] =
"[-1p1?\n@(#)$Id: vgxml (AT&T Labs Research) 2006-03-09 $\n]"
USAGE_LICENSE
"[+NAME?vgxml - operate on XML files]"
"[+DESCRIPTION?\bvgxml\b operates on XML files."
" \bvgxml\b reads XML data and outputs their contents in various formats."
" The XML data can be specified as a single quoted argument, or can be read"
" from standard input."
" The default format is pretty-printed ascii."
"]"
"[100:iws?specifies that leading and trailing white space (space, tab, NL)"
" is to be filtered out."
" The default is to exactly preserve all spaces."
"]"
"[101:topmark?specifies the name of the top mark."
" The XML tags in the data form a tree under this mark."
"]:[topmark]"
"[200:ascii?specifies that the contents of the XML file are to be printed"
" out as pretty-printed ASCII."
" This is the default."
"]"
"[201:ksh?specifies that the contents of the XML file should be printed out"
" as a KSH compound variable."
"]"
"[202:strict?specifies that the contents of the XML file follow a stricter"
" interpretation."
"]"
"[203:flatten?specifies that the contents of the XML file should be printed out"
" as a KSH compound variable, with each record flattened with respect to the"
" tag argument."
"]:[subtag]"
"[204:flatten2?specifies that the contents of the XML file should be printed"
" out"
" as a KSH compound variable, with each record flattened with respect to the"
" tag argument."
" The output of this option can be read into the shell using read -C."
"]:[subtag]"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"\n"
"\n[ QUOTED-XML-string ]\n"
"\n"
;

#define MODE_ASCII    1
#define MODE_KSH      2
#define MODE_FLATTEN  3
#define MODE_FLATTEN2 4

int main (int argc, char **argv) {
    int norun;
    int iwsflag, sflag, mode;
    char *topmark, *flattag;
    Vmalloc_t *vm;
    int n;

    XMLnode_t *np;

    iwsflag = FALSE, sflag = FALSE;
    mode = MODE_ASCII;
    topmark = "__TOP_TAG__";
    flattag = "";
    norun = 0;
    for (;;) {
        switch (optget (argv, usage)) {
        case -100:
            iwsflag = TRUE;
            continue;
        case -101:
            topmark = opt_info.arg;
            continue;
        case -200:
            mode = MODE_ASCII;
            continue;
        case -201:
            iwsflag = TRUE;
            sflag = TRUE;
            mode = MODE_KSH;
            continue;
        case -202:
            iwsflag = TRUE;
            sflag = TRUE;
            continue;
        case -203:
            iwsflag = TRUE;
            sflag = TRUE;
            mode = MODE_FLATTEN;
            flattag = opt_info.arg;
            continue;
        case -204:
            iwsflag = TRUE;
            sflag = TRUE;
            mode = MODE_FLATTEN2;
            flattag = opt_info.arg;
            continue;
        case -999:
            SUwarnlevel++;
            continue;
        case '?':
            SUusage (0, "vgxml", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "vgxml", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;
    if (argc > 1)
        SUerror ("vgxml", "too many arguments");

    if (!(vm = vmopen (Vmdcsbrk, Vmbest, 0)))
        SUerror ("vgxml", "cannot create region");

    if (mode == MODE_FLATTEN || mode == MODE_FLATTEN2) {
        n = 0;
        while ((np = XMLflattenread (sfstdin, iwsflag, vm))) {
            XMLflattenwrite (
                sfstdout, np, flattag, (mode == MODE_FLATTEN) ? TRUE : FALSE
            );
            if (++n % 1000 == 0) {
                vmclose (vm);
                if (!(vm = vmopen (Vmdcsbrk, Vmbest, 0)))
                    SUerror ("vgxml", "cannot create region");
            }
        }
        return 0;
    }

    if (argc == 1)
        np = XMLreadstring (argv[0], topmark, iwsflag, vm);
    else
        np = XMLreadfile (sfstdin, topmark, iwsflag, vm);

    if (!np)
        SUerror ("vgxml", "cannot parse XML string");

    switch (mode) {
    case MODE_ASCII:
        XMLwriteascii (sfstdout, np, 0);
        break;
    case MODE_KSH:
        XMLwriteksh (sfstdout, np, 0, sflag);
        break;
    }

    return 0;
}
