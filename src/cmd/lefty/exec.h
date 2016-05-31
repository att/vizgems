#pragma prototyped
/* Lefteris Koutsofios - AT&T Labs Research */

#ifndef _EXEC_H
#define _EXEC_H
typedef struct Tonm_t lvar_t;

extern Tobj root, null;
extern Tobj rtno;
extern int Erun;
extern int Eerrlevel, Estackdepth, Eshowbody, Eshowcalls, Eoktorun;

void Einit (void);
void Eterm (void);
Tobj Eunit (Tobj);
Tobj Efunction (Tobj, char *);
#endif /* _EXEC_H */
