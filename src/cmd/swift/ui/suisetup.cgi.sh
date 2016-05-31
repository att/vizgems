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

function header {
    print "Content-type: text/html\n"
    print '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 TRANSITIONAL//EN">'
    print "<html>"
    print "<head>"
    print "<script>"
    print "var padw"
    print "function submt () {"
    print "  var f"
    print "  f = document.forms[0]"
    print "  if (!padw || padw.closed)"
    print "    padw = window.open ("
    print "      '$SWIFTWEBPREFIX' +"
    print "      '/cgi-bin-${instance}-members/${instance}_start.cgi?prefname='"
    print "      + f.prefname.options[f.prefname.selectedIndex].value +"
    print "      '&sessionid=' +"
    if [[ $SWIFTWEBMODE == secure ]] then
        print "      f.sessionid.value,"
    else
        print "      f.sessionid.options[f.sessionid.selectedIndex].value,"
    fi
    print "      '${instance}_top',"
    print "      'directories=no,hotkeys=no,status=yes,resizable=yes' +"
    print "      ',menubar=no,personalbar=no,titlebar=no,toolbar=no' +"
    print "      ',scrollbars=yes'"
    print "    )"
    print "  else"
    print "    padw.focus ()"
    print "  return false"
    print "}"
    print "</script>"
    print "</head>"
    print "<body>"
}

function footer {
    print "</body>"
    print "</html>"
}

display=$(swmiplookup ${REMOTE_ADDR} 2> /dev/null)
display=${display##* }
if [[ $display == "" ]] then
    display=$REMOTE_ADDR
fi

suireadcgikv

header

print "<form>"
print "<table border=1>"
print "<tr><th colspan=2>Setup for Instance $instance</th></tr>"
print "<tr><td>"
print "  <input type=button name=start value=Start onClick='submt()'>"
print "</td></tr>"
print "<tr><th colspan=2>Preference Sets</th></tr>"
print "<tr>"
print "  <td><select name=prefname>"
for file in $SWMDIR/$instance.*.prefs; do
    if [[ ! -f $file ]] then
        break
    fi
    name=${file##*/$instance.}
    name=${name%.prefs}
    if [[ $name != ${display}* ]] then
        continue
    fi
    print "    <option value='$name'>$name"
done
print "    <option value=default>default"
for file in $SWMDIR/$instance.*.prefs; do
    if [[ ! -f $file ]] then
        break
    fi
    name=${file##*/$instance.}
    name=${name%.prefs}
    if [[ $name == ${display}* || $name == default ]] then
        continue
    fi
    print "    <option value='$name'>$name"
done
print "  </select></td>"
print "</tr>"

if [[ $SWIFTWEBMODE == secure ]] then
    ${instance}_services start
    ${instance}_services newid | read key
    print "<input type=hidden name=sessionid value='$(swmhostname);;$key'>"
else
    print "<tr><th colspan=2>Running Sessions</th></tr>"
    print "<tr><td>"
    print "  <select name=sessionid>"
    ${instance}_services start
    ${instance}_services newid | read key
    print "    <option value='$(swmhostname);;$key'>Start Server Session"
    print "    <option value=';;$key'>Start Client Session"
    ${instance}_services query "" "" SWIFTMSERVER "" \
    | while read ip port type key; do
        name=$(swmiplookup $ip 2> /dev/null)
        name=${name##* }
        if [[ $name == "" ]] then
            name=$ip
        fi
        print "    <option value='$ip;$port;$key'>$name $port $key"
    done
    print "  </select>"
    print "</td></tr>"
fi
print "</table>"
print "</form>"

footer
