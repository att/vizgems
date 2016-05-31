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
  char *obj;
  /* end key */
} obj_t;

static Dt_t *objdict;
static Dtdisc_t objdisc = {
    sizeof (Dtlink_t), -1, 0, NULL, NULL, NULL, NULL, NULL, NULL
};

#define OPN_KIND_P 1
#define OPN_KIND_S 2

#define V_TYPE_STRING          1
#define V_TYPE_SIMPLE          2
#define V_TYPE_WITHBASE        3
#define V_TYPE_WITHTIMENFREQ   4
#define V_TYPE_WITH100NSEC     5
#define V_TYPE_WITH100NSECINV  6
#define V_TYPE_SUPPORT         7
#define V_TYPE_ISBASE          7
#define V_TYPE_ISTIME          8
#define V_TYPE_ISFREQ          9
#define V_TYPE_IS100NSEC      10

typedef struct opn_t {
  Dtlink_t link;
  /* begin key */
  char *opn;
  /* end key */
  int kind, type, havep, havec;
  char *mname, *mtype, *munit, *op, *pp, *np, *vp, *ep, *lp;
  unsigned long long pv, cv;
  obj_t *objp;
} opn_t;

static Dt_t *opndict;
static Dtdisc_t opndisc = {
    sizeof (Dtlink_t), -1, 0, NULL, NULL, NULL, NULL, NULL, NULL
};

typedef struct kvt_t {
  char *kp, *vp;
  int type;
  unsigned long long v;
} kvt_t;

static kvt_t *kvts;
static int kvtl, kvtn;

static opn_t *opninsert (
    int, char *, char *, char *, char *, char *, char *, char *
);
static obj_t *objinsert (char *);

int SWMIopnload (char *tfile, char *sfile) {
  Sfio_t *tfp, *sfp;
  obj_t *objp;
  opn_t *opnp;
  char *line, *mname, *mtype, *munit, *op, *pp, *np, *vp, *tp, *ep, *lp, *s;

  if (!(objdict = dtopen (&objdisc, Dtset))) {
    sfprintf (sfstderr, "cannot create obj dict\n");
    return -1;
  }
  if (!(opndict = dtopen (&opndisc, Dtset))) {
    sfprintf (sfstderr, "cannot create opn dict\n");
    return -1;
  }

  if (!(tfp = sfopen (NULL, tfile, "r"))) {
    sfprintf (sfstderr, "cannot open file %s\n", tfile);
    return -1;
  }
  while ((line = sfgetr (tfp, '\n', 1))) {
    mname = line;
    if (!(mtype = strchr (mname, '|'))) {
      sfprintf (sfstderr, "badly formed opn line %s", line);
      continue;
    }
    if (!(munit = strchr (mtype + 1, '|'))) {
      sfprintf (sfstderr, "badly formed opn line %s", line);
      continue;
    }
    if (!(op = strchr (munit + 1, '|'))) {
      sfprintf (sfstderr, "badly formed opn line %s", line);
      continue;
    }
    if (!(tp = strchr (op + 1, '|'))) {
      sfprintf (sfstderr, "badly formed opn line %s", line);
      continue;
    }
    if (!(ep = strchr (tp + 1, '|'))) {
      sfprintf (sfstderr, "badly formed opn line %s", line);
      continue;
    }
    if (!(lp = strchr (ep + 1, '|'))) {
      sfprintf (sfstderr, "badly formed opn line %s", line);
      continue;
    }
    if (!(pp = strchr (op + 1, '.')) || !(np = strchr (pp + 1, '.'))) {
      sfprintf (sfstderr, "badly formed opn line %s", line);
      continue;
    }
    *mtype++ = 0, *munit++ = 0, *op++ = 0, *tp++ = 0, *ep++ = 0, *lp++ = 0;
    if (dtmatch (opndict, op))
      continue;
    *pp++ = 0, *np++ = 0;
    if (!(opnp = opninsert (OPN_KIND_P, mname, mtype, munit, op, pp, np, lp))) {
      sfprintf (sfstderr, "cannot insert opn\n");
      return -1;
    }
    if (*tp) {
        if (strcmp (tp, "simple") == 0)
            opnp->type = V_TYPE_SIMPLE;
        else if (strcmp (tp, "withbase") == 0)
            opnp->type = V_TYPE_WITHBASE;
    }
    if (*ep) {
      if (!(opnp->ep = vmstrdup (Vmheap, ep))) {
        sfprintf (sfstderr, "cannot allocate ep\n");
        return NULL;
      }
    }

    if ((objp = (obj_t *) dtmatch (objdict, op))) {
      opnp->objp = objp;
      continue;
    }
    if (!(objp = objinsert (op))) {
      sfprintf (sfstderr, "cannot insert obj\n");
      return -1;
    }

    opnp->objp = objp;
  }
  sfclose (tfp);

  if (!(sfp = sfopen (NULL, sfile, "r"))) {
    sfprintf (sfstderr, "cannot open file %s\n", sfile);
    return -1;
  }
  while ((line = sfgetr (sfp, '\n', 1))) {
    if (!(vp = strrchr (line, '|'))) {
      sfprintf (sfstderr, "badly formed opn state %s", line);
      return -1;
    }
    *vp++ = 0;
    op = line;
    if (!(pp = strchr (op, '.')) || !(np = strchr (pp + 1, '.'))) {
      sfprintf (sfstderr, "badly formed opn %s", line);
      continue;
    }
    if (!(opnp = (opn_t *) dtmatch (opndict, op))) {
      *pp++ = 0, *np++ = 0;
      if (!(opnp = opninsert (OPN_KIND_S, "", "", "", op, pp, np, ""))) {
        sfprintf (sfstderr, "cannot insert opn\n");
        return -1;
      }
      if ((objp = (obj_t *) dtmatch (objdict, op)))
        opnp->objp = objp;
    }

    if (strcmp (vp, "NULL") == 0) {
      opnp->havep = FALSE;
    } else {
      opnp->pv = strtoull (vp, &s, 10);
      if (s && *s) {
        sfprintf (sfstderr, "cannot calculate previous value\n");
        opnp->havep = FALSE;
      } else
        opnp->havep = TRUE;
    }
  }
  sfclose (sfp);

  if (!(kvts = (kvt_t *) vmalloc (Vmheap, 100 * sizeof (kvt_t)))) {
    sfprintf (sfstderr, "cannot allocate kvt array\n");
    return -1;
  }
  kvtn = 100;

  return 0;
}

int SWMIopnsave (char *sfile) {
  Sfio_t *sfp;
  opn_t *opnp;

  if (!(sfp = sfopen (NULL, sfile, "w"))) {
    sfprintf (sfstderr, "cannot open file %s\n", sfile);
    return -1;
  }
  for (
    opnp = (opn_t *) dtfirst (opndict); opnp;
    opnp = (opn_t *) dtnext (opndict, opnp)
  ) {
    if (!opnp->havec || opnp->type == V_TYPE_STRING)
      continue;
    sfprintf (sfp, "%s.%s.%s|%s\n", opnp->op, opnp->pp, opnp->np, opnp->vp);
  }
  sfclose (sfp);

  return 0;
}

int SWMIopnexec (state_t *sp) {
  char q[1024], opnstr[1024], *name, *origname, *s;
  opn_t *opnp, *opnbasep, *opntimep, *opnfreqp;
  obj_t *objp;
  HRESULT hr;
  BSTR propb, ctb, wqlb, qb;
  SAFEARRAY *props;
  VARIANT var;
  LONG pu, pl, pi;
  ULONG ul;
  double v;
  kvt_t *kvtp;
  int kvti;

  ctb = s2b ("countertype");
  wqlb = s2b ("WQL");
  for (
    objp = (obj_t *) dtfirst (objdict); objp;
    objp = (obj_t *) dtnext (objdict, objp)
  ) {
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
        if (
            !(kvtp->vp = (char *) V2ccp (&var)) ||
            !(kvtp->vp = vmstrdup (Vmheap, kvtp->vp))
        ) {
          kvtl--;
          continue;
        }
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
          origname = name;
        }

        if ((hr = sp->pClsObj->GetPropertyQualifierSet (
          propb, &sp->pQualSet
        )) != S_OK) {
          sfprintf (
            sfstderr, "cannot get quals for %d/%s, error %s\n",
            pi, objp->obj, E2ccp (hr)
          );
          continue;
        }
        kvtp->type = V_TYPE_STRING;
        if ((hr = sp->pQualSet->Get (
          ctb, 0, &var, 0
        )) == S_OK && var.vt == VT_I4) {
          switch (var.lVal) {
          case PERF_COUNTER_RAWCOUNT:
          case PERF_COUNTER_LARGE_RAWCOUNT:
            kvtp->type = V_TYPE_SIMPLE; break;
          case PERF_RAW_FRACTION:
            kvtp->type = V_TYPE_WITHBASE; break;
          case PERF_RAW_BASE:
          case PERF_PRECISION_TIMESTAMP:
            kvtp->type = V_TYPE_ISBASE; break;
          case PERF_COUNTER_COUNTER:
          case PERF_COUNTER_BULK_COUNT:
            kvtp->type = V_TYPE_WITHTIMENFREQ; break;
          case PERF_100NSEC_TIMER:
            kvtp->type = V_TYPE_WITH100NSEC; break;
          case PERF_100NSEC_TIMER_INV:
            kvtp->type = V_TYPE_WITH100NSECINV; break;
          default:
            kvtp->type = V_TYPE_SIMPLE; break;
          }
        } else {
          if (strcmp (kvtp->kp, "Timestamp_PerfTime") == 0)
            kvtp->type = V_TYPE_ISTIME;
          else if (strcmp (kvtp->kp, "Frequency_PerfTime") == 0)
            kvtp->type = V_TYPE_ISFREQ;
          else if (strcmp (kvtp->kp, "Timestamp_Sys100NS") == 0)
            kvtp->type = V_TYPE_IS100NSEC;
          else if (kvtp->vp[0] == 0 || isdigit (kvtp->vp[0]))
            kvtp->type = V_TYPE_SIMPLE;
        }
      }
      ::SafeArrayDestroy (props);
      sp->pClsObj->Release ();

      for (kvti = 0; kvti < kvtl; kvti++) {
        kvtp = &kvts[kvti];
        sfsprintf (opnstr, 1024, "%s.%s.%s", objp->obj, kvtp->kp, name);
        if (!(opnp = (opn_t *) dtmatch (opndict, opnstr))) {
          sfsprintf (opnstr, 1024, "%s.%s.ALL", objp->obj, kvtp->kp);
          if ((opnp = (opn_t *) dtmatch (opndict, opnstr)))
            name = "ALL";
          else
            sfsprintf (opnstr, 1024, "%s.%s.%s", objp->obj, kvtp->kp, name);
        }

        if (!opnp) {
          if (kvtp->type < V_TYPE_SUPPORT)
            continue;
          if (!(opnp = opninsert (
            OPN_KIND_S, "", "", "", objp->obj, kvtp->kp, name, ""
          ))) {
            sfprintf (sfstderr, "cannot insert opn\n");
            return -1;
          }
          opnp->objp = objp;
        }
        if (opnp->ep && strstr (origname, opnp->ep))
          continue;
        if (opnp->type == 0)
          opnp->type = kvtp->type;
        opnp->vp = kvtp->vp;
        opnp->cv += kvtp->v;
        opnp->havec = TRUE;
      }
      for (kvti = 0; kvti < kvtl; kvti++) {
        kvtp = &kvts[kvti];
        sfsprintf (opnstr, 1024, "%s.%s.SUM", objp->obj, kvtp->kp);
        if (!(opnp = (opn_t *) dtmatch (opndict, opnstr)))
          continue;

        if (opnp->ep && strstr (origname, opnp->ep))
          continue;
        if (opnp->type == 0)
          opnp->type = kvtp->type;
        opnp->vp = kvtp->vp;
        opnp->cv += kvtp->v;
        opnp->havec = TRUE;
      }
    }
  }
  for (
    opnp = (opn_t *) dtfirst (opndict); opnp;
    opnp = (opn_t *) dtnext (opndict, opnp)
  ) {
    if (opnp->kind != OPN_KIND_P || !opnp->havec)
      continue;
    if (opnp->type >= V_TYPE_SUPPORT)
      continue;
    switch (opnp->type) {
    case V_TYPE_STRING:
#if 0
      sfprintf (
        sfstdout, "rt=STAT type=string name=%s str=%s%s%s%s\n",
        opnp->mname, opnp->vp,
        (opnp->lp[0]) ? " label='" : "", (opnp->lp[0]) ? opnp->lp : "",
        (opnp->lp[0]) ? "'" : ""
      );
#endif
      break;
    case V_TYPE_SIMPLE:
      sfprintf (
        sfstdout, "rt=STAT type=%s name=%s unit=%s num=%lld%s%s%s\n",
        opnp->mtype, opnp->mname, opnp->munit, opnp->cv,
        (opnp->lp[0]) ? " label='" : "", (opnp->lp[0]) ? opnp->lp : "",
        (opnp->lp[0]) ? "'" : ""
      );
      break;
    case V_TYPE_WITHBASE:
      sfsprintf (opnstr, 1024, "%s.%s_Base.%s", opnp->op, opnp->pp, opnp->np);
      if (!(opnbasep = (opn_t *) dtmatch (opndict, opnstr))) {
        sfprintf (sfstderr, "cannot find base property for %s\n", opnp->opn);
        continue;
      }
      if (!opnbasep->havec)
        continue;
      if ((v = (opnbasep->cv == 0) ? 0.00 : 100.0 * (
        opnp->cv / (double) opnbasep->cv
      )) < 0.0)
        continue;
      sfprintf (
        sfstdout, "rt=STAT type=%s name=%s unit=%s num=%lf%s%s%s\n",
        opnp->mtype, opnp->mname, opnp->munit, v,
        (opnp->lp[0]) ? " label='" : "", (opnp->lp[0]) ? opnp->lp : "",
        (opnp->lp[0]) ? "'" : ""
      );
      break;
    case V_TYPE_WITHTIMENFREQ:
      if (!opnp->havep)
        continue;
      sfsprintf (opnstr, 1024, "%s.Timestamp_PerfTime.%s", opnp->op, opnp->np);
      if (!(opntimep = (opn_t *) dtmatch (opndict, opnstr))) {
        sfprintf (sfstderr, "cannot find time property for %s\n", opnp->opn);
        continue;
      }
      if (!opntimep->havec || !opntimep->havep)
        continue;
      sfsprintf (opnstr, 1024, "%s.Frequency_PerfTime.%s", opnp->op, opnp->np);
      if (!(opnfreqp = (opn_t *) dtmatch (opndict, opnstr))) {
        sfprintf (sfstderr, "cannot find freq property for %s\n", opnp->opn);
        continue;
      }
      if (!opnfreqp->havec)
        continue;
      if (
        opnp->cv < opnp->pv || opntimep->cv <= opntimep->pv ||
        opnfreqp->cv == 0
      )
        continue;
      if ((v = (opnp->cv - opnp->pv) / (
        (opntimep->cv - opntimep->pv) / (double) opnfreqp->cv
      )) < 0.0) {
        if (v > -1)
          v = 0.0;
        else
          continue;
      }
      sfprintf (
        sfstdout, "rt=STAT type=%s name=%s unit=%s num=%lf%s%s%s\n",
        opnp->mtype, opnp->mname, opnp->munit, v,
        (opnp->lp[0]) ? " label='" : "", (opnp->lp[0]) ? opnp->lp : "",
        (opnp->lp[0]) ? "'" : ""
      );
      break;
    case V_TYPE_WITH100NSEC:
    case V_TYPE_WITH100NSECINV:
      if (!opnp->havep)
        continue;
      sfsprintf (opnstr, 1024, "%s.Timestamp_Sys100NS.%s", opnp->op, opnp->np);
      if (!(opntimep = (opn_t *) dtmatch (opndict, opnstr))) {
        sfprintf (sfstderr, "cannot find time property for %s\n", opnp->opn);
        continue;
      }
      if (!opntimep->havec || !opntimep->havep)
        continue;
      if (opnp->cv < opnp->pv || opntimep->cv <= opntimep->pv)
        continue;
      if (opnp->type == V_TYPE_WITH100NSEC) {
        if ((v = 100.0 * (
          (opnp->cv - opnp->pv) /
          ((double) opntimep->cv - opntimep->pv)
        )) < 0.0) {
          if (v > -1)
            v = 0.0;
          else
            continue;
        }
        sfprintf (
          sfstdout, "rt=STAT type=%s name=%s unit=%s num=%lf%s%s%s\n",
          opnp->mtype, opnp->mname, opnp->munit, v,
          (opnp->lp[0]) ? " label='" : "", (opnp->lp[0]) ? opnp->lp : "",
          (opnp->lp[0]) ? "'" : ""
        );
      } else {
        if ((v = 100.0 * (
          1.0 - (opnp->cv - opnp->pv) /
          ((double) opntimep->cv - opntimep->pv)
        )) < 0.0) {
          if (v > -1)
            v = 0.0;
          else
            continue;
        }
        sfprintf (
          sfstdout, "rt=STAT type=%s name=%s unit=%s num=%lf%s%s%s\n",
          opnp->mtype, opnp->mname, opnp->munit, v,
          (opnp->lp[0]) ? " label='" : "", (opnp->lp[0]) ? opnp->lp : "",
          (opnp->lp[0]) ? "'" : ""
        );
      }
      break;
    }
  }
  return 0;
}

static opn_t *opninsert (
  int kind, char *mname, char *mtype, char *munit,
  char *op, char *pp, char *np, char *lp
) {
  opn_t *opnmem, *opnp;

  if (!(opnmem = (opn_t *) vmalloc (Vmheap, sizeof (opn_t)))) {
    sfprintf (sfstderr, "cannot allocate opn\n");
    return NULL;
  }
  memset (opnmem, 0, sizeof (opn_t));
  if (!(opnmem->opn = vmstrdup (Vmheap, sfprints (
    "%s.%s.%s", op, pp, np
  )))) {
    sfprintf (sfstderr, "cannot allocate opn id\n");
    return NULL;
  }
  if ((opnp = (opn_t *) dtinsert (opndict, opnmem)) != opnmem) {
    sfprintf (sfstderr, "cannot insert opn in dict\n");
    return NULL;
  }
  opnp->kind = kind;
  if (
    !(opnp->mname = vmstrdup (Vmheap, mname)) ||
    !(opnp->mtype = vmstrdup (Vmheap, mtype)) ||
    !(opnp->munit = vmstrdup (Vmheap, munit)) ||
    !(opnp->op = vmstrdup (Vmheap, op)) ||
    !(opnp->pp = vmstrdup (Vmheap, pp)) ||
    !(opnp->np = vmstrdup (Vmheap, np)) ||
    !(opnp->lp = vmstrdup (Vmheap, lp))
  ) {
    sfprintf (sfstderr, "cannot allocate o p n\n");
    return NULL;
  }

  return opnp;
}

static obj_t *objinsert (char *op) {
  obj_t *objmem, *objp;

  if (!(objmem = (obj_t *) vmalloc (Vmheap, sizeof (obj_t)))) {
    sfprintf (sfstderr, "cannot allocate obj\n");
    return NULL;
  }
  memset (objmem, 0, sizeof (obj_t));
  if (!(objmem->obj = vmstrdup (Vmheap, op))) {
    sfprintf (sfstderr, "cannot allocate obj id\n");
    return NULL;
  }
  if ((objp = (obj_t *) dtinsert (objdict, objmem)) != objmem) {
    sfprintf (sfstderr, "cannot insert obj in dict\n");
    return NULL;
  }
  return objp;
}
