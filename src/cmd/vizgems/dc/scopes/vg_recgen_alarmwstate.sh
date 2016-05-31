
function vg_recgen_alarmwstate { # $1: mode, $2: rec, $3: ai, $4: pi, $5: txt
    typeset mode=$1 rec=$2 astr=$3 estr=$4 txt=$5

    typeset -n asrref=$rec
    typeset -n asaref=$rec.$astr
    typeset -n aseref=$rec.$astr.$estr

    typeset ts=$VG_JOBTS

    typeset alarmid tmode rtmode cond sev min per max fival msg
    typeset p_ts p_open p_sev p_states p_maxstates p_per p_msg p_min p_fival
    typeset open sts count action sevi sevj stsi counti mini
    typeset havep date file aid id1

    alarmid=${asaref.alarmid}
    [[ ${aseref.alarmid} != '' ]] && alarmid=${aseref.alarmid}
    tmode=${asrref.tmode}
    [[ ${asaref.tmode} != '' ]] && tmode=${asaref.tmode}
    [[ ${aseref.tmode} != '' ]] && tmode=${aseref.tmode}
    if [[ $tmode == *:* ]] then
        rtmode=${tmode#*:}
        tmode=${tmode%%:*}
    else
        rtmode=$tmode
    fi
    cond=${asaref.cond}
    [[ ${aseref.cond} != '' ]] && cond=${aseref.cond}
    sev=${asaref.sev}
    [[ ${aseref.sev} != '' ]] && sev=${aseref.sev}
    min=${asaref.min.count}
    [[ ${aseref.min.count} != '' ]] && min=${aseref.min.count}
    per=${asaref.min.per}
    [[ ${aseref.min.per} != '' ]] && per=${aseref.min.per}
    max=${asaref.max.count}
    [[ ${aseref.max.count} != '' ]] && max=${aseref.max.count}
    fival=${asaref.max.ival}
    [[ ${aseref.max.ival} != '' ]] && fival=${aseref.max.ival}
    msg="$cond$txt"

    [[ $min != +([0-9]) ]] && min=1
    [[ $per == '' ]] && per=$min
    [[ $max != +([0-9]) ]] && max=1
    if (( max > 1 )) then
        (( fival = fival / max ))
        max=1
    elif (( max == 0 )) then
        (( fival = 0 ))
        max=1
    fi
    (( sev < 1 || sev > 5 )) && sev=5

    havep=n
    if [[ -f ./alarmstate.state.$alarmid ]] then
        . ./alarmstate.state.$alarmid || {
            rm -f ./alarmstate.state.$alarmid
            return 1
        }
        havep=y
    elif [[ $mode == clear ]] then
        return 0
    fi
    [[ $p_ts == '' ]] && p_ts=$ts
    [[ $p_open == '' ]] && p_open=n
    [[ $p_sev == '' ]] && p_sev=5
    [[ $p_maxstates == '' ]] && p_maxstates=${#p_states}

    if (( per > p_maxstates )) then
        p_maxstates=$per
    fi
    p_per[$sev]=$per
    if [[ $mode == alarm ]] then
        p_msg[$sev]="$msg"
        p_min[$sev]=$min
        p_fival[$sev]=$fival
    else
        p_msg[0]="$msg"
        p_min[0]=$min
    fi

    case $mode in
    alarm) p_states="$sev$p_states" ;;
    clear) p_states="0$p_states" ;;
    esac
    p_states=${p_states:0:$p_maxstates}
    open=$p_open

    action=noop
    case $mode in
    alarm)
        sts=${p_states:0:$per}
        sts=${sts//0/}
        count=${#sts}

        if (( count > 0 )) then
            sevj=1
            for sevi in 1 2 3 4; do
                stsi=${p_states:0:${p_per[$sevi]:-$p_maxstates}}
                stsi=${stsi//[$sevj]/$sevi}
                stsi=${stsi//[!$sevi]/}
                counti=${#stsi}
                mini=${p_min[$sevi]}
                sevj=$sevj$sevi
                [[ $mini == '' ]] && continue
                if (( counti >= mini )) then
                    if [[ $p_open != y ]] then
                        action=send
                        p_ts=$ts
                        sev=$sevi
                        msg=${p_msg[$sevi]}
                    else
                        if (( sevi < p_sev )) then
                            action=send
                            p_ts=$ts
                            sev=$sevi
                            msg=${p_msg[$sevi]}
                        fi
                    fi
                    break
                fi
            done
        fi
        if [[ $action != send && $p_open == y ]] then
            if (( p_fival[$p_sev] != 0 && ts - p_ts >= p_fival[$p_sev] )) then
                action=send
                p_ts=$ts
                case $p_states in
                *1*) sev=1 ;;
                *2*) sev=2 ;;
                *3*) sev=3 ;;
                *4*) sev=4 ;;
                *) sev=5 ;;
                esac
                msg="${p_msg[$sev]} (repeat)"
                tmode=$rtmode
            fi
        fi
        ;;
    clear)
        sts=${p_states:0:$per}
        sts=${sts//[1-9]/}
        count=${#sts}
        if (( count >= min )) then
            if [[ $p_open == y ]] then
                action=send
            fi
        fi
        ;;
    esac

    if [[ $action == send ]] then
        date=$(printf '%(%Y.%m.%d.%H.%M.%S)T' \#$VG_JOBTS) || return 1
        file=$VG_DSCOPESDIR/outgoing/alarm
        file+=/alarms.$date.$alarmid.$VG_JOBID.$VG_SCOPENAME.xml
        if [[ $alarmid == *._id_* ]] then
            id1=${alarmid##*._id_}
            aid=${alarmid%._id_*}
        else
            id1=
            aid=$alarmid
        fi
        case $mode in
        alarm)
            open=y
            {
                msg=${msg//'<'/%3C}
                msg=${msg//'>'/%3E}
                print -n "<alarm>"
                print -n "<v>${asrref.version}</v>"
                print -n "<jid>$VG_JOBID</jid>"
                print -n "<lv1>${asrref.target.level}</lv1>"
                print -n "<id1>${id1:-${asrref.target.name}}</id1>"
                print -n "<sid>${asrref.scope.name}</sid>"
                if [[ ${asrref.tid} != '' ]] then
                    print -n "<origmsg>${asrref.tid}</origmsg>"
                fi
                print -n "<ti>$ts</ti>"
                print -n "<tm>$tmode</tm>"
                print -n "<aid>$aid</aid>"
                print -n "<sev>$sev</sev>"
                print -n "<tp>ALARM</tp>"
                print -n "<txt>VG ALARM $msg</txt>"
                print "</alarm>"
            } > ./alarmstate.file.$alarmid || return 1
            cp ./alarmstate.file.$alarmid $file.tmp && mv $file.tmp $file
            ;;
        clear)
            open=n
            {
                msg=${msg//'<'/%3C}
                msg=${msg//'>'/%3E}
                print -n "<alarm>"
                print -n "<v>${asrref.version}</v>"
                print -n "<jid>$VG_JOBID</jid>"
                print -n "<lv1>${asrref.target.level}</lv1>"
                print -n "<id1>${id1:-${asrref.target.name}}</id1>"
                print -n "<sid>${asrref.scope.name}</sid>"
                if [[ ${asrref.tid} != '' ]] then
                    print -n "<origmsg>${asrref.tid}</origmsg>"
                fi
                print -n "<ti>$ts</ti>"
                print -n "<tm>$tmode</tm>"
                print -n "<aid>$aid</aid>"
                print -n "<sev>$sev</sev>"
                print -n "<tp>CLEAR</tp>"
                print -n "<txt>VG CLEAR $msg</txt>"
                print "</alarm>"
            } > ./alarmstate.file.$alarmid || return 1
            cp ./alarmstate.file.$alarmid $file.tmp && mv $file.tmp $file
            ;;
        esac
    fi

    sts=${p_states//0/}
    count=${#sts}
    if (( count > 0 )) then
        {
            print "p_open=$open"
            print "p_states=$p_states"
            print "p_maxstates=$p_maxstates"
            [[ $action == send ]] && p_sev=$sev
            print "p_sev=$p_sev"
            print "p_ts=$p_ts"
            for sevi in 0 1 2 3 4; do
                print "p_msg[$sevi]='${p_msg[$sevi]}'"
                print "p_min[$sevi]=${p_min[$sevi]}"
                print "p_fival[$sevi]=${p_fival[$sevi]}"
            done
        } > ./alarmstate.state.$alarmid.tmp \
        && mv ./alarmstate.state.$alarmid.tmp ./alarmstate.state.$alarmid
    else
        if [[ $havep == y ]] then
            rm -f ./alarmstate.state.$alarmid ./alarmstate.file.$alarmid
        fi
    fi
}
