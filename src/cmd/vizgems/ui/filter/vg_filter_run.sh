
if [[ $SWIFTWARNLEVEL != '' ]] then
    set -x
fi

if ! swmuseringroups "vg_att_admin|vg_confview"; then
    print -u2 "SWIFT ERROR vg_filter_run security violation user: $SWMID"
    print "<html><body><b><font color=red>"
    print "sorry, you are not authorized to access this page"
    print "</font></b></body></html>"
    exit
fi

fq_data=(
    pid=''
    bannermenu='link|home||Home|return to the home page|link|back||Back|go back one page|list|tools|p:c|Tools|tools|list|help|userguide:filterpage|Help|documentation'
)

function fq_init {
    fq_data.pid=$pid

    return 0
}

function fq_emit_header_begin {
    print '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 TRANSITIONAL//EN">'
    print "<html>"
    print "<head>"

    ph_emitheader page banner bdiv
    print "<title>${ph_data.title}</title>"

    return 0
}

function fq_emit_header_end {
    print "</head>"

    return 0
}

function fq_emit_body_begin {
    typeset vt st img

    print "<body>"
    ph_emitbodyheader ${fq_data.pid} banner 1600

    return 0
}

function fq_emit_body_end {
    ph_emitbodyfooter
    print "</body>"
    print "</html>"

    return 0
}

if [[ $SWIFTWARNLEVEL != "" ]] then
    PS4='${.sh.fun}-$LINENO: '
    typeset -ft fq_init
    typeset -ft fq_emit_body_begin fq_emit_body_end
    typeset -ft fq_emit_header_begin fq_emit_header_end
fi

. vg_hdr
. vg_prefshelper
. $SWIFTDATADIR/dpfiles/config.sh

suireadkv 0 vars

date=${vars.fdate}
if [[ $date != '' ]] then
    ct=$(printf '%(%#)T' "$date")
else
    ct=$(printf '%(%#)T')
fi
if [[ $? != 0 ]] then
    print "<font color=red><b>Cannot parse time $date</b></font>"
    exit 1
fi

. $SWIFTDATADIR/etc/confmgr.info

if [[ ! -f $VGCM_FILTERFILE ]] then
    print "<font color=red><b>"
    print "    ERROR cannot access filter file: $VGCM_FILTERFILE"
    print "</b></font>"
    print -u2 SWIFT ERROR cannot access filter file: $VGCM_FILTERFILE
    exit 1
fi

pid=${vars.pid}
if [[ $pid == '' ]] then
    print -u2 SWIFT ERROR configuration not specified
    exit 1
fi
ph_init $pid "${fq_data.bannermenu}"
style='style="font-size:80%"'

fq_init
fq_emit_header_begin
fq_emit_header_end

fq_emit_body_begin

print "<div class=page>${ph_data.title}</div>"
print "<div class=page>Filters active at: $(printf '%T' \#$ct)</div>"

print "<div class=page><table class=page>"

print "<tr class=page>"
print "<td class=pageborder>Asset Name</td>"
print "<td class=pageborder>Alarm ID</td>"
print "<td class=pageborder>Message Text</td>"
print "<td class=pageborder>Start Time</td>"
print "<td class=pageborder>End Time</td>"
print "<td class=pageborder>Repeat</td>"
print "<td class=pageborder>Ticket Mode</td>"
print "<td class=pageborder>Viz Mode</td>"
print "<td class=pageborder>Severity</td>"
print "<td class=pageborder>Comment</td>"
print "<td class=pageborder>User</td>"
print "</tr>"

typeset -A fs

while read -r line; do
    rest=$line
    for (( i = 0; i < 13; i++ )) do
        fs[$i]=${rest%%\|+\|*}
        rest=${rest#"${fs[$i]}"\|+\|}
    done
    show=n

    (( st = ${fs[3]} * 60 ))
    (( et = ${fs[4]} * 60 ))
    if (( ${fs[5]} == ${fs[6]} && ${fs[4]} < ${fs[3]} )) then
        (( et += (24 * 60 * 60) ))
    fi
    (( dt = et - st + 1 ))

    case ${fs[7]} in
    once)
        if (( ct < ${fs[5]} + st )) then
            show=n
        elif [[
            ${fs[6]} == indefinitely
        ]] || (( ct <= ${fs[6]} + et )) then
            show=y
            (( t = ${fs[5]} + st ))
            ft=$(printf '%(%Y.%m.%d-%H:%M:%S %a)T' \#$t)
            if [[ ${fs[6]} == indefinitely ]] then
                tt=indefinitely
            else
                (( t = ${fs[6]} + et ))
                tt=$(printf '%(%Y.%m.%d-%H:%M:%S %a)T' \#$t)
            fi
        fi
        ;;
    *)
        if (( ct < ${fs[5]} + st )) then
            show=n
        elif [[
            ${fs[6]} == indefinitely
        ]] || (( ct <= ${fs[6]} + et )) then
            (( t = ${fs[5]} + st ))
            set -A l1 -- $(printf '%(%Y %m %d %H %M %S %a)T' \#$t)
            (( t = ${fs[5]} + st + dt ))
            set -A l2 -- $(printf '%(%Y %m %d %H %M %S %a)T' \#$t)
            set -A lc -- $(printf '%(%Y %m %d %H %M %S %a)T' \#$ct)
            (( t = ${lc[3]} * 3600 + ${lc[4]} * 60 ))
            case ${fs[7]} in
            monthly)
                if (( ${lc[2]} == ${l1[2]} && t >= st && t <= et )) then
                    show=y
                elif (( ${lc[2]} == ${l2[2]} && t <= ${fs[4]} * 60 )) then
                    show=y
                fi
                ;;
            weekly)
                if (( ${lc[6]} == ${l1[6]} && t >= st && t <= et )) then
                    show=y
                elif (( ${lc[6]} == ${l2[6]} && t <= ${fs[4]} * 60 )) then
                    show=y
                fi
                ;;
            daily)
                if (( t >= st && t <= et )) then
                    show=y
                fi
                ;;
            esac
            if [[ $show == y ]] then
                l1[0]=${lc[0]}
                l1[1]=${lc[1]}
                l1[2]=${lc[2]}
                l2[0]=${lc[0]}
                l2[1]=${lc[1]}
                l2[2]=${lc[2]}
                ft="${l1[0]}.${l1[1]}.${l1[2]}-${l1[3]}:${l1[4]} ${li[6]}"
                tt="${l2[0]}.${l2[1]}.${l2[2]}-${l2[3]}:${l2[4]} ${li[6]}"
            fi
        fi
    esac

    if [[ $show == y ]] then
        print -r "<tr class=page>"
        print -r "<td class=pageborder $style>${fs[0]}</td>"
        print -r "<td class=pageborder $style>${fs[1]}</td>"
        print -r "<td class=pageborder $style>${fs[2]}</td>"
        print -r "<td class=pageborder $style>$ft</td>"
        print -r "<td class=pageborder $style>$tt</td>"
        print -r "<td class=pageborder $style>${fs[7]}</td>"
        print -r "<td class=pageborder $style>${fs[8]}</td>"
        print -r "<td class=pageborder $style>${fs[9]}</td>"
        print -r "<td class=pageborder $style>${fs[10]}</td>"
        print -r "<td class=pageborder $style>${fs[11]}</td>"
        print -r "<td class=pageborder $style>${fs[12]}</td>"
        print -r "</tr>"
    fi
done < $VGCM_FILTERFILE

print "</table></div>"

fq_emit_body_end
