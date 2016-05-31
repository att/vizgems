//
// init
//

function vg_init () {
  if (vg_browser != 'IP')
    vg_mousemenubutton = 0
  if (vg_browser == 'IE') {
    document.attachEvent ('onmousedown', vg_mouseevent)
    document.attachEvent ('onmouseup', vg_mouseevent)
    document.attachEvent ('onclick', vg_mouseevent)
    document.attachEvent ('oncontextmenu', vg_contextevent)
  } else {
    document.addEventListener ('mousedown', vg_mouseevent, false)
    document.addEventListener ('mouseup', vg_mouseevent, false)
    document.addEventListener ('click', vg_mouseevent, false)
    document.addEventListener ('contextmenu', vg_contextevent, false)
  }
}


//
// bdiv
//

var vg_bdivel = null

function vg_bdivshow () {
  if (vg_bdivel == null) {
    vg_bdivel = document.getElementById ('bdiv')
  }

  vg_bdivel.innerHTML = vg_genmenu ()
  vg_activatemenu ()
}


//
// banner menu
//

var vg_menuentries, vg_submenuentries, vg_bdivisup = false

function vg_genmenu () {
  var s, sep, mi

  vg_menuentries = new Array ()
  vg_submenuentries = new Array ()

  s = ''
  sep = ''
  for (mi = 0; mi < vg_bannermenun; mi++) {
    s += sep
    sep = '|'
    s += vg_genmenuentries[vg_bannermenus[mi].type] (vg_bannermenus[mi])
    vg_menuentries[vg_bannermenus[mi].key] = {
      type:vg_bannermenus[mi].type,
      key:vg_bannermenus[mi].key,
      args:vg_bannermenus[mi].args,
      label:vg_bannermenus[mi].label,
      help:vg_bannermenus[mi].help,
      id:'bdiv_' + vg_bannermenus[mi].key,
      el:null
    }
    vg_submenuentries[vg_bannermenus[mi].key] = {
      key:vg_bannermenus[mi].key,
      id:'bdiv_' + vg_bannermenus[mi].key,
      el:null
    }
  }

  return s
}

function vg_activatemenu () {
  var mid

  for (mid in vg_menuentries) {
    vg_menuentries[mid].idel = document.getElementById (
      vg_menuentries[mid].id
    )
    vg_menuentries[mid].datael = document.getElementById (
      vg_menuentries[mid].id + '_data'
    )
    vg_submenuentries[mid].idel = vg_menuentries[mid].idel
    vg_submenuentries[mid].generated = false
  }
}

function vg_togglemenuentry (mid, flag) {
  var el, rel, l, t

  el = document.getElementById ('bdiv_' + mid + '_data')
  if (flag != null) {
    if (flag == true && el.style.visibility == 'visible')
      return
    if (flag == false && el.style.visibility != 'visible')
      return
  } else {
    if (el.style.visibility == 'visible')
      flag = false
    if (el.style.visibility != 'visible')
      flag = true
  }

  if (flag == false) {
    el.innerHTML = ''
    el.style.visibility = 'hidden'
    vg_bdivisup = false
  } else {
    if (vg_mousepopupisup)
      vg_mousepopuphidefunc ()
    vg_hidemenuentries (mid)
    el.innerHTML = vg_gensubmenu (mid)
    vg_activatesubmenu (mid)
    l = 0
    t = 0
    for (rel = vg_menuentries[mid].idel; rel; rel = rel.offsetParent) {
        l += rel.offsetLeft
        t += rel.offsetTop
    }
    el.style.left = l
    el.style.top = t + vg_menuentries[mid].idel.offsetHeight
    el.style.visibility = 'visible'
    vg_bdivisup = true
  }
  el.style.width = 'auto'
  el.style.height = 'auto'

  return false
}

function vg_hidemenuentries (nohidemid) {
  var mid

  for (mid in vg_menuentries) {
    if (nohidemid == mid)
      continue

    if (vg_menuentries[mid].datael != null) {
      vg_menuentries[mid].datael.innerHTML = ''
      vg_menuentries[mid].datael.style.visibility = 'hidden'
    }
  }
  if (nohidemid == null)
    vg_bdivisup = false
}

function vg_gensubmenu (mid) {
  return vg_gensubmenuentries[mid] ()
}

function vg_activatesubmenu (mid) {
  vg_activatesubmenuentries[mid] ()
}

function vg_togglesubmenuentry (mid, flag) {
  var el

  el = document.getElementById (mid)
  if (flag != null) {
    if (flag == true && el.style.visibility == 'visible')
      return
    if (flag == false && el.style.visibility != 'visible')
      return
  } else {
    if (el.style.visibility == 'visible')
      flag = false
    if (el.style.visibility != 'visible')
      flag = true
  }

  if (flag == false) {
    el.style.visibility = 'hidden'
    el.style.height = '0px'
    el.style.width = '0px'
    el.style.display = 'none'
  } else {
    el.style.visibility = 'visible'
    el.style.height = 'auto'
    el.style.width = 'auto'
    el.style.display = 'block'
  }

  return false
}

var vg_bannermenus = new Array (), vg_bannermenun = 0

var vg_genmenuentries = new Array ()
vg_genmenuentries['link'] = function (m) {
  var s, qs

  if (m.key == 'home') {
    qs = vg_cgidir + '/vg_home.cgi'
    s = '<a'
    s += ' class=bdivil id="bdiv_' + m.key + '"'
    s += ' href="javascript:vg_runurl(\'' + qs + '\')" title="' + m.help + '"'
    s += '>&nbsp;' + m.label.replace (/ /, '&nbsp;') + '&nbsp;</a>'
  } else if (m.key == 'back') {
    s = '<a'
    s += ' class=bdivil id="bdiv_' + m.key + '"'
    s += ' href="javascript:void(0)" title="' + m.help + '"'
    s += ' onClick="history.go(-1); return false"'
    s += '>&nbsp;' + m.label.replace (/ /, '&nbsp;') + '&nbsp;</a>'
  } else if (m.key == 'logout') {
    qs = vg_opencgidir + '/vg_swmaccess.cgi?mode=logout'
    s = '<a'
    s += ' class=bdivil id="bdiv_' + m.key + '"'
    s += ' href="javascript:vg_runurl(\'' + qs + '\')" title="' + m.help + '"'
    s += '>&nbsp;' + m.label.replace (/ /, '&nbsp;') + '&nbsp;</a>'
  }

  return s
}
vg_genmenuentries['list'] = function (m) {
  var s

  s = '<a'
  s += ' class=bdivil href="javascript:void(0)"'
  s += ' onClick="return vg_togglemenuentry(\'' + m.key + '\', null)"'
  s += ' title="' + m.help + '" id="bdiv_' + m.key + '"'
  s += '>&nbsp;' + m.label.replace (/ /, '&nbsp;') + '&nbsp;</a>'
  s += '<div'
  s += '  class=bdivlist id="bdiv_' + m.key + '_data"'
  s += '></div>'

  return s
}

var vg_gensubmenuentries = new Array ()
var vg_activatesubmenuentries = new Array ()

vg_gensubmenuentries['favorites'] = function () {
  var s, i, qs

  s = '<table class=bdiv>'

  if (typeof dq_mode != "undefined" && dq_mode != "embed") {
    s += '<tr class=bdiv><td class=bdiv><a'
    s += ' href="javascript:void(0)" '
    s += 'onClick="return vg_togglesubmenuentry(\'bdiv_favorites_form\', null)"'
    s += '  class=bdiv title="add this page to the favorites list"'
    s += '>Add to Favorites'
    s += '</a>'

    s += '<div'
    s += ' class=bdiv id="bdiv_favorites_form"'
    s += ' style="height:0px; width:0px; visibility:hidden; display:none"'
    s += '><table class=bdivform>'
    s += '<tr class=bdivform><td colspan=2 class=bdivform>'
    s += '&nbsp;<a'
    s += ' class=bdivil href="javascript:void(0)" onClick="vg_fav_add()"'
    s += ' title="add this favorite"'
    s += '>&nbsp;OK&nbsp;</a>&nbsp;'
    s += '<a'
    s += ' class=bdivil href="javascript:void(0)" onClick="vg_fav_reset()"'
    s += ' title="reset this form"'
    s += '>&nbsp;Reset&nbsp;</a>&nbsp;'
    if (vg_showhelpbuttons == '1') {
      s += '<a'
      s += '  class=bdivil href="javascript:void(0)"'
      s += '  onClick="vg_showhelp(\'quicktips/favoritesform\', false)"'
      s += '  title="click to read about using this form"'
      s += '>&nbsp;?&nbsp;</a>'
    }
    s += '</td></tr><tr class=bdivform><td class=bdivform><a'
    s += ' class=bdivlabel href="javascript:void(0)"'
    s += ' title="mandatory - the name of this favorite"'
    s += '>&nbsp;name&nbsp;</a></td><td class=bdivform>'
    s += '<input'
    s += ' class=bdiv id=bdiv_favorites_name value="' + vg_fav_name + '"'
    s += ' tabindex=31 title="mandatory - the name of this favorite"'
    s += '>'
    s += '</td></tr><tr class=bdivform><td class=bdivform><a'
    s += ' class=bdivlabel href="javascript:void(0)"'
    s += ' title="mandatory - the URL of this favorite"'
    s += '>&nbsp;URL&nbsp;</a></td><td class=bdivform>'
    s += '<input'
    s += ' class=bdiv id=bdiv_favorites_url value="' + dq_mainurl + '"'
    s += ' tabindex=32 title="mandatory - the URL of this favorite"'
    s += '>'
    s += '</td></tr></table></div>'

    s += '</td></tr>'
    s += '<tr class=bdiv><td class=bdiv><hr/></td></tr>'
  }

  for (i = 0; i < vg_favorites.length; i++) {
    qs = vg_favorites[i].q + '&' + vg_favorites[i].p
    s += '<tr class=bdiv><td class=bdiv><a'
    s += ' class=bdiv href="javascript:vg_fav_run(\''
    s += vg_favorites[i].q
    s += '\')"'
    s += ' title="click to run this query"'
    s += '>' + unescape (vg_favorites[i].n.replace (/ /, '&nbsp;'))
    s += '</a></td></tr>'
  }
  if (vg_favorites.length == 0) {
    s += '<tr class=bdiv><td class=bdiv><a'
    s += ' class=bdiv href="javascript:void(0)"'
    s += ' title="no favorites stored"'
    s += '>no favorites'
    s += '</a></td></tr>'
  }
  s += '</table>'

  return s
}
vg_activatesubmenuentries['favorites'] = function () {
  return
}

vg_gensubmenuentries['tools'] = function () {
  var s, args, argi

  vg_menuentries['tools'].fillupdateform = false
  s = '<table class=bdiv>'
  args = vg_menuentries['tools'].args.split (':')
  for (argi = 0; argi < args.length; argi++) {
    if (args[argi] == 'x') {
      s += '<tr class=bdiv><td class=bdiv><a'
      s += ' href="javascript:void(0)"'
      s += ' onClick="vg_togglesubmenuentry(\'bdiv_export_form\', null)"'
      s += ' class=bdiv title="export this page"'
      s += '>Export'
      s += '</a>'

      s += '<div'
      s += ' class=bdiv id="bdiv_export_form"'
      s += ' style="height:0px; width:0px; visibility:hidden; display:none"'
      s += '><table class=bdivform>'
      s += '<tr class=bdivform><td colspan=2 class=bdivform>'
      s += '&nbsp;<a'
      s += ' class=bdivil href="javascript:void(0)" onClick="vg_exp_run()"'
      s += ' title="export the data"'
      s += '>&nbsp;OK&nbsp;</a>&nbsp;'
      s += '<a'
      s += ' class=bdivil href="javascript:void(0)" onClick="vg_exp_clear()"'
      s += ' title="clear this form"'
      s += '>&nbsp;Clear&nbsp;</a>&nbsp;'
      if (vg_showhelpbuttons == '1') {
        s += '<a'
        s += '  class=bdivil href="javascript:void(0)"'
        s += '  onClick="vg_showhelp(\'quicktips/exportform\', false)"'
        s += '  title="click to read about using this form"'
        s += '>&nbsp;?&nbsp;</a>'
      }
      s += '</td></tr><tr class=bdivform><td class=bdivform><a'
      s += ' class=bdivlabel href="javascript:void(0)"'
      s += ' title="the file name to produce"'
      s += '>&nbsp;file&nbsp;</a></td><td class=bdivform>'
      s += '<input'
      s += ' class=bdiv id=bdiv_export_file'
      s += ' tabindex=41 title="the file name to produce"'
      s += '>'
      s += '</td></tr><tr class=bdivform><td class=bdivform><a'
      s += ' class=bdivlabel href="javascript:void(0)"'
      s += ' title="type of file to generate"'
      s += '>&nbsp;type&nbsp;</a></td><td class=bdivform>'
      s += '<select'
      s += ' class=bdiv id=bdiv_export_type'
      s += ' tabindex=42 title="type of file to generate"'
      s += '>'
      s += '<option value=csv>CSV - Comma Separated Values</option>'
      s += '<option value=xml>XML - Excel / OpenOffice</option>'
      s += '<option value=html>HTML - ZIP Bundle of Web Page</option>'
      s += '<option value=pdf>PDF - Portable Document Format</option>'
      s += '</select>'
      s += '</td></tr><tr class=bdivform><td class=bdivform><a'
      s += ' class=bdivlabel href="javascript:void(0)"'
      s += ' title="optional notes to include in export file"'
      s += '>&nbsp;notes&nbsp;</a></td><td class=bdivform>'
      s += '<input'
      s += ' class=bdiv id=bdiv_export_notes'
      s += ' tabindex=43 title="optional notes to include in export file"'
      s += '>'
      s += '</td></tr></table></div>'

      s += '</td></tr>'
    } else if (args[argi] == 'm') {
      s += '<tr class=bdiv><td class=bdiv><a'
      s += ' class=bdiv href="javascript:void(0)"'
      s += ' onClick="vg_togglesubmenuentry(\'bdiv_email_form\', null)"'
      s += ' title="email this page"'
      s += '>Email'
      s += '</a>'

      s += '<div'
      s += ' class=bdiv id="bdiv_email_form"'
      s += ' style="height:0px; width:0px; visibility:hidden; display:none"'
      s += '><table class=bdivform>'
      s += '<tr class=bdivform><td colspan=2 class=bdivform>'
      s += '&nbsp;<a'
      s += ' class=bdivil href="javascript:void(0)" onClick="vg_email_run()"'
      s += ' title="email the data"'
      s += '>&nbsp;OK&nbsp;</a>&nbsp;'
      s += '<a'
      s += ' class=bdivil href="javascript:void(0)" onClick="vg_email_clear()"'
      s += ' title="clear this form"'
      s += '>&nbsp;Clear&nbsp;</a>&nbsp;'
      if (vg_showhelpbuttons == '1') {
        s += '<a'
        s += '  class=bdivil href="javascript:void(0)"'
        s += '  onClick="vg_showhelp(\'quicktips/emailform\', false)"'
        s += '  title="click to read about using this form"'
        s += '>&nbsp;?&nbsp;</a>'
      }
      s += '</td></tr><tr class=bdivform><td class=bdivform><a'
      s += ' class=bdivlabel href="javascript:void(0)"'
      s += ' title="mandatory - email address that replies should go to"'
      s += '>&nbsp;from&nbsp;</a></td><td class=bdivform>'
      s += '<input'
      s += ' tabindex=44 class=bdiv id=bdiv_email_from'
      s += ' title="mandatory - email address that replies should go to"'
      s += '>'
      s += '</td></tr><tr class=bdivform><td class=bdivform><a'
      s += ' class=bdivlabel href="javascript:void(0)"'
      s += ' title="mandatory - email addresses to send to"'
      s += '>&nbsp;to&nbsp;</a></td><td class=bdivform>'
      s += '<input'
      s += ' tabindex=45 class=bdiv id=bdiv_email_to'
      s += ' title="mandatory - email addresses to send to"'
      s += '>'
      s += '</td></tr><tr class=bdivform><td class=bdivform><a'
      s += ' class=bdivlabel href="javascript:void(0)"'
      s += ' title="subject line"'
      s += '>&nbsp;subject&nbsp;</a></td><td class=bdivform>'
      s += '<input'
      s += ' tabindex=46 class=bdiv id=bdiv_email_subject'
      s += ' title="subject line"'
      s += '>'
      s += '</td></tr><tr class=bdivform><td class=bdivform><a'
      s += ' class=bdivlabel href="javascript:void(0)"'
      s += ' title="optional notes to include in email file"'
      s += '>&nbsp;notes&nbsp;</a></td><td class=bdivform>'
      s += '<input'
      s += ' tabindex=47 class=bdiv id=bdiv_email_notes'
      s += ' title="optional notes to include in email file"'
      s += '>'
      s += '</td></tr></table></div>'

      s += '</td></tr>'
    } else if (args[argi] == 'u') {
      s += '<tr class=bdiv><td class=bdiv><a'
      s += ' href="javascript:void(0)"'
      s += ' onClick="vg_togglesubmenuentry(\'bdiv_update_form\', null)"'
      s += ' class=bdiv title="change the update rate of this page"'
      s += '>Update Rate'
      s += '</a>'

      s += '<div'
      s += ' class=bdiv id="bdiv_update_form"'
      s += ' style="height:0px; width:0px; visibility:hidden; display:none"'
      s += '><table class=bdivform><tr class=bdivform><td class=bdivform>'
      s += '<select'
      s += ' tabindex=48 class=bdiv id=bdiv_update_rate'
      s += ' title="available update rates" onChange="vg_update_change()"'
      s += '>'
      s += '<option value=-1>No Updates</option>'
      s += '<option value=0>Update Now</option>'
      s += '<option value=1>Every 1 Min</option>'
      s += '<option value=5>Every 5 Mins</option>'
      s += '<option value=10>Every 10 Mins</option>'
      s += '<option value=15>Every 15 Mins</option>'
      s += '<option value=30>Every 30 Mins</option>'
      s += '<option value=60>Every 60 Mins</option>'
      s += '</select>'
      s += '</td></tr></table></div>'

      s += '</td></tr>'
      vg_menuentries['tools'].fillupdateform = true
    } else if (args[argi] == 'c') {
      s += '<tr class=bdiv><td class=bdiv><a'
      s += ' class=bdiv href="javascript:vg_confmgr_run()"'
      s += ' title="run the configuration manager"'
      s += '>Conf. Manager'
      s += '</a></td></tr>'
    } else if (args[argi] == 'p') {
      s += '<tr class=bdiv><td class=bdiv><a'
      s += ' class=bdiv href="javascript:vg_prefedit_run()"'
      s += ' title="run the preferences editor"'
      s += '>Preferences'
      s += '</a></td></tr>'
    }
  }
  s += '</table>'

  return s
}
vg_activatesubmenuentries['tools'] = function () {
  var sme = vg_menuentries['tools']
  var el, v, i

  if (sme.fillupdateform) {
    v = dq_updaterate + ''
    el = document.getElementById ('bdiv_update_rate')
    for (i = 0; i < el.options.length; i++) {
      if (v == el.options[i].value) {
        el.selectedIndex = i
        break
      }
    }
  }
}

vg_gensubmenuentries['help'] = function () {
  var s, args, argi, qt

  s = '<table class=bdiv>'
  args = vg_menuentries['help'].args.split (':')
  for (argi = 0; argi < args.length; argi++) {
    if (args[argi] == 'userguide') {
      s += '<tr class=bdiv><td class=bdiv><a'
      s += ' class=bdiv href="javascript:vg_showhelp(\'userguide\', true)"'
      s += ' title="click to read the complete user guide"'
      s += '>User Guide'
      s += '</a></td></tr>'
    } else {
      qt = 'quicktips/' + args[argi]
      s += '<tr class=bdiv><td class=bdiv><a'
      s += ' class=bdiv href="javascript:vg_showhelp(\'' + qt + '\', true)"'
      s += ' title="click to read about using this page"'
      s += '>About this Page'
      s += '</a></td></tr>'
    }
  }
  s += '</table>'

  return s
}
vg_activatesubmenuentries['help'] = function () {
  return
}


//
// favorites utilities
//

var vg_fav_name

function vg_fav_add () {
  var el, name, url

  el = document.getElementById ('bdiv_favorites_name')
  name = el.value
  el = document.getElementById ('bdiv_favorites_url')
  url = el.value
  if (name.length == 0 || url.length == 0) {
    alert ('please fill out both the name and URL fields')
    return
  }
  u = vg_cgidir + '/vg_cm_favoriteshelper.cgi?name=' + escape (name)
  u += '&url=' + escape (url)
  vg_togglemenuentry ('favorites', false)
  vg_xmlquery (u, vg_fav_result)
}
function vg_fav_reset () {
  var el

  el = document.getElementById ('bdiv_favorites_name')
  el.value = vg_fav_name
  el = document.getElementById ('bdiv_favorites_url')
  el.value = dq_mainurl
}
function vg_fav_run (qspec) {
  location.href = qspec
}
function vg_fav_result (req) {
  var res, recs, fields

  res = req.responseXML
  recs = res.getElementsByTagName ('r')
  if (recs.length < 1) {
    alert ('operation failed')
    return
  }
  fields = recs[0].firstChild.data.split ('|')
  if (fields[0] == 'OK') {
    alert ('success')
  } else {
    alert ('failure - errors: ' + unescape (fields[1]))
  }
}


//
// tool utilities
//

var vg_exp_win = null
function vg_exp_run () {
  var el, file, type, notes, u, n

  el = document.getElementById ('bdiv_export_file')
  if (el.value != '')
    file = el.value
  else
    file = 'visualizer'
  el = document.getElementById ('bdiv_export_type')
  type = el.options[el.selectedIndex].value
  el = document.getElementById ('bdiv_export_notes')
  notes = el.value
  vg_togglemenuentry ('tools', false)
  u = vg_cgidir + '/vg_exporthelper.cgi?file=' + escape (file)
  u += '&type=' + type + '&notes=' + escape (notes)
  u += '&name=' + dq_mainname + '&secret=' + dq_secret
  n = 'vg_export' + Math.random ().toString ().replace (/\./, '_')
  if (vg_exp_win == null || vg_exp_win.closed)
    vg_exp_win = window.open (u, n, '')
  else
    vg_exp_win.location = u
}
function vg_exp_clear () {
  var el

  el = document.getElementById ('bdiv_export_file')
  el.value = ''
  el = document.getElementById ('bdiv_export_type')
  el.selectedIndex = 0
  el = document.getElementById ('bdiv_export_notes')
  el.value = ''
}

function vg_email_run () {
  var el, from, to, subject, notes, u

  el = document.getElementById ('bdiv_email_from')
  from = el.value
  if (!from.match (/.*@.*/)) {
    alert ('please enter valid email addresses in the from and to fields')
    return
  }
  el = document.getElementById ('bdiv_email_to')
  to = el.value
  if (!to.match (/.*@.*/)) {
    alert ('please enter valid email addresses in the from and to fields')
    return
  }
  el = document.getElementById ('bdiv_email_subject')
  subject = el.value
  el = document.getElementById ('bdiv_email_notes')
  notes = el.value
  vg_togglemenuentry ('tools', false)
  u = vg_cgidir + '/vg_exporthelper.cgi?from=' + escape (from)
  u += '&to=' + escape (to) + '&subject=' + escape (subject)
  u += '&type=email' + '&notes=' + escape (notes)
  u += '&name=' + dq_mainname + '&secret=' + dq_secret
  vg_xmlquery (u, vg_email_result)
}
function vg_email_clear () {
  var el

  el = document.getElementById ('bdiv_email_from')
  el.value = ''
  el = document.getElementById ('bdiv_email_to')
  el.value = ''
  el = document.getElementById ('bdiv_email_subject')
  el.value = ''
  el = document.getElementById ('bdiv_email_notes')
  el.value = ''
}
function vg_email_result (req) {
  var res, recs, fields

  res = req.responseXML
  recs = res.getElementsByTagName ('r')
  if (recs.length < 1) {
    alert ('operation failed')
    return
  }
  fields = recs[0].firstChild.data.split ('|')
  if (fields[0] == 'OK') {
    alert ('success')
  } else {
    alert ('failure - errors: ' + unescape (fields[1]))
  }
}

function vg_update_change () {
  var el, r

  el = document.getElementById ('bdiv_update_rate')
  r = parseInt (el.options[el.selectedIndex].value)
  vg_togglemenuentry ('tools', false)
  dq_updaterate = r
  if (r == -1 || r == 0) {
    dq_settimer (r)
    return
  }

  dq_currurl = dq_mainurl
  dq_currparams = null
  dq_runcurrurl (null, null)  // this will pick up dq_updaterate
}

function vg_confmgr_run () {
  var u

  vg_getwindowsize ()
  u = vg_cgidir + '/vg_confmgr.cgi?pid=' + vg_pid + '&mode=list'
  u += '&winw=' + vg_windowsize.x
  location.href = u
}

var vg_prefeditwin = null
var vg_prefeditattr = (
  'directories=no,hotkeys=no,status=yes,resizable=yes,menubar=no' +
  ',personalbar=no,titlebar=no,toolbar=no,scrollbars=yes,width=700,height=500'
)

function vg_prefedit_run (section) {
  var u

  vg_togglemenuentry ('tools', false)
  u = vg_cgidir + '/vg_prefseditor.cgi?pid=' + vg_pid + '&mode=list'
  u += '&winw=500&winh=500'
  if (vg_prefeditwin == null || vg_prefeditwin.closed)
    vg_prefeditwin = window.open (u, "vg_prefedit", vg_prefeditattr)
  else
    vg_prefeditwin.location = u
}


//
// help utilities
//

var vg_helpwin = null
var vg_helpattr = (
  'directories=no,hotkeys=no,status=yes,resizable=yes,menubar=no' +
  ',personalbar=no,titlebar=no,toolbar=no,scrollbars=yes,width=500,height=500'
)

function vg_showhelp (section, mflag) {
  var u

  if (mflag)
    vg_togglemenuentry ('help', false)
  u = vg_opencgidir + '/vg_help.cgi?pid=' + vg_pid
  u += '&section=' + escape (section) + '&winw=500&winh=500'
  if (vg_helpwin == null || vg_helpwin.closed)
    vg_helpwin = window.open (u, "vg_help", vg_helpattr)
  else
    vg_helpwin.location = u
}


//
// events
//

var vg_mousemenubutton = 0
var vg_mousestate = new Array ()
var vg_mousereturn = new Array ()
var vg_mousepopupel = null, vg_mousepopupisup = false
var vg_mousepopupshowfunc = null, vg_mousepopuphidefunc = false

function vg_mouseevent (e) {
  var haveurl, intab, url, el, p, b, i

  haveurl = false
  intab = false
  url = ''
  if (vg_browser == "IE") {
    for (el = e.srcElement; el; el = el.parentElement) {
      if (
        !haveurl && typeof el.href != "undefined" &&
        !el.href.match (/vg_dq_imghelper/)
      ) {
        url = el.href
        haveurl = true
      }
      if (
        el.className.match (/.*page.*/) &&
        el.nodeName.match (/^(A|INPUT|SELECT)$/)
      )
        return
      if (
        el.className.match (/.*leaflet.*/) ||
        el.className.match (/.*nomousemenu.*/)
      ) {
        intab = true
        break
      }
      if (el.className.match (/.*tab_.*/))
        intab = true
      if (el == vg_bdivel || el == vg_mousepopupel)
        return
    }
  } else {
    if (e.target.nodeName == 'HTML' && vg_browser != 'IP')
      return
    for (el = e.target; el; el = el.parentNode) {
      if (!haveurl && typeof el.href != "undefined") {
        url = el.href
        haveurl = true
      }
      if (typeof el.className != "undefined") {
        if (
          el.className.match (/page.*/) &&
          el.nodeName.match (/^(A|INPUT|SELECT)$/)
        )
          return
        if (
          el.className.match (/.*leaflet.*/) ||
          el.className.match (/.*nomousemenu.*/)
        ) {
          intab = true
          break
        }
        if (el.className.match (/.*tab_.*/))
          intab = true
      }
      if (el == vg_bdivel || el == vg_mousepopupel)
        return
    }
  }
  if (vg_bdivisup) {
    vg_hidemenuentries (null)
    return
  }
  if (vg_mousepopupshowfunc == null)
    return
  if (intab && !haveurl) {
    if (vg_mousepopupisup)
      vg_mousepopuphidefunc ()
    return
  }

  if (e.type == 'mousedown')
    p = 0
  else if (e.type == 'mouseup')
    p = 1
  else if (e.type == 'click')
    p = 2
  else
    p = 3

  if (p < 2) {
    if (vg_browser == "IE") {
      b = -1
      if (e.button == 1)
        b = 0
      else if (e.button == 4)
        b = 1
      else if (e.button == 2)
        b = 2
    } else {
      if (e.button >= 0 && e.button <= 2)
        b = e.button
    }
    if (b == -1)
      return

    if (p == 0) {
      vg_mousestate[b] = 'down'
      vg_mousereturn[b] = true
    } else {
// not sure why this was here
//      if (vg_browser == 'FF' && vg_mousestate[b] != 'down')
//        return
      vg_mousestate[b] = 'up'
    }
  } else if (p == 2) {
    for (i = 0; i < 2; i++)
      if (vg_mousestate[i] == 'up') {
        vg_mousestate[i] = ''
        if (!vg_mousereturn[i])
          vg_stopchain (e)
        return
      }
    return
  }

  // p == 0|1 from this point on

  if (p == 0) {
    vg_stopchain (e)
    return
  }

  // p == 1 from this point on

  if (vg_mousemenubutton != b) {
    vg_mousereturn[b] = true
    return
  }

  // p == 1 and b == menubutton from this point on

  if (url == '') {
    if (vg_mousepopupshowfunc != null)
      vg_mousepopupshowfunc (null, e, true)
    vg_mousereturn[b] = false
  } else if (url.match (/javascript:void.*/)) {
    vg_mousereturn[b] = false
  } else if (!url.match (/.*dserverhelper.*obj=.*/)) {
    vg_mousereturn[b] = true
    return
  } else {
    if (vg_mousepopupshowfunc != null)
      vg_mousepopupshowfunc (url, e, true)
    vg_mousereturn[b] = false
  }

  vg_stopchain (e)
  return
}

function vg_contextevent (e) {
  if (vg_mousemenubutton == 2)
    vg_stopchain (e)
  return
}

function vg_stopchain (e) {
  if (vg_browser == 'IE') {
    e.returnValue = false
    e.cancelBubble = true
  } else
    e.preventDefault ()
}


//
// URL management
//

function vg_runurl (url) {
  document.cookie = 'attVGvars=; path=/'
  vg_getwindowsize ()
  if (url.indexOf ('?') >= 0)
    url += '&'
  else
    url += '?'
  url += 'winw=' + vg_windowsize.x + '&winh=' + vg_windowsize.y
  location.href = url
  return false
}


//
// XML handling
//

function vg_xmlquery (url, func) {
  var req

  function handler () {
    if (req.readyState != 4)
      return
    if (req.status != 200) {
      alert ('cannot execute query')
      return
    }
    func (req)
  }

  if (window.XMLHttpRequest) {
    req = new XMLHttpRequest ();
    if (req.overrideMimeType)
      req.overrideMimeType ('text/xml')
  } else if (window.ActiveXObject) {
    try {
      req = new ActiveXObject ('Msxml2.XMLHTTP')
    } catch (e) {
      try {
        req = new ActiveXObject ('Microsoft.XMLHTTP')
      } catch (e) {
      }
    }
  }
  if (!req) {
    alert ('cannot create xml instance')
    return false
  }
  req.onreadystatechange = handler
  req.open ('GET', url, true)
  req.send (null)
}


//
// utilities
//

var vg_windowsize = { x:1024, y:768 }

function vg_getwindowsize () {
  if (vg_browser == 'IE') {
    vg_windowsize.x = document.body.clientWidth
    vg_windowsize.y = document.body.clientHeight
  } else if (vg_browser == 'BB') {
    vg_windowsize.x = window.innerWidth
    vg_windowsize.y = window.innerHeight
  } else if (typeof window.innerWidth != "undefined") {
    vg_windowsize.x = window.innerWidth
    vg_windowsize.y = window.innerHeight
  }
}
