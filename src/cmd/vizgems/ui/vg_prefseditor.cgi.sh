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

function queueins {
    typeset file=$1 rfile=$2 ofile=$3
    typeset ts pfile sn
    ts=$(printf '%(%Y%m%d-%H%M%S)T')
    sn=$VG_SYSNAME
    pfile=cm.$ts.$file.${rfile//./_X_X_}.$sn.$sn.$RANDOM.full.xml
    pfile=$VG_DSYSTEMDIR/incoming/cm/$pfile
    (
        print "<a>full</a>"
        print "<u>$sn</u>"
        print "<f>"
        cat $ofile
        print "</f>"
    ) > $pfile.tmp && mv $pfile.tmp $pfile
}

pe_data=(
    pid=''
    bannermenu='link|home||Home|return to the home page|list|help|userguide:prefseditorpage|Help|documentation'
)

function pe_init {
    pe_data.pid=$pid

    return 0
}

function pe_emit_header_begin {
    print '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 TRANSITIONAL//EN">'
    print "<html>"
    print "<head>"

    ph_emitheader page banner bdiv
    print "<title>${pe_data.title}</title>"
    print "<script src='$SWIFTWEBPREFIX/proj/vg/vg_prefseditor.js'></script>"

    return 0
}

function pe_emit_header_end {
    print "</head>"

    return 0
}

function pe_emit_body_begin {
    typeset vt st img

    print "<body>"
    ph_emitbodyheader ${pe_data.pid} banner $qs_winw

    return 0
}

function pe_emit_body_end {
    ph_emitbodyfooter
    print "</body>"
    print "</html>"

    return 0
}

if [[ $SWIFTWARNLEVEL != "" ]] then
    PS4='${.sh.fun}-$LINENO: '
    typeset -ft pe_init
    typeset -ft pe_emit_body_begin pe_emit_body_end
    typeset -ft pe_emit_header_begin pe_emit_header_end
fi

. vg_hdr
. vg_prefshelper
. $SWIFTDATADIR/dpfiles/config.sh

suireadcgikv

ill='*([#A-Za-z0-9_.-])'
qslist=" $qslist "
qsarrays=" $qsarrays "
for i in $qslist; do
    k=qs_$i
    if [[ $k == *+([!a-zA-Z0-9_])* ]] then
        print -r -u2 "SWIFT ERROR illegal key in parameters"
        unset $k
        qslist=${qslist//' '$i' '/' '}
        qsarrays=${qsarrays//' '$i' '/' '}
        continue
    fi
    typeset -n v=$k
    if [[ " "$qsarrays" " == *' '$i' '* ]] then
        for (( j = 0; j < ${#v[@]}; j++ )) do
            vl=${v[$j]}
            vl=${vl//+([[:space:]])/}
            if [[ $vl != $ill ]] then
                print -r -u2 "SWIFT ERROR illegal value for key $i"
                unset $k
                qslist=${qslist//' '$i' '/' '}
                qsarrays=${qsarrays//' '$i' '/' '}
                continue
            fi
        done
    else
        vl=$v
        vl=${vl//+([[:space:]])/}
        if [[ $vl != $ill ]] then
            print -r -u2 "SWIFT ERROR illegal value for key $i"
            unset $k
            qslist=${qslist//' '$i' '/' '}
            qsarrays=${qsarrays//' '$i' '/' '}
            continue
        fi
    fi
done
qslist=${qslist//' '+(' ')/' '}
qslist=${qslist#' '}
qslist=${qslist%' '}
qsarrays=${qsarrays//' '+(' ')/' '}
qsarrays=${qsarrays#' '}
qsarrays=${qsarrays%' '}

pid=$qs_pid
if [[ $pid == '' ]] then
    print -u2 SWIFT ERROR configuration not specified
    exit 1
fi

ph_init $pid "${pe_data.bannermenu}"

if swmuseringroups 'vg_cus_*'; then
    print "Content-type: text/html\n"
    print "<html><body><b><font color=red>"
    print "You are not authorized to access this page"
    print "</font></b></body></html>"
    exit 0
fi

if [[ $SUIPREFGENUSERFILE != '' ]] then
    userfile=$SUIPREFGENUSERFILE
else
    userfile=$SWMDIR/$instance.$pid.prefs
fi

if [[ $qs_mode == submit ]] then
    for (( count = 0; ; count++ )) do
        typeset -n name=qs_map$count
        typeset -n value=qs_field$count
        [[ $name == '' ]] && break
        printf "%s=%q\n" "$name" "$value"
    done > $userfile.tmp && mv $userfile.tmp $userfile
    queueins preferences $SWMID.sh $userfile
fi

print "Content-type: text/html\n"

pe_init
pe_emit_header_begin
pe_emit_header_end

pe_emit_body_begin

print "<script>"
print "function resetvalues () {"
print "  location = 'vg_prefseditor.cgi?pid=$qs_pid&winw=$qs_winw&winh=$qs_winh&mode=list&reset=yes'"
print "}"
print "</script>"

print "<div class=pagemain align=center>"
print "<form"
print "  method=post action=vg_prefseditor.cgi"
print "  enctype=multipart/form-data"
print ">"
print "<input type=submit value=Save>"
print "<input type=button value=Reset onClick='resetvalues ()'>"
print "<input type=button value=Close onClick='window.close ()'>"
print "<input type=hidden name=pid value='$pid'>"
print "<input type=hidden name=winw value='$qs_winw'>"
print "<input type=hidden name=winh value='$qs_winh'>"
print "<input type=hidden name=mode value=submit>"

count=0
groupID=0
fontVal="Arial, Comic Sans MS, Courier New, Helvetica, Impact, Lucida Grande, Tahoma, Times New Roman, Verdana, sans-serif"
fontsizeVal="8pt 9pt 10pt 11pt 12pt 14pt 16pt 18pt 20pt 22pt 24pt 26pt 28pt 36pt 48pt 72pt"
statsizeVal="100 120 140 150 160 180 200 220 240 250 260 280 300 320 340 350 360 380 400 420 440 450 460 480 500 520 540 550 560 580 600 620 640 650 660 680 700 720 740 750 760 780 800 820 840 850 860 880 900 920 940 950 960 980 1000"

for tool in $SWIFTTOOLS; do
    for dir in ${PATH//:/ }; do
        if [[ -f $dir/../lib/prefs/$tool.prefs ]] then
            . $dir/../lib/prefs/$tool.prefs
            break
        fi
    done
done
for dir in ${PATH//:/ }; do
    if [[ -f $dir/../lib/prefs/$instance.prefs ]] then
        . $dir/../lib/prefs/$instance.prefs
        break
    fi
done
if [[ $SUIPREFGENSYSTEMFILE != '' ]] then
    if [[ -f $SUIPREFGENSYSTEMFILE ]] then
        . $SUIPREFGENSYSTEMFILE
    fi
fi
if [[ $qs_reset != yes ]] then
    . $userfile
fi

for tool in $SWIFTTOOLS; do
    typeset -n x=$tool
    print "<p><b>${x.label}</b></p>"
    for group in ${x.groups}; do
        [[ $group == general ]] && continue
        typeset -n y=$tool.$group
        print "<p>"
        print "<table style='border:1px #369 solid;border-collapse: collapse'>"
        print "<tr><th style='background:#369;color:#FFF;text-align:left;padding:2px 5px;' onclick='showHideGRP(\"GRP$groupID\"); return false;'><b>${y.label}</b></th></tr>"
        print "<tr><td><div id=GRP$groupID>"

        str="<table cellspacing='0'>\n"
        for entry in ${y.entrylist}; do
            epath=$tool.$group.entries.$entry
            typeset -n z=$tool.$group.entries.$entry
            str+="<tr><td>${z.label}:</td><td>\n"

            case ${z.type} in
                yesno)
                    ycheck= ncheck=
                    if [[ ${z.val} == yes ]] then
                        ycheck=checked
                    else
                        ncheck=checked
                    fi
                    str+="<input type=radio $ycheck name='radio${count}' value='yes' onclick='set_radio_val(\"radio${count}\", \"field${count}\")'>Yes"
                    str+="<input type=radio $ncheck name='radio${count}' value='no' onclick='set_radio_val(\"radio${count}\", \"field${count}\")'>No"
                    str+="<input type=hidden name=field$count id=field$count value='${z.val}'>"
                    ;;
                choice)
                    ifs="$IFS"
                    IFS='|'
                    str+="<select size='1' id=field${count}G onchange='set_graphsize_val(\"field${count}\")'>"
                    for tmpgval in ${z.choices}; do
                        tmpgval=${tmpgval##+(' ')}
                        tmpgval=${tmpgval%%+(' ')}
                        key=${tmpgval%%:*}
                        desc=${tmpgval##*:}
                        if [[ ${z.val} == $key ]] then
                            sel=selected
                        else
                           sel=
                        fi
                        str+="<option value='${key}' $sel>$desc</option>"
                    done
                    str+="<input type=hidden name=field$count id=field$count value='${z.val}'>"
                    IFS="$ifs"
                    ;;
                timezone)
                    str+="<select name=field$count id='timeSel'>"
                    str+="<script type='text/javascript'>"
                    str+="setTimeVal('${z.val}');"
                    str+="</script>"
                    str+="</select>"
                    str+="<div id='timeDiv'></div>"
                    ;;
                fontfamily)
                    ifs="$IFS"
                    IFS='|'
                    str+="<select size='1' id=field${count}G onchange='set_graphsize_val(\"field${count}\")'>"
                    for tmpgval in ${z.choices}; do
                        tmpgval=${tmpgval##+(' ')}
                        tmpgval=${tmpgval%%+(' ')}
                        key=${tmpgval%%:*}
                        desc=${tmpgval##*:}
                        if [[ ${z.val} == $key ]] then
                            sel=selected
                        else
                            sel=
                        fi
                        str+="<option value='${key}' $sel>$desc</option>"
                    done
                    str+="<input type=hidden name=field$count id=field$count value='${z.val}'>"
                    IFS="$ifs"
                    ;;
                fontsize)
                    i=0
                    str+="<select size='1' name=field$count>\n"
                    for fontsizeCurVal in $fontsizeVal; do
                        if [[ ${z.val} == $fontsizeCurVal ]] then
                            sel=selected
                        else
                            sel=
                        fi
                        str+="<option value='${fontsizeCurVal}' $sel>$fontsizeCurVal</option>"
                    done
                    str+="</select>"
                    ;;
                color)
                    str+="<a href='javascript:void(0)' onclick='showHide(\"BGfield$count\",\"stylecol${z.val}$count\",\"field$count\",1); return false;' style='border:1px solid #369; font-size:8pt; cursor:pointer; text-decoration:none'>"
                    str+="<input type=text size=10 id=BGfield$count style='background-color:${z.val};font-size:8;' align='absmiddle' readonly>"
                    str+="<img id='dropIcon$count' border='0' src='$SWIFTWEBPREFIX/proj/vg/vg_ralxpbtn_o.gif' onmouseout='document.getElementById(\"dropIcon$count\").src=\"$SWIFTWEBPREFIX/proj/vg/vg_ralxpbtn_o.gif\"' onmouseover='document.getElementById(\"dropIcon$count\").src=\"$SWIFTWEBPREFIX/proj/vg/vg_ralxpbtn_p.gif\"') align='absmiddle'></a>"
                    str+="<div id=\"stylecol${z.val}$count\" style='top:0px;border:1px solid navy;display: none;'></div>"
                    str+="<input type=hidden name=field$count id=field$count value='${z.val}'>"
                    ;;
                size)
                    windir="Width"
                    zvaltmp=""
                    for windirVal in ${z.val}; do
                        if [[ $windir == "Width" ]] then
                            str+="Width: <select size='1' id=field${count}W onchange='set_viewsize_val(\"field$count\")'>"
                            for dirVal in $statsizeVal; do
                                if [[ $windirVal == $dirVal ]] then
                                    str+="<option value='${dirVal}' selected>$dirVal</option>"
                                else
                                    str+="<option value='${dirVal}'>$dirVal</option>"
                                fi
                            done
                            str+="</select>"
                            windir="Height"
                        else
                            str+="Height: <select size='1' id=field${count}H onchange='set_viewsize_val(\"field$count\")'>"
                            for dirVal in $statsizeVal; do
                                if [[ $windirVal == $dirVal ]] then
                                    str+="<option value='${dirVal}' selected>$dirVal</option>"
                                else
                                    str+="<option value='${dirVal}'>$dirVal</option>"
                                fi
                            done
                            str+="</select>"
                        fi
                    done
                    str+="<input type=hidden name=field$count id=field$count value='${z.val}'>"
                    ;;
                *)
                    str+="<input type=text name=field${count} value='${z.val}'>"
                    ;;
            esac
            str+="<input type=hidden name=map$count value=$epath.val>"
            str+="</td></tr>"
            (( count++ ))
        done
        str+="</table>"
        print "${str}"
        print "</div></td></tr>"
        (( groupID++ ))
        print "</table></p>"
    done
done
print "</form>"
print "</div>"

pe_emit_body_end
