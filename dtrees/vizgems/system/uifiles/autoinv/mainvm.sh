
function dq_vt_autoinvtab_tool_mainvm {
    dq_vt_autoinvtab_tool_mainvm_begin
    dq_vt_autoinvtab_tool_mainvm_body
    dq_vt_autoinvtab_tool_mainvm_end
}

function dq_vt_autoinvtab_tool_mainvm_begin {
    typeset tbli name

    dq_vt_autoinvtab_tool_default_begin
}

function dq_vt_autoinvtab_tool_mainvm_end {
    dq_vt_autoinvtab_tool_default_end
}

function dq_vt_autoinvtab_tool_mainvm_body {
    typeset tbli tblj
    integer cpun=0

    dq_vt_autoinvtab_tool_default_body
    tbli=${dq_vt_autoinvtab_tool_default_data.tbli}
    dq_vt_autoinvtab_adddatarow $tbli "id|${dq_vt_autoinvtab_data.id}"
    cpun=${dq_vt_autoinvtab_data.is.flavorvcpus}
    (( cpun > 0 )) && dq_vt_autoinvtab_adddatarow $tbli "cores|$cpun"
    typeset -n vp=dq_vt_autoinvtab_data.is.status
    [[ $vp != '' ]] && dq_vt_autoinvtab_adddatarow $tbli "status|$vp"
    typeset -n vp=dq_vt_autoinvtab_data.is.flavorram
    [[ $vp != '' ]] && dq_vt_autoinvtab_adddatarow $tbli "memory|$vp MB"
    typeset -n vp=dq_vt_autoinvtab_data.is.flavordisk
    [[ $vp != '' ]] && dq_vt_autoinvtab_adddatarow $tbli "disk|$vp GB"
    typeset -n vp=dq_vt_autoinvtab_data.is.flavorswap
    [[ $vp != '' ]] && dq_vt_autoinvtab_adddatarow $tbli "swap|$vp GB"
    typeset -n vp=dq_vt_autoinvtab_data.is.hypervisor
    [[ $vp != '' ]] && dq_vt_autoinvtab_adddatarow $tbli "host|$vp"
    typeset -n vp=dq_vt_autoinvtab_data.is.image
    [[ $vp != '' ]] && dq_vt_autoinvtab_adddatarow $tbli "image|$vp"
    typeset -n vp=dq_vt_autoinvtab_data.is.flavorname
    [[ $vp != '' ]] && dq_vt_autoinvtab_adddatarow $tbli "flavor|$vp"
    typeset -n vp=dq_vt_autoinvtab_data.is.created
    [[ $vp != '' ]] && dq_vt_autoinvtab_adddatarow $tbli "created|$vp"
    typeset -n vp=dq_vt_autoinvtab_data.is.updated
    [[ $vp != '' ]] && dq_vt_autoinvtab_adddatarow $tbli "updated|$vp"
    typeset -n vp=dq_vt_autoinvtab_data.is.launched
    [[ $vp != '' ]] && dq_vt_autoinvtab_adddatarow $tbli "launched|$vp"
    typeset -n vp=dq_vt_autoinvtab_data.is.terminated
    [[ $vp != '' ]] && dq_vt_autoinvtab_adddatarow $tbli "terminated|$vp"
    typeset -n vp=dq_vt_autoinvtab_data.is.keyname
    [[ $vp != '' ]] && dq_vt_autoinvtab_adddatarow $tbli "key name|$vp"

    typeset -n vp=dq_vt_autoinvtab_data.is.ip_addr0
    if [[ $vp != '' ]] then
        dq_vt_autoinvtab_beginsubtable $tbli "ip addresses" ''
        tblj=${dq_vt_autoinvtab_data.tbli}
        dq_vt_autoinvtab_addheaderrow $tblj "ip|mac|type"
        for (( i = 0; ; i++ )) do
            typeset -n ip=dq_vt_autoinvtab_data.is.ip_addr$i
            [[ $ip == '' ]] && break
            typeset -n mp=dq_vt_autoinvtab_data.is.ip_mac$i
            typeset -n tp=dq_vt_autoinvtab_data.is.ip_type$i
            dq_vt_autoinvtab_adddatarow $tblj "$ip|$mp|$tp"
        done
        dq_vt_autoinvtab_endsubtable $tblj
    fi
}
