static int NAME (
    AGGRfile_t *iafp, int fframei, int lframei, AGGRkv_t *kvs, int kvn
) {
    int framei, itemi, keep;
    void *datap;
    unsigned char *cp;
    int cl, vl;
    AGGRunit_t kvu, ivu;
    int kvi, kvl;

    for (kvi = 0; kvi < kvn; kvi++)
        kvs[kvi].framei = -1, kvs[kvi].itemi = -1;
    kvl = 0;
    cl = AGGRtypelens[iafp->hdr.valtype];
    for (framei = fframei; framei <= lframei; framei++) {
        if (!(datap = AGGRget (iafp, framei))) {
            SUwarning (1, "AGGRtopn", "get failed");
            return -1;
        }
        cp = datap;
        for (itemi = 0; itemi < iafp->ad.itemn; itemi++) {
            keep = 0;
            if (kvl < kvn)
                keep = 1;
            else {
                for (vl = 0; vl < iafp->hdr.vallen; vl += cl) {
                    kvu.u.IN = IV (&kvs[kvl - 1].valp[vl]);
                    ivu.u.IN = IV (&cp[vl]);
#if OPER == TOPN_OPER_MAX
                    if (kvu.u.IN < ivu.u.IN) {
                        keep = 1;
                        break;
                    } else if (kvu.u.IN > ivu.u.IN)
                        break;
#endif
#if OPER == TOPN_OPER_MIN
                    if (kvu.u.IN > ivu.u.IN) {
                        keep = 1;
                        break;
                    } else if (kvu.u.IN < ivu.u.IN)
                        break;
#endif
                }
            }
            if (keep == 0)
                goto done2;
            for (kvi = kvl - 1; kvi >= 0; kvi--) {
                for (vl = 0; vl < iafp->hdr.vallen; vl += cl) {
                    kvu.u.IN = IV (&kvs[kvi].valp[vl]);
                    ivu.u.IN = IV (&cp[vl]);
#if OPER == TOPN_OPER_MAX
                    if (kvu.u.IN > ivu.u.IN)
                        goto done1;
                    else if (kvu.u.IN < ivu.u.IN)
                        break;
#endif
#if OPER == TOPN_OPER_MIN
                    if (kvu.u.IN < ivu.u.IN)
                        goto done1;
                    else if (kvu.u.IN > ivu.u.IN)
                        break;
#endif
                }
                if (kvi < kvn - 1) {
                    kvs[kvi + 1].framei = kvs[kvi].framei;
                    kvs[kvi + 1].itemi = kvs[kvi].itemi;
                    memcpy (kvs[kvi + 1].valp, kvs[kvi].valp, iafp->hdr.vallen);
                }
            }
done1:
            kvi++;
            kvs[kvi].framei = framei;
            kvs[kvi].itemi = itemi;
            memcpy (kvs[kvi].valp, cp, iafp->hdr.vallen);
            if (kvl < kvn)
                kvl++;
done2:
            cp += iafp->hdr.vallen;
        }
        if (AGGRrelease (iafp) == -1) {
            SUwarning (1, "AGGRtopn", "release failed");
            return -1;
        }
    }

    return 0;
}
