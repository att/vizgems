#!/bin/ksh

export SWMROOT=${DOCUMENT_ROOT%/htdocs}
export SWMGROUPS=  # may have already been set
. $SWMROOT/bin/swmenv
[[ $SECONDS != *.* ]] && exec $SHELL $0 "$@"
. $SWMROOT/bin/swmgetinfo

if [[ $SWMMODE == secure ]] then
    exit 0
fi

suireadcgikv

. $SWMROOT/proj/$qs_instance/bin/${qs_instance}_env

typeset ill='+(@(\<|%3c)@([a-z][a-z0-9]|a)*@(\>|%3e)|\`*\`|\$*\(*\)|\$*\{*\})'
if [[ $QUERY_STRING == *$ill* ]] then
    print -r -u2 "SWIFT ERROR suiprefshow: illegal characters in QUERY_STRING"
    exit 1
fi

typeset -A langmap
langmap[lefty]=Unix
langmap[java]=Java
langmap[ksh]=Ksh

typeset -A langorder
langorder[0]=lefty
langorder[1]=java
langorder[2]=ksh

if [[ $qs_eedit != '' ]] then
    prefname=$qs_preflist
else
    prefname=$qs_prefname
fi

for tool in $SWIFTTOOLS; do
    for dir in ${PATH//:/ }; do
        if [[ -f $dir/../lib/prefs/$tool.prefs ]] then
            . $dir/../lib/prefs/$tool.prefs
            break
        fi
    done
done
for dir in ${PATH//:/ }; do
    if [[ -f $dir/../lib/prefs/$qs_instance.prefs ]] then
        . $dir/../lib/prefs/$qs_instance.prefs
        break
    fi
done

if [[ $qs_restore != yes && -f $SWMDIR/$qs_instance.$prefname.prefs ]] then
    . $SWMDIR/$qs_instance.$prefname.prefs
fi

print "Content-type: text/html\n"
print "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 TRANSITIONAL//EN\">"
print "<html>"
print "<head>"
print "<title>Preferences for instance: $qs_instance</title>"
print "</head>"
print "<body>"
print "<h3>Preferences for instance: $qs_instance, name: $prefname</h3>"
print "<script>"
print "function resetvalues () {"
print "  location = 'suiprefshow.cgi?$QUERY_STRING&restore=yes'"
print "}"
print "</script>"
print "<form"
print "  method=post action=suiprefproc.cgi"
print "  enctype=multipart/form-data"
print ">"
print "<input type=hidden name=instance value='$qs_instance'>"
print "<input type=hidden name=prefname value='$prefname'>"
print "<table width=100%><tr><td align=left>"
print "  <input type=submit value=Save>"
print "</td><td align=left>"
print "  <input type=button value=Reset onClick='resetvalues ()'>"
print "</td></tr></table>"

count=0
for tool in $SWIFTTOOLS; do
    typeset -n x=$tool
    if [[ ${x.launch} != yes ]] then
        print "<input type=hidden name=run_${tool} value=nolaunch>"
        continue
    fi
    print "  <input type=hidden name=map$count value=$tool.uselang>"
    name=field$count
    (( count++ ))
    print "<table border=1>"
    print "  <tr><th colspan=2>${x.label}</th></tr>"
    print "  <tr><th>Start</th><th>Don't Start</th></tr>"
    print "  <tr><td>"
    uselang=${x.uselang}
    if [[ $uselang == "" ]] then
        uselang=none
    fi
    for lang in ${x.langs}; do
        if [[ $uselang == $lang ]] then
            checked=checked
        else
            checked=
        fi
        print "    <input type=radio $checked name=$name value=$lang>"
        print "    ${langmap[$lang]}"
    done
    print "  </td><td>"
    if [[ $uselang == none ]] then
        checked=checked
    else
        checked=
    fi
    print "    <input type=radio $checked name=$name value=none>"
    print "  </td></tr>"
    print "</table>"
done
for tool in $SWIFTTOOLS; do
    typeset -n x=$tool
    print "<table border=1>"
    print "  <tr><th colspan=2>${x.label}</th></tr>"
    for group in ${x.groups}; do
        print "  <tr><th colspan=2>Group $group</th></tr>"
        typeset -n y=$tool.$group
        for entry in ${y.entrylist}; do
            epath=$tool.$group.entries.$entry
            typeset -n z=$tool.$group.entries.$entry
            print "  <tr>"
            print "    <td>${z.label}</td>"
            print "    <td>"
            print "      <input type=text name=field$count value='${z.val}'>"
            print "    </td>"
            print "  </tr>"
            print "  <input type=hidden name=map$count value=$epath.val>"
            (( count++ ))
        done
    done
    print "</table>"
done
print "</form>"
print "</body>"
print "</html>"
