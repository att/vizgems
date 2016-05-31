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

suireadcgikv

name=$qs_name
tool=$qs_tool
file=$qs_file
page=$qs_page
cdir=$SWIFTDATADIR/cache/main/$SWMID/$name
cd "$cdir" || exit 1

secret=$(< secret.txt)
if [[ $qs_secret != $secret ]] then
    print -u2 ALARM: illegal image access by $SWMID: $secret / $qs_secret
    print "Content-type: text/html\n"
    exit 1
fi

. vg_hdr

print "Content-type: text/html\n"
print "<html><body>"
vg_dq_datahelper $tool $file $page | while read -r line; do
    printf '%#H\n' "$line"
done
print "</body></html>"
