#!/bin/ksh

export SWMROOT=${DOCUMENT_ROOT%/htdocs}
. $SWMROOT/bin/swmenv
[[ $KSHREC != 1 && ${KSH_VERSION##*' '} < $SHELLVERSION ]] && \
KSHREC=1 exec $SHELL $0 "$@"
. $SWMROOT/bin/swmgetinfo

argv0=${0##*/}
instance=${argv0%%_*}
rest=${argv0#$instance}
rest=${rest#_}
if [[ $instance == '' || $rest == '' ]] then
    print -u2 SWIFT ERROR program name not in correct form
    exit 1
fi
. $SWMROOT/proj/$instance/bin/${instance}_env || exit 1

. vg_hdr

for dir in ${PATH//:/' '}; do
    if [[ -x $dir/vg_dq_main ]] then
        . $dir/vg_dq_main
        break
    fi
done

suireadcgikv

name=$qs_name
dir=$SWIFTDATADIR/cache/main/$SWMID/$name
mkdir -p $dir || exit 1
cd $dir || exit 1

vars=(date="$qs_date")
dq_main_parsedates
rdir=${dq_main_data.rdir}

aattr=
set | egrep qs_action_ | while read -r line; do
    n=${line%%=*}
    n=${n#qs_}
    typeset -n nr=qs_$n
    if [[ $n == action__invdir ]] then
        rdir=$SWIFTDATADIR/cache/invedit/$SWMID/${nr//[/]/}
    fi
    if [[ $n == action__filter ]] then
        filter=${nr//[/]/}
    fi
    aattr+="&$n=$nr"
done

exec 3> invnode.filter 4> invedge.filter
typeset lvorder
typeset -A lvs sns alvs
ifs="$IFS"
IFS='|'
egrep -v '^#' $SWIFTDATADIR/uifiles/dataqueries/levelorder.txt \
| while read lv sn nl fn pn af attf cusf; do
    [[ $lv == '' ]] && continue
    lvorder[${#lvorder[@]}]=$lv
    lvs[$lv]=$lv
    sns[$lv]=$sn
done
IFS=':'
$SHELL vg_accountinfo $SWMID | while read -r line; do
    [[ $line != vg_level_* ]] && continue
    lv=${line%%=*}
    vs=${line##"$lv"?(=)}
    if [[ :$vs: != *:__ALL__:* ]] then
        s1= s2=
        for vi in $vs; do
            s1+="|$vi"
            s2+="|$vi${VG_EDGESEP}.*|.*${VG_EDGESEP}$vi"
        done
        s1=${s1#\|} s2=${s2#\|}
        print -r -u3 "F|${lv#*level_}|$s1"
        print -r -u4 "F|${lv#*level_}|$s2"
    fi
done
IFS="$ifs"

for lv in "${lvorder[@]}"; do
    s1= s2=
    typeset -n vs=qs_level_$lv
    for vi in "${vs[@]}"; do
        if [[ $vi == *${VG_EDGESEP}* ]] then
            vil=${vi%%${VG_EDGESEP}*}
            vir=${vi##*${VG_EDGESEP}}
            if [[ $vil != '.*' ]] then
                s1+="|$vil"
            fi
            if [[ $vir != '.*' ]] then
                s1+="|$vir"
            fi
            s2+="|$vi"
        else
            s1+="|$vi"
            vil="${vi}${VG_EDGESEP}.*"
            vir=".*${VG_EDGESEP}${vi}"
            s2+="|$vil|$vir"
        fi
    done
    s1=${s1#\|} s2=${s2#\|}
    [[ $s1 != '' ]] && print -r -u3 "I|${lv#*level_}|$s1"
    [[ $s2 != '' ]] && print -r -u4 "I|${lv#*level_}|$s2"
    [[ $s1 != '' ]] && alvs[$lv]=1
    s1= s2=
    typeset -n vs=qs_lname_$lv
    for vi in "${vs[@]}"; do
        if [[ $vi == *${VG_EDGESEP}* ]] then
            vil=${vi%%${VG_EDGESEP}*}
            vir=${vi##*${VG_EDGESEP}}
            if [[ $vil != '.*' ]] then
                s1+="|$vil"
            fi
            if [[ $vir != '.*' ]] then
                s1+="|$vir"
            fi
            s2+="|$vi"
        else
            s1+="|$vi"
            vil="${vi}${VG_EDGESEP}.*"
            vir=".*${VG_EDGESEP}${vi}"
            s2+="|$vil|$vir"
        fi
    done
    s1=${s1#\|} s2=${s2#\|}
    [[ $s1 != '' ]] && print -r -u3 "N|${lv#*level_}|$s1"
    [[ $s2 != '' ]] && print -r -u4 "N|${lv#*level_}|$s2"
    [[ $s1 != '' ]] && alvs[$lv]=1
done

exec 3>&- 4>&-

if [[ $qs_olevels == '' ]] then
    for lv in "${lvorder[@]}"; do
        [[ ${alvs[$lv]} == '' ]] && continue
        print -- $lv
    done
else
    for lv in ${qs_olevels//:/' '}; do
        print -- $lv
    done
fi > levellist.txt
export LEVELLISTFILE=levellist.txt
export LEVELLISTSIZE=$(wc -l < $LEVELLISTFILE)

export INVNODEFILTERFILE=invnode.filter
export INVNODEFILTERSIZE=$(wc -l < $INVNODEFILTERFILE)

export KEYVALUEFILTERFILE=keyvalue.filter
export KEYVALUEFILTERSIZE=0

export LEVELMAPFILE=$rdir/LEVELS-maps.dds
export INVMAPFILE=$rdir/inv-maps-uniq.dds
export INVNODEFILE=$rdir/inv-nodes-uniq.dds
export INVNODEATTRFILE=$rdir/inv-nodes.dds

export SHOWMAPS=0 SHOWRECS=1 SHOWATTRS=0 DOENC=1
export MAXN=${200:-$qs_maxn} SHOWPARTIAL=1

multiflag=n id=
ifs="$IFS"
IFS='|'
set -f
ddsfilter -osm none -fso vg_dq_invhelper.filter.so \
    $INVNODEFILE \
| sort -u -t '|' | while read -r line; do
    set -A ls -- $line
    case ${ls[0]} in
    B)
        [[ $id != '' ]] && multiflag=y
        lv=${ls[1]}; id=${ls[2]}
        ;;
    esac
done
set +f
IFS="$ifs"
print "Content-type: text/xml"
print "Expires: Mon, 19 Oct 1998 00:00:00 GMT\n"
print "<response>"
if [[ $id == '' || $multiflag == y ]] then
    print -u2 SWIFT ERROR no id for menu action
else
    typeset -A ans

    ifs="$IFS"
    IFS='|'
    {
        if [[ -f $SWIFTDATADIR/tmp/actions/cache1 ]] then
            if [[ $filter == '' ]] then
                look "$lv|$id|" $SWIFTDATADIR/tmp/actions/cache1
            else
                $filter < $SWIFTDATADIR/tmp/actions/cache1 \
                | egrep "^$lv\|$id\|" $SWIFTDATADIR/tmp/actions/cache1
            fi
        fi
        if [[ -f $SWIFTDATADIR/tmp/actions/cache2 ]] then
            if [[ $filter == '' ]] then
                cat $SWIFTDATADIR/tmp/actions/cache2
            else
                $filter < $SWIFTDATADIR/tmp/actions/cache2
            fi
        fi
    }| while read -r l1 i1 l2 i2 g u an al ae; do
        cl1=${l1//[!a-zA-Z0-9]/}
        ci1=${i1//[!a-zA-Z0-9]/}
        cl2=${l2//[!a-zA-Z0-9]/}
        ci2=${i2//[!a-zA-Z0-9]/}
        cg=${g//[!a-zA-Z0-9]/}
        cu=${u//[!a-zA-Z0-9]/}
        line="$l1|$i1|$l2|$i2|$g|$u|$an|$al|$ae"
        print "${#cl1}|${#ci1}|${#cl2}|${#ci2}|${#cg}|${#cu}|__END__|$line"
    done | sort -k1,4rn | sed 's/^.*__END__|//' \
    | while read -r l1 i1 l2 i2 g u an al ae; do
        [[ ${ans[$an]} != "" ]] && continue
        if [[
            $lv != ~(E)(^$l1$) || $id != ~(E)(^$i1$) || $SWMID != ~(E)(^$u$)
        ]] then
            continue
        fi
        if ! swmuseringroups "vg_att_admin|${g//:/'|'}"; then
            continue
        fi

        ans[$an]=1
        wtype=random
        case $an in
        *:random)
            wtype=random
            ;;
        *:same)
            wtype=same
            ;;
        *:*)
            wtype=name_${an#*:}
            wtype=${wtype//[!a-zA-Z0-9_]/_}
            ;;
        esac
        if [[ $ae == __SKIP__ ]] then
            continue
        fi
        [[ $aattr != '' ]] && ae+="$aattr"
        aurl=$(eval print -r "\"$ae\"")
        print -r "<d>$(printf '%#H' "$aurl")|$(printf '%#H' "$al")|$wtype</d>"
    done
    IFS="$ifs"
fi
print "</response>"
