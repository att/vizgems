ps=()
typeset -A psvs psus psls

function vg_ssh_fun_mainps_init {
    ps.varn=0
    ps.procn=0
    ps.mcpu=-1.0
    psls[os_nproc._total]='Number of Procs'
    return 0
}

function vg_ssh_fun_mainps_term {
    return 0
}

function vg_ssh_fun_mainps_add {
    typeset var=${as[var]} inst=${as[inst]} exclude=${as[exclude]}

    typeset -n psr=ps._${ps.varn}
    psr.name=$name
    psr.unit=$unit
    psr.type=$type
    psr.var=$var
    psr.inst=$inst
    (( ps.varn++ ))
    ps.excludere+="|${exclude//,/'|'}"
    ps.excludere=${ps.excludere#'|'}
    ps.excludere=${ps.excludere%'|'}
    return 0
}

function vg_ssh_fun_mainps_send {
    typeset cmd

    case $targettype in
    *linux*)
        cmd="/bin/ps -e -o '%cpu,user,comm'"
        ;;
    *solaris*)
        cmd="/bin/ps -e -o 'pcpu,user,comm'"
        ;;
    esac
    cmd+=" | sed 1d"
    print -r "$cmd"
    return 0
}

function vg_ssh_fun_mainps_receive {
    typeset nu='+([0-9.])' sp='*( )' nu2='*( )+([0-9.])@([a-zA-Z])*( )' n v u
    typeset -u U

    case $targettype in
    *linux*)
        set -f
        set -A ls -- $1
        set +f
        v=${ls[0]}
        [[ $v != [0-9]* ]] && return 0
        u=${ls[1]}
        n=${ls[2]}
        [[ ${n%/*} != '' ]] && n=${n%/*}
        n=${n//[!a-zA-Z0-9.,-]/}
        n=${n#-}
        (( ps.procn++ ))
        psvs[os_nproc._total]=${ps.procn}
        psus[os_nproc._total]=
        if (( v > ps.mcpu )) && [[ $u:$n != @(${ps.excludere}) ]] then
            psvs[proc_topcpu.1]=$v
            psus[proc_topcpu.1]=%
            psls[proc_topcpu.1]=$n
            (( ps.mcpu = v ))
        fi
        ;;
    *solaris*)
        set -f
        set -A ls -- $1
        set +f
        v=${ls[0]}
        [[ $v != [0-9]* ]] && return 0
        u=${ls[1]}
        n=${ls[2]}
        [[ ${n%/} != '' ]] && n=${n%/}
        [[ ${n##*/} != '' ]] && n=${n##*/}
        n=${n//[!a-zA-Z0-9.,-]/}
        n=${n#-}
        (( ps.procn++ ))
        psvs[os_nproc._total]=${ps.procn}
        psus[os_nproc._total]=
        if (( v > ps.mcpu )) && [[ $u:$n != @(${ps.excludere}) ]] then
            psvs[proc_topcpu.1]=$v
            psus[proc_topcpu.1]=%
            psls[proc_topcpu.1]=$n
            (( ps.mcpu = v ))
        fi
        ;;
    esac
    return 0
}

function vg_ssh_fun_mainps_emit {
    typeset psi

    for (( psi = 0; psi < ps.varn; psi++ )) do
        typeset -n psr=ps._$psi
        if [[ ${psvs[${psr.var}.${psr.inst}]} == '' ]] then
            if [[ ${psr.var} == proc_topcpu ]] then
                psvs[${psr.var}.${psr.inst}]=0
                psus[${psr.var}.${psr.inst}]=%
                psls[${psr.var}.${psr.inst}]=none
            else
                continue
            fi
        fi
        typeset -n vref=vars._$varn
        (( varn++ ))
        vref.rt=STAT
        vref.name=${psr.name}
        vref.type=${psr.type}
        vref.num=${psvs[${psr.var}.${psr.inst}]}
        vref.unit=${psus[${psr.var}.${psr.inst}]}
        if [[ ${psls[${psr.var}.${psr.inst}]} != '' ]] then
            vref.label=${psls[${psr.var}.${psr.inst}]}
        fi
    done
    return 0
}

function vg_ssh_fun_mainps_invsend {
    print 'echo'
    return 0
}

function vg_ssh_fun_mainps_invreceive {
    return 0
}

if [[ $SWIFTWARNLEVEL != "" ]] then
    typeset -ft vg_ssh_fun_mainps_init vg_ssh_fun_mainps_term
    typeset -ft vg_ssh_fun_mainps_add vg_ssh_fun_mainps_send
    typeset -ft vg_ssh_fun_mainps_receive vg_ssh_fun_mainps_emit
    typeset -ft vg_ssh_fun_mainps_invsend vg_ssh_fun_mainps_invreceive
fi
