#pragma prototyped

#include <ast.h>
#include <option.h>
#include <vmalloc.h>
#include <regex.h>
#include <swift.h>
#include <dds.h>
#include <time.h>
#define VG_DEFS_MAIN
#include "vg_hdr.h"
#include "sl_level_map.c"
#include "sl_inv_map1.c"

static const char usage[] =
"[-1p1?\n@(#)$Id: vg_prop_filter (AT&T Labs Research) 2009-06-11 $\n]"
USAGE_LICENSE
"[+NAME?vg_prop_filter - filter tool for data propagation tool]"
"[+DESCRIPTION?\bvg_prop_filter\b reads a filter file of ids and uses it to"
" filter lines from standard input."
" The ids are usually customer ids."
" Any line that contains an asset that maps to one of the filter ids is kept."
" Any line that contains an asset that does not map to a filter is is dropped."
" Lines that do not contain asset info are always kept."
"]"
"[100:mode?specifies the mode to use."
" Possible modes are: \binv\b, \bstat\b, \balarm\b."
"]:[(inv|stat|alarm):oneof]{[+inv][+stat][+alarm]}"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"[+SEE ALSO?\bdds\b(1), \bdds\b(3)]"
;

#define MODE_INV   1
#define MODE_STAT  2
#define MODE_ALARM 3

#define SZ_level VG_inv_node_level_L
#define SZ_id VG_inv_node_id_L

#define INVKIND_I 1
typedef struct inv_t {
    int s_kind;
    char s_level[SZ_level], *s_id;
    regex_t s_code;
} inv_t;
static inv_t *ins;
static int inn;

#define ICMPFLAGS (REG_NULL | REG_EXTENDED | REG_LEFT | REG_RIGHT)
#define ICMP(p, i, s) (regexec (&p[i].s_code, s, 0, NULL, ICMPFLAGS) == 0)

int main (int argc, char **argv) {
    int norun;
    int mode;
    Sfio_t *fp;
    char *line, *s1, *s2, *s3, *s4, *s5;
    int ini, inm, emit, emit1, emit2, pemit1, pemit2;
    char *m1p;
    char level1[SZ_level], id1[SZ_id], plevel1[SZ_level], pid1[SZ_id];
    char level2[SZ_level], id2[SZ_id], plevel2[SZ_level], pid2[SZ_id];

    mode = -1;
    norun = 0;
    for (;;) {
        switch (optget (argv, usage)) {
        case -100:
            if (strcmp (opt_info.arg, "inv") == 0)
                mode = MODE_INV;
            else if (strcmp (opt_info.arg, "stat") == 0)
                mode = MODE_STAT;
            else if (strcmp (opt_info.arg, "alarm") == 0)
                mode = MODE_ALARM;
            continue;
        case -999:
            SUwarnlevel++;
            continue;
        case '?':
            SUusage (0, "vg_prop_filter", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "vg_prop_filter", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;
    if (mode == -1) {
        SUwarning (0, "vg_prop_filter", "please specify a mode");
        return -1;
    }

    DDSinit ();
    sl_level_mapopen (getenv ("LEVELMAPFILE"));
    sl_inv_map1open (getenv ("INVMAPFILE"));
    M1I (TRUE);

    inm = 0, inn = atoi (getenv ("INVPROPFILTERSIZE"));
    if (inn > 0) {
        if (!(ins = vmalloc (Vmheap, sizeof (inv_t) * inn)))
            SUerror ("vg_dq_dt_inv_node", "cannot allocate ins");
        memset (ins, 0, sizeof (inv_t) * inn);
        if (!(fp = sfopen (NULL, getenv ("INVPROPFILTERFILE"), "r")))
            SUerror ("vg_dq_dt_inv_node", "cannot open inv filter file");
        while ((line = sfgetr (fp, '\n', 1))) {
            if (!(s1 = strchr (line, '|'))) {
                SUwarning (0, "vg_dq_dt_inv_node", "bad line: %s", line);
                break;
            }
            *s1++ = 0;
            if (line[0] == 'I' || line[0] == 'F')
                ins[inm].s_kind = INVKIND_I;
            else {
                SUwarning (0, "vg_dq_dt_inv_node", "bad line: %s", s1);
                break;
            }
            if (!(s2 = strchr (s1, '|'))) {
                SUwarning (0, "vg_dq_dt_inv_node", "bad line: %s", s1);
                break;
            }
            *s2++ = 0;
            strcpy (ins[inm].s_level, s1);
            if (!(ins[inm].s_id = vmstrdup (Vmheap, s2))) {
                SUwarning (0, "vg_dq_dt_inv_node", "cannot copy id: %s", s2);
                break;
            }
            if (regcomp (
                &ins[inm].s_code, ins[inm].s_id, ICMPFLAGS
            ) != 0) {
                SUwarning (
                    0, "vg_dq_dt_inv_node",
                    "cannot compile regex: %s", ins[inm].s_id
                );
                break;
            }
            inm++;
        }
        sfclose (fp);
    }
    if (inm != inn)
        inn = -1;

    plevel1[0] = plevel2[0] = pid1[0] = pid2[0] = 0;
    pemit1 = pemit2 = FALSE;
    while ((line = sfgetr (sfstdin, '\n', SF_STRING))) {
        if (inn < 0)
            continue;

        emit = emit1 = emit2 = FALSE;
        switch (mode) {
        case MODE_INV:
            if (strncmp (line, "node|", 5) == 0) {
                s1 = s2 = s3 = NULL;
                if (
                    !(s1 = strstr (line, "|")) ||
                    !(s2 = strstr (s1 + 1, "|")) ||
                    !(s3 = strstr (s2 + 1, "|"))
                ) {
                    emit1 = TRUE;
                } else {
                    memset (level1, 0, SZ_level);
                    memset (id1, 0, SZ_id);
                    memcpy (level1, s1 + 1, s2 - s1 - 1);
                    memcpy (id1, s2 + 1, s3 - s2 - 1);
                    if (
                        strcmp (level1, plevel1) == 0 && strcmp (id1, pid1) == 0
                    ) {
                        emit1 = pemit1;
                    } else {
                        strcpy (plevel1, level1);
                        strcpy (pid1, id1);
                        inm = 0;
                        for (ini = 0; ini < inn; ini++) {
                            for (
                                m1p = M1F (level1, id1, ins[ini].s_level); m1p;
                                m1p = M1N (level1, id1, ins[ini].s_level)
                            ) {
                                if (ICMP (ins, ini, m1p)) {
                                    inm++;
                                    break;
                                }
                            }
                            if (ini + 1 != inm)
                                break;
                        }
                        if (inm == inn)
                            emit1 = TRUE;
                        pemit1 = emit1;
                    }
                }
                emit = emit1;
            } else {
                s1 = s2 = s3 = NULL;
                if (
                    !(s1 = strstr (line, "|")) ||
                    !(s2 = strstr (s1 + 1, "|")) ||
                    !(s3 = strstr (s2 + 1, "|"))
                ) {
                    emit1 = TRUE;
                } else {
                    memset (level1, 0, SZ_level);
                    memset (id1, 0, SZ_id);
                    memcpy (level1, s1 + 1, s2 - s1 - 1);
                    memcpy (id1, s2 + 1, s3 - s2 - 1);
                    if (
                        strcmp (level1, plevel1) == 0 && strcmp (id1, pid1) == 0
                    ) {
                        emit1 = pemit1;
                    } else {
                        strcpy (plevel1, level1);
                        strcpy (pid1, id1);
                        inm = 0;
                        for (ini = 0; ini < inn; ini++) {
                            for (
                                m1p = M1F (level1, id1, ins[ini].s_level); m1p;
                                m1p = M1N (level1, id1, ins[ini].s_level)
                            ) {
                                if (ICMP (ins, ini, m1p)) {
                                    inm++;
                                    break;
                                }
                            }
                            if (ini + 1 != inm)
                                break;
                        }
                        if (inm == inn)
                            emit1 = TRUE;
                        pemit1 = emit1;
                    }
                }
                if (
                    !s3 ||
                    !(s4 = strstr (s3 + 1, "|")) ||
                    !(s5 = strstr (s4 + 1, "|"))
                ) {
                    emit2 = TRUE;
                } else {
                    memset (level2, 0, SZ_level);
                    memset (id2, 0, SZ_id);
                    memcpy (level2, s3 + 1, s4 - s3 - 1);
                    memcpy (id2, s4 + 1, s5 - s4 - 1);
                    if (
                        strcmp (level2, plevel2) == 0 && strcmp (id2, pid2) == 0
                    ) {
                        emit2 = pemit2;
                    } else {
                        strcpy (plevel2, level2);
                        strcpy (pid2, id2);
                        inm = 0;
                        for (ini = 0; ini < inn; ini++) {
                            for (
                                m1p = M1F (level2, id2, ins[ini].s_level); m1p;
                                m1p = M1N (level2, id2, ins[ini].s_level)
                            ) {
                                if (ICMP (ins, ini, m1p)) {
                                    inm++;
                                    break;
                                }
                            }
                            if (ini + 1 != inm)
                                break;
                        }
                        if (inm == inn)
                            emit2 = TRUE;
                        pemit2 = emit1;
                    }
                }
                emit = emit1 || emit2;
            }
            break;
        case MODE_STAT:
            s1 = s2 = s3 = s4 = NULL;
            if (
                !(s1 = strstr (line, "<lv>")) ||
                !(s2 = strstr (s1, "</lv>")) ||
                !(s3 = strstr (line, "<id>")) ||
                !(s4 = strstr (s3, "</id>"))
            ) {
                emit1 = TRUE;
            } else {
                memset (level1, 0, SZ_level);
                memset (id1, 0, SZ_id);
                memcpy (level1, s1 + 4, s2 - s1 - 4);
                memcpy (id1, s3 + 4, s4 - s3 - 4);
                if (strcmp (level1, plevel1) == 0 && strcmp (id1, pid1) == 0) {
                    emit1 = pemit1;
                } else {
                    strcpy (plevel1, level1);
                    strcpy (pid1, id1);
                    inm = 0;
                    for (ini = 0; ini < inn; ini++) {
                        for (
                            m1p = M1F (level1, id1, ins[ini].s_level); m1p;
                            m1p = M1N (level1, id1, ins[ini].s_level)
                        ) {
                            if (ICMP (ins, ini, m1p)) {
                                inm++;
                                break;
                            }
                        }
                        if (ini + 1 != inm)
                            break;
                    }
                    if (inm == inn)
                        emit1 = TRUE;
                    pemit1 = emit1;
                }
            }
            emit = emit1;
            break;
        case MODE_ALARM:
            s1 = s2 = s3 = s4 = NULL;
            if (
                !(s1 = strstr (line, "<lv1>")) ||
                !(s2 = strstr (s1, "</lv1>")) ||
                !(s3 = strstr (line, "<id1>")) ||
                !(s4 = strstr (s3, "</id1>"))
            ) {
                emit1 = TRUE;
            } else {
                memset (level1, 0, SZ_level);
                memset (id1, 0, SZ_id);
                memcpy (level1, s1 + 5, s2 - s1 - 5);
                memcpy (id1, s3 + 5, s4 - s3 - 5);
                if (strcmp (level1, plevel1) == 0 && strcmp (id1, pid1) == 0) {
                    emit1 = pemit1;
                } else {
                    strcpy (plevel1, level1);
                    strcpy (pid1, id1);
                    inm = 0;
                    for (ini = 0; ini < inn; ini++) {
                        for (
                            m1p = M1F (level1, id1, ins[ini].s_level); m1p;
                            m1p = M1N (level1, id1, ins[ini].s_level)
                        ) {
                            if (ICMP (ins, ini, m1p)) {
                                inm++;
                                break;
                            }
                        }
                        if (ini + 1 != inm)
                            break;
                    }
                    if (inm == inn)
                        emit1 = TRUE;
                    pemit1 = emit1;
                }
            }
            emit = emit1;
            break;
        }
        if (emit)
            sfputr (sfstdout, line, '\n');
    }

    DDSterm ();
    return 0;
}
