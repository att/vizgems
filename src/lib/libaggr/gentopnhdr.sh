
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
for itype in CHAR SHORT INT LLONG FLOAT DOUBLE; do
    echo "#undef IN"
    echo "#define IN ${nams[$itype]}"
    echo "#undef IV"
    echo "#define IV ${vals[$itype]}"
    for oper in MIN MAX; do
        echo "#undef OPER"
        echo "#define OPER TOPN_OPER_$oper"
        echo "#undef NAME"
        echo "#define NAME topn_${nams[$itype]}_$oper"
        echo "#include \"libtopn.tmpl.h\""
    done
done
echo "#undef IN"
echo "#undef IV"
echo "#undef OPER"
echo "#undef NAME"
echo "static topnfunc_t topnfuncs[TYPE_MAXID][TOPN_OPER_MAXID];"
echo "void _aggrtopninit (void) {"
for itype in CHAR SHORT INT LLONG FLOAT DOUBLE; do
    for oper in MIN MAX; do
        s1="[TYPE_$itype][TOPN_OPER_$oper]"
        s2="topn_${nams[$itype]}_$oper"
        echo "    topnfuncs$s1 = $s2;"
    done
done
echo "}"
echo "int _aggrtopnrun ("
echo "    AGGRfile_t *iafp, int fframei, int lframei, AGGRkv_t *kvs, int kvn,"
echo "    int itype, int oper"
echo ") {"
echo "    return (*topnfuncs[itype][oper]) ("
echo "        iafp, fframei, lframei, kvs, kvn"
echo "    );"
echo "}"
