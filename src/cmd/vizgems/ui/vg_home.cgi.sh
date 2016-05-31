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

home_data=(
    pid=''
    bannermenu='link|home||Home|reload this home page|list|favorites||Favorites|run a favorite query|list|tools|p:c|Tools|tools|list|help|userguide:homepage|Help|documentation|link|logout||Logout|terminate this session'
)

function home_init {
    home_data.pid=$pid

    return 0
}

function home_emit_header_begin {
    print '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 TRANSITIONAL//EN">'
    print "<html>"
    print "<head>"

    ph_emitheader page banner bdiv mdiv
    print "<title>${ph_data.title}</title>"

    return 0
}

function home_emit_header_end {
    print "</head>"

    return 0
}

function home_emit_body_begin {
    typeset vt st img

    print "<body>"
    ph_emitbodyheader ${home_data.pid} banner 1600

    return 0
}

function home_emit_body_end {
    ph_emitbodyfooter
    print "</body>"
    print "</html>"

    return 0
}

function home_helper {
    typeset lastentry line rest groups ids name query qid params custs url

    if [[ -x $SWIFTDATADIR/uifiles/homehelper.sh ]] then
        . $SWIFTDATADIR/uifiles/homehelper.sh "$@"
        return 0
    fi

    case $1 in
    sectionorder)
        print "alarmsummary quicklinks queries adminlinks news"
        ;;
    quicklinks)
        lastentry=line
        while read -r line; do
            if [[ $line == LINE ]] then
                if [[ $lastentry != line ]] then
                    print "<tr class=page><td class=page>"
                    print -r "<hr style='width:100%; height:1px'>"
                    print "</td></tr>"
                fi
                lastentry=line
                continue
            elif [[ $line == HTML* ]] then
                print -r "${line#HTML}"
                lastentry=html
                continue
            fi
            [[ $line == \#* || $line != *\|* ]] && continue
            rest=$line
            groups=${rest%%\|*}
            rest=${rest#"$groups"\|}
            ids=${rest%%\|*}
            rest=${rest#"$ids"\|}
            name=${rest%%\|*}
            rest=${rest#"$name"\|}
            query=${rest%%\|*}
            rest=${rest#"$query"\|}
            qid=${rest%%\|*}
            rest=${rest#"$qid"\|}
            params=${rest%%\|*}
            rest=${rest#"$params"\|}
            if [[ $groups != '' ]] && ! swmuseringroups "${groups//:/'|'}"; then
                continue
            fi
            if [[ $ids != '' && :$ids: != *:$SWMID:* ]] then
                continue
            fi
            if [[ $params == *HIGHALERTCUST* ]] then
                custs=$(sed 's/ .*//' $SWIFTDATADIR/uifiles/highalertcusts.txt)
                custs=${custs//$'\n'/';'}
                params=${params//HIGHALERTCUST/$custs}
            fi
            url=$(home_gendqurl "$query" "$qid" "$params")
            print "<tr class=page><td class=page>"
            print -r "<a"
            print -r "  class=page href='javascript:vg_runurl(\"$url\")'"
            print -r "  title='click to run this query'"
            print -r ">&nbsp;$name&nbsp;</a>"
            print "</td></tr>"
            lastentry=query
        done < $SWIFTDATADIR/uifiles/homequeries.txt
        ;;
    esac
}

function home_gendqurl { # $1 = query $2 = qid $3 = params
    print -rn "$SWIFTWEBPREFIX/cgi-bin-vg-members/vg_dserverhelper.cgi"
    print -rn "?pid=${home_data.pid}&update=5"
    print -r "&query=$1&qid=$2&$(printf '%#H' "$3")"
}

function home_genfqurl {
    print -rn "$SWIFTWEBPREFIX/cgi-bin-vg-members/vg_dserverhelper.cgi"
    print -rn "?pid=${home_data.pid}"
    print -r "&query=filterquery"
}

function home_genfmurl {
    print -rn "$SWIFTWEBPREFIX/cgi-bin-vg-members/vg_filemanager.cgi"
    print -r "?pid=${home_data.pid}&mode=list"
}

function home_gencmurl {
    print -rn "$SWIFTWEBPREFIX/cgi-bin-vg-members/vg_confmgr.cgi"
    print -r "?pid=${home_data.pid}&mode=list"
}

function home_genamurl {
    case $1 in
    admin)
        print -rn "$SWIFTWEBPREFIX/cgi-bin-vg-members/vg_accountadmin.cgi"
        print -r "?pid=${home_data.pid}&action=form"
        ;;
    user)
        print -rn "$SWIFTWEBPREFIX/cgi-bin-vg-members/vg_accountuser.cgi"
        print -r "?pid=${home_data.pid}&action=form"
        ;;
    esac
}

function home_genwqurl {
    print -rn "$SWIFTWEBPREFIX/cgi-bin-vg-members/vg_dserverhelper.cgi"
    print -rn "?pid=${home_data.pid}"
    print -r "&query=wusagequery"
}

if [[ $SWIFTWARNLEVEL != "" ]] then
    PS4='${.sh.fun}-$LINENO: '
    typeset -ft home_init
    typeset -ft home_emit_body_begin home_emit_body_end
    typeset -ft home_emit_header_begin home_emit_header_end
    typeset -ft home_gendqrul home_genfqurl home_genfmurl home_gencmurl
    typeset -ft home_genamurl home_genwqurl
fi

. vg_hdr
. vg_prefshelper
. $SWIFTDATADIR/dpfiles/config.sh

suireadcgikv

case $VG_SYSMODE in
ems) group=vg_hevonems ;;
viz) group=vg_hevonweb ;;
esac

tabind=1

print "Content-type: text/html\n"

pid=default
if [[ $pid == '' ]] then
    print -u2 SWIFT ERROR configuration not specified
    exit 1
fi
ph_init $pid "${home_data.bannermenu}"

home_init
home_emit_header_begin
home_emit_header_end

home_emit_body_begin

print "<div class=pagemain><table class=page>"

sections=$(home_helper sectionorder)

for section in $sections; do
    case $section in
    alarmsummary)
        if swmuseringroups "vg_cus_user|vg_cus_admin"; then
            HIDEINFO=y $SHELL vg_dq -mode embed -qid customerhome | read dir
            dir=${dir#*' '}
            dir=${dir#*' '}

            print "<tr class=page>"
            print "<td class=pageborder style='text-align:right'>"
            print "<a"
            print "  class=pagelabel href='javascript:void(0)'"
            print "  title='counts of open alarms'"
            print ">&nbsp;dashboard&nbsp;</a>"
            if [[ $SHOWHELP == 1 ]] then
                print "<a"
                print "  class=pageil href='javascript:void(0)'"
                print "  onClick='vg_showhelp(\"quicktips/homealarmsum\")'"
                print "  title='click to read about dashboard'"
                print ">&nbsp;?&nbsp;</a>"
            fi
            print "</td>"
            print "<td class=pageborder>"
            cat $dir/rep.out
            print "</td>"
            print "</tr>"
        fi
        ;;
    quicklinks)
        if swmuseringroups "vg_att_admin|$group"; then
            print "<tr class=page>"
            print "<td class=pageborder style='text-align:right'>"
            print "<a"
            print "  class=pagelabel href='javascript:void(0)'"
            print "  title='useful queries'"
            print ">&nbsp;quick links&nbsp;</a>"
            if [[ $SHOWHELP == 1 ]] then
                print "<a"
                print "  class=pageil href='javascript:void(0)'"
                print "  onClick='vg_showhelp(\"quicktips/homequicklinks\")'"
                print "  title='click to read about quick links'"
                print ">&nbsp;?&nbsp;</a>"
            fi
            print "</td>"
            print "<td class=pageborder>"
            print "<table class=page>"
            home_helper quicklinks
            print "</table>"
            print "</td>"
            print "</tr>"
        fi
        ;;
    queries)
        if swmuseringroups "vg_att_admin|$group"; then
            print "<tr class=page>"
            print "<td class=pageborder style='text-align:right'>"
            print "<a"
            print "  class=pagelabel href='javascript:void(0)'"
            print "  title='standard queries'"
            print ">&nbsp;queries&nbsp;</a>"
            if [[ $SHOWHELP == 1 ]] then
                print "<a"
                print "  class=pageil href='javascript:void(0)'"
                print "  onClick='vg_showhelp(\"quicktips/homequeries\")'"
                print "  title='click to read about these queries'"
                print ">&nbsp;?&nbsp;</a>"
            fi
            print "</td>"
            print "<td class=pageborder>"
            print "<table class=page>"
            typeset -A lvnames
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
                lvnames[$lv]=$fn
            done
            egrep -v '^#|^$' $SWIFTDATADIR/uifiles/dataqueries/queries.txt \
            | while read -r line; do
                ls=${line%%'|'*}; rest=${line##"$ls"?('|')}
                us=${rest%%'|'*}; rest=${rest##"$us"?('|')}
                gs=${rest%%'|'*}; rest=${rest##"$gs"?('|')}
                qs=${rest%%'|'*}; rest=${rest##"$qs"?('|')}
                ns=${rest%%'|'*}; rest=${rest##"$ns"?('|')}
                ds=${rest%%'|'*}; rest=${rest##"$ds"?('|')}
                hs=${rest%%'|'*}; rest=${rest##"$hs"?('|')}
                if [[ $hs == no || $hs != *:*:*:* ]] then
                    continue
                fi
                h1=${hs%%:*}
                h2=${hs#*:}; h2=${h2%%:*}
                h3=${hs#*:}; h3=${h3#*:}; h3=${h3%%:*}
                h4=${hs##*:}
                if [[ $us != '' && " $SWMID " != @(*\ (${us//:/'|'})\ *) ]] then
                    continue
                fi
                if [[ $gs != '' ]] && ! swmuseringroups "${gs//:/'|'}"; then
                    continue
                fi
                print "<tr class=page><td class=page>"
                url=$(home_gendqurl "dataquery" "$qs" "")
                print "<script>"
                print "function hq_run_$qs (e) {"
                print "  var url, pel, lel, lv, id"
                print "  if (e != null && e.keyCode != 13)"
                print "    return false"
                print -r "  url = \"$url\""
                print "  lel = document.getElementById ('list_$qs')"
                print "  pel = document.getElementById ('param_$qs')"
                lv=$h2
                lvname=${lvnames[$lv]:-$lv}
                print "  lv = '$lv'"
                print "  id = pel.value"
                print "  if (lel && lel.selectedIndex != 0)"
                print "    id = lel.options[lel.selectedIndex].value"
                print "  if (id.length > 0) {"
                print "    id = id.replace (/\|/, '.*|.*')"
                print "    url += '&lname_$lv=' + escape ('.*' + id + '.*')"
                if [[ $h3 == y ]] then
                    print "  } else {"
                    print "    alert ('please enter a specific $lvname first')"
                    print "    return false"
                else
                    print "  } else {"
                    print "    url += '&lname_$lv=' + escape ('.*')"
                fi
                print "  }"
                print "  url += '&tryasid=y'"
                print "  return vg_runurl (url)"
                print "}"
                print "</script>"
                print "<a"
                print "  class=page href='javascript:void(0)'"
                print "  onClick='return hq_run_$qs(null)'"
                if [[ $h3 == y ]] then
                    print -n "  title='"
                    print -n "first enter the name of a specific $lvname in the"
                    print " adjacent field then click here to run this query'"
                else
                    print -n "  title='"
                    print -n "click to run this query -"
                    print -n " optionally enter the name"
                    print " of a specific $lvname in the adjacent field'"
                fi
                name="&nbsp;show&nbsp;${ns//' '/'&nbsp;'}&nbsp;"
                print -r ">$name</a>"
                print "</td><td class=page>"
                print "<input"
                print "  tabindex=$tabind class=page type=text"
                print "  id=param_$qs size=22"
                if [[ $h3 == y ]] then
                    print -n "  title='"
                    print -n "mandatory - type the name of"
                    print -n " a specific $lvname in this"
                    print -n " field then click on the adjacent button"
                    print " to run this query'"
                else
                    print -n "  title='"
                    print -n "optional - type the name of"
                    print -n " a specific $lvname in this"
                    print -n " field then click on the adjacent button"
                    print " to run this query'"
                fi
                print "  onKeyUp='hq_run_$qs(event)'"
                print ">"
                (( tabind++ ))
                print "</td><td class=page>"
                if [[ $h4 == y ]] then
                    print "<select"
                    print "  tabindex=$tabind class=page"
                    print "  class=page id=list_$qs style='width:20em'"
                    print "  title='select an item from this list'"
                    print "  onChange='hq_run_$qs(null)'"
                    print ">"
                    print "<option selected class=page value=''>$lvname list"
                    SHOWRECS=1 $SHELL vg_dq_invhelper -lv $lv \
                    | sort -t'|' -u -k4,4 | while read line; do
                        [[ $line != B* ]] && continue
                        name=${line#*'|'}
                        name=${name#*'|'}
                        name=${name#*'|'}
                        val=${name//@([\[\]\(\)\{\}\^\$\|\*\.])/\\\1}
                        print "<option class=page value='$val'>$name"
                    done
                    print "</select>"
                fi
                print "</td></tr>"
            done
            print "</table>"
            print "</td>"
            print "</tr>"
        fi
        ;;
    adminlinks)
        if \
            swmuseringroups "vg_att_admin|vg_att_user|vg_confedi*|vg_fileedit" \
            || swmuseringroups "vg_passchange|vg_docview|vg_webusage" \
        ; then
            print "<tr class=page>"
            print "<td class=pageborder style='text-align:right'>"
            print "<a"
            print "  class=pagelabel href='javascript:void(0)'"
            print "  title='various administrative tools'"
            print ">&nbsp;admin links&nbsp;</a>"
            if [[ $SHOWHELP == 1 ]] then
                print "<a"
                print "  class=pageil href='javascript:void(0)'"
                print "  onClick='vg_showhelp(\"quicktips/homeadminlinks\")'"
                print "  title='click to read about admin tools'"
                print ">&nbsp;?&nbsp;</a>"
            fi
            print "</td>"
            print "<td class=pageborder>"
            print "<table class=page>"
            if swmuseringroups "vg_att_admin|vg_confeditfilter"; then
                print "<tr class=page><td class=page>"
                print "<table class=page>"
                print "<tr class=page><td class=page>"
                url=$(home_genfqurl)
                print "<script>"
                print "function hq_run_filemanager (e) {"
                print "  var url, el, date"
                print "  if (e != null && e.keyCode != 13)"
                print "    return"
                print -r "  url = \"$url\""
                print "  el = document.getElementById ('filter_date')"
                print "  date = el.value"
                print "  if (date.length > 0) {"
                print "    url += '&fdate=' + escape (date)"
                print "  }"
                print "  return vg_runurl (url)"
                print "}"
                print "</script>"
                print "<a"
                print "  class=page href='javascript:void(0)'"
                print "  onClick='return hq_run_filemanager(null)'"
                print -n "  title='"
                print -n "show currently active filters -"
                print " specify optional different date in the adjacent field'"
                print -r ">&nbsp;show Filters&nbsp;</a>"
                print "</td><td class=page>"
                print "<input"
                print "  tabindex=$tabind class=page type=text"
                print "  id=filter_date size=22"
                print -n "  title='optional - enter date (YYYY.MM.DD-hh:mm:ss)"
                print "  in this field before clicking on the adjacent button'"
                print "  onKeyUp='hq_run_filemanager(event)'"
                print ">"
                (( tabind++ ))
                print "</td></tr>"
                print "</table></td></tr>"
            fi
            if \
                [[ $VG_SYSMODE == ems ]] && \
                swmuseringroups "vg_att_admin|vg_fileedit"; \
            then
                url=$(home_genfmurl)
                print "<tr class=page><td class=page><a"
                print "  class=page href='javascript:void(0)'"
                print "  onClick='return vg_runurl(\"$url\")'"
                print "  title='click to run the file manager tool'"
                print ">&nbsp;run File Manager&nbsp;</a></td></tr>"
            fi
            if swmuseringroups "vg_att_admin|vg_confedit*"; then
                url=$(home_gencmurl)
                print "<tr class=page><td class=page><a"
                print "  class=page href='javascript:void(0)'"
                print "  onClick='return vg_runurl(\"$url\")'"
                print "  title='click to run the configuration manager tool'"
                print ">&nbsp;run Configuration Manager&nbsp;</a></td></tr>"
            fi
            if swmuseringroups "vg_att_admin|vg_passchange"; then
                if swmuseringroups "vg_att_admin"; then
                    url=$(home_genamurl admin)
                    print "<tr class=page><td class=page><a"
                    print "  class=page href='javascript:void(0)'"
                    print "  onClick='return vg_runurl(\"$url\")'"
                    print "  title='click to run the account manager tool'"
                    print ">&nbsp;run Account Manager&nbsp;</a></td></tr>"
                fi
                url=$(home_genamurl user)
                print "<tr class=page><td class=page><a"
                print "  class=page href='javascript:void(0)'"
                print "  onClick='return vg_runurl(\"$url\")'"
                print "  title='click to run the password manager tool'"
                print ">&nbsp;run Password Manager&nbsp;</a></td></tr>"
            fi
            if swmuseringroups "vg_att_admin|vg_webusage"; then
                url=$(home_genwqurl)
                print "<tr class=page><td class=page><a"
                print "  class=page href='javascript:void(0)'"
                print "  onClick='return vg_runurl(\"$url\")'"
                print "  title='click to run the web usage tool'"
                print ">&nbsp;run Web Usage Tool&nbsp;</a></td></tr>"
            fi
            print "</table>"
            print "</td>"
            print "</tr>"
        fi
        ;;
    news)
        print "<tr class=page>"
        print "<td class=pageborder style='text-align:right'>"
        print "<a"
        print "  class=pagelabel href='javascript:void(0)'"
        print "  title='news about this website'"
        print ">&nbsp;news&nbsp;</a>"
        if [[ $SHOWHELP == 1 ]] then
            print "<a"
            print "  class=pageil href='javascript:void(0)'"
            print "  onClick='vg_showhelp(\"quicktips/homenews\")'"
            print "  title='click to read about news'"
            print ">&nbsp;?&nbsp;</a>"
        fi
        print "</td>"
        print "<td class=pageborder>"
        lastentry=line
        while read -r line; do
            if [[ $line == LINE ]] then
                if [[ $lastentry != line ]] then
                    print -r "<hr style='width:100%; height:1px'>"
                fi
                lastentry=line
                continue
            elif [[ $line == HTML* ]] then
                print -r "${line#HTML}"
                lastentry=html
                continue
            fi
            [[ $line == \#* || $line != *\|* ]] && continue
            rest=$line
            groups=${rest%%\|*}
            rest=${rest#"$groups"\|}
            ids=${rest%%\|*}
            rest=${rest#"$ids"\|}
            file=${rest%%\|*}
            rest=${rest#"$file"\|}
            subject=${rest%%\|*}
            rest=${rest#"$subject"\|}
            if [[ $groups != '' ]] && ! swmuseringroups "${groups//:/'|'}"; then
                continue
            fi
            if [[ $ids != '' && :$ids: != *:$SWMID:* ]] then
                continue
            fi
            if [[ -f $SWIFTDATADIR/uifiles/news/$file ]] then
                print -r "<b>$subject</b><br>"
                cat $SWIFTDATADIR/uifiles/news/$file
            fi
            lastentry=query
        done < $SWIFTDATADIR/uifiles/news/news.list
        print "</td>"
        print "</tr>"
        ;;
    *)
        home_helper $section
        ;;
    esac
done

print "</table></div>"

home_emit_body_end
