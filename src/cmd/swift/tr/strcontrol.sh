
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
    print -u2 "    [-maxdate <date>]  (default: ${gvars.maxdate})"
}

autoload strensuredirs
autoload strruncheck strspacecheck
autoload ${instance}_init ${instance}_cntl_init

if [[ $SWIFTWARNLEVEL != '' ]] then
    typeset -ft strensuredirs
    typeset -ft strruncheck strspacecheck
    typeset -ft ${instance}_init ${instance}_cntl_init
    set -x
fi

gvars=(
    maindir=UNSET

    files=UNSET
)

${instance}_init gvars
if [[ $? != 0 ]] then
    print -u2 ERROR cannot invoke instance customization function
    exit 1
fi
${instance}_cntl_init gvars
if [[ $? != 0 ]] then
    print -u2 ERROR cannot invoke group customization function
    exit 1
fi
if [[ -f ${gvars.maindir}/etc/${instance}_cntl_vars ]] then
    . ${gvars.maindir}/etc/${instance}_cntl_vars gvars
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

if [[ $1 == '' ]] then
    showusage
    exit
fi
cmd=$1
mode=${2:-nowait}

if ! strensuredirs ${gvars.maindir}; then
    exit 1
fi

case $cmd in
start)
    for file in ${gvars.files}; do
        rm -f ${gvars.maindir}/exit.$file
    done
    ;;
stop)
    for file in ${gvars.files}; do
        touch ${gvars.maindir}/exit.$file
    done
    if [[ $mode == wait ]] then
        for file in ${gvars.files}; do
            while [[ -f ${gvars.maindir}/lock.$file ]] do
                sleep 5
            done
        done
    fi
    ;;
esac
