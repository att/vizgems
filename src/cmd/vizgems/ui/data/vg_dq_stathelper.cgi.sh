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

if [[ -f $SWIFTDATADIR/uifiles/dataqueries/setup.sh ]] then
    . $SWIFTDATADIR/uifiles/dataqueries/setup.sh
fi
vars=(date="$qs_date")
dq_main_parsedates
rdir=${dq_main_data.rdir}

if [[ $REMOTESPECS != '' ]] then
    dirs=
    for spec in $REMOTESPECS; do
        dir=${spec//'/'/_}
        rm -rf $dir
        mkdir $dir
        (
            cd $dir
            cmd=${spec#*:}; cmd=${cmd//rundataquery/runstatquery}
            (
                for i in $qslist; do
                    typeset -n qsn=qs_$i
                    print -r "$i=$qsn"
                done
            ) | ssh ${spec%%:*} $cmd -remote -user "$SWMID" \
            > stat.txt.tmp 2> errors
            if [[ $? != 0 ]] then
                print -u2 VG QUERY ERROR: remote query to $spec failed
                exit 1
            fi
            mv stat.txt.tmp stat.txt
        ) &
        dirs+=" $dir"
    done
    wait
    print "Content-type: text/xml"
    print "Expires: Mon, 19 Oct 1998 00:00:00 GMT\n"
    print "<response>"
    set -f
    for dir in ${dirs#' '}; do
        egrep ^A $dir/stat.txt
    done | while read -r line; do
        ifs="$IFS"
        IFS='|'
        set -A ls -- $line
        case ${ls[2]} in
        1)
            (( sum11 += ${ls[1]} ))
            (( sum12 += ${ls[3]} ))
            ;;
        2)
            (( sum21 += ${ls[1]} ))
            (( sum22 += ${ls[3]} ))
            ;;
        esac
        IFS="$ifs"
    done
    print "<s>1|${sum11}|${sum12}</s>"
    print "<s>2|${sum21}|${sum22}</s>"
    for dir in ${dirs#' '}; do
        egrep ^B $dir/stat.txt
    done | sort -u -t '|' | while read -r line; do
        ifs="$IFS"
        IFS='|'
        set -A ls -- $line
        print -r "<d>${ls[2]}|${ls[1]}|${ls[3]//\'/}</d>"
        IFS="$ifs"
    done
    set +f
    print "</response>"
    exit 0
fi

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
typeset -A sts
egrep -v '^#' $SWIFTDATADIR/dpfiles/stat/statlist.txt \
| while read key lab1 j; do
    [[ $key == '' ]] && continue
    sts[$key]=$lab1
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
exec 3> stat.filter

typeset -A list2
for param in key label; do
    s1=
    typeset -n vs=qs_stat_$param
    for vi in "${vs[@]}"; do
        if [[ $vi == *'.*' ]] then
            key=${vi%.*}
            key=${key%.}
            if [[ ${sts[$key]} != '' ]] then
                list2[$key]=${sts[$key]}
                continue
            fi
        fi
        s1+="|$vi"
    done
    s1=${s1#\|}
    [[ $s1 != '' ]] && print -r -u3 "S|$param|$s1"
done

exec 3>&-

export INVNODEFILTERFILE=invnode.filter
export INVNODEFILTERSIZE=$(wc -l < $INVNODEFILTERFILE)

export STATFILTERFILE=stat.filter
export STATFILTERSIZE=$(wc -l < $STATFILTERFILE)

export LEVELMAPFILE=$rdir/LEVELS-maps.dds
export INVMAPFILE=$rdir/inv-maps-uniq.dds
export INVNODEFILE=$rdir/inv-nodes-uniq.dds
export INVNODEATTRFILE=$rdir/inv-nodes.dds
export STATUNIQFILE=$rdir/uniq.stats.dds

export SHOWMAPS=0 DOENC=1
export MAXN=${200:-$qs_maxn} SHOWPARTIAL=1

print "Content-type: text/xml"
print "Expires: Mon, 19 Oct 1998 00:00:00 GMT\n"
print "<response>"
ifs="$IFS"
IFS='|'
set -f
(
    if (( STATFILTERSIZE > 0 )) then
        ddsfilter -osm none -fso vg_dq_stathelper.filter.so $STATUNIQFILE
    fi
    for key in "${!list2[@]}"; do
        print -r "B|$key.*|2|${list2[$key]}"
    done
) | sort -u -t '|' | while read -r line; do
    set -A ls -- $line
    case ${ls[0]} in
    A)
        print -r "<s>${ls[2]}|${ls[1]}|${ls[3]}</s>"
        ;;
    B)
        print -r "<d>${ls[2]}|${ls[1]}|${ls[3]//\'/}</d>"
        ;;
    esac
done
set +f
IFS="$ifs"
print "</response>"
