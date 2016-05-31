
dq_vt_alarmstattab_data=(
    deps='' rerun=''
    attrs=() embeds=''
)

function dq_vt_alarmstattab_init {
    typeset -n vt=$1
    typeset -A prefs
    typeset n n1 n2 k v id ev lab fmt group lod
    typeset -n qprefs=$1.args

    for k in "${!qprefs.@}"; do
        n=${k##*$1.args?(.)}
        [[ $n == '' || $n == *.* ]] && continue
        typeset -n vr=$k
        prefs[$n]=$vr
    done
    for n in "${!vars.stattab_@}"; do
        typeset -n vr=$n
        k=${n##*stattab_}
        prefs[$k]=$vr
    done

    group=${dq_main_data.alarmgroup}
    case $group in
    open)
        lab='(raw)'
        fmt='%H:%M'
        ;;
    all)
        lod=${dq_main_data.alarmlod}
        case $lod in
        ymd)
            lab='(raw)'
            fmt='%H:%M'
            ;;
        ym)
            lab='(hourly)'
            fmt='%Y/%m/%d-%H'
            ;;
        y)
            lab='(daily)'
            fmt='%Y/%m/%d'
            ;;
        esac
        ;;
    esac

    prefs[stattabattr_timelabel]=$lab
    prefs[stattabattr_timeformat]=$fmt

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
            typeset -n attr=dq_vt_alarmstattab_data.attrs.$n1
            if [[ $attr != '' ]] then
                attr+=" $n2='${v//\'/\"}'"
            else
                attr="$n2='${v//\'/\"}'"
            fi
        else
            typeset -n attr=dq_vt_alarmstattab_data.attrs.$k
            attr=$v
        fi
        if [[ $v == *EMBED:*qimg* ]] then
            ev=$v
            while [[ $ev == *'<qimg>'* ]] do
                id=${ev%%'</qimg>'*}
                ev=${ev#*'</qimg>'}
                id=${id%%'</path>'}
                id=${id##*'<path>'}
                typeset -n embeds=dq_vt_alarmstattab_data.embeds
                embeds+=" $id"
                dq_vt_alarmstattab_data.deps+=" $id"
            done
        fi
    done
    dq_vt_alarmstattab_data.rendermode=${prefs[rendermode]:-final}
    dq_vt_alarmstattab_data.divmode=${prefs[divmode]:-max}
    dq_vt_alarmstattab_data.attrs.pagemode=${prefs[pagemode]:-all}
    dq_vt_alarmstattab_data.attrs.runid=stattab

    return 0
}

function dq_vt_alarmstattab_run {
    typeset -n vt=$1
    typeset attri embed files filei

    export ALARMSTATTABATTRFILE=alarmstattab.attr
    export ALARMSTATTABEMBEDLISTFILE=alarmstattab.embedlist
    rm -f alarmstattab.*

    (
        for attri in "${!dq_vt_alarmstattab_data.attrs.@}"; do
            typeset -n attr=$attri
            print -r "${attri##*.}=$attr"
        done
        typeset -n ffr=dq_dt_alarmstat_data.fframe
        typeset -n lfr=dq_dt_alarmstat_data.lframe
        print framen=$(( lfr - ffr + 1 ))
    ) > $ALARMSTATTABATTRFILE

    (
        for embed in ${dq_vt_alarmstattab_data.embeds}; do
            print "$embed|$embed.embed"
        done
    ) | sort -u > $ALARMSTATTABEMBEDLISTFILE

    STATTABATTRFILE=$ALARMSTATTABATTRFILE \
    STATTABEMBEDLISTFILE=$ALARMSTATTABEMBEDLISTFILE \
    STATMAPFILE=$ALARMSTATMAPFILE \
    STATORDERFILE=$ALARMSTATORDERFILE \
    STATORDERSIZE=$ALARMSTATORDERSIZE \
    TZ=$PHTZ \
    ddsfilter -osm none -fso vg_dq_vt_stattab.filter.so alarmstat.dds \
    > stattab.list

    return $?
}

function dq_vt_alarmstattab_emitheader {
    typeset file

    while read file; do
        [[ $file != *.css ]] && continue
        cat $file
    done < stattab.list

    return 0
}

function dq_vt_alarmstattab_emitbody {
    typeset nodataflag=y file pagen page pagei paget findex

    [[ ${dq_vt_alarmstattab_data.rendermode} == embed ]] && return 0

    dq_main_emit_sdiv_begin \
        stattab $SCREENWIDTH 200 "${dq_vt_alarmstattab_data.divmode}" \
    "Stat Tabular View" "text-align:left"
    while read file; do
        [[ $file != *.html ]] && continue
        if [[ ${dq_vt_alarmstattab_data.attrs.pagemode} == all ]] then
            pagen=
        else
            pagen=$(tail -1 ${file%.html}.toc); pagen=${pagen%%'|'*}
            [[ $pagen == 1 ]] && pagen=
        fi
        if [[ $pagen == '' ]] then
            cat $file
        else
            findex=${file//*.+([0-9]).*/\1}
            typeset -n pi=dq_vt_alarmstattab_data.attrs.pageindex$findex
            pi=${pi:-1}
            dq_main_emit_sdivpage_begin stattab $file $pagen $pi
            while read page; do
                pagei=${page%%'|'*}
                paget=${page##*'|'}
                paget="$pagei/$pagen - $paget..."
                dq_main_emit_sdivpage_option "$pagei" "$paget"
            done < ${file%.html}.toc
            dq_main_emit_sdivpage_middle stattab $file $pagen $pi
            dq_vt_alarmstattab_emitpage $file $pi
            dq_main_emit_sdivpage_end stattab $file $pagen $pi
        fi
        print "<br>"
        nodataflag=n
    done < stattab.list
    [[ $HIDEINFO == '' && -f statchart.info ]] && cat statchart.info
    if [[ $nodataflag == y ]] then
        print "The query returned no alarm records."
    fi
    dq_main_emit_sdiv_end

    return 0
}

function dq_vt_alarmstattab_emitpage { # $1 = file $2 = page
    typeset htmlfile=$1 pageindex=$2
    typeset tocfile=${htmlfile%.html}.toc pagen

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

function dq_vt_alarmstattab_setup {
    dq_main_data.query.vtools.alarmstattab=(args=())
    typeset -n astr=dq_main_data.query.vtools.alarmstattab.args
    typeset -n qdastr=querydata.vt_alarmstattab
    typeset k
    typeset curlfmt=${dq_main_data.charturlfmt}

    astr.rendermode=${qdastr.rendermode:-final}
    astr.pagemode=${prefqp.alarmstattabpage:-all}

    astr.sortorder=${qdastr.sortorder:-asset c1 ci c2}
    astr.grouporder=${qdastr.grouporder:-asset c1 ci c2}
    astr.metricorder=""

    astr.stattabattr_bgcolor=${prefpp.bgcolor}
    astr.stattabattr_color=${prefpp.fgcolor}
    astr.stattabattr_fontname=${prefpp.fontfamily}
    astr.stattabattr_fontsize=${prefpp.fontsize}
    astr.stattabattr_timeformat=""
    astr.stattabattr_timelabel=""
    for k in "${!qdastr.stattabattr.@}"; do
        typeset -n srcr=$k dastr=astr.stattabattr_${k##*.}
        dastr=$srcr
    done

    astr.titleattr_label="%(name)%"
    for k in "${!qdastr.titleattr.@}"; do
        typeset -n srcr=$k dastr=astr.titleattr_${k##*.}
        dastr=$srcr
    done
    if [[ ${qdastr.titlenextqid} != '' ]] then
        astr.titleurl=${curlfmt//___NEXTQID___/${qdastr.titlenextqid}}
    fi

    astr.metricattr_label="%(stat_invlabel/stat_fulllabel)%"
    astr.metricattr_bgcolor=${prefpp.bgcolor}
    astr.metricattr_color=${prefpp.fgcolor}
    astr.metricattr_fontname=${prefpp.fontfamily}
    astr.metricattr_fontsize=${ fssmaller ${prefpp.fontsize}; }
    for k in "${!qdastr.metricattr.@}"; do
        typeset -n srcr=$k dastr=astr.metricattr_${k##*.}
        dastr=$srcr
    done
    if [[ ${qdastr.metricnextqid} != '' ]] then
        astr.metricurl=${curlfmt//___NEXTQID___/${qdastr.metricnextqid}}
    fi

    return 0
}

if [[ $SWIFTWARNLEVEL != "" ]] then
    PS4='${.sh.fun}-$LINENO: '
    typeset -ft dq_vt_alarmstattab_init dq_vt_alarmstattab_run
    typeset -ft dq_vt_alarmstattab_setup
    typeset -ft dq_vt_alarmstattab_emitbody dq_vt_alarmstattab_emitheader
    typeset -ft dq_vt_alarmstattab_emitpage
fi
