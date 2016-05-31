#pragma prototyped

#include <ast.h>
#include <vmalloc.h>
#include <recsort.h>
#include <dlfcn.h>
#include <swift.h>
#include <dds.h>
#include <dds_int.h>

static char *createsorterstr (DDSschema_t *, char *, int, int *, int *, int *);
static int loadsorterso (
    DDSschema_t *, char *, int, DDSsorter_t *, int *, int *
);

DDSsorter_t *DDScreatesorter (
    DDSschema_t *schemap, char *klist,
    int rsflags, char *ccflags, char *ldflags, int flags
) {
    DDSsorter_t *sorterp;
    char *cstr, *sostr;
    int off, len;
    int usework;

    if (!(sorterp = vmalloc (Vmheap, sizeof (DDSsorter_t)))) {
        SUwarning (1, "DDScreatesorter", "vmalloc failed for sorterp");
        return NULL;
    }
    memset (sorterp, 0, sizeof (DDSsorter_t));
    if (!(cstr = createsorterstr (
        schemap, klist, flags, &usework, &off, &len
    ))) {
        SUwarning (1, "DDScreatesorter", "createsorterstr failed");
        return NULL;
    }
    if (!usework) {
        sorterp->rsdisc.type = RS_KSAMELEN | RS_DSAMELEN;
        sorterp->rsdisc.data = schemap->recsize;
        sorterp->rsdisc.key = off;
        sorterp->rsdisc.keylen = len;
        sorterp->rsdisc.defkeyf = NULL;
        sorterp->rsdisc.eventf = NULL;
        if (!(sorterp->rsp = rsopen (
            &sorterp->rsdisc, Rsrasp, 16777216 / schemap->recsize, rsflags
        ))) {
            SUwarning (1, "DDScreatesorter", "rsopen failed (1)");
            return NULL;
        }
        sorterp->handle = NULL;
        sorterp->init = NULL;
        sorterp->term = NULL;
        sorterp->work = NULL;
        return sorterp;
    }
    if (!(sostr = _ddscreateso (cstr, NULL, NULL, ccflags, ldflags))) {
        SUwarning (1, "DDScreatesorter", "createso failed");
        return NULL;
    }
    if (loadsorterso (schemap, sostr, TRUE, sorterp, &off, &len) == -1) {
        SUwarning (1, "DDScreatesorter", "loadsorterso failed");
        return NULL;
    }
    sorterp->rsdisc.type = RS_KSAMELEN | RS_DSAMELEN;
    sorterp->rsdisc.data = schemap->recsize;
    sorterp->rsdisc.key = 1;
    sorterp->rsdisc.keylen = len;
    sorterp->rsdisc.defkeyf = sorterp->work;
    sorterp->rsdisc.eventf = NULL;
    if (!(sorterp->rsp = rsopen (
        &sorterp->rsdisc, Rsrasp, 16777216 / schemap->recsize, rsflags
    ))) {
        SUwarning (1, "DDScreatesorter", "rsopen failed (2)");
        return NULL;
    }
    return sorterp;
}

char *DDScompilesorter (
    DDSschema_t *schemap, char *klist, char *sostr,
    char *ccflags, char *ldflags, int flags
) {
    char *cstr;
    int off, len;
    int usework;

    if (!(cstr = createsorterstr (
        schemap, klist, flags, &usework, &off, &len
    ))) {
        SUwarning (1, "DDScompilesorter", "createsorterstr failed");
        return NULL;
    }
    if (!usework) {
        SUwarning (
            1, "DDScompilesorter", "SO inappropriate for sequencial key"
        );
        return NULL;
    }
    if (!(sostr = _ddscreateso (cstr, NULL, sostr, ccflags, ldflags))) {
        SUwarning (1, "DDScompilesorter", "createso failed");
        return NULL;
    }
    return sostr;
}

DDSsorter_t *DDSloadsorter (DDSschema_t *schemap, char *sostr, int rsflags) {
    DDSsorter_t *sorterp;
    char file[PATH_MAX];
    int off, len;

    if (!(sorterp = vmalloc (Vmheap, sizeof (DDSsorter_t)))) {
        SUwarning (1, "DDSloadsorter", "vmalloc failed for sorterp");
        return NULL;
    }
    memset (sorterp, 0, sizeof (DDSsorter_t));
    if (access (sostr, R_OK) == 0) {
        if (strchr (sostr, '/') == NULL)
            file[0] = '.', file[1] = '/', strcpy (file + 2, sostr);
        else
            strcpy (file, sostr);
    } else if (
        !pathaccess (getenv ("PATH"), "../lib/dds", sostr, R_OK, file, PATH_MAX)
    ) {
        SUwarning (1, "DDSloadsorter", "access failed for %s", sostr);
        return NULL;
    }
    if (loadsorterso (schemap, file, FALSE, sorterp, &off, &len) == -1) {
        SUwarning (1, "DDSloadsorter", "loadsorterso failed");
        return NULL;
    }
    sorterp->rsdisc.type = RS_KSAMELEN | RS_DSAMELEN;
    sorterp->rsdisc.data = schemap->recsize;
    sorterp->rsdisc.key = 1;
    sorterp->rsdisc.keylen = len;
    sorterp->rsdisc.defkeyf = sorterp->work;
    sorterp->rsdisc.eventf = NULL;
    if (!(sorterp->rsp = rsopen (
        &sorterp->rsdisc, Rsrasp, 16777216 / schemap->recsize, rsflags
    ))) {
        SUwarning (1, "DDSloadsorter", "rsopen failed (2)");
        return NULL;
    }
    return sorterp;
}

int DDSdestroysorter (DDSsorter_t *sorterp) {
    rsclose (sorterp->rsp);
    if (sorterp->handle)
        dlclose (sorterp->handle);
    vmfree (Vmheap, sorterp);
    return 0;
}

static char *createsorterstr (
    DDSschema_t *schemap, char *klist, int fastmode,
    int *useworkp, int *offp, int *lenp
) {
    DDSfield_t **fieldps;
    int fieldpn, fieldpi;
    int off, len, aoff, alen, usework;
    Sfio_t *fp;
    DDSfield_t *fieldp;
    int fieldi;
    int i;
    char *cstr;

    if ((fieldpn = _ddslist2fields (
        klist, schemap, NULL, &fieldps, NULL
    )) < 0) {
        SUwarning (1, "createsorterstr", "list2fields failed");
        return NULL;
    }
    len = alen = fieldps[0]->off;
    usework = FALSE;
    off = fieldps[0]->off;
    for (fieldpi = 0; fieldpi < fieldpn; fieldpi++) {
        len += (
            _ddsfieldtypesize[fieldps[fieldpi]->type] * fieldps[fieldpi]->n
        );
        aoff = (
            (
                alen + _ddsfieldtypesize[fieldps[fieldpi]->type] - 1
            ) / _ddsfieldtypesize[fieldps[fieldpi]->type]
        ) * _ddsfieldtypesize[fieldps[fieldpi]->type];
        alen = aoff + (
            _ddsfieldtypesize[fieldps[fieldpi]->type] * fieldps[fieldpi]->n
        );
        if (fieldps[fieldpi]->off != aoff || len != alen)
            usework = TRUE;
        if (!fastmode && !fieldps[fieldpi]->isunsigned)
            usework = TRUE;
    }
    *useworkp = usework;
    *offp = off;
    len -= off;
    *lenp = len;
    if (!usework)
        return "";

    if (!(fp = sfopen (NULL, NULL, "sw+"))) {
        SUwarning (1, "createsorterstr", "sfopen failed");
        return NULL;
    }

    sfprintf (fp, "#include <ast.h>\n#include <cdt.h>\n");
    sfprintf (fp, "#include <swift.h>\n#include <dds.h>\n");
    sfprintf (fp, "#include <vmalloc.h>\n#include <recsort.h>\n\n");
    if (schemap->include)
        sfprintf (fp, "%s\n\n", schemap->include);
    _ddswritestruct (schemap, sfprints ("rec_%s", schemap->name), fp);
    for (fieldi = 0; fieldi < schemap->fieldn; fieldi++) {
        fieldp = &schemap->fields[fieldi];
        sfprintf (fp, "#define %s __recp->%s\n", fieldp->name, fieldp->name);
    }
    sfprintf (fp, "\n");
    sfprintf (fp,
        "DDS_EXPORT int sorter_%s_off = %d, sorter_%s_len = %d;\n\n",
    schemap->name, off, schemap->name, len);
    sfprintf (fp,
        "DDS_EXPORT int sorter_%s_init (void) {\n"
        "    return 0;\n"
        "}\n\n",
    schemap->name);
    sfprintf (fp,
        "DDS_EXPORT int sorter_%s_term (void) {\n"
        "    return 0;\n"
        "}\n\n",
    schemap->name);
    sfprintf (fp,
        "DDS_EXPORT ssize_t sorter_%s_work (\n"
        "    Rs_t *__rsp, void *__dp, ssize_t __dn,\n"
        "    void *__kp, ssize_t __kn, Rsdisc_t *__discp\n"
        ") {\n"
        "    unsigned char *__key;\n"
        "    struct __rec_%s_t *__recp;\n\n"
        "    __key = __kp;\n"
        "    __recp = (struct __rec_%s_t *) __dp;\n",
    schemap->name, schemap->name, schemap->name);
    for (fieldpi = 0, len = 0; fieldpi < fieldpn; fieldpi++) {
        fieldp = fieldps[fieldpi];
        sfprintf (
            fp,
            "    memcpy (&__key[%d], &%s, %d);\n",
            len, fieldp->name, _ddsfieldtypesize[fieldp->type] * fieldp->n
        );
        if (!fieldp->isunsigned) {
            if (
                fieldp->type != DDS_FIELD_TYPE_FLOAT &&
                fieldp->type != DDS_FIELD_TYPE_DOUBLE
            ) {
                sfprintf (
                    fp, "    __key[%d] ^= 0x80;\n", len
                );
            } else {
                sfprintf (
                    fp,
                    "    if (!(__key[%d] & 0x80))\n"
                    "        __key[%d] ^= 0x80;\n"
                    "    else {\n",
                    len, len
                );
                for (
                    i = 0; i < _ddsfieldtypesize[fieldp->type] * fieldp->n; i++
                )
                    sfprintf (fp, "    __key[%d + %d] ^= 0xFF;\n", len, i );
                sfprintf (fp, "    }\n");
            }
        }
        len += (_ddsfieldtypesize[fieldp->type] * fieldp->n);
    }
    sfprintf (fp, "    return %d;\n}\n", len);

    if (!(cstr = _ddscopystr (fp))) {
        SUwarning (1, "createsorterstr", "copystr failed");
        return NULL;
    }
    sfclose (fp);
    return cstr;
}

static int loadsorterso (
    DDSschema_t *schemap, char *sofile, int rmflag,
    DDSsorter_t *sorterp, int *offp, int *lenp
) {
    int *i1p, *i2p;

    if (
        !(sorterp->handle = dlopen (sofile, RTLD_LAZY)) ||
        !(sorterp->init = (DDSsorterinitfunc) dlsym (
            sorterp->handle, sfprints ("sorter_%s_init", schemap->name)
        )) ||
        !(sorterp->term = (DDSsortertermfunc) dlsym (
            sorterp->handle, sfprints ("sorter_%s_term", schemap->name)
        )) ||
        !(sorterp->work = (DDSsorterworkfunc) dlsym (
            sorterp->handle, sfprints ("sorter_%s_work", schemap->name)
        )) ||
        !(i1p = (int *) dlsym (
            sorterp->handle, sfprints ("sorter_%s_off", schemap->name)
        )) ||
        !(i2p = (int *) dlsym (
            sorterp->handle, sfprints ("sorter_%s_len", schemap->name)
        ))
    ) {
        SUwarning (1, "loadsorterso", "init failed: %s", dlerror ());
        return -1;
    }
    if (rmflag)
        unlink (sofile);
    *offp = *i1p, *lenp = *i2p;
    return 0;
}
