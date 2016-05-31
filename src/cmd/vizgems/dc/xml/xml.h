#pragma prototyped

#ifndef XML_H
#define XML_H

#include <ast.h>
#include <vmalloc.h>
#include <swift.h>

#define XML_TYPE_TEXT 1
#define XML_TYPE_TAG 2

typedef struct attr_s {
    char *key, *val;
} attr_t;

typedef struct XMLnode_s XMLnode_t;
struct XMLnode_s {
    int type;
    char *tag;
    attr_t **attrs;
    int attrn, attrm;
    XMLnode_t **nodes;
    int noden, nodem;
    char *text;
    XMLnode_t *parentp;
    int indexplusone;
};

_BEGIN_EXTERNS_ /* public data */
#if _BLD_vgxml && defined(__EXPORT__)
#define __PUBLIC_DATA__ __EXPORT__
#else
#if !_BLD_vgxml && defined(__IMPORT__)
#define __PUBLIC_DATA__ __IMPORT__
#else
#define __PUBLIC_DATA__
#endif
#endif

#undef __PUBLIC_DATA__

#if _BLD_vgxml && defined(__EXPORT__)
#define extern  __EXPORT__
#endif

extern XMLnode_t *XMLreadfile (Sfio_t *, char *, int, Vmalloc_t *);
extern XMLnode_t *XMLreadstring (char *, char *, int, Vmalloc_t *);
extern XMLnode_t *XMLfind (XMLnode_t *, char *, int, int, int);
extern void XMLwriteksh (Sfio_t *, XMLnode_t *, int, int);
extern void XMLwriteascii (Sfio_t *, XMLnode_t *, int);

extern XMLnode_t *XMLflattenread (Sfio_t *, int, Vmalloc_t *);
extern int XMLflattenwrite (Sfio_t *, XMLnode_t *, char *, int);

#undef extern
_END_EXTERNS_

#endif
