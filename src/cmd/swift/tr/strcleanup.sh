
argv0=${0##*/}
if [[ $(whence -p $argv0) == "" ]] then
    print -u2 ERROR environment not set up
    exit 1
fi

instance=${argv0%%_*}
rest=${argv0#$instance}
rest=${rest#_}
if [[ $instance == '' || $rest == '' ]] then
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

autoload strensuredirs
autoload strruncheck strspacecheck
autoload ${instance}_init ${instance}_clean_init

if [[ $SWIFTWARNLEVEL != '' ]] then
    typeset -ft strensuredirs
    typeset -ft strruncheck strspacecheck
    typeset -ft ${instance}_init ${instance}_clean_init
    set -x
fi

gvars=(
    maindir=UNSET

    mainspace=UNSET
)

${instance}_init gvars ""
if [[ $? != 0 ]] then
    print -u2 ERROR cannot invoke instance customization function
    exit 1
fi
${instance}_clean_init gvars
if [[ $? != 0 ]] then
    print -u2 ERROR cannot invoke customization function
    exit 1
fi
if [[ -f ${gvars.maindir}/etc/${instance}_clean_vars ]] then
    . ${gvars.maindir}/etc/${instance}_clean_vars
    if [[ $? != 0 ]] then
        print -u2 ERROR cannot invoke customization script
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

incdir=${gvars.maindir}/incoming
logdir=${gvars.maindir}/logs
tmpdir=${gvars.maindir}/tmp
export TMPDIR=$tmpdir

if ! strensuredirs $incdir; then
    exit 1
fi
if ! strensuredirs -c $logdir $tmpdir; then
    exit 1
fi

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

logfile=$logdir/clean.$(date -f '%Y%m%d-%H%M%S').log
gvars.exitfile=${gvars.maindir}/exit.clean
gvars.lockfile=${gvars.maindir}/lock.clean

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

if [[ ${gvars.cleanlogs} != '' ]] then
    logremovedate=$(date -s -f %Y.%m.%d "${gvars.logremovedays} day ago")
    logcatdate=$(date -s -f %Y.%m.%d "${gvars.logcatdays} day ago")
    prefixlist=$(ls -1 $logdir | sed 's/\.[0-9].*$//' | sort -u)
    for prefix in $prefixlist; do
        for file in $logdir/$prefix.*; do
            if [[ ! -f $file ]] then
                continue
            fi
            base=${file##*/}
            rest=${base#$prefix.}
            year=${rest:0:4}
            month=${rest:4:2}
            day=${rest:6:2}
            if [[ $year.$month.$day < $logremovedate ]] then
                rm $file
            elif [[ $year.$month.$day < $logcatdate ]] then
                dayfile=${file//-*./-day.}
                if [[ $dayfile != $file ]] then
                    cat $file >> $dayfile
                    rm $file
                fi
            fi
        done
    done
fi

if [[ ${gvars.cleanfeeds} != '' ]] then
    for feed in ${gvars.cleanfeeds}; do
        tw -i -H -d $incdir/$feed -e '
            (type==REG && mtime < "'${gvars.feedremovedays}' day ago")
        ' rm
    done
fi

if [[ ${gvars.cleancaches} != '' ]] then
    for cachedir in $cachedirs; do
        tw -i -H -d $cachedir -e '
            (type==REG && mtime < "'${gvars.cacheremovedays}' day ago")
        ' rm
    done
fi

cleanexit
