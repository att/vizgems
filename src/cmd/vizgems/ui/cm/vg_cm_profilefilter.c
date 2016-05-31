#pragma prototyped

#include <ast.h>
#include <option.h>
#include <vmalloc.h>
#include <swift.h>
#include <dds.h>
#define VG_DEFS_MAIN
#include "vg_hdr.h"
#include "sl_inv_nodeattr.c"
#define VG_STATMAP_MAIN
#include "vg_statmap.c"

static const char usage[] =
"[-1p1?\n@(#)$Id: vg_cm_profilefilter (AT&T Labs Research) 2009-06-11 $\n]"
USAGE_LICENSE
"[+NAME?vg_cm_profilefilter - filter tool for profile records]"
"[+DESCRIPTION?\bvg_cm_profilefilter\b a profile record at a time, attempts"
" to match the statistic id to a human readable name, and outputs the record"
" with the human readable name included in the statistic id field."
"]"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"[+SEE ALSO?\bdds\b(1), \bdds\b(3)]"
;

#define SZ_level VG_inv_node_level_L
#define SZ_id VG_inv_node_id_L

int main (int argc, char **argv) {
    int norun;
    sl_inv_nodeattr_t ina, *inap;
    statmap_t *smp;
    int smi, nattri;
    int noprocessing;
    char *line, *s1, *s2, *s3, *s4, *s5;
    char level[SZ_level], id[SZ_id];

    noprocessing = FALSE;
    norun = 0;
    for (;;) {
        switch (optget (argv, usage)) {
        case -999:
            SUwarnlevel++;
            continue;
        case '?':
            SUusage (0, "vg_cm_profilefilter", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "vg_cm_profilefilter", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;

    DDSinit ();
    memset (level, 0, SZ_level);
    strcpy (level, getenv ("DEFAULTLEVEL"));
    sl_inv_nodeattropen (getenv ("INVNODEATTRFILE"));
    if (statmapload (getenv ("STATMAPFILE"), FALSE, FALSE, TRUE) == -1) {
        SUwarning (0, "vg_cm_profilefilter", "cannot load statmap file");
        noprocessing = TRUE;
    }

    while ((line = sfgetr (sfstdin, '\n', SF_STRING))) {
        if (noprocessing) {
            sfputr (sfstdout, line, '\n');
            continue;
        }
        if (
            !(s1 = strchr (line, '|')) || !(s2 = strchr (s1 + 1, '|')) ||
            !(s3 = strchr (s2 + 1, '|')) || !(s4 = strchr (s3 + 1, '|'))
        ) {
            SUwarning (0, "vg_cm_profilefilter", "bad record: %s", line);
            sfputr (sfstdout, line, '\n');
            continue;
        }
        *s1++ = 0, *s2++ = 0, *s3++ = 0, *s4++ = 0;
        if (!(s5 = strchr (s1, '.')) || (smi = statmapfind (s1)) == -1) {
            sfprintf (sfstdout, "%s|%s|%s|%s|%s\n", line, s1, s2, s3, s4);
            continue;
        }
        s5++;
        smp = &statmaps[smi];
        memset (id, 0, SZ_id);
        strcpy (id, line);
        for (nattri = 0; nattri < smp->nattrn; nattri++) {
            memset (ina.sl_key, 0, sizeof (ina.sl_key));
            sfsprintf (
                ina.sl_key, sizeof (ina.sl_key), "%s%s", smp->nattrs[nattri], s5
            );
            if ((inap = sl_inv_nodeattrfind (level, id, ina.sl_key)))
                break;
        }
        if (nattri == smp->nattrn) {
            sfprintf (sfstdout, "%s|%s|%s|%s|%s\n", line, s1, s2, s3, s4);
            continue;
        }
        sfprintf (
            sfstdout, "%s|%s__LABEL__%s|%s|%s|%s\n",
            line, inap->sl_val, s1, s2, s3, s4
        );
        sfsync (sfstdout);
    }

    DDSterm ();
    return 0;
}
