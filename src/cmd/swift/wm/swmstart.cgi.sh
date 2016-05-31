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
<frameset cols='20%,80%'>
  <frame name=swmcontrol src=/cgi-bin-members/swmcontrol.cgi>
  <frame name=swmgeneral src=/cgi-bin-members/swmgreet.cgi>
</frameset>
</html>
"
