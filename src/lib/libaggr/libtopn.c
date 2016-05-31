#pragma prototyped

#include <ast.h>
#include <swift.h>
#define _AGGR_PRIVATE
#include <aggr.h>
#include <aggr_int.h>

int AGGRtopn (
    AGGRfile_t *iafp, int fframei, int lframei,
    AGGRkv_t *kvs, int kvn, int oper
) {
    int rtn;

    if (
        !iafp || fframei < 0 || fframei > lframei ||
        lframei >= iafp->hdr.framen || !kvs || kvn < 1 ||
        oper < 0 || oper >= TOPN_OPER_MAXID
    ) {
        SUwarning (
            1, "AGGRtopn", "bad arguments: %x %d %d %x %d %d",
            iafp, fframei, lframei, kvs, kvn, oper
        );
        return -1;
    }
    rtn = _aggrtopnrun (
        iafp, fframei, lframei, kvs, kvn, iafp->hdr.valtype, oper
    );
    return rtn;
}
