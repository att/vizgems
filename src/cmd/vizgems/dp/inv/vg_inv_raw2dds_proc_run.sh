function vg_inv_raw2dds_proc_run {
    typeset -n gv=$1
    typeset datadir=$2
    typeset linkdir=$3
    shift 3
    typeset dates="$@"

    typeset args maxjobs today date year month day rest indir outdir
    typeset ifs invdir id n nn line ls file nre='?([-+])*([0-9])?(.)+([0-9])'
    typeset -l aid
    typeset -F p=3.14159265358979323846 lx ly x y s1 s2 rly ry

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

    invdir=$VGMAINDIR/dpfiles/inv

    args="-X accept -K -k"
    maxjobs=${gv.raw2ddsjobs}
    today=$(printf '%(%Y.%m.%d)T')
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
        if [[
            -f $datadir/$date/notcomplete.stamp &&
            -s $datadir/$date/processed/total/inv-cc-nd2cc.dds
        ]] then
            continue
        fi
        (
            exec 3< ${gv.lockfile}
            (
                print begin inv processing $date $(date)
                cd $outdir/total
                export VPATH=$PWD:$indir:../../../../../../../dpfiles
                export VPATH=$VPATH:../../../../../../../dpfiles/inv
                if [[
                    $invdir/HOSTING-LEVELS.txt -nt \
                    ../../input/HOSTING-LEVELS.txt
                ]] then
                    cp $invdir/HOSTING-LEVELS.txt ../../input
                fi
                if [[
                    $invdir/business.txt -nt ../../input/BUSINESS-inv.txt
                ]] then
                    ifs="$IFS"
                    IFS='|'
                    while read -r id n nn; do
                        [[ $id == '#'* ]] && continue
                        print -r "node|b|$id|name|$n"
                        print -r "node|b|$id|nodename|$nn"
                    done < $invdir/business.txt > ../../input/BUSINESS-inv.txt
                    IFS="$ifs"
                fi
                if [[
                    $invdir/location.txt -nt ../../input/LOCATIONS-inv.txt
                ]] then
                    ifs="$IFS"
                    IFS='|'
                    set -f
                    while read -r line; do
                        [[ $line == '#'* ]] && continue
                        case $line in
                        "node|"*"|coord|"$nre+( )$nre)
                            set -A ls -- ${line}
                            lx=${ls[4]%%' '*}; ly=${ls[4]##*' '}
                            x=$lx
                            (( rly = p * ly / 180.0 ))
                            (( s1 = 1.0 + sin(rly) ))
                            (( s2 = 1.0 - sin(rly) ))
                            if (( s2 == 0.0 )) then
                                y=$ly
                            else
                                (( ry = 0.5 * log(s1 / s2) ))
                                (( y = (ry * 180.0) / p ))
                            fi
                            print -r "node|${ls[1]}|${ls[2]}|${ls[3]}|$x $y"
                            print -r "node|${ls[1]}|${ls[2]}|llcoord|${ls[4]}"
                            ;;
                        "node|"*"|coord|na"*)
                            ls[4]=${ls[4]#na}
                            print -r "node|${ls[1]}|${ls[2]}|${ls[3]}|${ls[4]}"
                            print -r "node|${ls[1]}|${ls[2]}|llcoord|${ls[4]}"
                            ;;
                        *)
                            print -r "$line"
                            ;;
                        esac
                    done < $invdir/location.txt \
                    | sort -u > ../../input/LOCATIONS-inv.txt
                    set +f
                    IFS="$ifs"
                fi
                if [[
                    $invdir/customer.txt -nt ../../input/CUSTOMER-inv.txt
                ]] then
                    ifs="$IFS"
                    IFS='|'
                    while read -r id n nn; do
                        [[ $id == '#'* ]] && continue
                        print -r "node|c|$id|name|$n"
                        print -r "node|c|$id|nodename|$nn"
                    done < $invdir/customer.txt > ../../input/CUSTOMER-inv.txt
                    IFS="$ifs"
                fi
                if [[
                    $invdir/type.txt -nt ../../input/TYPE-inv.txt
                ]] then
                    ifs="$IFS"
                    IFS='|'
                    while read -r id n nn; do
                        [[ $id == '#'* ]] && continue
                        print -r "node|t|$id|name|$n"
                        print -r "node|t|$id|nodename|$nn"
                        print -r "node|t|$id|groupname|$n"
                    done < $invdir/type.txt > ../../input/TYPE-inv.txt
                    IFS="$ifs"
                fi
                for file in $invdir/view/*-inv.txt; do
                    [[ ! -f $file ]] && continue
                    if [[ $file -nt ../../input/${file##*/} ]] then
                        print -u2 MESSAGE copying $file to $date
                        cp $file ../../input/${file##*/}
                    fi
                done
                for file in $invdir/scope/*-inv.txt; do
                    [[ ! -f $file ]] && continue
                    if [[ $file -nt ../../input/${file##*/} ]] then
                        print -u2 MESSAGE copying $file to $date
                        case $file in
                        */auto-*)
                            sed 's/|aid=.*$//' $file > ../../input/${file##*/}
                            ;;
                        *)
                            cp $file ../../input/${file##*/}
                            ;;
                        esac
                    fi
                done
                for file in ../../input/*-inv.txt; do
                    [[ ! -f $file ]] && continue
                    [[ -f $invdir/view/${file##*/} ]] && continue
                    [[ -f $invdir/scope/${file##*/} ]] && continue
                    [[ $file == */BUSINESS-inv.txt ]] && continue
                    [[ $file == */LOCATIONS-inv.txt ]] && continue
                    [[ $file == */CUSTOMER-inv.txt ]] && continue
                    [[ $file == */TYPE-inv.txt ]] && continue
                    [[ $file == */KEEP*-inv.txt ]] && continue
                    if [[ -s $file ]] then
                        print -u2 MESSAGE zeroing $file in $date
                        > $file
                    else
                        print -u2 MESSAGE deleting $file from $date
                        rm $file
                    fi
                done
                export COSHELL_OPTIONS=cross
                nmake $args -f vg_inv_process.mk -j $maxjobs all
                print end inv processing $date $(date)
            ) 2>&1
            if [[ $date == ${today//.//} ]] then
                if [[ ! $datadir/$date -ef $datadir/latest ]] then
                    rm -f $datadir/latest
                    ln -s $datadir/$date $datadir/latest
                fi
            fi
        )
    done
    if [[ ! -d $datadir/${today//.//} ]] then
        sdpensuredualdirs ${today//.//} $datadir $linkdir
    fi

    typeset +n gv
}
