
dq_vt_statchart_data=(
    deps='' rerun=''
    attrs=() embeds=''
    xaxislabel=''
)

function dq_vt_statchart_init {
    typeset -n vt=$1
    typeset -A prefs
    typeset n n1 n2 k v id ev xl xlabeli ft lt fspec lspec
    typeset -n qprefs=$1.args

    for k in "${!qprefs.@}"; do
        n=${k##*$1.args?(.)}
        [[ $n == '' || $n == *.* ]] && continue
        typeset -n vr=$k
        prefs[$n]=$vr
    done
    for n in "${!vars.statchart_@}"; do
        typeset -n vr=$n
        k=${n##*statchart_}
        prefs[$k]=$vr
    done

    ft=${dq_main_data.ft}
    lt=${dq_main_data.lt}
    typeset -n lod=dq_dt_stat_data.lod
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

    typeset -n xlabels=dq_dt_stat_data.xlabels
    typeset -n xlabeln=dq_dt_stat_data.xlabeln

    for (( xlabeli = 0; xlabeli < xlabeln; xlabeli++ )) do
        typeset -n xlabel=dq_dt_stat_data.xlabels._$xlabeli
        xl+="|$xlabel"
    done
    prefs[xaxisattr_label]=$xl

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
            typeset -n attr=dq_vt_statchart_data.attrs.$n1
            if [[ $attr != '' ]] then
                attr+=" $n2='${v//\'/\"}'"
            else
                attr="$n2='${v//\'/\"}'"
            fi
        else
            typeset -n attr=dq_vt_statchart_data.attrs.$k
            attr=$v
        fi
        if [[ $v == *EMBED:*qimg* ]] then
            ev=$v
            while [[ $ev == *'<qimg>'* ]] do
                id=${ev%%'</qimg>'*}
                ev=${ev#*'</qimg>'}
                id=${id%%'</path>'}
                id=${id##*'<path>'}
                typeset -n embeds=dq_vt_statchart_data.embeds
                embeds+=" $id"
                dq_vt_statchart_data.deps+=" $id"
            done
        fi
    done
    dq_vt_statchart_data.rendermode=${prefs[rendermode]:-final}
    dq_vt_statchart_data.divmode=${prefs[divmode]:-max}
    dq_vt_statchart_data.attrs.pagemode=${prefs[pagemode]:-all}
    dq_vt_statchart_data.attrs.runid=statchart

    return 0
}

function dq_vt_statchart_run {
    typeset -n vt=$1
    typeset attri embed files filei

    export STATCHARTATTRFILE=statchart.attr
    export STATCHARTEMBEDLISTFILE=statchart.embedlist
    rm -f statchart.*

    (
        print "maxpixel=${dq_main_data.maxpixel}"
        for attri in "${!dq_vt_statchart_data.attrs.@}"; do
            typeset -n attr=$attri
            print -r "${attri##*.}=$attr"
        done
        typeset -n ffr=dq_dt_stat_data.fframe
        typeset -n lfr=dq_dt_stat_data.lframe
        print framen=$(( lfr - ffr + 1 ))
    ) > $STATCHARTATTRFILE

    (
        for embed in ${dq_vt_statchart_data.embeds}; do
            print "$embed|$embed.embed"
        done
    ) | sort -u > $STATCHARTEMBEDLISTFILE

    if [[ -f stat.dds ]] then
        ddsfilter -osm none -fso vg_dq_vt_statchart.filter.so stat.dds \
        > statchart.list
    else
        files=
        for (( filei = 0; ; filei++ )) do
            [[ ! -f stat_$filei.dds ]] && break
            files+=" stat_$filei.dds"
        done
        ddscat $files \
        | ddsfilter -osm none -fso vg_dq_vt_statchart.filter.so \
        > statchart.list
    fi
    return $?
}

function dq_vt_statchart_emitheader {
    typeset file

    while read file; do
        [[ $file != *.css ]] && continue
        cat $file
    done < statchart.list

    return 0
}

function dq_vt_statchart_emitbody {
    typeset nodataflag=y file pagen page pagei paget s

    [[ ${dq_vt_statchart_data.rendermode} == embed ]] && return 0

    while read file; do
        [[ $file != *.toc ]] && continue
        cat $file
    done < statchart.list > statchart.toc
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
    done < statchart.list > statchart.html
    [[ nodataflag == n ]] && print "<!--page 123456789-->" >> statchart.html

    dq_main_emit_sdiv_begin \
        statchart $SCREENWIDTH 200 "${dq_vt_statchart_data.divmode}" \
    "Graph View"
    if [[ ${dq_vt_statchart_data.attrs.pagemode} == all ]] then
        pagen=
    else
        pagen=$(tail -1 statchart.toc); pagen=${pagen%%'|'*}
        [[ $pagen == 1 ]] && pagen=
    fi
    if [[ $pagen == '' ]] then
        cat statchart.html
    else
        typeset -n pi=dq_vt_statchart_data.attrs.pageindex
        pi=${pi:-1}
        dq_main_emit_sdivpage_begin statchart statchart.html $pagen $pi
        while read page; do
            pagei=${page%%'|'*}
            paget=${page##*'|'}
            paget="$pagei/$pagen - ${paget:0:20}..."
            dq_main_emit_sdivpage_option "$pagei" "$paget"
        done < statchart.toc
        dq_main_emit_sdivpage_middle statchart statchart.html $pagen $pi
        dq_vt_statchart_emitpage statchart.html $pi
        dq_main_emit_sdivpage_end statchart statchart.html $pagen $pi
    fi
    [[ $HIDEINFO == '' && -f statchart.info ]] && cat statchart.info
    if [[ $nodataflag == y ]] then
        print "The query returned no statistics records."
    fi
    dq_main_emit_sdiv_end

    return 0
}

function dq_vt_statchart_emitpage { # $1 = file $2 = page
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

function dq_vt_statchart_setup {
    dq_main_data.query.vtools.statchart=(args=())
    typeset -n scr=dq_main_data.query.vtools.statchart.args
    typeset -n qdscr=querydata.vt_statchart
    typeset -n lvs=dq_main_data.levels
    typeset pnlv=${querydata.dt_inv.poutlevel}
    typeset k s w h
    typeset curlfmt=${dq_main_data.charturlfmt}

    scr.rendermode=${qdscr.rendermode:-final}
    scr.pagemode=${prefqp.statchartpage:-all}

    scr.sortorder=${qdscr.sortorder:-asset c1 ci c2}
    scr.grouporder=${qdscr.grouporder:-asset c1 ci c2}
    scr.metricorder=${qdscr.metricorder}
    scr.displaymode=${vars.statdisp:-actual}

    s=${prefqp.statchartsize:-'440 120'}
    w=${s%' '*}
    h=${s#*' '}
    if [[ ${vars.statchartzoom} == 1 ]] then
        (( w *= 2 ))
        (( h *= 2 ))
    fi
    scr.statchartattr_bgcolor=${prefpp.bgcolor}
    scr.statchartattr_width=$w
    scr.statchartattr_height=$h
    for k in "${!qdscr.statchartattr.@}"; do
        typeset -n srcr=$k dstr=scr.statchartattr_${k##*.}
        dstr=$srcr
    done
    if [[ ${qdscr.statchartnextqid} != '' ]] then
        scr.statcharturl=${curlfmt//___NEXTQID___/${qdscr.statchartnextqid}}
    fi

    scr.titleattr_label="%(name)%"
    scr.titleattr_color=${prefpp.fgcolor}
    scr.titleattr_fontname=${prefpp.fontfamily}
    scr.titleattr_fontsize=$(fs2gd ${prefpp.fontsize})
    scr.titleattr_info="${lvs[$pnlv].fn}: %(name)%"
    for k in "${!qdscr.titleattr.@}"; do
        typeset -n srcr=$k dstr=scr.titleattr_${k##*.}
        dstr=$srcr
    done
    if [[ ${qdscr.titlenextqid} != '' ]] then
        scr.titleurl=${curlfmt//___NEXTQID___/${qdscr.titlenextqid}}
    fi

    scr.metricattr_flags="label_lines:1 quartiles:#FFFFFF|#DDDDDD"
    if [[ ${#dq_main_data.timeranges[@]} != 0 ]] then
        scr.metricattr_flags=${scr.metricattr_flags//' '/' draw_multilines '}
    fi
    scr.metricattr_label="%(stat_invlabel/stat_fulllabel)%"
    scr.metricattr_color="#006400 blue black cyan magenta"
    scr.metricattr_capcolor=${prefpp.fgcolor}
    scr.metricattr_linewidth=2
    scr.metricattr_avglinewidth=1
    scr.metricattr_fontname=${prefpp.fontfamily}
    scr.metricattr_fontsize=$(fs2gd ${prefpp.fontsize})
    for k in "${!qdscr.metricattr.@}"; do
        typeset -n srcr=$k dstr=scr.metricattr_${k##*.}
        dstr=$srcr
    done
    if [[ ${qdscr.metricnextqid} != '' ]] then
        scr.metricurl=${curlfmt//___NEXTQID___/${qdscr.metricnextqid}}
    fi

    scr.xaxisattr_flags="units values axis"
    scr.xaxisattr_label=""
    scr.xaxisattr_color=${prefpp.fgcolor}
    scr.xaxisattr_fontname=${prefpp.fontfamily}
    scr.xaxisattr_fontsize=$(fs2gd ${prefpp.fontsize})
    for k in "${!qdscr.xaxisattr.@}"; do
        typeset -n srcr=$k dstr=scr.xaxisattr_${k##*.}
        dstr=$srcr
    done
    if [[ ${qdscr.xaxisnextqid} != '' ]] then
        scr.xaxisurl=${curlfmt//___NEXTQID___/${qdscr.xaxisnextqid}}
    fi

    scr.yaxisattr_flags="units values axis"
    scr.yaxisattr_color=${prefpp.fgcolor}
    scr.yaxisattr_fontname=${prefpp.fontfamily}
    scr.yaxisattr_fontsize=$(fs2gd ${prefpp.fontsize})
    for k in "${!qdscr.yaxisattr.@}"; do
        typeset -n srcr=$k dstr=scr.yaxisattr_${k##*.}
        dstr=$srcr
    done
    if [[ ${qdscr.yaxisnextqid} != '' ]] then
        scr.yaxisurl=${curlfmt//___NEXTQID___/${qdscr.yaxisnextqid}}
    fi

    return 0
}

if [[ $SWIFTWARNLEVEL != "" ]] then
    PS4='${.sh.fun}-$LINENO: '
    typeset -ft dq_vt_statchart_init dq_vt_statchart_run dq_vt_statchart_setup
    typeset -ft dq_vt_statchart_emitbody dq_vt_statchart_emitheader
    typeset -ft dq_vt_statchart_emitpage
fi
