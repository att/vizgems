
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
fi
uflag=
if [[ $SORTUNIQSTATS == y ]] then
    uflag='-u'
fi

mode=$1
shift

dir=${PWD%/processed/*}
dir=${dir##*main/}
date=${dir//\//.}

if [[ $mode == @(monthly|yearly) ]] then
    exec 4>&2
    lockfile=lock.stat
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
sl)
    n=$(egrep -v '^#' $1 | egrep '.*\|.*\|.*\|.*' | sed 's/\r//g' | wc -l)
    (
        print -- $n
        ifs="$IFS"
        IFS='|'
        egrep -v '^#' $1 | egrep '.*\|.*\|.*\|.*' | sed 's/\r//g' \
        | while read -r line; do
            set -A ls -- $line
            if [[ ${ls[1]} != *%* && ${ls[2]} != *%*%* ]] then
                print -r "$line"
            else
                ls[1]=${ls[1]//\%\%/___XXX___}
                while [[ ${ls[1]} == *%* ]] do
                    ls[1]=${ls[1]/\%/_}
                done
                ls[1]=${ls[1]//___XXX___/\%\%}
                ls[2]=${ls[2]//\%\%/___XXX___}
                while [[ ${ls[2]} == *%*%* ]] do
                    ls[2]=${ls[2]/\%/_}
                done
                ls[2]=${ls[2]//___XXX___/\%\%}
                print "${ls[*]}"
            fi
        done
    ) > $2.tmp && sdpmv $2.tmp $2
    ;;
xml)
    [[ -f ${4%.done}.dds ]] && touch $4
    [[ -f $4 ]] && exit 0

    if (( STATSPLITFILESIZE >= 100000 )) then
        if (( $(wc -c < $1) > STATSPLITFILESIZE )) then
            print -u2 SWIFT WARNING splitting large stat file $1 $(ls -lh $1)
            d=${1%/*}
            [[ $1 != */* ]] && d=.
            split -C $(( $STATSPLITFILESIZE / 2 )) -a 4 $1 $1.
            if [[ $? == 0 ]] then
                n=0
                for i in $1.*; do
                    if [[ -f $d/$n.${1##*/} ]] then
                        (( n++ ))
                        continue
                    fi
                    mv $i $d/$n.${1##*/}
                    (( n++ ))
                done
                if [[ $STATSAVEDIR != '' ]] then
                    mv $1 $STATSAVEDIR
                else
                    touch $4
                fi
                exit 0
            else
                rm -f $1.*
            fi
        fi
    fi

    d1="$date-00:00:00"
    d2="$date-23:59:59"
    export MINTIME=$(( $(printf '%(%#)T' "$d1") - 6 * 30 * 24 * 60 * 60 ))
    export MAXTIME=$(( $(printf '%(%#)T' "$d2") + 1 * 24 * 60 * 60 ))

    export OBJECTTRANSFILE=$2
    export STATMAPFILE=$3

    if [[ ! -f $OBJECTTRANSFILE ]] then
        print -u2 SWIFT MESSAGE waiting for inventory files
        exit 0
    fi

    rm -f ${4%.done}.dds.fwd.[0-9][0-9][0-9][0-9].[0-9][0-9].[0-9][0-9]
    vgxml -flatten2 vars < $1 | while read -u3 -r -C stat; do
        typeset -n sref=stat
        [[ $sref == '' ]] && continue
        typeset -n vref=stat.var
        [[ $vref == '' ]] && continue

        sid=${sref.sid:-${sref.scope}}
        ti=${sref.ti:-${sref.timeissued:-${sref.ts}}}
        frame=${sref.fr:-${sref.frame:--1}}
        l=${sref.lv:-${sref.level:-$dl}}
        id=${sref.id:-${sref.target}}
        id=${id//[!.a-zA-Z0-9_:-]/}
        key=${vref.k:-${vref.name}}
        val=${vref.n:-${vref.num}}
        unit=${vref.u:-${vref.unit}}
        label=${vref.l:-${vref.label}}
        print -u4 -nr "$sid|||$ti"
        print -u4 -r "|$l|$id|$key|$unit|$frame|$val|$label"
    done 3<&0- 4>&1- \
    | ddsload -os vg_stat.schema -le '|' -type ascii -dec url \
        -lnts -lso vg_stat.load.so \
    | ddsfilter -fso vg_stat.filter.so \
    | ddssort -fast -ke 'level id key unit frame' \
    | ddssplit -sso vg_stat_bydate.split.so -p ${4%.done}.dds.fwd.%s
    if [[ -f ${4%.done}.dds.fwd.$date ]] then
        mv ${4%.done}.dds.fwd.$date ${4%.done}.dds
    fi
    if [[ $STATSAVEDIR != '' ]] then
        mv $1 $STATSAVEDIR
    else
        touch $4
    fi
    ;;
txt)
    [[ -f ${4%.done}.dds ]] && touch $4
    [[ -f $4 ]] && exit 0

    d1="$date-00:00:00"
    d2="$date-23:59:59"
    export MINTIME=$(( $(printf '%(%#)T' "$d1") - 6 * 30 * 24 * 60 * 60 ))
    export MAXTIME=$(( $(printf '%(%#)T' "$d2") + 1 * 24 * 60 * 60 ))

    export OBJECTTRANSFILE=$2
    export STATMAPFILE=$3

    if [[ ! -f $OBJECTTRANSFILE ]] then
        print -u2 SWIFT MESSAGE waiting for inventory files
        exit 0
    fi

    rm -f ${4%.done}.dds.fwd.[0-9][0-9][0-9][0-9].[0-9][0-9].[0-9][0-9]
    while read -r line; do
        cust=${line%%\|*}
        rest=${line##"$cust"?(\|)}
        i1=${rest%%\|*}
        rest=${rest##"$i1"?(\|)}
        dstr=${rest%%\|*}
        rest=${rest##"$dstr"?(\|)}
        tstr=${rest%%\|*}
        rest=${rest##"$tstr"?(\|)}
        key=${rest%%\|*}
        rest=${rest##"$key"?(\|)}
        dnt=${dstr:11:4}.${dstr:5:2}.${dstr:8:2}-${tstr:5:2}:${tstr:8:2}:00
        if [[ $pdnt != $dnt ]] then
            t=$(printf '%(%#)T' $dnt)
            pdnt=$dnt
        fi
        l=$dl
        id=$i1
        id=${id//[!.a-zA-Z0-9_:-]/}
        key=${key##*=}
        label=$key
        k=
        while [[ $rest != '' ]] do
            kv=${rest%%\|*}
            rest=${rest##"$kv"?(\|)}
            k=${kv%%=*}
            v=${kv##"$k"?(=)}
            if [[ $v == *% ]] then
                unit=%
                v=${v%'%'}
            elif [[ $k == *% ]] then
                unit=%
            else
                unit=
            fi
            print -nr "|||$t"
            print -r "|$l|$id|$key.$k|$unit|-1|$v|$label $k"
        done
    done < $1 \
    | ddsload -os vg_stat.schema -le '|' -type ascii -dec simple \
        -lnts -lso vg_stat.load.so \
    | ddsfilter -fso vg_stat.filter.so \
    | ddssort -fast -ke 'level id key unit frame' \
    | ddssplit -sso vg_stat_bydate.split.so -p ${4%.done}.dds.fwd.%s
    if [[ -f ${4%.done}.dds.fwd.$date ]] then
        mv ${4%.done}.dds.fwd.$date ${4%.done}.dds
    fi
    if [[ $STATSAVEDIR != '' ]] then
        mv $1 $STATSAVEDIR
    else
        touch $4
    fi
    ;;
all)
    typeset -Z2 hh mm

    if ! ddscat -checksf stat.state; then
        print -u2 SWIFT ERROR uniq.stats.dds corrupted - recomputing
        rm -f stat.state uniq.stats.dds* all.*.stats.dds*
        for i in [0-9][0-9]*.stats.done; do
            rm -f $i
        done
    fi

    export STATMAPFILE=$1
    $SHELL vg_stat_ce check || exit 1

    hms=$(printf '%(%H.%M.%S)T')

    set -x
    args=${ print -r [0-9]*.stats.dds; }
    argcur=${#args}
    filen=${args//[! ]/}; filen=${#filen}
    argmax=${ getconf ARG_MAX; }
    filemax=${ ulimit -n; }
    if [[ $args == '[0-9]*.stats.dds' ]] then
        argcur=$argmax
    fi
    if (( argmax - argcur > 1024 && filemax - filen > 16 )) then
        for file in $args; do
            [[ ! -f $file ]] && continue
            print -r -- "./$file"
        done > stat.state
        ddssort -m -fast -ke 'level id key unit frame' $args
    else
        ddscat -is vg_stat.schema -sf stat.state \
            -fp '[0-9]*.stats.dds' \
        | ddssort -fast -ke 'level id key unit frame'
    fi \
    | case $PROPSTATS in
    y) ddsfilter -fso vg_stat_prop.filter.so ;;
    *) cat ;;
    esac \
    3> stats.$date.$hms.$$.2 \
    | ddssplit $zflag -sso vg_stat_byframe.split.so -p new.stats.byframe.%d
    if [[ $PCEPHASEON == y ]] then
        ddscat -is vg_stat.schema -fp 'new.stats.byframe.*' \
        | $SHELL vg_stat_ce run 3> alarms.$date.$hms.$$.3
    fi
    f=n
    (( fph = 60 * 60 / STATINTERVAL ))
    for i in new.stats.byframe.*; do
        [[ ! -f $i ]] && continue
        fr=${i##*.byframe.}
        hh=$(( fr / fph ))
        mm=$(( 5 * (((fr % fph) * 12) / fph) ))
        if [[ ! -f all.$hh.$mm.stats.dds ]] then
            ddscat -is vg_stat.schema \
            > all.$hh.$mm.stats.dds.tmp \
            && mv all.$hh.$mm.stats.dds.tmp all.$hh.$mm.stats.dds
        fi
        ddssort $zflag -fast -m $uflag -ke 'level id key unit frame' \
            $i all.$hh.$mm.stats.dds \
        > all.$hh.$mm.stats.dds.tmp \
        && mv all.$hh.$mm.stats.dds.tmp all.$hh.$mm.stats.dds
        ddssort $zflag -fast -u -m \
            -ke 'level id key unit' all.$hh.$mm.stats.dds \
        > uniq.$hh.$mm.stats.dds.tmp \
        && mv uniq.$hh.$mm.stats.dds.tmp uniq.$hh.$mm.stats.dds
        rm $i
        f=y
    done
    if [[ ! -f $2 ]] then
        ddscat -is vg_stat.schema \
        > $2.tmp && mv $2.tmp $2
    fi
    if [[ $f == y ]] then
        files=
        for file in uniq.*.stats.dds; do
            [[ ! -f $file ]] && continue
            files+=" $file"
        done
        if [[ $files != '' ]] then
            ddssort $zflag -fast -m -u -ke 'level id key unit' $2 $files \
            > $2.tmp && mv $2.tmp $2
            rm $files
        fi
    fi
    if [[ -s stats.$date.$hms.$$.2 ]] then
        file=stats.$date.$hms.$$
        while [[ -f $VGMAINDIR/outgoing/stat/$file.xml ]] do
            file=$file.0
        done
        mv stats.$date.$hms.$$.2 $VGMAINDIR/outgoing/stat/$file.xml
    else
        rm stats.$date.$hms.$$.2
    fi
    if [[ -s alarms.$date.$hms.$$.3 ]] then
        file=$hms.$$.ticketed
        while [[ -f $file.alarms.xml ]] do
            file=$file.0
        done
        mv alarms.$date.$hms.$$.3 $file.alarms.xml
    else
        rm -f alarms.$date.$hms.$$.3
    fi
    set +x
    egrep / stat.state | while read i; do
        rm $i
    done
    rm stat.state
    cymdh=$(printf '%(%Y.%m.%d.%H)T' "$CONCATDELAY ago")
    [[ $FORCESTATCAT == y ]] && cymdh=9999.99.99.99
    for (( hh = 0; hh < 24; hh++ )) do
        [[ $date.$hh > $cymdh ]] && continue
        if [[ ! -f all.$hh.stats.dds ]] then
            ddscat -is vg_stat.schema \
            > all.$hh.stats.dds.tmp \
            && mv all.$hh.stats.dds.tmp all.$hh.stats.dds
        fi
        files=
        for file in all.$hh.*.stats.dds; do
            [[ ! -f $file ]] && continue
            files+=" $file"
        done
        if [[ $files != '' ]] then
            set -x
            ddssort $zflag -fast -m $uflag -ke 'level id key unit frame' \
                $files all.$hh.stats.dds \
            > all.$hh.stats.dds.tmp \
            && mv all.$hh.stats.dds.tmp all.$hh.stats.dds
            rm $files
            touch $2
            set +x
        fi
    done
    ;;
monthly)
    [[ $STATNOAGGR == [yY] ]] && exit 0

    typeset -Z2 hh

    for d1 in ../../[0-9][0-9]; do
        d2=$d1/processed/total
        if [[ $d2/stat.list -nt stat.list ]] then
            cp -p $d2/stat.list stat.list
        fi
    done
    if [[ ! -f stat.list ]] then
        print -u2 SWIFT MESSAGE waiting for level1 stat.list file
        exit 0
    fi
    export STATMAPFILE=stat.list

    cymdh=$(printf '%(%Y.%m.%d.%H)T' "$AGGRDELAY ago")
    ts=$(printf '%(%Y.%m.%d.%H:%M:%S)T')
    f=n
    for d1 in ../../[0-9][0-9]; do
        dd=${d1##*/}
        d2=$d1/processed/total
        [[ ! -f $d2/uniq.stats.dds ]] && continue
        if [[ all.$dd.stats.dds -nt $d2/uniq.stats.dds ]] then
            if [[ -f $d1/complete.stamp && ! -f $d1/complete.stamp1 ]] then
                rm -f _stat$dd*
            fi
            continue
        fi
        f2=n
        files=
        for (( hh = 0; hh < 24; hh++ )) do
            [[ ! -f $d2/all.$hh.stats.dds ]] && continue
            files+=" $d2/all.$hh.stats.dds"
            [[ $date.$dd.$hh > $cymdh && $WAITLOCK != y ]] && continue
            [[ all.$dd.stats.dds -nt $d2/all.$hh.stats.dds ]] && continue
            f2=y
        done
        [[ $f2 == n ]] && continue
        set -x
        for file in $files; do
            ofile=_stat$dd.${file##*/}
            [[ -f $ofile && $ofile -nt $file ]] && continue
            MODE=1 ddsconv -ozm rtb \
                -os vg_stat.schema -cso vg_stat_aggr.conv.so $file \
            > $ofile &
            sdpjobcntl 2
        done
        wait
        ofiles=$(print _stat$dd*)
        [[ $ofiles != '_stat$dd*' ]] && ddssort \
            -fast -ozm rtb -m -u -ke 'level id key unit frame' $ofiles \
        > all.$dd.stats.dds.tmp \
        && mv all.$dd.stats.dds.tmp all.$dd.stats.dds
        touch -d $ts all.$dd.stats.dds
        ddssort -fast -ozm rtb -m -u -ke 'level id key unit' all.$dd.stats.dds \
        > uniq.$dd.stats.dds.tmp \
        && mv uniq.$dd.stats.dds.tmp uniq.$dd.stats.dds
        set +x
        f=y
    done
    if [[ $f == y ]] then
        files=
        for file in uniq.*.stats.dds; do
            [[ ! -f $file ]] && continue
            files+=" $file"
        done
        if [[ $files != '' ]] then
            set -x
            ddssort -fast -ozm rtb -m -u -ke 'level id key unit' $files \
            > uniq.stats.dds.tmp && mv uniq.stats.dds.tmp uniq.stats.dds
            set +x
        fi
    elif [[ ! -f uniq.stats.dds ]] then
        file=
        for m1 in ../../../[0-9][0-9]; do
            mm=${m1##*/}
            [[ $mm == $dir ]] && continue
            m2=$m1/processed/total
            [[ -f $m2/uniq.stats.dds ]] && file=$m2/uniq.stats.dds
        done
        if [[ $file != '' ]] then
            set -x
            cp $file uniq.stats.dds.tmp \
            && mv uniq.stats.dds.tmp uniq.stats.dds
            set +x
        fi
    fi
    ;;
yearly)
    [[ $STATNOAGGR == [yY] ]] && exit 0

    typeset -Z2 dd

    for d1 in ../../[0-9][0-9]; do
        d2=$d1/processed/total
        if [[ $d2/stat.list -nt stat.list ]] then
            cp -p $d2/stat.list stat.list
        fi
    done
    if [[ ! -f stat.list ]] then
        print -u2 SWIFT MESSAGE waiting for level2 stat.list file
        exit 0
    fi
    export STATMAPFILE=stat.list

    f=n
    for d1 in ../../[0-9][0-9]; do
        mm=${d1##*/}
        d2=$d1/processed/total
        [[ ! -f $d2/uniq.stats.dds ]] && continue
        if [[ all.$mm.stats.dds -nt $d2/uniq.stats.dds ]] then
            if [[ -f $d1/complete.stamp && ! -f $d1/complete.stamp1 ]] then
                rm -f _stat$mm*
            fi
            continue
        fi
        files=
        for (( dd = 1; dd < 32; dd++ )) do
            [[ ! -f $d2/all.$dd.stats.dds ]] && continue
            files+=" $d2/all.$dd.stats.dds"
        done
        [[ $files == '' ]] && continue
        set -x
        for file in $files; do
            ofile=_stat$mm.${file##*/}
            [[ -f $ofile && $ofile -nt $file ]] && continue
            MODE=2 ddsconv -ozm rtb \
                -os vg_stat.schema -cso vg_stat_aggr.conv.so $file \
            > $ofile &
            sdpjobcntl 2
        done
        wait
        ofiles=$(print _stat$mm*)
        [[ $ofiles != '_stat$mm*' ]] && ddssort \
            -fast -ozm rtb -m -u -ke 'level id key unit frame' $ofiles \
        > all.$mm.stats.dds.tmp \
        && mv all.$mm.stats.dds.tmp all.$mm.stats.dds
        ddssort -fast -ozm rtb -m -u -ke 'level id key unit' all.$mm.stats.dds \
        > uniq.$mm.stats.dds.tmp \
        && mv uniq.$mm.stats.dds.tmp uniq.$mm.stats.dds
        set +x
        f=y
    done
    if [[ $f == y ]] then
        files=
        for file in uniq.*.stats.dds; do
            [[ ! -f $file ]] && continue
            files+=" $file"
        done
        if [[ $files != '' ]] then
            set -x
            ddssort -fast -ozm rtb -m -u -ke 'level id key unit' $files \
            > uniq.stats.dds.tmp && mv uniq.stats.dds.tmp uniq.stats.dds
            set +x
        fi
    elif [[ ! -f uniq.stats.dds ]] then
        file=
        for y1 in ../../../[0-9][0-9][0-9][0-9]; do
            yy=${y1##*/}
            [[ $yy == $dir ]] && continue
            y2=$y1/processed/total
            [[ -f $y2/uniq.stats.dds ]] && file=$y2/uniq.stats.dds
        done
        if [[ $file != '' ]] then
            set -x
            cp $file uniq.stats.dds.tmp \
            && mv uniq.stats.dds.tmp uniq.stats.dds
            set +x
        fi
    fi
    ;;
esac
