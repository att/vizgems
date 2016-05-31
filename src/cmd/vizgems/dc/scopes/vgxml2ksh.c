#include <ast.h>
#include <swift.h>
#include <stdio.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

static Sfio_t *strfp;

static void xml_parse (xmlNode *);

int main (int argc, char **argv) {
    xmlDoc *dp;
    xmlNode *np;
    Sfio_t *strfp;

    LIBXML_TEST_VERSION

    if (argc < 2)
        SUerror ("xml2ksh", "missing top level variable name");
    if (!(strfp = sfstropen ()))
        SUerror ("xml2ksh", "cannot allocate sting stream");
    if (sfmove (sfstdin, strfp, -1, -1) == -1)
        SUerror ("xml2ksh", "cannot read xml data");

    if (!(dp = xmlReadMemory (
        sfstruse (strfp), sfsize (strfp), "URL", NULL, XML_PARSE_NOBLANKS
    )))
        SUerror ("xml2ksh", "cannot parse xml data");

    sfprintf (sfstdout, "%s=(\n", argv[1]);
    xml_parse (xmlDocGetRootElement (dp));
    sfprintf (sfstdout, ")\n");
}

static void xml_parse (xmlNode *np) {
    xmlNode *cnp;
    xmlAttr *ap;
    int ci;

    if (np->type != XML_ELEMENT_NODE) {
        sfprintf (sfstdout, "text=\"%s\"\n", np->content);
        return;
    }

    if (np->type != XML_ELEMENT_NODE) {
        SUwarning (0, "xml_parse", "unknown node type %d", np->type);
        return;
    }

    sfprintf (sfstdout, "name=\"%s\"\n", np->name);
    if (np->ns && np->ns->prefix)
        sfprintf (sfstdout, "ns=\"%s\"\n", np->ns->prefix);

    sfprintf (sfstdout, "typeset -A as=(\n");
    for (ap = np->properties; ap; ap = ap->next) {
        if (ap->type != XML_ATTRIBUTE_NODE) {
            if (ap->type != XML_CDATA_SECTION_NODE)
                SUwarning (
                    0, "xml_parse", "unknown property type %d", ap->type
                );
            continue;
        }
        if (!ap->children)
            continue;
        if (ap->children->type != XML_TEXT_NODE) {
            if (ap->children->type != XML_CDATA_SECTION_NODE)
                SUwarning (
                    0, "xml_parse", "unknown property value type %d",
                    ap->children->type
                );
            continue;
        }
        if (ap->ns && ap->ns->prefix)
            sfprintf (
                sfstdout, "[\"%s:%s\"]=\"%s\"\n",
                ap->ns->prefix, ap->name, ap->children->content
            );
        else
            sfprintf (
                sfstdout, "[\"%s\"]=\"%s\"\n", ap->name, ap->children->content
            );
    }
    sfprintf (sfstdout, ")\n");

    ci = 0;
    sfprintf (sfstdout, "typeset -A cs=(\n");
    for (cnp = np->children; cnp; cnp = cnp->next) {
        if (cnp->type != XML_ELEMENT_NODE && cnp->type != XML_TEXT_NODE) {
            if (cnp->type != XML_CDATA_SECTION_NODE)
                SUwarning (
                    0, "xml_parse", "unknown property type %d", cnp->type
                );
            continue;
        }
        sfprintf (sfstdout, "[%d]=(\n", ci++);
        xml_parse (cnp);
        sfprintf (sfstdout, ")\n");
    }
    sfprintf (sfstdout, ")\n");
}
