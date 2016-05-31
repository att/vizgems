#pragma prototyped

#include <ast.h>
#include <cmd.h>
#include <times.h>
#include <option.h>
#include <vmalloc.h>
#include <tok.h>
#include <cdt.h>
#include <swift.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include "key.h"

#define DEFAULT_PORT 40000

static const char usage[] =
"[-1p1s1D?\n@(#)$Id: suidclient (AT&T Labs Research) 2001-09-24 $\n]"
USAGE_LICENSE
"[+NAME?suidclient - client tool for SWIFT data service]"
"[+DESCRIPTION?\bsuidclient\b interfaces with the SWIFT data service to"
" download files from a SWIFT server."
"]"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"\n"
"\n\"localfile1 http://server:port/remotefile1\" ...\n"
"\n"
"[+SEE ALSO?\bsuidserver\b(1)]"
;

typedef struct server_t {
    int inuse;
    char *host;
    int port;
    Sfio_t *fp;
} server_t;

static server_t servers[10000];
static int servern;

static server_t *serverget (char *);
static int serverrelease (server_t *);
static int getrequest (server_t *, char *, char *);

int main (int argc, char **argv) {
    int norun;
    server_t *sp;
    int argi;
    char *files[4], *s;
    int filen;

    norun = 0;
    for (;;) {
        switch (optget (argv, usage)) {
        case -999:
            SUwarnlevel++;
            continue;
        case '?':
            SUusage (0, "suidclient", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "suidclient", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;

    for (argi = 0; argi < argc; argi++) {
        if ((filen = tokscan (
            argv[argi], &s, " %v ", files, 4
        )) < 2 || filen > 3)
            SUerror ("suidclient", "bad argument");
        if (!(sp = serverget (files[1])))
            SUerror ("suidclient", "cannot connect to server");
        if (filen == 2) {
            switch (getrequest (sp, files[0], files[1])) {
            case -1:
                sfprintf (sfstdout, "failed\n");
                break;
            case 1:
                sfprintf (sfstdout, "uptodate\n");
                break;
            default:
                sfprintf (sfstdout, "updated\n");
                break;
            }
        }
        serverrelease (sp);
    }
    return 0;
}

static server_t *serverget (char *namep) {
    char host[PATH_MAX], *portp;
    int port;
    server_t *sp;
    int si;
    struct hostent *hp;
    struct sockaddr_in sin;
    int cfd;

    if (strstr (namep, "http://") != namep || !strchr (namep + 7, '/')) {
        SUwarning (1, "serverget", "bad URL");
        return NULL;
    }
    strncpy (host, namep + 7, PATH_MAX - 1);
    *(strchr (host, '/')) = 0;
    if ((portp = strchr (host, ':')))
        port = atoi (portp + 1), *portp = 0;
    else
        port = DEFAULT_PORT;
    for (si = 0; si < servern; si++) {
        if ((sp = &servers[si])->inuse)
            continue;
        if (strcmp (sp->host, host) == 0 && sp->port == port)
            break;
    }
    if (si == servern) {
        if (servern == 10000) {
            SUwarning (1, "serverget", "too many servers");
            return NULL;
        }
        sp = &servers[servern++];
        sp->host = vmstrdup (Vmheap, host), sp->port = port;
    }
    if (!(hp = gethostbyname (sp->host))) {
        SUwarning (1, "serverget", "cannot find host %s", host);
        return NULL;
    }
    memset ((char *) &sin, 1, sizeof (sin));
    memcpy ((char *) &sin.sin_addr, hp->h_addr, hp->h_length);
    sin.sin_family = hp->h_addrtype;
    sin.sin_port = htons (port);
    if ((cfd = socket (hp->h_addrtype, SOCK_STREAM, 0)) < 0) {
        SUwarning (1, "serverget", "cannot create socket");
        return NULL;
    }
    if (connect (cfd, (struct sockaddr *) &sin, sizeof (sin)) < 0) {
        SUwarning (1, "serverget", "cannot connect");
        return NULL;
    }
    if (SUauth (SU_AUTH_CLIENT, cfd, "SWIFTDSERVER", SWIFTAUTHKEY) == -1) {
        SUwarning (1, "serverget", "authentication failed");
        return NULL;
    }
    if (!(sp->fp = sfnew (NULL, NULL, -1, cfd, SF_READ | SF_WRITE))) {
        SUwarning (1, "serverget", "cannot create sfio channel");
        return NULL;
    }
    sfsetbuf (sp->fp, NULL, 64 * 1024);
    return sp;
}

static int serverrelease (server_t *sp) {
    sp->inuse = 0;
    return 0;
}

static int getrequest (server_t *sp, char *localfile, char *remotefile) {
    struct stat sb;
    int fd;
    struct flock lock;
    char *line, *result, *file, *avs[4];
    long size;
    time_t mtime;
    Sfio_t *fp;
    char dirs[PATH_MAX];
    int dirn;
    char bufs[64 * 1024];
    int bufn;

    result = NULL;
    strcpy (dirs, localfile);
    dirn = strlen (dirs);
    while (dirs[dirn - 1] == '/')
        dirs[--dirn] = 0;
    while (dirn > 0 && dirs[dirn - 1] != '/')
        dirn--;
    if (dirn > 1) {
        dirs[--dirn] = 0;
        avs[0] = "makedir", avs[1] = "-p", avs[2] = dirs, avs[3] = NULL;
        b_mkdir (3, avs, NULL);
    }
doopen:
    errno = 0;
    if ((fd = open (localfile, O_RDWR)) <= 0) {
        if (errno != ENOENT) {
            SUwarning (1, "getrequest", "open failed");
            return -1;
        }
        errno = 0;
        if ((fd = open (localfile, O_RDWR | O_CREAT | O_EXCL, 0666)) == -1) {
            if (errno == EEXIST)
                goto doopen;
            SUwarning (1, "getrequest", "create failed");
            return -1;
        }
    }
    lock.l_type = F_WRLCK;
    lock.l_whence = 0;
    lock.l_start = 0;
    lock.l_len = 0;
    if (fcntl (fd, F_SETLKW, &lock) == -1) {
        SUwarning (1, "getrequest", "lock failed");
        close (fd);
        return -1;
    }
    if (stat (localfile, &sb) == -1) {
        sb.st_size = 0;
        sb.st_mtime = 0;
    }
    remotefile = strchr (remotefile + 7, '/');
    if (
        sfputr (sp->fp, "GET", '\n') == -1 ||
        sfputr (sp->fp, remotefile, '\n') == -1 ||
        sfputr (sp->fp, sfprints ("%d", sb.st_size), '\n') == -1 ||
        sfputr (sp->fp, sfprints ("%d", sb.st_mtime), '\n') == -1 ||
        sfsync (sp->fp) == -1
    ) {
        SUwarning (1, "getrequest", "cannot send request");
        close (fd);
        return -1;
    }
    while ((line = sfgetr (sp->fp, '\n', 1))) {
        if (strncmp (line, "STATUS:", 7) == 0)
            SUwarning (0, "getrequest", "%s", line);
        else {
            result = vmstrdup (Vmheap, line);
            break;
        }
    }
    if (!result ||
        !(line = sfgetr (sp->fp, '\n', 1)) ||
        !(file = vmstrdup (Vmheap, line)) ||
        !(line = sfgetr (sp->fp, '\n', 1)) ||
        (size = strtol (line, NULL, 10)) < 0 ||
        !(line = sfgetr (sp->fp, '\n', 1)) ||
        (mtime = strtol (line, NULL, 10)) < 0
    ) {
        SUwarning (1, "getrequest", "cannot receive response");
        close (fd);
        return -1;
    }
    *file = 0;
    if (strcmp (result, "FAILED") == 0) {
        SUwarning (1, "getrequest", "request failed");
        close (fd);
        return -1;
    }
    if (strcmp (result, "UPTODATE") == 0) {
        SUwarning (1, "getrequest", "file is up to date");
        close (fd);
        return 1;
    }
    if (ftruncate (fd, 0) == -1) {
        SUwarning (1, "getrequest", "truncate failed");
        close (fd);
        return -1;
    }
    if (strcmp (result, "RESULTS") == 0) {
        if (!(fp = sfopen (NULL, localfile, "w"))) {
            SUwarning (1, "getrequest", "cannot open local file");
            return -1;
        }
        sfsetbuf (fp, NULL, 1024 * 1024);
        if (sfmove (sp->fp, fp, size, -1) == -1) {
            sfclose (fp);
            SUwarning (1, "getrequest", "cannot copy data");
            close (fd);
            return -1;
        }
        if (sfclose (fp) == -1) {
            SUwarning (1, "getrequest", "cannot close local file");
            close (fd);
            return -1;
        }
    } else if (strcmp (result, "GZRESULTS") == 0) {
        if (!(fp = sfpopen (
            NULL, sfprints ("gunzip > %s", localfile), "w"
        ))) {
            SUwarning (1, "getrequest", "cannot open local file");
            close (fd);
            return -1;
        }
        sfsetbuf (fp, NULL, 64 * 1024);
        while (((bufn = sfread (sp->fp, bufs, 64 * 1024)) > 0)) {
            if (sfwrite (fp, bufs, bufn) != bufn) {
                SUwarning (1, "getrequest", "cannot copy data");
                close (fd);
                return -1;
            }
        }
        if (sfclose (fp) == -1) {
            SUwarning (1, "getrequest", "cannot close local file");
            close (fd);
            return -1;
        }
    }
    if (touch (localfile, mtime, mtime, 0) == -1) {
        SUwarning (1, "getrequest", "cannot change access time");
        close (fd);
        return -1;
    }
    return 0;
}
