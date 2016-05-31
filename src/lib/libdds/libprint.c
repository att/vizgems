#pragma prototyped

#include <ast.h>
#include <vmalloc.h>
#include <dlfcn.h>
#include <swift.h>
#include <dds.h>
#include <dds_int.h>

static char *createprinterstr (DDSschema_t *, DDSexpr_t *);
static int loadprinterso (DDSschema_t *, char *, int, DDSprinter_t *);
static int createdefaultprinterstr (DDSschema_t *, Sfio_t *);

DDSprinter_t *DDScreateprinter (
    DDSschema_t *schemap, DDSexpr_t *pep, char *ccflags, char *ldflags
) {
    DDSprinter_t *printerp;
    char *cstr, *sostr;

    if (!(printerp = vmalloc (Vmheap, sizeof (DDSprinter_t)))) {
        SUwarning (1, "DDScreateprinter", "vmalloc failed for printerp");
        return NULL;
    }
    if (!(cstr = createprinterstr (schemap, pep))) {
        SUwarning (1, "DDScreateprinter", "createprinterstr failed");
        return NULL;
    }
    if (!(sostr = _ddscreateso (cstr, pep->libs, NULL, ccflags, ldflags))) {
        SUwarning (1, "DDScreateprinter", "createso failed");
        return NULL;
    }
    if (loadprinterso (schemap, sostr, TRUE, printerp) == -1) {
        SUwarning (1, "DDScreateprinter", "loadprinterso failed");
        return NULL;
    }
    return printerp;
}

char *DDScompileprinter (
    DDSschema_t *schemap, DDSexpr_t *pep, char *sostr,
    char *ccflags, char *ldflags
) {
    char *cstr;

    if (!(cstr = createprinterstr (schemap, pep))) {
        SUwarning (1, "DDScompileprinter", "createprinterstr failed");
        return NULL;
    }
    if (!(sostr = _ddscreateso (cstr, pep->libs, sostr, ccflags, ldflags))) {
        SUwarning (1, "DDScompileprinter", "createso failed");
        return NULL;
    }
    return sostr;
}

DDSprinter_t *DDSloadprinter (DDSschema_t *schemap, char *sostr) {
    DDSprinter_t *printerp;
    char file[PATH_MAX];

    if (!(printerp = vmalloc (Vmheap, sizeof (DDSprinter_t)))) {
        SUwarning (1, "DDSloadprinter", "vmalloc failed for printerp");
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
        SUwarning (1, "DDSloadprinter", "access failed for %s", sostr);
        return NULL;
    }
    if (loadprinterso (schemap, file, FALSE, printerp) == -1) {
        SUwarning (1, "DDSloadprinter", "loadprinterso failed");
        return NULL;
    }
    return printerp;
}

int DDSdestroyprinter (DDSprinter_t *printerp) {
    dlclose (printerp->handle);
    vmfree (Vmheap, printerp);
    return 0;
}

static char *createprinterstr (DDSschema_t *schemap, DDSexpr_t *pep) {
    Sfio_t *fp;
    DDSfield_t *fieldp;
    int fieldi;
    char *cstr;

    if (!(fp = sfopen (NULL, NULL, "sw+"))) {
        SUwarning (1, "createprinterstr", "sfopen failed");
        return NULL;
    }

    sfprintf (fp, "#include <ast.h>\n#include <swift.h>\n#include <dds.h>\n\n");
    if (schemap->include)
        sfprintf (fp, "%s\n\n", schemap->include);
    _ddswritestruct (schemap, sfprints ("rec_%s", schemap->name), fp);
    if (pep->vars)
        sfprintf (fp, "#line 1 \"VARS\"\n%s\n", pep->vars);

    sfprintf (fp,
        "DDS_EXPORT int printer_%s_init (void) {\n"
        "    {\n#line 1 \"BEGIN\"\n        %s\n    }\n"
        "    return 0;\n"
        "}\n\n",
    schemap->name, (pep->begin ? pep->begin : ""));
    sfprintf (fp,
        "DDS_EXPORT int printer_%s_term (void) {\n"
        "    {\n#line 1 \"END\"\n        %s\n    }\n"
        "    return 0;\n"
        "}\n\n",
    schemap->name, (pep->end ? pep->end : ""));
    for (fieldi = 0; fieldi < schemap->fieldn; fieldi++) {
        fieldp = &schemap->fields[fieldi];
        sfprintf (fp, "#define %s __recp->%s\n", fieldp->name, fieldp->name);
    }
    sfprintf (fp, "\n");
    sfprintf (fp,
        "DDS_EXPORT int printer_%s_work (\n"
        "    char *__buf, int __len, Sfio_t *fp\n"
        ") {\n"
        "    struct __rec_%s_t *__recp;\n\n"
        "    __recp = (struct __rec_%s_t *) __buf;\n",
    schemap->name, schemap->name, schemap->name);
    if (pep->loop)
        sfprintf (
            fp, "    {\n#line 1 \"LOOP\"\n        %s\n    }\n\n", pep->loop
        );
    else
        createdefaultprinterstr (schemap, fp);
    sfprintf (fp, "    return 0;\n");
    sfprintf (fp, "}\n");

    if (!(cstr = _ddscopystr (fp))) {
        SUwarning (1, "createprinterstr", "copystr failed");
        return NULL;
    }
    sfclose (fp);
    return cstr;
}

static int loadprinterso (
    DDSschema_t *schemap, char *sofile, int rmflag, DDSprinter_t *printerp
) {
    if (
        !(printerp->handle = dlopen (sofile, RTLD_LAZY)) ||
        !(printerp->init = (DDSprinterinitfunc) dlsym (
            printerp->handle, sfprints ("printer_%s_init", schemap->name)
        )) ||
        !(printerp->term = (DDSprintertermfunc) dlsym (
            printerp->handle, sfprints ("printer_%s_term", schemap->name)
        )) ||
        !(printerp->work = (DDSprinterworkfunc) dlsym (
            printerp->handle, sfprints ("printer_%s_work", schemap->name)
        ))
    ) {
        SUwarning (1, "loadprinterso", "init failed: %s", dlerror ());
        return -1;
    }
    if (rmflag)
        unlink (sofile);
    return 0;
}

static int createdefaultprinterstr (DDSschema_t *schemap, Sfio_t *fp) {
    DDSfield_t *fieldp;
    int fieldi;
    int i;

    sfprintf (fp, "    { sfprintf (fp, \"");
    for (fieldi = 0; fieldi < schemap->fieldn; fieldi++) {
        fieldp = &schemap->fields[fieldi];
        if (fieldi > 0)
            sfprintf (fp, "|");
        switch (fieldp->type) {
        case DDS_FIELD_TYPE_CHAR:
            for (i = 0; i < fieldp->n; i++) {
                if (i > 0)
                    sfprintf (fp, ":");
                sfprintf (fp, "%%d");
            }
            break;
        case DDS_FIELD_TYPE_SHORT:
            for (i = 0; i < fieldp->n; i++) {
                if (i > 0)
                    sfprintf (fp, ":");
                sfprintf (fp, "%%6d");
            }
            break;
        case DDS_FIELD_TYPE_INT:
            for (i = 0; i < fieldp->n; i++) {
                if (i > 0)
                    sfprintf (fp, ":");
                sfprintf (fp, "%%11d");
            }
            break;
        case DDS_FIELD_TYPE_LONG:
            for (i = 0; i < fieldp->n; i++) {
                if (i > 0)
                    sfprintf (fp, ":");
                sfprintf (fp, "%%11d");
            }
            break;
        case DDS_FIELD_TYPE_LONGLONG:
            for (i = 0; i < fieldp->n; i++) {
                if (i > 0)
                    sfprintf (fp, ":");
                sfprintf (fp, "%%22d");
            }
            break;
        case DDS_FIELD_TYPE_FLOAT:
            for (i = 0; i < fieldp->n; i++) {
                if (i > 0)
                    sfprintf (fp, ":");
                sfprintf (fp, "%%20f");
            }
            break;
        case DDS_FIELD_TYPE_DOUBLE:
            for (i = 0; i < fieldp->n; i++) {
                if (i > 0)
                    sfprintf (fp, ":");
                sfprintf (fp, "%%20f");
            }
            break;
        }
    }
    sfprintf (fp, "\\n\"");
    for (fieldi = 0; fieldi < schemap->fieldn; fieldi++) {
        fieldp = &schemap->fields[fieldi];
        if (fieldp->n > 1)
            for (i = 0; i < fieldp->n; i++)
                sfprintf (fp, ", %s[%d]", fieldp->name, i);
        else
            sfprintf (fp, ", %s", fieldp->name);
    }
    sfprintf (fp, ");\n    }\n");

    return 0;
}
