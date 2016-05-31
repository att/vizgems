
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
print "#include <math.h>"
print "#define MLEN (mafp->hdr.vallen)"
print "#define DLEN (mafp->hdr.vallen)"
print "#define CLEN (mafp->hdr.vallen)"
print "#define ILEN (iafp->hdr.vallen)"
print "#define ML (mitemi * MLEN + mvl)"
print "#define DL (mitemi * DLEN + dvl)"
print "#define CL (mitemi * CLEN + cvl)"
print "#define IL (itemi * ILEN + ivl)"
for itype in CHAR SHORT INT LLONG FLOAT DOUBLE; do
    print "#undef IV"
    print "#define IV ${vals[$itype]}"
    for otype in CHAR SHORT INT LLONG FLOAT DOUBLE; do
        print "#undef MV"
        print "#define MV ${vals[$otype]}"
        print "#undef DV"
        print "#define DV ${vals[$otype]}"
        print "#undef CV"
        print "#define CV ${vals[$otype]}"
        print "#undef NAME"
        print "#define NAME mean_${nams[$itype]}_${nams[$otype]}"
        print "#include \"libmean.tmpl.h\""
    done
done
print "#undef IV"
print "#undef MV"
print "#undef DV"
print "#undef CV"
print "#undef NAME"
print "static meanfunc_t meanfuncs[TYPE_MAXID][TYPE_MAXID];"
print "void _aggrmeaninit (void) {"
for itype in CHAR SHORT INT LLONG FLOAT DOUBLE; do
    for otype in CHAR SHORT INT LLONG FLOAT DOUBLE; do
        s1="[TYPE_$itype][TYPE_$otype]"
        s2="mean_${nams[$itype]}_${nams[$otype]}"
        print "    meanfuncs$s1 = $s2;"
    done
done
print "}"
print "int _aggrmeanrun ("
print "    AGGRfile_t **iafps, int iafpn,"
print "    merge_t *merges, AGGRfile_t *mafp, AGGRfile_t *dafp,"
print "    int itype, int otype"
print ") {"
print "    return (*meanfuncs[itype][otype]) ("
print "        iafps, iafpn, merges, mafp, dafp"
print "    );"
print "}"
