/*
 * SWIFT WMI interface
 * purpose: querying and dumping WMI resources
 * author: Don Caldwell dfwc@research.att.com
 & edited by: Lefteris Koutsofios ek@research.att.com
 */

# pragma comment(lib, "wbemuuid.lib")

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

#define LOG_KIND_P 1
#define LOG_KIND_S 2

typedef struct log_t {
  Dtlink_t link;
  /* begin key */
  char *log;
  /* end key */
  char *vname;
  int kind, havern;
  unsigned long long rn;
  char **srcs;
  int srcn;
  int *types;
  int typen;
} log_t;

static Dt_t *logdict;
static Dtdisc_t logdisc = {
    sizeof (Dtlink_t), -1, 0, NULL, NULL, NULL, NULL, NULL, NULL
};

typedef struct event_t {
    int type, code;
    char *msg, *src;
} event_t;

static log_t *loginsert (int, char *, char *, char *, char *);
static int urlenc (char *, char *, int);

int SWMIlogload (char *lfile, char *sfile) {
  Sfio_t *tfp, *sfp;
  log_t *logp;
  char *line, *vp, *lp, *sp, *tp, *np, *s;

  if (!(logdict = dtopen (&logdisc, Dtset))) {
    sfprintf (sfstderr, "cannot create log dict\n");
    return -1;
  }

  if (!(tfp = sfopen (NULL, lfile, "r"))) {
    sfprintf (sfstderr, "cannot open file %s\n", lfile);
    return -1;
  }
  while ((line = sfgetr (tfp, '\n', 1))) {
    vp = line;
    if (
      !(lp = strchr (vp, '|')) || !(sp = strchr (lp + 1, '|')) ||
      !(tp = strchr (sp + 1, '|'))
    ) {
      sfprintf (sfstderr, "badly formed log line %s", line);
      continue;
    }
    *lp++ = 0, *sp++ = 0, *tp++ = 0;
    if (dtmatch (logdict, lp)) {
      sfprintf (sfstderr, "duplicate record for logfile %s", lp);
      continue;
    }
    if (!(logp = loginsert (LOG_KIND_P, vp, lp, sp, tp))) {
      sfprintf (sfstderr, "cannot insert log %s\n", lp);
      return -1;
    }
  }
  sfclose (tfp);

  if (!(sfp = sfopen (NULL, sfile, "r"))) {
    sfprintf (sfstderr, "cannot open file %s\n", sfile);
    return -1;
  }
  while ((line = sfgetr (sfp, '\n', 1))) {
    lp = line;
    if (!(np = strchr (lp, '|'))) {
      sfprintf (sfstderr, "badly formed log state %s", line);
      continue;
    }
    *np++ = 0;
    if (!(logp = (log_t *) dtmatch (logdict, lp)))
      continue;
    logp->rn = strtoll (np, &s, 10);
    if (s && *s) {
      sfprintf (sfstderr, "cannot calculate previous record number\n");
      return -1;
    }
    logp->havern = TRUE;
  }
  sfclose (sfp);

  return 0;
}

int SWMIlogsave (char *sfile) {
  Sfio_t *sfp;
  log_t *logp;

  if (!(sfp = sfopen (NULL, sfile, "w"))) {
    sfprintf (sfstderr, "cannot open file %s\n", sfile);
    return -1;
  }
  for (
    logp = (log_t *) dtfirst (logdict); logp;
    logp = (log_t *) dtnext (logdict, logp)
  ) {
    if (!logp->havern || logp->kind != LOG_KIND_P)
      continue;
    sfprintf (sfp, "%s|%lld\n", logp->log, logp->rn);
  }
  sfclose (sfp);

  return 0;
}

int SWMIlogexec (char *sfile, state_t *sp) {
  char q[1024], buf[1024], *s, *kp, *vp, *type;
  log_t *logp;
  HRESULT hr;
  BSTR propb, wqlb, qb;
  SAFEARRAY *props;
  VARIANT var;
  LONG pu, pl, pi;
  ULONG ul;
  unsigned long long v, ri, rc;
  event_t e;
  int srci, typei, found, sev;

  wqlb = s2b ("WQL");
  for (
    logp = (log_t *) dtfirst (logdict); logp;
    logp = (log_t *) dtnext (logdict, logp)
  ) {
    if (!logp->havern) {
      sfsprintf (
        q, 1024, "select * from Win32_NTLogEvent where LogFile='%s'", logp->log
      );
      if (!(qb = s2b (q))) {
        sfprintf (sfstderr, "cannot convert log name %s to bstr\n", logp->log);
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
        sp->pEnum->Next (60 * 1000, 1, &sp->pClsObj, &ul
      ) == S_OK && ul == 1) {
        if((hr = sp->pClsObj->GetNames (
          0, WBEM_FLAG_ALWAYS | WBEM_FLAG_NONSYSTEM_ONLY, 0, &props
        )) != S_OK) {
          sfprintf (
            sfstderr, "cannot get props for log %s, error %s\n",
            logp->log, E2ccp (hr)
          );
          return -1;
        }
        if (
          (hr = SafeArrayGetLBound (props, 1, &pl)) != S_OK ||
          (hr = SafeArrayGetUBound (props, 1, &pu)) != S_OK
        ) {
          sfprintf (
            sfstderr, "cannot get props bounds for %s, error %s\n",
            logp->log, E2ccp (hr)
          );
          return -1;
        }
        for (pi = pl; pi <= pu; pi++) {
          if ((hr = SafeArrayGetElement (props, &pi, &propb)) != S_OK) {
            sfprintf (
              sfstderr, "cannot get prop name for %d/%s, error %s\n",
              pi, logp->log, E2ccp (hr)
            );
            continue;
          }
          kp = (char *) b2s (propb);
          if (strcmp (kp, "RecordNumber") != 0)
            continue;

          if ((hr = sp->pClsObj->Get (propb, 0, &var, 0, 0)) != S_OK) {
            sfprintf (
              sfstderr, "cannot get prop value for %d/%s, error %s\n",
              pi, logp->log, E2ccp (hr)
            );
            continue;
          }
          switch (var.vt) {
          case VT_UI1: v = var.bVal; break;
          case VT_I2: v = var.iVal;  break;
          case VT_I4: v = var.lVal;  break;
          case VT_NULL: v = 0;       break;
          default: v = strtoull (vp, &s, 10); break;
          }
          if (!logp->havern || logp->rn < v)
            logp->rn = v, logp->havern = TRUE;
        }
        ::SafeArrayDestroy (props);
        sp->pClsObj->Release ();
        if (logp->havern)
          break;
      }
      if (logp->havern)
        logp->rn++;
      continue;
    }

    rc = 0, ri = logp->rn;
    for (;;) {
      sfsprintf (
        q, 1024, "Win32_NTLogEvent.LogFile='%s',RecordNumber=%lld",
        logp->log, logp->rn
      );
      if (!(qb = s2b (q))) {
        sfprintf (sfstderr, "cannot convert log name %s to bstr\n", logp->log);
        return -1;
      }
      if ((hr = sp->pSvc->GetObject (
        qb, WBEM_FLAG_RETURN_WBEM_COMPLETE, 0, &sp->pClsObj, 0
      )) != S_OK) {
        // if we read some log record then we're done
        if (logp->rn > ri)
          break;

        // else if the previous last record is still there we're done
        sfsprintf (
          q, 1024, "Win32_NTLogEvent.LogFile='%s',RecordNumber=%lld",
          logp->log, ri - 1
        );
        if (!(qb = s2b (q))) {
          sfprintf (
            sfstderr, "cannot convert log name %s to bstr\n", logp->log
          );
          return -1;
        }
        if ((hr = sp->pSvc->GetObject (
          qb, WBEM_FLAG_RETURN_WBEM_COMPLETE, 0, &sp->pClsObj, 0
        )) == S_OK)
          break;

        // else if record #1 is not there, invalidate rn
        ri = 1;
        sfsprintf (
          q, 1024, "Win32_NTLogEvent.LogFile='%s',RecordNumber=%lld",
          logp->log, ri
        );
        if (!(qb = s2b (q))) {
          sfprintf (
            sfstderr, "cannot convert log name %s to bstr\n", logp->log
          );
          return -1;
        }
        if ((hr = sp->pSvc->GetObject (
          qb, WBEM_FLAG_RETURN_WBEM_COMPLETE, 0, &sp->pClsObj, 0
        )) != S_OK) {
          logp->havern = FALSE;
          break;
        }

        // else we're read record #0, go on from there
        logp->rn = ri;
      }

      logp->rn++;

#if 0
      if (SWMIproxysecurity (sp, sp->pClsObj) < 0) {
        sfprintf (sfstderr, "cannot enable security for query %s\n", q);
        return -1;
      }
#endif

      if((hr = sp->pClsObj->GetNames (
        0, WBEM_FLAG_ALWAYS | WBEM_FLAG_NONSYSTEM_ONLY, 0, &props
      )) != S_OK) {
        sfprintf (
          sfstderr, "cannot get props for log %s, error %s\n",
          logp->log, E2ccp (hr)
        );
        return -1;
      }
      if (
        (hr = SafeArrayGetLBound (props, 1, &pl)) != S_OK ||
        (hr = SafeArrayGetUBound (props, 1, &pu)) != S_OK
      ) {
        sfprintf (
          sfstderr, "cannot get props bounds for %s, error %s\n",
          logp->log, E2ccp (hr)
        );
        return -1;
      }
      memset (&e, 0, sizeof (e));
      for (pi = pl; pi <= pu; pi++) {
        if ((hr = SafeArrayGetElement (props, &pi, &propb)) != S_OK) {
          sfprintf (
            sfstderr, "cannot get prop name for %d/%s, error %s\n",
            pi, logp->log, E2ccp (hr)
          );
          continue;
        }
        kp = (char *) b2s (propb);

        if ((hr = sp->pClsObj->Get (propb, 0, &var, 0, 0)) != S_OK) {
          sfprintf (
            sfstderr, "cannot get prop value for %d/%s, error %s\n",
            pi, logp->log, E2ccp (hr)
          );
          continue;
        }
        vp = NULL;
        switch (var.vt) {
        case VT_UI1: v = var.bVal; break;
        case VT_I2: v = var.iVal;  break;
        case VT_I4: v = var.lVal;  break;
        default: vp = (char *) V2ccp (&var); break;
        }

        if (strcmp (kp, "EventType") == 0) {
          e.type = (int) v;
        } else if (strcmp (kp, "EventCode") == 0) {
          e.code = (int) v;
        } else if (strcmp (kp, "Message") == 0) {
          e.msg = vp;
        } else if (strcmp (kp, "SourceName") == 0) {
          e.src = vp;
        }
      }
      ::SafeArrayDestroy (props);
      sp->pClsObj->Release ();

      for (typei = 0, found = TRUE; typei < logp->typen; typei++) {
        found = FALSE;
        if (logp->types[typei] == e.type) {
          found = TRUE;
          break;
        }
      }
      if (!found)
        continue;

      for (srci = 0, found = TRUE; srci < logp->srcn; srci++) {
        found = FALSE;
        if (strstr (e.src, logp->srcs[srci])) {
          found = TRUE;
          break;
        }
      }
      if (!found)
        continue;

      switch (e.type) {
      case 1: sev = 1; type = "ALARM"; break;
      case 2: sev = 4; type = "ALARM"; break;
      case 3: sev = 5; type = "ALARM"; break;
      case 4: sev = 5; type = "CLEAR"; break;
      case 5: sev = 1; type = "ALARM"; break;
      }

      urlenc (e.msg, buf, 1024);
      sfprintf (
        sfstdout,
        "rt=ALARM alarmid='%s' sev=%d type=%s tech=WMI"
        " cond='%s-%s-%d: ' txt='%s'\n",
        e.src, sev, type, logp->log, e.src, e.code, buf
      );
      if (++rc >= 200)
        break;
      if (rc % 50 == 0) {
        if (SWMIlogsave (sfile) == -1) {
          sfprintf (
            sfstderr, "cannot save intermediate state for log %s\n", logp->log
          );
          return -1;
        }
      }
    }
    if (logp->havern)
      sfprintf (
        sfstdout, "rt=STAT type=counter name=%s unit= num=%lld\n",
        logp->vname, logp->rn
      );
  }
  return 0;
}

static log_t *loginsert (int kind, char *vp, char *lp, char *sp, char *tp) {
  log_t *logmem, *logp;
  char *s1, *s2;

  if (!(logmem = (log_t *) vmalloc (Vmheap, sizeof (log_t)))) {
    sfprintf (sfstderr, "cannot allocate log\n");
    return NULL;
  }
  memset (logmem, 0, sizeof (log_t));
  if (!(logmem->log = vmstrdup (Vmheap, lp))) {
    sfprintf (sfstderr, "cannot allocate log id\n");
    return NULL;
  }
  if ((logp = (log_t *) dtinsert (logdict, logmem)) != logmem) {
    sfprintf (sfstderr, "cannot insert log in dict\n");
    return NULL;
  }
  if (vp && !(logmem->vname = vmstrdup (Vmheap, vp))) {
    sfprintf (sfstderr, "cannot allocate log var name\n");
    return NULL;
  }
  logp->kind = kind;
  for (s1 = sp, logp->srcn = (*sp) ? 1 : 0; s1; ) {
    if ((s2 = strchr (s1, ':')))
      logp->srcn++, s1 = s2 + 1;
    else
      s1 = NULL;
  }
  if (!(logp->srcs = (char **) vmalloc (
    Vmheap, logp->srcn * sizeof (char *)
  ))) {
    sfprintf (sfstderr, "cannot allocate log srcs\n");
    return NULL;
  }
  for (s1 = sp, logp->srcn = 0; s1; ) {
    if ((s2 = strchr (s1, ':')))
      *s2++ = 0;
    if (*s1 && !(logp->srcs[logp->srcn++] = vmstrdup (Vmheap, s1))) {
      sfprintf (sfstderr, "cannot allocate log src %s\n", s1);
      return NULL;
    }
    s1 = s2;
  }

  for (s1 = tp, logp->typen = (*tp) ? 1 : 0; s1; ) {
    if ((s2 = strchr (s1, ':')))
      logp->typen++, s1 = s2 + 1;
    else
      s1 = NULL;
  }
  if (!(logp->types = (int *) vmalloc (Vmheap, logp->typen * sizeof (int)))) {
    sfprintf (sfstderr, "cannot allocate log types\n");
    return NULL;
  }
  for (s1 = tp, logp->typen = 0; s1; ) {
    if ((s2 = strchr (s1, ':')))
      *s2++ = 0;
    if (*s1)
      logp->types[logp->typen++] = atoi (s1);
    s1 = s2;
  }

  return logp;
}

static int urlenc (char *istr, char *ostr, int size) {
  char *is, *os;
  int flag;

  flag = FALSE;
  for (is = istr, os = ostr; *is; is++) {
    if (os - ostr + 3 > size) {
      *os = 0;
      return -1;
    }
    switch (*is) {
    case '%':
      *os++ = '%', *os++ = '2', *os++ = '5', flag = TRUE;
      break;
    case ';':
      *os++ = '%', *os++ = '3', *os++ = 'B', flag = TRUE;
      break;
    case '&':
      *os++ = '%', *os++ = '2', *os++ = '6', flag = TRUE;
      break;
    case '|':
      *os++ = '%', *os++ = '7', *os++ = 'C', flag = TRUE;
      break;
    case '\'':
      *os++ = '%', *os++ = '2', *os++ = '7', flag = TRUE;
      break;
    case '\n':
    case '\r':
    case ' ':
      if (os == ostr || *(os - 1) != ' ')
          *os++ = ' ';
      break;
    default:
      *os++ = *is;
      break;
    }
  }
  if (os > ostr && *(os - 1) == ' ')
      os--;
  *os = 0;
  return flag ? 1 : 0;
}
