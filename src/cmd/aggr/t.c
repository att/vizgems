#pragma prototyped

#include <ast.h>
#include <vmalloc.h>
#include <swift.h>
#include <aggr.h>

static void prompt (char *);

main (int argc, char **argv) {
    AGGRfile_t *afp;
    int i;
    char foo[4];
    char *cp;
    AGGRkv_t kv;

    if (AGGRinit () == -1)
        SUerror ("main", "AGGRinit failed");
    if (strcmp (argv[1], "c") == 0) {
        if (!(afp = AGGRcreate (
            "foo", 1, 1, AGGR_TYPE_STRING, AGGR_TYPE_CHAR,
            "abc123", 3, 1, 256, 256, 1, FALSE
        )))
            SUerror ("main", "create failed");
        prompt ("press to close");
        if (AGGRclose (afp) == -1)
            SUerror ("main", "close failed");
    } else if (strcmp (argv[1], "r") == 0) {
        if (!(afp = AGGRopen ("foo", AGGR_RWMODE_RD, 1, FALSE)))
            SUerror ("main", "open failed");
        SUwarning (0, "main", "dict sernum: %d", afp->hdr.dictsn);
        SUwarning (0, "main", "data sernum: %d", afp->hdr.datasn);
        SUwarning (0, "main", "itemn: %d", afp->hdr.itemn);
        SUwarning (0, "main", "ditemn: %d", afp->ad.itemn);
        SUwarning (0, "main", "framen: %d", afp->hdr.framen);
        SUwarning (0, "main", "ktype: %d", (int) afp->hdr.keytype);
        SUwarning (0, "main", "vtype: %d", (int) afp->hdr.valtype);
        SUwarning (0, "main", "dictlen: %d", afp->hdr.dictlen);
        SUwarning (0, "main", "datalen: %d", afp->hdr.datalen);
        SUwarning (0, "main", "minval: %f", afp->hdr.minval);
        SUwarning (0, "main", "maxval: %f", afp->hdr.maxval);
        SUwarning (0, "main", "class: %s", afp->hdr.class);
        SUwarning (0, "main", "dict size: %d", afp->ad.byten);
        prompt ("press to close");
        if (AGGRclose (afp) == -1)
            SUerror ("main", "close failed");
    } else if (strcmp (argv[1], "g") == 0) {
        if (!(afp = AGGRopen ("foo", AGGR_RWMODE_RW, 1, FALSE)))
            SUerror ("main", "open failed");
        if (AGGRgrow (afp, afp->hdr.itemn + 2, afp->hdr.framen + 2))
            SUerror ("main", "grow failed");
        SUwarning (0, "main", "dict sernum: %d", afp->hdr.dictsn);
        SUwarning (0, "main", "data sernum: %d", afp->hdr.datasn);
        SUwarning (0, "main", "itemn: %d", afp->hdr.itemn);
        SUwarning (0, "main", "framen: %d", afp->hdr.framen);
        SUwarning (0, "main", "ktype: %d", (int) afp->hdr.keytype);
        SUwarning (0, "main", "vtype: %d", (int) afp->hdr.valtype);
        SUwarning (0, "main", "dictlen: %d", afp->hdr.dictlen);
        SUwarning (0, "main", "datalen: %d", afp->hdr.datalen);
        SUwarning (0, "main", "minval: %f", afp->hdr.minval);
        SUwarning (0, "main", "maxval: %f", afp->hdr.maxval);
        SUwarning (0, "main", "class: %s", afp->hdr.class);
        SUwarning (0, "main", "dict size: %d", afp->ad.byten);
        prompt ("press to close");
        if (AGGRclose (afp) == -1)
            SUerror ("main", "close failed");
    } else if (strcmp (argv[1], "f") == 0) {
        if (!(afp = AGGRopen ("foo", AGGR_RWMODE_RW, 1, FALSE)))
            SUerror ("main", "open failed");
        if (!(cp = AGGRget (afp, 0)))
            SUerror ("main", "get failed");
        for (i = 0; i < afp->hdr.itemn; i++)
            cp[i] = 31;
        prompt ("press to release");
        if (AGGRrelease (afp) == -1)
            SUerror ("main", "release failed");
        prompt ("press to close");
        if (AGGRclose (afp) == -1)
            SUerror ("main", "close failed");
    } else if (strcmp (argv[1], "q") == 0) {
        if (!(afp = AGGRopen ("foo", AGGR_RWMODE_RW, 10, FALSE)))
            SUerror ("main", "open failed");
        foo[0] = 'a', foo[1] = 'b', foo[2] = 'c', foo[3] = 0;
        kv.framei = 0;
        kv.keyp = (unsigned char *) foo;
        for (i = 0; i < 20; i++) {
            foo[i % 3] = 'a' + random () % 20;
            kv.valp = (unsigned char *) &i;
            kv.itemi = -1;
            SUwarning (
                0, "main", "queue: %s %d", kv.keyp, AGGRappend (afp, &kv, TRUE)
            );
            SUwarning (0, "main", "          %d", kv.itemi);
            SUwarning (0, "main", "ditemn: %d", afp->ad.itemn);
        }
        prompt ("press to flush");
        if (AGGRflush (afp) == -1)
            SUerror ("main", "flush failed");
        prompt ("press to close");
        if (AGGRclose (afp) == -1)
            SUerror ("main", "close failed");
    }
    AGGRterm ();
    return 0;
}

static void prompt (char *str) {
    sfprintf (sfstderr, "%s", str);
    getchar ();
}
