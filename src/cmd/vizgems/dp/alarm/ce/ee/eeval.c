#pragma prototyped

#include <ast.h>
#include <swift.h>
#include "eeval.h"
#include "eeval_int.h"

int systemerror;

int EEinit (void) {
    if (EEcinit () != 0) {
        SUwarning (0, "EEinit", "code init failed");
        return -1;
    }
    if (EEpinit () != 0) {
        SUwarning (0, "EEinit", "parse init failed");
        return -1;
    }
    if (EEeinit () != 0) {
        SUwarning (0, "EEinit", "exec init failed");
        return -1;
    }
    return 0;
}

int EEterm (void) {
    if (EEeterm () != 0) {
        SUwarning (0, "EEterm", "exec term failed");
        return -1;
    }
    if (EEpterm () != 0) {
        SUwarning (0, "EEterm", "parse term failed");
        return -1;
    }
    if (EEcterm () != 0) {
        SUwarning (0, "EEterm", "code term failed");
        return -1;
    }
    return 0;
}

int EEparse (char *codestr, char **dict, EE_t *eep) {
    return EEpunit (codestr, dict, eep);
}

void EEprint (EE_t *eep, int initialindent) {
    EEpprint (eep, initialindent);
}

int EEeval (
    EE_t *eep, EEgetvar getfunc, EEsetvar setfunc, void *context, int peval
) {
    return EEeunit (eep, getfunc, setfunc, context, peval);
}
