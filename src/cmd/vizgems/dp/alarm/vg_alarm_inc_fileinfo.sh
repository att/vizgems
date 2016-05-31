function vg_alarm_inc_fileinfo {
    typeset -n gv=$1
    typeset -n ov=$2
    typeset inname=$3

    typeset tim undo t1 t2

    case $inname in
    alarms.[0-9]*.xml)
        tim=${inname#alarms.}
        tim=${tim%.xml}
        ov.date=${tim:0:10}
        ov.outname=${tim:11}.alarms.xml
        ;;
    alarms.[0-9]*.[0-9]*.[0-9]*.[0-9]*.[0-9]*)
        tim=${inname#alarms.}
        ov.date=${tim:0:10}
        ov.outname=${tim:11}.alarms.txt
        ;;
    ppalarms.[0-9]*.[0-9]*.[0-9]*.[0-9]*.[0-9]*)
        tim=${inname#ppalarms.}
        ov.date=${tim:0:10}
        ov.outname=${tim:11}.alarms.pptxt
        ;;
    esac

    if [[ -f $VGMAINDIR/data/main/${ov.date//.//}/complete.stamp ]] then
        undo=n
        t1=${ printf '%(%#)T' "${ov.date}-00:00:00"; }
        t2=${ printf '%(%#)T'; }
        if [[ $ACCEPTOLDINCOMING == +([0-9])d ]] then
            if (( t2 - t1 < ${ACCEPTOLDINCOMING%d} * 24 * 60 * 60 )) then
                undo=y
            fi
        elif [[ $ACCEPTOLDINCOMING == +([0-9])h ]] then
            if (( t2 - t1 < ${ACCEPTOLDINCOMING%h} * 60 * 60 )) then
                undo=y
            fi
        fi
        if [[ $undo == y ]] then
            touch $VGMAINDIR/data/main/${ov.date//.//}/notcomplete.stamp
            rm -f $VGMAINDIR/data/main/${ov.date//.//}/complete.stamp*
            rm -f $VGMAINDIR/data/main/${ov.date//.//}/../complete.stamp*
            rm -f $VGMAINDIR/data/main/${ov.date//.//}/../../complete.stamp*
        fi
    fi

    typeset +n gv
    typeset +n ov
}
