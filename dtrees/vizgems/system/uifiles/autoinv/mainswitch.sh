
function dq_vt_autoinvtab_tool_mainswitch {
    dq_vt_autoinvtab_tool_mainswitch_begin
    dq_vt_autoinvtab_tool_mainswitch_body
    dq_vt_autoinvtab_tool_mainswitch_end
}

function dq_vt_autoinvtab_tool_mainswitch_begin {
    typeset tbli name

    dq_vt_autoinvtab_tool_default_begin
}

function dq_vt_autoinvtab_tool_mainswitch_end {
    dq_vt_autoinvtab_tool_default_end
}

function dq_vt_autoinvtab_tool_mainswitch_body {
    typeset tbli tblj row v ik k ikey itype iname iid pid pindex line
    integer cpun=0 tun=0 tdn=0 ttn=0
    typeset -Z4 z
    typeset -A ifkeys sumkeys
    typeset -A hwhaveps
    typeset hwi hwls hwroot

    unset dq_vt_autoinvtab_data._ifs; dq_vt_autoinvtab_data._ifs=()
    unset dq_vt_autoinvtab_data._sums; dq_vt_autoinvtab_data._sums=()
    dq_vt_autoinvtab_tool_default_body
    tbli=${dq_vt_autoinvtab_tool_default_data.tbli}
    for ik in "${!dq_vt_autoinvtab_data.is.si_cpu@}"; do
        [[ $ik == *cpu[0-9]* ]] && (( cpun++ ))
    done
    for ik in "${!dq_vt_autoinvtab_data.is.scopeinv_cpu@}"; do
        [[ $ik == *cpu[0-9]* ]] && (( cpun++ ))
    done
    (( cpun > 0 )) && dq_vt_autoinvtab_adddatarow $tbli "cpus|$cpun"

    typeset -n vp=dq_vt_autoinvtab_data.is.si_sysname
    [[ $vp == '' ]] && typeset -n vp=dq_vt_autoinvtab_data.is.scopeinv_sysname
    [[ $vp != '' ]] && dq_vt_autoinvtab_adddatarow $tbli "sysname|$vp"

    typeset -n vp=dq_vt_autoinvtab_data.is.[si_sz_memory_used._total]
    [[ $vp == '' ]] && typeset -n vp=dq_vt_autoinvtab_data.is.[scopeinv_size_memory_used._total]
    [[ $vp != '' ]] && dq_vt_autoinvtab_adddatarow $tbli "memory|$vp MB"

    typeset -n vp=dq_vt_autoinvtab_data.is.[si_sz_swap_used._total]
    [[ $vp == '' ]] && typeset -n vp=dq_vt_autoinvtab_data.is.[scopeinv_size_swap_used._total]
    [[ $vp != '' ]] && dq_vt_autoinvtab_adddatarow $tbli "swap|$vp MB"

    for ik in "${!dq_vt_autoinvtab_data.is.@}"; do
        [[ $ik != *@(si|scopeinv)_istatus* ]] && continue
        ifi=${ik#*_istatus}
        ifkeys[$ifi]=$ifi
        typeset -n ifp=dq_vt_autoinvtab_data._ifs.if$ifi
        typeset -n ip=dq_vt_autoinvtab_data.is.si_istatus$ifi
        [[ $ip == '' ]] && typeset -n ip=dq_vt_autoinvtab_data.is.scopeinv_istatus$ifi
        ifp.status=$ip
        typeset -n ip=dq_vt_autoinvtab_data.is.si_iface$ifi
        [[ $ip == '' ]] && typeset -n ip=dq_vt_autoinvtab_data.is.scopeinv_iface$ifi
        ifp.index=$ip
        typeset -n ip=dq_vt_autoinvtab_data.is.si_iname$ifi
        [[ $ip == '' ]] && typeset -n ip=dq_vt_autoinvtab_data.is.scopeinv_iname$ifi
        ifp.name=$ip
        typeset -n ip=dq_vt_autoinvtab_data.is.si_idescr$ifi
        [[ $ip == '' ]] && typeset -n ip=dq_vt_autoinvtab_data.is.scopeinv_idescr$ifi
        ifp.descr=$ip
        typeset -n ip=dq_vt_autoinvtab_data.is.si_itype$ifi
        [[ $ip == '' ]] && typeset -n ip=dq_vt_autoinvtab_data.is.scopeinv_itype$ifi
        ifp.type=$ip
        typeset -n ip=dq_vt_autoinvtab_data.is.si_ialias$ifi
        [[ $ip == '' ]] && typeset -n ip=dq_vt_autoinvtab_data.is.scopeinv_ialias$ifi
        ifp.alias=$ip
        typeset -n ip=dq_vt_autoinvtab_data.is.si_ispeed$ifi
        [[ $ip == '' ]] && typeset -n ip=dq_vt_autoinvtab_data.is.scopeinv_ispeed$ifi
        ifp.speed=$ip
        if [[ ${ifp.status} == '' ]] then
            if [[ ${ifp.index} != '' ]] then
                ifp.status=up
            else
                ifp.status=down
            fi
        fi
        if [[ ${ifp.descr} == *Ethernet* ]] then
            itype=${ifp.descr%%thernet*}thernet
            iid=${ifp.descr##*thernet}
            pid=${iid#*/}
            if [[ $pid == +([0-9])/+([0-9]) ]] then
                (( pindex = ${pid%/*} * 10000 + ${pid#*/} ))
            elif [[ $pid == +([0-9]) ]] then
                (( pindex = pid ))
            else
                pindex=0
            fi
            iid=${iid%%/*}
            iname=$itype$iid
            if [[ $iid == +([0-9]) ]] then
                z=$iid
                iid=$z
            fi
            sumi=$itype$iid
            z=$pindex
            ikey=$itype$iid$z
        elif [[ ${ifp.descr} == *ethernet* ]] then
            pid=
            pindex=0
            iname=${ifp.descr#*-}
            sumi=$iname
            ikey=$iname
        else
            ikey=${ifp.descr}
        fi
        ifp.key=$ikey
        if [[ ${ifp.speed} != '' ]] then
            if (( ${ifp.speed} >= 1000000000 )) then
                (( ifp.speed /= 1000000000.0 ))
                ifp.speed+=' Gbps'
            elif (( ${ifp.speed} >= 1000000 )) then
                (( ifp.speed /= 1000000.0 ))
                ifp.speed+=' Mbps'
            fi
            [[ ${ifp.speed} == *.+(0) ]] && ifp.speed=${ifp.speed%.*}
        fi
        [[ ${ifp.descr} != *[Ee]thernet* ]] && continue

        sumi=${sumi//./_}
        sumkeys[$sumi]=$sumi
        typeset -n sump=dq_vt_autoinvtab_data._sums.[$sumi]
        if [[ ${sump.tn} == '' ]] then
            sump=(
                name=$iname un=0 dn=0 tn=0
                fii=$pindex lii=$pindex finame=$pid liname=$pid
            )
        fi
        if (( ${sump.fii} > pindex )) then
            sump.fii=$pindex
            sump.finame=$pid
        fi
        if (( ${sump.lii} < pindex )) then
            sump.lii=$pindex
            sump.liname=$pid
        fi
        if [[ ${ifp.status} == up ]] then
            (( sump.tn++ ))
            (( sump.un++ ))
            (( tun++ ))
            (( ttn++ ))
        else
            (( sump.tn++ ))
            (( sump.dn++ ))
            (( tdn++ ))
            (( ttn++ ))
        fi
    done
    if (( $ttn > 0 )) then
        dq_vt_autoinvtab_beginsubtable $tbli "port summary" ''
        tblj=${dq_vt_autoinvtab_data.tbli}
        dq_vt_autoinvtab_addheaderrow $tblj "interface|port range|port count|ports up|ports down"
        for sumi in "${sumkeys[@]}"; do
            typeset -n sump=dq_vt_autoinvtab_data._sums.[$sumi]
            print -r -- "${sump.name}|${sump.finame}-${sump.liname}|right:${sump.tn}|right:${sump.un}|right:${sump.dn}"
        done | sort -t'|' -k1,1 | while read -r line; do
            dq_vt_autoinvtab_adddatarow $tblj "$line"
        done
        dq_vt_autoinvtab_adddatarow $tblj "${#sumkeys[@]} interfaces||right:$ttn|right:$tun|right:$tdn"
        dq_vt_autoinvtab_endsubtable $tblj
    fi
    if (( ${#ifkeys[@]} > 0 )) then
        dq_vt_autoinvtab_beginsubtable $tbli "ports" ''
        tblj=${dq_vt_autoinvtab_data.tbli}
        dq_vt_autoinvtab_addheaderrow $tblj "name|description|alias|speed|status"
        for ifi in "${ifkeys[@]}"; do
            typeset -n ifp=dq_vt_autoinvtab_data._ifs.if$ifi
            print -r -- "${ifp.key}|${ifp.name}|${ifp.descr}|${ifp.alias}|right:${ifp.speed}|${ifp.status}"
        done | sort -t'|' -k1,1 | while read -r line; do
            dq_vt_autoinvtab_adddatarow $tblj "${line#*'|'}"
        done
        dq_vt_autoinvtab_endsubtable $tblj
    fi

    [[ $SHOWFILTERS == 0 ]] && return 0

    unset dq_vt_autoinvtab_data._hwnames; dq_vt_autoinvtab_data._hwnames=()
    unset dq_vt_autoinvtab_data._hwinfos; dq_vt_autoinvtab_data._hwinfos=()
    unset dq_vt_autoinvtab_data._hwcois; dq_vt_autoinvtab_data._hwcois=()
    for ik in "${!dq_vt_autoinvtab_data.is.@}"; do
        [[ $ik != *@(si|scopeinv)_hwname* ]] && continue
        hwi=${ik#*_hwname}
        typeset -n ip=dq_vt_autoinvtab_data.is.si_hwname$hwi
        [[ $ip == '' ]] && typeset -n ip=dq_vt_autoinvtab_data.is.scopeinv_hwname$hwi
        dq_vt_autoinvtab_data._hwnames[$hwi]=$ip
        [[ ${hwhaveps[$hwi]} == '' ]] && hwhaveps[$hwi]=no
        typeset -n ip=dq_vt_autoinvtab_data.is.si_hwinfo$hwi
        [[ $ip == '' ]] && typeset -n ip=dq_vt_autoinvtab_data.is.scopeinv_hwinfo$hwi
        dq_vt_autoinvtab_data._hwinfos[$hwi]=$ip
        typeset -n ip=dq_vt_autoinvtab_data.is.si_hwloc$hwi
        [[ $ip == '' ]] && typeset -n ip=dq_vt_autoinvtab_data.is.scopeinv_hwloc$hwi
        set -f
        set -A hwls -- ${ip//:/'|'}
        set +f
        (( ${hwls[1]} < 0 )) && continue
        dq_vt_autoinvtab_data._hwcois[${hwls[0]}]+="|${hwls[2]}:${hwls[1]}:$hwi"
        hwhaveps[$hwi]=yes
    done
    if (( ${#hwhaveps[@]} > 0 )) then
        for hwi in "${!hwhaveps[@]}"; do
            [[ ${hwhaveps[$hwi]} != no ]] && continue
            if [[ $hwroot != '' ]] then
                print -u2 multiple roots: $hwroot $hwi
                continue
            fi
            hwroot=$hwi
        done
        dq_vt_autoinvtab_beginsubtable $tbli "hardware components" ''
        tblj=${dq_vt_autoinvtab_data.tbli}
        dq_vt_autoinvtab_addheaderrow $tblj "name|hardware version|firmware version|software version|serial number|model name"
        dq_vt_autoinvtab_tool_mainswitch_printhw $tblj $hwroot 0
        dq_vt_autoinvtab_endsubtable $tblj
    fi
}

function dq_vt_autoinvtab_tool_mainswitch_printhw { # $1=tbli $2=hwi $3=level
    typeset tbli=$1 hwi=$2 level=$3
    typeset name info hw fw sw sn mn coi ls o1s o1 o2s o2 oi i

    name=${dq_vt_autoinvtab_data._hwnames[$hwi]}
    (( level > 1 )) && name="&nbsp;&nbsp;&nbsp;&nbsp;$name"
    info=${dq_vt_autoinvtab_data._hwinfos[$hwi]#:HW:}
    hw=${info%%:FW:*}; info=${info#*:FW:}
    fw=${info%%:SW:*}; info=${info#*:SW:}
    sw=${info%%:SN:*}; info=${info#*:SN:}
    sn=${info%%:MN:*}; info=${info#*:MN:}
    mn=${info}
    [[ $hw$fw$sw$sn$mn != '' || $level == 1 ]] && dq_vt_autoinvtab_adddatarow $tbli "$name|$hw|$fw|$sw|$sn|$mn"

    for coi in ${dq_vt_autoinvtab_data._hwcois[$hwi]#'|'}; do
        set -f
        set -A ls -- ${coi//:/'|'}
        set +f
        o1s[${ls[0]}]+="|${ls[1]}:${ls[2]}"
    done
    for o1 in "${o1s[@]}"; do
        unset o2s; typeset o2s
        for oi in ${o1#'|'}; do
            set -f
            set -A ls -- ${oi//:/'|'}
            set +f
            o2s[${ls[0]}]+="|${ls[1]}"
        done
        for o2 in "${o2s[@]}"; do
            for i in ${o2#'|'}; do
                dq_vt_autoinvtab_tool_mainswitch_printhw $tbli $i $(( level + 1 ))
            done
        done
    done
}
