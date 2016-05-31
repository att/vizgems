#!/bin/ksh

PATH=$PATH:/usr/sbin

export SWMROOT=${0%/*}
SWMROOT=${SWMROOT%/*}
. $SWMROOT/bin/swmenv
[[ $SECONDS != *.* ]] && exec $SHELL $0 "$@"

mustrestart=n
for l in $SWMROOT/logs/*_log; do
    if [[ ! -f $l ]] then
        continue
    fi
    ls -l $l | read j1 j2 j3 j4 s j5
    if (( s > 10 * 1024 * 1024 )) then
        print -u2 rotating $l
        rm -f $l.5
        for v in 4 3 2 1; do
            if [[ ! -f $l.$v ]] then
                continue
            fi
            ((vp1 = v + 1))
            mv $l.$v $l.$vp1
        done
        mv $l $l.1
        mustrestart=y
    fi
    touch $l
done

if [[ -x $SWMROOT/bin/apachectl ]] then
    if [[ $mustrestart == y ]] then
        print -u2 restarting web server
        $SWMROOT/bin/apachectl graceful > /dev/null
    else
        $SWMROOT/bin/apachectl start > /dev/null 2>&1
    fi
fi
