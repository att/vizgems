
if [[ $SWIFTWARNLEVEL != "" ]] then
    PS4='${.sh.file}-${.sh.fun}-$LINENO: '
    exec 2> url_driver.log
    set -x
fi

usage=$'
[-1p1?
@(#)$Id: vg_url_driver (AT&T Labs Research) 2012-09-06 $
]
'$USAGE_LICENSE$'
[+NAME?vg_url_driver - extract URL information]
[+DESCRIPTION?\bvg_url_driver\b extracts URL information by accessing the
specified URL.
The host name and type are specified on the command line.
The parameters to extract appear in stdin, one per line.
The first word in each line is the standard parameter name.
The rest of the text in the line is a CGI-style string.
The path name of the CGI can be \braw\b, to indicate that a single parameter
is to be collected.
When the path name is not \braw\b, it is assumed to indicate the suffix of the
name of a script.
The full script name will be \bvg_url_cmd_\bname, eg \bvg_url_cmd_xyz\b.
The output of that script must be zero or more lines of the form:
\brt=STAT ... stat param key=value pairs ...\b or
\brt=ALARM ... alarm param key=value pairs ...\b
The output of \bvg_url_driver\b is a \bksh\b compound variable containing
all the data from all the raw and scripted executions.
]
[999:v?increases the verbosity level. May be specified multiple times.]

targetname targetaddr targettype

[+SEE ALSO?\bVizGEMS\b(1)]
'

while getopts -a vg_url_driver "$usage" opt; do
    case $opt in
    999) (( SWIFTWARNLEVEL++ )) ;;
    *) OPTIND=0 getopts -a vg_url_driver "$usage" opt '-?'; exit 1 ;;
    esac
done
shift $OPTIND-1

targetname=$1
targetaddr=$2
targettype=$3

set -o pipefail

vars=()
varn=0
typeset -A js
otheri=0

ifs="$IFS"
IFS='|'
while read -r name type unit impl; do
    impl=${impl#URL:}
    mode=${impl%%\?*}
    impl=${impl##"$mode"?(\?)}

    IFS='&'
    set -f
    set -A al -- $impl
    set +f
    IFS='|'
    unset as; typeset -A as
    for a in "${al[@]}"; do
        k=${a%%=*}
        v=${a#*=}
        v=${v//'+'/' '}
        v=${v//@([\'\\])/'\'\1}
        eval "v=\$'${v//'%'@(??)/'\x'\1"'\$'"}'"
        v=${v//%26/'&'}
        as[$k]=$v
    done

    case $mode in
    raw)
        if [[ ${as[var]} == '' ]] then
            print -u2 vg_url_driver: no variable specified for $name
            continue
        fi
        if [[ ${as[url]} == '' ]] then
            print -u2 vg_url_driver: no url specified for $name
            continue
        fi
        ji=${name#*.}
        ;;
    cached|option)
        if [[ ${as[var]} == '' ]] then
            print -u2 vg_url_driver: no variable specified for $name
            continue
        fi
        if [[ ${as[url]} == '' ]] then
            print -u2 vg_url_driver: no url specified for $name
            continue
        fi
        ji=$mode
        ;;
    *)
        ji=other.$otheri
        (( otheri++ ))
        ;;
    esac

    if [[ ${as[pass]} == SWENC*SWENC*SWENC ]] then
        pass=${as[pass]}
        builtin -f swiftsh swiftshenc swiftshdec
        builtin -f codex codex
        phrase=${pass#SWENC*SWENC}; phrase=${phrase%SWENC}
        swiftshdec phrase code
        pass=${pass#SWENC}; pass=${pass%%SWENC*}
        print -r "$pass" \
        | codex --passphrase="$code" "<uu-base64-string<aes" \
        | read -r pass
        as[pass]=$pass
    fi
    if [[ ${as[pass]} == SWMUL*SWMUL && -f url.lastmulti ]] then
        touch url.lastmulti
    fi

    if [[ ${js[$ji].mode} == '' ]] then
        js[$ji]=( mode=$mode; typeset -A ms; typeset -A as )
    fi
    js[$ji].ms[$name]=(
        name=$name type=$type unit=$unit impl=$impl typeset -A as
    )
    for ai in "${!as[@]}"; do
        js[$ji].ms[$name].as[$ai]=${as[$ai]}
        [[ $ai == @(*_exp_*|*_sto_*_*) ]] && havestoorexp=y
    done

    js[$ji].as[timeout]=${as[timeout]:-10}
done
IFS="$ifs"

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
            print -u2 vg_url_driver: timeout for job "$@"
        fi
    fi
    wait $jid 2> /dev/null
    return $?
}

function uget {
    typeset args unp code time ret res alarm

    if [[ ,${mr.as[option]}, == *,ipv[46],* ]] then
        if [[ ,${mr.as[option]}, == *,ipv6,* ]] then
            args[${#args[@]}]=-6
        else
            args[${#args[@]}]=-4
        fi
    else
        if [[ $targetaddr == *:*:* ]] then
            args[${#args[@]}]=-6
        else
            args[${#args[@]}]=-4
        fi
    fi
    args[${#args[@]}]=-g
    args[${#args[@]}]=-w
    args[${#args[@]}]="%{http_code} %{time_total}\n"
    if [[ ${mr.as[user]} != '' && ${mr.as[user]} != '_NONE_' ]] then
        unp=${mr.as[user]}
        if [[ ${mr.as[pass]} != '' && ${mr.as[pass]} != '_NONE_' ]] then
            unp+=":${mr.as[pass]}"
        fi
        args[${#args[@]}]=--user
        args[${#args[@]}]=$unp
    fi
    if [[ ${mr.as[header]} != '' ]] then
        args[${#args[@]}]=-H
        args[${#args[@]}]=${mr.as[header]}
    fi
    if [[ ${mr.as[proxy]} != '' ]] then
        args[${#args[@]}]=-x
        if [[ ${mr.as[proxy]} == *:* ]] then
            args[${#args[@]}]=${mr.as[proxy]}
        else
            args[${#args[@]}]=${mr.as[proxy]}:80
        fi
    fi
    if [[ ${mr.as[success]} != '' || ${mr.as[failure]} != '' ]] then
        args[${#args[@]}]=-o
        args[${#args[@]}]=url.out
    else
        args[${#args[@]}]=-o
        args[${#args[@]}]=/dev/null
    fi

    curl -k -s -m ${jr.as[timeout]} "${args[@]}" ${mr.as[url]} < /dev/null \
    | read code time
    ret=1
    (( code >= 400 && code <= 599 )) && ret=1
    (( code >= 300 && code <= 310 )) && ret=0
    (( code >= 200 && code <= 210 )) && ret=0

    res=success
    alarm=
    if [[ $ret == 1 ]] then
        res=failure
        alarm="code:$code"
    elif [[
        ${mr.as[success]} != '' && $(< url.out) != *${mr.as[success]}*
    ]] then
        res=failure
        alarm="did not return correct string"
        ret=1
    elif [[
        ${mr.as[failure]} != '' && $(< url.out) == *${mr.as[failure]}*
    ]] then
        res=failure
        alarm="returned error string"
        ret=1
    fi
    print -- "$time $code $res $alarm" > url.data
    return $ret
}

if [[ $SWIFTWARNLEVEL != "" ]] then
    typeset -ft timedrun uget
fi

typeset -A avails times alarms

> url.err
> url.data

for ji in "${!js[@]}"; do
    typeset -n jr=js[$ji]

    case ${jr.mode} in
    raw|cached|option)
        for name in "${!jr.ms[@]}"; do
            typeset -n mr=jr.ms[$name]
            inst=${name#*.}
            [[ ${avails[$inst]} != '' ]] && continue

            success=y
            if ! timedrun $(( ${jr.as[timeout]} + 5 )) uget; then
                success=n
            fi 2>> url.err
            read urltime urlcode urlres urlalarm < url.data
            (( urltime < 0.0 )) && urltime=0.0
            times[$inst]=$(( urltime * 1000.0 ))
            if [[ $urlres == success ]] then
                avails[$inst]=100
                alarms[$inst]=_NO_ALARMS_
            else
                avails[$inst]=0
                alarms[$inst]=$urlalarm
                sed 's/^/URL LOG: /' url.err 1>&2
            fi
        done

        for name in "${!jr.ms[@]}"; do
            typeset -n mr=jr.ms[$name]
            inst=${name#*.}
            label=${mr.as[label]:-${mr.as[url]}}
            if [[ ,${mr.as[option]}, == *,ipv4,* ]] then
                label="IPv4-$label"
            elif [[ ,${mr.as[option]}, == *,ipv6,* ]] then
                label="IPv6-$label"
            fi
            case $name in
            url_avail.*)
                typeset -n vr=vars._$varn
                (( varn++ ))
                vr.rt=STAT
                vr.name=$name
                vr.type=${mr.type}
                vr.unit=%
                vr.label="Avail (${label})"
                vr.num=${avails[$inst]}
                if [[ ${alarms[$inst]} != @(''|_NO_ALARMS_) ]] then
                    vr.alarmlabel="Avail (${label}-${alarms[$inst]})"
                fi
                ;;
            url_time.*)
                typeset -n vr=vars._$varn
                (( varn++ ))
                vr.rt=STAT
                vr.name=$name
                vr.type=${mr.type}
                vr.unit=ms
                vr.label="Latency (${label})"
                if [[ ${alarms[$inst]} == _NO_ALARMS_ ]] then
                    vr.num=${times[$inst]}
                else
                    vr.num=
                    vr.nodata=2
                fi
                ;;
            url_diff.*)
                typeset -n vr=vars._$varn
                (( varn++ ))
                vr.rt=STAT
                vr.name=$name
                vr.type=${mr.type}
                vr.unit=ms
                vr.label="Latency Diff (${label})"
                if [[
                    ${alarms[_main]} == _NO_ALARMS_ &&
                    ${alarms[$inst]} == _NO_ALARMS_
                ]] then
                    vr.num=$(( ${times[_main]} - ${times[$inst]} ))
                else
                    vr.num=
                    vr.nodata=2
                fi
                ;;
            esac
        done
        ;;
    *)
        name="${!jr.ms[@]}"
        typeset -n mr=jr.ms[$name]
        print "${mr.name}|${mr.type}|${mr.unit}|${mr.impl}" \
        | $VG_SSCOPESDIR/current/url/vg_url_cmd_${jr.mode} \
            $targetname $targetaddr $targettype \
        | while read -r line; do
            typeset -n vr=vars._$varn
            (( varn++ ))
            eval vr=\( $line \)
        done
        if [[ $? != 0 ]] then
            print -u2 "vg_url_driver: failed to execute command ${jr.mode}"
        fi
        ;;
    esac
done

print "${vars}"
