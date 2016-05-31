static int NAME (AGGRfile_t *iafp, merge_t *mergep, AGGRfile_t *oafp) {
    int framei, itemi, mitemi;
    void *datap;
    unsigned char *icp, *ocp;
    int icl, ocl;
    long long ivl, ovl;

    ocl = AGGRtypelens[oafp->hdr.valtype];
    icl = AGGRtypelens[iafp->hdr.valtype];
    for (framei = 0; framei < iafp->hdr.framen; framei++) {
        if (!(datap = AGGRget (oafp, framei))) {
            SUwarning (1, "AGGRreaggr", "get failed");
            return -1;
        }
        ocp = datap;
        if (!(mergep->datap = AGGRget (iafp, framei))) {
            SUwarning (1, "AGGRreaggr", "get failed");
            return -1;
        }
        icp = mergep->datap;
        for (itemi = 0; itemi < iafp->ad.itemn; itemi++)
            if ((mitemi = mergep->maps[itemi]) != -1)
                for (ovl = ivl = 0; ovl < OLEN; ovl += ocl, ivl += icl)
                    OV (&ocp[OL]) += IV (&icp[IL]);
        if (AGGRrelease (iafp) == -1) {
            SUwarning (1, "AGGRreaggr", "release failed");
            return -1;
        }
        if (AGGRrelease (oafp) == -1) {
            SUwarning (1, "AGGRreaggr", "release failed");
            return -1;
        }
    }
    return 0;
}
