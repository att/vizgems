#pragma prototyped

#include <ast.h>
#include <swift.h>
#include <sys/types.h>
#include <sys/param.h>
#include <pwd.h>
#include <netdb.h>
#include <time.h>
#include "key.h"

int main (int argc, char **argv) {
    uid_t uid;
    gid_t gid;
    pid_t pid;
    struct passwd *pwp;
    char host[MAXHOSTNAMELEN + 1];
    unsigned char dkey[10000], input[10000], output[33];
    char *swmid, *line;

    argc--, argv++;
    SUdecodekey (SWIFTAUTHKEY, dkey);
    sfputr (sfstderr, "enter customer info", '\n');
    if (!(line = sfgetr (sfstdin, '\n', 1)) || !line[0])
        return 0;
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
        (char *) input, 10000, "C:%s:%s:%d:%d:%d:%s:%d:%s", swmid,
        pwp->pw_name, uid, gid, pid, host, time (NULL), line
    );
    SUhmac (
        input, strlen ((char *) input), dkey, strlen ((char *) dkey), output
    );
    sfprintf (sfstdout, "plain: %s\n", input);
    sfprintf (sfstdout, "key: %s\n", output);
    return 0;
}
