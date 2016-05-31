
sdpmpdir=
sdpmpjobmax=
function sdpmpjobcntl { # $1 = group $2 = jobmax
    sdpmpdir=$SWIFTDATADIR/tmp/sdpmp/${1:-jobs}
    if ! mkdir -p $sdpmpdir; then
        print -u2 ERROR cannot create REST group dir $sdpmpdir
        return 1
    fi
    sdpmpjobmax=${2:-2}
    if ! touch $sdpmpdir/state; then
        print -u2 ERROR cannot touch REST state file $sdpmpdir/state
        return 1
    fi
}

sdpmplocked=n
function sdpmplock {
    set -o noclobber
    while ! command exec 3> $sdpmpdir/lock; do
        if kill -0 $(< $sdpmpdir/lock); then
            sleep $(( RANDOM % 4 ))
        elif [[ ${ fuser $sdpmpdir/lock; } != '' ]] then
            sleep $(( RANDOM % 4 ))
        else
            rm $sdpmpdir/lock
            sleep 5
        fi
    done 2> /dev/null
    print -u3 $$
    set +o noclobber
    sdpmplocked=y
}

function sdpmpunlock {
    if [[ $sdpmplocked == y ]] then
        rm $sdpmpdir/lock
        sdpmplocked=n
    fi
}

typeset -A sdpmpprocs
function sdpmpreadstate {
    typeset now

    for proci in "${!sdpmpprocs[@]}"; do
        unset sdpmpprocs[$proci]
    done
    . $sdpmpdir/state
    now=${ printf '%(%#)T'; }
    for proci in "${!sdpmpprocs[@]}"; do
        if (( now - sdpmpprocs[$proci] > 60 )) then
            if [[
                ! -f $sdpmpdir/$proci ||
                ${ fuser $sdpmpdir/$proci 2> /dev/null; } == ''
            ]] then
                rm -f $sdpmpdir/$proci
                unset sdpmpprocs[$proci]
            fi
        fi
    done
}

function sdpmpwritestate {
    typeset proci

    for proci in "${!sdpmpprocs[@]}"; do
        print "sdpmpprocs[$proci]=${sdpmpprocs[$proci]}"
    done > $sdpmpdir/state.tmp && mv $sdpmpdir/state.tmp $sdpmpdir/state
}

function sdpmpstartjob {
    while true; do
        [[ -f $sdpmpdir/exit ]] && return 1
        sdpmplock || return 1
        sdpmpreadstate
        if (( ${#sdpmpprocs[@]} < sdpmpjobmax )) then
            sdpmpprocs[$$]=${ printf '%(%#)T'; }
            exec 4> $sdpmpdir/$$
            sdpmpwritestate
            sdpmpunlock
            break
        fi
        sdpmpunlock
        sleep 1
    done
}

function sdpmpendjob {
    sdpmplock || return 1
    sdpmpreadstate
    unset sdpmpprocs[$$]
    sdpmpwritestate
    exec 4>&-
    rm $sdpmpdir/$$
    sdpmpunlock
}
