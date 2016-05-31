#!/bin/ksh

export SWMROOT=${0%/*}
SWMROOT=${SWMROOT%/*}
. $SWMROOT/bin/swmenv
[[ $SECONDS != *.* ]] && exec $SHELL $0 "$@"

(cd $SWMROOT/tmp && (
    find . ! -name . ! -name lost+found -type f -atime +1 -exec rm -f {} \;
    find . ! -name . ! -name lost+found -type l -atime +1 -exec rm -f {} \;
    find . ! -name . ! -name lost+found -type d -mtime +1 -exec rmdir {} \;
)) 2> /dev/null
