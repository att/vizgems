
n=$(uname -n)

typeset -A rules

. ./mainsyslog_rules.sh

datere1='{3}([A-Za-z])+( )+([0-9]) {2}([0-9]):{2}([0-9]):{2}([0-9])'
datere2='{4}([0-9])-{2}([0-9])-{2}([0-9])T{2}([0-9]):{2}([0-9]):{2}([0-9])'
typeset -l tool

for p in syslog messages; do
    f=syslog
    typeset -n rr=rules[$f]
    [[ $rr == '' ]] && continue

    mint=${ printf '%(%#)T'; }
    (( mint -= 12 * 60 * 60 ))
    maxt=${ printf '%(%#)T'; }
    (( maxt += 10 * 60 ))
    typeset -A counts txts
    ./vgtail /var/log/$p "" ./$f.$n.$p | while read -r line; do
        [[ $line != @($datere1|$datere2)*:* ]] && continue

        line=${line// +( )/ }
        if [[ $line == $datere1* ]] then
            set -f
            set -A ls -- $line
            set +f
            time="${ls[0]} ${ls[1]} ${ls[2]}"
            line=${line#*' '}; line=${line#*' '}; line=${line#*' '}
        else
            time="${line%%' '*}"
            line=${line#*' '}
        fi

        id=${line%%' '*}
        [[ $id == localhost ]] && id=${TAILHOST:-$id}
        line=${line#*' '}
        tool=${line%%' '*}
        txt=${line#*' '}
        txt=${txt##'['+([0-9.])']'+( )}
        tool=${tool##*/}
        tool=${tool%%[\(\[:]*}
        [[ $tool == '' ]] && continue
        tool=${tool// /_}

        sev=${rr.sev}
        tmode=${rr.tmode}
        counted=${rr.counted}

        exclude=n
        [[ $txt == '' ]] && exclude=y
        for (( i = 0; ; i++ )) do
            typeset -n rrx=rules[$f].exclude[$i]
            [[ $rrx == '' ]] && break

            [[ ${rrx.tool} != '' && $tool != ${rrx.tool} ]] && continue
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
            [[ ${rri.tool} != '' && $tool != ${rri.tool} ]] && continue
            [[ ${rri.txt} != '' && $txt != ${rri.txt} ]] && continue

            include=y
            sev=${rri.sev:-${rr.sev}}
            tmode=${rri.tmode:-${rr.tmode}}
            counted=${rri.counted:-${rr.counted}}
            break
        done

        if [[ $include == y || $exclude != y ]] then
            t=${ printf '%(%#)T' "$time"; }
            if (( t > maxt )) then
                t=$maxt
            elif (( t < mint )) then
                t=$mint
            fi
            enc=${ printf '%#H' "$txt"; }

            if [[ $counted == y ]] then
                k="$id:$tool:ALARM:$sev:$tmode:SYSLOG:$enc"
                txts[$k]="ID=$id:AID=$tool:TYPE=ALARM:SEV=$sev:TMODE=$tmode:TECH=SYSLOG:TI=$t:TXT=$enc"
                (( counts[$k]++ ))
            else
                set -f
                print -r "ID=$id:AID=$tool:TYPE=ALARM:SEV=$sev:TMODE=$tmode:TECH=SYSLOG:TI=$t:TXT=$enc"
                set +f
            fi
        fi
    done

    set -f
    for k in "${!counts[@]}"; do
        print -r "${txts[$k]} [VG:${counts[$k]}]"
    done
    set +f
    unset counts txts
done
