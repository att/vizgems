
targetname=$1
targetaddr=$2
targettype=$3

typeset -A names args topics

ifs="$IFS"
IFS='|'
set -f
while read -r name type unit impl; do
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
        case $k in
        topic) topics[$v]=$v ;;
        esac
    done
    IFS='|'
done
set +f
IFS="$ifs"

if [[ ${args[urlbase]} == '' ]] then
    print "rt=ALARM sev=1 type=ALARM tech=hadoop txt=\"url base not specified for hadoop collection\""
    exit 0
fi
urlbase=${args[urlbase]}

curl[0]=curl
curl[${#curl[@]}]=-s
curl[${#curl[@]}]=-H
curl[${#curl[@]}]='Accept: application/xml'
curln=${#curl[@]}

for topici in "${!topics[@]}"; do
    curl[$curln]=$urlbase/$topici
    { "${curl[@]}"; print ""; } | sed 's/?//' > $topici.txt
    vgxml -topmark hdata -ksh < $topici.txt > $topici.sh
    unset hdata
    . ./$topici.sh
    for namei in "${!names[@]}"; do
        [[ ${names[$namei].done} == y ]] && continue
        typeset -n nr=names[$namei] ar=names[$namei].args
        for hdatai in "${!hdata.@}"; do
            if [[ (
                ${ar[var]} != '' && $hdatai == hdata.xml.${ar[var]}
            ) && (
                ${ar[evar]} == '' || $hdatai != ${ar[evar]}
            ) ]] then
                typeset -n vr=$hdatai
                case $INVMODE in
                y)
                    names[$namei].done=y
                    k=$namei
                    if [[ ${ar[id]} != '' ]] then
                        typeset -n kr=${hdatai%.*}.${ar[id]}
                        k=$namei${kr//[!a-zA-Z0-9]/_}
                    fi
                    v=$vr
                    if [[ ${ar[type]:-${nr.type}} == time ]] then
                        v=${ printf '%(%Y/%m/%d-%H:%M:%S)T' \#${vr%???}; }
                    elif [[ $hdatai == *MB ]] then
                        (( v /= 1024 ))
                    fi
                    if [[ ${ar[map]} != '' ]] then
                        ifs="$IFS"
                        IFS=':'
                        set -f
                        set -A maps -- ${ar[map]}
                        set +f
                        for (( mapi = 0; mapi < ${#maps[@]}; mapi += 2 )) do
                            if [[ $v == ${maps[$mapi]} ]] then
                                v=${maps[$(( mapi + 1 ))]}
                                break
                            fi
                        done
                        IFS="$ifs"
                    fi
                    print "node|o|$AID|$k|$v"
                    ;;
                *)
                    k=$namei
                    if [[ ${ar[id]} != '' ]] then
                        typeset -n kr=${hdatai%.*}.${ar[id]}
                        [[ ${namei##*.} != ${kr//[!a-zA-Z0-9]/_} ]] && continue
                    fi
                    names[$namei].done=y
                    v=$vr
                    if [[ ${ar[type]:-${nr.type}} == time ]] then
                        v=${ printf '%(%Y/%m/%d-%H:%M:%S)T' \#${vr%???}; }
                    elif [[ $hdatai == *MB ]] then
                        (( v /= 1024 ))
                    fi
                    if [[ ${ar[alarm]} != '' ]] then
                        if [[ $v == ${ar[alarm]} ]] then
                            if [[ ! -f alarm.hadoop.${namei//\//_} ]] then
                                print "rt=ALARM sev=2 type=ALARM tech=hadoop txt=\"${ar[alarmlabel]:-$namei} = $v\""
                            fi
                            touch alarm.hadoop.${namei//\//_}
                        else
                            if [[ -f alarm.hadoop.${namei//\//_} ]] then
                                print "rt=ALARM sev=2 type=CLEAR tech=hadoop txt=\"${ar[alarmlabel]:-$namei} = $v\""
                                rm alarm.hadoop.${namei//\//_}
                            fi
                        fi
                    fi
                    if [[ ${ar[map]} != '' ]] then
                        ifs="$IFS"
                        IFS=':'
                        set -f
                        set -A maps -- ${ar[map]}
                        set +f
                        for (( mapi = 0; mapi < ${#maps[@]}; mapi += 2 )) do
                            if [[ $v == ${maps[$mapi]} ]] then
                                v=${maps[$(( mapi + 1 ))]}
                                break
                            fi
                        done
                        IFS="$ifs"
                    fi
                    [[ $v != +([0-9.-]) ]] && continue
                    label=$namei
                    if [[ ${ar[label]} != '' ]] then
                        label=${ printf "${ar[label]}" "${namei##*.}"; }
                    fi
                    print "rt=STAT name=${nr.name} type=${nr.type} num=$v unit=${nr.unit} label=\"$label\""
                    ;;
                esac
            fi
        done
    done
done
