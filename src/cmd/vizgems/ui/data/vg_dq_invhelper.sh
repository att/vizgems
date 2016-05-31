
if [[ $SWIFTDATADIR == '' ]] then
    print -u2 ERROR SWIFTDATADIR not valid
    exit 1
fi

invdir=''
while [[ $1 == -* ]] do
    case $1 in
    -lv)
        ll=$2
        shift 2
        ;;
    -kv)
        if [[ $2 == *=* ]] then
            k=${2%%=*}
            v=${2#"$k"=}
            kvs[${#kvs[@]}]="$k|$v"
        fi
        shift 2
        ;;
    -date)
        date=$2
        shift 2
        ;;
    -invdir)
        invdir=$2
        shift 2
        ;;
    *)
        break
        ;;
    esac
done

ll=${ll:-$1}
date=${date:-latest}

level=$1
id1=$2
id2=$3

. vg_hdr

for dir in ${PATH//:/' '}; do
    if [[ -x $dir/vg_dq_main ]] then
        . $dir/vg_dq_main
        break
    fi
done

dir=$SWIFTDATADIR/cache/main/invhelper/invhelper.$$.$RANDOM.$RANDOM
mkdir -p $dir || exit 1
cd $dir || exit 1
trap 'rm -rf $dir' HUP QUIT TERM KILL EXIT

vars=(date="$date")
dq_main_parsedates
rdir=${invdir:-${dq_main_data.rdir}}

exec 3> invnode.filter 4> invedge.filter
if [[ $SWMID == '' ]] then
    exit
else
    ifs="$IFS"
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
typeset -l lid1=$id1 lid2=$id2
if [[ $lid1 == name=* ]] then
    print -r -u3 "N|$level|${id1#*=}"
elif [[ $id1 != '' ]] then
    print -r -u3 "I|$level|$id1"
fi
if [[ $lid2 == name=* ]] then
    print -r -u3 "N|$level|${id2#*=}"
elif [[ $id2 != '' ]] then
    print -r -u3 "I|$level|$id2"
fi
print -r -u4 "I|$level|$id1$VG_EDGESEP$id2|$id2$VG_EDGESEP$id1"
exec 3>&- 4>&-

for li in $ll; do
    print -- $li
done > levellist.txt
export LEVELLISTFILE=levellist.txt
export LEVELLISTSIZE=$(wc -l < $LEVELLISTFILE)

for kvi in "${!kvs[@]}"; do
    print -- ${kvs[$kvi]}
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

export SHOWMAPS=${SHOWMAPS:-0} SHOWRECS=${SHOWRECS:-0} SHOWATTRS=${SHOWATTRS:-0}
export MAXN=${MAXN:-0} DOENC=${DOENC:-0} SHOWPARTIAL=0

if [[ $id2 == '' ]] then
    ddsfilter -osm none -fso vg_dq_invhelper.filter.so $INVNODEFILE
else
    ddsfilter -osm none -fso vg_dq_invhelper_edge.filter.so $INVEDGEFILE
fi
