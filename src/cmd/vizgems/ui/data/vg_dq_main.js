//
// init
//

var dq_data_tznames = [
  { val:"default",          txt:"Default Time Zone"       },
  { val:"AKST9AKDT",        txt:"Alaskan Time"            },
  { val:"AST4ADT",          txt:"Atlantic Time"           },
  { val:"ACST-9:30ACDT",    txt:"Australian Central Time" },
  { val:"AEST-10AEDT",      txt:"Australian Eastern Time" },
  { val:"AWST-8",           txt:"Australian Western Time" },
  { val:"BT-3",             txt:"Baghdad Time"            },
  { val:"BST-1",            txt:"British Summer Time"     },
  { val:"CET-1CEST",        txt:"Central European Time"   },
  { val:"CST6CDT",          txt:"Central Time"            },
  { val:"CHADT-13:45CHAST", txt:"Chatham Islands Time"    },
  { val:"CCT-8",            txt:"China Coast Time"        },
  { val:"UTC",              txt:"Universal Time (UTC)"    },
  { val:"EET-2EEST",        txt:"Eastern European Time"   },
  { val:"EST5EDT",          txt:"Eastern Time"            },
  { val:"GMT",              txt:"Greenwich Mean Time"     },
  { val:"GST-10",           txt:"Guam Time"               },
  { val:"HST10",            txt:"Hawaiian Time"           },
  { val:"IST-5:30",         txt:"India Time"              },
  { val:"IDLE-12",          txt:"Date Line East"          },
  { val:"IDLW12",           txt:"Date Line West"          },
  { val:"IST-1",            txt:"Irish Summer Time"       },
  { val:"JST-9",            txt:"Japan Time"              },
  { val:"MET-1",            txt:"Middle European Time"    },
  { val:"MSK-3MSD",         txt:"Moscow Time"             },
  { val:"MST7MDT",          txt:"Mountain Time"           },
  { val:"NZST-12NZDT",      txt:"New Zealand Time"        },
  { val:"NST3:30NDT",       txt:"Newfoundland Time"       },
  { val:"PST8PDT",          txt:"Pacific Time"            },
  { val:"SWT0",             txt:"Swedish Winter Time"     },
  { val:"UT",               txt:"Universal Time"          },
  { val:"WAT1",             txt:"West African Time"       },
  { val:"WET0WEST",         txt:"Western European Time"   }
]

function dq_init () {
  vg_mousepopupshowfunc = dq_mdivshow
  vg_mousepopuphidefunc = dq_mdivhide
  dq_settimer (dq_updaterate)
}


//
// timer
//

var dq_timer = null
var dq_timerrate = -1
function dq_settimer (newtimerrate) {
  if (newtimerrate == dq_timerrate)
    return

  if (dq_timer != null) {
    clearTimeout (dq_timer)
    dq_timer = null
  }

  dq_timerrate = newtimerrate

  if (dq_timerrate == -1) {
    return
  }

  if (dq_timerrate == 0) {
    dq_alarm ()
    return  // NOT REACHED
  }

  dq_timer = setTimeout ('dq_alarm ()', dq_timerrate * 60000)
}
function dq_alarm () {
  if ((dq_mdivisup == true || vg_bdivisup == true) && dq_timer != null) {
    // popup or dropdown menu is up - postpone timer
    dq_timer = setTimeout ('dq_alarm ()', dq_timerrate * 60000)
    return
  }

  location.reload ()
}


//
// sdiv
//
// dq_sdivset controls the display of the query results
// can be: min, med, max

var dq_sdivs = new Array ()

function dq_sdivset (id, mode) {
  var el, w, h

  if (!(el = document.getElementById (id + '_data')))
    return

  if (mode == 'min') {
    el.style.display = 'none'
  } else if (mode == 'med') {
    w = '800'
    h = '200'
    if (typeof dq_sdivs[id] != 'undefined') {
      w = dq_sdivs[id].w
      h = dq_sdivs[id].h
    }
    el.style.display = 'block'
    el.style.width = w
    el.style.height = h
    el.style.overflow = 'scroll'
  } else {
    el.style.display = 'block'
    el.style.width = 'auto'
    el.style.height = 'auto'
  }
  dq_modvarstate (id + '_divmode', mode)
}

var dq_sdivpage_files = new Array ()
var dq_sdivpage_inprogress = false
var dq_sdivpage_inprogresssdpf = null

function dq_sdivpage_set (file, tool, pagen, pagei, allpages) {
  var del, sel, k

  sel = document.getElementById ('sdiv.' + file + '.select')
  if (sel)
    if (allpages)
      sel.selectedIndex = 0
    else
      sel.selectedIndex = pagei
  del = document.getElementById ('sdiv.' + file + '.data')
  dq_sdivpage_files[file] = {
    file:file, tool:tool,
    pagen:pagen, pagei:pagei, allpages:allpages,
    sel:sel, del:del
  }
  k = file.replace (tool, '').replace (/\.[^.]*$/, '').replace ('.', '')
  k = tool + '_pageindex' + k
  if (allpages)
    dq_setvarstate (k, 'all')
  else
    dq_setvarstate (k, '' + pagei)
}
function dq_sdivpage_goto (file, tool, op) {
  var sdpf = dq_sdivpage_files[file]
  var page, pagei, off, k

  if (!sdpf)
     return

  if (op == 'all') {
    page = 'all'
    sdpf.allpages = true
    sdpf.pagei = 1
    sdpf.sel.selectedIndex = 0
  } else if (op == 'select') {
    page = sdpf.sel.options[sdpf.sel.selectedIndex].value
    if (sdpf.sel.selectedIndex == 0) {
      sdpf.allpages = true
      sdpf.pagei = 1
    } else {
      sdpf.allpages = false
      sdpf.pagei = parseInt (page)
    }
  } else if (op == 'fwd' || op == 'bwd') {
    if (sdpf.allpages) {
      page = sdpf.sel.options[1].value
      sdpf.sel.selectedIndex = 1
      sdpf.allpages = false
      sdpf.pagei = 1
    } else {
      if (op == 'fwd')
        off = 1
      else
        off = -1
      pagei = sdpf.pagei + off
      if (pagei < 1 || pagei > sdpf.pagen)
        return

      page = pagei + ''
      sdpf.pagei = pagei
      sdpf.sel.selectedIndex += off
    }
  }
  k = file.replace (tool, '').replace (/\.[^.]*$/, '').replace ('.', '')
  k = tool + '_pageindex' + k
  if (sdpf.allpages)
    dq_modvarstate (k, 'all')
  else
    dq_modvarstate (k, '' + sdpf.pagei)

  dq_sdivpage_send (file, tool, page)
}
function dq_sdivpage_send (file, tool, page) {
  var sdpf = dq_sdivpage_files[file]
  var s

  if (!sdpf)
     return

  s = vg_cgidir + '/vg_dq_datahelper.cgi'
  s += '?name=' + dq_mainname + '&secret=' + dq_secret
  s += '&tool=' + tool + '&file=' + file + '&page=' + page

  dq_sdivpage_inprogress = true
  dq_sdivpage_inprogresssdpf = sdpf
  vg_xmlquery (s, dq_sdivpage_receive)
}
function dq_sdivpage_receive (req) {
  var sdpf = dq_sdivpage_inprogresssdpf
  var res

  if (!sdpf)
     return

  res = req.responseText
  sdpf.del.innerHTML = unescape (res)
  dq_sdivpage_inprogress = false
}


//
// mdiv
//

var dq_mdivel = null
var dq_mdivisup = false
var dq_mdivorig = { x:0, y:0 }
var dq_mdivsize = { x:200, y:200 }
var dq_mdivpoff = { x:0, y:0 }
var dq_mdivsoff = { x:0, y:0 }
var dq_mdivssize = { x:0, y:0 }
var dq_mdivanimarray = new Array (10)
var dq_mdivanimstepn = 5
var dq_mdivanimstepi = 0

function dq_mdivshow (u, e, aflag) {
  var i, px, py, el, nel, t

  if (dq_mdivisup) {
    dq_mdivhide ()
    return
  }
  dq_mdivisup = true
  vg_mousepopupisup = dq_mdivisup

  if (dq_mdivel == null) {
    dq_mdivel = document.getElementById ('mdiv')
    vg_mousepopupel = dq_mdivel
  }
  if (vg_browser == 'IE') {
    dq_mdivpoff.x = e.clientX + document.body.scrollLeft
    dq_mdivpoff.y = e.clientY + document.body.scrollTop
    dq_mdivsoff.x = e.clientX
    dq_mdivsoff.y = e.clientY
    dq_mdivssize.x = document.body.clientWidth
    dq_mdivssize.y = document.body.clientHeight
  } else if (vg_browser == 'BB') {
    dq_mdivpoff.x = e.clientX
    dq_mdivpoff.y = e.clientY
    dq_mdivsoff.x = e.clientX
    dq_mdivsoff.y = e.clientY
    dq_mdivssize.x = document.body.clientWidth
    dq_mdivssize.y = document.body.clientHeight
  } else if (vg_browser == 'AD') {
    px = e.pageX
    py = e.pageY
    if (px == 0 && py == 0 && typeof e.target == "object") {
      for (el = e.target; el; el = nel) {
        if (
          typeof el.offsetLeft == "number" && typeof el.offsetTop == "number"
        ) {
          px += el.offsetLeft
          py += el.offsetTop
        }
        if (typeof el.coords != "undefined") {
          t = el.coords.split (',')
          px += (parseInt (t[0]) + parseInt (t[2])) / 2
          py += (parseInt (t[1]) + parseInt (t[3])) / 2
        }
        if (el.offsetParent)
          nel = el.offsetParent
        else
          nel = el.parentNode
      }
    }
    dq_mdivpoff.x = px
    dq_mdivpoff.y = py
    dq_mdivsoff.x = px - window.pageXOffset
    dq_mdivsoff.y = py - window.pageYOffset
    dq_mdivssize.x = window.innerWidth
    dq_mdivssize.y = window.innerHeight
  } else {
    dq_mdivpoff.x = e.pageX
    dq_mdivpoff.y = e.pageY
    dq_mdivsoff.x = e.pageX - window.pageXOffset
    dq_mdivsoff.y = e.pageY - window.pageYOffset
    dq_mdivssize.x = window.innerWidth
    dq_mdivssize.y = window.innerHeight
  }

  dq_mdivel.style.overflow = 'visible'
  dq_mdivel.style.width = 'auto'
  dq_mdivel.style.height = 'auto'

  dq_mdivel.innerHTML = dq_genmenu (u)
  dq_activatemenu ()

  dq_mdivsize.x = dq_mdivel.offsetWidth
  dq_mdivsize.y = dq_mdivel.offsetHeight

  if (aflag) {
    dq_mdivel.style.overflow = 'hidden'
    for (i = 0.0; i < dq_mdivanimstepn; i++)
      dq_mdivanimarray[i] = (i / (dq_mdivanimstepn + 0.0))
    dq_mdivanimstepi = 0
    dq_mdivplace (0.1)
    if (typeof dq_mdivtid != "undefined")
      clearTimeout (dq_mdivtid)
    dq_mdivtid = setTimeout ('dq_mdivanim ()', 20)
  } else
    dq_mdivplace (1.0)

  dq_mdivel.style.visibility = 'visible'
}

function dq_mdivhide () {
  dq_mdivisup = false
  vg_mousepopupisup = dq_mdivisup
  dq_mdivel.style.visibility = 'hidden'
  dq_mdivel.innerHTML = ''
}

function dq_mdivanim () {
  dq_mdivanimstepi++
  if (dq_mdivanimstepi == dq_mdivanimstepn) {
    dq_mdivel.style.overflow = 'visible'
    dq_mdivplace (1.0)
    if (typeof dq_mdivtid != "undefined")
      clearTimeout (dq_mdivtid)
  dq_mdivel.style.width = 'auto'
  dq_mdivel.style.height = 'auto'
  } else {
    dq_mdivplace (dq_mdivanimarray[dq_mdivanimstepi])
    dq_mdivtid = setTimeout ('dq_mdivanim ()', 20)
  }
}

function dq_mdivplace (f) {
  var w, h

  w = f * dq_mdivsize.x
  h = f * dq_mdivsize.y
  dq_mdivorig.x = dq_mdivsoff.x - w / 2
  dq_mdivorig.y = dq_mdivsoff.y - 20

  if (dq_mdivorig.x < 0)
    dq_mdivorig.x = 0
  else if (dq_mdivorig.x + w > dq_mdivssize.x)
    dq_mdivorig.x = dq_mdivssize.x - w
  if (dq_mdivorig.y < 0)
    dq_mdivorig.y = 0
  else if (dq_mdivorig.y + h > dq_mdivssize.y)
    dq_mdivorig.y = dq_mdivssize.y - h

  dq_mdivorig.x += (dq_mdivpoff.x - dq_mdivsoff.x)
  dq_mdivorig.y += (dq_mdivpoff.y - dq_mdivsoff.y)

  dq_mdivel.style.left = dq_mdivorig.x
  dq_mdivel.style.top = dq_mdivorig.y
  dq_mdivel.style.width = w
  dq_mdivel.style.height = h
}


// menus

var dq_menuentries, dq_submenuentries

function dq_genmenu (u) {
  var t, s, ms, mn, mi

  dq_menuentries = new Array ()
  dq_submenuentries = new Array ()

  if (u == null)
    dq_currurl = dq_mainurl
  else
    dq_currurl = u
  dq_currparams = null
  dq_splitcurrurl ()

  t = 'page'
  if (u != null && typeof dq_currparams.main['kind'] != 'undefined')
    t = dq_currparams.main['kind']

  if (t == 'node') {
    ms = dq_nodemenus
    mn = dq_nodemenun
  } else if (t == 'edge') {
    ms = dq_edgemenus
    mn = dq_edgemenun
  } else if (t == 'chart') {
    ms = dq_chartmenus
    mn = dq_chartmenun
  } else if (t == 'achart') {
    ms = dq_achartmenus
    mn = dq_achartmenun
  } else if (t == 'alarm') {
    ms = dq_alarmmenus
    mn = dq_alarmmenun
  } else {
    ms = dq_pagemenus
    mn = dq_pagemenun
  }

  s = ''
  for (mi = 0; mi < mn; mi++) {
    s += dq_genmenuentries[ms[mi].type] (ms[mi])
    dq_menuentries[ms[mi].key] = {
      type:ms[mi].type,
      key:ms[mi].key,
      label:ms[mi].label,
      help:ms[mi].help,
      id:'mdiv_' + ms[mi].key,
      el:null
    }
    dq_submenuentries[ms[mi].key] = {
      key:ms[mi].key,
      id:'mdiv_' + ms[mi].key,
      el:null
    }
  }

  return s
}

var dq_activatemenucb

function dq_activatemenu () {
  var mid

  for (mid in dq_menuentries) {
    dq_menuentries[mid].idel = document.getElementById (
      dq_menuentries[mid].id
    )
    dq_submenuentries[mid].idel = dq_menuentries[mid].idel
  }
  if (typeof dq_activatemenucb != "undefined")
    dq_activatemenucb ()
}

function dq_togglemenuentry (mid, flag) {
  var el

  el = document.getElementById ('mdiv_' + mid + '_data')
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
    el.style.display = 'none'
  } else {
    el.innerHTML = dq_gensubmenu (mid)
    dq_activatesubmenu (mid)
    el.style.visibility = 'visible'
    el.style.display = 'block'
  }
  el.style.width = 'auto'
  el.style.height = 'auto'

  return false
}

function dq_gensubmenu (mid) {
  return dq_gensubmenuentries[mid] ()
}

function dq_activatesubmenu (mid) {
  dq_activatesubmenuentries[mid] ()
}

var dq_nodemenus = new Array (), dq_nodemenun = 0
var dq_edgemenus = new Array (), dq_edgemenun = 0
var dq_pagemenus = new Array (), dq_pagemenun = 0
var dq_chartmenus = new Array (), dq_chartmenun = 0
var dq_alarmmenus = new Array (), dq_alarmmenun = 0

var dq_genmenuentries = new Array ()
dq_genmenuentries['dq'] = function (m) {
  var s, qs, qid, i

  qs = ''
  if (typeof dq_currparams.main['qid'] != undefined) {
    qid = dq_currparams.main['qid']
    for (i = 0; i < dq_queries.length; i++) {
      if (dq_queries[i].q == qid) {
        qs = '&nbsp;(show&nbsp;' + dq_queries[i].n.replace (/ /, '&nbsp;') + ')'
        break
      }
    }
  }

  s = '<div class=mdiv id="mdiv_' + m.key + '">'
  s += '<a'
  s += ' class=mdivgo id="mdiv_' + m.key + '_data"'
  s += ' href="javascript:dq_runcurrurl(null, null)" title="' + m.help + '"'
  s += '>' + m.label.replace (/ /, '&nbsp;') + qs + '</a>'
  s += '</div>'

  return s
}
dq_genmenuentries['list'] = function (m) {
  var s

  s =  '<div class=mdiv id="mdiv_' + m.key + '">'
  s += '<a'
  s += ' class=mdiv href="javascript:void(0)"'
  s += ' onClick="return dq_togglemenuentry(\'' + m.key + '\', null)"'
  s += ' title="' + m.help + '"'
  s += '>&gt;&nbsp;' + m.label.replace (/ /, '&nbsp;') + '</a>'
  s += '</div>'
  s += '<div class=mdiv id="mdiv_'
  s += m.key
  s += '_data" style="visibility:hidden">'
  s += '</div>'

  return s
}
dq_genmenuentries['aq'] = function (m) {
  var s, ccid, aid, args

  if (typeof dq_currparams.main['clear_ccid'] != "undefined")
    ccid = dq_currparams.main['clear_ccid']
  else
    ccid = ''
  if (typeof dq_currparams.main['clear_aid'] != "undefined")
    aid = dq_currparams.main['clear_aid']
  else
    aid = ''
  if (
    (m.key == 'clearone') || (m.key == 'clearobj') ||
    (m.key == 'clearccid' && ccid != '') ||
    (m.key == 'clearaid' && aid != '')
  ) {
    s = '<div class=mdiv id="mdiv_' + m.key + '">'
    s += '<a'
    s += ' class=mdivgo id="mdiv_' + m.key + '_data"'
    args = '\'' + m.key + '\', \'' + m.label + '\''
    s += ' href="javascript:dq_aq_runclear(' + args + ')"'
    s += ' title="' + m.help + '"'
    s += '>' + m.label.replace (/ /, '&nbsp;') + '</a>'
    s += '</div>'
  } else
    s = ''

  return s
}
dq_genmenuentries['sq'] = function (m) {
  var kind, s, zoom

  if (typeof dq_currparams.main['kind'] != "undefined")
    kind = dq_currparams.main['kind']
  else
    kind = 'chart'
  if (typeof dq_currparams.main['statchartzoom'] != "undefined")
    zoom = dq_currparams.main['statchartzoom']
  else
    zoom = ''
  if (
    (m.key == 'zoomin' && zoom != '1') ||
    (m.key == 'zoomout' && zoom == '1')
  ) {
    if (m.key == 'zoomin')
      zoom = '1'
    else if (m.key == 'zoomout')
      zoom = '0'
    s = '<div class=mdiv id="mdiv_' + m.key + '">'
    s += '<a'
    s += ' class=mdivgo id="mdiv_' + m.key + '_data"'
    s += ' href="javascript:dq_sq_setzoommode('
    s += '\'' + kind + '\', \'' + zoom + '\''
    s += ')"'
    s += ' title="' + m.help + '"'
    s += '>' + m.label.replace (/ /, '&nbsp;') + '</a>'
    s += '</div>'
  } else
    s = ''

  return s
}

var dq_gensubmenuentries = new Array ()
var dq_activatesubmenuentries = new Array ()

dq_gensubmenuentries['otherqueries'] = function () {
  var s, i, j, n, m, lv

  n = 0
  s = '<div class=mdivmenu>'
  for (i = 0; i < dq_queries.length; i++) {
    m = 0
    for (j = 0; j < dq_queries[i].ls.length; j++) {
      lv = dq_queries[i].ls[j]
      if (
        (lv == 'EMPTY') ||
        (lv == 'ANY' && dq_currparams.inv.length > 0) ||
        (
          lv != '' &&
          typeof dq_currparams.inv['level_' + lv] != 'undefined' &&
          dq_currparams.inv['level_' + lv] != ''
        )
      ) {
        m++
        break
      }
    }
    if (m == 0)
      continue

    s += '<a'
    s += ' class=mdivgo href="javascript:dq_runcurrurl(null, {'
    s += ' \'main\':{ \'qid\':\'' + dq_queries[i].q + '\'}'
    s += ' })"'
    s += ' title="' + dq_queries[i].d + '"'
    s += '>show&nbsp;' + dq_queries[i].n.replace (/ /, '&nbsp;')
    s += '</a>'
  }
  s += '</div>'
  return s
}
dq_activatesubmenuentries['otherqueries'] = function () {
  return
}

dq_gensubmenuentries['actions'] = function () {
  var s

  s = '<div class=mdivmenu id=action_list></div>'
  return s
}
dq_activatesubmenuentries['actions'] = function () {
  var sme = dq_submenuentries['actions']

  sme.el = document.getElementById ('action_list')
  dq_action_send ()
}

dq_gensubmenuentries['setdateparam'] = function () {
  var sme = dq_submenuentries['setdateparam']
  var s, i, hh, tznamei

  s = '<table class=mdivform><tr class=mdivform>'
  s += '  <td class=mdivform colspan=2>&nbsp;<a'
  s += '    class=mdivilset href="javascript:void(0)"'
  s += '    onClick="dq_usedateparam()"'
  s += '    title="use current selection"'
  s += '  >&nbsp;OK&nbsp;</a>&nbsp;<a'
  s += '    class=mdivilset href="javascript:void(0)"'
  s += '    onClick="dq_resetdateparam()"'
  s += '    title="revert to original selection"'
  s += '  >&nbsp;Reset&nbsp;</a>&nbsp;<a'
  s += '    class=mdivilset href="javascript:void(0)"'
  s += '    onClick="dq_cleardateparam()"'
  s += '    title="clear all selection items"'
  s += '  >&nbsp;Clear&nbsp;</a>&nbsp;'
  if (vg_showhelpbuttons == '1') {
    s += '  <a'
    s += '    class=mdivilset href="javascript:void(0)"'
    s += '    onClick="vg_showhelp(\'quicktips/dateform\', false)"'
    s += '    title="click to read about using this form"'
    s += '  >&nbsp;?&nbsp;</a>'
  }
  s += '  </td>'
  s += '</tr><tr class=mdivform>'
  s += '  <td class=mdivform>'
  s += '    <a'
  s += '      class=mdivlabel href="javascript:void(0)"'
  s += '      title="common date ranges"'
  s += '    >'
  s += '      &nbsp;Quick Links&nbsp;'
  s += '    </a>'
  s += '  </td><td class=mdivform>'
  s += '    <select'
  s += '      class=mdiv id=setdate_qlink title="common date ranges"'
  s += '      onChange="dq_date_work_changeqlink(event)"'
  s += '    >'
  s += '      <option class=mdiv value="">select...</option>'
  for (i = 0; i < dq_date_qlinks.length; i++) {
    s += '      <option class=mdiv value=' + dq_date_qlinks[i].v + '>'
    s += dq_date_qlinks[i].l
    s += '      </option>'
  }
  s += '    </select>'
  s += '  </td>'
  s += '</tr><tr class=mdivform>'
  s += '  <td class=mdivform>'
  s += '    <a'
  s += '      class=mdivlabel href="javascript:void(0)"'
  s += '      title="select days of the week to show"'
  s += '    >'
  s += '      &nbsp;Filter&nbsp;'
  s += '    </a>'
  s += '  </td><td class=mdivform>'
  for (dow = 1; dow < dq_date_downames.length; dow++) {
    s += dq_date_downames[dow] + '<input'
    s += ' type="checkbox" id=setdate_dow' + dow + ' name="setdate_dow' + dow
    s += '" value="' + dq_date_dowid2name[dow]
    s += '" title="select which days of the week to show"'
    s += '>'
  }
  s += '  </td>'
  s += '</tr><tr class=mdivform>'
  s += '  <td class=mdivform style="vertical-align:bottom">'
  s += '    <a'
  s += '      class=mdivlabel href="javascript:void(0)"'
  s += '      title="type in a date or use quick links"'
  s += '    >'
  s += '      &nbsp;Start Date&nbsp;'
  s += '    </a>'
  s += '  </td><td class=mdivform>'
  s += '    <div class=mdivform>'
  s += '      <div class=mdivform style="min-height:6em" id=setdate_date1>'
  s += '      </div>'
  s += '      <div class=mdivform>&nbsp;Hours&nbsp;<select'
  s += '        class=mdiv id=setdate_time1 title="hours"'
  s += '        onChange="dq_date_work_changetime(event, 1)"'
  s += '      >'
  s += '        <option class=mdiv value="">select...</option>'
  for (hh = 0; hh < dq_date_hrnames.length - 1; hh++) {
    s += '        <option class=mdiv value="' + dq_date_hrnames[hh] + ':00">'
    s += dq_date_hrnames[hh] + ':00'
    s += '        </option>'
  }
  s += '      </select></div>'
  s += '      <div class=mdivform><input'
  s += '        class=mdiv type=text id=setdate_text1'
  s += '        name=setdate_text1'
  s += '        onKeyUp="dq_date_work_changetext(event, 1)" size=20'
  s += '        title="YYYY/MM/DD-hh:mm or use quick links for special dates"'
  s += '      ></div>'
  s += '    </div>'
  s += '  </td>'
  s += '</tr><tr class=mdivform>'
  s += '  <td class=mdivform style="vertical-align:top">'
  s += '    <a'
  s += '      class=mdivlabel href="javascript:void(0)"'
  s += '      title="type in a date or use quick links"'
  s += '    >'
  s += '      &nbsp;End Date&nbsp;'
  s += '    </a>'
  s += '  </td><td class=mdivform>'
  s += '    <div class=mdivform>'
  s += '      <div class=mdivform><input'
  s += '        class=mdiv type=text id=setdate_text2'
  s += '        name=setdate_text2'
  s += '        onKeyUp="dq_date_work_changetext(event, 2)" size=20'
  s += '        title="YYYY/MM/DD-hh:mm or use quick links for special dates"'
  s += '      ></div>'
  s += '      <div class=mdivform>&nbsp;Hours&nbsp;<select'
  s += '        class=mdiv id=setdate_time2 title="hours"'
  s += '        onChange="dq_date_work_changetime(event, 2)"'
  s += '      >'
  s += '        <option class=mdiv value="">select...</option>'
  for (hh = 1; hh < dq_date_hrnames.length; hh++) {
    s += '        <option class=mdiv value="' + dq_date_hrnames[hh] + ':00">'
    s += dq_date_hrnames[hh] + ':00'
    s += '        </option>'
  }
  s += '      </select></div>'
  s += '      <div class=mdivform style="min-height:6em" id=setdate_date2>'
  s += '      </div>'
  s += '    </div>'
  s += '  </td>'
  s += '</tr><tr class=mdivform>'
  s += '  <td class=mdivform>'
  s += '    <a'
  s += '      class=mdivlabel href="javascript:void(0)"'
  s += '      title="select a time zone"'
  s += '    >'
  s += '      &nbsp;Time Zone&nbsp;'
  s += '    </a>'
  s += '  </td><td class=mdivform>'
  s += '    <div class=mdivform>'
  s += '      <div class=mdivform><select'
  s += '        class=mdiv id=setdate_tz name=setdate_tz'
  s += '      >'
  for (tznamei = 0; tznamei < dq_data_tznames.length; tznamei++) {
    s += '        <option'
    s += '          class=mdiv value="' + dq_data_tznames[tznamei].val + '"'
    s += '        >'
    s += '          ' + dq_data_tznames[tznamei].txt
    s += '        </option>'
  }
  s += '      </select></div>'
  s += '      </div>'
  s += '    </div>'
  s += '  </td>'
  s += '</tr></table>'

  return s
}
dq_activatesubmenuentries['setdateparam'] = function () {
  var sme = dq_submenuentries['setdateparam']
  var s, tznamei

  sme.qlink = document.getElementById ('setdate_qlink')
  sme.texts = new Array ()
  sme.texts[1] = document.getElementById ('setdate_text1')
  sme.texts[2] = document.getElementById ('setdate_text2')
  sme.dates = new Array ()
  sme.dates[1] = document.getElementById ('setdate_date1')
  sme.dates[2] = document.getElementById ('setdate_date2')
  sme.times = new Array ()
  sme.times[1] = document.getElementById ('setdate_time1')
  sme.times[2] = document.getElementById ('setdate_time2')
  sme.dows = new Array ()
  for (dow = 1; dow < 8; dow++)
    sme.dows[dow] = document.getElementById ('setdate_dow' + dow)
  if (
    typeof dq_currparams.date['datefilter'] != 'undefined' &&
    dq_currparams.date['datefilter'] != ''
  )
    s = dq_currparams.date['datefilter']
  else
    s = ''
  dq_date_work_showdow (s)
  if (
    typeof dq_currparams.date['fdate'] != 'undefined' &&
    dq_currparams.date['fdate'] != ''
  )
    s = dq_currparams.date['fdate']
  else
    s = 'latest'
  dq_date_work_showdate (s, 1)
  dq_date_work_showtext (s, 1)
  if (
    typeof dq_currparams.date['ldate'] != 'undefined' &&
    dq_currparams.date['ldate'] != ''
  )
    s = dq_currparams.date['ldate']
  else
    s = ''
  dq_date_work_showdate (s, 2)
  dq_date_work_showtext (s, 2)
  sme.tz = document.getElementById ('setdate_tz')
  if (typeof dq_currparams.date['tzoverride'] != 'undefined') {
    for (tznamei = 0; tznamei < dq_data_tznames.length; tznamei++) {
      if (dq_data_tznames[tznamei].val == dq_currparams.date['tzoverride']) {
        sme.tz.selectedIndex = tznamei
        break
      }
    }
  }
}

dq_gensubmenuentries['setinvparam'] = function () {
  var sme = dq_submenuentries['setinvparam']
  var s, i

  s = '<table class=mdivform><tr class=mdivform>'
  s += '  <td class=mdivform colspan=2>&nbsp;<a'
  s += '    class=mdivilset href="javascript:void(0)"'
  s += '    onClick="dq_useinvparam()"'
  s += '    title="use current selection"'
  s += '  >OK</a>&nbsp;<a'
  s += '    href="javascript:void(0)" onClick="dq_resetinvmparam()"'
  s += '    class=mdivilset title="revert to original selection"'
  s += '  >Reset</a>&nbsp;<a'
  s += '    href="javascript:void(0)" onClick="dq_clearinvmparam()"'
  s += '    class=mdivilset title="clear all selection items"'
  s += '  >Clear</a>&nbsp;'
  if (vg_showhelpbuttons == '1') {
    s += '  <a'
    s += '    class=mdivilset href="javascript:void(0)"'
    s += '    onClick="vg_showhelp(\'quicktips/invform\', false)"'
    s += '    title="click to read about using this form"'
    s += '  >&nbsp;?&nbsp;</a>'
  }
  s += '  </td>'
  s += '</tr><tr class=mdivform>'
  s += '  <td class=mdivform><a'
  s += '    class=mdivlabel href="javascript:void(0)"'
  s += '    title="current selection"'
  s += '  >&nbsp;Selection&nbsp;</a></td><td class=mdivform>'
  s += '    <div class=mdivcurr id=setinvparam_curr></div>'
  s += '  </td>'
  s += '</tr><tr class=mdivform>'
  s += '  <td class=mdivform><a'
  s += '    class=mdivlabel href="javascript:void(0)"'
  s += '    title="work list"'
  s += '  >&nbsp;Matches&nbsp;</a></td><td class=mdivform>'
  s += '    <div class=mdivwork id=setinvparam_work></div>'
  s += '  </td>'
  s += '</tr><tr class=mdivform>'
  s += '  <td class=mdivform><a'
  s += '    class=mdivlabel href="javascript:void(0)"'
  s += '    title="select type of item to search for"'
  s += '  >&nbsp;Display&nbsp;</a></td><td class=mdivform>'
  s += '    <select class=mdiv id=setinvparam_level title="pick a level">'
  for (i = 0; i < dq_levels.length; i++) {
    s += '      <option class=mdiv ' + dq_levels[i].sf
    s += ' value=' + dq_levels[i].lv + '>' + dq_levels[i].pn
  }
  s += '    </select>'
  s += '  </td>'
  s += '</tr><tr class=mdivform>'
  s += '  <td class=mdivform><a'
  s += '    class=mdivlabel href="javascript:void(0)"'
  s += '    title="search for an item"'
  s += '  >&nbsp;Search&nbsp;</a></td><td class=mdivform>'
  s += '    <a'
  s += '      class=mdivilpick id=setinvparam_search name=setinvparam_search'
  s += '      href="javascript:void(0)"'
  s += '      onClick="dq_invparam_work_search(event)"'
  s += '      title="search and display matches"'
  s += '    >Go</a>'
  s += '    <input'
  s += '      class=mdiv type=text id=setinvparam_text name=setinvparam_text'
  s += '      onKeyUp="dq_invparam_work_search(event)" size=15'
  s += '      title="type a complete or partial asset name"'
  s += '    >'
  s += '    <a'
  s += '      class=mdivilpick id=setinvparam_search name=setinvparam_search'
  s += '      href="javascript:void(0)"'
  s += '      onClick="dq_invparam_work_prevsearch()"'
  s += '      title="show previous search parameters"'
  s += '    >Prev</a>'
  s += '  </td>'
  s += '</tr><tr class=mdivform>'
  s += '  <td class=mdivform><a'
  s += '    class=mdivlabel href="javascript:void(0)"'
  s += '    title="optional inventory filter"'
  s += '  >&nbsp;Filter&nbsp;</a></td><td class=mdivform>'
  s += '    <select class=mdiv id=setinvparam_filter title="pick filter">'
  s += '      <option class=mdiv value="none">none</option>'
  s += '      <option'
  s += '        class=mdiv value="onalarms"'
  s += '      >assets with alarms only</option>'
  s += '    </select>'
  s += '  </td>'
  s += '</tr></table>'

  return s
}
dq_activatesubmenuentries['setinvparam'] = function () {
  var sme = dq_submenuentries['setinvparam']

  sme.cel = document.getElementById ('setinvparam_curr')
  sme.wel = document.getElementById ('setinvparam_work')
  sme.tel = document.getElementById ('setinvparam_text')
  sme.lel = document.getElementById ('setinvparam_level')
  sme.level = sme.lel.options[sme.lel.selectedIndex].value
  sme.text = sme.tel.value
  sme.fel = document.getElementById ('setinvparam_filter')
  dq_invparam_curr_send ()
}

dq_gensubmenuentries['setstatparam'] = function () {
  var sme = dq_submenuentries['setstatparam']
  var s

  s = '<table class=mdivform><tr class=mdivform>'
  s += '  <td class=mdivform colspan=2>&nbsp;<a'
  s += '    class=mdivilset href="javascript:void(0)"'
  s += '    onClick="dq_usestatparam()"'
  s += '    title="use current selection"'
  s += '  >OK</a>&nbsp;<a'
  s += '    href="javascript:void(0)" onClick="dq_resetstatmparam()"'
  s += '    class=mdivilset title="revert to original selection"'
  s += '  >Reset</a>&nbsp;<a'
  s += '    href="javascript:void(0)" onClick="dq_clearstatmparam()"'
  s += '    class=mdivilset title="clear all selection items"'
  s += '  >Clear</a>&nbsp;'
  if (vg_showhelpbuttons == '1') {
    s += '  <a'
    s += '    class=mdivilset href="javascript:void(0)"'
    s += '    onClick="vg_showhelp(\'quicktips/statform\', false)"'
    s += '    title="click to read about using this form"'
    s += '  >&nbsp;?&nbsp;</a>'
  }
  s += '  </td>'
  s += '</tr><tr class=mdivform>'
  s += '  <td class=mdivform><a'
  s += '    class=mdivlabel href="javascript:void(0)"'
  s += '    title="current selection"'
  s += '  >&nbsp;Selection&nbsp;</a></td><td class=mdivform>'
  s += '    <div class=mdivcurr id=setstatparam_curr></div>'
  s += '  </td>'
  s += '</tr><tr class=mdivform>'
  s += '  <td class=mdivform><a'
  s += '    class=mdivlabel href="javascript:void(0)"'
  s += '    title="work list"'
  s += '  >&nbsp;Matches&nbsp;</a></td><td class=mdivform>'
  s += '    <div class=mdivwork id=setstatparam_work></div>'
  s += '  </td>'
  s += '</tr><tr class=mdivform>'
  s += '  <td class=mdivform><a'
  s += '    class=mdivlabel href="javascript:void(0)"'
  s += '    title="search for a statistic"'
  s += '  >&nbsp;Search&nbsp;</a></td><td class=mdivform>'
  s += '    <a'
  s += '      class=mdivilpick id=setstatparam_search name=setstatparam_search'
  s += '      href="javascript:void(0)"'
  s += '      onClick="dq_statparam_work_search(event)"'
  s += '      title="search and display matches"'
  s += '    >Go</a>'
  s += '    <input'
  s += '      class=mdiv type=text id=setstatparam_text name=setstatparam_text'
  s += '      onKeyUp="dq_statparam_work_search(event)" size=15'
  s += '      title="type a complete or partial statistic name"'
  s += '    >'
  s += '    <a'
  s += '      class=mdivilpick id=setstatparam_search name=setstatparam_search'
  s += '      href="javascript:void(0)"'
  s += '      onClick="dq_statparam_work_prevsearch()"'
  s += '      title="show previous search parameters"'
  s += '    >Prev</a>'
  s += '  </td>'
  s += '</tr><tr class=mdivform>'
  s += '  <td class=mdivform><a'
  s += '    class=mdivlabel href="javascript:void(0)"'
  s += '    title="graph contents"'
  s += '  >&nbsp;Content&nbsp;</a></td><td class=mdivform>'
  s += '    <select class=mdiv id=setstatparam_group title="pick contents">'
  s += '      <option'
  s += '        class=mdiv value="both"'
  s += '      >current &amp; profile data</option>'
  s += '      <option class=mdiv value="curr">current data only</option>'
  s += '      <option'
  s += '        class=mdiv value="proj"'
  s += '      >current &amp; projection data</option>'
  s += '    </select>'
  s += '  </td>'
  s += '</tr><tr class=mdivform>'
  s += '  <td class=mdivform><a'
  s += '    class=mdivlabel href="javascript:void(0)"'
  s += '    title="graph display"'
  s += '  >&nbsp;Display&nbsp;</a></td><td class=mdivform>'
  s += '    <select class=mdiv id=setstatparam_disp title="pick display style">'
  s += '      <option class=mdiv value="actual">actual values</option>'
  s += '      <option class=mdiv value="perc">utilization (%)</option>'
  s += '      <option class=mdiv value="nocap">no capacity lines</option>'
  s += '    </select>'
  s += '  </td>'
  s += '</tr></table>'

  return s
}
dq_activatesubmenuentries['setstatparam'] = function () {
  var sme = dq_submenuentries['setstatparam']

  sme.cel = document.getElementById ('setstatparam_curr')
  sme.wel = document.getElementById ('setstatparam_work')
  sme.tel = document.getElementById ('setstatparam_text')
  sme.text = sme.tel.value
  sme.gel = document.getElementById ('setstatparam_group')
  sme.del = document.getElementById ('setstatparam_disp')
  dq_statparam_curr_send ()
}

dq_gensubmenuentries['setalarmparam'] = function () {
  var sme = dq_submenuentries['setalarmparam']
  var s, sevi, tmodei

  s = '<table class=mdivform><tr class=mdivform>'
  s += '  <td class=mdivform colspan=2>&nbsp;<a'
  s += '    class=mdivilset href="javascript:void(0)"'
  s += '    onClick="dq_usealarmparam()"'
  s += '    title="use current selection"'
  s += '  >OK</a>&nbsp;<a'
  s += '    class=mdivilset href="javascript:void(0)"'
  s += '    onClick="dq_resetalarmparam()"'
  s += '    title="revert to original selection"'
  s += '  >Reset</a>&nbsp;<a'
  s += '    class=mdivilset href="javascript:void(0)"'
  s += '    onClick="dq_clearalarmparam()"'
  s += '    title="clear all selection items"'
  s += '  >Clear</a>&nbsp;'
  if (vg_showhelpbuttons == '1') {
    s += '  <a'
    s += '    class=mdivilset href="javascript:void(0)"'
    s += '    onClick="vg_showhelp(\'quicktips/alarmform\', false)"'
    s += '    title="click to read about using this form"'
    s += '  >&nbsp;?&nbsp;</a>'
  }
  s += '  </td>'
  s += '</tr><tr class=mdivform>'
  s += '  <td class=mdivform><a'
  s += '    class=mdivlabel href="javascript:void(0)"'
  s += '    title="current selection"'
  s += '  >&nbsp;Selection&nbsp;</a></td><td class=mdivform>'
  s += '    <div class=mdivcurr id=setalarmparam_curr></div>'
  s += '  </td>'
  s += '</tr><tr class=mdivform>'
  s += '  <td class=mdivform><a'
  s += '    class=mdivlabel href="javascript:void(0)"'
  s += '    title="correlation id"'
  s += '  >&nbsp;CCID&nbsp;</a></td><td class=mdivform>'
  s += '    <a'
  s += '      class=mdivilpick'
  s += '      href="javascript:void(0)"'
  s += '      onClick="dq_alarmparam_work_ccid(event)"'
  s += '      title="add CCID text to the selection"'
  s += '    >ADD</a>'
  s += '    <input'
  s += '      class=mdiv type=text id=setalarmparam_ccid'
  s += '      name=setalarmparam_ccid'
  s += '      onKeyUp="dq_alarmparam_work_ccid(event)" size=15'
  s += '      title="type a complete or partial CCID"'
  s += '    >'
  s += '  </td>'
  s += '</tr><tr class=mdivform>'
  s += '  <td class=mdivform><a'
  s += '    class=mdivlabel href="javascript:void(0)"'
  s += '    title="severities"'
  s += '  >&nbsp;Severity&nbsp;</a></td><td class=mdivform>'
  s += '    <div class=mdivform id=setalarmparam_sev>'
  for (sevi = 0; sevi < dq_sevs.length; sevi++) {
    s += '      <div class=mdivform><a'
    s += '        class=mdivilpick href="javascript:void(0)"'
    s += '        onClick="dq_alarmparam_work_sev(' + sevi + ')"'
    s += '        title="add this severity to selection"'
    s += '      >ADD</a>' + dq_sevs[sevi].n + '</div>'
  }
  s += '    </div>'
  s += '  </td>'
  s += '</tr><tr class=mdivform>'
  s += '  <td class=mdivform><a'
  s += '    class=mdivlabel href="javascript:void(0)"'
  s += '    title="ticket modes"'
  s += '  >&nbsp;Ticket Mode&nbsp;</a></td><td class=mdivform>'
  s += '    <div class=mdivform id=setalarmparam_tmode>'
  for (tmodei = 0; tmodei < dq_tmodes.length; tmodei++) {
    s += '      <div class=mdivform><a'
    s += '        class=mdivilpick href="javascript:void(0)"'
    s += '        onClick="dq_alarmparam_work_tmode(' + tmodei + ')"'
    s += '        title="add this ticket mode to selection"'
    s += '      >ADD</a>' + dq_tmodes[tmodei].n + '</div>'
  }
  s += '    </div>'
  s += '  </td>'
  s += '</tr><tr class=mdivform>'
  s += '  <td class=mdivform><a'
  s += '    class=mdivlabel href="javascript:void(0)"'
  s += '    title="message text"'
  s += '  >&nbsp;Message&nbsp;</a></td><td class=mdivform>'
  s += '    <a'
  s += '      class=mdivilpick href="javascript:void(0)"'
  s += '      onClick="dq_alarmparam_work_msg(event)"'
  s += '      title="add message text to the selection"'
  s += '    >ADD</a>'
  s += '    <input'
  s += '      class=mdiv type=text id=setalarmparam_msg'
  s += '      name=setalarmparam_msg'
  s += '      onKeyUp="dq_alarmparam_work_msg(event)" size=15'
  s += '      title="type a complete or partial alarm message text"'
  s += '    >'
  s += '  </td>'
  s += '</tr><tr class=mdivform>'
  s += '  <td class=mdivform><a'
  s += '    class=mdivlabel href="javascript:void(0)"'
  s += '    title="comment text"'
  s += '  >&nbsp;Comment&nbsp;</a></td><td class=mdivform>'
  s += '    <a'
  s += '      class=mdivilpick href="javascript:void(0)"'
  s += '      onClick="dq_alarmparam_work_com(event)"'
  s += '      title="add comment text to the selection"'
  s += '    >ADD</a>'
  s += '    <input'
  s += '      class=mdiv type=text id=setalarmparam_com'
  s += '      name=setalarmparam_com'
  s += '      onKeyUp="dq_alarmparam_work_com(event)" size=15'
  s += '      title="type a complete or partial alarm comment text"'
  s += '    >'
  s += '  </td>'
  s += '</tr></table>'

  return s
}
dq_activatesubmenuentries['setalarmparam'] = function () {
  var sme = dq_submenuentries['setalarmparam']

  sme.cel = document.getElementById ('setalarmparam_curr')
  sme.ccidel = document.getElementById ('setalarmparam_ccid')
  sme.msgel = document.getElementById ('setalarmparam_msg')
  sme.comel = document.getElementById ('setalarmparam_com')
  dq_alarmparam_curr_show ()
}


//
// action utilities
//

var dq_action_inprogress = false

function dq_action_send () {
  var sme = dq_submenuentries['actions']
  var s, k

  s = vg_cgidir + '/vg_dq_actionhelper.cgi'
  s += '?name=' + dq_mainname + '/actionhelper.curr'
  s += '&date=' + dq_currparams.date.ldate
  s += '&maxn=0'
  for (k in dq_currparams.inv)
    s += '&' + k + '=' + dq_currparams.inv[k]
  for (k in dq_currparams.main)
    if (k.match (/^action_/))
      s += '&' + k + '=' + dq_currparams.main[k]

  dq_action_inprogress = true
  vg_xmlquery (s, dq_action_receive)
}
function dq_action_receive (req) {
  var sme = dq_submenuentries['actions']
  var res, drs, dri, dr, dt
  var del, dtel, haveactions, sp = String.fromCharCode (160)

  haveactions = false
  res = req.responseXML
  drs = res.getElementsByTagName ('d')
  for (dri = 0; dri < drs.length; dri++) {
    if (!(dr = drs[dri]))
      break
    dt = dr.firstChild.data.split ('|')
    dt[1] = unescape (dt[1])
    del = document.createElement ('a')
    del.className = 'mdivgo'
    del.href = unescape (dt[0])
    if (dt.length < 3 || dt[2] == 'random')
        del.target = 'vg_action' + Math.random ()
    else if (dt[2] != 'same')
        del.target = dt[2]
    dtel = document.createTextNode (dt[1])
    del.appendChild (dtel)
    sme.el.appendChild (del)
    haveactions = true
  }
  if (!haveactions) {
    del = document.createElement ('a')
    del.className = 'mdiv'
    del.href = 'javascript:void(0)'
    dtel = document.createTextNode (
      'no' + sp + 'actions' + sp + 'for' + sp + 'this' + sp + 'asset'
    )
    del.appendChild (dtel)
    sme.el.appendChild (del)
  }
  dq_action_inprogress = false
}


//
// date utilities
//

var dq_date_today = new Date ()
var dq_date_thisdate = (
  dq_date_today.getFullYear () + '/' + dq_date_today.getMonth () + '/' +
  dq_date_today.getDay ()
)
var dq_date_downames = new Array ("", "M", "T", "W", "T", "F", "S", "S")
var dq_date_dowid2name = new Array (
  "", "mon", "tue", "wed", "thu", "fri", "sat", "sun"
)
var dq_date_downame2id = new Array ()
dq_date_downame2id["mon"]=1
dq_date_downame2id["tue"]=2
dq_date_downame2id["wed"]=3
dq_date_downame2id["thu"]=4
dq_date_downame2id["fri"]=5
dq_date_downame2id["sat"]=6
dq_date_downame2id["sun"]=7
var dq_date_hrnames = new Array (
  "00", "01", "02", "03", "04", "05", "06", "07", "08", "09", "10", "11",
  "12", "13", "14", "15", "16", "17", "18", "19", "20", "21", "22", "23", "24"
)
var dq_date_currobjs = new Array ()
dq_date_currobjs[1] = new Object ()
dq_date_currobjs[2] = new Object ()
var dq_date_currtextdates = new Array ()
dq_date_currtextdates[1] = ''
dq_date_currtextdates[2] = ''

function dq_date_work_changeqlink (e) {
  var sme = dq_submenuentries['setdateparam']
  var s

  if (sme.qlink.selectedIndex <= 0)
    return

  s = sme.qlink.options[sme.qlink.selectedIndex].value
  dq_date_work_showtext (s, 1)
  dq_date_work_showtext (s, 2)
  sme.qlink.selectedIndex = 0
}

function dq_date_work_changedate (str, index) {
  var sme = dq_submenuentries['setdateparam']
  var o

  o = dq_date_currobjs[index]
  dq_date_splittext (str, o)
  dq_date_showcal (o, index)
}

function dq_date_work_changetime (e, index) {
  var sme = dq_submenuentries['setdateparam']
  var s

  if (sme.times[index].selectedIndex <= 0)
    return

  s = sme.times[index].options[sme.times[index].selectedIndex].value
  dq_date_work_showtext ('-' + s, index)
  sme.times[index].selectedIndex = 0
}

function dq_date_work_changetext (e, index) {
  var sme = dq_submenuentries['setdateparam']
  var s, cs, o

  if (e.type != 'keyup')
    return

  s = sme.texts[index].value
  if (s.length < 4 || s.indexOf ('-') > -1)
    return

  o = dq_date_currobjs[index]
  dq_date_splittext (s, o)
  if (o.dtype != 'ymd')
    return

  cs = o.year
  if (o.month != '')
    cs += '/' + o.month
  else
    cs += '/01'

  if (dq_date_currtextdates[1] == cs)
    return

  dq_date_currtextdates[1] = cs
  dq_date_work_changedate (cs, index)
}

function dq_date_work_showdow (str) {
  var sme = dq_submenuentries['setdateparam']
  var o, i

  for (i in sme.dows)
    sme.dows[i].checked = false
  o = dq_date_currobjs[1]
  dq_date_splitdow (str, o)
  for (i in o.dows)
    sme.dows[i].checked = true
}

function dq_date_splitdow (str, o) {
  var t, i

  if (typeof o.dows != "undefined")
    delete o.dows
  o.dows = new Array ()
  t = str.split (';')
  for (i = 0; i < t.length; i++) {
    if (typeof dq_date_downame2id[t[i]] != "undefined")
      o.dows[dq_date_downame2id[t[i]]]=t[i]
  }
}

function dq_date_work_showdate (str, index) {
  var sme = dq_submenuentries['setdateparam']
  var o, s

  o = dq_date_currobjs[index]
  dq_date_splittext (str, o)
  dq_date_showcal (o, index)
}

function dq_date_work_showtext (str, index) {
  var sme = dq_submenuentries['setdateparam']
  var o, s, ndate, ntime

  o = dq_date_currobjs[index]

  if (str == '') {
    sme.texts[index].value = ''
    return
  }

  dq_date_splittext (str, o)
  if (o.dtype != 'ymd') {
    sme.texts[index].value = str
    return
  }

  ndate = o.date
  ntime = o.time

  s = sme.texts[index].value
  dq_date_splittext (s, o)
  if (o.dtype != 'ymd') {
    sme.texts[index].value = str
    return
  }

  s = ''
  if (ndate != '')
    s = ndate
  else if (o.date != '')
    s = o.date
  else
    s = dq_date_thisdate
  if (ntime != '')
    s += '-' + ntime
  else if (o.time != '')
    s += '-' + o.time

  sme.texts[index].value = s
}

function dq_date_splittext (str, o) {
  var sme = dq_submenuentries['setdateparam']
  var s, d, t, i

  if ((i = str.indexOf ('-')) > -1) {
    d = str.substr (0, i)
    t = str.substr (i + 1)
    d = d.replace (/ /, '')
    t = t.replace (/ /, '')
  } else if  ((i = str.indexOf (' ')) > -1) {
    d = str.substr (0, i)
    t = str.substr (i + 1)
    d = d.replace (/ /, '')
    t = t.replace (/ /, '')
  } else {
    d = str
    d = d.replace (/ /, '')
    t = ''
  }

  o.date = d
  o.time = t

  if (
    d.match (/^[0-9]+\/[0-9]+\/[0-9]+$/) ||
    d.match (/^[0-9]+\/[0-9]+$/) || d.match (/^[0-9]+$/)
  ) {
    o.dtype = 'ymd'
    o.year = ''
    o.month = ''
    o.day = ''
    dt = d.split ('/')
    if (dt.length > 0)
      o.year = dt[0]
    if (dt.length > 1)
      o.month = dt[1]
    if (dt.length > 2)
      o.day = dt[2]
    if (o.year.length == 4) {
      if (o.month.length == 1 && o.month != '0')
        o.month = '0' + o.month
      if (o.day.length == 1 && o.day != '0')
        o.day = '0' + o.day
    } else
      o.dtype = 'special'
  } else if (
    d.match (/[0-9]+\.[0-9]+\.[0-9]+/) ||
    d.match (/[0-9]+\.[0-9]+/) || d.match (/[0-9]+/)
  ) {
    o.dtype = 'ymd'
    o.year = ''
    o.month = ''
    o.day = ''
    dt = d.split ('.')
    if (dt.length > 0)
      o.year = dt[0]
    if (dt.length > 1)
      o.month = dt[1]
    if (dt.length > 2)
      o.day = dt[2]
    if (o.year.length == 4) {
      if (o.month.length == 1 && o.month != '0')
        o.month = '0' + o.month
      if (o.day.length == 1 && o.day != '0')
        o.day = '0' + o.day
    } else
      o.dtype = 'special'
  } else if (d == '') {
    o.dtype = 'ymd'
    o.year = ''
    o.month = ''
    o.day = ''
  } else
    o.dtype = 'special'

  if (t.match (/[0-9]+:[0-9]+/)) {
    o.ttype = 'hm'
    tt = t.split (':')
    o.hour = tt[0]
    o.min = tt[1]
    if (o.hour.length > 2)
      o.ttype = 'special'
    else if (o.hour.length == 1)
      o.hour = '0' + o.hour
    if (o.min.length > 2)
      o.ttype = 'special'
    else if (o.min.length == 1)
      o.min = '0' + o.min
  } else
    o.ttype = 'special'
}

function dq_date_showcal (o, index) {
  var sme = dq_submenuentries['setdateparam']
  var el, tf, tl, ti, dowf, dowl, dow, di, s, inrow, args
  var year, month, yyyy, mm, dd, pyyyy, pmm, pyyyymm, nyyyy, nmm, nyyyymm

  el = sme.dates[index]

  if (o.dtype == 'ymd' && o.year != '' && o.month != '') {
    yyyy = parseInt (o.year.replace (/^00*/, ''))
    mm = parseInt (o.month.replace (/^00*/, ''))
    year = o.year
    month = o.month
  } else {
    yyyy = dq_date_today.getFullYear ()
    mm = dq_date_today.getMonth () + 1
    year = yyyy + ''
    month = mm + ''
    if (month.length == 1)
      month = '0' + month
  }
  tf = new Date (yyyy, mm - 1, 1, 0, 0, 0, 0)
  tl = new Date (yyyy, mm + 0, 1, 0, 0, 0, 0)
  dowf = tf.getDay ()
  if (dowf == 0)
    dowf = 7
  dowl = tl.getDay ()
  if (dowl == 0)
    dowl = 7
  s = '<table class=mdivform>'
  s += '<caption class=mdivform>'
  pyyyy = yyyy - 1
  pmm = mm
  pyyyymm = pyyyy + '/' + pmm
  s += '<a'
  s += '  class=mdivilpick href="javascript:void(0)" title="previous year"'
  s += '  onClick="dq_date_work_changedate(\'' + pyyyymm + '\',' + index + ')"'
  s += '>'
  s += '&lt;&lt;'
  s += '</a>&nbsp;'
  pmm = mm - 1
  if (pmm < 1) {
    pmm = 12
    pyyyy = yyyy - 1
  } else
    pyyyy = yyyy
  pyyyymm = pyyyy + '/' + pmm
  s += '<a'
  s += '  class=mdivilpick href="javascript:void(0)" title="previous month"'
  s += '  onClick="dq_date_work_changedate(\'' + pyyyymm + '\',' + index + ')"'
  s += '>'
  s += '&nbsp;&lt;&nbsp;'
  s += '</a>'
  s += '&nbsp;' + year + '/' + month + '&nbsp;'
  nmm = mm + 1
  if (nmm > 12) {
    nmm = 1
    nyyyy = yyyy + 1
  } else
    nyyyy = yyyy
  nyyyymm = nyyyy + '/' + nmm
  s += '<a'
  s += '  class=mdivilpick href="javascript:void(0)" title="next month"'
  s += '  onClick="dq_date_work_changedate(\'' + nyyyymm + '\',' + index + ')"'
  s += '>'
  s += '&nbsp;&gt;&nbsp;'
  s += '</a>&nbsp;'
  nyyyy = yyyy + 1
  nmm = mm
  nyyyymm = nyyyy + '/' + nmm
  s += '<a'
  s += '  class=mdivilpick href="javascript:void(0)" title="next year"'
  s += '  onClick="dq_date_work_changedate(\'' + nyyyymm + '\', ' + index + ')"'
  s += '>'
  s += '&gt;&gt;'
  s += '</a>'
  s += '</caption>'
  s += '<tr class=mdivform>'
  for (dow = 1; dow < 8; dow++)
    s += '<td class=mdivformnb>' + dq_date_downames[dow] + '</td>'
  s += '</tr>'
  inrow = true
  s += '<tr class=mdivform>'
  for (dow = 1; dow < dowf; dow++)
    s += '<td class=mdivformnb></td>'
  for (di = 1; di < 33; di++) {
    ti = new Date (yyyy, mm - 1, di, 0, 0, 0, 0)
    if (ti >= tl)
      break
    if (!inrow) {
      s += '<tr>'
      inrow = true
    }
    if (di < 10)
      dd = '0' + di
    else
      dd = '' + di
    yyyymmdd = '\'' + year + '/' + month + '/' + dd + '\''
    args = yyyymmdd + ', ' + index
    s += '<td class=mdivformnb><a'
    s += '  class=mdivilpick href="javascript:void(0)" title="click to select"'
    s += '  style="display:block" onClick="dq_date_work_showtext(' + args + ')"'
    s += '>'
    s += '&nbsp;' + dd + '&nbsp;'
    s += '</a></td>'
    dow++
    if (dow == 8) {
      s += '</tr>'
      inrow = false
      dow = 1
    }
  }
  if (dowl != 1) {
    for (dow = dowl; dow < dowf; dow++) {
      s += '<td class=mdivformnb></td>'
      s += '</tr>'
    }
  }
  s += '</table>'
  el.innerHTML = s
}
function dq_usedateparam () {
  var sme = dq_submenuentries['setdateparam']
  var edits, s, dow

  edits = new Object
  edits.date = new Array ()
  s = ''
  for (dow in sme.dows) {
    if (sme.dows[dow].checked) {
      if (s != '')
        s += ';'
      s += sme.dows[dow].value
    }
  }
  edits.date['datefilter'] = s
  edits.date['fdate'] = sme.texts[1].value
  edits.date['ldate'] = sme.texts[2].value
  edits.date['tzoverride'] = sme.tz.options[sme.tz.selectedIndex].value
  dq_editcurrurl (edits)
  dq_togglemenuentry ('setdateparam', false)
  dq_togglemenuentry ('otherqueries', true)
}
function dq_resetdateparam () {
  var sme = dq_submenuentries['setdateparam']
  var s

  if (
    typeof dq_currparams.date['datefilter'] != 'undefined' &&
    dq_currparams.date['datefilter'] != ''
  )
    s = dq_currparams.date['datefilter']
  else
    s = ''
  dq_date_work_showdow (s)
  if (
    typeof dq_currparams.date['fdate'] != 'undefined' &&
    dq_currparams.date['fdate'] != ''
  )
    s = dq_currparams.date['fdate']
  else
    s = ''
  dq_date_work_showdate (s, 1)
  dq_date_work_showtext (s, 1)
  if (
    typeof dq_currparams.date['ldate'] != 'undefined' &&
    dq_currparams.date['ldate'] != ''
  )
    s = dq_currparams.date['ldate']
  else
    s = ''
  dq_date_work_showdate (s, 2)
  dq_date_work_showtext (s, 2)
  sme.tz.selectedIndex = 0
}
function dq_cleardateparam () {
  var sme = dq_submenuentries['setdateparam']

  sme.texts[1].value = ''
  sme.texts[2].value = ''
  sme.tz.selectedIndex = 0
}


//
// invparam utilities
//

var dq_invparam_curr_el, dq_invparam_work_el
var dq_invparam_curr_els, dq_invparam_work_els
var dq_invparam_curr_inprogress = false, dq_invparam_work_inprogress = false
var dq_invparam_searchs, dq_invparam_searchn

function dq_invparam_curr_send () {
  var sme = dq_submenuentries['setinvparam']
  var s, k, empty

  dq_invparam_searchs = new Array ()
  dq_invparam_searchn = 0

  if (dq_currparams.main['reruninv'] == 'none')
    sme.fel.selectedIndex = 0
  else if (dq_currparams.main['reruninv'] == 'onalarms')
    sme.fel.selectedIndex = 1

  empty = true
  s = vg_cgidir + '/vg_dq_invhelper.cgi'
  s += '?name=' + dq_mainname + '/invhelper.curr'
  s += '&date=' + dq_currparams.date.ldate
  s += '&maxn=0'
  for (k in dq_currparams.inv) {
    s += '&' + k + '=' + dq_currparams.inv[k]
    empty = false
  }
  if (empty) {
    dq_invparam_curr_els = new Array ()
    dq_invparam_curr_el = sme.cel
    return
  }

  dq_invparam_curr_inprogress = true
  vg_xmlquery (s, dq_invparam_curr_receive)
}
function dq_invparam_curr_receive (req) {
  var sme = dq_submenuentries['setinvparam']
  var res, srs, sri, sr, st, drs, dri, dr, dt, deli
  var deli, del, dael, datel, dtel

  dq_invparam_curr_els = new Array ()
  dq_invparam_curr_el = sme.cel
  while (sme.cel.firstChild != null)
    sme.cel.removeChild (sme.cel.firstChild)

  res = req.responseXML
  srs = res.getElementsByTagName ('s')
  for (sri = 0; sri < srs.length; sri++) {
    if (!(sr = srs[sri]))
      break
    st = sr.firstChild.data.split ('|')
  }
  drs = res.getElementsByTagName ('d')
  for (dri = 0; dri < drs.length; dri++) {
    if (!(dr = drs[dri]))
      break
    deli = dq_invparam_curr_els.length
    dt = dr.firstChild.data.split ('|')
    dt[2] = unescape (dt[2])
    del = document.createElement ('div')
    del.className = 'mdivform'
    dael = document.createElement ('a')
    datel = document.createTextNode ('DEL')
    dael.appendChild (datel)
    dael.href = 'javascript:dq_invparam_curr_delete(' + deli + ')'
    dael.title = 'delete this entry'
    dael.className = 'mdivilpick'
    del.appendChild (dael)
    dtel = document.createTextNode (' ' + dt[2])
    del.appendChild (dtel)
    sme.cel.appendChild (del)
    dq_invparam_curr_els[deli] = { el:del, level:dt[0], id:dt[1], name:dt[2] }
  }
  dq_invparam_curr_inprogress = false
}
function dq_invparam_curr_add (level, id, name) {
  var sme = dq_submenuentries['setinvparam']
  var deli, del, dael, datel, dtel

  deli = dq_invparam_curr_els.length
  del = document.createElement ('div')
  del.className = 'mdivform'
  dael = document.createElement ('a')
  datel = document.createTextNode ('DEL')
  dael.className = 'mdivilpick'
  dael.appendChild (datel)
  dael.href = 'javascript:dq_invparam_curr_delete(' + deli + ')'
  dael.title = 'delete this entry'
  del.appendChild (dael)
  dtel = document.createTextNode (' ' + name)
  del.appendChild (dtel)
  sme.cel.appendChild (del)
  dq_invparam_curr_els[deli] = { el:del, level:level, id:id, name:name }
}
function dq_invparam_curr_delete (deli) {
  dq_invparam_curr_el.removeChild (dq_invparam_curr_els[deli].el)
  dq_invparam_curr_els[deli].el = null
}

function dq_invparam_work_send (maxn, level, text, olevel, mode) {
  var s

  s = vg_cgidir + '/vg_dq_invhelper.cgi'
  s += '?name=' + dq_mainname + '/invhelper.work'
  s += '&date=' + dq_currparams.date.ldate
  s += '&maxn=' + maxn
  s += '&olevels=' + olevel
  if (mode == 'name')
    s += '&lname_' + level + '=.*' + text + '.*'
  else
    s += '&level_' + level + '=.*' + text + '.*'

  dq_invparam_work_inprogress = true
  vg_xmlquery (s, dq_invparam_work_receive)
}
function dq_invparam_work_receive (req) {
  var sme = dq_submenuentries['setinvparam']
  var res, srs, sri, sr, st, drs, dri, dr, dt, emitfull, arg
  var deli, del, dael, datel, dtel

  dq_invparam_work_els = new Array ()
  dq_invparam_work_el = sme.wel
  while (sme.wel.firstChild != null)
    sme.wel.removeChild (sme.wel.firstChild)

  emitfull = false
  res = req.responseXML
  srs = res.getElementsByTagName ('s')
  for (sri = 0; sri < srs.length; sri++) {
    if (!(sr = srs[sri]))
      break
    st = sr.firstChild.data.split ('|')
    if (st[1] != st[2])
      emitfull = true
  }
  drs = res.getElementsByTagName ('d')
  for (dri = 0; dri < drs.length; dri++) {
    if (!(dr = drs[dri]))
      break
    deli = dq_invparam_work_els.length
    dt = dr.firstChild.data.split ('|')
    dt[2] = unescape (dt[2])
    arg = '"' + dt[0] + '", "' + dt[1] + '", "' + dt[2] + '"'
    del = document.createElement ('div')
    del.className = 'mdivform'
    dael = document.createElement ('a')
    datel = document.createTextNode ('ADD')
    dael.className = 'mdivilpick'
    dael.title = 'add this entry to the selection'
    dael.appendChild (datel)
    dael.href = 'javascript:dq_invparam_curr_add(' + arg + ')'
    del.appendChild (dael)
    dtel = document.createTextNode (' ')
    del.appendChild (dtel)
    dael = document.createElement ('a')
    datel = document.createTextNode ('SEARCH')
    dael.className = 'mdivilpick'
    dael.appendChild (datel)
    dael.href = 'javascript:dq_invparam_work_changesearch(' + arg + ')'
    dael.title = 'show the next level of items for this entry'
    del.appendChild (dael)
    dtel = document.createTextNode (' ' + dt[2])
    del.appendChild (dtel)
    sme.wel.appendChild (del)
    dq_invparam_work_els[deli] = { el:del, level:dt[0], id:dt[1], name:dt[2] }
  }
  if (emitfull) {
    del = document.createElement ('div')
    del.className = 'mdivform'
    dael = document.createElement ('a')
    datel = document.createTextNode ('...click to see all the entries')
    dael.appendChild (datel)
    dael.href = 'javascript:dq_invparam_work_research(' + deli + ')'
    dael.title = 'list too long but show all the entries anyway'
    del.appendChild (dael)
    sme.wel.appendChild (del)
  }
  dq_invparam_work_inprogress = false
}
function dq_invparam_work_delete (deli) {
  dq_invparam_work_el.removeChild (dq_invparam_work_els[deli].el)
  dq_invparam_work_els[deli].el = null
}
function dq_invparam_work_search (e) {
  var sme = dq_submenuentries['setinvparam']
  var s, i, level, id

  if (e.type != 'click' && e.type != 'prev' && (
    e.type != 'keyup' || e.keyCode != 13
  ))
    return

  sme.level = sme.lel.options[sme.lel.selectedIndex].value
  sme.text = sme.tel.value
  if (sme.text.match (/obj=.*:.*/)) {
    s = sme.text.substr (4)
    i = s.indexOf (':')
    level = s.substring (0, i)
    id = s.substr (i + 1)
    dq_invparam_work_send (1000, level, id, sme.level, 'id')
  } else
    dq_invparam_work_send (1000, sme.level, sme.text, sme.level, 'name')

  if (e.type != 'prev') {
    dq_invparam_searchs[dq_invparam_searchn] = {
      index:sme.lel.selectedIndex, text:sme.text
    }
    dq_invparam_searchn++
  }
}
function dq_invparam_work_research (deli) {
  var sme = dq_submenuentries['setinvparam']
  var s, i, level, id

  sme.level = sme.lel.options[sme.lel.selectedIndex].value
  sme.text = sme.tel.value
  if (sme.text.match (/obj=.*:.*/)) {
    s = sme.text.substr (4)
    i = s.indexOf (':')
    level = s.substring (0, i)
    id = s.substr (i + 1)
    dq_invparam_work_send (0, level, id, sme.level, 'id')
  } else
    dq_invparam_work_send (0, sme.level, sme.text, sme.level, 'name')
}
function dq_invparam_work_changesearch (level, id, name) {
  var sme = dq_submenuentries['setinvparam']
  var nlevel, i

  nlevel = null
  for (i = 0; i < dq_levels.length; i++) {
    if (dq_levels[i].lv == level) {
      nlevel = dq_levels[i].nl
      break
    }
  }
  if (nlevel == null)
    return

  for (i = 0; i < sme.lel.options.length; i++) {
    if (sme.lel.options[i].value == nlevel) {
      sme.lel.selectedIndex = i
      break
    }
  }
  sme.tel.value = 'obj=' + level + ':' + id

  sme.level = sme.lel.options[sme.lel.selectedIndex].value
  sme.text = sme.tel.value
  dq_invparam_work_send (1000, level, id, sme.level, 'id')
  dq_invparam_searchs[dq_invparam_searchn] = {
    index:sme.lel.selectedIndex, text:sme.text
  }
  dq_invparam_searchn++
}
function dq_useinvparam () {
  var sme = dq_submenuentries['setinvparam']
  var edits, deli, o, k

  edits = new Object
  edits.inv = new Array
  edits.main = new Array
  for (deli = 0; deli < dq_invparam_curr_els.length; deli++) {
    if (dq_invparam_curr_els[deli].el == null)
      continue
    o = dq_invparam_curr_els[deli]
    k = 'level_' + o.level
    if (typeof edits.inv[k] != 'undefined')
      edits.inv[k] += '|' + o.id
    else
      edits.inv[k] = o.id
  }
  edits.main['reruninv'] = sme.fel.options[sme.fel.selectedIndex].value
  dq_editcurrurl (edits)
  dq_togglemenuentry ('setinvparam', false)
  dq_togglemenuentry ('otherqueries', true)
}
function dq_resetinvmparam () {
  dq_currparams = null
  dq_splitcurrurl ()
  dq_invparam_curr_send ()
}
function dq_clearinvmparam () {
  var sme = dq_submenuentries['setinvparam']
  var deli

  for (deli = 0; deli < dq_invparam_curr_els.length; deli++) {
    if (dq_invparam_curr_els[deli].el == null)
      continue
    dq_invparam_curr_el.removeChild (dq_invparam_curr_els[deli].el)
    dq_invparam_curr_els[deli].el = null
  }
  sme.fel.selectedIndex = 0
}
function dq_invparam_work_prevsearch () {
  var sme = dq_submenuentries['setinvparam']

  if (dq_invparam_searchn < 2)
    return

  sme.lel.selectedIndex = dq_invparam_searchs[dq_invparam_searchn - 2].index
  sme.tel.value = dq_invparam_searchs[dq_invparam_searchn - 2].text
  dq_invparam_searchn--
  dq_invparam_work_search ({ type:'prev' })
}


//
// statparam utilities
//

var dq_statparam_curr_el, dq_statparam_work_el
var dq_statparam_curr_els, dq_statparam_work_els
var dq_statparam_curr_inprogress = false, dq_statparam_work_inprogress = false
var dq_statparam_searchs, dq_statparam_searchn

function dq_statparam_curr_send () {
  var sme = dq_submenuentries['setstatparam']
  var s, k, empty

  dq_statparam_searchs = new Array ()
  dq_statparam_searchn = 0

  if (dq_currparams.main['statgroup'] == 'both')
    sme.gel.selectedIndex = 0
  else if (dq_currparams.main['statgroup'] == 'curr')
    sme.gel.selectedIndex = 1
  else if (dq_currparams.main['statgroup'] == 'proj')
    sme.gel.selectedIndex = 2

  if (dq_currparams.main['statdisp'] == 'actual')
    sme.del.selectedIndex = 0
  else if (dq_currparams.main['statdisp'] == 'perc')
    sme.del.selectedIndex = 1
  else if (dq_currparams.main['statdisp'] == 'nocap')
    sme.del.selectedIndex = 2

  empty = true
  s = vg_cgidir + '/vg_dq_stathelper.cgi'
  s += '?name=' + dq_mainname + '/stathelper.curr'
  s += '&date=' + dq_currparams.date.ldate
  s += '&maxn=0'
  for (k in dq_currparams.inv)
    s += '&' + k + '=' + dq_currparams.inv[k]
  for (k in dq_currparams.stat) {
    s += '&' + k + '=' + dq_currparams.stat[k]
    empty = false
  }
  if (empty) {
    dq_statparam_curr_els = new Array ()
    dq_statparam_curr_el = sme.cel
    return
  }

  dq_statparam_curr_inprogress = true
  vg_xmlquery (s, dq_statparam_curr_receive)
}
function dq_statparam_curr_receive (req) {
  var sme = dq_submenuentries['setstatparam']
  var res, haveone, srs, sri, sr, st, drs, dri, dr, dt, deli
  var deli, del, dael, datel, dtel

  dq_statparam_curr_els = new Array ()
  dq_statparam_curr_el = sme.cel
  while (sme.cel.firstChild != null)
    sme.cel.removeChild (sme.cel.firstChild)

  haveone = false
  res = req.responseXML
  srs = res.getElementsByTagName ('s')
  for (sri = 0; sri < srs.length; sri++) {
    if (!(sr = srs[sri]))
      break
    st = sr.firstChild.data.split ('|')
    if (st[0] == '1')
      haveone = true
  }
  drs = res.getElementsByTagName ('d')
  for (dri = 0; dri < drs.length; dri++) {
    if (!(dr = drs[dri]))
      break
    deli = dq_statparam_curr_els.length
    dt = dr.firstChild.data.split ('|')
    if (dt[0] == '2' && haveone == true)
      continue
    dt[2] = unescape (dt[2])
    del = document.createElement ('div')
    del.className = 'mdivform'
    dael = document.createElement ('a')
    datel = document.createTextNode ('DEL')
    dael.appendChild (datel)
    dael.href = 'javascript:dq_statparam_curr_delete(' + deli + ')'
    dael.title = 'delete this entry'
    dael.className = 'mdivilpick'
    del.appendChild (dael)
    dtel = document.createTextNode (' ' + dt[2])
    del.appendChild (dtel)
    sme.cel.appendChild (del)
    dq_statparam_curr_els[deli] = { el:del, type:dt[0], id:dt[1], name:dt[2] }
  }
  dq_statparam_curr_inprogress = false
}
function dq_statparam_curr_add (type, id, name) {
  var sme = dq_submenuentries['setstatparam']
  var deli, del, dael, datel, dtel, t

  deli = dq_statparam_curr_els.length
  del = document.createElement ('div')
  del.className = 'mdivform'
  dael = document.createElement ('a')
  datel = document.createTextNode ('DEL')
  dael.className = 'mdivilpick'
  dael.appendChild (datel)
  dael.href = 'javascript:dq_statparam_curr_delete(' + deli + ')'
  dael.title = 'delete this entry'
  del.appendChild (dael)
  if (type == '2')
    t = 'All stats - '
  else
    t = ''
  dtel = document.createTextNode (' ' + t + name)
  del.appendChild (dtel)
  sme.cel.appendChild (del)
  dq_statparam_curr_els[deli] = { el:del, type:type, id:id, name:name }
}
function dq_statparam_curr_delete (deli) {
  dq_statparam_curr_el.removeChild (dq_statparam_curr_els[deli].el)
  dq_statparam_curr_els[deli].el = null
}

function dq_statparam_work_send (maxn, text, mode) {
  var k, s

  s = vg_cgidir + '/vg_dq_stathelper.cgi'
  s += '?name=' + dq_mainname + '/stathelper.work'
  s += '&date=' + dq_currparams.date.ldate
  s += '&maxn=' + maxn
  for (k in dq_currparams.inv)
    s += '&' + k + '=' + dq_currparams.inv[k]
  if (mode == 'name')
    s += '&stat_label' + '=.*' + text + '.*'
  else
    s += '&stat_key' + '=.*' + text + '.*'

  dq_statparam_work_inprogress = true
  vg_xmlquery (s, dq_statparam_work_receive)
}
function dq_statparam_work_receive (req) {
  var sme = dq_submenuentries['setstatparam']
  var res, srs, sri, sr, st, drs, dri, dr, dt, emitfull, arg, t
  var deli, del, dael, datel, dtel

  dq_statparam_work_els = new Array ()
  dq_statparam_work_el = sme.wel
  while (sme.wel.firstChild != null)
    sme.wel.removeChild (sme.wel.firstChild)

  emitfull = false
  res = req.responseXML
  srs = res.getElementsByTagName ('s')
  for (sri = 0; sri < srs.length; sri++) {
    if (!(sr = srs[sri]))
      break
    st = sr.firstChild.data.split ('|')
    if (st[1] != st[2])
      emitfull = true
  }
  drs = res.getElementsByTagName ('d')
  for (dri = 0; dri < drs.length; dri++) {
    if (!(dr = drs[dri]))
      break
    deli = dq_statparam_work_els.length
    dt = dr.firstChild.data.split ('|')
    dt[2] = unescape (dt[2])
    arg = '"' + dt[0] + '", "' + dt[1] + '", "' + dt[2] + '"'
    del = document.createElement ('div')
    del.className = 'mdivform'
    dael = document.createElement ('a')
    datel = document.createTextNode ('ADD')
    dael.className = 'mdivilpick'
    dael.title = 'add this entry to the selection'
    dael.appendChild (datel)
    dael.href = 'javascript:dq_statparam_curr_add(' + arg + ')'
    del.appendChild (dael)
    if (dt[0] == '2')
      t = 'All stats - '
    else
      t = ''
    dtel = document.createTextNode (' ' + t + dt[2])
    del.appendChild (dtel)
    sme.wel.appendChild (del)
    dq_statparam_work_els[deli] = { el:del, type:dt[0], id:dt[1], name:dt[2] }
  }
  if (emitfull) {
    del = document.createElement ('div')
    del.className = 'mdivform'
    dael = document.createElement ('a')
    datel = document.createTextNode ('...click to see all the entries')
    dael.appendChild (datel)
    dael.href = 'javascript:dq_statparam_work_research(' + deli + ')'
    dael.title = 'list too long but show all the entries anyway'
    del.appendChild (dael)
    sme.wel.appendChild (del)
  }
  dq_statparam_work_inprogress = false
}
function dq_statparam_work_delete (deli) {
  dq_statparam_work_el.removeChild (dq_statparam_work_els[deli].el)
  dq_statparam_work_els[deli].el = null
}
function dq_statparam_work_search (e) {
  var sme = dq_submenuentries['setstatparam']
  var id

  if (e.type != 'click' && e.type != 'prev' && (
    e.type != 'keyup' || e.keyCode != 13
  ))
    return

  sme.text = sme.tel.value
  if (sme.text.match (/key=.*/)) {
    id = sme.text.substr (4)
    dq_statparam_work_send (1000, id, 'id')
  } else
    dq_statparam_work_send (1000, sme.text, 'name')

  if (e.type != 'prev') {
    dq_statparam_searchs[dq_statparam_searchn] = {
      text:sme.text
    }
    dq_statparam_searchn++
  }
}
function dq_statparam_work_research (deli) {
  var sme = dq_submenuentries['setstatparam']
  var id

  sme.text = sme.tel.value
  if (sme.text.match (/key=.*:.*/)) {
    id = sme.text.substr (4)
    dq_statparam_work_send (0, id, 'id')
  } else
    dq_statparam_work_send (0, sme.text, 'name')
}
function dq_statparam_work_changesearch (type, id, name) {
  var sme = dq_submenuentries['setstatparam']

  sme.tel.value = 'key=' + id

  sme.text = sme.tel.value
  dq_statparam_work_send (1000, id, 'id')
  dq_statparam_searchs[dq_statparam_searchn] = {
    text:sme.text
  }
  dq_statparam_searchn++
}
function dq_usestatparam () {
  var sme = dq_submenuentries['setstatparam']
  var edits, deli, o

  edits = new Object
  edits.stat = new Array
  edits.main = new Array
  for (deli = 0; deli < dq_statparam_curr_els.length; deli++) {
    if (dq_statparam_curr_els[deli].el == null)
      continue
    o = dq_statparam_curr_els[deli]
    if (typeof edits.stat['stat_key'] != 'undefined')
      edits.stat['stat_key'] += '|' + o.id
    else
      edits.stat['stat_key'] = o.id
  }
  edits.main['statgroup'] = sme.gel.options[sme.gel.selectedIndex].value
  edits.main['statdisp'] = sme.del.options[sme.del.selectedIndex].value
  dq_editcurrurl (edits)
  dq_togglemenuentry ('setstatparam', false)
  dq_togglemenuentry ('otherqueries', true)
}
function dq_resetstatmparam () {
  dq_currparams = null
  dq_splitcurrurl ()
  dq_statparam_curr_send ()
}
function dq_clearstatmparam () {
  var sme = dq_submenuentries['setstatparam']
  var deli

  for (deli = 0; deli < dq_statparam_curr_els.length; deli++) {
    if (dq_statparam_curr_els[deli].el == null)
      continue
    dq_statparam_curr_el.removeChild (dq_statparam_curr_els[deli].el)
    dq_statparam_curr_els[deli].el = null
  }
  sme.gel.selectedIndex = 0
  sme.del.selectedIndex = 0
}
function dq_statparam_work_prevsearch () {
  var sme = dq_submenuentries['setstatparam']

  if (dq_statparam_searchn < 2)
    return

  sme.tel.value = dq_statparam_searchs[dq_statparam_searchn - 2].text
  dq_statparam_searchn--
  dq_statparam_work_search ({ type:'prev' })
}


//
// alarmparam utilities
//

var dq_alarmparam_curr_el
var dq_alarmparam_curr_els

function dq_alarmparam_curr_show () {
  var sme = dq_submenuentries['setalarmparam']
  var s, k, v, re, sevi, tmodei

  dq_alarmparam_curr_els = new Array ()
  dq_alarmparam_curr_el = sme.cel

  for (k in dq_currparams.alarm) {
    v = dq_currparams.alarm[k]
    if (k == 'alarm_ccid')
      dq_alarmparam_curr_add ('ccid', v, v)
    else if (k == 'alarm_msgtxt')
      dq_alarmparam_curr_add ('msgtxt', v, v)
    else if (k == 'alarm_comment')
      dq_alarmparam_curr_add ('comment', v, v)
    else if (k == 'alarm_severity') {
      re = new RegExp (v)
      for (sevi = 0; sevi < dq_sevs.length; sevi++)
        if (dq_sevs[sevi].v.match (re))
          dq_alarmparam_curr_add ('severity', dq_sevs[sevi].v, dq_sevs[sevi].n)
    } else if (k == 'alarm_tmode') {
      re = new RegExp (v)
      for (tmodei = 0; tmodei < dq_tmodes.length; tmodei++)
        if (dq_tmodes[tmodei].v.match (re))
          dq_alarmparam_curr_add (
            'tmode', dq_tmodes[tmodei].v, dq_tmodes[tmodei].n
          )
    }
  }
}
function dq_alarmparam_curr_add (param, id, name) {
  var sme = dq_submenuentries['setalarmparam']
  var deli, del, dael, datel, dtel, t

  deli = dq_alarmparam_curr_els.length
  del = document.createElement ('div')
  del.className = 'mdivform'
  dael = document.createElement ('a')
  datel = document.createTextNode ('DEL')
  dael.className = 'mdivilpick'
  dael.appendChild (datel)
  dael.href = 'javascript:dq_alarmparam_curr_delete(' + deli + ')'
  dael.title = 'delete this entry'
  del.appendChild (dael)
  dtel = document.createTextNode (' ' + name)
  del.appendChild (dtel)
  sme.cel.appendChild (del)
  dq_alarmparam_curr_els[deli] = { el:del, param:param, id:id, name:name }
}
function dq_alarmparam_curr_delete (deli) {
  dq_alarmparam_curr_el.removeChild (dq_alarmparam_curr_els[deli].el)
  dq_alarmparam_curr_els[deli].el = null
}

function dq_alarmparam_work_ccid (e) {
  var sme = dq_submenuentries['setalarmparam']

  if (e.type != 'click' && e.type != 'prev' && (
    e.type != 'keyup' || e.keyCode != 13
  ))
    return

  dq_alarmparam_curr_add ('ccid', sme.ccidel.value, sme.ccidel.value)
}
function dq_alarmparam_work_sev (sevi) {
  dq_alarmparam_curr_add ('severity', dq_sevs[sevi].n, dq_sevs[sevi].n)
}
function dq_alarmparam_work_tmode (tmodei) {
  dq_alarmparam_curr_add ('tmode', dq_tmodes[tmodei].v, dq_tmodes[tmodei].n)
}
function dq_alarmparam_work_msg (e) {
  var sme = dq_submenuentries['setalarmparam']

  if (e.type != 'click' && e.type != 'prev' && (
    e.type != 'keyup' || e.keyCode != 13
  ))
    return

  dq_alarmparam_curr_add ('msgtxt', sme.msgel.value, sme.msgel.value)
}
function dq_alarmparam_work_com (e) {
  var sme = dq_submenuentries['setalarmparam']

  if (e.type != 'click' && e.type != 'prev' && (
    e.type != 'keyup' || e.keyCode != 13
  ))
    return

  dq_alarmparam_curr_add ('comment', sme.comel.value, sme.comel.value)
}
function dq_usealarmparam () {
  var edits, deli, o, k

  edits = new Object
  edits.alarm = new Array
  for (deli = 0; deli < dq_alarmparam_curr_els.length; deli++) {
    if (dq_alarmparam_curr_els[deli].el == null)
      continue
    o = dq_alarmparam_curr_els[deli]
    k = 'alarm_' + o.param
    if (typeof edits.alarm[k] != 'undefined')
      edits.alarm[k] += '|' + o.id
    else
      edits.alarm[k] = o.id
  }
  dq_editcurrurl (edits)
  dq_togglemenuentry ('setalarmparam', false)
  dq_togglemenuentry ('otherqueries', true)
}
function dq_resetalarmparam () {
  dq_currparams = null
  dq_splitcurrurl ()
  dq_alarmparam_curr_send ()
}
function dq_clearalarmparam () {
  var deli

  for (deli = 0; deli < dq_alarmparam_curr_els.length; deli++) {
    if (dq_alarmparam_curr_els[deli].el == null)
      continue
    dq_alarmparam_curr_el.removeChild (dq_alarmparam_curr_els[deli].el)
    dq_alarmparam_curr_els[deli].el = null
  }
}
function dq_alarmparam_work_prevsearch () {
  var sme = dq_submenuentries['setalarmparam']

  if (dq_alarmparam_searchn < 2)
    return

  sme.tel.value = dq_alarmparam_searchs[dq_alarmparam_searchn - 2].text
  dq_alarmparam_searchn--
  dq_alarmparam_work_search ({ type:'prev' })
}


//
// Alarm Queries
//

function dq_aq_runclear (mode, label) {
  if (!confirm ('confirm ' + label))
    return

  dq_startcurrurlxml (null, { 'main':{ 'mode':mode }})
}


//
// Stat Queries
//

function dq_sq_setzoommode (kind, mode) {
  if (kind == 'achart')
    dq_runcurrurl (
      null, { 'main':{ 'qid':'alarmstatchart', 'statchartzoom':mode }}
    )
  else
    dq_runcurrurl (null, { 'main':{ 'qid':'statchart', 'statchartzoom':mode }})
}


//
// variable management
//

var dq_varstate = new Array ()

function dq_setvarstate (k, v) {
  dq_varstate[k] = v
}

function dq_modvarstate (k, v) {
  var s

  if (dq_varstate[k] == v)
    return
  dq_varstate[k] = v

  s = ''
  for (k in dq_varstate) {
    if (s.length > 0)
      s += ';'
    s += k + '=' + dq_varstate[k]
  }
  document.cookie = 'attVGvars=' + escape (s) + '; path=/'
}


//
// URL management
//

var dq_currurl = null, dq_currparams = null

function dq_runcurrurl (url, edits) {
  if (url != null) {
    dq_currurl = url
    dq_splitcurrurl ()
  } else if (dq_currparams == null)
    dq_splitcurrurl ()

  if (typeof edits != "undefined" && edits == null)
    edits = {}
  dq_editcurrurl (edits)
  document.cookie = 'attVGvars=; path=/'
  location.href = dq_packcurrurl ()
}

function dq_startcurrurlxml (url, edits) {
  if (url != null) {
    dq_currurl = url
    dq_splitcurrurl ()
  } else if (dq_currparams == null)
    dq_splitcurrurl ()

  if (typeof edits != "undefined" && edits == null)
    edits = {}
  dq_editcurrurl (edits)
  vg_xmlquery (dq_packcurrurl (), dq_endcurrurlxml)
}

function dq_endcurrurlxml (req) {
  var res, recs, fields

  res = req.responseXML
  recs = res.getElementsByTagName ('r')
  if (recs.length < 1) {
    alert ('operation failed')
    return
  }
  fields = recs[0].firstChild.data.split ('|')
  if (fields[0] == 'OK') {
    alert ('success - ' + unescape (fields[1]))
  } else {
    alert ('failure - errors: ' + unescape (fields[1]))
  }
}

function dq_splitcurrurl () {
  var t1s, t2s, t2i, t3s, t3i, t4s, t4i, k, v

  t1s = dq_currurl.split ('?')
  if (t1s.length < 2)
    return

  dq_currparams = new Object ()
  dq_currparams.inv = new Array ()
  dq_currparams.alarm = new Array ()
  dq_currparams.stat = new Array ()
  dq_currparams.date = new Array ()
  dq_currparams.main = new Array ()
  dq_currparams.path = t1s[0]

  t2s = t1s[1].split ('&')
  for (t2i = 0; t2i < t2s.length; t2i++) {
    t3s = t2s[t2i].split ('=')
    if (t3s.length < 2)
      continue
    if (t3s.length > 2) {
      for (t3i = 2; t3i < t3s.length; t3i++)
        t3s[1] += '=' + t3s[t3i]
    }
    k = unescape (t3s[0])
    v = unescape (t3s[1])
    if (k == 'obj') {
      t4s = v.split (':')
      if (t4s.length < 3)
        continue
      if (t4s.length > 3) {
        for (t4i = 3; t4i < t4s.length; t4i++)
          t4s[2] += ':' + t4s[t4i]
      }
      k = 'level_' + t4s[1]
      v = t4s[2]
    }

    if (k.match (/level_.*/) || k.match (/lname_.*/)) {
      if (typeof dq_currparams.inv[k] != 'undefined')
        dq_currparams.inv[k] += '|' + v
      else
        dq_currparams.inv[k] = v
    } else if (k.match (/alarm_.*/)) {
      if (typeof dq_currparams.alarm[k] != 'undefined')
        dq_currparams.alarm[k] += '|' + v
      else
        dq_currparams.alarm[k] = v
    } else if (k.match (/stat_.*/)) {
      if (typeof dq_currparams.stat[k] != 'undefined')
        dq_currparams.stat[k] += '|' + v
      else
        dq_currparams.stat[k] = v
    } else if (k.match (/(fdate|ldate|datefilter|tzoverride)/))
      dq_currparams.date[k] = v
    else if (k != 'name')
      dq_currparams.main[k] = v
  }
  if (
    typeof dq_currparams.date['fdate'] == 'undefined' &&
    typeof dq_currparams.date['ldate'] == 'undefined'
  ) {
    dq_currparams.date['fdate'] = 'latest'
    dq_currparams.date['ldate'] = ''
  }
}

function dq_editcurrurl (edits) {
  var k

  if (typeof edits.inv != "undefined") {
    dq_currparams.inv = new Array ()
    for (k in edits.inv)
      dq_currparams.inv[k] = edits.inv[k]
    if (typeof dq_currparams.main['objlabel'] != 'undefined')
      delete dq_currparams.main['objlabel']
  }
  if (typeof edits.alarm != "undefined") {
    dq_currparams.alarm = new Array ()
    for (k in edits.alarm)
      dq_currparams.alarm[k] = edits.alarm[k]
  }
  if (typeof edits.stat != "undefined") {
    dq_currparams.stat = new Array ()
    for (k in edits.stat)
      dq_currparams.stat[k] = edits.stat[k]
  }
  if (typeof edits.date != "undefined") {
    dq_currparams.date = new Array ()
    for (k in edits.date)
      dq_currparams.date[k] = edits.date[k]
  }
  if (typeof edits.main != "undefined") {
    for (k in edits.main)
      dq_currparams.main[k] = edits.main[k]
  }

  vg_getwindowsize ()
  dq_currparams.main['winw'] = vg_windowsize.x
  dq_currparams.main['winh'] = vg_windowsize.y
  dq_currparams.main['update'] = dq_updaterate
}

function dq_packcurrurl () {
  var s, sep

  s = dq_currparams.path
  sep = '?'
  for (k in dq_currparams.inv) {
    s += sep + k + '=' + escape (dq_currparams.inv[k])
    sep = '&'
  }
  for (k in dq_currparams.alarm) {
    s += sep + k + '=' + escape (dq_currparams.alarm[k])
    sep = '&'
  }
  for (k in dq_currparams.stat) {
    s += sep + k + '=' + escape (dq_currparams.stat[k])
    sep = '&'
  }
  for (k in dq_currparams.date) {
    s += sep + k + '=' + escape (dq_currparams.date[k])
    sep = '&'
  }
  for (k in dq_currparams.main) {
    s += sep + k + '=' + escape (dq_currparams.main[k])
    sep = '&'
  }

  return s
}
