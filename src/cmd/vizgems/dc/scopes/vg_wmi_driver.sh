
if [[ $SWIFTWARNLEVEL != "" ]] then
    PS4='${.sh.file}-${.sh.fun}-$LINENO: '
    exec 2> wmi_driver.log
    set -x
fi

usage=$'
[-1p1?
@(#)$Id: vg_wmi_driver (AT&T Labs Research) 2006-03-12 $
]
'$USAGE_LICENSE$'
[+NAME?vg_wmi_driver - extract system information using the WMI interface]
[+DESCRIPTION?\bvg_wmi_driver\b extracts information through the WMI interface.
The host name and type are specified on the command line.
The parameters to extract appear in stdin, one per line.
The first word in each line is the standard parameter name.
The rest of the text in the line is a CGI-style string.
The path name of the CGI can be \braw\b, to indicate that a single parameter
is to be collected.
When the path name is not \braw\b, it is assumed to indicate the suffix of the
name of a script.
The full script name will be \bvg_wmi_cmd_\bname, eg \bvg_wmi_cmd_xyz\b.
The output of that script must be zero or more lines of the form:
\brt=STAT ... stat param key=value pairs ...\b or
\brt=ALARM ... alarm param key=value pairs ...\b
The output of \bvg_wmi_driver\b is a \bksh\b compound variable containing
all the data from all the raw and scripted executions.
]
[999:v?increases the verbosity level. May be specified multiple times.]

targetname targetaddr targettype

[+SEE ALSO?\bVizGEMS\b(1)]
'

while getopts -a vg_wmi_driver "$usage" opt; do
    case $opt in
    999) (( SWIFTWARNLEVEL++ )) ;;
    *) OPTIND=0 getopts -a vg_wmi_driver "$usage" opt '-?'; exit 1 ;;
    esac
done
shift $OPTIND-1

targetname=$1
targetaddr=$2
targettype=$3

set -o pipefail

if [[ $VG_SCOPENAME == $targetname ]] then
    target=.
else
    target=$targetaddr
fi

vars=()
varn=0
typeset -A js
otheri=0

ifs="$IFS"
IFS='|'
while read -r name type unit impl; do
    impl=${impl#WMI:}
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

    timeout=120
    case $mode in
    raw)
        if [[ ${as[var]} == '' ]] then
            print -u2 vg_wmi_driver: no variable specified for $name
            continue
        fi
        ji=raw
        ;;
    log)
        if [[ ${as[file]} == '' ]] then
            print -u2 vg_wmi_driver: no log file specified for $name
            continue
        fi
        ji=log
        timeout=250
        ;;
    *)
        ji=other.$otheri
        (( otheri++ ))
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
    js[$ji].as[user]=${as[user]}
    js[$ji].as[pass]=${as[pass]}
    js[$ji].as[timeout]=${as[timeout]:-$timeout}
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
            print -u2 vg_wmi_driver: timeout for job "$@"
        fi
    fi
    wait $jid 2> /dev/null
    return $?
}

if [[ $SWIFTWARNLEVEL != "" ]] then
    typeset -ft timedrun
fi

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
    if [[ ${jr.as[pass]} == SWMUL*SWMUL && -f wmi.lastmulti ]] then
        touch wmi.lastmulti
    fi

    unset WMIUSER WMIPASSWD
    if [[ $target != . ]] then
        [[ ${jr.as[user]} != '' ]] && export WMIUSER=${jr.as[user]}
        [[ ${jr.as[pass]} != '' ]] && export WMIPASSWD=${jr.as[pass]}
    fi

    case ${jr.mode} in
    raw)
        for name in "${!jr.ms[@]}"; do
            typeset -n mr=jr.ms[$name]
            line="$name|${mr.type}|${mr.as[unit]}|${mr.as[var]}"
            line+="|${mr.as[mt]}|${mr.as[except]}|${mr.as[label]}"
            print -r -- "$line"
        done > wmi.tdfile
        [[ ! -f wmi.tsfile ]] && > wmi.tsfile
        if (( $(date -f '%#' -m wmi.tsfile) < $VG_JOBTS - $VG_JOBIV * 3 )) then
            print -u2 WARNING tstate file too old - clearing
            > wmi.tsfile
        fi
        timedrun ${jr.as[timeout]} \
            vgwmi -h $target -t wmi.tdfile -s wmi.tsfile \
        > wmi.tout
        while read -r line; do
            typeset -n vr=vars._$varn
            (( varn++ ))
            eval vr=\( $line \)
        done < wmi.tout
        ;;
    log)
        for name in "${!jr.ms[@]}"; do
            typeset -n mr=jr.ms[$name]
            line="$name|${mr.as[file]}|${mr.as[source]}|${mr.as[etype]}"
            print -r -- "$line"
        done > wmi.ldfile
        [[ ! -f wmi.lsfile ]] && > wmi.lsfile
        if (( $(date -f '%#' -m wmi.lsfile) < $VG_JOBTS - $VG_JOBIV * 3 )) then
            print -u2 WARNING lstate file too old - clearing
            > wmi.lsfile
        fi
        timedrun ${jr.as[timeout]} \
            vgwmi -h $target -l wmi.ldfile -s wmi.lsfile \
        > wmi.lout
        while read -r line; do
            typeset -n vr=vars._$varn
            (( varn++ ))
            eval vr=\( $line \)
        done < wmi.lout
        ;;
    *)
        name="${!jr.ms[@]}"
        typeset -n mr=jr.ms[$name]
        print "${mr.name}|${mr.type}|${mr.unit}|${mr.impl}" \
        | $VG_SSCOPESDIR/current/wmi/vg_wmi_cmd_${jr.mode} \
            $targetname $targetaddr $targettype \
        | while read -r line; do
            typeset -n vr=vars._$varn
            (( varn++ ))
            eval vr=\( $line \)
        done
        if [[ $? != 0 ]] then
            print -u2 "vg_wmi_driver: failed to execute command ${jr.mode}"
        fi
        ;;
    esac
done

print "${vars}"
