#!/bin/ksh

# DO NOT CHANGE
# this is a special query that generates alarm clear records

print "<response>"

if ! swmuseringroups "vg_att_admin|vg_hevalarmclear"; then
    print -u2 "ALARM clearalarm security violation user: $SWMID"
    print "<r>ERROR|You are not authorized to clear alarms</r>"
    print "</response>"
    exit
fi

ct=$(printf '%(%#)T')
date=$(printf '%(%Y.%m.%d.%H.%M.%S)T' \#$ct)

level=
id=
aid=
ccid=
tim=0

for vari in "${!vars.@}"; do
    [[ $vari != *.level_* ]] && continue
    if [[ $lv != '' ]] then
        print -u2 ERROR - multiple assets
        print "<r>ERROR|multiple assets specified</r>"
        print "</response>"
        exit
    fi
    lv=${vari##*_}
    typeset -n id=$vari
done

SHOWRECS=1 vg_dq_invhelper "$lv" "$id" | egrep "^B\|$lv\|$id\|" | read res
if [[ $res == '' ]] then
    print -u2 "ALARM clearalarm security violation user: $SWMID $lv:$id"
    print "<r>ERROR|You are not authorized to clear alarms for this asset</r>"
    print "</response>"
    exit
fi

case ${vars.mode} in
clearone)
    ti=${vars.clear_ti}
    tlabel=$(printf '%(%Y.%m.%d-%H:%M:%S)T' \#$ti)
    text1="Alarm $lv-$id/$tlabel cleared by $SWMID through the GUI"
    text2="alarm"
    ;;
clearaid)
    aid=${vars.clear_aid}
    text1="All alarms for ${vars.aid} cleared by $SWMID through the GUI"
    text2="all alarms for Alarm ID ${vars.aid}"
    ;;
clearccid)
    ccid=${vars.clear_ccid}
    text1="All alarms for ${vars.ccid} cleared by $SWMID through the GUI"
    text2="all alarms for CCID ${vars.ccid}"
    ;;
clearobj)
    text1="All alarms for $lv-$id cleared by $SWMID through the GUI"
    text2="all alarms for object $id"
    ;;
esac

mkdir -p $SWIFTDATADIR/outgoing/clear
(
    print "<alarm>"
    print "<v>$VG_S_VERSION</v><sid>$VG_SYSNAME</sid>"
    print "<aid>$aid</aid><ccid>$ccid</ccid>"
    print "<st></st><sm></sm><vars></vars>"
    print "<di></di><hi></hi>"
    print "<tc>$ti</tc><ti>$ct</ti>"
    print "<tp>CLEAR</tp><so></so>"
    print "<pm>$VG_ALARM_S_PMODE_PASSTHROUGH</pm>"
    print "<lv1>$lv</lv1><id1>$id</id1><lv2></lv2><id2></id2>"
    print "<tm></tm><sev>5</sev>"
    print "<txt>VG CLEAR $text1</txt><com></com>"
    print "</alarm>"
) > $SWIFTDATADIR/incoming/alarm/alarms.${date}U.tmp
cp \
    $SWIFTDATADIR/incoming/alarm/alarms.${date}U.tmp \
$SWIFTDATADIR/outgoing/clear/alarms.${date}U.tmp
mv \
    $SWIFTDATADIR/outgoing/clear/alarms.${date}U.tmp \
$SWIFTDATADIR/outgoing/clear/alarms.${date}U.xml
mv \
    $SWIFTDATADIR/incoming/alarm/alarms.${date}U.tmp \
$SWIFTDATADIR/incoming/alarm/alarms.${date}U.xml

print "<r>OK|Request to clear $text2 queued</r>"

print "</response>"

querydone=y
