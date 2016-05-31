#pragma prototyped

#include <ast.h>
#include <vmalloc.h>
#include <dlfcn.h>
#include <swift.h>
#include <dds.h>
#include <dds_int.h>

static char *createextractorstr (DDSschema_t *, char *, char *);
static int loadextractorso (DDSschema_t *, char *, int, DDSextractor_t *);

DDSextractor_t *DDScreateextractor (
    DDSschema_t *schemap, char *ename, char *elist, char *ccflags, char *ldflags
) {
    DDSextractor_t *extractorp;
    char *cstr, *sostr;

    if (!(extractorp = vmalloc (Vmheap, sizeof (DDSextractor_t)))) {
        SUwarning (1, "DDScreateextractor", "vmalloc failed");
        return NULL;
    }
    if (!(cstr = createextractorstr (schemap, ename, elist))) {
        SUwarning (1, "DDScreateextractor", "createextractorstr failed");
        return NULL;
    }
    if (!(sostr = _ddscreateso (cstr, NULL, NULL, ccflags, ldflags))) {
        SUwarning (1, "DDScreateextractor", "createso failed");
        return NULL;
    }
    if (loadextractorso (schemap, sostr, TRUE, extractorp) == -1) {
        SUwarning (1, "DDScreateextractor", "loadextractorso failed");
        return NULL;
    }
    return extractorp;
}

char *DDScompileextractor (
    DDSschema_t *schemap, char *ename, char *elist, char *sostr,
    char *ccflags, char *ldflags
) {
    char *cstr;

    if (!(cstr = createextractorstr (schemap, ename, elist))) {
        SUwarning (1, "DDScompileextractor", "createextractorstr failed");
        return NULL;
    }
    if (!(sostr = _ddscreateso (cstr, NULL, sostr, ccflags, ldflags))) {
        SUwarning (1, "DDScompileextractor", "createso failed");
        return NULL;
    }
    return sostr;
}

DDSextractor_t *DDSloadextractor (DDSschema_t *schemap, char *sostr) {
    DDSextractor_t *extractorp;
    char file[PATH_MAX];

    if (!(extractorp = vmalloc (Vmheap, sizeof (DDSextractor_t)))) {
        SUwarning (1, "DDSloadextractor", "vmalloc failed");
        return NULL;
    }
    if (access (sostr, R_OK) == 0) {
        if (strchr (sostr, '/') == NULL)
            file[0] = '.', file[1] = '/', strcpy (file + 2, sostr);
        else
            strcpy (file, sostr);
    } else if (
        !pathaccess (getenv ("PATH"), "../lib/dds", sostr, R_OK, file, PATH_MAX)
    ) {
        SUwarning (1, "DDSloadextractor", "access failed for %s", sostr);
        return NULL;
    }
    if (loadextractorso (schemap, file, FALSE, extractorp) == -1) {
        SUwarning (1, "DDSloadextractor", "loadextractorso failed");
        return NULL;
    }
    return extractorp;
}

int DDSdestroyextractor (DDSextractor_t *extractorp) {
    DDSdestroyschema (extractorp->schemap);
    dlclose (extractorp->handle);
    vmfree (Vmheap, extractorp);
    return 0;
}

static char *createextractorstr (
    DDSschema_t *schemap, char *ename, char *elist
) {
    char *elist2;
    DDSfield_t *fields;
    int fieldn, fieldi;
    DDSschema_t *eschemap;
    Sfio_t *fp;
    DDSfield_t *fieldp;
    int i;
    char *cstr;

    if (!ename)
        ename = "extracted";
    if (!(elist2 = vmstrdup (Vmheap, elist))) {
        SUwarning (1, "createextractorstr", "vmstrdup failed");
        return NULL;
    }
    if ((fieldn = _ddslist2fields (elist, schemap, &fields, NULL, NULL)) < 0) {
        SUwarning (1, "createextractorstr", "list2fields failed");
        return NULL;
    }
    if (!(eschemap = DDScreateschema (ename, fields, fieldn, NULL, NULL))) {
        SUwarning (1, "createextractorstr", "DDScreateschema failed");
        return NULL;
    }

    if (!(fp = sfopen (NULL, NULL, "sw+"))) {
        SUwarning (1, "createextractorstr", "sfopen failed");
        return NULL;
    }

    sfprintf (fp, "#include <ast.h>\n#include <swift.h>\n#include <cdt.h>\n");
    sfprintf (fp, "#include <vmalloc.h>\n#include <dds.h>\n\n");
    if (schemap->include)
        sfprintf (fp, "%s\n\n", schemap->include);
    _ddswritestruct (schemap, sfprints ("irec_%s", schemap->name), fp);
    _ddswritestruct (eschemap, sfprints ("orec_%s", eschemap->name), fp);
    sfprintf (fp,
        "DDS_EXPORT char extractor_%s_ename[] = \"%s\";\n"
        "DDS_EXPORT char extractor_%s_elist[] = \"%s\";\n\n",
    schemap->name, ename, schemap->name, elist2);
    sfprintf (fp,
        "DDS_EXPORT int extractor_%s_init (void) {\n"
        "    return 0;\n"
        "}\n\n",
    schemap->name);
    sfprintf (fp,
        "DDS_EXPORT int extractor_%s_term (void) {\n"
        "    return 0;\n"
        "}\n\n",
    schemap->name);
    sfprintf (fp,
        "DDS_EXPORT int extractor_%s_work (char *__ibuf, int __ilen,"
        "        char *__obuf, int __olen) {\n"
        "    struct __irec_%s_t *__irecp;\n\n"
        "    struct __orec_%s_t *__orecp;\n\n"
        "    __irecp = (struct __irec_%s_t *) __ibuf;\n"
        "    __orecp = (struct __orec_%s_t *) __obuf;\n",
        schemap->name, schemap->name, eschemap->name,
        schemap->name, eschemap->name
    );
    for (fieldi = 0; fieldi < fieldn; fieldi++) {
        fieldp = &fields[fieldi];
        if (fieldp->n > 1) {
            for (i = 0; i < fieldp->n; i++)
                sfprintf (
                    fp, "    __orecp->%s[%d] = __irecp->%s[%d];\n",
                    fieldp->name, i, fieldp->name, i
                );
        } else
            sfprintf (
                fp, "    __orecp->%s = __irecp->%s;\n",
                fieldp->name, fieldp->name
            );
    }
    sfprintf (fp, "    return 0;\n}\n");

    if (!(cstr = _ddscopystr (fp))) {
        SUwarning (1, "createextractorstr", "copystr failed");
        return NULL;
    }
    sfclose (fp);
    DDSdestroyschema (eschemap);
    return cstr;
}

static int loadextractorso (
    DDSschema_t *schemap, char *sofile, int rmflag, DDSextractor_t *extractorp
) {
    char *ename, *elist;
    DDSfield_t *fields;
    int fieldn;

    if (
        !(extractorp->handle = dlopen (sofile, RTLD_LAZY)) ||
        !(extractorp->init = (DDSextractorinitfunc) dlsym (
            extractorp->handle, sfprints ("extractor_%s_init", schemap->name)
        )) ||
        !(extractorp->term = (DDSextractortermfunc) dlsym (
            extractorp->handle, sfprints ("extractor_%s_term", schemap->name)
        )) ||
        !(extractorp->work = (DDSextractorworkfunc) dlsym (
            extractorp->handle, sfprints ("extractor_%s_work", schemap->name)
        )) ||
        !(ename = (char *) dlsym (
            extractorp->handle, sfprints ("extractor_%s_ename", schemap->name)
        )) ||
        !(elist = (char *) dlsym (
            extractorp->handle, sfprints ("extractor_%s_elist", schemap->name)
        ))
    ) {
        SUwarning (1, "loadextractorso", "init failed: %s", dlerror ());
        return -1;
    }
    if (!(elist = vmstrdup (Vmheap, elist))) {
        SUwarning (1, "loadextractorso", "vmstrdup failed");
        return -1;
    }
    if ((fieldn = _ddslist2fields (elist, schemap, &fields, NULL, NULL)) < 0) {
        SUwarning (1, "loadextractorso", "list2fields failed");
        return -1;
    }
    vmfree (Vmheap, elist);
    if (!(extractorp->schemap = DDScreateschema (
        ename, fields, fieldn, NULL, NULL
    ))) {
        SUwarning (1, "loadextractorso", "DDScreateschema failed");
        return -1;
    }
    if (rmflag)
        unlink (sofile);
    return 0;
}
