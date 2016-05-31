
n=$(uname -n)

typeset -A rules

. ./mainopenstack_rules.sh

datere='{4}([0-9])-{2}([0-9])-{2}([0-9]) {2}([0-9]):{2}([0-9]):{2}([0-9])'

for p in \
    nova/nova-api nova/nova-cert nova/nova-compute nova/nova-conductor \
    nova/nova-consoleauth nova/nova-manage nova/nova-novncproxy \
    nova/nova-scheduler cinder/cinder-api cinder/cinder-volume \
    cinder/cinder-scheduler neutron/dhcp-agent neutron/l3-agent \
    neutron/neutron-server neutron/neutron-metadata-agent \
    neutron/ovs-cleanup neutron/openvswitch-agent keystone/admin \
    keystone/keystone-manage keystone/main heat/heat-engine heat/heat-api \
    heat/heat-api-cfn glance/glance-api glance/glance-registry \
    ceilometer/ceilometer-agent-notification \
    ceilometer/ceilometer-alarm-evaluator \
    ceilometer/ceilometer-agent-compute ceilometer/ceilometer-agent-central \
    ceilometer/ceilometer-collector ceilometer/ceilometer-alarm-notifier \
    ceilometer/ceilometer-api; \
do
    f=${p#**/}
    typeset -n rr=rules[$f]
    [[ $rr == '' ]] && continue
    [[ ${rr.anchor} == '' ]] && continue

    maxt=${ printf '%(%#)T'; }
    (( maxt += 30 * 60 ))
    typeset -A counts txts
    ./vgtail /var/log/$p.log "" ./$f.$n | while read -r line; do
        [[ $line != $datere* ]] && continue

        prefix=${line%' '${rr.anchor}' '*}
        [[ $prefix == "$line" ]] && continue

        sev=${rr.sev}
        aid=${rr.aid}
        tmode=${rr.tmode}
        counted=${rr.counted}

        set -f
        set -A ls -- $line
        set +f
        time="${ls[0]} ${ls[1]}"
        line=${line#"$prefix "}
        level=${line%%' '*}
        txt=${line#*"$level "}
        txt=${txt// +( )/ }

        exclude=n
        for (( i = 0; ; i++ )) do
            typeset -n rrx=rules[$f].exclude[$i]
            [[ $rrx == '' ]] && break

            [[ ${rrx.level} != '' && $level != ${rrx.level} ]] && continue
            [[ ${rrx.txt} != '' && $txt != ${rrx.txt} ]] && continue

            exclude=y
            break
        done

        haveinclude=n
        include=n
        for (( i = 0; ; i++ )) do
            typeset -n rri=rules[$f].include[$i]
            [[ $rri == '' ]] && break

            haveinclude=y
            [[ ${rri.level} != '' && $level != ${rri.level} ]] && continue
            [[ ${rri.txt} != '' && $txt != ${rri.txt} ]] && continue

            include=y
            sev=${rri.sev:-${rr.sev}}
            aid=${rri.aid:-${rr.aid}}
            tmode=${rri.tmode:-${rr.tmode}}
            counted=${rri.counted:-${rr.counted}}
            break
        done

        if [[ $include == y || $exclude != y ]] then
            t=${ printf '%(%#)T' "$time"; }
            if (( t > maxt )) then
                t=$maxt
            fi
            if [[ $txt == *\[*\'Traceback*\]* ]] then
                l=${txt%\[*Traceback\ *}
                r=${txt##*\]}
                m=${txt#"$l["}
                m=${m%"]$r"}
                m=${m%%+([\\n\'\"])}
                m=${m##*\\n}
                txt="$l ... $m ... $r"
                txt=${txt//\\n/ }
                txt=${txt//\\/ }
                txt=${txt//\'/}
                txt=${txt// +( )/ }
            fi
            enc=${ printf '%#H' "$txt"; }

            if [[ $counted == y ]] then
                txts["$aid:ALARM:$sev:$tmode:OS:$enc"]="ID=:AID=$aid:TYPE=ALARM:SEV=$sev:TMODE=$tmode:TECH=OS:TI=$t:TXT=$enc"
                (( counts["$aid:ALARM:$sev:$tmode:OS:$enc"]++ ))
            else
                print -r "ID=:AID=$aid:TYPE=ALARM:SEV=$sev:TMODE=$tmode:TECH=OS:TI=$t:TXT=$enc"
            fi
        fi
    done
    for k in "${!counts[@]}"; do
        print -r "${txts[$k]} [VG:${counts[$k]}]"
    done
    unset counts txts
done
