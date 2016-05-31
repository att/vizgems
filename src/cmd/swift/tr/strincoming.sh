
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
autoload ${instance}_init ${instance}_${group}_inc_init
autoload ${instance}_${group}_inc_fileinfo

if [[ $SWIFTWARNLEVEL != '' ]] then
    typeset -ft strensuredirs
    typeset -ft strruncheck strspacecheck
    typeset -ft ${instance}_init ${instance}_${group}_inc_init
    typeset -ft ${instance}_${group}_inc_fileinfo
    set -x
fi

gvars=(
    maindir=UNSET
    feeddir=UNSET

    mainspace=UNSET
    cachespace=UNSET

    filetype=UNSET
    filepatincl=UNSET
    filepatexcl=UNSET
    filepatremove=UNSET
    filecheckuse=UNSET
    filesperround=UNSET
    filesinparallel=UNSET
    fileremove=UNSET
    filemover=UNSET
)

${instance}_init gvars $group
if [[ $? != 0 ]] then
    print -u2 ERROR cannot invoke instance customization function
    exit 1
fi
${instance}_${group}_inc_init gvars
if [[ $? != 0 ]] then
    print -u2 ERROR cannot invoke group customization function
    exit 1
fi
if [[ -f ${gvars.maindir}/etc/${instance}_${group}_inc_vars ]] then
    . ${gvars.maindir}/etc/${instance}_${group}_inc_vars gvars
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

incdir=${gvars.maindir}/incoming/${gvars.feeddir}
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
if [[ $cachedirs == "" ]] then
    print -u2 ERROR no cache directories found
    exit
fi

logfile=$logdir/inc.$group.$(date -f '%Y%m%d-%H%M%S').log
gvars.exitfile=${gvars.maindir}/exit.inc.$group
gvars.lockfile=${gvars.maindir}/lock.inc.$group

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

(( gvars.filesinparallel++ ))

strruncheck ${gvars.exitfile} || cleanexit
cdirn=0
for cachedir in $cachedirs; do
    if strspacecheck $cachedir ${gvars.cachespace}; then
        cdirs[$cdirn]=$cachedir
        (( cdirn++ ))
    fi
done
(( cdirn > 0 )) || cleanexit
filesthisround=0
tw -i -H -d $incdir -e "
    type==${gvars.filetype} && name==\"${gvars.filepatincl}\" &&
    name!=\"${gvars.filepatexcl}\"
" 2> /dev/null | while read inpath; do
    inname=${inpath##*/}
    if [[ $inname == ${gvars.filepatremove} ]] then
        rm -f $inpath
        continue
    fi
    ovars=(outname=ERROR)
    ${instance}_${group}_inc_fileinfo gvars ovars $inname
    if [[ ${ovars.outname} == ERROR ]] then
        print -u2 ERROR unknown file pattern $inname
        continue
    fi
    cdir=${cdirs[$(( filesthisround % cdirn ))]}
    (
        exec 3< ${gvars.lockfile}
        if [[
            ${gvars.filecheckuse} == y && $(fuser $inpath 2> /dev/null) != ''
        ]] then
            exit
        fi
        print begin inc processing file $inname
        if ! ${gvars.filemover} $inpath $cdir/${ovars.outname}; then
            print -u2 ERROR move failed for file $infile to $cdir
        fi
        print end inc processing file $inname
    ) &
    strjobcntl ${gvars.filesinparallel}
    (( filesthisround++ ))
    if (( $filesthisround == ${gvars.filesperround} )) then
        strjobcntl 2
        strruncheck ${gvars.exitfile} || break
        cdirn=0
        for cachedir in $cachedirs; do
            if strspacecheck $cachedir ${gvars.cachespace}; then
                cdirs[$cdirn]=$cachedir
                (( cdirn++ ))
            fi
        done
        (( cdirn > 0 )) || break
        filesthisround=0
    fi
done
wait; wait

cleanexit
