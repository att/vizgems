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

account_data=(
    pid=''
    bannermenu='link|home||Home|return to the home page|link|back||Back|go back one page|list|tools|p:c|Tools|tools|list|help|userguide:accountuserpage|Help|documentation'
)

function account_init {
    account_data.pid=$pid

    return 0
}

function account_emit_header_begin {
    print '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 TRANSITIONAL//EN">'
    print "<html>"
    print "<head>"

    ph_emitheader page banner bdiv
    print "<title>${ph_data.title}</title>"

    return 0
}

function account_emit_header_end {
    print "</head>"

    return 0
}

function account_emit_body_begin {
    typeset vt st img

    print "<body>"
    ph_emitbodyheader ${account_data.pid} banner 1600

    return 0
}

function account_emit_body_end {
    ph_emitbodyfooter
    print "</body>"
    print "</html>"

    return 0
}

if [[ $SWIFTWARNLEVEL != "" ]] then
    PS4='${.sh.fun}-$LINENO: '
    typeset -ft account_init
    typeset -ft account_emit_body_begin account_emit_body_end
    typeset -ft account_emit_header_begin account_emit_header_end
fi

. vg_hdr
. vg_prefshelper
. $SWIFTDATADIR/dpfiles/config.sh

suireadcgikv

mgroups="vg_att_admin vg_att_user vg_cus_admin vg_cus_user"

fgroups="vg_hevonems vg_hevonweb vg_hevopsview vg_hevalarmclear"
fgroups+=" vg_restdataquery vg_restdataupload"
fgroups+=" vg_restconfquery vg_restconfupdate"
fgroups+=" vg_confeditfilter vg_confeditemail vg_confeditthreshold"
fgroups+=" vg_confeditprofile vg_confpoweruser vg_confview"
fgroups+=" vg_fileedit vg_fileview"
fgroups+=" vg_createticket vg_viewticket vg_getassetinfo"
fgroups+=" vg_passchange vg_docview vg_webusage"
set -A gs -- $fgroups
fgroupn=${#gs[@]}
unset gs

allgroups="all vg $mgroups $fgroups"

tabind=1

print "Content-type: text/html\n"

if ! swmuseringroups 'vg_att_admin|vg_passchange'; then
    print -u2 "SWIFT ERROR accountuser security violation user: $SWMID"
    print "<html><body><b><font color=red>"
    print "You are not authorized to access this page"
    print "</font></b></body></html>"
    exit
fi

pid=$qs_pid
if [[ $pid == '' ]] then
    print -u2 SWIFT ERROR configuration not specified
    exit 1
fi
ph_init $pid "${account_data.bannermenu}"

account_init
account_emit_header_begin

helpercgi="${ph_data.cgidir}/vg_accounthelper.cgi"
print "<script>"
print "var account_helper = '$helpercgi'"
print "function account_modify () {"
print "  var url, seli"
print "  url = account_helper + '?mode=passchange'"
print "  url += '&id=' + '$SWMID'"
print "  url += '&pass1=' + account_pass1el.value"
print "  url += '&pass2=' + account_pass2el.value"
print "  vg_xmlquery (url, account_editresult)"
print "}"
print "function account_editresult (req) {"
print "  var res, recs"
print "  res = req.responseXML"
print "  recs = res.getElementsByTagName ('r')"
print "  if (recs.length < 1) {"
print "    alert ('operation failed')"
print "    return"
print "  }"
print "  fields = recs[0].firstChild.data.split ('|')"
print "  if (fields[0] == 'OK') {"
print "    alert ('success')"
print "    location.reload ()"
print "  } else {"
print "    alert ('failure - errors: ' + unescape (fields[1]))"
print "  }"
print "}"
print "function account_clearfields () {"
print "  var seli, asseti, asset"
print "  account_pass1el.value = ''"
print "  account_pass2el.value = ''"
print "}"
print "</script>"
account_emit_header_end

account_emit_body_begin

print "<div class=page>${ph_data.title}</div>"

print "<div class=page><table class=page>"

print "<td class=pageborder colspan=2>&nbsp;<a"
print "  class=pageil href='javascript:void(0)' onClick='account_modify()'"
print "  title='change the password for this account'"
print ">&nbsp;Modify&nbsp;</a>&nbsp;<a"
print "  class=pageil href='javascript:void(0)' onClick='account_clearfields()'"
print "  title='clear all fields'"
print ">&nbsp;Clear&nbsp;</a>&nbsp;"
if [[ $SHOWHELP == 1 ]] then
    print "<a"
    print "  class=pageil href='javascript:void(0)'"
    print "  onClick='vg_showhelp(\"quicktips/accountuserform\")'"
    print "  title='click to read about using this form'"
    print ">&nbsp;?&nbsp;</a>"
fi
print "</td>"

print "<tr class=page>"
print "<td class=pageborder style='text-align:right'>"
print "<a"
print "  class=pagelabel href='javascript:void(0)'"
print "  title='password - enter twice to confirm'"
print ">&nbsp;Password&nbsp;</a>"
if [[ $SHOWHELP == 1 ]] then
    print "<a"
    print "  class=pageil href='javascript:void(0)'"
    print "  onClick='vg_showhelp(\"quicktips/accountuserpass\")'"
    print "  title='click to read about user passwords'"
    print ">&nbsp;?&nbsp;</a>"
fi
print "</td>"
print "<td class=pageborder style='text-align:left'>"
print "&nbsp;enter twice:<input"
print "  tabindex=$tabind class=page type=password id=pass1 size=20"
print "  title='4-20 chars'"
print ">"
(( tabind++ ))
print "<input"
print "  tabindex=$tabind class=page type=password id=pass2 size=20"
print "  title='4-20 chars'"
print ">"
(( tabind++ ))
print "</td>"
print "</tr>"

print "</table></div>"

print "<script>"
print "var account_pass1el = document.getElementById ('pass1')"
print "var account_pass2el = document.getElementById ('pass2')"
print "account_clearfields ()"
print "</script>"

account_emit_body_end
