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
    print -u2 ERROR program name not in correct form
    exit 1
fi
. $SWMROOT/proj/$instance/bin/${instance}_env || exit 1

cm_data=(
    pid=''
    bannermenu='link|home||Home|reload this home page|link|back||Back|go back one page|list|favorites||Favorites|run a favorite query|list|tools|p:c|Tools|tools|list|help|userguide:confmgrpage|Help|documentation'
)

function cm_init {
    cm_data.pid=$pid

    return 0
}

function cm_emit_header_begin {
    print '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 TRANSITIONAL//EN">'
    print "<html>"
    print "<head>"

    ph_emitheader page banner bdiv
    print "<title>${ph_data.title}</title>"

    return 0
}

function cm_emit_header_end {
    print "</head>"

    return 0
}

function cm_emit_body_begin {
    typeset vt st img

    if [[ $qs_mode == show ]] then
        print "<body onload='beginprinthelper()'>"
    else
        print "<body>"
    fi
    ph_emitbodyheader ${cm_data.pid} banner $qs_winw

    return 0
}

function cm_emit_body_end {
    ph_emitbodyfooter
    print "</body>"
    print "</html>"

    return 0
}

if [[ $SWIFTWARNLEVEL != "" ]] then
    PS4='${.sh.fun}-$LINENO: '
    typeset -ft cm_init
    typeset -ft cm_emit_body_begin cm_emit_body_end
    typeset -ft cm_emit_header_begin cm_emit_header_end
fi

. vg_hdr
. vg_prefshelper

suireadcgikv

if [[ $HTTP_USER_AGENT == *MSIE* ]] then
    wname=width
    hname=height
else
    wname=innerWidth
    hname=innerHeight
fi
winattr="directories=no,hotkeys=no,status=yes,resizable=yes,menubar=no"
winattr="$winattr,personalbar=no,titlebar=no,toolbar=no,scrollbars=yes"
st="style='text-align:left'"

pid=$qs_pid
if [[ $pid == '' ]] then
    print -u2 SWIFT ERROR configuration not specified
    exit 1
fi
if [[ $qs_mode == show && $qs_file != '' ]] then
    cm_data.bannermenu=${cm_data.bannermenu//confmgrpage/cm${qs_file}page}
fi
ph_init $pid "${cm_data.bannermenu}"

cgiprefix="vg_confmgr.cgi?pid=$qs_pid"

if [[ $SWMINFO == *vg_cmid=* ]] then
    export SWMCMID=${SWMINFO#*vg_cmid=}
    SWMCMID=${SWMCMID%%';'*}
fi
[[ $SWMCMID == '' ]] && export SWMCMID=$SWMID

if ! swmuseringroups 'vg_att_admin|vg_*'; then
    print "Content-type: text/html\n"
    print "<html><body><b><font color=red>"
    print "You are not authorized to access this page"
    print "</font></b></body></html>"
    exit 0
fi

. $SWIFTDATADIR/dpfiles/config.sh
export DEFAULTLEVEL
if [[ $PROPCONFS == NONE || $PROPCONFS == '' ]] then
    print "Content-type: text/html\n"
    print "<html><body><b><font color=red>"
    print "The Configuration Manager is not enabled on this system"
    print "</font></b></body></html>"
    exit 0
fi

. $SWIFTDATADIR/etc/confmgr.info
for dir in ${PATH//:/ }; do
    if [[ -f $dir/../lib/vg/vg_cmfilelist ]] then
        configfile=$dir/../lib/vg/vg_cmfilelist
        break
    fi
done

if [[ $qs_mode == @(valuehelper|list|show|edit|commit) ]] then
    print "Content-type: text/html\n"

    cm_init
    cm_emit_header_begin
    cm_emit_header_end

    cm_emit_body_begin

    print "<div class=pagemain>${ph_data.title}</div>"
fi

case $qs_mode in
list)
    . $configfile
    if [[ $? != 0 ]] then
        print -u2 ERROR missing or bad configuration file
        exit 1
    fi
    print "<div class=pagemain align=center><table class=page align=center>"
    for file in "${!files.@}"; do
        file=${file#files.}
        file=${file%%.*}
        print $file
    done | sort -u | while read -r file; do
        typeset -n fdata=files.$file
        [[ ${fdata.hidden} == yes ]] && continue
        if ! swmuseringroups "${fdata.group}"; then
            continue
        fi
        print "<tr class=page><td class=pageborder $st>"
        print "<form action=vg_confmgr.cgi method=get>"
        if [[ ${fdata.locationmode} == dir ]] then
            print "<select class=page name=rfile>"
            $SHELL ${fdata.filelist} $configfile $file | while read -r line; do
                print "<option value='${line%%'|'*}'>${line#*'|'}"
            done
            print "</select>"
            print "<input class=page type=submit name=action value=Edit>"
        elif [[
            ${fdata.accessmode} == byid
        ]] && swmuseringroups "${fdata.admingroup}"; then
            print "<input class=page type=submit name=action value='Edit Own'>"
            print "<input class=page type=submit name=action value='Edit All'>"
        else
            print "<input class=page type=submit name=action value=Edit>"
        fi
        print "<input type=hidden name=mode value=show>"
        print "<input type=hidden name=file value=$file>"
        print "<input type=hidden name=pid value=${cm_data.pid}>"
        print "<input type=hidden name=winw value=$qs_winw>"
        print "</form>"
        print "</td>"
        print "<td class=pageborder $st>${fdata.summary}</td>"
        print "<td class=pageborder $st>${fdata.description}</td>"
        print "</tr>"
        typeset +n fdata
    done
    print "</table></div>"
    ;;
show)
    . vg_hdr
    . $configfile
    if [[ $? != 0 ]] then
        print -u2 ERROR missing or bad configuration file
        exit 1
    fi
    file=$qs_file
    rfile=$qs_rfile
    export VGCM_RFILE=$rfile
    typeset -n fdata=files.$file
    if [[ ${fdata.locationmode} == dir ]] then
        filepath=${fdata.location}/$rfile
    else
        filepath=${fdata.location}
    fi
    if [[ ! -f $filepath ]] then
        print -u2 ERROR file not found $filepath
        print "<b><font color=red>"
        print "file not found $filepath"
        print "</font></b>"
        exit 1
    fi
    if ! swmuseringroups "${fdata.group}"; then
        print "<b><font color=red>"
        print "You are not authorized to modify this file"
        print "</font></b>"
        exit
    fi
    if [[ ${fdata.accessmode} == byid ]] then
        if swmuseringroups "${fdata.admingroup}"; then
            export EDITALL=1
            [[ $qs_action == 'Edit All' ]] && qs_view=all
            [[ $qs_view == all ]] && export VIEWALL=1
        fi
    fi
    if swmuseringroups "vg_att_admin|vg_confpoweruser"; then
        IGNOREREADONLY=y
    elif [[ $SWMID != $SWMCMID ]] then
        IGNOREREADONLY=y
    fi

    tmpdir=$SWIFTDATADIR/cache/cm/$SWMID.$$
    if ! mkdir -p $tmpdir; then
        print -u2 ERROR cannot create $tmpdir
        exit 1
    fi
    if ! cd $tmpdir; then
        print -u2 ERROR cannot cd to $tmpdir
        exit 1
    fi

    print "<script>"

    print "var ffields = new Array ()"
    print "var cfields = new Array ()"
    print "var recs = new Array ()"
    print "var file = '$file'"
    print "var choicefieldi = -1, currchoicei = -1"
    print "function newfield ("
    print "  path, id, name, type, defval, value, info, helper, search, size"
    print ") {"
    print "  var i, j, f = new Object (), fo, t, tt"
    print "  f.path = path"
    print "  f.id = id"
    print "  f.name = name"
    print "  f.type = type"
    print "  f.defval = defval"
    print "  f.value = value"
    print "  f.info = info"
    print "  f.helper = helper"
    print "  f.search = search"
    print "  if (size == '')"
    print "    f.size = 40"
    print "  else"
    print "    f.size = size"
    print "  f.label = document.createTextNode (f.name)"
    print "  if (type == 'choice') {"
    print "    j = -1"
    print "    f.data = document.createElement ('select')"
    print "    f.data.className = 'page'"
    print "    f.data.name = '_' + f.id"
    print "    t = f.defval.split (';')"
    print "    for (i = 0; i < t.length; i++) {"
    print "      tt = t[i].split ('|')"
    print "      fo = new Option (tt[1], tt[0])"
    print "      if (tt[0] == f.value)"
    print "        j = i"
    print "      fo.selected = false"
    print "      f.data.options[f.data.options.length] = fo"
    print "    }"
    print "    if (j != -1)"
    print "      f.data.options[j].selected = true"
    print "    f.data.size = 1"
    print "  } else if (type == 'text' || type == 'textro') {"
    print "    f.data = document.createElement ('input')"
    print "    f.data.className = 'page'"
    print "    f.data.name = '_' + f.id"
    print "    f.data.type = 'text'"
    print "    if (f.value != '')"
    print "      f.data.value = f.value"
    print "    else"
    print "      f.data.value = f.defval"
    print "    f.data.size = f.size"
    if [[ $IGNOREREADONLY != y ]] then
        print "    if (type == 'textro')"
        print "      f.data.readOnly = true"
    fi
    print "    f.data.onkeydown = disableenter"
    print "    f.data.onkeyup = disableenter"
    print "  }"
    print "  if (helper != '') {"
    print "    f.vbutton = document.createElement ('input')"
    print "    f.vbutton.className = 'page'"
    print "    f.vbutton.name = '_' + f.id + '_vbutton'"
    print "    f.vbutton.type = 'button'"
    print "    f.vbutton.value = 'Values'"
    print "    f.vbutton.title = 'Click to select from sample values'"
    print "    f.vbutton._data = f"
    print "    f.vbutton.onclick = launchvaluehelper"
    print "  } else {"
    print "    f.vbutton = document.createTextNode ('')"
    print "  }"
    print "  if (search == 'yes') {"
    print "    f.sbutton = document.createElement ('input')"
    print "    f.sbutton.className = 'page'"
    print "    f.sbutton.name = '_' + f.id + '_sbutton'"
    print "    f.sbutton.type = 'button'"
    print "    f.sbutton.value = 'Search'"
    print "    f.sbutton.title = 'Click to search file contents by this field'"
    print "    f.sbutton._data = f"
    print "    f.sbutton.onclick = valuesearch"
    print "  } else {"
    print "    f.sbutton = document.createTextNode ('')"
    print "  }"
    print "  return f"
    print "}"
    print "function newffield ("
    print "  fieldi, id, name, type, defval, value, info, helper, search, size"
    print ") {"
    print "  ffields[fieldi] = newfield ("
    print "    'fields.field' + fieldi,"
    print "    id, name, type, defval, value, info, helper, search, size"
    print "  )"
    print "  ffields[fieldi].fieldi = fieldi"
    print "  ffields[fieldi].choicei = -1"
    print "}"
    print "function newcfield ("
    print "  choicei, fieldi,"
    print "  id, name, type, defval, value, info, helper, search, size"
    print ") {"
    print "  if (cfields.length <= fieldi)"
    print "    cfields[fieldi] = new Array ()"
    print "  cfields[fieldi][choicei] = newfield ("
    print "    'multifields.choice' + choicei + '.field' + fieldi,"
    print "    id, name, type, defval, value, info, helper, search, size"
    print "  )"
    print "  cfields[fieldi][choicei].fieldi = fieldi"
    print "  cfields[fieldi][choicei].choicei = choicei"
    print "}"
    print "function setcfieldswitch () {"
    print "  if (choicefieldi > -1)"
    print "    ffields[choicefieldi].data.onchange = showcfields"
    print "}"
    print "function showffields () {"
    print "  var fieldi, f, fl, fd, fh, fs"
    print "  for (fieldi = 0; fieldi < ffields.length; fieldi++) {"
    print "    f = ffields[fieldi]"
    print "    fl = document.getElementById ('field' + fieldi + '_label')"
    print "    fd = document.getElementById ('field' + fieldi + '_data')"
    print "    fh = document.getElementById ('field' + fieldi + '_vbutton')"
    print "    fs = document.getElementById ('field' + fieldi + '_sbutton')"
    print "    fl.title = ''"
    print "    if (fl.firstChild != null)"
    print "      fl.removeChild (fl.firstChild)"
    print "    if (fd.firstChild != null)"
    print "      fd.removeChild (fd.firstChild)"
    print "    if (fh.firstChild != null)"
    print "      fh.removeChild (fh.firstChild)"
    print "    if (fs.firstChild != null)"
    print "      fs.removeChild (fs.firstChild)"
    print "    fl.title = f.info"
    print "    fl.appendChild (f.label)"
    print "    fd.appendChild (f.data)"
    print "    fh.appendChild (f.vbutton)"
    print "    fs.appendChild (f.sbutton)"
    print "  }"
    print "}"
    print "function showcfields () {"
    print "  var choicei, fieldi, f, ft, fl, fd, fh, fs"
    print "  if (choicefieldi == -1)"
    print "    return"
    print "  choicei = ffields[choicefieldi].data.selectedIndex"
    print "  currchoicei = choicei"
    print "  ft = document.getElementById ('fieldtable')"
    print "  ft.style.display = 'none'"
    print "  for (fieldi = 0; fieldi < cfields.length; fieldi++) {"
    print "    f = cfields[fieldi][choicei]"
    print "    fl = document.getElementById ('cfield' + fieldi + '_label')"
    print "    fd = document.getElementById ('cfield' + fieldi + '_data')"
    print "    fh = document.getElementById ('cfield' + fieldi + '_vbutton')"
    print "    fs = document.getElementById ('cfield' + fieldi + '_sbutton')"
    print "    fl.title = ''"
    print "    if (fl.firstChild != null)"
    print "      fl.removeChild (fl.firstChild)"
    print "    if (fd.firstChild != null)"
    print "      fd.removeChild (fd.firstChild)"
    print "    if (fh.firstChild != null)"
    print "      fh.removeChild (fh.firstChild)"
    print "    if (fs.firstChild != null)"
    print "      fs.removeChild (fs.firstChild)"
    print "    if (typeof (f) != 'undefined') {"
    print "      fl.title = f.info"
    print "      fl.appendChild (f.label)"
    print "      fd.appendChild (f.data)"
    print "      fh.appendChild (f.vbutton)"
    print "      fs.appendChild (f.sbutton)"
    print "    }"
    print "  }"
    print "  ft.style.display = 'inline'"
    print "}"
    print "var printreq, printres"
    print "function beginprinthelper () {"
    print "  var s"
    print "  s = '$cgiprefix&mode=printhelper&file=$file&rfile=$rfile'"
    if [[ $qs_view == all ]] then
        print "  s += '&view=$qs_view'"
    fi
    print "  if (window.XMLHttpRequest) {"
    print "    printreq = new XMLHttpRequest ();"
    print "    if (printreq.overrideMimeType)"
    print "      printreq.overrideMimeType ('text/xml')"
    print "  } else if (window.ActiveXObject) {"
    print "    try {"
    print "      printreq = new ActiveXObject ('Msxml2.XMLHTTP')"
    print "    } catch (e) {"
    print "      try {"
    print "        printreq = new ActiveXObject ('Microsoft.XMLHTTP')"
    print "      } catch (e) {"
    print "      }"
    print "    }"
    print "  }"
    print "  if (!printreq) {"
    print "    alert ('cannot create xml instance')"
    print "    return false"
    print "  }"
    print "  printreq.onreadystatechange = endprinthelper"
    print "  printreq.open ('GET', s, true)"
    print "  printreq.send (null)"
    print "}"
    print "function endprinthelper () {"
    print "  var list, i, rec, rs, ri, r, kvs, kvi, kv, kvts, kts, ls, ts, o"
    print "  if (printreq.readyState != 4)"
    print "    return"
    print "  if (printreq.status != 200) {"
    print "    alert ('cannot execute query')"
    print "    return"
    print "  }"
    print "  printres = printreq.responseXML"
    print "  list = document.getElementById ('reclist')"
    print "  for (i = list.options.length - 1; i >= 0; i--)"
    print "    list.options[i] = null"
    print "  if (recs.length > 0)"
    print "    recs = new Array ()"
    print "  rs = printres.getElementsByTagName ('r')"
    print "  for (ri = 0; ri < rs.length; ri++) {"
    print "    if (!(r = rs[ri]))"
    print "      break"
    print "    rec = new Object ()"
    print "    rec.mode = ''"
    print "    rec.kvs = new Array ()"
    print "    kvs = r.getElementsByTagName ('f')"
    print "    for (kvi = 0; kvi < kvs.length; kvi++) {"
    print "      if (!(kv = kvs[kvi]))"
    print "        break"
    print "      rec.kvs[kvi] = new Object ()"
    print "      kvts = kv.firstChild.data.split ('|')"
    print "      kts = kvts[0].split (':')"
    print "      if (kts.length == 2) {"
    print "        rec.mode = 'multi'"
    print "        rec.kvs[kvi].fieldi = kts[0]"
    print "        rec.kvs[kvi].choicei = kts[1]"
    print "      } else {"
    print "        rec.kvs[kvi].fieldi = kts[0]"
    print "        rec.kvs[kvi].choicei = -1"
    print "      }"
    print "      rec.kvs[kvi].value = unescape (kvts[1])"
    print "    }"
    print "    ls = r.getElementsByTagName ('l')"
    print "    ts = r.getElementsByTagName ('t')"
    print "    rec.label = unescape (ls[0].firstChild.data)"
    print "    rec.text = unescape (ts[0].firstChild.data)"
    print "    recs[ri] = rec"
    print "    recs[ri].o = new Option (rec.label, rec.text)"
    print "    recs[ri].o.reci = ri"
    print "    rec.opti = list.options.length"
    print "    list.options[list.options.length] = recs[ri].o"
    print "  }"
    print "  showliststatus (true)"
    print "}"
    print "function edithelper () {"
    print "  var s, l, f, fieldi, fv"
    print "  s = '<r>'"
    print "  l = ''"
    print "  for (fieldi = 0; fieldi < ffields.length; fieldi++) {"
    print "    f = ffields[fieldi]"
    print "    s += '<f>'"
    print "    fv = escape (f.data.value)"
    print "    s += fieldi + '|' + fv"
    print "    s += '</f>'"
    print "    if (l.length > 0)"
    print "      l += ' - '"
    print "    l += fv"
    print "  }"
    print "  if (choicefieldi != -1 && currchoicei != -1) {"
    print "    for (fieldi = 0; fieldi < cfields.length; fieldi++) {"
    print "      f = cfields[fieldi][currchoicei]"
    print "      if (typeof (f) == 'undefined')"
    print "        break"
    print "      s += '<f>'"
    print "      fv = escape (f.data.value)"
    print "      s += fieldi + ':' + currchoicei + '|' + fv"
    print "      s += '</f>'"
    print "      if (l.length > 0)"
    print "        l += ' - '"
    print "      l += fv"
    print "    }"
    print "  }"
    print "  s += '<l>' + l + '</l></r>'"
    print "  return s"
    print "}"
    print "function fieldsfromlist (t) {"
    print "  var rec, kvi, kv, field, fi, fo"
    print "  rec = recs[t.options[t.selectedIndex].reci]"
    print "  for (kvi = 0; kvi < rec.kvs.length; kvi++) {"
    print "    kv = rec.kvs[kvi]"
    print "    if (kv.choicei == -1)"
    print "      field = ffields[kv.fieldi]"
    print "    else"
    print "      field = cfields[kv.fieldi][kv.choicei]"
    print "    field.data.value = kv.value"
    print "  }"
    print "  showcfields ()"
    print "}"
    print "function disableenter (e) {"
    print "  var ev"
    print "  ev = (e) ? e : ((event) ? event : null)"
    print "  if (ev && ev.keyCode == 13)"
    print "    return false"
    print "}"
    print "function launchvaluehelper () {"
    print "  var f = this._data"
    print "  window.open ("
    print "    '$cgiprefix&mode=valuehelper&helper=' +"
    print "    f.helper + '&file=' + file + '&path=' + f.path +"
    print "    '&dir=' + '${tmpdir#$SWIFTDATADIR}&winw=500',"
    print "    'vg_cmvaluehelper', '$wname=500,$hname=500,$winattr'"
    print "  )"
    print "}"
    print "function valuesearch () {"
    print "  var f, s, list, i, reci, rec, kvi, kv"
    print "  f = this._data"
    print "  s = f.data.value.toLowerCase ()"
    print "  list = document.getElementById ('reclist')"
    print "  for (i = list.options.length - 1; i >= 0; i--)"
    print "    list.options[i] = null"
    print "  for (reci = 0; reci < recs.length; reci++) {"
    print "    rec = recs[reci]"
    print "    rec.o.selected = false"
    print "    for (kvi = 0; kvi < rec.kvs.length; kvi++) {"
    print "      kv = rec.kvs[kvi]"
    print "      if (kv.choicei != f.choicei || kv.fieldi != f.fieldi)"
    print "        continue"
    print "      if (f.type == 'choice') {"
    print "        if (kv.value != f.data.value)"
    print "          continue"
    print "      } else {"
    print "        if (kv.value.toLowerCase ().indexOf (s) == -1)"
    print "          continue"
    print "      }"
    print "      break"
    print "    }"
    print "    if (kvi == rec.kvs.length)"
    print "      continue"
    print "    list.options[list.options.length] = rec.o"
    print "  }"
    print "  if (list.options.length == 1) {"
    print "    list.selectedIndex = 0"
    print "    fieldsfromlist (list)"
    print "  }"
    print "  showliststatus (true)"
    print "}"
    print "function resetlist () {"
    print "  var list, i, reci, rec"
    print "  list = document.getElementById ('reclist')"
    print "  for (i = list.options.length - 1; i >= 0; i--)"
    print "    list.options[i] = null"
    print "  for (reci = 0; reci < recs.length; reci++) {"
    print "    rec = recs[reci]"
    print "    rec.o.selected = false"
    print "    list.options[list.options.length] = rec.o"
    print "  }"
    print "  showliststatus (true)"
    print "}"
    print "function showliststatus (done) {"
    print "  var list, liststatus, v"
    print "  list = document.getElementById ('reclist')"
    print "  liststatus = document.getElementById ('liststatus')"
    print "  if (!done)"
    print "    v = 'Reading - ' + recs.length + ' records'"
    print "  else if (list.options.length == recs.length)"
    print "    v = 'Showing all ' + recs.length + ' records'"
    print "  else {"
    print "    v = list.options.length + '/' + recs.length"
    print "    v = 'Showing ' + v + ' records'"
    print "  }"
    print "  liststatus.firstChild.nodeValue = v"
    print "}"
    print "function submitform () {"
    print "  var r"
    print "  r = document.getElementById ('record')"
    print "  r.value = edithelper ()"
    print "  return true"
    print "}"

    for (( fieldi = 0; ; fieldi++ )) do
        typeset -n field=fdata.fields.field$fieldi
        [[ ${field} == '' ]] && break
        typeset -n qsv=qs_${field.name}
        qsv=${qsv//__AMP__/'&'}
        print "newffield ("
        print "  $fieldi, '${field.name}', '${field.text}', '${field.type}',"
        print "  '${field.defval}', '${qsv}',"
        print "  '${field.info}', '${field.helper}', '${field.search}',"
        print "  '${field.size}'"
        print ")"
    done
    choicefieldi=-1
    fieldn=$fieldi
    cfieldn=-1
    if [[ ${fdata.fieldmode} == multi:* ]] then
        choicefieldi=${fdata.fieldmode#*:}
        print "choicefieldi = ${choicefieldi}"
        for (( choicei = 0; ; choicei++ )) do
            typeset -n choice=fdata.multifields.choice$choicei
            [[ ${choice} == '' ]] && break
            for (( fieldi = 0; ; fieldi++ )) do
                typeset -n field=choice.field$fieldi
                [[ ${field} == '' ]] && break
                typeset -n qsv=qs_${field.name//./_}
                qsv=${qsv//__AMP__/'&'}
                print "newcfield ("
                print "  $choicei, $fieldi, '${field.name}', '${field.text}',"
                print "  '${field.type}', '${field.defval}', '${qsv}',"
                print "  '${field.info}', '${field.helper}', '${field.search}',"
                print "  '${field.size}'"
                print ")"
            done
            if (( cfieldn < fieldi )) then
                (( cfieldn = fieldi ))
            fi
        done
        choicen=$choicei
    fi
    print "setcfieldswitch ()"

    print "</script>"

    print "<center><b>Editing ${fdata.summary}</b></center>"
    print "<div class=pagemain><form"
    print "  onSubmit='return submitform ()' action=vg_confmgr.cgi"
    if [[ $HTTP_COOKIE == *SWIFT_CODE* ]] then
        print "  method=get"
    else
        print "  method=post enctype='multipart/form-data'"
    fi
#    print "  method=post enctype='multipart/form-data'"
    print ">"
    print "<table class=page>"
    print "<tr class=page><td class=pageborder $st>"
    print "  <input class=page type=submit name=action value=Insert>"
    print "  <input class=page type=submit name=action value=Modify>"
    print "  <input class=page type=submit name=action value=Remove>"
    print "  <input class=page type=reset>"
    print "  <input type=hidden name=mode value=edit>"
    print "  <input type=hidden name=file value='$file'>"
    print "  <input type=hidden name=rfile value='$rfile'>"
    print "  <input type=hidden name=rec id=record value=''>"
    print "  <input type=hidden name=pid value=${cm_data.pid}>"
    print "  <input type=hidden name=winw value=$qs_winw>"
    print "  <input type=hidden name=view value='$qs_view'>"
    print "  <input type=hidden name=dir value='${tmpdir#$SWIFTDATADIR}'>"
    print "</td></tr>"
    print "<tr class=page><td class=pageborder $st>"
    print "  <b>Record Fields</b>"
    print "</td></tr>"
    print "<tr class=page><td class=pageborder $st>"
    print "<table class=page style='margin:1px' id=fieldtable>"

    for (( fieldi = 0; fieldi < fieldn; fieldi++ )) do
        print "<tr class=page $st>"
        print "<td class=page $st><a"
        print "  class=pagelabel id=field${fieldi}_label title=''"
        print "  href='javascript:void(0)'"
        print "></a></td>"
        print "<td class=page $st id=field${fieldi}_data name=field${fieldi}>"
        print "</td>"
        print "<td class=page $st id=field${fieldi}_vbutton></td>"
        print "<td class=page $st id=field${fieldi}_sbutton></td>"
        print "</tr>"
    done
    if [[ ${fdata.fieldmode} == multi:* ]] then
        for (( fieldi = 0; fieldi < cfieldn; fieldi++ )) do
            print "<tr class=page>"
            print "<td class=page $st><a"
            print "  class=pagelabel id=cfield${fieldi}_label title=''"
            print "  href='javascript:void(0)'"
            print "></a></td>"
            print "<td"
            print "class=page $st id=cfield${fieldi}_data name=cfield${fieldi}"
            print ">"
            print "</td>"
            print "<td class=page $st id=cfield${fieldi}_vbutton></td>"
            print "<td class=page $st id=cfield${fieldi}_sbutton></td>"
            print "</tr>"
        done
    fi

    print "</table></td></tr>"

    if [[ ${fdata.bulkinsertfield} != '' ]] then
        print "<tr class=page><td class=pageborder $st>"
        print "  <b>Bulk Insert</b>"
        print "</td></tr>"
        print "<tr class=page><td class=pageborder $st>"
        print "<table class=page style='margin:1px'>"
        typeset -n field=files.$file.bulkinsertfield
        print "<tr class=page><td class=page $st>"
        print "<a"
        print "  class=pagelabel title='${field.info}'"
        print "  href='javascript:void(0)'"
        print ">Client ${field.text}</a>"
        print "</td><td class=page $st>"
        print "<input class=page"
        print " type="file" name=cbulkfile size=${field.size:-40} value='file'"
        print ">"
        print "</td></tr><tr class=page><td class=page $st>"
        print "<a"
        print "  class=pagelabel title='${field.info}'"
        print "  href='javascript:void(0)'"
        print ">Server ${field.text}</a>"
        print "</td><td class=page $st>"
        print "<input class=page"
        print " type="text" name=sbulkfile size=${field.size:-40} value=''"
        print ">"
        print "</td></tr>"
        print "</table></td></tr>"
        if [[ ${field.format} != '' || ${field.example} != '' ]] then
            print "<tr class=page><td class=page $st>"
            if [[ ${field.format} != '' ]] then
                print "<a"
                print " class=pagelabel title='Click to see Format'"
                l='Format - cut and paste to use'
                t=$(printf '%#H' "${field.format}")
                print " onClick=\"prompt ('$l', unescape ('$t'));return false\""
                print ">Bulk Format</a>"
            fi
            [[ ${field.format} != '' && ${field.example} != '' ]] && print " - "
            if [[ ${field.example} != '' ]] then
                print "<a"
                print " class=pagelabel title='Click to see Example'"
                l='Example - cut and paste to use'
                t=$(printf '%#H' "${field.example}")
                print " onClick=\"prompt ('$l', unescape ('$t'));return false\""
                print ">Bulk Example</a>"
            fi
            print "</td></tr>"
        fi
    fi

    print "<tr class=page><td class=pageborder $st>"
    print "  <b>File Contents</b>"
    print "</td></tr>"
    print "<tr class=page><td class=pageborder $st>"
    print "<input"
    print "  class=page type=button value='Show All' onClick='resetlist ()'"
    print ">"
    print "<span id=liststatus>Reading records</span>"
    print "</td></tr>"
    print "<tr class=page><td class=pageborder $st>"
    print "<select class=page"
    # temporary fix until BD corrects problem with POST queries
    if [[ $HTTP_COOKIE == *SWIFT_CODE* ]] then
        print "id=reclist name=reclist size=${fdata.listsize:-10}"
    else
        print "id=reclist name=reclist size=${fdata.listsize:-10} multiple"
    fi
#    print "id=reclist name=reclist size=${fdata.listsize:-10} multiple"
    print "onChange='fieldsfromlist (this)'"
    print ">"
    print "</select>"
    print "</td></tr>"

    print "</table>"
    print "</form>"
    print "</div><br>"

    print "<div class=page>"

    print "<b>${fdata.summary}</b>"
    print "<p>${fdata.description}"
    print "<br><br><b>Fields</b>"

    for (( fieldi = 0; ; fieldi++ )) do
        typeset -n field=fdata.fields.field$fieldi
        [[ ${field} == '' ]] && break
        print "<p><b>${field.text}</b><br>${field.info}<br>"
    done
    if [[ ${fdata.fieldmode} == multi:* ]] then
        for (( choicei = 0; ; choicei++ )) do
            typeset -n choice=fdata.multifields.choice$choicei
            [[ ${choice} == '' ]] && break
            for (( fieldi = 0; ; fieldi++ )) do
                typeset -n field=choice.field$fieldi
                [[ ${field} == '' ]] && break
                print "<p><b>${field.text}</b><br>${field.info}<br>"
            done
        done
    fi
    if [[ ${fdata.bulkinsertfield} != '' ]] then
        print "<b>Bulk Insert</b>"
        typeset -n field=files.$file.bulkinsertfield
        print "<p><b>${field.text}</b><br>${field.info}<br>"
    fi

    print "<br><br><b>Buttons</b>"
    print "<p><b>Insert</b><br>Insert a new record."
    print "Fill out all fields and then click on Insert.<br>"
    print "<p><b>Modify</b><br>Modify a record."
    print "Select a record from the list,"
    print "modify its values and click Modify.<br>"
    print "<p><b>Remove</b><br>Remove records."
    print "Select one or more records from the list"
    print "and then click on Remove.<br>"
    print "<p><b>Reset</b><br>Reset the record fields.<br>"
    print "<p><b>Show all</b><br>Reset the record list to show all records.<br>"

    print "</div>"

    print "<script>"
    print "showffields ()"
    print "showcfields ()"
    print "</script>"
    ;;
edit)
    . $configfile
    if [[ $? != 0 ]] then
        print -u2 ERROR missing or bad configuration file
        exit 1
    fi
    file=$qs_file
    rfile=$qs_rfile
    export VGCM_RFILE=$rfile
    typeset -n fdata=files.$file
    if ! swmuseringroups "${fdata.group}"; then
        print "<b><font color=red>"
        print "You are not authorized to modify this file"
        print "</font></b>"
        exit
    fi
    if [[ ${fdata.accessmode} == byid ]] then
        if swmuseringroups "${fdata.admingroup}"; then
            export EDITALL=1
            [[ $qs_view == all ]] && export VIEWALL=1
        fi
    fi

    tmpdir=$SWIFTDATADIR/$qs_dir
    if ! cd $tmpdir; then
        print -u2 ERROR cannot cd to $tmpdir
        exit 1
    fi

    print "<script>"
    print "function cancelform () {"
    print "  window.close ()"
    print "}"
    print "</script>"
    print "<center><b>Checking Changes to ${fdata.summary}</b></center><br>"
    print "<div class=pagemain align=center><table class=page>"

    eflag=n
    typeset -l action=$qs_action
    case $action in
    remove)
        if (( ${#qs_reclist[@]} < 1 )) then
            eflag=y
            print "<tr class=page><td class=page><b><font color=red>"
            print "ERROR: must select at least one record to remove"
            print "</font></b></td></tr>"
        else
            for rec in "${qs_reclist[@]}"; do
                print -r "$rec"
            done | $SHELL ${fdata.fileprint} $configfile $file - def \
            | while read -r rec; do
                print -r "<a>remove</a><u>$SWMID</u><o>$rec</o>"
            done > edits2check.xml
        fi
        ;;
    modify)
        if (( ${#qs_reclist[@]} != 1 )) then
            eflag=y
            print "<tr class=page><td class=page><b><font color=red>"
            print "ERROR: must select exactly one record to modify"
            print "</font></b></td></tr>"
        else
            for rec in "${qs_reclist[@]}"; do
                print -r "$rec"
                break
            done | $SHELL ${fdata.fileprint} $configfile $file - def \
            | while read -r rec; do
                print -r "<a>modify</a><u>$SWMID</u><o>$rec</o><n>$qs_rec</n>"
            done > edits2check.xml
        fi
        ;;
    insert)
        if [[ $qs_cbulkfile != '' || $qs_sbulkfile != '' ]] then
            (
                if [[ $qs_cbulkfile != '' ]] then
                    for v in "${qs_cbulkfile[@]}"; do
                        print -r -- "$v"
                    done
                fi
                if [[ $qs_sbulkfile != '' ]] then
                    cat ${qs_sbulkfile}
                fi
            ) | $SHELL ${fdata.fileprint} $configfile $file - def \
            | while read -r rec; do
                print -r "<a>insert</a><u>$SWMID</u><n>$rec</n>"
            done
        else
            print -r "<a>insert</a><u>$SWMID</u><n>$qs_rec</n>"
        fi > edits2check.xml
        ;;
    esac

    $SHELL ${fdata.filecheck} $configfile $file edits2check.xml \
    | while read -r line; do
        tag=${line%%'|'*}
        rest=${line#$tag'|'}
        case $tag in
        ERROR)
            eflag=y
            print "<tr class=page><td class=page>"
            print "<b><font color=red>ERROR: $rest</font></b>"
            print "</td></tr>"
            ;;
        WARNING)
            print "<tr class=page><td class=page>"
            print "<font color=red>WARNING: $rest</font>"
            print "</td></tr>"
            ;;
        REC)
            print -u3 -r "$rest"
            ;;
        *)
            print "<tr class=page><td class=page>$tag: $rest</td></tr>"
            ;;
        esac
    done 3> editschecked.xml
    print "</table></div>"
    print "<div class=page align=center><form"
    print "  action=vg_confmgr.cgi method=get"
    print ">"
    print "<table class=page><tr class=page><td class=page>"
    if [[ $eflag == n ]] then
        print "  <input class=page type=submit name=action value=Commit>"
        print "  <input"
        print "    class=page type=button name=action value='Back'"
        print "    onClick='history.back ()'"
        print "  >"
        print "y" > cancommit
        print "$rfile" > rfile
    else
        print "  <input"
        print "    class=page type=button name=action value='Back'"
        print "    onClick='history.back ()'"
        print "  >"
        print "n" > cancommit
    fi
    print "</td></tr></table>"
    secret=$RANDOM.$RANDOM.$RANDOM
    print -- $secret > secret
    print "  <input type=hidden name=mode value=commit>"
    print "  <input type=hidden name=file value=$file>"
    print "  <input type=hidden name=pid value=${cm_data.pid}>"
    print "  <input type=hidden name=winw value=$qs_winw>"
    print "  <input type=hidden name=dir value='${tmpdir#$SWIFTDATADIR}'>"
    print "  <input type=hidden name=view value=$qs_view>"
    print "  <input type=hidden name=secret value='$secret'>"
    print "</form></div>"
    ;;
commit)
    . $configfile
    if [[ $? != 0 ]] then
        print -u2 ERROR missing or bad configuration file
        exit 1
    fi
    file=$qs_file
    typeset -n fdata=files.$file
    if ! swmuseringroups "${fdata.group}"; then
        print "<b><font color=red>"
        print "You are not authorized to modify this file"
        print "</font></b>"
        exit
    fi
    if [[ ${fdata.accessmode} == byid ]] then
        if swmuseringroups "${fdata.admingroup}"; then
            export EDITALL=1
            [[ $qs_view == all ]] && export VIEWALL=1
        fi
    fi

    tmpdir=$SWIFTDATADIR/$qs_dir
    if ! cd $tmpdir; then
        print -u2 ERROR cannot cd to $tmpdir
        exit 1
    fi

    if [[ $(< secret) != $qs_secret ]] then
        print "<b><font color=red>"
        print "You are not authorized to commit these changes"
        print "</font></b>"
        exit
    fi
    if [[ $(< cancommit) != y ]] then
        print "<b><font color=red>"
        print "You are not allowed to commit these changes"
        print "</font></b>"
        exit
    fi
    rfile=$(< rfile)
    export VGCM_RFILE=$rfile

    print "<center><b>Commiting Changes to ${fdata.summary}</b></center><br>"

    ts=$(printf '%(%Y%m%d-%H%M%S)T')
    sn=$VG_SYSNAME
    of=cm.$ts.$file.${rfile//./_X_X_}.$sn.$SWMID.$RANDOM.edit.xml
    mkdir -p $SWIFTDATADIR/incoming/cm
    cp editschecked.xml $SWIFTDATADIR/incoming/cm/$of.tmp \
    && mv $SWIFTDATADIR/incoming/cm/$of.tmp $SWIFTDATADIR/incoming/cm/$of

    if [[ $? != 0 ]] then
        print "<font color=red><b>"
        print ERROR cannot commit changes for file $file
        print "</b></font><br>"
        exit 0
    fi

    $SWIFTDATADIR/etc/confjob once &

    print "<div class=page align=center>"
    print "Changes have been submitted<br>"
    print "It usually takes 30 seconds<br>"
    print "for changes to propagate to<br>"
    print "all systems"
    print "</div>"
    print "<div class=page align=center><form"
    print "  action=vg_confmgr.cgi method=get"
    print ">"
    print "<table class=page><tr class=page><td>"
    print "  <input class=page type=submit name=action value=Continue>"
    print "</tr></table>"
    print "  <input type=hidden name=mode value=show>"
    print "  <input type=hidden name=file value=$file>"
    print "  <input type=hidden name=rfile value=$rfile>"
    print "  <input type=hidden name=pid value=${cm_data.pid}>"
    print "  <input type=hidden name=view value=$qs_view>"
    print "  <input type=hidden name=winw value=$qs_winw>"
    print "</form>"
    print "</div>"
    ;;
printhelper)
    . $configfile
    if [[ $? != 0 ]] then
        print -u2 ERROR missing or bad configuration file
        exit 1
    fi
    file=$qs_file
    rfile=$qs_rfile
    export VGCM_RFILE=$rfile
    typeset -n fdata=files.$file
    if ! swmuseringroups "${fdata.group}"; then
        print "<b><font color=red>"
        print "You are not authorized to modify this file"
        print "</font></b>"
        exit
    fi
    if [[ ${fdata.accessmode} == byid ]] then
        if swmuseringroups "${fdata.admingroup}"; then
            export EDITALL=1
            [[ $qs_view == all ]] && export VIEWALL=1
        fi
    fi
    print "Content-type: text/xml"
    print "Expires: Sat, 1 Jan 2000 00:00:00 EST\n"
    print "<response>"
    $SHELL ${fdata.fileprint} $configfile $file "$rfile" def
    print "</response>"
    ;;
valuehelper)
    tmpdir=$SWIFTDATADIR/$qs_dir
    if ! cd $tmpdir; then
        print -u2 ERROR cannot cd to $tmpdir
        exit 1
    fi

    CGIPREFIX=$cgiprefix TMPDIR=$tmpdir \
    $qs_helper "$qs_file" "$qs_path"
    ;;
valuehelperhelper)
    tmpdir=$SWIFTDATADIR/$qs_dir
    if ! cd $tmpdir; then
        print -u2 ERROR cannot cd to $tmpdir
        exit 1
    fi

    CGIPREFIX=$cgiprefix TMPDIR=$tmpdir \
    $qs_helper "$qs_file" "$qs_path" "$qs_args"
    ;;
esac

if [[ $qs_mode == @(valuehelper|list|show|edit|commit) ]] then
    cm_emit_body_end
fi
