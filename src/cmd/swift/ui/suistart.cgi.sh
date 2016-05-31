#!/bin/ksh

export SWMROOT=${DOCUMENT_ROOT%/htdocs}
. $SWMROOT/bin/swmenv
[[ $SECONDS != *.* ]] && exec $SHELL $0 "$@"
. $SWMROOT/bin/swmgetinfo

argv0=${0##*/}
instance=${argv0%%_*}
rest=${argv0#$instance}
rest=${rest#_}
if [[ $instance == '' || $rest == '' ]] then
    print -u2 ERROR program name not in correct form
    exit 1
fi
. $SWMROOT/proj/$instance/bin/${instance}_env || exit 1

suireadcgikv

prefdir=/cache/profiles/$SWMID
mkdir -p $SWIFTDATADIR$prefdir
suiprefgen \
    -oprefix $SWIFTDATADIR$prefdir/$instance.$qs_prefname.prefs \
-prefname $qs_prefname

mkdir -p $SWMROOT/tmp/$SWMID.$$
rm -f $SWMROOT/tmp/$SWMID.$$/*

launchargs="prefprefix=$prefdir/$instance.$qs_prefname.prefs"
launchargs="${launchargs}&sessionid=$qs_sessionid&dir=tmp/$SWMID.$$"

cgibin=$SWIFTWEBPREFIX/cgi-bin-${instance}-members

print "Content-type: text/html\n"
print '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 TRANSITIONAL//EN">'
print "<html>"
print "<head>"
print "<title>$SWIFTINSTANCENAME User Interface</title>"
print "</head>"
print "<frameset rows='75%,24%,1%'>"
print "<frame name=${instance}_pad src=$cgibin/${instance}_pad.cgi?$launchargs>"
print "<frame name=${instance}_lss src=$cgibin/${instance}_lss.cgi?$launchargs>"
print "<frame"
print "  name=${instance}_res"
print "  src=$cgibin/${instance}_launch.cgi?$launchargs"
print ">"
print "</frameset>"
print "</html>"
