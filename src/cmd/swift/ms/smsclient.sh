#!/bin/ksh

dir=$SWIFTMSDIR_PLACEHOLDER

mode=submit

jobdir=.
jobcmd=
jobargs=

minspace=100

if [[ $(whence -p $name) == "" ]] then
    PATH=$dir/bin:$PATH
    export FPATH=$dir/fun:$FPATH
fi

function showusage {
    print -u2 "$name [optional flags]"
    print -u2 "    [-serverdir <dir>] (default: $dir)"
    print -u2 "    [-minspace <megs>] (default: $minspace)"
    print -u2 "    [-query]"
    print -u2 "    [-remove <jobid>]"
    print -u2 "    [-dir <dir>]       (default: $jobdir)"
    print -u2 "    [-cmd <cmd>]"
    print -u2 "    [-args <args>]"
}

if [[ $SWIFTWARNLEVEL != '' ]] then
    set -x
fi

while (( $# > 0 )) do
    if [[ $1 == '-serverdir' ]] then
        if (( $# < 2 )) then
            showusage
            exit
        fi
        dir=$2
        shift; shift
    elif [[ $1 == '-minspace' ]] then
        if (( $# < 2 )) then
            showusage
            exit
        fi
        minspace=$2
        shift; shift
    elif [[ $1 == '-remove' ]] then
        if (( $# < 2 )) then
            showusage
            exit
        fi
        mode=remove
        jobid=$2
        shift; shift
    elif [[ $1 == '-query' ]] then
        mode=query
        shift
    elif [[ $1 == '-dir' ]] then
        if (( $# < 2 )) then
            showusage
            exit
        fi
        jobdir=$2
        shift; shift
    elif [[ $1 == '-cmd' ]] then
        if (( $# < 2 )) then
            showusage
            exit
        fi
        jobcmd=$2
        shift; shift
    elif [[ $1 == '-args' ]] then
        if (( $# < 2 )) then
            showusage
            exit
        fi
        jobargs=$2
        shift; shift
    else
        showusage
        exit
    fi
done

if [[ $mode == submit ]] then
    if [[ $jobdir == '' ]] then
        print -u2 ERROR missing job directory
        exit 1
    elif [[ $jobdir == . ]] then
        jobdir=$PWD
    fi
    if [[ $jobcmd == '' ]] then
        print -u2 ERROR missing command name
        exit 1
    fi
fi

if [[ ! -d $dir ]] then
    print ERROR directory $dir not found
    exit 1
fi

queuedir=$dir/queue
completeddir=$dir/completed
logdir=$dir/logs

if [[ ! -d $queuedir ]] then
    print ERROR directory $queuedir not found
    exit 1
elif [[ ! -d $completeddir ]] then
    print ERROR directory $completeddir not found
    exit 1
elif [[ ! -d $logdir ]] then
    print ERROR directory $logdir not found
    exit 1
fi

logfile=$logdir/$name.$(date -f '%Y%m%d-%H%M%S').log
exitfile=$dir/exit$name
lockfile=$dir/lock$name

set -o noclobber
if ! command exec 3> $lockfile; then
    if [[ $(fuser $lockfile) != '' ]] then
        exit 0
    elif kill -0 $(< $lockfile); then
        exit 0
    else
        rm $lockfile
        exit 0
    fi
fi 2> /dev/null
print -u3 $$
set +o noclobber

trap 'rm -f $lockfile' HUP QUIT TERM KILL EXIT

exec 4>&1
exec > $logfile
exec 2>&1

case $mode in
submit)
    smsruncheck $exitfile || exit
    smsspacecheck $dir $minspace || exit
    count=0
    qname=$(date -f '%Y.%m.%d-%H%M%S').job
    while [[ -f $queuedir/$qname || -d $completeddir/${qname%.job} ]] do
        if (( count++ >= 20 )) then
            print ERROR cannot create job file
            qname=
            break
        fi
        sleep 1
        qname=$(date -f '%Y.%m.%d-%H%M%S').job
    done
    (
        print export JOB_user=$(id -nu)
        print export JOB_dir=$jobdir
        print export JOB_cmd=$jobcmd
        print export JOB_args=\"$jobargs\"
    ) > $queuedir/$qname
    exec >&-
    exec 1>&4
    ;;
query)
    smsruncheck $exitfile || exit
    exec >&-
    exec 1>&4
    for i in $queuedir/[0-9]*.job; do
        if [[ ! -f $i ]] then
            break
        fi
        jobid=${i##*/}
        jobid=${jobid%.job}
        jobuser=$(egrep "JOB_user=$(id -nu)" $i)
        jobuser=${jobuser##*=}
        print $jobid $jobuser
    done
    ;;
remove)
    smsruncheck $exitfile || exit
    exec >&-
    exec 1>&4
    if egrep "JOB_user=$(id -nu)" $queuedir/$jobid.job > /dev/null; then
        rm -f $queuedir/$jobid.job
    fi
    ;;
esac

if [[ ! -s $logfile ]] then
    rm -f $logfile
fi
