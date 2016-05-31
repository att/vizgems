
if [[ $SWIFTWARNLEVEL != '' ]] then
    set -x
fi

if ! swmuseringroups "vg_att_admin|vg_webusage"; then
    print -u2 "SWIFT ERROR vg_wusage_run security violation user: $SWMID"
    print "<b><font color=red>"
    print "You are not authorized to access this page"
    print "</font></b>"
    exit
fi

wuq_data=(
    pid=''
    bannermenu='link|home||Home|return to the home page|link|back||Back|go back one page|list|tools|p:c|Tools|tools|list|help|userguide:wusagepage|Help|documentation'
)

function wuq_init {
    wuq_data.pid=$pid

    return 0
}

function wuq_emit_header_begin {
    print '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 TRANSITIONAL//EN">'
    print "<html>"
    print "<head>"

    ph_emitheader page banner bdiv
    print "<title>${ph_data.title}</title>"

    return 0
}

function wuq_emit_header_end {
    print "</head>"

    return 0
}

function wuq_emit_body_begin {
    typeset vt st img

    print "<body>"
    ph_emitbodyheader ${wuq_data.pid} banner 1600

    return 0
}

function wuq_emit_body_end {
    ph_emitbodyfooter
    print "</body>"
    print "</html>"

    return 0
}

if [[ $SWIFTWARNLEVEL != "" ]] then
    PS4='${.sh.fun}-$LINENO: '
    typeset -ft wuq_init
    typeset -ft wuq_emit_body_begin wuq_emit_body_end
    typeset -ft wuq_emit_header_begin wuq_emit_header_end
fi

. vg_hdr
. vg_prefshelper
. $SWIFTDATADIR/dpfiles/config.sh

suireadkv 0 vars

. $SWMROOT/proj/vg/bin/vg_env

logfile=$SWMROOT/logs/access_log
if [[ ! -f $logfile ]] then
    print "<font color=red><b>"
    print "    ERROR cannot find access log file: $logfile"
    print "</b></font>"
    print -u2 SWIFT ERROR cannot find access log file: $logfile
    exit 1
fi

dayn=${vars.days:-30}

pid=${vars.pid}
if [[ $pid == '' ]] then
    print -u2 SWIFT ERROR configuration not specified
    exit 1
fi
ph_init $pid "${wuq_data.bannermenu}"
style='style="font-size:80%"'

wuq_init
wuq_emit_header_begin
wuq_emit_header_end

wuq_emit_body_begin

names="home dataquery rest filterquery clearalarm"
names+=" prefseditor export mail filemanager confmgr"
names+=" accountadmin accountuser help misc"
labels="Home Data<br>Query REST<br> Filter<br>Query Clear<br>Alarm"
labels+=" Pref<br>Editor Exports Emails File<br>Manager Conf<br>Mgr"
labels+=" Account<br>Admin Account<br>User Help Misc"
typeset -A rdates users tcs dtcs udtcs fcs dfcs udfcs

prevdate=
dayi=0
{
    for i in 5 4 3 2 1; do
        [[ -f $logfile.$i ]] && cat $logfile.$i
    done
    cat $logfile
} | rev -l | egrep '/vg_|/rest' \
| sutawk '{ print $4,$3,$(NF-1),$7 }' | sed -E \
    -e 's/^\[//' -e 's/^(...........):..:..:../\1/' \
| while read -r date user res url; do
    if [[ $prevdate != $date ]] then
        if (( dayi++ >= dayn )) then
            cat > /dev/null
            break
        fi
        prevdate=$date
    fi

    name=
    path=${url%%\?*}
    case $path in
    */vg_home.cgi)
        name=home
        ;;
    */vg_accountadmin.cgi)
        name=accountadmin
        ;;
    */vg_accountuser.cgi)
        name=accountuser
        ;;
    */vg_filemanager.cgi)
        name=filemanager
        ;;
    */vg_prefseditor.cgi)
        name=prefseditor
        ;;
    */vg_confmgr.cgi)
        name=confmgr
        ;;
    */vg_exporthelper.cgi)
        if [[ $url == *type=email* ]] then
            name=mail
        else
            name=export
        fi
        ;;
    */vg_help.cgi)
        name=help
        ;;
    */vg_dserverhelper.cgi)
        if [[ $url == *qid=clearalarm* ]] then
            name=clearalarm
        elif [[ $url == *query=filterquery* ]] then
            name=filterquery
        else
            name=dataquery
        fi
        ;;
    */rest/*)
        name=rest
        ;;
    *)
        name=misc
        ;;
    esac

    [[ $name == '' ]] && continue
    [[ $res != 200 ]] && continue

    [[ ${rdates[$date]} == '' ]] && dates[${#dates[@]}]=$date
    rdates[$date]=$date
    users[$user]=$user

    (( tcs[$name]++ ))
    (( dtcs[$date:$name]++ ))
    (( udtcs[$user:$date:$name]++ ))
    (( tcs[ALL]++ ))
    (( dtcs[$date:ALL]++ ))
    (( udtcs[$user:$date:ALL]++ ))
done

prevdate=
prevapachedate=
logfile=$SWMROOT/logs/user_log
{
    for i in 5 4 3 2 1; do
        [[ -f $logfile.$i ]] && cat $logfile.$i
    done
    [[ -f $logfile ]] && cat $logfile
} | rev -l \
| while read -r line; do
    date=${line##*T=}
    date=${date%%' '*}
    date=${date%%-*}
    if [[ $prevdate != $date ]] then
        apachedate=${ printf '%(%d/%b/%Y)T' "$date"; }
        if [[ ${rdates[$apachedate]} == '' ]] then
            cat > /dev/null
            break
        fi
        prevdate=$date
        prevapachedate=$apachedate
    fi

    user=${line##*U=}
    user=${user%%' '*}
    user=${user%%-*}
    [[ $user == '' ]] && continue

    src=${line##*S=}
    src=${src%%' '*}
    src=${src%%:*}
    case $src in
    rest)
        name=rest
        ;;
    *)
        name=dataquery
        ;;
    esac

    users[$user]=$user

    (( udtcs[-:$apachedate:$name]-- ))
    (( udtcs[-:$apachedate:$name] <= 0 )) && udtcs[-:$apachedate:$name]=
    (( udtcs[-:$apachedate:ALL]-- ))
    (( udtcs[-:$apachedate:ALL] <= 0 )) && udtcs[-:$apachedate:ALL]=
    (( udtcs[$user:$apachedate:$name]++ ))
    (( udtcs[$user:$apachedate:ALL]++ ))
done

print "<div class=page>${ph_data.title}</div>"
print "<div class=page>Web Server Usage</div>"

print "<div class=page><table class=page>"

print "<tr class=page>"
print "<td class=pageborder>Date</td>"
print "<td class=pageborder>User</td>"
print "<td class=pageborder>All</td>"
for label in $labels; do
    print -n "<td class=pageborder>$label</td>"
done
print "</tr>"

for date in "${dates[@]}"; do
    for user in "${users[@]}"; do
        [[ ${udtcs[$user:$date:ALL]} == '' ]] && continue
        print -r "<tr class=page>"
        print -r "<td class=pageborder $style>$date</td>"
        print -r "<td class=pageborder $style>$user</td>"
        print -r "<td class=pageborder $style><b>${udtcs[$user:$date:ALL]}</b></td>"
        for name in $names; do
            print -rn "<td class=pageborder $style>"
            print -rn "${udtcs[$user:$date:$name]:-&nbsp;}"
            print -r "</td>"
        done
        print -r "</tr>"
    done
    print -r "<tr class=page>"
    print -r "<td class=pageborder $style><b>$date</b></td>"
    print -r "<td class=pageborder $style><b>Total</b></td>"
    print -r "<td class=pageborder $style><b>${dtcs[$date:ALL]}</b></td>"
    for name in $names; do
        print -rn "<td class=pageborder $style>"
        print -rn "<b>${dtcs[$date:$name]:-&nbsp;}</b>"
        print -r "</td>"
    done
    print -r "</tr>"
done
print -r "<tr class=page>"
print -r "<td class=pageborder $style>Total</td>"
print -r "<td class=pageborder $style>Total</td>"
print -r "<td class=pageborder $style><b>${tcs[ALL]}</b></td>"
for name in $names; do
    print -rn "<td class=pageborder $style>"
    print -rn "<b>${tcs[$name]:-&nbsp;}</b>"
    print -r "</td>"
done
print -r "</tr>"

print "</table></div>"

wuq_emit_body_end
