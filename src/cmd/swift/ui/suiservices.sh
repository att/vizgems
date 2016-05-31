
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

function startrserver {
    if ! kill -0 $( \
        egrep '^pid ' $prefix.suirserver.info 2> /dev/null \
        | sed 's/^.*pid = //'
    ) 2> /dev/null; then
        stopdserver
        suirserver $secarg -log $prefix.suirserver.log > $prefix.suirserver.info
    fi
}

function stoprserver {
    kill $( \
        egrep '^pid ' $prefix.suirserver.info | sed 's/^.*pid = //'
    )
    rm -f $prefix.suirserver.info
}

function listrserver {
    print rserver $(< $prefix.suirserver.info)
}

function queryrserver {
    rport=$(egrep '^port ' $prefix.suirserver.info | sed 's/^.*port = //')
    suirclient $hname/$rport |&
    print -p "$(printf "search %q %q %q %q\n" "$@")"
    while read -p line; do
        if [[ $line == SUCCESS || $line == FAILURE ]] then
            break
        fi
        print $line
    done
}

function newrserverid {
    rport=$(egrep '^port ' $prefix.suirserver.info | sed 's/^.*port = //')
    suirclient $hname/$rport |&
    print -p newid
    while read -p line; do
        if [[ $line == SUCCESS || $line == FAILURE ]] then
            break
        fi
        print $line
    done
}

function startdserver {
    if [[ $SWIFTDSERVERHELPER != '' ]] then
        cargs="-helper $SWIFTDSERVERHELPER -regname $rname/$rport"
    else
        cargs="-regname $rname/$rport"
    fi
    args="$cargs -regkey $instance-u"
    if ! kill -0 $( \
        egrep '^pid ' $prefix.suidserver-u.info 2> /dev/null \
        | sed 's/^.*pid = //'
    ) 2> /dev/null; then
        suidserver $secarg $args -log $prefix.suidserver-u.log \
        > $prefix.suidserver-u.info
    fi
    args="$cargs -compress -regkey $instance-c"
    if ! kill -0 $( \
        egrep '^pid ' $prefix.suidserver-c.info 2> /dev/null \
        | sed 's/^.*pid = //'
    ) 2> /dev/null; then
        suidserver $secarg $args -log $prefix.suidserver-c.log \
        > $prefix.suidserver-c.info
    fi
}

function stopdserver {
    kill $( \
        egrep '^pid ' $prefix.suidserver-?.info | sed 's/^.*pid = //'
    )
    rm -f $prefix.suidserver-?.info
}

function listdserver {
    print dserver-$1 $(cat $prefix.suidserver-$1.info)
}

hname=$(swmhostname)

while (( $# > 0 )) do
    case $1 in
    -root)
        export root=$2
        shift 2
        ;;
    *)
        break
        ;;
    esac
done

case $DOCUMENT_ROOT in
?*)
    SWMROOT=${DOCUMENT_ROOT%/htdocs}
    ;;
*)
    if [[ $root == "" ]] then
        print -u2 ERROR web root not defined
        exit 1
    fi
    SWMROOT=$root
    ;;
esac
export SWMROOT
. $SWMROOT/bin/swmenv
. $SWMROOT/proj/$instance/bin/${instance}_env

prefix=$SWMROOT/logs/$instance

lockfile=$SWMROOT/proj/$instance/lock.services

set -o noclobber
integer n=0
while ! command exec 3> ${lockfile}; do
    if (( n++ > 50 )) then
        break
    fi
    if [[ $(fuser ${lockfile}) != '' ]] then
        sleep 1
        continue
    elif kill -0 $(< ${lockfile}); then
        sleep 1
        continue
    else
        rm ${lockfile}
        continue
    fi
done 2> /dev/null
print -u3 $$
set +o noclobber

trap 'rm -f ${lockfile}' HUP QUIT TERM KILL EXIT

if [[ $SWIFTWEBMODE == secure ]] then
    secarg="-ipdir $SWMROOT/ipdir"
fi

case $1 in
start)
    if [[ $SWIFTWEBMODE == secure ]] then
        mkdir -p $SWMROOT/ipdir
    fi
    startrserver
    rname=$hname
    rport=$(egrep '^port ' $prefix.suirserver.info | sed 's/^.*port = //')
    startdserver
    ;;
stop)
    stopdserver
    stoprserver
    ;;
list)
    shift
    case $1 in
    r)
        listrserver
        ;;
    d)
        listdserver u
        listdserver c
        ;;
    d-u)
        listdserver u
        ;;
    d-c)
        listdserver c
        ;;
    a)
        listrserver
        listdserver u
        listdserver c
        ;;
    esac
    ;;
query)
    shift
    queryrserver "$@"
    ;;
newid)
    shift
    newrserverid newid
    ;;
*)
    ;;
esac
