
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
        print "#undef NAME"
        print "#define NAME extract_${nams[$itype]}_${nams[$otype]}"
        print "#include \"libextract.tmpl.h\""
    done
done
print "#undef IV"
print "#undef OV"
print "#undef NAME"
print "static extractfunc_t extractfuncs[TYPE_MAXID][TYPE_MAXID];"
print "void _aggrextractinit (void) {"
for itype in CHAR SHORT INT LLONG FLOAT DOUBLE; do
    for otype in CHAR SHORT INT LLONG FLOAT DOUBLE; do
        s1="[TYPE_$itype][TYPE_$otype]"
        s2="extract_${nams[$itype]}_${nams[$otype]}"
        print "    extractfuncs$s1 = $s2;"
    done
done
print "}"
print "int _aggrextractrun ("
print "    AGGRfile_t *iafp, merge_t *merges, AGGRfile_t *oafp,"
print "    int itype, int otype"
print ") {"
print "    return (*extractfuncs[itype][otype]) ("
print "        iafp, merges, oafp"
print "    );"
print "}"
