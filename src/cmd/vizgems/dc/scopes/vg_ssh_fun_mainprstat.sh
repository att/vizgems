prstat=()
typeset -A prstatvs prstatls

function vg_ssh_fun_mainprstat_init {
    prstat.varn=0
    prstatls[os_nproc._total]='Number of Procs'
    prstatls[os_nthread._total]='Number of Threads'
    return 0
}

function vg_ssh_fun_mainprstat_term {
    return 0
}

function vg_ssh_fun_mainprstat_add {
    typeset var=${as[var]}

    typeset -n prstatr=prstat._${prstat.varn}
    prstatr.name=$name
    prstatr.unit=$unit
    prstatr.type=$type
    prstatr.var=$var
    (( prstat.varn++ ))
    return 0
}

function vg_ssh_fun_mainprstat_send {
    typeset cmd

    case $targettype in
    *solaris*)
        cmd="prstat -c 1 1 | tail -1"
        ;;
    esac
    print -r "$cmd"
    return 0
}

function vg_ssh_fun_mainprstat_receive {
    typeset nu='+([0-9.])' sp='*( )'

    case $targettype in
    *solaris*)
        [[ $1 == *${sp}${nu}${sp}processes,${sp}${nu}${sp}lwps,* ]] && {
            set -f
            set -A res -- "${.sh.match[@]}"
            set +f
            prstatvs[os_nproc._total]=${res[2]}
            prstatvs[os_nthread._total]=${res[5]}
        }
        ;;
    esac
    return 0
}

function vg_ssh_fun_mainprstat_emit {
    typeset prstati

    for (( prstati = 0; prstati < prstat.varn; prstati++ )) do
        typeset -n prstatr=prstat._$prstati
        [[ ${prstatvs[${prstatr.var}._total]} == '' ]] && continue
        typeset -n vref=vars._$varn
        (( varn++ ))
        vref.rt=STAT
        vref.name=${prstatr.name}
        vref.type=${prstatr.type}
        vref.num=${prstatvs[${prstatr.var}._total]}
        vref.unit=
        vref.label=${prstatls[${prstatr.var}._total]}
    done
    return 0
}

function vg_ssh_fun_mainprstat_invsend {
    print 'echo'
    return 0
}

function vg_ssh_fun_mainprstat_invreceive {
    return 0
}

if [[ $SWIFTWARNLEVEL != "" ]] then
    typeset -ft vg_ssh_fun_mainprstat_init vg_ssh_fun_mainprstat_term
    typeset -ft vg_ssh_fun_mainprstat_add vg_ssh_fun_mainprstat_send
    typeset -ft vg_ssh_fun_mainprstat_receive vg_ssh_fun_mainprstat_emit
    typeset -ft vg_ssh_fun_mainprstat_invsend vg_ssh_fun_mainprstat_invreceive
fi
