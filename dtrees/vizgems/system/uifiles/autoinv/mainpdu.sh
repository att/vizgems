
function dq_vt_autoinvtab_tool_mainpdu {
    dq_vt_autoinvtab_tool_mainpdu_begin
    dq_vt_autoinvtab_tool_mainpdu_body
    dq_vt_autoinvtab_tool_mainpdu_end
}

function dq_vt_autoinvtab_tool_mainpdu_begin {
    typeset tbli name

    dq_vt_autoinvtab_tool_default_begin
}

function dq_vt_autoinvtab_tool_mainpdu_end {
    dq_vt_autoinvtab_tool_default_end
}

function dq_vt_autoinvtab_tool_mainpdu_body {
    typeset tbli tblj ik outleti name status

    dq_vt_autoinvtab_tool_default_body
    tbli=${dq_vt_autoinvtab_tool_default_data.tbli}

    typeset -n ip=dq_vt_autoinvtab_data.is.si_sysname
    [[ $ip == '' ]] && typeset -n ip=dq_vt_autoinvtab_data.is.scopeinv_sysname
    [[ $ip != '' ]] && dq_vt_autoinvtab_adddatarow $tbli "sysname|$ip"

    typeset -n ip=dq_vt_autoinvtab_data.is.si_model
    [[ $ip == '' ]] && typeset -n ip=dq_vt_autoinvtab_data.is.scopeinv_model
    [[ $ip != '' ]] && dq_vt_autoinvtab_adddatarow $tbli "model|$ip"

    typeset -n ip=dq_vt_autoinvtab_data.is.si_firmware
    [[ $ip == '' ]] && typeset -n ip=dq_vt_autoinvtab_data.is.scopeinv_firmware
    [[ $ip != '' ]] && dq_vt_autoinvtab_adddatarow $tbli "firmware|$ip"

    dq_vt_autoinvtab_beginsubtable $tbli "outlets" ''
    tblj=${dq_vt_autoinvtab_data.tbli}
    dq_vt_autoinvtab_addheaderrow $tblj "id|name|status"
    unset dq_vt_autoinvtab_data._outlets; dq_vt_autoinvtab_data._outlets=()
    for ik in "${!dq_vt_autoinvtab_data.is.@}"; do
        [[ $ik != *@(si|scopeinv)_name* ]] && continue
        outleti=${ik#*_name}
        typeset -n ip=dq_vt_autoinvtab_data.is.si_name$outleti
        [[ $ip == '' ]] && typeset -n ip=dq_vt_autoinvtab_data.is.scopeinv_name$outleti
        name=$ip
        typeset -n ip=dq_vt_autoinvtab_data.is.si_status$outleti
        [[ $ip == '' ]] && typeset -n ip=dq_vt_autoinvtab_data.is.scopeinv_status$outleti
        status=$ip
        dq_vt_autoinvtab_adddatarow $tblj "$outleti|$name|$status"
    done
    dq_vt_autoinvtab_endsubtable $tblj
}
