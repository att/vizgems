
#define ATTR_RUNID           0
#define ATTR_RENDERMODE      1
#define ATTR_MAXPIXEL        2
#define ATTR_PAGEMODE        3

#define ATTR_LAYOUTSTYLE     4
#define ATTR_GRAPHLEVEL      5
#define ATTR_PCLUSTERLEVEL   6
#define ATTR_SCLUSTERLEVEL   7
#define ATTR_RANKLEVEL       8
#define ATTR_GRAPHATTR       9
#define ATTR_SGRAPHATTR     10
#define ATTR_PCLUSTERATTR   11
#define ATTR_SCLUSTERATTR   12
#define ATTR_RANKATTR       13
#define ATTR_PNODEATTR      14
#define ATTR_SNODEATTR      15
#define ATTR_PPEDGEATTR     16
#define ATTR_PSEDGEATTR     17
#define ATTR_SPEDGEATTR     18
#define ATTR_SSEDGEATTR     19
#define ATTR_GRAPHURL       20
#define ATTR_SGRAPHURL      21
#define ATTR_PCLUSTERURL    22
#define ATTR_SCLUSTERURL    23
#define ATTR_RANKURL        24
#define ATTR_PNODEURL       25
#define ATTR_SNODEURL       26
#define ATTR_PPEDGEURL      27
#define ATTR_PSEDGEURL      28
#define ATTR_SPEDGEURL      29
#define ATTR_SSEDGEURL      30

#define ATTR_FRAMEN         31
#define ATTR_METRICORDER    32
#define ATTR_SORTORDER      33
#define ATTR_SORTURL        34
#define ATTR_GROUPORDER     35
#define ATTR_DISPLAYMODE    36
#define ATTR_STATCHARTATTR  37
#define ATTR_TITLEATTR      38
#define ATTR_METRICATTR     39
#define ATTR_XAXISATTR      40
#define ATTR_YAXISATTR      41
#define ATTR_STATCHARTURL   42
#define ATTR_TITLEURL       43
#define ATTR_METRICURL      44
#define ATTR_XAXISURL       45
#define ATTR_YAXISURL       46

#define ATTR_ALARMGRIDATTR  47
#define ATTR_ALARMATTR      48
#define ATTR_ALARMGROUP     49
#define ATTR_ALARMGRIDURL   50

#define ATTR_ALARMSTYLEATTR 51

#define ATTR_INVMAPATTR     52

#define ATTR_INVTABATTR     53

#define ATTR_ALARMTABATTR   54

#define ATTR_INVGRIDATTR    55

#define ATTR_STATTABATTR    56

#define ATTR_THISURL        57

#define ATTRSIZE 58

static char *attrnames[ATTRSIZE] = {
    "runid",
    "rendermode",
    "maxpixel",
    "pagemode",

    "layoutstyle",
    "graphlevel",
    "pclusterlevel",
    "sclusterlevel",
    "ranklevel",
    "graphattr",
    "sgraphattr",
    "pclusterattr",
    "sclusterattr",
    "rankattr",
    "pnodeattr",
    "snodeattr",
    "ppedgeattr",
    "psedgeattr",
    "spedgeattr",
    "ssedgeattr",
    "graphurl",
    "sgraphurl",
    "pclusterurl",
    "sclusterurl",
    "rankurl",
    "pnodeurl",
    "snodeurl",
    "ppedgeurl",
    "psedgeurl",
    "spedgeurl",
    "ssedgeurl",

    "framen",
    "metricorder",
    "sortorder",
    "sorturl",
    "grouporder",
    "displaymode",
    "statchartattr",
    "titleattr",
    "metricattr",
    "xaxisattr",
    "yaxisattr",
    "statcharturl",
    "titleurl",
    "metricurl",
    "xaxisurl",
    "yaxisurl",

    "alarmgridattr",
    "alarmattr",
    "alarmgroup",
    "alarmgridurl",

    "alarmstyleattr",

    "invmapattr",

    "invtabattr",

    "alarmtabattr",

    "invgridattr",

    "stattabattr",

    "thisurl",
};

#define ATTRENC_NONE   0
#define ATTRENC_SIMPLE 1
#define ATTRENC_URL    2
#define ATTRENC_HTML   3

#define ATTRENC_METAMODE   0
#define ATTRENC_NOMETAMODE 1

typedef struct attr_s {
    char *str;
    char *level1, *id1, *level2, *id2;
    int encmode;
} attr_t;

static attr_t attrs[ATTRSIZE];

static char *(*attrfunc) (char *, char *, char *, char *, char *);

#define ATTRBUFLEN 10240
static char attrbuf[ATTRBUFLEN];

static char *attrstr;
static int attrlen, attrpos;

#define SZ_level VG_inv_node_level_L
#define SZ_id VG_inv_node_id_L
#define SZ_key VG_inv_node_key_L

static char *baseattrfunc (
    char *level1, char *id1, char *level2, char *id2, char *key
) {
    if (!key || key[0] != '_')
        return NULL;

    if (strcmp (key, "_object_") == 0) {
        if (level2 && level2[0] && id2 && id2[0])
            sfsprintf (
                attrbuf, ATTRBUFLEN, "e|%s|%s|%s|%s",
                level1, id1, level2, id2
            );
        else
            sfsprintf (attrbuf, ATTRBUFLEN, "n:%s:%s", level1, id1);
        return &attrbuf[0];
    }
    if (strcmp (key, "_id_") == 0 || strcmp (key, "_id1_") == 0) {
        return &id1[0];
    }
    if (strcmp (key, "_id2_") == 0) {
        return &id2[0];
    }
    if (strcmp (key, "_level_") == 0 || strcmp (key, "_level1_") == 0) {
        return &level1[0];
    }
    if (strcmp (key, "_level2_") == 0) {
        return &level2[0];
    }
    if (strncmp (key, "_str_", 5) == 0) {
        return &key[5];
    }

    return NULL;
}

static int initstr (void) {
    if (!(attrstr = vmalloc (Vmheap, 128 * sizeof (char))))
        SUerror ("initstr", "cannot grow attrstr");
    attrlen = 128;
    attrpos = 0;

    return 0;
}

static int appendstr (char *str, int encmode, int encsubmode) {
    int len, sm;

    if ((len = strlen (str)) == 0)
        return 0;

    switch (encmode) {
    case ATTRENC_SIMPLE: len *= 2; break;
    case ATTRENC_URL:    len *= 3; break;
    case ATTRENC_HTML:   len *= 6; break;
    }
    sm = TRUE;
    switch (encsubmode) {
    case ATTRENC_METAMODE:   sm = TRUE;  break;
    case ATTRENC_NOMETAMODE: sm = FALSE; break;
    }
    if (attrpos + len >= attrlen) {
        if (!(attrstr = vmresize (
            Vmheap, attrstr, (attrlen + 2 * len) * sizeof (char), VM_RSCOPY
        )))
            SUerror ("appendstr", "cannot grow attrstr");
        attrlen += (2 * len);
    }
    switch (encmode) {
    case ATTRENC_NONE:
        strcpy (attrstr + attrpos, str);
        break;
    case ATTRENC_SIMPLE:
        len = VG_simpleenc (str, attrstr + attrpos, attrlen - attrpos, "'\"");
        break;
    case ATTRENC_URL:
        len = VG_urlenc (str, attrstr + attrpos, attrlen - attrpos);
        break;
    case ATTRENC_HTML:
        len = VG_htmlenc (str, attrstr + attrpos, attrlen - attrpos, sm);
        break;
    }
    attrpos += len;

    return 0;
}

static int termstr (void) {
    attrstr[attrpos] = 0;

    return 0;
}

static int expandspec (attr_t *attrp, char *level, char *key, int lastflag) {
    sl_inv_edgeattr_t iea, *ieap;
    sl_inv_nodeattr_t ina, *inap;
    char olevel1[SZ_level], olevel2[SZ_level], okey[SZ_key], *m1p, *m2p, *s;
    int matchn;

    if (!key[0])
        return 1;

    memset (olevel1, 0, SZ_level);
    memset (olevel2, 0, SZ_level);
    memset (okey, 0, SZ_key);
    if (level && level[0]) {
        strcpy (olevel1, level);
        strcpy (olevel2, level);
    } else {
        strcpy (olevel1, attrp->level1);
        if (attrp->level2 && attrp->level2[0] && attrp->id2 && attrp->id2[0])
            strcpy (olevel2, attrp->level2);
    }
    strcpy (okey, key);

    if (attrfunc && (s = (*attrfunc) (
        attrp->level1, attrp->id1, attrp->level2, attrp->id2, okey
    ))) {
        appendstr (s, attrp->encmode, ATTRENC_NOMETAMODE);
        return 1;
    }
    if (!level || !level[0]) {
        if (okey[0] == '_' && (s = baseattrfunc (
            attrp->level1, attrp->id1, attrp->level2, attrp->id2, okey
        ))) {
            appendstr (s, attrp->encmode, ATTRENC_METAMODE);
            return 1;
        }
    }

    if (attrp->level2 && attrp->level2[0] && attrp->id2 && attrp->id2[0]) {
        matchn = 0;
        for (
            m1p = M1F (attrp->level1, attrp->id1, olevel1); m1p;
            m1p = M1N (attrp->level1, attrp->id1, olevel1)
        ) {
            for (
                m2p = M2F (attrp->level2, attrp->id2, olevel2); m2p;
                m2p = M2N (attrp->level2, attrp->id2, olevel2)
            ) {
                s = NULL;
                if (okey[0] == '_') {
                    s = baseattrfunc (
                        olevel1, m1p, olevel2, m2p, okey
                    );
                } else {
                    memset (&iea, 0, sizeof (sl_inv_edgeattr_t));
                    strcpy (iea.sl_key, okey);
                    if ((ieap = sl_inv_edgeattrfind (
                        olevel1, m1p, olevel2, m2p, iea.sl_key
                    )))
                        s = ieap->sl_val;
                }
                if (s) {
                    if (matchn > 0) {
                        if (attrp->encmode == ATTRENC_URL)
                            appendstr ("|", attrp->encmode, ATTRENC_METAMODE);
                        else {
                            appendstr ("...", attrp->encmode, ATTRENC_METAMODE);
                            return matchn;
                        }
                    }
                    appendstr (s, attrp->encmode, ATTRENC_METAMODE);
                    matchn++;
                }
            }
        }
        if (matchn == 0 && lastflag) {
            appendstr (attrp->id1, attrp->encmode, ATTRENC_METAMODE);
            appendstr ("-", attrp->encmode, ATTRENC_METAMODE);
            appendstr (attrp->id2, attrp->encmode, ATTRENC_METAMODE);
            matchn++;
        }
    } else {
        matchn = 0;
        for (
            m1p = M1F (attrp->level1, attrp->id1, olevel1); m1p;
            m1p = M1N (attrp->level1, attrp->id1, olevel1)
        ) {
            s = NULL;
            if (okey[0] == '_') {
                s = baseattrfunc (
                    olevel1, m1p, NULL, NULL, okey
                );
            } else {
                memset (&ina, 0, sizeof (sl_inv_nodeattr_t));
                strcpy (ina.sl_key, okey);
                if ((inap = sl_inv_nodeattrfind (
                    olevel1, m1p, ina.sl_key
                )))
                    s = inap->sl_val;
            }
            if (s) {
                if (matchn > 0) {
                    if (attrp->encmode == ATTRENC_URL)
                        appendstr ("|", attrp->encmode, ATTRENC_METAMODE);
                    else {
                        appendstr ("...", attrp->encmode, ATTRENC_METAMODE);
                        return matchn;
                    }
                }
                appendstr (s, attrp->encmode, ATTRENC_METAMODE);
                matchn++;
            }
        }
        if (matchn == 0 && lastflag) {
            appendstr (attrp->id1, attrp->encmode, ATTRENC_METAMODE);
            matchn++;
        }
    }

    return matchn;
}

static int attrload (char *file) {
    Sfio_t *fp;
    char *line, *s1;
    attr_t *attrp;
    int attri;

    if (!(fp = sfopen (NULL, file, "r")))
        SUerror ("attrload", "cannot open attr file");
    while ((line = sfgetr (fp, '\n', 1))) {
        if (!(s1 = strchr (line, '='))) {
            if (!line[0])
                continue;
            SUwarning (0, "attrload", "bad line: %s", line);
            continue;
        }
        *s1++ = 0;
        for (attri = 0; attri < ATTRSIZE; attri++)
            if (strcmp (line, attrnames[attri]) == 0)
                break;
        if (attri == ATTRSIZE)
            continue;
        attrp = &attrs[attri];
        memset (attrp, 0, sizeof (attr_t));
        if (!(attrp->str = vmstrdup (Vmheap, s1)))
            SUerror ("attrload", "cannot allocate str for line %s", s1);
    }
    sfclose (fp);
    return 0;
}

static int attreval (
    char *level1, char *id1, char *level2, char *id2, int attri, int encmode
) {
    attr_t *attrp;
    char *s1, *s2, *s3, *s4, *s5, *s6, *s7, *s8;
    int c2, c3, c5, c6;

    attrp = &attrs[attri];
    attrp->level1 = level1, attrp->id1 = id1;
    attrp->level2 = level2, attrp->id2 = id2;
    attrp->encmode = encmode;

    initstr ();
    for (s1 = attrp->str; s1; ) {
        if (!(s2 = strstr (s1, "%(")) || !(s3 = strstr (s2 + 2, ")%"))) {
            appendstr (s1, ATTRENC_NONE, ATTRENC_METAMODE);
            break;
        }
        c2 = *s2; *s2 = 0;
        appendstr (s1, ATTRENC_NONE, ATTRENC_METAMODE);
        *s2 = c2;
        c3 = *s3, *s3 = 0;
        for (s4 = s2 + 2; s4; ) {
            if (!(s5 = strchr (s4, '/'))) {
                if ((s6 = strchr (s4, '.')))
                    c6 = *s6, *s6 = 0, s7 = s4, s8 = s6 + 1;
                else
                    s7 = NULL, s8 = s4;
                expandspec (attrp, s7, s8, TRUE);
                if (s6)
                    *s6 = c6;
                break;
            }
            c5 = *s5, *s5 = 0;
            if ((s6 = strchr (s4, '.')))
                c6 = *s6, *s6 = 0, s7 = s4, s8 = s6 + 1;
            else
                s7 = NULL, s8 = s4;
            if (expandspec (attrp, s7, s8, FALSE) > 0) {
                if (s6)
                    *s6 = c6;
                *s5 = c5;
                break;
            }
            if (s6)
                *s6 = c6;
            *s5 = c5;
            s4 = s5 + 1;
        }
        *s3 = c3;
        s1 = s3 + 2;
    }
    termstr ();

    return 0;
}

static int attrsetfunc (
    char *(*func) (char *, char *, char *, char *, char *)
) {
    attrfunc = func;
    return 0;
}
