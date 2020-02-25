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

function queueins {
    typeset file=$1 rfile=$2 ofile=$3
    typeset ts pfile sn
    ts=$(printf '%(%Y%m%d-%H%M%S)T')
    sn=$VG_SYSNAME
    pfile=cm.$ts.$file.${rfile//./_X_X_}.$sn.$sn.$RANDOM.full.xml
    pfile=$VG_DSYSTEMDIR/incoming/cm/$pfile
    (
        print "<a>full</a>"
        print "<u>$sn</u>"
        print "<f>"
        cat $ofile
        print "</f>"
    ) > $pfile.tmp && mv $pfile.tmp $pfile
}

. vg_hdr

suireadcgikv

dir=$SWIFTDATADIR/cache/main/$SWMID/favs
mkdir -p $dir
cd $dir || exit 1

print "Content-type: text/xml\n"
print "<response>"
if [[ $qs_name == '' || $qs_url == '' ]] then
    print "<r>ERROR|the name and URL fields are mandatory</r>"
elif [[ $qs_url != /cgi-bin-vg-members/vg_dserverhelper.cgi*query=* ]] then
    print -u2 SWIFT URL is not in the correct form
    print "<r>ERROR|the URL must be a local data query URL</r>"
else
    qs_url=${qs_url//'&prev='*([!&])'&'/'&'}
    {
        if [[ -f $VG_DSYSTEMDIR/uifiles/favorites/$SWMID.txt ]] then
            cat $VG_DSYSTEMDIR/uifiles/favorites/$SWMID.txt
        fi
        print -r "$qs_name|+|$qs_url|+|$SWMID"
    } | sort -u -t'|' > $VG_DSYSTEMDIR/uifiles/favorites/$SWMID.tmp && \
    sdpmv \
        $VG_DSYSTEMDIR/uifiles/favorites/$SWMID.tmp \
    $VG_DSYSTEMDIR/uifiles/favorites/$SWMID.txt
    print "<r>OK|OK</r>"
    queueins favorites $SWMID.txt $VG_DSYSTEMDIR/uifiles/favorites/$SWMID.txt
fi
print "</response>"
