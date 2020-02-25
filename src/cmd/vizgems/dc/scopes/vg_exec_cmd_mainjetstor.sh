
targetname=$1
targetaddr=$2
targettype=$3

typeset -A names args

ifs="$IFS"
IFS='|'
set -f
while read -r name type unit impl; do
    [[ $name != @(storageinv|storagestat)* ]] && continue
    names[$name]=( name=$name type=$type unit=$unit impl=$impl typeset -A args )
    IFS='&'
    set -A al -- ${impl#*\?}
    unset as; typeset -A as
    for a in "${al[@]}"; do
        k=${a%%=*}
        v=${a#*=}
        v=${v//'+'/' '}
        v=${v//@([\'\\])/'\'\1}
        eval "v=\$'${v//'%'@(??)/'\x'\1"'\$'"}'"
        v=${v//%26/'&'}
        args[$k]=$v
        names[$name].args[$k]=$v
    done
    IFS='|'
done
set +f
IFS="$ifs"

user=$EXECUSER pass=$EXECPASS

u=''
[[ $INVMODE == y ]] && u='-u2'

if [[ ${args[urlbase]} == '' ]] then
    print $u "rt=ALARM sev=1 type=ALARM tech=jetstor txt=\"url base not specified for jetstor collection\""
    exit 0
fi
urlbase=${args[urlbase]}

authurl=$urlbase
dataurl=$urlbase

aid=

typeset -A array state

# authentication

function gettoken { # $1 = user, $2 = pass (apitoken)
    typeset user=$1 pass=$2

    touch hdr.txt
    curl -D hdr.txt -k -s -H 'Expect:' -X POST \
        -F "username=$user" -F "password=$pass" \
    $authurl/login.php > /dev/null

    aid=$( egrep '^Set-Cookie:.*PHPSESSID=' hdr.txt )
    aid=${aid#*' '}
    aid=${aid%%';'*}
    aid=${aid%$'\r'}
    rm -f hdr.txt
}

# inventory

function getarray { # no args
    typeset ar sr i

    eval $(
        curl -k -s \
            -b "$aid" -H 'Expect:' \
        "$dataurl/sys_setting_x.php" | vgxml2ksh ar
    )
    array=(
        name=${ findcsbyname ar sysname; } version= id= memory=
        typeset -A ctrls ifaces luns disks
    )

    eval $(
        curl -k -s \
            -b "$aid" -H 'Expect:' \
        "$dataurl/sysinfo_x.php" | vgxml2ksh ar
    )
    for (( i = 0; i < ${#ar.cs[@]}; i++ )) do
        case ${ar.cs[$i].cs[0].cs[0].text} in
        'System Memory')
            array.memory=${ar.cs[$i].cs[1].cs[0].text%%B*}B
            ;;
        'Firmware Version')
            array.version=${ar.cs[$i].cs[1].cs[0].text}
            ;;
        'Backplane ID and HW No.')
            array.id=${ar.cs[$i].cs[1].cs[0].text}
            array.id=${array.id//[$'\n\r']/ }
            array.id=${array.id%%+( )}
            ;;
        esac
    done
    array.capacity=0
}

function getifaces { # no args
    typeset ir i j id name speed link ip mac status mbps en

    eval $(
        curl -k -s \
            -b "$aid" -H 'Expect:' \
        "$dataurl/iscsi_nic_x.php" | vgxml2ksh ir
    )
    for (( i = 0; i < ${#ir.cs[@]}; i++ )) do
        if [[ ${ir.cs[$i].name} == nic ]] then
            id=${ findcsbyname ir.cs[$i] id; }
            name=${ findcsbyname ir.cs[$i] name; }
            speed=${ findcsbyname ir.cs[$i] nic_type; }
            link=${ findcsbyname ir.cs[$i] link; }
            ip=${ findcsbyname ir.cs[$i] ip; }
            mac=${ findcsbyname ir.cs[$i] mac; }
            if [[ $link == Down ]] then
                status=down
            else
                status=up
                speed=${link%ps*}
                speed=${speed// /}
            fi
            if [[ $speed == *Gb* ]] then
                (( mbps = ${speed%Gb*} * 1000.0 ))
            elif [[ $speed == *Mb* ]] then
                mbps=${speed%Mb*}
            else
                mbps=1
            fi
            array.ifaces[$id]=(
                id=$id name=$name speed=$speed ip=$ip mac=$mac status=$status
                mbps=$mbps
            )
            state[iface:$id]=$status
            if [[ $status == up ]] then
                en=1
            else
                en=0
            fi
            curl -k -s \
                -b "$aid" -H 'Expect:' \
                -d "op=iscsi_set_monitor&ctrl_idx=0&nic_idx=$id&is_enable=$en" \
            "$dataurl/monitor_x.php" > /dev/null
        fi
    done
}

function getvolumes { # no args
    typeset vr i id name size
    typeset -l status health

    eval $(
        curl -k -s \
            -b "$aid" -H 'Expect:' \
        "$dataurl/rg_x.php?md5sum=0&rg_size_unit=gb" | vgxml2ksh vr
    )
    for (( i = 0; i < ${#vr.cs[@]}; i++ )) do
        if [[ ${vr.cs[$i].name} == vg ]] then
            id=${ findcsbyname vr.cs[$i] id; }
            name=${ findcsbyname vr.cs[$i] name; }
            size=${ findcsbyname vr.cs[$i] total_size; }
            status=${ findcsbyname vr.cs[$i] status; }
            health=${ findcsbyname vr.cs[$i] health; }
            array.luns[$id]=(
                id=$id name=$name status=$status health=$health size=$size
                serial=$id
            )
            state[lun:$id]=$status
        fi
    done
}

function getdisks { # no args
    typeset dr i id slot rgname size serial vendor rate en
    typeset -l status health

    eval $(
        curl -k -s \
            -b "$aid" -H 'Expect:' \
        "$dataurl/pd_x.php?enc_idx=0&md5sum=0&pd_size_unit=gb" | vgxml2ksh dr
    )
    for (( i = 0; i < ${#dr.cs[@]}; i++ )) do
        if [[ ${dr.cs[$i].name} == hdd ]] then
            id=${ findcsbyname dr.cs[$i] id; }
            slot=${ findcsbyname dr.cs[$i] slot; }
            rgname=${ findcsbyname dr.cs[$i] vg_name; }
            size=${ findcsbyname dr.cs[$i] size; }
            status=${ findcsbyname dr.cs[$i] status; }
            health=${ findcsbyname dr.cs[$i] health; }
            serial=${ findcsbyname dr.cs[$i] serial; }
            vendor=${ findcsbyname dr.cs[$i] vendor; }
            rate=${ findcsbyname dr.cs[$i] rate; }
            array.disks[$slot]=(
                id=$id slot=$slot rgname=$rgname status=$status health=$health
                size=$size serial=$serial vendor=$vendor rate=$rate
            )
            (( array.capacity += size ))
            state[disk:$slot]=$status
            if [[ $status == online ]] then
                en=1
            else
                en=0
            fi
            curl -k -s \
                -b "$aid" -H 'Expect:' \
                -d "op=disk_set_monitor&enc_idx=0&slot=$slot&is_enable=$en" \
            "$dataurl/monitor_x.php" > /dev/null
        fi
    done
}

function geninventory { # no args
    typeset iname lname dname

    print "node|o|$AID|si_name|${array.name}"
    print "node|o|$AID|si_version|${array.version}"
    print "node|o|$AID|si_id|${array.id}"
    print "node|o|$AID|si_capacity|${array.capacity} GB"
    print "node|o|$AID|si_sz_space_used._total|${array.capacity}"

    for iname in "${!array.ifaces[@]}"; do
        typeset -n ir=array.ifaces[$iname]
        [[ ${ir.status} == up ]] && print "node|o|$AID|si_iface$iname|$iname"
        print "node|o|$AID|si_iname$iname|${ir.name}"
        print "node|o|$AID|si_idescr$iname|${ir.name}"
        print "node|o|$AID|si_ialias$iname|${ir.name}"
        print "node|o|$AID|si_ispeed$iname|$(( ${ir.mbps} * 1000000.0 ))"
        print "node|o|$AID|si_istatus$iname|${ir.status}"
        print "node|o|$AID|si_sz_tcpip_inbw$iname|${ir.mbps}"
        print "node|o|$AID|si_sz_tcpip_outbw$iname|${ir.mbps}"
    done
    for lname in "${!array.luns[@]}"; do
        typeset -n lr=array.luns[$lname]
        print "node|o|$AID|si_lunname$lname|${lr.name}"
        print "node|o|$AID|si_lunsize$lname|${lr.size} GB"
    done
    for dname in "${!array.disks[@]}"; do
        typeset -n dr=array.disks[$dname]
        [[ ${dr.status} == online ]] && print "node|o|$AID|si_disk$dname|${dname}"
        print "node|o|$AID|si_dname$dname|${dname}"
        print "node|o|$AID|si_hwname$dname|${dr.size} GB"
        print "node|o|$AID|si_hwinfo$dname|:FW::SN:${dr.serial}:MN:${dr.vendor} ${dr.rate}"
    done
    typeset -p state > state.jetstor.sh.tmp \
    && mv state.jetstor.sh.tmp state.jetstor.sh
}

# alarms and stats

function genstatsandalarms { # no args
    typeset ir vr dr i j k l m n amode f1 f2 id link slot
    typeset -l status health
    typeset -F03 v

    f1='rt=STAT name="%s" num=%.3f unit="%s" type=number label="%s"'
    f2='rt=STAT name="%s" rti=%ld num=%.3f unit="%s" type=number label="%s"'

    eval $(
        curl -k -s \
            -b "$aid" -H 'Expect:' \
        "$dataurl/iscsi_nic_x.php" | vgxml2ksh ir
    )
    for (( i = 0; i < ${#ir.cs[@]}; i++ )) do
        if [[ ${ir.cs[$i].name} == nic ]] then
            id=${ findcsbyname ir.cs[$i] id; }
            link=${ findcsbyname ir.cs[$i] link; }
            if [[ $link == Down ]] then
                status=down
            else
                status=up
            fi
            [[ ${state[iface:$id]} != $status ]] && needreinv=y
        fi
    done
    eval $(
        curl -k -s \
            -b "$aid" -H 'Expect:' \
        "$dataurl/monitor_x.php?cmd=monitor_iscsi" | vgxml2ksh ir
    )
    for (( i = 0; i < ${#ir.cs[@]}; i++ )) do
        if [[ ${ir.cs[$i].name} == ctrl_iscsi_info ]] then
            id=${ findcsbyname ir.cs[$i] ctrl_idx; }
            [[ $id != 0 ]] && continue # only handles controller 0
            findcsbyname ir.cs[$i] iscsi_nics_stats y
            j=$csbynameindex
            [[ $j == '' ]] && continue
            typeset -n nr=ir.cs[$i].cs[$j]
            for (( k = 0; k < ${#nr.cs[@]}; k++ )) do
                [[ ${nr.cs[$k].name} != iscsi_nic_stats ]] && continue
                id=${ findcsbyname ir.cs[$i].cs[$j].cs[$k] nic_idx; }
                [[ $id == '' || ${state[iface:$id]} == '' ]] && continue
                findcsbyname ir.cs[$i].cs[$j].cs[$k] tx_rate y
                l=$csbynameindex
                [[ $l == '' ]] && continue
                typeset -n vr=ir.cs[$i].cs[$j].cs[$k].cs[$l]
                v=0 n=0
                for (( m = 0; m < ${#vr.cs[@]}; m++ )) do
                    (( v += ${vr.cs[$m].cs[0].text} ))
                    (( n++ ))
                done
                (( v = ((v / n) * 8.0) / 1024.0 ))
                printf "$f1\n" "tcpip_outbw.$id" $v "Mbps" "Out BW ($id)"
                findcsbyname ir.cs[$i].cs[$j].cs[$k] rx_rate y
                l=$csbynameindex
                [[ $l == '' ]] && continue
                typeset -n vr=ir.cs[$i].cs[$j].cs[$k].cs[$l]
                v=0 n=0
                for (( m = 0; m < ${#vr.cs[@]}; m++ )) do
                    (( v += ${vr.cs[$m].cs[0].text} ))
                    (( n++ ))
                done
                (( v = ((v / n) * 8.0) / 1024.0 ))
                printf "$f1\n" "tcpip_inbw.$id" $v "Mbps" "In BW ($id)"
            done
        fi
    done

    eval $(
        curl -k -s \
            -b "$aid" -H 'Expect:' \
        "$dataurl/rg_x.php?md5sum=0&rg_size_unit=gb" | vgxml2ksh vr
    )
    for (( i = 0; i < ${#vr.cs[@]}; i++ )) do
        if [[ ${vr.cs[$i].name} == vg ]] then
            id=${ findcsbyname vr.cs[$i] id; }
            name=${ findcsbyname vr.cs[$i] name; }
            status=${ findcsbyname vr.cs[$i] status; }
            health=${ findcsbyname vr.cs[$i] health; }
            [[ ${state[lun:$id]} != $status ]] && needreinv=y
            amode=clear
            [[ $health != good ]] && amode=set
            togglealarm $amode lun.$id "Lun $l status=$status health=$health"
        fi
    done

    eval $(
        curl -k -s \
            -b "$aid" -H 'Expect:' \
        "$dataurl/pd_x.php?enc_idx=0&md5sum=0&pd_size_unit=gb" | vgxml2ksh dr
    )
    for (( i = 0; i < ${#dr.cs[@]}; i++ )) do
        if [[ ${dr.cs[$i].name} == hdd ]] then
            id=${ findcsbyname dr.cs[$i] id; }
            slot=${ findcsbyname dr.cs[$i] slot; }
            status=${ findcsbyname dr.cs[$i] status; }
            health=${ findcsbyname dr.cs[$i] health; }
            [[ ${state[disk:$slot]} != $status ]] && needreinv=y
            amode=clear
            [[ $health != good ]] && amode=set
            togglealarm $amode disk.$id "Disk $l status=$status health=$health"
        fi
    done
    eval $(
        curl -k -s \
            -b "$aid" -H 'Expect:' \
        "$dataurl/monitor_x.php?cmd=monitor_disk" | vgxml2ksh dr
    )
    for (( i = 0; i < ${#dr.cs[@]}; i++ )) do
        if [[ ${dr.cs[$i].name} == disk_monitor_stats ]] then
            slot=${ findcsbyname dr.cs[$i] slot; }
            [[ $slot == '' ]] && continue
            findcsbyname dr.cs[$i] latency_rate y
            j=$csbynameindex
            [[ $j == '' ]] && continue
            typeset -n nr=dr.cs[$i].cs[$j]
            v=0 n=0
            for (( k = 0; k < ${#nr.cs[@]}; k++ )) do
                [[ ${nr.cs[$k].name} != latency ]] && continue
                (( v += ${nr.cs[$k].cs[0].text} ))
                (( n++ ))
            done
            (( v = (v / n) ))
            printf "$f1\n" "io_latency.$slot" $v "ms" "I/O Latency (Disk $slot)"
            findcsbyname dr.cs[$i] thruput_rate y
            j=$csbynameindex
            [[ $j == '' ]] && continue
            typeset -n nr=dr.cs[$i].cs[$j]
            v=0 n=0
            for (( k = 0; k < ${#nr.cs[@]}; k++ )) do
                [[ ${nr.cs[$k].name} != thruput ]] && continue
                (( v += ${nr.cs[$k].cs[0].text} ))
                (( n++ ))
            done
            (( v = (v / n) ))
            printf "$f1\n" "io_bw.$slot" $v "KB" "I/O Throughput (Disk $slot)"
        fi
    done
}

# utilities

csbynameindex=
function findcsbyname { # $1 = varref, $2 = name, $3 = y to just set index
    typeset -n cr=$1.cs
    typeset name=$2 saveindex=$3

    typeset i

    csbynameindex=
    for (( i = 0; i < ${#cr[@]}; i++ )) do
        if [[ ${cr[$i].name} == $name ]] then
            if [[ $saveindex != '' ]] then
                csbynameindex=$i
            else
                print ${cr[$i].cs[0].text}
            fi
            return 0
        fi
    done
    return 1
}

function togglealarm { # $1 = mode $2 = alarmid $3 = text
    typeset mode=$1 aid=$2 text=$3

    case $mode in
    set)
        if [[ ! -f alarm.jetstor.$aid ]] then
            print "rt=ALARM sev=1 alarmid=$aid type=ALARM tech=jetstor txt=\"$text\""
        fi
        touch alarm.jetstor.$aid
        ;;
    clear)
        if [[ -f alarm.jetstor.$aid ]] then
            print "rt=ALARM sev=5 alarmid=$aid type=CLEAR tech=jetstor txt=\"$text\""
            rm alarm.jetstor.$aid
        fi
        ;;
    esac
    print rt=STAT name=storagestat nodata=2
}

case $INVMODE in
y)
    gettoken "$user" "$pass"

    getarray
    getifaces
    getvolumes
    getdisks
    geninventory
    ;;
*)
    [[ -f state.jetstor.sh ]] && . ./state.jetstor.sh
    needreinv=n
    gettoken "$user" "$pass" || exit

    genstatsandalarms

    [[ $needreinv == y ]] && touch -d "14 day ago" inv.out
    ;;
esac

exit 0
