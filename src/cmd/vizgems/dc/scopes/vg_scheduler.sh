#!/bin/ksh

exitfile=$VG_DSCOPESDIR/exit.schedule
lockfile=$VG_DSCOPESDIR/lock.schedule

if [[ -f $exitfile ]] then
    exit
fi
set -o noclobber
if ! command exec 3> ${lockfile}; then
    if kill -0 $(< ${lockfile}); then
        exit 0
    else
        rm -f ${lockfile}
        exit 0
    fi
fi 2> /dev/null
print -u3 $$
set +o noclobber

trap 'rm -f ${lockfile}' HUP QUIT TERM KILL EXIT

mkdir -p $VG_DSCOPESDIR
cd $VG_DSCOPESDIR || {
    print -u2 vg_scheduler: cannot cd $VG_DSCOPESDIR; exit 1
}

[[ -f ./etc/scheduler.info ]] && . ./etc/scheduler.info

timefile=./etc/time.diff

vgsched \
    -ld $VG_DSCOPESDIR/logs -ef $exitfile -j ${VG_SCHEDJOBS:-32} -tf $timefile \
    -rt ${VG_RECTIME:-schedule} \
schedule
