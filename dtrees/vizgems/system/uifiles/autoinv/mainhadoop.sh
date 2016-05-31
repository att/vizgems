
function dq_vt_autoinvtab_tool_mainhadoop {
    dq_vt_autoinvtab_tool_mainhadoop_begin
    dq_vt_autoinvtab_tool_mainhadoop_body
    dq_vt_autoinvtab_tool_mainhadoop_end
}

function dq_vt_autoinvtab_tool_mainhadoop_begin {
    typeset tbli name

    dq_vt_autoinvtab_tool_default_begin
}

function dq_vt_autoinvtab_tool_mainhadoop_end {
    dq_vt_autoinvtab_tool_default_end
}

function dq_vt_autoinvtab_tool_mainhadoop_body {
    typeset tbli tblj ik k
    typeset -A queues nodes

    dq_vt_autoinvtab_tool_default_body
    tbli=${dq_vt_autoinvtab_tool_default_data.tbli}

    typeset -n vp=dq_vt_autoinvtab_data.is.[si_id]
    [[ $vp != '' ]] && dq_vt_autoinvtab_adddatarow $tbli "id|$vp"

    typeset -n vp=dq_vt_autoinvtab_data.is.[si_startedon]
    [[ $vp != '' ]] && dq_vt_autoinvtab_adddatarow $tbli "started on|$vp"

    typeset -n vp=dq_vt_autoinvtab_data.is.[si_state._total]
    [[ $vp != '' ]] && dq_vt_autoinvtab_adddatarow $tbli "state|$vp"

    typeset -n vp=dq_vt_autoinvtab_data.is.[si_hastate]
    [[ $vp != '' ]] && dq_vt_autoinvtab_adddatarow $tbli "ha state|$vp"

    typeset -n vp=dq_vt_autoinvtab_data.is.[si_version._total]
    [[ $vp != '' ]] && dq_vt_autoinvtab_adddatarow $tbli "version|$vp"

    typeset -n vp=dq_vt_autoinvtab_data.is.[si_rmversion]
    [[ $vp != '' ]] && dq_vt_autoinvtab_adddatarow $tbli "rm version|$vp"

    typeset -n vp=dq_vt_autoinvtab_data.is.[si_sz_memory_used._total]
    [[ $vp != '' ]] && dq_vt_autoinvtab_adddatarow $tbli "memory|$vp GB"

    typeset -n vp=dq_vt_autoinvtab_data.is.[si_sz_nodes_active._total]
    [[ $vp != '' ]] && dq_vt_autoinvtab_adddatarow $tbli "# of nodes|$vp"

    for ik in "${!dq_vt_autoinvtab_data.is.@}"; do
        if [[ $ik == *si_queue* ]] then
            typeset -n ip=$ik
            k=${ik#dq_vt_autoinvtab_data.is.}
            [[ $k == '['@(*)']' ]] && k=${.sh.match[1]}
            queues[${k#si_queue}].name=$ip
        fi
        if [[ $ik == *si_sz_queuecapacity_used.* ]] then
            typeset -n ip=$ik
            k=${ik#dq_vt_autoinvtab_data.is.}
            [[ $k == '['@(*)']' ]] && k=${.sh.match[1]}
            queues[${k#si_sz_queuecapacity_used.}].capacity=$ip
        fi
        if [[ $ik == *si_sz_queueapps_used.* ]] then
            typeset -n ip=$ik
            k=${ik#dq_vt_autoinvtab_data.is.}
            [[ $k == '['@(*)']' ]] && k=${.sh.match[1]}
            queues[${k#si_sz_queueapps_used.}].apps=$ip
        fi
    done
    if (( ${#queues[@]} > 0 )) then
        dq_vt_autoinvtab_beginsubtable $tbli "queues" ''
        tblj=${dq_vt_autoinvtab_data.tbli}
        dq_vt_autoinvtab_addheaderrow $tblj "name|capacity|apps"
        for k in "${!queues[@]}"; do
            print -r -- "${queues[$k].name}|right:${queues[$k].capacity}|right:${queues[$k].apps}"
        done | sort -t'|' -k1,1 | while read k v; do
            dq_vt_autoinvtab_adddatarow $tblj "$k|$v"
        done
        dq_vt_autoinvtab_endsubtable $tblj
    fi

    for ik in "${!dq_vt_autoinvtab_data.is.@}"; do
        if [[ $ik == *si_nodeid* ]] then
            typeset -n ip=$ik
            k=${ik#dq_vt_autoinvtab_data.is.}
            [[ $k == '['@(*)']' ]] && k=${.sh.match[1]}
            nodes[${k#si_nodeid}].id=$ip
        fi
        if [[ $ik == *si_nodename* ]] then
            typeset -n ip=$ik
            k=${ik#dq_vt_autoinvtab_data.is.}
            [[ $k == '['@(*)']' ]] && k=${.sh.match[1]}
            nodes[${k#si_nodename}].name=$ip
        fi
        if [[ $ik == *si_rack* ]] then
            typeset -n ip=$ik
            k=${ik#dq_vt_autoinvtab_data.is.}
            [[ $k == '['@(*)']' ]] && k=${.sh.match[1]}
            nodes[${k#si_rack}].rack=$ip
        fi
        if [[ $ik == *si_state* ]] then
            typeset -n ip=$ik
            k=${ik#dq_vt_autoinvtab_data.is.}
            [[ $k == '['@(*)']' ]] && k=${.sh.match[1]}
            [[ $k == *._total ]] && continue
            nodes[${k#si_state}].state=$ip
        fi
        if [[ $ik == *si_version* ]] then
            typeset -n ip=$ik
            k=${ik#dq_vt_autoinvtab_data.is.}
            [[ $k == '['@(*)']' ]] && k=${.sh.match[1]}
            [[ $k == *._total ]] && continue
            nodes[${k#si_version}].version=$ip
        fi
    done
    if (( ${#nodes[@]} > 0 )) then
        dq_vt_autoinvtab_beginsubtable $tbli "nodes" ''
        tblj=${dq_vt_autoinvtab_data.tbli}
        dq_vt_autoinvtab_addheaderrow $tblj "name|rack|state|version"
        for k in "${!nodes[@]}"; do
            print -r -- "${nodes[$k].name}|${nodes[$k].rack}|${nodes[$k].state}|${nodes[$k].version}"
        done | sort -t'|' -k1,1 | while read k v; do
            dq_vt_autoinvtab_adddatarow $tblj "$k|$v"
        done
        dq_vt_autoinvtab_endsubtable $tblj
    fi

}
