netstat=()
typeset -A netstatns netstatus netstatls netstatis netstatvs

function vg_ssh_fun_mainnetstat_init {
    netstat.varn=0
    case $targettype in
    *linux*)
        netstatns[tcpip_inpkt]=RX-OK;      netstatus[tcpip_inpkt]=pkts
        netstatns[tcpip_outpkt]=TX-OK;     netstatus[tcpip_outpkt]=pkts
        netstatns[tcpip_inerrpkt]=RX-ERR;  netstatus[tcpip_inerrpkt]=pkts
        netstatns[tcpip_outerrpkt]=TX-ERR; netstatus[tcpip_outerrpkt]=pkts
        ;;
    *solaris*)
        netstatns[tcpip_inpkt]=Ipkts;     netstatus[tcpip_inpkt]=pkts
        netstatns[tcpip_outpkt]=Opkts;    netstatus[tcpip_outpkt]=pkts
        netstatns[tcpip_inerrpkt]=Ierrs;  netstatus[tcpip_inerrpkt]=pkts
        netstatns[tcpip_outerrpkt]=Oerrs; netstatus[tcpip_outerrpkt]=pkts
        ;;
    *darwin*)
        netstatns[tcpip_inpkt]=Ipkts;     netstatus[tcpip_inpkt]=pkts
        netstatns[tcpip_outpkt]=Opkts;    netstatus[tcpip_outpkt]=pkts
        netstatns[tcpip_inerrpkt]=Ierrs;  netstatus[tcpip_inerrpkt]=pkts
        netstatns[tcpip_outerrpkt]=Oerrs; netstatus[tcpip_outerrpkt]=pkts
        ;;
    esac
    netstatls[tcpip_inpkt]='In Packets'
    netstatls[tcpip_outpkt]='Out Packets'
    netstatls[tcpip_inerrpkt]='In Errors'
    netstatls[tcpip_outerrpkt]='Out Errors'
    return 0
}

function vg_ssh_fun_mainnetstat_term {
    return 0
}

function vg_ssh_fun_mainnetstat_add {
    typeset var=${as[var]} inst=${as[inst]}

    typeset -n netstatr=netstat._${netstat.varn}
    netstatr.name=$name
    netstatr.unit=${netstatus[$var]:-$unit}
    netstatr.type=$type
    netstatr.var=$var
    netstatr.inst=$inst
    (( netstat.varn++ ))
    return 0
}

function vg_ssh_fun_mainnetstat_send {
    typeset cmd

    case $targettype in
    *linux*)
        cmd="netstat -i | egrep -v '^Kernel|no statistics'"
        ;;
    *solaris*)
        cmd="netstat -in | egrep '^[a-zA-Z]'"
        ;;
    *darwin*)
        cmd="netstat -i | egrep -v '^lo'"
        ;;
    esac
    cmd+=" | egrep -v '^lo'"
    print -r "$cmd"
    return 0
}

function vg_ssh_fun_mainnetstat_receive {
    typeset vs vi vi1 vi2 vi3 vi4 ni inst

    [[ $targettype == *sgi* && $1 == ' '* ]] && return 0

    line=$1
    while [[ $line == *' '0[0-9]* ]] do
        line=${line//' 0'/' 0 '}
    done
    set -f
    set -A vs -- $line
    set +f

    if [[ $line == Iface* || $line == Name* ]] then
        for (( vi = 0; vi < ${#vs[@]}; vi++ )) do
            for ni in "${!netstatns[@]}"; do
                if [[ ${netstatns[$ni]} == ${vs[$vi]} ]] then
                    netstatis[$vi]=$ni
                    break
                fi
            done
        done
        return 0
    fi

    inst=${vs[0]}
    if [[ $inst == lo* ]] then
        return 0
    fi

    if [[ $targettype == *darwin* ]] then
        [[ $inst == *'*' ]] && return 0
        (( vi = ${#vs[@]} - 5 ))
        (( vi1 = vi ))
        (( vi2 = vi + 1 ))
        (( vi3 = vi + 2 ))
        (( vi4 = vi + 3 ))
        [[ ${vs[$vi1]} == 0 && ${vs[$vi3]} == 0 ]] && return 0
        [[ ${vs[$vi2]} != +([0-9]) || ${vs[$vi4]} != +([0-9]) ]] && return 0
        [[ ${vs[$vi1]} != +([0-9]) || ${vs[$vi3]} != +([0-9]) ]] && return 0
        (( netstatvs[tcpip_inpkt.$inst] = vs[$vi1] ))
        (( netstatvs[tcpip_inerrpkt.$inst] = vs[$vi2] ))
        (( netstatvs[tcpip_outpkt.$inst] = vs[$vi3] ))
        (( netstatvs[tcpip_outerrpkt.$inst] = vs[$vi4] ))
    else
        for (( vi = 0; vi < ${#vs[@]}; vi++ )) do
            [[ ${netstatis[$vi]} == '' ]] && continue
            (( netstatvs[${netstatis[$vi]}.$inst] = vs[$vi] ))
        done
    fi
    return 0
}

function vg_ssh_fun_mainnetstat_emit {
    typeset niinst ni netstati
    typeset -A totalvs

    for niinst in "${!netstatvs[@]}"; do
        ni=${niinst%%.*}
        (( totalvs[$ni] += ${netstatvs[$niinst]} ))
    done
    for ni in "${!totalvs[@]}"; do
        (( netstatvs[$ni._total] = totalvs[$ni] ))
    done

    for (( netstati = 0; netstati < netstat.varn; netstati++ )) do
        typeset -n netstatr=netstat._$netstati
        [[ ${netstatvs[${netstatr.var}.${netstatr.inst}]} == '' ]] && continue
        typeset -n vref=vars._$varn
        (( varn++ ))
        vref.rt=STAT
        vref.name=${netstatr.name}
        vref.type=${netstatr.type}
        vref.num=${netstatvs[${netstatr.var}.${netstatr.inst}]}
        vref.unit=${netstatr.unit}
        if [[ ${netstatr.inst} == _total ]] then
            vref.label="${netstatls[${netstatr.var}]} (SUM)"
        else
            vref.label="${netstatls[${netstatr.var}]} (${netstatr.inst})"
        fi
    done
    return 0
}

function vg_ssh_fun_mainnetstat_invsend {
    typeset cmd

    case $targettype in
    *linux*)
        cmd="netstat -i | egrep -v '^Kernel|no statistics'"
        ;;
    *solaris*)
        cmd="netstat -in | egrep '^[a-zA-Z]'"
        ;;
    *darwin*)
        cmd="netstat -i | egrep -v '^lo'"
        ;;
    esac
    cmd+=" | egrep -v '^lo'"
    print -r "$cmd"
    return 0
}

function vg_ssh_fun_mainnetstat_invreceive {
    typeset inst vs vi vi1 vi2 vi3 vi4

    [[ $targettype == *sgi* && $1 == ' '* ]] && return 0

    set -f
    set -A vs -- $1
    set +f

    if [[ $1 == Iface* || $1 == Name* ]] then
        return 0
    fi

    inst=${vs[0]}
    if [[ $inst == lo* ]] then
        return 0
    fi

    if [[ $targettype == *darwin* ]] then
        [[ $inst == *'*' ]] && return 0
        (( vi = ${#vs[@]} - 5 ))
        (( vi1 = vi ))
        (( vi2 = vi + 1 ))
        (( vi3 = vi + 2 ))
        (( vi4 = vi + 3 ))
        [[ ${vs[$vi1]} == 0 && ${vs[$vi3]} == 0 ]] && return 0
        [[ ${vs[$vi2]} != +([0-9]) || ${vs[$vi4]} != +([0-9]) ]] && return 0
        [[ ${vs[$vi1]} != +([0-9]) || ${vs[$vi3]} != +([0-9]) ]] && return 0
    fi

    print "node|o|$aid|si_iface$inst|$inst"
    return 0
}

if [[ $SWIFTWARNLEVEL != "" ]] then
    typeset -ft vg_ssh_fun_mainnetstat_init vg_ssh_fun_mainnetstat_term
    typeset -ft vg_ssh_fun_mainnetstat_add vg_ssh_fun_mainnetstat_send
    typeset -ft vg_ssh_fun_mainnetstat_receive vg_ssh_fun_mainnetstat_emit
    typeset -ft vg_ssh_fun_mainnetstat_invsend vg_ssh_fun_mainnetstat_invreceive
fi
