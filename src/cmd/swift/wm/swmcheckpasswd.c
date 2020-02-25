#include <ast.h>
#include <swift.h>
#include <FEATURE/crypt>
#ifdef _hdr_crypt
#include <crypt.h>
#else
#include <unistd.h>
#endif

int main (int argc, char **argv) {
    char *plain, *encrypted, *result, settings[4];

    plain = argv[1];
    encrypted = argv[2];
    strncpy (settings, encrypted, 2);
    settings[2] = 0;

    result = crypt (plain, settings);
    SUwarning (1, "swmcheckpasswd", "%s / %s", encrypted, result);
    if (strcmp (result, encrypted) == 0)
        return 0;
    else
        return 1;
}
