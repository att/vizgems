function vg_main_clean_run {
    typeset -n gv=$1
    typeset datadir=$2
    typeset linkdir=$3
    shift 3
    typeset dates="$@"

    typeset date year month day rest indir outdir

    for date in $dates; do
        sdpruncheck ${gv.exitfile} || break
        sdpspacecheck $datadir ${gv.mainspace} 2> /dev/null && break
        if [[ $linkdir != '' ]] then
            if ! sdpspacecheck $linkdir ${gv.permspace}; then
                continue
            fi
        fi
        year=${date%%/*}
        rest=${date#*/}
        month=${rest%%/*}
        rest=${rest#*/}
        day=${rest%%/*}
        [[ ! -d $datadir/$date ]] && continue
        indir=$datadir/$date/input
        outdir=$datadir/$date/processed
        case $linkdir in
        "")
            (
                print begin remove of data for $date $(date)
                mv $datadir/$date $datadir/$date.tmp
                if [[ $VG_NFSMODE == client && $date == ????/??/?? ]] then
                    ln -s $VG_DASYSTEMDIR/data/main/$date $datadir/$date
                fi
                sleep 10
                rm -rf $datadir/$date.tmp
                print end remove of data for $date $(date)
            ) 2> /dev/null
            ;;
        *)
            (
                (
                    cd $outdir/total || exit
                    rm -f *.tmp
                    rm -f log *.m[so]
                )
                (
                    cd $outdir/avg/total || exit
                    rm -f *.tmp
                    rm -f log *.m[so]
                )
                tw -i -H -d $datadir/$date -e 'name=="core"' rm
                if [[ ! -L $linkdir/$date && -d $linkdir/$date ]] then
                    if [[ -d $datadir/$date ]] then
                        mv $datadir/$date $datadir/$date.tmp
                        ln -s $linkdir/$date $datadir/$date
                        rm -rf $datadir/$date.tmp
                    else
                        print -u2 SWIFT ERROR directory exits $linkdir/$date
                    fi
                    continue
                fi
                rm -rf $linkdir/$date.tmp
                mkdir -p $linkdir/$date.tmp
                (
                    cd $datadir/$date &&
                    pax -rwv -o 'preserve=e' . $linkdir/$date.tmp
                ) 2> /dev/null
                if [[ $? != 0 ]] then
                    print -u2 SWIFT ERROR failed to migrate $datadir/$date
                    rm -rf $linkdir/$date.tmp
                    continue
                fi
                rm -f $linkdir/$date
                mv $linkdir/$date.tmp $linkdir/$date
                mv $datadir/$date $datadir/$date.tmp
                ln -s $linkdir/$date $datadir/$date
                rm -rf $datadir/$date.tmp
            )
            ;;
        esac
    done

    typeset +n gv
}
