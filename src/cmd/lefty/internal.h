#pragma prototyped
/* Lefteris Koutsofios - AT&T Labs Research */

#ifndef _INTERNAL_H
#define _INTERNAL_H
typedef struct Ifunc_t {
    char *name;
    int (*func) (int, Tonm_t *);
    int min, max;
} Ifunc_t;

void Iinit (void);
void Iterm (void);
int Igetfunc (char *);

extern Ifunc_t Ifuncs[];
extern int Ifuncn;
#endif /* _INTERNAL_H */
