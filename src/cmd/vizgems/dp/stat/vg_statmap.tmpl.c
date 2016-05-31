
#define VG_STATMAP_AGGRMODE_AVG 1
#define VG_STATMAP_AGGRMODE_SUM 2
#define VG_STATMAP_AGGRMODE_MIN 3
#define VG_STATMAP_AGGRMODE_MAX 4
#define VG_STATMAP_GROUPMODE_AVG 1
#define VG_STATMAP_GROUPMODE_SUM 2
#define VG_STATMAP_GROUPMODE_MIN 3
#define VG_STATMAP_GROUPMODE_MAX 4

typedef struct statmap_s {
    char *name, *clabel, *flabel, *comb, *nattr;
    int aggrmode, groupmode;
    int havemaxv;
    float maxv;
    char **nattrs;
    int nattrn;
} statmap_t;

#ifndef VG_STATMAP_MAIN

extern statmap_t *statmaps;
extern int statmapn;

extern int statmapload (char *, int, int, int);
extern int statmapfind (char *);

#else

statmap_t *statmaps;
int statmapn;

int statmapload (char *file, int combflag, int maxvflag, int nattrflag) {
    Sfio_t *fp;
    int statmapi;
    char *line, *s1, *s2, *s3, *s4, *s5, *s6, *s7, c;

    if (!file)
        SUerror ("statmapload", "no file specified");
    if (!(fp = sfopen (NULL, file, "r")))
        SUerror ("statmapload", "cannot open stat info file");
    if (!(line = sfgetr (fp, '\n', 1))) {
        SUwarning (0, "statmapload", "cannot read array size");
        return -1;
    }
    if ((statmapn = atoi (line)) < 0) {
        SUwarning (0, "statmapload", "bad array size %d", statmapn);
        return -1;
    }
    if (!(statmaps = vmalloc (Vmheap, sizeof (statmap_t) * statmapn)))
        SUerror ("vg_query_alarm_edge", "cannot allocate stat array");
    memset (statmaps, 0, sizeof (statmap_t) * statmapn);

    statmapi = 0;
    while ((line = sfgetr (fp, '\n', 1))) {
        if (
            !(s1 = strchr (line, '|')) || !(s2 = strchr (s1 + 1, '|')) ||
            !(s3 = strchr (s2 + 1, '|'))
        ) {
            SUwarning (0, "statmapload", "bad line: %s", line);
            continue;
        }
        *s1++ = 0, *s2++ = 0, *s3++ = 0;
        if ((s4 = strchr (s3, '|')))
            *s4++ = 0;
        else
            s4 = "";
        if ((s5 = strchr (s4, '|')))
            *s5++ = 0;
        else
            s5 = "";
        if (
            !(statmaps[statmapi].name = vmstrdup (Vmheap, line)) ||
            !(statmaps[statmapi].clabel = vmstrdup (Vmheap, s1)) ||
            !(statmaps[statmapi].flabel = vmstrdup (Vmheap, s2)) ||
            !(statmaps[statmapi].comb = vmstrdup (Vmheap, s3)) ||
            !(statmaps[statmapi].nattr = vmstrdup (Vmheap, s4))
        ) {
            SUwarning (0, "statmapload", "cannot allocate entry: %d", statmapi);
            return -1;
        }
        if (s4[0] >= '0' && s4[0] <= '9') {
            statmaps[statmapi].havemaxv = TRUE;
            statmaps[statmapi].maxv = atof (s4);
        } else
            statmaps[statmapi].havemaxv = FALSE;
        if (combflag) {
            statmaps[statmapi].aggrmode = VG_STATMAP_AGGRMODE_AVG;
            statmaps[statmapi].groupmode = VG_STATMAP_GROUPMODE_AVG;
            if ((s7 = strchr (statmaps[statmapi].comb, ':')))
                c = *s7, *s7 = 0;
            if (strcmp (statmaps[statmapi].comb, "avg") == 0)
                statmaps[statmapi].aggrmode = VG_STATMAP_AGGRMODE_AVG;
            else if (strcmp (statmaps[statmapi].comb, "sum") == 0)
                statmaps[statmapi].aggrmode = VG_STATMAP_AGGRMODE_SUM;
            else if (strcmp (statmaps[statmapi].comb, "min") == 0)
                statmaps[statmapi].aggrmode = VG_STATMAP_AGGRMODE_MIN;
            else if (strcmp (statmaps[statmapi].comb, "max") == 0)
                statmaps[statmapi].aggrmode = VG_STATMAP_AGGRMODE_MAX;
            else
                SUwarning (0, "statmapload", "bad aggr mode: %s", s3);
            statmaps[statmapi].groupmode = statmaps[statmapi].aggrmode;
            if (s7) {
                if (strcmp (s7 + 1, "avg") == 0)
                    statmaps[statmapi].groupmode = VG_STATMAP_GROUPMODE_AVG;
                else if (strcmp (s7 + 1, "sum") == 0)
                    statmaps[statmapi].groupmode = VG_STATMAP_GROUPMODE_SUM;
                else if (strcmp (s7 + 1, "min") == 0)
                    statmaps[statmapi].groupmode = VG_STATMAP_GROUPMODE_MIN;
                else if (strcmp (s7 + 1, "max") == 0)
                    statmaps[statmapi].groupmode = VG_STATMAP_GROUPMODE_MAX;
                else
                    SUwarning (0, "statmapload", "bad group mode: %s", s7 + 1);
                *s7 = c;
            }
        }
        if (nattrflag) {
            statmaps[statmapi].nattrn = 0;
            while (s5 && *s5) {
                if ((s6 = strchr (s5, ' ')))
                    *s6++ = 0;
                if (*s5 == 0) {
                    s5 = s6;
                    continue;
                }
                if (!(statmaps[statmapi].nattrs = vmresize (
                    Vmheap, statmaps[statmapi].nattrs,
                    sizeof (char *) * (statmaps[statmapi].nattrn + 1), VM_RSCOPY
                ))) {
                    SUwarning (0, "statmapload", "cannot grow nattrs");
                    return -1;
                }
                if (!(
                    statmaps[statmapi].nattrs[statmaps[statmapi].nattrn++] =
                    vmstrdup (Vmheap, s5)
                )) {
                    SUwarning (
                        0, "statmapload", "cannot allocate nattr: %s", s5
                    );
                    return -1;
                }
                s5 = s6;
            }
        }

        statmapi++;
    }

    sfclose (fp);
    return 0;
}

int statmapfind (char *name) {
    int l, statmapi;
    char *s1;

    for (statmapi = 0; statmapi < statmapn; statmapi++)
        if (strcmp (statmaps[statmapi].name, name) == 0)
            return statmapi;

    if ((s1 = strchr (name, '.')))
        l = s1 - name;
    else
        l = strlen (name);
    for (statmapi = 0; statmapi < statmapn; statmapi++)
        if (
            strncmp (statmaps[statmapi].name, name, l) == 0 &&
            strlen (statmaps[statmapi].name) <= l
        )
            return statmapi;
    return -1;
}
#endif
