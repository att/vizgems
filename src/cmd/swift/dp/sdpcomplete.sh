
argv0=${0##*/}
if [[ $(whence -p $argv0) == "" ]] then
    print -u2 ERROR environment not set up
    exit 1
fi

instance=${argv0%%_*}
rest=${argv0#$instance}
rest=${rest#_}
group=${rest%%_*}
rest=${rest#$group}
rest=${rest#_}
if [[ $instance == '' || $group == '' || $rest == '' ]] then
    print -u2 ERROR program name not in correct form
    exit 1
fi

function cleanexit {
    exec >&-
    exec 1>&4

    if [[ ! -s $logfile ]] then
        rm -f $logfile
    fi
    exit
}

function showusage {
    print -u2 "$argv [optional flags]"
    print -u2 "    [-mindate <date>]  (default: ${gvars.mindate})"
    print -u2 "    [-maxdate <date>]  (default: ${gvars.maxdate})"
}

autoload sdpensuredirs
autoload sdpruncheck sdpspacecheck
autoload ${instance}_init ${instance}_${group}_compl_init
autoload ${instance}_${group}_compl_run

if [[ $SWIFTWARNLEVEL != '' ]] then
    typeset -ft sdpensuredirs
    typeset -ft sdpruncheck sdpspacecheck
    typeset -ft ${instance}_init ${instance}_${group}_compl_init
    typeset -ft ${instance}_${group}_compl_run
    set -x
fi

gvars=(
    maindir=UNSET
    permdir=UNSET
    feeddir=UNSET
    datadir=

    mindate=UNSET
    maxdate=UNSET

    mainspace=UNSET
    permspace=UNSET

    level1=y
    level2=n
    level3=n
)

${instance}_init gvars $group
if [[ $? != 0 ]] then
    print -u2 ERROR cannot invoke instance customization function
    exit 1
fi
${instance}_${group}_compl_init gvars
if [[ $? != 0 ]] then
    print -u2 ERROR cannot invoke customization function
    exit 1
fi
if [[ -f ${gvars.maindir}/etc/${instance}_${group}_compl_vars ]] then
    . ${gvars.maindir}/etc/${instance}_${group}_compl_vars
    if [[ $? != 0 ]] then
        print -u2 ERROR cannot invoke customization script
        exit 1
    fi
fi
if [[ ${gvars.datadir} == "" ]] then
    gvars.datadir=${gvars.feeddir}
fi
for i in "${!gvars.@}"; do
    if [[ $(eval print \${$i}) == UNSET ]] then
        print -u2 ERROR variable $i not set
        exit 1
    fi
done

while (( $# > 0 )) do
    if [[ $1 == '-mindate' ]] then
        if (( $# < 2 )) then
            showusage
            exit
        fi
        gvars.mindate=$2
        shift; shift
    elif [[ $1 == '-maxdate' ]] then
        if (( $# < 2 )) then
            showusage
            exit
        fi
        gvars.maxdate=$2
        shift; shift
    else
        showusage
        exit
    fi
done

if ! sdpensuredirs ${gvars.maindir}; then
    exit 1
fi
if [[ ${gvars.permdir} != '' ]] && ! sdpensuredirs ${gvars.permdir}; then
    exit 1
fi

if [[ ${gvars.permdir} != "" ]] then
    datadir=${gvars.maindir}/data/${gvars.datadir}
    linkdir=${gvars.permdir}/data/${gvars.datadir}
else
    datadir=${gvars.maindir}/data/${gvars.datadir}
    linkdir=
fi
recdir=${gvars.maindir}/record/compl.$group
logdir=${gvars.maindir}/logs
tmpdir=${gvars.maindir}/tmp/misc
export TMPDIR=$tmpdir

if ! sdpensuredirs $datadir $linkdir; then
    exit 1
fi
if ! sdpensuredirs -c $recdir $logdir $tmpdir; then
    exit 1
fi

logfile=$logdir/compl.$group.$(date -f '%Y%m%d-%H%M%S').log
gvars.exitfile=${gvars.maindir}/exit.compl.$group
gvars.lockfile=${gvars.maindir}/lock.compl.$group

set -o noclobber
if ! command exec 3> ${gvars.lockfile}; then
    if kill -0 $(< ${gvars.lockfile}); then
        exit 0
    elif [[ $(fuser ${gvars.lockfile}) != '' ]] then
        exit 0
    else
        rm ${gvars.lockfile}
        exit 0
    fi
fi 2> /dev/null
print -u3 $$
set +o noclobber

trap 'rm -f ${gvars.lockfile}' HUP QUIT TERM KILL EXIT

exec 4>&1
exec >> $logfile
exec 2>&1

sdpruncheck ${gvars.exitfile} || cleanexit
sdpspacecheck $datadir ${gvars.mainspace} || cleanexit
unset dates
for year in $datadir/[0-9][0-9][0-9][0-9]; do
    y=${year##*/}
    if [[ -f $datadir/$y/complete.stamp ]] then
        continue
    fi
    for month in $year/[0-9][0-9]; do
        m=${month##*/}
        if [[ -f $datadir/$y/$m/complete.stamp ]] then
            continue
        fi
        for day in $month/[0-9][0-9]; do
            d=${day##*/}
            if [[ ! -d $datadir/$y/$m/$d ]] then
                continue
            fi
            if [[
                -f $datadir/$y/$m/$d/complete.stamp &&
                ! -f $datadir/$y/$m/$d/complete.stamp1
            ]] then
                continue
            fi
            if [[
                $y.$m.$d < ${gvars.mindate} || $y.$m.$d > ${gvars.maxdate}
            ]] then
                continue
            fi
            dates[${#dates[@]}]=$y/$m/$d
        done
    done
done
if [[ ${gvars.level1} == y ]] then
    ${instance}_${group}_compl_run gvars "$datadir" "$linkdir" ${dates[@]}
fi
wait; wait

for year in $datadir/[0-9][0-9][0-9][0-9]; do
    y=${year##*/}
    if [[ -f $datadir/$y/complete.stamp ]] then
        continue
    fi
    yeardone=y
    for month in $year/[0-9][0-9]; do
        m=${month##*/}
        if [[ -f $datadir/$y/$m/complete.stamp ]] then
            continue
        fi
        yeardone=n
        monthdone=y
        for day in $month/[0-9][0-9]; do
            d=${day##*/}
            if [[ ! -d $datadir/$y/$m/$d ]] then
                continue
            fi
            if [[
                -f $datadir/$y/$m/$d/complete.stamp &&
                ! -f $datadir/$y/$m/$d/complete.stamp1
            ]] then
                continue
            fi
            monthdone=n
        done
        if [[ $monthdone == y ]] then
            if [[ ${gvars.level2} == y ]] then
                ${instance}_${group}_compl_run gvars "$datadir" "$linkdir" $y/$m
            fi
            wait; wait
            touch $datadir/$y/$m/complete.stamp
        fi
    done
    if [[ $yeardone == y ]] then
        if [[ ${gvars.level3} == y ]] then
            ${instance}_${group}_compl_run gvars "$datadir" "$linkdir" $y
        fi
        wait; wait
        touch $datadir/$y/complete.stamp
    fi
done

cleanexit
