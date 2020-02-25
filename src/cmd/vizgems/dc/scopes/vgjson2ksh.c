#include <ast.h>
#include <swift.h>
#include <json.h>
#include <stdio.h>

static Sfio_t *strfp;

static void json_parse (json_object *, char *, char *);

int main (int argc, char **argv) {
    json_object *jobj;
    json_tokener *jtok;
    char buf[10000];
    int len, done;

    done = FALSE;
    if (argc < 2)
        SUerror ("json2ksh", "missing top level variable name");
    if (!(jtok = json_tokener_new ()))
        SUerror ("json2ksh", "cannot create tokener");
    while ((len = read (0, buf, 10000)) > 0) {
        if ((jobj = json_tokener_parse_ex (jtok, buf, len))) {
            json_parse (jobj, argv[1], NULL);
            done = TRUE;
            break;
        } else if (json_tokener_get_error (jtok) != json_tokener_continue)
            break;
    }
    if (!done)
        sfprintf (sfstdout, "%s=(nodata=1)\n", argv[1]);
}

static void json_parse (json_object *jobj, char *path, char *key) {
    enum json_type type;
    json_object_iter iter;
    json_object *jobj2, *jarray, *jval;
    int jarrayn, jarrayi;
    Sfio_t *strfp;
    char *npath, ibuf[100], *s;

    if (path) {
        if (key)
            sfprintf (sfstdout, "%s.[%s]=", path, key);
        else
            sfprintf (sfstdout, "%s=", path);
    } else if (key)
        sfprintf (sfstdout, "%s=", key);

    type = json_object_get_type (jobj);
    switch (type) {
    case json_type_boolean:
        sfprintf (
            sfstdout, "%s\n", json_object_get_boolean (jobj) ? "true": "false"
        );
        break;
    case json_type_double:
        sfprintf (sfstdout, "%lf\n", json_object_get_double (jobj));
        break;
    case json_type_int:
        sfprintf (sfstdout, "%ld\n", json_object_get_int64 (jobj));
        break;
    case json_type_string:
        strfp = sfstropen ();
        for (s = (char *) json_object_get_string (jobj); *s; s++) {
            if (*s == '$')
                sfputc (strfp, '\\');
            sfputc (strfp, *s);
        }
        sfprintf (sfstdout, "\"%s\"\n", sfstruse (strfp));
        sfstrclose (strfp);
        break;
    case json_type_object:
        strfp = sfstropen ();
        if (path) {
            if (key)
                sfprintf (strfp, "%s.[%s]", path, key);
            else
                sfprintf (strfp, "%s", path);
        } else
            sfprintf (strfp, "%s", key);
        npath = sfstruse (strfp);

        sfprintf (sfstdout, "()\n");
        json_object_object_foreach (jobj, key, val) {
            json_parse (val, npath, key);
        }
        sfstrclose (strfp);
        break;
    case json_type_array:
        strfp = sfstropen ();
        if (path) {
            if (key)
                sfprintf (strfp, "%s.[%s]", path, key);
            else
                sfprintf (strfp, "%s", path);
        } else
            sfprintf (strfp, "%s", key);
        npath = sfstruse (strfp);

        sfprintf (sfstdout, "()\n");
        jarray = jobj;
        jarrayn = json_object_array_length (jarray);

        for (jarrayi = 0; jarrayi < jarrayn; jarrayi++) {
            jval = json_object_array_get_idx (jarray, jarrayi);
            sfsprintf (ibuf, 100, "%d", jarrayi);
            json_parse (jval, npath, ibuf);
        }
        break;
    default:
        sfprintf (sfstdout, "\n");
        break;
    }
}
