#include <ast.h>
#include <cdt.h>
#include <dt.h>
#include <swift.h>
#include <sys/types.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <errno.h>
#define FUSE_USE_VERSION 26
#define _FILE_OFFSET_BITS 64
#include <fuse.h>

extern void TSUwarning (int, char *, char *, ...);
#define SUwarning TSUwarning
extern void TSUerror (char *, char *, ...);
#define SUerror TSUerror

#ifdef _BLD_DEBUG
#define VMFLAGS VM_TRACE
#define LOCK(m) ( \
  TSUwarning (2, "debug", "about to lock %x %s %d", &m, __FILE__, __LINE__), \
  pthread_mutex_lock (&m) \
)
#define UNLOCK(m) ( \
  TSUwarning (2, "debug", "about to unlock %x %s %d", &m, __FILE__, __LINE__), \
  pthread_mutex_unlock (&m) \
)
#else
#define VMFLAGS 0
#define LOCK(m) pthread_mutex_lock (&m)
#define UNLOCK(m) pthread_mutex_unlock (&m)
#endif

extern Vmalloc_t *vm;

#define NDFS_FLAG_TLO 1
#define NDFS_ALLFLAGS (NDFS_FLAG_TLO)
#define NDFS_HASFLAG_TLO(f) ((f) & NDFS_FLAG_TLO)

#define NDFS_ISDIR 1
#define NDFS_ISREG 2

typedef struct ndfsdirinfo_s {
    Dtlink_t link;
    /* begin key */
    char *name;
    /* end key */
    int opqflag;
    ino_t ino;
    off_t off;
    unsigned char type;
} ndfsdirinfo_t;

typedef struct ndfsfile_s {
    int type;
    int fd;
    union {
        struct {
            int inited;
            ndfsdirinfo_t **dirinfops;
            int dirinfopn;
        } d;
        struct {
            int flags;
        } r;
    } u;
} ndfsfile_t;

typedef struct ndfspid_s {
    Dtlink_t link;
    /* begin key */
    pid_t pid;
    /* end key */
    int flags;
} ndfspid_t;

#define NDFS_SPECIALPATH_NOT 0
#define NDFS_SPECIALPATH_DEL 1
#define NDFS_SPECIALPATH_TMP 2
#define NDFS_SPECIALPATH_TLO 3
#define NDFS_SPECIALPATHSIZE 4

#define SETFSOWNER { \
    if (uid == 0) { \
        setfsuid (fuse_get_context ()->uid); \
        setfsgid (fuse_get_context ()->gid); \
    } \
}

extern char *tprefixstr, *bprefixstr, *pprefixstr;
extern int tprefixlen, bprefixlen, pprefixlen;
extern int dfd;
extern time_t currtime;
extern uid_t uid;
extern gid_t gid;
extern pthread_mutex_t mutex;

// from ndfsops.c

extern int ndfsstatfs (const char *, struct statvfs *);
extern int ndfsmkdir (const char *, mode_t);
extern int ndfsrmdir (const char *);
extern int ndfslink (const char *, const char *);
extern int ndfssymlink (const char *, const char *);
extern int ndfsunlink (const char *);
extern int ndfsreadlink (const char *, char *, size_t);
extern int ndfschmod (const char *, mode_t);
extern int ndfsutimens (const char *, const struct timespec *);
extern int ndfsrename (const char *, const char *);
extern int ndfsaccess (const char *, int);
extern int ndfsgetattr (const char *, struct stat *);
extern int ndfsfgetattr (const char *, struct stat *, struct fuse_file_info *);

extern int ndfsopendir (const char *, struct fuse_file_info *);
extern int ndfsreaddir (
    const char *, void *, fuse_fill_dir_t, off_t, struct fuse_file_info *
);
extern int ndfsreleasedir (const char *, struct fuse_file_info *);
extern int ndfscreate (const char *, mode_t, struct fuse_file_info *);
extern int ndfsopen (const char *, struct fuse_file_info *);
extern int ndfsflush (const char *, struct fuse_file_info *);
extern int ndfsrelease (const char *, struct fuse_file_info *);
extern int ndfsread (
    const char *, char *, size_t, off_t, struct fuse_file_info *
);
extern int ndfswrite (
    const char *, const char *, size_t, off_t, struct fuse_file_info *
);
extern int ndfstruncate (const char *, off_t);
extern int ndfsftruncate (const char *, off_t, struct fuse_file_info *);
extern int ndfsfsync (const char *, int, struct fuse_file_info *);
extern int ndfsioctl (
    const char *, int, void *, struct fuse_file_info *, unsigned int, void *
);

// from ndfsutils.c

extern int utilinit (void);
extern int utilterm (void);

extern int utilgetrealfile (const char *, char *);
extern int utilgetbottomfile (const char *, char *);
extern int utilensuretopfile (const char *);
extern int utilensuretopfileparent (const char *);

extern int utilinitdir (ndfsfile_t *, char *, char *);
extern int utiltermdir (ndfsfile_t *);

extern int utilmkspecialpath (const char *, int, char *);
extern int utilisspecialpath (const char *, int *);
extern int utilisspecialname (const char *, int *, char **);

extern int utilmkspecialpid (pid_t, int, int);
extern int utilrmspecialpid (pid_t);
extern int utilisspecialpid (pid_t, int *);

extern ssize_t bvprintf (char **, char *, const char *, va_list ap);
extern ssize_t bprintf (char **, char *, const char *, ...);
extern ssize_t bprintf1 (char *, ssize_t, const char *, ...);
