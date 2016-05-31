static int NAME (AGGRfile_t *iafp, int dir, AGGRfile_t *oafp) {
    int framei, itemi, mitemi;
    void *datap;
    unsigned char *icp, *ocp;
    int icl, ocl;
    long long ivl, ovl;

    ocl = AGGRtypelens[oafp->hdr.valtype];
    icl = AGGRtypelens[iafp->hdr.valtype];
    if (dir == SUM_DIR_FRAMES) {
        if (!(datap = AGGRget (oafp, 0))) {
            SUwarning (1, "AGGRsum", "get failed");
            return -1;
        }
        ocp = datap;
        for (framei = 0; framei < iafp->hdr.framen; framei++) {
            if (!(datap = AGGRget (iafp, framei))) {
                SUwarning (1, "AGGRsum", "get failed");
                return -1;
            }
            icp = datap;
            if (framei == 0) {
                for (itemi = 0; itemi < iafp->ad.itemn; itemi++) {
                    mitemi = itemi;
                    for (ovl = ivl = 0; ovl < OLEN; ovl += ocl, ivl += icl)
                        OV (&ocp[OL]) = IV (&icp[IL]);
                }
            } else {
                for (itemi = 0; itemi < iafp->ad.itemn; itemi++) {
                    mitemi = itemi;
                    for (ovl = ivl = 0; ovl < OLEN; ovl += ocl, ivl += icl)
#if OPER == SUM_OPER_ADD
                        OV (&ocp[OL]) += IV (&icp[IL]);
#endif
#if OPER == SUM_OPER_MIN
                        if (OV (&ocp[OL]) > IV (&icp[IL]))
                            OV (&ocp[OL]) = IV (&icp[IL]);
#endif
#if OPER == SUM_OPER_MAX
                        if (OV (&ocp[OL]) < IV (&icp[IL]))
                            OV (&ocp[OL]) = IV (&icp[IL]);
#endif
                }
            }
            if (AGGRrelease (iafp) == -1) {
                SUwarning (1, "AGGRsum", "release failed");
                return -1;
            }
        }
        if (AGGRrelease (oafp) == -1) {
            SUwarning (1, "AGGRsum", "release failed");
            return -1;
        }
    } else {
        for (framei = 0; framei < iafp->hdr.framen; framei++) {
            if (!(datap = AGGRget (oafp, framei))) {
                SUwarning (1, "AGGRsum", "get failed");
                return -1;
            }
            ocp = datap;
            if (!(datap = AGGRget (iafp, framei))) {
                SUwarning (1, "AGGRsum", "get failed");
                return -1;
            }
            icp = datap;
            mitemi = 0;
            if (iafp->ad.itemn > 0) {
                itemi = 0;
                for (ovl = ivl = 0; ovl < OLEN; ovl += ocl, ivl += icl)
                    OV (&ocp[OL]) = IV (&icp[IL]);
            }
            for (itemi = 1; itemi < iafp->ad.itemn; itemi++)
                for (ovl = ivl = 0; ovl < OLEN; ovl += ocl, ivl += icl)
#if OPER == SUM_OPER_ADD
                    OV (&ocp[OL]) += IV (&icp[IL]);
#endif
#if OPER == SUM_OPER_MIN
                    if (OV (&ocp[OL]) > IV (&icp[IL]))
                        OV (&ocp[OL]) = IV (&icp[IL]);
#endif
#if OPER == SUM_OPER_MAX
                    if (OV (&ocp[OL]) < IV (&icp[IL]))
                        OV (&ocp[OL]) = IV (&icp[IL]);
#endif
            if (AGGRrelease (iafp) == -1) {
                SUwarning (1, "AGGRsum", "release failed");
                return -1;
            }
            if (AGGRrelease (oafp) == -1) {
                SUwarning (1, "AGGRsum", "release failed");
                return -1;
            }
        }
    }
    return 0;
}
