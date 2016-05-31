
typedef struct tcmap_t {
    char level[SZ_level1];
    char id[SZ_id1];
} tcmap_t;

static tcmap_t *tcmaps[2];
static int tcmapn[2], tcmapi[2];

void vg_inv_map_tcgrow (int index, int mapj) {
    int mapm;

    mapm = mapj + 1;
    if (mapm > tcmapn[index]) {
        tcmapn[index] = 2 * ((mapm < 32) ? 32 : mapm);
        if (!(tcmaps[index] = vmresize (
            Vmheap, tcmaps[index], sizeof (tcmap_t) * tcmapn[index], VM_RSCOPY
        )))
            SUerror ("vg_inv_map_tc", "cannot grow tcmaps[%d]", index);
    }
}

void vg_inv_map_tcinit (void) {
    tcmapn[0] = tcmapn[1] = 0;
    tcmaps[0] = tcmaps[1] = NULL;
    vg_inv_map_tcgrow (0, 32);
    vg_inv_map_tcgrow (1, 32);
}

void vg_inv_map_tcfind (char *flevel, char *fid, char *tlevel) {
    sl_level_map_t *lmp;
    sl_inv_map1_t *imfp;
    sl_inv_map2_t *imrp;
    tcmap_t *mapp;
    int mapm, mapj;

    tcmapi[0] = tcmapi[1] = 0;
    if (strcmp (flevel, tlevel) == 0) {
        memcpy (tcmaps[0][tcmapi[0]].level, flevel, SZ_level1);
        memcpy (tcmaps[0][tcmapi[0]++].id, fid, SZ_id1);
        return;
    }

    memcpy (tcmaps[0][tcmapi[0]].level, flevel, SZ_level1);
    memcpy (tcmaps[0][tcmapi[0]++].id, fid, SZ_id1);
    for (
        lmp = sl_level_mapfindfirst (flevel, tlevel); lmp;
        lmp = sl_level_mapfindnext (flevel, tlevel)
    ) {
        for (mapj = 0; mapj < tcmapi[0]; mapj++) {
            switch (lmp->sl_dir) {
            case VG_INV_N_LEVELMAP_DIR_FWD:
                for (imfp = sl_inv_map1findfirst (
                    lmp->sl_iid1, tcmaps[0][mapj].id, lmp->sl_iid2
                ); imfp; imfp = sl_inv_map1findnext (
                    lmp->sl_iid1, tcmaps[0][mapj].id, lmp->sl_iid2
                )) {
                    vg_inv_map_tcgrow (1, tcmapi[1]);
                    memcpy (
                        tcmaps[1][tcmapi[1]].level, imfp->sl_level2, SZ_level1
                    );
                    memcpy (tcmaps[1][tcmapi[1]++].id, imfp->sl_id2, SZ_id1);
                }
                break;
            case VG_INV_N_LEVELMAP_DIR_REV:
                for (imrp = sl_inv_map2findfirst (
                    lmp->sl_iid1, tcmaps[0][mapj].id, lmp->sl_iid2
                ); imrp; imrp = sl_inv_map2findnext (
                    lmp->sl_iid1, tcmaps[0][mapj].id, lmp->sl_iid2
                )) {
                    vg_inv_map_tcgrow (1, tcmapi[1]);
                    memcpy (
                        tcmaps[1][tcmapi[1]].level, imrp->sl_level2, SZ_level1
                    );
                    memcpy (tcmaps[1][tcmapi[1]++].id, imrp->sl_id2, SZ_id1);
                }
                break;
            }
        }
        mapp = tcmaps[0], tcmaps[0] = tcmaps[1], tcmaps[1] = mapp;
        mapm = tcmapn[0], tcmapn[0] = tcmapn[1], tcmapn[1] = mapm;
        mapj = tcmapi[0], tcmapi[0] = tcmapi[1], tcmapi[1] = mapj;
        tcmapi[1] = 0;
    }
}

tcmap_t *vg_inv_map_tccopy (int index) {
    tcmap_t *mapp;

    if (!(mapp = vmalloc (Vmheap, sizeof (tcmap_t) * tcmapi[index])))
        SUerror ("vg_inv_map_tc", "cannot copy maps");
    memcpy (mapp, tcmaps[index], sizeof (tcmap_t) * tcmapi[index]);
    return mapp;
}

void vg_inv_map_tcfree (tcmap_t *mapp) {
    vmfree (Vmheap, mapp);
}
