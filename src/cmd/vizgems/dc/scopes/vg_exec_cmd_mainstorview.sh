
targetname=$1
targetaddr=$2
targettype=$3

typeset -A names args

ifs="$IFS"
IFS='|'
set -f
while read -r name type unit impl; do
    [[ $name != @(idracinv|idracstat)* ]] && continue
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

urlbase=http://$targetaddr:9292
if [[ ${args[urlbase]} != '' ]] then
    urlbase=${args[urlbase]}
fi

alarmurl="$urlbase/cgi-bin/cgiloader.cgi"
alarmurl+="?method=ViewLogs&RandomNumber=1&MaxView=1000"

typeset -A alarms

# get alarms

function genalarms { # no args
    typeset line rest txt alarm

    ifs="$IFS"
    IFS=''
    curl -s --user "$user:$pass" "$alarmurl" | while read line; do
        case $line in
        ' <td height="15" align="center" valign="top"  class="tabb"'*)
            rest=${line%'</span>'*}
            rest=${rest##*'">'}
            if [[ $rest == [0-9][0-9]/[0-9][0-9]/[0-9][0-9] ]] then
                txt=$rest
            elif [[ $rest == [0-9][0-9]:[0-9][0-9]:[0-9][0-9] ]] then
                txt+=" - $rest"
            fi
            ;;
        ' <td height="15" align="left" valign="top"  class="tabb"'*)
            rest=${line%'</span>'*}
            rest=${rest##*'">'}
            rest=${rest//\"/\'}
            txt+=" / $rest"
            ;;
        ' <td height="15" align="left" valign="top" class="tabb"'*)
            rest=${line%'</span>'*}
            rest=${rest##*'">'}
            rest=${rest//\"/\'}
            txt+=" - $rest"
            [[ $txt == " - Device"* ]] && continue
            alarm="rt=ALARM sev=1 type=ALARM tech=storview txt=\"$txt\""
            if [[ ${alarms[$alarm]} == o ]] then
                alarms[$alarm]=s
            else
                alarms[$alarm]=n
            fi
            ;;
        esac
    done

    for alarm in "${!alarms[@]}"; do
        if [[ ${alarms[$alarm]} == n ]] then
            print -r "$alarm"
            alarms[$alarm]=o
        elif [[ ${alarms[$alarm]} == s ]] then
            alarms[$alarm]=o
        else
            unset alarms[$alarm]
        fi
    done

    print rt=STAT name=storviewalarm nodata=2
    IFS="$ifs"
}

case $INVMODE in
y)
    ;;
*)
    [[ -f storview.sh ]] && . ./storview.sh
    genalarms
    typeset -p alarms > ./storview.sh.tmp \
    && mv ./storview.sh.tmp ./storview.sh
    ;;
esac

exit 0
