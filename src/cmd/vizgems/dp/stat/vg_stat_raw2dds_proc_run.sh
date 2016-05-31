function vg_stat_raw2dds_proc_run {
    typeset -n gv=$1
    typeset datadir=$2
    typeset linkdir=$3
    shift 3
    typeset dates="$@"

    typeset args maxjobs date year month day rest indir outdir
    typeset ifile ofile ymd t i j n doall

    if [[ -f $VGMAINDIR/dpfiles/config.sh ]] then
        . $VGMAINDIR/dpfiles/config.sh
    fi
    export CASEINSENSITIVE=${CASEINSENSITIVE:-n}
    export DEFAULTLEVEL=${DEFAULTLEVEL:-o}
    export DEFAULTTMODE=${DEFAULTTMODE:-keep}
    export MAXALARMKEEPMIN=${MAXALARMKEEPMIN:-120}
    export PROPALARMS=${PROPALARMS:-n}
    export PROPEMAILS=${PROPEMAILS:-n}
    export PROPSTATS=${PROPSTATS:-n}
    export STATINTERVAL=${STATINTERVAL:-300}
    export PROFILESAMPLES=${PROFILESAMPLES:-4}
    export GCEPHASEON=${GCEPHASEON:-n}
    export PCEPHASEON=${PCEPHASEON:-n}
    export NOTIMENORM=${NOTIMENORM:-0}
    export AGGRDELAY=${AGGRDELAY:-'1 hour'}
    export CONCATDELAY=${CONCATDELAY:-'120 min'}
    export ALWAYSCOMPRESSDP=${ALWAYSCOMPRESSDP:-n}
    export ALARMNOAGGR=${ALARMNOAGGR:-n}
    export STATNOAGGR=${STATNOAGGR:-n}
    export SORTUNIQSTATS=${SORTUNIQSTATS:-n}
    export ALARMSPLITFILESIZE=${ALARMSPLITFILESIZE:-0}
    export ALARMMAXFILERUN=${ALARMMAXFILERUN:-0}
    export STATSPLITFILESIZE=${STATSPLITFILESIZE:-0}
    export STATMAXFILERUN=${STATMAXFILERUN:-0}

    args="-X accept -K -k"
    maxjobs=${gv.raw2ddsjobs}
    for date in $dates; do
        sdpruncheck ${gv.exitfile} || break
        sdpspacecheck $datadir ${gv.mainspace} || break
        year=${date%%/*}
        rest=${date#*/}
        month=${rest%%/*}
        rest=${rest#*/}
        day=${rest%%/*}
        if ! sdpensuredualdirs $date $datadir $linkdir; then
            continue
        fi
        indir=$datadir/$date/input
        outdir=$datadir/$date/processed
        if ! sdpensuredirs -c $outdir/total; then
            continue
        fi
        if ! sdpensuredirs -c $outdir/avg/total; then
            continue
        fi
        (
            exec 3< ${gv.lockfile}
            if [[ -f $outdir/avg/total/avg.ahead ]] then
                rm -f $outdir/avg/total/avg.ahead
                rm -f $outdir/avg/total/avg.done
            fi
            if [[ ! -f $outdir/avg/total/avg.done ]] then
                (
                    print begin stat prof processing $date $(date)
                    cd $outdir/avg/total
                    $SHELL vg_stat_prof daily
                    print end stat prof processing $date $(date)
                ) 2>&1
            elif [[ $(printf '%(%Y.%m.%d)T') == ${year}.${month}.${day} ]] then
                t=$(printf '%(%#)T' ${year}.${month}.${day}-04:00:00)
                (( t += 24 * 60 * 60 ))
                ymd=$(printf '%(%Y.%m.%d)T' \#$t)
                if ! sdpensuredualdirs ${ymd//.//} $datadir $linkdir; then
                    continue
                fi
                noutdir=$datadir/${ymd//.//}/processed
                if ! sdpensuredirs -c $noutdir/total; then
                    continue
                fi
                if ! sdpensuredirs -c $noutdir/avg/total; then
                    continue
                fi
                if [[ ! -f $noutdir/avg/total/avg.done ]] then
                    (
                        print begin stat prof nd processing ${ymd//.//} $(date)
                        cd $noutdir/avg/total
                        $SHELL vg_stat_prof daily
                        touch avg.ahead
                        print end stat prof nd processing ${ymd//.//} $(date)
                    ) 2>&1
                fi
            fi
            (
                print begin stat processing $date $(date)
                cd $outdir/total
                mkdir -p $indir/stat
                export STATSAVEDIR=$indir/stat
                if [[
                    ! -f stat.list ||
                    $VGMAINDIR/dpfiles/stat/statlist.txt -nt stat.list
                ]] then
                    set -x
                    vg_stat sl $VGMAINDIR/dpfiles/stat/statlist.txt stat.list
                    set +x
                fi
                if [[ -f stat.list ]] then
                    doall=n
                    n=0
                    for i in ../../input/*.stats.@(xml|txt); do
                        [[ ! -f $i ]] && continue
                        j=${i##*/}
                        j=${j%.*}.done
                        [[ -f $j ]] && continue
                        set -x
                        vg_stat ${i##*.} $i objecttrans.dds stat.list $j &
                        set +x
                        doall=y
                        sdpjobcntl $maxjobs
                        if ((
                            STATMAXFILERUN > 10 && ++n >= STATMAXFILERUN
                        )) then
                            print -u2 SWIFT WARNING exceeded stat file limit $n
                            break
                        fi
                    done
                    wait
                    for i in *.stats.dds; do
                        if [[ -f $i ]] then
                            doall=y
                            break
                        fi
                    done
                    set -x
                    [[ $doall == y ]] && vg_stat all stat.list uniq.stats.dds
                    set +x
                fi
                for ifile in *stats*.fwd.*; do
                    [[ ! -f $ifile ]] && continue
                    ymd=${ifile##*.fwd.}
                    ofile=${ifile%.fwd.*}
                    ofile=${ofile%.dds}
                    ft=${ofile##*.}
                    ofile=${ofile%."$ft"}.${date//\//.}.$ft.dds
                    if ! sdpensuredualdirs ${ymd//.//} $datadir $linkdir; then
                        continue
                    fi
                    noutdir=$datadir/${ymd//.//}/processed
                    if ! sdpensuredirs -c $noutdir/total; then
                        continue
                    fi
                    if [[ ! -f $datadir/${ymd//.//}/complete.stamp ]] then
                        print -u2 MESSAGE moving $ifile to ${ymd//.//}
                        mv $ifile $noutdir/total/$ofile
                    else
                        rm $ifile
                    fi
                done
                print end stat processing $date $(date)
            ) 2>&1
        )
    done

    typeset +n gv
}
