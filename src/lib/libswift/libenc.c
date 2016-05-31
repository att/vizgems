#include <ast.h>
#include <vmalloc.h>
#include <swift.h>

#define MAX(a, b) (((a) >= (b)) ? (a) : (b))

static unsigned char hexmap[16] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

static int is[] = {
    15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0
};

static unsigned char decmap[] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, 0, 10, 11, 12, 13, 14, 15
};

static unsigned char *cnvs, *obuf;
static int cnvn;
static int *pis, pn;

int SWasciienc (unsigned char *, unsigned char **);
int SWasciidec (unsigned char *, unsigned char **);

int SWenc (Vmalloc_t *vm, unsigned char *astr, unsigned char **estrp) {
    int elen, eoff, esiz, i, j, k, l, m, n, r1, r2;

    if (!vm) {
        SUwarning (0, "SWenc", "bad vm arena");
        return -1;
    }
    if (!astr || !estrp) {
        SUwarning (0, "SWenc", "bad string pointers");
        return -1;
    }
    if ((n = strlen ((char *) astr)) < 1 || n > 250) {
        SUwarning (0, "SWenc", "ascii string must have 1-250 characters");
        return -1;
    }

    r1 = random () % 15;
    r2 = random () % 15;
    r1 = MAX (r1, 5);
    r2 = MAX (r2, 5);
    eoff = r1, esiz = 3 * n;
    elen = r1 + 3 * n + r2;

    if (cnvn < elen) {
        if (!(cnvs = vmresize (vm, cnvs, sizeof (char) * elen, VM_RSMOVE))) {
            SUwarning (0, "SWenc", "cannot grow cnvs");
            return -1;
        }
        if (!(obuf = vmresize (
            vm, obuf, sizeof (char) * (elen * 2 + 3), VM_RSMOVE
        ))) {
            SUwarning (0, "SWenc", "cannot grow obuf");
            return -1;
        }
        cnvn = elen;
    }
    if (pn < n) {
        if (!(pis = vmresize (vm, pis, sizeof (int) * n, 0))) {
            SUwarning (0, "SWenc", "cannot grow pis");
            return -1;
        }
        pn = n;
    }

    for (i = 0; i < elen; i++)
        cnvs[i] = random () % 128;
    for (i = 0, j = -1; i < eoff - 2; i++) {
        if (cnvs[i] == 42) {
            j = i, cnvs[i + 1] = eoff, cnvs[i + 2] = n;
            break;
        }
    }
    if (j == -1) {
        j = random () % (eoff - 2);
        cnvs[j] = 42, cnvs[j + 1] = eoff, cnvs[j + 2] = n;
    }
    for (i = 0; i < n; i++)
        pis[i] = i;

    for (i = 0, m = n, l = 0; i < n; i++) {
        j = random () % m;
        k = pis[j];
        pis[j] = pis[m - 1], m--;

        cnvs[eoff + 3 * i + l] = astr[k];
        cnvs[eoff + 3 * i + l + 1] = k;
        l = astr[k] % 2;
    }

    for (i = 0; i < elen; i++) {
        obuf[2 * i + 2] = hexmap[is[(cnvs[i] >> 4) & 0xf]];
        obuf[2 * i + 3] = hexmap[is[(cnvs[i]) & 0xf]];
    }
    obuf[0] = '0', obuf[1] = '1';
    obuf[2 * elen + 2] = 0;

    *estrp = obuf;
    return 0;
}

int SWdec (Vmalloc_t *vm, unsigned char *estr, unsigned char **astrp) {
    int elen, eoff, esiz, i, k, l, n;

    if (!vm) {
        SUwarning (0, "SWdec", "bad vm arena");
        return -1;
    }
    if (!estr || !astrp) {
        SUwarning (0, "SWdec", "bad string pointers");
        return -1;
    }

    if (estr[0] != '0' || estr[1] != '1') {
        SUwarning (0, "SWdec", "bad version number");
        return -1;
    }
    estr += 2;
    if ((elen = strlen ((char *) estr)) < 20) {
        SUwarning (0, "SWdec", "encoded string less than 20 characters");
        return -1;
    }

    if (cnvn < elen) {
        if (!(cnvs = vmresize (vm, cnvs, sizeof (char) * elen, VM_RSMOVE))) {
            SUwarning (0, "SWdec", "cannot grow cnvs");
            return -1;
        }
        if (!(obuf = vmresize (
            vm, obuf, sizeof (char) * (elen * 2 + 3), VM_RSMOVE
        ))) {
            SUwarning (0, "SWdec", "cannot grow obuf");
            return -1;
        }
        cnvn = elen;
    }
    for (i = 0; i < elen; i += 2) {
        cnvs[i / 2] = (
            is[decmap[estr[i + 0] - '0']] << 4
        ) + is[decmap[estr[i + 1] - '0']];
    }
    elen /= 2;
    eoff = esiz = -1;
    for (i = 0; i < elen - 2; i++) {
        if (cnvs[i] == 42) {
            eoff = cnvs[i + 1], n = cnvs[i + 2], esiz = 3 * n;
            break;
        }
    }
    if (eoff == -1 || n < 1 || n > 256) {
        SUwarning (0, "SWdec", "cannot read offset and size");
        return -1;
    }
    for (i = 0, l = 0; i < n; i++) {
        k = cnvs[eoff + 3 * i + l + 1];
        obuf[k] = cnvs[eoff + 3 * i + l];
        l = obuf[k] % 2;
    }
    obuf[n] = 0;

    *astrp = obuf;
    return 0;
}
