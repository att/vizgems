
export cid=$CID aid=$AID ip=$IP user=$USER pass=$PASS cs=$CS
export targettype=$TARGETTYPE scopetype=$SCOPETYPE servicelevel=$SERVICELEVEL

function timedrun { # $1 = timeout, $2-* cmd and args
    typeset maxt jid i

    maxt=$1
    shift 1

    set -o monitor
    "$@" &
    jid=$!
    for (( i = 0; i < maxt; i += 0.1 )) do
        kill -0 $jid 2> /dev/null || break
        sleep 0.1
    done
    if (( $i >= maxt )) then
        if kill -9 -$jid 2> /dev/null; then
            print -u2 vg_scopeinv_mainwin32: timeout for job "$@"
        fi
    fi
    wait $jid 2> /dev/null
    return $?
}

set -o pipefail

. vg_units

ii=0
unset WMIUSER WMIPASSWD
[[ $USER != '' ]] && export WMIUSER=$user
[[ $PASS != '' ]] && export WMIPASSWD=$pass
typeset -A fssizes fstypes
(
    # identification and uptime
    print "Win32_OperatingSystem|ident0||CSName"
    print "Win32_OperatingSystem|ident1||Caption"
    print "Win32_OperatingSystem|ident2||CSDVersion"
    print "Win32_OperatingSystem|uptime0||LocalDateTime"
    print "Win32_OperatingSystem|uptime1||LastBootUpTime"
    # OS level
    print "Win32_LogicalDisk|fs||Size"
    print "Win32_LogicalDisk|fstype||FileSystem"
    print "Win32_PerfRawData_PerfOS_Processor|cpu||"
    print "Win32_PerfRawData_Tcpip_NetworkInterface|iface|loopback_OR_6to4*_OR_*pseudo*_OR_isatap*|"
    print "Win32_ComputerSystem|mem||TotalPhysicalMemory"
    print "Win32_OperatingSystem|swap||SizeStoredInPagingFiles"
    # for IIS - optional
    if [[ $servicelevel == @(man|mon|soss) ]] then
        print "Win32_PerfRawData_ASP_ActiveServerPages|asp||"
        #print "Win32_PerfRawData_MSFtpsvc_FTPService|ftp||"
        print "Win32_PerfRawData_W3SVC_WebService|iis||"
    fi
) > wmi.in
timedrun 120 vgwmi -h $ip -i - < wmi.in > wmi.out
while read -r line2; do
    [[ $line2 != *'|'*'|'* ]] && continue
    name=${line2%%'|'*}
    rest=${line2#*'|'}
    val=${line2##*'|'}
    rid=${rest%'|'*}
    if [[ $rid == _Total ]] then
        kid=_total
    else
        kid=${rid//[![:alnum:]]/_}
    fi
    if [[ $name == ident[0-9]* ]] then
        ident[${name#ident}]=$val
        if (( ${#ident[@]} == 3 )) then
            print "node|o|$aid|si_ident|${ident[@]}"
        fi
    elif [[ $name == uptime[0-9]* ]] then
        uptime[${name#uptime}]=$val
        if (( ${#uptime[@]} == 2 )) then
            s1=${uptime[0]}
            d1=${s1:0:4}.${s1:4:2}.${s1:6:2}-${s1:8:2}:${s1:10:2}:${s1:12:2}
            t1=$(printf '%(%#)T' "$d1")
            s2=${uptime[1]}
            d2=${s2:0:4}.${s2:4:2}.${s2:6:2}-${s2:8:2}:${s2:10:2}:${s2:12:2}
            t2=$(printf '%(%#)T' "$d2")
            (( dt = t1 - t2 ))
            if (( dt > 0 )) then
                (( dd = dt / (24 * 60 * 60) ))
                (( dt -= (dd * 24 * 60 * 60) ))
                (( hh = dt / (60 * 60) ))
                (( dt -= (hh * 60 * 60) ))
                (( mm = dt / 60 ))
                if (( dd == 1 )) then
                    s="$dd day, $hh:$mm"
                else
                    s="$dd days, $hh:$mm"
                fi
                print "node|o|$aid|si_uptime|$s"
            fi
        fi
    elif [[ $name == iface ]] then
        [[ $rid == *'|'* ]] && continue
        print "$rid" | sum -x md5 | read ifi
        print "node|o|$aid|si_ifaceif$ifi|$rid"
    elif [[ $name == fs ]] then
        [[ $rid == *'|'* ]] && continue
        [[ $val != [0-9]* ]] && continue
        vg_unitconv "$val" B GB
        val=$vg_ucnum
        if [[ $val != '' ]] then
            fssizes[$rid]=$val
        fi
        if [[ ${fssizes[$rid]} != '' && ${fstypes[$rid]} == NTFS ]] then
            print "node|o|$aid|si_fs${rid%:}|$rid"
            print "node|o|$aid|si_sz_fs_used.${rid%:}|${fssizes[$rid]}"
        fi
    elif [[ $name == fstype ]] then
        [[ $rid == *'|'* ]] && continue
        fstypes[$rid]=$val
        if [[ ${fssizes[$rid]} != '' && ${fstypes[$rid]} == NTFS ]] then
            print "node|o|$aid|si_fs${rid%:}|$rid"
            print "node|o|$aid|si_sz_fs_used.${rid%:}|${fssizes[$rid]}"
        fi
    elif [[ $name == mem ]] then
        vg_unitconv "$val" B MB
        val=$vg_ucnum
        if [[ $val != '' ]] then
            print "node|o|$aid|si_sz_memory_used._total|$val"
        fi
    elif [[ $name == swap ]] then
        vg_unitconv "$val" KB MB
        val=$vg_ucnum
        if [[ $val != '' ]] then
            print "node|o|$aid|si_sz_swap_used._total|$val"
        fi
    else
        if [[ $name == @(iis|ftp|asp) ]] then
            print "node|o|$aid|si_chits_total|ALL"
            if [[ $name == ftp && $rid == _Total ]] then
                timedrun 60 vgport $ip 21 | while read -r a b c; do
                    n=${a#*=}
                    r=${b#*=}
                    [[ $r == Y ]] && print "node|o|$aid|si_port$n|$n"
                done
            fi
            if [[ $name == iis && $rid == _Total ]] then
                timedrun 60 vgport $ip 80 443 | while read -r a b c; do
                    n=${a#*=}
                    r=${b#*=}
                    [[ $r == Y ]] && print "node|o|$aid|si_port$n|$n"
                done
            fi
        fi
        print "node|o|$aid|si_${name}$kid|$rid"
    fi
done < wmi.out
