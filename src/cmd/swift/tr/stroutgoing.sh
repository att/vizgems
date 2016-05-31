
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
}

autoload strensuredirs
autoload strruncheck strspacecheck
autoload ${instance}_init ${instance}_${group}_out_init

if [[ $SWIFTWARNLEVEL != '' ]] then
    typeset -ft strensuredirs
    typeset -ft strruncheck strspacecheck
    typeset -ft ${instance}_init ${instance}_${group}_out_init
    set -x
fi

gvars=(
    maindir=UNSET
    feeddir=UNSET

    remotehost=UNSET
    remotedir=UNSET
    filespermove=UNSET
    filesinparallel=UNSET
    filemover=UNSET
)

${instance}_init gvars $group
if [[ $? != 0 ]] then
    print -u2 ERROR cannot invoke instance customization function
    exit 1
fi
${instance}_${group}_out_init gvars
if [[ $? != 0 ]] then
    print -u2 ERROR cannot invoke group customization function
    exit 1
fi
if [[ -f ${gvars.maindir}/etc/${instance}_${group}_out_vars ]] then
    . ${gvars.maindir}/etc/${instance}_${group}_out_vars gvars
    if [[ $? != 0 ]] then
        print -u2 ERROR cannot invoke local customization script
        exit 1
    fi
fi
for i in "${!gvars.@}"; do
    if [[ $(eval print \${$i}) == UNSET ]] then
        print -u2 ERROR variable $i not set
        exit 1
    fi
done

if ! strensuredirs ${gvars.maindir}; then
    exit 1
fi

logdir=${gvars.maindir}/logs
tmpdir=${gvars.maindir}/tmp
export TMPDIR=$tmpdir

if ! strensuredirs -c $logdir $tmpdir; then
    exit 1
fi

(( gvars.filesinparallel++ ))

cachedirs=
for cachedir in ${gvars.maindir}/cache*; do
    if [[ $(df -P -m $cachedir | egrep $cachedir) == *$cachedir ]] then
        if strensuredirs -c $cachedir/${gvars.feeddir}; then
            cachedirs="$cachedirs $cachedir/${gvars.feeddir}"
        else
            print -u2 ERROR directory $cachedir/${gvars.feeddir} not found
        fi
    fi
done
cachedirs=${cachedirs# }
if [[ $cachedirs == "" ]] then
    print -u2 ERROR no cache directories found
    exit
fi

logfile=$logdir/out.$group.$(date -f '%Y%m%d-%H%M%S').log
gvars.exitfile=${gvars.maindir}/exit.out.$group
gvars.lockfile=${gvars.maindir}/lock.out.$group

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
exec > $logfile
exec 2>&1

strruncheck ${gvars.exitfile} || cleanexit
for cachedir in $cachedirs; do
    (
        filesthismove=0
        filestomove=""
        tw -i -H -d $cachedir -e "type==REG && name!=\"*.tmp\"" 2> /dev/null \
        | while read inpath; do
            filestomove="$filestomove $inpath"
            if (( ++filesthismove >= ${gvars.filespermove} )) then
                (
                    exec 3< ${gvars.lockfile}
                    print begin out processing
                    ${gvars.filemover} gvars $filestomove
                    print end out processing
                ) &
                sleep 1
                strjobcntl ${gvars.filesinparallel}
                strruncheck ${gvars.exitfile} || break
                filesthismove=0
                filestomove=""
            fi
        done
        if (( filesthismove > 0 )) then
            (
                print begin out processing
                ${gvars.filemover} gvars $filestomove
                print end out processing
            )
        fi
        wait; wait
    ) &
    sleep 1
done
wait; wait

cleanexit
