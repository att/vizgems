
if [[ $SWIFTWARNLEVEL != "" ]] then
    PS4='${.sh.file}-${.sh.fun}-$LINENO: '
    exec 2> ssh_driver.log
    set -x
fi

usage=$'
[-1p1?
@(#)$Id: vg_ssh_driver (AT&T Labs Research) 2006-03-12 $
]
'$USAGE_LICENSE$'
[+NAME?vg_ssh_driver - collect information using SSH]
[+DESCRIPTION?\bvg_ssh_driver\b collects system information by running various
commands on a remote system, through ssh.
The host name and type are specified on the command line.
The parameters to extract appear in stdin, one per line.
The first word in each line is the standard parameter name.
The rest of the text in the line is a CGI-style string.
The path name of the CGI is assumed to indicate the suffix of the
name of a script.
The full script name will be \bvg_ssh_cmd_\bname, eg \bvg_ssh_cmd_xyz\b.
The output of that script must be zero or more lines of the form:
\brt=STAT ... stat param key=value pairs ...\b or
\brt=ALARM ... alarm param key=value pairs ...\b
The output of \bvg_ssh_driver\b is a \bksh\b compound variable containing
all the data from all the executions.
]
[999:v?increases the verbosity level. May be specified multiple times.]

targetname targetaddr targettype

[+SEE ALSO?\bVizGEMS\b(1)]
'

while getopts -a vg_ssh_driver "$usage" opt; do
    case $opt in
    999) (( SWIFTWARNLEVEL++ )) ;;
    *) OPTIND=0 getopts -a vg_ssh_driver "$usage" opt '-?'; exit 1 ;;
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
    impl=${impl#SSH:}
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
        print -u2 vg_ssh_driver: no variable specified for $name
        continue
    fi
    tool=
    case $mode in
    raw)
        if [[ ${as[path]} == '' ]] then
            print -u2 vg_ssh_driver: no command specified for $name
            continue
        fi
        ji=raw
        ;;
    cooked)
        if [[ ${as[tool]} == '' ]] then
            print -u2 vg_ssh_driver: no tool specified for $name
            continue
        fi
        tool=vg_ssh_fun_${as[tool]}
        ji=cooked
        ;;
    embedded)
        if [[ ${as[tool]} == '' ]] then
            print -u2 vg_ssh_driver: no tool specified for $name
            continue
        fi
        tool=vg_ssh_fun_${as[tool]}
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
            . $VG_SSCOPESDIR/current/ssh/${tool}
            ${tool}_init
            js[$ji].tools[$tool]=$tool
        fi
        ${tool}_add
    fi

    js[$ji].as[port]=${as[port]}
    js[$ji].as[timeout]=${as[timeout]}
    js[$ji].as[user]=${as[user]}
    js[$ji].as[pass]=${as[pass]}
    if [[ ${as[user]} != '' ]] then
        js[$ji].as[ustr]="${as[user]}@"
    fi
done
IFS="$ifs"

> ssh.err

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
    if [[ ${jr.as[pass]} == SWMUL*SWMUL && -f ssh.lastmulti ]] then
        touch ssh.lastmulti
    fi

    unset SSHKEY
    if [[ ${jr.as[pass]} == sshkey=* ]] then
        print -- "${jr.as[pass]#*=}" > ssh.key
        chmod 600 ssh.key
        export SSHKEY=./ssh.key
        unset jr.as[pass]
    fi

    export SSHUSER=${jr.as[user]} SSHPASS=${jr.as[pass]}
    export SSHPORT=${jr.as[port]} SSHTIMEOUT=${jr.as[timeout]}
    export SSHDEST=${jr.as[ustr]}$target

    case ${jr.mode} in
    raw)
        for name in "${!jr.ms[@]}"; do
            typeset -n mr=jr.ms[$name]
            vgssh ${jr.as[ustr]}$target "${mr.as[path]}" 2>> ssh.err \
            | while read -r line; do
                typeset -n vr=vars._$varn
                (( varn++ ))
                eval vr=\( $line \)
            done
        done
        ;;
    cooked)
        for tool in "${!jr.tools[@]}"; do
            ${tool}_send | sed "s!\$! | sed 's/^/A${tool}Z/'!" 2> /dev/null
        done | vgssh ${jr.as[ustr]}$target 2>> ssh.err \
        | while read -r line; do
            [[ $line != A*Z* ]] && continue
            tool=${line%%Z*}
            tool=${tool#A}
            line=${line##A"$tool"Z}
            ${tool}_receive "$line"
        done
        for tool in "${!jr.tools[@]}"; do
            ${tool}_emit
        done
        for tool in "${!jr.tools[@]}"; do
            ${tool}_term
        done
        if [[ $? != 0 ]] then
            print -u2 "vg_ssh_driver: failed to execute cooked tools"
        fi
        ;;
    embedded)
        for tool in "${!jr.tools[@]}"; do
            ${tool}_run ${jr.as[ustr]}$target 2>> ssh.err
        done
        for tool in "${!jr.tools[@]}"; do
            ${tool}_emit
        done
        if [[ $? != 0 ]] then
            print -u2 "vg_ssh_driver: failed to execute embedded tools"
        fi
        ;;
    esac
done

if fgrep 'REMOTE HOST IDENTIFICATION HAS CHANGED' ssh.err > /dev/null 2>&1; then
    print -u2 SWIFT ERROR: ssh key has changed for asset "$targetname"
    ssh-keygen -R $target > /dev/null 2>&1
fi

print "${vars}"
