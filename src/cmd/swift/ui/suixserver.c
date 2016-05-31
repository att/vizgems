#pragma prototyped

#include <ast.h>
#include <option.h>
#include <vmalloc.h>
#include <tok.h>
#include <swift.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#include "key.h"

#define DEFAULT_PORT 40900

static const char usage[] =
"[-1p1s1D?\n@(#)$Id: suixserver (AT&T Labs Research) 2001-09-24 $\n]"
USAGE_LICENSE
"[+NAME?suixserver - SWIFT X server]"
"[+DESCRIPTION?\bsuixserver\b Implements the SWIFT X service."
"]"
"[100:port?specifies the port to listen on."
" If that port is already in use, the server will keep trying the next higher"
" port until it finds a free port or runs out of ports."
" The default port is \b40900\b."
"]#[port]"
"[101:fg?specifies that the server should not background itself."
"]"
"[102:log?specifies the log file to use."
" The default is to not log anything, unless the \bfg\b option is specified"
" in which case log entries are printed on stderr."
"]:[logfile]"
"[999:v?increases the verbosity level. May be specified multiple times.]"
"\n"
"[+SEE ALSO?\bsuixclient\b(1)]"
;

typedef struct cconn_t {
    int fd;
    Sfio_t *fp;
    int *xcmaps;
    int xcmapn;
} cconn_t;

static cconn_t *cconns;
static int cconnn;

typedef struct xconn_t {
    int fd;
    Display *disp;
    XEvent ev;
    char *name;
    Window root;
    int rx, ry;
    unsigned int rw, rh;
    int ref;
    int usewarp;
} xconn_t;

static xconn_t *xconns;
static int xconnn;

typedef struct s2ksmap_t {
    int ascii;
    int ks1, ks2;
} s2ksmap_t;

static s2ksmap_t s2ksmaps[] = {
    { 0, NoSymbol, NoSymbol },
    { 1, XK_Control_L, XK_a },
    { 2, XK_Control_L, XK_b },
    { 3, XK_Control_L, XK_c },
    { 4, XK_Control_L, XK_d },
    { 5, XK_Control_L, XK_e },
    { 6, XK_Control_L, XK_f },
    { 7, XK_Control_L, XK_g },
    { 8, XK_BackSpace, NoSymbol },
    { 9, XK_Tab, NoSymbol },
    { 10, XK_Return, NoSymbol },
    { 11, XK_Control_L, XK_k },
    { 12, XK_Control_L, XK_l },
    { 13, XK_Return, NoSymbol },
    { 14, XK_Control_L, XK_n },
    { 15, XK_Control_L, XK_o },
    { 16, XK_Control_L, XK_p },
    { 17, XK_Control_L, XK_q },
    { 18, XK_Control_L, XK_r },
    { 19, XK_Control_L, XK_s },
    { 20, XK_Control_L, XK_t },
    { 21, XK_Control_L, XK_u },
    { 22, XK_Control_L, XK_v },
    { 23, XK_Control_L, XK_w },
    { 24, XK_Control_L, XK_x },
    { 25, XK_Control_L, XK_y },
    { 26, XK_Control_L, XK_z },
    { 27, XK_Escape, NoSymbol },
    { 28, NoSymbol, NoSymbol },
    { 29, NoSymbol, NoSymbol },
    { 30, NoSymbol, NoSymbol },
    { 31, NoSymbol, NoSymbol },
    { 32, XK_space, NoSymbol },
    { 33, XK_Shift_L, XK_exclam },
    { 34, XK_Shift_L, XK_quotedbl },
    { 35, XK_Shift_L, XK_numbersign },
    { 36, XK_Shift_L, XK_dollar },
    { 37, XK_Shift_L, XK_percent },
    { 38, XK_Shift_L, XK_ampersand },
    { 39, XK_apostrophe, NoSymbol },
    { 40, XK_Shift_L, XK_parenleft },
    { 41, XK_Shift_L, XK_parenright },
    { 42, XK_Shift_L, XK_asterisk },
    { 43, XK_Shift_L, XK_plus },
    { 44, XK_comma, NoSymbol },
    { 45, XK_minus, NoSymbol },
    { 46, XK_period, NoSymbol },
    { 47, XK_slash, NoSymbol },
    { 48, XK_0, NoSymbol },
    { 49, XK_1, NoSymbol },
    { 50, XK_2, NoSymbol },
    { 51, XK_3, NoSymbol },
    { 52, XK_4, NoSymbol },
    { 53, XK_5, NoSymbol },
    { 54, XK_6, NoSymbol },
    { 55, XK_7, NoSymbol },
    { 56, XK_8, NoSymbol },
    { 57, XK_9, NoSymbol },
    { 58, XK_Shift_L, XK_colon },
    { 59, XK_semicolon, NoSymbol },
    { 60, XK_Shift_L, XK_less },
    { 61, XK_equal, NoSymbol },
    { 62, XK_Shift_L, XK_greater },
    { 63, XK_Shift_L, XK_question },
    { 64, XK_Shift_L, XK_at },
    { 65, XK_Shift_L, XK_A },
    { 66, XK_Shift_L, XK_B },
    { 67, XK_Shift_L, XK_C },
    { 68, XK_Shift_L, XK_D },
    { 69, XK_Shift_L, XK_E },
    { 70, XK_Shift_L, XK_F },
    { 71, XK_Shift_L, XK_G },
    { 72, XK_Shift_L, XK_H },
    { 73, XK_Shift_L, XK_I },
    { 74, XK_Shift_L, XK_J },
    { 75, XK_Shift_L, XK_K },
    { 76, XK_Shift_L, XK_L },
    { 77, XK_Shift_L, XK_M },
    { 78, XK_Shift_L, XK_N },
    { 79, XK_Shift_L, XK_O },
    { 80, XK_Shift_L, XK_P },
    { 81, XK_Shift_L, XK_Q },
    { 82, XK_Shift_L, XK_R },
    { 83, XK_Shift_L, XK_S },
    { 84, XK_Shift_L, XK_T },
    { 85, XK_Shift_L, XK_U },
    { 86, XK_Shift_L, XK_V },
    { 87, XK_Shift_L, XK_W },
    { 88, XK_Shift_L, XK_X },
    { 89, XK_Shift_L, XK_Y },
    { 90, XK_Shift_L, XK_Z },
    { 91, XK_bracketleft, NoSymbol },
    { 92, XK_backslash, NoSymbol },
    { 93, XK_bracketright, NoSymbol },
    { 94, XK_Shift_L, XK_asciicircum },
    { 95, XK_Shift_L, XK_underscore },
    { 96, XK_grave, NoSymbol },
    { 97, XK_a, NoSymbol },
    { 98, XK_b, NoSymbol },
    { 99, XK_c, NoSymbol },
    { 100, XK_d, NoSymbol },
    { 101, XK_e, NoSymbol },
    { 102, XK_f, NoSymbol },
    { 103, XK_g, NoSymbol },
    { 104, XK_h, NoSymbol },
    { 105, XK_i, NoSymbol },
    { 106, XK_j, NoSymbol },
    { 107, XK_k, NoSymbol },
    { 108, XK_l, NoSymbol },
    { 109, XK_m, NoSymbol },
    { 110, XK_n, NoSymbol },
    { 111, XK_o, NoSymbol },
    { 112, XK_p, NoSymbol },
    { 113, XK_q, NoSymbol },
    { 114, XK_r, NoSymbol },
    { 115, XK_s, NoSymbol },
    { 116, XK_t, NoSymbol },
    { 117, XK_u, NoSymbol },
    { 118, XK_v, NoSymbol },
    { 119, XK_w, NoSymbol },
    { 120, XK_x, NoSymbol },
    { 121, XK_y, NoSymbol },
    { 122, XK_z, NoSymbol },
    { 123, XK_Shift_L, XK_braceleft },
    { 124, XK_Shift_L, XK_bar },
    { 125, XK_Shift_L, XK_braceright },
    { 126, XK_Shift_L, XK_asciitilde },
    { 127, XK_Delete, NoSymbol }
};

typedef int (*cmdfunc_t) (cconn_t *, int, char **);

typedef struct cmd_t {
    char *name;
    cmdfunc_t func;
    int avn;
} cmd_t;

static int cmdopen (cconn_t *, int, char **);
static int cmdclose (cconn_t *, int, char **);
static int cmdgetinfo (cconn_t *, int, char **);
static int cmdpointer (cconn_t *, int, char **);
static int cmdbutton (cconn_t *, int, char **);
static int cmdkey (cconn_t *, int, char **);
static int cmdgetstate (cconn_t *, int, char **);

static cmd_t cmds[] = {
    { "open", cmdopen, 2 },
    { "close", cmdclose, 2 },
    { "getinfo", cmdgetinfo, 2 },
    { "pointer", cmdpointer, 4 },
    { "button", cmdbutton, 4 },
    { "key", cmdkey, 4 },
    { "getstate", cmdgetstate, 1 },
    { NULL, NULL, 0 }
};

static fd_set infds, outfds;

static struct sigaction act;

static int serverstart (int);
static int serverstop (int);
static int clientadd (int);
static int clientdelete (int);
static int xserveradd (cconn_t *, char *);
static int xserverdelete (cconn_t *, int);
static int xserverinfo (xconn_t *, cconn_t *);
static int xserverinforec (
    cconn_t *, xconn_t *, Window, int, int, int, int, int
);
static int serve (fd_set);

static int errorhandler (Display *, XErrorEvent *);

int main (int argc, char **argv) {
    int norun;
    int port, fgmode, sfd;
    char *logname;

    act.sa_flags = SA_RESTART;
    act.sa_handler = SIG_IGN;
    if (sigaction (SIGPIPE, &act, NULL) == -1) {
        SUwarning (1, "suixserver", "sigaction set failed");
        return -1;
    }
    port = DEFAULT_PORT;
    fgmode = FALSE;
    logname = "/dev/null";
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
        case -999:
            SUwarnlevel++;
            continue;
        case '?':
            SUusage (0, "suixserver", opt_info.arg);
            norun = 1;
            continue;
        case ':':
            SUusage (1, "suixserver", opt_info.arg);
            norun = 2;
            continue;
        }
        break;
    }
    if (norun)
        return norun - 1;
    argc -= opt_info.index, argv += opt_info.index;

    if (SUwarnlevel > 0 && !fgmode && !logname)
        SUwarnlevel = 0;
    if ((sfd = serverstart (port)) < 0)
        SUerror ("suixserver", "cannot setup server port");
    FD_ZERO (&infds);
    FD_SET (sfd, &infds);
    if (!fgmode) {
        switch (fork ()) {
        case -1:
            SUerror ("suixserver", "fork failed");
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
    for (;;) {
        outfds = infds;
        if (select (FD_SETSIZE, &outfds, NULL, NULL, NULL) < 0)
            SUerror ("suixserver", "select failed");
        if (FD_ISSET (sfd, &outfds)) {
            if (clientadd (sfd) == -1)
                SUerror ("suixserver", "cannot add client");
            continue;
        }
        if (serve (outfds) < 0)
            break;
    }
    serverstop (sfd);
    vmfree (Vmheap, cconns);
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
    struct sockaddr sas;
    socklen_t san;
    cconn_t *cconnp;
    int cconni, cconnj;
    int xcmapi;

    SUwarning (1, "clientadd", "adding client");
    san = sizeof (struct sockaddr);
    if ((cfd = accept (sfd, &sas, &san)) < 0) {
        SUwarning (1, "clientadd", "accept failed");
        return -1;
    }
    SUwarning (
        0, "clientadd", "connection request from %u.%u.%u.%u",
        (unsigned char) sas.sa_data[2], (unsigned char) sas.sa_data[3],
        (unsigned char) sas.sa_data[4], (unsigned char) sas.sa_data[5]
    );
    if (SUauth (SU_AUTH_SERVER, cfd, "SWIFTXSERVER", SWIFTAUTHKEY) == -1) {
        SUwarning (1, "clientadd", "authentication failed");
        return 0;
    }
    SUwarning ( 0, "clientadd", "added client on fd %d", cfd);
    cconni = cfd;
    if (cconni >= cconnn) {
        if (!(cconns = vmresize (
            Vmheap, cconns, (cconni + 1) * sizeof (cconn_t), VM_RSCOPY
        ))) {
            SUwarning (1, "clientadd", "resize failed for cconns");
            return -1;
        }
        for (cconnj = cconnn; cconnj < cconni + 1; cconnj++)
            cconns[cconnj].fd = -1;
        cconnn = cconni + 1;
    }
    if (cconns[cconni].fd != -1) {
        SUwarning (1, "clientadd", "connection exists for fd %d", cfd);
        return -1;
    }
    cconnp = &cconns[cconni];
    cconnp->fd = cfd;
    if (xconnn == 0)
        cconnp->xcmaps = NULL, cconnp->xcmapn = 0;
    else {
        if (!(cconnp->xcmaps = vmresize (
            Vmheap, NULL, xconnn * sizeof (int), VM_RSCOPY
        ))) {
            SUwarning (1, "clientadd", "resize failed for xcmaps");
            return -1;
        }
        cconnp->xcmapn = xconnn;
        for (xcmapi = 0; xcmapi < cconnp->xcmapn; xcmapi++)
            cconnp->xcmaps[xcmapi] = -1;
    }
    if (!(cconnp->fp = sfnew (NULL, NULL, -1, cfd, SF_READ | SF_WRITE))) {
        SUwarning (1, "clientadd", "sfnew failed");
        return -1;
    }
    FD_SET (cfd, &infds);
    SUwarning (1, "clientadd", "client added on fd %d", cfd);
    return cfd;
}

static int clientdelete (int cfd) {
    cconn_t *cconnp;
    int cconni;
    int xconni;

    SUwarning (1, "clientadd", "deleting client on fd %d", cfd);
    if (cfd >= cconnn || cconns[cfd].fd == -1) {
        SUwarning (1, "clientdelete", "bad file descriptor");
        return -1;
    }
    SUwarning (0, "clientdelete", "deleted client on fd %d", cfd);
    cconni = cfd;
    cconnp = &cconns[cconni];
    for (xconni = 0; xconni < xconnn; xconni++)
        if (cconnp->xcmaps[xconni] != -1)
            xserverdelete (cconnp, xconni);
    sfclose (cconnp->fp);
    vmfree (Vmheap, cconnp->xcmaps);
    cconnp->fd = -1;
    FD_CLR (cfd, &infds);
    return 0;
}

static int xserveradd (cconn_t *cconnp, char *xname) {
    Display *xdisp;
    int xfd;
    xconn_t *xconnp;
    int xconni, xconnj;
    int cconni;
    int eventb, errorb, vmajor, vminor;
    Window root;
    int rx, ry;
    unsigned int rw, rh, bw, d;

    SUwarning (1, "xserveradd", "adding xserver");
    for (xconni = 0; xconni < xconnn; xconni++) {
        xconnp = &xconns[xconni];
        if (xconnp->fd != -1 && strcmp (xconnp->name, xname) == 0) {
            xconnp->ref++;
            cconnp->xcmaps[xconni] = 1;
            SUwarning (1, "xserveradd", "xserver ref++ on fd %d", xconni);
            return xconni;
        }
    }
    if (!(xdisp = XOpenDisplay (xname))) {
        SUwarning (1, "xserveradd", "cannot open display");
        return -1;
    }
    if (!XTestQueryExtension (xdisp, &eventb, &errorb, &vmajor, &vminor)) {
        SUwarning (1, "xserveradd", "display does not support extension");
        return -1;
    }
    XTestGrabControl (xdisp, True);
    XSetErrorHandler (errorhandler);
    if (!XGetGeometry (
        xdisp, DefaultRootWindow (xdisp), &root, &rx, &ry, &rw, &rh, &bw, &d
    )) {
        SUwarning (1, "xserveradd", "xgetgeometry failed for root");
        return -1;
    }
    xfd = ConnectionNumber (xdisp);
    xconni = xfd;
    if (xconni >= xconnn) {
        if (!(xconns = vmresize (
            Vmheap, xconns, (xconni + 1) * sizeof (xconn_t), VM_RSCOPY
        ))) {
            SUwarning (1, "xserveradd", "resize failed for xconns");
            return -1;
        }
        for (xconnj = xconnn; xconnj < xconni + 1; xconnj++)
            xconns[xconnj].fd = -1;
        xconnn = xconni + 1;
        for (cconni = 0; cconni < cconnn; cconni++) {
            if (cconns[cconni].fd == -1)
                continue;
            if (!(cconns[cconni].xcmaps = vmresize (
                Vmheap, cconns[cconni].xcmaps, xconnn * sizeof (int), VM_RSCOPY
            ))) {
                SUwarning (1, "xserveradd", "resize failed for xcmaps");
                return -1;
            }
            for (xconnj = cconns[cconni].xcmapn; xconnj < xconnn; xconnj++)
                cconns[cconni].xcmaps[xconnj] = -1;
            cconns[cconni].xcmapn = xconnn;
        }
    }
    xconnp = &xconns[xconni];
    xconnp->fd = xfd;
    xconnp->disp = xdisp;
    xconnp->ref = 1;
    xconnp->name = vmstrdup (Vmheap, xname);
    xconnp->root = DefaultRootWindow (xdisp);
    XSelectInput (
        xdisp, xconnp->root, ExposureMask | VisibilityChangeMask |
        StructureNotifyMask | SubstructureNotifyMask
    );
    xconnp->rx = rx, xconnp->ry = ry, xconnp->rw = rw, xconnp->rh = rh;
    cconnp->xcmaps[xconni] = 1;
#if 0
    if (strcmp (xname, "bilbo:0") == 0)
        xconnp->usewarp = TRUE;
    else
        xconnp->usewarp = FALSE;
#else
        xconnp->usewarp = TRUE;
#endif
    FD_SET (xfd, &infds);
    XSync (xdisp, False);
    SUwarning (1, "xserveradd", "xserver added on fd %d", xfd);
    return xfd;
}

static int xserverdelete (cconn_t *cconnp, int xfd) {
    xconn_t *xconnp;
    int xconni;

    SUwarning (1, "xserverdelete", "deleting xserver on fd %d", xfd);
    if (xfd >= xconnn || xconns[xfd].fd == -1 || cconnp->xcmaps[xfd] == -1) {
        SUwarning (1, "xserverdelete", "bad file descriptor");
        return -1;
    }
    xconni = xfd;
    xconnp = &xconns[xconni];
    cconnp->xcmaps[xconni] = -1;
    if (xconnp->ref > 1) {
        xconnp->ref--;
        SUwarning (1, "xserverdelete", "xserver ref-- on fd %d", xconni);
        return 0;
    }
    XTestGrabControl (xconnp->disp, False);
    XCloseDisplay (xconnp->disp);
    xconnp->fd = -1;
    vmfree (Vmheap, xconnp->name);
    FD_CLR (xfd, &infds);
    return 0;
}

static int xserverinfo (xconn_t *xconnp, cconn_t *onlycconnp) {
    cconn_t *cconnp;
    int cconni;

    SUwarning (1, "xserverinfo", "info for xserver on fd %d", xconnp->fd);
    for (cconni = 0; cconni < cconnn; cconni++) {
        cconnp = &cconns[cconni];
        if (cconnp->fd == -1 || cconnp->xcmaps[xconnp->fd] == -1)
            continue;
        if (onlycconnp && onlycconnp != cconnp)
            continue;
        sfprintf (cconnp->fp, "INFO\n%d\n", xconnp->fd);
    }
    xserverinforec (
        onlycconnp, xconnp, xconnp->root, 0, xconnp->rx, xconnp->ry,
        xconnp->rx + xconnp->rw - 1, xconnp->ry + xconnp->rh - 1
    );
    for (cconni = 0; cconni < cconnn; cconni++) {
        cconnp = &cconns[cconni];
        if (cconnp->fd == -1 || cconnp->xcmaps[xconnp->fd] == -1)
            continue;
        if (onlycconnp && onlycconnp != cconnp)
            continue;
        sfprintf (
            cconnp->fp, "root %d %d %d %d\n", xconnp->rx, xconnp->ry,
            xconnp->rx + xconnp->rw - 1, xconnp->ry + xconnp->rh - 1
        );
    }
    return 0;
}

static int xserverinforec (
    cconn_t *onlycconnp, xconn_t *xconnp, Window win, int level,
    int ox, int oy, int cx, int cy
) {
    Window root, parent, *ws;
    unsigned int wn, wi;
    XWindowAttributes wa;
    int x, y, o2x, o2y, c2x, c2y;
    unsigned int w, h, bw, d;
    cconn_t *cconnp;
    int cconni;

    ws = NULL;
    if (!XQueryTree (xconnp->disp, win, &root, &parent, &ws, &wn)) {
        SUwarning (1, "xserverinforec", "xquerytree failed %x %d", win, level);
        return -1;
    }
    for (wi = 0; wi < wn; wi++) {
        if (!XGetWindowAttributes (xconnp->disp, ws[wi], &wa))
            continue;
        if (wa.class != InputOutput || wa.map_state != IsViewable)
            continue;
        if (!XGetGeometry (
            xconnp->disp, ws[wi], &root, &x, &y, &w, &h, &bw, &d
        ))
            continue;
        if (x < 0)
            w += x, o2x = ox;
        else
            o2x = ox + x;
        if (o2x > cx)
            continue;
        c2x = o2x + w + 2 * bw - 1;
        if (c2x > cx)
            c2x = cx;
        if (y < 0)
            h += y, o2y = oy;
        else
            o2y = oy + y;
        if (o2y > cy)
            continue;
        c2y = o2y + h + 2 * bw - 1;
        if (c2y > cy)
            c2y = cy;
        for (cconni = 0; cconni < cconnn; cconni++) {
            cconnp = &cconns[cconni];
            if (cconnp->fd == -1 || cconnp->xcmaps[xconnp->fd] == -1)
                continue;
            if (onlycconnp && onlycconnp != cconnp)
                continue;
            sfprintf (
                cconnp->fp, "%x %d %d %d %d\n", ws[wi], o2x, o2y, c2x, c2y
            );
        }
        xserverinforec (
            onlycconnp, xconnp, ws[wi], level + 1,
            o2x + bw, o2y + bw, c2x - bw, c2y - bw
        );
    }
    if (ws)
        XFree (ws);
    return 0;
}

static int serve (fd_set fds) {
    int sendinfo;
    xconn_t *xconnp;
    int xconni;
    XAnyEvent *aevp;
    cconn_t *cconnp;
    int cconni;
    int goagain;
    char *line, buf[10000], *avs[10000], *s;
    int avn, i, j, n;
    cmd_t *cmdp;
    int cmdi;

    sendinfo = FALSE;
    for (xconni = 0; xconni < xconnn; xconni++) {
        xconnp = &xconns[xconni];
        if (xconnp->fd == -1 || !FD_ISSET (xconnp->fd, &fds))
            continue;
        while (XPending (xconnp->disp)) {
            XNextEvent (xconnp->disp, &xconnp->ev);
            aevp = (XAnyEvent *) &xconnp->ev;
            switch (aevp->type) {
            case Expose:
                sendinfo = TRUE;
                break;
            case ConfigureNotify:
                sendinfo = TRUE;
                break;
            case MapNotify:
                sendinfo = TRUE;
                break;
            case UnmapNotify:
                sendinfo = TRUE;
                break;
            case CreateNotify:
                sendinfo = TRUE;
            case DestroyNotify:
                sendinfo = TRUE;
                break;
            default:
                sfprintf (
                    sfstdout, "window: %x unknown: %d\n",
                    aevp->window, aevp->type
                );
                break;
            }
            sfprintf (sfstdout, "%d\n", aevp->type);
        }
        if (sendinfo)
            xserverinfo (xconnp, NULL);
    }
    for (cconni = 0; cconni < cconnn; cconni++) {
        cconnp = &cconns[cconni];
        if (cconnp->fd == -1)
            continue;
        if (!FD_ISSET (cconnp->fd, &fds))
            continue;
again:
        goagain = FALSE;
        if ((line = sfgetr (cconnp->fp, '\n', 1))) {
            if (line[strlen (line) - 1] == '\r')
                line[strlen (line) - 1] = 0;
            if (cconnp->fp->_next < cconnp->fp->_endb)
                goagain = TRUE;
            SUwarning (2, "serve", "client: %d line: %s", cconnp->fd, line);
            for (i = j = 0, n = strlen (line); i <= n; i++) {
                if (line[i] == '\\') {
                    if (line[i + 1] == 'n')
                        buf[j++] = '\n', i++;
                    else if ( line[i + 1] == 'r')
                        buf[j++] = '\r', i++;
                    else
                        buf[j++] = line[i];
                } else
                    buf[j++] = line[i];
            }
            if ((avn = tokscan (buf, &s, " %v ", avs, 10000)) < 1) {
                SUwarning (1, "serve", "bad line: %s", line);
                continue;
            }
            for (cmdi = 0; cmds[cmdi].name; cmdi++)
                if (strcmp (cmds[cmdi].name, avs[0]) == 0)
                    break;
            if (!cmds[cmdi].name) {
                SUwarning (1, "serve", "unknown command: %s", avs[0]);
                continue;
            }
            cmdp = &cmds[cmdi];
            if (cmdp->avn != avn) {
                SUwarning (1, "serve", "argn missmatch: %d", avn);
                continue;
            }
            if ((*cmdp->func) (cconnp, avn, avs) == -1) {
                SUwarning (1, "serve", "command %s failed", cmdp->name);
                continue;
            }
            if (goagain)
                goto again;
        } else
            clientdelete (cconnp->fd);
    }
    if (sfsync (NULL) == -1) {
        SUwarning (1, "serve", "sfsync failed");
        return -1;
    }
    return 0;
}

static int cmdopen (cconn_t *cconnp, int avn, char **avs) {
    int xfd;

    if ((xfd = xserveradd (cconnp, avs[1])) == -1) {
        SUwarning (1, "cmdopen", "xserveradd failed");
        sfprintf (cconnp->fp, "FAILURE\n");
        return -1;
    }
    sfprintf (cconnp->fp, "SUCCESS\n%d\n", xfd);
    xserverinfo (&xconns[xfd], cconnp);
    return 0;
}

static int cmdclose (cconn_t *cconnp, int avn, char **avs) {
    if (xserverdelete (cconnp, atoi (avs[1]))) {
        SUwarning (1, "cmdclose", "xserverdelete failed");
        sfprintf (cconnp->fp, "FAILURE\n");
        return -1;
    }
    sfprintf (cconnp->fp, "SUCCESS\n");
    return 0;
}

static int cmdgetinfo (cconn_t *cconnp, int avn, char **avs) {
    int xfd;
    xconn_t *xconnp;
    int xconni;

    xfd = atoi (avs[1]);
    SUwarning (1, "cmdgetinfo", "info for xserver on fd %d", xfd);
    if (xfd >= xconnn || xconns[xfd].fd == -1 || cconnp->xcmaps[xfd] == -1) {
        SUwarning (1, "cmdgetinfo", "bad file descriptor");
        sfprintf (cconnp->fp, "FAILURE\n");
        return -1;
    }
    xconni = xfd;
    xconnp = &xconns[xconni];
    xserverinfo (xconnp, cconnp);
    return 0;
}

static int cmdpointer (cconn_t *cconnp, int avn, char **avs) {
    int xfd;
    xconn_t *xconnp;
    int xconni;
    float fx, fy;
    int x, y;

    xfd = atoi (avs[1]);
    fx = atof (avs[2]), fy = atof (avs[3]);
    if (fx < 0.0)
        fx = 0.0;
    if (fx > 1.0)
        fx = 1.0;
    if (fy < 0.0)
        fy = 0.0;
    if (fy > 1.0)
        fy = 1.0;
    SUwarning (1, "cmdpointer", "pointer warp for xserver on fd %d", xfd);
    if (xfd >= xconnn || xconns[xfd].fd == -1 || cconnp->xcmaps[xfd] == -1) {
        SUwarning (1, "cmdpointer", "bad file descriptor");
        return -1;
    }
    xconni = xfd;
    xconnp = &xconns[xconni];
    x = fx * xconnp->rw, y = fy * xconnp->rh;
    if (xconnp->usewarp) {
        if (!XWarpPointer (
            xconnp->disp, None, xconnp->root, 0, 0, 0, 0, x, y
        )) {
            SUwarning (1, "cmdpointer", "xwarppointer failed");
            return -1;
        }
    } else {
        if (!XTestFakeMotionEvent (
            xconnp->disp, DefaultScreen (xconnp->disp), x, y, 0
        )) {
            SUwarning (1, "cmdpointer", "xtestfakemotionevent failed");
            return -1;
        }
    }
    XSync (xconnp->disp, False);
    return 0;
}

static int cmdbutton (cconn_t *cconnp, int avn, char **avs) {
    int xfd;
    xconn_t *xconnp;
    int xconni;
    int button, pos;

    xfd = atoi (avs[1]);
    button = atoi (avs[2]), pos = atoi (avs[3]);
    if (button < 0 || button > 2) {
        SUwarning (1, "cmdbutton", "bad button id: %d", button);
        return -1;
    }
    if (pos != 0 && pos != 1) {
        SUwarning (1, "cmdbutton", "bad pos: %d", pos);
        return -1;
    }
    SUwarning (1, "cmdbutton", "button fake for xserver on fd %d", xfd);
    if (xfd >= xconnn || xconns[xfd].fd == -1 || cconnp->xcmaps[xfd] == -1) {
        SUwarning (1, "cmdbutton", "bad file descriptor");
        return -1;
    }
    xconni = xfd;
    xconnp = &xconns[xconni];
    if (!XTestFakeButtonEvent (
        xconnp->disp, button + Button1, (pos == 1 ? True : False), 0
    )) {
        SUwarning (1, "cmdbutton", "xwarpbutton failed");
        return -1;
    }
    XSync (xconnp->disp, False);
    return 0;
}

static int cmdkey (cconn_t *cconnp, int avn, char **avs) {
    int xfd;
    xconn_t *xconnp;
    int xconni;
    char *key, *s;
    s2ksmap_t *s2ksmapp;
    int pos;
    KeyCode kc1, kc2;

    xfd = atoi (avs[1]);
    key = avs[2], pos = atoi (avs[3]);
    if (strlen (key) < 1) {
        SUwarning (1, "cmdkey", "bad key: %s", key);
        return -1;
    }
    if (pos != 0 && pos != 1 && pos != 2) {
        SUwarning (1, "cmdkey", "bad pos: %d", pos);
        return -1;
    }
    SUwarning (1, "cmdkey", "key fake for xserver on fd %d", xfd);
    if (xfd >= xconnn || xconns[xfd].fd == -1 || cconnp->xcmaps[xfd] == -1) {
        SUwarning (1, "cmdkey", "bad file descriptor");
        return -1;
    }
    xconni = xfd;
    xconnp = &xconns[xconni];
    for (s = key; *s; s++) {
        if (*s > 127)
            continue;
        s2ksmapp = &s2ksmaps[*s];
        if (s2ksmapp->ks1 != NoSymbol) {
            kc1 = XKeysymToKeycode (xconnp->disp, s2ksmapp->ks1);
            XTestFakeKeyEvent (xconnp->disp, kc1, True, 0);
            if (s2ksmapp->ks2 != NoSymbol) {
                kc2 = XKeysymToKeycode (xconnp->disp, s2ksmapp->ks2);
                XTestFakeKeyEvent (xconnp->disp, kc2, True, 0);
                XTestFakeKeyEvent (xconnp->disp, kc2, False, 0);
            }
            XTestFakeKeyEvent (xconnp->disp, kc1, False, 0);
        }
    }
    XSync (xconnp->disp, False);
    return 0;
}

static int cmdgetstate (cconn_t *cconnp, int avn, char **avs) {
    xconn_t *xconnp;
    int xconni;
    int cconni;

    sfprintf (cconnp->fp, "SUCCESS\n");
    for (xconni = 0; xconni < xconnn; xconni++) {
        xconnp = &xconns[xconni];
        if (xconnp->fd == -1)
            continue;
        sfprintf (
            cconnp->fp, "x: %d %s %d\n", xconni, xconnp->name, xconnp->ref
        );
    }
    for (cconni = 0; cconni < cconnn; cconni++) {
        if (cconns[cconni].fd == -1)
            continue;
        sfprintf (cconnp->fp, "c: %d", cconni);
        for (xconni = 0; xconni < xconnn; xconni++)
            if (cconns[cconni].xcmaps[xconni] != -1)
                sfprintf (cconnp->fp, " %d", xconni);
        sfprintf (cconnp->fp, "\n");

    }
    return 0;
}

static int errorhandler (Display *disp, XErrorEvent *err) {
    SUwarning (1, "errorhandler", "window id gone");
    return 0;
}
