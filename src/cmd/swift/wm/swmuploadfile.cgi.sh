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

if [[ $(wc -w < sfile) == *\ 0* ]] then
    print "Content-type: text/html\n"
    print '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 TRANSITIONAL//EN">'
    print "<html><head><title>SWIFT</title></head><body>"
    print "You must fill out both one of the entries for locating"
    print "the file to upload and the 'File Name on Server' fields"
    print "</body></html>"
    exit 0
fi

if [[ $(wc -w < cfile) != *\ 0* ]] then
    mv cfile "$SWMDIR/$(< sfile)"
else
    print "Content-type: text/html\n"
    print '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 TRANSITIONAL//EN">'
    print "<html><head><title>SWIFT</title></head><body>"
    print "You must fill out both one of the entries for locating"
    print "the file to upload and the 'File Name on Server' fields"
    print "</body></html>"
    exit 0
fi

mv descr "$SWMDIR/.descr.$(< sfile)"

print "Content-type: text/html\n"
print '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 TRANSITIONAL//EN">'
print "<html><head><title>SWIFT</title></head><body>"
print "Done"
print "</body></html>"
