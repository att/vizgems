#pragma prototyped

#include <ast.h>
#include <vmalloc.h>
#include <swift.h>
#define _AGGR_PRIVATE
#include <aggr.h>
#include <aggr_int.h>

int AGGRextract (
    AGGRfile_t *iafp, AGGRextract_t *extracts, int extractn, AGGRfile_t *oafp
) {
    int keytype, keylen, valtype, vallen, framen;
    int extracti, itemi;
    merge_t merge;
    int rtn;

    if (!iafp || extractn < 0 || !extracts || !oafp) {
        SUwarning (
            1, "AGGRextract", "bad arguments: %x %x %d %x",
            iafp, extracts, extractn, oafp
        );
        return -1;
    }
    keytype = iafp->hdr.keytype;
    keylen = iafp->ad.keylen;
    valtype = iafp->hdr.valtype;
    vallen = iafp->hdr.vallen;
    framen = iafp->hdr.framen;
    if (
        oafp->hdr.keytype != keytype ||
        oafp->ad.keylen != keylen ||
        oafp->hdr.vallen / AGGRtypelens[oafp->hdr.valtype] !=
        vallen / AGGRtypelens[valtype]
    ) {
        SUwarning (1, "AGGRextract", "incompatible files");
        return -1;
    }
    merge.maps = NULL;
    if (!(merge.maps = vmalloc (Vmheap, iafp->ad.itemn * sizeof (int))))
        goto abortAGGRextract;
    for (itemi = 0; itemi < iafp->ad.itemn; itemi++)
        merge.maps[itemi] = -1;
    switch (iafp->ad.keylen) {
    case -1:
        for (extracti = 0; extracti < extractn; extracti++)
            if (_aggrdictlookupvar (
                iafp, extracts[extracti].keyp, -1, FALSE
            ) == -1)
                extracts[extracti].keyp = NULL;
        break;
    default:
        for (extracti = 0; extracti < extractn; extracti++)
            if (_aggrdictlookupfixed (
                iafp, extracts[extracti].keyp, -1, FALSE
            ) == -1)
                extracts[extracti].keyp = NULL;
        break;
    }
    switch (oafp->ad.keylen) {
    case -1:
        for (extracti = 0; extracti < extractn; extracti++) {
            if (!extracts[extracti].keyp)
                continue;
            extracts[extracti].nitemi = _aggrdictlookupvar (
                oafp, extracts[extracti].keyp, extracts[extracti].nitemi, TRUE
            );
        }
        break;
    default:
        for (extracti = 0; extracti < extractn; extracti++) {
            if (!extracts[extracti].keyp)
                continue;
            extracts[extracti].nitemi = _aggrdictlookupfixed (
                oafp, extracts[extracti].keyp, extracts[extracti].nitemi, TRUE
            );
        }
        break;
    }
    switch (iafp->ad.keylen) {
    case -1:
        for (extracti = 0; extracti < extractn; extracti++) {
            if (!extracts[extracti].keyp)
                continue;
            itemi = _aggrdictlookupvar (
                iafp, extracts[extracti].keyp, -1, FALSE
            );
            if (itemi != -1 && extracts[extracti].nitemi != -1)
                merge.maps[itemi] = extracts[extracti].nitemi;
        }
        break;
    default:
        for (extracti = 0; extracti < extractn; extracti++) {
            if (!extracts[extracti].keyp)
                continue;
            itemi = _aggrdictlookupfixed (
                iafp, extracts[extracti].keyp, -1, FALSE
            );
            if (itemi != -1 && extracts[extracti].nitemi != -1)
                merge.maps[itemi] = extracts[extracti].nitemi;
        }
        break;
    }
    if (_aggrgrow (oafp, oafp->ad.itemn, framen) == -1)
        goto abortAGGRextract;
    rtn = _aggrextractrun (
        iafp, &merge, oafp, iafp->hdr.valtype, oafp->hdr.valtype
    );
    vmfree (Vmheap, merge.maps);
    return rtn;

abortAGGRextract:
    SUwarning (1, "AGGRextract", "extract failed");
    SUwarning (1, "AGGRextract", "merge.maps = %x", merge.maps);
    if (merge.maps)
        vmfree (Vmheap, merge.maps);
    return -1;
}
