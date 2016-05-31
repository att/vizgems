#pragma prototyped

#include <ast.h>
#include <option.h>
#include <vmalloc.h>
#include <swift.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>
#include <errno.h>

static struct timeval t1, t2;

#ifdef _UWIN
#include <windows.h>
LARGE_INTEGER count;
static unsigned long long c, f;
#define GETTIME(t) { \
    if (f == 0) \
        QueryPerformanceFrequency (&count), f = *(long long *) &count; \
    QueryPerformanceCounter (&count), c = *(long long *) &count; \
    t.tv_sec = (c / f); \
    t.tv_usec = ((c / (long double) f) - t.tv_sec) * 1000000; \
}
#else
#define GETTIME(t) { \
    if (gettimeofday (&t, NULL) == -1) \
        SUerror ("vgport", "cannot get time"); \
}
#endif

#define DEFAULT_PORT 40000

static const char usage[] =
"[-1p1?\n@(#)$Id: vgport (AT&T Labs Research) 2001-09-24 $\n]"
USAGE_LICENSE
"[+NAME?vgport - client tool for SWIFT port scanning]"
"[+DESCRIPTION?\bvgport\b attempts to connect to the specified IP address"
" using the ports specified."
"]"
"[100:t?specifies the timeout.]#[timeout]"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"\n"
"ipaddress port1 port2 ..."
"\n"
"[+SEE ALSO?\bvgport\b(1)]"
;

int main (int argc, char **argv) {
    char *ipstr, *portstr;
    int norun;
    int sfd;
    struct addrinfo aih, *ailist, *aip;
    int enable, ret;
    fd_set infds, outfds;
    struct timeval tv;
    int to;
    int errv;
    socklen_t errl;

    to = 5;
    norun = 0;
    for (;;) {
        switch (optget (argv, usage)) {
        case -100:
            to = opt_info.num;
            continue;
        case -999:
            SUwarnlevel++;
            continue;
        case '?':
            SUusage (0, "vgport", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "vgport", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;
    if (argc < 2)
        SUerror ("vgport", "usage: %s", optusage (NULL));

    ipstr = argv[0];
    argc--, argv++;

    while (argc > 0) {
        portstr = argv[0];
        memset (&aih, 0, sizeof (struct addrinfo));
        aih.ai_family = AF_UNSPEC;
        aih.ai_socktype = SOCK_STREAM;
        aih.ai_protocol = 0;
        if (getaddrinfo (ipstr, portstr, &aih, &ailist) == -1)
            SUerror ("vgport", "cannot get addrinfo");
        GETTIME (t1);
        for (aip = ailist; aip; aip = aip->ai_next) {
            if ((sfd = socket (
                aip->ai_family, aip->ai_socktype, aip->ai_protocol
            )) < 0) {
                freeaddrinfo (ailist);
                SUerror ("vgport", "cannot create socket");
            }
            enable = 1;
            if (ioctl (sfd, FIONBIO, &enable) < 0)
                SUerror ("vgport", "cannot set non-blocking mode on socket");
            FD_ZERO (&infds);
            FD_SET (sfd, &infds);
            tv.tv_sec = to, tv.tv_usec = 0;
            errl = sizeof (int);
            ret = connect (sfd, aip->ai_addr, aip->ai_addrlen);
            freeaddrinfo (ailist);
            if (ret < 0 && errno == EINPROGRESS) {
                outfds = infds;
                if ((ret = select (FD_SETSIZE, NULL, &outfds, NULL, &tv)) > 0) {
                    ret = -1;
                    if (FD_ISSET (sfd, &outfds) && getsockopt (
                        sfd, SOL_SOCKET, SO_ERROR, &errv, &errl
                    ) >= 0 && errl == sizeof (int) && errv == 0)
                        ret = 0;
                } else if (ret == 0)
                    ret = -1;
            }
            if (ret == 0)
                break;
            if (ret == -1 && errno != EINTR) {
                close (sfd);
                SUwarning (1, "vgport", "cannot connect socket");
                exit (0);
            }
        }
        GETTIME (t2);

        close (sfd);
        sfprintf (
            sfstdout, "port=%s result=%s time=%f\n",
            portstr, (ret >= 0) ? "Y" : "N",
            (t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec) / 1000000.0
        );
        argc--, argv++;
    }

    return 0;
}
