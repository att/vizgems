function vg_stat_yearly_proc_run {
    typeset -n gv=$1
    typeset datadir=$2
    typeset linkdir=$3
    shift 3
    typeset dates="$@"

    typeset date year outdir

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

    for date in $dates; do
        sdpruncheck ${gv.exitfile} || break
        sdpspacecheck $datadir ${gv.mainspace} || break
        year=${date%%/*}
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
            if [[ ! -f $outdir/avg/total/avg.done ]] then
                (
                    print begin yearly stat prof processing $date $(date)
                    cd $outdir/avg/total
                    $SHELL vg_stat_prof yearly
                    print end yearly stat prof processing $date $(date)
                ) 2>&1
            fi
            (
                print begin yearly stat processing $date $(date)
                cd $outdir/total
                $SHELL vg_stat yearly
                print end yearly stat processing $date $(date)
            ) 2>&1
        )
    done

    typeset +n gv
}
