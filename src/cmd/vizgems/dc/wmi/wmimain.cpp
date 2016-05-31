
#include <Wbemidl.h>
#include <ast/sfio.h>

#include "wmi.h"
#include "wmiv2s.h"
#include "wmiarg.h"

int verbose;

static int usage (char *, char *, int);

int main (int argc, char **argv) {
  state_t *sp = NULL;
  char *host = ".", *ns = "root\\cimv2";
  char *user = getenv ("WMIUSER"), *passwd = getenv ("WMIPASSWD");
  char *sfile = "sfile", *tfile = NULL, *lfile = NULL, *ifile = NULL;
  int viflag;

  verbose = FALSE;

  if (!(sp = SWMIopen ()))
    exit (1);

  ARGBEGIN {
  case 'h': host = ARGF();                  break;
  case 'n': ns = ARGF();                    break;
  case 'u': user = ARGF();                  break;
  case 'p': passwd = ARGF();                break;
  case 's': sfile = ARGF();                 break;
  case 't': tfile = ARGF();                 break;
  case 'i': ifile = ARGF(), viflag = FALSE; break;
  case 'I': ifile = ARGF(), viflag = TRUE;  break;
  case 'l': lfile = ARGF();                 break;
  case 'v': verbose = TRUE;                 break;
  default: exit (usage (argv0, __FILE__, __LINE__));
  } ARGEND

  if (SWMIconnect (sp, host, ns, user, passwd) < 0)
    exit(1);

  if (tfile) {
    if (
      SWMIopnload (tfile, sfile) == -1 ||
      SWMIopnexec (sp) == -1 ||
      SWMIopnsave (sfile) == -1
    )
      sfprintf (sfstderr, "OPN operation failed\n");
  }
  if (lfile) {
    if (
      SWMIlogload (lfile, sfile) == -1 ||
      SWMIlogexec (sfile, sp) == -1 ||
      SWMIlogsave (sfile) == -1
    )
      sfprintf (sfstderr, "LOG operation failed\n");
  }
  if (ifile) {
    if (
      SWMIinvload (ifile) == -1 ||
      SWMIinvexec (sp, viflag) == -1 ||
      SWMIinvsave () == -1
    )
      sfprintf (sfstderr, "INV operation failed\n");
  }

  SWMIclose(sp);
}

static int usage (char* pn, char *f, int l) {
  if (f)
    sfprintf (
      sfstderr,
      "%s: %d\n"
      "Usage: %s [-u url] [-q] [-l user -p passwd]"
      " -o objectspec || -n enumspec || -w wquery || -e estream\n",
      f, l, pn
    );
  return 1;
}
