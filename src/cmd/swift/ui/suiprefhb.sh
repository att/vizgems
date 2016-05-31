#!/bin/ksh

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

for user in "$@"; do
    export SWMID=$user
    export SWMDIR=$SWMROOT/htdocs/members/$user
    prefdir=$SWIFTDATADIR/cache/profiles/$SWMID
    mkdir -p $prefdir
    for name in $SWMDIR/$instance.*.prefs; do
        prefname=${name##*/}
        prefname=${prefname#$instance.}
        prefname=${prefname%.prefs}
        if [[ $name -nt $prefdir/$instance.$prefname.prefs.sh ]] then
            suiprefgen \
                -oprefix $prefdir/$instance.$prefname.prefs \
            -prefname $prefname
        fi
    done
done
