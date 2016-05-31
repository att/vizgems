#pragma prototyped

#include <ast.h>
#include <option.h>
#include <signal.h>
#include <errno.h>
#include <vmalloc.h>
#include <tok.h>
#include <swift.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "key.h"

#define DEFAULT_PORT 40200

static const char usage[] =
"[-1p1s1D?\n@(#)$Id: suimserver (AT&T Labs Research) 2001-09-24 $\n]"
USAGE_LICENSE
"[+NAME?suimserver - SWIFT message server]"
"[+DESCRIPTION?\bsuimserver\b Implements the SWIFT message service."
"]"
"[100:port?specifies the port to listen on."
" If that port is already in use, the server will keep trying the next higher"
" port until it finds a free port or runs out of ports."
" The default port is\b40200\b."
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
" to register this instance of the message server."
" The default is to not register."
"]:[host/port]"
"[201:regkey?specifies the key to use to uniquely identify this server on the"
" registry server."
" The default is \bdefault\b."
"]:[key]"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"\n"
"[+SEE ALSO?\bsuimclient\b(1), \bsuirserver\b(1)]"
;

typedef struct conn_t {
    int fd;
    Sfio_t *fp, *bfp;
    int sync, syncreq, syncpend, echo, primary;
} conn_t;

static conn_t *conns;
static int connn;

static Sfio_t *bfp;

static fd_set infds, outfds;

static int seencmd;

static char *indir;
static unsigned char ownip[4];

static struct sigaction act;

static int serverstart (int *);
static int serverstop (int);
static int clientadd (int);
static int clientdelete (int);
static int serve (fd_set);

static int registryopen (char *, char *, char *, int);
static int registryclose (int);

int main (int argc, char **argv) {
    int norun;
    int port, fgmode, sfd, rfd, rtn;
    char *regname, *regkey, *logname;
    struct timeval *tvp, tv = { 60, 0 };
    char host[MAXHOSTNAMELEN + 1];
    struct hostent *hp;

    act.sa_flags = SA_RESTART;
    act.sa_handler = SIG_IGN;
    if (sigaction (SIGPIPE, &act, NULL) == -1) {
        SUwarning (1, "suimserver", "sigaction set failed");
        return -1;
    }
    port = DEFAULT_PORT;
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
        case -103:
            indir = opt_info.arg;
            continue;
        case -200:
            regname = opt_info.arg;
            continue;
        case -201:
            regkey = opt_info.arg;
            continue;
        case -999:
            SUwarnlevel++;
            continue;
        case '?':
            SUusage (0, "suimserver", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "suimserver", opt_info.arg);
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
            SUwarning (1, "suimserver", "gethostname failed");
            return -1;
        }
        if (!(hp = gethostbyname (host))) {
            SUwarning (1, "suimserver", "cannot find host %s", host);
            return -1;
        }
        ownip[0] = hp->h_addr_list[0][0];
        ownip[1] = hp->h_addr_list[0][1];
        ownip[2] = hp->h_addr_list[0][2];
        ownip[3] = hp->h_addr_list[0][3];
    }
    if (!(conns = vmalloc (Vmheap, sizeof (conn_t))))
        SUerror ("suimserver", "cannot allocate connection space");
    connn = 1;
    conns[0].fd = -1;
    if (!(bfp = sftmp (1024 * 1024))) {
        SUwarning (1, "suimserver", "sftmp failed");
        return -1;
    }
    if ((sfd = serverstart (&port)) < 0)
        SUerror ("suimserver", "cannot setup server port");
    FD_ZERO (&infds);
    FD_SET (sfd, &infds);
    if (!fgmode) {
        switch (fork ()) {
        case -1:
            SUerror ("suimserver", "fork failed");
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
            return 0;
        }
    }
    if (regname && (rfd = registryopen (regname, regkey, "-", port)) == -1)
        SUerror ("suimserver", "registryopen failed");
    tvp = &tv;
    for (;;) {
        outfds = infds;
        if ((rtn = select (FD_SETSIZE, &outfds, NULL, NULL, tvp)) < 0)
            SUerror ("suimserver", "select failed");
        if (rtn == 0)
            break;
        if (FD_ISSET (sfd, &outfds)) {
            if ((rtn = clientadd (sfd)) == -1)
                SUerror ("suimserver", "cannot add client");
            if (rtn >= 0)
                tvp = NULL;
            continue;
        }
        if ((rtn = serve (outfds)) < 0)
            break;
        if (rtn == 0)
            if (!seencmd)
                tvp = &tv;
            else
                break;
    }
    if (regname)
        registryclose (rfd);
    serverstop (sfd);
    sfclose (bfp);
    vmfree (Vmheap, conns);
    return 0;
}

static int serverstart (int *portp) {
    int sfd, slen, rtn;
    struct sockaddr_in sin;

    SUwarning (1, "serverstart", "starting server");
    if ((sfd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
        SUwarning (1, "serverstart", "cannot create socket");
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
        SUwarning (1, "serverstart", "cannot bind socket");
        return -1;
    }
    if (listen (sfd, 5) < 0) {
        SUwarning (1, "serverstart", "cannot listen on socket");
        return -1;
    }
    (*portp)--;
    sfprintf (sfstdout, "port = %d\n", *portp);
    sfsync (NULL);
    SUwarning (1, "serverstart", "server started on port %d", *portp);
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
    if (SUauth (SU_AUTH_SERVER, cfd, "SWIFTMSERVER", SWIFTAUTHKEY) == -1) {
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
    connp->fd = cfd;
    if (!(connp->fp = sfnew (NULL, NULL, -1, cfd, SF_READ | SF_WRITE))) {
        SUwarning (0, "clientadd", "sfnew failed");
        return -1;
    }
    if (!(connp->bfp = sftmp (1024 * 1024))) {
        SUwarning (0, "clientadd", "sftmp failed");
        return -1;
    }
    FD_SET (cfd, &infds);
    connp->sync = FALSE;
    connp->syncreq = 0;
    connp->syncpend = 0;
    connp->echo = FALSE;
    connp->primary = TRUE;
    SUwarning (0, "clientadd", "client added on fd %d", cfd);
    return cfd;
}

static int clientdelete (int cfd) {
    conn_t *connp;
    int conni, connj;

    SUwarning (1, "clientdelete", "deleting client on fd %d", cfd);
    if ((conni = cfd) >= connn || conns[cfd].fd == -1) {
        SUwarning (0, "clientdelete", "bad file descriptor");
        return -1;
    }
    connp = &conns[conni];
    if (connp->syncreq) {
        for (connj = 0; connj < connn; connj++) {
            if (conns[connj].syncpend > 0)
                conns[connj].syncpend--;
        }
        connp->syncreq = 0;
    }
    if (connp->syncpend) {
        for (connj = 0; connj < connn; connj++) {
            if (conns[connj].syncreq > 1)
                conns[connj].syncreq--;
        }
        connp->syncpend = 0;
    }
    sfclose (connp->bfp);
    sfclose (connp->fp);
    connp->fd = -1;
    FD_CLR (cfd, &infds);
    return 0;
}

#define STATE_COMPLETE 1
#define STATE_BODY     2
#define STATE_ERROR    3

static int serve (fd_set fds) {
    conn_t *connp, *conn2p;
    int conni, connj, connl;
    char *line;
    int state, noecho;
    void *bp;
    Sfoff_t blen;

    for (conni = 0; conni < connn; conni++) {
        connp = &conns[conni];
        if (connp->fd == -1)
            continue;
        if (sfseek (connp->bfp, 0, SEEK_SET) != 0) {
            SUwarning (1, "serve", "sfseek failed");
            return -1;
        }
    }
    for (conni = 0; conni < connn; conni++) {
        connp = &conns[conni];
        if (connp->fd == -1 || !FD_ISSET (connp->fd, &fds))
            continue;
        if (sfseek (bfp, 0, SEEK_SET) != 0) {
            SUwarning (1, "serve", "sfseek failed");
            return -1;
        }
        state = STATE_COMPLETE;
        while ((line = sfgetr (connp->fp, '\n', 1))) {
            if (line[strlen (line) - 1] == '\r')
                line[strlen (line) - 1] = 0;
            SUwarning (2, "serve", "client: %d line: %s", connp->fd, line);
            noecho = FALSE;
            if (state == STATE_COMPLETE && strcmp (line, "begin") == 0)
                state = STATE_BODY;
            else if (state == STATE_BODY && strcmp (line, "end") == 0)
                state = STATE_COMPLETE;
            else if (state == STATE_COMPLETE) {
                noecho = TRUE;
                if (strcmp (line, "syncon") == 0)
                    connp->sync = TRUE;
                else if (strcmp (line, "syncoff") == 0)
                    connp->sync = FALSE;
                else if (strcmp (line, "echoon") == 0)
                    connp->echo = TRUE;
                else if (strcmp (line, "echooff") == 0)
                    connp->echo = FALSE;
                else if (strcmp (line, "primaryon") == 0)
                    connp->primary = TRUE;
                else if (strcmp (line, "primaryoff") == 0)
                    connp->primary = FALSE;
                else if (strcmp (line, "syncall") == 0) {
                    if (connp->syncreq) {
                        SUwarning (1, "serve", "multiple syncall calls");
                        state = STATE_ERROR;
                        break;
                    }
                    connp->syncreq = 1;
                    for (connj = 0; connj < connn; connj++) {
                        conn2p = &conns[connj];
                        if (
                            conn2p->fd == -1 || !conn2p->sync ||
                            (conni == connj && !connp->echo)
                        )
                            continue;
                        if (sfputr (
                            conn2p->bfp, sfprints ("sync %d", conni), '\n'
                        ) == -1) {
                            SUwarning (
                                1, "serve", "sfputr failed for %d", conni
                            );
                            continue;
                        }
                        conn2p->syncpend++, connp->syncreq++;
                    }
                } else if (strncmp (line, "sync ", 5) == 0) {
                    if (connp->sync && connp->syncpend == 0) {
                        SUwarning (
                            1, "serve", "unsolicited sync from %d", conni
                        );
                        state = STATE_ERROR;
                        break;
                    } else if (connp->sync) {
                        if ((connj = atoi (line + 5)) < 0 || connj >= connn) {
                            SUwarning (1, "serve", "bad conn id %d", connj);
                            state = STATE_ERROR;
                            break;
                        }
                        conn2p = &conns[connj];
                        if (conn2p->fd == -1 || conn2p->syncreq <= 1) {
                            SUwarning (
                                1, "serve", "unsolicited sync for %d", connj
                            );
                            state = STATE_ERROR;
                            break;
                        }
                        connp->syncpend--, conn2p->syncreq--;
                    }
                } else {
                    state = STATE_ERROR;
                    break;
                }
            } else if (state != STATE_BODY) {
                SUwarning (1, "serve", "unexpected token %s", line);
                state = STATE_ERROR;
                break;
            }
            if (!noecho && connp->primary)
                seencmd = TRUE;
            if (!noecho && sfputr (bfp, line, '\n') == -1) {
                SUwarning (1, "serve", "sfputr failed");
                return -1;
            }
            if (state == STATE_COMPLETE) {
                if ((blen = sfseek (bfp, 0, SEEK_CUR)) == -1) {
                    SUwarning (1, "serve", "sfseek failed");
                    return -1;
                }
                if (blen > 0) {
                    if (sfseek (bfp, 0, SEEK_SET) == -1) {
                        SUwarning (1, "serve", "sfseek failed");
                        return -1;
                    }
                    if (!(bp = sfreserve (
                        bfp, blen, 0)) || sfvalue (bfp) < blen
                    ) {
                        SUwarning (1, "serve", "sfreserve failed");
                        return -1;
                    }
                    for (connj = 0; connj < connn; connj++) {
                        conn2p = &conns[connj];
                        if (
                            conn2p->fd == -1 ||
                            (conni == connj && !conn2p->echo)
                        )
                            continue;
                        if (sfwrite (conn2p->bfp, bp, blen) != blen) {
                            SUwarning (1, "serve", "sfputr failed");
                            return -1;
                        }
                    }
                }
                if (sfseek (bfp, 0, SEEK_SET) != 0) {
                    SUwarning (1, "serve", "sfseek failed");
                    return -1;
                }
            }
            sfset (connp->fp, SF_READ, 1);
            if (state == STATE_COMPLETE && connp->fp->_next >= connp->fp->_endb)
                break;
        }
        if (state != STATE_COMPLETE)
            state = STATE_ERROR;
        if (sfeof (connp->fp) || state == STATE_ERROR)
            clientdelete (connp->fd);
    }
    SUwarning (2, "serve", "echoing data");
    for (conni = connl = 0; conni < connn; conni++) {
        connp = &conns[conni];
        if (connp->fd == -1)
            continue;
        if (connp->primary)
            connl++;
        if (connp->syncreq == 1) {
            SUwarning (2, "serve", "synced %d", conni);
            if (sfputr (connp->bfp, "synced", '\n') == -1) {
                SUwarning (1, "serve", "synced failed for %d", connj);
                continue;
            }
            connp->syncreq = 0;
        }
        if ((blen = sfseek (connp->bfp, 0, SEEK_CUR)) == -1) {
            SUwarning (1, "serve", "sfseek failed");
            return -1;
        }
        if (blen > 0) {
            if (sfseek (connp->bfp, 0, SEEK_SET) == -1) {
                SUwarning (1, "serve", "sfseek failed");
                return -1;
            }
            if (
                !(bp = sfreserve (connp->bfp, blen, 0)) ||
                sfvalue (connp->bfp) < blen
            ) {
                SUwarning (1, "serve", "sfreserve failed");
                return -1;
            }
            if (sfwrite (connp->fp, bp, blen) != blen) {
                SUwarning (1, "serve", "sfputr failed");
                return -1;
            }
        }
    }
    if (sfsync (NULL) == -1) {
        SUwarning (1, "serve", "sfsync failed");
        return -1;
    }
    return connl;
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
        SUwarning (1, "registryopen", "cannot find host %s", host);
        return -1;
    }
    memset ((char *) &sin, 1, sizeof (sin));
    memcpy ((char *) &sin.sin_addr, hp->h_addr, hp->h_length);
    sin.sin_family = hp->h_addrtype;
    sin.sin_port = htons (port);
    if ((rfd = socket (hp->h_addrtype, SOCK_STREAM, 0)) < 0) {
        SUwarning (1, "registryopen", "cannot create socket");
        return -1;
    }
    if (connect (rfd, (struct sockaddr *) &sin, sizeof (sin)) < 0) {
        SUwarning (1, "registryopen", "cannot connect");
        return -1;
    }
    if (SUauth (SU_AUTH_CLIENT, rfd, "SWIFTRSERVER", SWIFTAUTHKEY) == -1) {
        SUwarning (1, "registryopen", "authentication failed");
        return -1;
    }
    if (!(rfp = sfnew (NULL, NULL, -1, rfd, SF_READ | SF_WRITE))) {
        SUwarning (1, "registryopen", "sfnew failed");
        return -1;
    }
    sfprintf (rfp, "add %s %d SWIFTMSERVER '%s'\n", rname, rport, regkey);
    if (!(line = sfgetr (rfp, '\n', 1)) || strcmp (line, "SUCCESS") != 0) {
        SUwarning (1, "registryopen", "registration failed");
        return -1;
    }
    return rfd;
}

static int registryclose (int rfd) {
    close (rfd);
    return 0;
}
