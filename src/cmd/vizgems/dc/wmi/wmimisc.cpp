/*
 * SWMI library
 * swift WMI library
 * purpose: querying and dumping WMI resources
 * author: Don Caldwell dfwc@research.att.com
 */

# pragma comment(lib, "wbemuuid.lib")

#define _WIN32_DCOM
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

state_t *SWMIopen (void) {
  HRESULT hr;
  state_t *sp;

  if ((hr = CoInitializeEx (0, COINIT_MULTITHREADED)) != S_OK) {
    sfprintf (sfstderr, "SWMIopen(CoIninitlizeEx()) failed: %s\n", E2ccp (hr));
    return NULL;
  }
  if ((hr = CoInitializeSecurity (
    0,
    -1,                          // COM authentication
    0,                           // Authentication services
    0,                           // Reserved
    RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication
    RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation
    0,                           // Authentication info
    EOAC_NONE,                   // Additional capabilities
    0                            // Reserved
  )) != S_OK) {
    sfprintf (
      sfstderr, "SWMIopen(CoIninitlizeSecurity()) failed: %s\n", E2ccp(hr)
    );
    CoUninitialize ();
    return NULL;
  }
  if (!(sp = (state_t *) vmalloc (Vmheap, sizeof(state_t)))) {
    sfprintf (sfstderr, "Out of memory\n");
    return NULL;
  }
  memset (sp, '\0', sizeof(state_t));
  if ((hr = CoCreateInstance (
    CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
    IID_IWbemLocator, (LPVOID *) &sp->pLoc
  )) != S_OK) {
    sfprintf (
      sfstderr, "SWMIopen(CoCreateInstance()) failed: %s\n", E2ccp (hr)
    );
    CoUninitialize ();
    return NULL;
  }
  return sp;
}

int SWMIconnect (
  state_t *sp, const char* host, const char* ns,
  const char* user, const char* passwd
) {
  HRESULT hr;
  char url[MAX_PATH], *s;
  char **passwords;
  int passwordn, passwordm, passwordl, passwordi, passwordj;

  if (!sp || !sp->pLoc || !host || !ns)
    return 0;

  if (host) {
    sp->host = s2b (host);
    strcpy (url, "\\\\");
    strcat (url, host);
    if (ns)
      sp->ns = s2b (ns), strcat (url, "\\"), strcat (url, ns);
    else
      strcat (url, "\\root\\cimv2");
  } else {
    if (ns)
      sp->ns = s2b (ns), strcpy (url, "\\\\.\\"), strcat (url, ns);
    else
      strcpy (url, "\\\\.\\root\\cimv2");
  }
  sp->url = s2b (url);
  if (verbose)
    sfprintf (sfstderr, "SWMIconnect url: %s\n", url);

  passwordn = passwordm = passwordl = passwordi = 0;

  if (passwd && strncmp (passwd, "SWMUL", 5) == 0) {
      char *s1, *s2, *line;
      Sfio_t *fp;

      s1 = strdup (passwd) + 5;
      if (!(passwords = (char **) malloc (10 * sizeof (char *)))) {
          sfprintf (sfstderr, "memory: cannot malloc passwords\n");
          exit (1);
      }
      passwordn = 10;
      while (s1) {
          if ((s2 = strstr (s1, "SWMUL")))
              *s2 = 0, s2 += 5;
          if (!*s2)
              s2 = NULL;
          if (passwordm >= passwordn) {
              if (!(passwords = (char **) realloc (
                  passwords, (passwordn + 10) * sizeof (char *)
              ))) {
                  sfprintf (sfstderr, "memory: cannot realloc passwords\n");
                  exit (1);
              }
              passwordn += 10;
          }
          if (!(passwords[passwordm++] = (char *) strdup (s1))) {
              sfprintf (
                  sfstderr, "memory: cannot malloc password %d\n", passwordm
              );
              exit (1);
          }
          s1 = s2;
      }
      if ((fp = sfopen (NULL, "wmi.lastmulti", "r"))) {
          if (!(line = sfgetr (fp, '\n', 1)) || (passwordl = atoi (line)) < 0)
              passwordl = 0;
          sfclose (fp);
      }
      if (passwordm < 1) {
          sfprintf (sfstderr, "memory: no password in multi password mode\n");
          exit (1);
      }
      passwordi = 0;
      passwordj = (passwordi++ + passwordl) % passwordm;
      passwd = passwords[passwordj];
  }

again:

  if (user) {
    if (verbose)
      sfprintf (sfstderr, "SWMIconnect user: %s, pass: %s\n", user, passwd);
    sp->dnu = s2b (user);
    if (!sp->dnu) {
      sfprintf (sfstderr, "Out of memory\n");
      return -1;
    }
    if ((s = (char *) strchr (user, '\\'))) {
      *s = 0;
      sp->domain = s2b (user);
      *s = '\\';
      if (!sp->domain) {
        sfprintf (sfstderr, "Out of memory\n");
        return -1;
      }
      user = s + 1;
    } else
      sp->domain = NULL;
    sp->user = s2b (user);
    if (!sp->user) {
      sfprintf (sfstderr, "Out of memory\n");
      return -1;
    }
    if (passwd) {
      sp->passwd = s2b (passwd);
      if (!sp->passwd) {
        sfprintf (sfstderr, "Out of memory\n");
        ::SysFreeString (sp->user);
        sp->user = 0;
        return -1;
      }
    }
  }

  if ((hr = sp->pLoc->ConnectServer (
    sp->url,                         // Object path of WMI namespace
    sp->dnu,                         // User name. NULL = current user
    sp->passwd,                      // User password. NULL = current
    0,                               // Locale. NULL indicates current
    WBEM_FLAG_CONNECT_USE_MAX_WAIT,  // Security flags.
    0,                               // Authority (e.g. Kerberos)
    0,                               // Context object
    &sp->pSvc                        // pointer to IWbemServices proxy
  )) != S_OK && hr == WBEM_E_LOCAL_CREDENTIALS)
    hr = sp->pLoc->ConnectServer (
      sp->url,                         // Object path of WMI namespace
      NULL,                            // User name. NULL = current user
      NULL,                            // User password. NULL = current
      0,                               // Locale. NULL indicates current
      WBEM_FLAG_CONNECT_USE_MAX_WAIT,  // Security flags.
      0,                               // Authority (e.g. Kerberos)
      0,                               // Context object
      &sp->pSvc                        // pointer to IWbemServices proxy
    );
  if (hr != S_OK) {
    if (passwordi < passwordm) {
      passwordj = (passwordi++ + passwordl) % passwordm;
      passwd = passwords[passwordj];
      goto again;
    }
    sfprintf (
      sfstderr, "SWMIopen(ConnectServer()) failed: %s\n", E2ccp (hr)
    );
    if (sp->pSvc)
      sp->pSvc->Release ();
    if (sp->pLoc)
      sp->pLoc->Release ();
    CoUninitialize ();
    if (sp->user)
      SysFreeString (sp->user);
    if (sp->passwd)
      SysFreeString (sp->passwd);
    return -1;
  }

  if (SWMIproxysecurity (sp, sp->pSvc) < 0)
    return -1;

  if (passwordm > 0 && passwordj != passwordl) {
    Sfio_t *fp;

    if ((fp = sfopen (NULL, "wmi.lastmulti", "w"))) {
      sfprintf (fp, "%d\n", passwordj);
      sfclose (fp);
    }
  }

  if (verbose)
    sfprintf (sfstderr, "Connected to WMI namespace %s\n", url);
  return 0;
}

int SWMIclose (state_t *sp) {
#if 0
  if (!sp)
    return -1;
  if (sp->pSvc)
    sp->pSvc->Release ();
  if (sp->pLoc)
    sp->pLoc->Release ();
  if (sp->pEnum)
    sp->pEnum->Release ();
  if (sp->pClsObj)
    sp->pClsObj->Release ();

  if (sp->url)
    ::SysFreeString (sp->url);
  CoUninitialize ();

  vmfree (Vmheap, sp);
#endif
  return 0;
}

static COAUTHIDENTITY authid;

int SWMIproxysecurity (state_t *sp, IUnknown *proxy) {
  HRESULT hr;
  DWORD dwAuthnSvc, dwAuthzSvc ,dwAuthnLevel, dwImpLevel, dwCapabilities;
  WCHAR wUserName[1024 + 1], wPassWord[1024 + 1], wDomainName[1024 + 1];
  char UserName[1024 + 1], PassWord[1024 + 1], DomainName[1024 + 1];

  memset ((void *) &authid, 0,sizeof (COAUTHIDENTITY));
  memset ((void*) wUserName, 0, sizeof (wUserName));
  memset ((void*) wPassWord, 0, sizeof (wPassWord));
  memset ((void*) wDomainName, 0, sizeof (wDomainName));

  if (sp->user) {
    if (verbose)
      sfprintf (
        sfstderr, "SWMIproxysecurity for %s %s\n",
        b2s (sp->host), b2s (sp->user)
      );
    if (sp->domain)
      wcscpy (wDomainName, sp->domain);
    wcscpy (wUserName, sp->user);
    wcscpy (wPassWord, sp->passwd);
    if (verbose) {
      if (sp->domain)
        wcstombs (DomainName, wDomainName, sizeof (DomainName));
      else
        DomainName[0] = 0;
      wcstombs (UserName, wUserName, sizeof (UserName));
      wcstombs (PassWord, wPassWord, sizeof (PassWord));
      sfprintf (
        sfstderr, "domain: %s, user: %s, pass: %s\n",
        DomainName, UserName, PassWord
      );
    }

    // retrieve the previous authentication level, so
    // we can echo-back the value when we set the blanket
    if ((hr = CoQueryProxyBlanket (
      proxy,          //Location for the proxy to query
      &dwAuthnSvc,    //Location for the current authentication service
      &dwAuthzSvc,    //Location for the current authorization service
      0,              //Location for the current principal name
      &dwAuthnLevel,  //Location for the current authentication level
      &dwImpLevel,    //Location for the current impersonation level
      0,              //Location for the value to IClientSecurity::SetBlanket
      &dwCapabilities //Location for indicating further capabilities of proxy
    )) != S_OK) {
      sfprintf (
        sfstderr, "SWMIproxysecurity(CoQueryProxyBlanket()) failed: %s\n",
        E2ccp (hr)
      );
      return -1;
    }
    if (verbose)
      sfprintf (
        sfstderr,
        "SWMIproxysecurity(CoQueryProxyBlanket) numbers: %d %d %d %d %d\n",
        dwAuthnSvc, dwAuthzSvc, dwAuthnLevel, dwImpLevel, dwCapabilities
      );

    if (sp->domain) {
      authid.DomainLength   = (ULONG) wcslen (wDomainName);
      authid.Domain         = (USHORT *) (LPWSTR) wDomainName;
    }
    authid.UserLength     = (ULONG) wcslen (wUserName);
    authid.User           = (USHORT *) (LPWSTR) wUserName;
    authid.PasswordLength = (ULONG) wcslen (wPassWord);
    authid.Password       = (USHORT *) (LPWSTR) wPassWord;
    authid.Flags          = SEC_WINNT_AUTH_IDENTITY_UNICODE;

    if ((hr = CoSetProxyBlanket (
      proxy, RPC_C_AUTHN_DEFAULT, RPC_C_AUTHZ_DEFAULT, COLE_DEFAULT_PRINCIPAL,
      dwAuthnLevel, RPC_C_IMP_LEVEL_IMPERSONATE, &authid, EOAC_DEFAULT
    )) != S_OK) {
      sfprintf (
        sfstderr, "SWMIproxysecurity(CoSetProxyBlanket()) failed: %s\n",
        E2ccp (hr)
      );
      return -1;
    }

    // There is a remote IUnknown interface that lurks behind IUnknown.
    // If that is not set, then the Release call can return access denied.

    IUnknown *pUnk = 0;
    if ((hr = sp->pSvc->QueryInterface (
      IID_IUnknown, (void **)&pUnk
    )) != S_OK) {
      sfprintf (
        sfstderr, "SWMIproxysecurity(QueryInterface()) failed: %s\n", E2ccp (hr)
      );
      return -1;
    }
    if ((hr = CoSetProxyBlanket (
      pUnk, RPC_C_AUTHN_DEFAULT, RPC_C_AUTHZ_DEFAULT, COLE_DEFAULT_PRINCIPAL,
      dwAuthnLevel, RPC_C_IMP_LEVEL_IMPERSONATE, &authid, EOAC_DEFAULT
    )) != S_OK) {
      sfprintf (
        sfstderr, "SWMIproxysecurity(CoSetProxyBlanket unk()) failed: %s\n",
        E2ccp (hr)
      );
      return -1;
    }
    pUnk->Release ();
  }
  if(verbose)
    sfprintf (
      sfstderr, "proxy security set up for domain: %s, user: %s, pass: %s\n",
      DomainName, UserName, PassWord
    );
  return 0;
}
