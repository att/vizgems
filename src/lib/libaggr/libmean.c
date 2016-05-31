#pragma prototyped

#include <ast.h>
#include <vmalloc.h>
#include <swift.h>
#define _AGGR_PRIVATE
#include <aggr.h>
#include <aggr_int.h>

int AGGRmean (
    AGGRfile_t **iafps, int iafpn, AGGRfile_t *mafp, AGGRfile_t *dafp
) {
    int keytype, keylen, valtype, vallen, framen;
    int iafpi;
    Vmalloc_t *vm;
    merge_t *merges;
    int rtn;

    if (iafpn < 1 || !iafps || !mafp || !dafp) {
        SUwarning (
            1, "AGGRmean", "bad arguments: %x %d %x %x",
            iafps, iafpn, mafp, dafp
        );
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
            iafps[iafpi]->hdr.framen != framen ||
            iafps[iafpi]->hdr.valtype != valtype ||
            iafps[iafpi]->hdr.vallen != vallen
        ) {
            SUwarning (1, "AGGRmean", "incompatible files");
            return -1;
        }
    }
    if (
        mafp->hdr.keytype != keytype ||
        mafp->ad.keylen != keylen ||
        mafp->hdr.vallen / AGGRtypelens[mafp->hdr.valtype] !=
        vallen / AGGRtypelens[valtype]
    ) {
        SUwarning (1, "AGGRmean", "incompatible files");
        return -1;
    }
    if (
        !(vm = vmopen (Vmdcsbrk, Vmbest, VMFLAGS)) ||
        !(merges = vmalloc (vm, iafpn * sizeof (merge_t)))
    )
        goto abortAGGRmean;
    for (iafpi = 0; iafpi < iafpn; iafpi++)
        if (!(merges[iafpi].maps = vmalloc (
            vm, iafps[iafpi]->ad.itemn * sizeof (int)
        )))
            goto abortAGGRmean;
    if (
        _aggrdictmerge (iafps, iafpn, merges, mafp) == -1 ||
        _aggrdictcopy (mafp, dafp) == -1
    )
        goto abortAGGRmean;
    if (
        _aggrgrow (mafp, mafp->ad.itemn, framen) == -1 ||
        _aggrgrow (dafp, dafp->ad.itemn, framen) == -1
    )
        goto abortAGGRmean;
    for (iafpi = 0; iafpi < iafpn; iafpi++)
        if (!(merges[iafpi].cachep = vmresize (
            vm, NULL, mafp->ad.itemn * vallen, VM_RSZERO
        )))
            goto abortAGGRmean;
    rtn = _aggrmeanrun (
        iafps, iafpn, merges, mafp, dafp, valtype, mafp->hdr.valtype
    );
    vmclose (vm);
    return rtn;

abortAGGRmean:
    SUwarning (1, "AGGRmean", "mean failed");
    SUwarning (1, "AGGRmean", "vm = %x", vm);
    if (vm)
        vmclose (vm);
    return -1;
}
