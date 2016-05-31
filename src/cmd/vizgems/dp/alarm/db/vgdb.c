#pragma prototyped

#include <ast.h>
#include <regex.h>
#include <vmalloc.h>
#include <swift.h>
#include "vg_hdr.h"
#include "vgdb_pub.h"
#include "vg_sevmap.c"

#define TYPE_VID 1
#define TYPE_LIT 2

typedef struct n2i_s {
    char *name;
    int id;
} n2i_t;

static n2i_t statemap[] = {
    { VG_ALARM_S_STATE_INFO, VG_ALARM_N_STATE_INFO },
    { VG_ALARM_S_STATE_DEG,  VG_ALARM_N_STATE_DEG  },
    { VG_ALARM_S_STATE_DOWN, VG_ALARM_N_STATE_DOWN },
    { VG_ALARM_S_STATE_UP,   VG_ALARM_N_STATE_UP   },
    { NULL,                  0                     },
};

static n2i_t modemap[] = {
    { VG_ALARM_S_MODE_DROP,  VG_ALARM_N_MODE_DROP  },
    { VG_ALARM_S_MODE_KEEP,  VG_ALARM_N_MODE_KEEP  },
    { VG_ALARM_S_MODE_DEFER, VG_ALARM_N_MODE_DEFER },
    { NULL,                  0                     },
};

static alarm_t *alarms;
static int alarmn, alarml;

#define RESTR_L 10 * VG_alarm_text_L
static char restr[RESTR_L];

#define VAR_L 2 * VG_alarm_variables_L
#define VAR_N VG_alarm_variables_L
static char tvarstr[VAR_L];

static int parsetextre (alarm_t *, char *);
static int parseidre (alarm_t *, char *);
static int addvar (alarm_t *, char *);
static int addvid (alarm_t *, int);
static int mapn2i (n2i_t *, char *);
static int cmp (const void *, const void *);

int alarmdbload (char *name) {
    Sfio_t *fp;
    char *line, *s;
    int ln;
    int state;
    alarm_t *alarmp;
    int i;

    alarms = NULL;
    alarmn = alarml = 0;
    for (i = 0; i < 10; i++) {
        if ((fp = sfopen (NULL, name, "r")))
            break;
        sleep (1);
    }
    if (!fp) {
        SUwarning (0, "alarmdbload", "cannot open database file");
        return -1;
    }

    ln = 0, state = 0, alarmp = NULL;
    while ((line = sfgetr (fp, '\n', 1))) {
        ln++;
        for (s = line; *s; s++)
            if (*s == '#' || (*s != ' ' && *s != '\t'))
                break;
        if (*s == 0 || *s == '#')
            continue;
        if (strcmp (line, "BEGIN") == 0) {
            if (state != 0) {
                SUwarning (
                    0, "alarmdbload", "BEGIN tag out of order, line: %d", ln
                );
                return -1;
            }
            state = 1;
            if (alarml == alarmn) {
                if (!(alarms = vmresize (
                    Vmheap, alarms, sizeof (alarm_t) * (alarmn + 128), VM_RSCOPY
                ))) {
                    SUwarning (0, "alarmdbload", "malloc failed");
                    return -1;
                }
                alarmn += 128;
            }
            alarmp = &alarms[alarml++];
            memset (alarmp, 0, sizeof (alarm_t));
            alarmp->tm = alarmp->sm = -1;
            alarmp->sevnum = -1;
            alarmp->statenum = VG_ALARM_N_STATE_NONE;
        } else if (strcmp (line, "END") == 0) {
            if (state != 1 || !alarmp) {
                SUwarning (
                    0, "alarmdbload", "END tag out of order, line: %d", ln
                );
                return -1;
            }
            if (!alarmp->id || !alarmp->textstr) {
                SUwarning (0, "alarmdbload", "incomplete alarm entry");
                alarml--;
                return -1;
            }
            VG_warning (
                1, "DATA INFO", "DB loaded alarm entry text=%s object=%s",
                alarmp->textstr, (alarmp->objstr) ? alarmp->objstr : ""
            );
            state = 0;
            alarmp = NULL;
        } else if (strncmp (line, "TEXT ", 5) == 0) {
            if (state != 1 || !alarmp) {
                SUwarning (
                    0, "alarmdbload", "TEXT tag unexpected, line: %d", ln
                );
                return -1;
            }
            if (parsetextre (alarmp, line + 5) == -1) {
                SUwarning (0, "alarmdbload", "cannot parse alarm text");
                return -1;
            }
        } else if (strncmp (line, "OBJECT ", 7) == 0) {
            if (state != 1 || !alarmp) {
                SUwarning (
                    0, "alarmdbload", "OBJECT tag unexpected, line: %d", ln
                );
                return -1;
            }
            if (parseidre (alarmp, line + 7) == -1) {
                SUwarning (0, "alarmdbload", "cannot parse id text");
                return -1;
            }
        } else if (strncmp (line, "ID ", 3) == 0) {
            if (state != 1 || !alarmp) {
                SUwarning (0, "alarmdbload", "ID tag unexpected, line: %d", ln);
                return -1;
            }
            if (!(alarmp->id = vmstrdup (Vmheap, line + 3))) {
                SUwarning (0, "alarmdbload", "cannot allocate id");
                return -1;
            }
            if (strlen (alarmp->id) >= VG_alarm_alarmid_L) {
                SUwarning (
                    0, "alarmdbload", "id too long: %s", alarmp->id
                );
                return -1;
            }
        } else if (strncmp (line, "STATE ", 6) == 0) {
            if (state != 1 || !alarmp) {
                SUwarning (
                    0, "alarmdbload", "STATE tag unexpected, line: %d", ln
                );
                return -1;
            }
            if ((alarmp->statenum = mapn2i (statemap, line + 6)) == -1) {
                SUwarning (
                    0, "alarmdbload", "cannot map state %s", line + 6
                );
                return -1;
            }
        } else if (strncmp (line, "SEVERITY ", 9) == 0) {
            if (state != 1 || !alarmp) {
                SUwarning (
                    0, "alarmdbload", "SEVERITY tag unexpected, line: %d", ln
                );
                return -1;
            }
            if ((alarmp->sevnum = sevmapfind (line + 9)) == -1) {
                SUwarning (
                    0, "alarmdbload", "cannot map severity %s", line + 9
                );
                return -1;
            }
        } else if (strncmp (line, "VARIABLE ", 9) == 0) {
            if (state != 1 || !alarmp) {
                SUwarning (
                    0, "alarmdbload", "VARIABLE tag unexpected, line: %d", ln
                );
                return -1;
            }
            if (addvar (alarmp, line + 9) == -1) {
                SUwarning (
                    0, "alarmdbload", "cannot parse variable %s", line + 9
                );
                return -1;
            }
        } else if (strncmp (line, "UNIQUE ", 7) == 0) {
            if (state != 1 || !alarmp) {
                SUwarning (
                    0, "alarmdbload", "UNIQUE tag unexpected, line: %d", ln
                );
                return -1;
            }
            if (!(alarmp->unique = vmstrdup (Vmheap, line + 7))) {
                SUwarning (0, "alarmdbload", "cannot allocate unique");
                return -1;
            }
        } else if (strncmp (line, "TMODE ", 6) == 0) {
            if (state != 1 || !alarmp) {
                SUwarning (
                    0, "alarmdbload", "TMODE tag unexpected, line: %d", ln
                );
                return -1;
            }
            if ((alarmp->tm = mapn2i (modemap, line + 6)) == -1) {
                SUwarning (
                    0, "alarmdbload", "cannot map tmode %s", line + 6
                );
                return -1;
            }
        } else if (strncmp (line, "SMODE ", 6) == 0) {
            if (state != 1 || !alarmp) {
                SUwarning (
                    0, "alarmdbload", "SMODE tag unexpected, line: %d", ln
                );
                return -1;
            }
            if ((alarmp->sm = mapn2i (modemap, line + 6)) == -1) {
                SUwarning (
                    0, "alarmdbload", "cannot map smode %s", line + 6
                );
                return -1;
            }
        } else if (strncmp (line, "COMMENT ", 8) == 0) {
            if (state != 1 || !alarmp) {
                SUwarning (
                    0, "alarmdbload", "COMMENT tag unexpected, line: %d", ln
                );
                return -1;
            }
            if (!(alarmp->com = vmstrdup (Vmheap, line + 8))) {
                SUwarning (0, "alarmdbload", "cannot allocate comment");
                return -1;
            }
            if (strlen (alarmp->com) >= VG_alarm_comment_L) {
                SUwarning (0, "alarmdbload", "comment too long");
                return -1;
            }
        } else {
            SUwarning (0, "alarmdbload", "unknown tag, line %d: %s", ln, line);
            return -1;
        }
    }
    qsort (alarms, alarml, sizeof (alarm_t), cmp);
    sfclose (fp);
    return 0;
}

alarm_t *alarmdbfind (char *alarmid, char *text, char *obj) {
    alarm_t *alarmp;
    int alarmi;
    regmatch_t match[100];
    vid_t *vidp;
    int vidl, vidi;
    char c;

    for (alarmi = 0; alarmi < alarml; alarmi++) {
        alarmp = &alarms[alarmi];
        if (alarmid && alarmid[0]) {
            if (strcmp (alarmid, alarmp->id) == 0) {
                VG_warning (
                    1, "DATA INFO", "DB matched %s id=%s", alarmid, alarmp->id
                );
                return alarmp;
            }
            continue;
        }
        if (!strstr (text, alarmp->textlcss))
            continue;
        if (alarmp->objlcss && !strstr (obj, alarmp->objlcss))
            continue;
        if (alarmp->objstr && regexec (
            &alarmp->objcode, obj, elementsof (match), match,
            REG_NULL | REG_EXTENDED | REG_LEFT | REG_RIGHT
        ) != 0)
            continue;
        if (regexec (
            &alarmp->textcode, text, elementsof (match), match,
            REG_NULL | REG_EXTENDED | REG_LEFT | REG_RIGHT
        ) != 0)
            continue;
        VG_warning (1, "DATA INFO", "DB matched %s id=%s", text, alarmp->id);
        for (vidl = alarmp->textcode.re_nsub; vidl > 0; vidl--)
            if (match[vidl].rm_so != -1)
                break;
        if (alarmp->textcode.re_nsub != vidl) {
            SUwarning (
                0, "alarmdbfind",
                "mismatch %d %d", vidl, alarmp->textcode.re_nsub
            );
            return NULL;
        }
        for (vidi = 0; vidi < vidl; vidi++) {
            vidp = &alarmp->vids[vidi];
            c = text[match[vidi + 1].rm_eo], text[match[vidi + 1].rm_eo] = 0;
            if (!(vidp->value = vmstrdup (
                Vmheap, &text[match[vidi + 1].rm_so]
            ))) {
                SUwarning (0, "alarmdbfind", "cannot allocate value");
                return NULL;
            }
            text[match[vidi + 1].rm_eo] = c;
        }
        return alarmp;
    }
    if (alarmid && alarmid[0]) {
        for (alarmi = 0; alarmi < alarml; alarmi++) {
            alarmp = &alarms[alarmi];
            if (strcmp (alarmp->id, "__ANY__") == 0) {
                VG_warning (
                    1, "DATA INFO", "DB matched %s id=%s", alarmid, alarmp->id
                );
                return alarmp;
            }
        }
    }
    VG_warning (2, "DATA INFO", "DB cannot match %s\n", text);
    return NULL;
}

#define CHECKNCOPY(s) { \
    int l; \
    l = strlen (s); \
    if (VAR_L - (vp - tvarstr) < l + 1) { \
        SUwarning (0, "alarmdbvars", "out of variable space"); \
        return NULL; \
    } \
    strcpy (vp, s), vp += l; \
}

char *alarmdbvars (alarm_t *alarmp) {
    var_t *varp;
    int vari;
    vid_t *vidp;
    int vidi;
    char *s1, *s2, *vp, c;

    tvarstr[0] = 0, vp = &tvarstr[0];
    if (alarmp->varl == 0)
        return "";
    for (vari = 0; vari < alarmp->varl; vari++) {
        varp = &alarmp->vars[vari];
        if (varp->type == TYPE_VID) {
            for (vidi = 0; vidi < alarmp->vidl; vidi++) {
                vidp = &alarmp->vids[vidi];
                if (varp->u.vid == vidp->vid) {
                    if (vari > 0)
                        CHECKNCOPY ("\t\t");
                    CHECKNCOPY (varp->name);
                    CHECKNCOPY ("=");
                    CHECKNCOPY (vidp->value);
                    break;
                }
            }
            if (vidi == alarmp->vidl) {
                SUwarning (
                    0, "alarmdbvars", "cannot find variable %s", varp->name
                );
                return NULL;
            }
        } else {
            if (vari > 0)
                CHECKNCOPY ("\t\t");
            CHECKNCOPY (varp->name);
            CHECKNCOPY ("=");
            CHECKNCOPY (varp->u.lit);
        }
    }
    if (!alarmp->unique)
        return tvarstr;
    CHECKNCOPY ("\t\tunique=");
    for (s1 = alarmp->unique; s1; ) {
        if ((s2 = strchr (s1, ' ')))
            c = *s2, *s2 = 0;
        for (vari = 0; vari < alarmp->varl; vari++) {
            varp = &alarmp->vars[vari];
            if (strcmp (varp->name, s1) != 0)
                continue;
            if (varp->type == TYPE_VID) {
                for (vidi = 0; vidi < alarmp->vidl; vidi++) {
                    vidp = &alarmp->vids[vidi];
                    if (varp->u.vid == vidp->vid) {
                        if (s1 != alarmp->unique)
                            CHECKNCOPY ("\t");
                        CHECKNCOPY (vidp->value);
                        break;
                    }
                }
                if (vidi == alarmp->vidl) {
                    SUwarning (
                        0, "alarmdbvars", "cannot find variable %s", varp->name
                    );
                    return NULL;
                }
            } else {
                if (s1 != alarmp->unique)
                    CHECKNCOPY ("\t");
                CHECKNCOPY (varp->u.lit);
                break;
            }
        }
        if (s2)
            *s2++ = c;
        s1 = s2;
    }
    if (vp - tvarstr + 1 > VG_alarm_variables_L) {
        SUwarning (0, "alarmdbvars", "variable string too long");
        return NULL;
    }
    return tvarstr;
}

static int parsetextre (alarm_t *alarmp, char *str) {
    char *s1, *s2, *s3;
    int vid, pos, siz, maxpos, maxsiz;

    s1 = &restr[0];
    for (maxpos = maxsiz = pos = siz = 0, s2 = str; *s2; ) {
        if (*s2 == '$' && *(s2 + 1) == 'A') {
            *s1++ = '.';
            *s1++ = '*';
            s2 += 2;
            if (siz > maxsiz)
                maxpos = pos, maxsiz = siz;
            pos = s2 - str, siz = 0;
        } else if (*s2 == '$' && *(s2 + 1) >= '0' && *(s2 + 1) <= '9') {
            vid = 0;
            for (s3 = s2 + 1; *s3 >= '0' && *s3 <= '9'; s3++)
                vid = vid * 10 + *s3 - '0';
            if (addvid (alarmp, vid) == -1) {
                SUwarning (0, "parsetextre", "cannot add vid: %d", vid);
                return -1;
            }
            *s1++ = '(';
            *s1++ = '.';
            *s1++ = '*';
            *s1++ = ')';
            s2 = s3;
            if (siz > maxsiz)
                maxpos = pos, maxsiz = siz;
            pos = s2 - str, siz = 0;
        } else if (
            *s2 == '(' || *s2 == ')' || *s2 == '[' || *s2 == ']' ||
            *s2 == '{' || *s2 == '}' ||*s2 == '+' || *s2 == '|'
        ) {
            *s1++ = '\\';
            *s1++ = *s2++;
            if (siz > maxsiz)
                maxpos = pos, maxsiz = siz;
            pos = s2 - str, siz = 0;
        } else {
            *s1++ = *s2++;
            siz++;
        }
    }
    *s1 = 0;
    if (siz > maxsiz)
        maxpos = pos, maxsiz = siz;
    if (!(alarmp->textstr = vmstrdup (Vmheap, str))) {
        SUwarning (0, "parsetextre", "cannot allocate text str");
        return -1;
    }
    if (!(alarmp->textrestr = vmstrdup (Vmheap, restr))) {
        SUwarning (0, "parsetextre", "cannot allocate text re str");
        return -1;
    }
    if (!(alarmp->textlcss = vmalloc (Vmheap, maxsiz + 1))) {
        SUwarning (0, "parsetextre", "cannot allocate text lcs");
        return -1;
    }
    alarmp->textlcsn = maxsiz;
    memcpy (alarmp->textlcss, str + maxpos, maxsiz);
    alarmp->textlcss[maxsiz] = 0;
    if (regcomp (
        &alarmp->textcode, restr,
        REG_NULL | REG_EXTENDED | REG_LEFT | REG_RIGHT
    ) != 0) {
        SUwarning (0, "parsetextre", "cannot compile regex");
        return -1;
    }
    if (alarmp->textcode.re_nsub != alarmp->vidl) {
        SUwarning (
            0, "parsetextre", "mismatch: %d %d",
            alarmp->textcode.re_nsub, alarmp->vidl
        );
        return -1;
    }
    return 0;
}

static int parseidre (alarm_t *alarmp, char *str) {
    char *s1, *s2;
    int pos, siz, maxpos, maxsiz;

    s1 = &restr[0];
    for (maxpos = maxsiz = pos = siz = 0, s2 = str; *s2; ) {
        if (*s2 == '$' && *(s2 + 1) == 'A') {
            *s1++ = '.';
            *s1++ = '*';
            s2 += 2;
            if (siz > maxsiz)
                maxpos = pos, maxsiz = siz;
            pos = s2 - str, siz = 0;
        } else if (*s2 == '$' && *(s2 + 1) >= '0' && *(s2 + 1) <= '9') {
            SUwarning (0, "parseidre", "$<num> not allowed in OBJECT tag");
            return -1;
        } else if (
            *s2 == '(' || *s2 == ')' || *s2 == '[' || *s2 == ']' ||
            *s2 == '{' || *s2 == '}' ||*s2 == '+' || *s2 == '|'
        ) {
            *s1++ = '\\';
            *s1++ = *s2++;
            if (siz > maxsiz)
                maxpos = pos, maxsiz = siz;
            pos = s2 - str, siz = 0;
        } else {
            *s1++ = *s2++;
            siz++;
        }
    }
    *s1 = 0;
    if (siz > maxsiz)
        maxpos = pos, maxsiz = siz;
    if (!(alarmp->objstr = vmstrdup (Vmheap, str))) {
        SUwarning (0, "parseidre", "cannot allocate id str");
        return -1;
    }
    if (!(alarmp->objrestr = vmstrdup (Vmheap, restr))) {
        SUwarning (0, "parseidre", "cannot allocate id re str");
        return -1;
    }
    if (!(alarmp->objlcss = vmalloc (Vmheap, maxsiz + 1))) {
        SUwarning (0, "parseidre", "cannot allocate id lcs");
        return -1;
    }
    alarmp->objlcsn = maxsiz;
    memcpy (alarmp->objlcss, str + maxpos, maxsiz);
    alarmp->objlcss[maxsiz] = 0;
    if (regcomp (
        &alarmp->objcode, restr,
        REG_NULL | REG_EXTENDED | REG_LEFT | REG_RIGHT
    ) != 0) {
        SUwarning (0, "parseidre", "cannot compile regex");
        return -1;
    }
    return 0;
}


static int addvar (alarm_t *alarmp, char *line) {
    var_t *varp;
    char *name, *value;

    name = line;
    if (!(value = strchr (line, ' '))) {
        SUwarning (0, "addvar", "cannot find value string: %s", line);
        return -1;
    }
    *value++ = 0;
    if (alarmp->varl == alarmp->varn) {
        if (!(alarmp->vars = vmresize (
            Vmheap, alarmp->vars, sizeof (var_t) * (alarmp->varn + 8), VM_RSCOPY
        ))) {
            SUwarning (0, "addvar", "cannot allocate vars");
            return -1;
        }
        alarmp->varn += 8;
    }
    varp = &alarmp->vars[alarmp->varl++];
    if (!(varp->name = vmstrdup (Vmheap, name))) {
        SUwarning (0, "addvar", "cannot allocate name");
        return -1;
    }
    if (*value != '$' || *(value + 1) < '0' || *(value + 1) > '9') {
        varp->type = TYPE_LIT;
        if (!(varp->u.lit = vmstrdup (Vmheap, value))) {
            SUwarning (0, "addvar", "cannot allocate literal value");
            return -1;
        }
    } else {
        varp->type = TYPE_VID;
        varp->u.vid = atoi (value + 1);
    }
    return 0;
}

static int addvid (alarm_t *alarmp, int vid) {
    if (alarmp->vidl == alarmp->vidn) {
        if (!(alarmp->vids = vmresize (
            Vmheap, alarmp->vids, sizeof (vid_t) * (alarmp->vidn + 8), VM_RSCOPY
        ))) {
            SUwarning (0, "addvid", "cannot allocate vids");
            return -1;
        }
        alarmp->vidn += 8;
    }
    alarmp->vids[alarmp->vidl].value = NULL;
    alarmp->vids[alarmp->vidl++].vid = vid;
    return 0;
}

static int mapn2i (n2i_t *maps, char *name) {
    int mapi;

    for (mapi = 0; maps[mapi].name; mapi++)
        if (strcasecmp (maps[mapi].name, name) == 0)
            return maps[mapi].id;
    return -1;
}

static int cmp (const void *a, const void *b) {
    alarm_t *ap, *bp;

    ap = (alarm_t *) a;
    bp = (alarm_t *) b;
    if (bp->textlcsn != ap->textlcsn)
        return bp->textlcsn - ap->textlcsn;
    return bp->objlcsn - ap->objlcsn;
}
