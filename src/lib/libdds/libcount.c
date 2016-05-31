#pragma prototyped

#include <ast.h>
#include <vmalloc.h>
#include <dlfcn.h>
#include <swift.h>
#include <dds.h>
#include <dds_int.h>

static char *createcounterstr (DDSschema_t *, char *, char *);
static int loadcounterso (DDSschema_t *, char *, int, DDScounter_t *);

DDScounter_t *DDScreatecounter (
    DDSschema_t *schemap, char *klist, char *cname, char *ccflags, char *ldflags
) {
    DDScounter_t *counterp;
    char *cstr, *sostr;

    if (!(counterp = vmalloc (Vmheap, sizeof (DDScounter_t)))) {
        SUwarning (1, "DDScreatecounter", "vmalloc failed");
        return NULL;
    }
    if (!(cstr = createcounterstr (schemap, klist, cname))) {
        SUwarning (1, "DDScreatecounter", "createcounterstr failed");
        return NULL;
    }
    if (!(sostr = _ddscreateso (cstr, NULL, NULL, ccflags, ldflags))) {
        SUwarning (1, "DDScreatecounter", "createso failed");
        return NULL;
    }
    if (loadcounterso (schemap, sostr, TRUE, counterp) == -1) {
        SUwarning (1, "DDScreatecounter", "loadcounterso failed");
        return NULL;
    }
    return counterp;
}

char *DDScompilecounter (
    DDSschema_t *schemap, char *klist, char *cname, char *sostr,
    char *ccflags, char *ldflags
) {
    char *cstr;

    if (!(cstr = createcounterstr (schemap, klist, cname))) {
        SUwarning (1, "DDScompilecounter", "createcounterstr failed");
        return NULL;
    }
    if (!(sostr = _ddscreateso (cstr, NULL, sostr, ccflags, ldflags))) {
        SUwarning (1, "DDScompilecounter", "createso failed");
        return NULL;
    }
    return sostr;
}

DDScounter_t *DDSloadcounter (DDSschema_t *schemap, char *sostr) {
    DDScounter_t *counterp;
    char file[PATH_MAX];

    if (!(counterp = vmalloc (Vmheap, sizeof (DDScounter_t)))) {
        SUwarning (1, "DDSloadcounter", "vmalloc failed for counterp");
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
        SUwarning (1, "DDSloadcounter", "access failed for %s", sostr);
        return NULL;
    }
    if (loadcounterso (schemap, file, FALSE, counterp) == -1) {
        SUwarning (1, "DDSloadcounter", "loadcounterso failed");
        return NULL;
    }
    return counterp;
}

int DDSdestroycounter (DDScounter_t *counterp) {
    DDSdestroyschema (counterp->schemap);
    dlclose (counterp->handle);
    vmfree (Vmheap, counterp);
    return 0;
}

static char *createcounterstr (
    DDSschema_t *schemap, char *klist, char *cname
) {
    DDSfield_t **fieldps;
    int fieldpn, fieldpi;
    char *cschemaname;
    DDSschema_t *cschemap;
    DDSfield_t *cfields, *cfieldp;
    int cfieldn, cfieldi, ctype;
    int n;
    int counter, maxcounter;
    Sfio_t *fp;
    DDSfield_t *fieldp;
    int fieldi;
    int len;
    char *cstr;

    if ((fieldpn = _ddslist2fields (
        klist, schemap, NULL, &fieldps, &len
    )) < 0) {
        SUwarning (1, "createcounterstr", "list2fields failed");
        return NULL;
    }
    if (cname) {
        cschemaname = schemap->name;
        cschemap = schemap;
        for (cfieldi = 0; cfieldi < cschemap->fieldn; cfieldi++) {
            cfieldp = &cschemap->fields[cfieldi];
            if (strcmp (cfieldp->name, cname) == 0)
                break;
        }
        if (cfieldi == cschemap->fieldn) {
            SUwarning (1, "createcounterstr", "no such field %s", cname);
            return NULL;
        }
    } else {
        if (!(cfields = vmalloc (
            Vmheap, sizeof (DDSfield_t) * (cfieldn = schemap->fieldn + 1)
        ))) {
            SUwarning (1, "createcounterstr", "vmalloc failed");
            return NULL;
        }
        n = strlen ("count");
        for (
            maxcounter = -1, cfieldi = 0; cfieldi < schemap->fieldn; cfieldi++
        ) {
            cfields[cfieldi] = schemap->fields[cfieldi];
            if (strncmp ("count", cfields[cfieldi].name, n) == 0) {
                counter = atoi (&cfields[cfieldi].name[n]);
                if (counter > maxcounter)
                    maxcounter = counter;
            }
        }
        maxcounter++;
        if (!(cfields[cfieldi].name = vmstrdup (
            Vmheap, sfprints ("count%d", maxcounter)
        )) || !(cschemaname = vmstrdup (
            Vmheap, sfprints ("%s_count%d", schemap->name, maxcounter)
        ))) {
            SUwarning (1, "createcounterstr", "vmstrdups failed");
            return NULL;
        }
        cfields[cfieldi].type = DDS_FIELD_TYPE_INT;
        cfields[cfieldi].isunsigned = FALSE;
        cfields[cfieldi].n = 1;
        if (!(cschemap = DDScreateschema (
            cschemaname, cfields, cfieldn, NULL, NULL
        ))) {
            SUwarning (1, "createcounterstr", "DDScreateschema failed");
            return NULL;
        }
        cfieldi = cschemap->fieldn - 1;
    }
    ctype = cschemap->fields[cfieldi].type;

    if (!(fp = sfopen (NULL, NULL, "sw+"))) {
        SUwarning (1, "createcounterstr", "sfopen failed");
        return NULL;
    }

    sfprintf (fp, "#include <ast.h>\n#include <cdt.h>\n#include <swift.h>\n");
    sfprintf (fp, "#include <vmalloc.h>\n#include <dds.h>\n\n");
    if (schemap->include)
        sfprintf (fp, "%s\n\n", schemap->include);
    sfprintf (fp,
        "typedef struct __k2rec_t {\n"
        "    Dtlink_t link;\n"
        "    char __key[%d];\n"
        "    %s *__counterp;\n"
        "    char __val[%d];\n"
        "} __k2rec_t;\n"
        "static Dtdisc_t __k2recdisc = {\n"
        "    sizeof (Dtlink_t), sizeof (char) * %d, 0,\n"
        "    NULL, NULL, NULL, NULL, NULL, NULL\n"
        "};\n"
        "static Dt_t *__k2recdict;\n"
        "static Vmalloc_t *__k2recvm;\n\n",
        len, _ddsfieldtypestring[ctype], cschemap->recsize, len
    );
    _ddswritestruct (schemap, sfprints ("rec_%s", schemap->name), fp);
    sfprintf (fp,
        "DDS_EXPORT char counter_%s_cname[] = \"%s\";\n\n",
    schemap->name, cname ? cname : "");
    sfprintf (fp,
        "DDS_EXPORT int counter_%s_init (Dt_t **__dictp, int *__offp) {\n"
        "    __k2rec_t __k2rec;\n"
        "    if (\n"
        "        !(__k2recvm = vmopen (Vmdcsbrk, Vmbest, 0)) ||\n"
        "        !(__k2recdict = dtopen (&__k2recdisc, Dtset))\n"
        "    )\n"
        "        return -1;\n"
        "        *__dictp = __k2recdict;\n"
        "        *__offp = (char *) &__k2rec.__val - (char *) &__k2rec;\n"
        "    return 0;\n"
        "}\n\n",
    schemap->name);
    sfprintf (fp,
        "DDS_EXPORT int counter_%s_term (void) {\n"
        "    dtclose (__k2recdict);\n"
        "    vmclose (__k2recvm);\n"
        "    return 0;\n"
        "}\n\n",
    schemap->name);
    for (fieldi = 0; fieldi < schemap->fieldn; fieldi++) {
        fieldp = &schemap->fields[fieldi];
        sfprintf (fp, "#define %s __recp->%s\n", fieldp->name, fieldp->name);
    }
    sfprintf (fp, "\n");
    sfprintf (fp,
        "DDS_EXPORT int counter_%s_work (\n"
        "    char *__buf, int __len, int __insflag\n"
        ") {\n"
        "    __k2rec_t *__k2recp;\n"
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
    if (cname) {
        sfprintf (fp,
            "    if ((__k2recp = dtmatch (__k2recdict, __key))) {\n"
            "        *__k2recp->__counterp += * (%s *) &__buf[%d];\n"
            "        return 1;\n"
            "    }\n"
            "    if (!__insflag)\n"
            "        return 2;\n"
            "    if (!(__k2recp = vmalloc (__k2recvm, sizeof (__k2rec_t))))\n"
            "        return -1;\n"
            "    memcpy (__k2recp->__key, __key, %d);\n"
            "    if (__k2recp != (__k2rec_t *)"
            " dtinsert (__k2recdict, __k2recp))\n"
            "        return -1;\n"
            "    memcpy (__k2recp->__val, __buf, %d);\n"
            "    __k2recp->__counterp = (%s *) &__k2recp->__val[%d];\n"
            "    return 1;\n"
            "}\n",
            _ddsfieldtypestring[ctype], schemap->fields[cfieldi].off, len,
            schemap->recsize, _ddsfieldtypestring[ctype],
            schemap->fields[cfieldi].off
        );
    } else {
        sfprintf (fp,
            "    if ((__k2recp = dtmatch (__k2recdict, __key))) {\n"
            "        (*__k2recp->__counterp)++;\n"
            "        return 1;\n"
            "    }\n"
            "    if (!__insflag)\n"
            "        return 2;\n"
            "    if (!(__k2recp = vmalloc (__k2recvm, sizeof (__k2rec_t))))\n"
            "        return -1;\n"
            "    memcpy (__k2recp->__key, __key, %d);\n"
            "    if (__k2recp != (__k2rec_t *)"
            " dtinsert (__k2recdict, __k2recp))\n"
            "        return -1;\n"
            "    memcpy (__k2recp->__val, __buf, %d);\n"
            "    __k2recp->__counterp = (int *) &__k2recp->__val[%d];\n"
            "    *__k2recp->__counterp = 1;\n"
            "    return 1;\n"
            "}\n",
            len, schemap->recsize, cschemap->fields[cfieldi].off
        );
    }

    if (!(cstr = _ddscopystr (fp))) {
        SUwarning (1, "createcounterstr", "copystr failed");
        return NULL;
    }
    sfclose (fp);
    if (schemap != cschemap)
        DDSdestroyschema (cschemap);
    return cstr;
}

static int loadcounterso (
    DDSschema_t *schemap, char *sofile, int rmflag, DDScounter_t *counterp
) {
    char *cname;
    char *cschemaname;
    DDSfield_t *cfields;
    int cfieldn, cfieldi;
    int n;
    int counter, maxcounter;

    if (
        !(counterp->handle = dlopen (sofile, RTLD_LAZY)) ||
        !(counterp->init = (DDScounterinitfunc) dlsym (
            counterp->handle, sfprints ("counter_%s_init", schemap->name)
        )) ||
        !(counterp->term = (DDScountertermfunc) dlsym (
            counterp->handle, sfprints ("counter_%s_term", schemap->name)
        )) ||
        !(counterp->work = (DDScounterworkfunc) dlsym (
            counterp->handle, sfprints ("counter_%s_work", schemap->name)
        )) ||
        !(cname = (char *) dlsym (
            counterp->handle, sfprints ("counter_%s_cname", schemap->name)
        ))
    ) {
        SUwarning (1, "loadcounterso", "init failed: %s", dlerror ());
        return -1;
    }
    if (rmflag)
        unlink (sofile);
    if (cname[0]) {
        cschemaname = schemap->name;
        cfields = schemap->fields, cfieldn = schemap->fieldn;
    } else {
        if (!(cfields = vmalloc (
            Vmheap, sizeof (DDSfield_t) * (cfieldn = schemap->fieldn + 1)
        ))) {
            SUwarning (1, "loadcounterso", "vmalloc failed");
            return -1;
        }
        n = strlen ("count");
        for (
            maxcounter = -1, cfieldi = 0; cfieldi < schemap->fieldn; cfieldi++
        ) {
            cfields[cfieldi] = schemap->fields[cfieldi];
            if (strncmp ("count", cfields[cfieldi].name, n) == 0) {
                counter = atoi (&cfields[cfieldi].name[n]);
                if (counter > maxcounter)
                    maxcounter = counter;
            }
        }
        maxcounter++;
        if (!(cfields[cfieldi].name = vmstrdup (
            Vmheap, sfprints ("count%d", maxcounter)
        )) || !(cschemaname = vmstrdup (
            Vmheap, sfprints ("%s_count%d", schemap->name, maxcounter)
        ))) {
            SUwarning (1, "loadcounterso", "vmstrdups failed");
            return -1;
        }
        cfields[cfieldi].type = DDS_FIELD_TYPE_INT;
        cfields[cfieldi].isunsigned = FALSE;
        cfields[cfieldi].n = 1;
    }
    if (!(counterp->schemap = DDScreateschema (
        cschemaname, cfields, cfieldn, NULL, NULL
    ))) {
        SUwarning (1, "loadcounterso", "DDScreateschema failed");
        return -1;
    }
    return 0;
}
