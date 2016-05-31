function vg_generic_raw2dds_proc_run {
    typeset -n gv=$1
    typeset datadir=$2
    typeset linkdir=$3
    shift 3
    typeset dates="$@"

    typeset maxjobs date year month day rest indir outdir
    typeset ifile ofile ymd i j n doall

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
            (
                print begin generic:${spec.name} processing $date $(date)
                cd $outdir/total
                mkdir -p $indir/generic
                export GENERICSAVEDIR=$indir/generic

                doall=n
                n=0
                for i in ../../input/*.${spec.name}; do
                    [[ ! -f $i ]] && continue
                    j=${i##*/}
                    set -x
                    vg_generic one $i $j &
                    set +x
                    doall=y
                    sdpjobcntl $maxjobs
                done
                wait
                for i in [0-9]*.${spec.name}; do
                    if [[ -f $i ]] then
                        doall=y
                        break
                    fi
                done
                if [[ $doall == y ]] then
                    set -x
                    vg_generic all
                    set +x
                fi


                for ifile in *.${spec.name}.fwd.*; do
                    [[ ! -f $ifile ]] && continue
                    ymd=${ifile##*.fwd.}
                    ofile=${ifile%.fwd.*}
                    ofile=${date//\//.}.${ofile}
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
                print end generic:${spec.name} processing $date $(date)
            ) 2>&1
        )
    done

    typeset +n gv
}
