sar=()
typeset -A sarns sarus sarls saris sarvs sarcs

function vg_ssh_fun_mainsar_init {
    sar.varn=0
    sar.count=1
    sar.round=0
    sar.cpuvi=
    case $targettype in
    *linux*)
        ;;
    *solaris*)
        sarns[memory_free]=freemem; sarus[memory_free]=B
        sarns[memory_total]=Memory; sarus[memory_total]=MB
        ;;
    esac
    sarls[memory_total]='Total Memory'
    sarls[memory_free]='Free Memory'
    sarls[cpu_free]=''
    sarls[cpu_sys]='CPU System'
    sarls[cpu_usr]='CPU User'
    sarls[cpu_wait]='CPU I/O Wait'
    return 0
}

function vg_ssh_fun_mainsar_term {
    return 0
}

function vg_ssh_fun_mainsar_add {
    typeset var=${as[var]} inst=${as[inst]} count=${as[count]}

    if (( count > sar.count )) then
        sar.count=$count
    fi

    typeset -n sarr=sar._${sar.varn}
    sarr.name=$name
    sarr.unit=${sarus[$var]:-$unit}
    sarr.type=$type
    sarr.var=$var
    sarr.inst=$inst
    (( sar.varn++ ))
    return 0
}

function vg_ssh_fun_mainsar_send {
    typeset cmd

    case $targettype in
    *linux*)
        ;;
    *solaris*)
        cmd="(/usr/sbin/prtconf | egrep 'Memory size';"
        cmd+="sar -r 1 $(( ${sar.count} + 1 )) | sed 1,3d)"
        ;;
    esac
    print -r "$cmd"
    return 0
}

function vg_ssh_fun_mainsar_receive {
    typeset vi

    set -f
    set -A vs -- $1
    set +f

    case $targettype in
    *solaris*)
        if [[ $1 == *Memory*size:* ]] then
            (( sarvs[memory_total._total] = ${vs[2]} ))
            return 0
        fi

        if [[ $1 == *free* ]] then
            if (( ${#saris[@]} == 0 )) then
                for (( vi = 1; vi < ${#vs[@]}; vi++ )) do
                    for ni in "${!sarns[@]}"; do
                        if [[ ${sarns[$ni]} == ${vs[$vi]} ]] then
                            saris[$vi]=$ni
                            break
                        fi
                    done
                done
            fi
            return 0
        fi

        [[ $1 != [0-9][0-9]:* ]] && return 0

        (( sar.round++ ))
        (( sar.round == 1 )) && return 0

        for (( vi = 0; vi < ${#vs[@]}; vi++ )) do
            [[ ${saris[$vi]} == '' ]] && continue
            (( sarvs[${saris[$vi]}._total] += (vs[$vi] * 8192) ))
        done
        ;;
    esac
    return 0
}

function vg_ssh_fun_mainsar_emit {
    typeset nicpu cpu ni sari
    typeset -A totalvs
    typeset -F3 v

    case $targettype in
    *solaris*)
        for ni in "${!sarvs[@]}"; do
            [[ $ni != memory_free._total ]] && continue
            (( sarvs[$ni] /= (${sar.round}.0 - 1) ))
        done
        if ((
            sarvs[memory_total._total] * 1024 * 1024 -
            sarvs[memory_free._total] < 0.0
        )) then
            sarvs[memory_free._total]=
        fi
        for (( sari = 0; sari < sar.varn; sari++ )) do
            typeset -n sarr=sar._$sari
            [[ ${sarvs[${sarr.var}._total]} == '' ]] && continue
            typeset -n vref=vars._$varn
            (( varn++ ))
            vref.rt=STAT
            vref.name=${sarr.name}
            vref.type=${sarr.type}
            v=${sarvs[${sarr.var}.${sarr.inst}]}
            vref.num=$v
            vref.unit=${sarr.unit}
            vref.label=${sarls[${sarr.var}]}
        done
        ;;
    esac
    return 0
}

function vg_ssh_fun_mainsar_invsend {
    typeset cmd

    case $targettype in
    *linux*)
        ;;
    *solaris*)
        cmd="/usr/sbin/prtconf | egrep 'Memory size'"
        ;;
    esac
    print -r "$cmd"
    return 0
}

function vg_ssh_fun_mainsar_invreceive {
    typeset vi cpu v val
    typeset -u U

    set -f
    set -A vs -- $1
    set +f

    case $targettype in
    *solaris*)
        v=${vs[2]}
        U=${vs[3]:0:1}B
        vg_unitconv "$v" $U MB
        val=$vg_ucnum
        if [[ $val != '' ]] then
            print "node|o|$aid|si_sz_memory_used._total|$val"
        fi
        ;;
    esac
    return 0
}

if [[ $SWIFTWARNLEVEL != "" ]] then
    typeset -ft vg_ssh_fun_mainsar_init vg_ssh_fun_mainsar_term
    typeset -ft vg_ssh_fun_mainsar_add vg_ssh_fun_mainsar_send
    typeset -ft vg_ssh_fun_mainsar_receive vg_ssh_fun_mainsar_emit
    typeset -ft vg_ssh_fun_mainsar_invsend vg_ssh_fun_mainsar_invreceive
fi
