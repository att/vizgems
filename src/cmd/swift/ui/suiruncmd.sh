#!/bin/ksh

export SHELL=$(which ksh 2> /dev/null)
[[ $SECONDS != *.* ]] && exec $SHELL $0 "$@"

typeset -A validtools
validtools[suiplot]=1
validtools[suigmap]=1
validtools[suigraph]=1
validtools[suibrowser]=1

tmpfile=${TMPDIR:-/tmp}/suiruncmd.$USER.$$
trap 'rm -f $tmpfile' HUP QUIT TERM KILL EXIT

localdir=${TMPDIR:-/tmp}/sui.$(id -un)
mkdir -p $localdir

exec 3< $1

suireadcmd 3 cmd args

export SWIFTINSTANCENAME=${args.instancename}

suidclient "\
    $localdir/${args.instance}${args.prefprefix}.lefty \
    http://${args.dname}:${args.dport}${args.prefprefix}.lefty \
" > /dev/null 2>&1

case $cmd in
launch)
    if [[ ${args.mname} != '' ]] then
        mname=${args.mname}
        mport=${args.mport}
    else
        mname=$(swmhostname)
        mport=$(
            suimserver -regname ${args.rname}/${args.rport} \
                -regkey ${args.rkey} \
            | egrep port | sed 's/^.* //'
        )
        suimhistory ${args.instance} $mname $mport \
            ${args.dname} ${args.dport} \
            $localdir/${args.instance} ${args.prefprefix} \
        &
    fi
    for tool in ${args.tools}; do
        lang=${tool#*:}
        tool=${tool%:*}
        if [[ $lang == java || $lang == nolaunch || $lang == none ]] then
            continue
        fi
        if [[ ${validtools[$tool]} == '' ]] then
            print -u2 ERROR tool $tool not in valid list
            exit 1
        fi
        $tool ${args.instance} $mname $mport \
            ${args.dname} ${args.dport} \
            $localdir/${args.instance} ${args.prefprefix} \
        &
    done
    ;;
esac
