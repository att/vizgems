#!/bin/ksh

export SWMROOT=${DOCUMENT_ROOT%/htdocs}
. $SWMROOT/bin/swmenv
[[ $SECONDS != *.* ]] && exec $SHELL $0 "$@"
. $SWMROOT/bin/swmgetinfo

if [[ $SWMMODE == secure ]] then
    exit 0
fi

print "Content-type: text/html\n
<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 TRANSITIONAL//EN\">
<html>
<head>
<title>SWIFT</title>
</head>
<body>
"

print "Personal files:"

$SWMROOT/bin/swmlistfiles

print "
</body>
</html>
"
