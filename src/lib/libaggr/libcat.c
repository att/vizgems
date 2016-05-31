#pragma prototyped

#include <ast.h>
#include <vmalloc.h>
#include <swift.h>
#define _AGGR_PRIVATE
#include <aggr.h>
#include <aggr_int.h>

int AGGRcat (AGGRfile_t **iafps, int iafpn, AGGRfile_t *oafp) {
    int keytype, keylen, valtype, vallen, framen;
    int iafpi;
    Vmalloc_t *vm;
    merge_t *merges;
    int rtn;

    if (iafpn < 1 || !iafps || !oafp) {
        SUwarning (1, "AGGRcat", "bad arguments: %x %d %x", iafps, iafpn, oafp);
        return -1;
    }
    keytype = iafps[0]->hdr.keytype;
    keylen = iafps[0]->ad.keylen;
    valtype = iafps[0]->hdr.valtype;
    vallen = iafps[0]->hdr.vallen;
    framen = iafps[0]->hdr.framen;
    for (iafpi = 1; iafpi < iafpn; iafpi++) {
        if (
            iafps[iafpi]->hdr.keytype != keytype ||
            iafps[iafpi]->ad.keylen != keylen ||
            iafps[iafpi]->hdr.valtype != valtype ||
            iafps[iafpi]->hdr.vallen != vallen
        ) {
            SUwarning (1, "AGGRcat", "incompatible files");
            return -1;
        }
        framen += iafps[iafpi]->hdr.framen;
    }
    if (
        oafp->hdr.keytype != keytype ||
        oafp->ad.keylen != keylen ||
        oafp->hdr.vallen / AGGRtypelens[oafp->hdr.valtype] !=
        vallen / AGGRtypelens[valtype]
    ) {
        SUwarning (1, "AGGRcat", "incompatible files");
        return -1;
    }
    if (
        !(vm = vmopen (Vmdcsbrk, Vmbest, VMFLAGS)) ||
        !(merges = vmalloc (vm, iafpn * sizeof (merge_t)))
    )
        goto abortAGGRcat;
    for (iafpi = 0; iafpi < iafpn; iafpi++)
        if (!(merges[iafpi].maps = vmalloc (
            vm, iafps[iafpi]->ad.itemn * sizeof (int)
        )))
            goto abortAGGRcat;
    if (_aggrdictmerge (iafps, iafpn, merges, oafp) == -1)
        goto abortAGGRcat;
    if (_aggrgrow (oafp, oafp->ad.itemn, framen) == -1)
        goto abortAGGRcat;
    rtn = _aggrcatrun (iafps, iafpn, merges, oafp, valtype, oafp->hdr.valtype);
    vmclose (vm);
    return rtn;

abortAGGRcat:
    SUwarning (1, "AGGRcat", "cat failed");
    SUwarning (1, "AGGRcat", "vm = %x", vm);
    if (vm)
        vmclose (vm);
    return -1;
}
