
typedef struct timerange_t {
    time_t ft, lt;
} timerange_t;
static timerange_t *trs;
static int trn;

int timerangesload (char *file, int size) {
    Sfio_t *fp;
    char *line, *s1, *s2;
    int trm;

    trm = 0, trn = atoi (getenv ("TIMERANGESSIZE"));
    if (trn > 0) {
        if (!(trs = vmalloc (Vmheap, sizeof (timerange_t) * trn)))
            SUerror ("vg_timeranges", "cannot allocate trs");
        memset (trs, 0, sizeof (timerange_t) * trn);
        if (!(fp = sfopen (NULL, getenv ("TIMERANGESFILE"), "r")))
            SUerror ("vg_timeranges", "cannot open inv filter file");
        while ((line = sfgetr (fp, '\n', 1))) {
            if (!(s1 = strchr (line, '|'))) {
                SUwarning (0, "vg_timeranges", "bad line: %s", line);
                break;
            }
            *s1++ = 0;
            trs[trm].ft = strtoll (line, &s2, 10);
            trs[trm].lt = strtoll (s1, &s2, 10);
            trm++;
        }
        sfclose (fp);
    }
    if (trm != trn)
        trn = -1;

    return 0;
}

int timerangesinrange (time_t t) {
    int tri;

    for (tri = 0; tri < trn; tri++)
        if (trs[tri].ft <= t && trs[tri].lt >= t)
            return TRUE;
    return FALSE;
}
