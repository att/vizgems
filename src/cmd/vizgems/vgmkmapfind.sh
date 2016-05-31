
id=${name#inv_map}
l1i=${inds[level1]}
i1i=${inds[id1]}
l2i=${inds[level2]}
i2i=${inds[id2]}

print -r "
static char sl_${name}_us[${sizes[$i1i]}], *sl_${name}_up;
static int sl_${name}_flag;
static sl_${name}_t *sl_${name}_rp;

#define M${id}I(fl) { \\
    memset (sl_${name}_us, 0, ${sizes[$i1i]}); \\
    strcpy (sl_${name}_us, \"UNKNOWN\"); \\
    sl_${name}_up = (fl) ? sl_${name}_us : NULL; \\
}
#define M${id}F(l1, i1, l2) ( \\
    (sl_${name}_flag = (strcmp (l1, l2) != 0)) ? ( \\
        (sl_${name}_rp = sl_${name}findfirst (l1, i1, l2)) ? \\
        sl_${name}_rp->sl_id2 : \\
        ((sl_level_mapfind (l1, l2)) ? sl_${name}_up : NULL) \\
    ) : i1 \\
)
#define M${id}N(l1, i1, l2) ( \\
    (sl_${name}_flag) ? ( \\
        (sl_${name}_rp = sl_${name}findnext (l1, i1, l2)) ? \\
        sl_${name}_rp->sl_id2 : NULL \\
    ) : NULL \\
)
"
