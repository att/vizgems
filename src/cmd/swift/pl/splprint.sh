#!/bin/ksh

export SHELL=$(which ksh 2> /dev/null)
[[ $SECONDS != *.* ]] && exec $SHELL $0 "$@"

ORIENTATION="-Gorientation=land"
SIZE='-Gsize=10,7.5'

while [ $# != 0 ]; do
    case $1 in
    -landscape)
        ORIENTATION="-Gorientation=land"
        if [ "$SIZE" = "" ]; then
            SIZE='-Gsize=10,7.5'
        fi
        ;;
    -portrait)
        ORIENTATION=""
        if [ "$SETSIZE" = "" ]; then
            SIZE='-Gsize=7.5,10'
        fi
        ;;
    -size)
        SETSIZE=1
        SIZE="-Gsize=$2"
        shift
        ;;
    -paginate)
        PAGINATE=1
        ;;
    *)
        FILES="$(echo $FILES $1)"
    esac
    shift
done

if [ "$PAGINATE" = 1 ]; then
    if [ "$SETSIZE" = "" ]; then
        SIZE=""
    fi
    PAGESIZE='-Gpage=8.5,11'
fi

cat $FILES | sed -e 's/fontcolor = "back"/fontcolor = "black"/' | \
dot -Tps $ORIENTATION $SIZE $PAGESIZE > printme.ps
echo "created file: printme.ps"
