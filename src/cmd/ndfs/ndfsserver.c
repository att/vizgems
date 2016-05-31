#include <ast.h>
#include <aso.h>
#include <cdt.h>
#include <dt.h>
#include <option.h>
#include <swift.h>
#include "ndfsserver.h"

static const char usage[] =
"[-1p1?\n@(#)$Id: ndfsserver (AT&T Labs Research) 2013-01-10 $\n]"
USAGE_LICENSE
"[+NAME?ndfsserver - server for the AST 3d filesystem]"
"[+DESCRIPTION?\bndfsserver\b is a FUSE implementation of the AST 3d"
" filesystem."
"]"
"[100:f?specifies to run this server in the foreground."
" The default is to detach and run as a daemon."
"]"
"[101:s?specifies to run in single-thread mode."
" The default is multi-threaded."
"]"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"\n"
"\n<bottom directory> <top directory>\n"
"\n"
"[+SEE ALSO?\b3d\b(1)]"
;

Vmalloc_t *vm;

char *tprefixstr, *bprefixstr, *pprefixstr;
int tprefixlen, bprefixlen, pprefixlen;
int dfd;
time_t currtime;
uid_t uid;
gid_t gid;
pthread_mutex_t mutex;

static void *init (struct fuse_conn_info *);

struct fuse_operations ndfsops = {
    // start clock thread
    .init       = init,

    // filesystem calls
    .statfs     = ndfsstatfs,

    // metadata calls
    .mkdir      = ndfsmkdir,
    .rmdir      = ndfsrmdir,
    .link       = ndfslink,
    .symlink    = ndfssymlink,
    .unlink     = ndfsunlink,
    .readlink   = ndfsreadlink,
    .chmod      = ndfschmod,
    .utimens    = ndfsutimens,
    .rename     = ndfsrename,
    .access     = ndfsaccess,
    .getattr    = ndfsgetattr,
    .fgetattr   = ndfsfgetattr,

    // data calls
    .opendir    = ndfsopendir,
    .readdir    = ndfsreaddir,
    .releasedir = ndfsreleasedir,
    .create     = ndfscreate,
    .open       = ndfsopen,
    .flush      = ndfsflush,
    .release    = ndfsrelease,
    .read       = ndfsread,
    .write      = ndfswrite,
    .truncate   = ndfstruncate,
    .ftruncate  = ndfsftruncate,
    .fsync      = ndfsfsync,
    .ioctl      = ndfsioctl,

    .flag_nullpath_ok = 1,
};

int main (int argc, char **argv) {
    int norun;
    char *fuseargv[100];
    int fuseargc, fuseargc1;
    int daemonmode, multithreadmode;

    int ret;
    char *s, path[PATH_MAX + 10];
    int l;

    norun = 0;
    fuseargc = 0;
    daemonmode = TRUE;
    multithreadmode = TRUE;
    for (;;) {
        switch (optget (argv, usage)) {
        case -100:
            daemonmode = FALSE;
            continue;
        case -101:
            multithreadmode = FALSE;
            continue;
        case -999:
            SUwarnlevel++;
            continue;
        case '?':
            SUusage (0, "ndfsserver", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "ndfsserver", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;
    if (argc != 2) {
        SUusage (1, "ndfsserver", opt_info.arg);
        return 1;
    }

    pthread_mutex_init (&mutex, NULL);
    tprefixstr = NULL;
    tprefixlen = 0;
    bprefixstr = NULL;
    bprefixlen = 0;
    dfd = -1;

    uid = getuid ();
    gid = getgid ();
    if (!(vm = vmopen (Vmdcsbrk, Vmbest, VMFLAGS)))
        SUerror ("ndfsserver", "cannot open vmregion");
    if (utilinit () == -1)
        SUerror ("ndfsserver", "cannot initialize utils");

    strcpy (path, argv[0]);
    if ((l = strlen (path)) == 0 || l >= PATH_MAX || path[0] != '/')
        SUerror ("ndfsserver", "bad bottom prefix %s", path);
    if (path[l - 1] != '/')
        path[l++] = '/', path[l] = 0;
    if (!(bprefixstr = vmstrdup (vm, path)))
        SUerror ("ndfsserver", "cannot copy bottom prefix %s", path);
    bprefixlen = strlen (bprefixstr);
    strcpy (path, argv[1]);
    if ((l = strlen (path)) == 0 || l >= PATH_MAX || path[0] != '/')
        SUerror ("ndfsserver", "bad top prefix %s", path);
    if (path[l - 1] != '/')
        path[l++] = '/', path[l] = 0;
    if (!(tprefixstr = vmstrdup (vm, path)))
        SUerror ("ndfsserver", "cannot copy top prefix %s", path);
    tprefixlen = strlen (tprefixstr);
    path[l - 1] = 0;
    s = strrchr (path, '/'), *s = 0;
    if (!(pprefixstr = vmstrdup (vm, path)))
        SUerror ("ndfsserver", "cannot copy parent prefix %s", path);
    pprefixlen = strlen (pprefixstr);

    fuseargv[fuseargc++] = "ndfsserver";
    fuseargv[fuseargc++] = "-o";
    fuseargv[fuseargc++] = "nonempty";
    fuseargv[fuseargc++] = "-o";
    fuseargv[fuseargc++] = "use_ino";
    if (!daemonmode)
        fuseargv[fuseargc++] = "-f";
    if (!multithreadmode)
        fuseargv[fuseargc++] = "-s";
    fuseargv[fuseargc] = NULL;
    fuseargc1 = fuseargc;
    fuseargv[fuseargc++] = "-o";
    fuseargv[fuseargc++] = "allow_other";
    fuseargv[fuseargc++] = "-o";
    fuseargv[fuseargc++] = "default_permissions";
    fuseargv[fuseargc++] = tprefixstr;
    fuseargv[fuseargc] = NULL;

    if ((dfd = open (tprefixstr, O_RDONLY)) == -1)
        SUerror ("ndfsserver", "cannot open top directory %s", tprefixstr);

    if ((ret = fuse_main (fuseargc, fuseargv, &ndfsops, NULL)) != 0) {
        SUwarning (0, "ndfsserver", "trying mount without allow_other option");
        fuseargc = fuseargc1;
        fuseargv[fuseargc++] = tprefixstr;
        fuseargv[fuseargc] = NULL;
        ret = fuse_main (fuseargc, fuseargv, &ndfsops, NULL);
    }

    utilterm ();

    return ret;
}

static pthread_t clockthread;

static void *clockserve (void *data) {
    time_t t;

    for (;;) {
        t = time (NULL);
#ifdef _lib___sync_lock_test_and_set
        __sync_lock_test_and_set (&currtime, t);
#else
        currtime = t;
#endif
        sleep (1);
    }
    return NULL;
}

static void *init (struct fuse_conn_info *fcip) {
    fchdir (dfd);
    close (dfd);
    pthread_create (&clockthread, NULL, &clockserve, NULL);
    return NULL;
}
