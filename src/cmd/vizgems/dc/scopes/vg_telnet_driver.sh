
if [[ $SWIFTWARNLEVEL != "" ]] then
    PS4='${.sh.file}-${.sh.fun}-$LINENO: '
    exec 2> telnet_driver.log
    set -x
fi

usage=$'
[-1p1?
@(#)$Id: vg_telnet_driver (AT&T Labs Research) 2006-03-12 $
]
'$USAGE_LICENSE$'
[+NAME?vg_telnet_driver - collect information using TELNET]
[+DESCRIPTION?\bvg_telnet_driver\b collects system information by running
various commands on a remote system, through telnet.
The host name and type are specified on the command line.
The parameters to extract appear in stdin, one per line.
The first word in each line is the standard parameter name.
The rest of the text in the line is a CGI-style string.
The path name of the CGI is assumed to indicate the suffix of the
name of a script.
The full script name will be \bvg_telnet_cmd_\bname, eg \bvg_telnet_cmd_xyz\b.
The output of that script must be zero or more lines of the form:
\brt=STAT ... stat param key=value pairs ...\b or
\brt=ALARM ... alarm param key=value pairs ...\b
The output of \bvg_telnet_driver\b is a \bksh\b compound variable containing
all the data from all the executions.
]
[999:v?increases the verbosity level. May be specified multiple times.]

targetname targetaddr targettype

[+SEE ALSO?\bVizGEMS\b(1)]
'

. $VG_SSCOPESDIR/current/telnet/vg_telnet_driver_fun

while getopts -a vg_telnet_driver "$usage" opt; do
    case $opt in
    999) (( SWIFTWARNLEVEL++ )) ;;
    *) OPTIND=0 getopts -a vg_telnet_driver "$usage" opt '-?'; exit 1 ;;
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
    impl=${impl#TELNET:}
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

    if [[ ${as[var]} == '' ]] then
        print -u2 vg_telnet_driver: no variable specified for $name
        continue
    fi
    tool=
    case $mode in
    embedded)
        if [[ ${as[tool]} == '' ]] then
            print -u2 vg_telnet_driver: no tool specified for $name
            continue
        fi
        tool=vg_telnet_fun_${as[tool]}
        ji=embedded
    esac

    if [[ ${js[$ji].mode} == '' ]] then
        js[$ji]=( mode=$mode; typeset -A ms; typeset -A as; typeset -A tools )
    fi
    js[$ji].ms[$name]=(
        name=$name type=$type unit=$unit impl=$impl typeset -A as
    )
    for ai in "${!as[@]}"; do
        js[$ji].ms[$name].as[$ai]=${as[$ai]}
    done

    if [[ $tool != '' ]] then
        if [[ ${js[$ji].tools[$tool]} == '' ]] then
            . $VG_SSCOPESDIR/current/telnet/${tool}
            ${tool}_init
            js[$ji].tools[$tool]=$tool
        fi
        ${tool}_add
    fi

    js[$ji].as[port]=${as[port]}
    js[$ji].as[user]=${as[user]}
    js[$ji].as[pass]=${as[pass]}
    if [[ ${as[user]} != '' ]] then
        js[$ji].as[ustr]="${as[user]}@"
    fi
done
IFS="$ifs"

for ji in "${!js[@]}"; do
    typeset -n jr=js[$ji]

    if [[ ${jr.as[pass]} == SWENC*SWENC*SWENC ]] then
        pass=${jr.as[pass]}
        builtin -f swiftsh swiftshenc swiftshdec
        builtin -f codex codex
        phrase=${pass#SWENC*SWENC}; phrase=${phrase%SWENC}
        swiftshdec phrase code
        pass=${pass#SWENC}; pass=${pass%%SWENC*}
        print -r "$pass" \
        | codex --passphrase="$code" "<uu-base64-string<aes" \
        | read -r pass
        jr.as[pass]=$pass
    fi
    if [[ ${jr.as[pass]} == SWMUL*SWMUL && -f telnet.lastmulti ]] then
        touch telnet.lastmulti
    fi

    export TELNETUSER=${jr.as[user]} TELNETPASS=${jr.as[pass]}
    export TELNETPORT=${jr.as[port]}

    case ${jr.mode} in
    embedded)
        for tool in "${!jr.tools[@]}"; do
            ${tool}_run $target
        done
        for tool in "${!jr.tools[@]}"; do
            ${tool}_emit
        done
        if [[ $? != 0 ]] then
            print -u2 "vg_telnet_driver: failed to execute embedded tools"
        fi
        ;;
    esac
done

print "${vars}"
