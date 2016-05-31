#pragma prototyped

#include <ast.h>
#include <option.h>
#include <vmalloc.h>
#include <tok.h>
#include <swift.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include "key.h"

#define DEFAULT_PORT 40100

static const char usage[] =
"[-1p1s1D?\n@(#)$Id: suidserver (AT&T Labs Research) 2001-09-24 $\n]"
USAGE_LICENSE
"[+NAME?suidserver - SWIFT data server]"
"[+DESCRIPTION?\bsuidserver\b serves data files to SWIFT UI clients."
"]"
"[100:port?specifies the port to listen on."
" If that port is already in use, the server will keep trying the next higher"
" port until it finds a free port or runs out of ports."
" The default port is \b40100\b."
"]#[port]"
"[101:fg?specifies that the server should not background itself."
"]"
"[102:log?specifies the log file to use."
" The default is to not log anything, unless the \bfg\b option is specified"
" in which case log entries are printed on stderr."
"]:[logfile]"
"[103:ipdir?specifies the filesystem directory that contains a file per IP"
" address that is allowed to connect to this server."
" The default is to allow all ip addresses to connect."
"]:[ipaddress]"
"[200:regname?specifies the host and port of a registry server to connect to"
" to register this instance of the data server."
" The default is to not register."
"]:[host/port]"
"[201:regkey?specifies the key to use to uniquely identify this server on the"
" registry server."
" The default is \bdefault\b."
"]:[key]"
"[300:helper?specifies a helper program."
" The helper program reads the requested filename from stdin and prints the"
" actual filename that the server should use to handle the request."
" The helper acts as a CGI processor that generates files on the fly."
" The default is to not use a helper."
"]:[helperprogram]"
"[301:compress?specifies that the server should use a compressing discipline"
" for the connections to clients."
"]"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"\n"
"[+SEE ALSO?\bsuidclient\b(1), \bsuirserver\b(1)]"
;

static int compressmode;
static int nchildren;

static char *indir;
static unsigned char ownip[4];

static int serverstart (int *);
static int serverstop (int);
static int serve (int, char *);

static int registryopen (char *, char *, char *, int);
static int registryclose (int);

static void sigchldhandler (int);

int main (int argc, char **argv) {
    int norun;
    int port, fgmode;
    char *helper, *regname, *regkey, *logname;
    fd_set infds, outfds;
    int sfd, rfd, cfd, pid;
    struct sockaddr sas;
    socklen_t san;
    char host[MAXHOSTNAMELEN + 1];
    struct hostent *hp;
    char path[PATH_MAX];
    unsigned clientip[4];

    port = DEFAULT_PORT;
    helper = NULL;
    fgmode = FALSE;
    regname = NULL;
    regkey = "default";
    logname = "/dev/null";
    indir = NULL;
    norun = 0;
    for (;;) {
        switch (optget (argv, usage)) {
        case -100:
            port = opt_info.num;
            continue;
        case -101:
            fgmode = TRUE;
            continue;
        case -102:
            logname = opt_info.arg;
            continue;
        case -200:
            regname = opt_info.arg;
            continue;
        case -103:
            indir = opt_info.arg;
            continue;
        case -201:
            regkey = opt_info.arg;
            continue;
        case -300:
            helper = opt_info.arg;
            continue;
        case -301:
            compressmode = TRUE;
            continue;
        case -999:
            SUwarnlevel++;
            continue;
        case '?':
            SUusage (0, "suidserver", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "suidserver", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;

    if (indir) {
        if (gethostname (host, MAXHOSTNAMELEN) == -1) {
            SUwarning (1, "suidserver", "gethostname failed");
            return -1;
        }
        if (!(hp = gethostbyname (host))) {
            SUwarning (1, "suidserver", "cannot find host %s", host);
            return -1;
        }
        ownip[0] = hp->h_addr_list[0][0];
        ownip[1] = hp->h_addr_list[0][1];
        ownip[2] = hp->h_addr_list[0][2];
        ownip[3] = hp->h_addr_list[0][3];
    }
    putenv ("SWMID=");
    putenv ("SWMNAME=");
    putenv ("SWMGROUPS=");
    if ((sfd = serverstart (&port)) < 0)
        SUerror ("suidserver", "cannot create socket");
    FD_ZERO (&infds);
    FD_SET (sfd, &infds);
    if (!fgmode) {
        switch ((pid = fork ())) {
        case -1:
            SUerror ("suidserver", "fork failed");
            break;
        case 0:
            if (
                !sfopen (sfstdin, "/dev/null", "r") ||
                !sfopen (sfstdout, logname, "w") ||
                !sfopen (sfstderr, logname, "w")
            )
                SUwarnlevel = 0;
            break;
        default:
            sfprintf (sfstdout, "pid = %d\n", pid);
            return 0;
        }
    }
    fcntl (sfd, F_SETFD, FD_CLOEXEC);
    signal (SIGCHLD, sigchldhandler);
    if (regname && (rfd = registryopen (regname, regkey, "-", port)) == -1)
        SUerror ("suidserver", "registryopen failed");
    san = sizeof (struct sockaddr);
    for (;;) {
        outfds = infds;
        errno = 0;
        if ((select (FD_SETSIZE, &outfds, NULL, NULL, NULL)) <= 0) {
            if (errno == EINTR)
                continue;
            SUerror ("suidserver", "select failed");
        }
        if (!FD_ISSET (sfd, &outfds))
            break;
doaccept:
        errno = 0;
        if ((cfd = accept (sfd, &sas, &san)) < 0) {
            if (errno == EINTR)
                goto doaccept;
            SUerror ("suidserver", "accept failed");
        }
        clientip[0] = (unsigned char) sas.sa_data[2];
        clientip[1] = (unsigned char) sas.sa_data[3];
        clientip[2] = (unsigned char) sas.sa_data[4];
        clientip[3] = (unsigned char) sas.sa_data[5];
        SUwarning (
            0, "suidserver", "connection request from %u.%u.%u.%u",
            clientip[0], clientip[1], clientip[2], clientip[3]
        );
        if (indir) {
            sfsprintf (
                path, PATH_MAX, "%s/%u.%u.%u.%u", indir,
                clientip[0], clientip[1], clientip[2], clientip[3]
            );
            if (!(
                clientip[0] == ownip[0] && clientip[1] == ownip[1] &&
                clientip[2] == ownip[2] && clientip[3] == ownip[3]
            ) && access (path, R_OK) == -1) {
                SUwarning (
                    0, "suidserver", "ip address not in list of allowed IPs"
                );
                close (cfd);
                continue;
            }
        }
        switch (fork ()) {
        case -1:
            SUerror ("suidserver", "fork failed");
            break;
        case 0:
            signal (SIGCHLD, SIG_DFL);
            setsid ();
            if (SUauth (
                SU_AUTH_SERVER, cfd, "SWIFTDSERVER", SWIFTAUTHKEY
            ) == -1) {
                SUwarning (0, "suidserver", "authentication failed");
                return -1;
            }
            alarm (5 * 60);
            if (serve (cfd, helper) == -1)
                kill (0, SIGTERM);
            return 0;
        default:
            nchildren++;
            close (cfd);
            break;
        }
    }
    if (regname)
        registryclose (rfd);
    serverstop (sfd);
    return 0;
}

static int serverstart (int *portp) {
    int sfd, slen, rtn;
    struct sockaddr_in sin;

    if ((sfd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
        SUwarning (0, "serverstart", "cannot create socket");
        return -1;
    }
    do {
        sin.sin_family = AF_INET;
        sin.sin_port = htons ((*portp)++);
        sin.sin_addr.s_addr = INADDR_ANY;
        slen = sizeof (sin);
    } while (
        (rtn = bind (sfd, (struct sockaddr *) &sin, slen)) == -1 &&
        errno == EADDRINUSE
    );
    if (rtn == -1) {
        SUwarning (0, "serverstart", "cannot bind socket");
        return -1;
    }
    if (listen (sfd, 5) < 0) {
        SUwarning (0, "serverstart", "cannot listen on socket");
        return -1;
    }
    (*portp)--;
    sfprintf (sfstdout, "port = %d\n", *portp);
    sfsync (NULL);
    return sfd;
}

static int serverstop (int sfd) {
    return close (sfd);
}

static int serve (int cfd, char *helper) {
    Sfio_t *fps[1], *hfp, *cfp, *fp;
    char *line, *query, file[PATH_MAX], status[PATH_MAX];
    struct stat sb;
    long size;
    time_t mtime;
    char bufs[64 * 1024];
    int bufn;
    int fd, rtn;
    struct flock lock;

    setsid ();
    if (!(cfp = sfnew (NULL, NULL, -1, cfd, SF_READ | SF_WRITE))) {
        SUwarning (0, "serve", "sfnew failed");
        return -1;
    }
    sfsetbuf (cfp, NULL, 64 * 1024);
    if (!(query = sfgetr (cfp, '\n', 1))) {
        SUwarning (0, "serve", "no request from client");
        return -1;
    }
    if (strcmp (query, "GET") == 0) {
        if (
            !(line = sfgetr (cfp, '\n', 1)) ||
            !strcpy (file, line) ||
            !(line = sfgetr (cfp, '\n', 1)) ||
            (size = strtol (line, NULL, 10)) < 0 ||
            !(line = sfgetr (cfp, '\n', 1)) ||
            (mtime = strtol (line, NULL, 10)) < 0
        ) {
            SUwarning (0, "serve", "cannot receive request");
            return -1;
        }
        SUwarning (0, "serve", "GET %s", file);
        if (helper) {
            if (!(hfp = sfpopen (NULL, helper, "r+"))) {
                SUwarning (0, "serve", "helper startup failed");
                return -1;
            }
            if (sfputr (hfp, file, '\n') == -1 || sfsync (hfp) == -1) {
                SUwarning (0, "serve", "helper does not respond");
                kill (0, 15);
                return -1;
            }
            if (sfset (hfp, SF_READ, 1) < 0) {
                SUwarning (0, "serve", "sfset failed");
                kill (0, 15);
                return -1;
            }
            strcpy (status, "STATUS:?");
            line = NULL;
            for (;;) {
                fps[0] = hfp;
                if ((rtn = sfpoll (fps, 1, 5000)) == -1) {
                    SUwarning (0, "serve", "helper does not respond");
                    kill (0, 15);
                    break;
                }
                if (rtn > 0) {
                    if (!(line = sfgetr (hfp, '\n', 1))) {
                        SUwarning (0, "serve", "helper does not respond");
                        kill (0, 15);
                        break;
                    }
                    if (strncmp (line, "status:", 7) != 0)
                        break;
                    sfsprintf (status, PATH_MAX, "STATUS:%s", &line[7]);
                }
                if (
                    sfputr (cfp, status, '\n') == -1 || sfsync (cfp) == -1
                ) {
                    SUwarning (0, "serve", "cannot send status");
                    kill (0, 15);
                    return -1;
                }
            }
            if (!line) {
                SUwarning (0, "serve", "helper does not respond");
                kill (0, 15);
                return -1;
            }
            strcpy (file, line);
            SUwarning (0, "serve", "helper %s", file);
            if (helper)
                sfclose (hfp);
        }
        if (stat (file, &sb) == -1) {
            SUwarning (0, "serve", "stat failed");
            if (
                sfputr (cfp, "FAILED", '\n') == -1 ||
                sfputr (cfp, file, '\n') == -1 ||
                sfputr (cfp, sfprints ("%d", size), '\n') == -1 ||
                sfputr (cfp, sfprints ("%d", mtime), '\n') == -1 ||
                sfsync (cfp) == -1
            ) {
                SUwarning (0, "serve", "cannot send result (1)");
                return -1;
            }
            return -1;
        }
        if (sb.st_size == size && sb.st_mtime == mtime) {
            SUwarning (0, "serve", "file up to date");
            if (
                sfputr (cfp, "UPTODATE", '\n') == -1 ||
                sfputr (cfp, file, '\n') == -1 ||
                sfputr (cfp, sfprints ("%d", size), '\n') == -1 ||
                sfputr (cfp, sfprints ("%d", mtime), '\n') == -1 ||
                sfsync (cfp) == -1
            ) {
                SUwarning (0, "serve", "cannot send result (2)");
                return -1;
            }
            return -1;
        }
        if ((fd = open (file, O_RDONLY)) == -1) {
            SUwarning (0, "serve", "cannot open file");
            return -1;
        }
        lock.l_type = F_RDLCK;
        lock.l_whence = 0;
        lock.l_start = 0;
        lock.l_len = 0;
        if (fcntl (fd, F_SETLKW, &lock) == -1) {
            SUwarning (0, "serve", "lock failed");
            return -1;
        }
        if (fstat (fd, &sb) == -1) {
            SUwarning (0, "serve", "stat failed");
            if (
                sfputr (cfp, "FAILED", '\n') == -1 ||
                sfputr (cfp, file, '\n') == -1 ||
                sfputr (cfp, sfprints ("%d", size), '\n') == -1 ||
                sfputr (cfp, sfprints ("%d", mtime), '\n') == -1 ||
                sfsync (cfp) == -1
            ) {
                SUwarning (0, "serve", "cannot send result (1a)");
                return -1;
            }
            return -1;
        }
        if (!compressmode || sb.st_size < 100000) {
            if (!(fp = sfopen (NULL, file, "r"))) {
                SUwarning (0, "serve", "open failed");
                return -1;
            }
            sfsetbuf (fp, NULL, 1024 * 1024);
            if (
                sfputr (cfp, "RESULTS", '\n') == -1 ||
                sfputr (cfp, file, '\n') == -1 ||
                sfputr (
                    cfp, sfprints ("%lld", (long long) sb.st_size), '\n'
                ) == -1 ||
                sfputr (cfp, sfprints ("%d", sb.st_mtime), '\n') == -1
            ) {
                SUwarning (0, "serve", "cannot send result (2)");
                return -1;
            }
            if (
                sfmove (fp, cfp, sb.st_size, -1) == -1 ||
                sfsync (cfp) == -1
            ) {
                SUwarning (0, "serve", "copy failed");
                return -1;
            }
            if (sfclose (fp) == -1) {
                SUwarning (0, "serve", "close failed");
                return -1;
            }
        } else {
            if (!(fp = sfpopen (
                NULL, sfprints ("gzip < %s", file), "r"
            ))) {
                SUwarning (0, "serve", "open failed");
                return -1;
            }
            sfsetbuf (fp, NULL, 64 * 1024);
            if (
                sfputr (cfp, "GZRESULTS", '\n') == -1 ||
                sfputr (cfp, file, '\n') == -1 ||
                sfputr (
                    cfp, sfprints ("%lld", (long long) sb.st_size), '\n'
                ) == -1 ||
                sfputr (cfp, sfprints ("%d", sb.st_mtime), '\n') == -1
            ) {
                SUwarning (0, "serve", "cannot send result (2)");
                return -1;
            }
            while ((bufn = sfread (fp, bufs, 64 * 1024)) > 0) {
                if (sfwrite (cfp, bufs, bufn) != bufn) {
                    SUwarning (0, "serve", "cannot copy data");
                    return -1;
                }
            }
            if (bufn == -1 || sfsync (cfp) == -1) {
                SUwarning (0, "serve", "copy failed");
                return -1;
            }
            if (sfclose (fp) == -1) {
                SUwarning (0, "serve", "close failed");
                return -1;
            }
        }
        lock.l_type = F_UNLCK;
        lock.l_whence = 0;
        lock.l_start = 0;
        lock.l_len = 0;
        if (fcntl (fd, F_SETLKW, &lock) == -1)
            SUwarning (0, "serve", "unlock failed");
        close (fd);
        SUwarning (
            0, "serve", "TRANSFERED %lld bytes", (long long) sb.st_size
        );
    }
    sfclose (cfp);
    return 0;
}

static int registryopen (char *regname, char *regkey, char *rname, int rport) {
    char *host, *portp;
    int port;
    struct hostent *hp;
    struct sockaddr_in sin;
    int rfd;
    Sfio_t *rfp;
    char *line;

    host = regname;
    if ((portp = strchr (host, '/')))
        port = atoi (portp + 1), *portp = 0;
    else
        port = DEFAULT_PORT;
    if (!(hp = gethostbyname (host))) {
        SUwarning (0, "registryopen", "cannot find host %s", host);
        return -1;
    }
    memset ((char *) &sin, 1, sizeof (sin));
    memcpy ((char *) &sin.sin_addr, hp->h_addr, hp->h_length);
    sin.sin_family = hp->h_addrtype;
    sin.sin_port = htons (port);
    if ((rfd = socket (hp->h_addrtype, SOCK_STREAM, 0)) < 0) {
        SUwarning (0, "registryopen", "cannot create socket");
        return -1;
    }
    if (connect (rfd, (struct sockaddr *) &sin, sizeof (sin)) < 0) {
        SUwarning (0, "registryopen", "cannot connect");
        return -1;
    }
    if (SUauth (SU_AUTH_CLIENT, rfd, "SWIFTRSERVER", SWIFTAUTHKEY) == -1) {
        SUwarning (0, "registryopen", "authentication failed");
        return -1;
    }
    if (!(rfp = sfnew (NULL, NULL, -1, rfd, SF_READ | SF_WRITE))) {
        SUwarning (0, "registryopen", "sfnew failed");
        return -1;
    }
    sfprintf (rfp, "add %s %d SWIFTDSERVER '%s'\n", rname, rport, regkey);
    if (!(line = sfgetr (rfp, '\n', 1)) || strcmp (line, "SUCCESS") != 0) {
        SUwarning (0, "registryopen", "registration failed");
        return -1;
    }
    return rfd;
}

static int registryclose (int rfd) {
    close (rfd);
    return 0;
}

static void sigchldhandler (int data) {
    while (waitpid (-1, NULL, WNOHANG) > 0)
        nchildren--;
    signal (SIGCHLD, sigchldhandler);
}
