
if [[ $SWIFTWARNLEVEL != '' ]] then
    typeset -ft nodenorm mapnorm edgenorm
    set -x
fi

if [[ $CASEINSENSITIVE == y ]] then
    typeset -l l
fi

mode=$1
shift

case $mode in
one)
    if [[ $ALWAYSCOMPRESSDP == y ]] then
        zflag='-ozm|rtb'
    fi
    in=$1
    outn=$2
    oute=$3
    outm=$4

    IFS='|'
    egrep '^node\|' $in | sed 's/^node.//' \
    | if [[ $CASEINSENSITIVE == y ]] then
        while read -r -u3 line; do
            r=${line#*'|'*'|'}; l=${line%"$r"}
            print -u4 -r "$l$r"
        done 3<&0- 4>&1-
    else
        cat
    fi | ddsload $zflag -os vg_inv_node.schema -le '|' -type ascii -dec simple \
        -lnts -lso vg_inv_node.load.so \
    > $outn.tmp && sdpmv $outn.tmp $outn

    egrep '^edge\|' $in | sed 's/^edge.//' \
    | if [[ $CASEINSENSITIVE == y ]] then
        while read -r -u3 line; do
            r=${line#*'|'*'|'*'|'*'|'}; l=${line%"$r"}
            print -u4 -r "$l$r"
        done 3<&0- 4>&1-
    else
        cat
    fi | ddsload $zflag -os vg_inv_edge.schema -le '|' -type ascii -dec simple \
        -lnts -lso vg_inv_edge.load.so \
    > $oute.tmp && sdpmv $oute.tmp $oute

    egrep '^map\|' $in | sed 's/^map.//' \
    | if [[ $CASEINSENSITIVE == y ]] then
        while read -r -u3 line; do
            r=${line#*'|'*'|'*'|'*'|'}; l=${line%"$r"}
            print -u4 -r "$l$r"
        done 3<&0- 4>&1-
    else
        cat
    fi | ddsload $zflag -os vg_inv_map.schema -le '|' -type ascii -dec simple \
        -lnts -lso vg_inv_map.load.so \
    > $outm.tmp && sdpmv $outm.tmp $outm
    ;;
all)
    if [[ $ALWAYSCOMPRESSDP == y ]] then
        zflag='-ozm rtb'
    fi
    leveln=$1
    levelm=$2
    shift 2
    suffixn=$1
    suffixe=$2
    suffixm=$3
    shift 3
    outn=$1
    outnu=$2
    oute=$3
    outeu=$4
    outm=$5
    outmu=$6
    outmuf=${outmu%.dds}-fwd.dds
    outmur=${outmu%.dds}-rev.dds
    outcf=$7
    outcr=$8

    export LEVELNODEFILE=$leveln
    export LEVELMAPFILE=$levelm
    export INVMAPFWDFILE=$outmuf
    export INVMAPREVFILE=$outmur

    set -x
    ddscat -fp "*-$suffixn.dds" \
    | ddssort $zflag -fast -u -ke 'level id key' \
    > $outn.tmp && sdpmv $outn.tmp $outn
    ddsconv $zflag -os vg_inv_node.schema -cso vg_inv_uniquenode.conv.so $outn \
    > $outnu.tmp && sdpmv $outnu.tmp $outnu
    touch inv.uniquebyname

    ddscat -fp "*-$suffixe.dds" \
    | ddssort $zflag -fast -u -ke 'level1 id1 level2 id2 key' \
    > $oute.tmp && sdpmv $oute.tmp $oute
    ddssort $zflag -fast -u -ke 'level1 id1 level2 id2' $oute \
    > $outeu.tmp && sdpmv $outeu.tmp $outeu

    ddscat -fp "*-$suffixm.dds" \
    | ddssort $zflag -fast -u -ke 'level1 id1 level2 id2 key' \
    > $outm.tmp && sdpmv $outm.tmp $outm
    ddssort $zflag -fast -u -ke 'level1 id1 level2 id2' $outm \
    > $outmuf.tmp && sdpmv $outmuf.tmp $outmuf
    ddsconv -os vg_inv_map.schema -cso vg_inv_map_rev.conv.so $outmuf \
    | ddssort $zflag -fast -u -ke 'level1 id1 level2 id2 key' \
    > $outmur.tmp && sdpmv $outmur.tmp $outmur
    ddsconv -os vg_inv_map.schema -cso vg_inv_map.conv.so $outnu \
    | ddssort $zflag -fast -u -ke 'level1 id1 level2 id2 key' \
    > $outmu.tmp && sdpmv $outmu.tmp $outmu

    ddsconv -os vg_inv_cc.schema -cso vg_inv_nd2cc.conv.so $outeu \
    | ddssort $zflag -fast -u -ke 'ndlevel ndid' \
    > $outcf.tmp && sdpmv $outcf.tmp $outcf
    ddssort $zflag -fast -ke 'cclevel ccid' $outcf \
    > $outcr.tmp && sdpmv $outcr.tmp $outcr
    set +x
    ;;
esac
