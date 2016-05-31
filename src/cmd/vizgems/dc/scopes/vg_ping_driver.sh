
if [[ $SWIFTWARNLEVEL != "" ]] then
    PS4='${.sh.file}-${.sh.fun}-$LINENO: '
    exec 2> ping_driver.log
    set -x
fi

usage=$'
[-1p1?
@(#)$Id: vg_ping_driver (AT&T Labs Research) 2006-03-12 $
]
'$USAGE_LICENSE$'
[+NAME?vg_ping_driver - collect information using PING]
[+DESCRIPTION?\bvg_ping_driver\b collects latency and packet loss information
by running ping.
The host name and type are specified on the command line.
The parameters to extract appear in stdin, one per line.
The first word in each line is the standard parameter name.
The rest of the text in the line is a CGI-style string.
The path name of the CGI is assumed to indicate the suffix of the
name of a script.
The full script name will be \bvg_ping_cmd_\bname, eg \bvg_ping_cmd_xyz\b.
The output of that script must be zero or more lines of the form:
\brt=STAT ... stat param key=value pairs ...\b or
\brt=ALARM ... alarm param key=value pairs ...\b
The output of \bvg_ping_driver\b is a \bksh\b compound variable containing
all the data from all the pingutions.
]
[999:v?increases the verbosity level. May be specified multiple times.]

targetname targetaddr targettype

[+SEE ALSO?\bVizGEMS\b(1)]
'

while getopts -a vg_ping_driver "$usage" opt; do
    case $opt in
    999) (( SWIFTWARNLEVEL++ )) ;;
    *) OPTIND=0 getopts -a vg_ping_driver "$usage" opt '-?'; exit 1 ;;
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

ifs="$IFS"
IFS='|'
while read -r name type unit impl; do
    impl=${impl#PING:}
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
            print -u2 vg_ping_driver: no variable specified for $name
            continue
        fi
        ji=${name#*.}
        ;;
    esac

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

    js[$ji].as[count]=${as[count]:-4}
    js[$ji].as[interval]=${as[interval]:-0.2}
    js[$ji].as[timeout]=${as[timeout]:-2}
    js[$ji].as[ip]=${as[ip]:-$targetaddr}
done
IFS="$ifs"

> ping.err

for ji in "${!js[@]}"; do
    typeset -n jr=js[$ji]

    case ${jr.mode} in
    raw)
        loss= time= lunit= tunit=
        case $VG_SCOPETYPE in
        *linux*)
            case ${jr.as[ip]} in
            *:*:*) cmd=/bin/ping6 ;;
            *)     cmd=/bin/ping  ;;
            esac
            $cmd -q -n -i ${jr.as[interval]} -W ${jr.as[timeout]} \
                -c ${jr.as[count]} ${jr.as[ip]} \
            2>> ping.err \
            | while read line; do
                case $line in
                *packets\ transmitted*)
                    [[ $line == *,\ +([0-9])%\ packet\ loss,\ time\ * ]]
                    set -f
                    set -A res -- "${.sh.match[@]}"
                    set +f
                    loss=${res[1]}
                    lunit=%
                    ;;
                rtt\ min/avg/max/mdev*)
                    [[ $line == *\ =\ */@(*)/*/+([0-9.])\ +([^,])@(*) ]]
                    set -f
                    set -A res -- "${.sh.match[@]}"
                    set +f
                    time=${res[1]}
                    tunit=${res[3]}
                    ;;
                esac
            done
            ;;
        *win32*)
            /sys/ping -n ${jr.as[count]} ${jr.as[ip]} \
            2>> ping.err \
            | while read line; do
                line=${line%%$'\r'}
                case $line in
                *Packets*Lost*=*)
                    [[ $line == *Lost\ =\ +([0-9])\ \(+([0-9.])%* ]]
                    set -f
                    set -A res -- "${.sh.match[@]}"
                    set +f
                    loss=${res[2]}
                    lunit=%
                    ;;
                *Average*)
                    [[ $line == *Average\ =\ +([0-9.])@(*) ]]
                    set -f
                    set -A res -- "${.sh.match[@]}"
                    set +f
                    time=${res[1]}
                    tunit=${res[2]}
                    ;;
                esac
            done
            ;;
        esac
        if [[ $loss == '' && $time == '' ]] then
            sed 's/^/PING LOG: /' ping.err 1>&2
            continue
        fi

        for name in "${!jr.ms[@]}"; do
            typeset -n mr=jr.ms[$name]
            case $name in
            ping_loss.*)
                typeset -n vr=vars._$varn
                (( varn++ ))
                vr.rt=STAT
                vr.name=$name
                vr.type=number
                vr.unit=$lunit
                vr.num=$loss
                vr.label='Ping Packet Loss'
                if [[ $name == *._main ]] then
                    vr.label+=" (Main-${jr.as[ip]})"
                else
                    vr.label+=" (${jr.as[ip]})"
                fi
                ;;
            ping_time.*)
                typeset -n vr=vars._$varn
                (( varn++ ))
                vr.rt=STAT
                vr.name=$name
                vr.type=number
                vr.unit=$tunit
                if [[ $time != '' ]] then
                    vr.num=$time
                else
                    vr.num=
                    vr.nodata=2
                fi
                vr.label='Ping Latency'
                if [[ $name == *._main ]] then
                    vr.label+=" (Main-${jr.as[ip]})"
                else
                    vr.label+=" (${jr.as[ip]})"
                fi
                ;;
            esac
        done
        ;;
    esac
done

print "${vars}"
