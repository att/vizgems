#!/bin/ksh

export SHELL=$(which ksh 2> /dev/null)
[[ $SECONDS != *.* ]] && exec $SHELL $0 "$@"
if [[ $SWMROOT != '' && $SWMID == '' ]] then
    . $SWMROOT/bin/swmgetinfo
fi

argv0=${0##*/}
if [[ $(whence -p $argv0) == "" ]] then
    print -u2 ERROR environment not set up
    exit 1
fi

instance=${argv0%%_*}
rest=${argv0#$instance}
if [[ $instance == '' || $rest == '' ]] then
    print -u2 ERROR program name not in correct form
    exit 1
fi

trap \
    '[[ ${gvars.lockfile} != "" ]] && rm -f ${gvars.lockfile}' \
HUP QUIT TERM KILL EXIT

function wait4lock { # $1 = lock file
    set -o noclobber
    mkdir -p ${1%.lock}
    while ! command exec 3> $1; do
        if [[ $(fuser $1 2> /dev/null) != '' ]] then
            sleep 1
        elif kill -0 $(< $1); then
            sleep 1
        else
            rm $1
        fi
    done 2> /dev/null
    print -u3 $$
    set +o noclobber
}

function createcache { # $1 = gvars, $2 = name, $3 = params
    typeset -n gv=$1
    typeset name=$2
    # eventually drop
    #typeset -n params=$3

    typeset tmpfile varfile param pi

    gv.outdir=$SWIFTDATADIR/${name/data/cache}
    gv.lockfile=${gv.outdir}.lock
    tmpfile=${gv.outdir}/vars.new
    varfile=${gv.outdir}/vars.lst

    wait4lock ${gv.lockfile}

    for param in "${!params.@}"; do
        typeset -n pn=$param
        for pi in "${pn[@]}"; do
            print -r -- "${param#params.}=$pi"
        done
        typeset +n pn
    done | sort -u > $tmpfile
    [[ ${params.name} == '' ]] && print "name=$2" >> $tmpfile
    touch -t "${SWIFTDSERVERCACHEMAX:-15} sec ago" $tmpfile
    if [[ $tmpfile -nt $varfile ]] || ! cmp -s $tmpfile $varfile; then
        mv $tmpfile $varfile
        touch $varfile
        return 0
    else
        return 1
    fi
}

autoload ${instance}_dserverhelper_init
autoload ${instance}_dserverhelper_run

if [[ $SWIFTWARNLEVEL != '' ]] then
    typeset -ft wait4lock createcache
    typeset -ft ${instance}_dserverhelper_init
    typeset -ft ${instance}_dserverhelper_run
    set -x
fi

gvars=(
    infile=ERROR
    outfile=ERROR
    outdir=ERROR
    lockfile=ERROR
)

[[ $SWIFTDATADIR == "" ]] && (
    print -u2 ERROR SWIFTDATADIR variable not set
    exit 1
)

umask 002

${instance}_dserverhelper_init gvars
if [[ $? != 0 ]] then
    print -u2 ERROR cannot invoke instance customization function
    exit 1
fi
if [[ -f $SWIFTDATADIR/etc/${instance}_dserverhelper_vars ]] then
    . $SWIFTDATADIR/etc/${instance}_dserverhelper_vars gvars
    if [[ $? != 0 ]] then
        print -u2 ERROR cannot invoke local customization script
        exit 1
    fi
fi

read infile || exit 0

gvars.infile=$infile
gvars.outfile=ERROR

params=()
typeset ill='+(@(\<|%3c)@([a-z][a-z0-9]|a)*|\`*\`|\$*\(*\)|\$*\{*\})'
typeset -l vl

name=
set -o noglob
if [[ ${gvars.infile} == *\&* ]] then
    ifs="$IFS"
    IFS='&'
    set -A list ${gvars.infile#*[?]}
    for i in "${list[@]}"; do
        [[ $i == '' ]] && continue
        k=${i%%=*}
        [[ $k == '' ]] && continue
        if [[ $k == *+([!a-zA-Z0-9_])* ]] then
            print -r -u2 "SWIFT ERROR illegal key in parameters"
            continue
        fi
        v=${i#"$k"=}
        typeset -n pn=params.$k
        if [[ $v == %5B*%5D ]] then
            v=${v//'+'/' '}
            v=${v//@([\'\\])/'\'\1}
            eval "v=\$'${v//'%'@(??)/'\x'\1"'\$'"}'"
        fi
        if [[ $v == \[*\] ]] then
            v=${v#\[}
            v=${v%\]}
            v=${v#\;}
            while [[ $v != '' ]] do
                vi=${v%%\;*}
                v=${v#"$vi"}
                v=${v#\;}
                vi=${vi//'+'/' '}
                vi=${vi//@([\'\\])/'\'\1}
                eval "vi=\$'${vi//'%'@(??)/'\x'\1"'\$'"}'"
                vl=$vi
                vl=${vl//+([[:space:]])/}
                if [[ $vl == *$ill* ]] then
                    print -r -u2 "SWIFT ERROR illegal value for key $k"
                    continue
                fi
                pn[${#pn[@]}]=${vi#*=}
            done
        else
            v=${v//'+'/' '}
            v=${v//@([\'\\])/'\'\1}
            eval "v=\$'${v//'%'@(??)/'\x'\1"'\$'"}'"
            vl=$v
            vl=${vl//+([[:space:]])/}
            if [[ $vl == *$ill* ]] then
                print -r -u2 "SWIFT ERROR illegal value for key $k"
                continue
            fi
            pn[${#pn[@]}]="$v"
        fi
        typeset +n pn
    done
    IFS="$ifs"
    name=${params.name}
else
    name=${gvars.infile}
fi
set +o noglob
if [[ $name == '' ]] then
    name=data.$$.$RANDOM.$RANDOM
fi
if [[ $name != +([a-z0-9.]) ]] then
    print -r -u2 "SWIFT ERROR illegal directory name $name"
    exit 1
fi

if ${instance}_dserverhelper_run gvars "$name" params; then
    :
elif [[ ${gvars.outfile} == ERROR ]] then
    print -u2 ERROR cannot expand input file $infile
    exit 1
fi

if [[ ${gvars.outfile} == ERROR ]] then
    print -u2 ERROR cannot expand input file $infile
    exit 1
fi

print ${gvars.outfile}
