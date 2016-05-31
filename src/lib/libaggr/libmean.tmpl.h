#ifndef SQ
#define SQ(a) ((a) * (a))
#endif

static int NAME (
    AGGRfile_t **iafps, int iafpn,
    merge_t *merges, AGGRfile_t *mafp, AGGRfile_t *dafp
) {
    int framei, itemi, mitemi;
    void *datap;
    AGGRfile_t *iafp;
    int iafpi;
    merge_t *mergep;
    unsigned char *icp, *ccp, *mcp, *dcp;
    int icl, ccl, mcl, dcl;
    long long ivl, cvl, mvl, dvl;

    ccl = dcl = mcl = AGGRtypelens[mafp->hdr.valtype];
    for (framei = 0; framei < mafp->hdr.framen; framei++) {
        if (!(datap = AGGRget (mafp, framei))) {
            SUwarning (1, "AGGRmean", "get failed");
            return -1;
        }
        mcp = datap;
        if (!(datap = AGGRget (dafp, framei))) {
            SUwarning (1, "AGGRmean", "get failed");
            return -1;
        }
        dcp = datap;
        for (iafpi = 0; iafpi < iafpn; iafpi++) {
            iafp = iafps[iafpi];
            icl = AGGRtypelens[iafp->hdr.valtype];
            mergep = &merges[iafpi];
            if (!(mergep->datap = AGGRget (iafp, framei))) {
                SUwarning (1, "AGGRmean", "get failed");
                return -1;
            }
            icp = mergep->datap;
            ccp = mergep->cachep;
            for (itemi = 0; itemi < iafp->ad.itemn; itemi++)
                if ((mitemi = mergep->maps[itemi]) != -1)
                    for (cvl = ivl = 0; cvl < CLEN; cvl += ccl, ivl += icl)
                        CV (&ccp[CL]) = IV (&icp[IL]);
            if (AGGRrelease (iafp) == -1) {
                SUwarning (1, "AGGRmean", "release failed");
                return -1;
            }
        }
        for (mitemi = 0; mitemi < mafp->ad.itemn; mitemi++) {
            mergep = &merges[0];
            ccp = mergep->cachep;
            for (mvl = cvl = 0; mvl < MLEN; mvl += mcl, cvl += ccl)
                MV (&mcp[ML]) = CV (&ccp[CL]);
            for (iafpi = 1; iafpi < iafpn; iafpi++) {
                mergep = &merges[iafpi];
                ccp = mergep->cachep;
                for (mvl = cvl = 0; mvl < MLEN; mvl += mcl, cvl += ccl)
                    MV (&mcp[ML]) += CV (&ccp[CL]);
            }
            for (mvl = cvl = 0; mvl < MLEN; mvl += mcl, cvl += ccl)
                MV (&mcp[ML]) /= iafpn;
            mergep = &merges[0];
            ccp = mergep->cachep;
            for (
                dvl = mvl = cvl = 0; dvl < DLEN;
                dvl += dcl, mvl += mcl, cvl += ccl
            )
                DV (&dcp[DL]) = SQ (CV (&ccp[CL]) - MV (&mcp[ML]));
            for (iafpi = 1; iafpi < iafpn; iafpi++) {
                mergep = &merges[iafpi];
                ccp = mergep->cachep;
                for (
                    dvl = mvl = cvl = 0; dvl < DLEN;
                    dvl += dcl, mvl += mcl, cvl += ccl
                )
                    DV (&dcp[DL]) += SQ (CV (&ccp[CL]) - MV (&mcp[ML]));
            }
            for (dvl = 0; dvl < DLEN; dvl += dcl)
                DV (&dcp[DL]) = sqrt (DV (&dcp[DL]) / (iafpn - 1));
        }
        if (AGGRrelease (dafp) == -1) {
            SUwarning (1, "AGGRmean", "release failed");
            return -1;
        }
        if (AGGRrelease (mafp) == -1) {
            SUwarning (1, "AGGRmean", "release failed");
            return -1;
        }
    }
    return 0;
}
