#pragma prototyped

#include <ast.h>
#include <sfdisc.h>
#include <vmalloc.h>
#include <tok.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <swift.h>
#include <time.h>

#ifndef MAP_AUTOGROW
#define MAP_AUTOGROW 0
#endif

#ifndef MAP_AUTORESRV
#define MAP_AUTORESRV 0
#endif

typedef struct mpipew_t {
    Sfdisc_t disc;
    Sfio_t *fp;
    char *datap;
    char *fnamep;
    size_t buflen;
    int *pids;
    int *rpipes;
    int *wpipes;
    int n;
    Vmalloc_t *vm;
} mpipew_t;

typedef struct mpiper_t {
    Sfdisc_t disc;
    Sfio_t *fp;
    char *datap;
    char *fnamep;
    size_t off, len, buflen;
    int wpipe;
    Vmalloc_t *vm;
} mpiper_t;

static mpipew_t *mpipewp;
static mpiper_t *mpiperp;

static mpipew_t *SUopenmpipew (char **, int, size_t);
static int SUclosempipew (mpipew_t *);
static mpiper_t *SUopenmpiper (void);
static int SUclosempiper (mpiper_t *);

static ssize_t mpipewread (Sfio_t *, void *, size_t, Sfdisc_t *);
static ssize_t mpipewwrite (Sfio_t *, const void *, size_t, Sfdisc_t *);
static int mpipewexcept (Sfio_t *, int, void *, Sfdisc_t *);

static ssize_t mpiperread (Sfio_t *, void *, size_t, Sfdisc_t *);
static ssize_t mpiperwrite (Sfio_t *, const void *, size_t, Sfdisc_t *);
static int mpiperexcept (Sfio_t *, int, void *, Sfdisc_t *);

static int fullread (int, void *, size_t);
static int fullwrite (int, void *, size_t);

Sfio_t *SUsetupinput (Sfio_t *ifp, size_t buflen) {
    char *fnamep;

    if (ifp != sfstdin || !(fnamep = getenv ("SWIFTMPIPE")) || !*fnamep) {
        if (getenv ("SWIFTDIO") && sfseek (ifp, 0, SEEK_CUR) != -1)
            sfdcdio (ifp, buflen);
        else
            sfsetbuf (ifp, NULL, buflen);
        return ifp;
    }
    if (!(mpiperp = SUopenmpiper ())) {
        SUwarning (1, "SUsetupinput", "setupmpipe failed");
        return NULL;
    }
    return mpiperp->fp;
}

Sfio_t *SUsetupoutput (char *cmdstr, Sfio_t *ofp, size_t buflen) {
    char *cmds[1000], *s;
    int cmdn;

    if (!cmdstr) {
        if (getenv ("SWIFTDIO") && sfseek (ofp, 0, SEEK_CUR) != -1)
            sfdcdio (ofp, buflen);
        else
            sfsetbuf (ofp, NULL, buflen);
        return ofp;
    }
    if ((cmdn = tokscan (cmdstr, &s, " %v ", cmds, 1000)) < 1 || cmdn > 1000) {
        SUwarning (1, "SUsetupoutput", "tokscan failed");
        return NULL;
    }
    if (!(mpipewp = SUopenmpipew (cmds, cmdn, buflen))) {
        SUwarning (1, "SUsetupoutput", "openmpipe failed");
        return NULL;
    }
    return mpipewp->fp;
}

static mpipew_t *SUopenmpipew (char **cmds, int cmdn, size_t buflen) {
    Vmalloc_t *vm;
    mpipew_t *mpwp;
    Sfio_t *fp;
    char *datap;
    char *tmpdir;
    char *fnamep;
    int p1[2], p2[2];
    int i;

    static char env[1000];
    static char c;

    vm = NULL;
    mpwp = NULL;
    fp = NULL;
    datap = NULL;
    srandom (time (NULL));
    if (!(tmpdir = getenv ("TMPDIR")))
        tmpdir = "/tmp";
    for (i = 0; i < 100; i++) {
        fnamep = sfprints (
            "%s/ddsmpipe.%d.%d.%d", tmpdir, getpid (), getuid (), random ()
        );
        if (
            access (fnamep, F_OK) == -1 &&
            (fp = sfopen (NULL, fnamep, "w+")) &&
            lseek (fp->_file, buflen - 1, SEEK_SET) == buflen - 1 &&
            write (fp->_file, &c, 1) == 1 &&
            lseek (fp->_file, 0, SEEK_SET) == 0 &&
            (datap = mmap (
                NULL, buflen, PROT_WRITE,
                MAP_SHARED | MAP_AUTOGROW | MAP_AUTORESRV, fp->_file, 0)
            )
        )
            break;
    }
    if (i == 100) {
        SUwarning (1, "SUopenmpipew", "cannot find shared file");
        goto abortSUopenmpipew;
    }
    datap[buflen - 1] = 0;
    if (
        !(vm = vmopen (Vmdcsbrk, Vmbest, 0)) ||
        !(mpwp = vmalloc (vm, sizeof (mpipew_t))) ||
        !(mpwp->fnamep = vmstrdup (vm, fnamep)) ||
        !(mpwp->pids = vmalloc (vm, cmdn * sizeof (int))) ||
        !(mpwp->rpipes = vmalloc (vm, cmdn * sizeof (int))) ||
        !(mpwp->wpipes = vmalloc (vm, cmdn * sizeof (int)))
    ) {
        SUwarning (1, "SUopenmpipew", "allocation failed");
        goto abortSUopenmpipew;
    }
    mpwp->vm = vm;
    memset (&mpwp->disc, 0, sizeof (mpwp->disc));
    mpwp->disc.readf = mpipewread;
    mpwp->disc.writef = mpipewwrite;
    mpwp->disc.exceptf = mpipewexcept;
    if(sfdisc (fp, (Sfdisc_t *) mpwp) != (Sfdisc_t *) mpwp) {
        SUwarning (1, "SUopenmpipew", "sfdisc failed");
        goto abortSUopenmpipew;
    }
    sfsetbuf (fp, NULL, buflen);
    mpwp->fp = fp;
    mpwp->datap = datap;
    mpwp->buflen = buflen;
    mpwp->n = cmdn;
    sfsprintf (env, 1000, "SWIFTMPIPE=%s", fnamep);
    putenv (env);
    for (i = 0; i < cmdn; i++) {
        if (pipe (p1) == -1 || pipe (p2) == -1) {
            SUwarning (1, "SUopenmpipew", "pipe failed");
            goto abortSUopenmpipew;
        }
        switch ((mpwp->pids[i] = fork ())) {
        case -1:
            SUwarning (1, "SUopenmpipew", "fork failed");
            goto abortSUopenmpipew;
        case 0:
            close (p1[0]), close (p2[1]);
            close (0), dup (p2[0]), close (p2[0]);
            execl ("/bin/ksh", "ksh", "-c", cmds[i], NULL);
            SUerror ("SUopenmpipew", "execl failed for %s", cmds[i]);
            break;
        default:
            close (p1[1]), close (p2[0]);
            break;
        }
        fcntl (p1[0], F_SETFD, FD_CLOEXEC);
        fcntl (p2[1], F_SETFD, FD_CLOEXEC);
        mpwp->wpipes[i] = p2[1], mpwp->rpipes[i] = p1[0];
        if (
            fullwrite (mpwp->wpipes[i], &p1[1], sizeof (int)) == -1 ||
            fullwrite (mpwp->wpipes[i], &buflen, sizeof (size_t)) == -1
        ) {
            SUwarning (1, "SUopenmpipew", "process startup failed");
            goto abortSUopenmpipew;
        }
    }
    sfsprintf (env, 1000, "SWIFTMPIPE=");
    putenv (env);
    return mpwp;

abortSUopenmpipew:
    if (datap)
        munmap (datap, buflen);
    if (fp)
        sfclose (fp), unlink (fnamep);
    if (vm)
        vmclose (vm);
    return NULL;
}

static int SUclosempipew (mpipew_t *mpwp) {
    int ret, i, child, status;

    ret = 0;
    for (i = 0; i < mpwp->n; i++) {
        if (fullread (mpwp->rpipes[i], &child, sizeof (int)) == -1) {
            SUwarning (1, "SUclosepipew", "fullread failed");
            ret = -1;
        }
        close (mpwp->wpipes[i]), close (mpwp->rpipes[i]);
    }
    for (i = 0; i < mpwp->n; i++) {
        if (
            waitpid (mpwp->pids[i], &status, 0) == -1 ||
            !WIFEXITED (status) || WEXITSTATUS (status)
        ) {
            SUwarning (1, "SUclosepipew", "waitpid failed");
            ret = -1;
        }
    }
    munmap (mpwp->datap, mpwp->buflen);
    unlink (mpwp->fnamep);
    vmclose (mpwp->vm);
    return ret;
}

static mpiper_t *SUopenmpiper (void) {
    Vmalloc_t *vm;
    mpiper_t *mprp;
    char *fnamep;
    int ok;

    static char env[1000];

    if (!(fnamep = getenv ("SWIFTMPIPE")) || !*fnamep)
        return NULL;
    ok = 0;
    vm = NULL;
    mprp = NULL;
    if (
        !(vm = vmopen (Vmdcsbrk, Vmbest, 0)) ||
        !(mprp = vmalloc (vm, sizeof (mpiper_t))) ||
        !(mprp->fnamep = vmstrdup (vm, fnamep))
    ) {
        SUwarning (1, "SUopenmpiper", "allocation failed");
        goto abortSUopenmpiper;
    }
    mprp->fp = NULL;
    mprp->datap = NULL;
    if (
        fullread (0, &mprp->wpipe, sizeof (int)) != sizeof (int) ||
        fullread (0, &mprp->buflen, sizeof (size_t)) != sizeof (size_t)
    ) {
        SUwarning (1, "SUopenmpiper", "read failed");
        goto abortSUopenmpiper;
    }
    if (
        !(mprp->fp = sfopen (NULL, fnamep, "r")) ||
        !(mprp->datap = mmap (
            NULL, mprp->buflen, PROT_READ, MAP_SHARED, mprp->fp->_file, 0
        ))
    ) {
        SUwarning (1, "SUopenmpiper", "open failed");
        goto abortSUopenmpiper;
    }
    if (fullwrite (mprp->wpipe, &ok, sizeof (int)) == -1) {
        SUwarning (1, "SUopenmpiper", "write failed");
        goto abortSUopenmpiper;
    }
    mprp->vm = vm;
    mprp->off = mprp->len = 0;
    memset (&mprp->disc, 0, sizeof (mprp->disc));
    mprp->disc.readf = mpiperread;
    mprp->disc.writef = mpiperwrite;
    mprp->disc.exceptf = mpiperexcept;
    if(sfdisc (mprp->fp, (Sfdisc_t *) mprp) != (Sfdisc_t *) mprp) {
        SUwarning (1, "SUopenmpiper", "sfdisc failed");
        goto abortSUopenmpiper;
    }
    sfsetbuf (mprp->fp, NULL, mprp->buflen);
    sfsprintf (env, 1000, "SWIFTMPIPE=");
    putenv (env);
    return mprp;

abortSUopenmpiper:
    if (mprp) {
        if (mprp->datap)
            munmap (mprp->datap, mprp->buflen);
        if (mprp->fp)
            sfclose (mprp->fp);
    }
    if (vm)
        vmclose (vm);
    return NULL;
}

static int SUclosempiper (mpiper_t *mprp) {
    munmap (mprp->datap, mprp->buflen);
    vmclose (mprp->vm);
    return 0;
}

static ssize_t mpipewread (
    Sfio_t *fp, void *buf, size_t len, Sfdisc_t *discp
) {
    return -1;
}

static ssize_t mpipewwrite (
    Sfio_t *fp, const void *buf, size_t len, Sfdisc_t *discp
) {
    mpipew_t *mpwp;
    int i, child;

    mpwp = (mpipew_t *) discp;
    if (len > mpwp->buflen)
        len = mpwp->buflen;
    for (i = 0; i < mpwp->n; i++) {
        if (fullread (mpwp->rpipes[i], &child, sizeof (int)) != sizeof (int)) {
            mpwp->n = 0;
            return -1;
        }
    }
    memcpy (mpwp->datap, buf, len);
    for (i = 0; i < mpwp->n; i++) {
        if (fullwrite (mpwp->wpipes[i], &len, sizeof (size_t)) == -1) {
            mpwp->n = 0;
            return -1;
        }
    }
    return len;
}

static int mpipewexcept (
    Sfio_t *fp, int type, void *datap, Sfdisc_t *discp
) {
    if (type == SF_FINAL || type == SF_DPOP)
        return SUclosempipew ((mpipew_t *) discp);
    return 0;
}

static ssize_t mpiperread (
    Sfio_t *fp, void *buf, size_t len, Sfdisc_t *discp
) {
    mpiper_t *mprp;
    int ok, ret;

    ok = 0;
    mprp = (mpiper_t *) discp;
    if (len > mprp->buflen)
        len = mprp->buflen;
    if (mprp->len - mprp->off == 0) {
        if ((ret = fullread (0, &mprp->len, sizeof (size_t))) == -1) {
            SUwarning (1, "mpiperread", "read failed");
            return -1;
        } else if (ret == 0) {
            mprp->len = 0;
        } else if (ret != sizeof (size_t)) {
            SUwarning (1, "mpiperread", "incomplete read");
            return -1;
        }
        mprp->off = 0;
    }
    if (mprp->len == 0)
        return 0;
    if (len > mprp->len - mprp->off)
        len = mprp->len - mprp->off;
    memcpy (buf, mprp->datap + mprp->off, len);
    mprp->off += len;
    if (mprp->off == mprp->len) {
        if (fullwrite (mprp->wpipe, &ok, sizeof (int)) == -1) {
            SUwarning (1, "mpiperread", "read failed");
            return -1;
        }
    }
    return len;
}

static ssize_t mpiperwrite (
    Sfio_t *fp, const void *buf, size_t len, Sfdisc_t *discp
) {
    return -1;
}

static int mpiperexcept (
    Sfio_t *fp, int type, void *datap, Sfdisc_t *discp
) {
    if (type == SF_FINAL || type == SF_DPOP)
        return SUclosempiper ((mpiper_t *) discp);
    return 0;
}

static int fullread (int fd, void *buf, size_t len) {
    size_t off;
    int ret;

    off = 0;
    while ((ret = read (fd, (char *) buf + off, len - off)) > 0)
        if ((off += ret) == len)
            break;
    return off ? off : ret;
}

static int fullwrite (int fd, void *buf, size_t len) {
    size_t off;
    int ret;

    off = 0;
    while ((ret = write (fd, (char *) buf + off, len - off)) > 0)
        if ((off += ret) == len)
            break;
    return off == len ? off : -1;
}
