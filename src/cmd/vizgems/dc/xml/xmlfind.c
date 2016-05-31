#pragma prototyped

#include <ast.h>
#include <vmalloc.h>
#include <swift.h>
#include "xml.h"

XMLnode_t *XMLfind (
    XMLnode_t *nodep, char *name, int type, int index, int sflag
) {
    XMLnode_t *np;
    int ni, rtype;

    if (!nodep || nodep->type == XML_TYPE_TEXT)
        return NULL;

    rtype = type;
    if (sflag && type == XML_TYPE_TEXT)
        type = XML_TYPE_TAG;
    for (ni = 0; ni < nodep->nodem; ni++) {
        np = nodep->nodes[ni];
        if (type != -1 && type != np->type)
            continue;
        if (index != -1 && index != np->indexplusone - 1)
            continue;
        if (np->type == XML_TYPE_TAG && strcmp (name, np->tag) != 0)
            continue;
        if (sflag && rtype == XML_TYPE_TEXT)
            if (np->nodem == 1 && np->nodes[0]->type == rtype)
                return np->nodes[0];
            else
                return NULL;
        return np;
    }
    return NULL;
}
