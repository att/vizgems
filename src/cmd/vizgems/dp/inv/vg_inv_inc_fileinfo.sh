function vg_inv_inc_fileinfo {
    typeset -n gv=$1
    typeset -n ov=$2
    typeset inname=$3

    typeset tim undo t1 t2

    case $inname in
    *-LEVELS*.txt)
        tim=${inname%.txt}
        tim=${tim%*-LEVELS}
        tim=${tim#.*}
        ov.outname=${inname%%.*}.txt
        if [[ $tim == '' ]] then
            ov.date=$(printf '%(%Y.%m.%d)T')
        else
            ov.date=${tim:0:4}.${tim:5:2}.${tim:8:2}
            ov.outname=KEEP${inname:11}
        fi
        ;;
    *-inv*.txt)
        tim=${inname%.txt}
        tim=${tim%*-inv}
        tim=${tim%.*}
        ov.outname=KEEP$inname
        if [[ $tim == '' ]] then
            ov.date=$(printf '%(%Y.%m.%d)T')
        else
            ov.date=${tim:0:4}.${tim:5:2}.${tim:8:2}
            ov.outname=KEEP${inname:11}
        fi
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
