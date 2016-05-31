
#define ATTR_DOMAIN       0
#define ATTR_EDGESEP      1
#define ATTR_FFRAME       2
#define ATTR_LFRAME       3
#define ATTR_FTIME        4
#define ATTR_LTIME        5
#define ATTR_USETIMERANGE 6
#define ATTR_INLEVELS     7
#define ATTR_POUTLEVEL    8
#define ATTR_SOUTLEVEL    9

#define ATTRSIZE 10

static char *attrnames[ATTRSIZE] = {
    "domain",
    "edgesep",
    "fframe",
    "lframe",
    "ftime",
    "ltime",
    "usetimerange",
    "inlevels",
    "poutlevel",
    "soutlevel"
};

static char *attrs[ATTRSIZE];

static int attrload (char *file) {
    Sfio_t *fp;
    char *line, *s1;
    int attri;

    if (!(fp = sfopen (NULL, file, "r"))) {
        SUwarning (0, "attrload", "cannot open data attr file");
        return -1;
    }
    while ((line = sfgetr (fp, '\n', 1))) {
        if (!(s1 = strchr (line, '='))) {
            SUwarning (0, "attrload", "bad line: %s", line);
            continue;
        }
        *s1++ = 0;
        for (attri = 0; attri < ATTRSIZE; attri++)
            if (strcmp (line, attrnames[attri]) == 0)
                break;
        if (attri == ATTRSIZE)
            continue;
        if (!(attrs[attri] = vmstrdup (Vmheap, s1))) {
            SUwarning (0, "attrload", "cannot copy attr: %s", s1);
            return -1;
        }
    }
    sfclose (fp);
    return 0;
}
