#!/bin/ksh

dir=$SWIFTMSDIR_PLACEHOLDER
display=$SWIFTMSDISPLAY_PLACEHOLDER

minspace=100

if [[ $(whence -p $name) == "" ]] then
    PATH=$dir/bin:$PATH
    export FPATH=$dir/fun:$FPATH
fi

function showusage {
    print -u2 "$name [optional flags]"
    print -u2 "    [-dir <dir>]       (default: $dir)"
    print -u2 "    [-minspace <megs>] (default: $minspace)"
}

if [[ $SWIFTWARNLEVEL != '' ]] then
    set -x
fi

if [[ -f $dir/etc/vars.sh ]] then
    . $dir/etc/vars.sh
fi

while (( $# > 0 )) do
    if [[ $1 == '-dir' ]] then
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
    else
        showusage
        exit
    fi
done

if [[ ! -d $dir ]] then
    print ERROR directory $dir not found
    exit 1
fi

queuedir=$dir/queue
completeddir=$dir/completed
logdir=$dir/logs

if [[ ! -d $dir ]] then
    print ERROR directory $dir not found
    exit 1
fi

for i in $queuedir $completeddir $logdir; do
    if [[ $i != '' && ! -d $i ]] then
        mkdir -p $i
        if [[ ! -d $i ]] then
            print ERROR directory $i not found
            exit 1
        fi
        chmod g+w $i
    fi
done

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

while true; do
    smsruncheck $exitfile || break
    smsspacecheck $dir $minspace || break
    count=0
    tw -i -H -d $queuedir -e 'name=="*[0-9].job"' | while read qfile; do
        jobid=${qfile##*/}
        jobid=${jobid%.job}
        if [[ $(fuser $qfile 2> /dev/null) != '' ]] then
            continue
        fi
        if [[ -f $completeddir/$jobid/$jobid.job ]] then
            rm -f $qfile
            continue
        fi
        mkdir $completeddir/$jobid
        chmod g+w $completeddir/$jobid
        export SWIFTMSJOBDIR=$completeddir/$jobid
        (
            exec 3< $lockfile
            print processing job $qfile
            . $qfile
            cd $JOB_dir || (print ERROR cannot cd to $JOB_dir; exit 1)
            export DISPLAY=$display
            $JOB_cmd $JOB_args > $completeddir/$jobid/$jobid.log
            JOB_status=$?
            if [[ $JOB_status != 0 ]] then
                print ERROR job $qfile had errors
            fi
            print JOB_status=$JOB_status >> $qfile
            mv $qfile $completeddir/$jobid
            print finished file $qfile
        )
        (( count++ ))
        if (( $count == 10 )) then
            smsruncheck $exitfile || break
            smsspacecheck $dir $minspace || break
            count=0
        fi
    done
    break
done

exec >&-
exec 1>&4

if [[ ! -s $logfile ]] then
    rm -f $logfile
fi
