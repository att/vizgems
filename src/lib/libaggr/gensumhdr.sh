
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
    print "#undef IV"
    print "#define IV ${vals[$itype]}"
    for otype in CHAR SHORT INT LLONG FLOAT DOUBLE; do
        print "#undef OV"
        print "#define OV ${vals[$otype]}"
        for oper in ADD MIN MAX; do
            print "#undef OPER"
            print "#define OPER SUM_OPER_$oper"
            print "#undef NAME"
            print "#define NAME sum_${nams[$itype]}_${nams[$otype]}_$oper"
            print "#include \"libsum.tmpl.h\""
        done
    done
done
print "#undef IV"
print "#undef OV"
print "#undef OPER"
print "#undef NAME"
print "static sumfunc_t sumfuncs[TYPE_MAXID][TYPE_MAXID][SUM_OPER_MAXID];"
print "void _aggrsuminit (void) {"
for itype in CHAR SHORT INT LLONG FLOAT DOUBLE; do
    for otype in CHAR SHORT INT LLONG FLOAT DOUBLE; do
        for oper in ADD MIN MAX; do
            s1="[TYPE_$itype][TYPE_$otype][SUM_OPER_$oper]"
            s2="sum_${nams[$itype]}_${nams[$otype]}_$oper"
            print "    sumfuncs$s1 = $s2;"
        done
    done
done
print "}"
print "int _aggrsumrun ("
print "    AGGRfile_t *iafp, int dir, AGGRfile_t *oafp,"
print "    int itype, int otype, int oper"
print ") {"
print "    return (*sumfuncs[itype][otype][oper]) ("
print "        iafp, dir, oafp"
print "    );"
print "}"
