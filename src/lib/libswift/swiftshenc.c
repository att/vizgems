#include <ast.h>
#include <vmalloc.h>
#include <shell.h>
#include <swift.h>

_BEGIN_EXTERNS_ /* public data */
#if _BLD_swift && defined(__EXPORT__)
#define __PUBLIC_DATA__ __EXPORT__
#else
#if !_BLD_swift && defined(__IMPORT__)
#define __PUBLIC_DATA__ __IMPORT__
#else
#define __PUBLIC_DATA__
#endif
#endif
#undef __PUBLIC_DATA__

#if _BLD_swift && defined(__EXPORT__)
#define extern  __EXPORT__
#endif

extern int b_swiftshenc (int, char **, void *);
extern int b_swiftshdec (int, char **, void *);

#undef extern
_END_EXTERNS_

unsigned long plugin_version (void) {
    return 20111111;
}

int b_swiftshenc (int argc, char **argv, void *extra) {
    Namval_t *invp, *onvp, *fvp;
    Shell_t *shp = ((Shbltin_t *) extra)->shp;
    unsigned char *astr, *ostr;
    struct stat st;
    int uid, euid;
    char *file, name[100];

    if (argc < 3) {
        error (0, "swiftshenc: %s", "bad usage");
        return 0;
    }

    /* security checks */
    strcpy (name, ".sh.file");
    fvp = nv_open (name, shp->var_tree, NV_VARNAME | NV_NOASSIGN | NV_NOADD);
    if (nv_isnull (fvp))
        return 0;
    if (!(file = nv_getval (fvp)))
        return 0;
    if (stat (file, &st) == -1)
        return 0;
    uid = getuid (), euid = geteuid ();
    if (st.st_uid != uid || st.st_uid != euid)
        return 0;

    invp = nv_open (
        argv[1], shp->var_tree, NV_VARNAME | NV_NOASSIGN | NV_NOADD
    );
    if (nv_isnull (invp)) {
        error (0, "swiftshenc: %s", "no input variable");
        return 0;
    }
    astr = (unsigned char *) nv_getval (invp);

    if (SWenc (Vmregion, astr, &ostr) == -1) {
        error (0, "swiftshenc: %s", "encoding failed");
        return 0;
    }

    onvp = nv_open (argv[2], shp->var_tree, NV_VARNAME | NV_NOASSIGN);
    if (!nv_isnull (onvp))
        nv_unset (onvp);
    nv_putval (onvp, (char *) ostr, 0);

    nv_close (invp);
    nv_close (onvp);

    return 0;
}

int b_swiftshdec (int argc, char **argv, void *extra) {
    Namval_t *invp, *onvp, *fvp;
    Shell_t *shp = ((Shbltin_t *) extra)->shp;
    unsigned char *estr, *ostr;
    struct stat st;
    int uid, euid;
    char *file, name[100];

    if (argc < 3) {
        error (0, "swiftshdec: %s", "bad usage");
        return 0;
    }

    /* security checks */
    strcpy (name, ".sh.file");
    fvp = nv_open (name, shp->var_tree, NV_VARNAME | NV_NOASSIGN | NV_NOADD);
    if (nv_isnull (fvp))
        return 0;
    if (!(file = nv_getval (fvp)))
        return 0;
    if (stat (file, &st) == -1)
        return 0;
    uid = getuid (), euid = geteuid ();
    if (st.st_uid != uid || st.st_uid != euid)
        return 0;

    invp = nv_open (
        argv[1], shp->var_tree, NV_VARNAME | NV_NOASSIGN | NV_NOADD
    );
    if (nv_isnull (invp)) {
        error (0, "swiftshdec: %s", "no input variable");
        return 0;
    }
    estr = (unsigned char *) nv_getval (invp);

    if (SWdec (Vmregion, estr, &ostr) == -1) {
        error (0, "swiftshdec: %s", "decoding failed");
        return 0;
    }

    onvp = nv_open (argv[2], shp->var_tree, NV_VARNAME | NV_NOASSIGN);
    if (!nv_isnull (onvp))
        nv_unset (onvp);
    nv_putval (onvp, (char *) ostr, 0);

    nv_close (invp);
    nv_close (onvp);

    return 0;
}
