#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>

int main (int argc, char **argv) {
    char *s;
    unsigned int t;
    pid_t pid;
    int status;

    if (argc < 2) {
        fprintf (stderr, "usage: %s cmd [arg1 ...]\n", argv[0]);
        return 1;
    }

    t = 0;
    if ((s = getenv ("VGNEWSESSIONTIMEOUT")))
        t = atoi (s);

    if (getpgrp () == getpid ()) {
        switch ((pid = fork ())) {
        case -1:
            fprintf (stderr, "SWIFT ERROR: vgnewsession: cannot fork\n");
            return 1;
        case 0:
            break;
        default:
            waitpid (pid, &status, 0);
            return 0;
        }
    }
    if (setsid () < 0) {
        fprintf (stderr, "SWIFT ERROR: vgnewsession: cannot set sid\n");
        return 1;
    }
    if (t > 0)
        alarm (t);
    execvp (argv[1], argv + 1);
    fprintf (stderr, "SWIFT ERROR: vgnewsession: cannot exec\n");
    return 0;
}
