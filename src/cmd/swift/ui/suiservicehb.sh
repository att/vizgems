#!/bin/ksh

PATH=$PATH:/usr/sbin

export SWMROOT=${0%/*}
SWMROOT=${SWMROOT%/*}
SWMROOT=${SWMROOT%/*}
SWMROOT=${SWMROOT%/*}
. $SWMROOT/bin/swmenv
[[ $SECONDS != *.* ]] && exec $SHELL $0 "$@"

argv0=${0##*/}
instance=${argv0%%_*}
rest=${argv0#$instance}
rest=${rest#_}
if [[ $instance == '' || $rest == '' ]] then
    print -u2 ERROR program name not in correct form
    exit 1
fi
. $SWMROOT/proj/$instance/bin/${instance}_env

${instance}_services -root $SWMROOT start

if [[ $SWIFTWEBMODE == secure ]] then
    for i in $SWMROOT/ipdir/*; do
        if [[ ! -f $i ]] then
            continue
        fi
        if [[ $(fuser $i 2> /dev/null) != '' ]] then
            continue
        fi
        rm $i
    done
fi
