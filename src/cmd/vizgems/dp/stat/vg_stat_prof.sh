
if [[ $SWIFTWARNLEVEL != '' ]] then
    set -x
fi

if [[ $ALWAYSCOMPRESSDP == y ]] then
    zflag='-ozm rtb'
fi

. vg_hdr
ver=$VG_S_VERSION

mode=$1
shift

dir=${PWD%/processed/*}
date=${dir##*main/}
dir=${dir%/"$date"}
date=${date//\//.}

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
daily)
    typeset -Z2 hh

    dflag=y
    ct=${ printf '%(%#)T' $date-12:00:00;  }
    for (( i = 0; i < PROFILESAMPLES; i++ )) do
        pdate=${ printf '%(%Y.%m.%d)T' \#$(( ct - (i + 1) * 604800 )); }
        if [[ -d $dir/${pdate//.//}/processed/total ]] then
            dirs[$i]=$dir/${pdate//.//}/processed/total
            [[ ! -f $dir/${pdate//.//}/complete.stamp ]] && dflag=n
        fi
    done
    if (( ${#dirs[@]} < PROFILESAMPLES / 2 )) then
        dflag=y
        for (( i = 0; i < PROFILESAMPLES; i++ )) do
            pdate=${ printf '%(%Y.%m.%d)T' \#$(( ct - (i + 1) * 86400 )); }
            if [[ -d $dir/${pdate//.//}/processed/total ]] then
                dirs[$i]=$dir/${pdate//.//}/processed/total
                [[ ! -f $dir/${pdate//.//}/complete.stamp ]] && dflag=n
            fi
        done
    fi
    if (( ${#dirs[@]} == 0 )) then
        touch avg.done
        exit 0
    fi

    for (( hh = 0; hh < 24; hh++ )) do
        files= rflag=n
        for dir in "${dirs[@]}"; do
            file=$dir/all.$hh.stats.dds
            if [[ -f $file ]] then
                [[ $file -nt mean.$hh.stats.dds ]] && rflag=y
                files+=" $file"
            fi
            for file in $dir/all.$hh.*.stats.dds; do
                if [[ -f $file ]] then
                    [[ $file -nt mean.$hh.stats.dds ]] && rflag=y
                    files+=" $file"
                fi
            done
        done
        [[ $rflag == n ]] && continue
        set -x
        export BASETIME=$(printf '%(%#)T' $date-$hh:00:00)
        (( fph = 3600 / STATINTERVAL ))
        export MINFRAME=$(( hh * fph )) MAXFRAME=$(( (hh + 1) * fph - 1 ))
        ddssort -fast -m -ke 'level id key unit frame' $files \
        | ddsconv -os vg_stat.schema -cso vg_stat_prof.conv.so \
        | ddssplit $zflag -sso vg_stat_prof.split.so -p %s.$hh.stats.dds
        for i in ___*___.$hh.stats.dds; do
            [[ ! -f $i ]] && continue
            mv $i ${i//_/}
        done
        if [[ ! -f mean.$hh.stats.dds ]] then
            ddscat -is vg_stat.schema > mean.$hh.stats.dds
        fi
        if [[ ! -f sdev.$hh.stats.dds ]] then
            ddscat -is vg_stat.schema > sdev.$hh.stats.dds
        fi
        set +x
        # do one file per run to not delay realtime processing
        [[ $DOALLPROF != y ]] && exit 0
    done

    if [[ $dflag == y ]] then
        set -x
        touch avg.done
        set +x
    fi
    ;;
monthly)
    if [[ $STATNOAGGR == [yY] ]] then
        touch avg.done
        exit 0
    fi

    typeset -Z2 hh

    for d1 in ../../../[0-9][0-9]; do
        d2=$d1/processed/avg/total
        if [[ $d2/../../total/stat.list -nt stat.list ]] then
            cp -p $d2/../../total/stat.list stat.list
        fi
    done
    if [[ ! -f stat.list ]] then
        print -u2 SWIFT MESSAGE waiting for level1 stat.list file
        exit 0
    fi
    export STATMAPFILE=stat.list

    done=y
    f=n
    for d1 in ../../../[0-9][0-9]; do
        dd=${d1##*/}
        d2=$d1/processed/avg/total
        [[ ! -f $d1/complete.stamp ]] && done=n
        [[ ! -f $d2/avg.done ]] && continue
        [[ -f $d2/avg.ahead ]] && continue
        f=y
        [[ mean.$dd.stats.dds -nt $d2/avg.done ]] && continue
        files=
        for (( hh = 0; hh < 24; hh++ )) do
            [[ ! -f $d2/mean.$hh.stats.dds ]] && continue
            files+=" $d2/mean.$hh.stats.dds"
        done
        [[ $files == '' ]] && continue
        set -x
        rm -f _stat*
        for file in $files; do
            ofile=_stat$dd.${file##*/}
            MODE=1 ddsconv -ozm rtb \
                -os vg_stat.schema -cso vg_stat_aggr.conv.so $file \
            > $ofile &
            sdpjobcntl 2
        done
        wait
        ofiles=$(print _stat$dd*)
        [[ $ofiles != '_stat$dd*' ]] && ddssort \
            -fast -ozm rtb -m -u -ke 'level id key unit frame' $ofiles \
        > mean.$dd.stats.dds.tmp \
        && mv mean.$dd.stats.dds.tmp mean.$dd.stats.dds
        rm -f _stat*
        set +x
    done
    f=n
    for d1 in ../../../[0-9][0-9]; do
        dd=${d1##*/}
        d2=$d1/processed/avg/total
        [[ ! -f $d2/avg.done ]] && continue
        [[ -f $d2/avg.ahead ]] && continue
        f=y
        [[ sdev.$dd.stats.dds -nt $d2/avg.done ]] && continue
        files=
        for (( hh = 0; hh < 24; hh++ )) do
            [[ ! -f $d2/sdev.$hh.stats.dds ]] && continue
            files+=" $d2/sdev.$hh.stats.dds"
        done
        [[ $files == '' ]] && continue
        set -x
        rm -f _stat*
        for file in $files; do
            ofile=_stat$dd.${file##*/}
            MODE=1 ddsconv -ozm rtb \
                -os vg_stat.schema -cso vg_stat_aggr.conv.so $file \
            > $ofile &
            sdpjobcntl 2
        done
        wait
        ofiles=$(print _stat$dd*)
        [[ $ofiles != '_stat$dd*' ]] && ddssort \
            -fast -ozm rtb -m -u -ke 'level id key unit frame' $ofiles \
        > sdev.$dd.stats.dds.tmp \
        && mv sdev.$dd.stats.dds.tmp sdev.$dd.stats.dds
        rm -f _stat*
        set +x
    done
    if [[ $f == y && $done == y ]] then
        touch avg.done
    fi
    ;;
yearly)
    if [[ $STATNOAGGR == [yY] ]] then
        touch avg.done
        exit 0
    fi

    typeset -Z2 dd

    for d1 in ../../../[0-9][0-9]; do
        d2=$d1/processed/avg/total
        if [[ $d2/../../total/stat.list -nt stat.list ]] then
            cp -p $d2/../../total/stat.list stat.list
        fi
    done
    if [[ ! -f stat.list ]] then
        print -u2 SWIFT MESSAGE waiting for level2 stat.list file
        exit 0
    fi
    export STATMAPFILE=stat.list

    done=y
    f=n
    for d1 in ../../../[0-9][0-9]; do
        mm=${d1##*/}
        d2=$d1/processed/avg/total
        [[ ! -f $d1/complete.stamp ]] && done=n
        [[ ! -f $d2/avg.done ]] && continue
        f=y
        [[ mean.$mm.stats.dds -nt $d2/avg.done ]] && continue
        files=
        for (( dd = 1; dd < 32; dd++ )) do
            [[ ! -f $d2/mean.$dd.stats.dds ]] && continue
            files+=" $d2/mean.$dd.stats.dds"
        done
        [[ $files == '' ]] && continue
        set -x
        rm -f _stat*
        for file in $files; do
            ofile=_stat$mm.${file##*/}
            MODE=2 ddsconv -ozm rtb \
                -os vg_stat.schema -cso vg_stat_aggr.conv.so $file \
            > $ofile &
            sdpjobcntl 2
        done
        wait
        ofiles=$(print _stat$mm*)
        [[ $ofiles != '_stat$mm*' ]] && ddssort \
            -fast -ozm rtb -m -u -ke 'level id key unit frame' $ofiles \
        > mean.$mm.stats.dds.tmp \
        && mv mean.$mm.stats.dds.tmp mean.$mm.stats.dds
        rm -f _stat*
        set +x
    done
    f=n
    for d1 in ../../../[0-9][0-9]; do
        mm=${d1##*/}
        d2=$d1/processed/avg/total
        [[ ! -f $d2/avg.done ]] && continue
        f=y
        [[ sdev.$mm.stats.dds -nt $d2/avg.done ]] && continue
        files=
        for (( dd = 1; dd < 32; dd++ )) do
            [[ ! -f $d2/sdev.$dd.stats.dds ]] && continue
            files+=" $d2/sdev.$dd.stats.dds"
        done
        [[ $files == '' ]] && continue
        set -x
        rm -f _stat*
        for file in $files; do
            ofile=_stat$mm.${file##*/}
            MODE=2 ddsconv -ozm rtb \
                -os vg_stat.schema -cso vg_stat_aggr.conv.so $file \
            > $ofile &
            sdpjobcntl 2
        done
        wait
        ofiles=$(print _stat$mm*)
        [[ $ofiles != '_stat$mm*' ]] && ddssort \
            -fast -ozm rtb -m -u -ke 'level id key unit frame' $ofiles \
        > sdev.$mm.stats.dds.tmp \
        && mv sdev.$mm.stats.dds.tmp sdev.$mm.stats.dds
        rm -f _stat*
        set +x
    done
    if [[ $f == y && $done == y ]] then
        touch avg.done
    fi
    ;;
esac
