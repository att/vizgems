#pragma prototyped

#include <ast.h>
#include <vmalloc.h>
#include <swift.h>
#include <ctype.h>
#include "xml.h"

static XMLnode_t *root;
static Vmalloc_t *xvm;

static char *filestr;
static int filelen;

static XMLnode_t *newnode (int, XMLnode_t *);
static XMLnode_t *findnode (XMLnode_t *, char *);
static attr_t *addattr (XMLnode_t *, char *, char *);
static void finalize (XMLnode_t *);

XMLnode_t *XMLreadfile (Sfio_t *fp, char *toptag, int iwsflag, Vmalloc_t *vm) {
    Sfio_t *sp;
    char *s1;
    XMLnode_t *np;
    int len;

    if (sfseek (fp, 0L, 1) < 0) {
        if (!(sp = sfopen (NULL, 0, "s+"))) {
            SUwarning (0, "XMLreadfile", "cannot open string fd");
            return NULL;
        }
        if (sfmove (fp, sp, -1, -1) == -1) {
            SUwarning (0, "XMLreadfile", "cannot copy contents");
            return NULL;
        }
        fp = sp;
    }
    if ((len = sfsize (fp)) == 0)
        return XMLreadstring ("", toptag, iwsflag, vm);

    if (!(s1 = sfreserve (fp, len, 0))) {
        SUwarning (0, "XMLreadfile", "cannot reserve contents");
        return NULL;
    }
    if (!(filestr = vmalloc (vm, (len + 1)))) {
        SUwarning (0, "XMLreadfile", "cannot allocate filestr");
        return NULL;
    }
    filelen = len + 1;
    memcpy (filestr, s1, len);
    filestr[len] = 0;
    np = XMLreadstring (filestr, toptag, iwsflag, vm);
    return np;
}

XMLnode_t *XMLreadstring (char *str, char *toptag, int iwsflag, Vmalloc_t *vm) {
    XMLnode_t *cnp, *tnp;
    char *s1, *s2, *s3, *s4, *kvp, *kp, c;
    int seeneq, seenkey, l;

    xvm = vm;
    if (!(cnp = root = newnode (XML_TYPE_TAG, NULL))) {
        SUwarning (0, "XMLreadstring", "cannot create root node");
        return NULL;
    }
    if (!(cnp->tag = vmstrdup (xvm, toptag))) {
        SUwarning (0, "XMLreadstring", "cannot copy tag");
        return NULL;
    }

    s1 = str;
    while (*s1) {
        if (!(s2 = strchr (s1, '<')) || !(s3 = strchr (s2 + 1, '>')))
            break;
        *s2++ = *s3 = 0;

        if (s1 + 1 != s2) {
            if (iwsflag) {
                while (isspace (*s1))
                    s1++;
                for (s4 = s2 - 2; isspace (*s4); s4--)
                    ;
                *++s4 = 0;
            }
            if (*s1) {
                if (!(tnp = newnode (XML_TYPE_TEXT, cnp))) {
                    SUwarning (0, "XMLreadstring", "cannot create text node");
                    return NULL;
                }
                if (!(tnp->text = vmstrdup (xvm, s1))) {
                    SUwarning (0, "XMLreadstring", "cannot copy text");
                    return NULL;
                }
            }
        }

        while (isspace (*s2))
            s2++;
        if (!*s2) {
            SUwarning (0, "XMLreadstring", "EOF while skipping tag space");
            return NULL;
        }
        if (!*s2) {
            SUwarning (0, "XMLreadstring", "empty tag");
            return NULL;
        }
        if (*s2 == '<') {
            SUwarning (0, "XMLreadstring", "nested tag");
            return NULL;
        }

        for (s4 = s2; *s4 && !isspace (*s4) && *s4 != '='; s4++)
            ;

        if (*s2 == '=') {
            SUwarning (0, "XMLreadstring", "attribute pair before tag name");
            return NULL;
        }
        if (s4 != s3)
            *s4++ = 0;

        if (*s2 == '/') {
            if (!(cnp = findnode (cnp, s2 + 1))) {
                SUwarning (0, "XMLreadstring", "cannot match node");
                return NULL;
            }
            s1 = s3 + 1;
            continue;
        }

        if (!(tnp = newnode (XML_TYPE_TAG, cnp))) {
            SUwarning (0, "XMLreadstring", "cannot create tag node");
            return NULL;
        }

        if (!(tnp->tag = vmstrdup (xvm, s2))) {
            SUwarning (0, "XMLreadstring", "cannot copy tag");
            return NULL;
        }
        l = strlen (tnp->tag);
        if (l > 0 && tnp->tag[l - 1] == '/')
            tnp->tag[l - 1] = 0;
        s2 = s4;

        for (seeneq = FALSE, seenkey = FALSE; ; ) {
            while (isspace (*s2))
                s2++;
            if (!*s2)
                break;
            if (*s2 == '<') {
                SUwarning (0, "XMLreadstring", "nested tag");
                return NULL;
            }

            if (*s2 == '=') {
                if (!seenkey) {
                    SUwarning (0, "XMLreadstring", "'=' before attribute name");
                    return NULL;
                }
                seeneq = TRUE;
                s2++;
                continue;
            }

            if (*s2 == '"' || *s2 == '\'') {
                for (s4 = s2 + 1; *s4 && *s4 != *s2; s4++)
                    ;
                if (*s4 != *s2) {
                    SUwarning (0, "XMLreadstring", "unmatched quotes");
                    return NULL;
                }
                *s4++ = 0;
                if (!(kvp = vmstrdup (xvm, s2 + 1))) {
                    SUwarning (0, "XMLreadstring", "cannot copy text");
                    return NULL;
                }
            } else {
                for (s4 = s2; *s4 && !isspace (*s4) && *s4 != '='; s4++)
                    ;
                c = *s4, *s4 = 0;
                if (!(kvp = vmstrdup (xvm, s2))) {
                    SUwarning (0, "XMLreadstring", "cannot copy text");
                    return NULL;
                }
                *s4 = c;
            }
            if (seenkey) {
                if (seeneq) {
                    if (!addattr (tnp, kp, kvp)) {
                        SUwarning (
                            0, "XMLreadstring", "cannot add kv attribute"
                        );
                        return NULL;
                    }
                    seenkey = seeneq = FALSE;
                } else {
                    if (!addattr (tnp, kp, NULL)) {
                        SUwarning (
                            0, "XMLreadstring", "cannot add unary attribute"
                        );
                        return NULL;
                    }
                    kp = kvp;
                }
            } else {
                kp = kvp;
                seenkey = TRUE;
            }
            s2 = s4;
        }
        if (seenkey) {
            if (!addattr (tnp, kp, NULL)) {
                SUwarning (0, "XMLreadstring", "cannot add unary attribute");
                return NULL;
            }
        }
        s1 = s3 + 1;

        if (*(s3 - 1) != '/')
            cnp = tnp;
    }
    if (*s1) {
        if (iwsflag) {
            while (isspace (*s1))
                s1++;
            for (s4 = s1 + strlen (s1) - 1; isspace (*s4); s4--)
                ;
            *++s4 = 0;
        }
        if (*s1) {
            if (!(tnp = newnode (XML_TYPE_TEXT, cnp))) {
                SUwarning (0, "XMLreadstring", "cannot create text node");
                return NULL;
            }
            if (!(tnp->text = vmstrdup (xvm, s1))) {
                SUwarning (0, "XMLreadstring", "cannot copy text");
                return NULL;
            }
        }
    }
    finalize (root);

    return root;
}

static XMLnode_t *newnode (int type, XMLnode_t *parentp) {
    XMLnode_t *np;

    if (!(np = vmalloc (xvm, sizeof (XMLnode_t)))) {
        SUwarning (0, "newnode", "cannot allocate node");
        return NULL;
    }
    memset (np, 0, sizeof (XMLnode_t));
    np->type = type;
    if (!parentp)
        return np;

    if (parentp->nodem >= parentp->noden) {
        if (!(parentp->nodes = vmresize (
            xvm, parentp->nodes,
            (parentp->noden * 2 + 10) * sizeof (XMLnode_t *), VM_RSCOPY
        ))) {
            SUwarning (0, "newnode", "cannot grow node array");
            return NULL;
        }
        parentp->noden = parentp->noden * 2 + 10;
    }
    parentp->nodes[parentp->nodem++] = np;
    np->parentp = parentp;
    return np;
}

static XMLnode_t *findnode (XMLnode_t *nodep, char *tag) {
    XMLnode_t *np;

    for (np = nodep; np; np = np->parentp)
        if (strcasecmp (tag, np->tag) == 0)
            return np->parentp;
    return nodep;
}

static attr_t *addattr (XMLnode_t *np, char *kp, char *vp) {
    attr_t *ap;

    if (!(ap = vmalloc (xvm, sizeof (attr_t)))) {
        SUwarning (0, "addattr", "cannot allocate attr");
        return NULL;
    }
    memset (ap, 0, sizeof (attr_t));
    ap->key = kp, ap->val = vp;
    if (np->attrm >= np->attrn) {
        if (!(np->attrs = vmresize (
            xvm, np->attrs,
            (np->attrn * 2 + 2) * sizeof (attr_t *), VM_RSCOPY
        ))) {
            SUwarning (0, "addattr", "cannot grow attr array");
            return NULL;
        }
        np->attrn = np->attrn * 2 + 2;
    }
    np->attrs[np->attrm++] = ap;
    return ap;
}

static void finalize (XMLnode_t *nodep) {
    XMLnode_t *np1, *np2;
    int ni1, ni2, indexplusone;

    for (ni1 = 0; ni1 < nodep->nodem; ni1++) {
        np1 = nodep->nodes[ni1];
        if (np1->type != XML_TYPE_TAG || np1->indexplusone > 0)
            continue;
        indexplusone = 1;
        for (ni2 = ni1 + 1; ni2 < nodep->nodem; ni2++) {
            np2 = nodep->nodes[ni2];
            if (np2->type != XML_TYPE_TAG)
                continue;
            if (strcmp (np1->tag, np2->tag) != 0)
                continue;
            if (indexplusone == 1)
                np1->indexplusone = indexplusone++;
            np2->indexplusone = indexplusone++;
        }
    }
    indexplusone = 1;
    for (ni1 = 0; ni1 < nodep->nodem; ni1++) {
        np1 = nodep->nodes[ni1];
        if (np1->type != XML_TYPE_TEXT)
            continue;
        np1->indexplusone = indexplusone++;
    }
    for (ni1 = 0; ni1 < nodep->nodem; ni1++) {
        np1 = nodep->nodes[ni1];
        finalize (np1);
    }
}
