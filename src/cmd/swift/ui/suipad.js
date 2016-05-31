var suipad_padwindow = parent.frames[0]
var suipad_lsswindow = parent.frames[1]
var suipad_reswindow = parent.frames[2]
var suipad_lsobjs = new Array (1)
var suipad_mname
var suipad_mport
var suipad_dname
var suipad_dport
var suipad_ddir
var suipad_whost = (
  parent.frames[0].location.protocol + '//' + parent.frames[0].location.host
)
var suipad_winattr = (
  'directories=no,hotkeys=no,status=yes,resizable=yes,menubar=no' +
  ',personalbar=no,titlebar=no,toolbar=no,scrollbars=yes'
)
var suipad_datewindow
var suipad_viewwindow
var suipad_querywindow
var suipad_date = "latest"
var suipad_viewname = "Complete Network"
var suipad_view = 0
var suipad_timeron = 0
var suipad_timerind = 0
var suipad_timerid
var suipad_reloadonce = 0

// initialization

function suipad_main (mhost, mport, dhost, dport, ddir) {
  suipad_mname = mhost
  suipad_mport = mport
  suipad_dname = dhost
  suipad_dport = dport
  suipad_ddir = ddir
}

function suipad_exit () {
  var lsi

  for (lsi = 0; lsi < suipad_lsobjs.length; lsi++)
    if (typeof (suipad_lsobjs[lsi]._win) != "undefined")
      if (!suipad_lsobjs[lsi]._win.closed)
        suipad_lsobjs[lsi]._win.close ()
  if (!suipad_datewindow.closed)
    suipad_datewindow.close ()
  if (!suipad_viewwindow.closed)
    suipad_viewwindow.close ()
  if (!suipad_querywindow.closed)
    suipad_querywindow.close ()
  if (typeof (suipad_ds_exit) != "undefined")
    return suipad_ds_exit ()
  return 0
}

// command handling functions

function suipad_insert (o) {
  var rtn, lsi, winname, win

  if ((rtn = suipad_allowinsert (o)) == 0)
    return
  if (rtn == 1 || rtn == 3) {
    for (lsi = 0; lsi < suipad_lsobjs.length; lsi++)
      if (suipad_lsobjs[lsi] && suipad_lsobjs[lsi]._name == o._name) {
        if (typeof (o._replay) == "undefined")
          alert ('dataset already loaded')
        return
      }
    lsi = suipad_lsobjs.length
    suipad_lsobjs[lsi] = o
    suipad_showloadedsets ()
  }
  if (rtn == 2 || rtn == 3) {
    if (rtn == 2)
      winname = o._winname
    else
      winname = o._winname + '_' + lsi
    win = window.open (o._url, winname, o._winattr)
    win.focus ()
    if (rtn == 3)
      suipad_lsobjs[lsi]._win = win
  }
}

function suipad_remove (o) {
  var lsi

  for (lsi = 0; lsi < suipad_lsobjs.length; lsi++)
    if (suipad_lsobjs[lsi] && suipad_lsobjs[lsi]._name == o._name)
      break
  if (lsi == suipad_lsobjs.length)
    return
  if (typeof (suipad_lsobjs[lsi]._win) != "undefined")
    if (!suipad_lsobjs[lsi]._win.closed)
      suipad_lsobjs[lsi]._win.close ()
  delete suipad_lsobjs[lsi]
  suipad_showloadedsets ()
}

function suipad_reload (o) {
  if (typeof (o._win) != "undefined")
    o._win.location = o._url
}

function suipad_removeall () {
  var lsi

  for (lsi = 0; lsi < suipad_lsobjs.length; lsi++)
    if (suipad_lsobjs[lsi])
      suipad_remove (suipad_lsobjs[lsi])
}

function suipad_reloadall (o) {
  var lsi

  for (lsi = 0; lsi < suipad_lsobjs.length; lsi++)
    if (suipad_lsobjs[lsi])
      suipad_reload (suipad_lsobjs[lsi])
}

function suipad_reloadmany (o) {
  var lsi

  if (suipad_timeron == 0 && suipad_reloadonce == 0)
    return
  suipad_reloadonce = 0
  if (typeof (o._name) != "undefined") {
      for (lsi = 0; lsi < suipad_lsobjs.length; lsi++)
        if (suipad_lsobjs[lsi] && suipad_lsobjs[lsi]._name == o._name)
          suipad_reload (suipad_lsobjs[lsi])
  } else {
    for (lsi = 0; lsi < suipad_lsobjs.length; lsi++)
      if (suipad_lsobjs[lsi])
        suipad_reload (suipad_lsobjs[lsi])
  }
}

function suipad_focus (o) {
  suipad_showselection (o);
}

function suipad_selection (o) {
  suipad_showselection (o);
}

// command handling support functions

function suipad_allowinsert (o) {
  var rtn

  if (typeof (suipad_ds_allowinsert) != "undefined")
    if ((rtn = suipad_ds_allowinsert (o)) == 0)
      return 0
  if (typeof (o._label) == "undefined")
    o._label = o._name
  return rtn
}

function suipad_showselection (o) {
  if (suipad_showselparam == "0")
    return
  if (!suipad_querywindow || suipad_querywindow.closed)
    suipad_querywindow = window.open (
      suipad_whost + suipad_wcgi + '/' + suipad_instance +
      '_showselection.cgi?' + suipad_packcmd (o),
      suipad_instance + "_showselection",
      suipad_wname + '=640,' + suipad_hname + '=400,' + suipad_winattr
    )
  else
    suipad_querywindow.location = (
      suipad_whost + suipad_wcgi + '/' + suipad_instance +
      '_showselection.cgi?' + suipad_packcmd (o)
    )
  suipad_querywindow.focus ()
}

function suipad_loadset (o) {
  if (typeof (suipad_ds_loadset) != "undefined")
    suipad_ds_loadset (o)
}

function suipad_unloadset (b) {
  if (typeof (suipad_ds_unloadset) != "undefined")
    suipad_ds_unloadset (b)
}

function suipad_showloadedsets () {
  var lsi, s

  s = '<table>'
  for (lsi = 0; lsi < suipad_lsobjs.length; lsi++) {
    if (suipad_lsobjs[lsi]) {
      s += '<tr><td width=10%><input'
      s += ' type=button name=unload_' + lsi + ' value=Unload'
      s += ' onClick="parent.frames[0].suipad_unloadset (this)"'
      s += '></td><td width=90%>' + suipad_lsobjs[lsi]._label + '</td></tr>'
    }
  }
  s += '</table>'
  suipad_lsswindow.suilss_update (s)
}

function suipad_dofocus (o, framei, itemi) {
  o._cmd = 'focus'
  o._framef = framei
  o._framel = framei
  o._items = new Array (itemi)
  suipad_reswindow.location = (
    suipad_whost + suipad_wcgi + '/' + suipad_instance + '_mserverhelper.cgi?' +
    suipad_packcmd (o)
  )
}

function suipad_seturl (o, group) {
  o._url = (
    suipad_whost + suipad_wcgi +
    '/' + suipad_instance + '_dserverhelper.cgi?' + suipad_packcmd (o)
  )
  o._reloadmode = 0
  o._winname = suipad_instance + '_' + group
  o._winattr = suipad_winattr
  if (
    typeof (suitable_x[group]) != "undefined" &&
    typeof (suitable_y[group]) != "undefined"
  )
    o._winattr += (
      ',screenX=' + suitable_x[group] + ',screenY=' + suitable_y[group]
    )
  if (
    typeof (suitable_w[group]) != "undefined" &&
    typeof (suitable_h[group]) != "undefined"
  )
    o._winattr += (
      ',' + suipad_wname + '=' + suitable_w[group] +
      ',' + suipad_hname + '=' + suitable_h[group]
    )
}

// utility functions

function suipad_packm () {
  return 'mname=' + suipad_mname + '&mport=' + suipad_mport
}

function suipad_packcmd (o) {
  var s, i

  s = suipad_packm () + '&prefprefix=' + suipad_prefprefix
  for (i in o) {
    if (typeof (o[i]) == "object")
      s += '&' + i.substr (1) + '=[' + suipad_packcmdrec (o[i]) + ']'
    else if (typeof (i) == "string")
      s += '&' + i.substr (1) + '=' + escape (o[i])
    else
      s += '&' + escape (o[i])
  }
  return s
}

function suipad_packcmdrec (o) {
  var s, c, i

  s = ''
  c = ''
  for (i in o) {
    if (typeof (o[i]) == "object")
      s += c + i.substr (1) + '=[' + suipad_packcmdrec (o[i]) + ']'
    else if ((typeof (i) == "string") && (i.substr(1) != ""))
      s += c + i.substr (1) + '=' + escape (o[i])
    else
      s += c + escape (o[i])
    c = ';'
  }
  return s
}

function suipad_selectdate () {
  if (!suipad_datewindow || suipad_datewindow.closed)
    suipad_datewindow = window.open (
      suipad_whost + suipad_wcgi + '/' +
      suipad_instance + '_dateselect.cgi?level=1&prefprefix=' +
      suipad_prefprefix,
      suipad_instance + "_date",
      suipad_wname + '=320,' + suipad_hname + '=600,' + suipad_winattr
    )
  else
    suipad_datewindow.location = (
      suipad_whost + suipad_wcgi + '/' +
      suipad_instance + '_dateselect.cgi?level=1&prefprefix=' +
      suipad_prefprefix
    )
  suipad_datewindow.focus ()
}

function suipad_selectview () {
  if (!suipad_viewwindow || suipad_viewwindow.closed)
    suipad_viewwindow = window.open (
      suipad_whost + suipad_wcgi + '/' +
      suipad_instance + '_viewselect.cgi?level=1&prefprefix=' +
      suipad_prefprefix,
      suipad_instance + "_view",
      suipad_wname + '=640,' + suipad_hname + '=400,' + suipad_winattr
    )
  else
    suipad_viewwindow.location = (
      suipad_whost + suipad_wcgi + '/' +
      suipad_instance + '_viewselect.cgi?level=1&prefprefix=' +
      suipad_prefprefix
    )
  suipad_viewwindow.focus ()
}

function suipad_cancelall () {
  var o

  o = new Object ()
  o._name = 'cancel'
  o._special = 'cancelall'
  suipad_reswindow.location = (
    suipad_whost + suipad_wcgi + '/' + suipad_instance + '_dserverhelper.cgi?' +
    suipad_packcmd (o)
  )
}

function suipad_settimer () {
  var i, v

  i = document.forms[0].update.selectedIndex
  v = document.forms[0].update.options[i].value
  if (v == -1) {
    if (suipad_timeron == 1)
      clearTimeout (suipad_timerid)
    suipad_timeron = 0
    suipad_timerind = i
  } else if (v == 0) {
    suipad_alarm (0)
    document.forms[0].update.selectedIndex = suipad_timerind
    suipad_reloadonce = 1
  } else {
    if (suipad_timeron == 1)
      clearTimeout (suipad_timerid)
    suipad_timerid = setTimeout ('suipad_alarm (1)', v * 60000)
    suipad_timeron = 1
    suipad_timerind = i
  }
}

// callback functions

function suipad_setdate (name, label) {
  var f

  f = document.forms[0]
  suipad_date = name
  f.date.value = label
}

function suipad_setview (name, id) {
  var f

  f = document.forms[0]
  suipad_viewname = name
  suipad_view = id
  f.view.value = name
}

function suipad_alarm (rearm) {
  var s

  s = suipad_packm () + '&cmd=reload'
  suipad_reswindow.location = (
    suipad_whost + suipad_wcgi + '/' + suipad_instance +
    '_mserverhelper.cgi?' + s
  )
  if (rearm > 0)
      suipad_settimer ()
}
