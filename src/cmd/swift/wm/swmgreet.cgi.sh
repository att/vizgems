#!/bin/ksh

export SWMROOT=${DOCUMENT_ROOT%/htdocs}
. $SWMROOT/bin/swmenv
[[ $SECONDS != *.* ]] && exec $SHELL $0 "$@"
. $SWMROOT/bin/swmgetinfo

print "Content-type: text/html\n
<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 TRANSITIONAL//EN\">
<html>
<head>
<title>SWIFT Member Services</title>
</head>
<body>
"

print "<H2 align=center> Welcome to the SWIFT Member Services!</H2>"
print "Greetings, $SWMNAME<br>"
print "Groups: $SWMGROUPS<br>"

if [[ $SWMMODE != secure ]] then
    print "Personal Files:<br>"
    $SWMROOT/bin/swmfilestats
fi

print "
</body>
</html>
"
