
file=$1
field=$2

for dir in ${PATH//:/ }; do
    if [[ -f $dir/../lib/vg/vg_cmfilelist ]] then
        configfile=$dir/../lib/vg/vg_cmfilelist
        break
    fi
done
. $configfile

if [[ ${files[@]} == '' ]] then
    print -u2 SWIFT ERROR missing configuration file
    exit 1
fi

typeset -n fdata=files.$file.$field

if [[ $# == 2 ]] then
    print "<script>"
    print "function set (v) {"
    case $field in
    fields.field*)
        fieldi=${field#fields.field}
        print "  opener.ffields[$fieldi].data.value = v"
        ;;
    multifields.choice*.field*)
        choicei=${field#multifields.choice}
        choicei=${choicei%%.*}
        fieldi=${field#multifields.choice*.field}
        print "  opener.cfields[$fieldi][$choicei].data.value = v"
    ;;
    esac
    print "}"
    print "function get () {"
    case $field in
    fields.field*)
        fieldi=${field#fields.field}
        print "  return opener.ffields[$fieldi].data.value"
        ;;
    multifields.choice*.field*)
        choicei=${field#multifields.choice}
        choicei=${choicei%%.*}
        fieldi=${field#multifields.choice*.field}
        print "  return opener.cfields[$fieldi][$choicei].data.value"
    ;;
    esac
    print "}"
    print "function setffield (fieldi, v) {"
    print "  opener.ffields[fieldi].data.value = v"
    print "}"
    print "function getffield (fieldi) {"
    print "  return opener.ffields[fieldi].data.value"
    print "}"
    print "</script>"

    print "<div class=page>"
    print "<form>"
    print "<input"
    print "class=page type=button name=_close value=Close onClick='top.close()'"
    print ">"
fi

case ${fdata.name} in
asset)
    print "<br><select"
    print "class=page name=list size=20"
    print "onChange='set (this.options[this.selectedIndex].value)'"
    print ">"
    export MAXN=200000 SHOWRECS=1 SHOWATTRS=0
    if [[ $file == threshold ]] then
        export INVATTRSKIPNULL=scope_systype
    fi
    $SHELL vg_dq_invhelper 'o' "" | egrep '^B\|o\|' | sort \
    | while read -u3 -r asset; do
        name=${asset##*'|'}
        encname=$(printf '%H' "$name")
        asset=${asset#*'|'}
        asset=${asset#*'|'}
        asset=${asset%%'|'*}
        print "<option value='$asset'>$asset"
        if [[ $name != $asset ]] then
            print "<option clas=page value='name=$encname'>"
            print "&nbsp;&nbsp;NAME: $name"
        fi
    done 3<&0-
    print "</select>"
    ;;
starttime)
    print "<br><select"
    print "class=page name=list size=20"
    print "onChange='set (this.options[this.selectedIndex].value)'"
    print ">"
    for t in now '1 hour' '2 hours' '3 hours'; do
        tt=$(TZ=$PHTZ printf '%(%H:%M)T' "$t")
        print "<option class=page value='$tt'>$t"
    done
    for ((t = 0; t < 24; t++)) do
        tt=$(printf '%02d:00' $t)
        print "<option class=page value='$tt'>at $tt"
    done
    print "</select>"
    ;;
endtime)
    print "<br><select"
    print "class=page name=list size=20"
    print "onChange='set (this.options[this.selectedIndex].value)'"
    print ">"
    for t in '1 hour' '2 hours' '3 hours'; do
        tt=$(TZ=$PHTZ printf '%(%H:%M)T' "$t")
        print "<option class=page value='$tt'>$t"
    done
    for ((t = 0; t < 24; t++)) do
        tt=$(printf '%02d:00' $t)
        print "<option class=page value='$tt'>at $tt"
    done
    print "<option value='23:59'>at 23:59"
    print "</select>"
    ;;
startdate)
    print "<br><select"
    print "class=page name=list size=20"
    print "onChange='set (this.options[this.selectedIndex].value)'"
    print ">"
    for t in today '1 day' '2 days' '3 days' '7 days'; do
        tt=$(TZ=$PHTZ printf '%(%Y.%m.%d %a)T' "$t")
        print "<option class=page value='$tt'>$t"
    done
    today=$(TZ=$PHTZ printf '%(%#)T' today)
    ((dsec = 24 * 60 * 60))
    for ((t = 0; t <= 30; t++)) do
        tt=$(
            TZ=$PHTZ \
            printf '%(%Y.%m.%d %a)T' \#$((today + dsec * t + dsec / 12))
        )
        print "<option class=page value='$tt'>$tt"
    done
    print "</select>"
    ;;
enddate)
    print "<br><select"
    print "class=page name=list size=20"
    print "onChange='set (this.options[this.selectedIndex].value)'"
    print ">"
    for t in today '1 day' '2 days' '3 days' '7 days'; do
        tt=$(TZ=$PHTZ printf '%(%Y.%m.%d %a)T' "$t")
        print "<option class=page value='$tt'>$t"
    done
    today=$(TZ=$PHTZ printf '%(%#)T' today)
    ((dsec = 24 * 60 * 60))
    for ((t = 0; t <= 30; t++)) do
        tt=$(
            TZ=$PHTZ \
            printf '%(%Y.%m.%d %a)T' \#$((today + dsec * t + dsec / 12))
        )
        print "<option class=page value='$tt'>$tt"
    done
    print "<option class=page value='indefinitely'>indefinitely"
    print "</select>"
    ;;
sname)
    . vg_hdr

    cgiprefix=$CGIPREFIX
    tmpdir=$TMPDIR

    if [[ $# == 3 ]] then
        typeset -A mid2name stmidmap stmid2alarm stid2pref sts attrs

        idre=${3//'+'/' '}
        idre=${idre//@([\'\\])/'\'\1}
        eval "idre=\$'${idre//'%'@(??)/'\x'\1"'\$'"}'"
        IFS='|'
        while read -r mid mname j; do
            mid2name[$mid]=${mname%%' ('*}
        done < $SWIFTDATADIR/dpfiles/stat/statlist.txt
        while read -r sl cst sst mid rep impl unit type alarm palarm estat; do
            [[ $cst == '' ]] && continue
            [[ $rep != y && $alarm == '' ]] && continue
            suffix=
            [[ $mid == *.* ]] && suffix=${mid#*.}
            if [[ ${mid2name[${mid%.*}]} != '' ]] then
                mid=${mid%.*}
            fi
            stmidmap[$cst:$mid]=${mid2name[$mid]:-${mid2name[${mid%.*}]:-$mid}}
            if [[ $impl == *'[['*']]'* ]] then
                impl=${impl#*'[['}
                impl=${impl%%']]'*}
                impl=${impl#*!P!}
                impl=${impl%!S!*}
                impl=${impl#!}
                impl=${impl%!*}
                if [[ "${stid2pref[$cst:$mid]}|" != *'|'$impl'|'* ]] then
                    stid2pref[$cst:$mid]+="|$impl"
                fi
                if [[ $impl == scopeinv* ]] then
                    impl=${impl//scopeinv_size_/si_sz_}
                    impl=${impl//scopeinv_/si_}
                    if [[ "${stid2pref[$cst:$mid]}|" != *'|'$impl'|'* ]] then
                        stid2pref[$cst:$mid]+="|$impl"
                    fi
                fi
            elif [[ $suffix != '' ]] then
                if [[ "${stid2pref[$cst:$mid]}|" != *'|'$suffix'|'* ]] then
                    stid2pref[$cst:$mid]+="|$suffix"
                fi
            fi
            stmid2alarm[$cst:$mid]=$(printf '%#H' "$alarm")
        done < $SWIFTDATADIR/dpfiles/stat/parameter.txt
        cn=0
        INVATTRKEY=scope_systype INVATTRSKIPNULL=scope_systype MAXN=200000 \
        SHOWRECS=1 SHOWATTRS=0 \
        $SHELL vg_dq_invhelper 'o' "$idre" | egrep '^B' \
        | while read -r t lv id st; do
            (( cn++ ))
            sts[$st]=1
        done
        set -s -A stlist -- ${!sts[@]}
        if (( cn == 1 )) then
            INVATTRSKIPNULL=name MAXN=200000 SHOWRECS=0 SHOWATTRS=1 \
            $SHELL vg_dq_invhelper 'o' "$idre" | egrep '^D.*\|(si|scopeinv)_' \
            | while read -r t lv id k v; do
                attrs[$k]=$v
                if [[ $k == scopeinv* ]] then
                    k=${k//scopeinv_size_/si_sz_}
                    k=${k//scopeinv_/si_}
                    attrs[$k]=$v
                fi
            done
        fi
        print "Content-type: text/xml"
        print "Expires: Sat, 1 Jan 2000 00:00:00 EST\n"
        print "<response>"
        for st in "${stlist[@]}"; do
            unset mnamemidmap; typeset -A mnamemidmap
            for stmid in "${!stmidmap[@]}"; do
                [[ $stmid != $st:* ]] && continue
                mnamemidmap[${stmidmap[$stmid]}:${stmid#*:}]=1
            done
            print -r "<r>$st|allstats|All Stats|</r>"
            set -s -A mnameidlist -- ${!mnamemidmap[@]}
            for mnameid in "${mnameidlist[@]}"; do
                mid=${mnameid##*:}
                mname=${mnameid%:*}
                mname=$(printf '%#H' "$mname")
                if [[ ${stid2pref[$st:$mid]} == *'|_'* ]] then
                    if [[ ${stid2pref[$st:$mid]} != *?'|'* ]] then
                        stid2pref[$st:$mid]=
                    fi
                fi
                stid2pref[$st:$mid]=${stid2pref[$st:$mid]#'|'}
                if (( cn != 1 )) then
                    print -r "<r>$st|$mid|$mname|${stmid2alarm[$st:$mid]}</r>"
                elif [[ ${stid2pref[$st:$mid]} == '' ]] then
                    print -rn "<r>$st|$mid|$mname"
                    print -r "|${stmid2alarm[$st:$mid]}</r>"
                else
                    print -rn "<r>$st|$mid|$mname (ALL)"
                    print -r "|${stmid2alarm[$st:$mid]}</r>"
                    pref=${stid2pref[$st:$mid]}
                    for suffix in $pref; do
                        [[ $suffix != _* ]] && continue
                        print -rn "<r>$st|$mid.$suffix|"
                        [[ $suffix == _main ]] && suffix=Main
                        [[ $suffix == _total ]] && suffix=Total
                        print -r "$mname ($suffix)|${stmid2alarm[$st:$mid]}</r>"
                    done
                    for attri in "${!attrs[@]}"; do
                        [[ $attri != @($pref)* ]] && continue
                        for suffix in $pref; do
                            [[ $attri == $suffix* ]] && break
                        done
                        print "$idre|$mid.${attri#"$suffix"}|${attrs[$attri]}||"
                    done | $SHELL vg_cm_thresholdhelper | sort -t'|' -k2,2 \
                    | while read -r line; do
                        attr=${line#*'|'}
                        label=${attr#*'|'}
                        label=${label%%'|'*}
                        attr=${attr%%'|'*}
                        if [[ $attr == *__LABEL__* ]] then
                            label=${attr%%__LABEL__*}
                            attr=${attr#*__LABEL__}
                        else
                            label=${label:-${attr#*.}}
                        fi
                        suffix=${attr#*.}
                        print -rn "<r>$st|$mid.$suffix"
                        print -r "|$mname ($label)|${stmid2alarm[$st:$mid]}</r>"
                    done
                fi
            done
        done
        print "</response>"
        exit 0
    fi

    print "<script>"
    print "var valuereq, valueres"
    print "function beginvaluehelper () {"
    print "  var s"
    print "  s = '$cgiprefix&mode=valuehelperhelper&helper=vg_cmdefvaluehelper'"
    print "  + '&file=threshold&path=$field&args=' + escape (getffield (0)) +"
    print "  '&dir=' + '${tmpdir#$SWIFTDATADIR}'"
    print "  if (window.XMLHttpRequest) {"
    print "    valuereq = new XMLHttpRequest ();"
    print "    if (valuereq.overrideMimeType)"
    print "      valuereq.overrideMimeType ('text/xml')"
    print "  } else if (window.ActiveXObject) {"
    print "    try {"
    print "      valuereq = new ActiveXObject ('Msxml2.XMLHTTP')"
    print "    } catch (e) {"
    print "      try {"
    print "        valuereq = new ActiveXObject ('Microsoft.XMLHTTP')"
    print "      } catch (e) {"
    print "      }"
    print "    }"
    print "  }"
    print "  if (!valuereq) {"
    print "    alert ('cannot create xml instance')"
    print "    return false"
    print "  }"
    print "  valuereq.onreadystatechange = endvaluehelper"
    print "  valuereq.open ('GET', s, true)"
    print "  valuereq.send (null)"
    print "}"
    print "function endvaluehelper () {"
    print "  var list, i, rs, pst, ri, r, t, label, value, og, o"
    print "  if (valuereq.readyState != 4)"
    print "    return"
    print "  if (valuereq.status != 200) {"
    print "    alert ('cannot execute query')"
    print "    return"
    print "  }"
    print "  valueres = valuereq.responseXML"
    print "  list = document.forms[0].list"
    print "  for (i = list.options.length - 1; i >= 0; i--)"
    print "    list.options[i] = null"
    print "  rs = valueres.getElementsByTagName ('r')"
    print "  pst = ''"
    print "  for (ri = 0; ri < rs.length; ri++) {"
    print "    if (!(r = rs[ri]))"
    print "      break"
    print "    t = r.firstChild.data.split ('|')"
    print "    if (t[0] != pst) {"
    print "      label = 'asset type: ' + t[0]"
    print "      og = document.createElement ('optgroup')"
    print "      og.value = label"
    print "      og.label = label"
    print "      list.appendChild (og)"
    print "      pst = t[0]"
    print "    }"
    print "    label = unescape (t[2])"
    print "    value = t[1] + '|' + unescape (t[3])"
    print "    o = new Option (label, value)"
    print "    list.options[list.options.length] = o"
    print "  }"
    print "}"
    print "function setsgroup (v) {"
    print "  var t"
    print "  t = v.split ('|')"
    print "  set (t[0])"
    print "  setffield (3, t[1])"
    print "}"
    print "</script>"

    print "<br><select"
    print "class=page name=list size=20"
    print "onChange='setsgroup (this.options[this.selectedIndex].value)'"
    print ">"
    print "</select>"
    print "<script>beginvaluehelper ()</script>"
    ;;
pname)
    . vg_hdr

    cgiprefix=$CGIPREFIX
    tmpdir=$TMPDIR

    if [[ $# == 3 ]] then
        typeset -A mid2name stmidmap stmid2alarm stid2pref sts attrs

        idre=${3//'+'/' '}
        idre=${idre//@([\'\\])/'\'\1}
        eval "idre=\$'${idre//'%'@(??)/'\x'\1"'\$'"}'"
        IFS='|'
        while read -r mid mname j; do
            mid2name[$mid]=${mname%%' ('*}
        done < $SWIFTDATADIR/dpfiles/stat/statlist.txt
        while read -r sl cst sst mid rep impl unit type alarm palarm estat; do
            [[ $cst == '' ]] && continue
            [[ $rep != y && $palarm == '' ]] && continue
            suffix=
            [[ $mid == *.* ]] && suffix=${mid#*.}
            if [[ ${mid2name[${mid%.*}]} != '' ]] then
                mid=${mid%.*}
            fi
            stmidmap[$cst:$mid]=${mid2name[$mid]:-${mid2name[${mid%.*}]:-$mid}}
            if [[ $impl == *'[['*']]'* ]] then
                impl=${impl#*'[['}
                impl=${impl%%']]'*}
                impl=${impl#*!P!}
                impl=${impl%!S!*}
                impl=${impl#!}
                impl=${impl%!*}
                if [[ "${stid2pref[$cst:$mid]}|" != *'|'$impl'|'* ]] then
                    stid2pref[$cst:$mid]+="|$impl"
                fi
                if [[ $impl == scopeinv* ]] then
                    impl=${impl//scopeinv_size_/si_sz_}
                    impl=${impl//scopeinv_/si_}
                    if [[ "${stid2pref[$cst:$mid]}|" != *'|'$impl'|'* ]] then
                        stid2pref[$cst:$mid]+="|$impl"
                    fi
                fi
            elif [[ $suffix != '' ]] then
                if [[ "${stid2pref[$cst:$mid]}|" != *'|'$suffix'|'* ]] then
                    stid2pref[$cst:$mid]+="|$suffix"
                fi
            fi
            stmid2alarm[$cst:$mid]=$(printf '%#H' "$palarm")
        done < $SWIFTDATADIR/dpfiles/stat/parameter.txt
        cn=0
        INVATTRKEY=scope_systype INVATTRSKIPNULL=scope_systype MAXN=200000 \
        SHOWRECS=1 SHOWATTRS=0 \
        $SHELL vg_dq_invhelper 'o' "$idre" | egrep '^B' \
        | while read -r t lv id st; do
            (( cn++ ))
            sts[$st]=1
        done
        set -s -A stlist -- ${!sts[@]}
        if (( cn == 1 )) then
            INVATTRSKIPNULL=name MAXN=200000 SHOWRECS=0 SHOWATTRS=1 \
            $SHELL vg_dq_invhelper 'o' "$idre" | egrep '^D.*\|(si|scopeinv)_' \
            | while read -r t lv id k v; do
                attrs[$k]=$v
            done
        fi
        print "Content-type: text/xml"
        print "Expires: Sat, 1 Jan 2000 00:00:00 EST\n"
        print "<response>"
        for st in "${stlist[@]}"; do
            unset mnamemidmap; typeset -A mnamemidmap
            for stmid in "${!stmidmap[@]}"; do
                [[ $stmid != $st:* ]] && continue
                mnamemidmap[${stmidmap[$stmid]}:${stmid#*:}]=1
            done
            print -r "<r>$st|allstats|All Stats|</r>"
            set -s -A mnameidlist -- ${!mnamemidmap[@]}
            for mnameid in "${mnameidlist[@]}"; do
                mid=${mnameid##*:}
                mname=${mnameid%:*}
                mname=$(printf '%#H' "$mname")
                if [[ ${stid2pref[$st:$mid]} == *'|_'* ]] then
                    if [[ ${stid2pref[$st:$mid]} != *?'|'* ]] then
                        stid2pref[$st:$mid]=
                    fi
                fi
                stid2pref[$st:$mid]=${stid2pref[$st:$mid]#'|'}
                if (( cn != 1 )) then
                    print -r "<r>$st|$mid|$mname|${stmid2alarm[$st:$mid]}</r>"
                elif [[ ${stid2pref[$st:$mid]} == '' ]] then
                    print -rn "<r>$st|$mid|$mname"
                    print -r "|${stmid2alarm[$st:$mid]}</r>"
                else
                    print -rn "<r>$st|$mid|$mname (ALL)"
                    print -r "|${stmid2alarm[$st:$mid]}</r>"
                    pref=${stid2pref[$st:$mid]}
                    for suffix in $pref; do
                        [[ $suffix != _* ]] && continue
                        print -rn "<r>$st|$mid.$suffix|"
                        [[ $suffix == _main ]] && suffix=Main
                        [[ $suffix == _total ]] && suffix=Total
                        print -r "$mname ($suffix)|${stmid2alarm[$st:$mid]}</r>"
                    done
                    for attri in "${!attrs[@]}"; do
                        [[ $attri != @($pref)* ]] && continue
                        for suffix in $pref; do
                            [[ $attri == $suffix* ]] && break
                        done
                        print "$idre|$mid.${attri#"$suffix"}|${attrs[$attri]}||"
                    done | $SHELL vg_cm_profilehelper | sort -t'|' -k2,2 \
                    | while read -r line; do
                        attr=${line#*'|'}
                        label=${attr#*'|'}
                        label=${label%%'|'*}
                        attr=${attr%%'|'*}
                        if [[ $attr == *__LABEL__* ]] then
                            label=${attr%%__LABEL__*}
                            attr=${attr#*__LABEL__}
                        else
                            label=${label:-${attr#*.}}
                        fi
                        suffix=${attr#*.}
                        print -rn "<r>$st|$mid.$suffix"
                        print -r "|$mname ($label)|${stmid2alarm[$st:$mid]}</r>"
                    done
                fi
            done
        done
        print "</response>"
        exit 0
    fi

    print "<script>"
    print "var valuereq, valueres"
    print "function beginvaluehelper () {"
    print "  var s"
    print "  s = '$cgiprefix&mode=valuehelperhelper&helper=vg_cmdefvaluehelper'"
    print "  + '&file=profile&path=$field&args=' + escape (getffield (0)) +"
    print "  '&dir=' + '${tmpdir#$SWIFTDATADIR}'"
    print "  if (window.XMLHttpRequest) {"
    print "    valuereq = new XMLHttpRequest ();"
    print "    if (valuereq.overrideMimeType)"
    print "      valuereq.overrideMimeType ('text/xml')"
    print "  } else if (window.ActiveXObject) {"
    print "    try {"
    print "      valuereq = new ActiveXObject ('Msxml2.XMLHTTP')"
    print "    } catch (e) {"
    print "      try {"
    print "        valuereq = new ActiveXObject ('Microsoft.XMLHTTP')"
    print "      } catch (e) {"
    print "      }"
    print "    }"
    print "  }"
    print "  if (!valuereq) {"
    print "    alert ('cannot create xml instance')"
    print "    return false"
    print "  }"
    print "  valuereq.onreadystatechange = endvaluehelper"
    print "  valuereq.open ('GET', s, true)"
    print "  valuereq.send (null)"
    print "}"
    print "function endvaluehelper () {"
    print "  var list, i, rs, pst, ri, r, t, label, value, og, o"
    print "  if (valuereq.readyState != 4)"
    print "    return"
    print "  if (valuereq.status != 200) {"
    print "    alert ('cannot execute query')"
    print "    return"
    print "  }"
    print "  valueres = valuereq.responseXML"
    print "  list = document.forms[0].list"
    print "  for (i = list.options.length - 1; i >= 0; i--)"
    print "    list.options[i] = null"
    print "  rs = valueres.getElementsByTagName ('r')"
    print "  pst = ''"
    print "  for (ri = 0; ri < rs.length; ri++) {"
    print "    if (!(r = rs[ri]))"
    print "      break"
    print "    t = r.firstChild.data.split ('|')"
    print "    if (t[0] != pst) {"
    print "      label = 'asset type: ' + t[0]"
    print "      og = document.createElement ('optgroup')"
    print "      og.value = label"
    print "      og.label = label"
    print "      list.appendChild (og)"
    print "      pst = t[0]"
    print "    }"
    print "    label = unescape (t[2])"
    print "    value = t[1] + '|' + unescape (t[3])"
    print "    o = new Option (label, value)"
    print "    list.options[list.options.length] = o"
    print "  }"
    print "}"
    print "function setsgroup (v) {"
    print "  var t"
    print "  t = v.split ('|')"
    print "  set (t[0])"
    print "  setffield (3, t[1])"
    print "}"
    print "</script>"

    print "<br><select"
    print "class=page name=list size=20"
    print "onChange='setsgroup (this.options[this.selectedIndex].value)'"
    print ">"
    print "</select>"
    print "<script>beginvaluehelper ()</script>"
    ;;
svalue)
    typeset -A mn2units havemaxvals sevid2name sevname2style
    ifs="$IFS"
    IFS='|'
    while read -r sl ast sst mn rep impl unit type alarm palarm estat; do
        [[ $sl == '' || $ast == '' || $sst == '' || $mn == '' ]] && continue
        if [[ ${mn2units[${mn%%.*}]} == '' ]] then
            mn2units[${mn%%.*}]=$unit
        elif [[ ${mn2units[${mn%%.*}]} != $unit ]] then
            mn2units[${mn%%.*}]=_many_
        fi
    done < $SWIFTDATADIR/dpfiles/stat/parameter.txt
    while read -r mid clabel flabel aggr maxval j; do
        [[ $maxval == @([0-9]*|y) ]] && havemaxvals[$mid]=1
    done < $SWIFTDATADIR/dpfiles/stat/statlist.txt
    while read -r sevid sevname; do
        sevid2name[$sevid]=$sevname
    done < $SWIFTDATADIR/dpfiles/alarm/sevlist.txt
    while read -r sevname cssstyle; do
        sevname2style[$sevname]="style='$cssstyle'"
    done < $SWIFTDATADIR/uifiles/dataqueries/alarmstyles.txt
    IFS="$ifs"

    print "<script>"
    print "function getsvalue () {"
    print "  var f = document.forms[0], fs"
    print "  var s, mn, mi, mj, v1, v2, vpflag"
    print "  var t, i, sev, vt, mint, min, per, maxt, max, ival"
    print "  s = get ()"
    print "  mn = getffield (1)"
    print "  t = mn.split ('.')"
    print "  mn = t[0]"
    print "  for (mi = 0; mi < mns.length; mi++) {"
    print "    if (mns[mi][0] == mn) {"
    for (( sevid = 1; sevid < 5; sevid++ )); do
        print "      fs = f.unit$sevid"
        print "      for (mj = 1; mj < mns[mi].length; mj++) {"
        print "        t = mns[mi][mj].split ('|')"
        print "        fs.options[fs.options.length] = new Option (t[1], t[0])"
        print "      }"
    done
    print "      break"
    print "    }"
    print "  }"
    print "  t = s.split (':')"
    print "  for (i = 0; i < t.length; ) {"
    print "    if (t[i] == 'CLEAR') {"
    print "      if (t.length - i < 3)"
    print "        break"
    print "      sev = t[i + 1]"
    print "      mint = t[i + 2].split ('/')"
    print "      if (mint.length != 2)"
    print "        break"
    print "      min = mint[0]"
    print "      per = mint[1]"
    print "      i += 3"
    print "      f.clearmin.value = min"
    print "    } else {"
    print "      if (t.length - i < 4)"
    print "        break"
    print "      v1 = ''"
    print "      v2 = ''"
    print "      if ("
    print "        t[i].search (/^\(.*-.*\)$/) != -1 ||"
    print "        t[i].search (/^\(.*-.*\]$/) != -1 ||"
    print "        t[i].search (/^\[.*-.*\)$/) != -1 ||"
    print "        t[i].search (/^\[.*-.*\]$/) != -1"
    print "      ) {"
    print "        vt = t[i].substr (1, t[i].length - 2).split ('-')"
    print "        if (vt.length != 2)"
    print "          break"
    print "        v1 = vt[0]"
    print "        v2 = vt[1]"
    print "      } else if (t[i].search (/^>=.*/) != -1) {"
    print "        v1 = t[i].substr (2)"
    print "      } else if (t[i].search (/^>.*/) != -1) {"
    print "        v1 = t[i].substr (1)"
    print "      } else if (t[i].search (/^<=.*/) != -1) {"
    print "        v2 = t[i].substr (2)"
    print "      } else if (t[i].search (/^<.*/) != -1) {"
    print "        v2 = t[i].substr (1)"
    print "      } else if (t[i].search (/^=.*/) != -1) {"
    print "        v1 = t[i].substr (1)"
    print "        v2 = v1"
    print "      } else {"
    print "        break"
    print "      }"
    print "      vpflag = false"
    print "      vt = v1.split ('%')"
    print "      if (vt.length == 2) {"
    print "        vpflag = true"
    print "        v1 = vt[0]"
    print "      }"
    print "      vt = v2.split ('%')"
    print "      if (vt.length == 2) {"
    print "        vpflag = true"
    print "        v2 = vt[0]"
    print "      }"
    print "      sev = t[i + 1]"
    print "      mint = t[i + 2].split ('/')"
    print "      if (mint.length != 2)"
    print "        break"
    print "      min = mint[0]"
    print "      per = mint[1]"
    print "      maxt = t[i + 3].split ('/')"
    print "      if (maxt.length != 2)"
    print "        break"
    print "      max = maxt[0]"
    print "      ival = maxt[1]"
    print "      if (max > 0 && ival > 0 && ival != 3600)"
    print "        max = (3600 / ival) * max"
    print "      i += 4"
    for (( sevid = 1; sevid < 5; sevid++ )); do
        print "      if (sev == $sevid) {"
        print "        f.v1$sevid.value = v1"
        print "        f.v2$sevid.value = v2"
        print "        f.min$sevid.value = min"
        print "        f.per$sevid.value = per"
        print "        f.max$sevid.value = max"
        print "        fs = f.unit$sevid"
        print "        if (fs.options.length > 1) {"
        print "          if (vpflag)"
        print "            fs.selectedIndex = 0"
        print "          else"
        print "            fs.selectedIndex = 1"
        print "        }"
        print "      }"
    done
    print "    }"
    print "  }"
    print "}"
    print "function setsvalue () {"
    print "  var f = document.forms[0], maxper = -1, s = ''"
    print "  var v1, v2, min, per, maxper, max, unit, clearmin"
    for (( sevid = 1; sevid < 5; sevid++ )); do
        print "  v1 = f.v1$sevid.value"
        print "  v2 = f.v2$sevid.value"
        print "  min = f.min$sevid.value"
        print "  per = f.per$sevid.value"
        print "  max = f.max$sevid.value"
        print "  unit = f.unit$sevid.options[f.unit$sevid.selectedIndex].value"
        print "  if (v1 != '' || v2 != '') {"
        print "    if (unit == '%') {"
        print "      if (v1 != '')"
        print "        v1 += '%'"
        print "      if (v2 != '')"
        print "        v2 += '%'"
        print "    }"
        print "    if (min == '')"
        print "      min = 2"
        print "    if (per == '' || per < min)"
        print "      per = min"
        print "    if (maxper < per)"
        print "      maxper = per"
        print "    if (max == '')"
        print "      max = 0"
        print "    if (s != '')"
        print "      s += ':'"
        print "    if (v1 == '')"
        print "      s += '<=' + v2"
        print "    else if (v2 == '')"
        print "      s += '>=' + v1"
        print "    else if (v1 == v2)"
        print "      s += '=' + v1"
        print "    else"
        print "      s += '[' + v1 + '-' + v2 + ']'"
        print "    s += ':$sevid:' + min + '/' + per + ':' + max + '/3600'"
        print "  }"
    done
    print "  clearmin = f.clearmin.value"
    print "  if (clearmin == '' || clearmin < maxper)"
    print "    clearmin = maxper"
    print "  if (s != '')"
    print "    s += ':CLEAR:5:' + clearmin + '/' + clearmin"
    print "  set (s)"
    print "}"
    print "var mns = new Array ()"
    i=0
    for mn in "${!mn2units[@]}"; do
        unit=${mn2units[$mn]}
        unset vs
        if [[ ${havemaxvals[$mn]} == 1 ]] then
            vs[${#vs[@]}]="%|percent"
        fi
        if [[ $unit != '' && $unit != _many_ && $unit != '%' ]] then
            vs[${#vs[@]}]="$unit|actual ($unit)"
        elif [[ $unit != '%' ]] then
            vs[${#vs[@]}]="|actual"
        fi
        # workaround when parameter.txt and statlist.txt don't agree on %
        if [[ ${#vs[@]} == 0 ]] then
            vs[${#vs[@]}]="|actual"
        fi
        print "mns[$i] = new Array (${#vs[@]} + 1)"
        print "mns[$i][0] = '$mn'"
        for vi in "${!vs[@]}"; do
            print "mns[$i][$vi + 1] = '${vs[$vi]}'"
        done
        (( i++ ))
    done
    print "</script>"

    print "<div width=100% class=page>"
    print "<input"
    print "class=page type=button value='Insert/Modify' onClick='setsvalue ()'"
    print ">"
    print "</div>"
    st="style='border-style:solid; border-width:1px; border-collapse:collapse'"
    for (( sevid = 1; sevid < 5; sevid++ )); do
        print "<table class=page $st>"
        print "<tr class=page>"
        print "<td class=page ${sevname2style[${sevid2name[$sevid]}]}>"
        print "severity: ${sevid2name[$sevid]}"
        print "</td><td class=page></td></tr><tr class=page><td class=page>"
        print "<a"
        print "class=pagelabel"
        print "title='leave min and max fields empty to skip this severity'"
        print ">value range for this severity (min,max):</a>"
        print "</td><td class=page>"
        print "<input class=page type=text size=6 name=v1$sevid>"
        print ","
        print "<input class=page type=text size=6 name=v2$sevid>"
        print "<select class=page name=unit$sevid>"
        print "</select>"
        print "</td></tr><tr class=page><td class=page>"
        print "<a"
        print "class=pagelabel"
        print "title='ex: 2/3 means 2 matches / 3 consecutive intervals'"
        print "># of matches for first alarm:</a>"
        print "</td><td class=page>"
        print "<input class=page type=text size=3 name=min$sevid>"
        print "/"
        print "<input class=page type=text size=3 name=per$sevid>"
        print "cycles"
        print "</td></tr><tr class=page><td class=page>"
        print "<a"
        print "class=pagelabel"
        print "title='leave blank to only send 1 alarm per incident'"
        print ">repeat alarm:</a>"
        print "</td><td class=page>"
        print "<input class=page type=text size=3 name=max$sevid>"
        print "times / hour"
        print "</td></tr>"
        print "</table>"
    done
    print "<table class=page $st>"
    print "<tr class=page><td class=page ${sevname2style[clear]}>"
    print "alarm clear"
    print "</td><td class=page></td></tr><tr class=page><td class=page>"
    print "<a"
    print "class=pagelabel"
    print "title='ex: 3/3 means wait for 3 consecutive intervals to clear'"
    print ">clear if no matches for:</a>"
    print "</td><td class=page>"
    print "<input class=page type=text size=3 name=clearmin>"
    print "cycles"
    print "</td></tr>"
    print "</table>"
    print "<script>getsvalue ()</script>"
    ;;
pvalue)
    typeset -A mn2units havemaxvals sevid2name sevname2style
    ifs="$IFS"
    IFS='|'
    while read -r sl ast sst mn rep impl unit type alarm palarm estat; do
        [[ $sl == '' || $ast == '' || $sst == '' || $mn == '' ]] && continue
        if [[ ${mn2units[${mn%%.*}]} == '' ]] then
            mn2units[${mn%%.*}]=$unit
        elif [[ ${mn2units[${mn%%.*}]} != $unit ]] then
            mn2units[${mn%%.*}]=_many_
        fi
    done < $SWIFTDATADIR/dpfiles/stat/parameter.txt
    while read -r mid clabel flabel aggr maxval j; do
        [[ $maxval == @([0-9]*|y) ]] && havemaxvals[$mid]=1
    done < $SWIFTDATADIR/dpfiles/stat/statlist.txt
    while read -r sevid sevname; do
        sevid2name[$sevid]=$sevname
    done < $SWIFTDATADIR/dpfiles/alarm/sevlist.txt
    while read -r sevname cssstyle; do
        sevname2style[$sevname]="style='$cssstyle'"
    done < $SWIFTDATADIR/uifiles/dataqueries/alarmstyles.txt
    IFS="$ifs"

    print "<script>"
    print "function getsvalue () {"
    print "  var f = document.forms[0], fs"
    print "  var s, mn, mi, mj, v1, v2, vpflag"
    print "  var t, i, mode, sev, vt, min, per, max, ival"
    print "  s = get ()"
    print "  mn = getffield (1)"
    print "  t = mn.split ('.')"
    print "  mn = t[0]"
    print "  for (mi = 0; mi < mns.length; mi++) {"
    print "    if (mns[mi][0] == mn) {"
    print "      fs = f.vrunit"
    print "      for (mj = 1; mj < mns[mi].length; mj++) {"
    print "        t = mns[mi][mj].split ('|')"
    print "        fs.options[fs.options.length] = new Option (t[1], t[0])"
    print "      }"
    print "      break"
    print "    }"
    print "  }"
    print "  t = s.split (':')"
    print "  for (i = 0; i < t.length; ) {"
    print "    if (t[i] == 'VR') {"
    print "      if (t.length - i < 5)"
    print "        break"
    print "      v1 = t[i + 2]"
    print "      v2 = t[i + 4]"
    print "      vpflag = false"
    print "      vt = v1.split ('%')"
    print "      if (vt.length == 2) {"
    print "        vpflag = true"
    print "        v1 = vt[0]"
    print "      }"
    print "      vt = v2.split ('%')"
    print "      if (vt.length == 2) {"
    print "        vpflag = true"
    print "        v2 = vt[0]"
    print "      }"
    print "      i += 5"
    print "      f.vr1.value = v1"
    print "      f.vr2.value = v2"
    print "      fs = f.vrunit"
    print "      if (fs.options.length > 1) {"
    print "        if (vpflag)"
    print "          fs.selectedIndex = 0"
    print "        else"
    print "          fs.selectedIndex = 1"
    print "      }"
    print "    } else if (t[i] == 'SIGMA' || t[i] == 'ACTUAL') {"
    print "      if (t.length - i < 10)"
    print "        break"
    print "      mode = t[i]"
    print "      v1 = t[i + 2]"
    print "      v2 = t[i + 4]"
    print "      vpflag = false"
    print "      vt = v1.split ('%')"
    print "      if (vt.length == 2) {"
    print "        vpflag = true"
    print "        v1 = vt[0]"
    print "      }"
    print "      vt = v2.split ('%')"
    print "      if (vt.length == 2) {"
    print "        vpflag = true"
    print "        v2 = vt[0]"
    print "      }"
    print "      sev = t[i + 5]"
    print "      min = t[i + 6]"
    print "      per = t[i + 7]"
    print "      max = t[i + 8]"
    print "      ival = t[i + 9]"
    print "      if (max > 0 && ival > 0 && ival != 3600)"
    print "        max = (3600 / ival) * max"
    print "      i += 10"
    for (( sevid = 1; sevid < 5; sevid++ )); do
        print "      if (sev == $sevid) {"
        print "        f.v1$sevid.value = v1"
        print "        f.v2$sevid.value = v2"
        print "        f.min$sevid.value = min"
        print "        f.per$sevid.value = per"
        print "        f.max$sevid.value = max"
        print "        fs = f.unit$sevid"
        print "        if (mode == 'SIGMA')"
        print "          fs.selectedIndex = 0"
        print "        else if (vpflag)"
        print "          fs.selectedIndex = 2"
        print "        else"
        print "          fs.selectedIndex = 1"
        print "      }"
    done
    print "    } else if (t[i] == 'CLEAR') {"
    print "      if (t.length - i < 4)"
    print "        break"
    print "      sev = t[i + 1]"
    print "      min = t[i + 2]"
    print "      per = t[i + 3]"
    print "      i += 4"
    print "      f.clearmin.value = min"
    print "    } else {"
    print "      break"
    print "    }"
    print "  }"
    print "}"
    print "function setsvalue () {"
    print "  var f = document.forms[0], maxper = -1, s = ''"
    print "  var v1, v2, min, per, maxper, max, unit, clearmin"
    print "  unit = f.vrunit.options[f.vrunit.selectedIndex].value"
    print "  v1 = f.vr1.value"
    print "  v2 = f.vr2.value"
    print "  if (v1 != '' || v1 != '') {"
    print "    if (unit == '%') {"
    print "      if (v1 != '')"
    print "        v1 += '%'"
    print "      if (v2 != '')"
    print "        v2 += '%'"
    print "    }"
    print "    s += 'VR'"
    print "    if (f.vr1.value)"
    print "      s += ':i:' + v1"
    print "    else"
    print "      s += ':n:'"
    print "    if (f.vr2.value)"
    print "      s += ':i:' + v2"
    print "    else"
    print "      s += ':n:'"
    print "  }"
    for (( sevid = 1; sevid < 5; sevid++ )); do
        print "  v1 = f.v1$sevid.value"
        print "  v2 = f.v2$sevid.value"
        print "  min = f.min$sevid.value"
        print "  per = f.per$sevid.value"
        print "  max = f.max$sevid.value"
        print "  unit = f.unit$sevid.options[f.unit$sevid.selectedIndex].value"
        print "  if (v1 != '' || v2 != '') {"
        print "    if (unit == 'PACTUAL') {"
        print "      if (v1 != '')"
        print "        v1 += '%'"
        print "      if (v2 != '')"
        print "        v2 += '%'"
        print "      unit = 'ACTUAL'"
        print "    }"
        print "    if (min == '')"
        print "      min = 2"
        print "    if (per == '' || per < min)"
        print "      per = min"
        print "    if (maxper < per)"
        print "      maxper = per"
        print "    if (max == '')"
        print "      max = 0"
        print "    if (s != '')"
        print "      s += ':'"
        print "    s += unit"
        print "    if (v1 != '')"
        print "      s += ':i:' + v1"
        print "    else"
        print "      s += ':n:'"
        print "    if (v2 != '')"
        print "      s += ':i:' + v2"
        print "    else"
        print "      s += ':n:'"
        print "    s += ':$sevid:' + min + ':' + per + ':' + max + ':3600'"
        print "  }"
    done
    print "  clearmin = f.clearmin.value"
    print "  if (clearmin == '' || clearmin < maxper)"
    print "    clearmin = maxper"
    print "  if (s != '')"
    print "    s += ':CLEAR:5:' + clearmin + ':' + clearmin"
    print "  set (s)"
    print "}"
    print "var mns = new Array ()"
    i=0
    for mn in "${!mn2units[@]}"; do
        unit=${mn2units[$mn]}
        unset vs
        if [[ ${havemaxvals[$mn]} == 1 ]] then
            vs[${#vs[@]}]="%|percent"
        fi
        if [[ $unit != '' && $unit != _many_ && $unit != '%' ]] then
            vs[${#vs[@]}]="$unit|actual ($unit)"
        elif [[ $unit != '%' ]] then
            vs[${#vs[@]}]="|actual"
        fi
        # workaround when parameter.txt and statlist.txt don't agree on %
        if [[ ${#vs[@]} == 0 ]] then
            vs[${#vs[@]}]="|actual"
        fi
        print "mns[$i] = new Array (${#vs[@]} + 1)"
        print "mns[$i][0] = '$mn'"
        for vi in "${!vs[@]}"; do
            print "mns[$i][$vi + 1] = '${vs[$vi]}'"
        done
        (( i++ ))
    done
    print "</script>"

    print "<div class=page>"
    print "<input"
    print "class=page type=button value='Insert/Modify' onClick='setsvalue ()'"
    print ">"
    print "</div>"
    st="style='border-style:solid; border-width:1px; border-collapse:collapse'"
    print "<table class=page $st>"
    print "<tr class=page><td class=page>"
    print "<a"
    print "class=pagelabel"
    print "title='only specify values to exclude very high/low values'"
    print ">value range for this rule (min,max):</a>"
    print "</td><td class=page>"
    print "<input class=page type=text size=6 name=vr1>"
    print ","
    print "<input class=page type=text size=6 name=vr2>"
    print "<select class=page name=vrunit>"
    print "</select>"
    print "</td></tr>"
    print "</table>"

    for (( sevid = 1; sevid < 5; sevid++ )); do
        print "<table class=page $st>"
        print "<tr class=page>"
        print "<td class=page ${sevname2style[${sevid2name[$sevid]}]}>"
        print "severity: ${sevid2name[$sevid]}"
        print "</td><td class=page></td></tr><tr class=page><td class=page>"
        print "<a"
        print "class=pagelabel"
        print "title='leave min and max fields empty to skip this severity'"
        print ">profile deviation for this severity (min,max):</a>"
        print "</td><td class=page>"
        print "<input class=page type=text size=6 name=v1$sevid>"
        print ","
        print "<input class=page type=text size=6 name=v2$sevid>"
        print "<select class=page name=unit$sevid>"
        print "<option class=page value='SIGMA'>st.dev"
        print "<option class=page value='ACTUAL'>actual"
        print "<option class=page value='PACTUAL'>%actual"
        print "</select>"
        print "</td></tr><tr class=page><td class=page>"
        print "<a"
        print "class=pagelabel"
        print "title='ex: 2/3 means 2 matches / 3 consecutive intervals'"
        print "># of matches for first alarm:</a>"
        print "</td><td class=page>"
        print "<input class=page type=text size=3 name=min$sevid>"
        print "/"
        print "<input class=page type=text size=3 name=per$sevid>"
        print "cycles"
        print "</td></tr><tr class=page><td class=page>"
        print "<a"
        print "class=pagelabel"
        print "title='leave blank to only send 1 alarm per incident'"
        print ">repeat alarm:</a>"
        print "</td><td class=page>"
        print "<input class=page type=text size=3 name=max$sevid>"
        print "times / hour"
        print "</td></tr>"
        print "</table>"
    done
    print "<table class=page $st>"
    print "<tr class=page><td class=page ${sevname2style[clear]}>"
    print "alarm clear"
    print "</td><td class=page></td></tr><tr class=page><td class=page>"
    print "<a"
    print "class=pagelabel"
    print "title='ex: 3/3 means wait for 3 consecutive intervals to clear'"
    print ">clear if no matches for:</a>"
    print "</td><td class=page>"
    print "<input class=page type=text size=3 name=clearmin>"
    print "cycles"
    print "</td></tr>"
    print "</table>"
    print "<script>getsvalue ()</script>"
    ;;
esac
print "</form></div>"
print "<b>Help</b>"
print "<p>Click on an entry to transfer its value to the field in the main form"

print "</html>"
