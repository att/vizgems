#!/bin/ksh

export SWMROOT=${DOCUMENT_ROOT%/htdocs}
. $SWMROOT/bin/swmenv
[[ $KSHREC != 1 && ${KSH_VERSION##*' '} < $SHELLVERSION ]] && \
KSHREC=1 exec $SHELL $0 "$@"
export SWMID=__no_user__

argv0=${0##*/}
instance=${argv0%%_*}
rest=${argv0#$instance}
rest=${rest#_}
if [[ $instance == '' || $rest == '' ]] then
    print -u2 SWIFT ERROR program name not in correct form
    exit 1
fi
. $SWMROOT/proj/$instance/bin/${instance}_env || exit 1

help_data=(
    pid=''
)

function help_init {
    typeset dt vt k v n n1 n2 fu bu fsep bsep vari val vali
    typeset -A prefs

    help_data.pid=${qs_pid}

    return 0
}

function help_emit_header_begin {
    print '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 TRANSITIONAL//EN">'
    print "<html>"
    print "<head>"

    ph_emitheader page banner
    print "<title>${ph_data.title}</title>"

    return 0
}

function help_emit_header_end {
    print "</head>"

    return 0
}

function help_emit_body_begin {
    typeset vt st img

    print "<body>"
    ph_emitbodyheader $pid banner $qs_winw

    return 0
}

function help_emit_body_end {
    ph_emitbodyfooter
    print "</body>"
    print "</html>"

    return 0
}

if [[ $SWIFTWARNLEVEL != "" ]] then
    PS4='${.sh.fun}-$LINENO: '
    typeset -ft help_init
    typeset -ft help_emit_body_begin help_emit_body_end
    typeset -ft help_emit_header_begin help_emit_header_end
fi

. vg_prefshelper

suireadcgikv

if [[ $qs_section == userguide ]] then
    print "Content-type: application/pdf\n"
    udir=$SWMROOT/htdocs/proj/vg/help/userguide
    [[ -f $udir/vg_userguide.pdf ]] && cat $udir/vg_userguide.pdf
elif [[ $qs_section == quicktips/* ]] then
    print "Content-type: text/html\n"
    pid=${qs_pid}
    if [[ $pid == '' ]] then
        print -u2 SWIFT ERROR configuration not specified
        exit 1
    fi
    ph_init $pid

    help_init
    help_emit_header_begin
    help_emit_header_end
    help_emit_body_begin

    dir=${qs_section#quicktips/}
    dir=${dir//[!a-zA-Z0-9_]/}
    udir=$SWMROOT/htdocs/proj/vg/help/quicktips/$dir
    wdir=$SWIFTWEBPREFIX/proj/vg/help/quicktips/$dir
    if [[ -f $udir/index.html ]] then
        sed -e "s!src=\"!src=\"$wdir/!" $udir/index.html
    fi
    help_emit_body_end
fi
