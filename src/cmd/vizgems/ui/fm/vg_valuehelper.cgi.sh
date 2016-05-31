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

fm_data=(
    pid=''
    bannermenu=''
)

function fm_init {
    fm_data.pid=$pid

    return 0
}

function fm_emit_header_begin {
    print '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 TRANSITIONAL//EN">'
    print "<html>"
    print "<head>"

    ph_emitheader page banner bdiv
    print "<title>${ph_data.title}</title>"

    return 0
}

function fm_emit_header_end {
    print "</head>"

    return 0
}

function fm_emit_body_begin {
    typeset vt st img

    print "<body>"
    ph_emitbodyheader ${fm_data.pid} banner $qs_winw

    return 0
}

function fm_emit_body_end {
    ph_emitbodyfooter
    print "</body>"
    print "</html>"

    return 0
}

if [[ $SWIFTWARNLEVEL != "" ]] then
    PS4='${.sh.fun}-$LINENO: '
    typeset -ft fm_init
    typeset -ft fm_emit_body_begin fm_emit_body_end
    typeset -ft fm_emit_header_begin fm_emit_header_end
fi

. vg_hdr
. vg_prefshelper

suireadcgikv

pid=$qs_pid
if [[ $pid == '' ]] then
    print -u2 SWIFT ERROR configuration not specified
    exit 1
fi
ph_init $pid "${fm_data.bannermenu}"

print "Content-type: text/html\n"

if ! swmuseringroups 'vg_att_admin|vg_fileedit'; then
    print "<html><body><b><font color=red>"
    print "You are not authorized to access this page"
    print "</font></b></body></html>"
fi

fm_init
fm_emit_header_begin
fm_emit_header_end

fm_emit_body_begin

print "<div class=pagemain>${ph_data.title}</div>"

if [[ $qs_helper != vg_* ]] then
    print -u2 SWIFT ERROR cannot execute helper: $qs_helper
    exit
fi

for qsi in $qslist; do
    [[ $qsi != field_* ]] && continue
    typeset -n qsv=qs_$qsi
    qss[${#qss[@]}]="$qsi=$qsv"
done

$qs_helper $qs_file $qs_field "${qss[@]}"

fm_emit_body_end
