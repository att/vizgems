#pragma prototyped

#include <ast.h>
#include <swift.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <pwd.h>
#include <netdb.h>
#include <time.h>

typedef struct md5context_t {
    unsigned long a, b, c, d;
    unsigned long n;
    unsigned char data[64];
} md5context_t;

static char hexmap[16] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

static char decmap[] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, 0, 10, 11, 12, 13, 14, 15
};

#define F(X, Y, Z) (((X) & (Y)) | ((~(X)) & (Z)))
#define G(X, Y, Z) (((X) & (Z)) | ((Y) & (~(Z))))
#define H(X, Y, Z) ((X) ^ (Y) ^ (Z))
#define I(X, Y, Z) ((Y) ^ ((X) | (~(Z))))

#define R(v, b) (((v) << (b)) | ((v) >> (32 - (b))))

#define R1(a, b, c, d, k, s, i) { \
    (a) += F ((b), (c), (d)) + idata[k] + T[i]; \
    (a) = R ((a), (s)); (a) += (b); \
}
#define R2(a, b, c, d, k, s, i) { \
    (a) += G ((b), (c), (d)) + idata[k] + T[i]; \
    (a) = R ((a), (s)); (a) += (b); \
}
#define R3(a, b, c, d, k, s, i) { \
    (a) += H ((b), (c), (d)) + idata[k] + T[i]; \
    (a) = R ((a), (s)); (a) += (b); \
}
#define R4(a, b, c, d, k, s, i) { \
    (a) += I ((b), (c), (d)) + idata[k] + T[i]; \
    (a) = R ((a), (s)); (a) += (b); \
}

#define ITOB(b, i) do { \
    (b)[0] = (unsigned char) ((i) & 0xff); \
    (b)[1] = (unsigned char) (((i) >> 8) & 0xff); \
    (b)[2] = (unsigned char) (((i) >> 16) & 0xff); \
    (b)[3] = (unsigned char) (((i) >> 24) & 0xff); \
} while (0)

#define BTOI(i, b) do { \
    *(i) = (b)[0] | ((b)[1] << 8) | ((b)[2] << 16) | ((b)[3] << 24); \
} while (0)

static unsigned long T[64] = {
    0xd76aa478L, 0xe8c7b756L, 0x242070dbL, 0xc1bdceeeL,
    0xf57c0fafL, 0x4787c62aL, 0xa8304613L, 0xfd469501L,
    0x698098d8L, 0x8b44f7afL, 0xffff5bb1L, 0x895cd7beL,
    0x6b901122L, 0xfd987193L, 0xa679438eL, 0x49b40821L,
    0xf61e2562L, 0xc040b340L, 0x265e5a51L, 0xe9b6c7aaL,
    0xd62f105dL,  0x2441453L, 0xd8a1e681L, 0xe7d3fbc8L,
    0x21e1cde6L, 0xc33707d6L, 0xf4d50d87L, 0x455a14edL,
    0xa9e3e905L, 0xfcefa3f8L, 0x676f02d9L, 0x8d2a4c8aL,
    0xfffa3942L, 0x8771f681L, 0x6d9d6122L, 0xfde5380cL,
    0xa4beea44L, 0x4bdecfa9L, 0xf6bb4b60L, 0xbebfbc70L,
    0x289b7ec6L, 0xeaa127faL, 0xd4ef3085L,  0x4881d05L,
    0xd9d4d039L, 0xe6db99e5L, 0x1fa27cf8L, 0xc4ac5665L,
    0xf4292244L, 0x432aff97L, 0xab9423a7L, 0xfc93a039L,
    0x655b59c3L, 0x8f0ccc92L, 0xffeff47dL, 0x85845dd1L,
    0x6fa87e4fL, 0xfe2ce6e0L, 0xa3014314L, 0x4e0811a1L,
    0xf7537e82L, 0xbd3af235L, 0x2ad7d2bbL, 0xeb86d391L
};

static unsigned char pad[64] = { 0x80 };

static void md5init (md5context_t *);
static void md5update (md5context_t *, unsigned char *, unsigned int);
static void md5term (md5context_t *, unsigned char [16]);
static void md5transform (md5context_t *, unsigned char [64]);

static int lenread (int, void *, size_t);
static int lenwrite (int, void *, size_t);
static int nlread (int, void *, size_t);
static int nlwrite (int, void *, size_t);
static int fullread (int, void *, size_t);
static int fullwrite (int, void *, size_t);

int SUencodekey (unsigned char *ikey, unsigned char *okey) {
    unsigned char *is, *os, c;

    for (is = ikey, os = okey; *is; is++, os += 2) {
        c = *is ^ 0x36, c = c ^ 0x5c;
        os[0] = hexmap[(c >> 4) & 0xf];
        os[1] = hexmap[c & 0xf];
    }
    *os = '\000';
    return 0;
}

int SUdecodekey (unsigned char *ikey, unsigned char *okey) {
    unsigned char *is, *os, c;
    for (is = ikey, os = okey; *is; is += 2, os++) {
        c = (decmap[is[0] - '0'] << 4) + decmap[is[1] - '0'];
        c = c ^ 0x5c, *os = c ^ 0x36;
    }
    *os = '\000';
    return 0;
}

int SUauth (int side, int fd, char *service, unsigned char *ekey) {
    fd_set infds, outfds;
    struct timeval tv;
    uid_t uid;
    gid_t gid;
    pid_t pid;
    struct passwd *pwp;
    char host[MAXHOSTNAMELEN + 1];
    char dkey[10000], input[10000], output[33], buf[10000];
    char mode;
    char *swmid, *s1;
    int (*readfunc) (int, void *, size_t);
    int (*writefunc) (int, void *, size_t);

    SUdecodekey (ekey, (unsigned char *) dkey);
    switch (side) {
    case SU_AUTH_CLIENT:
        if (write (fd, "B", 1) != 1) {
            SUwarning (1, "SUauth", "mode write failed");
            return -1;
        }
        if (lenwrite (fd, service, strlen (service) + 1) == -1) {
            SUwarning (1, "SUauth", "service write failed");
            return -1;
        }
        FD_ZERO (&infds);
        FD_SET (fd, &infds);
        outfds = infds;
        tv.tv_sec = 15;
        tv.tv_usec = 0;
        if ((select (FD_SETSIZE, &outfds, NULL, NULL, &tv)) <= 0) {
            SUwarning (1, "SUauth", "timeout from server");
            return -1;
        }
        if (lenread (fd, buf, 10000) == -1) {
            SUwarning (1, "SUauth", "service read failed");
            return -1;
        }
        if (strcmp (buf, "YES") != 0) {
            SUwarning (1, "SUauth", "wrong service");
            return -1;
        }
        uid = getuid (), gid = getgid (), pid = getpid ();
        if (!(pwp = getpwuid (uid))) {
            SUwarning (1, "SUauth", "getpwuid failed");
            return -1;
        }
        if (gethostname (host, MAXHOSTNAMELEN) == -1) {
            SUwarning (1, "SUauth", "gethostname failed");
            return -1;
        }
        if (!(swmid = getenv ("SWMID")))
            swmid = pwp->pw_name;
        sfsprintf (
            input, 10000, "C:%s:%s:%d:%d:%d:%s:%d", swmid,
            pwp->pw_name, uid, gid, pid, host, time (NULL)
        );
        SUhmac (
            (unsigned char *) input, strlen (input),
            (unsigned char *) dkey, strlen (dkey), (unsigned char *) output
        );
        if (lenwrite (fd, input, strlen (input) + 1) == -1) {
            SUwarning (1, "SUauth", "input write failed");
            return -1;
        }
        if (lenwrite (fd, output, strlen (output) + 1) == -1) {
            SUwarning (1, "SUauth", "output write failed");
            return -1;
        }
        if (lenread (fd, buf, 10000) == -1) {
            SUwarning (1, "SUauth", "service read failed");
            return -1;
        }
        if (strcmp (buf, "YES") != 0) {
            SUwarning (1, "SUauth", "wrong service");
            return -1;
        }
        break;
    case SU_AUTH_SERVER:
        if (read (fd, &mode, 1) != 1) {
            SUwarning (1, "SUauth", "mode read failed");
            return -1;
        }
        if (mode == 'B')
            readfunc = lenread, writefunc = lenwrite;
        else
            readfunc = nlread, writefunc = nlwrite;
        if ((*readfunc) (fd, buf, 10000) == -1) {
            SUwarning (1, "SUauth", "service read failed");
            return -1;
        }
        if (strcmp (service, buf) == 0) {
            if ((*writefunc) (fd, "YES", 4) == -1) {
                SUwarning (1, "SUauth", "service write failed");
                return -1;
            }
        } else {
            if ((*writefunc) (fd, "NO", 3) == -1)
                SUwarning (1, "SUauth", "service write failed");
            SUwarning (1, "SUauth", "wrong service %s", buf);
            return -1;
        }
        if ((*readfunc) (fd, input, 10000) == -1) {
            SUwarning (1, "SUauth", "input read failed");
            return -1;
        }
        SUhmac (
            (unsigned char *) input, strlen (input), (unsigned char *) dkey,
            strlen (dkey), (unsigned char *) output
        );
        if ((*readfunc) (fd, buf, 10000) == -1) {
            SUwarning (1, "SUauth", "output read failed");
            return -1;
        }
        if (strcmp (output, buf) == 0) {
            if ((*writefunc) (fd, "YES", 4) == -1) {
                SUwarning (1, "SUauth", "output write failed");
                return -1;
            }
            SUwarning (0, "SUauth", "authentication match %s/%s", input, buf);
        } else {
            if ((*writefunc) (fd, "NO", 3) == -1)
                SUwarning (1, "SUauth", "service write failed");
            SUwarning (1, "SUauth", "authentication failed %s/%s", input, buf);
            return -1;
        }
        strcpy (buf, "SWMIDOVERRIDE=");
        if (!(swmid = strchr (input, ':')))
            break;
        if (!(s1 = strchr (++swmid, ':')))
            break;
        *s1 = 0;
        strncat (buf, swmid, 100);
        putenv (buf);
        break;
    }
    return 0;
}

void SUhmac (
    unsigned char *bufs, int bufn, unsigned char *keys, int keyn,
    unsigned char output[33]
) {
    md5context_t c;
    unsigned char ipad[64], opad[64], key16[16], digest[16];
    int i;

    if (keyn > 64) {
        md5init (&c);
        md5update (&c, keys, keyn);
        md5term (&c, key16);
        keys = key16;
        keyn = 16;
    }

    memset (ipad, 0, sizeof (ipad)), memcpy (ipad, keys, keyn);
    memset (opad, 0, sizeof (opad)), memcpy (opad, keys, keyn);
    for (i = 0; i < 64; i++)
        ipad[i] ^= 0x36, opad[i] ^= 0x5c;

    md5init (&c);
    md5update (&c, ipad, 64);
    md5update (&c, bufs, bufn);
    md5term (&c, digest);

    md5init (&c);
    md5update (&c, opad, 64);
    md5update (&c, digest, 16);
    md5term (&c, digest);

    for (i = 0; i < 16; i++) {
        output[i * 2] = hexmap[(digest[i] >> 4) & 0xf];
        output[i * 2 + 1] = hexmap[digest[i] & 0xf];
    }
    output[32] = 0;
}

static void md5init (md5context_t *cp) {
    cp->n = 0;
    cp->a = 0x67452301L;
    cp->b = 0xefcdab89L;
    cp->c = 0x98badcfeL;
    cp->d = 0x10325476L;
}

static void md5update (
    md5context_t *cp, unsigned char *bufs, unsigned int bufn
) {
    unsigned int m;

    if ((m = cp->n % 64) > 0) {
        if (bufn + m >= 64) {
            memcpy (cp->data + m, bufs, 64 - m);
            md5transform (cp, cp->data);
            cp->n += (64 - m);
            bufs += (64 - m), bufn -= (64 - m);
        } else {
            memcpy (cp->data + m, bufs, bufn);
            cp->n += bufn;
            bufs += bufn, bufn = 0;
        }
    }
    while (bufn >= 64) {
        md5transform (cp, bufs);
        cp->n += 64;
        bufs += 64, bufn -= 64;
    }
    if (bufn > 0) {
        memcpy (cp->data, bufs, bufn);
        cp->n += bufn;
    }
}

static void md5term (md5context_t *cp, unsigned char digest[16]) {
    unsigned int m, n;
    unsigned char len[8];

    n = (cp->n << 3);
    if ((m = cp->n % 64) < 56)
        md5update (cp, pad, 56 - m);
    else
        md5update (cp, pad, 120 - m);
    ITOB (len, n);
    len[4] = len[5] = len[6] = len[7] = 0;
    md5update (cp, len, 8);
    ITOB (digest, cp->a);
    ITOB (digest + 4, cp->b);
    ITOB (digest + 8, cp->c);
    ITOB (digest + 12, cp->d);
}

static void md5transform (md5context_t *cp, unsigned char data[64]) {
    unsigned int idata[16], a, b, c, d, i;

    for (i = 0; i < 64; i += 4)
        BTOI (idata + i / 4, data + i);
    a = cp->a, b = cp->b, c = cp->c, d = cp->d;

    R1 (a, b, c, d,  0,  7,  0);
    R1 (d, a, b, c,  1, 12,  1);
    R1 (c, d, a, b,  2, 17,  2);
    R1 (b, c, d, a,  3, 22,  3);
    R1 (a, b, c, d,  4,  7,  4);
    R1 (d, a, b, c,  5, 12,  5);
    R1 (c, d, a, b,  6, 17,  6);
    R1 (b, c, d, a,  7, 22,  7);
    R1 (a, b, c, d,  8,  7,  8);
    R1 (d, a, b, c,  9, 12,  9);
    R1 (c, d, a, b, 10, 17, 10);
    R1 (b, c, d, a, 11, 22, 11);
    R1 (a, b, c, d, 12,  7, 12);
    R1 (d, a, b, c, 13, 12, 13);
    R1 (c, d, a, b, 14, 17, 14);
    R1 (b, c, d, a, 15, 22, 15);
    R2 (a, b, c, d,  1,  5, 16);
    R2 (d, a, b, c,  6,  9, 17);
    R2 (c, d, a, b, 11, 14, 18);
    R2 (b, c, d, a,  0, 20, 19);
    R2 (a, b, c, d,  5,  5, 20);
    R2 (d, a, b, c, 10,  9, 21);
    R2 (c, d, a, b, 15, 14, 22);
    R2 (b, c, d, a,  4, 20, 23);
    R2 (a, b, c, d,  9,  5, 24);
    R2 (d, a, b, c, 14,  9, 25);
    R2 (c, d, a, b,  3, 14, 26);
    R2 (b, c, d, a,  8, 20, 27);
    R2 (a, b, c, d, 13,  5, 28);
    R2 (d, a, b, c,  2,  9, 29);
    R2 (c, d, a, b,  7, 14, 30);
    R2 (b, c, d, a, 12, 20, 31);
    R3 (a, b, c, d,  5,  4, 32);
    R3 (d, a, b, c,  8, 11, 33);
    R3 (c, d, a, b, 11, 16, 34);
    R3 (b, c, d, a, 14, 23, 35);
    R3 (a, b, c, d,  1,  4, 36);
    R3 (d, a, b, c,  4, 11, 37);
    R3 (c, d, a, b,  7, 16, 38);
    R3 (b, c, d, a, 10, 23, 39);
    R3 (a, b, c, d, 13,  4, 40);
    R3 (d, a, b, c,  0, 11, 41);
    R3 (c, d, a, b,  3, 16, 42);
    R3 (b, c, d, a,  6, 23, 43);
    R3 (a, b, c, d,  9,  4, 44);
    R3 (d, a, b, c, 12, 11, 45);
    R3 (c, d, a, b, 15, 16, 46);
    R3 (b, c, d, a,  2, 23, 47);
    R4 (a, b, c, d,  0,  6, 48);
    R4 (d, a, b, c,  7, 10, 49);
    R4 (c, d, a, b, 14, 15, 50);
    R4 (b, c, d, a,  5, 21, 51);
    R4 (a, b, c, d, 12,  6, 52);
    R4 (d, a, b, c,  3, 10, 53);
    R4 (c, d, a, b, 10, 15, 54);
    R4 (b, c, d, a,  1, 21, 55);
    R4 (a, b, c, d,  8,  6, 56);
    R4 (d, a, b, c, 15, 10, 57);
    R4 (c, d, a, b,  6, 15, 58);
    R4 (b, c, d, a, 13, 21, 59);
    R4 (a, b, c, d,  4,  6, 60);
    R4 (d, a, b, c, 11, 10, 61);
    R4 (c, d, a, b,  2, 15, 62);
    R4 (b, c, d, a,  9, 21, 63);

    cp->a += a, cp->b += b, cp->c += c, cp->d += d;
}

static int lenread (int fd, void *buf, size_t maxlen) {
    unsigned char siz[2];
    size_t len;

    if (fullread (fd, siz, 2) != 2) {
        SUwarning (1, "lenread", "size fullread failed");
        return -1;
    }
    if ((len = (siz[1] << 8) + siz[0]) > maxlen) {
        SUwarning (1, "lenread", "size too big");
        return -1;
    }
    if (fullread (fd, buf, len) != len) {
        SUwarning (1, "lenread", "data fullread failed");
        return -1;
    }
    return len;
}

static int lenwrite (int fd, void *buf, size_t len) {
    unsigned char siz[2];

    siz[0] = len & 0xff;
    siz[1] = (len >> 8) & 0xff;
    if (fullwrite (fd, siz, 2) != 2) {
        SUwarning (1, "lenwrite", "size fullwrite failed");
        return -1;
    }
    if (fullwrite (fd, buf, len) != len) {
        SUwarning (1, "lenwrite", "data fullwrite failed");
        return -1;
    }
    return len;
}

static int nlread (int fd, void *buf, size_t maxlen) {
    int ret;
    size_t len;

    len = 0;
    if (maxlen == 0)
        return 0;
    while ((ret = read (fd, (char *) buf + len, 1)) > 0) {
        if (*((char *) buf + len) == '\n') {
            *((char *) buf + len) = 0;
            break;
        }
        if (++len == maxlen) {
            SUwarning (1, "nlread", "size too big");
            return -1;
        }
    }
    if (ret == -1 && len == 0) {
        SUwarning (1, "nlread", "read failed");
        return -1;
    }
    return len;
}

static int nlwrite (int fd, void *buf, size_t len) {
    size_t off;
    int ret;

    off = 0;
    while ((ret = write (fd, (char *) buf + off, len - off)) > 0)
        if ((off += ret) == len)
            break;
    if (off != len) {
        SUwarning (1, "nlwrite", "data write failed");
        return -1;
    }
    if (write (fd, "\n", 1) != 1) {
        SUwarning (1, "nlwrite", "nl write failed");
        return -1;
    }
    return len;
}

static int fullread (int fd, void *buf, size_t len) {
    size_t off;
    int ret;

    off = 0;
    while ((ret = read (fd, (char *) buf + off, len - off)) > 0)
        if ((off += ret) == len)
            break;
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
