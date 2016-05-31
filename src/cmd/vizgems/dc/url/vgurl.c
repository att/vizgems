#pragma prototyped

#include <ast.h>
#include <option.h>
#include <vmalloc.h>
#include <swift.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>
#include <errno.h>

static struct timeval t1, t2, t3, t4, t5, t6;

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
        SUerror ("vgurl", "cannot get time"); \
}
#endif

#define DEFAULT_PORT 40000

static const char usage[] =
"[-1p1?\n@(#)$Id: vgurl (AT&T Labs Research) 2001-09-24 $\n]"
USAGE_LICENSE
"[+NAME?vgurl - client tool for SWIFT url timing measuments]"
"[+DESCRIPTION?\bvgurl\b reads lines from stdin that specify the ip, port,"
" host name, and url to collect."
" It outputs information about whether the url was available and how long"
" it took to complete the transaction."
"]"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"\n"
"[+SEE ALSO?\bvgurl\b(1)]"
;

#define BUFLEN 10 * 1024
char buf[BUFLEN];

static int fullread (int, void *, size_t);
static int fullwrite (int, void *, size_t);

int main (int argc, char **argv) {
    char *pipstr, *pportstr, *hoststr, *urlstr;
    int norun;
    int sfd;
    struct addrinfo aih, *ailist, *aip;
    char *s, *cp, *ep, *resstr, *restxt, *nlstr;
    int ret, len, pllen, nlflag, state;

    norun = 0;
    for (;;) {
        switch (optget (argv, usage)) {
        case -999:
            SUwarnlevel++;
            continue;
        case '?':
            SUusage (0, "vgurl", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "vgurl", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;
    if (argc != 0)
        SUerror ("vgurl", "usage: %s", optusage (NULL));

    if (
        !(pipstr = strdup (sfgetr (sfstdin, '\n', 1))) ||
        !(pportstr = strdup (sfgetr (sfstdin, '\n', 1))) ||
        !(hoststr = strdup (sfgetr (sfstdin, '\n', 1))) ||
        !(urlstr = strdup (sfgetr (sfstdin, '\n', 1)))
    )
        SUerror ("vgurl", "cannot read connection parameters");

    sfsprintf (
        buf, 10 * 1024, "GET %s HTTP/1.0\r\nHost: %s\r\n\n",
        urlstr, hoststr
    );
    len = strlen (buf);

    GETTIME (t1);

    memset (&aih, 0, sizeof (struct addrinfo));
    aih.ai_family = AF_UNSPEC;
    aih.ai_socktype = SOCK_STREAM;
    aih.ai_protocol = 0;
    if (getaddrinfo (pipstr, pportstr, &aih, &ailist) == -1)
        SUerror ("vgurl", "cannot get addrinfo");

    GETTIME (t2);

    for (aip = ailist; aip; aip = aip->ai_next) {
        if ((sfd = socket (
            aip->ai_family, aip->ai_socktype, aip->ai_protocol
        )) < 0) {
            freeaddrinfo (ailist);
            SUerror ("vgurl", "cannot create socket");
        }
        GETTIME (t3);
        ret = connect (sfd, aip->ai_addr, aip->ai_addrlen);
        GETTIME (t4);
        freeaddrinfo (ailist);
        if (ret == 0)
            break;
        if (ret == -1 && errno != EINTR) {
            close (sfd);
            SUwarning (1, "vgurl", "cannot connect socket");
            exit (0);
        }
    }

    if (fullwrite (sfd, buf, len) == -1)
        SUerror ("vgurl", "cannot send request");

    state = 0, nlflag = FALSE, pllen = 0;
    while ((len = fullread (sfd, buf, 1024)) > 0) {
        cp = buf, ep = buf + len;
        if (state == 0) {
            if (!(s = memchr (buf, '\n', len)))
                SUerror ("vgurl", "cannot parse server response");
            nlstr = s;
            if (memcmp (buf, "HTTP", 4) != 0)
                SUerror ("vgurl", "bad server response");
            if (!(s = memchr (buf, ' ', len)))
                SUerror ("vgurl", "cannot find server response code");
            resstr = s + 1;
            if (!(s = memchr (resstr, ' ', len - (resstr - buf))))
                SUerror ("vgurl", "cannot read server response text");
            *s = 0, restxt = s + 1, *nlstr = 0;
            state = 1;
            cp = nlstr + 1;
            if (restxt[strlen (restxt) - 1] == '\r')
                restxt[strlen (restxt) - 1] = 0;
            resstr = strdup (resstr);
            restxt = strdup (restxt);
        }
        if (state == 1) {
            for (s = cp; s < ep; s++) {
                if (*s == '\r')
                    continue;
                else if (*s == '\n') {
                    if (nlflag) {
                        cp = s + 1;
                        state = 2;
                        GETTIME (t5);
                        break;
                    }
                    nlflag = TRUE;
                } else
                    nlflag = FALSE;
            }
        }
        if (state == 2) {
            pllen += (ep - cp);
        }
    }
    if (len == -1)
        SUerror ("vgurl", "cannot read response");

    GETTIME (t6);

    close (sfd);

    sfprintf (sfstdout, "code=%s\n", resstr);
    sfprintf (sfstdout, "txt=%s\n", restxt);
    sfprintf (sfstdout, "len=%d\n", pllen);
    sfprintf (
        sfstdout, "tdns=%f\n",
        (t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec) / 1000000.0
    );
    sfprintf (
        sfstdout, "tsock=%f\n",
        (t3.tv_sec - t2.tv_sec) + (t3.tv_usec - t2.tv_usec) / 1000000.0
    );
    sfprintf (
        sfstdout, "tconn=%f\n",
        (t4.tv_sec - t3.tv_sec) + (t3.tv_usec - t3.tv_usec) / 1000000.0
    );
    sfprintf (
        sfstdout, "tres1=%f\n",
        (t5.tv_sec - t3.tv_sec) + (t5.tv_usec - t3.tv_usec) / 1000000.0
    );
    sfprintf (
        sfstdout, "tres2=%f\n",
        (t6.tv_sec - t3.tv_sec) + (t6.tv_usec - t3.tv_usec) / 1000000.0
    );

    return 0;
}

static int fullread (int fd, void *buf, size_t len) {
    size_t off;
    int ret;

    off = 0;
    while ((ret = read (fd, (char *) buf + off, len - off)) > 0) {
        if ((off += ret) == len)
            break;
        if (*((char *) buf + off - 1) == '\n')
            break;
    }
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
