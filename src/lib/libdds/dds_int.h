#pragma prototyped

#ifndef _DDS_INT_INCLUDE
#define _DDS_INT_INCLUDE

#include <ast.h>
#include <vcodex.h>
#include <swift.h>
#include <dds.h>

#ifdef FEATURE_NO64FS
typedef off_t off64_t;
#endif

#define MAGIC "DDSCHEMA"
#define VERSION 4

#if SWIFT_LITTLEENDIAN == 1
static unsigned char *__cp, __c;
#define SW2(sp) ( \
    __cp = (char *) (sp), \
    __c = __cp[0], __cp[0] = __cp[1], __cp[1] = __c \
)
#define SW4(ip) ( \
    __cp = (char *) (ip), \
    __c = __cp[0], __cp[0] = __cp[3], __cp[3] = __c, \
    __c = __cp[1], __cp[1] = __cp[2], __cp[2] = __c \
)
#define SW8(dp) ( \
    __cp = (char *) (dp), \
    __c = __cp[0], __cp[0] = __cp[7], __cp[7] = __c, \
    __c = __cp[1], __cp[1] = __cp[6], __cp[6] = __c, \
    __c = __cp[2], __cp[2] = __cp[5], __cp[5] = __c, \
    __c = __cp[3], __cp[3] = __cp[4], __cp[4] = __c \
)
#else
#define SW2(ip) (ip)
#define SW4(sp) (sp)
#define SW8(dp) (dp)
#endif

_BEGIN_EXTERNS_ /* public data */
#if _BLD_dds && defined(__EXPORT__)
#define __PUBLIC_DATA__ __EXPORT__
#else
#if !_BLD_dds && defined(__IMPORT__)
#define __PUBLIC_DATA__ __IMPORT__
#else
#define __PUBLIC_DATA__
#endif
#endif

extern __PUBLIC_DATA__ char **_ddsfieldtypestring;
extern __PUBLIC_DATA__ int *_ddsfieldtypesize;

#undef __PUBLIC_DATA__

#if _BLD_dds && defined(__EXPORT__)
#define extern  __EXPORT__
#endif

extern char *_ddscreateso (char *, char *, char *, char *, char *);
extern int _ddswritestruct (DDSschema_t *, char *, Sfio_t *);
extern char *_ddscopystr (Sfio_t *);
extern int _ddslist2fields (
    char *, DDSschema_t *, DDSfield_t **, DDSfield_t ***, int *
);

typedef struct vczwin_t {
    int uflag;
    Sfoff_t coff, uoff;
    ssize_t csize, usize;
    char *datap;
    int datan, datal;
} vczwin_t;

typedef struct vczi_t {
    Sfdisc_t disc;
    Sfio_t *fp;
    char *memp;
    Vcodex_t *vcp;
    vczwin_t *vczwins;
    int vczwinn, vczwinm, vczwinl, vczwini, vczwinc;
    Sfoff_t baseoff, endoff, ccoff, cuoff, rendoff;
    char *datap;
    int datan;
    int recflag;
    Vmalloc_t *vm;
    char *rtschemap;
} vczi_t;

typedef struct vczo_t {
    Sfdisc_t disc;
    Sfio_t *fp;
    Vcodex_t *vcp;
    vczwin_t *vczwins;
    int vczwinn, vczwinl;
    vczwin_t vczwin;
    int recflag, finished;
    Vmalloc_t *vm;
    char *rtschemap;
} vczo_t;

extern char *_ddsvczgenrdb (DDSschema_t *);
extern char *_ddsvczgenrtb (DDSschema_t *);
extern vczi_t *_ddsvczopeni (Sfio_t *, char *);
extern int _ddsvczclosei (Sfio_t *, vczi_t *);
extern vczo_t *_ddsvczopeno (Sfio_t *, char *, int);
extern int _ddsvczcloseo (Sfio_t *, vczo_t *);

#undef extern
_END_EXTERNS_

#endif /* _DDS_INT_INCLUDE */
