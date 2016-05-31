#pragma prototyped

#include <ast.h>
#include <swift.h>
#include <aggr.h>
#include <aggr_int.h>

#define USAGE "\
fix2 [optional flags] [<infile>]\n\
  optional flags:\n\
    -v\
"

int main (int argc, char **argv) {
    int fd;
    AGGRhdr_t hdr;

    if (AGGRinit () == -1)
        SUerror ("main", "init failed");
    argc--, argv++;
    while (argc > 0 && argv[0][0] == '-') {
        if (strcmp (argv[0], "-v") == 0) {
            SUwarnlevel++;
            argc--, argv++;
        } else
            SUerror ("main", "usage: %s", USAGE);
    }
    for (; argc != 0; argc--, argv++) {
        if ((fd = open (argv[0], O_RDWR)) == -1)
            SUerror ("main", "open failed");
        if (lseek (fd, 0, SEEK_SET) != 0) {
            SUwarning (1, "fix2", "lseek failed");
            return -1;
        }
        if (read (fd, &hdr, sizeof (hdr)) != sizeof (hdr)) {
            SUwarning (1, "fix2", "read failed");
            return -1;
        }
        memcpy (
            (char *) &hdr + AGGR_MAGIC_LEN +
            10 * sizeof (int) + 2 * sizeof (short) + 2 * sizeof (double),
            (char *) &hdr + AGGR_MAGIC_LEN +
            9 * sizeof (int) + 2 * sizeof (short) + 2 * sizeof (double),
            sizeof (AGGRhdr_t) - (
                AGGR_MAGIC_LEN +
                10 * sizeof (int) + 2 * sizeof (short) + 2 * sizeof (double)
            )
        );
        hdr.vallen = AGGRtypelens[hdr.valtype];
        if (lseek (fd, 0, SEEK_SET) != 0) {
            SUwarning (1, "fix2", "lseek failed");
            return -1;
        }
        if (write (fd, &hdr, sizeof (hdr)) != sizeof (hdr)) {
            SUwarning (1, "fix2", "write failed");
            return -1;
        }
        close (fd);
    }
    return 0;
}
