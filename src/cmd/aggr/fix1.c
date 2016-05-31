#pragma prototyped

#include <ast.h>
#include <swift.h>
#include <aggr.h>

#define USAGE "\
fix1 [optional flags] [<infile>]\n\
  optional flags:\n\
    -kt <type>          (default: string)\n\
  type:\n\
    (char|short|int|llong|float|double|string) [<len>]\n\
    length must be specified when first letter is capitalized\n\
    string cannot be used in -vt\n\
    -v\
"

int main (int argc, char **argv) {
    int argn;
    int keytype, keylen;
    int fd;
    AGGRhdr_t hdr;

    if (AGGRinit () == -1)
        SUerror ("main", "init failed");
    keytype = AGGR_TYPE_STRING;
    keylen = -1;
    argc--, argv++;
    while (argc > 0 && argv[0][0] == '-') {
        if (strcmp (argv[0], "-kt") == 0) {
            argc--, argv++;
            if ((argn = _aggrscantype (argc, argv, &keytype, &keylen)) == -1)
                SUerror ("main", "usage: %s", USAGE);
            argc -= argn, argv += argn;
        } else if (strcmp (argv[0], "-v") == 0) {
            SUwarnlevel++;
            argc--, argv++;
        } else
            SUerror ("main", "usage: %s", USAGE);
    }
    for (; argc != 0; argc--, argv++) {
        if ((fd = open (argv[0], O_RDWR)) == -1)
            SUerror ("main", "open failed");
        if (lseek (fd, 0, SEEK_SET) != 0) {
            SUwarning (1, "fix1", "lseek failed");
            return -1;
        }
        if (read (fd, &hdr, sizeof (hdr)) != sizeof (hdr)) {
            SUwarning (1, "fix1", "read failed");
            return -1;
        }
        hdr.keytype = keytype;
        if (lseek (fd, 0, SEEK_SET) != 0) {
            SUwarning (1, "fix1", "lseek failed");
            return -1;
        }
        if (write (fd, &hdr, sizeof (hdr)) != sizeof (hdr)) {
            SUwarning (1, "fix1", "write failed");
            return -1;
        }
        close (fd);
    }
    return 0;
}
