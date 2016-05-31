
function dq_vt_autoinvtab_tool_mainstorage {
    dq_vt_autoinvtab_tool_mainstorage_begin
    dq_vt_autoinvtab_tool_mainstorage_body
    dq_vt_autoinvtab_tool_mainstorage_end
}

function dq_vt_autoinvtab_tool_mainstorage_begin {
    typeset tbli name

    dq_vt_autoinvtab_tool_default_begin
}

function dq_vt_autoinvtab_tool_mainstorage_end {
    dq_vt_autoinvtab_tool_default_end
}

function dq_vt_autoinvtab_tool_mainstorage_body {
    typeset tbli tblj ik luni lunsize hdi hdname hdinfo fw sn mn

    dq_vt_autoinvtab_tool_default_body
    tbli=${dq_vt_autoinvtab_tool_default_data.tbli}

    typeset -n ip=dq_vt_autoinvtab_data.is.si_sysname
    [[ $ip == '' ]] && typeset -n ip=dq_vt_autoinvtab_data.is.scopeinv_sysname
    [[ $ip != '' ]] && dq_vt_autoinvtab_adddatarow $tbli "sysname|$ip"

    typeset -n ip=dq_vt_autoinvtab_data.is.si_vendor
    [[ $ip == '' ]] && typeset -n ip=dq_vt_autoinvtab_data.is.scopeinv_vendor
    [[ $ip != '' ]] && dq_vt_autoinvtab_adddatarow $tbli "vendor|$ip"

    typeset -n ip=dq_vt_autoinvtab_data.is.si_model
    [[ $ip == '' ]] && typeset -n ip=dq_vt_autoinvtab_data.is.scopeinv_model
    [[ $ip != '' ]] && dq_vt_autoinvtab_adddatarow $tbli "model|$ip"

    typeset -n ip=dq_vt_autoinvtab_data.is.si_sysid
    [[ $ip == '' ]] && typeset -n ip=dq_vt_autoinvtab_data.is.scopeinv_sysid
    [[ $ip != '' ]] && dq_vt_autoinvtab_adddatarow $tbli "unit id|$ip"

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
    dq_vt_autoinvtab_addheaderrow $tblj "slot|size|firmware version|serial number|model name"
    unset dq_vt_autoinvtab_data._hds; dq_vt_autoinvtab_data._hds=()
    for ik in "${!dq_vt_autoinvtab_data.is.@}"; do
        [[ $ik != *@(si|scopeinv)_hwname* ]] && continue
        print ${ik#*_hwname}
    done | sort -n | while read hdi; do
        typeset -n ip=dq_vt_autoinvtab_data.is.si_hwname$hdi
        [[ $ip == '' ]] && typeset -n ip=dq_vt_autoinvtab_data.is.scopeinv_hwname$hdi
        hdname=$ip
        typeset -n ip=dq_vt_autoinvtab_data.is.si_hwinfo$hdi
        [[ $ip == '' ]] && typeset -n ip=dq_vt_autoinvtab_data.is.scopeinv_hwinfo$hdi
        hdinfo=${ip#:FW:}
        fw=${hdinfo%%:SN:*}; hdinfo=${hdinfo#*:SN:}
        sn=${hdinfo%%:MN:*}; hdinfo=${hdinfo#*:MN:}
        mn=${hdinfo}
        dq_vt_autoinvtab_adddatarow $tblj "$hdi|$hdname|$fw|$sn|$mn"
    done
    dq_vt_autoinvtab_endsubtable $tblj
}
