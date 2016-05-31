uptime=()
typeset -A uptimevs uptimels

function vg_ssh_fun_mainuptime_init {
    uptime.varn=0
    uptimels[os_nuser._total]='Number of Users'
    uptimels[os_loadavg.1]='Load Average'
    uptimels[os_loadavg.5]='Load Average'
    uptimels[os_loadavg.15]='Load Average'
    return 0
}

function vg_ssh_fun_mainuptime_term {
    return 0
}

function vg_ssh_fun_mainuptime_add {
    typeset var=${as[var]} inst=${as[inst]}

    typeset -n uptimer=uptime._${uptime.varn}
    uptimer.name=$name
    uptimer.unit=$unit
    uptimer.type=$type
    uptimer.var=$var
    uptimer.inst=$inst
    (( uptime.varn++ ))
    return 0
}

function vg_ssh_fun_mainuptime_send {
    typeset cmd

    cmd="uptime"
    print -r "$cmd"
    return 0
}

function vg_ssh_fun_mainuptime_receive {
    typeset nu='+([0-9.])' sp='+( )'

    [[ $1 == *${sp}${nu}${sp}user*ge*:${sp}${nu},${sp}${nu},${sp}${nu}* ]] && {
        set -f
        set -A res -- "${.sh.match[@]}"
        set +f
        uptimevs[os_nuser._total]=${res[2]}
        uptimevs[os_loadavg.1]=${res[5]}
        uptimevs[os_loadavg.5]=${res[7]}
        uptimevs[os_loadavg.15]=${res[9]}
    }
    return 0
}

function vg_ssh_fun_mainuptime_emit {
    typeset uptimei

    for (( uptimei = 0; uptimei < uptime.varn; uptimei++ )) do
        typeset -n uptimer=uptime._$uptimei
        [[ ${uptimevs[${uptimer.var}.${uptimer.inst}]} == '' ]] && continue
        typeset -n vref=vars._$varn
        (( varn++ ))
        vref.rt=STAT
        vref.name=${uptimer.name}
        vref.type=${uptimer.type}
        vref.num=${uptimevs[${uptimer.var}.${uptimer.inst}]}
        vref.unit=
        vref.label=${uptimels[${uptimer.var}.${uptimer.inst}]}
    done
    return 0
}

function vg_ssh_fun_mainuptime_invsend {
    typeset cmd

    cmd="( uptime; uname -a )"
    print -r "$cmd"
    return 0
}

function vg_ssh_fun_mainuptime_invreceive {
    typeset val=$1

    val=${val//'|'/' '}
    val=${val##+(' ')}
    val=${val%%+(' ')}
    if [[ $1 == *'load average'* ]] then
        print "node|o|$aid|si_uptime|$val"
    else
        print "node|o|$aid|si_ident|$val"
    fi
    return 0
}

if [[ $SWIFTWARNLEVEL != "" ]] then
    typeset -ft vg_ssh_fun_mainuptime_init vg_ssh_fun_mainuptime_term
    typeset -ft vg_ssh_fun_mainuptime_add vg_ssh_fun_mainuptime_send
    typeset -ft vg_ssh_fun_mainuptime_receive vg_ssh_fun_mainuptime_emit
    typeset -ft vg_ssh_fun_mainuptime_invsend vg_ssh_fun_mainuptime_invreceive
fi
