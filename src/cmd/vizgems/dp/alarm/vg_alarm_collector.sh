function vg_alarm_collector {
    typeset -n gv=$1

    typeset file noticket

    if [[ $SWIFTWARNLEVEL != '' ]] then
        set -x
    fi

    if [[ $VGMAINDIR == "" || ! -d $VGMAINDIR/incoming/runigems ]] then
        print -u2 SWIFT ERROR dir: $VGMAINDIR/incoming/runigems not found
        return
    fi
    if [[ $VGMAINDIR == "" || ! -d $VGMAINDIR/incoming/alarm ]] then
        print -u2 SWIFT ERROR dir: $VGMAINDIR/incoming/alarm not found
        return
    fi
    cd $VGMAINDIR/incoming/alarm
    for file in $VGMAINDIR/incoming/runigems/correlation_file-[0-9]*; do
        if [[ ! -f $file || $file == *.tmp ]] then
            continue
        fi
        print -u2 SWIFT MESSAGE incoming alarm file $file
        ts=${file##*-}
        ts=${ts:0:4}.${ts:4:2}.${ts:6:2}.${ts:8:2}.${ts:10:2}.${ts:12}
        if ! cp $file alarms.$ts.tmp; then
            print -u2 SWIFT ERROR cannot copy file $file
            return
        fi
        if ! mv alarms.$ts.tmp alarms.$ts; then
            print -u2 SWIFT ERROR cannot rename file alarms.$ts.tmp
            return
        fi
        if ! rm -f $file; then
            print -u2 SWIFT ERROR cannot remove file $file
        fi
    done
    if [[ ! -d $VGMAINDIR/outgoing/runigems ]] then
        print -u2 SWIFT ERROR dir: $VGMAINDIR/outgoing/runigems not found
        return
    fi
    if [[ ! -d $VGMAINDIR/outgoing/ticket ]] then
        print -u2 SWIFT ERROR dir: $VGMAINDIR/outgoing/ticket not found
        return
    fi
    if [[ -f $VGMAINDIR/outgoing/noticket ]] then
        noticket=y
        print -u2 "SWIFT MESSAGE noticket mode - deleting ticket files"
    fi
    for file in $VGMAINDIR/outgoing/ticket/tickets.[0-9]*; do
        if [[ ! -f $file || $file == *.tmp ]] then
            continue
        fi
        if [[ $noticket == y ]] then
            rm -f $file
            continue
        fi
        print -u2 SWIFT WARNING outgoing ticket file $file
        if ! cat $file >> $VGMAINDIR/outgoing/runigems/swift_tickets.log; then
            print -u2 -n "SWIFT ERROR cannot write to runigems log"
            print -u2 " $VGMAINDIR/outgoing/runigems/swift_tickets.log"
            break
        fi
        rm $file
    done

    typeset +n gv
}
