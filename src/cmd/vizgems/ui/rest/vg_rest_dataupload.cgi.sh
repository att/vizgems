#!/bin/ksh

export SWMROOT=${DOCUMENT_ROOT%/htdocs}
. $SWMROOT/bin/swmenv
[[ $KSHREC != 1 && ${KSH_VERSION##*' '} < $SHELLVERSION ]] && \
KSHREC=1 exec $SHELL $0 "$@"
. $SWMROOT/bin/swmgetinfo

argv0=${0##*/}
instance=${argv0%%_*}
rest=${argv0#$instance}
rest=${rest#_}
if [[ $instance == '' || $rest == '' ]] then
    print -u2 SWIFT ERROR program name not in correct form
    exit 1
fi
. $SWMROOT/proj/$instance/bin/${instance}_env || exit 1

wdir=$SWIFTDATADIR/cache/main/$SWMID/data.$$.$RANDOM.$RANDOM
mkdir -p $wdir
cd $wdir || {
    print -u2 ERROR cannot cd to $wdir
    exit 1
}
rm -f *

if ! sdpmpjobcntl rest ${SWIFTRESTJOBMAX:-16} || ! sdpmpstartjob; then
    print -u2 ERROR unable to start job
    exit 1
fi

typeset -l lfmt
rest=$QUERY_STRING
kind=${rest%%'/'*}
rest=${rest#"$kind"'/'}
fmt=${rest%%'/'*}
lfmt=${fmt:-xml}
rest=${rest#"$fmt"'/'}
ifs="$IFS"
IFS='&'
set -f
set -A qs -- $rest
set +f
IFS="$ifs"

date=$(printf '%(%Y.%m.%d.%H.%M.%s)T')
sn=$VG_SYSNAME

if ! swmuseringroups 'vg_att_admin|vg_restdataupload'; then
    errors[${#errors[@]}]="you are not authorized to upload data on this system"
fi

case $kind in
stat)  prefix=stats  dir=stat  ;;
alarm) prefix=alarms dir=alarm ;;
*)
    errors[${#errors[@]}]="unknown data kind $kind"
    ;;
esac

case $lfmt in
csv) type=text/plain suffix=txt ;;
xml) type=text/xml   suffix=xml ;;
*)
    errors[${#errors[@]}]="unknown format $fmt"
    type=text/html
    ;;
esac

print "Content-type: $type\n"

if (( ${#errors[@]} > 0 )) then
    case $type in
    text/xml)
        print "<response><errors>"
        for error in "${errors[@]}"; do
            print "<error>$error</error>"
        done
        print "</errors></response>"
        ;;
    text/plain)
        for error in "${errors[@]}"; do
            print "ERROR: $error"
        done
        ;;
    esac
    cat > /dev/null
    sdpmpendjob
    exit 0
fi

file=$prefix.$date.rest.$sn.$suffix

cat > $file

cp $file $SWIFTDATADIR/incoming/$dir/$file.tmp && \
mv $SWIFTDATADIR/incoming/$dir/$file.tmp $SWIFTDATADIR/incoming/$dir/$file

case $lfmt in
csv)                    ;;
xml) print "<response>" ;;
esac
case $lfmt in
csv)                     ;;
xml) print "</response>" ;;
esac

sdpmpendjob
