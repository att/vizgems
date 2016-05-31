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

for kvi in "${!qs_kv[@]}"; do
    print -- ${qs_kv[$kvi]}
done > keyvalue.filter
export KEYVALUEFILTERFILE=keyvalue.filter
export KEYVALUEFILTERSIZE=$(wc -l < $KEYVALUEFILTERFILE)

export INVNODEFILTERFILE=invnode.filter
export INVNODEFILTERSIZE=$(wc -l < $INVNODEFILTERFILE)
export INVEDGEFILTERFILE=invedge.filter
export INVEDGEFILTERSIZE=$(wc -l < $INVEDGEFILTERFILE)

export LEVELMAPFILE=$rdir/LEVELS-maps.dds
export INVMAPFILE=$rdir/inv-maps-uniq.dds
export INVNODEFILE=$rdir/inv-nodes-uniq.dds
export INVNODEATTRFILE=$rdir/inv-nodes.dds
export INVEDGEFILE=$rdir/inv-edges-uniq.dds
export INVEDGEATTRFILE=$rdir/inv-edges.dds

export SHOWMAPS=0 SHOWRECS=${qs_showrecs:-1} SHOWATTRS=${qs_showattrs:-0}
export DOENC=1 MAXN=${qs_maxn:-200} SHOWPARTIAL=1

print "Content-type: text/xml"
print "Expires: Mon, 19 Oct 1998 00:00:00 GMT\n"
print "<response>"
ifs="$IFS"
IFS='|'
set -f
ddsfilter -osm none -fso vg_dq_invhelper.filter.so \
    $INVNODEFILE \
| sort -t '|' -k4,4 | while read -r line; do
    set -A ls -- $line
    case ${ls[0]} in
    A)
        print -r "<s>${ls[1]}|${ls[2]}|${ls[3]}</s>"
        ;;
    B)
        print -r "<d>${ls[1]}|${ls[2]}|${ls[3]}</d>"
        ;;
    D)
        print -r "<a>${ls[1]}|${ls[2]}|${ls[3]}|${ls[4]}</a>"
        ;;
    esac
done
set +f
IFS="$ifs"
print "</response>"
