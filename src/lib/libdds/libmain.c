#pragma prototyped

#include <ast.h>
#include <ctype.h>
#include <vmalloc.h>
#include <tok.h>
#include <swift.h>
#include <dds.h>
#include <dds_int.h>

char **_ddsfieldtypestring;
int *_ddsfieldtypesize;

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

static char *fieldtypestring[] = {
    NULL,
    /* DDS_FIELD_TYPE_CHAR */     "char",
    /* DDS_FIELD_TYPE_SHORT */    "short",
    /* DDS_FIELD_TYPE_INT */      "int",
    /* DDS_FIELD_TYPE_LONG */     "long",
    /* DDS_FIELD_TYPE_LONGLONG */ "long long",
    /* DDS_FIELD_TYPE_FLOAT */    "float",
    /* DDS_FIELD_TYPE_DOUBLE */   "double"
};

static int fieldtypesize[] = {
    0,
    /* DDS_FIELD_TYPE_CHAR */     sizeof (char),
    /* DDS_FIELD_TYPE_SHORT */    sizeof (short),
    /* DDS_FIELD_TYPE_INT */      sizeof (int),
    /* DDS_FIELD_TYPE_LONG */     sizeof (long),
    /* DDS_FIELD_TYPE_LONGLONG */ sizeof (long long),
    /* DDS_FIELD_TYPE_FLOAT */    sizeof (float),
    /* DDS_FIELD_TYPE_DOUBLE */   sizeof (double)
};

static int nocompress;

int DDSinit (void) {
    _ddsfieldtypestring = fieldtypestring;
    _ddsfieldtypesize = fieldtypesize;
    nocompress = getenv ("DDSNOCOMPRESS") ? TRUE : FALSE;
    return 0;
}

int DDSterm (void) {
    return 0;
}

DDSschema_t *DDScreateschema (
    char *name, DDSfield_t *fields, int fieldn, char *preload, char *include
) {
    Vmalloc_t *vm;
    DDSschema_t *schemap;
    DDSfield_t *fieldp;
    int fieldi;

    if (!(vm = vmopen (Vmdcsbrk, Vmbest, 0))) {
        SUwarning (1, "DDScreateschema", "vmopen failed");
        return NULL;
    }
    if (!(schemap = vmalloc (vm, sizeof (DDSschema_t)))) {
        SUwarning (1, "DDScreateschema", "vmalloc failed for schemap");
        return NULL;
    }
    memset (schemap, 0, sizeof (DDSschema_t));
    schemap->vm = vm;
    if (
        !(schemap->name = vmstrdup (vm, name)) ||
        (preload && !(schemap->preload = vmstrdup (vm, preload))) ||
        (include && !(schemap->include = vmstrdup (vm, include)))
    ) {
        SUwarning (1, "DDScreateschema", "vmstrdup failed");
        return NULL;
    }
    schemap->fieldn = fieldn;
    if (!(schemap->fields = vmalloc (vm, fieldn * sizeof (DDSfield_t)))) {
        SUwarning (1, "DDScreateschema", "vmalloc failed for fields");
        return NULL;
    }
    schemap->recsize = 0;
    for (fieldi = 0; fieldi < fieldn; fieldi++) {
        fieldp = &schemap->fields[fieldi];
        if (!(fieldp->name = vmstrdup (vm, fields[fieldi].name))) {
            SUwarning (
                1, "DDScreateschema", "vmstrdup failed for field %d", fieldi
            );
            return NULL;
        }
        fieldp->type = fields[fieldi].type;
        fieldp->isunsigned = fields[fieldi].isunsigned;
        fieldp->n = fields[fieldi].n;
        fieldp->off = ((
            schemap->recsize + _ddsfieldtypesize[fieldp->type] - 1
        ) / _ddsfieldtypesize[fieldp->type]) * _ddsfieldtypesize[fieldp->type];
        schemap->recsize = fieldp->off + (
            _ddsfieldtypesize[fieldp->type] * fieldp->n
        );
        if (fieldp->n <= 0) {
            SUwarning (
                1, "DDScreateschema",
                "number of elements not positive for field %d", fieldi
            );
            return NULL;
        }
    }
    if (!(schemap->recbuf = vmalloc (vm, schemap->recsize * sizeof (char)))) {
        SUwarning (1, "DDScreateschema", "vmalloc failed for recbuf");
        return NULL;
    }
    return schemap;
}

int DDSdestroyschema (DDSschema_t *schemap) {
    if (schemap && schemap->vm)
        vmclose (schemap->vm);
    return 0;
}

DDSschema_t *DDSloadschema (char *fname, Sfio_t *sfp, int *lenp) {
    char file[PATH_MAX];
    Sfio_t *fp;
    Vmalloc_t *vm;
    DDSschema_t *schemap;
    int seenbegin;
    char *line;
    int getra, getrb, len;
    char *av[100];
    int an, ai;
    char *s;
    DDSfield_t *fieldp;

    if (fname) {
        getra = '\n', getrb = SF_STRING;
        if (access (fname, R_OK) == 0)
            strcpy (file, fname);
        else if (!pathaccess (
            getenv ("PATH"), "../lib/dds", fname, R_OK, file, PATH_MAX
        )) {
            SUwarning (1, "DDSloadschema", "access failed for %s", fname);
            return NULL;
        }
        if (!(fp = sfopen (NULL, file, "r"))) {
            SUwarning (1, "DDSloadschema", "open failed for %s", file);
            return NULL;
        }
    } else
        fp = sfp, getra = 0, getrb = 0;
    if (!(vm = vmopen (Vmdcsbrk, Vmbest, 0))) {
        SUwarning (1, "DDSloadschema", "vmopen failed");
        return NULL;
    }
    if (!(schemap = vmalloc (vm, sizeof (DDSschema_t)))) {
        SUwarning (1, "DDSloadschema", "vmalloc failed for schemap");
        return NULL;
    }
    schemap->vm = vm;
    schemap->fields = NULL, schemap->fieldn = 0;
    schemap->preload = NULL;
    schemap->include = NULL;
    schemap->recsize = 0;
    seenbegin = FALSE;
    len = 0;
    while ((line = sfgetr (fp, getra, getrb))) {
        len += strlen (line) + 1;
        if ((s = strchr (line, '#')))
            *s = 0;
        if ((an = tokscan (line, &s, " %v ", av, 100)) < 0) {
            SUwarning (1, "DDSloadschema", "tokscan failed");
            return NULL;
        }
        if (an == 0)
            continue;
        if (strcmp (av[0], "SCHEMABEGIN") == 0) {
            seenbegin = TRUE;
        } else if (strcmp (av[0], "SCHEMAEND") == 0) {
            if (!seenbegin) {
                SUwarning (1, "DDSloadschema", "unmatched end statement");
                return NULL;
            }
            break;
        } else if (strcmp (av[0], "name") == 0) {
            if (an != 2) {
                SUwarning (1, "DDSloadschema", "bad name line");
                return NULL;
            }
            if (!(schemap->name = vmstrdup (vm, av[1]))) {
                SUwarning (1, "DDSloadschema", "vmstrdup failed (1)");
                return NULL;
            }
        } else if (strcmp (av[0], "field") == 0) {
            if (an < 3 || an > 6) {
                SUwarning (
                    1, "DDSloadschema", "bad field line %d", schemap->fieldn
                );
                return NULL;
            }
            if (!(schemap->fields = vmresize (
                vm, schemap->fields, ++schemap->fieldn * sizeof (DDSfield_t),
                VM_RSCOPY
            ))) {
                SUwarning (1, "DDSloadschema", "vmresize failed");
                return NULL;
            }
            fieldp = &schemap->fields[schemap->fieldn - 1];
            if (!(fieldp->name = vmstrdup (vm, av[1]))) {
                SUwarning (1, "DDSloadschema", "vmstrdup failed (2)");
                return NULL;
            }
            ai = 2;
            fieldp->isunsigned = 0, fieldp->n = 1;
            if (strcmp (av[ai], "unsigned") == 0)
                fieldp->isunsigned = 1, ai++;
            else if (strcmp (av[ai], "signed") == 0)
                fieldp->isunsigned = 0, ai++;
            if (strcmp (av[ai], "char") == 0)
                fieldp->type = DDS_FIELD_TYPE_CHAR;
            else if (strcmp (av[ai], "short") == 0)
                fieldp->type = DDS_FIELD_TYPE_SHORT;
            else if (strcmp (av[ai], "int") == 0)
                fieldp->type = DDS_FIELD_TYPE_INT;
            else if (strcmp (av[ai], "long") == 0) {
                fieldp->type = DDS_FIELD_TYPE_LONG;
                if (ai + 1 < an && strcmp (av[ai + 1], "long") == 0)
                    fieldp->type = DDS_FIELD_TYPE_LONGLONG, ai++;
            } else if (strcmp (av[ai], "float") == 0)
                fieldp->type = DDS_FIELD_TYPE_FLOAT;
            else if (strcmp (av[ai], "double") == 0)
                fieldp->type = DDS_FIELD_TYPE_DOUBLE;
            ai++;
            if (ai < an)
                fieldp->n = atoi (av[ai]);
            if (fieldp->n <= 0) {
                SUwarning (
                    1, "DDSloadschema",
                    "number of elements not positive: %d", fieldp->n
                );
                return NULL;
            }
            fieldp->off = (
                (schemap->recsize + _ddsfieldtypesize[fieldp->type] - 1) /
                _ddsfieldtypesize[fieldp->type]
            ) * _ddsfieldtypesize[fieldp->type];
            schemap->recsize = fieldp->off + (
                _ddsfieldtypesize[fieldp->type] * fieldp->n
            );
        } else if (strcmp (av[0], "preload") == 0) {
            if (an != 2) {
                SUwarning (
                    1, "DDSloadschema", "bad field line %d", schemap->fieldn
                );
                return NULL;
            }
            if (!(schemap->preload = vmstrdup (vm, av[1]))) {
                SUwarning (1, "DDSloadschema", "vmstrdup failed (3)");
                return NULL;
            }
        } else if (strcmp (av[0], "include") == 0) {
            if (an != 2) {
                SUwarning (
                    1, "DDSloadschema", "bad field line %d", schemap->fieldn
                );
                return NULL;
            }
            if (!(schemap->include = vmstrdup (vm, av[1]))) {
                SUwarning (1, "DDSloadschema", "vmstrdup failed (4)");
                return NULL;
            }
        }
    }
    if (!(schemap->recbuf = vmalloc (vm, schemap->recsize * sizeof (char)))) {
        SUwarning (1, "DDSloadschema", "vmalloc failed for recbuf");
        return NULL;
    }
    if (fp != sfp)
        sfclose (fp);
    if (lenp)
        *lenp = len;
    return schemap;
}

int DDSsaveschema (char *fname, Sfio_t *sfp, DDSschema_t *schemap, int *lenp) {
    Sfio_t *fp;
    DDSfield_t *fieldp;
    int fieldi;
    int putra, len;

    if (fname) {
        putra = '\n';
        if (!(fp = sfopen (NULL, fname, "w"))) {
            SUwarning (1, "DDSsaveschema", "open failed for %s", fname);
            return -1;
        }
    } else
        fp = sfp, putra = 0;
    len = 0;
    len += sfputr (fp, "SCHEMABEGIN", putra);
    len += sfputr (fp, sfprints ("name %s", schemap->name), putra);
    for (fieldi = 0; fieldi < schemap->fieldn; fieldi++) {
        fieldp = &schemap->fields[fieldi];
        len += sfputr (
            fp, sfprints ("field %s %s %s %d", fieldp->name,
            fieldp->isunsigned ? "unsigned" : "signed",
            _ddsfieldtypestring[fieldp->type], fieldp->n), putra
        );
    }
    if (schemap->preload)
        len += sfputr (fp, sfprints ("preload %s", schemap->preload), putra);
    if (schemap->include)
        len += sfputr (fp, sfprints ("include %s", schemap->include), putra);
    len += sfputr (fp, "SCHEMAEND", putra);
    if (fp != sfp)
        sfclose (fp);
    if (lenp)
        *lenp = len;
    return 0;
}

DDSfield_t *DDSfindfield (DDSschema_t *schemap, char *name) {
    int fieldi;

    for (fieldi = 0; fieldi < schemap->fieldn; fieldi++)
        if (strcmp (schemap->fields[fieldi].name, name) == 0)
            return &schemap->fields[fieldi];
    return NULL;
}

int DDSreadheader (Sfio_t *fp, DDSheader_t *hdrp) {
    int i, c;
    unsigned char buf[sizeof (int)];
    int version, len1, len2;

    if (!nocompress)
        sfsetbuf (fp, NULL, 0);
    hdrp->schemaname = NULL;
    hdrp->schemap = NULL;
    hdrp->vczspec = NULL;
    hdrp->vczp = NULL;
    for (i = 0; i < strlen (MAGIC); i++) {
        if ((c = sfgetc (fp)) != MAGIC[i]) {
            if (i == 0 && c == -1)
                return -2;
            if (c != -1)
                sfungetc (fp, c);
            for (i--; i >= 0; i--)
                sfungetc (fp, MAGIC[i]);
            return 1;
        }
    }

    if (sfread (fp, buf, 4) != 4) {
        SUwarning (1, "DDSreadheader", "read failed for version");
        return -1;
    }
    version = (
        buf[0] * 256 * 256 * 256 + buf[1] * 256 * 256 + buf[2] * 256 + buf[3]
    );
    if (version > VERSION) {
        SUwarning (1, "DDSreadheader", "cannot handle newer version");
        return -1;
    }
    if (sfread (fp, buf, 4) != 4) {
        SUwarning (1, "DDSreadheader", "read failed for contents");
        return -1;
    }
    hdrp->contents = (
        buf[0] * 256 * 256 * 256 + buf[1] * 256 * 256 + buf[2] * 256 + buf[3]
    );
    len1 = strlen (MAGIC) + 2 * sizeof (int);
    if (
        (hdrp->contents & DDS_HDR_NAME) &&
        (!(hdrp->schemaname = sfgetr (fp, 0, 0)) ||
        !(hdrp->schemaname = vmstrdup (Vmheap, hdrp->schemaname)))
    ) {
        SUwarning (1, "DDSreadheader", "read failed for schemaname");
        return -1;
    }
    len1 += strlen (hdrp->schemaname) + 1;
    if (
        (hdrp->contents & DDS_HDR_DESC) &&
        !(hdrp->schemap = DDSloadschema (NULL, fp, &len2))
    ) {
        SUwarning (1, "DDSreadheader", "DDSloadschema failed");
        return -1;
    }
    len1 += len2;
    if (version >= 4) {
        if (
            (!(hdrp->vczspec = sfgetr (fp, 0, 0)) ||
            !(hdrp->vczspec = vmstrdup (Vmheap, hdrp->vczspec)))
        ) {
            SUwarning (1, "DDSreadheader", "read failed for vczspec");
            return -1;
        }
        len1 += strlen (hdrp->vczspec) + 1;
    } else
        hdrp->vczspec = "";
    if (version > 2 && (len2 = len1 % hdrp->schemap->recsize) > 0) {
        len2 = hdrp->schemap->recsize - len2;
        while (len2--)
            sfgetc (fp);
    }
    hdrp->vczp = NULL;
    if (*hdrp->vczspec && !(hdrp->vczp = _ddsvczopeni (fp, hdrp->vczspec))) {
        SUwarning (1, "DDSreadheader", "cannot push vcodex disc");
        return -1;
    }
    if (!nocompress)
        sfsetbuf (fp, NULL, 1024 * 1024);
    return 0;
}

int DDSwriteheader (Sfio_t *fp, DDSheader_t *hdrp) {
    unsigned char buf[sizeof (int)];
    int version, len1, len2, zero;
    char *vczspec;

    version = VERSION;
    if (sfwrite (fp, MAGIC, strlen (MAGIC)) != 8) {
        SUwarning (1, "DDSwriteheader", "write failed for magic number");
        return -1;
    }
    buf[0] = version / (256 * 256 * 256);
    buf[1] = (version / (256 * 256)) % 256;
    buf[2] = (version / 256) % 256;
    buf[3] = version % 256;
    if (sfwrite (fp, buf, 4) != 4) {
        SUwarning (1, "DDSwriteheader", "write failed for version");
        return -1;
    }
    buf[0] = hdrp->contents / (256 * 256 * 256);
    buf[1] = (hdrp->contents / (256 * 256)) % 256;
    buf[2] = (hdrp->contents / 256) % 256;
    buf[3] = hdrp->contents % 256;
    if (sfwrite (fp, buf, 4) != 4) {
        SUwarning (1, "DDSwriteheader", "write failed for contents");
        return -1;
    }
    len1 = strlen (MAGIC) + 2 * sizeof (int);
    if (
        (hdrp->contents & DDS_HDR_NAME) &&
        sfputr (fp, hdrp->schemaname, 0) == -1
    ) {
        SUwarning (1, "DDSwriteheader", "write failed for schemaname");
        return -1;
    }
    len1 += strlen (hdrp->schemaname) + 1;
    if (
        (hdrp->contents & DDS_HDR_DESC) &&
        DDSsaveschema (NULL, fp, hdrp->schemap, &len2) == -1
    ) {
        SUwarning (1, "DDSwriteheader", "DDSsaveschema failed");
        return -1;
    }
    len1 += len2;
    vczspec = (hdrp->vczspec) ? hdrp->vczspec : "";
    if (strcmp (vczspec, "none") == 0)
        vczspec = "";
    else if (strcmp (vczspec, "rdb") == 0) {
        if (!(vczspec = _ddsvczgenrdb (hdrp->schemap))) {
            SUwarning (1, "DDSwriteheader", "cannot generate rdb spec");
            return -1;
        }
    } else if (strcmp (vczspec, "rtb") == 0) {
        if (!(vczspec = _ddsvczgenrtb (hdrp->schemap))) {
            SUwarning (1, "DDSwriteheader", "cannot generate rtb spec");
            return -1;
        }
    }
    if (sfputr (fp, vczspec, 0) == -1) {
        SUwarning (1, "DDSwriteheader", "write failed for vczspec");
        return -1;
    }
    len1 += strlen (vczspec) + 1;
    if ((len2 = len1 % hdrp->schemap->recsize) > 0) {
        zero = 0;
        len2 = hdrp->schemap->recsize - len2;
        while (len2--)
            sfputc (fp, zero);
    }
    hdrp->vczp = NULL;
    if (*vczspec && !(hdrp->vczp = _ddsvczopeno (
        fp, vczspec, hdrp->schemap->recsize
    ))) {
        SUwarning (1, "DDSwriteheader", "cannot push vcodex disc");
        return -1;
    }
    sfsetbuf (fp, NULL, 1024 * 1024);
    return 0;
}

int DDSparseexpr (char *str, char *fname, DDSexpr_t *exprp) {
    char file[PATH_MAX];
    Sfio_t *fp;
    char *s1, *s2, **sp;
    int qmode, brcount, c;

    if (fname) {
        if (access (fname, R_OK) == 0)
            strcpy (file, fname);
        else if (!pathaccess (
            getenv ("PATH"), "../lib/dds", fname, R_OK, file, PATH_MAX
        )) {
            SUwarning (1, "DDSparseexpr", "access failed for %s", fname);
            return -1;
        }
        if (!(fp = sfopen (NULL, file, "r"))) {
            SUwarning (1, "DDSparseexpr", "open failed for %s", file);
            return -1;
        }
        if (!(s1 = sfreserve (fp, sfsize (fp), 0))) {
            sfclose (fp);
            SUwarning (1, "DDSparseexpr", "sfreserve failed for %s", file);
            return -1;
        }
        if (!(str = vmalloc (Vmheap, sizeof (char) * (sfsize (fp) + 1)))) {
            SUwarning (1, "DDSparseexpr", "vmalloc failed for str");
            return -1;
        }
        if (!strncpy (str, s1, sfsize (fp))) {
            SUwarning (1, "DDSparseexpr", "strncpy failed for str");
            return -1;
        }
        str[sfsize (fp)] = 0;
    }
    exprp->vars = exprp->libs = exprp->begin = exprp->end = exprp->loop = NULL;
    for (s1 = str; *s1; ) {
        while (isspace (*s1))
            s1++;
        if (!*s1)
            break;
        if (strncmp (s1, "VARS", 4) == 0) {
            s1 += 4;
            sp = &exprp->vars;
        } else if (strncmp (s1, "LIBS", 4) == 0) {
            s1 += 4;
            sp = &exprp->libs;
        } else if (strncasecmp (s1, "BEGIN", 5) == 0) {
            s1 += 5;
            sp = &exprp->begin;
        } else if (strncasecmp (s1, "END", 3) == 0) {
            s1 += 3;
            sp = &exprp->end;
        } else if (strncasecmp (s1, "LOOP", 4) == 0) {
            s1 += 4;
            sp = &exprp->loop;
        } else if (*s1 == '{') {
            sp = &exprp->loop;
        } else {
            if (fname)
                sfclose (fp);
            SUwarning (1, "DDSparseexpr", "1parse error at %s", s1);
            return -1;
        }
        while (*s1 && *s1 != '{')
            s1++;
        if (*s1 != '{') {
            if (fname)
                sfclose (fp);
            SUwarning (1, "DDSparseexpr", "2parse error at %s", s1);
            return -1;
        }
        *sp = ++s1;
        qmode = 0;
        brcount = 1;
        for (; *s1; s1++) {
            if ((*s1 == '\'' || *s1 == '"') && *(s1 - 1) != '\\') {
                if (!qmode)
                    qmode = *s1;
                else if (qmode == *s1)
                    qmode = 0;
            } else if (*s1 == '{')
                brcount++;
            else if (*s1 == '}') {
                if (--brcount == 0)
                    break;
            }
        }
        if (*s1 != '}') {
            if (fname)
                sfclose (fp);
            SUwarning (1, "DDSparseexpr", "3parse error at %s", s1);
            return -1;
        }
        c = *s1, *s1 = 0;
        *sp = vmstrdup (Vmheap, *sp);
        if (sp == &exprp->libs) {
            for (s2 = *sp; *s2; s2++)
                if (*s2 == '\n' || *s2 == '\r')
                    *s2 = ' ';
        }
        *s1++ = c;
    }
    if (fname)
        sfclose (fp);
    return 0;
}

int DDSreadrecord (Sfio_t *fp, void *buf, DDSschema_t *schemap) {
    char *cbuf;
    DDSfield_t *fieldp;
    int fieldi;
    int ret;

    if ((ret = sfread (fp, buf, schemap->recsize)) != schemap->recsize) {
        if (ret == 0)
            return 0;
        SUwarning (1, "DDSreadrecord", "read failed");
        return -1;
    }
    cbuf = (char *) buf;
    for (fieldi = 0; fieldi < schemap->fieldn; fieldi++) {
        fieldp = &schemap->fields[fieldi];
        switch (fieldp->type) {
        case DDS_FIELD_TYPE_SHORT:
            SW2 (cbuf + fieldp->off);
            break;
        case DDS_FIELD_TYPE_INT:
        case DDS_FIELD_TYPE_LONG:
            SW4 (cbuf + fieldp->off);
            break;
        case DDS_FIELD_TYPE_LONGLONG:
            SW8 (cbuf + fieldp->off);
            break;
        case DDS_FIELD_TYPE_FLOAT:
            SW4 (cbuf + fieldp->off);
            break;
        case DDS_FIELD_TYPE_DOUBLE:
            SW8 (cbuf + fieldp->off);
            break;
        }
    }
    return schemap->recsize;
}

int DDSwriterecord (Sfio_t *fp, void *buf, DDSschema_t *schemap) {
    char *cbuf;
    DDSfield_t *fieldp;
    int fieldi;

    cbuf = (char *) buf;
    for (fieldi = 0; fieldi < schemap->fieldn; fieldi++) {
        fieldp = &schemap->fields[fieldi];
        switch (fieldp->type) {
        case DDS_FIELD_TYPE_SHORT:
            SW2 (cbuf + fieldp->off);
            break;
        case DDS_FIELD_TYPE_INT:
        case DDS_FIELD_TYPE_LONG:
            SW4 (cbuf + fieldp->off);
            break;
        case DDS_FIELD_TYPE_LONGLONG:
            SW8 (cbuf + fieldp->off);
            break;
        case DDS_FIELD_TYPE_FLOAT:
            SW4 (cbuf + fieldp->off);
            break;
        case DDS_FIELD_TYPE_DOUBLE:
            SW8 (cbuf + fieldp->off);
            break;
        }
    }
    if (sfwrite (fp, buf, schemap->recsize) != schemap->recsize) {
        SUwarning (1, "DDSwriterecord", "write failed");
        return -1;
    }
    return schemap->recsize;
}

int DDSswaprecord (void *buf, DDSschema_t *schemap) {
    char *cbuf;
    DDSfield_t *fieldp;
    int fieldi;

    cbuf = (char *) buf;
    for (fieldi = 0; fieldi < schemap->fieldn; fieldi++) {
        fieldp = &schemap->fields[fieldi];
        switch (fieldp->type) {
        case DDS_FIELD_TYPE_SHORT:
            SW2 (cbuf + fieldp->off);
            break;
        case DDS_FIELD_TYPE_INT:
        case DDS_FIELD_TYPE_LONG:
            SW4 (cbuf + fieldp->off);
            break;
        case DDS_FIELD_TYPE_LONGLONG:
            SW8 (cbuf + fieldp->off);
            break;
        case DDS_FIELD_TYPE_FLOAT:
            SW4 (cbuf + fieldp->off);
            break;
        case DDS_FIELD_TYPE_DOUBLE:
            SW8 (cbuf + fieldp->off);
            break;
        }
    }
    return 0;
}

int DDSreadrawrecord (Sfio_t *fp, void *buf, DDSschema_t *schemap) {
    int ret;

    if ((ret = sfread (fp, buf, schemap->recsize)) != schemap->recsize) {
        if (ret == 0)
            return 0;
        SUwarning (1, "DDSreadrawrecord", "read failed");
        return -1;
    }
    return schemap->recsize;
}

int DDSwriterawrecord (Sfio_t *fp, void *buf, DDSschema_t *schemap) {
    if (sfwrite (fp, buf, schemap->recsize) != schemap->recsize) {
        SUwarning (1, "DDSwriterawrecord", "write failed");
        return -1;
    }
    return schemap->recsize;
}

char *_ddscreateso (
    char *cstr, char *libstr, char *sostr, char *ccflags, char *ldflags
) {
    Sfio_t *cfp, *sofp, *pfp;
    int i, l;

    static char cfile[PATH_MAX], sofile[PATH_MAX + 2];

    for (i = 0; i < 20; i++) {
        sfsprintf (
            cfile, PATH_MAX, "/tmp/dds.%d.%d.%d.c",
            getuid (), getpid (), random ()
        );
        if (sostr)
            strcpy (sofile, sostr);
        else {
            strcpy (sofile, cfile);
            l = strlen (sofile);
            sofile[l - 1] = 's', sofile[l] = 'o', sofile[l + 1] = 0;
        }
        if (
            (cfp = sfopen (NULL, cfile, "w")) &&
            (sofp = sfopen (NULL, sofile, "w"))
        )
            break;
    }
    if (!cfp || !sofp) {
        SUwarning (1, "createso", "sfopen(s) failed");
        if (cfp)
            sfclose (cfp);
        return NULL;
    }
    if (!libstr)
        libstr = "";
    if (sfputr (cfp, cstr, -1) == -1) {
        SUwarning (1, "createso", "sfputr failed");
        sfclose (sofp), sfclose (cfp);
        return NULL;
    }
    sfclose (sofp), sfclose (cfp);
    if (!(pfp = sfpopen (
        NULL, sfprints (
            "ddsmkso -o %s -libs '%s' -ccflags '%s' -ldflags '%s' %s",
            sofile, libstr, ccflags, ldflags, cfile
        ), "w"
    ))) {
        SUwarning (1, "createso", "sfpopen failed");
        return NULL;
    }
    if (sfclose (pfp) != 0) {
        SUwarning (1, "createso", "sfclose failed");
        return NULL;
    }
    return &sofile[0];
}

int _ddswritestruct (DDSschema_t *schemap, char *name, Sfio_t *fp) {
    DDSfield_t *fieldp;
    int fieldi;

    sfprintf (fp, "struct __%s_t {\n", name);
    for (fieldi = 0; fieldi < schemap->fieldn; fieldi++) {
        fieldp = &schemap->fields[fieldi];
        if (fieldp->isunsigned)
            sfprintf (
                fp, "    unsigned %s %s",
                _ddsfieldtypestring[fieldp->type], fieldp->name
            );
        else
            sfprintf (
                fp, "    %s %s",
                _ddsfieldtypestring[fieldp->type], fieldp->name
            );
        if (fieldp->n > 1)
            sfprintf (fp, "[%d];\n", fieldp->n);
        else
            sfprintf (fp, ";\n");
    }
    sfprintf (fp, "};\n\n");
    for (fieldi = 0; fieldi < schemap->fieldn; fieldi++) {
        fieldp = &schemap->fields[fieldi];
        sfprintf (
            fp, "#define DDSSZ_%s_%s %d\n",
            name, fieldp->name, fieldp->n * fieldtypesize[fieldp->type]
        );
    }
    sfprintf (fp, "\n");
    return 0;
}

char *_ddscopystr (Sfio_t *fp) {
    char *s;
    int n;

    static char *bufs;
    static int bufn;

    n = sfsize (fp);
    if (n + 1 > bufn) {
        if (!(bufs = vmresize (Vmheap, bufs, (n + 1) * sizeof (char), 0))) {
            SUwarning (1, "copystr", "vmresize failed");
            return NULL;
        }
        bufn = n + 1;
    }
    if (sfseek (fp, 0, SEEK_SET) != 0 || !(s = sfreserve (fp, n, 0))) {
        SUwarning (1, "copystr", "sfreserve failed");
        return NULL;
    }
    strncpy (bufs, s, n);
    bufs[n] = 0;
    return bufs;
}

int _ddslist2fields (
    char *list, DDSschema_t *schemap,
    DDSfield_t **fields, DDSfield_t ***fieldps, int *lenp
) {
    int an, ai;
    char *s;
    DDSfield_t *fieldp;
    int fieldn, fieldi;
    int len;

    static char *av[1000];
    static DDSfield_t fs[1000], *fps[1000];

    if ((an = tokscan (list, &s, " %v ", av, 1000)) < 0) {
        SUwarning (1, "list2fields", "tokscan failed");
        return -1;
    }
    len = 0;
    for (ai = 0; ai < an; ai++) {
        for (fieldi = 0; fieldi < schemap->fieldn; fieldi++) {
            fieldp = &schemap->fields[fieldi];
            if (strcmp (fieldp->name, av[ai]) == 0)
                break;
        }
        if (fieldi == schemap->fieldn) {
            SUwarning (1, "list2fields", "no such field %s", av[ai]);
            return -1;
        }
        fps[ai] = &schemap->fields[fieldi];
        len += (_ddsfieldtypesize[fps[ai]->type] * fps[ai]->n);
    }
    fieldn = an;
    if (fieldps)
        *fieldps = &fps[0];
    if (fields) {
        for (fieldi = 0; fieldi < fieldn; fieldi++)
            fs[fieldi] = *fps[fieldi];
        *fields = &fs[0];
    }
    if (lenp)
        *lenp = len;
    return fieldn;
}
