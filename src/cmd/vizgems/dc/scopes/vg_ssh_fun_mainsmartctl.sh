smartctl=( rti= state= typeset -A devs metrics props attrs stats; metricn=0 )

function vg_ssh_fun_mainsmartctl_init {
    return 0
}

function vg_ssh_fun_mainsmartctl_term {
    return 0
}

function vg_ssh_fun_mainsmartctl_add {
    typeset var=${as[var]} inst=${as[inst]} lab=${as[lab]} dev=${as[dev]}

    smartctl.metrics[${smartctl.metricn}]=(
        name=$name
        unit=$unit
        type=$type
        var=$var
        inst=$inst
    )
    (( smartctl.metricn++ ))

    return 0
}

function vg_ssh_fun_mainsmartctl_send {
    print '[[ -f /var/log/vgsmartctl.log ]] && cat /var/log/vgsmartctl.log'
    return 0
}

function vg_ssh_fun_mainsmartctl_receive {
    typeset k v t attri
    typeset -F3 num

    if [[ $1 == VIZGEMS1:* ]] then
        k=${1%%:*}
        v=${1##"$k":*( )}
        v=${v%%' '*}
        smartctl.dev=$v
        smartctl.devid=${v//[!a-zA-Z0-9.-]/_}
        smartctl.state=''
        return 0
    elif [[ $1 == VIZGEMS2:* ]] then
        smartctl.dev=
        smartctl.devid=
        smartctl.state=''
        return 0
    elif [[ $1 == DATE:* ]] then
        smartctl.rti=${ printf '%(%#)T' "${1#'DATE: '}"; }
        return 0
    fi

    [[ $1 == *'SMART Attributes Data Structure'* ]] && smartctl.state=attr
    [[ $1 == '' ]] && smartctl.state=

    if [[ ${smartctl.state} == attr && $1 == *( )+([0-9])' '[A-Z]* ]] then
        set -f
        set -A ls -- $1
        set +f
        k=${ls[1]}
        v=${ls[3]##*( )}
        t=${ls[5]##*( )}
        (( num = 100.0 * (255.0 - v) / (255.0 - t) ))
        attri=${k//[!a-zA-Z0-9.-]/_}_${smartctl.devid}
        smartctl.stats[smartdisk_attr.${attri}]=(
            num=$num unit=% lab="${k//_/' '} (${smartctl.dev})"
        )

        case $k in
        *'Temperature'*'Cel'*)
            k=${ls[1]}
            v=${ls[9]}
            attri=${k//[!a-zA-Z0-9.-]/_}_${smartctl.devid}
            smartctl.stats[sensor_temp.${attri}]=(
                num=${ls[9]} unit=C lab="RV ${k//_/' '} (${smartctl.dev})"
            )
            ;;
        esac
    fi

    return 0
}

function vg_ssh_fun_mainsmartctl_emit {
    typeset metrici

    for (( metrici = 0; metrici < smartctl.metricn; metrici++ )) do
        typeset -n mr=smartctl.metrics[$metrici]
        [[ ${smartctl.stats[${mr.var}.${mr.inst}]} == '' ]] && continue
        typeset -n sr=smartctl.stats[${mr.var}.${mr.inst}]
        typeset -n vref=vars._$varn
        (( varn++ ))
        vref.rt=STAT
        vref.name=${mr.name}
        vref.type=${mr.type}
        vref.num=${sr.num}
        vref.unit=${sr.unit:-${mr.unit}}
        vref.label=${sr.lab}
        vref.rti=${smartctl.rti}
    done
    for (( alarmi = 0; alarmi < ${#smartctl.alarms[@]}; alarmi++ )) do
        typeset -n alarmr=smartctl.alarms[$alarmi]
        typeset -n vref=vars._$varn
        (( varn++ ))
        vref.rt=ALARM
        vref.alarmid=${alarmr.alarmid}
        vref.type=${alarmr.type}
        vref.sev=${alarmr.sev}
        vref.tech=${alarmr.tech}
        vref.tmode=${alarmr.tmode}
        vref.txt=${alarmr.txt}
        vref.rti=${smartctl.rti}
    done

    return 0
}

function vg_ssh_fun_mainsmartctl_invsend {
    print '[[ -f /var/log/vgsmartctl.log ]] && cat /var/log/vgsmartctl.log'
    return 0
}

function vg_ssh_fun_mainsmartctl_invreceive {
    typeset k v propi attri ifs

    if [[ $1 == VIZGEMS1:* ]] then
        k=${1%%:*}
        v=${1##"$k":*( )}
        v=${v%%' '*}
        smartctl.dev=$v
        smartctl.devid=${v//[!a-zA-Z0-9.-]/_}
        smartctl.devenabled=
        unset smartctl.props; typeset -A smartctl.props
        unset smartctl.attrs; typeset -A smartctl.attrs
        smartctl.state=''
        return 0
    elif [[ $1 == VIZGEMS2:* ]] then
        if [[ ${smartctl.devid} != '' && ${smartctl.devenabled} == y ]] then
            print "node|o|$AID|devid${smartctl.devid}|${smartctl.dev}"
            for propi in "${!smartctl.props[@]}"; do
                typeset -n propv=smartctl.props[$propi]
                print "node|o|$AID|${propi}|$propv"
            done
            for attri in "${!smartctl.attrs[@]}"; do
                typeset -n attrv=smartctl.attrs[$attri]
                print "node|o|$AID|${attri}|$attrv"
            done
        fi
        smartctl.dev=
        smartctl.devid=
        smartctl.devenabled=
        unset smartctl.props; typeset -A smartctl.props
        unset smartctl.attrs; typeset -A smartctl.attrs
        smartctl.state=''
        return 0
    elif [[ $1 == DATE:* ]] then
        smartctl.rti=${ printf '%(%#)T' "${1#'DATE: '}"; }
        return 0
    fi

    [[ $1 == *'START OF INFORMATION SECTION'* ]] && smartctl.state=prop
    [[ $1 == *'SMART Attributes Data Structure'* ]] && smartctl.state=attr
    [[ $1 == '' ]] && smartctl.state=

    if [[ ${smartctl.state} == prop && $1 == *:* ]] then
        k=${1%%:*}
        v=${1##"$k":*( )}
        propi=${k//[!a-zA-Z0-9.-]/_}_${smartctl.devid}
        smartctl.props[prop_$propi]=$v
        case $k in
        'SMART support is')
            [[ $v == Enabled ]] && smartctl.devenabled=y
            ;;
        esac
    elif [[ ${smartctl.state} == attr && $1 == *( )+([0-9])' '[A-Z]* ]] then
        set -f
        set -A ls -- $1
        set +f
        k=${ls[1]}
        v=${ls[3]}
        attri=${k//[!a-zA-Z0-9.-]/_}_${smartctl.devid}
        ifs="$IFS"
        IFS=':'
        smartctl.attrs[attr_$attri]="${ls[*]//\|/_}"
        IFS="$ifs"

        smartctl.attrs[si_sdid${attri}]=$attri
        smartctl.attrs[si_sdlabel${attri}]="${k//_/' '} (${smartctl.dev})"

        case $k in
        *'Temperature'*'Cel'*)
            k=${ls[1]}
            v=${ls[9]}
            attri=${k//[!a-zA-Z0-9.-]/_}_${smartctl.devid}
            smartctl.attrs[si_tempid${attri}]=$attri
            smartctl.attrs[si_templabel${attri}]="${k//_/' '} (${smartctl.dev})"
            ;;
        esac
    fi

    return 0
}

if [[ $SWIFTWARNLEVEL != "" ]] then
    typeset -ft vg_ssh_fun_mainsmartctl_init vg_ssh_fun_mainsmartctl_term
    typeset -ft vg_ssh_fun_mainsmartctl_add vg_ssh_fun_mainsmartctl_send
    typeset -ft vg_ssh_fun_mainsmartctl_receive vg_ssh_fun_mainsmartctl_emit
    typeset -ft vg_ssh_fun_mainsmartctl_invsend vg_ssh_fun_mainsmartctl_invreceive
fi
