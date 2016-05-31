#!/bin/ksh

export SWMROOT=${DOCUMENT_ROOT%/htdocs}
. $SWMROOT/bin/swmenv
[[ $SECONDS != *.* ]] && exec $SHELL "$0" "$@"

print "Content-type: text/html"
print

v=${QUERY_STRING//+/ }
eval "v=\$'${v//'%'@(??)/'\x'\1"'\$'"}'"

print "<script>"
print -r -- "$v"
print "</script>"
