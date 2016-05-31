#pragma prototyped

#include <ast.h>
#include <swift.h>

int main (int argc, char **argv) {
    sfprintf (sfstdout, "%d\n", SWIFT_ENDIAN);
    return 0;
}
