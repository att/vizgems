#ifndef WMI_H

typedef struct state_s {
  BSTR                 url;
  BSTR                 host;
  BSTR                 ns;
  BSTR                 dnu;
  BSTR                 domain;
  BSTR                 user;
  BSTR                 passwd;
  IWbemLocator         *pLoc;
  IWbemServices        *pSvc;
  IEnumWbemClassObject *pEnum;
  IWbemClassObject     *pClsObj;
  IWbemQualifierSet    *pQualSet;
} state_t;

extern int verbose;

state_t *SWMIopen (void);
int SWMIconnect (
  state_t *, const char *, const char *, const char *, const char *
);
int SWMIclose (state_t *);
int SWMIproxysecurity (state_t *, IUnknown *);

int SWMIopnload (char *, char *);
int SWMIopnsave (char *);
int SWMIopnexec (state_t *);

int SWMIlogload (char *, char *);
int SWMIlogsave (char *);
int SWMIlogexec (char *, state_t *);

int SWMIinvload (char *);
int SWMIinvsave (void);
int SWMIinvexec (state_t *, int);

#endif /* WMI_H */
