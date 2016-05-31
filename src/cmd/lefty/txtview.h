#pragma prototyped
/* Lefteris Koutsofios - AT&T Labs Research */

#ifndef _TXTVIEW_H
#define _TXTVIEW_H
void TXTinit (Grect_t);
void TXTterm (void);
int TXTmode (int argc, lvar_t *argv);
int TXTask (int argc, lvar_t *argv);
void TXTprocess (int, char *);
void TXTupdate (void);
void TXTtoggle (int, void *);
#endif /* _TXTVIEW_H */
