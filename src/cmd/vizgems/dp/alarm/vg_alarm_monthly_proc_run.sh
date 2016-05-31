function vg_alarm_monthly_proc_run {
    typeset -n gv=$1
    typeset datadir=$2
    typeset linkdir=$3
    shift 3
    typeset dates="$@"

    typeset date year month rest outdir

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

    for date in $dates; do
        sdpruncheck ${gv.exitfile} || break
        sdpspacecheck $datadir ${gv.mainspace} || break
        year=${date%%/*}
        rest=${date#*/}
        month=${rest%%/*}
        if ! sdpensuredualdirs $date $datadir $linkdir; then
            continue
        fi
        outdir=$datadir/$date/processed
        if ! sdpensuredirs -c $outdir/total; then
            continue
        fi
        if ! sdpensuredirs -c $outdir/avg/total; then
            continue
        fi
        (
            exec 3< ${gv.lockfile}
            (
                print begin monthly alarm processing $date $(date)
                cd $outdir/total
                $SHELL vg_alarm monthly
                print end monthly alarm processing $date $(date)
            ) 2>&1
        )
    done

    typeset +n gv
}
