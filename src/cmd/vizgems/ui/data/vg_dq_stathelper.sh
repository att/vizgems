
if [[ $SWIFTDATADIR == '' ]] then
    print -u2 ERROR SWIFTDATADIR not valid
    exit 1
fi

statdir=''
while [[ $1 == -* ]] do
    case $1 in
    -user)
        export SWMID=$2
        shift 2
        ;;
    -remote)
        remote=y; export REMOTEQUERY=y
        shift 1
        ;;
    -date)
        date=$2
        shift 2
        ;;
    -statdir)
        statdir=$2
        shift 2
        ;;
    *)
        break
        ;;
    esac
done

if [[ -f $SWIFTDATADIR/uifiles/dataqueries/setup.sh ]] then
    . $SWIFTDATADIR/uifiles/dataqueries/setup.sh
fi

if [[ $remote == y ]] then
    alist=
    while read i; do
        eval $i
        alist+=" ${i%%=*}"
    done
else
    alist=
    for i in "$@"; do
        eval $i
        alist+=" ${i%%=*}"
    done
fi

date=${date:-latest}

. vg_hdr

for dir in ${PATH//:/' '}; do
    if [[ -x $dir/vg_dq_main ]] then
        . $dir/vg_dq_main
        break
    fi
done

dir=$SWIFTDATADIR/cache/main/stathelper/stathelper.$$.$RANDOM.$RANDOM
mkdir -p $dir || exit 1
cd $dir || exit 1
trap 'rm -rf $dir' HUP QUIT TERM KILL EXIT

vars=(date="$date")
dq_main_parsedates
rdir=${statdir:-${dq_main_data.rdir}}

if [[ $REMOTESPECS != '' && $remote != y ]] then
    dirs=
    for spec in $REMOTESPECS; do
        dir=${spec//'/'/_}
        rm -rf $dir
        mkdir $dir
        (
            cd $dir
            cmd=${spec#*:}; cmd=${cmd//rundataquery/runstatquery}
            (
                for i in $alist; do
                    typeset -n an=$i
                    print -r "$i=$an"
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
    for dir in $dirs; do
        cat $dir/stat.txt
    done | sort -u -t '|' | while read -r line; do
        ifs="$IFS"
        IFS='|'
        set -f
        set -A ls -- $line
        set +f
        case ${ls[0]} in
        A)
            print -r "A|${ls[1]}|${ls[2]}|${ls[3]//\'/}"
            ;;
        B)
            print -r "B|${ls[1]}|${ls[2]}|${ls[3]//\'/}"
            ;;
        esac
        IFS="$ifs"
    done
    for key in "${!list2[@]}"; do
        print -r "B|$key.*|2|${list2[$key]}"
    done
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
if [[ $SWMID == '' ]] then
    exit
else
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
fi

for lv in "${lvorder[@]}"; do
    s1= s2=
    typeset -n vs=level_$lv
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
    typeset -n vs=lname_$lv
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
    typeset -n vs=stat_$param
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

export SHOWMAPS=0 DOENC=0
export MAXN=${200:-$maxn} SHOWPARTIAL=1

(
    if (( STATFILTERSIZE > 0 )) then
        ddsfilter -osm none -fso vg_dq_stathelper.filter.so $STATUNIQFILE
    fi
    for key in "${!list2[@]}"; do
        print -r "B|$key.*|2|${list2[$key]}"
    done
) | sort -u -t '|'
