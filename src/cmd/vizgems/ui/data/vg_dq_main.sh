
dq_main_data=(
    qid='' query=() querylabel='' displayorder='' label='' fdate='' ldate=''
    name='' prev='' pid='' thisurl='' fullurl='' baseurl='' objlabel=''
    pagewidth=1024 pageheight=1024 maxpixel=50000000 typeset -A levels
    datefilter='' typeset -A timeranges
    attrs=() ft='' lt='' rdate='' rdir='' statlod='' alarmlod='' alarmgroup=''
    alarmattr=() genericlod='' remotespecs=''
    doupdate='' updaterate='' qlline1='' qlline2='' qlline3='' qlline4=''
    nodeurlfmt='_URL_BASE_&kind=node&obj=%(_object_)%&objlabel=%(_level_)%:%(name)%&qid=%(___NEXTQID___)%&_ALARM_FILTERS_&_STAT_FILTERS_'
    charturlfmt='_URL_BASE_&kind=chart&obj=%(_object_)%&objlabel=%(_level_)%:%(name)%&qid=%(___NEXTQID___)%&stat_key=%(stat_key)%&_ALARM_FILTERS_'
    acharturlfmt='_URL_BASE_&kind=achart&obj=%(_object_)%&objlabel=%(_level_)%:%(name)%&qid=%(___NEXTQID___)%&stat_key=%(stat_key)%&_ALARM_FILTERS_'
    alarmurlfmt='_URL_BASE_&kind=alarm&obj=%(_object_)%&qid=clearalarm&_ct_=xml&clear_ccid=%(alarm_ccid)%&clear_aid=%(alarm_alarmid)%&clear_ti=%(alarm_time2clear)%'
    menus=(
        nodemenu="dq|defaultquery|Drill In|expand this node|list|otherqueries|Other Queries|select a different query|list|actions|Actions|select an action to perform|list|setdateparam|Change Date|select a different date|list|setalarmparam|Alarm Parameters|select what alarms to show|list|setstatparam|Stat Parameters|select what stats to show"
        edgemenu=""
        pagemenu="dq|defaultquery|Refresh|rerun query|list|otherqueries|Other Queries|select a different query|list|actions|Actions|select an action to perform|list|setdateparam|Change Date|select a different date|list|setinvparam|Inventory Parameters|select what assets to show|list|setalarmparam|Alarm Parameters|select what alarms to show|list|setstatparam|Stat Parameters|select what stats to show"
        chartmenu="dq|defaultquery|Drill In|expand this node|sq|zoomin|Enlarge Graph|show this graph in more detail|sq|zoomout|Shrink Graph|show this graph in normal mode|list|otherqueries|Other Queries|select a different query|list|actions|Actions|select an action to perform|list|setdateparam|Change Date|select a different date|list|setalarmparam|Alarm Parameters|select what alarms to show|list|setstatparam|Stat Parameters|select what stats to show"
        achartmenu="dq|defaultquery|Drill In|expand this node|sq|zoomin|Enlarge Graph|show this graph in more detail|sq|zoomout|Shrink Graph|show this graph in normal mode|list|otherqueries|Other Queries|select a different query|list|actions|Actions|select an action to perform|list|setdateparam|Change Date|select a different date|list|setalarmparam|Alarm Parameters|select what alarms to show|list|setstatparam|Stat Parameters|select what stats to show"
        alarmmenu="aq|clearone|Clear Alarm|clear this alarm only|aq|clearccid|Clear Corr.|clear all alarms with this correlation id|aq|clearaid|Clear AID|clear all alarms with this alarm id|aq|clearobj|Clear Asset|clear all alarms for this asset"
    )
)

function dq_main_init {
    typeset dt vt k v n n1 n2 tu fu bu tsep fsep bsep vari val vali
    typeset -A prefs dtinited vtinited
    typeset line rest lv sn nl fn pn af attf cusf sep s
    typeset havedate haveodate haveadate

    if [[ $REMOTESPECS != '' && $REMOTEQUERY != y ]] then
        dq_main_data.remotespecs=$REMOTESPECS
    fi

    egrep -v '^#' $SWIFTDATADIR/uifiles/dataqueries/levelorder.txt \
    | while read -r line; do
        [[ $line != ?*'|'?*'|'?*'|'?*'|'?* ]] && continue
        lv=${line%%'|'*}; rest=${line##"$lv"?('|')}
        sn=${rest%%'|'*}; rest=${rest##"$sn"?('|')}
        nl=${rest%%'|'*}; rest=${rest##"$nl"?('|')}
        fn=${rest%%'|'*}; rest=${rest##"$fn"?('|')}
        pn=${rest%%'|'*}; rest=${rest##"$pn"?('|')}
        af=${rest%%'|'*}; rest=${rest##"$af"?('|')}
        attf=${rest%%'|'*}; rest=${rest##"$attf"?('|')}
        cusf=${rest%%'|'*}; rest=${rest##"$cusf"?('|')}
        dq_main_data.levels[$lv]=(
            lv=$lv sn=$sn nl=$nl fn=$fn pn=$pn af=$af attf=$attf cusf=$cusf
        )
    done
    egrep -v '^#' $SWIFTDATADIR/uifiles/dataqueries/alarmattrs.txt \
    | while read -r line; do
        case $line in
        color=*)
            dq_main_data.alarmattr.color=${line#color=}
            ;;
        severitylabel=*)
            dq_main_data.alarmattr.severitylabel=${line#severitylabel=}
            ;;
        ticketlabel10=*)
            dq_main_data.alarmattr.ticketlabel10=${line#*=}
            ;;
        ticketlabel11=*)
            dq_main_data.alarmattr.ticketlabel11=${line#*=}
            ;;
        ticketlabel12=*)
            dq_main_data.alarmattr.ticketlabel12=${line#*=}
            ;;
        ticketlabel13=*)
            dq_main_data.alarmattr.ticketlabel13=${line#*=}
            ;;
        ticketlabel20=*)
            dq_main_data.alarmattr.ticketlabel20=${line#*=}
            ;;
        ticketlabel21=*)
            dq_main_data.alarmattr.ticketlabel21=${line#*=}
            ;;
        ticketlabel22=*)
            dq_main_data.alarmattr.ticketlabel22=${line#*=}
            ;;
        ticketlabel23=*)
            dq_main_data.alarmattr.ticketlabel23=${line#*=}
            ;;
        esac
    done

    case ${ph_data.browser} in
    IE)
        dq_main_data.maxpixel=10000000
        ;;
    IP)
        dq_main_data.maxpixel=2000000
        ;;
    BB)
        dq_main_data.maxpixel=2000000
        ;;
    AD)
        dq_main_data.maxpixel=2000000
        ;;
    *)
        dq_main_data.maxpixel=20000000
        ;;
    esac

    if [[ ${vars.tzoverride} != '' && ${vars.tzoverride} != default ]] then
        PHTZ=${vars.tzoverride}
    fi

    if [[ ${querydata.args.onempty} == '' ]] then
        or=()
    else
        typeset -n or=querydata.args.onempty
    fi
    if [[ ${querydata.args.always} == '' ]] then
        ar=()
    else
        typeset -n ar=querydata.args.always
    fi
    havedate=n haveodate=n haveadate=n
    if [[ ${or.date} != '' || ${or.fdate} != '' || ${or.ldate} != '' ]] then
        haveodate=y
    fi
    if [[ ${ar.date} != '' || ${ar.fdate} != '' || ${ar.ldate} != '' ]] then
        haveadate=y
    fi
    if [[
        ${vars.date} != '' || ${vars.fdate} != '' || ${vars.ldate} != ''
    ]] then
        havedate=y
    fi
    if [[ ( $haveodate == y && $havedate == n ) || $haveadate == y ]] then
        unset vars.date vars.fdate vars.ldate
        if [[ ${or.date} != '' ]] then
            vars.date=${or.date}
        elif [[ ${ar.date} != '' ]] then
            vars.date=${ar.date}
        fi
        if [[ ${or.fdate} != '' ]] then
            vars.fdate=${or.fdate}
        elif [[ ${ar.fdate} != '' ]] then
            vars.fdate=${ar.fdate}
        fi
        if [[ ${or.ldate} != '' ]] then
            vars.ldate=${or.ldate}
        elif [[ ${ar.ldate} != '' ]] then
            vars.ldate=${ar.ldate}
        fi
    fi
    dq_main_parsedates
    if [[ $DQLOD == y && ${vars.lod} != '' ]] then
        dq_main_data.statlod=${vars.lod}
        dq_main_data.alarmlod=${vars.lod}
        dq_main_data.genericlod=${vars.lod}
    fi
    dq_main_data.name=${vars.name}
    dq_main_data.prev=${vars.prev}
    dq_main_data.pid=${vars.pid}
    dq_main_data.updaterate=${vars.update:-'-1'}
    dq_main_data.objlabel=${vars.objlabel}

    tu=${ph_data.cgidir}/vg_dserverhelper.cgi fu=$tu bu=$tu
    tsep='?' fsep='?' bsep='?'
    for vari in "${!vars.@}"; do
        case $vari in
        *.@(class|date|format|gfx|label|parent|prefprefix|txt|view|viewname))
            continue
            ;;
        *.@(debug))
            continue
            ;;
        esac
        typeset -n var=$vari
        if (( ${#var[@]} == 1 )) then
            val=$var
        else
            val=
            for vali in "${var[@]}"; do
                val+="|$vali"
            done
            val="(${val#'|'})"
        fi
        val=$(printf '%#H' "$val")
        tu+="$tsep${vari##*.}=$val"
        tsep='&'
        if [[ ${vari##*.} == prev ]] then
            continue
        fi
        if [[ ${vari##*.} == name ]] then
            vari=prev
        fi
        fu+="$fsep${vari##*.}=$val"
        fsep='&'
        if [[ ${vari##*.} == @(qid|objlabel|obj|kind|pid|tryasid) ]] then
            continue
        fi
        if [[ ${vari##*.} == @(level_*|lname_*|alarm_*|stat_*|attr_*) ]] then
            continue
        fi
        if [[ ${vari##*.} == @(*_divmode|*_pageindex*) ]] then
            continue
        fi
        bu+="$bsep${vari##*.}=$val"
        bsep='&'
    done
    dq_main_data.thisurl="$tu"
    dq_main_data.fullurl="$fu"
    dq_main_data.baseurl="$bu&pid=${vars.pid}"

    [[ ${vars.winw} != '' ]] && dq_main_data.pagewidth=${vars.winw}
    [[ ${vars.winh} != '' ]] && dq_main_data.pageheight=${vars.winh}

    if [[ ${vars.qid} == '' ]] then
        print -u2 "VG QUERY ERROR: no query name specified"
        return 1
    fi
    dq_main_data.qid=${vars.qid}
    if [[ ${querydata.args.dtools} == '' ]] then
        typeset -n qr=${dq_main_data.qid}
        if [[ ${qr.args.labellist} != '' ]] then
            typeset -C dq_main_data.query=qr
        fi
    else
        dq_main_data.query.args=()
        dq_main_data.query.dtools=()
        dq_main_data.query.vtools=()
        if [[ ${querydata.args.invkeeplevels} != '' ]] then
            s=${querydata.args.invkeeplevels}
            dq_main_data.query.args.invkeeplevels=$s
            dq_main_data.nodeurlfmt+="&_INV_KEEPFILTERS_"
        fi
        sep=
        for dt in ${querydata.args.dtools}; do
            if [[ ${dtinited[$dt]} == '' ]] then
                . vg_dq_dt_${dt}
                dtinited[$dt]=y
            fi
            if ! dq_dt_${dt}_setup; then
                print -u2 "VG QUERY ERROR: setup for dtool $dt failed"
                return 1
            fi
            dq_main_data.query.args.labellist+="$sep$dt"
            sep=' '
        done
        sep=
        for vt in ${querydata.args.vtools}; do
            if [[ ${vtinited[$vt]} == '' ]] then
                . vg_dq_vt_${vt}
                vtinited[$vt]=y
            fi
            if ! dq_vt_${vt}_setup; then
                print -u2 "VG QUERY ERROR: setup for vtool $vt failed"
                return 1
            fi
            typeset -n rmr=dq_main_data.query.vtools.$vt.args.rendermode
            if [[ $rmr == final ]] then
                dq_main_data.query.args.displayorder+="$sep$vt"
                sep=' '
            fi
        done
    fi
    if [[ ${dq_main_data.query.args.labellist} == '' ]] then
        print -u2 "VG QUERY ERROR: no data tool information specified"
        return 1
    fi
    if [[ ${dq_main_data.query.args.displayorder} == '' ]] then
        print -u2 "VG QUERY ERROR: no viz tool information specified"
        return 1
    fi

    typeset -n qprefs=dq_main_data.query.args

    for k in "${!qprefs.@}"; do
        n=${k##*$1.args?(.)}
        [[ $n == '' || $n == *.* ]] && continue
        typeset -n vr=$k
        prefs[$n]=$vr
    done
    for n in "${!vars.main_@}"; do
        typeset -n vr=$n
        k=${n##*main_}
        prefs[$k]=$vr
    done

    for k in "${!prefs[@]}"; do
        v=${prefs[$k]}
        if [[ $k == *_* ]] then
            n1=${k%%_*}
            n2=${k#*_}
            typeset -n attr=dq_main_data.attrs.$n1
            if [[ $attr != '' ]] then
                attr+=" $n2='${v//\'/\"}'"
            else
                attr="$n2='${v//\'/\"}'"
            fi
        else
            typeset -n attr=dq_main_data.attrs.$k
            attr=$v
        fi
    done
    dq_main_data.displayorder=${prefs[displayorder]}

    typeset -n dts=dq_main_data.query.dtools
    for dt in "${!dts.@}"; do
        dt=${dt#dq_main_data.query.dtools.}
        [[ $dt == *.* ]] && continue
        if [[ ${dtinited[$dt]} == '' ]] then
            . vg_dq_dt_${dt}
            dtinited[$dt]=y
        fi
        dq_dt_${dt}_init dq_main_data.query.dtools.$dt || return 1
    done

    typeset -n dtlist=dq_main_data.query.args.labellist
    for dt in ${dtlist}; do
        [[ $dt == '' ]] && continue
        if [[ ${dtinited[$dt]} == '' ]] then
            . vg_dq_dt_${dt}
            dtinited[$dt]=y
        fi
        dq_dt_${dt}_emitlabel dq_main_data.query.dtools.$dt || return 1
        typeset -n dtlabel=dq_dt_${dt}_data.label
        dq_main_data.label+=" $dtlabel"
    done
    dq_main_data.label=${dq_main_data.label#' '}

    typeset -n vts=dq_main_data.query.vtools
    for vt in "${!vts.@}"; do
        vt=${vt#dq_main_data.query.vtools.}
        [[ $vt == *.* ]] && continue
        if [[ ${vtinited[$vt]} == '' ]] then
            . vg_dq_vt_${vt}
            vtinited[$vt]=y
        fi
        dq_vt_${vt}_init dq_main_data.query.vtools.$vt || return 1
        if [[ ${prefs[displayorder]} == '' ]] then
            dq_main_data.displayorder+=" $vt"
        fi
    done

    print -r "${dq_main_data.thisurl}" > this.txt

    [[ $SAVEDATA == y ]] && typeset -p dq_main_data > dq_main_data.sh

    return 0
}

function dq_main_parsedates {
    typeset d4='{4}([0-9])' d2='{1,2}([0-9])'
    typeset -l fdate ldate tdate
    typeset dir fspec lspec latestymd oymd havedds currt ft lt t rdate lwrap n d

    if [[ ${vars.date} == '' && (
        ${vars.fdate} == '' && ${vars.ldate} == ''
    ) ]] then
        vars.fdate=latest
    fi

    if [[ ${vars.date} != '' && (
        ${vars.fdate} == '' && ${vars.ldate} == ''
    ) ]] then
        vars.fdate=${vars.date}
    fi

    dq_main_data.fdate=${vars.fdate}
    dq_main_data.ldate=${vars.ldate}
    dq_main_data.datefilter=${vars.datefilter}

    currt=$(printf '%(%#)T')
    dir=$(cd $SWIFTDATADIR/data/main/latest/processed/total && pwd -P)
    latestymd=${dir##*/data/main/}
    latestymd=${latestymd%%/processed/*}
    latestymd=${latestymd//'/'/.}
    oymd=$latestymd
    havedds=n
    for (( n = 0; n < 30; n++ )) do
        d=${latestymd//.//}
        if [[
            -s $SWIFTDATADIR/data/main/$d/processed/total/inv-nodes.dds &&
            -s $SWIFTDATADIR/data/main/$d/processed/total/open.alarms.dds &&
            -s $SWIFTDATADIR/data/main/$d/processed/total/uniq.stats.dds
        ]] then
            havedds=y
            break
        fi
        t=$(printf '%(%#)T' "$latestymd-00:00:00")
        latestymd=$(printf '%(%Y.%m.%d)T' \#$(( t - 24 * 60 * 60 )))
    done

    [[ $havedds != y ]] && latestymd=$oymd

    if [[ ${vars.fdate} == @(latest|today|yesterday|this*|last*) ]] then
        vars.ldate=
    fi

    fdate=${vars.fdate//[$' \t']/}
    ldate=${vars.ldate//[$' \t']/}
    if [[ $fdate == [0-9]* && $ldate == [0-9]* && $fdate > $ldate ]] then
        tdate=$fdate; fdate=$ldate; ldate=$tdate
    fi
    if [[ $fdate == ${d4}[./]${d2}[./]${d2} ]] then
        fspec=${fdate//'/'/.}-00:00:00
    elif [[ $fdate == ${d4}[./]${d2}[./]${d2}-${d2} ]] then
        fspec=${fdate//'/'/.}:00:00
    elif [[ $fdate == ${d4}[./]${d2}[./]${d2}-${d2}:${d2} ]] then
        fspec=${fdate//'/'/.}:00
    elif [[ $fdate == ${d4}[./]${d2}[./]${d2}-${d2}:${d2}:${d2} ]] then
        fspec=${fdate//'/'/.}
    fi
    if [[ $ldate == ${d4}[./]${d2}[./]${d2} ]] then
        lspec=${ldate//'/'/.}-24:00:00
        lwrap=y
    elif [[ $ldate == ${d4}[./]${d2}[./]${d2}-${d2} ]] then
        [[ $ldate == *-24 ]] && lwrap=y
        lspec=${ldate//'/'/.}:00:00
    elif [[ $ldate == ${d4}[./]${d2}[./]${d2}-${d2}:${d2} ]] then
        [[ $ldate == *-24:00 ]] && lwrap=y
        lspec=${ldate//'/'/.}:00
    elif [[ $ldate == ${d4}[./]${d2}[./]${d2}-${d2}:${d2}:${d2} ]] then
        [[ $ldate == *-24:00:00 ]] && lwrap=y
        lspec=${ldate//'/'/.}
    fi
    if [[ $fspec != '' && $lspec == '' ]] then
        lspec=${fspec%-*}-23:59:59
    elif [[ $fspec == '' && $lspec != '' ]] then
        fspec=${fspec%-*}-00:00:00
    fi
    if [[ $fspec != '' ]] then
        ft=$(TZ=$PHTZ printf '%(%#)T' "$fspec")
        lt=$(TZ=$PHTZ printf '%(%#)T' "$lspec")
        [[ $lwrap == y ]] && (( lt-- ))
        (( ft > lt )) && { t=$lt; lt=$ft; ft=$t; }
        dq_main_data.ft=$ft
        dq_main_data.lt=$lt
        rdate=$(printf '%(%Y/%m/%d)T' \#$lt)
        if [[
            ${dq_main_data.remotespecs} != '' &&
            ! -f $SWIFTDATADIR/data/main/$rdate/processed/total/inv-nodes.dds
        ]] then
            rdate=$latestymd
        elif [[
            ! -f $SWIFTDATADIR/data/main/$rdate/processed/total/inv-nodes.dds ||
            ! \
            -f $SWIFTDATADIR/data/main/$rdate/processed/total/open.alarms.dds ||
            ! -f $SWIFTDATADIR/data/main/$rdate/processed/total/uniq.stats.dds
        ]] then
            rdate=$latestymd
        fi
        dq_main_data.rdir=$SWIFTDATADIR/data/main/${rdate//.//}/processed/total
        dq_main_data.rdate=$rdate
        dq_main_data.alarmgroup=all
        if (( lt - ft > 32 * 24 * 60 * 60 )) then
            dq_main_data.statlod=y
            dq_main_data.alarmlod=y
            dq_main_data.genericlod=y
        elif (( lt - ft > 2 * 25 * 60 * 60 )) then
            dq_main_data.statlod=ym
            dq_main_data.alarmlod=ym
            dq_main_data.genericlod=ym
        else
            dq_main_data.statlod=ymd
            dq_main_data.alarmlod=ymd
            dq_main_data.genericlod=ymd
        fi
        dq_main_data.doupdate=n
        if [[ ${dq_main_data.statlod} == ymd ]] && (( lt >= currt )) then
            dq_main_data.doupdate=y
        fi
        [[ ${dq_main_data.datefilter} != '' ]] && dq_main_parsedatefilters
        {
            print "ft=${dq_main_data.ft}"
            print "lt=${dq_main_data.lt}"
        } > info.lst
        return 0
    fi

    if [[ $fdate == latest ]] then
        lspec=$(TZ=$PHTZ printf '%(%Y.%m.%d)T' "0 day ago")
        lt=$(TZ=$PHTZ printf '%(%#)T' "$lspec-23:59:59")
        fspec=$(TZ=$PHTZ printf '%(%Y.%m.%d)T' "0 day ago")
        ft=$(TZ=$PHTZ printf '%(%#)T' "$fspec-00:00:00")
        if [[ ! -d $SWIFTDATADIR/data/main/${fspec//.//}/processed/total ]] then
            lt=$(TZ=$PHTZ printf '%(%#)T' "$latestymd-23:59:59")
            ft=$(TZ=$PHTZ printf '%(%#)T' "$latestymd-00:00:00")
        fi
    elif [[ $fdate == today ]] then
        lspec=$(TZ=$PHTZ printf '%(%Y.%m.%d)T' "0 day ago")
        lt=$(TZ=$PHTZ printf '%(%#)T' "$lspec-23:59:59")
        fspec=$(TZ=$PHTZ printf '%(%Y.%m.%d)T' "0 day ago")
        ft=$(TZ=$PHTZ printf '%(%#)T' "$fspec-00:00:00")
    elif [[ $fdate == yesterday ]] then
        fspec=$(TZ=$PHTZ printf '%(%Y.%m.%d)T' "1 day ago")
        ft=$(TZ=$PHTZ printf '%(%#)T' "$fspec-00:00:00")
        lspec=$(TZ=$PHTZ printf '%(%Y.%m.%d)T' "0 day ago")
        lt=$(TZ=$PHTZ printf '%(%#)T' "$lspec-00:00:00")
        (( lt-- ))
    elif [[ $fdate == thismin ]] then
        lt=$currt
        fspec=$(TZ=$PHTZ printf '%(%Y.%m.%d-%H:%M)T' "1 min ago")
        ft=$(TZ=$PHTZ printf '%(%#)T' "$fspec:00")
    elif [[ $fdate == last*([0-9])min?(s) ]] then
        lt=$currt
        n=${.sh.match[1]:-1}
        (( n < 1 )) && n=1
        fspec=$(TZ=$PHTZ printf '%(%Y.%m.%d-%H:%M)T' "$n min ago")
        ft=$(TZ=$PHTZ printf '%(%#)T' "$fspec:00")
    elif [[ $fdate == thishour ]] then
        lt=$currt
        fspec=$(TZ=$PHTZ printf '%(%Y.%m.%d-%H)T' "0 hour ago")
        ft=$(TZ=$PHTZ printf '%(%#)T' "$fspec:00:00")
    elif [[ $fdate == last*([0-9])hour?(s) ]] then
        lt=$currt
        n=${.sh.match[1]:-1}
        (( n < 1 )) && n=1
        fspec=$(TZ=$PHTZ printf '%(%Y.%m.%d-%H)T' "$n hour ago")
        ft=$(TZ=$PHTZ printf '%(%#)T' "$fspec:00:00")
    elif [[ $fdate == thisday ]] then
        lt=$currt
        fspec=$(TZ=$PHTZ printf '%(%Y.%m.%d)T' "0 day ago")
        ft=$(TZ=$PHTZ printf '%(%#)T' "$fspec-00:00:00")
    elif [[ $fdate == last*([0-9])day?(s) ]] then
        n=${.sh.match[1]:-1}
        (( n < 1 )) && n=1
        fspec=$(TZ=$PHTZ printf '%(%Y.%m.%d)T' "$n day ago")
        ft=$(TZ=$PHTZ printf '%(%#)T' "$fspec-00:00:00")
        lspec=$(TZ=$PHTZ printf '%(%Y.%m.%d)T' "0 day ago")
        lt=$(TZ=$PHTZ printf '%(%#)T' "$lspec-00:00:00")
        (( lt-- ))
    elif [[ $fdate == thisweek ]] then
        lt=$currt
        ft=$(TZ=$PHTZ printf '%(%#)T' "0 week ago")
    elif [[ $fdate == last*([0-9])week?(s) ]] then
        n=${.sh.match[1]:-1}
        (( n < 1 )) && n=1
        ft=$(TZ=$PHTZ printf '%(%#)T' "$n week ago")
        lt=$(TZ=$PHTZ printf '%(%#)T' "0 week ago")
        (( lt-- ))
    elif [[ $fdate == thismonth ]] then
        lt=$currt
        ft=$(TZ=$PHTZ printf '%(%#)T' "0 month ago")
    elif [[ $fdate == last*([0-9])month?(s) ]] then
        n=${.sh.match[1]:-1}
        (( n < 1 )) && n=1
        ft=$(TZ=$PHTZ printf '%(%#)T' "$n month ago")
        lt=$(TZ=$PHTZ printf '%(%#)T' "0 month ago")
        (( lt-- ))
    elif [[ $fdate == thisyear ]] then
        lt=$currt
        ft=$(TZ=$PHTZ printf '%(%#)T' "0 year ago")
    elif [[ $fdate == last*([0-9])year?(s) ]] then
        n=${.sh.match[1]:-1}
        (( n < 1 )) && n=1
        ft=$(TZ=$PHTZ printf '%(%#)T' "$n year ago")
        lt=$(TZ=$PHTZ printf '%(%#)T' "0 year ago")
        (( lt-- ))
    else
        lt=$(TZ=$PHTZ printf '%(%#)T' "$latestymd-23:59:59")
        ft=$(TZ=$PHTZ printf '%(%#)T' "$latestymd-00:00:00")
    fi
    dq_main_data.ft=$ft
    dq_main_data.lt=$lt
    rdate=$(printf '%(%Y/%m/%d)T' \#$lt)
    if [[
        ! -f $SWIFTDATADIR/data/main/$rdate/processed/total/inv-nodes.dds ||
        ! -f $SWIFTDATADIR/data/main/$rdate/processed/total/open.alarms.dds ||
        ! -f $SWIFTDATADIR/data/main/$rdate/processed/total/uniq.stats.dds
    ]] then
        rdate=$latestymd
    fi
    dq_main_data.rdir=$SWIFTDATADIR/data/main/${rdate//.//}/processed/total
    dq_main_data.rdate=$rdate
    dq_main_data.alarmgroup=all
    if [[ $fdate == latest ]] then
        dq_main_data.alarmgroup=open
    fi
    if (( lt - ft > 32 * 24 * 60 * 60 )) then
        dq_main_data.statlod=y
        dq_main_data.alarmlod=y
        dq_main_data.genericlod=y
    elif (( lt - ft > 2 * 25 * 60 * 60 )) then
        dq_main_data.statlod=ym
        dq_main_data.alarmlod=ym
        dq_main_data.genericlod=ym
    else
        dq_main_data.statlod=ymd
        dq_main_data.alarmlod=ymd
        dq_main_data.genericlod=ymd
    fi
    dq_main_data.doupdate=n
    if [[ ${dq_main_data.statlod} == ymd ]] && (( lt >= currt )) then
        dq_main_data.doupdate=y
    elif [[ $fdate == latest ]] then
        dq_main_data.doupdate=y
    fi
    [[ ${dq_main_data.datefilter} != '' ]] && dq_main_parsedatefilters
    {
        print "ft=${dq_main_data.ft}"
        print "lt=${dq_main_data.lt}"
    } > info.lst
    return 0
}

function dq_main_parsedatefilters {
    typeset ifs fspec lspec hms mft mlt eft elt dft dlt ft lt ct dayt
    typeset fdow ldow ymda ymd datefilteri trn
    typeset -l datafilteri dow
    typeset dowre='mon|tue|wed|thu|fri|sat|sun'
    typeset ymdre='[0-9][0-9][0-9][0-9][./][0-9][0-9][./][0-9][0-9]'
    typeset hmsre='[0-9][0-9]:[0-9][0-9]:[0-9][0-9]'

    mft=${dq_main_data.ft}
    mlt=${dq_main_data.lt}
    (( eft = mft - 24 * 60 * 60 ))
    (( elt = mlt + 24 * 60 * 60 ))
    (( dayt = 24 * 60 * 60 ))

    trn=0
    ifs="$IFS"
    IFS=';'
    for datefilteri in ${dq_main_data.datefilter}; do
        if [[ $datefilteri == *,* ]] then
            fspec=${datefilteri%%,*}
            fspec=${fspec//[!a-zA-Z0-9:;,.-]/}
            [[ $fspec != *-* ]] && fspec=$fspec-00:00:00
            lspec=${datefilteri#*,}
            lspec=${lspec//[!a-zA-Z0-9:;,.-]/}
            [[ $lspec != *-* ]] && lspec=$lspec-23:59:59
        else
            fspec=$datefilteri
            fspec=${fspec//[!a-zA-Z0-9:;,.-]/}
            [[ $fspec != *-* ]] && fspec=$fspec-00:00:00
            lspec=$datefilteri
            lspec=${lspec//[!a-zA-Z0-9:;,.-]/}
            [[ $lspec != *-* ]] && lspec=$lspec-23:59:59
        fi
        if [[
            (${fspec%%'-'*} != @($dowre) && ${fspec%%'-'*} != @($ymdre)) ||
            (${lspec%%'-'*} != @($dowre) && ${lspec%%'-'*} != @($ymdre))
        ]] then
            print -u2 VG QUERY ERROR: bad day specs $fspec / $lspec
            fspec= lspec=
        fi
        if [[ ${fspec#*'-'} != @($hmsre) || ${lspec#*'-'} != @($hmsre) ]] then
            print -u2 VG QUERY ERROR: bad hms specs $fspec / $lspec
            fspec= lspec=
        fi
        if [[ $fspec == '' || $lspec == '' ]] then
            continue
        fi
        hms=${fspec##*-}
        (( dft = ${hms:0:2} * 60 * 60 + ${hms:3:2} * 60 + ${hms:6:2} ))
        hms=${lspec##*-}
        (( dlt = ${hms:0:2} * 60 * 60 + ${hms:3:2} * 60 + ${hms:6:2} ))
        if [[ ${fspec%%'-'*} == @($dowre) && ${lspec%%'-'*} != @($dowre) ]] then
            print -u2 VG QUERY ERROR: dow vs. non-dow error
            continue
        fi

        if [[ ${fspec%%'-'*} != @($dowre) ]] then
            ft=${ printf '%(%#)T' "$fspec"; }
            lt=${ printf '%(%#)T' "$lspec"; }
            dq_main_data.timeranges[${#timeranges[@]}]=(
                fdate=$fspec ldate=$lspec
                ft=$ft lt=$lt dft=$dft dlt=$dlt
            )
            print -r "$ft|$lt"
            (( trn++ ))
        else
            fdow=${fspec%%'-'*}
            ldow=${lspec%%'-'*}
            ft= lt=
            for (( ct = eft; ct < elt; ct += dayt )) do
                ymda=${ printf '%(%Y.%m.%d %a)T' "#$ct"; }
                ymd=${ymda%' '*}
                dow=${ymda##*' '}
                dt=${ printf '%(%#)T' "$ymd-00:00:00"; }
                if [[ $fdow != $dow && $ldow != $dow ]] then
                    continue
                fi
                if [[ $fdow == $dow ]] then
                    (( ft = dt + dft ))
                fi
                if [[ $ldow == $dow ]] then
                    (( lt = dt + dlt ))
                fi
                if [[ $lt != '' && $ft == '' ]] then
                    lt=
                    continue
                fi
                if [[ $ft != '' && $lt != '' ]] then
                    dq_main_data.timeranges[${#timeranges[@]}]=(
                        fdate=$fspec ldate=$lspec
                        ft=$ft lt=$lt dft=$dft dlt=$dlt
                    )
                    print -r "$ft|$lt"
                    (( trn++ ))
                    ft= lt=
                fi
            done
        fi
    done > timeranges.txt
    IFS="$ifs"

    if (( trn > 0 )) then
        export TIMERANGESFILE=timeranges.txt TIMERANGESSIZE=$trn
    fi

    return 0
}

function dq_main_run {
    typeset dt vt deps dep progress
    typeset -A dtqueue vtqueue
    typeset -A filesbytype
    typeset dirs dir file files filetype spec

    if [[ ${dq_main_data.remotespecs} == '' ]] then
        typeset -n dts=dq_main_data.query.dtools
        for dt in "${!dts.@}"; do
            dt=${dt#dq_main_data.query.dtools.}
            [[ $dt == *.* ]] && continue
            typeset -n dtt=dq_dt_${dt}_data
            dtqueue[$dt]=${dtt.deps}
        done
        while (( ${#dtqueue[@]} > 0 )) do
            while (( ${#dtqueue[@]} > 0 )) do
                progress=n
                for dt in "${!dtqueue[@]}"; do
                    [[ ${dtqueue[$dt]} != '' ]] && continue
                    dq_dt_${dt}_run dq_main_data.query.dtools.$dt || return 1
                    unset dtqueue[$dt]
                    progress=y
                done
                for dt in "${!dtqueue[@]}"; do
                    deps=
                    for dep in ${dtqueue[$dt]}; do
                        [[ ${dtqueue[$dep]} == '' ]] && continue
                        deps+=" $dep"
                    done
                    [[ ${dtqueue[$dt]} != ${deps#' '} ]] && progress=y
                    dtqueue[$dt]=${deps#' '}
                done
                if [[ $progress != y ]] then
                    print -u2 VG QUERY ERROR: cannot resolve dtool dependencies
                    return 1
                fi
            done
            for dt in "${!dts.@}"; do
                dt=${dt#dq_main_data.query.dtools.}
                [[ $dt == *.* ]] && continue
                typeset -n dtt=dq_dt_${dt}_data
                if [[ ${dtt.rerun} == y ]] then
                    dtqueue[$dt]=
                    dtt.rerun=n
                fi
            done
        done
    else
        dirs=
        for spec in ${dq_main_data.remotespecs}; do
            dir=${spec//'/'/_}
            rm -rf $dir
            mkdir $dir
            (
                cd $dir
                ssh ${spec%%:*} ${spec#*:} -remote -user "$SWMID" \
                < ../vars.lst > data.pax 2> errors
                if [[ $? != 0 ]] then
                    print -u2 VG QUERY ERROR: remote query to $spec failed
                    exit 1
                fi
                pax -s,/,_,g -rf data.pax 2> /dev/null
            ) &
            dirs+=" $dir"
        done
        wait
        for dir in $dirs; do
            for file in $dir/*.dds; do
                case $file in
                */inode*.dds)
                    filesbytype[inode.dds]+=" $file"
                    ;;
                */iedge.dds)
                    filesbytype[iedge.dds]+=" $file"
                    ;;
                */*.dds)
                    filesbytype[${file##*/}]+=" $file"
                    ;;
                esac
            done
        done
        for filetype in "${!filesbytype[@]}"; do
            files=${filesbytype[$filetype]}
            rm -f $filetype
            case $filetype in
            inode.dds)
                ddssort \
                    -fast -m -u -ke 'cat type level1 id1' $files \
                > inode.dds
                ;;
            iedge.dds)
                ddssort \
                    -fast -m -u -ke 'cat type level1 id1 level2 id2' $files \
                > iedge.dds
                ;;
            alarm.dds)
                ddssort \
                    -fast -m -r -ke 'timeissued type sortorder' $files \
                > alarm.dds
                ;;
            alarmstat.dds)
                ddscat $files > alarmstat.dds
                ;;
            stat.dds)
                ddscat $files > stat.dds
                ;;
            *)
                if [[ $GENERICSPEC != '' && -f $GENERICPATH/bin/spec.sh ]] then
                    . $GENERICPATH/bin/spec.sh
                    ${spec.querydtmerge} $files > $filetype
                else
                    ddscat $files > $filetype
                fi
                ;;
            esac
        done
        if [[ -f iedge.dds ]] then
            ln -s iedge.dds inv.dds
        else
            ln -s inode.dds inv.dds
        fi
    fi

    [[ $DQVTOOLS == n ]] && return 0

    typeset -n vts=dq_main_data.query.vtools
    for vt in "${!vts.@}"; do
        vt=${vt#dq_main_data.query.vtools.}
        [[ $vt == *.* ]] && continue
        typeset -n vtt=dq_vt_${vt}_data
        vtqueue[$vt]=${vtt.deps}
    done
    while (( ${#vtqueue[@]} > 0 )) do
        while (( ${#vtqueue[@]} > 0 )) do
            progress=n
            for vt in "${!vtqueue[@]}"; do
                [[ ${vtqueue[$vt]} != '' ]] && continue
                dq_vt_${vt}_run dq_main_data.query.vtools.$vt || return 1
                unset vtqueue[$vt]
                progress=y
            done
            for vt in "${!vtqueue[@]}"; do
                deps=
                for dep in ${vtqueue[$vt]}; do
                    [[ ${vtqueue[$dep]} == '' ]] && continue
                    deps+=" $dep"
                done
                [[ ${vtqueue[$vt]} != ${deps#' '} ]] && progress=y
                vtqueue[$vt]=${deps#' '}
            done
            if [[ $progress != y ]] then
                print -u2 VG QUERY ERROR: cannot resolve vtool dependencies
                return 1
            fi
        done
        for vt in "${!vts.@}"; do
            vt=${vt#dq_main_data.query.vtools.}
            [[ $vt == *.* ]] && continue
            typeset -n vtt=dq_vt_${vt}_data
            if [[ ${vtt.rerun} == y ]] then
                vtqueue[$vt]=
                vtt.rerun=n
            fi
        done
    done

    return 0
}

function dq_main_emit_header_begin {
    typeset vt attri as ai mt mi rest lv id name us gs qs ns vs ls ds hs
    typeset line sn nl fn pn af attf cusf f tm sevi tmi ifs dt s

    typeset -A qdatelinks

    [[ $DQVTOOLS == n ]] && return 0

    if [[ ${prefqp.legendmode} == 0 ]] then
        export HIDEINFO=y
    fi

    if [[ $dqmode != embed ]] then
        print '<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 TRANSITIONAL//EN">'
        case ${ph_data.browser} in
        IP)
            print '<meta name="viewport" content="width=device-width">'
            ;;
        esac
        print "<html>"
        print "<head>"

        ph_emitheader page banner bdiv sdiv mdiv
        print "<title>${ph_data.title}</title>"
    fi

    print "<!--BEGIN SKIP-->"
    print "<script src='$SWIFTWEBPREFIX/proj/vg/vg_dq_main.js'></script>"
    print "<script>"
    print "var dq_mode='$dqmode'"
    for attri in "${!dq_main_data.menus.@}"; do
        typeset -n attr=$attri
        ifs="$IFS"
        IFS='|'
        set -f
        set -A as -- $attr
        set +f
        IFS="$ifs"
        mi=0
        mt=${attri##*.}
        print "var dq_${mt}s = new Array ()"
        for (( ai = 0; ai < ${#as[@]}; ai += 4 )) do
            print "dq_${mt}s[$mi] = {"
            print "  type: '${as[$(( ai + 0 ))]}',"
            print "  key: '${as[$(( ai + 1 ))]}',"
            print "  label: '${as[$(( ai + 2 ))]}',"
            print "  help: '${as[$(( ai + 3 ))]}'"
            print "}"
            (( mi++ ))
        done
        print "dq_${mt}n = $mi"
    done

    id=0
    print "var dq_levels = new Array ()"
    for lv in "${!dq_main_data.levels[@]}"; do
        sn=${dq_main_data.levels[$lv].sn}
        nl=${dq_main_data.levels[$lv].nl}
        fn=${dq_main_data.levels[$lv].fn}
        pn=${dq_main_data.levels[$lv].pn}
        af=${dq_main_data.levels[$lv].af}
        attf=${dq_main_data.levels[$lv].attf}
        cusf=${dq_main_data.levels[$lv].cusf}
        if swmuseringroups "vg_att_*"; then
            f=$attf
        else
            f=$cusf
        fi
        if [[ $f == *s* ]] then
            f=selected
        else
            f=''
        fi
        print "dq_levels[$id] = {"
        print "  lv:'$lv', sn:'$sn', nl:'$nl', fn:'$fn', pn:'$pn', sf:'$f'"
        print "}"
        (( id++ ))
    done

    id=0
    print "var dq_queries = new Array ()"
    egrep -v '^#|^$' $SWIFTDATADIR/uifiles/dataqueries/queries.txt \
    | while read -r line; do
        ls=${line%%'|'*}; rest=${line##"$ls"?('|')}
        us=${rest%%'|'*}; rest=${rest##"$us"?('|')}
        gs=${rest%%'|'*}; rest=${rest##"$gs"?('|')}
        qs=${rest%%'|'*}; rest=${rest##"$qs"?('|')}
        ns=${rest%%'|'*}; rest=${rest##"$ns"?('|')}
        ds=${rest%%'|'*}; rest=${rest##"$ds"?('|')}
        hs=${rest%%'|'*}; rest=${rest##"$hs"?('|')}
        if [[ $qs == ${dq_main_data.qid} ]] then
            dq_main_data.querylabel=$ns
        fi
        if [[ $us != '' && " $SWMID " != @(*\ (${us//:/'|'})\ *) ]] then
            continue
        fi
        if [[ $gs != '' ]] && ! swmuseringroups "${gs//:/'|'}"; then
            continue
        fi
        ls=${ls//:/"','"}
        [[ $ls != '' ]] && ls="'$ls'"
        print "dq_queries[$id] = { q:'$qs', n:'$ns', d:'$ds' }"
        print "dq_queries[$id].ls = new Array ($ls)"
        (( id++ ))
    done

    sevi=0
    print "var dq_sevs = new Array ()"
    egrep -v '^#' $SWIFTDATADIR/dpfiles/alarm/sevlist.txt \
    | while read -r line; do
        id=${line%%'|'*} name=${line#*'|'}
        if (( id >= 1 )) && [[ $name != '' ]] then
            print "dq_sevs[$sevi] = { n:'$name', v:'$name' }"
            (( sevi++ ))
        fi
    done
    tmi=0
    print "var dq_tmodes = new Array ()"
    for tm in \
        ticketed:$VG_ALARM_S_MODE_KEEP deferred:$VG_ALARM_S_MODE_DEFER \
        filtered:$VG_ALARM_S_MODE_DROP; \
    do
        print "dq_tmodes[$tmi] = { n:'${tm%:*}', v:'${tm#*:}' }"
        (( tmi++ ))
    done

    id=0
    print "var dq_date_qlinks = new Array ()"
    egrep -v '^#' $SWIFTDATADIR/uifiles/dataqueries/dateqlinks.txt \
    | while read -r line; do
        us=${line%%'|'*}; rest=${line##"$us"?('|')}
        gs=${rest%%'|'*}; rest=${rest##"$gs"?('|')}
        vs=${rest%%'|'*}; rest=${rest##"$vs"?('|')}
        ls=${rest%%'|'*}; rest=${rest##"$ls"?('|')}
        if [[ $us != '' && " $SWMID " != @(*\ (${us//:/'|'})\ *) ]] then
            continue
        fi
        if [[ $gs != '' ]] && ! swmuseringroups "${gs//:/'|'}"; then
            continue
        fi
        qdatelinks[$vs]=$ls
        print "dq_date_qlinks[$id] = { v:'$vs', l:'$ls' }"
        (( id++ ))
    done

    if [[ ${qdatelinks[${dq_main_data.fdate}]} != '' ]] then
        dq_main_data.qlline3="Date: ${qdatelinks[${dq_main_data.fdate}]}"
    else
        dq_main_data.qlline3="Date: ${dq_main_data.fdate}"
        if [[ ${dq_main_data.ldate} != '' ]] then
            dq_main_data.qlline3+=" - ${dq_main_data.ldate}"
        fi
    fi

    case ${dq_main_data.alarmlod} in
    ymd) dq_main_data.qlline4='Data: raw' ;;
    ym)  dq_main_data.qlline4='Data: hourly' ;;
    y)   dq_main_data.qlline4='Data: daily' ;;
    esac

    typeset -n dts=dq_main_data.query.dtools
    for dt in "${!dts.@}"; do
        dt=${dt#dq_main_data.query.dtools.}
        [[ $dt == *.* ]] && continue
        dq_dt_${dt}_emitparams dq_main_data.query.dtools.$dt
    done

    print "var dq_mainurl = '${dq_main_data.fullurl}'"
    print "var dq_baseurl = '${dq_main_data.baseurl}'"
    print "var dq_pid = '${dq_main_data.pid}'"
    print "var dq_mainname = '${dq_main_data.name}'"
    print "var dq_secret = '$(< secret.txt)'"
    if [[ ${dq_main_data.doupdate} == n ]] then
        print "var dq_updaterate = -1"
    else
        print "var dq_updaterate = ${dq_main_data.updaterate}"
    fi

    if [[ $dqmode != embed ]] then
        dq_main_data.qlline1="Query: ${dq_main_data.querylabel}"
        if [[ ${dq_main_data.objlabel} != '' ]] then
            lv=${dq_main_data.objlabel%%:*}
            id=${dq_main_data.objlabel#*:}
            dq_main_data.qlline2="${dq_main_data.levels[$lv].fn}: $id"
        fi
    fi

    if [[ ${dq_main_data.qlline2} != '' ]] then
        s="${dq_main_data.qlline1}-${dq_main_data.qlline2}"
    else
        s="${dq_main_data.qlline1}"
    fi
    print -r "vg_fav_name = unescape ('$(printf '%#H' "$s")')"

    print "dq_init ()"
    print "</script>"
    print "<!--END SKIP-->"

    for vt in ${dq_main_data.displayorder}; do
        dq_vt_${vt}_emitheader dq_main_data.query.vtools.$vt || return 1
    done

    print "<div"
    print "  id=mdiv class=mdivmain"
    print "  style='visibility:hidden' width=100% height=100%"
    print "></div>"

    return 0
}

function dq_main_emit_header_end {
    [[ $DQVTOOLS == n ]] && return 0

    if [[ $dqmode != embed ]] then
        print "</head>"
    fi

    return 0
}

function dq_main_emit_body_begin {
    [[ $DQVTOOLS == n ]] && return 0

    typeset vt bk ql ql1 ql2 ql3 ql4

    if [[ $dqmode != embed ]] then
        if [[
            ${dq_main_data.prev} != '' && -f ../${dq_main_data.prev}/this.txt
        ]] then
            bk="<a"
            bk+=" class=bdivil href='$(< ../${dq_main_data.prev}/this.txt)'"
            bk+=" title='re-run the previous query'"
            bk+=">"
            bk+="&nbsp;Previous Query&nbsp;"
            bk+="</a>"
        fi

        ql1=${dq_main_data.qlline1}
        ql2=${dq_main_data.qlline2}
        ql3=${dq_main_data.qlline3}
        ql4=${dq_main_data.qlline4}

        if [[ ${ph_data.browser} == @(IP|BB) ]] then
            ql=$ql1
            [[ $ql2 != '' ]] && ql+="<br>$ql2"
            [[ $ql3 != '' ]] && ql+="<br>$ql3"
            [[ $ql4 != '' ]] && ql+="<br>$ql4"
            if [[ $bk != '' ]] then
                ql="<div class=bdivrev>$ql<br>$bk</div>"
            else
                ql="<div class=bdivrev>$ql</div>"
            fi
        else
            ql=$ql1
            [[ $ql2 != '' ]] && ql+="&nbsp;-&nbsp;$ql2"
            [[ $ql3 != '' ]] && ql+="&nbsp;-&nbsp;$ql3"
            [[ $ql4 != '' ]] && ql+="&nbsp;-&nbsp;$ql4"
            if [[ $bk != '' ]] then
                ql="<div class=bdivrev>$ql&nbsp;$bk</div>"
            else
                ql="<div class=bdivrev>$ql</div>"
            fi
        fi
        print "<body>"
        ph_emitbodyheader \
            ${dq_main_data.pid} banner ${dq_main_data.pagewidth} \
        "$ql"
    fi

    for vt in ${dq_main_data.displayorder}; do
        dq_vt_${vt}_emitbody dq_main_data.query.vtools.$vt || return 1
    done

    return 0
}

function dq_main_emit_body_end {
    typeset i

    [[ $DQVTOOLS == n ]] && return 0

    if [[ $dqmode != embed ]] then
        ph_emitbodyfooter
        if [[ $DEBUG != "" && -s debug.log ]] then
            print "<pre>"
            print "cache dir=$PWD"
            sed -e 's/</\&#x3C;/' -e 's/>/\&#x3E;/' debug.log
            for i in *.txt *.dds; do
                [[ ! -f $i ]] && continue
                ls -l $i
                case $i in
                *.txt)
                    cat $i
                    ;;
                *.dds)
                    ddsinfo $i
                    ;;
                esac
            done
            print "</pre>"
        fi
        print "</body>"
        print "</html>"
    fi

    return 0
}

function dq_main_emit_sdiv_begin { # $1=id $2=w $3=h $4=mode $5=name $6=style
    typeset id=$1 w=$2 h=$3 mode=$4 name=$5 est=$6
    typeset st st2 data attr

    [[ $dqmode == embed ]] && return

    case $mode in
    min)
        data=none
        attr=''
        ;;
    med)
        data=block
        attr="overflow:auto; width:$w; height:$h"
        ;;
    max)
        data=block
        attr=''
        ;;
    esac
    [[ $est != '' ]] && attr="$est; $attr"

    print "<!--BEGIN SKIP-->"
    print "<script>"
    print "  dq_sdivs['$id'] = { 'w':$w, 'h':$h }"
    print "  dq_setvarstate ('${id}_divmode', '$mode')"
    print "</script>"
    print "<!--END SKIP-->"
    st='class=sdiv'
    st2='line-height:50%'
    print "<br style='$st2'><div $st><table $st><tr $st><td $st>"
    if [[ ${vg.style.general.replacesdivtab} != '' ]] then
        $SHELL $SWIFTDATADIR/uifiles/pref_${vg.style.general.replacesdivtab} rst
    else
        print "<!--BEGIN SKIP-->"
        print "  <table ${st}menu><tr $st><td $st>"
        print "    <a"
        print "      $st href=\"javascript:dq_sdivset('${id}', 'min')\""
        print "      title=\"click to minimize this section\""
        print "    >"
        print "      &nbsp;Min&nbsp;"
        print "    </a>"
        print "    <a"
        print "      $st href=\"javascript:dq_sdivset('${id}', 'med')\""
        print "      title=\"click to shrink this section\""
        print "    >"
        print "      &nbsp;Medium&nbsp;"
        print "    </a>"
        print "    <a"
        print "      $st href=\"javascript:dq_sdivset('${id}', 'max')\""
        print "      title=\"click to maximize this section\""
        print "    >"
        print "      &nbsp;Max&nbsp;"
        print "    </a>"
        if [[ $SHOWHELP == 1 ]] then
            print "    <a"
            print "      $st href=\"javascript:void(0)\""
            print "      onClick=\"vg_showhelp('quicktips/containers')\""
            print "      title=\"click to read about data containers\""
            print "    >"
            print "      &nbsp;?&nbsp;"
            print "    </a>"
        fi
        print "  </td></tr></table>"
        print "<!--END SKIP-->"
    fi
    print "</td></tr><tr $st><td $st><table ${st}line><tr $st><td $st>"
    print "  <div $st id=${id}_data style='display:$data; $attr'>"
}

function dq_main_emit_sdiv_end {
    [[ $dqmode == embed ]] && return

    print "  </div>"
    print "</td></tr></table></td></tr></table>"
    print "</div>"
}

function dq_main_emit_sdivpage_begin { # $1=id $2=file $3=pagen $4=pageindex
    typeset id=$1 file=$2 pagen=$3 pagei=$4
    typeset st stx

    [[ $dqmode == embed ]] && return

    st='class=sdiv'
    stx="style='text-align:left; padding-top:4px; padding-bottom:10px'"
    print "<!--BEGIN SKIP-->"
    print "<table ${st}menu><tr $st $stx><td $st $stx><a"
    print "  ${st}label href='javascript:void(0)'"
    print "  title='current page range'"
    print ">current page:</a><select"
    print "    ${st}page id='sdiv.${file}.select' href=\"javascript:void(0)\""
    print "    onChange=\"dq_sdivpage_goto('$file', '$id', 'select')\""
    print "    title=\"click to select another page\""
    print "  >"
    print "    <option ${st}page value=all>all pages"
}

function dq_main_emit_sdivpage_option { # $1=value $2=label
    typeset val=$1 lab=$2
    typeset st

    [[ $dqmode == embed ]] && return

    st='class=sdiv'
    print "    <option ${st}page value='$val'>$lab"
}

function dq_main_emit_sdivpage_middle { # $1=id $2=file $3=pagen $4=pageindex
    typeset id=$1 file=$2 pagen=$3 pagei=$4
    typeset st stx

    [[ $dqmode == embed ]] && return

    st='class=sdiv'
    stx="style='text-align:left'"
    print "  </select>"
    print "  <a"
    print "    $st href=\"javascript:dq_sdivpage_goto('$file', '$id', 'bwd')\""
    print "    title=\"click to see previous page\""
    print "  >"
    print "    &nbsp;&lt;&nbsp;prev&nbsp;"
    print "  </a>"
    print "  |<a"
    print "    $st href=\"javascript:dq_sdivpage_goto('$file', '$id', 'fwd')\""
    print "    title=\"click to see next page\""
    print "  >"
    print "    &nbsp;next&nbsp;&gt;&nbsp;"
    print "  </a>"
    print "  |<a"
    print "    $st href=\"javascript:dq_sdivpage_goto('$file', '$id', 'all')\""
    print "    title=\"click to all the data\""
    print "  >"
    print "    &nbsp;all pages&nbsp;"
    print "  </a>"
    print "</td></tr></table>"
    print "<!--END SKIP-->"
    print "<div $st id='sdiv.${file}.data'>"
}

function dq_main_emit_sdivpage_end { # $1=id $2=file $3=pagen $4=pageindex
    typeset id=$1 file=$2 pagen=$3 pagei=$4
    typeset st stx

    [[ $dqmode == embed ]] && return

    st='class=sdiv'
    stx="style='text-align:left; padding-top:10px; padding-bottom:4px'"
    print "</div>"
    print "<table ${st}menu><tr $st $stx><td $st $stx><a"
    print "  ${st}label href='javascript:void(0)'"
    print "  title='use the page controls above to navigate'"
    print ">this is a paged display - use the page controls above to navigate"
    print "</a>"
    print "</td></tr></table>"
    print "<!--BEGIN SKIP-->"
    print "<script>"
    if [[ $pagei == +([0-9]) ]] then
        print "dq_sdivpage_set('$file', '$id', $pagen, $pagei, false)"
    else
        print "dq_sdivpage_set('$file', '$id', $pagen, 1, true)"
    fi
    print "</script>"
    print "<!--END SKIP-->"
}

if [[ $SWIFTWARNLEVEL != "" ]] then
    PS4='${.sh.fun}-$LINENO: '
    typeset -ft dq_main_init dq_main_parsedates dq_main_parsedatefilters
    typeset -ft dq_main_run
    typeset -ft dq_main_emit_header_begin dq_main_emit_header_end
    typeset -ft dq_main_emit_body_begin dq_main_emit_body_end
    typeset -ft dq_main_emit_sdiv_begin dq_main_emit_sdiv_end
    typeset -ft dq_main_emit_sdivpage_begin dq_main_emit_sdivpage_middle
    typeset -ft dq_main_emit_sdivpage_end
fi
