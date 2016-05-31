static int NAME (
    AGGRfile_t **iafps, int iafpn, merge_t *merges, float *ws, AGGRfile_t *oafp
) {
    int framei, itemi, mitemi;
    void *datap;
    AGGRfile_t *iafp;
    int iafpi;
    merge_t *mergep;
    unsigned char *icp, *ocp;
    int icl, ocl;
    long long ivl, ovl;
#if OPER == COMB_OPER_MUL || OPER == COMB_OPER_DIV
    char *cnts;
#endif

#if OPER == COMB_OPER_MUL || OPER == COMB_OPER_DIV
    if (!(cnts = vmresize (Vmheap, NULL, oafp->ad.itemn, VM_RSZERO))) {
        SUwarning (1, "AGGRcombine", "alloc failed");
        return -1;
    }
#endif
    ocl = AGGRtypelens[oafp->hdr.valtype];
    for (framei = 0; framei < oafp->hdr.framen; framei++) {
        if (!(datap = AGGRget (oafp, framei))) {
            SUwarning (1, "AGGRcombine", "get failed");
            return -1;
        }
        ocp = datap;
#if OPER == COMB_OPER_MIN || OPER == COMB_OPER_MAX
        memset (ocp, 0, oafp->hdr.vallen * oafp->hdr.itemn);
#endif
        iafp = iafps[0];
        icl = AGGRtypelens[iafp->hdr.valtype];
        mergep = &merges[0];
        if (!(mergep->datap = AGGRget (iafp, framei))) {
            SUwarning (1, "AGGRcombine", "get failed");
            return -1;
        }
        icp = mergep->datap;
        for (itemi = 0; itemi < iafp->ad.itemn; itemi++) {
            if ((mitemi = mergep->maps[itemi]) != -1)
                for (ovl = ivl = 0; ovl < OLEN; ovl += ocl, ivl += icl)
#if OPER == COMB_OPER_WAVG
                    OV (&ocp[OL]) = (ws[0] * IV (&icp[IL]));
#else
                    OV (&ocp[OL]) = IV (&icp[IL]);
#endif
        }
        if (AGGRrelease (iafp) == -1) {
            SUwarning (1, "AGGRcombine", "release failed");
            return -1;
        }
        for (iafpi = 1; iafpi < iafpn; iafpi++) {
            iafp = iafps[iafpi];
            icl = AGGRtypelens[iafp->hdr.valtype];
            mergep = &merges[iafpi];
            if (!(mergep->datap = AGGRget (iafp, framei))) {
                SUwarning (1, "AGGRcombine", "get failed");
                return -1;
            }
            icp = mergep->datap;
            for (itemi = 0; itemi < iafp->ad.itemn; itemi++) {
                if ((mitemi = mergep->maps[itemi]) == -1)
                    continue;
                for (ovl = ivl = 0; ovl < OLEN; ovl += ocl, ivl += icl)
#if OPER == COMB_OPER_ADD
                    OV (&ocp[OL]) += IV (&icp[IL]);
#endif
#if OPER == COMB_OPER_SUB
                    OV (&ocp[OL]) -= IV (&icp[IL]);
#endif
#if OPER == COMB_OPER_MUL
                    OV (&ocp[OL]) *= IV (&icp[IL]);
                cnts[mitemi]++;
#endif
#if OPER == COMB_OPER_DIV
                    if (IV (&icp[IL]) != 0)
                        OV (&ocp[OL]) /= IV (&icp[IL]);
                    else
                        OV (&ocp[OL]) = 0;
                cnts[mitemi]++;
#endif
#if OPER == COMB_OPER_MIN
                    if (OV (&ocp[OL]) > IV (&icp[IL]))
                        OV (&ocp[OL]) = IV (&icp[IL]);
#endif
#if OPER == COMB_OPER_MAX
                    if (OV (&ocp[OL]) < IV (&icp[IL]))
                        OV (&ocp[OL]) = IV (&icp[IL]);
#endif
#if OPER == COMB_OPER_WAVG
                    OV (&ocp[OL]) += (ws[iafpi] * IV (&icp[IL]));
#endif
            }
            if (AGGRrelease (iafp) == -1) {
                SUwarning (1, "AGGRcombine", "release failed");
                return -1;
            }
        }
#if OPER == COMB_OPER_MUL || OPER == COMB_OPER_DIV
        for (mitemi = 0; mitemi < oafp->ad.itemn; mitemi++) {
            if (cnts[mitemi] != iafpn - 1)
                for (ovl = 0; ovl < OLEN; ovl += ocl)
                    OV (&ocp[OL]) = 0;
            cnts[mitemi] = 0;
        }
#endif
        if (AGGRrelease (oafp) == -1) {
            SUwarning (1, "AGGRcombine", "release failed");
            return -1;
        }
    }
#if OPER == COMB_OPER_MUL || OPER == COMB_OPER_DIV
    vmfree (Vmheap, cnts);
#endif
    return 0;
}
