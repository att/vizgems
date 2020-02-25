ntp=()
typeset -A ntpvs

function vg_ssh_fun_mainntp_init {
    ntp.varn=0
    return 0
}

function vg_ssh_fun_mainntp_term {
    return 0
}

function vg_ssh_fun_mainntp_add {
    typeset var=${as[var]} inst=${as[inst]}

    typeset -n ntpr=ntp._${ntp.varn}
    ntpr.name=$name
    ntpr.unit=ms
    ntpr.type=$type
    ntpr.var=$var
    ntpr.inst=$inst
    (( ntp.varn++ ))
    return 0
}

function vg_ssh_fun_mainntp_send {
    typeset cmd

    case $targettype in
    *linux*)
        cmd="(/usr/sbin/ntpq -p || /usr/sbin/ntpdc -c peers || /usr/bin/ntpq -p || /usr/bin/ntpdc -c peers || /usr/bin/chronyc sourcestats) 2> /dev/null"
        ;;
    esac
    print -r "$cmd"
    return 0
}

function vg_ssh_fun_mainntp_receive {
    typeset vn fs

    [[ $1 == *remote*local* || $1 == *@(LOCAL|POOL|INIT)* || $1 == *Name* || $1 == *Number* || $1 == *====* ]] && return 0
    set -f
    set -A vs -- $1
    set +f
    vn=${#vs[@]}

    if (( vn == 8 )) then
        ntpvs[ntp_offset._total]=${vs[6]%%+([a-z])}
    elif (( vn == 10 )) then
        ntpvs[ntp_offset._total]=${vs[8]}
    fi
    return 0
}

function vg_ssh_fun_mainntp_emit {
    typeset ntpi

    for (( ntpi = 0; ntpi < ntp.varn; ntpi++ )) do
        typeset -n ntpr=ntp._$ntpi
        [[ ${ntpvs[${ntpr.var}.${ntpr.inst}]} == '' ]] && continue
        typeset -n vref=vars._$varn
        (( varn++ ))
        vref.rt=STAT
        vref.name=${ntpr.name}
        vref.type=${ntpr.type}
        vref.num=${ntpvs[${ntpr.var}.${ntpr.inst}]}
        vref.unit=${ntpr.unit}
        vref.label="NTP Offset (All)"
    done
    return 0
}

function vg_ssh_fun_mainntp_invsend {
    return 0
}

function vg_ssh_fun_mainntp_invreceive {
    return 0
}

if [[ $SWIFTWARNLEVEL != "" ]] then
    typeset -ft vg_ssh_fun_mainntp_init vg_ssh_fun_mainntp_term
    typeset -ft vg_ssh_fun_mainntp_add vg_ssh_fun_mainntp_send
    typeset -ft vg_ssh_fun_mainntp_receive vg_ssh_fun_mainntp_emit
    typeset -ft vg_ssh_fun_mainntp_invsend vg_ssh_fun_mainntp_invreceive
fi
