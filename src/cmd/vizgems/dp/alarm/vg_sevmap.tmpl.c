
typedef struct sevmap_s {
    char *name;
    int id;
} sevmap_t;

#ifndef VG_SEVMAP_MAIN

extern sevmap_t *sevmaps;
extern int sevmapn, sevdefi;

extern int sevmapload (char *);
extern int sevmapfind (char *);

#else

sevmap_t *sevmaps;
int sevmapn, sevdefi;

int sevmapload (char *file) {
    Sfio_t *fp;
    int sevmapi;
    char *line, *s1;

    if (!file)
        SUerror ("sevmapload", "no file specified");
    if (!(fp = sfopen (NULL, file, "r")))
        SUerror ("sevmapload", "cannot open severity info file");
    if (!(line = sfgetr (fp, '\n', 1))) {
        SUwarning (0, "sevmapload", "cannot read array size");
        return -1;
    }
    if ((sevmapn = atoi (line)) < 0) {
        SUwarning (0, "sevmapload", "bad array size %d", sevmapn);
        return -1;
    }
    if (!(line = sfgetr (fp, '\n', 1))) {
        SUwarning (0, "sevmapload", "cannot read default severity");
        return -1;
    }
    if ((sevdefi = atoi (line)) < 0 || sevdefi >= sevmapn) {
        SUwarning (0, "sevmapload", "bad severity %d", sevdefi);
        return -1;
    }
    if (!(sevmaps = vmalloc (Vmheap, sizeof (sevmap_t) * sevmapn)))
        SUerror ("vg_query_alarm_edge", "cannot allocate ins");
    memset (sevmaps, 0, sizeof (sevmap_t) * sevmapn);

    while ((line = sfgetr (fp, '\n', 1))) {
        if (!(s1 = strchr (line, '|'))) {
            SUwarning (0, "sevmapload", "bad line: %s", line);
            continue;
        }
        *s1++ = 0;
        if ((sevmapi = atoi (line)) < 0 || sevmapi >= sevmapn) {
            SUwarning (0, "sevmapload", "bad id: %d", sevmapi);
            continue;
        }
        sevmaps[sevmapi].id = sevmapi;
        if (!(sevmaps[sevmapi].name = vmstrdup (Vmheap, s1))) {
            SUwarning (0, "sevmapload", "cannot allocate name: %s", s1);
            return -1;
        }
    }

    sfclose (fp);
    return 0;
}

int sevmapfind (char *name) {
    int sevmapi;

    for (sevmapi = 1; sevmapi < sevmapn; sevmapi++)
        if (strcasecmp (sevmaps[sevmapi].name, name) == 0)
            return sevmapi;
    return -1;
}
#endif
