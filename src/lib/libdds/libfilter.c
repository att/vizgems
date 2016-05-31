#pragma prototyped

#include <ast.h>
#include <vmalloc.h>
#include <dlfcn.h>
#include <swift.h>
#include <dds.h>
#include <dds_int.h>

static char *createfilterstr (DDSschema_t *, DDSexpr_t *);
static int loadfilterso (DDSschema_t *, char *, int, DDSfilter_t *);

DDSfilter_t *DDScreatefilter (
    DDSschema_t *schemap, DDSexpr_t *fep, char *ccflags, char *ldflags
) {
    DDSfilter_t *filterp;
    char *cstr, *sostr;

    if (!(filterp = vmalloc (Vmheap, sizeof (DDSfilter_t)))) {
        SUwarning (1, "DDScreatefilter", "vmalloc failed for filterp");
        return NULL;
    }
    if (!(cstr = createfilterstr (schemap, fep))) {
        SUwarning (1, "DDScreatefilter", "createfilterstr failed");
        return NULL;
    }
    if (!(sostr = _ddscreateso (cstr, fep->libs, NULL, ccflags, ldflags))) {
        SUwarning (1, "DDScreatefilter", "createso failed");
        return NULL;
    }
    if (loadfilterso (schemap, sostr, TRUE, filterp) == -1) {
        SUwarning (1, "DDScreatefilter", "loadfilterso failed");
        return NULL;
    }
    return filterp;
}

char *DDScompilefilter (
    DDSschema_t *schemap, DDSexpr_t *fep, char *sostr,
    char *ccflags, char *ldflags
) {
    char *cstr;

    if (!(cstr = createfilterstr (schemap, fep))) {
        SUwarning (1, "DDScompilefilter", "createfilterstr failed");
        return NULL;
    }
    if (!(sostr = _ddscreateso (cstr, fep->libs, sostr, ccflags, ldflags))) {
        SUwarning (1, "DDScompilefilter", "createso failed");
        return NULL;
    }
    return sostr;
}

DDSfilter_t *DDSloadfilter (DDSschema_t *schemap, char *sostr) {
    DDSfilter_t *filterp;
    char file[PATH_MAX];

    if (!(filterp = vmalloc (Vmheap, sizeof (DDSfilter_t)))) {
        SUwarning (1, "DDSloadfilter", "vmalloc failed for filterp");
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
        SUwarning (1, "DDSloadfilter", "access failed for %s", sostr);
        return NULL;
    }
    if (loadfilterso (schemap, file, FALSE, filterp) == -1) {
        SUwarning (1, "DDSloadfilter", "loadfilterso failed");
        return NULL;
    }
    return filterp;
}

int DDSdestroyfilter (DDSfilter_t *filterp) {
    dlclose (filterp->handle);
    vmfree (Vmheap, filterp);
    return 0;
}

static char *createfilterstr (DDSschema_t *schemap, DDSexpr_t *fep) {
    Sfio_t *fp;
    DDSfield_t *fieldp;
    int fieldi;
    char *cstr;

    if (!(fp = sfopen (NULL, NULL, "sw+"))) {
        SUwarning (1, "createfilterstr", "sfopen failed");
        return NULL;
    }

    sfprintf (fp, "#include <ast.h>\n#include <swift.h>\n#include <dds.h>\n\n");
    if (schemap->include)
        sfprintf (fp, "%s\n\n", schemap->include);
    _ddswritestruct (schemap, sfprints ("rec_%s", schemap->name), fp);
    if (fep->vars)
        sfprintf (fp, "#line 1 \"VARS\"\n%s\n", fep->vars);

    sfprintf (fp,
        "DDS_EXPORT int filter_%s_init (void) {\n"
        "    {\n#line 1 \"BEGIN\"\n        %s\n    }\n"
        "    return 0;\n"
        "}\n\n",
    schemap->name, (fep->begin ? fep->begin : ""));
    sfprintf (fp,
        "DDS_EXPORT int filter_%s_term (void) {\n"
        "    {\n#line 1 \"END\"\n        %s\n    }\n"
        "    return 0;\n"
        "}\n\n",
    schemap->name, (fep->end ? fep->end : ""));
    sfprintf (fp, "#define KEEP __result = 1\n#define DROP __result = 0\n\n");
    sfprintf (fp, "#define NREC __nrec\n#define INDEX (*__indexp)\n\n");
    for (fieldi = 0; fieldi < schemap->fieldn; fieldi++) {
        fieldp = &schemap->fields[fieldi];
        sfprintf (fp, "#define %s __recp->%s\n", fieldp->name, fieldp->name);
    }
    sfprintf (fp, "\n");
    sfprintf (fp,
        "DDS_EXPORT int filter_%s_work (\n"
        "    char *__buf, int __len, long long *__indexp, long long __nrec\n"
        ") {\n"
        "    struct __rec_%s_t *__recp;\n"
        "    int __result;\n\n"
        "    __recp = (struct __rec_%s_t *) __buf;\n",
    schemap->name, schemap->name, schemap->name);
    if (fep->loop)
        sfprintf (
            fp, "    {\n#line 1 \"LOOP\"\n        %s\n    }\n\n", fep->loop
        );
    else
        sfprintf (fp, "\n    KEEP;\n\n");
    sfprintf (fp, "    return __result;\n");
    sfprintf (fp, "}\n");

    if (!(cstr = _ddscopystr (fp))) {
        SUwarning (1, "createfilterstr", "copystr failed");
        return NULL;
    }
    sfclose (fp);
    return cstr;
}

static int loadfilterso (
    DDSschema_t *schemap, char *sofile, int rmflag, DDSfilter_t *filterp
) {
    if (
        !(filterp->handle = dlopen (sofile, RTLD_LAZY)) ||
        !(filterp->init = (DDSfilterinitfunc) dlsym (
            filterp->handle, sfprints ("filter_%s_init", schemap->name)
        )) ||
        !(filterp->term = (DDSfiltertermfunc) dlsym (
            filterp->handle, sfprints ("filter_%s_term", schemap->name)
        )) ||
        !(filterp->work = (DDSfilterworkfunc) dlsym (
            filterp->handle, sfprints ("filter_%s_work", schemap->name)
        ))
    ) {
        SUwarning (1, "loadfilterso", "init failed: %s", dlerror ());
        return -1;
    }
    if (rmflag)
        unlink (sofile);
    return 0;
}
