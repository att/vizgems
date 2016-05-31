
dq_vt_alarmstatchart_data=(
    deps='' rerun=''
    attrs=() embeds=''
    xaxislabel=''
)

function dq_vt_alarmstatchart_init {
    typeset -n vt=$1
    typeset -A prefs
    typeset n n1 n2 k v id ev
    typeset -n qprefs=$1.args

    for k in "${!qprefs.@}"; do
        n=${k##*$1.args?(.)}
        [[ $n == '' || $n == *.* ]] && continue
        typeset -n vr=$k
        prefs[$n]=$vr
    done
    for n in "${!vars.alarmstatchart_@}"; do
        typeset -n vr=$n
        k=${n##*alarmstatchart_}
        prefs[$k]=$vr
    done

    prefs[xaxisattr_label]="___XAXISLABEL___"

    for k in "${!prefs[@]}"; do
        v=${prefs[$k]}
        if [[ $v == *_URL_BASE_* ]] then
            typeset -n mdt=dq_main_data
            v=${v//_URL_BASE_/${mdt.baseurl}}
        fi
        if [[ $v == *_INV_FILTERS_* ]] then
            typeset -n idt=dq_dt_inv_data
            v=${v//_INV_FILTERS_/${idt.filterstr}}
        fi
        if [[ $v == *_INV_KEEPFILTERS_* ]] then
            typeset -n idt=dq_dt_inv_data
            v=${v//_INV_KEEPFILTERS_/${idt.keepfilterstr}}
        fi
        if [[ $v == *_STAT_FILTERS_* ]] then
            typeset -n sdt=dq_dt_stat_data
            v=${v//_STAT_FILTERS_/${sdt.filterstr}}
        fi
        if [[ $v == *_ALARM_FILTERS_* ]] then
            typeset -n adt=dq_dt_alarm_data
            v=${v//_ALARM_FILTERS_/${adt.filterstr}}
        fi
        if [[ $k == *_* ]] then
            n1=${k%%_*}
            n2=${k#*_}
            typeset -n attr=dq_vt_alarmstatchart_data.attrs.$n1
            if [[ $attr != '' ]] then
                attr+=" $n2='${v//\'/\"}'"
            else
                attr="$n2='${v//\'/\"}'"
            fi
        else
            typeset -n attr=dq_vt_alarmstatchart_data.attrs.$k
            attr=$v
        fi
        if [[ $v == *EMBED:*qimg* ]] then
            ev=$v
            while [[ $ev == *'<qimg>'* ]] do
                id=${ev%%'</qimg>'*}
                ev=${ev#*'</qimg>'}
                id=${id%%'</path>'}
                id=${id##*'<path>'}
                typeset -n embeds=dq_vt_alarmstatchart_data.embeds
                embeds+=" $id"
                dq_vt_alarmstatchart_data.deps+=" $id"
            done
        fi
    done
    dq_vt_alarmstatchart_data.rendermode=${prefs[rendermode]:-final}
    dq_vt_alarmstatchart_data.divmode=${prefs[divmode]:-max}
    dq_vt_alarmstatchart_data.attrs.pagemode=${prefs[pagemode]:-all}
    dq_vt_alarmstatchart_data.attrs.runid=alarmstatchart

    return 0
}

function dq_vt_alarmstatchart_run {
    typeset -n vt=$1
    typeset -n xl=dq_vt_alarmstatchart_data.xaxislabel
    typeset attri embed xlabeli ft lt fspec lspec group lod
    typeset spec dir tffr tlfr tft tlt fileffr filelfr fileft filelt

    group=${dq_main_data.alarmgroup}
    lod=${dq_main_data.alarmlod}
    if [[ ${dq_main_data.remotespecs} != '' ]] then
        for spec in ${dq_main_data.remotespecs}; do
            dir=${spec//'/'/_}
            read tffr tlfr tft tlt < $dir/alarmstat.info
            [[ $tft == 0 || $tlt == 0 ]] && continue
            (( fileft == 0 || fileft > tft )) && fileft=$tft
            (( filelt == 0 || filelt < tlt )) && filelt=$tlt
        done
        [[ $ft == '' ]] && ft=${dq_main_data.ft}
        [[ $lt == '' ]] && lt=${dq_main_data.lt}
        case $group in
        open)
            ft=$fileft
            lt=$filelt
            dq_dt_alarmstat_info_open $ft $lt
            ;;
        all)
            case $lod in
            ymd)
                dq_dt_alarmstat_info_ymd $ft $lt
                ;;
            ym)
                dq_dt_alarmstat_info_ym $ft $lt
                ;;
            y)
                dq_dt_alarmstat_info_y $ft $lt
                ;;
            esac
            ;;
        esac
        dq_dt_alarmstat_data.ft=$ft
        dq_dt_alarmstat_data.lt=$lt
    fi
    ft=${dq_dt_alarmstat_data.ft}
    lt=${dq_dt_alarmstat_data.lt}
    case $group in
    open)
        fspec=$(TZ=$PHTZ printf '%(%Y/%m/%d-%H:%M)T' "#$ft")
        lspec=$(TZ=$PHTZ printf '%(%Y/%m/%d-%H:%M)T' "#$lt")
        xl+="raw"
        xl+=" ($fspec - $lspec)"
        ;;
    all)
        case $lod in
        ymd)
            fspec=$(TZ=$PHTZ printf '%(%Y/%m/%d-%H:%M)T' "#$ft")
            lspec=$(TZ=$PHTZ printf '%(%Y/%m/%d-%H:%M)T' "#$lt")
            xl+="raw"
            xl+=" ($fspec - $lspec)"
            ;;
        ym)
            fspec=$(TZ=$PHTZ printf '%(%Y/%m/%d-%H:00)T' "#$ft")
            lspec=$(TZ=$PHTZ printf '%(%Y/%m/%d-%H:59)T' "#$lt")
            xl+="hourly"
            xl+=" ($fspec - $lspec)"
            ;;
        y)
            fspec=$(TZ=$PHTZ printf '%(%Y/%m/%d)T' "#$ft")
            lspec=$(TZ=$PHTZ printf '%(%Y/%m/%d)T' "#$lt")
            xl+="daily"
            xl+=" ($fspec - $lspec)"
            ;;
        esac
        ;;
    esac

    typeset -n xlabels=dq_dt_alarmstat_data.xlabels
    typeset -n xlabeln=dq_dt_alarmstat_data.xlabeln

    for (( xlabeli = 0; xlabeli < xlabeln; xlabeli++ )) do
        typeset -n xlabel=dq_dt_alarmstat_data.xlabels._$xlabeli
        xl+="|$xlabel"
    done

    export ALARMCHARTATTRFILE=alarmstatchart.attr
    export ALARMCHARTEMBEDLISTFILE=alarmstatchart.embedlist
    rm -f alarmstatchart.*

    (
        print "maxpixel=${dq_main_data.maxpixel}"
        for attri in "${!dq_vt_alarmstatchart_data.attrs.@}"; do
            typeset -n attr=$attri
            print -r "${attri##*.}=${attr//___XAXISLABEL___/$xl}"
        done
        typeset -n ffr=dq_dt_alarmstat_data.fframe
        typeset -n lfr=dq_dt_alarmstat_data.lframe
        print framen=$(( lfr - ffr + 1 ))
    ) > $ALARMCHARTATTRFILE

    (
        for embed in ${dq_vt_alarmstatchart_data.embeds}; do
            print "$embed|$embed.embed"
        done
    ) | sort -u > $ALARMCHARTEMBEDLISTFILE

    STATCHARTATTRFILE=$ALARMCHARTATTRFILE \
    STATCHARTEMBEDLISTFILE=$ALARMCHARTEMBEDLISTFILE \
    STATMAPFILE=$ALARMSTATMAPFILE \
    STATORDERFILE=$ALARMSTATORDERFILE \
    STATORDERSIZE=$ALARMSTATORDERSIZE \
    ddsfilter -osm none -fso vg_dq_vt_statchart.filter.so alarmstat.dds \
    > alarmstatchart.list

    return $?
}

function dq_vt_alarmstatchart_emitheader {
    typeset file

    while read file; do
        [[ $file != *.css ]] && continue
        cat $file
    done < alarmstatchart.list

    return 0
}

function dq_vt_alarmstatchart_emitbody {
    typeset nodataflag=y file pagen page pagei paget s

    [[ ${dq_vt_alarmstatchart_data.rendermode} == embed ]] && return 0

    while read file; do
        [[ $file != *.toc ]] && continue
        cat $file
    done < alarmstatchart.list > alarmstatchart.toc
    while read file; do
        [[ $file != *.gif ]] && continue
        if [[ -s ${file%.gif}.toc ]] then
            page=$(< ${file%.gif}.toc)
            pagei=${page%%'|'*}
            [[ $pagei != '' ]] && print "<!--page $pagei-->"
        fi
        s=
        if [[ -f ${file%.gif}.imginfo ]] then
            set -A s $(< ${file%.gif}.imginfo)
            if (( s[0] > 0 && s[0] < 10000 && s[1] > 0 && s[1] < 10000 )) then
                s[0]="width=${s[0]}" s[1]="height=${s[1]}"
            fi
        fi
        s[${#s[@]}]='class=page'
        if [[ -f ${file%.gif}.cmap ]] then
            print "<img ${s[@]} src='$IMAGEPREFIX$file' usemap='#${file%.gif}'>"
            print "<map name='${file%.gif}'>"
            cat ${file%.gif}.cmap
            print "</map>"
        else
            print "<img ${s[@]} src='$IMAGEPREFIX$file'>"
        fi
        nodataflag=n
    done < alarmstatchart.list > alarmstatchart.html
    [[ nodataflag == n ]] && \
    print "<!--page 123456789-->" >> alarmstatchart.html

    dq_main_emit_sdiv_begin \
        alarmstatchart $SCREENWIDTH 200 "${dq_vt_alarmstatchart_data.divmode}" \
    "Graph View"
    if [[ ${dq_vt_alarmstatchart_data.attrs.pagemode} == all ]] then
        pagen=
    else
        pagen=$(tail -1 alarmstatchart.toc); pagen=${pagen%%'|'*}
        [[ $pagen == 1 ]] && pagen=
    fi
    if [[ $pagen == '' ]] then
        cat alarmstatchart.html
    else
        typeset -n pi=dq_vt_alarmstatchart_data.attrs.pageindex
        pi=${pi:-1}
        dq_main_emit_sdivpage_begin \
            alarmstatchart alarmstatchart.html \
        $pagen $pi
        while read page; do
            pagei=${page%%'|'*}
            paget=${page##*'|'}
            paget="$pagei/$pagen - ${paget:0:20}..."
            dq_main_emit_sdivpage_option "$pagei" "$paget"
        done < alarmstatchart.toc
        dq_main_emit_sdivpage_middle \
            alarmstatchart alarmstatchart.html \
        $pagen $pi
        dq_vt_alarmstatchart_emitpage alarmstatchart.html $pi
        dq_main_emit_sdivpage_end alarmstatchart alarmstatchart.html $pagen $pi
    fi
    [[ $HIDEINFO == '' && -f alarmstatchart.info ]] && cat alarmstatchart.info
    if [[ $nodataflag == y ]] then
        print "The query returned no alarm records."
    fi
    dq_main_emit_sdiv_end

    return 0
}

function dq_vt_alarmstatchart_emitpage { # $1 = file $2 = page
    typeset htmlfile=$1 pageindex=$2
    typeset tocfile=${htmlfile%.html}.toc

    if [[ $pageindex == all ]] then
        cat $htmlfile
    else
        pagen=$(tail -1 $tocfile)
        pagen=${pagen%%'|'*}
        exec 5< $htmlfile
        5<##'<!--page 1-->'
        5<#"<!--page $pageindex-->"
        read -u5
        5<##'<!--page*-->'
        5<#'<!--page 123456789-->'
        5<##EOF
        exec 5<&-
    fi
}

function dq_vt_alarmstatchart_setup {
    dq_main_data.query.vtools.alarmstatchart=(args=())
    typeset -n ascr=dq_main_data.query.vtools.alarmstatchart.args
    typeset -n qdascr=querydata.vt_alarmstatchart
    typeset -n lvs=dq_main_data.levels
    typeset pnlv=${querydata.dt_inv.poutlevel}
    typeset k s w h
    typeset curlfmt=${dq_main_data.acharturlfmt}

    ascr.rendermode=${qdascr.rendermode:-final}
    ascr.pagemode=${prefqp.alarmstatchartpage:-all}

    ascr.sortorder=${qdascr.sortorder:-asset c1 ci c2}
    ascr.grouporder=${qdascr.grouporder:-asset c1 ci c2}
    ascr.metricorder=""
    ascr.displaymode=${vars.statdisp:-actual}

    s=${prefqp.alarmstatchartsize:-'440 120'}
    w=${s%' '*}
    h=${s#*' '}
    if [[ ${vars.alarmstatchartzoom} == 1 ]] then
        (( w *= 2 ))
        (( h *= 2 ))
    fi
    ascr.statchartattr_bgcolor=${prefpp.bgcolor}
    ascr.statchartattr_width=$w
    ascr.statchartattr_height=$h
    for k in "${!qdascr.statchartattr.@}"; do
        typeset -n srcr=$k dstr=ascr.statchartattr_${k##*.}
        dstr=$srcr
    done
    if [[ ${qdascr.statchartnextqid} != '' ]] then
        ascr.statcharturl=${curlfmt//___NEXTQID___/${qdascr.statchartnextqid}}
    fi

    ascr.titleattr_label="%(name)%"
    ascr.titleattr_color=${prefpp.fgcolor}
    ascr.titleattr_fontname=${prefpp.fontfamily}
    ascr.titleattr_fontsize=$(fs2gd ${prefpp.fontsize})
    ascr.titleattr_info="${lvs[$pnlv].fn}: %(name)%"
    for k in "${!qdascr.titleattr.@}"; do
        typeset -n srcr=$k dstr=ascr.titleattr_${k##*.}
        dstr=$srcr
    done
    if [[ ${qdascr.titlenextqid} != '' ]] then
        ascr.titleurl=${curlfmt//___NEXTQID___/${qdascr.titlenextqid}}
    fi

    ascr.metricattr_flags="label_lines:1 draw_impulses"
    ascr.metricattr_flags+=" quartiles:#FFFFFF|#DDDDDD"
    ascr.metricattr_label="%(stat_fulllabel)%"
    ascr.metricattr_color="#006400 blue black cyan magenta"
    ascr.metricattr_capcolor=${prefpp.fgcolor}
    ascr.metricattr_linewidth=2
    ascr.metricattr_avglinewidth=1
    ascr.metricattr_fontname=${prefpp.fontfamily}
    ascr.metricattr_fontsize=$(fs2gd ${prefpp.fontsize})
    for k in "${!qdascr.metricattr.@}"; do
        typeset -n srcr=$k dstr=ascr.metricattr_${k##*.}
        dstr=$srcr
    done
    if [[ ${qdascr.metricnextqid} != '' ]] then
        ascr.metricurl=${curlfmt//___NEXTQID___/${qdascr.metricnextqid}}
    fi

    ascr.xaxisattr_flags="units values axis"
    ascr.xaxisattr_label=""
    ascr.xaxisattr_color=${prefpp.fgcolor}
    ascr.xaxisattr_fontname=${prefpp.fontfamily}
    ascr.xaxisattr_fontsize=$(fs2gd ${prefpp.fontsize})
    for k in "${!qdascr.xaxisattr.@}"; do
        typeset -n srcr=$k dstr=ascr.xaxisattr_${k##*.}
        dstr=$srcr
    done
    if [[ ${qdascr.xaxisnextqid} != '' ]] then
        ascr.xaxisurl=${curlfmt//___NEXTQID___/${qdascr.xaxisnextqid}}
    fi

    ascr.yaxisattr_flags="units values axis"
    ascr.yaxisattr_color=${prefpp.fgcolor}
    ascr.yaxisattr_fontname=${prefpp.fontfamily}
    ascr.yaxisattr_fontsize=$(fs2gd ${prefpp.fontsize})
    for k in "${!qdascr.yaxisattr.@}"; do
        typeset -n srcr=$k dstr=ascr.yaxisattr_${k##*.}
        dstr=$srcr
    done
    if [[ ${qdascr.yaxisnextqid} != '' ]] then
        ascr.yaxisurl=${curlfmt//___NEXTQID___/${qdascr.yaxisnextqid}}
    fi

    return 0
}

if [[ $SWIFTWARNLEVEL != "" ]] then
    PS4='${.sh.fun}-$LINENO: '
    typeset -ft dq_vt_alarmstatchart_init dq_vt_alarmstatchart_run
    typeset -ft dq_vt_alarmstatchart_setup
    typeset -ft dq_vt_alarmstatchart_emitheader dq_vt_alarmstatchart_emitbody
    typeset -ft dq_vt_alarmstatchart_emitpage
fi
