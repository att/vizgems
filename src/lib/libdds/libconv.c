#pragma prototyped

#include <ast.h>
#include <vmalloc.h>
#include <dlfcn.h>
#include <swift.h>
#include <dds.h>
#include <dds_int.h>

static char *createconverterstr (DDSschema_t *, DDSschema_t *, DDSexpr_t *);
static int loadconverterso (
    DDSschema_t *, DDSschema_t *, char *, int, DDSconverter_t *
);

DDSconverter_t *DDScreateconverter (
    DDSschema_t *ischemap, DDSschema_t *oschemap, DDSexpr_t *cep,
    char *ccflags, char *ldflags
) {
    DDSconverter_t *converterp;
    char *cstr, *sostr;

    if (!(converterp = vmalloc (Vmheap, sizeof (DDSconverter_t)))) {
        SUwarning (1, "DDScreateconverter", "vmalloc failed");
        return NULL;
    }
    if (!(cstr = createconverterstr (ischemap, oschemap, cep))) {
        SUwarning (1, "DDScreateconverter", "createconverterstr failed");
        return NULL;
    }
    if (!(sostr = _ddscreateso (cstr, cep->libs, NULL, ccflags, ldflags))) {
        SUwarning (1, "DDScreateconverter", "createso failed");
        return NULL;
    }
    if (loadconverterso (ischemap, oschemap, sostr, TRUE, converterp) == -1) {
        SUwarning (1, "DDScreateconverter", "loadconverterso failed");
        return NULL;
    }
    return converterp;
}

char *DDScompileconverter (
    DDSschema_t *ischemap, DDSschema_t *oschemap, DDSexpr_t *cep,
    char *sostr, char *ccflags, char *ldflags
) {
    char *cstr;

    if (!(cstr = createconverterstr (ischemap, oschemap, cep))) {
        SUwarning (1, "DDScompileconverter", "createconverterstr failed");
        return NULL;
    }
    if (!(sostr = _ddscreateso (cstr, cep->libs, sostr, ccflags, ldflags))) {
        SUwarning (1, "DDScompileconverter", "createso failed");
        return NULL;
    }
    return sostr;
}

DDSconverter_t *DDSloadconverter (
    DDSschema_t *ischemap, DDSschema_t *oschemap, char *sostr
) {
    DDSconverter_t *converterp;
    char file[PATH_MAX];

    if (!(converterp = vmalloc (Vmheap, sizeof (DDSconverter_t)))) {
        SUwarning (1, "DDSloadconverter", "vmalloc failed for converterp");
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
        SUwarning (1, "DDSloadconverter", "access failed for %s", sostr);
        return NULL;
    }
    if (loadconverterso (ischemap, oschemap, file, FALSE, converterp) == -1) {
        SUwarning (1, "DDSloadconverter", "loadconverterso failed");
        return NULL;
    }
    return converterp;
}

int DDSdestroyconverter (DDSconverter_t *converterp) {
    dlclose (converterp->handle);
    vmfree (Vmheap, converterp);
    return 0;
}

static char *createconverterstr (
    DDSschema_t *ischemap, DDSschema_t *oschemap, DDSexpr_t *cep
) {
    Sfio_t *fp;
    char *cstr;

    if (!cep->loop) {
        SUwarning (1, "createconverterstr", "no loop code specified");
        return NULL;
    }

    if (!(fp = sfopen (NULL, NULL, "sw+"))) {
        SUwarning (1, "createconverterstr", "sfopen failed");
        return NULL;
    }

    sfprintf (fp, "#include <ast.h>\n#include <cdt.h>\n#include <swift.h>\n");
    sfprintf (fp, "#include <vmalloc.h>\n#include <dds.h>\n\n");
    if (ischemap->include)
        sfprintf (fp, "%s\n\n", ischemap->include);
    _ddswritestruct (ischemap, sfprints ("inrec_%s", ischemap->name), fp);
    _ddswritestruct (oschemap, sfprints ("outrec_%s", oschemap->name), fp);
    if (cep->vars)
        sfprintf (fp, "#line 1 \"VARS\"\n%s\n", cep->vars);
    sfprintf (fp,
        "DDS_EXPORT int converter_%s_init (void) {\n"
        "    {\n#line 1 \"BEGIN\"\n        %s\n    }\n"
        "    return 0;\n"
        "}\n\n",
    ischemap->name, (cep->begin ? cep->begin : ""));
    sfprintf (fp,
        "DDS_EXPORT int converter_%s_term (void) {\n"
        "    {\n#line 1 \"END\"\n        %s\n    }\n"
        "    return 0;\n"
        "}\n\n",
    ischemap->name, (cep->end ? cep->end : ""));
    sfprintf (fp, "#define KEEP __result = 1\n#define DROP __result = 0\n\n");
    sfprintf (fp, "#define NREC __nrec\n#define INDEX (*__indexp)\n\n");
    sfprintf (fp, "#define IN __inrecp\n");
    sfprintf (fp, "#define OUT __outrecp\n\n");
    sfprintf (
        fp, "#define EMIT (*__emitfunc) ("
        "  __emitfp, __outrecp, __outschemap, __outlen"
        "), __result = 0\n\n"
    );
    sfprintf (fp,
        "DDS_EXPORT int converter_%s_work ("
        "  char *__inbuf, int __inlen, DDSschema_t *__inschemap,"
        "  char *__outbuf, int __outlen, DDSschema_t *__outschemap,"
        "  long long *__indexp, long long __nrec, "
        "  int (*__emitfunc) (Sfio_t *, void *, DDSschema_t *, size_t), "
        "  Sfio_t *__emitfp"
        ") {\n"
        "    struct __inrec_%s_t *__inrecp;\n"
        "    struct __outrec_%s_t *__outrecp;\n"
        "    int __result = 1;\n\n"
        "    __inrecp = (struct __inrec_%s_t *) __inbuf;\n"
        "    __outrecp = (struct __outrec_%s_t *) __outbuf;\n",
        ischemap->name, ischemap->name, oschemap->name,
        ischemap->name, oschemap->name
    );
    sfprintf (fp, "    {\n#line 1 \"LOOP\"\n        %s\n    }\n\n", cep->loop);
    sfprintf (fp, "    return __result;\n");
    sfprintf (fp, "}\n");

    if (!(cstr = _ddscopystr (fp))) {
        SUwarning (1, "createconverterstr", "copystr failed");
        return NULL;
    }
    sfclose (fp);
    return cstr;
}

static int loadconverterso (
    DDSschema_t *ischemap, DDSschema_t *oschemap,
    char *sofile, int rmflag, DDSconverter_t *converterp
) {
    if (
        !(converterp->handle = dlopen (sofile, RTLD_LAZY)) ||
        !(converterp->init = (DDSconverterinitfunc) dlsym (
            converterp->handle, sfprints ("converter_%s_init", ischemap->name)
        )) ||
        !(converterp->term = (DDSconvertertermfunc) dlsym (
            converterp->handle, sfprints ("converter_%s_term", ischemap->name)
        )) ||
        !(converterp->work = (DDSconverterworkfunc) dlsym (
            converterp->handle, sfprints ("converter_%s_work", ischemap->name)
        ))
    ) {
        SUwarning (1, "loadconverterso", "init failed: %s", dlerror ());
        return -1;
    }
    if (rmflag)
        unlink (sofile);
    return 0;
}
