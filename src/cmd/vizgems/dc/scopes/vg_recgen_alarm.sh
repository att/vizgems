
function vg_recgen_alarm { # $1: rec, $2: alarms
    typeset rec=$1 as=$2

    typeset msg tm ti

    typeset -n rref=$rec
    eval typeset an=\${#${as}[@]}

    date=$(printf '%(%Y.%m.%d.%H.%M.%S)T' \#$VG_JOBTS) || return 1
    file=$VG_DSCOPESDIR/outgoing/alarm/alarms.$date.$VG_JOBID.$VG_SCOPENAME.xml
    {
        for (( ai = 0; ai < an; ai++ )) do
            typeset -n aref=${as}[$ai]
            [[ ${aref} == '' ]] && continue
            msg="${aref.type} ${aref.tech} ${aref.cond}${aref.txt}"
            msg=${msg//'<'/%3C}
            msg=${msg//'>'/%3E}
            ti=${aref.rti:-$VG_JOBTS}
            print -n "<alarm>"
            print -n "<v>${rref.version}</v>"
            print -n "<jid>$VG_JOBID</jid>"
            print -n "<lv1>${aref.lv1:-${rref.target.level}}</lv1>"
            print -n "<id1>${aref.id1:-${rref.target.name}}</id1>"
            print -n "<sid>${rref.scope.name}</sid>"
            if [[ ${rref.tid} != '' ]] then
                print -n "<origmsg>${rref.tid}</origmsg>"
            fi
            print -n "<ti>$ti</ti>"
            tm=${aref.tmode:-${rref.tmode:-keep}}
            print -n "<tm>${tm%%:*}</tm>"
            print -n "<aid>${aref.alarmid}</aid>"
            print -n "<sev>${aref.sev}</sev>"
            print -n "<tp>${aref.type}</tp>"
            print -n "<txt>"
            print -n "VG $msg"
            print -n "</txt>"
            print "</alarm>"
        done
    } > ./alarm.file.$VG_JOBID || return 1
    if [[ ! -s ./alarm.file.$VG_JOBID ]] then
        rm ./alarm.file.$VG_JOBID
        return 0
    fi
    cp ./alarm.file.$VG_JOBID $file.tmp && mv $file.tmp $file
}
