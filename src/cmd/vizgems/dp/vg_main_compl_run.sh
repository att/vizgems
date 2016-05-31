function vg_main_compl_run {
    typeset -n gv=$1
    typeset datadir=$2
    typeset linkdir=$3
    shift 3
    typeset dates="$@"

    typeset todaysec date datesec diffsec year month day rest
    typeset indir outdir state state2 file t1 t2

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

    todaysec=$(printf '%(%#)T')
    if [[ ${gv.completeday1} == *h ]] then
        (( t1 = ${gv.completeday1%h} * 60 * 60 ))
    elif [[ ${gv.completeday1} == *d ]] then
        (( t1 = ${gv.completeday1%d} * 24 * 60 * 60 ))
    else
        (( t1 = ${gv.completeday1} * 24 * 60 * 60 ))
    fi
    if [[ ${gv.completeday2} == *h ]] then
        (( t2 = ${gv.completeday2%h} * 60 * 60 ))
    elif [[ ${gv.completeday2} == *d ]] then
        (( t2 = ${gv.completeday2%d} * 24 * 60 * 60 ))
    else
        (( t2 = ${gv.completeday2} * 24 * 60 * 60 ))
    fi
    for date in $dates; do
        [[ $date != ????/??/?? ]] && continue
        sdpruncheck ${gv.exitfile} || break
        sdpspacecheck $datadir ${gv.mainspace} || break
        year=${date%%/*}
        rest=${date#*/}
        month=${rest%%/*}
        rest=${rest#*/}
        day=${rest%%/*}
        datesec=$(printf '%(%#)T' ${date//\//.}-23:59:59)
        [[ ! -d $datadir/$date ]] && continue
        if [[ $datadir/$date -ef $datadir/latest ]] then
            continue
        fi
        indir=$datadir/$date/input
        outdir=$datadir/$date/processed
        if [[ -f $datadir/$date/notcomplete.stamp ]] then
            if [[ $(
                tw -d $outdir/total -e '
                    mtime > "1 hours ago" && name=="*.(alarms|stats).dds" &&
                    name!="open.*"
                '
            ) == '' ]] then
                rm $datadir/$date/notcomplete.stamp
            else
                continue
            fi
        fi
        (( diffsec = todaysec - datesec ))
        if (( diffsec > t2 )) then
            state=2
        elif (( diffsec > t1 )) then
            state=1
        else
            state=0
        fi
        if (( diffsec > 2 * t2 )) then
            state2=2
        elif (( diffsec > (t1 + t2) / 2 )) then
            state2=1
        else
            state2=0
        fi
        if [[ $state == 2 && ! -f $datadir/$date/complete.stamp1 ]] then
            state=1
        fi
        case $state in
        0)
            ;;
        1)
            if [[ -f $datadir/$date/complete.stamp ]] then
                (
                    cd $indir || exit
                    tw -i -e 'type==REG && mtime < "60 min ago"' rm
                )
            fi
            for file in $datadir/$date/processed/total/all.??.??.stats.dds; do
                break
            done
            if [[ -f $file ]] then
                if [[ $state2 == 1 ]] then
                    (
                        print begin incomplete1 processing $date $(date)
                        cd $outdir/total || exit
                        PCEPHASEON=n $SHELL vg_stat all stat.list uniq.stats.dds
                        print end incomplete1 processing $date $(date)
                    )
                fi
            fi
            touch $datadir/$date/complete.stamp $datadir/$date/complete.stamp1
            ;;
        2)
            for file in $datadir/$date/processed/total/all.??.??.stats.dds; do
                break
            done
            if [[ -f $file ]] then
                if [[ $state2 == 1 ]] then
                    (
                        print begin incomplete2 processing $date $(date)
                        cd $outdir/total || exit
                        PCEPHASEON=n $SHELL vg_stat all stat.list uniq.stats.dds
                        print end incomplete2 processing $date $(date)
                    )
                fi
                if [[ $state2 != 2 ]] then
                    print WARNING keeping $date open
                    continue
                fi
            fi
            (
                print begin complete processing $date $(date)
                (
                    cd $outdir/avg/total || exit
                    [[ ! -f avg.done ]] && $SHELL vg_stat_prof daily
                    rm -f log
                    vg_main_compl_compress mean.*.stats.dds sdev.*.stats.dds
                )
                (
                    cd $outdir/total || exit
                    PCEPHASEON=n FORCESTATCAT=y \
                    $SHELL vg_stat all stat.list uniq.stats.dds
                    rm -f log *.mo *.ms *.state
                    tw -i -d . -e 'name=="[0-9]*.alarms.dds"' rm
                    tw -i -d . -e 'name=="[0-9]*.alarms.done"' rm
                    tw -i -d . -e 'name=="[0-9]*.alarms.xml"' rm
                    tw -i -d . -e 'name=="[0-9]*.stats.dds"' rm
                    tw -i -d . -e 'name=="[0-9]*.stats.done"' rm
                    tw -i -d . -e 'name=="*-inv-*.dds"' rm
                    tw -i -d . -e 'name=="*-LEVELS-*.dds"' rm
                    tw -i -d . -e 'name=="uniq.[0-9]*.stats.dds"' rm
                    tw -i -d . -e 'name=="email.alarms*.xml"' rm
                    rm -f inv-maps-uniq-*.dds
                    vg_main_compl_compress *.alarms.dds *.stats.dds inv*.dds
                )
                (
                    cd $indir || exit
                    tw -i -d . -e 'type==REG' rm
                    rmdir alarm stat 2> /dev/null
                )
                tw -i -H -d $datadir/$date -e 'name=="core"' rm
                print end complete processing $date $(date)
            )
            rm $datadir/$date/complete.stamp1
            ;;
        esac
    done
    for date in $dates; do
        [[ $date != ????/?? ]] && continue
        year=${date%%/*}
        rest=${date#*/}
        month=${rest%%/*}
        if [[ -d $datadir/$year/$month/processed ]] then
            (
                print begin complete processing $year/$month $(date)
                (
                    cd $datadir/$year/$month/processed/avg/total || exit
                    [[ ! -f avg.done ]] && \
                    WAITLOCK=y $SHELL vg_stat_prof monthly
                    rm -f log _stat*
                    vg_main_compl_compress mean.*.stats.dds sdev.*.stats.dds
                )
                (
                    cd $datadir/$year/$month/processed/total || exit
                    WAITLOCK=y $SHELL vg_stat monthly
                    WAITLOCK=y $SHELL vg_alarm monthly
                    rm -f log _stat*
                    rm -f uniq.[0-9]*.stats.dds uniq.[0-9]*.alarms.dds
                    vg_main_compl_compress *.alarms.dds *.stats.dds inv*.dds
                )
                tw -i -H -d $datadir/$year/$month -e 'name=="core"' rm
                print end complete processing $year/$month $(date)
            )
        fi
    done
    for date in $dates; do
        [[ $date != ???? ]] && continue
        year=${date%%/*}
        if [[ -d $datadir/$year/processed ]] then
            (
                print begin complete processing $year $(date)
                (
                    cd $datadir/$year/processed/avg/total || exit
                    [[ ! -f avg.done ]] && WAITLOCK=y $SHELL vg_stat_prof yearly
                    rm -f log _stat*
                    vg_main_compl_compress mean.*.stats.dds sdev.*.stats.dds
                )
                (
                    cd $datadir/$year/processed/total || exit
                    WAITLOCK=y $SHELL vg_stat yearly
                    WAITLOCK=y $SHELL vg_alarm yearly
                    rm -f log _stat*
                    rm -f uniq.[0-9]*.stats.dds uniq.[0-9]*.alarms.dds
                    vg_main_compl_compress *.alarms.dds *.stats.dds inv*.dds
                )
                tw -i -H -d $datadir/$year -e 'name=="core"' rm
                print end complete processing $year $(date)
            )
        fi
    done

    typeset +n gv
}
