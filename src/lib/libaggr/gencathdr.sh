
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
        print "#undef ROUND"
        if [[ $itype == FLOAT || $itype == DOUBLE ]] then
            if [[ $otype != FLOAT && $otype != DOUBLE ]] then
                print "#define ROUND 1"
            fi
        fi
        print "#undef NAME"
        print "#define NAME cat_${nams[$itype]}_${nams[$otype]}"
        print "#include \"libcat.tmpl.h\""
    done
done
print "#undef IV"
print "#undef OV"
print "#undef OPER"
print "#undef NAME"
print "static catfunc_t catfuncs[TYPE_MAXID][TYPE_MAXID];"
print "void _aggrcatinit (void) {"
for itype in CHAR SHORT INT LLONG FLOAT DOUBLE; do
    for otype in CHAR SHORT INT LLONG FLOAT DOUBLE; do
        s1="[TYPE_$itype][TYPE_$otype]"
        s2="cat_${nams[$itype]}_${nams[$otype]}"
        print "    catfuncs$s1 = $s2;"
    done
done
print "}"
print "int _aggrcatrun ("
print "    AGGRfile_t **iafps, int iafpn,"
print "    merge_t *merges, AGGRfile_t *oafp,"
print "    int itype, int otype"
print ") {"
print "    return (*catfuncs[itype][otype]) ("
print "        iafps, iafpn, merges, oafp"
print "    );"
print "}"
