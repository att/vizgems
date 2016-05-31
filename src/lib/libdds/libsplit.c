#pragma prototyped

#include <ast.h>
#include <vmalloc.h>
#include <dlfcn.h>
#include <swift.h>
#include <dds.h>
#include <dds_int.h>

static char *createsplitterstr (DDSschema_t *, char *);
static int loadsplitterso (DDSschema_t *, char *, int, DDSsplitter_t *);

DDSsplitter_t *DDScreatesplitter (
    DDSschema_t *schemap, char *slist, char *ccflags, char *ldflags
) {
    DDSsplitter_t *splitterp;
    char *cstr, *sostr;

    if (!(splitterp = vmalloc (Vmheap, sizeof (DDSsplitter_t)))) {
        SUwarning (1, "DDScreatesplitter", "vmalloc failed");
        return NULL;
    }
    if (!(cstr = createsplitterstr (schemap, slist))) {
        SUwarning (1, "DDScreatesplitter", "createsplitterstr failed");
        return NULL;
    }
    if (!(sostr = _ddscreateso (cstr, NULL, NULL, ccflags, ldflags))) {
        SUwarning (1, "DDScreatesplitter", "createso failed");
        return NULL;
    }
    if (loadsplitterso (schemap, sostr, TRUE, splitterp) == -1) {
        SUwarning (1, "DDScreatesplitter", "loadsplitterso failed");
        return NULL;
    }
    return splitterp;
}

char *DDScompilesplitter (
    DDSschema_t *schemap, char *slist, char *sostr, char *ccflags, char *ldflags
) {
    char *cstr;

    if (!(cstr = createsplitterstr (schemap, slist))) {
        SUwarning (1, "DDScompilesplitter", "createsplitterstr failed");
        return NULL;
    }
    if (!(sostr = _ddscreateso (cstr, NULL, sostr, ccflags, ldflags))) {
        SUwarning (1, "DDScompilesplitter", "createso failed");
        return NULL;
    }
    return sostr;
}

DDSsplitter_t *DDSloadsplitter (DDSschema_t *schemap, char *sostr) {
    DDSsplitter_t *splitterp;
    char file[PATH_MAX];

    if (!(splitterp = vmalloc (Vmheap, sizeof (DDSsplitter_t)))) {
        SUwarning (1, "DDSloadsplitter", "vmalloc failed");
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
        SUwarning (1, "DDSloadsplitter", "access failed for %s", sostr);
        return NULL;
    }
    if (loadsplitterso (schemap, file, FALSE, splitterp) == -1) {
        SUwarning (1, "DDSloadsplitter", "loadsplitterso failed");
        return NULL;
    }
    return splitterp;
}

int DDSdestroysplitter (DDSsplitter_t *splitterp) {
    dlclose (splitterp->handle);
    vmfree (Vmheap, splitterp);
    return 0;
}

static char *createsplitterstr (DDSschema_t *schemap, char *slist) {
    DDSfield_t **fieldps;
    int fieldpn, fieldpi;
    int len;
    Sfio_t *fp;
    DDSfield_t *fieldp;
    int fieldi;
    int i;
    char *cstr;

    if ((fieldpn = _ddslist2fields (
        slist, schemap, NULL, &fieldps, &len
    )) < 0) {
        SUwarning (1, "createsplitterstr", "list2fields failed");
        return NULL;
    }

    if (!(fp = sfopen (NULL, NULL, "sw+"))) {
        SUwarning (1, "createsplitterstr", "sfopen failed");
        return NULL;
    }

    sfprintf (fp, "#include <ast.h>\n#include <cdt.h>\n#include <swift.h>\n");
    sfprintf (fp, "#include <vmalloc.h>\n#include <dds.h>\n\n");
    if (schemap->include)
        sfprintf (fp, "%s\n\n", schemap->include);
    sfprintf (fp,
        "typedef struct __k2chan_t {\n"
        "    Dtlink_t link;\n"
        "    char __key[%d];\n"
        "    char *__name;\n"
        "    void *__chanp;\n"
        "} __k2chan_t;\n"
        "static Dtdisc_t __k2chandisc = {\n"
        "    sizeof (Dtlink_t), sizeof (char) * %d, 0,\n"
        "    NULL, NULL, NULL, NULL, NULL, NULL\n"
        "};\n"
        "static Dt_t *__k2chandict;\n"
        "static Vmalloc_t *__k2chanvm;\n"
        "static char *__fmt;\n\n",
    len, len);
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
    sfprintf (fp,
        "DDS_EXPORT int splitter_%s_init (\n"
        "    char *__prefix, void *(*chopen) (char *),\n"
        "    void *(*chreopen) (char *), int (*chclose) (void *)\n"
        ") {\n"
        "    if (\n"
        "        !(__k2chanvm = vmopen (Vmdcsbrk, Vmbest, 0)) ||\n"
        "        !(__k2chandict = dtopen (&__k2chandisc, Dtset))\n"
        "    )\n"
        "        return -1;\n"
        "    openfunc = chopen, reopenfunc = chreopen, closefunc = chclose;\n",
    schemap->name);
    sfprintf (fp,
        "    if (strchr (__prefix, '%%'))\n"
        "        __fmt = __prefix;\n"
        "    else if (!(__fmt = vmstrdup (__k2chanvm, sfprints (\n"
        "        \"%%s%%s\", __prefix, \""
    );
    for (fieldpi = 0; fieldpi < fieldpn; fieldpi++) {
        fieldp = fieldps[fieldpi];
        switch (fieldp->type) {
        case DDS_FIELD_TYPE_CHAR:
            sfprintf (fp, ".%%%dc", fieldp->n);
            break;
        case DDS_FIELD_TYPE_SHORT:
            for (i = 0; i < fieldp->n; i++)
                sfprintf (fp, ".%%d");
            break;
        case DDS_FIELD_TYPE_INT:
            for (i = 0; i < fieldp->n; i++)
                sfprintf (fp, ".%%d");
            break;
        case DDS_FIELD_TYPE_LONG:
            for (i = 0; i < fieldp->n; i++)
                sfprintf (fp, ".%%d");
            break;
        case DDS_FIELD_TYPE_LONGLONG:
            for (i = 0; i < fieldp->n; i++)
                sfprintf (fp, ".%%d");
            break;
        case DDS_FIELD_TYPE_FLOAT:
            for (i = 0; i < fieldp->n; i++)
                sfprintf (fp, ".%%f");
            break;
        case DDS_FIELD_TYPE_DOUBLE:
            for (i = 0; i < fieldp->n; i++)
                sfprintf (fp, ".%%f");
            break;
        }
    }
    sfprintf (fp, "\"\n    ))))\n        return -1;\n");
    sfprintf (fp, "    return 0;\n}\n\n");
    sfprintf (fp,
        "DDS_EXPORT int splitter_%s_term (void) {\n"
        "    __k2chan_t *__k2chanp;\n\n"
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
    schemap->name);
    for (fieldi = 0; fieldi < schemap->fieldn; fieldi++) {
        fieldp = &schemap->fields[fieldi];
        sfprintf (fp, "#define %s __recp->%s\n", fieldp->name, fieldp->name);
    }
    sfprintf (fp, "\n");
    sfprintf (fp,
        "DDS_EXPORT Sfio_t *splitter_%s_work (char *__buf, int __len) {\n"
        "    __k2chan_t *__k2chanp;\n"
        "    char __key[%d];\n"
        "    struct __rec_%s_t *__recp;\n\n"
        "    __recp = (struct __rec_%s_t *) __buf;\n",
    schemap->name, len, schemap->name, schemap->name);
    for (fieldpi = 0, len = 0; fieldpi < fieldpn; fieldpi++) {
        fieldp = fieldps[fieldpi];
        sfprintf (
            fp, "    memcpy (&__key[%d], &%s, %d);\n",
            len, fieldp->name, _ddsfieldtypesize[fieldp->type] * fieldp->n
        );
        len += (_ddsfieldtypesize[fieldp->type] * fieldp->n);
    }
    sfprintf (fp,
        "    if ((__k2chanp = dtmatch (__k2chandict, __key))) {\n"
        "        if (!__k2chanp->__chanp)\n"
        "            __k2chanp->__chanp = safereopen (__k2chanp->__name);\n"
        "        return __k2chanp->__chanp;\n"
        "    }\n"
        "    if (!(__k2chanp = vmalloc (__k2chanvm, sizeof (__k2chan_t))))\n"
        "        return NULL;\n"
        "    memcpy (__k2chanp->__key, __key, %d);\n"
        "    if (__k2chanp != (__k2chan_t *) "
        "dtinsert (__k2chandict, __k2chanp))\n"
        "        return NULL;\n"
        "    __k2chanp->__chanp = NULL;\n",
    len);
    sfprintf (fp,
        "    if (\n"
        "        !(__k2chanp->__name = vmstrdup (__k2chanvm, sfprints (\n"
        "            __fmt"
    );
    for (fieldpi = 0; fieldpi < fieldpn; fieldpi++) {
        fieldp = fieldps[fieldpi];
        switch (fieldp->type) {
        case DDS_FIELD_TYPE_CHAR:
            sfprintf (fp, ", %s", fieldp->name);
            break;
        case DDS_FIELD_TYPE_SHORT:
        case DDS_FIELD_TYPE_INT:
        case DDS_FIELD_TYPE_LONG:
        case DDS_FIELD_TYPE_LONGLONG:
        case DDS_FIELD_TYPE_FLOAT:
        case DDS_FIELD_TYPE_DOUBLE:
            if (fieldp->n == 1)
                sfprintf (fp, ", %s", fieldp->name);
            else
                for (i = 0; i < fieldp->n; i++)
                    sfprintf (fp, ", %s[%d]", fieldp->name, i);
            break;
        }
    }
    sfprintf (fp,
        "\n        ))) ||\n"
        "        !(__k2chanp->__chanp = safeopen (__k2chanp->__name))\n"
        "    ) {\n"
        "        dtdelete (__k2chandict, __k2chanp);\n"
        "        return NULL;\n"
        "    }\n"
        "    return __k2chanp->__chanp;\n"
        "}\n"
    );

    if (!(cstr = _ddscopystr (fp))) {
        SUwarning (1, "createsplitterstr", "copystr failed");
        return NULL;
    }
    sfclose (fp);
    return cstr;
}

static int loadsplitterso (
    DDSschema_t *schemap, char *sofile, int rmflag, DDSsplitter_t *splitterp
) {
    if (
        !(splitterp->handle = dlopen (sofile, RTLD_LAZY)) ||
        !(splitterp->init = (DDSsplitterinitfunc) dlsym (
            splitterp->handle, sfprints ("splitter_%s_init", schemap->name)
        )) ||
        !(splitterp->term = (DDSsplittertermfunc) dlsym (
            splitterp->handle, sfprints ("splitter_%s_term", schemap->name)
        )) ||
        !(splitterp->work = (DDSsplitterworkfunc) dlsym (
            splitterp->handle, sfprints ("splitter_%s_work", schemap->name)
        ))
    ) {
        SUwarning (1, "loadsplitterso", "init failed: %s", dlerror ());
        return -1;
    }
    if (rmflag)
        unlink (sofile);
    return 0;
}
