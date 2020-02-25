
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
        for oper in ADD MIN MAX; do
            echo "#undef OPER"
            echo "#define OPER SUM_OPER_$oper"
            echo "#undef NAME"
            echo "#define NAME sum_${nams[$itype]}_${nams[$otype]}_$oper"
            echo "#include \"libsum.tmpl.h\""
        done
    done
done
echo "#undef IV"
echo "#undef OV"
echo "#undef OPER"
echo "#undef NAME"
echo "static sumfunc_t sumfuncs[TYPE_MAXID][TYPE_MAXID][SUM_OPER_MAXID];"
echo "void _aggrsuminit (void) {"
for itype in CHAR SHORT INT LLONG FLOAT DOUBLE; do
    for otype in CHAR SHORT INT LLONG FLOAT DOUBLE; do
        for oper in ADD MIN MAX; do
            s1="[TYPE_$itype][TYPE_$otype][SUM_OPER_$oper]"
            s2="sum_${nams[$itype]}_${nams[$otype]}_$oper"
            echo "    sumfuncs$s1 = $s2;"
        done
    done
done
echo "}"
echo "int _aggrsumrun ("
echo "    AGGRfile_t *iafp, int dir, AGGRfile_t *oafp,"
echo "    int itype, int otype, int oper"
echo ") {"
echo "    return (*sumfuncs[itype][otype][oper]) ("
echo "        iafp, dir, oafp"
echo "    );"
echo "}"
