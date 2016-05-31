#pragma prototyped

#include <ast.h>
#include <vmalloc.h>
#include <dlfcn.h>
#include <swift.h>
#include <dds.h>
#include <dds_int.h>

static char *creategrouperstr (DDSschema_t *, DDSexpr_t *);
static int loadgrouperso (DDSschema_t *, char *, int, DDSgrouper_t *);

DDSgrouper_t *DDScreategrouper (
    DDSschema_t *schemap, DDSexpr_t *gep, char *ccflags, char *ldflags
) {
    DDSgrouper_t *grouperp;
    char *cstr, *sostr;

    if (!(grouperp = vmalloc (Vmheap, sizeof (DDSgrouper_t)))) {
        SUwarning (1, "DDScreategrouper", "vmalloc failed");
        return NULL;
    }
    if (!(cstr = creategrouperstr (schemap, gep))) {
        SUwarning (1, "DDScreategrouper", "creategrouperstr failed");
        return NULL;
    }
    if (!(sostr = _ddscreateso (cstr, gep->libs, NULL, ccflags, ldflags))) {
        SUwarning (1, "DDScreategrouper", "createso failed");
        return NULL;
    }
    if (loadgrouperso (schemap, sostr, TRUE, grouperp) == -1) {
        SUwarning (1, "DDScreategrouper", "loadgrouperso failed");
        return NULL;
    }
    return grouperp;
}

char *DDScompilegrouper (
    DDSschema_t *schemap, DDSexpr_t *gep, char *sostr,
    char *ccflags, char *ldflags
) {
    char *cstr;

    if (!(cstr = creategrouperstr (schemap, gep))) {
        SUwarning (1, "DDScompilegrouper", "creategrouperstr failed");
        return NULL;
    }
    if (!(sostr = _ddscreateso (cstr, gep->libs, sostr, ccflags, ldflags))) {
        SUwarning (1, "DDScompilegrouper", "createso failed");
        return NULL;
    }
    return sostr;
}

DDSgrouper_t *DDSloadgrouper (DDSschema_t *schemap, char *sostr) {
    DDSgrouper_t *grouperp;
    char file[PATH_MAX];

    if (!(grouperp = vmalloc (Vmheap, sizeof (DDSgrouper_t)))) {
        SUwarning (1, "DDSloadgrouper", "vmalloc failed");
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
        SUwarning (1, "DDSloadgrouper", "access failed for %s", sostr);
        return NULL;
    }
    if (loadgrouperso (schemap, file, FALSE, grouperp) == -1) {
        SUwarning (1, "DDSloadgrouper", "loadgrouperso failed");
        return NULL;
    }
    return grouperp;
}

int DDSdestroygrouper (DDSgrouper_t *grouperp) {
    dlclose (grouperp->handle);
    vmfree (Vmheap, grouperp);
    return 0;
}

static char *creategrouperstr (DDSschema_t *schemap, DDSexpr_t *gep) {
    Sfio_t *fp;
    DDSfield_t *fieldp;
    int fieldi;
    char *cstr;

    if (!(fp = sfopen (NULL, NULL, "sw+"))) {
        SUwarning (1, "creategrouperstr", "sfopen failed");
        return NULL;
    }

    sfprintf (fp, "#include <ast.h>\n#include <cdt.h>\n#include <swift.h>\n");
    sfprintf (fp, "#include <vmalloc.h>\n#include <dds.h>\n\n");
    if (schemap->include)
        sfprintf (fp, "%s\n\n", schemap->include);
    sfprintf (fp,
        "typedef struct __k2chan_t {\n"
        "    Dtlink_t link;\n"
        "    int __key;\n"
        "    char *__name;\n"
        "    void *__chanp;\n"
        "} __k2chan_t;\n"
        "static Dtdisc_t __k2chandisc = {\n"
        "    sizeof (Dtlink_t), sizeof (int), 0,\n"
        "    NULL, NULL, NULL, NULL, NULL, NULL\n"
        "};\n"
        "static Dt_t *__k2chandict;\n"
        "static Vmalloc_t *__k2chanvm;\n"
        "static char *__fmt;\n\n"
    );
    sfprintf (fp,
        "void *(*openfunc) (char *);\n"
        "void *(*reopenfunc) (char *);\n\n"
        "int (*closefunc) (void *);\n"
    );
    sfprintf (fp,
        "static void *safeopen (char *name) {\n"
        "    void *__chanp;\n"
        "    __k2chan_t *__k2chanp;\n\n"
        "    if ((__chanp = openfunc (name)))\n"
        "        return __chanp;\n"
        "    for (\n"
        "        __k2chanp = dtfirst (__k2chandict); __k2chanp;\n"
        "        __k2chanp = dtnext (__k2chandict, __k2chanp)\n"
        "    )\n"
        "        if (__k2chanp->__chanp) {\n"
        "            closefunc (__k2chanp->__chanp);\n"
        "            __k2chanp->__chanp = NULL;\n"
        "        }\n"
        "    return openfunc (name);\n"
        "}\n\n"
    );
    sfprintf (fp,
        "static void *safereopen (char *name) {\n"
        "    void *__chanp;\n"
        "    __k2chan_t *__k2chanp;\n\n"
        "    if ((__chanp = reopenfunc (name)))\n"
        "        return __chanp;\n"
        "    for (\n"
        "        __k2chanp = dtfirst (__k2chandict); __k2chanp;\n"
        "        __k2chanp = dtnext (__k2chandict, __k2chanp)\n"
        "    )\n"
        "        if (__k2chanp->__chanp) {\n"
        "            closefunc (__k2chanp->__chanp);\n"
        "            __k2chanp->__chanp = NULL;\n"
        "        }\n"
        "    return reopenfunc (name);\n"
        "}\n\n"
    );
    sfprintf (fp,
        "static int safeclose (__k2chan_t *__k2chanp) {\n"
        "    void *__chanp;\n\n"
        "    if (!(__chanp = __k2chanp->__chanp))\n"
        "        return 0;\n"
        "    __k2chanp->__chanp = NULL;\n"
        "    return closefunc (__chanp) ? 0 : -1;\n"
        "}\n\n"
    );
    _ddswritestruct (schemap, sfprints ("rec_%s", schemap->name), fp);
    if (gep->vars)
        sfprintf (fp, "#line 1 \"VARS\"\n%s\n", gep->vars);
    sfprintf (fp,
        "DDS_EXPORT int grouper_%s_init (\n"
        "    char *__prefix, void *(*chopen) (char *),\n"
        "    void *(*chreopen) (char *), int (*chclose) (void *)\n"
        ") {\n"
        "    if (!(__k2chanvm = vmopen (Vmdcsbrk, Vmbest, 0)) ||\n"
        "            !(__k2chandict = dtopen (&__k2chandisc, Dtset)))\n"
        "        return -1;\n"
        "    openfunc = chopen, reopenfunc = chreopen, closefunc = chclose;\n",
    schemap->name);
    sfprintf (fp,
        "    if (strchr (__prefix, '%%'))\n"
        "        __fmt = __prefix;\n"
        "    else if (!(__fmt = vmstrdup (__k2chanvm, sfprints (\n"
        "        \"%%s%%s\", __prefix, \".%%d\"\n"
        "    ))))\n"
        "        return -1;\n"
    );
    sfprintf (fp,
        "    {\n#line 1 \"BEGIN\"\n        %s\n    }\n"
        "    return 0;\n"
        "}\n\n",
    (gep->begin ? gep->begin : ""));
    sfprintf (fp,
        "DDS_EXPORT int grouper_%s_term (void) {\n"
        "    __k2chan_t *__k2chanp;\n\n"
        "    {\n#line 1 \"END\"\n        %s\n    }\n"
        "    for (\n"
        "        __k2chanp = dtfirst (__k2chandict); __k2chanp;\n"
        "        __k2chanp = dtnext (__k2chandict, __k2chanp)\n"
        "    )\n"
        "        if (__k2chanp->__chanp)\n"
        "            safeclose (__k2chanp);\n"
        "    dtclose (__k2chandict);\n"
        "    vmclose (__k2chanvm);\n"
        "    return 0;\n"
        "}\n\n",
    schemap->name, (gep->end ? gep->end : ""));
    sfprintf (fp, "#define GROUP __key\n\n");
    for (fieldi = 0; fieldi < schemap->fieldn; fieldi++) {
        fieldp = &schemap->fields[fieldi];
        sfprintf (fp, "#define %s __recp->%s\n", fieldp->name, fieldp->name);
    }
    sfprintf (fp, "\n");
    sfprintf (fp,
        "DDS_EXPORT Sfio_t *grouper_%s_work (char *__buf, int __len) {\n"
        "    __k2chan_t *__k2chanp;\n"
        "    int __key;\n"
        "    struct __rec_%s_t *__recp;\n\n"
        "    __recp = (struct __rec_%s_t *) __buf;\n",
    schemap->name, schemap->name, schemap->name);
    if (gep->loop)
        sfprintf (
            fp, "    {\n#line 1 \"LOOP\"\n        %s\n    }\n\n", gep->loop
        );
    else
        sfprintf (fp, "\n    __key = 0;\n\n");
    sfprintf (fp,
        "    if (__key == -1)\n"
        "        return sfstdin;\n"
        "    if ((__k2chanp = dtmatch (__k2chandict, &__key))) {\n"
        "        if (!__k2chanp->__chanp)\n"
        "            __k2chanp->__chanp = safereopen (__k2chanp->__name);\n"
        "        return __k2chanp->__chanp;\n"
        "    }\n"
        "    if (!(__k2chanp = vmalloc (__k2chanvm, sizeof (__k2chan_t))))\n"
        "        return NULL;\n"
        "    __k2chanp->__key = __key;\n"
        "    if (__k2chanp != (__k2chan_t *) "
        "dtinsert (__k2chandict, __k2chanp))\n"
        "        return NULL;\n"
        "    __k2chanp->__chanp = NULL;\n"
        "    if (!(__k2chanp->__name = vmstrdup (__k2chanvm, \n"
        "        sfprints (__fmt, __key)\n"
        "    )) || !(__k2chanp->__chanp = safeopen (__k2chanp->__name))\n"
        "    ) {\n"
        "        dtdelete (__k2chandict, __k2chanp);\n"
        "        return NULL;\n"
        "    }\n"
        "    return __k2chanp->__chanp;\n"
        "}\n"
    );

    if (!(cstr = _ddscopystr (fp))) {
        SUwarning (1, "creategrouperstr", "copystr failed");
        return NULL;
    }
    sfclose (fp);
    return cstr;
}

static int loadgrouperso (
    DDSschema_t *schemap, char *sofile, int rmflag, DDSgrouper_t *grouperp
) {
    if (
        !(grouperp->handle = dlopen (sofile, RTLD_LAZY)) ||
        !(grouperp->init = (DDSgrouperinitfunc) dlsym (
            grouperp->handle, sfprints ("grouper_%s_init", schemap->name)
        )) ||
        !(grouperp->term = (DDSgroupertermfunc) dlsym (
            grouperp->handle, sfprints ("grouper_%s_term", schemap->name)
        )) ||
        !(grouperp->work = (DDSgrouperworkfunc) dlsym (
            grouperp->handle, sfprints ("grouper_%s_work", schemap->name)
        ))
    ) {
        SUwarning (1, "loadgrouperso", "init failed: %s", dlerror ());
        return -1;
    }
    if (rmflag)
        unlink (sofile);
    return 0;
}
