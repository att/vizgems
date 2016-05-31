#pragma prototyped

#include <ast.h>
#include <vmalloc.h>
#include <dlfcn.h>
#include <swift.h>
#include <dds.h>
#include <dds_int.h>

static char *createaggregatorstr (DDSschema_t *, DDSexpr_t *, int);
static int loadaggregatorso (DDSschema_t *, char *, int, DDSaggregator_t *);

DDSaggregator_t *DDScreateaggregator (
    DDSschema_t *schemap, DDSexpr_t *aep, int itype,
    char *ccflags, char *ldflags
) {
    DDSaggregator_t *aggregatorp;
    char *cstr, *sostr;

    if (!(aggregatorp = vmalloc (Vmheap, sizeof (DDSaggregator_t)))) {
        SUwarning (1, "DDScreateaggregator", "vmalloc failed");
        return NULL;
    }
    if (!(cstr = createaggregatorstr (schemap, aep, itype))) {
        SUwarning (1, "DDScreateaggregator", "createaggregatorstr failed");
        return NULL;
    }
    if (!(sostr = _ddscreateso (cstr, aep->libs, NULL, ccflags, ldflags))) {
        SUwarning (1, "DDScreateaggregator", "createso failed");
        return NULL;
    }
    if (loadaggregatorso (schemap, sostr, TRUE, aggregatorp) == -1) {
        SUwarning (1, "DDScreateaggregator", "loadaggregatorso failed");
        return NULL;
    }
    return aggregatorp;
}

char *DDScompileaggregator (
    DDSschema_t *schemap, DDSexpr_t *aep, int itype, char *sostr,
    char *ccflags, char *ldflags
) {
    char *cstr;

    if (!(cstr = createaggregatorstr (schemap, aep, itype))) {
        SUwarning (1, "DDScompileaggregator", "createaggregatorstr failed");
        return NULL;
    }
    if (!(sostr = _ddscreateso (cstr, aep->libs, sostr, ccflags, ldflags))) {
        SUwarning (1, "DDScompileaggregator", "createso failed");
        return NULL;
    }
    return sostr;
}

DDSaggregator_t *DDSloadaggregator (DDSschema_t *schemap, char *sostr) {
    DDSaggregator_t *aggregatorp;
    char file[PATH_MAX];

    if (!(aggregatorp = vmalloc (Vmheap, sizeof (DDSaggregator_t)))) {
        SUwarning (1, "DDSloadaggregator", "vmalloc failed");
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
        SUwarning (1, "DDSloadaggregator", "access failed for %s", sostr);
        return NULL;
    }
    if (loadaggregatorso (schemap, file, FALSE, aggregatorp) == -1) {
        SUwarning (1, "DDSloadaggregator", "loadaggregatorso failed");
        return NULL;
    }
    return aggregatorp;
}

int DDSdestroyaggregator (DDSaggregator_t *aggregatorp) {
    dlclose (aggregatorp->handle);
    vmfree (Vmheap, aggregatorp);
    return 0;
}

static char *createaggregatorstr (
    DDSschema_t *schemap, DDSexpr_t *aep, int itype
) {
    Sfio_t *fp;
    char *s;
    DDSfield_t *fieldp;
    int fieldi;
    char *cstr;

    if (!(fp = sfopen (NULL, NULL, "sw+"))) {
        SUwarning (1, "createaggregatorstr", "sfopen failed");
        return NULL;
    }

    sfprintf (fp, "#include <ast.h>\n#include <vmalloc.h>\n");
    sfprintf (fp, "#include <swift.h>\n");
    sfprintf (fp, "#include <dds.h>\n#include <aggr.h>\n\n");
    if (schemap->include)
        sfprintf (fp, "%s\n\n", schemap->include);
    _ddswritestruct (schemap, sfprints ("rec_%s", schemap->name), fp);
    if (aep->vars)
        sfprintf (fp, "#line 1 \"VARS\"\n%s\n", aep->vars);
    sfprintf (fp,
        "DDS_EXPORT int aggregator_%s_init (AGGRfile_t *__afp) {\n"
        "    {\n#line 1 \"BEGIN\"\n        %s\n    }\n"
        "    return 0;\n"
        "}\n\n",
    schemap->name, (aep->begin ? aep->begin : ""));
    sfprintf (fp,
        "DDS_EXPORT int aggregator_%s_term (AGGRfile_t *__afp) {\n"
        "    {\n#line 1 \"END\"\n        %s\n    }\n"
        "    return 0;\n"
        "}\n\n",
    schemap->name, (aep->end ? aep->end : ""));
    switch (itype) {
    case AGGR_TYPE_CHAR:   s = "c"; break;
    case AGGR_TYPE_SHORT:  s = "s"; break;
    case AGGR_TYPE_INT:    s = "i"; break;
    case AGGR_TYPE_LLONG:  s = "l"; break;
    case AGGR_TYPE_FLOAT:  s = "f"; break;
    case AGGR_TYPE_DOUBLE: s = "d"; break;
    }
    sfprintf (fp,
        "#define KEYFRAMEVALUE(key, frame, value) {\\\n"
        "    AGGRunit_t __vu;\\\n"
        "    __vu.u.%s = (value);\\\n"
        "    __kv.keyp = (unsigned char *) (key);\\\n"
        "    __kv.framei = (frame);\\\n"
        "    __kv.valp = (unsigned char *) &__vu.u.%s;\\\n"
        "    __kv.itemi = -1;\\\n"
        "    AGGRappend (__afp, &__kv, 1);\\\n"
        "}\n",
    s, s);
    sfprintf (fp,
        "#define ITEMFRAMEVALUE(item, frame, value) {\\\n"
        "    AGGRunit_t __vu;\\\n"
        "    __vu.u.%s = (value);\\\n"
        "    __kv.keyp = NULL;\\\n"
        "    __kv.framei = (frame);\\\n"
        "    __kv.valp = (unsigned char *) &__vu.u.%s;\\\n"
        "    __kv.itemi = (item);\\\n"
        "    AGGRappend (__afp, &__kv, 1);\\\n"
        "}\n",
    s, s);
    sfprintf (fp,
        "#define KEYFRAMEVALUEP(key, frame, value) {\\\n"
        "    __kv.keyp = (unsigned char *) (key);\\\n"
        "    __kv.framei = (frame);\\\n"
        "    __kv.valp = (unsigned char *) (value);\\\n"
        "    __kv.itemi = -1;\\\n"
        "    AGGRappend (__afp, &__kv, 1);\\\n"
        "}\n"
    );
    sfprintf (fp,
        "#define ITEMFRAMEVALUEP(item, frame, value) {\\\n"
        "    __kv.keyp = NULL;\\\n"
        "    __kv.framei = (frame);\\\n"
        "    __kv.valp = (unsigned char *) (value);\\\n"
        "    __kv.itemi = (item);\\\n"
        "    AGGRappend (__afp, &__kv, 1);\\\n"
        "}\n"
    );
    sfprintf (fp, "#define OPER __kv.oper\n");
    sfprintf (fp, "#define LASTITEM __kv.itemi\n");
    for (fieldi = 0; fieldi < schemap->fieldn; fieldi++) {
        fieldp = &schemap->fields[fieldi];
        sfprintf (fp, "#define %s __recp->%s\n", fieldp->name, fieldp->name);
    }
    sfprintf (fp, "\n");
    sfprintf (fp,
        "DDS_EXPORT int aggregator_%s_work (\n"
        "    char *__buf, int __len, AGGRfile_t *__afp\n"
        ") {\n"
        "    AGGRkv_t __kv;\n"
        "    struct __rec_%s_t *__recp;\n"
        "    __kv.oper = AGGR_APPEND_OPER_ADD;\n"
        "    __recp = (struct __rec_%s_t *) __buf;\n",
    schemap->name, schemap->name, schemap->name);
    sfprintf (fp, "\n    {\n#line 1 \"LOOP\"\n    %s\n    }\n", aep->loop);
    sfprintf (fp, "    return 0;\n");
    sfprintf (fp, "}\n");

    if (!(cstr = _ddscopystr (fp))) {
        SUwarning (1, "createaggregatorstr", "copystr failed");
        return NULL;
    }
    sfclose (fp);
    return cstr;
}

static int loadaggregatorso (
    DDSschema_t *schemap, char *sofile, int rmflag, DDSaggregator_t *aggregatorp
) {
    if (
        !(aggregatorp->handle = dlopen (sofile, RTLD_LAZY)) ||
        !(aggregatorp->init = (DDSaggregatorinitfunc) dlsym (
            aggregatorp->handle, sfprints ("aggregator_%s_init", schemap->name)
        )) ||
        !(aggregatorp->term = (DDSaggregatortermfunc) dlsym (
            aggregatorp->handle, sfprints ("aggregator_%s_term", schemap->name)
        )) ||
        !(aggregatorp->work = (DDSaggregatorworkfunc) dlsym (
            aggregatorp->handle, sfprints ("aggregator_%s_work", schemap->name)
        ))
    ) {
        SUwarning (1, "loadaggregatorso", "init failed: %s", dlerror ());
        return -1;
    }
    if (rmflag)
        unlink (sofile);
    return 0;
}
