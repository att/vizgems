
function dq_vt_autoinvtab_tool_mainila {
    dq_vt_autoinvtab_tool_mainila_begin
    dq_vt_autoinvtab_tool_mainila_body
    dq_vt_autoinvtab_tool_mainila_end
}

function dq_vt_autoinvtab_tool_mainila_begin {
    typeset tbli name

    dq_vt_autoinvtab_tool_default_begin
}

function dq_vt_autoinvtab_tool_mainila_end {
    dq_vt_autoinvtab_tool_default_end
}

function dq_vt_autoinvtab_tool_mainila_body {
    typeset tbli tblj row v ik k
    typeset -A fsnames fssizes ifaces

    dq_vt_autoinvtab_tool_default_body
    tbli=${dq_vt_autoinvtab_tool_default_data.tbli}

    typeset -n vp=dq_vt_autoinvtab_data.is.[arch]
    [[ $vp != '' ]] && dq_vt_autoinvtab_adddatarow $tbli "architecture|$vp"

    typeset -n vp=dq_vt_autoinvtab_data.is.[number_of_cpus]
    [[ $vp != '' ]] && dq_vt_autoinvtab_adddatarow $tbli "number of cpus|$vp"

    typeset -n vp=dq_vt_autoinvtab_data.is.[total_memory]
    [[ $vp != '' ]] && dq_vt_autoinvtab_adddatarow $tbli "total memory|$vp"

    typeset -n vp=dq_vt_autoinvtab_data.is.[total_swap_space]
    [[ $vp != '' ]] && dq_vt_autoinvtab_adddatarow $tbli "total swap space|$vp"

    typeset -n vp=dq_vt_autoinvtab_data.is.[ila_version]
    [[ $vp != '' ]] && dq_vt_autoinvtab_adddatarow $tbli "iLA version|$vp"

    typeset -n vp=dq_vt_autoinvtab_data.is.[ila_modules]
    [[ $vp != '' ]] && dq_vt_autoinvtab_adddatarow $tbli "iLA modules|$vp"

    for ik in "${!dq_vt_autoinvtab_data.is.@}"; do
        if [[ $ik == *si_fs* ]] then
            typeset -n ip=$ik
            k=${ik#dq_vt_autoinvtab_data.is.}
            [[ $k == '['@(*)']' ]] && k=${.sh.match[1]}
            fsnames[${k#si_fs}]=$ip
        fi
        if [[ $ik == *si_fz* ]] then
            typeset -n ip=$ik
            k=${ik#dq_vt_autoinvtab_data.is.}
            [[ $k == '['@(*)']' ]] && k=${.sh.match[1]}
            fssizes[${k#si_fz}]=$ip
        fi
        if [[ $ik == *si_packets* ]] then
            typeset -n ip=$ik
            k=${ik#dq_vt_autoinvtab_data.is.}
            [[ $k == '['@(*)']' ]] && k=${.sh.match[1]}
            ifaces[${k#si_packets}]=$ip
        fi
    done
    if (( ${#fsnames[@]} > 0 )) then
        dq_vt_autoinvtab_beginsubtable $tbli "filesystems" ''
        tblj=${dq_vt_autoinvtab_data.tbli}
        dq_vt_autoinvtab_addheaderrow $tblj "name|size"
        for k in "${!fsnames[@]}"; do
            print -r -- "${fsnames[$k]}|right:${fssizes[$k]}"
        done | sort -t'|' -k1,1 | while read k v; do
            dq_vt_autoinvtab_adddatarow $tblj "$k|$v"
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
}
