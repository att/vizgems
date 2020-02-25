
typeset -A vals nams

vals[CHAR]=CVAL
vals[SHORT]=SVAL
vals[INT]=IVAL
vals[LLONG]=LVAL
vals[FLOAT]=FVAL
vals[DOUBLE]=DVAL

nams[CHAR]=c
nams[SHORT]=s
nams[INT]=i
nams[LLONG]=l
nams[FLOAT]=f
nams[DOUBLE]=d

echo "#pragma prototyped"
echo "#define _AGGR_PRIVATE"
echo "#include \"aggr.h\""
echo "#include \"aggr_int.h\""
echo "#include <math.h>"
echo "#define MLEN (mafp->hdr.vallen)"
echo "#define DLEN (mafp->hdr.vallen)"
echo "#define CLEN (mafp->hdr.vallen)"
echo "#define ILEN (iafp->hdr.vallen)"
echo "#define ML (mitemi * MLEN + mvl)"
echo "#define DL (mitemi * DLEN + dvl)"
echo "#define CL (mitemi * CLEN + cvl)"
echo "#define IL (itemi * ILEN + ivl)"
for itype in CHAR SHORT INT LLONG FLOAT DOUBLE; do
    echo "#undef IV"
    echo "#define IV ${vals[$itype]}"
    for otype in CHAR SHORT INT LLONG FLOAT DOUBLE; do
        echo "#undef MV"
        echo "#define MV ${vals[$otype]}"
        echo "#undef DV"
        echo "#define DV ${vals[$otype]}"
        echo "#undef CV"
        echo "#define CV ${vals[$otype]}"
        echo "#undef NAME"
        echo "#define NAME mean_${nams[$itype]}_${nams[$otype]}"
        echo "#include \"libmean.tmpl.h\""
    done
done
echo "#undef IV"
echo "#undef MV"
echo "#undef DV"
echo "#undef CV"
echo "#undef NAME"
echo "static meanfunc_t meanfuncs[TYPE_MAXID][TYPE_MAXID];"
echo "void _aggrmeaninit (void) {"
for itype in CHAR SHORT INT LLONG FLOAT DOUBLE; do
    for otype in CHAR SHORT INT LLONG FLOAT DOUBLE; do
        s1="[TYPE_$itype][TYPE_$otype]"
        s2="mean_${nams[$itype]}_${nams[$otype]}"
        echo "    meanfuncs$s1 = $s2;"
    done
done
echo "}"
echo "int _aggrmeanrun ("
echo "    AGGRfile_t **iafps, int iafpn,"
echo "    merge_t *merges, AGGRfile_t *mafp, AGGRfile_t *dafp,"
echo "    int itype, int otype"
echo ") {"
echo "    return (*meanfuncs[itype][otype]) ("
echo "        iafps, iafpn, merges, mafp, dafp"
echo "    );"
echo "}"
