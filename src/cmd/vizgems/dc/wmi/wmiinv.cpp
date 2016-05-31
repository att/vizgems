/*
 * SWIFT WMI interface
 * purpose: querying and dumping WMI resources
 * author: Don Caldwell dfwc@research.att.com
 & edited by: Lefteris Koutsofios ek@research.att.com
 */

#pragma comment(lib, "wbemuuid.lib")

#include <comutil.h>
#include <wbemidl.h>
#include <ast/sfio.h>
#include <ast/cdt.h>
#include <ast/vmalloc.h>

#include "wmi.h"
#include "wmiv2s.h"

static inline const char *b2s (BSTR b) {
  return _com_util::ConvertBSTRToString (b);
}
static inline BSTR s2b (const char* ccp) {
  return _com_util::ConvertStringToBSTR (ccp);
}

typedef struct obj_t {
  Dtlink_t link;
  /* begin key */
  int index;
  /* end key */
  char *obj, *np, *ep, *ip;
} obj_t;

static Dt_t *objdict;
static Dtdisc_t objdisc = {
    sizeof (Dtlink_t),sizeof (int), 0, NULL, NULL, NULL, NULL, NULL, NULL
};

typedef struct kvt_t {
  char *kp, *vp;
  int type;
  unsigned long long v;
} kvt_t;

static kvt_t *kvts;
static int kvtl, kvtn;

static obj_t *objinsert (char *, char *, char *, char *);

int SWMIinvload (char *ifile) {
  Sfio_t *ifp;
  obj_t *objp;
  char *line, *op, *np, *ep, *ip;

  if (!(objdict = dtopen (&objdisc, Dtset))) {
    sfprintf (sfstderr, "cannot create obj dict\n");
    return -1;
  }

  if (strcmp (ifile, "-") == 0)
    ifp = sfstdin;
  else if (!(ifp = sfopen (NULL, ifile, "r"))) {
    sfprintf (sfstderr, "cannot open file %s\n", ifile);
    return -1;
  }
  while ((line = sfgetr (ifp, '\n', 1))) {
    op = line;
    if (!(np = strchr (op + 1, '|'))) {
      sfprintf (sfstderr, "badly formed inv line %s", line);
      continue;
    }
    if (!(ep = strchr (np + 1, '|'))) {
      sfprintf (sfstderr, "badly formed inv line %s", line);
      continue;
    }
    if (!(ip = strchr (ep + 1, '|'))) {
      sfprintf (sfstderr, "badly formed inv line %s", line);
      continue;
    }
    *ip++ = *ep++ = *np++ = 0;
    if (dtmatch (objdict, op))
      continue;
    if (!(objp = objinsert (op, np, ep, ip))) {
      sfprintf (sfstderr, "cannot insert obj\n");
      return -1;
    }
  }
  if (ifp != sfstdin)
    sfclose (ifp);

  if (!(kvts = (kvt_t *) vmalloc (Vmheap, 100 * sizeof (kvt_t)))) {
    sfprintf (sfstderr, "cannot allocate kvt array\n");
    return -1;
  }
  kvtn = 100;

  return 0;
}

int SWMIinvsave (void) {
  sfsync (NULL);
  return 0;
}

int SWMIinvexec (state_t *sp, int vflag) {
  char q[1024], *name, *val, *s;
  obj_t *objp;
  HRESULT hr;
  BSTR propb, ctb, wqlb, qb;
  SAFEARRAY *props;
  VARIANT var;
  LONG pu, pl, pi;
  ULONG ul;
  kvt_t *kvtp;
  int kvti;

  ctb = s2b ("countertype");
  wqlb = s2b ("WQL");
  for (
    objp = (obj_t *) dtfirst (objdict); objp;
    objp = (obj_t *) dtnext (objdict, objp)
  ) {
    val = NULL;
    sfsprintf (q, 1024, "select * from %s", objp->obj);
    if (verbose)
      sfprintf (sfstderr, "query: %s\n", q);
    if (!(qb = s2b (q))) {
      sfprintf (sfstderr, "cannot convert obj name %s to bstr\n", objp->obj);
      return -1;
    }

    if ((hr = sp->pSvc->ExecQuery (
      wqlb, qb, WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
      0, &sp->pEnum
    )) != S_OK) {
      sfprintf (
        sfstderr, "cannot execute query %s, error %s\n", q, E2ccp (hr)
      );
      return -1;
    }

    if (SWMIproxysecurity (sp, sp->pEnum) < 0) {
      sfprintf (sfstderr, "cannot enable security for query %s\n", q);
      return -1;
    }

    while (
      sp->pEnum->Next (30 * 1000, 1, &sp->pClsObj, &ul
    ) == S_OK && ul == 1) {
      if ((hr = sp->pClsObj->GetNames (
        0, WBEM_FLAG_ALWAYS | WBEM_FLAG_NONSYSTEM_ONLY, 0, &props
      )) != S_OK) {
        sfprintf (
          sfstderr, "cannot get props for obj %s, error %s\n",
          objp->obj, E2ccp (hr)
        );
        return -1;
      }
      if(
        (hr = SafeArrayGetLBound (props, 1, &pl)) != S_OK ||
        (hr = SafeArrayGetUBound (props, 1, &pu)) != S_OK
      ) {
        sfprintf (
          sfstderr, "cannot get props bounds for %s, error %s\n",
          objp->obj, E2ccp (hr)
        );
        return -1;
      }
      if (pu - pl + 1 > kvtn) {
        kvtn = pu - pl + 1;
        if (!(kvts = (kvt_t *) vmresize (
          Vmheap, kvts, kvtn * sizeof (kvt_t), VM_RSCOPY
        ))) {
          sfprintf (sfstderr, "cannot grow kvt array\n");
          return -1;
        }
      }
      name = NULL;
      for (pi = pl, kvtl = 0; pi <= pu; pi++, kvtl++) {
        kvtp = &kvts[kvtl];
        if ((hr = SafeArrayGetElement (props, &pi, &propb)) != S_OK) {
          sfprintf (
            sfstderr, "cannot get prop name for %d/%s, error %s\n",
            pi, objp->obj, E2ccp (hr)
          );
          continue;
        }
        kvtp->kp = (char *) b2s (propb);
        if ((hr = sp->pClsObj->Get (propb, 0, &var, 0, 0)) != S_OK) {
          sfprintf (
            sfstderr, "cannot get prop value for %d/%s, error %s\n",
            pi, objp->obj, E2ccp (hr)
          );
          continue;
        }
        if (!(kvtp->vp = vmstrdup (Vmheap, (char *) V2ccp (&var)))) {
          kvtl--;
          continue;
        }
        if (objp->ip[0] && strstr (kvtp->kp, objp->ip))
          val = kvtp->vp;
        switch (var.vt) {
        case VT_UI1: kvtp->v = var.bVal; break;
        case VT_I2: kvtp->v = var.iVal;  break;
        case VT_I4: kvtp->v = var.lVal;  break;
        default: kvtp->v = strtoull (kvtp->vp, &s, 10); break;
        }
        if (strcmp (kvtp->kp, "Name") == 0) {
          if (var.vt == VT_NULL)
            name = "ALL";
          else
            name = kvtp->vp;
        }
      }
      ::SafeArrayDestroy (props);
      sp->pClsObj->Release ();

      if (objp->ep[0] && strstr (name, objp->ep))
        continue;

      if (!vflag)
        sfprintf (sfstdout, "%s|%s|%s\n", objp->np, name, val ? val : "");
      else {
        for (kvti = 0; kvti < kvtl; kvti++) {
          kvtp = &kvts[kvti];
          sfprintf (
            sfstdout, "%s|%s|%s|%s\n", objp->np, name, kvtp->kp, kvtp->vp
          );
        }
      }
    }
  }
  return 0;
}

static obj_t *objinsert (char *op, char *np, char *ep, char *ip) {
  obj_t *objmem, *objp;

  if (!(objmem = (obj_t *) vmalloc (Vmheap, sizeof (obj_t)))) {
    sfprintf (sfstderr, "cannot allocate obj\n");
    return NULL;
  }
  memset (objmem, 0, sizeof (obj_t));
  objmem->index = dtsize (objdict);
  if ((objp = (obj_t *) dtinsert (objdict, objmem)) != objmem) {
    sfprintf (sfstderr, "cannot insert obj in dict\n");
    return NULL;
  }
  if (!(objp->obj = vmstrdup (Vmheap, op))) {
    sfprintf (sfstderr, "cannot allocate obj id\n");
    return NULL;
  }
  if (!(objp->np = vmstrdup (Vmheap, np))) {
    sfprintf (sfstderr, "cannot allocate name\n");
    return NULL;
  }
  if (!(objp->ep = vmstrdup (Vmheap, ep))) {
    sfprintf (sfstderr, "cannot allocate exclude pattern\n");
    return NULL;
  }
  if (!(objp->ip = vmstrdup (Vmheap, ip))) {
    sfprintf (sfstderr, "cannot allocate include pattern\n");
    return NULL;
  }
  return objp;
}
