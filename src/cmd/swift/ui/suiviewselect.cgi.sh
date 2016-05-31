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

if [[ $qs_prefprefix == '' ]] then
    print -u2 "ERROR configuration not specified"
    exit 1
fi

. $SWIFTDATADIR$qs_prefprefix.sh
for dir in ${PATH//:/ }; do
    if [[ -f $dir/${instance}_prefshelper ]] then
        . $dir/${instance}_prefshelper
        break
    fi
done

group=${group:-default}
xname=$instance.pad.$group
typeset -n x=$xname
if [[ ${!x.@} == '' ]] then
    group=default
    xname=$instance.pad.$group
    typeset -n x=$xname
    x.title="$SWIFTINSTANCENAME View Selection"
fi

print "Content-type: text/html\n
<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0 TRANSITIONAL//EN\">
<html>
<head>
<style type='text/css'>
BODY {
  font-family: ${x.viewfontname}; font-size: ${x.viewfontsize};
  background: ${x.viewcolor%%\ *}; color: ${x.viewcolor##*\ };
}"

for var in "${!x.class_@}"; do
    tc=${var##$xname.class_}
    tc=${tc%_style}
    if [[ $tc == *_* ]] then
        type=${tc%%_*}
        class=${tc#${type}_}
        eval print "$type.$class { \${x.class_${type}_${class}_style} }"
    else
        type=$tc
        eval print "$type { \${x.class_${type}_style} }"
    fi
done
print "</style>"
if [[ ${x.showtitle} == 1 ]] then
    if [[ ${x.title} == __RUNFUNC__ ]] then
        x.title=$(${instance}_${group}_pad_gentitle)
    fi
    print "<title>${x.title}</title>"
fi

print "<script language=javascript>
function suiview_set (name, id) {
  window.opener.suipad_setview (name, id)
  window.close ()
}
</script>
</head>
<body>
<h3>
${x.title}
</h3>
"

print "/name2id.map" | ${instance}_dserverhelper | read custfile

view0=$(egrep '\|0$' $custfile)
view0=${view0%\|*}
ncust=$(wc -l < $custfile)

if (( ncust < 50 )) then
    if [[ $view0 != '' ]] then
        print "<a"
        print "  class=a"
        print "  href='javascript:suiview_set (\"$view0\", 0)'"
        print ">"
        print "  $view0"
        print "</a><br>"
    fi

    egrep -v '\|0$' $custfile | sort -t '|' -k1 \
    | while IFS='|' read cust id; do
        print "<a"
        print "  class=a"
        print "  href='javascript:suiview_set (\"$cust\", $id)'"
        print ">"
        print "  $cust"
        print "</a><br>"
    done
else
    case $qs_level in
    1)
        if [[ $view0 != '' ]] then
            print "<a"
            print "  class=a"
            print "  href='javascript:suiview_set (\"$view0\", 0)'"
            print ">"
            print "  $view0"
            print "</a><br><br>"
        fi
        (
            egrep -v '\|0$' $custfile \
            | sed -E -e 's/\|.*$//' -e 's/^(.).*/\1/' | sort -u \
            | while read i; do
                print "<a"
                print "  class=a"
                print -n "  href='$argv0?level=2&initial=$i"
                print "&prefprefix=$qs_prefprefix'"
                print ">$i</a>"
            done
            print "<br>"
        )
        ;;
    2)
        print "<a"
        print "  class=a"
        print "  href='$argv0?level=1&prefprefix=$qs_prefprefix'"
        print ">"
        print "  Back"
        print "</a><br><br>"
        (
            egrep -v '\|0$' $custfile | egrep "^$qs_initial" \
            | sort -u -t'|' \
            | while read i; do
                id=${i##*\|}
                name=${i%\|$id}
                name=${name//[\'\"]/_}
                print "<a"
                print "  class=a"
                print "  href='javascript:suiview_set (\"$name\", $id)'"
                print ">"
                print "  $name"
                print "</a><br>"
            done
        )
        ;;
    esac
fi

print "
</body>
</html>
"
