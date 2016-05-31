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

group=dateselect

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
function suidate_set (name, label) {
  window.opener.suipad_setdate (name, label)
  window.close ()
}
</script>
</head>
<body>
<h3>
${x.title}
</h3>
"

case $qs_level in
1)
    print "<a class=a href='javascript:suidate_set (\"latest\", \"latest\")'>"
    print "  Latest"
    print "</a><br><br>"
    (
        cd $SWIFTDATADIR/$SWIFTDATEDIR
        for i in [0-9][0-9][0-9][0-9]; do
            print "<a"
            print "  class=a"
            print "  href='$argv0?level=2&initial=$i&prefprefix=$qs_prefprefix'"
            print ">$i</a>"
            if [[ $SWIFTDATEFLAGS == *y* ]] then
                print "<a"
                print "  class=a"
                print "  href='javascript:suidate_set (\"$i\", \"$i\")'"
                print ">select</a>"
            fi
            print "<br>"
        done
    )
    ;;
2)
    print "<a class=a href='$argv0?level=1&prefprefix=$qs_prefprefix'>Back</a>"
    print "<br><br>"
    (
        print "$qs_initial<br><br>"
        cd $SWIFTDATADIR/$SWIFTDATEDIR/$qs_initial
        for i in [0-9][0-9]; do
            ym=$qs_initial/$i
            ymn="$ym $(printf '%(%b)T' $qs_initial.$i.01)"
            print "<a"
            print "  class=a"
            print -n "  href='$argv0?level=3&initial=$qs_initial/$i"
            print "&prefprefix=$qs_prefprefix'"
            print ">$ymn</a>"
            if [[ $SWIFTDATEFLAGS == *m* ]] then
                print "<a"
                print "  class=a"
                print "  href='javascript:suidate_set (\"$ym\", \"$ymn\")'"
                print ">select</a>"
            fi
            print "<br>"
        done
    )
    ;;
3)
    print "<a"
    print "  class=a"
    print -n "  href='$argv0?level=2&initial=${qs_initial%/*}"
    print "&prefprefix=$qs_prefprefix'"
    print ">Back</a><br><br>"
    (
        print "$(date -f %B -s ${qs_initial//\//.}.01)<br><br>"
        cd $SWIFTDATADIR/$SWIFTDATEDIR/$qs_initial
        for i in [0-9][0-9]; do
            dat1=$qs_initial/$i
            dat2="$qs_initial/$i $(date -f %a -s ${qs_initial//\//.}.$i)"
            print "<a"
            print "  class=a"
            print "  href='javascript:suidate_set (\"$dat1\", \"$dat2\")'"
            print ">$dat2</a><br>"
        done
    )
    ;;
esac

print "
</body>
</html>
"

typeset +n x
