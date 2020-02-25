
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
echo "#define OLEN (oafp->hdr.vallen)"
echo "#define ILEN (iafp->hdr.vallen)"
echo "#define OL (mitemi * OLEN + ovl)"
echo "#define IL (itemi * ILEN + ivl)"
for itype in CHAR SHORT INT LLONG FLOAT DOUBLE; do
    echo "#undef IV"
    echo "#define IV ${vals[$itype]}"
    for otype in CHAR SHORT INT LLONG FLOAT DOUBLE; do
        echo "#undef OV"
        echo "#define OV ${vals[$otype]}"
        echo "#undef NAME"
        echo "#define NAME reaggr_${nams[$itype]}_${nams[$otype]}"
        echo "#include \"libreaggr.tmpl.h\""
    done
done
echo "#undef IV"
echo "#undef OV"
echo "#undef NAME"
echo "static reaggrfunc_t reaggrfuncs[TYPE_MAXID][TYPE_MAXID];"
echo "void _aggrreaggrinit (void) {"
for itype in CHAR SHORT INT LLONG FLOAT DOUBLE; do
    for otype in CHAR SHORT INT LLONG FLOAT DOUBLE; do
        s1="[TYPE_$itype][TYPE_$otype]"
        s2="reaggr_${nams[$itype]}_${nams[$otype]}"
        echo "    reaggrfuncs$s1 = $s2;"
    done
done
echo "}"
echo "int _aggrreaggrrun ("
echo "    AGGRfile_t *iafp, merge_t *merges, AGGRfile_t *oafp,"
echo "    int itype, int otype"
echo ") {"
echo "    return (*reaggrfuncs[itype][otype]) ("
echo "        iafp, merges, oafp"
echo "    );"
echo "}"
