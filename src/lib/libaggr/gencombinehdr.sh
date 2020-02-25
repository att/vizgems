
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
    echo "#undef IN"
    echo "#define IN ${nams[$itype]}"
    echo "#undef IV"
    echo "#define IV ${vals[$itype]}"
    for otype in CHAR SHORT INT LLONG FLOAT DOUBLE; do
        echo "#undef OV"
        echo "#define OV ${vals[$otype]}"
        for oper in ADD SUB MUL DIV MIN MAX WAVG; do
            echo "#undef OPER"
            echo "#define OPER COMB_OPER_$oper"
            echo "#undef NAME"
            echo "#define NAME comb_${nams[$itype]}_${nams[$otype]}_$oper"
            echo "#include \"libcombine.tmpl.h\""
        done
    done
done
echo "#undef IN"
echo "#undef IV"
echo "#undef OV"
echo "#undef OPER"
echo "#undef NAME"
echo "static combfunc_t combfuncs[TYPE_MAXID][TYPE_MAXID][COMB_OPER_MAXID];"
echo "void _aggrcombineinit (void) {"
for itype in CHAR SHORT INT LLONG FLOAT DOUBLE; do
    for otype in CHAR SHORT INT LLONG FLOAT DOUBLE; do
        for oper in ADD SUB MUL DIV MIN MAX WAVG; do
            s1="[TYPE_$itype][TYPE_$otype][COMB_OPER_$oper]"
            s2="comb_${nams[$itype]}_${nams[$otype]}_$oper"
            echo "    combfuncs$s1 = $s2;"
        done
    done
done
echo "}"
echo "int _aggrcombinerun ("
echo "    AGGRfile_t **iafps, int iafpn,"
echo "    merge_t *merges, AGGRfile_t *oafp,"
echo "    int itype, int otype, int oper, float *ws"
echo ") {"
echo "    return (*combfuncs[itype][otype][oper]) ("
echo "        iafps, iafpn, merges, ws, oafp"
echo "    );"
echo "}"
