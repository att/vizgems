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
rest=$QUERY_STRING
kind=${rest%%'/'*}
rest=${rest#"$kind"'/'}
fmt=${rest%%'/'*}
lfmt=${fmt:-xml}
rest=${rest#"$fmt"'/'}
lfmt=xml # override

if ! swmuseringroups 'vg_att_admin|vg_restconfquery'; then
    errors[${#errors[@]}]="you are not authorized to query CM on this system"
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

type=text/xml

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
    sdpmpendjob
    exit 0
fi

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

$SHELL ${fdata.fileprint} $configfile $file "$rfile" def

case $lfmt in
csv)                     ;;
xml) print "</response>" ;;
esac

sdpmpendjob
