top=()
typeset -A topvs topus topls

function vg_ssh_fun_maintop_init {
    top.varn=0
    top.cpui=8
    top.procn=0
    topls[memory_used._total]='Used Memory'
    topls[swap_used._total]='Used Swap'
    topls[os_nproc._total]='Number of Procs'
    topls[os_nthread._total]='Number of Threads'
    return 0
}

function vg_ssh_fun_maintop_term {
    return 0
}

function vg_ssh_fun_maintop_add {
    typeset var=${as[var]} inst=${as[inst]} exclude=${as[exclude]}

    typeset -n topr=top._${top.varn}
    topr.name=$name
    topr.unit=$unit
    topr.type=$type
    topr.var=$var
    topr.inst=$inst
    (( top.varn++ ))
    top.excludere+="|${exclude//,/'|'}"
    top.excludere=${top.excludere#'|'}
    top.excludere=${top.excludere%'|'}
    return 0
}

function vg_ssh_fun_maintop_send {
    typeset cmd

    case $targettype in
    *linux*)
        cmd="top -b -n1 | head -30"
        ;;
    *solaris*)
        cmd="(top -b -s1 || /usr/local/bin/top -b -s1 || /usr/sfw/bin/top -b -s1 || ./top -b -s1) 2> /dev/null | head -30"
        ;;
    *darwin*)
        cmd="top -l 2 -n 2 -S -o -cpu | awk '{ if (\$1 == \"Processes:\") n++; if (n > 1) print \$0 }'"
        ;;
    esac
    print -r "$cmd"
    return 0
}

function vg_ssh_fun_maintop_receive {
    typeset nu='+([0-9.])' sp='*( )'
    typeset nu2='*( )+([0-9.])@([a-zA-Z])*( )' sp2='+( )' n v u i
    typeset -u U
    typeset -A ucs

    case $targettype in
    *linux*)
        if [[ $1 == *PID* ]] then
            (( top.procn++ ))
            set -f
            set -A res -- $1
            set +f
            for (( i = 0; i < ${#res[@]}; i++ )) do
                if [[ ${res[$i]} == *CPU ]] then
                    top.cpui=$i
                    break
                fi
            done
            return 0
        fi

        if (( top.procn >= 1 )) then
            [[ $1 == ${sp}${nu}${sp}* ]] && {
                set -f
                set -A res -- $1
                set +f
                u=${res[1]}
                n=${res[*]:11}
                n=${n%/*}
                n=${n//[!a-zA-Z0-9.,-]/' '}
                v=${res[${top.cpui}]}
                v=${v//[!0-9.]/}
                if [[ $v != '' && $u:$n != @(${top.excludere}) ]] then
                    topvs[proc_topcpu.${top.procn}]=$v
                    topus[proc_topcpu.${top.procn}]=%
                    topls[proc_topcpu.${top.procn}]=$n
                    (( top.procn++ ))
                fi
            }
            return 0
        fi

        if [[ $1 == ${sp}${nu}${sp}processes:* ]] then
            set -f
            set -A res -- "${.sh.match[@]}"
            set +f
            topvs[os_nproc._total]=${res[2]}
            topus[os_nproc._total]=
        elif [[ $1 == Tasks:${sp}${nu}${sp}total,* ]] then
            set -f
            set -A res -- "${.sh.match[@]}"
            set +f
            topvs[os_nproc._total]=${res[2]}
            topus[os_nproc._total]=
        elif [[ $1 == Mem:${nu2}total,${nu2}used,${nu2}free* ]] then
            set -f
            set -A res -- "${.sh.match[@]}"
            set +f
            topvs[memory_total._total]=${res[2]}
            U=${res[3]}B
            topus[memory_total._total]=$U
            topvs[memory_free._total]=${res[10]}
            U=${res[11]}B
            topus[memory_free._total]=$U
        elif [[ $1 == Mem:${nu2}av,${nu2}used,${nu2}free* ]] then
            set -f
            set -A res -- "${.sh.match[@]}"
            set +f
            topvs[memory_total._total]=${res[2]}
            U=${res[3]}B
            topus[memory_total._total]=$U
            topvs[memory_free._total]=${res[10]}
            U=${res[11]}B
            topus[memory_free._total]=$U
        elif [[ $1 == Swap:${nu2}total,${nu2}used,${nu2}free* ]] then
            set -f
            set -A res -- "${.sh.match[@]}"
            set +f
            topvs[swap_total._total]=${res[2]}
            U=${res[3]}B
            topus[swap_total._total]=$U
            topvs[swap_free._total]=${res[10]}
            U=${res[11]}B
            topus[swap_free._total]=$U
        elif [[ $1 == Swap:${nu2}av,${nu2}used,${nu2}free* ]] then
            set -f
            set -A res -- "${.sh.match[@]}"
            set +f
            topvs[swap_total._total]=${res[2]}
            U=${res[3]}B
            topus[swap_total._total]=$U
            topvs[swap_free._total]=${res[10]}
            U=${res[11]}B
            topus[swap_free._total]=$U
        fi
        ;;
    *solaris*)
        if [[ $1 == *PID* ]] then
            (( top.procn++ ))
            return 0
        fi

        if (( top.procn >= 1 )) then
            [[ $1 == ${sp}${nu}${sp}* ]] && {
                set -f
                set -A res -- $1
                set +f
                u=${res[1]}
                n=${res[*]:10}
                n=${n%/*}
                n=${n//[!a-zA-Z0-9.,-]/' '}
                v=${res[9]%\%}
                v=${v//[!0-9.]/}
                if [[ $v != '' && $u:$n != @(${top.excludere}) ]] then
                    topvs[proc_topcpu.${top.procn}]=$v
                    topus[proc_topcpu.${top.procn}]=%
                    topls[proc_topcpu.${top.procn}]=$n
                    (( top.procn++ ))
                fi
            }
            return 0
        fi

        if [[ $1 == ${sp}${nu}${sp}processes:* ]] then
            set -f
            set -A res -- "${.sh.match[@]}"
            set +f
            topvs[os_nproc._total]=${res[2]}
            topus[os_nproc._total]=
        elif [[ $1 == M*y:${nu2}real,${nu2}free,${nu2}*use,${nu2}*free* ]] then
            set -f
            set -A res -- "${.sh.match[@]}"
            set +f
            topvs[memory_total._total]=${res[2]}
            U=${res[3]}B
            topus[memory_total._total]=$U
            topvs[memory_free._total]=${res[6]}
            U=${res[7]}B
            topus[memory_free._total]=$U
            topvs[swap_used._total]=${res[10]}
            U=${res[11]}B
            topus[swap_used._total]=$U
            topvs[swap_free._total]=${res[14]}
            U=${res[15]}B
            topus[swap_free._total]=$U
        fi
        ;;
    *darwin*)
        if [[ $1 == *PID* ]] then
            (( top.procn++ ))
            return 0
        fi

        if (( top.procn >= 1 )) then
            [[ $1 == ${nu}${sp2}+(?)${sp2}${nu}${sp2}${nu}:${nu}${sp2}${nu}/${nu}* ]] && {
                set -f
                set -A res -- "${.sh.match[@]}"
                set +f
                u=${1##*' '}
                n=${res[3]}
                n=${n%/*}
                n=${n##+(' ')}
                n=${n%%+(' ')}
                n=${n//[!a-zA-Z0-9.,-]/' '}
                v=${res[5]}
                v=${v//[!0-9.]/}
                if [[ $v != '' && $u:$n != @(${top.excludere}) ]] then
                    topvs[proc_topcpu.${top.procn}]=$v
                    topus[proc_topcpu.${top.procn}]=%
                    topls[proc_topcpu.${top.procn}]=$n
                    (( top.procn++ ))
                fi
            }
            return 0
        fi

        if [[ $1 == 'Processes: '${nu}${sp}total,${sp}${nu}${sp}running*,${sp}${nu}${sp}sleeping,${sp}${nu}${sp}threads* ]] then
            set -f
            set -A res -- "${.sh.match[@]}"
            set +f
            topvs[os_nproc._total]=${res[1]}
            topus[os_nproc._total]=
            topvs[os_runqueue._total]=${res[4]}
            topus[os_runqueue._total]=
            topvs[os_nthread._total]=${res[10]}
            topus[os_nthread._total]=
        elif [[ $1 == 'Load Avg: '${nu},${sp}${nu},${sp}${nu} ]] then
            set -f
            set -A res -- "${.sh.match[@]}"
            set +f
            topvs[os_loadavg.1]=${res[1]}
            topvs[os_loadavg.5]=${res[3]}
            topvs[os_loadavg.15]=${res[5]}
            topus[os_loadavg.1]=
            topus[os_loadavg.3]=
            topus[os_loadavg.15]=
        elif [[ $1 == 'CPU usage: '${nu}%?user,${sp}${nu}%?sys,${sp}${nu}%?idle* ]] then
            set -f
            set -A res -- "${.sh.match[@]}"
            set +f
            topvs[cpu_used._total]=$(( 100.0 - ${res[5]} ))
            topvs[cpu_sys._total]=${res[3]}
            topvs[cpu_usr._total]=${res[1]}
            topus[cpu_used._total]=%
            topus[cpu_sys._total]=%
            topus[cpu_usr._total]=%
        elif [[ $1 == 'PhysMem: '${nu2}wired,${nu2}active,${nu2}inactive,${nu2}used,${nu2}free* ]] then
            set -f
            set -A res -- "${.sh.match[@]}"
            set +f
            topvs[memory_used._total]=${res[14]}
            U=${res[15]}B
            topus[memory_used._total]=$U
        elif [[ $1 == 'VM: '${nu2}vsize,${nu2}framework?vsize,${sp}${nu}\(*\)?pageins,${sp}${nu}\(*\)?pageouts* ]] then
            set -f
            set -A res -- "${.sh.match[@]}"
            set +f
            topvs[os_pagein._total]=${res[10]}
            topus[os_pagein._total]=
            topvs[os_pageout._total]=${res[12]}
            topus[os_pageout._total]=
        elif [[ $1 == 'Swap: '${nu2}'+'${nu2}free* ]] then
            set -f
            set -A res -- "${.sh.match[@]}"
            set +f
            topvs[swap_used._total]=${res[2]}
            U=${res[15]}B
            topus[swap_used._total]=$U
        fi
        ;;
    esac
    return 0
}

function vg_ssh_fun_maintop_emit {
    typeset topi

    for (( topi = 0; topi < top.varn; topi++ )) do
        typeset -n topr=top._$topi
        if [[ ${topvs[${topr.var}.${topr.inst}]} == '' ]] then
            if [[ ${topr.var} == proc_topcpu ]] then
                topvs[${topr.var}.${topr.inst}]=0
                topus[${topr.var}.${topr.inst}]=%
                topls[${topr.var}.${topr.inst}]=none
            else
                continue
            fi
        fi
        typeset -n vref=vars._$varn
        (( varn++ ))
        vref.rt=STAT
        vref.name=${topr.name}
        vref.type=${topr.type}
        vref.num=${topvs[${topr.var}.${topr.inst}]}
        vref.unit=${topus[${topr.var}.${topr.inst}]}
        if [[ ${topls[${topr.var}.${topr.inst}]} != '' ]] then
            vref.label=${topls[${topr.var}.${topr.inst}]}
        fi
    done
    return 0
}

function vg_ssh_fun_maintop_invsend {
    typeset cmd

    case $targettype in
    *linux*)
        cmd="top -b -n1 | head -10"
        ;;
    *solaris*)
        cmd="(top -b -s1 || /usr/local/bin/top -b -s1) 2> /dev/null | head -10"
        ;;
    *darwin*)
        cmd="sysctl -a"
        ;;
    esac
    print -r "$cmd"
    return 0
}

function vg_ssh_fun_maintop_invreceive {
    typeset nu='+([0-9.])' sp='*( )' nu2='*( )+([0-9.])@([a-zA-Z])*( )' n v u
    typeset -u U
    typeset val val1 val2

    case $targettype in
    *linux*)
        if [[ $1 == *PID* ]] then
            (( top.procn++ ))
            return 0
        fi

        if (( top.procn >= 1 )) then
            return 0
        fi

        if [[ $1 == Mem:${nu2}total,${nu2}used,${nu2}free* ]] then
            set -f
            set -A res -- "${.sh.match[@]}"
            set +f
            U=${res[3]}B
            vg_unitconv "${res[2]}" "$U" MB
            val=$vg_ucnum
            if [[ $val != '' ]] then
                print "node|o|$aid|si_sz_memory_used._total|$val"
            fi
        elif [[ $1 == Mem:${nu2}av,${nu2}used,${nu2}free* ]] then
            set -f
            set -A res -- "${.sh.match[@]}"
            set +f
            U=${res[3]}B
            vg_unitconv "${res[2]}" "$U" MB
            val=$vg_ucnum
            if [[ $val != '' ]] then
                print "node|o|$aid|si_sz_memory_used._total|$val"
            fi
        elif [[ $1 == Swap:${nu2}total,${nu2}used,${nu2}free* ]] then
            set -f
            set -A res -- "${.sh.match[@]}"
            set +f
            U=${res[3]}B
            vg_unitconv "${res[2]}" "$U" MB
            val=$vg_ucnum
            if [[ $val != '' ]] then
                print "node|o|$aid|si_sz_swap_used._total|$val"
            fi
        elif [[ $1 == Swap:${nu2}av,${nu2}used,${nu2}free* ]] then
            set -f
            set -A res -- "${.sh.match[@]}"
            set +f
            U=${res[3]}B
            vg_unitconv "${res[2]}" "$U" MB
            val=$vg_ucnum
            if [[ $val != '' ]] then
                print "node|o|$aid|si_sz_swap_used._total|$val"
            fi
        fi
        ;;
    *solaris*)
        if [[ $1 == *PID* ]] then
            (( top.procn++ ))
            return 0
        fi

        if (( top.procn >= 1 )) then
            return 0
        fi

        if [[ $1 == M*y:${nu2}real,${nu2}free,${nu2}*use,${nu2}*free* ]] then
            set -f
            set -A res -- "${.sh.match[@]}"
            set +f
            U=${res[3]}B
            vg_unitconv "${res[2]}" "$U" MB
            val=$vg_ucnum
            if [[ $val != '' ]] then
                print "node|o|$aid|si_sz_memory_used._total|$val"
            fi
            U=${res[11]}B
            vg_unitconv "${res[10]}" "$U" MB
            val1=$vg_ucnum
            U=${res[15]}B
            vg_unitconv "${res[14]}" "$U" MB
            val2=$vg_ucnum
            if [[ $val1 != '' && $val2 != '' ]] then
                (( val = val1 + val2 ))
                print "node|o|$aid|si_sz_swap_used._total|$val"
            fi
        fi
        ;;
    *darwin*)
        if [[ $1 == 'hw.memsize = '${nu} ]] then
            set -f
            set -A res -- "${.sh.match[@]}"
            set +f
            U=B
            vg_unitconv "${res[1]}" "$U" MB
            val=$vg_ucnum
            if [[ $val != '' ]] then
                print "node|o|$aid|si_sz_memory_used._total|$val"
            fi
        elif [[ $1 == 'vm.swapusage: total = '${nu2}* ]] then
            set -f
            set -A res -- "${.sh.match[@]}"
            set +f
            U=${res[3]}B
            vg_unitconv "${res[2]}" "$U" MB
            val=$vg_ucnum
            if [[ $val != '' ]] then
                print "node|o|$aid|si_sz_swap_used._total|$val"
            fi
        fi
        ;;
    esac
    return 0
}

if [[ $SWIFTWARNLEVEL != "" ]] then
    typeset -ft vg_ssh_fun_maintop_init vg_ssh_fun_maintop_term
    typeset -ft vg_ssh_fun_maintop_add vg_ssh_fun_maintop_send
    typeset -ft vg_ssh_fun_maintop_receive vg_ssh_fun_maintop_emit
    typeset -ft vg_ssh_fun_maintop_invsend vg_ssh_fun_maintop_invreceive
fi
