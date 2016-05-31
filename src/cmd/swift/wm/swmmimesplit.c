#pragma prototyped

#include <ast.h>
#include <error.h>

static char buffer[1000000];
static char *start, *end;

static char buf2[2];
static int count2, wrotenl;

static int binmode;

static int init (int);
static char *getcurr (void);
static int getsize (void);
static char *advance (int);
static int ensure (int);
static int ensurechar (char c);
static int myputc (Sfio_t *, char);

int main (int argc, char **argv) {
    char *ctype, *s1, *s2, *s3, *boundary, *eof, *name;
    Sfio_t *fp;
    int state, boundaryn, eofn, atnl, i, n;
    char c1, c2;

    if (!(ctype = getenv ("CONTENT_TYPE")))
        error (3, "cannot find CONTENT_TYPE env. variable");
    if (!(s1 = strchr (ctype, ';')))
        error (3, "no field divider");
    *s1++ = 0;
    if (!(s2 = strstr (s1, "boundary=")))
        error (3, "no boundary field");
    s2 += strlen ("boundary=");
    boundary = strdup (sfprints ("--%s", s2));
    boundaryn = strlen (boundary);
    eof = strdup (sfprints ("%s--", boundary));
    eofn = strlen (eof);

    state = 0;
    name = NULL;
    fp = NULL;
    init (eofn);
    for (;;) {
next:
        if (state == 0) { /* before eof or boundary */
            if (ensurechar ('\n') == -1)
                error (3, "cannot find NL");
            s1 = getcurr ();
            if (strncmp (s1, eof, eofn) == 0)
                break;
            if (strncmp (s1, boundary, boundaryn) == 0) {
                if (s1[boundaryn] == '\r' && s1[boundaryn + 1] == '\n')
                    advance (boundaryn + 2);
                else if (s1[boundaryn] == '\n')
                    advance (boundaryn + 1);
                else
                    error (3, "no CR+NL or NL after boundary");
            } else
                error (3, "boundary or eof not found");
            binmode = 0;
            state = 1;
        } else if (state == 1) { /* at header section */
            if (ensurechar ('\n') == -1)
                error (3, "cannot find NL");
            s1 = getcurr ();
            if (*s1 == '\r' && *(s1 + 1) == '\n') {
                advance (2);
                state = 2;
                continue;
            } else if (*s1 == '\n') {
                advance (1);
                state = 2;
                continue;
            }
            s3 = strchr (s1, '\n');
            c1 = *s3, *s3 = 0;
            if (strstr (s1, "Content-Type: "))
                binmode = 1;
            if ((s2 = strstr (s1, " name=\""))) {
                name = s2 + 7;
                if (!(s2 = strchr (name, '"')))
                    error (3, "cannot find matching \" for name");
                c2 = *s2; *s2 = 0;
                name = strdup (name);
                *s2 = c2;
                if (!(fp = sfopen (NULL, name, "a")))
                    error (3, "sfopen for name %s failed", name);
            }
            *s3 = c1;
            advance (s3 - s1 + 1);
        } else if (state == 2) {
            atnl = 1;
            wrotenl = 0;
            for (;;) {
next2:
                ensure (eofn);
                if ((n = getsize ()) < eofn)
                    error (3, "unexpected EOF");
                s1 = getcurr ();
                if (atnl == 1 && strncmp (s1, boundary, boundaryn) == 0) {
                    if (count2 > 1 && buf2[0] != '\r')
                        sfputc (fp, buf2[0]);
                    if (!binmode && !wrotenl)
                        sfputc (fp, '\n');
                    count2 = 0;
                    sfclose (fp);
                    state = 0;
                    goto next;
                }
                atnl = 0;
                for (i = 0; i < n; i++) {
                    myputc (fp, s1[i]);
                    if (s1[i] == '\n') {
                        atnl = 1;
                        advance (i + 1);
                        goto next2;
                    }
                }
                advance (n);
            }
        }
    }
    return 0;
}

static int init (int count) {
    int n;

    if ((n = sfread (sfstdin, buffer, count)) < 1)
        error (3, "cannot read initial buffer");
    start = &buffer[0];
    end = start + n;
    return 0;
}

static char *getcurr (void) {
    return start;
}

static int getsize (void) {
    return end - start;
}

static char *advance (int count) {
    ensure (count);
    if (start + count > end)
        return NULL;
    start += count;
    ensure (1);
    return start;
}

static int ensure (int count) {
    int n, i;

    if (count > 1000000)
        error (3, "cannot ensure more than 1000000 bytes");
    if (start + count - 1 < end)
        return 0;
    n = end - start;
    if (start != buffer)
        for (i = 0; i < n; i++)
            buffer[i] = start[i];
    start = buffer, end = buffer + n;
    if ((n = sfread (sfstdin, end, count - n)) == -1)
        error (3, "cannot fill buffer");
    end += n;
    if (end - start < count)
        return -1;
    return 0;
}

static int ensurechar (char c) {
    char *s;
    int i;

    for (s = start; s != end; s++)
        if (*s == c)
            return 0;
    for (i = 1; i < 1000000; i++) {
        ensure (i);
        for (s = start; s != end; s++)
            if (*s == c)
                return 0;
    }
    return -1;
}

static int myputc (Sfio_t *fp, char c) {
    if (count2 < 2) {
        buf2[count2++] = c;
        return 0;
    }
    wrotenl = (buf2[0] == '\n') ? 1 : 0;
    sfputc (fp, buf2[0]), buf2[0] = buf2[1], buf2[1] = c;
    if (!binmode && buf2[1] == '\n' && buf2[0] == '\r')
        buf2[0] = buf2[1], count2 = 1;
    return 0;
}
