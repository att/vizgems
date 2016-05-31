#pragma prototyped

#include <ast.h>
#include <swift.h>
#include <ctype.h>
#include "eeval.h"
#include "eeval_int.h"

int EEltok;
char EElstrtok[1000];

EElname_t EElnames[] = {
    /* L_SEMI     */ ";",
    /* L_ASSIGN   */ "=",
    /* L_OR       */ "||",
    /* L_AND      */ "&&",
    /* L_EQ       */ "==",
    /* L_NE       */ "!=",
    /* L_LT       */ "<",
    /* L_LE       */ "<=",
    /* L_GT       */ ">",
    /* L_GE       */ ">=",
    /* L_PLUS     */ "+",
    /* L_MINUS    */ "-",
    /* L_MUL      */ "*",
    /* L_DIV      */ "/",
    /* L_MOD      */ "%",
    /* L_NOT      */ "!",
    /* L_STRING   */ "STRING",
    /* L_NUMBER   */ "NUMBER",
    /* L_ID       */ "IDENTIFIER",
    /* L_LP       */ "LEFT PARENTHESIS",
    /* L_RP       */ "RIGHT PARENTHESIS",
    /* L_LCB      */ "LEFT CURLY BRACE",
    /* L_RCB      */ "RIGHT CURLY BRACE",
    /* L_COLON    */ ":",
    /* L_COMMA    */ ",",
    /* L_IF       */ "IF",
    /* L_ELSE     */ "ELSE",
    /* L_EOF      */ "EOF"
};

static char *unitp, *ucp;
static int seeneof;

#define MAXBUF 10000

/* keyword mapping */
static struct keyword {
    char *str;
    int tok;
} keywords[] = {
    { "if",    L_IF,   },
    { "else",  L_ELSE, },
    { NULL,    0,      },
};

/* single character token mapping */
static struct keychar {
    int chr, tok;
} keychars[] = {
    { ';',    L_SEMI  },
    { '+',    L_PLUS  },
    { '-',    L_MINUS },
    { '*',    L_MUL   },
    { '/',    L_DIV   },
    { '%',    L_MOD   },
    { '(',    L_LP    },
    { ')',    L_RP    },
    { '{',    L_LCB   },
    { '}',    L_RCB   },
    { ':',    L_COLON },
    { ',',    L_COMMA },
    { '\000', 0       }
};

static int gtok (void);
static int sgetc (void);
static void sungetc (void);

void EElsetstr (char *s) {
    unitp = ucp = s;
    seeneof = FALSE;
    EEltok = gtok ();
}

char *EElgetstr (void) {
    return ucp;
}

void EElgtok (void) {
    EEltok = gtok ();
}

static int gtok (void) {
    struct keyword *kwp;
    struct keychar *kcp;
    int c, qc, nc;
    char *p;

    while ((c = sgetc ()) != EOF) {
        if (c == '#')
            while ((c = sgetc ()) != '\n')
                ;
        if (c != ' ' && c != '\t' && c != '\n')
            break;
    }
    if (c == EOF)
        return L_EOF;

    /* check for keywords and identifiers */
    if (isalpha (c) || c == '_') {
        p = &EElstrtok[0], *p++ = c;
        while (isalpha ((c = sgetc ())) || isdigit (c) || c == '_')
            *p++ = c;
        sungetc ();
        *p = '\000';
        for (kwp = &keywords[0]; kwp->str; kwp++)
            if (strcmp (kwp->str, EElstrtok) == 0)
                return kwp->tok;
        return L_ID;
    }

    /* check for number constant */
    if (isdigit (c)) {
        p = &EElstrtok[0], *p++ = c;
        while (isdigit ((c = sgetc ())))
            *p++ = c;
        if (c == '.') {
            *p++ = c;
            while (isdigit ((c = sgetc ())))
                *p++ = c;
        }
        sungetc ();
        *p = '\000';
        return L_NUMBER;
    }

    /* check for string constants */
    if (c == '"' || c == '\'') {
        p = &EElstrtok[0];
        qc = c;
        while ((c = sgetc ()) != EOF && c != qc)
            *p++ = c; /* FIXME: deal with \'s */
        if (c == EOF)
            return L_EOF;
        *p = '\000';
        return L_STRING;
    }

    /* check for single letter keywords */
    for (kcp = &keychars[0]; kcp->chr; kcp++)
        if (kcp->chr == c)
            return kcp->tok;

    /* check for 2/1 letter keywords */
    switch (c) {
    case '=':
    case '!':
    case '<':
    case '>':
        nc = sgetc ();
        if (nc == '=') {
            switch (c) {
            case '=': return L_EQ;
            case '!': return L_NE;
            case '<': return L_LE;
            case '>': return L_GE;
            }
        } else {
            sungetc ();
            switch (c) {
            case '=': return L_ASSIGN;
            case '!': return L_NOT;
            case '<': return L_LT;
            case '>': return L_GT;
            }
        }
        break;
    case '|':
    case '&':
        nc = sgetc ();
        if (nc == c) {
            switch (c) {
            case '|': return L_OR;
            case '&': return L_AND;
            }
        } else {
            sungetc ();
            return L_EOF;
        }
        break;
    }
    return L_EOF;
}

static int sgetc (void) {
    if (seeneof)
        return EOF;
    if (*ucp == '\000') {
        seeneof = TRUE;
        return EOF;
    }
    return *ucp++;
}

static void sungetc (void) {
    if (seeneof) {
        seeneof = FALSE;
        return;
    }
    if (ucp == unitp)
        SUerror ("sungetc", "unget before start of string");
    ucp--;
}
