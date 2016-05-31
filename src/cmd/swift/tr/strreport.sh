
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

function showusage {
    print -u2 "$argv [optional flags]"
}

autoload strensuredirs
autoload strruncheck strspacecheck
autoload ${instance}_init ${instance}_rep_init

if [[ $SWIFTWARNLEVEL != '' ]] then
    typeset -ft strensuredirs
    typeset -ft strruncheck strspacecheck
    typeset -ft ${instance}_init ${instance}_rep_init
    set -x
fi

gvars=(
    maindir=UNSET
)

${instance}_init gvars ""
if [[ $? != 0 ]] then
    print -u2 ERROR cannot invoke instance customization function
    exit 1
fi
${instance}_rep_init gvars
if [[ $? != 0 ]] then
    print -u2 ERROR cannot invoke customization function
    exit 1
fi
if [[ -f ${gvars.maindir}/etc/${instance}_rep_vars ]] then
    . ${gvars.maindir}/etc/${instance}_rep_vars
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

logdir=${gvars.maindir}/logs
tmpdir=${gvars.maindir}/tmp
export TMPDIR=$tmpdir

if ! strensuredirs -c $logdir $tmpdir; then
    exit 1
fi

gvars.exitfile=${gvars.maindir}/exit.rep
gvars.lockfile=${gvars.maindir}/lock.rep

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

strruncheck ${gvars.exitfile} || exit

date=$(date -s -f %Y%m%d "1 day ago")

print ""
print "Data Transfer Report for ${gvars.name} / $date"
print -- "-----------------------------------------------"
print ""

print "Feeds"
print -- "-----"
print ""

prefixlist=$(ls -1 $logdir | sed 's/\.[0-9].*$//' | sort -u)
for prefix in $prefixlist; do
    if [[ $prefix != out.* ]] then
        continue
    fi
    print "${prefix#*.} - \c"
    tw -i -H -d $logdir -e "name==\"$prefix.${date}-*\"" cat \
    | egrep '^(end|bad) transfer' | sutawk '{
        if ($1 == "bad") {
            bad++
            badbytes += $5
        } else {
            good++
            goodbytes += $5
        }
    }
    END{
        goodg = goodbytes / 1000000000.0
        badg = badbytes / 1000000000.0
        goodbw = goodbytes / (1000 * 24 * 60 * 60)
        printf "copied %d files (%6.2fGB - %4.2fKB/sec)", good, goodg, goodbw
        if (bad > 0)
            printf " %d failures (%6.2fGB)", bad, badg
        print ""
    }'
done

print ""
print "Disk Status"
print -- "-----------"
print ""

df -F xfs
