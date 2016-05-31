
if [[ $SWIFTWARNLEVEL != '' ]] then
    set -x
fi

. vg_hdr
ver=$VG_S_VERSION

dl=$DEFAULTLEVEL
if [[ $CASEINSENSITIVE == y ]] then
    typeset -l l id
fi
if [[ $ALWAYSCOMPRESSDP == y ]] then
    zflag='-ozm rtb'
    export ZFLAG=$zflag
fi

spec=()

if [[ -f $GENERICPATH/bin/spec.sh ]] then
    . $GENERICPATH/bin/spec.sh
fi

mode=$1
shift

dir=${PWD%/processed/*}
dir=${dir##*main/}
date=${dir//\//.}

if [[ $mode == @(monthly|yearly) ]] then
    exec 4>&2
    lockfile=lock.generic
    set -o noclobber
    while ! command exec 3> $lockfile; do
        if kill -0 $(< $lockfile); then
            [[ $WAITLOCK != y ]] && exit 0
        elif [[ $(fuser $lockfile) != '' ]] then
            [[ $WAITLOCK != y ]] && exit 0
        else
            rm -f $lockfile
        fi
        print -u4 SWIFT MESSAGE waiting for instance $(< ${lockfile}) to exit
        sleep 1
    done 2> /dev/null
    print -u3 $$
    set +o noclobber
    trap "rm -f $lockfile" HUP QUIT TERM KILL EXIT
    exec 4>&-
fi

case $mode in
one)
    d1="$date-00:00:00"
    d2="$date-23:59:59"
    export MINTIME=$(( $(printf '%(%#)T' "$d1") - 6 * 30 * 24 * 60 * 60 ))
    export MAXTIME=$(( $(printf '%(%#)T' "$d2") + 1 * 24 * 60 * 60 ))

    ${spec.inc2raw} < $1 | tee main1 | ${spec.splitbyday} $2.fwd.%s
    if [[ -f $2.fwd.$date ]] then
        mv $2.fwd.$date $2
    fi
    mv $1 $GENERICSAVEDIR
    ;;
all)
    ${spec.concatbyhour}
    ;;
monthly)
    ${spec.aggrbyday}
    ;;
yearly)
    ${spec.aggrbymonth}
    ;;
esac
