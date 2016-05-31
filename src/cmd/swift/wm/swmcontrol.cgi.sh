#!/bin/ksh

export SWMROOT=${DOCUMENT_ROOT%/htdocs}
. $SWMROOT/bin/swmenv
[[ $SECONDS != *.* ]] && exec $SHELL $0 "$@"
. $SWMROOT/bin/swmgetinfo

print 'Content-type: text/html

<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 TRANSITIONAL//EN">
<html>
<head>
<title>SWIFT Member Services</title>
</head>
<body>
<table>
<tr><td>
<h3>
SWIFT Member Services
</h3>
'

t='target=swmgeneral'
if [[ $SWMMODE != secure ]] then
    print "<h4>File Operations<br>"
    print "<a href=$SWIFTWEBPREFIX/cgi-bin-members/swmlistfiles.cgi $t>"
    print "  List Files"
    print "</a><br>"
    print "<a href=$SWIFTWEBPREFIX/members/swmuploadfile.html $t>"
    print "  Upload a File"
    print "</a><br>"
    print "<a href=$SWIFTWEBPREFIX/cgi-bin-members/swmdeletefile1.cgi $t>"
    print "  Delete a File"
    print "</a><br>"
    print "</h4>"
fi
print "<h4> Administrative<br>"
print "<a href=$SWIFTWEBPREFIX/cgi-bin-members/swmcpasswd.cgi $t>"
print "  Password</a>"
print "</h4>"
print "</td></tr>"

print "<tr><td><h4>"
for i in $SWMGROUPS; do
    if [[ -f $SWMROOT/htdocs/proj/$i/control.html ]] then
        print "$i<br>"
        if [[ $SWMMODE == secure ]] then
            egrep -v Preferences $SWMROOT/htdocs/proj/$i/control.html
        else
            cat $SWMROOT/htdocs/proj/$i/control.html
        fi
        print "<br>"
    fi
done

print "</h4></td></tr>"
print "</table>\n</body>\n</html>"
