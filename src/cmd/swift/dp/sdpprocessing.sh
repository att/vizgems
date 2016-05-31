
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
    if [[ $(egrep -v '^(begin|end)' $logfile | head -2) == '' ]] then
        rm -f $logfile
    fi
    exit
}

function showusage {
    print -u2 "$argv [optional flags]"
    print -u2 "    [-mindate <date>]       (default: ${gvars.mindate})"
    print -u2 "    [-maxdate <date>]       (default: ${gvars.maxdate})"
    print -u2 "    [-levels <levels>]      (default: all levels)"
    print -u2 "    [-locksuffix <string>]  (default: none)"
}

autoload sdpensuredualdirs sdpensuredirs
autoload sdpruncheck sdpspacecheck
autoload ${instance}_init ${instance}_${group}_proc_init

if [[ $SWIFTWARNLEVEL != '' ]] then
    typeset -ft sdpensuredualdirs sdpensuredirs
    typeset -ft sdpruncheck sdpspacecheck
    typeset -ft ${instance}_init ${instance}_${group}_proc_init
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

    level1procsteps=UNSET
    level2procsteps=UNSET
    level3procsteps=UNSET
)

${instance}_init gvars $group
if [[ $? != 0 ]] then
    print -u2 ERROR cannot invoke instance customization function
    exit 1
fi
${instance}_${group}_proc_init gvars
if [[ $? != 0 ]] then
    print -u2 ERROR cannot invoke group customization function
    exit 1
fi
if [[ -f ${gvars.maindir}/etc/${instance}_${group}_proc_vars ]] then
    . ${gvars.maindir}/etc/${instance}_${group}_proc_vars gvars
    if [[ $? != 0 ]] then
        print -u2 ERROR cannot invoke local customization script
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

for step in \
    ${gvars.level1procsteps} ${gvars.level2procsteps} ${gvars.level3procsteps}\
; do
    autoload ${instance}_${group}_${step}_proc_run
    if [[ $SWIFTWARNLEVEL != '' ]] then
        typeset -ft ${instance}_${group}_${step}_proc_run
    fi
done

levellist=' 1 2 3 '
locksuffix=
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
    elif [[ $1 == '-levels' ]] then
        if (( $# < 2 )) then
            showusage
            exit
        fi
        levellist=" $2 "
        shift; shift
    elif [[ $1 == '-locksuffix' ]] then
        if (( $# < 2 )) then
            showusage
            exit
        fi
        locksuffix=$2
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
recdir=${gvars.maindir}/record/proc.$group
logdir=${gvars.maindir}/logs
tmpdir=${gvars.maindir}/tmp/misc
export TMPDIR=$tmpdir

if ! sdpensuredirs $datadir $linkdir; then
    exit 1
fi
if ! sdpensuredirs -c $recdir $logdir $tmpdir; then
    exit 1
fi

logfile=$logdir/proc.$group$locksuffix.$(date -f '%Y%m%d-%H%M%S').log
gvars.exitfile=${gvars.maindir}/exit.proc.$group$locksuffix
gvars.lockfile=${gvars.maindir}/lock.proc.$group$locksuffix

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

for step in ${gvars.level1procsteps}; do
    [[ $levellist != *' 1 '* ]] && continue
    sdpruncheck ${gvars.exitfile} || break
    sdpspacecheck $datadir ${gvars.mainspace} || break
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
                if [[ -f $datadir/$y/$m/$d/complete.stamp ]] then
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
    ${instance}_${group}_${step}_proc_run \
        gvars "$datadir" "$linkdir" \
    ${dates[@]}
done
wait; wait

for step in ${gvars.level2procsteps}; do
    [[ $levellist != *' 2 '* ]] && continue
    sdpruncheck ${gvars.exitfile} || break
    sdpspacecheck $datadir ${gvars.mainspace} || break
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
            if [[ ! -d $datadir/$y/$m ]] then
                continue
            fi
            if [[
                $y.$m < ${gvars.mindate%.*} || $y.$m > ${gvars.maxdate%.*}
            ]] then
                continue
            fi
            dates[${#dates[@]}]=$y/$m
        done
    done
    ${instance}_${group}_${step}_proc_run gvars "$datadir" "$linkdir" ${dates[@]}
done
wait; wait

for step in ${gvars.level3procsteps}; do
    [[ $levellist != *' 3 '* ]] && continue
    sdpruncheck ${gvars.exitfile} || break
    sdpspacecheck $datadir ${gvars.mainspace} || break
    unset dates
    for year in $datadir/[0-9][0-9][0-9][0-9]; do
        y=${year##*/}
        if [[ -f $datadir/$y/complete.stamp ]] then
            continue
        fi
        if [[ ! -d $datadir/$y ]] then
            continue
        fi
        if [[ -f $datadir/$y/complete.stamp ]] then
            continue
        fi
        if [[
            $y < ${gvars.mindate%%.*} || $y > ${gvars.maxdate%%.*}
        ]] then
            continue
        fi
        dates[${#dates[@]}]=$y
    done
    ${instance}_${group}_${step}_proc_run gvars "$datadir" "$linkdir" ${dates[@]}
done
wait; wait

cleanexit
