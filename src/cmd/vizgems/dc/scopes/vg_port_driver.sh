
if [[ $SWIFTWARNLEVEL != "" ]] then
    PS4='${.sh.file}-${.sh.fun}-$LINENO: '
    exec 2> port_driver.log
    set -x
fi

usage=$'
[-1p1?
@(#)$Id: vg_port_driver (AT&T Labs Research) 2006-03-12 $
]
'$USAGE_LICENSE$'
[+NAME?vg_port_driver - generates statistics on ports]
[+DESCRIPTION?\bvg_port_driver\b generates availability and latency statistics
on the specified ip address and ports.
The host name and type are specified on the command line.
The parameters to extract appear in stdin, one per line.
The first word in each line is the standard parameter name.
The rest of the text in the line is a CGI-style string.
The path name of the CGI can be \braw\b, to indicate that a single parameter
is to be collected.
When the path name is not \braw\b, it is assumed to indicate the suffix of the
name of a script.
The full script name will be \bvg_port_cmd_\bname, eg \bvg_port_cmd_xyz\b.
The output of that script must be zero or more lines of the form:
\brt=STAT ... stat param key=value pairs ...\b or
\brt=ALARM ... alarm param key=value pairs ...\b
The output of \bvg_port_driver\b is a \bksh\b compound variable containing
all the data from all the raw and scripted executions.
]
[999:v?increases the verbosity level. May be specified multiple times.]

targetname targetaddr targettype

[+SEE ALSO?\bVizGEMS\b(1)]
'

while getopts -a vg_port_driver "$usage" opt; do
    case $opt in
    999) (( SWIFTWARNLEVEL++ )) ;;
    *) OPTIND=0 getopts -a vg_port_driver "$usage" opt '-?'; exit 1 ;;
    esac
done
shift $OPTIND-1

targetname=$1
targetaddr=$2
targettype=$3

set -o pipefail

target=$targetaddr

vars=()
varn=0
typeset -A js

ifs="$IFS"
IFS='|'
while read -r name type unit impl; do
    impl=${impl#PORT:}
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
        if [[ ${as[port]} == '' ]] then
            print -u2 vg_port_driver: no port specified for $name
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

    js[$ji].as[timeout]=${as[timeout]:-20}
    js[$ji].as[port]=${as[port]}
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
    if (( i >= maxt )) then
        if kill -9 -$jid 2> /dev/null; then
            print -u2 vg_port_driver: timeout for job "$@"
        fi
    fi
    wait $jid 2> /dev/null
    return $?
}

if [[ $SWIFTWARNLEVEL != "" ]] then
    typeset -ft timedrun
fi

typeset -A pmap
pmap[21]='-ftp'
pmap[22]='-ssh'
pmap[23]='-telnet'
pmap[25]='-smtp'
pmap[42]='-nameserver'
pmap[53]='-domain'
pmap[67]='-bootps'
pmap[68]='-bootpc'
pmap[69]='-tftp'
pmap[80]='-http'
pmap[111]='-sunrpc'
pmap[115]='-sftp'
pmap[123]='-ntp'
pmap[389]='-ldap'
pmap[443]='-https'
pmap[445]='-microsoft-ds'

> port.err

for ji in "${!js[@]}"; do
    typeset -n jr=js[$ji]

    case ${jr.mode} in
    raw)
        avail= time= aunit= tunit=
        if [[ ${jr.as[port]} == *:* ]] then
            timedrun $(( ${jr.as[timeout]} + 5 )) \
                vgport -t ${jr.as[timeout]} ${jr.as[port]%:*} ${jr.as[port]##*:}
        else
            timedrun $(( ${jr.as[timeout]} + 5 )) \
                vgport -t ${jr.as[timeout]} $target ${jr.as[port]}
        fi 2>> port.err | read -r a b c
        n=${a#*=}
        r=${b#*=}
        t=${c#*=}
        (( t < 0.0 )) && t=0.0
        if [[ $r != Y ]] then
            avail=0
            aunit=%
            time=
        else
            avail=100
            aunit=%
            time=$(( t * 1000.0 ))
            tunit=ms
        fi

        if [[ $avail == '' || $time == '' ]] then
            sed 's/^/PORT LOG: /' port.err 1>&2
            continue
        fi

        for name in "${!jr.ms[@]}"; do
            typeset -n mr=jr.ms[$name]
            case $name in
            port_avail.*)
                typeset -n vr=vars._$varn
                (( varn++ ))
                vr.rt=STAT
                vr.name=$name
                vr.type=number
                vr.unit=$aunit
                vr.num=$avail
                vr.label="Port Avail (${jr.as[port]}${pmap[${jr.as[port]}]})"
                ;;
            port_time.*)
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
                vr.label="Port Latency (${jr.as[port]}${pmap[${jr.as[port]}]})"
                ;;
            esac
        done
        ;;
    esac
done

print "${vars}"
