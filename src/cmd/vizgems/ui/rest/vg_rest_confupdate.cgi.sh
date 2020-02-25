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

wdir=$SWIFTDATADIR/cache/main/$SWMID/conf.$$.$RANDOM.$RANDOM
mkdir -p $wdir
cd $wdir || {
    print -u2 ERROR cannot cd to $wdir
    exit 1
}
rm -f *

. vg_hdr
. $SWIFTDATADIR/etc/confmgr.info
for dir in ${PATH//:/ }; do
    if [[ -f $dir/../lib/vg/vg_cmfilelist ]] then
        configfile=$dir/../lib/vg/vg_cmfilelist
        break
    fi
done
. $configfile
if [[ $? != 0 ]] then
    print -u2 ERROR missing or bad configuration file
    exit 1
fi

if ! sdpmpjobcntl rest ${SWIFTRESTJOBMAX:-16} || ! sdpmpstartjob; then
    print -u2 ERROR unable to start job
    exit 1
fi

typeset -l lfmt

typeset ill='+(@(\<|%3c)@([a-z][a-z0-9]|a)*@(\>|%3e)|\`*\`|\$*\(*\)|\$*\{*\})'
rest=$QUERY_STRING
if [[ $rest == *$ill* ]] then
    print -r -u2 "SWIFT ERROR rest: illegal characters in QUERY_STRING"
    exit 1
fi
kind=${rest%%'/'*}
rest=${rest#"$kind"'/'}
fmt=${rest%%'/'*}
lfmt=${fmt:-xml}
rest=${rest#"$fmt"'/'}
lfmt=xml # override

if ! swmuseringroups 'vg_att_admin|vg_restconfupdate'; then
    errors[${#errors[@]}]="you are not authorized to update CM on this system"
fi

if [[ $kind == *=* ]] then
    file=${kind%%=*} rfile=${kind#*=}
else
    file=$kind rfile=
fi

export VGCM_RFILE=$rfile
typeset -n fdata=files.$file
if [[ ${fdata.locationmode} == dir ]] then
    filepath=${fdata.location}/$rfile
else
    filepath=${fdata.location}
fi
if [[ ! -f $filepath ]] then
    print -u2 ERROR file not found $filepath
    errors[${#errors[@]}]="file not found $filepath"
fi
if ! swmuseringroups "${fdata.group}"; then
    errors[${#errors[@]}]="You are not authorized to modify this file"
fi
if [[ ${fdata.accessmode} == byid ]] then
    if swmuseringroups "${fdata.admingroup}"; then
        export EDITALL=1
        export VIEWALL=1
    fi
fi

( cat; print "" ) | sed $'s/<a>/\n<a>/g' | while read line; do
    [[ $line == *(' ') ]] && continue
    print -r -- "$line"
done > edits2check.xml
$SHELL ${fdata.filecheck} $configfile $file edits2check.xml \
| while read -r line; do
    tag=${line%%'|'*}
    rest=${line#$tag'|'}
    case $tag in
    ERROR)
        errors[${#errors[@]}]="$rest"
        ;;
    WARNING)
        warnings[${#warnings[@]}]="$rest"
        ;;
    REC)
        print -u3 -r "$rest"
        ;;
    esac
done 3> editschecked.xml

type=text/xml

date=$(printf '%(%Y%m%d-%H%M%s)T')
sn=$VG_SYSNAME
prefix=cm suffix=xml dir=cm

print "Content-type: $type\n"

if (( ${#errors[@]} > 0 )) then
    case $type in
    text/xml)
        print "<response><errors>"
        for error in "${errors[@]}"; do
            print "<error>$error</error>"
        done
        for warning in "${warnings[@]}"; do
            print "<warning>$warning</warning>"
        done
        print "</errors></response>"
        ;;
    text/plain)
        for error in "${errors[@]}"; do
            print "ERROR: $error"
        done
        for warning in "${warnings[@]}"; do
            print "WARNING: $warning"
        done
        ;;
    esac
    sdpmpendjob
    exit 0
fi

f=cm.$date.$file.${rfile//./_X_X_}.$sn.$SWMID.$RANDOM.edit.xml

cp editschecked.xml $SWIFTDATADIR/incoming/cm/$f.tmp && \
mv $SWIFTDATADIR/incoming/cm/$f.tmp $SWIFTDATADIR/incoming/cm/$f

case $lfmt in
csv)                    ;;
xml) print "<response>" ;;
esac
if (( ${#warnings[@]} > 0 )) then
    case $type in
    text/xml)
        print "<warnings>"
        for warning in "${warnings[@]}"; do
            print "<warning>$warning</warning>"
        done
        print "</warnings>"
        ;;
    text/plain)
        for warning in "${warnings[@]}"; do
            print "WARNING: $warning"
        done
        ;;
    esac
fi
case $lfmt in
csv)                     ;;
xml) print "</response>" ;;
esac

sdpmpendjob
