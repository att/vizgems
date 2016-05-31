static int NAME (
    AGGRfile_t **iafps, int iafpn, merge_t *merges, AGGRfile_t *oafp
) {
    int framei, iframei, itemi, mitemi;
    void *datap;
    AGGRfile_t *iafp;
    int iafpi;
    merge_t *mergep;
    unsigned char *icp, *ocp;
    int icl, ocl;
    long long ivl, ovl;
#ifdef ROUND
    float dv1 = 0.5, dv2 = 0.0;
#endif

    ocl = AGGRtypelens[oafp->hdr.valtype];
    for (framei = 0, iafpi = 0; iafpi < iafpn; iafpi++) {
        iafp = iafps[iafpi];
        icl = AGGRtypelens[iafp->hdr.valtype];
        mergep = &merges[iafpi];
        for (iframei = 0; iframei < iafp->hdr.framen; iframei++) {
            if (!(mergep->datap = AGGRget (iafp, iframei))) {
                SUwarning (1, "AGGRcat", "get failed");
                return -1;
            }
            icp = mergep->datap;
            if (!(datap = AGGRget (oafp, framei))) {
                SUwarning (1, "AGGRcat", "get failed");
                return -1;
            }
            ocp = datap;
            for (ovl = ivl = 0; ovl < OLEN; ovl += ocl, ivl += icl) {
                for (itemi = 0; itemi < iafp->ad.itemn; itemi++) {
                    if ((mitemi = mergep->maps[itemi]) != -1) {
#ifdef ROUND
                        dv2 = (dv1 >= 1.0 ? 0.9999 : dv1);
                        OV (&ocp[OL]) = IV (&icp[IL]) + dv2;
                        dv1 += (IV (&icp[IL]) - OV (&ocp[OL]));
#else
                        OV (&ocp[OL]) = IV (&icp[IL]);
#endif
                    }
                }
            }
            if (AGGRrelease (oafp) == -1) {
                SUwarning (1, "AGGRcat", "release failed");
                return -1;
            }
            if (AGGRrelease (iafp) == -1) {
                SUwarning (1, "AGGRcat", "release failed");
                return -1;
            }
            framei++;
        }
    }
    return 0;
}
