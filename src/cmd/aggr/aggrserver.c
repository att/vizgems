#pragma prototyped

#define _AGGR_PRIVATE

#include <ast.h>
#include <option.h>
#include <vmalloc.h>
#include <tok.h>
#include <swift.h>
#include <float.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <aggr.h>
#include "key.h"

static const char usage[] =
"[-1p1s1D?\n@(#)$Id: aggrserver (AT&T Labs Research) 2001-09-24 $\n]"
USAGE_LICENSE
"[+NAME?aggrserver - data server for AGGR files]"
"[+DESCRIPTION?\baggrserver\b is a data server for delivering AGGR files to"
" remote systems."
" \baggrserver\b uses the same locking mechanism as the local access routines"
" to ensure that data is extracted in a consistent way."
" Both the AGGR library interface and the standard AGGR tools can access files"
" through this server."
" All that is required is for the file name to be in URL form:"
" http://abc.att.com:10000/a/b/c."
" Authentication is used to verify that the client is authorized to use the"
" server."
" See \bswiftencode\b(1) for more information."
" The \baggrserver\b can also interface with the SWIFT registry server"
" \bsuirserver\b(1) to advertise its presence.]"
"[140:port?specifies the IP port to listen on."
" The default port is \b40900\b."
"]#[port]"
"[141:fg?specifies that the server should run in the foreground."
" The default action is to run in the background."
"]"
"[142:regname?specifies the host name and port of a registry server."
" \baggrserver\b will contact the registry server and use it to advertise its"
" existence."
" The default action is to not register."
"]:[host/port]"
"[143:regkey?specifies a unique key to advertize on the registry server."
" A client can query the registry server using this key."
" The default key is \bdefault\b."
"]:[key]"
"[144:log?specifies the name of a log file to use to log all service requests."
" The default action is to not write a log."
" However, if the \bfg\b flag is specified, the server will always write its"
" log to standard error."
"]:[logfile]"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"[+SEE ALSO?\baggr\b(1), \baggr\b(3), \bsuirserver\b(3)]"
;

#define DEFAULT_PORT 40900

static int nchildren;

static int registryopen (char *, char *, char *, int);
static int registryclose (int);

static void sigchldhandler (int);

int main (int argc, char **argv) {
    int norun;
    int port, fgmode;
    char *regname, *regkey, *logname;
    fd_set infds, outfds;
    AGGRserver_t as;
    int sfd, cfd, rfd, pid;
    struct sockaddr sas;
    socklen_t san;

    if (AGGRinit () == -1)
        SUerror ("aggrserver", "init failed");
    port = DEFAULT_PORT;
    fgmode = FALSE;
    regname = NULL;
    regkey = "default";
    logname = "/dev/null";
    norun = 0;
    for (;;) {
        switch (optget (argv, usage)) {
        case -140:
            port = opt_info.num;
            continue;
        case -141:
            fgmode = TRUE;
            continue;
        case -142:
            regname = opt_info.arg;
            continue;
        case -143:
            regkey = opt_info.arg;
            continue;
        case -144:
            logname = opt_info.arg;
            continue;
        case -999:
            SUwarnlevel++;
            continue;
        case '?':
            SUusage (0, "aggrserver", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "aggrserver", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;
    if ((sfd = AGGRserverstart (&port)) < 0)
        SUerror ("aggrserver", "cannot create socket");
    FD_ZERO (&infds);
    FD_SET (sfd, &infds);
    if (!fgmode) {
        switch ((pid = fork ())) {
        case -1:
            SUerror ("aggrserver", "fork failed");
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
        SUerror ("aggrserver", "registryopen failed");
    san = sizeof (struct sockaddr);
    for (;;) {
        outfds = infds;
        errno = 0;
        if ((select (FD_SETSIZE, &outfds, NULL, NULL, NULL)) <= 0) {
            if (errno == EINTR)
                continue;
            SUerror ("aggrserver", "select failed");
        }
        if (!FD_ISSET (sfd, &outfds))
            break;
doaccept:
        errno = 0;
        if ((cfd = accept (sfd, &sas, &san)) < 0) {
            if (errno == EINTR)
                goto doaccept;
            SUerror ("aggrserver", "accept failed");
        }
        SUwarning (
            0, "aggrserver", "connection request from %d.%d.%d.%d",
            sas.sa_data[2], sas.sa_data[3], sas.sa_data[4], sas.sa_data[5]
        );
        switch (fork ()) {
        case -1:
            SUerror ("aggrserver", "fork failed");
            break;
        case 0:
            if (SUauth (
                SU_AUTH_SERVER, cfd, "SWIFTASERVER", SWIFTAUTHKEY
            ) == -1) {
                SUwarning (1, "aggrserver", "authentication failed");
                return -1;
            }
            as.socket = cfd;
            AGGRserve (&as);
            return 0;
        default:
            nchildren++;
            close (cfd);
            break;
        }
    }
    if (regname)
        registryclose (rfd);
    AGGRserverstop (sfd);
    if (AGGRterm () == -1)
        SUerror ("aggrserver", "term failed");
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
    sfprintf (rfp, "add %s %d SWIFTDSERVER '%s'\n", rname, rport, regkey);
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

static void sigchldhandler (int data) {
    while (waitpid (-1, NULL, WNOHANG) > 0)
        nchildren--;
    signal (SIGCHLD, sigchldhandler);
}
