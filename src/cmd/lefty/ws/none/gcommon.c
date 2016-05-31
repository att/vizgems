#pragma prototyped
/* Lefteris Koutsofios - AT&T Labs Research */

#include "common.h"
#include "g.h"
#include "gcommon.h"
#include "mem.h"

int Gxfd;
Widget Groot;
Display *Gdisplay;
int Gpopdownflag;
int Gscreenn;
int Gdepth;

int Ginitgraphics (void) {
    return 0;
}

int Gtermgraphics (void) {
    return 0;
}

int Gsync (void) {
    return 0;
}

int Gresetbstate (int wi) {
    return 0;
}

int Gprocessevents (int waitflag, int mode) {
    return 0;
}
