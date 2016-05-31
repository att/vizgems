
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
for itype in CHAR SHORT INT LLONG FLOAT DOUBLE; do
    print "#undef IN"
    print "#define IN ${nams[$itype]}"
    print "#undef IV"
    print "#define IV ${vals[$itype]}"
    for oper in MIN MAX; do
        print "#undef OPER"
        print "#define OPER TOPN_OPER_$oper"
        print "#undef NAME"
        print "#define NAME topn_${nams[$itype]}_$oper"
        print "#include \"libtopn.tmpl.h\""
    done
done
print "#undef IN"
print "#undef IV"
print "#undef OPER"
print "#undef NAME"
print "static topnfunc_t topnfuncs[TYPE_MAXID][TOPN_OPER_MAXID];"
print "void _aggrtopninit (void) {"
for itype in CHAR SHORT INT LLONG FLOAT DOUBLE; do
    for oper in MIN MAX; do
        s1="[TYPE_$itype][TOPN_OPER_$oper]"
        s2="topn_${nams[$itype]}_$oper"
        print "    topnfuncs$s1 = $s2;"
    done
done
print "}"
print "int _aggrtopnrun ("
print "    AGGRfile_t *iafp, int fframei, int lframei, AGGRkv_t *kvs, int kvn,"
print "    int itype, int oper"
print ") {"
print "    return (*topnfuncs[itype][oper]) ("
print "        iafp, fframei, lframei, kvs, kvn"
print "    );"
print "}"
