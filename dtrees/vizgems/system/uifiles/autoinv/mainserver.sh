
function dq_vt_autoinvtab_tool_mainserver {
    dq_vt_autoinvtab_tool_mainserver_begin
    dq_vt_autoinvtab_tool_mainserver_body
    dq_vt_autoinvtab_tool_mainserver_end
}

function dq_vt_autoinvtab_tool_mainserver_begin {
    typeset tbli name

    dq_vt_autoinvtab_tool_default_begin
}

function dq_vt_autoinvtab_tool_mainserver_end {
    dq_vt_autoinvtab_tool_default_end
}

function dq_vt_autoinvtab_tool_mainserver_body {
    typeset tbli tblj row v ik k luni lunsize hdi hdname hdsize
    integer cpun=0
    typeset -A fsnames fssizes ifaces

    dq_vt_autoinvtab_tool_default_body
    tbli=${dq_vt_autoinvtab_tool_default_data.tbli}
    for ik in "${!dq_vt_autoinvtab_data.is.si_cpu@}"; do
        [[ $ik == *cpu[0-9]* ]] && (( cpun++ ))
    done
    for ik in "${!dq_vt_autoinvtab_data.is.scopeinv_cpu@}"; do
        [[ $ik == *cpu[0-9]* ]] && (( cpun++ ))
    done
    (( cpun > 0 )) && dq_vt_autoinvtab_adddatarow $tbli "cpus|$cpun"

    typeset -n vp=dq_vt_autoinvtab_data.is.[si_sz_memory_used._total]
    [[ $vp == '' ]] && typeset -n vp=dq_vt_autoinvtab_data.is.[scopeinv_size_memory_used._total]
    [[ $vp != '' ]] && dq_vt_autoinvtab_adddatarow $tbli "memory|$vp MB"

    typeset -n vp=dq_vt_autoinvtab_data.is.[si_sz_swap_used._total]
    [[ $vp == '' ]] && typeset -n vp=dq_vt_autoinvtab_data.is.[scopeinv_size_swap_used._total]
    [[ $vp != '' ]] && dq_vt_autoinvtab_adddatarow $tbli "swap|$vp MB"

    for ik in "${!dq_vt_autoinvtab_data.is.@}"; do
        if [[ $ik == *@(si|scopeinv)_fs* ]] then
            typeset -n ip=$ik
            k=${ik#dq_vt_autoinvtab_data.is.}
            [[ $k == '['@(*)']' ]] && k=${.sh.match[1]}
            fsnames[${k#@(si|scopeinv)_fs}]=$ip
        fi
        if [[ $ik == *@(si_sz|scopeinv_size)_fs_used.* ]] then
            typeset -n ip=$ik
            k=${ik#dq_vt_autoinvtab_data.is.}
            [[ $k == '['@(*)']' ]] && k=${.sh.match[1]}
            fssizes[${k#@(si_sz|scopeinv_size)_fs_used.}]=$ip
        fi
        if [[ $ik == *@(si|scopeinv)_iface* ]] then
            typeset -n ip=$ik
            k=${ik#dq_vt_autoinvtab_data.is.}
            [[ $k == '['@(*)']' ]] && k=${.sh.match[1]}
            ifaces[${k#@(si|scopeinv)_iface}]=$ip
        fi
    done
    if (( ${#fsnames[@]} > 0 )) then
        dq_vt_autoinvtab_beginsubtable $tbli "filesystems" ''
        tblj=${dq_vt_autoinvtab_data.tbli}
        dq_vt_autoinvtab_addheaderrow $tblj "name|size"
        for k in "${!fsnames[@]}"; do
            print -r -- "${fsnames[$k]}|right:${fssizes[$k]}"
        done | sort -t'|' -k1,1 | while read k v; do
            dq_vt_autoinvtab_adddatarow $tblj "$k|$v GB"
        done
        dq_vt_autoinvtab_endsubtable $tblj
    fi
    if (( ${#ifaces[@]} > 0 )) then
        dq_vt_autoinvtab_beginsubtable $tbli "interfaces" ''
        tblj=${dq_vt_autoinvtab_data.tbli}
        dq_vt_autoinvtab_addheaderrow $tblj "name"
        for k in "${!ifaces[@]}"; do
            print -r -- "${ifaces[$k]}"
        done | sort -t'|' -k1,1 | while read k; do
            dq_vt_autoinvtab_adddatarow $tblj "$k"
        done
        dq_vt_autoinvtab_endsubtable $tblj
    fi

    typeset -n vp=dq_vt_autoinvtab_data.is.si_pname
    [[ $vp == '' ]] && typeset -n vp=dq_vt_autoinvtab_data.is.si_pname
    [[ $vp != '' ]] && dq_vt_autoinvtab_adddatarow $tbli "storage model|$vp"

    if [[ $vp != '' ]] then
        typeset -n vp=dq_vt_autoinvtab_data.is.si_serialnum
        [[ $vp == '' ]] && typeset -n vp=dq_vt_autoinvtab_data.is.si_serialnum
        [[ $vp != '' ]] && dq_vt_autoinvtab_adddatarow $tbli "storage serial number|$vp"

        dq_vt_autoinvtab_beginsubtable $tbli "luns" ''
        tblj=${dq_vt_autoinvtab_data.tbli}
        dq_vt_autoinvtab_addheaderrow $tblj "id|size"
        unset dq_vt_autoinvtab_data._luns; dq_vt_autoinvtab_data._luns=()
        for ik in "${!dq_vt_autoinvtab_data.is.@}"; do
            [[ $ik != *@(si|scopeinv)_lunsize* ]] && continue
            luni=${ik#*_lunsize}
            typeset -n ip=dq_vt_autoinvtab_data.is.si_lunsize$luni
            [[ $ip == '' ]] && typeset -n ip=dq_vt_autoinvtab_data.is.scopeinv_lunsize$luni
            lunsize=$ip
            dq_vt_autoinvtab_adddatarow $tblj "$luni|$lunsize"
        done
        dq_vt_autoinvtab_endsubtable $tblj

        dq_vt_autoinvtab_beginsubtable $tbli "disks" ''
        tblj=${dq_vt_autoinvtab_data.tbli}
        dq_vt_autoinvtab_addheaderrow $tblj "id|size|model name"
        unset dq_vt_autoinvtab_data._hds; dq_vt_autoinvtab_data._hds=()
        for ik in "${!dq_vt_autoinvtab_data.is.@}"; do
            [[ $ik != *@(si|scopeinv)_hwname* ]] && continue
            hdi=${ik#*_hwname}
            typeset -n ip=dq_vt_autoinvtab_data.is.si_hwname$hdi
            [[ $ip == '' ]] && typeset -n ip=dq_vt_autoinvtab_data.is.scopeinv_hwname$hdi
            hdname=$ip
            typeset -n ip=dq_vt_autoinvtab_data.is.si_hwsize$hdi
            [[ $ip == '' ]] && typeset -n ip=dq_vt_autoinvtab_data.is.scopeinv_hwsize$hdi
            hdsize=$ip
            dq_vt_autoinvtab_adddatarow $tblj "$hdi|$hdsize|$hdname"
        done
        dq_vt_autoinvtab_endsubtable $tblj
    fi
}
