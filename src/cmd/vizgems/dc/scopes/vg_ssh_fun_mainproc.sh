proc=()
typeset -A procvs procus procls proc_state proc_map
procus[cpu_usr]='%';     procls[cpu_usr]='CPU User'
procus[cpu_sys]='%';     procls[cpu_sys]='CPU System'
procus[cpu_wait]='%';    procls[cpu_wait]='CPU I/O Wait'
procus[cpu_used]='%';    procls[cpu_used]='CPU Used'
procus[os_runqueue]='';  procls[os_runqueue]='Run Queue'
procus[os_loadavg]='';   procls[os_loadavg]='Load Average'
procus[os_nthread]='';   procls[os_nthread]='Number of Threads'
procus[os_pagein]='KB';  procls[os_pagein]='Pages In'
procus[os_pageout]='KB'; procls[os_pageout]='Pages Out'
procus[os_swapin]='KB';  procls[os_swapin]='Swap In'
procus[os_swapout]='KB'; procls[os_swapout]='Swap Out'

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
        vg_ssh_fun_mainproc_init_$file
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
    vg_ssh_fun_mainproc_receive_$file "$line"

    return 0
}

function vg_ssh_fun_mainproc_emit {
    typeset file

    for file in ${proc.files}; do
        vg_ssh_fun_mainproc_emit_$file
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
    vg_ssh_fun_mainproc_invreceive_$file "$line"

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
        v=
        case ${procr.name} in
        cpu_*)
            cpuid=${procr.inst}
            v1=${proc_state[${procr.name}]}
            v2=${procvs[${procr.name}]}
            tv1=${proc_state[cpu_total.$cpuid]}
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
            print -r "proc_state[$procvi]=${procvs[$procvi]}"
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
        v=
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
        v=
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
fi
