#pragma prototyped
/* Lefteris Koutsofios - AT&T Labs Research */

#ifndef _STR_H
#define _STR_H
void Sinit (void);
void Sterm (void);
char *Spath (char *, Tobj);
char *Sseen (Tobj, char *);
char *Sabstract (Tobj, Tobj);
char *Stfull (Tobj);
char *Ssfull (Tobj, Tobj);
char *Scfull (Tobj, int, int);
#endif /* _STR_H */
