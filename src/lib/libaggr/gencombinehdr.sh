
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

print "#pragma prototyped"
print "#define _AGGR_PRIVATE"
print "#include \"aggr.h\""
print "#include \"aggr_int.h\""
print "#define OLEN (oafp->hdr.vallen)"
print "#define ILEN (iafp->hdr.vallen)"
print "#define OL (mitemi * OLEN + ovl)"
print "#define IL (itemi * ILEN + ivl)"
for itype in CHAR SHORT INT LLONG FLOAT DOUBLE; do
    print "#undef IN"
    print "#define IN ${nams[$itype]}"
    print "#undef IV"
    print "#define IV ${vals[$itype]}"
    for otype in CHAR SHORT INT LLONG FLOAT DOUBLE; do
        print "#undef OV"
        print "#define OV ${vals[$otype]}"
        for oper in ADD SUB MUL DIV MIN MAX WAVG; do
            print "#undef OPER"
            print "#define OPER COMB_OPER_$oper"
            print "#undef NAME"
            print "#define NAME comb_${nams[$itype]}_${nams[$otype]}_$oper"
            print "#include \"libcombine.tmpl.h\""
        done
    done
done
print "#undef IN"
print "#undef IV"
print "#undef OV"
print "#undef OPER"
print "#undef NAME"
print "static combfunc_t combfuncs[TYPE_MAXID][TYPE_MAXID][COMB_OPER_MAXID];"
print "void _aggrcombineinit (void) {"
for itype in CHAR SHORT INT LLONG FLOAT DOUBLE; do
    for otype in CHAR SHORT INT LLONG FLOAT DOUBLE; do
        for oper in ADD SUB MUL DIV MIN MAX WAVG; do
            s1="[TYPE_$itype][TYPE_$otype][COMB_OPER_$oper]"
            s2="comb_${nams[$itype]}_${nams[$otype]}_$oper"
            print "    combfuncs$s1 = $s2;"
        done
    done
done
print "}"
print "int _aggrcombinerun ("
print "    AGGRfile_t **iafps, int iafpn,"
print "    merge_t *merges, AGGRfile_t *oafp,"
print "    int itype, int otype, int oper, float *ws"
print ") {"
print "    return (*combfuncs[itype][otype][oper]) ("
print "        iafps, iafpn, merges, ws, oafp"
print "    );"
print "}"
