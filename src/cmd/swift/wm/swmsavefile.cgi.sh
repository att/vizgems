#!/bin/ksh

export SWMROOT=${DOCUMENT_ROOT%/htdocs}
. $SWMROOT/bin/swmenv
[[ $SECONDS != *.* ]] && exec $SHELL $0 "$@"
. $SWMROOT/bin/swmgetinfo

if [[ $SWMMODE == secure ]] then
    exit 0
fi

tmpdir=${TMPDIR:-/tmp}/swm.${SWMID}.$$
mkdir $tmpdir || exit 1
trap 'rm -rf $tmpdir' HUP QUIT TERM KILL EXIT
cd $tmpdir || exit 1
$SWMROOT/bin/swmmimesplit

if [[ ! -s bname || ! -s sfile || ! -s multifile ]] then
    print "Content-type: text/html\n"
    print "<html><head><title>SWIFT</title></head><body>"
    print "You must fill out the 'File Name on Server' field"
    print "</body></html>"
    exit 0
fi

if [[ $(< multifile) == y ]] then
    set -A list $(< bname)
    prefix=$(< sfile)
    suffix=${prefix##*.}
    prefix=${prefix%.*}
    for (( i = 0; i < ${#list[@]}; i++ )) do
        mv $SWMDIR/${list[$i]} $SWMDIR/${prefix}-$i.$suffix
        cp descr $SWMDIR/.descr.${prefix}-$i.$suffix
    done
else
    mv $SWMDIR/$(< bname) "$SWMDIR/$(< sfile)"
    mv descr "$SWMDIR/.descr.$(< sfile)"
fi

print "Content-type: text/html\n"
print '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 TRANSITIONAL//EN">'
print "<html><head><title>SWIFT</title></head><body>"
print "Done"
print "</body></html>"
