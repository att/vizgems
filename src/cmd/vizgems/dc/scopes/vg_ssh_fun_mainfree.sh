free=()
typeset -A freevs freeus freels

function vg_ssh_fun_mainfree_init {
    free.varn=0
    freels[memory_used._total]='Used Memory'
    freels[memory_allused._total]='All Used Memory'
    freels[memory_buffersused._total]='Buffer Memory Used'
    freels[memory_cacheused._total]='Cache Memory Used'
    freels[swap_used._total]='Used Swap'
    return 0
}

function vg_ssh_fun_mainfree_term {
    return 0
}

function vg_ssh_fun_mainfree_add {
    typeset var=${as[var]}

    typeset -n freer=free._${free.varn}
    freer.name=$name
    freer.unit=$unit
    freer.type=$type
    freer.var=$var
    (( free.varn++ ))
    return 0
}

function vg_ssh_fun_mainfree_send {
    typeset cmd

    case $targettype in
    *linux*)
        cmd="free -m"
        ;;
    esac
    print -r "$cmd"
    return 0
}

function vg_ssh_fun_mainfree_receive {
    case $targettype in
    *linux*)
        if [[ $1 == Mem:* ]] then
            set -f
            set -A res -- $1
            set +f
            freevs[memory_allused._total]=${res[2]}
            freeus[memory_allused._total]=MB
            freevs[memory_buffersused._total]=${res[5]}
            freeus[memory_buffersused._total]=MB
            freevs[memory_cacheused._total]=${res[6]}
            freeus[memory_cacheused._total]=MB
        elif [[ $1 == *cache* ]] then
            set -f
            set -A res -- $1
            set +f
            freevs[memory_used._total]=${res[2]}
            freeus[memory_used._total]=MB
        elif [[ $1 == Swap:* ]] then
            set -f
            set -A res -- $1
            set +f
            freevs[swap_used._total]=${res[2]}
            freeus[swap_used._total]=MB
        fi
        ;;
    esac
    return 0
}

function vg_ssh_fun_mainfree_emit {
    typeset freei

    for (( freei = 0; freei < free.varn; freei++ )) do
        typeset -n freer=free._$freei
        [[ ${freevs[${freer.name}]} == '' ]] && continue
        typeset -n vref=vars._$varn
        (( varn++ ))
        vref.rt=STAT
        vref.name=${freer.name}
        vref.type=${freer.type}
        vref.num=${freevs[${freer.name}]}
        vref.unit=${freeus[${freer.name}]}
        vref.label=${freels[${freer.name}]}
    done
    return 0
}

function vg_ssh_fun_mainfree_invsend {
    typeset cmd

    case $targettype in
    *linux*)
        cmd="free -m"
        ;;
    esac
    print -r "$cmd"
    return 0
}

function vg_ssh_fun_mainfree_invreceive {
    typeset val

    case $targettype in
    *linux*)
        if [[ $1 == Mem:* ]] then
            set -f
            set -A res -- $1
            set +f
            val=${res[1]}
            if [[ $val != '' ]] then
                print "node|o|$aid|si_sz_memory_used._total|$val"
                print "node|o|$aid|si_sz_memory_allused._total|$val"
                print "node|o|$aid|si_sz_memory_buffersused._total|$val"
                print "node|o|$aid|si_sz_memory_cacheused._total|$val"
            fi
        elif [[ $1 == Swap:* ]] then
            set -f
            set -A res -- $1
            set +f
            val=${res[1]}
            if [[ $val != '' ]] then
                print "node|o|$aid|si_sz_swap_used._total|$val"
            fi
        fi
        ;;
    esac
    return 0
}

if [[ $SWIFTWARNLEVEL != "" ]] then
    typeset -ft vg_ssh_fun_mainfree_init vg_ssh_fun_mainfree_term
    typeset -ft vg_ssh_fun_mainfree_add vg_ssh_fun_mainfree_send
    typeset -ft vg_ssh_fun_mainfree_receive vg_ssh_fun_mainfree_emit
    typeset -ft vg_ssh_fun_mainfree_invsend vg_ssh_fun_mainfree_invreceive
fi
