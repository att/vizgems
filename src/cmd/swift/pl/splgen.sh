#!/bin/ksh

export SHELL=$(which ksh 2> /dev/null)
[[ $SECONDS != *.* ]] && exec $SHELL $0 "$@"

if [[ $1 == '' ]] then
    FILE="null"
else
    FILE="'${1%.spl}.spl'"
fi

lefty0d -x -e "
load ('spl.lefty');
spl.init (0);
spl.batchgen ($FILE);
txtview ('off');
"
