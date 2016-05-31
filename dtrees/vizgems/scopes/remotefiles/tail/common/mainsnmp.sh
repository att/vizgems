
set -o noglob
./vgtail /var/log/snmptrap.txt "" ./snmp.state \
| while read -r line; do
    unset recs
    integer recn=0

    #print FULL: $line
    rip=${line%%') TRAP,'*}
    rip=${rip##*' '}
    rest=${line#*SNMPv*-SMI::}
    while [[ $rest != '' ]] do
        recs[$recn]=${rest%%SNMPv*-SMI::*}
        if [[ $rest == *SNMPv*-SMI::* ]] then
            rest=${rest#*SNMPv*-SMI::}
        else
            rest=
        fi
        #print ENTRY: ${recs[$recn]}
        (( recn++ ))
    done

    id= ip= aid= type= sev= tmode= txt2= code=
    for (( reci = 0; reci < recn; reci++ )) do
        case "${recs[$reci]}" in
        *enterprises.1714*Uptime*)
            continue
            ;;
        esac
        str=${recs[$reci]#*STRING:}
        str=${str##+([ 	\"])}
        str=${str%%+([ 	\"])}
        #print GOOD:${recs[$reci]}
        #print STR=">"$str"<"
        case "${recs[$reci]}" in
        *enterprises.1714.1.2.3.1*)
            [[ ${recs[$reci]} != *STRING:* ]] && continue
            ip=${str#*' IP '}
            ip=${ip%%+([, 	])*}
            txt=${str#*Name:}
            aid=fc type=ALARM sev=1 tmode=keep
            [[ $txt == *'[Notification]'* ]] && sev=5
            ;;
        *enterprises.1588.*)
            [[ ${recs[$reci]} != *STRING:* ]] && continue
            ip=${recs[$reci]#*snmptrapd*": "}
            ip=${ip#*' '}
            ip=${ip#*' '}
            ip=${ip%%' '*}
            [[ $ip != [0-9]* ]] && ip=${ip%%'.'*}
            txt=${str#*SNMP' '}
            aid=fc type=ALARM sev=1 tmode=keep
            ;;
        *enterprises.5528.100.10.2.9' '*)
            code=${recs[$reci]#*'('}
            code=${code%%')'*}
            ;;
        *enterprises.5528.100.11.5' = '*'Door Switch'*)
            if [[ $code == @(10|110) ]] then
                ip=$rip
                if [[ $code == 10 ]] then
                    txt='Door Opened'
                else
                    txt='Door Closed'
                fi
                aid=nb type=ALARM sev=4 tmode=keep
            fi
            ;;
        *enterprises.5528.*)
            ;;
        *)
            [[ ${recs[$reci]} != *STRING:* ]] && continue
            txt2+=" $str"
            ;;
        esac
        [[ $sev != '' ]] && break
    done

    [[ $sev == '' && $txt2 == '' ]] && continue
    if [[ $sev == '' ]] then
        txt=$txt2
        aid= type=ALARM sev=1 tmode=keep
    fi

    id=${id:-$ip}

    txt=${txt//%/__P__}
    txt=${txt//' '/%20}
    txt=${txt//'+'/%2B}
    txt=${txt//';'/%3B}
    txt=${txt//'&'/%26}
    txt=${txt//'|'/%7C}
    txt=${txt//\'/%27}
    txt=${txt//\"/%22}
    txt=${txt//'<'/%3C}
    txt=${txt//'>'/%3E}
    txt=${txt//\\/%5C}
    txt=${txt//$'\n'/' '}
    txt=${txt//$'\r'/' '}
    txt=${txt//__P__/%25}
    print -r "ID=$id:AID=$aid:TYPE=$type:SEV=$sev:TMODE=$tmode:TECH=SNMPTRAP:TXT=$txt"
    #print ""
done
