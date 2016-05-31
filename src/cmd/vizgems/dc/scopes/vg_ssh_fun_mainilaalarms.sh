ilaalarms=(key=; typeset -A vars; alarmn=0; typeset -a alarms)

function vg_ssh_fun_mainilaalarms_init {
    return 0
}

function vg_ssh_fun_mainilaalarms_term {
    return 0
}

function vg_ssh_fun_mainilaalarms_add {
    typeset var=${as[var]} inst=${as[inst]} key=${as[key]} dir=${as[dir]}
    typeset rival=${as[realalarminterval]}
    ilaalarms.dir=$dir
    ilaalarms.rival=$rival
    ilaalarms.vars[$var.$inst]=(
        name=$name
        type=$type
        var=$var
        inst=$inst
    )
    return 0
}

function vg_ssh_fun_mainilaalarms_run {
    typeset userhost=$1
    typeset i line rest key val dir pt ct

    if [[ ${ilaalarms.rival} != '' ]] then
        ct=$VG_JOBTS
        [[ -f ./ila_alarms.pt ]] && . ./ila_alarms.pt
        [[ $pt == '' ]] && pt=0
        if (( ct + 5 - pt < ${ilaalarms.rival} )) then
            return 0
        fi
        print "pt=$ct" > ./ila_alarms.pt
    fi

    dir=${ilaalarms.dir:-vg}
    > got.alarms

    for (( i = 0; i < 2; i++ )) do
        if vgscp $userhost:$dir/alarms ila_alarms.txt; then
            if vgscp got.alarms $userhost:$dir/got.alarms; then
                break
            fi
        else
            > ila_alarms.txt
        fi
    done

    while read -r line; do
        typeset -A vals
        rest=${line##+([$' \t'])}; rest=${rest%%+([$' \t'])}
        while [[ $rest == *=* ]] do
            key=${rest%%=*}; rest=${rest#"$key"=}; key=${key//[!a-zA-Z0-9_]/_}
            if [[ ${rest:0:1} == \' ]] then
                rest=${rest#\'}
                val=${rest%%\'*}; rest=${rest##"$val"?(\')}
            elif [[ ${rest:0:1} == \" ]] then
                rest=${rest#\"}
                val=${rest%%\"*}; rest=${rest##"$val"?(\")}
            else
                val=${rest%%[$' \t']*}; rest=${rest##"$val"?([$' \t'])}
            fi
            rest=${rest##+([$' \t'])}
            [[ $key == '' || $key == [0-9]* ]] && continue
            vals[$key]=$val
        done
        ilaalarms.alarms[${ilaalarms.alarmn}]=(
            alarmid=${vals[alarmid]} sev=${vals[sev]} type=${vals[type]}
            cond=${vals[cond]} tech=${vals[tech]} txt=${vals[txt]}
            lv1=${vals[lv1]} id1=${vals[id1]}
        )
        (( ilaalarms.alarmn++ ))
        unset vals
    done < ila_alarms.txt

    return 0
}

function vg_ssh_fun_mainilaalarms_emit {
    typeset alarmi

    for (( alarmi = 0; alarmi < ilaalarms.alarmn; alarmi++ )) do
        typeset -n alarmr=ilaalarms.alarms[$alarmi]
        typeset -n vref=vars._$varn
        (( varn++ ))
        vref.rt=ALARM
        vref.alarmid=${alarmr.cond}
        vref.sev=${alarmr.sev}
        vref.type=${alarmr.type}
        vref.cond=${alarmr.cond}
        vref.tech=${alarmr.tech}
        vref.txt=" ${alarmr.txt}"
        [[ ${alarmr.lv1} != '' ]] && vref.lv1=${alarmr.lv1}
        [[ ${alarmr.id1} != '' ]] && vref.id1=${alarmr.id1}
    done
    typeset -n vref=vars._$varn
    (( varn++ ))
    vref.rt=STAT
    vref.name=alarms
    vref.type=number
    vref.num=1
    vref.unit=
    vref.label=
    return 0
}

function vg_ssh_fun_mainilaalarms_invrun {
    return 0
}

if [[ $SWIFTWARNLEVEL != "" ]] then
    typeset -ft vg_ssh_fun_mainilaalarms_init vg_ssh_fun_mainilaalarms_term
    typeset -ft vg_ssh_fun_mainilaalarms_add vg_ssh_fun_mainilaalarms_run
    typeset -ft vg_ssh_fun_mainilaalarms_emit vg_ssh_fun_mainilaalarms_invrun
fi
