#include <ast.h>
#include <cdt.h>
#include <dt.h>
#include <swift.h>
#include "ndfsserver.h"

static int pathensure (char *);
static int filecopy (char *, const char *);
static int dircmp (const void *, const void *);

static Dtdisc_t didisc = {
    sizeof (Dtlink_t), -1, 0, NULL, NULL, NULL, NULL, NULL, NULL
};

static Dtdisc_t piddisc = {
    sizeof (Dtlink_t), sizeof (pid_t), 0, NULL, NULL, NULL, NULL, NULL, NULL
};
static Dt_t *piddict;

static char *specialpaths[] = {
    "",        // NDFS_SPECIALPATH_NOT
    ";nddel;", // NDFS_SPECIALPATH_DEL
    ";ndtmp;", // NDFS_SPECIALPATH_TMP
    ";ndtlo;", // NDFS_SPECIALPATH_TLO
    ";ndopq;", // NDFS_SPECIALPATH_OPQ
};

int utilinit (void) {
    if (!(piddict = dtnew (vm, &piddisc, Dtset))) {
        SUwarning (0, "utilinit", "cannot open piddict");
        return -1;
    }
    return 0;
}

int utilterm (void) {
    ndfspid_t *pidp;

    while ((pidp = dtfirst (piddict))) {
        dtdelete (piddict, pidp);
        vmfree (vm, pidp);
    }
    dtclose (piddict);
    return 0;
}

// returns 0 if the top level file is there
// returns 1 if the bottom file is there
// else returns -1

int utilgetrealfile (const char *ipath, char *opath) {
    struct stat st;

    if (lstat (ipath, &st) != -1) {
        memccpy (opath, ipath, 0, PATH_MAX);
        return 0;
    }
    memcpy (opath, bprefixstr, bprefixlen + 1);
    memccpy (opath + bprefixlen, ipath, 0, PATH_MAX - bprefixlen);
    if (lstat (opath, &st) == -1)
        return -1;
    return 1;
}

int utilgetbottomfile (const char *ipath, char *opath) {
    struct stat st;

    memcpy (opath, bprefixstr, bprefixlen + 1);
    memccpy (opath + bprefixlen, ipath, 0, PATH_MAX - bprefixlen);
    if (lstat (opath, &st) == -1)
        return -1;
    return 0;
}

int utilensuretopfile (const char *path) {
    char p1[PATH_MAX];

    strcpy (p1, path);
    if (pathensure (p1) == -1)
        return -1;

    return 0;
}

int utilensuretopfileparent (const char *path) {
    char p1[PATH_MAX], *s;

    strcpy (p1, path);
    if ((s = strrchr (p1, '/'))) {
        *s = 0;
        if (pathensure (p1) == -1)
            return -1;
    }

    return 0;
}

int utilinitdir (ndfsfile_t *fp, char *tpath, char *bpath) {
    Dt_t *didict;
    ndfsdirinfo_t *dip;
    int dipi;
    DIR *dp;
    struct dirent *dep;
    int estate, i, pt;
    char *pp, *name, *rname;

    if (!(didict = dtnew (vm, &didisc, Dtset))) {
        SUwarning (0, "utilinitdir", "cannot open didict");
        return -1;
    }
    estate = 0;
    for (i = 0; i < 2; i++) {
        if (i == 0)
            pp = tpath;
        else
            pp = bpath;
        if (!pp)
            continue;
        if (!(dp = opendir (pp))) {
            SUwarning (2, "utilinitdir", "cannot opendir %s", pp);
        } else {
            for (;;) {
                if (!(dep = readdir (dp)))
                    break;
                name = dep->d_name;
                pt = NDFS_SPECIALPATH_NOT;
                if (utilisspecialname (name, &pt, &rname)) {
                    if (pt == NDFS_SPECIALPATH_DEL)
                        name = rname;
                    else
                        continue;
                }
                if ((dip = dtmatch (didict, name))) {
                    if (pt == NDFS_SPECIALPATH_DEL)
                        dip->opqflag = TRUE;
                    else if (i == 0 && !dip->opqflag) {
                        SUwarning (0, "utilinitdir", "di exists %s", name);
                        estate = 1;
                        dip = NULL;
                        goto cleanup;
                    }
                } else {
                    if (!(dip = vmalloc (vm, sizeof (ndfsdirinfo_t)))) {
                        SUwarning (
                            0, "utilinitdir", "cannot allocate di %s", name
                        );
                        estate = 2;
                        goto cleanup;
                    }
                    memset (dip, 0, sizeof (ndfsdirinfo_t));
                    if (!(dip->name = vmstrdup (vm, name))) {
                        SUwarning (
                            0, "utilinitdir", "cannot copy name %s", name
                        );
                        estate = 3;
                        goto cleanup;
                    }
                    dip->ino = dep->d_ino;
                    dip->type = dep->d_type;
                    if (pt == NDFS_SPECIALPATH_DEL)
                        dip->opqflag = TRUE;
                    if (dtinsert (didict, dip) != dip) {
                        SUwarning (
                            0, "utilinitdir", "cannot insert di %s", name
                        );
                        estate = 4;
                        goto cleanup;
                    }
                }
            }
            closedir (dp);
        }
    }
    fp->u.d.dirinfopn = dtsize (didict);
    if (!(fp->u.d.dirinfops = vmalloc (
        vm, fp->u.d.dirinfopn * sizeof (ndfsdirinfo_t *)
    ))) {
        SUwarning (
            0, "utilinitdir", "cannot allocate %d dis", fp->u.d.dirinfopn
        );
        estate = 5;
        goto cleanup;
    }
    for (dipi = 0, dip = dtfirst (didict); dip; dip = dtnext (didict, dip)) {
        if (dip->opqflag)
            continue;
        fp->u.d.dirinfops[dipi++] = dip;
    }
    fp->u.d.dirinfopn = dipi;
    qsort (
        fp->u.d.dirinfops, fp->u.d.dirinfopn, sizeof (ndfsdirinfo_t *), dircmp
    );
    for (dipi = 0; dipi < fp->u.d.dirinfopn; dipi++)
        fp->u.d.dirinfops[dipi]->off = dipi + 1;
    fp->u.d.inited = TRUE;

cleanup:
    if (estate > 0) {
        if (dip) {
            if (dip->name)
                vmfree (vm, dip->name);
            vmfree (vm, dip);
        }
        if (dp)
            closedir (dp);
    }
    while ((dip = dtfirst (didict))) {
        dtdelete (didict, dip);
        if (dip->opqflag) {
            vmfree (vm, dip->name);
            vmfree (vm, dip);
        }
    }
    dtclose (didict);
    return 0;
}

int utiltermdir (ndfsfile_t *fp) {
    int dipi;

    for (dipi = 0; dipi < fp->u.d.dirinfopn; dipi++) {
        vmfree (vm, fp->u.d.dirinfops[dipi]->name);
        vmfree (vm, fp->u.d.dirinfops[dipi]);
    }
    vmfree (vm, fp->u.d.dirinfops), fp->u.d.dirinfops = NULL;
    return 0;
}

int utilmkspecialpath (const char *ipath, int type, char *opath) {
    char *pp, *s;
    int l;

    if (type <= 0 || type >= NDFS_SPECIALPATHSIZE) {
        SUwarning (0, "utilmkspecialpath", "unknown special type %d", type);
        return -1;
    }
    if ((l = strlen (ipath)) + 7 > PATH_MAX) {
        SUwarning (0, "utilmkspecialpath", "special path too long %s", ipath);
        return -1;
    }
    if ((s = strrchr (ipath, '/'))) {
        memcpy (opath, ipath, (s + 1 - ipath));
        pp = opath + (s + 1 - ipath);
        memcpy (pp, specialpaths[type], 7), pp += 7;
        memccpy (pp, s + 1, 0, PATH_MAX - (pp - opath));
    } else {
        memcpy (opath, specialpaths[type], 7), pp = opath + 7;
        memccpy (pp, ipath, 0, PATH_MAX - (pp - opath));
    }
    return 0;
}

int utilisspecialpath (const char *path, int *typep) {
    char *pp;
    int pt;

    if ((pp = strstr (path, ";nd"))) {
        if (pp != path && pp[-1] != '/')
            return FALSE;
        for (pt = 1; pt < NDFS_SPECIALPATHSIZE; pt++) {
            if (strncmp (specialpaths[pt] + 3, pp + 3, 4) == 0) {
                if (typep)
                    *typep = pt;
                return TRUE;
            }
        }
    }
    return FALSE;
}

int utilisspecialname (const char *name, int *typep, char **rnamep) {
    int pt;

    if (name[0] == ';' && name[1] == 'h' && name[2] == 'd') {
        for (pt = 1; pt < NDFS_SPECIALPATHSIZE; pt++) {
            if (strncmp (specialpaths[pt] + 3, name + 3, 4) == 0) {
                if (typep)
                    *typep = pt;
                if (rnamep)
                    *rnamep = (char *) name + 7;
                return TRUE;
            }
        }
    }
    return FALSE;
}

int utilmkspecialpid (pid_t pid, int insflags, int remflags) {
    ndfspid_t *pidp;

    insflags &= (NDFS_ALLFLAGS);
    remflags &= (NDFS_ALLFLAGS);
    if (insflags == 0 && remflags == 0) {
        SUwarning (0, "utilmkspecialpid", "no valid flags specified");
        return -1;
    }

    if (!(pidp = dtmatch (piddict, &pid))) {
        if (!(pidp = vmalloc (vm, sizeof (ndfspid_t)))) {
            SUwarning (
                0, "utilmkspecialpid", "cannot allocate pid %d", pid
            );
            return -1;
        }
        memset (pidp, 0, sizeof (ndfspid_t));
        pidp->pid = pid;
        if (dtinsert (piddict, pidp) != pidp) {
            SUwarning (
                0, "utilmkspecialpid", "cannot insert pid %d", pid
            );
            vmfree (vm, pidp);
            return -1;
        }
    }

    if (remflags)
        pidp->flags &= ~remflags;
    if (insflags)
        pidp->flags |= insflags;
    if (pidp->flags == 0)
        return utilrmspecialpid (pid);
    return 0;
}

int utilrmspecialpid (pid_t pid) {
    ndfspid_t *pidp;

    if (!(pidp = dtmatch (piddict, &pid))) {
        dtdelete (piddict, pidp);
        vmfree (vm, pidp);
    }

    return 0;
}

int utilisspecialpid (pid_t pid, int *flagsp) {
    ndfspid_t *pidp;

    if (!(pidp = dtmatch (piddict, &pid))) {
        if (flagsp)
            *flagsp = 0;
        return FALSE;
    }

    if (flagsp)
        *flagsp = pidp->flags;
    return TRUE;
}

static int pathensure (char *path) {
    char p1[PATH_MAX], p2[PATH_MAX], *pp;
    struct stat st;
    char *s, c;

    if (lstat (path, &st) != -1)
        return 0;
    if (errno != ENOENT)
        return -1;

    memcpy (p1, bprefixstr, bprefixlen + 1);

    s = path;
    for (;;) {
        if ((s = strchr (s, '/')))
            c = *s, *s = 0;
        memccpy (p1 + bprefixlen, path, 0, PATH_MAX - bprefixlen);
        if (lstat (p1, &st) == -1) {
            SUwarning (0, "pathensure", "cannot lstat bottom path %s", p1);
            return -1;
        }
        if (S_ISLNK (st.st_mode)) {
            if (readlink (p1, p2, PATH_MAX) == -1) {
                SUwarning (
                    0, "pathensure", "cannot readlink bottom path %s", p1
                );
                return -1;
            }
            pp = p2;
            if (strncmp (p2, bprefixstr, bprefixlen - 1) == 0) {
                if (*(pp = p2 + bprefixlen - 1) == '/')
                    pp++;
            }
            if (symlink (pp, path) == -1 && errno != EEXIST) {
                SUwarning (0, "pathensure", "cannot symlink %s - %s", pp, path);
                return -1;
            }
        } else if (S_ISDIR (st.st_mode)) {
            if (mkdir (path, st.st_mode) == -1 && errno != EEXIST)
                return -1;
        } else if (S_ISREG (st.st_mode)) {
            LOCK (mutex);
            if (filecopy (p1, path) == -1) {
                UNLOCK (mutex);
                return -1;
            }
            UNLOCK (mutex);
        } else {
            SUwarning (0, "pathensure", "unknown file type %d", st.st_mode);
            return -1;
        }
        if (s)
            *s++ = c;
        else
            break;
    }

    if (lstat (path, &st) == -1) {
        SUwarning (0, "pathensure", "cannot lstat top path %s", path);
        return -1;
    }

    return 0;
}

static int filecopy (char *ipath, const char *opath) {
    char p1[PATH_MAX];
    Sfio_t *ifp, *ofp;
    struct stat st1;

    if (stat (ipath, &st1) == -1) {
        SUwarning (0, "filecopy", "cannot stat bottom path %s", ipath);
        return -1;
    }
    if (!(ifp = sfopen (NULL, ipath, "r"))) {
        SUwarning (0, "filecopy", "cannot open bottom path %s", ipath);
        return -1;
    }
    if (utilmkspecialpath (opath, NDFS_SPECIALPATH_TMP, p1) == -1) {
        SUwarning (0, "filecopy", "cannot make special path %s", opath);
        return -1;
    }
    if (!(ofp = sfopen (NULL, p1, "wx"))) {
        if (errno == EEXIST) {
            sfclose (ifp);
            return 0;
        }
        SUwarning (0, "filecopy", "cannot open top path %s", opath);
        return -1;
    }
    if (sfmove (ifp, ofp, -1, -1) == -1) {
        SUwarning (0, "filecopy", "cannot copy data %s to %s", ipath, opath);
        return -1;
    }
    sfclose (ofp);
    sfclose (ifp);
    if (chmod (p1, st1.st_mode) == -1) {
        SUwarning (0, "filecopy", "cannot chmod path %s", p1);
        return -1;
    }
    if (link (p1, opath) == -1 && errno != EEXIST) {
        SUwarning (0, "filecopy", "cannot link path %s to %s", p1, opath);
        unlink (p1);
        return -1;
    }
    unlink (p1);
    return 0;
}

static int dircmp (const void *a, const void *b) {
    ndfsdirinfo_t *ap, *bp;
    int aindex, bindex;
    long long lld;

    ap = *(ndfsdirinfo_t **) a;
    bp = *(ndfsdirinfo_t **) b;
    if (ap->name[0] == '.' && ap->name[1] == 0)
        aindex = 1;
    else if (ap->name[0] == '.' && ap->name[1] == '.' && ap->name[2] == 0)
        aindex = 2;
    else
        aindex = 3;
    if (bp->name[0] == '.' && bp->name[1] == 0)
        bindex = 1;
    else if (bp->name[0] == '.' && bp->name[1] == '.' && bp->name[2] == 0)
        bindex = 2;
    else
        bindex = 3;
    if (aindex != bindex)
        return aindex = bindex;
    lld = (ino_t) ap->ino - (ino_t) bp->ino;
    return (lld < 0) ? -1 : 1;
}

// logging routines

static int TSUinited = FALSE;

void TSUerror (char *func, char *fmt, ...) {
    va_list args;
    char buf[10240], *cur, *end, *s;

    if (!TSUinited) {
        if ((s = getenv ("SWIFTWARNLEVEL")))
            SUwarnlevel = atoi (s);
        TSUinited = TRUE;
    }
    cur = buf;
    end = &buf[sizeof (buf)];
    bprintf (
        &cur, end, "SWIFT ERROR: (%s %d %llx) ",
        func, getpid (), pthread_self ()
    );
    va_start(args, fmt);
    bvprintf (&cur, end, fmt, args);
    va_end(args);
    bprintf (&cur, end, "\n");
    write (2, buf, cur - buf);
}

void TSUwarning (int level, char *func, char *fmt, ...) {
    va_list args;
    char buf[10240], *cur, *end, *s;

    if (!TSUinited) {
        if ((s = getenv ("SWIFTWARNLEVEL")))
            SUwarnlevel = atoi (s);
        TSUinited = TRUE;
    }
    if (level > SUwarnlevel)
        return;
    cur = buf;
    end = &buf[sizeof (buf)];
    bprintf (
        &cur, end, "SWIFT WARNING: (%s %d %llx) ",
        func, getpid (), pthread_self ()
    );
    va_start(args, fmt);
    bvprintf (&cur, end, fmt, args);
    va_end(args);
    bprintf (&cur, end, "\n");
    write (2, buf, cur - buf);
}

/* non-mallocing printf, from UWIN, modified by gsf */

#define VASIGNED(a, n) (\
((n)>1)?va_arg(a,int64_t):((n)?va_arg(a,long):va_arg(a,int)) \
)
#define VAUNSIGNED(a, n) ( \
((n)>1)?\
va_arg(a,uint64_t):((n)?va_arg(a,unsigned long):va_arg(a,unsigned int)) \
)

ssize_t bvprintf (char **buf, char *end, const char *format, va_list ap) {
    int c;
    char *p;
    char *e;
    int64_t n;
    uint64_t u;
    ssize_t z;
    int w;
    int x;
    int l;
    int f;
    int g;
    int r;
    char *s;
    char *b;
    char *t;
    void *v;
    char num[32];
    char typ[32];

    static char digits[] = \
    "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLNMOPQRSTUVWXYZ@_";

    p = *buf;
    e = end;
    for (;;) {
        switch ((c = *format++)) {
        case 0:
            goto done;
        case '%':
            if (*format == '-') {
                format++;
                l = 1;
            } else
                l = 0;
            if (*format == '0') {
                format++;
                f = l ? ' ' : '0';
            } else
                f = ' ';
            if ((c = *format) == '*') {
                format++;
                w = va_arg(ap, int);
            } else {
                w = 0;
                while (c >= '0' && c <= '9') {
                    w = w * 10 + c - '0';
                    c = *++format;
                }
            }
            x = r = 0;
            if (c == '.') {
                if ((c = *++format) == '*') {
                    format++;
                    x = va_arg(ap, int);
                } else
                    while (c >= '0' && c <= '9') {
                        x = x * 10 + c - '0';
                        c = *++format;
                    }
                if (c == '.') {
                    if ((c = *++format) == '*') {
                        format++;
                        r = va_arg(ap, int);
                    } else
                        while (c >= '0' && c <= '9') {
                            r = r * 10 + c - '0';
                            c = *++format;
                        }
                }
            }
            if (c == '(') {
                t = typ;
                while ((c = *++format)) {
                    if ((c == ')')) {
                        format++;
                        break;
                    }
                    if (t < &typ[sizeof (typ) - 1])
                        *t++ = c;
                }
                *t = 0;
                t = typ;
            } else
                t = 0;
            n = 0;
            g = 0;
            b = num;
            for (;;) {
                switch (c = *format++) {
                case 'l':
                    if (n < 2)
                        n++;
                    continue;
                case 'L':
                    n = 2;
                    continue;
                }
                break;
            }
            switch (c) {
            case 0:
                break;
            case 'c':
                *b++ = va_arg(ap, int);
                break;
            case 'd':
                n = VASIGNED(ap, n);
                if (n < 0) {
                    g = '-';
                    n = -n;
                }
                u = n;
                goto dec;
            case 'o':
                u = VAUNSIGNED(ap, n);
                do *b++ = (char)(u & 07) + '0'; while (u >>= 3);
                break;
            case 's':
                if (!t) {
                    s = va_arg(ap, char*);
                    if (!s)
                        s = "(null)";
#if I18N
                    if (n) {
                        y = (wchar_t *) s;
                        r = (int) wcslen (y);
                        if (l && x && r > x)
                            r = x;
                        s = fmtbuf (4 * r + 1);
                        if (!WideCharToMultiByte (
                            CP_ACP, 0, y, r, s, 4 * r + 1, NULL, NULL
                        ))
                            s = "(BadUnicode)";
                    }
#endif
                } else
                    s = t;
                if (w || (l && x)) {
                    if (!l || !x)
                        x = (int) strlen (s);
                    if (!w)
                        w = x;
                    n = w - x;
                    if (l) {
                        while (w-- > 0) {
                            if (p >= e)
                                goto done;
                            if (!(*p = *s++))
                                break;
                            p++;
                        }
                        while (n-- > 0) {
                            if (p >= e)
                                goto done;
                            *p++ = f;
                        }
                        continue;
                    }
                    while (n-- > 0) {
                        if (p >= e)
                            goto done;
                        *p++ = f;
                    }
                }
                for (;;) {
                    if (p >= e)
                        goto done;
                    if (!(*p = *s++))
                        break;
                    p++;
                }
                continue;
            case 'u':
                u = VAUNSIGNED(ap, n);
            dec:
                if (r <= 0 || r >= sizeof (digits))
                    r = 10;
                do *b++ = digits[u % r]; while (u /= r);
                break;
            case 'p':
                v = va_arg(ap, void*);
                if ((u = (uint64_t)((char*)v - (char*)0))) {
                    if (sizeof (v) == 4)
                        u &= 0xffffffff;
                    g = 'x';
                    w = 10;
                    f = '0';
                    l = 0;
                }
                goto hex;
            case 'x':
                u = VAUNSIGNED(ap, n);
            hex:
                do {
                    *b++ = (
                        (c = (char)(u & 0xf)) >= 0xa
                    ) ? c - 0xa + 'a' : c + '0';
                } while (u >>= 4);
                break;
            case 'z':
                u = VAUNSIGNED(ap, n);
                do {
                    *b++ = (
                        (c = (char)(u & 0x1f)) >= 0xa
                    ) ? c - 0xa + 'a' : c + '0';
                } while (u >>= 5);
                break;
            default:
                if (p >= e)
                    goto done;
                *p++ = c;
                continue;
            }
            if (w) {
                if (g == 'x')
                    w -= 2;
                else
                    if (g) w -= 1;
                n = w - (b - num);
                if (!l) {
                    if (g && f != ' ') {
                        if (g == 'x') {
                            if (p >= e)
                                goto done;
                            *p++ = '0';
                            if (p >= e)
                                goto done;
                            *p++ = 'x';
                        } else if (p >= e)
                            goto done;
                        else
                            *p++ = g;
                        g = 0;
                    }
                    while (n-- > 0) {
                        if (p >= e)
                            goto done;
                        *p++ = f;
                    }
                }
            }
            if (g == 'x') {
                if (p >= e)
                    goto done;
                *p++ = '0';
                if (p >= e)
                    goto done;
                *p++ = 'x';
            } else if (g) {
                if (p >= e)
                    goto done;
                *p++ = g;
            }
            while (b > num) {
                if (p >= e)
                    goto done;
                *p++ = *--b;
            }
            if (w && l)
                while (n-- > 0) {
                    if (p >= e)
                        goto done;
                    *p++ = f;
                }
            continue;
        default:
            if (p >= e)
                goto done;
            *p++ = c;
            continue;
        }
        break;
    }
 done:
    if (p < e)
        *p = 0;
    z = (ssize_t)(p - *buf);
    *buf = p;
    return z;
}

ssize_t bprintf (char **buf, char *end, const char *format, ...) {
    va_list ap;
    ssize_t n;

    va_start(ap, format);
    n = bvprintf (buf, end, format, ap);
    va_end(ap);
    return n;
}

ssize_t bprintf1 (char *buf, ssize_t len, const char *format, ...) {
    va_list ap;
    ssize_t n;

    va_start(ap, format);
    n = bvprintf (&buf, buf + len, format, ap);
    va_end(ap);
    return n;
}
