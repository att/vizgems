#!/bin/ksh

export SWMROOT=${DOCUMENT_ROOT%/htdocs}
. $SWMROOT/bin/swmenv
[[ $SECONDS != *.* ]] && exec $SHELL $0 "$@"
. $SWMROOT/bin/swmgetinfo

suireadcgikv

if [[
    $qs_instance == '' || $qs_group == '' || $qs_conf == '' || $qs_file == ""
]] then
    print -u2 "ERROR configuration not specified"
    exit 1
fi

. $SWMROOT/proj/$qs_instance/bin/${qs_instance}_env || exit 1

print "Content-type: text/html\n"

url="file=$qs_file"
suitable -instance $qs_instance -group $qs_group \
    -conf ${SWIFTDATADIR}$qs_conf -sortkey "$qs_sortkey" -sorturl "$url" \
< $SWIFTDATADIR/$qs_file
