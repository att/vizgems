#!/bin/ksh

export SWMROOT=${DOCUMENT_ROOT%/htdocs}
. $SWMROOT/bin/swmenv
[[ $SECONDS != *.* ]] && exec $SHELL $0 "$@"
. $SWMROOT/bin/swmgetinfo

if [[ $SWMMODE == secure ]] then
    exit 0
fi

display=$(swmiplookup ${REMOTE_ADDR} 2> /dev/null)
display=${display##* }
if [[ $display == "" ]] then
    display=$REMOTE_ADDR
fi

suireadcgikv

. $SWMROOT/proj/$qs_instance/bin/${qs_instance}_env

print "Content-type: text/html\n"
print '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 TRANSITIONAL//EN">'
print "<html>"
print "<head>"
print "<title>Preferences for instance: $qs_instance</title>"
print "</head>"
print "<body>"
print "<h3>Preferences for instance: $qs_instance</h3>"
print "<form"
print "  method=get action=$SWIFTWEBPREFIX/cgi-bin-members/suiprefshow.cgi"
print ">"
print "<input type=hidden name=instance value='$qs_instance'>"
print "<table border=1>"
print "<tr><th colspan=2>Existing Preference Sets</th></tr>"
print "<tr>"
print "  <td><input type=submit name=eedit value=Edit></td>"
print "  <td><select name=preflist>"
for file in $SWMDIR/$qs_instance.*.prefs; do
    if [[ ! -f $file ]] then
        break
    fi
    name=${file##*/$qs_instance.}
    name=${name%.prefs}
    if [[ $name != ${display}* ]] then
        continue
    fi
    print "    <option>$name"
done
print "    <option>default"
for file in $SWMDIR/$qs_instance.*.prefs; do
    if [[ ! -f $file ]] then
        break
    fi
    name=${file##*/$qs_instance.}
    name=${name%.prefs}
    if [[ $name == ${display}* || $name == default ]] then
        continue
    fi
    print "    <option>$name"
done
print "  </select></td>"
print "</tr>"
print "<tr><th colspan=2>New Preference Set</th></tr>"
print "<tr>"
print "  <td><input type=submit name=nedit value=Edit></td>"
print "  <td><input type=text name=prefname value=$display></td>"
print "</tr>"
print "</table>"
print "</form>"
print "</body>"
print "</html>"
