#!/bin/ksh

export SHELL=$(which ksh 2> /dev/null)
[[ $SECONDS != *.* ]] && exec $SHELL $0 "$@"

function v2s { # $1 = cmd $2 = args
    print "begin"
    typeset -A seen
    typeset -n x=$2

    for i in "${!x.@}"; do
        k=${i#$2.}
        k=${k%%.*}
        if [[ ${seen[$k]} != '' ]] then
            continue
        fi
        seen[$k]=1
        typeset -n x=$2.$k
        print "$k=$x"
        typeset +n x
    done
    print "replay=1"
    print "end"
}

instance=$1
mserver=$2
mport=$3
dserver=$4
dport=$5
prefprefix=$6
dir=$7

suiencode | { read is; read os; }
exec 3<> /dev/tcp/$2/$3
print -u3 ASWIFTMSERVER
read -u3 ret
[[ $ret == YES ]] || exit 1
print -u3 $is
print -u3 $os
read -u3 ret
[[ $ret == YES ]] || exit 1
print -u3 primaryoff
print -u3 echooff

typeset -A list

while suireadcmd 3 cmd args; do
    if [[ $cmd != play ]] then
        suiwritecmd 4 $cmd args 4> $dir/msg.$RANDOM
    fi
    case $cmd in
    load)
        name=${args.name}
        list[$name]=$(v2s cmd args)
        ;;
    unload)
        name=${args.name}
        list[$name]=
        ;;
    history)
        for name in "${!list[@]}"; do
            if [[ ${list[$name]} != '' ]] then
                print -u3 "${list[$name]}"
            fi
        done
        ;;
    esac
done
