mpstat=()
typeset -A mpstatns mpstatus mpstatls mpstatis mpstatvs mpstatcs

function vg_ssh_fun_mainmpstat_init {
    mpstat.varn=0
    mpstat.total=
    mpstat.count=4
    mpstat.cpuvi=
    case $targettype in
    *linux*)
        mpstatns[cpu_usr]=%user;    mpstatus[cpu_usr]=%
        mpstatns[cpu_sys]=%sys;     mpstatus[cpu_sys]=%
        mpstatns[cpu_wait]=%iowait; mpstatus[cpu_wait]=%
        mpstatns[cpu_free]=%idle;   mpstatus[cpu_free]=%
        ;;
    *solaris*)
        mpstatns[cpu_usr]=usr;  mpstatus[cpu_usr]=%
        mpstatns[cpu_sys]=sys;  mpstatus[cpu_sys]=%
        mpstatns[cpu_wait]=wt;  mpstatus[cpu_wait]=%
        mpstatns[cpu_free]=idl; mpstatus[cpu_free]=%
        ;;
    esac
    mpstatls[cpu_usr]='CPU User'
    mpstatls[cpu_sys]='CPU System'
    mpstatls[cpu_wait]='CPU I/O Wait'
    mpstatls[cpu_free]='CPU Free'
    return 0
}

function vg_ssh_fun_mainmpstat_term {
    return 0
}

function vg_ssh_fun_mainmpstat_add {
    typeset var=${as[var]} inst=${as[inst]} count=${as[count]}

    if (( count > mpstat.count )) then
        mpstat.count=$count
    fi

    typeset -n mpstatr=mpstat._${mpstat.varn}
    mpstatr.name=$name
    mpstatr.unit=${mpstatus[$var]:-$unit}
    mpstatr.type=$type
    mpstatr.var=$var
    mpstatr.inst=$inst
    (( mpstat.varn++ ))
    return 0
}

function vg_ssh_fun_mainmpstat_send {
    typeset cmd n

    case $targettype in
    *linux*)
        n=${mpstat.count}
        cmd="(mpstat -P ALL 1 $n || mpstat 1 $n) 2> /dev/null"
        cmd+=" | egrep '^[0-9][0-9]:'"
        ;;
    *solaris*)
        (( n = ${mpstat.count} + 1 ))
        cmd="mpstat 1 $n"
        cmd+=" | egrep '^(CPU|  *[0-9])' | sed 's/syscl/csysl/'"
        ;;
    esac
    print -r -- "$cmd"
    return 0
}

function vg_ssh_fun_mainmpstat_receive {
    typeset vi ni cpu

    set -f
    set -A vs -- $1
    set +f

    if [[ $1 == *@(CPU|cpu)* ]] then
        if [[ ${mpstat.cpuvi} == '' ]] then
            for (( vi = 0; vi < ${#vs[@]}; vi++ )) do
                if [[ ${vs[$vi]} == @(CPU|cpu) ]] then
                    mpstat.cpuvi=$vi
                fi
                for ni in "${!mpstatns[@]}"; do
                    if [[ ${vs[$vi]} == ${mpstatns[$ni]}* ]] then
                        mpstatis[$vi]=$ni
                        break
                    fi
                done
            done
        fi
        return 0
    fi

    cpu=${vs[${mpstat.cpuvi}]}
    if [[ $cpu == all ]] then
        return 0
    fi

    (( mpstatcs[$cpu]++ ))
    if [[ $targettype == *solaris* ]] && (( mpstatcs[$cpu] == 1 )) then
        return 0
    fi

    for (( vi = 0; vi < ${#vs[@]}; vi++ )) do
        [[ ${mpstatis[$vi]} == '' ]] && continue
        if (( vs[$vi] > 100.00 )) then
            vs[$vi]=100.0
        elif (( vs[$vi] < 0.00 )) then
            vs[$vi]=0.0
        fi
        (( mpstatvs[${mpstatis[$vi]}.$cpu] += vs[$vi] ))
    done
    return 0
}

function vg_ssh_fun_mainmpstat_emit {
    typeset nicpu cpu ni mpstati
    typeset -A totalvs
    typeset -F3 v

    if [[ $targettype == *solaris* ]] then
        for cpu in "${!mpstatcs[@]}"; do
            (( mpstatcs[$cpu]-- ))
        done
    fi
    for nicpu in "${!mpstatvs[@]}"; do
        cpu=${nicpu#*.}
        (( mpstatvs[$nicpu] /= ${mpstatcs[$cpu]}.0 ))
    done
    for nicpu in "${!mpstatvs[@]}"; do
        ni=${nicpu%%.*}
        (( totalvs[$ni] += ${mpstatvs[$nicpu]} ))
    done
    for ni in "${!totalvs[@]}"; do
        (( mpstatvs[$ni._total] = totalvs[$ni] / ${#mpstatcs[@]}.0 ))
    done

    for (( mpstati = 0; mpstati < mpstat.varn; mpstati++ )) do
        typeset -n mpstatr=mpstat._$mpstati
        [[ ${mpstatvs[${mpstatr.var}.${mpstatr.inst}]} == '' ]] && continue
        typeset -n vref=vars._$varn
        (( varn++ ))
        vref.rt=STAT
        vref.name=${mpstatr.name}
        vref.type=${mpstatr.type}
        v=${mpstatvs[${mpstatr.var}.${mpstatr.inst}]}
        vref.num=$v
        vref.unit=${mpstatr.unit}
        if [[ ${mpstatr.inst} == _total ]] then
            vref.label="${mpstatls[${mpstatr.var}]} (All)"
        else
            vref.label="${mpstatls[${mpstatr.var}]} (${mpstatr.inst})"
        fi
    done
    return 0
}

function vg_ssh_fun_mainmpstat_invsend {
    typeset cmd

    case $targettype in
    *linux*)
        cmd="(mpstat -P ALL 1 1 || mpstat 1 1) 2> /dev/null"
        cmd+=" | egrep '^[0-9][0-9]:'"
        ;;
    *solaris*)
        cmd="mpstat 1 1"
        cmd+=" | egrep '^(CPU|  *[0-9])'"
        ;;
    esac
    print -r -- "$cmd"
    return 0
}

function vg_ssh_fun_mainmpstat_invreceive {
    typeset vi cpu

    set -f
    set -A vs -- $1
    set +f

    if [[ $1 == *@(CPU|cpu)* ]] then
        if [[ ${mpstat.cpuvi} == '' ]] then
            for (( vi = 0; vi < ${#vs[@]}; vi++ )) do
                if [[ ${vs[$vi]} == @(CPU|cpu) ]] then
                    mpstat.cpuvi=$vi
                    break
                fi
            done
        fi
        return 0
    fi

    cpu=${vs[${mpstat.cpuvi}]}
    if [[ $cpu == all ]] then
        return 0
    fi

    if [[ ${mpstat.total} == '' ]] then
        print "node|o|$aid|si_cpu_total|_total"
        mpstat.total=y
    fi
    print "node|o|$aid|si_cpu$cpu|$cpu"
    return 0
}

if [[ $SWIFTWARNLEVEL != "" ]] then
    typeset -ft vg_ssh_fun_mainmpstat_init vg_ssh_fun_mainmpstat_term
    typeset -ft vg_ssh_fun_mainmpstat_add vg_ssh_fun_mainmpstat_send
    typeset -ft vg_ssh_fun_mainmpstat_receive vg_ssh_fun_mainmpstat_emit
    typeset -ft vg_ssh_fun_mainmpstat_invsend vg_ssh_fun_mainmpstat_invreceive
fi
