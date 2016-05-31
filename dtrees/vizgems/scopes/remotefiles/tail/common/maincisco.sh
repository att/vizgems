
yre='{4}([0-9])'
mre='@(Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec)'
dre='{2}([0-9])'
hmsre='{2}([0-9]):{2}([0-9]):{2}([0-9])'
hmsre2='{2}([0-9]):{2}([0-9]):{2}([0-9])?(.+([0-9]))'
zre='?(-){2}([0-9]):{2}([0-9])'
zre2='+([A-Z])*([-0-9:])*([A-Z])'

set -o noglob
./vgtail /var/log/cisco.txt "" ./cisco.state \
| while read -r line; do
    set -f
    set -A ls -- $line
    set +f
    id=${ls[3]}
    i=4
    if [[ ${ls[$i]} == +([0-9]): ]] then
        (( i++ ))
    fi
    # zap date if present
    if [[
        ${ls[$i]} == $yre && ${ls[$(( i + 1 ))]} == $mre &&
        ${ls[$(( i + 2 ))]} == $dre && ${ls[$(( i + 3 ))]} == $hmsre &&
        ${ls[$(( i + 5 ))]} == $zre
    ]] then
        (( i += 6 ))
    elif [[ ${ls[$i]} == $mre && ${ls[$(( i + 2 ))]} == $hmsre: ]] then
        (( i += 3 ))
    elif [[
        ${ls[$i]} == $mre && ${ls[$(( i + 2 ))]} == $hmsre2
        && ${ls[$(( i + 3 ))]} == $zre2
    ]] then
        (( i += 4 ))
    fi
    for (( j = 0; j < i; j++ )) do
        unset ls[$j]
    done
    txt="${ls[*]}"
    case $txt in
    *HOSTFLAPPING:*)
        aid=net.flap type=ALARM sev=3 tmode=defer
        ;;
    *BLOCKEDTXQUEUE:*)
        aid=net.blocked type=ALARM sev=1 tmode=keep
        ;;
    *INVALIDSOURCEADDRESSPACKET:*)
        aid=net.invalidpkt type=ALARM sev=4 tmode=keep
        ;;
    *PORTFROMSTP:*)
        aid=net.port type=ALARM sev=4 tmode=keep
        ;;
    *PORTTOSTP:*)
        aid=net.port type=CLEAR sev=4 tmode=keep
        ;;
    *LINEPROTO*UPDOWN:*)
        aid=net.line type=ALARM sev=4 tmode=keep
        [[ $txt == *' to up'* ]] && type=CLEAR
        ;;
    *LINK*UPDOWN:*)
        aid=net.link type=ALARM sev=4 tmode=keep
        [[ $txt == *' to up'* ]] && type=CLEAR
        ;;
    *)
        aid= type=ALARM sev=1 tmode=keep
        ;;
    esac

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
    print -r "ID=$id:AID=$aid:TYPE=$type:SEV=$sev:TMODE=$tmode:TECH=SYSLOG:TXT=$txt"
done
