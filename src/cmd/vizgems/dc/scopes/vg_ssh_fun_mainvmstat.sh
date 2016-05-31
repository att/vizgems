vmstat=()
typeset -A vmstatns vmstatus vmstatls vmstatis vmstatvs hpux_psize

function vg_ssh_fun_mainvmstat_init {
    vmstat.varn=0
    vmstat.count=4
    vmstat.round=0
    case $targettype in
    *linux*)
        vmstatns[os_swapin]=si;  vmstatus[os_swapin]=KBps
        vmstatns[os_swapout]=so; vmstatus[os_swapout]=KBps
        vmstatns[os_pagein]=bi;  vmstatus[os_pagein]=KBps
        vmstatns[os_pageout]=bo; vmstatus[os_pageout]=KBps
        vmstatns[os_runqueue]=r; vmstatus[os_runqueue]=''
        vmstatns[cpu_free]=id;   vmstatus[cpu_free]='%'
        vmstatns[cpu_sys]=sy;    vmstatus[cpu_sys]='%'
        vmstatns[cpu_usr]=us;    vmstatus[cpu_usr]='%'
        vmstatns[cpu_wait]=wa;   vmstatus[cpu_wait]='%'
        ;;
    *solaris*)
        vmstatns[os_swapin]=si;  vmstatus[os_swapin]=KBps
        vmstatns[os_swapout]=so; vmstatus[os_swapout]=KBps
        vmstatns[os_pagein]=pi;  vmstatus[os_pagein]=KBps
        vmstatns[os_pageout]=po; vmstatus[os_pageout]=KBps
        vmstatns[os_runqueue]=r; vmstatus[os_runqueue]=''
        ;;
    esac
    vmstatls[os_swapin]='Swap In'
    vmstatls[os_swapout]='Swap Out'
    vmstatls[os_pagein]='Pages In'
    vmstatls[os_pageout]='Pages Out'
    vmstatls[os_runqueue]='Run Queue'
    vmstatls[cpu_free]=''
    vmstatls[cpu_sys]='CPU System (All CPUs)'
    vmstatls[cpu_usr]='CPU User (All CPUs)'
    vmstatls[cpu_wait]='CPU I/O Wait (All CPUs)'
    vmstatls[memory_free]=''
    vmstatls[memory_total]=''
    return 0
}

function vg_ssh_fun_mainvmstat_term {
    return 0
}

function vg_ssh_fun_mainvmstat_add {
    typeset var=${as[var]} inst=${as[inst]} count=${as[count]}

    if (( count > vmstat.count )) then
        vmstat.count=$count
    fi

    typeset -n vmstatr=vmstat._${vmstat.varn}
    vmstatr.name=$name
    vmstatr.unit=${vmstatus[$var]:-$unit}
    vmstatr.type=$type
    vmstatr.var=$var
    vmstatr.inst=$inst
    (( vmstat.varn++ ))
    return 0
}

function vg_ssh_fun_mainvmstat_send {
    typeset cmd

    case $targettype in
    *linux*)
        cmd="vmstat 1 $(( ${vmstat.count} + 1 ))"
        ;;
    *solaris*)
        cmd="vmstat -S 1 $(( ${vmstat.count} + 1 )) | sed 's/sy/ys/'"
        ;;
    esac
    cmd+="| sed 1d"
    print -r "$cmd"
    return 0
}

function vg_ssh_fun_mainvmstat_receive {
    typeset vi
    typeset v
    typeset -u U

    set -f
    set -A vs -- $1
    set +f

    # HPUX
    if [[ $targettype == *hpux* && $1 == *hpux_psize* ]] then
       v=${vs[1]}
       (( vmstatvs[memory_free._total] *= $v ))
       return 0
    fi

    if [[ $targettype == *hpux* && $1 == *Memory*size* ]] then
       v=${vs[2]}
       (( vmstatvs[memory_total._total] = $v ))
       return 0
    fi

    # AIX
    if [[ $1 == *'System configuration:'*mem* ]] then
        v=${vs[3]##*=}
        U=${v##+([0-9])}
        v=${v%?B}
        (( vmstatvs[memory_total._total] = $v ))
        (( vmstatus[memory_total._total] = $U ))
        return 0
    fi

    if [[ $1 == '' ]] then
        return 0
    fi

    if [[ $1 == *fre* ]] then
        if (( ${#vmstatis[@]} == 0 )) then
            for (( vi = 0; vi < ${#vs[@]}; vi++ )) do
                for ni in "${!vmstatns[@]}"; do
                    if [[ ${vmstatns[$ni]} == ${vs[$vi]} ]] then
                        vmstatis[$vi]=$ni
                        break
                    fi
                done
            done
        fi
        return 0
    fi

    if [[ $1 == *'State change'* ]] then
        vmstat.round=0
        return 0
    fi

    (( vmstat.round++ ))
    if (( vmstat.round < 2 )) then
        return 0
    fi

    for (( vi = 0; vi < ${#vs[@]}; vi++ )) do
        [[ ${vmstatis[$vi]} == '' ]] && continue
        (( vmstatvs[${vmstatis[$vi]}._total] += vs[$vi] ))
    done
    return 0
}

function vg_ssh_fun_mainvmstat_emit {
    typeset ni vmstati
    typeset -F3 v

    for ni in "${!vmstatvs[@]}"; do
        [[ $targettype == *aix* && $ni == memory_total* ]] && continue
        [[ $targettype == *hpux* && $ni == memory_total* ]] && continue
        (( vmstatvs[$ni] /= (${vmstat.round}.0 - 1.0) ))

        if [[ $targettype == *aix* && $ni == memory_free* ]] then
            (( vmstatvs[$ni] /= 256 ))
        fi

        if [[ $targettype == *hpux* && $ni == memory_free* ]] then
            (( vmstatvs[memory_free._total] /= 1048576 ))
        fi
    done

    for (( vmstati = 0; vmstati < vmstat.varn; vmstati++ )) do
        typeset -n vmstatr=vmstat._$vmstati
        [[ ${vmstatvs[${vmstatr.var}._total]} == '' ]] && continue
        typeset -n vref=vars._$varn
        (( varn++ ))
        vref.rt=STAT
        vref.name=${vmstatr.name}
        vref.type=${vmstatr.type}
        v=${vmstatvs[${vmstatr.var}.${vmstatr.inst}]}
        vref.num=$v
        vref.unit=${vmstatr.unit}
        vref.label=${vmstatls[${vmstatr.var}]}
    done
    return 0
}

function vg_ssh_fun_mainvmstat_invsend {
    typeset cmd

    case $targettype in
    *aix*)
        cmd="vmstat 1 1 | egrep '^Sys.*mem'"
        ;;
    *hpux*)
        cmd="./memsize"
        ;;
    *)
        cmd='echo'
        ;;
    esac
    print -r "$cmd"
    return 0
}

function vg_ssh_fun_mainvmstat_invreceive {
    typeset v val
    typeset -u U

    set -f
    set -A vs -- $1
    set +f
    if [[ $1 == *System*mem* ]] then
        v=${vs[3]##*=}
        U=${v##+([0-9])}
        v=${v%?B}
        vg_unitconv "$v" $U MB
        val=$vg_ucnum
        if [[ $val != '' ]] then
            print "node|o|$aid|si_sz_memory_used._total|$val"
        fi
    fi

    if [[ $targettype == *hpux* && $1 == *Memory*size* ]] then
        val=${vs[2]}
        if [[ $val != '' ]] then
            print "node|o|$aid|si_sz_memory_used._total|$val"
        fi
    fi

    return 0
}

if [[ $SWIFTWARNLEVEL != "" ]] then
    typeset -ft vg_ssh_fun_mainvmstat_init vg_ssh_fun_mainvmstat_term
    typeset -ft vg_ssh_fun_mainvmstat_add vg_ssh_fun_mainvmstat_send
    typeset -ft vg_ssh_fun_mainvmstat_receive vg_ssh_fun_mainvmstat_emit
    typeset -ft vg_ssh_fun_mainvmstat_invsend vg_ssh_fun_mainvmstat_invreceive
fi
