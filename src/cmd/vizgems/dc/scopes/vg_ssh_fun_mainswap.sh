swap=()
typeset -A swapvs swapus swapls

function vg_ssh_fun_mainswap_init {
    swap.varn=0
    swapls[swap_used._total]='Used Swap'
    swapls[swap_free._total]='Free Swap'
    swapls[swap_total._total]='Total Swap'
    return 0
}

function vg_ssh_fun_mainswap_term {
    return 0
}

function vg_ssh_fun_mainswap_add {
    typeset var=${as[var]}

    typeset -n swapr=swap._${swap.varn}
    swapr.var=$var
    swapr.name=$name
    swapr.unit=$unit
    swapr.type=$type
    (( swap.varn++ ))
    return 0
}

function vg_ssh_fun_mainswap_send {
    typeset cmd

    case $targettype in
    *solaris*)
        cmd="(swap -s || /usr/sbin/swap -s) 2> /dev/null"
        ;;
    esac
    print -r "$cmd"
    return 0
}

function vg_ssh_fun_mainswap_receive {
    typeset nu
    typeset -u U

    case $targettype in
    *solaris*)
        nu='+([0-9.])+([a-zA-Z])'
        [[ $1 == *=\ $nu\ used,\ $nu\ available ]] && {
            set -f
            set -A res -- "${.sh.match[@]}"
            set +f
            swapvs[swap_used._total]=${res[1]}
            U=${res[2]}B
            swapus[swap_used._total]=$U
            swapvs[swap_free._total]=${res[3]}
            U=${res[4]}B
            swapus[swap_free._total]=$U
            (( swapvs[swap_total._total] = ${res[1]} + ${res[3]} ))
            U=${res[4]}B
            swapus[swap_total._total]=$U
        }
        ;;
    esac
    return 0
}

function vg_ssh_fun_mainswap_emit {
    typeset swapi

    for (( swapi = 0; swapi < swap.varn; swapi++ )) do
        typeset -n swapr=swap._$swapi
        [[ ${swapvs[${swapr.name}]} == '' ]] && continue
        typeset -n vref=vars._$varn
        (( varn++ ))
        vref.rt=STAT
        vref.name=${swapr.name}
        vref.type=${swapr.type}
        vref.num=${swapvs[${swapr.name}]}
        vref.unit=KB
        vref.label=${swapls[${swapr.name}]}
    done
    return 0
}

function vg_ssh_fun_mainswap_invsend {
    typeset cmd

    case $targettype in
    *solaris*)
        cmd="(swap -s || /usr/sbin/swap -s) 2> /dev/null"
        ;;
    esac
    print -r "$cmd"
    return 0
}

function vg_ssh_fun_mainswap_invreceive {
    typeset nu
    typeset val
    typeset -u U

    case $targettype in
    *solaris*)
        nu='+([0-9.])+([a-zA-Z])'
        [[ $1 == *=\ $nu\ used,\ $nu\ available ]] && {
            set -f
            set -A res -- "${.sh.match[@]}"
            set +f
            U=${res[2]}B
            vg_unitconv "$(( ${res[1]} + ${res[3]} ))" "$U" MB
            val=$vg_ucnum
            if [[ $val != '' ]] then
                print "node|o|$aid|si_sz_swap_used._total|$val"
            fi
        }
        ;;
    esac
    return 0
}

if [[ $SWIFTWARNLEVEL != "" ]] then
    typeset -ft vg_ssh_fun_mainswap_init vg_ssh_fun_mainswap_term
    typeset -ft vg_ssh_fun_mainswap_add vg_ssh_fun_mainswap_send
    typeset -ft vg_ssh_fun_mainswap_receive vg_ssh_fun_mainswap_emit
    typeset -ft vg_ssh_fun_mainswap_invsend vg_ssh_fun_mainswap_invreceive
fi
