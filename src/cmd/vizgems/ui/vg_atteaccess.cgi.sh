#!/bin/ksh

export SWMROOT=${DOCUMENT_ROOT%/htdocs}
. $SWMROOT/bin/swmenv
[[ $KSHREC != 1 && ${KSH_VERSION##*' '} < $SHELLVERSION ]] && \
KSHREC=1 exec $SHELL $0 "$@"

argv0=${0##*/}
instance=${argv0%%_*}
rest=${argv0#$instance}
rest=${rest#_}
if [[ $instance == '' || $rest == '' ]] then
    print -u2 SWIFT ERROR program name not in correct form
    exit 1
fi
. $SWMROOT/proj/$instance/bin/${instance}_env || exit 1

. vg_hdr

typeset ill='+(@(\<|%3c)@([a-z][a-z0-9]|a)*|\`*\`|\$*\(*\)|\$*\{*\})'

authurl='https://www.e-access.att.com/empsvcs/hrpinmgt/pagLogin'
if [[ $HTTPS == on ]] then
    homeurl=https://$HTTP_HOST
else
    homeurl=http://$HTTP_HOST
fi
if [[ $QUERY_STRING == *$ill* ]] then
    print -r -u2 "SWIFT ERROR atteaccess: illegal characters in QUERY_STRING"
    exit 1
fi
if [[ $QUERY_STRING != '' ]] then
    homeurl+="$(printf '%#H' "$QUERY_STRING")"
else
    homeurl+="/cgi-bin-vg-members/vg_home.cgi"
fi

print "Content-type: text/html"
print "Status: 404 Not Found"
print "Cache-Control: no-cache, no-store, must-revalidate"
print "Pragma: no-cache"
print "Expires: 0\n"
print '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 TRANSITIONAL//EN">'
print "<html>"
print "<head>"
print "<meta http-equiv='refresh' content='0; URL=$$authurl?retURL=$homeurl&sysName=VizGEMS' />"
print "</head>"
print "</html>"
