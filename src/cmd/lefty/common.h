#pragma prototyped
/* Lefteris Koutsofios - AT&T Labs Research */

#ifndef _COMMON_H
#define _COMMON_H

#include <ast.h>
#include "config.h"

#include <math.h>
#include <stdio.h>
#include <setjmp.h>
#include <ctype.h>

#ifdef FEATURE_WIN32
#include <windows.h>
#include <commdlg.h>
#endif

#ifdef FEATURE_MS
#include <malloc.h>
#endif

#define POS __FILE__, __LINE__

#ifndef TRUE
#define TRUE  1
#define FALSE 0
#endif

#ifndef L_SUCCESS
#define L_SUCCESS 1
#define L_FAILURE 0
#endif

#define CHARSRC 0
#define FILESRC 1

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifdef FEATURE_MS
#define printf gprintf
#endif

extern int warnflag;
extern char *leftypath, *leftyoptions, *shellpath;
extern jmp_buf exitljbuf;
extern int idlerunmode;
extern fd_set inputfds;

int init (char *);
void term (void);
char *buildpath (char *, int);
char *buildcommand (char *, char *, int, int, char *);
void warning (char *, int, char *, char *, ...);
void panic (char *, int, char *, char *, ...);
void panic2 (char *, int, char *, char *, ...);
#endif /* _COMMON_H */
