dq_vt_autoinvtab_tool_default_data=(
    tbli=-1
)

function dq_vt_autoinvtab_tool_default {
    dq_vt_autoinvtab_tool_default_begin
    dq_vt_autoinvtab_tool_default_body
    dq_vt_autoinvtab_tool_default_end
}

function dq_vt_autoinvtab_tool_default_begin {
    typeset tbli name

    dq_vt_autoinvtab_begintable
    dq_vt_autoinvtab_tool_default_data.tbli=${dq_vt_autoinvtab_data.tbli}
    tbli=${dq_vt_autoinvtab_tool_default_data.tbli}
    dq_vt_autoinvtab_data.roottbli=$tbli
    name=${dq_vt_autoinvtab_data.is.name:-${dq_vt_autoinvtab_data.id}}
    dq_vt_autoinvtab_addcaption $tbli $name
}

function dq_vt_autoinvtab_tool_default_end {
    typeset tbli tblj showuser st sl fi flab ei elab ti tlab ts pi plab ps
    typeset asl ast j1 mn j2 j3 j4 j5 j6 j7 alarm palarm estat

    tbli=${dq_vt_autoinvtab_tool_default_data.tbli}

    if swmuseringroups "vg_att_*"; then
        showuser=y
    fi
    st=${dq_vt_autoinvtab_data.st}
    sl=${dq_vt_autoinvtab_data.sl}
    if [[ ${dq_vt_autoinvtab_data.sections} == *F* ]] then
        flab='asset|alarm id|text|ticket mode|viz mode|severity|start time'
        flab+='|end time|start date|end date|repeat|comments|account'
        [[ $showuser != y ]] && flab=${flab%'|'*}
        set -o noglob
        dq_vt_autoinvtab_beginsubtable $tbli "filters" ''
        tblj=${dq_vt_autoinvtab_data.tbli}
        dq_vt_autoinvtab_addheaderrow $tblj "$flab"
        for (( fi = 0; fi < dq_vt_autoinvtab_data.fn; fi++ )) do
            typeset -n fp=dq_vt_autoinvtab_data.fs.[$fi]
            if [[ $showuser != y ]] then
                dq_vt_autoinvtab_adddatarow $tblj "${fp%'|'*}"
            else
                dq_vt_autoinvtab_adddatarow $tblj "$fp"
            fi
        done
        dq_vt_autoinvtab_endsubtable $tblj
        set +o noglob
    fi
    if [[ ${dq_vt_autoinvtab_data.sections} == *E* ]] then
        elab='asset|alarm id|text|ticket mode|severity|start time|end time'
        elab+='|start date|end date|repeat|from|to|subject|style|account'
        [[ $showuser != y ]] && elab=${elab%'|'*}
        set -o noglob
        dq_vt_autoinvtab_beginsubtable $tbli "emails" ''
        tblj=${dq_vt_autoinvtab_data.tbli}
        dq_vt_autoinvtab_addheaderrow $tblj "$elab"
        for (( ei = 0; ei < dq_vt_autoinvtab_data.en; ei++ )) do
            typeset -n ep=dq_vt_autoinvtab_data.es.[$ei]
            if [[ $showuser != y ]] then
                dq_vt_autoinvtab_adddatarow $tblj "${ep%'|'*}"
            else
                dq_vt_autoinvtab_adddatarow $tblj "$ep"
            fi
        done
        dq_vt_autoinvtab_endsubtable $tblj
        set +o noglob
    fi
    if [[ ${dq_vt_autoinvtab_data.sections} == *P* ]] then
        plab='asset|name|status|value|account'
        [[ $showuser != y ]] && plab=${plab%'|'*}
        set -o noglob
        dq_vt_autoinvtab_beginsubtable $tbli "profiles" ''
        tblj=${dq_vt_autoinvtab_data.tbli}
        dq_vt_autoinvtab_addheaderrow $tblj "$plab"
        for (( pi = 0; pi < dq_vt_autoinvtab_data.pn; pi++ )) do
            typeset -n pp=dq_vt_autoinvtab_data.ps.[$pi]
            if [[ $showuser != y ]] then
                dq_vt_autoinvtab_adddatarow $tblj "${pp%'|'*}"
            else
                dq_vt_autoinvtab_adddatarow $tblj "$pp"
            fi
        done
        while read -r asl ast j1 mn j2 j3 j4 j5 j6 palarm estat; do
            [[ $asl != $sl || $ast != $st || $palarm == '' ]] && continue
            if [[ $showuser != y ]] then
                ps="default|$mn|keep|$palarm"
            else
                ps="default|$mn|keep|$palarm|"
            fi
            dq_vt_autoinvtab_adddatarow $tblj "$ps"
        done < $SWIFTDATADIR/dpfiles/stat/parameter.txt
        dq_vt_autoinvtab_endsubtable $tblj
        set +o noglob
    fi
    if [[ ${dq_vt_autoinvtab_data.sections} == *T* ]] then
        tlab='asset|name|status|value|account'
        [[ $showuser != y ]] && tlab=${tlab%'|'*}
        set -o noglob
        dq_vt_autoinvtab_beginsubtable $tbli "thresholds" ''
        tblj=${dq_vt_autoinvtab_data.tbli}
        dq_vt_autoinvtab_addheaderrow $tblj "$tlab"
        for (( ti = 0; ti < dq_vt_autoinvtab_data.tn; ti++ )) do
            typeset -n tp=dq_vt_autoinvtab_data.ts.[$ti]
            if [[ $showuser != y ]] then
                dq_vt_autoinvtab_adddatarow $tblj "${tp%'|'*}"
            else
                dq_vt_autoinvtab_adddatarow $tblj "$tp"
            fi
        done
        while read -r asl ast j1 mn j2 j3 j4 j5 alarm j6 j7; do
            [[ $asl != $sl || $ast != $st || $alarm == '' ]] && continue
            if [[ $showuser != y ]] then
                ts="default|$mn|keep|$alarm"
            else
                ts="default|$mn|keep|$alarm|"
            fi
            dq_vt_autoinvtab_adddatarow $tblj "$ts"
        done < $SWIFTDATADIR/dpfiles/stat/parameter.txt
        dq_vt_autoinvtab_endsubtable $tblj
        set +o noglob
    fi

    dq_vt_autoinvtab_endtable ${dq_vt_autoinvtab_tool_default_data.tbli}
}

function dq_vt_autoinvtab_tool_default_body {
    typeset tbli row v ik k

    tbli=${dq_vt_autoinvtab_tool_default_data.tbli}
    v=${dq_vt_autoinvtab_data.is.name:-${dq_vt_autoinvtab_data.id}}
    row="name|$v"
    dq_vt_autoinvtab_adddatarow $tbli "$row"
    v=${dq_vt_autoinvtab_data.is.alias}
    [[ $v != '' ]] && dq_vt_autoinvtab_adddatarow $tbli "alias|$v"
    v=${dq_vt_autoinvtab_data.is.info}
    [[ $v != '' ]] && dq_vt_autoinvtab_adddatarow $tbli "info|$v"
    if [[ ${dq_vt_autoinvtab_data.is.trans0} != '' ]] then
        for ik in "${!dq_vt_autoinvtab_data.is.trans@}"; do
            typeset -n ip=$ik
            dq_vt_autoinvtab_adddatarow $tbli "translation|$ip"
        done
    fi
    v=${dq_vt_autoinvtab_data.is.scope_systype}
    [[ $v != '' ]] && dq_vt_autoinvtab_adddatarow $tbli "type|$v"
    v=${dq_vt_autoinvtab_data.is.si_ident}
    [[ $v == '' ]] && v=${dq_vt_autoinvtab_data.is.scopeinv_ident}
    [[ $v != '' ]] && dq_vt_autoinvtab_adddatarow $tbli "identity|$v"
    v=${dq_vt_autoinvtab_data.is.si_uptime}
    [[ $v == '' ]] && v=${dq_vt_autoinvtab_data.is.scopeinv_uptime}
    [[ $v != '' ]] && dq_vt_autoinvtab_adddatarow $tbli "uptime|$v"
    if swmuseringroups "vg_att_*"; then
        v=${dq_vt_autoinvtab_data.is.scope_ip}
        [[ $v != '' ]] && dq_vt_autoinvtab_adddatarow $tbli "monitoring ip|$v"
    fi
    v=${dq_vt_autoinvtab_data.is.si_timestamp}
    [[ $v == '' ]] && v=${dq_vt_autoinvtab_data.is.scopeinv_timestamp}
    if [[ $v == [0-9]* ]] then
        v=$(TZ=$PHTZ printf '%T' \#$v)
        dq_vt_autoinvtab_adddatarow $tbli "last inventoried|$v"
    fi
}
