var times = {

  "default"          :["Default Time Zone",          0      ],
  "AKST9AKDT"        :["Alaskan Time",               -9     ],
  "AST4ADT"          :["Atlantic Time",              -4     ],
  "ACST-9:30ACDT"    :["Australian Central Time",    +9.5   ],
  "AEST-10AEDT"      :["Australian Eastern Time",    +10    ],
  "AWST-8"           :["Australian Western Time",    +8     ],
  "BT-3"             :["Baghdad Time",               +3     ],
  "BST-1"            :["British Summer Time",        +1     ],
  "CET-1CEST"        :["Central European Time",      +1     ],
  "CST6CDT"          :["Central Time",               -6     ],
  "CHADT-13:45CHAST" :["Chatham Islands Time",       +13.75 ],
  "CCT-8"            :["China Coast Time",           +8     ],
  "UTC"              :["Universal Time (UTC)",       0      ],
  "EET-2EEST"        :["Eastern European Time",      +2     ],
  "EST5EDT"          :["Eastern Time",               -5     ],
  "GMT"              :["Greenwich Mean Time",        0      ],
  "GST-10"           :["Guam Time",                  +10    ],
  "HST10"            :["Hawaiian Time",              -10    ],
  "IST-5:30"         :["India Time",                 +5.5   ],
  "IDLE-12"          :["Date Line East",             +12    ],
  "IDLW12"           :["Date Line West",             -12    ],
  "IST-1"            :["Irish Summer Time",          +1     ],
  "JST-9"            :["Japan Time",                 +9     ],
  "MET-1"            :["Middle European Time",       +1     ],
  "MSK-3MSD"         :["Moscow Time",                +3     ],
  "MST7MDT"          :["Mountain Time",              -7     ],
  "NZST-12NZDT"      :["New Zealand Time",           +12    ],
  "NST3:30NDT"       :["Newfoundland Time",          -3.5   ],
  "PST8PDT"          :["Pacific Time",               -8     ],
  "SWT0"             :["Swedish Winter Time",        0      ],
  "UT"               :["Universal Time",             0      ],
  "WAT1"             :["West African Time",          -1     ],
  "WET0WEST"         :["Western European Time",      0      ]
}
function getTopPos(id) {
  var obj = document.getElementById('upIcon'+id);
  var objP = document.getElementById('field'+id);
  var top=0;
  top += objP.offsetTop;
  obj.style.top = top +'px';
}
function getBottomPos(id) {
  var obj = document.getElementById('downIcon'+id);
  var objP = document.getElementById('field'+id);
  obj.style.top = objP.style.top+obj.style.height;
}
function getLeftPos(id) {
  var objT = document.getElementById('upIcon'+id);
  var objB = document.getElementById('downIcon'+id);
  var objP = document.getElementById('field'+id);
  objT.style.left = objP.style.left+objP.style.width-objT.style.width;
  objB.style.left = objP.style.left+objP.style.width-objB.style.width;
}
function showHide(id,hideid,hiddenId,colflg) {
  var item = document.getElementById(hideid);
  var obj = document.getElementById(id);
  if (item.style.display == "none") {
    item.innerHTML=gencolpan(id,hideid,hiddenId,colflg);
    item.style.display = "block";
    item.style.position = "absolute";
    var pos = findPos(obj);
    item.style.left = pos[0]+"px"
    item.style.top = pos[1]+obj.offsetHeight+"px"
    item.style.visibility = "visible"
  } else {
    item.style.display = "none";
  }
}
function showHideGRP(id) {
  var obj = document.getElementById(id);
  if (obj.style.display == "none") {
    obj.style.display = "block";
    obj.style.visibility = "visible"
  } else {
    obj.style.display = "none";
  }
}
function findPos(obj){
  var x = 0
  var y = 0

  if(obj.offsetParent)
    do {
      if(obj.tagName != "DIV"){
        x += obj.offsetLeft
        y += obj.offsetTop
      }
    }while (obj = obj.offsetParent)
  return [x,y]
}
function setImagePosition(id,imgstr){
  var obj = document.getElementById(imgstr+id)
  var objP = document.getElementById("field"+id)
  obj.style.position = "absolute";
  var pos = findPos(objP)
  obj.style.left = pos[0]+objP.offsetWidth-obj.offsetWidth-2+"px"
  if(imgstr=="downIcon"){
    obj.style.top = pos[1]+obj.offsetHeight+3+"px"
  } else {
    obj.style.top = pos[1]+3+"px"
  }
}
function upSize(id){
  var obj = document.getElementById("field"+id)
  var num = parseInt(obj.value.substring(0,obj.value.length-2))
  obj.value = (num +1) + "px"
}
function downSize(id){
  var obj = document.getElementById("field"+id)
  var num = parseInt(obj.value.substring(0,obj.value.length-2))

  if(num>0)
    obj.value = (num-1) + "px"
  else
    obj.value = "0px"
}
function hideall(hideid) {
  document.getElementById(hideid).style.display = 'none';
}
function getColor(tmpcolor) {
  if(tmpcolor.indexOf('#')>=0){
    return tmpcolor;
  }else{
    var ind = tmpcolor.indexOf(',');
    var tmpstr = tmpcolor.substring(ind+2,tmpcolor.length);
    var red = tmpcolor.substring(4,ind);
    var green = tmpcolor.substring(ind+2,ind+2+tmpstr.indexOf(','));
    var blue = tmpcolor.substring(ind+2+tmpstr.indexOf(',')+2,tmpcolor.indexOf(')'));
    return '#'+toHex(red)+toHex(green)+toHex(blue);
  }
}
function toHex(c) {
  if (c==null) return '00';
  c=parseInt(c);
  if (c==0 || isNaN(c)) return '00';
  c=Math.max(0,c);
  c=Math.min(c,255);
  c=Math.round(c);
  return '0123456789ABCDEF'.charAt((c-c%16)/16)+'0123456789ABCDEF'.charAt(c%16);
}
function gencolpan(id,hideid,hiddenId,colflg) {
  var colorData=new Array(
    '#FFFFFF', '#CCCCCC', '#999999', '#666666', '#333333', '#000000', '#FFCC00', '#FF9900', '#FF6600', '#FF3300', '#000000', '#333333', '#666666', '#999999', '#CCCCCC', '#FFFFFF',
    '#99CC00', '#000000', '#000000', '#000000', '#000000', '#CC9900', '#FFCC33', '#FFCC66', '#FF9966', '#FF6633', '#CC3300', '#000000', '#000000', '#000000', '#000000', '#CC0033',
    '#CCFF00', '#CCFF33', '#333300', '#666600', '#999900', '#CCCC00', '#FFFF00', '#CC9933', '#CC6633', '#330000', '#660000', '#990000', '#CC0000', '#FF0000', '#FF3366', '#FF0033',
    '#99FF00', '#CCFF66', '#99CC33', '#666633', '#999933', '#CCCC33', '#FFFF33', '#996600', '#993300', '#663333', '#993333', '#CC3333', '#FF3333', '#CC3366', '#FF6699', '#FF0066',
    '#66FF00', '#99FF66', '#66CC33', '#669900', '#999966', '#CCCC66', '#FFFF66', '#996633', '#663300', '#996666', '#CC6666', '#FF6666', '#990033', '#CC3399', '#FF66CC', '#FF0099',
    '#33FF00', '#66FF33', '#339900', '#66CC00', '#99FF33', '#CCCC99', '#FFFF99', '#CC9966', '#CC6600', '#CC9999', '#FF9999', '#FF3399', '#CC0066', '#990066', '#FF33CC', '#FF00CC',
    '#00CC00', '#33CC00', '#336600', '#669933', '#99CC66', '#CCFF99', '#FFFFCC', '#FFCC99', '#FF9933', '#FFCCCC', '#FF99CC', '#CC6699', '#993366', '#660033', '#CC0099', '#330033',
    '#33CC33', '#66CC66', '#00FF00', '#33FF33', '#66FF66', '#99FF99', '#CCFFCC', '#000000', '#000000', '#000000', '#CC99CC', '#996699', '#993399', '#990099', '#663366', '#660066',
    '#006600', '#336633', '#009900', '#339933', '#669966', '#99CC99', '#000000', '#000000', '#000000', '#FFCCFF', '#FF99FF', '#FF66FF', '#FF33FF', '#FF00FF', '#CC66CC', '#CC33CC',
    '#003300', '#00CC33', '#006633', '#339966', '#66CC99', '#99FFCC', '#CCFFFF', '#3399FF', '#99CCFF', '#CCCCFF', '#CC99FF', '#9966CC', '#663399', '#330066', '#9900CC', '#CC00CC',
    '#00FF33', '#33FF66', '#009933', '#00CC66', '#33FF99', '#99FFFF', '#99CCCC', '#0066CC', '#6699CC', '#9999FF', '#9999CC', '#9933FF', '#6600CC', '#660099', '#CC33FF', '#CC00FF',
    '#00FF66', '#66FF99', '#33CC66', '#009966', '#66FFFF', '#66CCCC', '#669999', '#003366', '#336699', '#6666FF', '#6666CC', '#666699', '#330099', '#9933CC', '#CC66FF', '#9900FF',
    '#00FF99', '#66FFCC', '#33CC99', '#33FFFF', '#33CCCC', '#339999', '#336666', '#006699', '#003399', '#3333FF', '#3333CC', '#333399', '#333366', '#6633CC', '#9966FF', '#6600FF',
    '#00FFCC', '#33FFCC', '#00FFFF', '#00CCCC', '#009999', '#006666', '#003333', '#3399CC', '#3366CC', '#0000FF', '#0000CC', '#000099', '#000066', '#000033', '#6633FF', '#3300FF',
    '#00CC99', '#000000', '#000000', '#000000', '#000000', '#0099CC', '#33CCFF', '#66CCFF', '#6699FF', '#3366FF', '#0033CC', '#000000', '#000000', '#000000', '#000000', '#3300CC',
    '#FFFFFF', '#CCCCCC', '#999999', '#666666', '#333333', '#000000', '#00CCFF', '#0099FF', '#0066FF', '#0033FF', '#000000', '#333333', '#666666', '#999999', '#CCCCCC', '#FFFFFF'
  );
  var colorTdHtml='';
  var colorHtml='';
  var setcolflg =0;
  var curcolor = getColor(document.getElementById(id).style.backgroundColor);
  for (i=1; i<=colorData.length;i++){
    if (setcolflg == 0 && curcolor.toLowerCase() == colorData[i-1].toLowerCase()) {
      colorTdHtml=colorTdHtml+'<td style=\"border: double\" bgcolor='+colorData[i-1]+' width=10 height=10 onclick=\"getcolor(\''+id+'\',\''+hideid+'\',\''+colorData[i-1]+'\',\''+hiddenId+'\',\''+colflg+'\')\">\n';
      setcolflg =1;
    } else {
      colorTdHtml=colorTdHtml+'<td style="background-color:'+colorData[i-1]+'" bgcolor='+colorData[i-1]+' width=10 height=10 onclick=\"getcolor(\''+id+'\',\''+hideid+'\',\''+colorData[i-1]+'\',\''+hiddenId+'\',\''+colflg+'\')\">\n';
    }
    if ((i % 16) == 0) {
      colorHtml=colorHtml+'<tr>'+colorTdHtml+'</tr>\n';
      colorTdHtml='';
    }
  }
  colorHtml='<table cellspacing=0 cellpadding=0 style=\"cursor:pointer; font-size: 10px; border-collapse: collapse\" border=1 bordercolorlight=\"000000\">\n'+colorHtml+'</table>';
  return colorHtml;
}
function getcolor(id,hideid,bgcol,hiddenId,colflg) {
  document.getElementById(id).style.backgroundColor=bgcol;
  if(colflg==2) set_color_val(hiddenId);
  if(colflg==1) set_color_val(hiddenId);
  //if(colflg==1) get_style_val(hiddenId);
  document.getElementById(hideid).style.display = 'none';
}
function set_radio_val(radioName,hiddenID) {
  for (var i=0; i < document.getElementsByName(radioName).length; i++)
  {
    if (document.getElementsByName(radioName)[i].checked)
    {
      document.getElementById(hiddenID).value = document.getElementsByName(radioName)[i].value;
    }
  }
}
function set_comb_val(hiddenID,len) {
  var tmpval = "";

  for (i=0; i<len; i++) {
    tmpval += document.getElementById(hiddenID+i).value + ", ";
  }
  document.getElementById(hiddenID).value =  tmpval.substring(0,tmpval.length-2);
}
function set_viewsize_val(hiddenID) {
  document.getElementById(hiddenID).value = document.getElementById(hiddenID+"W").value + " " + document.getElementById(hiddenID+"H").value;
}
function set_graphsize_val(hiddenID) {
  document.getElementById(hiddenID).value = document.getElementById(hiddenID+"G").value;
}
function set_viewpos_val(hiddenID) {
  document.getElementById(hiddenID).value = document.getElementById(hiddenID+"X").value + " " + document.getElementById(hiddenID+"Y").value;
}
function set_color_val(hiddenID) {
  document.getElementById(hiddenID).value = getColor(document.getElementById("BG"+hiddenID).style.backgroundColor);
  //document.getElementById(hiddenID).value = getColor(document.getElementById(hiddenID+"0").style.backgroundColor)
  //+ " " + getColor(document.getElementById(hiddenID+"1").style.backgroundColor);
}
function get_style_val(hiddenID) {
  var tmpval="";
  var tmpId="";
  for (var i=0; i < document.getElementsByName("style"+hiddenID).length; i++)
  {
    tmpId = document.getElementsByName("style"+hiddenID)[i].id;
    if (tmpId.indexOf("color")>=0)
      tmpval = tmpval + "color: " + getColor(document.getElementById(tmpId).style.backgroundColor) +"; ";
    else if(tmpId.indexOf("background")>=0)
      tmpval = tmpval + "background: " + getColor(document.getElementById(tmpId).style.backgroundColor) +"; ";
    else if(tmpId.indexOf("font-size")>=0)
      tmpval = tmpval + "font-size: " + document.getElementById(tmpId).value +"; ";
    else if(tmpId.indexOf("text-align")>=0)
      tmpval = tmpval + "text-align: " + document.getElementById(tmpId).value +"; ";
    else if(tmpId.indexOf("font-family0")>=0)
      tmpval = tmpval + "font-family: " + document.getElementById(tmpId).value + ", ";
    else if(tmpId.indexOf("font-family1")>=0)
      tmpval = tmpval + document.getElementById(tmpId).value + ", ";
    else if(tmpId.indexOf("font-family2")>=0)
      tmpval = tmpval + document.getElementById(tmpId).value + "; ";
    else if(tmpId.indexOf("margin0")>=0)
      tmpval = tmpval + "margin: " + document.getElementById(tmpId).value + " ";
    else if(tmpId.indexOf("margin1")>=0)
      tmpval = tmpval + document.getElementById(tmpId).value + " ";
    else if(tmpId.indexOf("margin2")>=0)
      tmpval = tmpval + document.getElementById(tmpId).value + " ";
    else if(tmpId.indexOf("margin3")>=0)
      tmpval = tmpval + document.getElementById(tmpId).value + "; ";
    else if(tmpId.indexOf("padding0")>=0)
      tmpval = tmpval + "padding: " + document.getElementById(tmpId).value + " ";
    else if(tmpId.indexOf("padding1")>=0)
      tmpval = tmpval + document.getElementById(tmpId).value + " ";
    else if(tmpId.indexOf("padding2")>=0)
      tmpval = tmpval + document.getElementById(tmpId).value + " ";
    else if(tmpId.indexOf("padding3")>=0)
      tmpval = tmpval + document.getElementById(tmpId).value + "; ";
    else
      tmpval = tmpval + tmpId.substring(hiddenID.length)+ ": " + document.getElementById(tmpId).value +"; ";
  }
  document.getElementById(hiddenID).value = tmpval.substring(0,tmpval.length-2)
  //alert(document.getElementById(hiddenID).value);
}
function showTime(sel) {
  var tzOffset = times[sel.options[sel.selectedIndex].value][1]
  var now = new Date();
  var actualTime = now.getTime();
  actualTime += (now.getTimezoneOffset() * 60000); // where are we?
  actualTime += (tzOffset * 3600000); // where are they
  now = new Date(actualTime);
  document.getElementById("timeDiv").innerHTML="Current time based on selected timezone is:<br \>"+now;
}
function setTimeVal(curTimeVal) {
  for (var op in times) {
    if (curTimeVal == op)
      document.write("<option value='"+op+"' selected>"+times[op][0]+"</option>");
    else
      document.write("<option value='"+op+"'>"+times[op][0]+"</option>");
  }
}
