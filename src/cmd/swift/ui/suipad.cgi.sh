#!/bin/ksh

export SWMROOT=${DOCUMENT_ROOT%/htdocs}
. $SWMROOT/bin/swmenv
[[ $SECONDS != *.* ]] && exec $SHELL $0 "$@"
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

group=default

suireadcgikv

if [[ $qs_prefprefix == '' ]] then
    print -u2 "ERROR configuration not specified"
    exit 1
fi

. $SWIFTDATADIR$qs_prefprefix.sh
for dir in ${PATH//:/ }; do
    if [[ -f $dir/${instance}_prefshelper ]] then
        . $dir/${instance}_prefshelper
        break
    fi
done

typeset -n x=$instance.dserver.$group.transmission
case ${x} in
Uncompressed) dtype=u ;;
Compressed) dtype=c ;;
esac
typeset +n x

rhost=$(swmhostname)
if [[ $SWMADDRMAPFUNC != '' ]] then
    mrhost=$($SWMADDRMAPFUNC)
    if [[ $mrhost == '' ]] then
        mrhost=$rhost
    else
        SWIFTWEBSERVER=${SWIFTWEBSERVER//$rhost/$mrhost}
    fi
else
    mrhost=$rhost
fi
${instance}_services start
${instance}_services list r | read j1 j2 j3 rport j4
${instance}_services list d-$dtype | read j1 j2 j3 dport j4
dname=$mrhost

rkey=${qs_sessionid##*\;}

for ((SECONDS = 0; SECONDS < 120; )) do
    ${instance}_services query "" "" SWIFTMSERVER $rkey \
    | read mname mport j1 j2
    if [[ $mname != '' ]] then
        break
    fi
    sleep 1
done
if [[ $mname == '' ]] then
    exit 1
fi
if [[ $SWMADDRMAPFUNC != '' ]] then
    mmname=$($SWMADDRMAPFUNC)
    if [[ $mmname != '' ]] then
        mname=$mmname
    fi
fi

xname=$instance.pad.$group
typeset -n x=$xname
typeset -A seen

print "Content-type: text/html\n"
print '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 TRANSITIONAL//EN">'
print "<html>"
print "<head>"
print "<style type='text/css'>"
print "BODY {"
print "  font-family: ${x.viewfontname}; font-size: ${x.viewfontsize};"
print "  background: ${x.viewcolor%%\ *}; color: ${x.viewcolor##*\ };"
print "}"
for var in "${!x.class_@}"; do
    tc=${var##$xname.class_}
    tc=${tc%_style}
    if [[ $tc == *_* ]] then
        type=${tc%%_*}
        class=${tc#${type}_}
        eval print "$type.$class { \${x.class_${type}_${class}_style} }"
    else
        type=$tc
        eval print "$type { \${x.class_${type}_style} }"
    fi
done
print "</style>"
if [[ ${x.showtitle} == 1 ]] then
    if [[ ${x.title} == __RUNFUNC__ ]] then
        x.title=$(${instance}_${group}_pad_gentitle)
    fi
    print "<title>${x.title}</title>"
fi
print "<script src=$SWIFTWEBPREFIX/swift/suipad.js></script>"
print "<script src=$SWIFTWEBPREFIX/proj/$instance/$instance.js></script>"
print "<script language=javascript>"
print "var suipad_user = '$SWMID'"
print "var suipad_xname = 'screenX'"
print "var suipad_yname = 'screenY'"
if [[ $HTTP_USER_AGENT == *MSIE* ]] then
    print "var suipad_wname = 'width'"
    print "var suipad_hname = 'height'"
else
    print "var suipad_wname = 'innerWidth'"
    print "var suipad_hname = 'innerHeight'"
fi
print "var suitable_x = new Array ()"
print "var suitable_y = new Array ()"
print "var suitable_w = new Array ()"
print "var suitable_h = new Array ()"
typeset -n y=$instance.table
for entry in "${!y.@}"; do
    group2=${entry#$instance.table.}
    group2=${group2%%.*}
    if [[ ${seen[$group2]} == 1 ]] then
        continue
    fi
    seen[$group2]=1
    typeset -n z=$instance.table.$group2
    xy=${z.vieworigin}
    if [[ $xy != '' ]] then
        print "suitable_x['${group2}']=${xy%%\ *}"
        print "suitable_y['${group2}']=${xy##*\ }"
    fi
    xy=${z.viewsize}
    if [[ $xy != '' ]] then
        print "suitable_w['${group2}']=${xy%%\ *}"
        print "suitable_h['${group2}']=${xy##*\ }"
    fi
done
print "var suipad_instance = '${instance}'"
yn=${x.showselparam}
print "var suipad_showselparam = '$yn'"
print "var suipad_wcgi = '$SWIFTWEBPREFIX/cgi-bin-${instance}-members'"
print "var suipad_wdoc = '$SWIFTWEBPREFIX/proj/${instance}'"
print "var suipad_mname = '$mname'"
print "var suipad_mport = '$mport'"
print "var suipad_dname = '$dname'"
print "var suipad_dport = '$dport'"
print "var suipad_prefprefix = '$qs_prefprefix'"
print "var suipad_ddir = '$SWIFTDATADIR'"
print "if (typeof (${instance}_exit) != 'undefined')"
print "  var suipad_ds_exit = ${instance}_exit"
print "if (typeof (${instance}_allowinsert) != 'undefined')"
print "  var suipad_ds_allowinsert = ${instance}_allowinsert"
print "if (typeof (${instance}_loadset) != 'undefined')"
print "  var suipad_ds_loadset = ${instance}_loadset"
print "if (typeof (${instance}_unloadset) != 'undefined')"
print "  var suipad_ds_unloadset = ${instance}_unloadset"

print "</script>"
print "</head>"
print "<body>"
if [[ ${x.showheader} == 1 ]] then
    if [[ ${x.header} == __RUNFUNC__ ]] then
        x.header=$(${instance}_${group}_pad_genheader)
    fi
    print "${x.header}"
fi

typeset -A rlist
typeset -A clist
integer rown=0
integer coln=0

if [[ ${x.rowhelper} != '' ]] then
    . ${x.rowhelper}
fi

for var in "${!x.row@}"; do
    rowi=${var##$xname.row}
    if [[ $rowi == *col* ]] then
        coli=${rowi##*col}
        rowi=${rowi%col*}
        typeset -n y=$xname.row${rowi}col${coli}
        clist[$rowi:$coli]=${y}
    else
        coli=0
        typeset -n y=$xname.row${rowi}
        rlist[$rowi]=${y}
    fi
    if (( $rowi >= $rown )) then
        (( rown = $rowi + 1 ))
    fi
    if (( $coli >= $coln )) then
        (( coln = $coli + 1 ))
    fi
done

print "<div align=center>"
print "<form"
print "  action=${instance}_pad.cgi name=padform method=post"
print "  onSubmit='return false'"
print ">"
print "<input type=hidden name=mport value=$mport>"
print "<input type=hidden name=mname value=$mname>"

if [[
    ${x.showdatesel} == 1 || ${x.showupdatesel} == 1 ||
    ${x.showviewsel} == 1 || ${x.showcancelall} == 1
]] then
    print "<table border=0><tr>"
    if [[ ${x.showdatesel} == 1 ]] then
        print "<td align=left><div align=left>"
        print "  <input type=hidden name=dateval>"
        print "  Select Day<br><input"
        print "    width=200 type=button name=date value=\"latest\""
        print "    onClick='suipad_selectdate ()'"
        print "  >"
        print "</div></td>"
    fi
    if [[ ${x.showupdatesel} == 1 ]] then
        print "<td align=center><div align=center>"
        print "  Update<br><select"
        print "    name=update onChange='suipad_settimer ()'"
        print "  >"
        print "    <option value=-1>Never"
        print "    <option value=0>Now"
        print "    <option value=1>1 Min"
        print "    <option value=2>2 Min"
        print "    <option value=3>3 Min"
        print "    <option value=4>4 Min"
        print "    <option value=5 selected>5 Min"
        print "    <option value=10>10 Min"
        print "    <option value=15>15 Min"
        print "    <option value=30>30 Min"
        print "    <option value=60>60 Min"
        print "  </select>"
        print "</div></td>"
    fi
    if [[ ${x.showviewsel} == 1 ]] then
        print "<td align=right><div align=right>"
        print "  <input type=hidden name=viewval>"
        print "  <input type=hidden name=viewname>"
        print "  Select Customer<br>"

        print "/name2id.map" | ${instance}_dserverhelper | read custfile
        view0=$(egrep '\|0$' $custfile)
        view0=${view0%\|*}
        ncust=$(wc -l < $custfile)

        if (( ncust < 50 )) then
            print "  <input type=hidden name=view>"
            print "  <select"
            print "    name=viewsel onChange='suipad_setview \c"
            print "    (\c"
            print "      this.options[this.selectedIndex].text,\c"
            print "      this.options[this.selectedIndex].value\c"
            print "    )'"
            print "  >"
            if [[ $view0 != '' ]] then
                print "    <option value=0>$view0"
                viewval=0
                viewname=$view0
            fi
            egrep -v '\|0$' $custfile | sort -t '|' -k1 \
            | while IFS='|' read cust val; do
                if [[ $viewval == '' ]] then
                    viewval=$val
                    viewname=$cust
                fi
                print "    <option value=$val>$cust"
            done
            print "  </select>"
        else
            print "  <input"
            print "    width=200 type=button name=view"
            print "    value=\"$view0\""
            print "    onClick='suipad_selectview ()'"
            print "  >"
            viewval=0
            viewname="$view0"
        fi
        print "</div></td>"
    fi
    if [[ ${x.showcancelall} == 1 ]] then
        print "<td align=left><div align=left>"
        print "  <input type=hidden name=cancellall>"
        print "  Active Requests<br><input"
        print "    width=200 type=button name=date value=\"cancel all\""
        print "    onClick='suipad_cancelall ()'"
        print "  >"
        print "</div></td>"
    fi
    print "</tr></table>"
fi

inrow=n
for (( rowi = 0; rowi < rown; rowi++ )) do
    rdata=${rlist[$rowi]}
    if [[ $rdata == "" ]] then
        continue
    fi
    case $rdata in
    _break_*)
        if [[ $inrow == y ]] then
            print "</table><br><table border=1>"
        fi
        ;;
    _label_*)
        if [[ $inrow == n ]] then
            print "<table border=1>"
            inrow=y
        fi
        rest=${rdata#_label_\;}
        span=${rest%%\;*}
        rest=${rest#"$span"}
        rest=${rest#\;}
        label=${rest%%\;}
        print "<tr><th colspan=$((span + 1))>$label</th></tr>"
        ;;
    _normal_*)
        if [[ $inrow == n ]] then
            print "<table border=1>"
            inrow=y
        fi
        rest=${rdata#_normal_\;}
        span=${rest%%\;*}
        rest=${rest#"$span"}
        rest=${rest#\;}
        label=${rest%%\;}
        rest=${rest#"$label"}
        rest=${rest#\;}
        print "<tr>"
        print "<td colspan=$span>$label</td>"
        for (( coli = 0; coli < coln; coli++ )) do
            if [[ ${clist[$rowi:$coli]} == "" ]] then
                continue
            fi
            cdata=${clist[$rowi:$coli]}
            type=${cdata%%\;*}
            rest=${cdata#"$type"}
            rest=${rest#\;}
            span=${rest%%\;*}
            rest=${rest#"$span"}
            rest=${rest#\;}
            case $type in
            _blank_*)
                print "<td colspan=$span></td>"
                ;;
            _load_*)
                var=${rest%%\;*}
                rest=${rest#"$var"}
                rest=${rest#\;}
                label=${rest%%\;*}
                rest=${rest#"$label"}
                rest=${rest#\;}
                attr=${rest%%\;*}
                rest=${rest#"$attr"}
                rest=${rest#\;}
                print "<td colspan=$span><input"
                print "  type=button name=$var value=\"$label\""
                print "  onClick=\"suipad_loadset (this)\" $attr"
                print "></td>"
                ;;
            _button_*)
                var=${rest%%\;*}
                rest=${rest#"$var"}
                rest=${rest#\;}
                label=${rest%%\;*}
                rest=${rest#"$label"}
                rest=${rest#\;}
                js=${rest%%\;*}
                rest=${rest#"$js"}
                rest=${rest#\;}
                attr=${rest%%\;*}
                rest=${rest#"$attr"}
                rest=${rest#\;}
                print "<td colspan=$span><input"
                print "  type=button name=$var value=\"$label\""
                print "  onClick=\"$js\" $attr"
                print "></td>"
                ;;
            _text_*)
                var=${rest%%\;*}
                rest=${rest#"$var"}
                rest=${rest#\;}
                label=${rest%%\;*}
                rest=${rest#"$label"}
                rest=${rest#\;}
                width=${rest%%\;*}
                rest=${rest#"$width"}
                rest=${rest#\;}
                text=${rest%%\;*}
                rest=${rest#"$text"}
                rest=${rest#\;}
                attr=${rest%%\;*}
                rest=${rest#"$attr"}
                rest=${rest#\;}
                print "<td colspan=$span>$label<input"
                print "  type=text name=$var value=\"$text\""
                print "  size=$width $attr"
                print "></td>"
                ;;
            _choice_*)
                var=${rest%%\;*}
                rest=${rest#"$var"}
                rest=${rest#\;}
                label=${rest%%\;*}
                rest=${rest#"$label"}
                rest=${rest#\;}
                attr=${rest%%\;*}
                rest=${rest#"$attr"}
                rest=${rest#\;}
                print "<td colspan=$span>$label<select"
                print "  name=$var"
                print "  $attr"
                print ">"
                while [[ $rest != '' ]] do
                    val=${rest%%\;*}
                    rest=${rest#"$val"}
                    rest=${rest#\;}
                    label=${rest%%\;*}
                    rest=${rest#"$label"}
                    rest=${rest#\;}
                    print "  <option value=\"$val\">$label"
                done
                print "</select></td>"
                ;;
            esac
        done
        print "</tr>"
        ;;
    esac
done
if [[ $inrow == y ]] then
    print "</table>"
fi

print "</form></div>"

tools2launch=
for tool in $tools; do
    lang=${tool#*:}
    tool=${tool%:*}
    if [[ $lang != java ]] then
        continue
    fi
    tools2launch="$tools2launch $tool"
done
if [[ ${x.showupdatesel} == 1 ]] then
    print "<script language=javascript>suipad_settimer ()</script>"
fi
if [[ $tools2launch != '' ]] then
    export SWMPREFPREFIX=$SWIFTDATADIR$qs_prefprefix
    jsui_starttools $instance $dname $dport $mname $mport $tools2launch
fi

if [[ ${x.showfooter} == 1 ]] then
    if [[ ${x.footer} == __RUNFUNC__ ]] then
        x.footer=$(${instance}_${group}_pad_genfooter)
    fi
    print "${x.footer}"
fi

if [[ ${x.showviewsel} == 1 ]] then
    print "<script language=javascript>"
    print "suipad_view = $viewval"
    print "suipad_viewname = \"$viewname\""
    print "</script>"
fi

typeset +n x

print "</body>"
print "</html>"
