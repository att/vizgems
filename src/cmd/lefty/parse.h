#pragma prototyped
/* Lefteris Koutsofios - AT&T Labs Research */

#ifndef _PARSE_H
#define _PARSE_H
typedef struct Psrc_t {
    int flag;
    char *s;
    FILE *fp;
    int tok;
    int lnum;
} Psrc_t;

void Pinit (void);
void Pterm (void);
Tobj Punit (Psrc_t *);
Tobj Pfcall (Tobj, Tobj);
Tobj Pfunction (char *, int);
#endif /* _PARSE_H */
