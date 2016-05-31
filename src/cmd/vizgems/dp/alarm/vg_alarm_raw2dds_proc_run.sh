function vg_alarm_raw2dds_proc_run {
    typeset -n gv=$1
    typeset datadir=$2
    typeset linkdir=$3
    shift 3
    typeset dates="$@"

    typeset args maxjobs date year month day rest indir outdir
    typeset thisdate prevdate poutdir noutdir ifile ofile ymd i j n doall

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
        thisdate=$(printf '%(%#)T' ${year}.${month}.${day}-04:00:00)
        prevdate=$(printf '%(%Y/%m/%d)T' \#$(( $thisdate - 24 * 60 * 60 )))
        poutdir=$datadir/$prevdate/processed
        (
            exec 3< ${gv.lockfile}
            (
                print begin alarm processing $date $(date)
                cd $outdir/total
                mkdir -p $indir/alarm
                export ALARMSAVEDIR=$indir/alarm
                if [[ -f $poutdir/total/open.alarms.dds ]] then
                    export PREVOPENALARMS=$poutdir/total/open.alarms.dds
                else
                    export PREVOPENALARMS=
                fi
                if [[
                    ! -f sev.list ||
                    $VGMAINDIR/dpfiles/alarm/sevlist.txt -nt sev.list
                ]] then
                    set -x
                    vg_alarm sl $VGMAINDIR/dpfiles/alarm/sevlist.txt sev.list
                    set +x
                fi
                if [[
                    ! -f objecttrans.dds || ! -f objecttrans-rev.dds ||
                    $VGMAINDIR/dpfiles/objecttrans.txt -nt objecttrans.dds
                ]] then
                    set -x
                    vg_trans \
                        $VGMAINDIR/dpfiles/objecttrans.txt \
                    objecttrans.dds objecttrans-rev.dds
                    set +x
                fi
                if [[ -f sev.list ]] then
                    doall=n
                    n=0
                    for i in ../../input/*.alarms.@(xml|txt|pptxt); do
                        [[ ! -f $i ]] && continue
                        j=${i##*/}
                        j=${j%.*}.done
                        [[ -f $j ]] && continue
                        set -x
                        vg_alarm ${i##*.} $i objecttrans.dds sev.list $j &
                        set +x
                        doall=y
                        sdpjobcntl $maxjobs
                        if ((
                            ALARMMAXFILERUN > 10 && ++n >= ALARMMAXFILERUN
                        )) then
                            print -u2 SWIFT WARNING exceeded alarm file limit $n
                            break
                        fi
                    done
                    for i in *.alarms.xml; do
                        [[ ! -f $i ]] && continue
                        j=${i##*/}
                        j=${j%.*}.done
                        [[ -f $j ]] && continue
                        set -x
                        vg_alarm ${i##*.} $i objecttrans.dds sev.list $j &
                        set +x
                        doall=y
                        sdpjobcntl $maxjobs
                        if ((
                            ALARMMAXFILERUN > 10 && ++n >= ALARMMAXFILERUN
                        )) then
                            print -u2 SWIFT WARNING exceeded alarm file limit $n
                            break
                        fi
                    done
                    wait
                    for i in *.alarms.dds; do
                        if [[ -f $i ]] then
                            doall=y
                            break
                        fi
                    done
                    set -x
                    [[ $doall == y ]] && vg_alarm all sev.list open.alarms.dds
                    set +x
                fi
                for ifile in *alarms*.fwd.*; do
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
                print end alarm processing $date $(date)
            ) 2>&1
        )
    done

    typeset +n gv
}
