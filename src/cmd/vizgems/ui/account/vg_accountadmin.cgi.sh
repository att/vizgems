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
    bannermenu='link|home||Home|return to the home page|link|back||Back|go back one page|list|tools|p:c|Tools|tools|list|help|userguide:accountadminpage|Help|documentation'
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
ogroups=''

allgroups="all vg $mgroups $fgroups $ogroups"

tabind=1

print "Content-type: text/html\n"

if ! swmuseringroups vg_att_admin; then
    print -u2 "SWIFT ERROR accountadmin security violation user: $SWMID"
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
print "function account_create () {"
print "  var url, seli, asset, vs, vi"
print "  url = account_helper + '?mode=create'"
print "  url += '&id=' + account_nuel.value"
print "  url += '&cmid=' + account_cmuel.value"
print "  url += '&pass1=' + account_pass1el.value"
print "  url += '&pass2=' + account_pass2el.value"
print "  url += '&name=' + escape (account_nameel.value)"
print "  url += '&info=' + escape (account_infoel.value)"
print "  for (seli = 0; seli < account_mgroupel.options.length; seli++)"
print "    if (account_mgroupel.options[seli].selected)"
print "      url += '&mgroup=' + account_mgroupel.options[seli].value"
print "  for (seli = 0; seli < account_fgroupel.options.length; seli++)"
print "    if (account_fgroupel.options[seli].selected)"
print "      url += '&fgroup=' + account_fgroupel.options[seli].value"
print "  vs = account_ogroupel.value.split (' ')"
print "  for (vi = 0; vi < vs.length; vi++) {"
print "    if (vs[vi] != '')"
print "      url += '&ogroup=vg_' + escape (vs[vi])"
print "  }"
print "  for (asseti = 0; asseti < account_assets.length; asseti++) {"
print "    asset = account_assets[asseti]"
print "    for (seli = 0; seli < asset.cel.options.length; seli++)"
print "      url += '&level_' + asset.lv + '=' + asset.cel.options[seli].value"
print "  }"
print "  vg_xmlquery (url, account_editresult)"
print "}"
print "function account_modify () {"
print "  var url, seli, asset, vs, vi"
print "  url = account_helper + '?mode=modify'"
print "  url += '&id=' + account_nuel.value"
print "  url += '&cmid=' + account_cmuel.value"
print "  url += '&pass1=' + account_pass1el.value"
print "  url += '&pass2=' + account_pass2el.value"
print "  url += '&name=' + escape (account_nameel.value)"
print "  url += '&info=' + escape (account_infoel.value)"
print "  for (seli = 0; seli < account_mgroupel.options.length; seli++)"
print "    if (account_mgroupel.options[seli].selected)"
print "      url += '&mgroup=' + account_mgroupel.options[seli].value"
print "  for (seli = 0; seli < account_fgroupel.options.length; seli++)"
print "    if (account_fgroupel.options[seli].selected)"
print "      url += '&fgroup=' + account_fgroupel.options[seli].value"
print "  vs = account_ogroupel.value.split (' ')"
print "  for (vi = 0; vi < vs.length; vi++) {"
print "    if (vs[vi] != '')"
print "      url += '&ogroup=vg_' + escape (vs[vi])"
print "  }"
print "  for (asseti = 0; asseti < account_assets.length; asseti++) {"
print "    asset = account_assets[asseti]"
print "    for (seli = 0; seli < asset.cel.options.length; seli++)"
print "      url += '&level_' + asset.lv + '=' + asset.cel.options[seli].value"
print "  }"
print "  vg_xmlquery (url, account_editresult)"
print "}"
print "function account_delete () {"
print "  var url"
print "  if (!confirm ('confirm deletion of user ' + account_nuel.value + '?'))"
print "    return"
print "  url = account_helper + '?mode=delete'"
print "  url += '&id=' + account_nuel.value"
print "  vg_xmlquery (url, account_editresult)"
print "}"
print "function account_editresult (req) {"
print "  var res, recs, fields"
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
print "function account_switch2user () {"
print "  var url, uid"
print "  uid = account_ouel.options[account_ouel.selectedIndex].value"
print "  if (uid == '')"
print "    return"
print "  url = account_helper + '?mode=listuser&uid=' + uid"
print "  vg_xmlquery (url, account_fillfields)"
print "}"
print "function account_fillfields (req) {"
print "  var seli, asseti, asset, res, recs, reci, rec, fields"
print "  var k, v, vs, vi, o, cl"
print "  account_clearfields ()"
print "  res = req.responseXML"
print "  recs = res.getElementsByTagName ('r')"
print "  for (reci = 0; reci < recs.length; reci++) {"
print "    rec = recs[reci]"
print "    fields = rec.firstChild.data.split ('|')"
print "    if (fields.length < 2)"
print "      continue"
print "    k = unescape (fields[0])"
print "    v = unescape (fields[1])"
print "    if (k == 'id')"
print "      account_nuel.value = v"
print "    else if (k == 'cmid')"
print "      account_cmuel.value = v"
print "    else if (k == 'name')"
print "      account_nameel.value = v"
print "    else if (k == 'info')"
print "      account_infoel.value = v"
print "    else if (k == 'hasconf') {"
print "      cel = document.getElementById ('hasconf')"
print "      cel.innerHTML = 'DO NOT DELETE - account has CM entries in ' + v"
print "    } else if (k == 'mgroup') {"
print "      for (seli = 0; seli < account_mgroupel.options.length; seli++) {"
print "        if (account_mgroupel.options[seli].value == v) {"
print "          account_mgroupel.options[seli].selected = true"
print "          break"
print "        }"
print "      }"
print "    } else if (k == 'fgroup') {"
print "      for (seli = 0; seli < account_fgroupel.options.length; seli++) {"
print "        if (account_fgroupel.options[seli].value == v) {"
print "          account_fgroupel.options[seli].selected = true"
print "          break"
print "        }"
print "      }"
print "    } else if (k == 'ogroup') {"
print "      if (account_ogroupel.value != '')"
print "        account_ogroupel.value += ' '"
print "      account_ogroupel.value += v.substring (3, v.length)"
print "    } else {"
print "      vs = v.split (':')"
print "      for (vi = 0; vi < vs.length; vi++) {"
print "        for (asseti = 0; asseti < account_assets.length; asseti++) {"
print "          asset = account_assets[asseti]"
print "          if (k != 'level_' + asset.lv)"
print "            continue"
print "          for (seli = 0; seli < asset.lel.options.length; seli++) {"
print "            if (asset.lel.options[seli].value == vs[vi]) {"
print "              o = new Option (asset.lel.options[seli].text, vs[vi])"
print "              asset.cel.options[asset.cel.options.length] = o"
print "              break"
print "            }"
print "          }"
print "        }"
print "      }"
print "    }"
print "  }"
print "}"
print "function account_clearfields () {"
print "  var seli, asseti, asset, cel"
print "  account_nuel.value = ''"
print "  account_cmuel.value = ''"
print "  account_ouel.value = ''"
print "  account_nameel.value = ''"
print "  account_infoel.value = ''"
print "  account_pass1el.value = ''"
print "  account_pass2el.value = ''"
print "  for (seli = 0; seli < account_mgroupel.options.length; seli++)"
print "    account_mgroupel.options[seli].selected = false"
print "  for (seli = 0; seli < account_fgroupel.options.length; seli++)"
print "    account_fgroupel.options[seli].selected = false"
print "  account_ogroupel.value = ''"
print "  for (asseti = 0; asseti < account_assets.length; asseti++) {"
print "    asset = account_assets[asseti]"
print "    for (seli = asset.cel.options.length - 1; seli >= 0; seli--)"
print "      asset.cel.options[seli] = null"
print "    for (seli = 0; seli < asset.lel.options.length; seli++)"
print "      asset.lel.options[seli].selected = false"
print "  }"
print "  cel = document.getElementById ('hasconf')"
print "  cel.innerHTML = ''"
print "}"
print "</script>"
account_emit_header_end

account_emit_body_begin

print "<div class=page>${ph_data.title}</div>"

print "<div class=page><table class=page>"

print "<td class=pageborder style='text-align:center' colspan=2>&nbsp;<a"
print "  class=pageil href='javascript:void(0)' onClick='account_create()'"
print "  title='create a new account'"
print ">&nbsp;Create&nbsp;</a>&nbsp;<a"
print "  class=pageil href='javascript:void(0)' onClick='account_modify()'"
print "  title='modify existing account'"
print ">&nbsp;Modify&nbsp;</a>&nbsp;<a"
print "  class=pageil href='javascript:void(0)' onClick='account_delete()'"
print "  title='remove existing account'"
print ">&nbsp;Delete&nbsp;</a>&nbsp;<div"
print " class=pagemain style='display:inline' id=hasconf"
print "></div>&nbsp;<a"
print "  class=pageil href='javascript:void(0)' onClick='account_clearfields()'"
print "  title='clear all fields'"
print ">&nbsp;Clear&nbsp;</a>&nbsp;"
if [[ $SHOWHELP == 1 ]] then
    print "<a"
    print "  class=pageil href='javascript:void(0)'"
    print "  onClick='vg_showhelp(\"quicktips/accountadminform\")'"
    print "  title='click to read about using this form'"
    print ">&nbsp;?&nbsp;</a>"
fi
print "</td>"

print "<tr class=page>"
print "<td class=pageborder style='text-align:right'>"
print "<a"
print "  class=pagelabel href='javascript:void(0)'"
print "  title='user id (new or existing)'"
print ">&nbsp;User ID&nbsp;</a>"
if [[ $SHOWHELP == 1 ]] then
    print "<a"
    print "  class=pageil href='javascript:void(0)'"
    print "  onClick='vg_showhelp(\"quicktips/accountuserid\")'"
    print "  title='click to read about user ids'"
    print ">&nbsp;?&nbsp;</a>"
fi
print "</td>"
print "<td align=left class=pageborder>"
print "&nbsp;id:<input"
print "  tabindex=$tabind class=page type=text id=new_id size=10"
print "  title='3-12 chars from the set: a-z,A-Z,0-9,_,-'"
print ">"
(( tabind++ ))
print "&nbsp;CM id:<input"
print "  tabindex=$tabind class=page type=text id=cmid size=10"
print "  title='3-12 chars from the set: a-z,A-Z,0-9,_,-'"
print ">"
(( tabind++ ))
print "&nbsp;ids:<select"
print "  tabindex=$tabind class=page id=old_id onChange='account_switch2user()'"
print "  title='select an existing user account to edit'"
print ">"
(( tabind++ ))
print "<option value=''>select...</option>"
$SHELL vg_accountinfo -usernames | while read -r line; do
    id=${line%%'|'*}; name=${line#*'|'}
    print "<option value='$id'>$id - $name</option>"
done
print "</select>"
print "</td>"
print "</tr>"

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
print "<td class=pageborder style='text-align:right'>"
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

print "<tr class=page>"
print "<td class=pageborder style='text-align:right'>"
print "<a"
print "  class=pagelabel href='javascript:void(0)'"
print "  title='groups the user should belong to'"
print ">&nbsp;User Groups&nbsp;</a>"
if [[ $SHOWHELP == 1 ]] then
    print "<a"
    print "  class=pageil href='javascript:void(0)'"
    print "  onClick='vg_showhelp(\"quicktips/accountusergroups\")'"
    print "  title='click to read about user groups'"
    print ">&nbsp;?&nbsp;</a>"
fi
print "</td>"
print "<td class=pageborder style='text-align:left'>"
print "&nbsp;main:<select"
print "  tabindex=$tabind class=page id=mgroup"
print "  title='pick primary group'"
print ">"
(( tabind++ ))
for mg in $mgroups; do
    print "<option value=$mg>${mg#vg_}</option>"
done
print "</select>"
print "&nbsp;features:<select"
print "  tabindex=$tabind class=page id=fgroup multiple size=$fgroupn"
print "  style='vertical-align:middle'"
print "  title='pick specific features'"
print ">"
(( tabind++ ))
for fg in $fgroups; do
    print "<option value=$fg>${fg#vg_}</option>"
done
print "</select>"
print "&nbsp;other:<input"
print "  tabindex=$tabind class=page id=ogroup size=20"
print "  style='vertical-align:middle'"
print "  title='additional groups' value='$ogroups'"
print ">"
(( tabind++ ))
print "</td>"
print "</tr>"

print "<tr class=page>"
print "<td class=pageborder style='text-align:right'>"
print "<a"
print "  class=pagelabel href='javascript:void(0)'"
print "  title='user information'"
print ">&nbsp;User Info:&nbsp;</a>"
if [[ $SHOWHELP == 1 ]] then
    print "<a"
    print "  class=pageil href='javascript:void(0)'"
    print "  onClick='vg_showhelp(\"quicktips/accountuserinfo\")'"
    print "  title='click to read about user information'"
    print ">&nbsp;?&nbsp;</a>"
fi
print "</td>"
print "<td class=pageborder style='text-align:left'>"
print "&nbsp;name:<input"
print "  tabindex=$tabind class=page type=text id=name size=20"
print "  title='type in the user real name'"
print ">"
(( tabind++ ))
print "&nbsp;info:<input"
print "  tabindex=$tabind class=page type=text id=info size=20"
print "  title='optional - type in a comment about this user'"
print ">"
(( tabind++ ))
print "</td>"
print "</tr>"

print "<tr class=page>"
print "<td class=pageborder colspan=2>"
print "accessible assets"
print "</td></tr>"
egrep -v '^#' $SWIFTDATADIR/uifiles/dataqueries/levelorder.txt \
| while read -r line; do
    lv=${line%%'|'*}; rest=${line##"$lv"?('|')}
    sn=${rest%%'|'*}; rest=${rest##"$sn"?('|')}
    nl=${rest%%'|'*}; rest=${rest##"$nl"?('|')}
    fn=${rest%%'|'*}; rest=${rest##"$fn"?('|')}
    pn=${rest%%'|'*}; rest=${rest##"$pn"?('|')}
    af=${rest%%'|'*}; rest=${rest##"$af"?('|')}
    attf=${rest%%'|'*}; rest=${rest##"$attf"?('|')}
    cusf=${rest%%'|'*}; rest=${rest##"$cusf"?('|')}
    [[ $line != ?*'|'?*'|'?*'|'?*'|'?* ]] && continue
    [[ $af != yes ]] && continue
    lvs[${#lvs[@]}]=$lv
    print "<tr class=page>"
    print "<td class=pageborder style='text-align:right'>"
    print "<a"
    print "  class=pagelabel href='javascript:void(0)'"
    print "  title='$pn the user should have access to'"
    print ">&nbsp;$pn&nbsp;</a>"
    if [[ $SHOWHELP == 1 ]] then
        print "<a"
        print "  class=pageil href='javascript:void(0)'"
        print "  onClick='vg_showhelp(\"quicktips/accountaccessible\")'"
        print "  title='click to read about accessible assets'"
        print ">&nbsp;?&nbsp;</a>"
    fi
    print "</td>"
    print "<td class=pageborder style='text-align:left'>"
    print "&nbsp;selection:<select"
    print "  tabindex=$tabind class=page id=curr_$lv multiple size=8"
    print "  style='vertical-align:middle; min-width:100px'"
    print "  onChange='account_del_$lv()'"
    print "  title='click to remove asset from user list'"
    print ">"
    (( tabind++ ))
    print "</select>"
    print "&nbsp;available:<select"
    print "  tabindex=$tabind class=page id=list_$lv multiple size=8"
    print "  style='vertical-align:middle' onChange='account_add_$lv()'"
    print "  title='click to insert asset in user list'"
    print ">"
    (( tabind++ ))
    print "<option value='__ALL__'>All $pn</option>"
    SHOWRECS=1 $SHELL vg_dq_invhelper $lv ".*" | sort -t'|' -k4,4 \
    | while read -r line; do
        type=${line%%'|'*}; rest=${line##"$type"?('|')}
        [[ $type != B ]] && continue
        level=${rest%%'|'*}; rest=${rest##"$level"?('|')}
        id=${rest%%'|'*}; rest=${rest##"$id"?('|')}
        name=${rest%%'|'*}; rest=${rest##"$name"?('|')}
        print "<option value='$id'>$name</option>"
    done
    print "</select>"
    print "</td>"
    print "</tr>"
done

print "</table></div>"

print "<script>"
print "var account_nuel = document.getElementById ('new_id')"
print "var account_cmuel = document.getElementById ('cmid')"
print "var account_ouel = document.getElementById ('old_id')"
print "var account_pass1el = document.getElementById ('pass1')"
print "var account_pass2el = document.getElementById ('pass2')"
print "var account_mgroupel = document.getElementById ('mgroup')"
print "var account_fgroupel = document.getElementById ('fgroup')"
print "var account_ogroupel = document.getElementById ('ogroup')"
print "var account_nameel = document.getElementById ('name')"
print "var account_infoel = document.getElementById ('info')"
print "var account_assets = new Array ()"
for lvi in "${!lvs[@]}"; do
    lv=${lvs[$lvi]}
    print "account_assets[$lvi] = {"
    print "  'lv':'$lv',"
    print "  'cel':document.getElementById ('curr_$lv'),"
    print "  'lel':document.getElementById ('list_$lv')"
    print "}"
    print "function account_add_$lv () {"
    print "  var asset, opti, optj, t, v"
    print ""
    print "  asset = account_assets[$lvi]"
    print "  for (opti = 0; opti < asset.lel.options.length; opti++) {"
    print "    if (!asset.lel.options[opti].selected)"
    print "      continue"
    print "    asset.lel.options[opti].selected = false"
    print "    v = asset.lel.options[opti].value"
    print "    t = asset.lel.options[opti].text"
    print "    if (v == '__ALL__') {"
    print "      for (optj = asset.cel.options.length - 1; optj >= 0; optj--)"
    print "        asset.cel.options[optj] = null"
    print "    } else {"
    print "      for (optj = asset.cel.options.length - 1; optj >= 0; optj--)"
    print "        if (asset.cel.options[optj].value == '__ALL__')"
    print "          return"
    print "    }"
    print "    for (optj = 0; optj < asset.cel.options.length; optj++)"
    print "      if (asset.cel.options[optj].value == v)"
    print "        break"
    print "    if (optj < asset.cel.options.length)"
    print "      continue"
    print "    asset.cel.options[asset.cel.options.length] = new Option (t, v)"
    print "  }"
    print "}"
    print "function account_del_$lv () {"
    print "  var asset, opti"
    print ""
    print "  asset = account_assets[$lvi]"
    print "  for (opti = asset.cel.options.length - 1; opti >= 0; opti--) {"
    print "    if (asset.cel.options[opti].selected)"
    print "      asset.cel.options[opti] = null"
    print "  }"
    print "}"
done
print "account_clearfields ()"
print "</script>"

account_emit_body_end
