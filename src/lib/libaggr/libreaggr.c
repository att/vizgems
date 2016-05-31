#pragma prototyped

#include <ast.h>
#include <vmalloc.h>
#include <swift.h>
#define _AGGR_PRIVATE
#include <aggr.h>
#include <aggr_int.h>

int AGGRreaggr (
    AGGRfile_t *iafp, AGGRreaggr_t *reaggrs, int reaggrn, AGGRfile_t *oafp
) {
    int valtype, vallen, framen;
    int reaggri, itemi;
    merge_t merge;
    int rtn;

    if (!iafp || reaggrn < 0 || !reaggrs || !oafp) {
        SUwarning (
            1, "AGGRreaggr", "bad arguments: %x %x %d %x",
            iafp, reaggrs, reaggrn, oafp
        );
        return -1;
    }
    valtype = iafp->hdr.valtype;
    vallen = iafp->hdr.vallen;
    framen = iafp->hdr.framen;
    if (
        oafp->hdr.vallen / AGGRtypelens[oafp->hdr.valtype] !=
        vallen / AGGRtypelens[valtype]
    ) {
        SUwarning (1, "AGGRreaggr", "incompatible files");
        return -1;
    }
    merge.maps = NULL;
    if (!(merge.maps = vmalloc (Vmheap, iafp->ad.itemn * sizeof (int))))
        goto abortAGGRreaggr;
    for (itemi = 0; itemi < iafp->ad.itemn; itemi++)
        merge.maps[itemi] = -1;
    switch (iafp->ad.keylen) {
    case -1:
        for (reaggri = 0; reaggri < reaggrn; reaggri++)
            if (_aggrdictlookupvar (
                iafp, reaggrs[reaggri].okeyp, -1, FALSE
            ) == -1)
                reaggrs[reaggri].okeyp = NULL;
        break;
    default:
        for (reaggri = 0; reaggri < reaggrn; reaggri++)
            if (_aggrdictlookupfixed (
                iafp, reaggrs[reaggri].okeyp, -1, FALSE
            ) == -1)
                reaggrs[reaggri].okeyp = NULL;
        break;
    }
    switch (oafp->ad.keylen) {
    case -1:
        for (reaggri = 0; reaggri < reaggrn; reaggri++) {
            if (!reaggrs[reaggri].okeyp)
                continue;
            reaggrs[reaggri].nitemi = _aggrdictlookupvar (
                oafp, reaggrs[reaggri].nkeyp, reaggrs[reaggri].nitemi, TRUE
            );
        }
        break;
    default:
        for (reaggri = 0; reaggri < reaggrn; reaggri++) {
            if (!reaggrs[reaggri].okeyp)
                continue;
            reaggrs[reaggri].nitemi = _aggrdictlookupfixed (
                oafp, reaggrs[reaggri].nkeyp, reaggrs[reaggri].nitemi, TRUE
            );
        }
        break;
    }
    switch (iafp->ad.keylen) {
    case -1:
        for (reaggri = 0; reaggri < reaggrn; reaggri++) {
            if (!reaggrs[reaggri].okeyp)
                continue;
            itemi = _aggrdictlookupvar (
                iafp, reaggrs[reaggri].okeyp, -1, FALSE
            );
            if (itemi != -1 && reaggrs[reaggri].nitemi != -1)
                merge.maps[itemi] = reaggrs[reaggri].nitemi;
        }
        break;
    default:
        for (reaggri = 0; reaggri < reaggrn; reaggri++) {
            if (!reaggrs[reaggri].okeyp)
                continue;
            itemi = _aggrdictlookupfixed (
                iafp, reaggrs[reaggri].okeyp, -1, FALSE
            );
            if (itemi != -1 && reaggrs[reaggri].nitemi != -1)
                merge.maps[itemi] = reaggrs[reaggri].nitemi;
        }
        break;
    }
    if (_aggrgrow (oafp, oafp->ad.itemn, framen) == -1)
        goto abortAGGRreaggr;
    rtn = _aggrreaggrrun (
        iafp, &merge, oafp, iafp->hdr.valtype, oafp->hdr.valtype
    );
    vmfree (Vmheap, merge.maps);
    return rtn;

abortAGGRreaggr:
    SUwarning (1, "AGGRreaggr", "reaggr failed");
    SUwarning (1, "AGGRreaggr", "merge.maps = %x", merge.maps);
    if (merge.maps)
        vmfree (Vmheap, merge.maps);
    return -1;
}
