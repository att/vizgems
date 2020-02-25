proc=()
typeset -A procvs procus procls proc_map
typeset -A proc_stat_state proc_net_dev_state
procus[cpu_usr]='%';             procls[cpu_usr]='CPU User'
procus[cpu_sys]='%';             procls[cpu_sys]='CPU System'
procus[cpu_wait]='%';            procls[cpu_wait]='CPU I/O Wait'
procus[cpu_used]='%';            procls[cpu_used]='CPU Used'
procus[os_runqueue]='';          procls[os_runqueue]='Run Queue'
procus[os_loadavg]='';           procls[os_loadavg]='Load Average'
procus[os_nthread]='';           procls[os_nthread]='Number of Threads'
procus[os_pagein]='KB';          procls[os_pagein]='Pages In'
procus[os_pageout]='KB';         procls[os_pageout]='Pages Out'
procus[os_swapin]='KB';          procls[os_swapin]='Swap In'
procus[os_swapout]='KB';         procls[os_swapout]='Swap Out'
procus[tcpip_inbyte]='MB';       procls[tcpip_inbyte]='In MBytes'
procus[tcpip_outbyte]='MB';      procls[tcpip_outbyte]='Out MBytes'
procus[tcpip_inbw]='mbps';       procls[tcpip_inbw]='In BW'
procus[tcpip_outbw]='mbps';      procls[tcpip_outbw]='Out BW'
procus[tcpip_inpkt]='pkts';      procls[tcpip_inpkt]='In Packets'
procus[tcpip_outpkt]='pkts';     procls[tcpip_outpkt]='Out Packets'
procus[tcpip_inerrpkt]='pkts';   procls[tcpip_inerrpkt]='In Errors'
procus[tcpip_outerrpkt]='pkts';  procls[tcpip_outerrpkt]='Out Errors'
procus[memory_allused]='MB';     procls[memory_allused]='All Used Memory'
procus[memory_buffersused]='MB'; procls[memory_buffersused]='Buffer Memory Used'
procus[memory_cacheused]='MB';   procls[memory_cacheused]='Cache Memory Used'
procus[memory_used]='MB';        procls[memory_used]='Used Memory'
procus[swap_used]='MB';          procls[swap_used]='Used Swap'

proc_map[pgpgin]=os_pagein
proc_map[pgpgout]=os_pageout
proc_map[pswpin]=os_swapin
proc_map[pswpout]=os_swapout

function vg_ssh_fun_mainproc_init {
    proc.varn=0
    proc.files=''
    return 0
}

function vg_ssh_fun_mainproc_term {
    return 0
}

function vg_ssh_fun_mainproc_add {
    typeset var=${as[var]} file=${as[file]} inst=${as[inst]}

    typeset -n procr=proc._${proc.varn}
    procr.name=$name
    procr.unit=
    procr.type=$type
    procr.var=$var
    procr.inst=$inst
    procr.file=$file
    (( proc.varn++ ))
    if [[ " ${proc.files} " != *' '$file' '* ]] then
        proc.files+=" $file"
        proc.files=${proc.files#' '}
        proc.cmd+=";cat /proc/$file | sed 's!^!$file:!'"
        proc.cmd=${proc.cmd#';'}
        vg_ssh_fun_mainproc_init_${file//'/'/_}
    fi

    return 0
}

function vg_ssh_fun_mainproc_send {
    print -r "( ${proc.cmd} )"

    return 0
}

function vg_ssh_fun_mainproc_receive {
    typeset file line

    file=${1%%:*}
    line=${1#"$file":}
    vg_ssh_fun_mainproc_receive_${file//'/'/_} "$line"

    return 0
}

function vg_ssh_fun_mainproc_emit {
    typeset file

    for file in ${proc.files}; do
        vg_ssh_fun_mainproc_emit_${file//'/'/_}
    done

    return 0
}

function vg_ssh_fun_mainproc_invsend {
    typeset cmd file

    cmd=
    for file in $PROC_FILES; do
        cmd+=";cat /proc/$file | sed 's!^!$file:!'"
    done
    print -r "( ${cmd#';'} )"

    return 0
}

function vg_ssh_fun_mainproc_invreceive {
    typeset file line

    file=${1%%:*}
    line=${1#"$file":}
    vg_ssh_fun_mainproc_invreceive_${file//'/'/_} "$line"

    return 0
}

function vg_ssh_fun_mainproc_init_stat {
    [[ ! -f proc_stat.state ]] && return 0
    . ./proc_stat.state

    return 0
}
function vg_ssh_fun_mainproc_receive_stat {
    typeset line proci ls cpuid

    line=$1
    case $line in
    cpu*)
        set -f
        set -A ls -- $line
        set +f
        cpuid=${line%%' '*}; cpuid=${cpuid#cpu}; cpuid=${cpuid:-_total}
        for (( proci = 0; proci < proc.varn; proci++ )) do
            typeset -n procr=proc._$proci
            [[ ${procr.file} != stat ]] && continue
            [[ ${procr.name} != cpu_*.$cpuid ]] && continue
            procvs[cpu_total.$cpuid]=$((
                ls[1] + ls[2] + ls[3] + ls[4] + ls[5] + ls[6] + ls[7]
            ))
            procvs[cpu_used.$cpuid]=$((
                ls[1] + ls[2] + ls[3] + ls[5] + ls[6] + ls[7]
            ))
            procvs[cpu_usr.$cpuid]=$(( ls[1] + ls[2] ))
            procvs[cpu_sys.$cpuid]=$(( ls[3] ))
            procvs[cpu_wait.$cpuid]=$(( ls[5] + ls[6] + ls[7] ))
            break
        done
        ;;
    procs_running*)
        set -f
        set -A ls -- $line
        set +f
        for (( proci = 0; proci < proc.varn; proci++ )) do
            typeset -n procr=proc._$proci
            [[ ${procr.file} != stat ]] && continue
            [[ ${procr.name} != os_runqueue* ]] && continue
            procvs[os_runqueue._total]=${ls[1]}
            break
        done
        ;;
    esac

    return 0
}
function vg_ssh_fun_mainproc_emit_stat {
    typeset proci cpuid u l v
    typeset -F3 cv v1 v2 tv1 tv2

    for (( proci = 0; proci < proc.varn; proci++ )) do
        typeset -n procr=proc._$proci
        [[ ${procr.file} != stat ]] && continue
        v= l=
        case ${procr.name} in
        cpu_*)
            cpuid=${procr.inst}
            v1=${proc_stat_state[${procr.name}]}
            v2=${procvs[${procr.name}]}
            tv1=${proc_stat_state[cpu_total.$cpuid]}
            tv2=${procvs[cpu_total.$cpuid]}
            if (( tv1 != tv2 )) then
                (( cv = 100.0 * (v2 - v1) / (tv2 - tv1) ))
                (( cv < 0.0 )) && cv=0.0
                (( cv > 100.0 )) && cv=100.0
                v=$cv
            fi
            u=${procus[${procr.var}]}
            if [[ $cpuid == _total ]] then
                l="${procls[${procr.var}]} (All)"
            else
                l="${procls[${procr.var}]} ($cpuid)"
            fi
            ;;
        os_runqueue*)
            v=${procvs[${procr.name}]}
            u=${procus[${procr.var}]}
            l=${procls[${procr.var}]}
            ;;
        esac
        if [[ $v != '' ]] then
            typeset -n vref=vars._$varn
            (( varn++ ))
            vref.rt=STAT
            vref.name=${procr.name}
            vref.type=${procr.type}
            vref.num=$v
            vref.unit=$u
            [[ $l != '' ]] && vref.label=$l
        fi
    done
    for procvi in "${!procvs[@]}"; do
        case $procvi in
        cpu_*)
            print -r "proc_stat_state[$procvi]=${procvs[$procvi]}"
            ;;
        esac
    done > proc_stat.state.tmp && mv proc_stat.state.tmp proc_stat.state

    return 0
}
function vg_ssh_fun_mainproc_invreceive_stat {
    typeset line proci ls cpuid

    line=$1
    case $line in
    cpu*)
        set -f
        set -A ls -- $line
        set +f
        cpuid=${line%%' '*}; cpuid=${cpuid#cpu}; cpuid=${cpuid:-_total}
        print "node|o|$aid|si_cpu$cpuid|$cpuid"
        ;;
    esac

    return 0
}

function vg_ssh_fun_mainproc_init_loadavg {
    return 0
}
function vg_ssh_fun_mainproc_receive_loadavg {
    typeset line proci ls inu='+([0-9])' fnu='+([0-9.])'

    line=$1
    case $line in
    */*)
        set -f
        set -A ls -- $line
        set +f
        cpuid=${line%%' '*}; cpuid=${cpuid#cpu}; cpuid=${cpuid:-_total}
        for (( proci = 0; proci < proc.varn; proci++ )) do
            typeset -n procr=proc._$proci
            [[ ${procr.file} != loadavg ]] && continue
            [[ ${procr.name} != @(os_loadavg|os_nthread)* ]] && continue
            if [[ $line == $fnu' '$fnu' '$fnu' '$inu/$inu' '* ]] then
                procvs[os_loadavg.1]=${.sh.match[1]}
                procvs[os_loadavg.5]=${.sh.match[2]}
                procvs[os_loadavg.15]=${.sh.match[3]}
                procvs[os_nthread._total]=${.sh.match[5]}
            fi
            break
        done
        ;;
    esac

    return 0
}
function vg_ssh_fun_mainproc_emit_loadavg {
    typeset proci u l v

    for (( proci = 0; proci < proc.varn; proci++ )) do
        typeset -n procr=proc._$proci
        [[ ${procr.file} != loadavg ]] && continue
        v= l=
        case ${procr.name} in
        os_loadavg*)
            v=${procvs[${procr.var}.${procr.inst}]}
            u=${procus[${procr.var}]}
            l=${procls[${procr.var}]}
            ;;
        os_nthread*)
            v=${procvs[${procr.var}.${procr.inst}]}
            u=${procus[${procr.var}]}
            l=${procls[${procr.var}]}
            ;;
        esac
        if [[ $v != '' ]] then
            typeset -n vref=vars._$varn
            (( varn++ ))
            vref.rt=STAT
            vref.name=${procr.name}
            vref.type=${procr.type}
            vref.num=$v
            vref.unit=$u
            [[ $l != '' ]] && vref.label=$l
        fi
    done

    return 0
}

function vg_ssh_fun_mainproc_init_vmstat {
    return 0
}
function vg_ssh_fun_mainproc_receive_vmstat {
    typeset line proci ls

    line=$1
    case $line in
    pgpgin*)
        set -f
        set -A ls -- $line
        set +f
        for (( proci = 0; proci < proc.varn; proci++ )) do
            typeset -n procr=proc._$proci
            [[ ${procr.file} != vmstat ]] && continue
            [[ ${procr.name} != os_pagein* ]] && continue
            procvs[${procr.name}]=${ls[1]}
        done
        ;;
    pgpgout*)
        set -f
        set -A ls -- $line
        set +f
        for (( proci = 0; proci < proc.varn; proci++ )) do
            typeset -n procr=proc._$proci
            [[ ${procr.file} != vmstat ]] && continue
            [[ ${procr.name} != os_pageout* ]] && continue
            procvs[${procr.name}]=${ls[1]}
        done
        ;;
    pswpin*)
        set -f
        set -A ls -- $line
        set +f
        for (( proci = 0; proci < proc.varn; proci++ )) do
            typeset -n procr=proc._$proci
            [[ ${procr.file} != vmstat ]] && continue
            [[ ${procr.name} != os_swapin* ]] && continue
            procvs[${procr.name}]=${ls[1]}
        done
        ;;
    pswpout*)
        set -f
        set -A ls -- $line
        set +f
        for (( proci = 0; proci < proc.varn; proci++ )) do
            typeset -n procr=proc._$proci
            [[ ${procr.file} != vmstat ]] && continue
            [[ ${procr.name} != os_swapout* ]] && continue
            procvs[${procr.name}]=${ls[1]}
        done
        ;;
    esac

    return 0
}
function vg_ssh_fun_mainproc_emit_vmstat {
    typeset proci cpuid u l v

    for (( proci = 0; proci < proc.varn; proci++ )) do
        typeset -n procr=proc._$proci
        [[ ${procr.file} != vmstat ]] && continue
        v= l=
        case ${procr.name} in
        os_page*|os_swap*)
            v=${procvs[${procr.name}]}
            u=${procus[${procr.var}]}
            l=${procls[${procr.var}]}
            ;;
        esac
        if [[ $v != '' ]] then
            typeset -n vref=vars._$varn
            (( varn++ ))
            vref.rt=STAT
            vref.name=${procr.name}
            vref.type=${procr.type}
            vref.num=$v
            vref.unit=$u
            [[ $l != '' ]] && vref.label=$l
        fi
    done

    return 0
}

vg_ssh_fun_mainproc_gotnames_net_dev=n
typeset -A vg_ssh_fun_mainproc_name2id_net_dev
function vg_ssh_fun_mainproc_init_net_dev {
    [[ ! -f proc_net_dev.state ]] && return 0
    . ./proc_net_dev.state

    return 0
}
function vg_ssh_fun_mainproc_receive_net_dev {
    typeset line li lj lo ls prefix iface

    line=$1
    case $line in
    *bytes*)
        set -f
        set -A ls -- ${line//'|'/' _X_X_ '}
        set +f
        lo=0
        for li in "${!ls[@]}"; do
            if [[ ${ls[$li]} == _X_X_ ]] then
                (( lo++ ))
                continue
            fi
            case $lo in
            0) prefix=inter ;;
            1) prefix=in    ;;
            2) prefix=out   ;;
            *) prefix=error ;;
            esac
            (( lj = li - lo ))
            vg_ssh_fun_mainproc_name2id_net_dev[$prefix${ls[$li]}]=$lj
        done
        vg_ssh_fun_mainproc_gotnames_net_dev=y
        ;;
    *:*)
        set -f
        set -A ls -- ${line//'|'/' '}
        set +f
        [[ $vg_ssh_fun_mainproc_gotnames_net_dev != y ]] && return 0
        iface=${ls[0]%%:*}
        li=${vg_ssh_fun_mainproc_name2id_net_dev[inbytes]}
        if [[ ${ls[$li]} != '' ]] then
            procvs[tcpip_inbyte.$iface]=${ls[$li]}
        fi
        li=${vg_ssh_fun_mainproc_name2id_net_dev[outbytes]}
        if [[ ${ls[$li]} != '' ]] then
            procvs[tcpip_outbyte.$iface]=${ls[$li]}
        fi
        li=${vg_ssh_fun_mainproc_name2id_net_dev[inpackets]}
        if [[ ${ls[$li]} != '' ]] then
            procvs[tcpip_inpkt.$iface]=${ls[$li]}
        fi
        li=${vg_ssh_fun_mainproc_name2id_net_dev[outpackets]}
        if [[ ${ls[$li]} != '' ]] then
            procvs[tcpip_outpkt.$iface]=${ls[$li]}
        fi
        li=${vg_ssh_fun_mainproc_name2id_net_dev[inerrs]}
        if [[ ${ls[$li]} != '' ]] then
            procvs[tcpip_inerrpkt.$iface]=${ls[$li]}
        fi
        li=${vg_ssh_fun_mainproc_name2id_net_dev[outerrs]}
        if [[ ${ls[$li]} != '' ]] then
            procvs[tcpip_outerrpkt.$iface]=${ls[$li]}
        fi
        ;;
    esac

    return 0
}
function vg_ssh_fun_mainproc_emit_net_dev {
    typeset proci k u l v
    typeset -F3 cv pv dv dt

    (( dt = VG_JOBTS - proc_net_dev_state[_time_] ))
    for (( proci = 0; proci < proc.varn; proci++ )) do
        typeset -n procr=proc._$proci
        [[ ${procr.file} != net/dev ]] && continue
        v= l=
        case ${procr.name} in
        *bw.*)
            k=${procr.var//bw/byte}.${procr.inst}
            [[ ${proc_net_dev_state[$k]} == '' ]] && continue
            pv=${proc_net_dev_state[$k]}
            cv=${procvs[$k]}
            (( dv = cv - pv ))
            if (( dt > 0 && dv >= 0 )) then
                (( v = (dv * 8) / (dt * 1000000.0) ))
            fi
            u=${procus[${procr.var}]}
            l="${procls[${procr.var}]} (${procr.inst})"
            ;;
        *byte.*)
            [[ ${proc_net_dev_state[${procr.name}]} == '' ]] && continue
            pv=${proc_net_dev_state[${procr.name}]}
            cv=${procvs[${procr.name}]}
            (( dv = cv - pv ))
            if (( dv >= 0 )) then
                (( v = dv / (1024.0 * 1024.0) ))
            fi
            u=${procus[${procr.var}]}
            l="${procls[${procr.var}]} (${procr.inst})"
            ;;
        *pkt.*)
            [[ ${proc_net_dev_state[${procr.name}]} == '' ]] && continue
            pv=${proc_net_dev_state[${procr.name}]}
            cv=${procvs[${procr.name}]}
            (( dv = cv - pv ))
            if (( dv >= 0 )) then
                (( v = dv ))
            fi
            u=${procus[${procr.var}]}
            l="${procls[${procr.var}]} (${procr.inst})"
            ;;
        esac
        if [[ $v != '' ]] then
            typeset -n vref=vars._$varn
            (( varn++ ))
            vref.rt=STAT
            vref.name=${procr.name}
            vref.type=${procr.type}
            vref.num=$v
            vref.unit=$u
            [[ $l != '' ]] && vref.label=$l
        fi
    done
    {
        for procvi in "${!procvs[@]}"; do
            print -r "proc_net_dev_state[$procvi]=${procvs[$procvi]}"
        done
        print -r "proc_net_dev_state[_time_]=$VG_JOBTS"
    } > proc_net_dev.state.tmp && mv proc_net_dev.state.tmp proc_net_dev.state

    return 0
}
function vg_ssh_fun_mainproc_invreceive_net_dev {
    typeset line ls iface

    line=$1
    case $line in
    *bytes*)
        vg_ssh_fun_mainproc_gotnames_net_dev=y
        ;;
    *:*)
        set -f
        set -A ls -- $line
        set +f
        [[ $vg_ssh_fun_mainproc_gotnames_net_dev != y ]] && return 0
        iface=${ls[0]%%:*}
        [[ $iface == lo* ]] && return 0
        print "node|o|$aid|si_iface$iface|$iface"
        ;;
    esac
    return 0
}

function vg_ssh_fun_mainproc_init_meminfo {
    return 0
}
function vg_ssh_fun_mainproc_receive_meminfo {
    typeset line v ls
    typeset -u u

    line=$1
    case $line in
    MemTotal:*)
        set -f
        set -A ls -- $line
        set +f
        v=${ls[1]} u=${ls[2]}
        vg_unitconv "$v" "$u" ${procus[memory_used]:-MB}
        procvs[memory_total]=$vg_ucnum
        ;;
    MemFree:*)
        set -f
        set -A ls -- $line
        set +f
        v=${ls[1]} u=${ls[2]}
        vg_unitconv "$v" "$u" ${procus[memory_used]:-MB}
        procvs[memory_free]=$vg_ucnum
        ;;
    Buffers:*)
        set -f
        set -A ls -- $line
        set +f
        v=${ls[1]} u=${ls[2]}
        vg_unitconv "$v" "$u" ${procus[memory_used]:-MB}
        procvs[memory_buffersused]=$vg_ucnum
        ;;
    Cached:*)
        set -f
        set -A ls -- $line
        set +f
        v=${ls[1]} u=${ls[2]}
        vg_unitconv "$v" "$u" ${procus[memory_used]:-MB}
        procvs[memory_cacheused]=$vg_ucnum
        ;;
    SwapTotal:*)
        set -f
        set -A ls -- $line
        set +f
        v=${ls[1]} u=${ls[2]}
        vg_unitconv "$v" "$u" ${procus[swap_used]:-MB}
        procvs[swap_total]=$vg_ucnum
        ;;
    SwapFree:*)
        set -f
        set -A ls -- $line
        set +f
        v=${ls[1]} u=${ls[2]}
        vg_unitconv "$v" "$u" ${procus[swap_used]:-MB}
        procvs[swap_free]=$vg_ucnum
        ;;
    esac

    return 0
}
function vg_ssh_fun_mainproc_emit_meminfo {
    typeset proci u l
    typeset -F3 v

    for (( proci = 0; proci < proc.varn; proci++ )) do
        typeset -n procr=proc._$proci
        [[ ${procr.file} != meminfo ]] && continue
        v= l=
        case ${procr.name} in
        memory_allused*)
            if [[
                ${procvs[memory_total]} != '' && ${procvs[memory_free]} != ''
            ]] then
                (( v = ${procvs[memory_total]} - ${procvs[memory_free]} ))
                u=${procus[memory_used]}
                l=${procls[memory_allused]}
            fi
            ;;
        memory_buffersused*)
            if [[ ${procvs[memory_buffersused]} != '' ]] then
                v=${procvs[memory_buffersused]}
                u=${procus[memory_used]}
                l=${procls[memory_buffersused]}
            fi
            ;;
        memory_cacheused*)
            if [[ ${procvs[memory_cacheused]} != '' ]] then
                v=${procvs[memory_cacheused]}
                u=${procus[memory_used]}
                l=${procls[memory_cacheused]}
            fi
            ;;
        memory_used*)
            if [[
                ${procvs[memory_total]} != '' &&
                ${procvs[memory_free]} != '' &&
                ${procvs[memory_buffersused]} != '' &&
                ${procvs[memory_cacheused]} != ''
            ]] then
                (( v = ${procvs[memory_total]} - (
                    ${procvs[memory_free]} +
                    ${procvs[memory_buffersused]} + ${procvs[memory_cacheused]}
                ) ))
                u=${procus[memory_used]}
                l=${procls[memory_used]}
            fi
            ;;
        swap_used*)
            if [[
                ${procvs[swap_total]} != '' && ${procvs[swap_free]} != ''
            ]] then
                (( v = ${procvs[swap_total]} - ${procvs[swap_free]} ))
                u=${procus[swap_used]}
                l=${procls[swap_used]}
            fi
            ;;
        esac
        if [[ $v != '' ]] then
            typeset -n vref=vars._$varn
            (( varn++ ))
            vref.rt=STAT
            vref.name=${procr.name}
            vref.type=${procr.type}
            vref.num=$v
            vref.unit=$u
            [[ $l != '' ]] && vref.label=$l
        fi
    done

    return 0
}
function vg_ssh_fun_mainproc_invreceive_meminfo {
    typeset line v ls

    line=$1
    case $line in
    MemTotal:*)
        set -f
        set -A ls -- $line
        set +f
        v=${ls[1]} u=${ls[2]}
        vg_unitconv "$v" "$u" ${procus[memory_used]:-MB}
        print "node|o|$aid|si_sz_memory_used._total|$vg_ucnum"
        print "node|o|$aid|si_sz_memory_allused._total|$vg_ucnum"
        print "node|o|$aid|si_sz_memory_buffersused._total|$vg_ucnum"
        print "node|o|$aid|si_sz_memory_cacheused._total|$vg_ucnum"
        ;;
    SwapTotal:*)
        set -f
        set -A ls -- $line
        set +f
        v=${ls[1]} u=${ls[2]}
        if [[ $v != 0 ]] then
            print "node|o|$aid|si_hasswap_total|_total"
        fi
        vg_unitconv "$v" "$u" ${procus[swap_used]:-MB}
        print "node|o|$aid|si_sz_swap_used._total|$vg_ucnum"
        ;;
    esac

    return 0
}

if [[ $SWIFTWARNLEVEL != "" ]] then
    typeset -ft vg_ssh_fun_mainproc_init vg_ssh_fun_mainproc_term
    typeset -ft vg_ssh_fun_mainproc_add vg_ssh_fun_mainproc_send
    typeset -ft vg_ssh_fun_mainproc_receive vg_ssh_fun_mainproc_emit
    typeset -ft vg_ssh_fun_mainproc_invsend vg_ssh_fun_mainproc_invreceive
    typeset -ft vg_ssh_fun_mainproc_init_stat
    typeset -ft vg_ssh_fun_mainproc_receive_stat
    typeset -ft vg_ssh_fun_mainproc_emit_stat
    typeset -ft vg_ssh_fun_mainproc_invreceive_stat
    typeset -ft vg_ssh_fun_mainproc_init_loadavg
    typeset -ft vg_ssh_fun_mainproc_receive_loadavg
    typeset -ft vg_ssh_fun_mainproc_emit_loadavg
    typeset -ft vg_ssh_fun_mainproc_init_vmstat
    typeset -ft vg_ssh_fun_mainproc_receive_vmstat
    typeset -ft vg_ssh_fun_mainproc_emit_vmstat
    typeset -ft vg_ssh_fun_mainproc_init_net_dev
    typeset -ft vg_ssh_fun_mainproc_receive_net_dev
    typeset -ft vg_ssh_fun_mainproc_emit_net_dev
    typeset -ft vg_ssh_fun_mainproc_invreceive_net_dev
    typeset -ft vg_ssh_fun_mainproc_init_meminfo
    typeset -ft vg_ssh_fun_mainproc_receive_meminfo
    typeset -ft vg_ssh_fun_mainproc_emit_meminfo
    typeset -ft vg_ssh_fun_mainproc_invreceive_meminfo
fi
