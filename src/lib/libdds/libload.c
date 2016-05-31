#pragma prototyped

#include <ast.h>
#include <vmalloc.h>
#include <dlfcn.h>
#include <swift.h>
#include <dds.h>
#include <dds_int.h>

static char *createloaderstr (DDSschema_t *);
static int loadloaderso (DDSschema_t *, char *, int, DDSloader_t *);

DDSloader_t *DDScreateloader (
    DDSschema_t *schemap, char *ccflags, char *ldflags
) {
    DDSloader_t *loaderp;
    char *cstr, *sostr;

    if (!(loaderp = vmalloc (Vmheap, sizeof (DDSloader_t)))) {
        SUwarning (1, "DDScreateloader", "vmalloc failed");
        return NULL;
    }
    if (!(cstr = createloaderstr (schemap))) {
        SUwarning (1, "DDScreateloader", "createloaderstr failed");
        return NULL;
    }
    if (!(sostr = _ddscreateso (cstr, NULL, NULL, ccflags, ldflags))) {
        SUwarning (1, "DDScreateloader", "createso failed");
        return NULL;
    }
    if (loadloaderso (schemap, sostr, TRUE, loaderp) == -1) {
        SUwarning (1, "DDScreateloader", "loadloaderso failed");
        return NULL;
    }
    return loaderp;
}

char *DDScompileloader (
    DDSschema_t *schemap, char *sostr, char *ccflags, char *ldflags
) {
    char *cstr;

    if (!(cstr = createloaderstr (schemap))) {
        SUwarning (1, "DDScompileloader", "createloaderstr failed");
        return NULL;
    }
    if (!(sostr = _ddscreateso (cstr, NULL, sostr, ccflags, ldflags))) {
        SUwarning (1, "DDScompileloader", "createso failed");
        return NULL;
    }
    return sostr;
}

DDSloader_t *DDSloadloader (DDSschema_t *schemap, char *sostr) {
    DDSloader_t *loaderp;
    char file[PATH_MAX];

    if (!(loaderp = vmalloc (Vmheap, sizeof (DDSloader_t)))) {
        SUwarning (1, "DDSloadloader", "vmalloc failed");
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
        SUwarning (1, "DDSloadloader", "access failed for %s", sostr);
        return NULL;
    }
    if (loadloaderso (schemap, file, FALSE, loaderp) == -1) {
        SUwarning (1, "DDSloadloader", "loadloaderso failed");
        return NULL;
    }
    return loaderp;
}

int DDSdestroyloader (DDSloader_t *loaderp) {
    dlclose (loaderp->handle);
    vmfree (Vmheap, loaderp);
    return 0;
}

static char *createloaderstr (DDSschema_t *schemap) {
    int fieldi;
    Sfio_t *fp;
    DDSfield_t *fieldp;
    char *cstr;

    if (!(fp = sfopen (NULL, NULL, "sw+"))) {
        SUwarning (1, "createloaderstr", "sfopen failed");
        return NULL;
    }

    sfprintf (fp, "#include <ast.h>\n#include <cdt.h>\n#include <swift.h>\n");
    sfprintf (fp, "#include <vmalloc.h>\n#include <dds.h>\n\n");
    if (schemap->include)
        sfprintf (fp, "%s\n\n", schemap->include);
    _ddswritestruct (schemap, sfprints ("rec_%s", schemap->name), fp);
    sfprintf (fp,
        "DDS_EXPORT int loader_%s_init (void) {\n"
        "    return 0;\n"
        "}\n\n",
    schemap->name);
    sfprintf (fp,
        "DDS_EXPORT int loader_%s_term (void) {\n"
        "    return 0;\n"
        "}\n\n",
    schemap->name);
    sfprintf (fp,
        "DDS_EXPORT int loader_%s_work (char **__iws, int __iwn,"
        "        char *__obuf, int __olen, int __ntsflag) {\n"
        "    struct __rec_%s_t *__recp;\n"
        "    int __iwi, __ret = 0, __i = 0;\n\n"
        "    memset (__obuf, 0, __olen);\n"
        "    __recp = (struct __rec_%s_t *) __obuf;\n"
        "    __iwi = 0;\n",
        schemap->name, schemap->name, schemap->name, schemap->fieldn
    );
    for (fieldi = 0; fieldi < schemap->fieldn; fieldi++) {
        fieldp = &schemap->fields[fieldi];
        switch (fieldp->type) {
        case DDS_FIELD_TYPE_CHAR:
            if (fieldp->n > 1) {
                sfprintf (fp,
                    "    if (__iwi >= __iwn)\n"
                    "        return -2;\n"
                    "    if ((__i = strlen (__iws[__iwi])) > %d) {\n"
                    "        SUwarning (\n"
                    "            1, \"loader_%s_work\",\n"
                    "            \"field %d truncated\"\n"
                    "        );\n"
                    "        __i = %d;\n"
                    "        __ret = -2;\n"
                    "    }\n"
                    "    strncpy (__recp->%s, __iws[__iwi++], %d);\n"
                    "    if (__ntsflag && __i == %d)\n"
                    "        __recp->%s[__i - 1] = 0;\n"
                    "    for (; __i < %d; __i++)\n"
                    "        __recp->%s[__i] = 0;\n",
                    fieldp->n, schemap->name, fieldi, fieldp->n, fieldp->name,
                    fieldp->n, fieldp->n, fieldp->name, fieldp->n, fieldp->name
                );
            } else {
                sfprintf (fp,
                    "    if ("
                    "        __iws[__iwi][0] == '-' || ("
                    "            __iws[__iwi][0] >= '0' &&"
                    "            __iws[__iwi][0] <= '9'"
                    "        )"
                    "    ) {\n"
                    "        if (__iwi >= __iwn)\n"
                    "            return -2;\n"
                    "        __recp->%s = atoi (__iws[__iwi++]);\n"
                    "    } else {\n"
                    "        if (__iwi >= __iwn)\n"
                    "            return -2;\n"
                    "        if (strlen (__iws[__iwi]) > 1) {\n"
                    "            SUwarning (\n"
                    "                1, \"loader_%s_work\",\n"
                    "                \"field %d truncated\"\n"
                    "            );\n"
                    "            __ret = -2;\n"
                    "        }\n"
                    "        __recp->%s = __iws[__iwi++][0];\n"
                    "    }\n",
                    fieldp->name, schemap->name, fieldi, fieldp->name
                );
            }
            break;
        case DDS_FIELD_TYPE_SHORT:
            if (fieldp->n > 1)
                sfprintf (fp,
                    "    if (__iwi + %d > __iwn)\n"
                    "        return -2;\n"
                    "    for (__i = 0; __i < %d; __i++)\n"
                    "        __recp->%s[__i] = atoi (__iws[__iwi++]);\n",
                    fieldp->n, fieldp->n, fieldp->name
                );
            else
                sfprintf (fp,
                    "    if (__iwi >= __iwn)\n"
                    "        return -2;\n"
                    "    __recp->%s = atoi (__iws[__iwi++]);\n",
                    fieldp->name, fieldp->name
                );
            break;
        case DDS_FIELD_TYPE_INT:
            if (fieldp->n > 1)
                sfprintf (fp,
                    "    if (__iwi + %d > __iwn)\n"
                    "        return -2;\n"
                    "    for (__i = 0; __i < %d; __i++)\n"
                    "        __recp->%s[__i] = atoi (__iws[__iwi++]);\n",
                    fieldp->n, fieldp->n, fieldp->name
                );
            else
                sfprintf (fp,
                    "    if (__iwi >= __iwn)\n"
                    "        return -2;\n"
                    "    __recp->%s = atoi (__iws[__iwi++]);\n",
                    fieldp->name, fieldp->name
                );
            break;
        case DDS_FIELD_TYPE_LONG:
            if (fieldp->n > 1)
                sfprintf (fp,
                    "    if (__iwi + %d > __iwn)\n"
                    "        return -2;\n"
                    "    for (__i = 0; __i < %d; __i++)\n"
                    "        __recp->%s[__i] = atol (__iws[__iwi++]);\n",
                    fieldp->n, fieldp->n, fieldp->name
                );
            else
                sfprintf (fp,
                    "    if (__iwi >= __iwn)\n"
                    "        return -2;\n"
                    "    __recp->%s = atol (__iws[__iwi++]);\n",
                    fieldp->name, fieldp->name
                );
            break;
        case DDS_FIELD_TYPE_LONGLONG:
            if (fieldp->n > 1)
                sfprintf (fp,
                    "    if (__iwi + %d > __iwn)\n"
                    "        return -2;\n"
                    "    for (__i = 0; __i < %d; __i++)\n"
                    "        __recp->%s[__i] = atoll (__iws[__iwi++]);\n",
                    fieldp->n, fieldp->n, fieldp->name
                );
            else
                sfprintf (fp,
                    "    if (__iwi >= __iwn)\n"
                    "        return -2;\n"
                    "    __recp->%s = atoll (__iws[__iwi++]);\n",
                    fieldp->name, fieldp->name
                );
            break;
        case DDS_FIELD_TYPE_FLOAT:
            if (fieldp->n > 1)
                sfprintf (fp,
                    "    if (__iwi + %d > __iwn)\n"
                    "        return -2;\n"
                    "    for (__i = 0; __i < %d; __i++)\n"
                    "        __recp->%s[__i] = atof (__iws[__iwi++]);\n",
                    fieldp->n, fieldp->n, fieldp->name
                );
            else
                sfprintf (fp,
                    "    if (__iwi >= __iwn)\n"
                    "        return -2;\n"
                    "    __recp->%s = atof (__iws[__iwi++]);\n",
                    fieldp->name, fieldp->name
                );
            break;
        case DDS_FIELD_TYPE_DOUBLE:
            if (fieldp->n > 1)
                sfprintf (fp,
                    "    if (__iwi + %d > __iwn)\n"
                    "        return -2;\n"
                    "    for (__i = 0; __i < %d; __i++)\n"
                    "        __recp->%s[__i] = atof (__iws[__iwi++]);\n",
                    fieldp->n, fieldp->n, fieldp->name
                );
            else
                sfprintf (fp,
                    "    if (__iwi >= __iwn)\n"
                    "        return -2;\n"
                    "    __recp->%s = atof (__iws[__iwi++]);\n",
                    fieldp->name, fieldp->name
                );
            break;
        }
    }
    sfprintf (fp, "    return __ret;\n}\n");

    if (!(cstr = _ddscopystr (fp))) {
        SUwarning (1, "createloaderstr", "copystr failed");
        return NULL;
    }
    sfclose (fp);
    return cstr;
}

static int loadloaderso (
    DDSschema_t *schemap, char *sofile, int rmflag, DDSloader_t *loaderp
) {
    if (
        !(loaderp->handle = dlopen (sofile, RTLD_LAZY)) ||
        !(loaderp->init = (DDSloaderinitfunc) dlsym (
            loaderp->handle, sfprints ("loader_%s_init", schemap->name)
        )) ||
        !(loaderp->term = (DDSloadertermfunc) dlsym (
            loaderp->handle, sfprints ("loader_%s_term", schemap->name)
        )) ||
        !(loaderp->work = (DDSloaderworkfunc) dlsym (
            loaderp->handle, sfprints ("loader_%s_work", schemap->name)
        ))
    ) {
        SUwarning (1, "loadloaderso", "init failed: %s", dlerror ());
        return -1;
    }
    if (rmflag)
        unlink (sofile);
    return 0;
}
