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

cd $SWMDIR

cat $tmpdir/files | while read file; do
    descr=.descr.$file
    if [[ -f $file ]] then
        rm -f $file $descr
    fi
done

print "Content-type: text/html\n"
print '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 TRANSITIONAL//EN">'
print "<html><head><title>SWIFT</title></head><body>"
print "Done"
print "</body></html>"
