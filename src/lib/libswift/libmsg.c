#pragma prototyped

#include <ast.h>
#include <swift.h>

int SUwarnlevel = 0;
int SUexitcode = 1;

static int SUinited = FALSE;

static void init (void);

void SUerror (char *func, char *fmt, ...) {
    va_list args;
    char *fmt2;

    if (!SUinited)
        init ();
    va_start(args, fmt);
    fmt2 = sfprints ("SWIFT ERROR: (%s %d) %s\n", func, getpid (), fmt);
    sfvprintf (sfstderr, fmt2, args);
    va_end(args);
    exit (SUexitcode);
}

void SUwarning (int level, char *func, char *fmt, ...) {
    va_list args;
    char *fmt2;

    if (!SUinited)
        init ();
    if (level > SUwarnlevel)
        return;
    va_start(args, fmt);
    fmt2 = sfprints ("SWIFT WARNING: (%s %d) %s\n", func, getpid (), fmt);
    sfvprintf (sfstderr, fmt2, args);
    va_end(args);
}

void SUmessage (int level, char *func, char *fmt, ...) {
    va_list args;
    char *fmt2;

    if (!SUinited)
        init ();
    if (level > SUwarnlevel)
        return;
    va_start(args, fmt);
    fmt2 = sfprints ("(%s) %s\n", func, fmt);
    sfvprintf (sfstderr, fmt2, args);
    va_end(args);
}

void SUusage (int unknown, char *name, char *msg) {
    if (!SUinited)
        init ();
    if (unknown)
        sfprintf (sfstderr, "%s: %s\n", name, msg);
    else if (msg[0] == '\f')
        sfprintf (sfstderr, "%s\n", msg + 1);
    else
        sfprintf (sfstderr, "usage: %s %s\n", name, msg);
}

int SUcanseek (Sfio_t *fp) {
    if (sfseek (fp, 0, SEEK_CUR) == -1)
        return FALSE;
    return TRUE;
}

static void init (void) {
    char *s;

    if ((s = getenv ("SWIFTWARNLEVEL")))
        SUwarnlevel = atoi (s);
    if ((s = getenv ("SWIFTEXITCODE")))
        SUexitcode = atoi (s);
    SUinited = TRUE;
}
