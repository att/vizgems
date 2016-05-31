#pragma prototyped

#include <ast.h>
#include <option.h>
#include <swift.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/termios.h>
#include "key.h"

#define DEFAULT_PORT 40900

static const char usage[] =
"[-1p1s1D?\n@(#)$Id: suixwacom (AT&T Labs Research) 2001-09-24 $\n]"
USAGE_LICENSE
"[+NAME?suixwacom - client tool for SWIFT X service]"
"[+DESCRIPTION?\bsuixwacom\b interfaces a WACOM tablet with the SWIFT X"
" service."
"]"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"\n"
"\nhost/port wacomserialport display\n"
"\n"
"[+SEE ALSO?\bsuixserver\b(1)]"
;

#define CDIST(v1, v2) ((v1) > (v2) ? (v1) - (v2) : (v2) - (v1))
#define XDIST(v1, v2) (CDIST ((v1), (v2)) * 800.0 / maxx)
#define YDIST(v1, v2) (CDIST ((v1), (v2)) * 600.0 / maxy)

typedef struct wdata_t {
    int sync;
    int det;
    int type;
    int but, butmask;
    int x;
    int y;
    int p;
} wdata_t;

static int xid;

static void parsedata (unsigned char *, wdata_t *);

static int serverget (char *);
static int serverrelease (int);

static int wacomopen (char *);
static int wacomclose (int);

static int readmsg (int, unsigned char *, int, int, char *);
static int writemsg (int, unsigned char *, int);

int main (int argc, char **argv) {
    int norun;
    int cfd, tfd;
    fd_set infds, outfds;
    unsigned char bufs[65536], *s;
    int bufn;
    int maxx, maxy;
    wdata_t pwd, cwd;

    norun = 0;
    for (;;) {
        switch (optget (argv, usage)) {
        case -999:
            SUwarnlevel++;
            continue;
        case '?':
            SUusage (0, "suixwacom", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "suixwacom", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;
    if (argc < 3)
        SUerror ("suiwacom", "usage: %s", optusage (NULL));

    if ((cfd = serverget (argv[0])) == -1)
        SUerror ("suiwacom", "cannot connect to server");
    if ((tfd = wacomopen (argv[1])) == -1)
        SUerror ("suiwacom", "cannot connect to wacom");
    FD_ZERO (&infds);
    FD_SET (cfd, &infds);
    FD_SET (tfd, &infds);

    sfsprintf ((char *) bufs, 65536, "open %s\n", argv[2]);
    if (writemsg (cfd, bufs, strlen ((char *) bufs)) < 0)
        SUerror ("suiwacom", "write to server failed");
    if ((bufn = readmsg (cfd, bufs, 65536, 65536, "\n")) < 0)
        SUerror ("suiwacom", "read from server failed");
    SUwarning (1, "suiwacom", "server: %s", bufs);
    if (strncmp ((char *) bufs, "SUCCESS\n", 8) != 0)
        SUerror ("suiwacom", "command failed");
    memmove (bufs, bufs + 8, bufn - 8 + 1);
    bufn -= 8;
    if (!strstr ((char *) bufs, "\n")) {
        if ((bufn = readmsg (cfd, bufs + bufn, 65536, 65536, "\n")) < 0)
            SUerror ("suiwacom", "read from server failed");
        SUwarning (1, "suiwacom", "server: %s", bufs);
    }
    xid = atoi ((char *) bufs);
    SUwarning (1, "suiwacom", "xid = %d", xid);

    if (readmsg (tfd, bufs, 0, 65536, NULL) < 0)
        SUerror ("suiwacom", "read from wacom failed");
    if (writemsg (tfd, "\r#", 2) < 0)
        SUerror ("suiwacom", "write to wacom failed");
    if ((bufn = readmsg (tfd, bufs, 0, 65536, NULL)) < 0)
        SUerror ("suiwacom", "read from wacom failed");
    if (writemsg (tfd, "~C\r", 3) < 0)
        SUerror ("suiwacom", "write to wacom failed");
    if ((bufn = readmsg (tfd, bufs, 14, 14, "\n")) < 0)
        SUerror ("suiwacom", "read from wacom failed");
        SUwarning (1, "suiwacom", "wacom: %s", bufs);
    if (!(s = (unsigned char *) strchr ((char *) bufs, ',')))
        SUerror ("suiwacom", "command to wacom failed");
    *s = '\000';
    maxx = atoi ((char *) bufs + 2);
    bufs[bufn - 1] = 0;
    maxy = atoi ((char *) s + 1);
    SUwarning (1, "suiwacom", "coord = %d %d", maxx, maxy);
    memset (&pwd, 0, sizeof (pwd));

    for (;;) {
        outfds = infds;
        if ((select (FD_SETSIZE, &outfds, NULL, NULL, NULL)) <= 0)
            SUerror ("suiwacom", "select failed");
        if (FD_ISSET (cfd, &outfds)) {
            if ((bufn = readmsg (cfd, bufs, 65536, 65536, "\n")) < 0)
                SUerror ("suiwacom", "read from server failed");
            if (bufn == 0)
                break;
            SUwarning (2, "suiwacom", "%s", bufs);
        }
        if (FD_ISSET (tfd, &outfds)) {
            if ((bufn = readmsg (tfd, bufs, 7, 7, NULL)) != 7) {
                SUerror ("suiwacom", "read from wacom failed");
                break;
            }
            if (bufn == 0)
                break;
            parsedata (bufs, &cwd);
            SUwarning (
                1, "suiwacom", "s: %d, d: %d, t: %d, b: %d, x,y: %d,%d",
                cwd.sync, cwd.det, cwd.type, cwd.but, cwd.x, cwd.y
            );
            if (cwd.but != pwd.but) {
                sfsprintf (
                    (char *) bufs, 65536, "button %d 0 %d\n", xid, cwd.but
                );
                if (writemsg (cfd, bufs, strlen ((char *) bufs)) < 0)
                    SUerror ("suiwacom", "write to server failed");
                pwd.but = cwd.but;
            }
            if (XDIST (cwd.x, pwd.x) >= 5.0 || YDIST (cwd.y, pwd.y) >= 5.0) {
                sfsprintf (
                    (char *) bufs, 65536, "pointer %d %f %f\n", xid,
                    (float) cwd.x / maxx, (float) cwd.y / maxy
                );
                if (writemsg (cfd, bufs, strlen ((char *) bufs)) < 0)
                    SUerror ("suiwacom", "write to server failed");
                pwd.x = cwd.x, pwd.y = cwd.y;
            }
        }
    }
    wacomclose (tfd);
    serverrelease (cfd);
    return 0;
}

static void parsedata (unsigned char *buf, wdata_t *wdp) {
    wdp->sync = (buf[0] & 0x80) >> 7;
    wdp->det = (buf[0] & 0x40) >> 6;
    wdp->type = (buf[0] & 0x20) >> 5;
    wdp->but = (buf[0] & 0x08) >> 3;
    wdp->x = ((buf[0] & 0x03) << 13) + ((buf[1] & 0x7f) << 7) + (buf[2] & 0x7f);
    wdp->y = ((buf[3] & 0x03) << 13) + ((buf[4] & 0x7f) << 7) + (buf[5] & 0x7f);
    wdp->p = ((buf[6] & 0x3f) << 1) + ((buf[3] & 0x04) >> 2);
    if (buf[6] & 0x40 == 0)
        wdp->p += 128;
    wdp->butmask = (buf[3] >> 3);
}

static int serverget (char *namep) {
    char *host, *portp;
    int port;
    struct hostent *hp;
    struct sockaddr_in sin;
    int cfd;

    host = namep;
    if ((portp = strchr (host, '/')))
        port = atoi (portp + 1), *portp = 0;
    else
        port = DEFAULT_PORT;
    if (!(hp = gethostbyname (host))) {
        SUwarning (1, "serverget", "cannot find host %s", host);
        return -1;
    }
    memset ((char *) &sin, 1, sizeof (sin));
    memcpy ((char *) &sin.sin_addr, hp->h_addr, hp->h_length);
    sin.sin_family = hp->h_addrtype;
    sin.sin_port = htons (port);
    if ((cfd = socket (hp->h_addrtype, SOCK_STREAM, 0)) < 0) {
        SUwarning (1, "serverget", "cannot create socket");
        return -1;
    }
    if (connect (cfd, (struct sockaddr *) &sin, sizeof (sin)) < 0) {
        SUwarning (1, "serverget", "cannot connect");
        return -1;
    }
    if (SUauth (SU_AUTH_CLIENT, cfd, "SWIFTXSERVER", SWIFTAUTHKEY) == -1) {
        SUwarning (1, "serverget", "authentication failed");
        return -1;
    }
    return cfd;
}

static int serverrelease (int cfd) {
    close (cfd);
    return 0;
}

static int wacomopen (char *namep) {
    int tfd;
    struct termios ts;

    if ((tfd = open (namep, O_RDWR)) == -1)
        return -1;
    tcgetattr (tfd, &ts);
    cfsetospeed (&ts, B9600);
    cfsetispeed (&ts, B9600);
    ts.c_iflag = IXOFF;
    ts.c_oflag = 0;
    ts.c_lflag = 0;
    ts.c_cflag = B9600 | CS8 | CREAD | CLOCAL;
    ts.c_cc[VMIN] = 0;
    ts.c_cc[VTIME] = 2;
    tcsetattr (tfd, TCSANOW, &ts);
    return tfd;
}

static int wacomclose (int tfd) {
    close (tfd);
    return 0;
}

static int readmsg (int fd, unsigned char *buf, int minn, int maxn, char *pat) {
    int l, ret;

    l = 0;
    while ((ret = read (fd, &buf[l], maxn - l)) >= 0) {
        l += ret;
        buf[l] = 0;
        if (l == maxn)
            break;
        if (pat) {
            if (strstr ((char *) buf, pat))
                break;
        } else if (l >= minn)
            break;
    }
    if (ret == -1)
        return -1;
    return l;
}

static int writemsg (int fd, unsigned char *buf, int n) {
    int l, ret;

    l = 0;
    while (n - l > 0 && (ret = write (fd, &buf[l], n - l)) > 0)
        l += ret;
    if (l != n)
        return -1;
    return l;
}
