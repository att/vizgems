#!/bin/ksh

export SWMROOT=${DOCUMENT_ROOT%/htdocs}
. $SWMROOT/bin/swmenv
[[ $KSHREC != 1 && ${KSH_VERSION##*' '} < $SHELLVERSION ]] && \
KSHREC=1 exec $SHELL $0 "$@"
export SWMNOUPDATE=y
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
cfile=$SWIFTDATADIR/cache/main/$SWMID/$name

secret=$(< ${cfile%/*}/secret.txt)
if [[ $qs_secret != $secret ]] then
    print -u2 ALARM: illegal image access by $SWMID: $secret / $qs_secret
    print "Content-type: text/html\n"
    exit 1
fi

print "Content-type: image/gif"
print "Expires: Mon, 19 Oct 1998 00:00:00 GMT\n"
cat "$cfile"
