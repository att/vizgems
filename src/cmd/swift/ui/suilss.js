
var suilss_url, suilss_del

function suilss_init (u) {
  var el, name, url

  suilss_del = document.getElementById ('suilss_div')
  suilss_url = u
  suilss_query ()
}
function suilss_update (s) {
  suilss_del.innerHTML = s
}
function suilss_query () {
  suilss_xmlquery (suilss_url, suilss_result)
}
function suilss_result (req) {
  var res, recs, rec, str

  res = req.responseXML
  recs = res.getElementsByTagName ('script')
  if (recs.length >= 1) {
    rec = recs[0];
    if (rec.firstChild) {
      str = rec.firstChild.data
      eval (unescape (str))
    }
  }
  suilss_query (suilss_url)
}

function suilss_xmlquery (url, func) {
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
