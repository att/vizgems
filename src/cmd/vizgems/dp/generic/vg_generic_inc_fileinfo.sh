
function vg_generic_inc_fileinfo {
    typeset -n gv=$1
    typeset -n ov=$2
    typeset inname=$3

    typeset date name t1 t2 undo

    ${spec.fileinfo} $1 $2 $3
    if [[ ${ov.date} == ERROR || ${ov.outname} == ERROR ]] then
        print -u2 SWIFT ERROR unknown file $inname
        return
    fi

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
