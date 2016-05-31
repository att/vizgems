
function dq_vt_autoinvtab_tool_mainsmartctl {
    dq_vt_autoinvtab_tool_mainsmartctl_begin
    dq_vt_autoinvtab_tool_mainsmartctl_body
    dq_vt_autoinvtab_tool_mainsmartctl_end
}

function dq_vt_autoinvtab_tool_mainsmartctl_begin {
    typeset tbli name

    dq_vt_autoinvtab_tool_default_begin
}

function dq_vt_autoinvtab_tool_mainsmartctl_end {
    dq_vt_autoinvtab_tool_default_end
}

function dq_vt_autoinvtab_tool_mainsmartctl_body {
    typeset tbli tblj ik propi propl attri attrl ifs ls ln

    dq_vt_autoinvtab_tool_default_body
    tbli=${dq_vt_autoinvtab_tool_default_data.tbli}

    unset dq_vt_autoinvtab_data._devs; typeset -A dq_vt_autoinvtab_data._devs
    unset dq_vt_autoinvtab_data._props; typeset -A dq_vt_autoinvtab_data._props
    unset dq_vt_autoinvtab_data._attrs; typeset -A dq_vt_autoinvtab_data._attrs
    for ik in "${!dq_vt_autoinvtab_data.is.@}"; do
        typeset -n ip=$ik
        [[ $ik == @(*).'['@(*)']' ]] && ik=${.sh.match[1]}.${.sh.match[2]}
        case $ik in
        *.devid*)
            dq_vt_autoinvtab_data._devs[${ik##*.devid}]=$ip
            ;;
        *.prop_*)
            dq_vt_autoinvtab_data._props[${ik##*.prop_}]=$ip
            ;;
        *.attr_*)
            dq_vt_autoinvtab_data._attrs[${ik##*.attr_}]=$ip
            ;;
        esac
    done

    for devi in "${!dq_vt_autoinvtab_data._devs[@]}"; do
        typeset -n devr=dq_vt_autoinvtab_data._devs[$devi]
        dq_vt_autoinvtab_adddatarow $tbli "disk|$devr"
        dq_vt_autoinvtab_beginsubtable $tbli "$devr properties" ''
        tblj=${dq_vt_autoinvtab_data.tbli}
        dq_vt_autoinvtab_addheaderrow $tblj "name|value"
        for propi in "${!dq_vt_autoinvtab_data._props[@]}"; do
            [[ $propi != *_$devi ]] && continue
            propl=${propi%_$devi}
            propl=${propl//_/' '}
            typeset -n propr=dq_vt_autoinvtab_data._props[$propi]
            dq_vt_autoinvtab_adddatarow $tblj "$propl|$propr"
        done
        dq_vt_autoinvtab_endsubtable $tblj

        dq_vt_autoinvtab_beginsubtable $tbli "$devr attributes" ''
        tblj=${dq_vt_autoinvtab_data.tbli}
        dq_vt_autoinvtab_addheaderrow $tblj "Id|Attr Name|Flag|Value|Worst|Thresh|Type|Updated|When Failed|Raw Value"
        for attri in "${!dq_vt_autoinvtab_data._attrs[@]}"; do
            [[ $attri != *_$devi ]] && continue
            typeset -n attrr=dq_vt_autoinvtab_data._attrs[$attri]
            ifs="$IFS"
            IFS=':'
            set -f
            set -A ls -- $attrr
            set +f
            IFS="$ifs"
            ln=${#ls[@]}
            attrl="${ls[0]}|${ls[1]}|${ls[2]}|${ls[3]}|${ls[4]}|${ls[5]}|${ls[6]}|${ls[7]}|${ls[8]}|${ls[9..$(( ln - 1 ))]}"
            print -r "$attrl"
        done | sort -t'|' -n -k1,1 | while read attrl; do
            dq_vt_autoinvtab_adddatarow $tblj "$attrl"
        done
        dq_vt_autoinvtab_endsubtable $tblj
    done
}
