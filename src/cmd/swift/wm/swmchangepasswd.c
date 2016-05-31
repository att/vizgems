/*
** Original code by Rob McCool, robm@ncsa.uiuc.edu.
**
** 06/28/95: Carlos Varela, cvarela@isr.co.jp
** 1.1 : Additional error message if password file not changed.
**       By default allows password addition, better feedback to "wizard".
**
*/

#include <ast.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/signal.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>


#define USER_FILE "/fs/swift/www/current/.passwd"
#define WIZARD "Admin"
#define PLEN	6
#define START_BODY	"<BODY BACKGROUND=\"/logos/gops.gif\">"
#define END_BODY	"</BODY>"

char *makeword(char *line, char stop);
char *fmakeword(FILE *f, char stop, int *len);
char x2c(char *what);
void unescape_url(char *url);
void plustospace(char *str);
static void err(const char *e, ...);
static void fail(const char *format, ...);

char *crypt(const char *pw, const char *salt); /* why aren't these prototyped in include */


char *tn;

/* From local_passwd.c (C) Regents of Univ. of California blah blah */
static unsigned char itoa64[] =         /* 0 ... 63 => ascii - 64 */
        "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

to64(s, v, n)
  register char *s;
  register long v;
  register int n;
{
    while (--n >= 0) {
        *s++ = itoa64[v&0x3f];
        v >>= 6;
    }
}

void change_password(char *user, char *pw, FILE *f) {
    char *cpw, salt[3];

    (void)srand((int)time((time_t *)NULL));
    to64(&salt[0],rand(),2);
    cpw = crypt(pw,salt);
    free(pw);
    fprintf(f,"%s:%s\n",user,cpw);
}

void putline(FILE *f,char *l) {
    int x;

    for(x=0;l[x];x++) fputc(l[x],f);
    fputc('\n',f);
}

/*------------------------------------------------------------*/

/* Form lock-file name from filename. */
static char *
lockfn(char *fn)
{
    /* Lock file name is ".lock" appended to fn. */
    int n = strlen(fn);
    char *s = (char *) malloc(n + 5 + 1);

    if (s == 0)
        fail("can't allocate space for lock file");
    sprintf(s, "%s.lock", fn);
    return s;
}


/* Create lock file to prevent simultaneous access.  fn is name of
** lock file (from lockfn).  Return file descriptor.
*/
static int
lockfile(char *fn)
{
    int fd;

    while ((fd = open(fn, O_CREAT | O_EXCL, 0660)) < 0) {
        if (errno != EEXIST)
            fail("can't create lock file: %s", fn);
        sleep(1);
    }
    unlink(fn);                         /* so it gets deleted on close */
    return fd;
}

static void
unlockfile(int fd)
{
    close(fd);
    return;
}

/*------------------------------------------------------------*/

main(int argc, char *argv[]) {
    register int x;
    int cl,found,create,plen,ulen;
    char *u,*t1,*t2,*p1,*p2,*user, command[256], line[256], l[256], w[256];
    FILE *tfp,*f;

    tn = NULL;

    printf("Content-type: text/html%c%c",10,10);

    if(strcmp(getenv("REQUEST_METHOD"),"POST")) {
        printf("This script should be referenced with a METHOD of POST.\n");
        printf("If you don't understand this, see this ");
        printf("<A HREF=\"http://www.ncsa.uiuc.edu/SDG/Software/Mosaic/Docs/fill-out-forms/overview.html\">forms overview</A>.%c",10);
        exit(1);
    }
    if(strcmp(getenv("CONTENT_TYPE"),"application/x-www-form-urlencoded")) {
        printf("This script can only be used to decode form results. \n");
        exit(1);
    }
    cl = atoi(getenv("CONTENT_LENGTH"));

    user=NULL;
    p1=NULL;
    p2=NULL;
    create=1;
    for(x=0;cl && (!feof(stdin));x++) {
        t1 = fmakeword(stdin,'&',&cl);
        t2 = makeword(t1,'=');
        unescape_url(t1);
        unescape_url(t2);
        if(!strcmp(t2,"user")) {
            if(!user)
                user = t1;
            else {
                printf("This script was accessed from the wrong form.\n");
                exit(1);
            }
        }
        else if(!strcmp(t2,"newpasswd1")) {
            if(!p1)
                p1 = t1;
            else {
                printf("This script was accessed from the wrong form.\n");
                exit(1);
            }
        }
        else if(!strcmp(t2,"newpasswd2")) {
            if(!p2)
                p2 = t1;
            else {
                printf("This script was accessed from the wrong form.\n");
                exit(1);
            }
        }
        else {
            printf("This script was accessed from the wrong form.\n");
            printf("Unrecognized directive %s.\n",t2);
            exit(1);
        }
        free(t2);
    }
    u=getenv("REMOTE_USER");


    if((strcmp(u,WIZARD)) && (strcmp(user,u))) {
            printf("<TITLE>User Mismatch</TITLE>%s",START_BODY);
            printf("<H1>User Mismatch</H1>");
            printf("The username you gave does not correspond with the ");
            printf("user you authenticated as.\n%s",END_BODY);
            exit(1);
        }
    if(strcmp(p1,p2)) {
        printf("<TITLE>Password Mismatch</TITLE>%s",START_BODY);
        printf("<H1>Password Mismatch</H1>");
        printf("The two copies of your password do not match. Please");
        printf(" try again.%s",END_BODY);
        exit(1);
    }

    /* check that lengths are okay */


    if ( (ulen=strlen(user)) == 0 )
    {
        printf("<TITLE>User Name Expected</TITLE>%s",START_BODY);
        printf("<H1>User Name Expected</H1>");
	printf("Your <I>user name</I> expected (%d).<br>", ulen);
        printf(" Try again.%s",END_BODY);
	exit(1);
    }

    if ( (plen=strlen(p1)) < PLEN )
    {
        printf("<TITLE>Password Length too Small</TITLE>%s",START_BODY);
        printf("<H1>Password Length too Small</H1>");
	printf("Your <I>new password</I> must at least %d characters.<br>",PLEN);
        printf(" Try again.%s",END_BODY);
        exit(1);
    }

    tn = tmpnam(NULL);
    if(!(tfp = fopen(tn,"w"))) {
        fprintf(stderr,"Could not open temp file.\n");
        exit(1);
    }

    if(!(f = fopen(USER_FILE,"r"))) {
        fprintf(stderr,
                "Could not open passwd file for reading.\n",USER_FILE);
        exit(1);
    }

    found = 0;
    while(!(getline(line,256,f))) {
        if(found || (line[0] == '#') || (!line[0])) {
            putline(tfp,line);
            continue;
        }
        strcpy(l,line);
        getword(w,l,':');
        if(strcmp(user,w)) {
            putline(tfp,line);
            continue;
        }
        else {
            change_password(user,p1,tfp);
            found=1;
        }
    }
    if((!found) && (create))
        change_password(user,p1,tfp);
    fclose(f);
    fclose(tfp);

    { /* update the .passwd file */
    int lf;
    lf = lockfile(lockfn(USER_FILE));

    /* backup the password file */

    sprintf(command,"cp %s %s.bkup",USER_FILE, USER_FILE);
    if (system(command)) {
    	unlockfile(lf);
	fprintf(stderr, "Could not backup the passwd file.\n",USER_FILE);
	exit(1);
    }

    sprintf(command,"cp %s %s",tn,USER_FILE);
    if (system(command)) {
    	unlockfile(lf);
	fprintf(stderr, "Could not overwrite passwd file.\n",USER_FILE);
	exit(1);
    }
    unlink(tn);
    unlockfile(lf);
    }

    if ((!found) && (create)) {
	printf("<TITLE>Successful Addition</TITLE>%s",START_BODY);
	printf("<H1>Successful Addition</H1>");
	printf("User <b>%s</b> has been successfully added.<P>",user);
    } else {
	printf("<TITLE>Successful Change</TITLE>%s",START_BODY);
	printf("<H1>Successful Change</H1>");
	printf("Your password has been successfully changed.<P>");
    }
    printf("%s", END_BODY);
    exit(0);
}

static void
fail(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    (void) vfprintf(stderr, fmt, args);
    fputc('\n', stderr);
    va_end(args);
    err("An internal server error occurred.");
}

static void
err(const char *e, ...)
{
    va_list args;
    va_start(args, e);
    printf("<H1>Sorry, failed.</H1>\n");
    printf("<EM>");
    vprintf(e, args);
    printf("</EM>\n");
    va_end(args);
    printf("<BR>\n");
    exit(1);
}

