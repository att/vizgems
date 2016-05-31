#!/bin/ksh

export SWMROOT=${DOCUMENT_ROOT%/htdocs}
. $SWMROOT/bin/swmenv
[[ $SECONDS != *.* ]] && exec $SHELL $0 "$@"
. $SWMROOT/bin/swmgetinfo

if [[ $SWMMODE == secure ]] then
    exit 0
fi

suireadcgikv

. $SWMROOT/proj/$qs_instance/bin/${qs_instance}_env

(
    for (( count = 0; ; count++ )) do
        eval name=\$qs_map$count
        eval value=\$qs_field$count
        if [[ $name == '' ]] then
            break
        fi
        printf "%s=%q\n" $name "$value"
    done
) > $SWMDIR/$qs_instance.$qs_prefname.prefs

if [[ ! -f $SWMDIR/.descr.$qs_instance.$qs_prefname.prefs ]] then
    (
        print "SWIFT preferences for instance: $qs_instance, \c"
        print "name: $qs_prefname"
    ) > $SWMDIR/.descr.$qs_instance.$qs_prefname.prefs
fi

print "Content-type: text/html\n"
print '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 TRANSITIONAL//EN">'
print "<html>"
print "<head>"
print "<title>Preferences for instance: $qs_instance</title>"
print "</head>"
print "<body>"
print "Preferences saved."
print "<P><FORM>"
print "<input type=button value=Continue onClick=history.go(-1)>"
print "</FORM>"
print "</body>"
print "</html>"
