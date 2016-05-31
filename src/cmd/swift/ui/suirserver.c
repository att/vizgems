#pragma prototyped

#include <ast.h>
#include <option.h>
#include <vmalloc.h>
#include <tok.h>
#include <swift.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "key.h"

#define DEFAULT_PORT 40000

static const char usage[] =
"[-1p1s1D?\n@(#)$Id: suirserver (AT&T Labs Research) 2001-09-24 $\n]"
USAGE_LICENSE
"[+NAME?suirserver - SWIFT registry server]"
"[+DESCRIPTION?\bsuirserver\b Implements the SWIFT registry service."
"]"
"[100:port?specifies the port to listen on."
" If that port is already in use, the server will keep trying the next higher"
" port until it finds a free port or runs out of ports."
" The default port is \b40000\b."
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
"[999:v?increases the verbosity level. May be specified multiple times.]"
"\n"
"[+SEE ALSO?\bsuirclient\b(1)]"
;

typedef struct conn_t {
    unsigned char ip[4];
    int fd;
    Sfio_t *fp;
} conn_t;

static conn_t *conns;
static int connn;

typedef struct registry_t {
    int conni;
    char *host, *port, *type, *key;
} registry_t;

static registry_t *registrys;
static int registryn;

static int uniqueid;

static fd_set infds, outfds;

static char *indir;
static unsigned char ownip[4];

static struct sigaction act;

static int serverstart (int);
static int serverstop (int);
static int clientadd (int);
static int clientdelete (int);
static int serve (fd_set);

static int registryadd (int, char **, int);
static int registrydelete (int, char **, int);
static int registrysearch (int, char **, int);
static int registrynewid (int, char **, int);

int main (int argc, char **argv) {
    int norun;
    int port, fgmode, sfd, pid;
    char *logname;
    char host[MAXHOSTNAMELEN + 1];
    struct hostent *hp;

    act.sa_flags = SA_RESTART;
    act.sa_handler = SIG_IGN;
    if (sigaction (SIGPIPE, &act, NULL) == -1) {
        SUwarning (1, "suirserver", "sigaction set failed");
        return -1;
    }
    port = DEFAULT_PORT;
    fgmode = FALSE;
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
        case -103:
            indir = opt_info.arg;
            continue;
        case -999:
            SUwarnlevel++;
            continue;
        case '?':
            SUusage (0, "suirserver", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "suirserver", opt_info.arg);
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
            SUwarning (1, "suirserver", "gethostname failed");
            return -1;
        }
        if (!(hp = gethostbyname (host))) {
            SUwarning (1, "suirserver", "cannot find host %s", host);
            return -1;
        }
        ownip[0] = hp->h_addr_list[0][0];
        ownip[1] = hp->h_addr_list[0][1];
        ownip[2] = hp->h_addr_list[0][2];
        ownip[3] = hp->h_addr_list[0][3];
    }
    if (!(conns = vmalloc (Vmheap, sizeof (conn_t))))
        SUerror ("suirserver", "cannot allocate connection space");
    connn = 1;
    conns[0].fd = -1;
    if (!(registrys = vmalloc (Vmheap, sizeof (registry_t))))
        SUerror ("suirserver", "cannot allocate registry space");
    registryn = 1;
    registrys[0].conni = -1;
    if ((sfd = serverstart (port)) < 0)
        SUerror ("suirserver", "cannot setup server port");
    FD_ZERO (&infds);
    FD_SET (sfd, &infds);
    if (!fgmode) {
        switch ((pid = fork ())) {
        case -1:
            SUerror ("suirserver", "fork failed");
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
    for (;;) {
        outfds = infds;
        if (select (FD_SETSIZE, &outfds, NULL, NULL, NULL) <= 0)
            SUerror ("suirserver", "select failed");
        if (FD_ISSET (sfd, &outfds)) {
            if (clientadd (sfd) == -1)
                SUerror ("suirserver", "cannot add client");
            continue;
        }
        if (serve (outfds) < 0)
            break;
    }
    serverstop (sfd);
    vmfree (Vmheap, registrys);
    vmfree (Vmheap, conns);
    return 0;
}

static int serverstart (int port) {
    int sfd, slen, rtn;
    struct sockaddr_in sin;

    SUwarning (1, "serverstart", "starting server");
    if ((sfd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
        SUwarning (1, "serverstart", "cannot create socket");
        return -1;
    }
    do {
        sin.sin_family = AF_INET;
        sin.sin_port = htons (port++);
        sin.sin_addr.s_addr = INADDR_ANY;
        slen = sizeof (sin);
    } while (
        (rtn = bind (sfd, (struct sockaddr *) &sin, slen)) == -1 &&
        errno == EADDRINUSE
    );
    if (rtn == -1) {
        SUwarning (1, "serverstart", "cannot bind socket");
        return -1;
    }
    if (listen (sfd, 5) < 0) {
        SUwarning (1, "serverstart", "cannot listen on socket");
        return -1;
    }
    sfprintf (sfstdout, "port = %d\n", port - 1);
    sfsync (NULL);
    SUwarning (1, "serverstart", "server started on port %d", port - 1);
    return sfd;
}

static int serverstop (int sfd) {
    SUwarning (1, "serverstop", "stopping server");
    return close (sfd);
}

static int clientadd (int sfd) {
    int cfd;
    conn_t *connp;
    int conni;
    struct sockaddr sas;
    socklen_t san;
    char path[PATH_MAX];
    unsigned clientip[4];

    SUwarning (1, "clientadd", "adding client");
    san = sizeof (struct sockaddr);
    if ((cfd = accept (sfd, &sas, &san)) < 0) {
        SUwarning (0, "clientadd", "accept failed");
        return -1;
    }
    clientip[0] = (unsigned char) sas.sa_data[2];
    clientip[1] = (unsigned char) sas.sa_data[3];
    clientip[2] = (unsigned char) sas.sa_data[4];
    clientip[3] = (unsigned char) sas.sa_data[5];
    SUwarning (
        0, "clientadd", "connection request from %u.%u.%u.%u",
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
            SUwarning (0, "clientadd", "ip address not in list of allowed IPs");
            close (cfd);
            return -2;
        }
    }
    if (SUauth (SU_AUTH_SERVER, cfd, "SWIFTRSERVER", SWIFTAUTHKEY) == -1) {
        SUwarning (0, "clientadd", "authentication failed");
        close (cfd);
        return -2;
    }
    conni = cfd;
    if (conni >= connn) {
        if (!(conns = vmresize (
            Vmheap, conns, (conni + 1) * sizeof (conn_t), VM_RSCOPY
        ))) {
            SUwarning (0, "clientadd", "resize failed for map");
            return -1;
        }
        for (; connn < conni + 1; connn++)
            conns[connn].fd = -1;
    }
    connp = &conns[conni];
    connp->ip[0] = sas.sa_data[2];
    connp->ip[1] = sas.sa_data[3];
    connp->ip[2] = sas.sa_data[4];
    connp->ip[3] = sas.sa_data[5];
    connp->fd = cfd;
    if (!(connp->fp = sfnew (NULL, NULL, -1, cfd, SF_READ | SF_WRITE))) {
        SUwarning (0, "clientadd", "sfnew failed");
        return -1;
    }
    FD_SET (cfd, &infds);
    SUwarning (0, "clientadd", "client added on fd %d", cfd);
    return cfd;
}

static int clientdelete (int cfd) {
    conn_t *connp;
    int conni;

    static char *null[6] = { "", "", "", "", "", "" };

    SUwarning (1, "clientadd", "deleting client on fd %d", cfd);
    if ((conni = cfd) >= connn || conns[cfd].fd == -1) {
        SUwarning (0, "clientdelete", "bad file descriptor");
        return -1;
    }
    connp = &conns[conni];
    sfclose (connp->fp);
    connp->fd = -1;
    FD_CLR (cfd, &infds);
    registrydelete (cfd, null, 5);
    return 0;
}

static int serve (fd_set fds) {
    conn_t *connp;
    int conni;
    int breakout;
    char *line, *as, *s, *avs[10000];
    int avn;

    for (conni = 0; conni < connn; conni++) {
        connp = &conns[conni];
        if (connp->fd == -1 || !FD_ISSET (connp->fd, &fds))
            continue;
        breakout = FALSE;
        while ((line = sfgetr (connp->fp, '\n', 1))) {
            if (line[strlen (line) - 1] == '\r')
                line[strlen (line) - 1] = 0;
            if (connp->fp->_next >= connp->fp->_endb)
                breakout = TRUE;
            SUwarning (2, "serve", "client: %d line: %s", connp->fd, line);
            as = vmstrdup (Vmheap, line);
            if ((avn = tokscan (as, &s, " %v ", avs, 10000)) < 1) {
                clientdelete (connp->fd);
            } else if (strcmp (avs[0], "add") == 0) {
                if (registryadd (conni, avs, avn) == -1)
                    clientdelete (connp->fd);
            } else if (strcmp (avs[0], "delete") == 0) {
                if (registrydelete (conni, avs, avn) == -1)
                    clientdelete (connp->fd);
            } else if (strcmp (avs[0], "search") == 0) {
                if (registrysearch (conni, avs, avn) == -1)
                    clientdelete (connp->fd);
            } else if (strcmp (avs[0], "newid") == 0) {
                if (registrynewid (conni, avs, avn) == -1)
                    clientdelete (connp->fd);
            }
            vmfree (Vmheap, as);
            if (breakout)
                break;
        }
        if (connp->fd != -1 && sfeof (connp->fp))
            clientdelete (connp->fd);
    }
    if (sfsync (NULL) == -1) {
        SUwarning (1, "serve", "sfsync failed");
        return -1;
    }
    return 0;
}

static int registryadd (int conni, char **avs, int avn) {
    char name[1000];
    conn_t *connp;
    registry_t *registryp;
    int registryi;

    SUwarning (1, "registryadd", "adding service");
    connp = &conns[conni];
    if (avn < 5) {
        sfprintf (connp->fp, "FAILURE\n");
        SUwarning (1, "registryadd", "syntax error");
        return 0;
    }
    if (avs[1][0] == '-' && avs[1][1] == 0)
        sfsprintf (
            name, 1000, "%d.%d.%d.%d",
            connp->ip[0], connp->ip[1], connp->ip[2], connp->ip[3]
        );
    else
        strcpy (name, avs[1]);
    for (registryi = 0; registryi < registryn; registryi++) {
        registryp = &registrys[registryi];
        if (registryp->conni == -1)
            continue;
        if (strcmp (registryp->host, name) != 0)
            continue;
        if (strcmp (registryp->port, avs[2]) != 0)
            continue;
        if (strcmp (registryp->type, avs[3]) != 0)
            continue;
        if (strcmp (registryp->key, avs[4]) != 0)
            continue;
        sfprintf (connp->fp, "FAILURE\n");
        return 0;
    }
    for (registryi = 0; registryi < registryn; registryi++) {
        registryp = &registrys[registryi];
        if (registryp->conni == -1)
            continue;
    }
    if (registryi == registryn) {
        if (!(registrys = vmresize (
            Vmheap, registrys, (registryn + 1) * sizeof (registry_t), VM_RSCOPY
        ))) {
            SUwarning (0, "registryadd", "resize failed for registry");
            return -1;
        }
        registrys[registryn++].conni = -1;
    }
    registryp = &registrys[registryi];
    registryp->conni = conni;
    registryp->host = vmstrdup (Vmheap, name);
    registryp->port = vmstrdup (Vmheap, avs[2]);
    registryp->type = vmstrdup (Vmheap, avs[3]);
    registryp->key = vmstrdup (Vmheap, avs[4]);
    sfprintf (connp->fp, "SUCCESS\n");
    SUwarning (1, "registryadd", "service added for conn %d", conni);
    return 0;
}

static int registrydelete (int conni, char **avs, int avn) {
    char name[1000];
    conn_t *connp;
    registry_t *registryp;
    int registryi;

    SUwarning (1, "registrydelete", "deleting service");
    connp = &conns[conni];
    if (avn < 5) {
        sfprintf (connp->fp, "FAILURE\n");
        SUwarning (1, "registrydelete", "syntax error");
        return 0;
    }
    if (avs[1][0] == '-' && avs[1][1] == 0)
        sfsprintf (
            name, 1000, "%d.%d.%d.%d",
            connp->ip[0], connp->ip[1], connp->ip[2], connp->ip[3]
        );
    else
        strcpy (name, avs[1]);
    for (registryi = 0; registryi < registryn; registryi++) {
        registryp = &registrys[registryi];
        if (registryp->conni == -1)
            continue;
        if (conni != registryp->conni)
            continue;
        if (avs[1][0] && strcmp (registryp->host, name) != 0)
            continue;
        if (avs[2][0] && strcmp (registryp->port, avs[2]) != 0)
            continue;
        if (avs[3][0] && strcmp (registryp->type, avs[3]) != 0)
            continue;
        if (avs[4][0] && strcmp (registryp->key, avs[4]) != 0)
            continue;
        vmfree (Vmheap, registryp->host);
        vmfree (Vmheap, registryp->port);
        vmfree (Vmheap, registryp->type);
        vmfree (Vmheap, registryp->key);
        registryp->conni = -1;
    }
    sfprintf (connp->fp, "SUCCESS\n");
    SUwarning (1, "registrydelete", "registry deleted for conn %d", conni);
    return 0;
}

static int registrysearch (int conni, char **avs, int avn) {
    registry_t *registryp;
    int registryi;

    SUwarning (1, "registrysearch", "searching service");
    if (avn < 5) {
        sfprintf (conns[conni].fp, "FAILURE\n");
        SUwarning (1, "registrysearch", "syntax error");
        return 0;
    }
    for (registryi = 0; registryi < registryn; registryi++) {
        registryp = &registrys[registryi];
        if (registryp->conni == -1)
            continue;
        if (avs[1][0] && strcmp (registryp->host, avs[1]) != 0)
            continue;
        if (avs[2][0] && strcmp (registryp->port, avs[2]) != 0)
            continue;
        if (avs[3][0] && strcmp (registryp->type, avs[3]) != 0)
            continue;
        if (avs[4][0] && strcmp (registryp->key, avs[4]) != 0)
            continue;
        sfprintf (
            conns[conni].fp, "%s %s %s %s\n",
            registryp->host, registryp->port, registryp->type, registryp->key
        );
    }
    sfprintf (conns[conni].fp, "SUCCESS\n");
    SUwarning (1, "registrysearch", "registry searched for conn %d", conni);
    return 0;
}

static int registrynewid (int conni, char **avs, int avn) {
    SUwarning (1, "registrynewid", "generating unique id");
    sfprintf (conns[conni].fp, "%d\n", uniqueid++);
    sfprintf (conns[conni].fp, "SUCCESS\n");
    SUwarning (1, "registrynewid", "newid for conn %d", conni);
    return 0;
}
