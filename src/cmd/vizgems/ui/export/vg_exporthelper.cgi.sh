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

. vg_hdr

suireadcgikv

name=$qs_name
dir=$SWIFTDATADIR/cache/main/$SWMID/$name
cd $dir || exit 1

secret=$(< secret.txt)
if [[ $qs_secret != $secret ]] then
    print -u2 ALARM: illegal export access by $SWMID: $secret / $qs_secret
    exit 1
fi

case $qs_type in
csv|xml|html|pdf)
    $SHELL vg_export_save \
        "$qs_type" "${qs_file//[!.a-zA-Z0-9_-]/_}" "$qs_notes" \
    | read file
    case "$file" in
    *.zip)
        print "Content-type: application/zip; name=\"$file\""
        print "Content-Disposition: inline; filename=\"$file\""
        print ""
        ;;
    *.csv)
        print "Content-type: application/x-ms-excel; name=\"$file\""
        print "Content-Disposition: inline; filename=\"$file\""
        print ""
        ;;
    *.xml)
        print "Content-type: application/x-ms-excel; name=\"$file\""
        print "Content-Disposition: inline; filename=\"$file\""
        print ""
        ;;
    *.pdf)
        print "Content-type: application/pdf; name=\"$file\""
        print "Content-Disposition: inline; filename=\"$file\""
        print ""
        ;;
    esac
    cat "$file"
    ;;
email)
    $SHELL vg_export_email "$qs_from" "$qs_to" "$qs_subject" "$qs_notes"
    ret=$?
    print "Content-type: text/xml\n"
    print "<response>"
    if [[ $ret == 0 ]] then
        print "<r>OK|OK</r>"
    else
        print "<r>ERROR|operation failed</r>"
    fi
    print "</response>"
    ;;
esac
