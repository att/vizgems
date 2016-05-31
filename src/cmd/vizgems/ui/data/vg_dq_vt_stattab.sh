
dq_vt_stattab_data=(
    deps='' rerun=''
    attrs=() embeds=''
)

function dq_vt_stattab_init {
    typeset -n vt=$1
    typeset -A prefs
    typeset n n1 n2 k v id ev lab fmt
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

    typeset -n lod=dq_dt_stat_data.lod
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
            typeset -n attr=dq_vt_stattab_data.attrs.$n1
            if [[ $attr != '' ]] then
                attr+=" $n2='${v//\'/\"}'"
            else
                attr="$n2='${v//\'/\"}'"
            fi
        else
            typeset -n attr=dq_vt_stattab_data.attrs.$k
            attr=$v
        fi
        if [[ $v == *EMBED:*qimg* ]] then
            ev=$v
            while [[ $ev == *'<qimg>'* ]] do
                id=${ev%%'</qimg>'*}
                ev=${ev#*'</qimg>'}
                id=${id%%'</path>'}
                id=${id##*'<path>'}
                typeset -n embeds=dq_vt_stattab_data.embeds
                embeds+=" $id"
                dq_vt_stattab_data.deps+=" $id"
            done
        fi
    done
    dq_vt_stattab_data.rendermode=${prefs[rendermode]:-final}
    dq_vt_stattab_data.divmode=${prefs[divmode]:-max}
    dq_vt_stattab_data.attrs.pagemode=${prefs[pagemode]:-all}
    dq_vt_stattab_data.attrs.runid=stattab

    return 0
}

function dq_vt_stattab_run {
    typeset -n vt=$1
    typeset attri embed files filei

    export STATTABATTRFILE=stattab.attr
    export STATTABEMBEDLISTFILE=stattab.embedlist
    rm -f stattab.*

    (
        for attri in "${!dq_vt_stattab_data.attrs.@}"; do
            typeset -n attr=$attri
            print -r "${attri##*.}=$attr"
        done
        typeset -n ffr=dq_dt_stat_data.fframe
        typeset -n lfr=dq_dt_stat_data.lframe
        print framen=$(( lfr - ffr + 1 ))
    ) > $STATTABATTRFILE

    (
        for embed in ${dq_vt_stattab_data.embeds}; do
            print "$embed|$embed.embed"
        done
    ) | sort -u > $STATTABEMBEDLISTFILE

    if [[ -f stat.dds ]] then
        TZ=$PHTZ ddsfilter -osm none -fso vg_dq_vt_stattab.filter.so stat.dds \
        > stattab.list
    else
        files=
        for (( filei = 0; ; filei++ )) do
            [[ ! -f stat_$filei.dds ]] && break
            files+=" stat_$filei.dds"
        done
        ddscat $files \
        | TZ=$PHTZ ddsfilter -osm none -fso vg_dq_vt_stattab.filter.so \
        > stattab.list
    fi

    return $?
}

function dq_vt_stattab_emitheader {
    typeset file

    while read file; do
        [[ $file != *.css ]] && continue
        cat $file
    done < stattab.list

    return 0
}

function dq_vt_stattab_emitbody {
    typeset nodataflag=y file pagen page pagei paget findex

    [[ ${dq_vt_stattab_data.rendermode} == embed ]] && return 0

    dq_main_emit_sdiv_begin \
        stattab $SCREENWIDTH 200 "${dq_vt_stattab_data.divmode}" \
    "Stat Tabular View" "text-align:left"
    while read file; do
        [[ $file != *.html ]] && continue
        if [[ ${dq_vt_stattab_data.attrs.pagemode} == all ]] then
            pagen=
        else
            pagen=$(tail -1 ${file%.html}.toc); pagen=${pagen%%'|'*}
            [[ $pagen == 1 ]] && pagen=
        fi
        if [[ $pagen == '' ]] then
            cat $file
        else
            findex=${file//*.+([0-9]).*/\1}
            typeset -n pi=dq_vt_stattab_data.attrs.pageindex$findex
            pi=${pi:-1}
            dq_main_emit_sdivpage_begin stattab $file $pagen $pi
            while read page; do
                pagei=${page%%'|'*}
                paget=${page##*'|'}
                paget="$pagei/$pagen - $paget..."
                dq_main_emit_sdivpage_option "$pagei" "$paget"
            done < ${file%.html}.toc
            dq_main_emit_sdivpage_middle stattab $file $pagen $pi
            dq_vt_stattab_emitpage $file $pi
            dq_main_emit_sdivpage_end stattab $file $pagen $pi
        fi
        print "<br>"
        nodataflag=n
    done < stattab.list
    [[ $HIDEINFO == '' && -f statchart.info ]] && cat statchart.info
    if [[ $nodataflag == y ]] then
        print "The query returned no statistics records."
    fi
    dq_main_emit_sdiv_end

    return 0
}

function dq_vt_stattab_emitpage { # $1 = file $2 = page
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

function dq_vt_stattab_setup {
    dq_main_data.query.vtools.stattab=(args=())
    typeset -n str=dq_main_data.query.vtools.stattab.args
    typeset -n qdstr=querydata.vt_stattab
    typeset k
    typeset curlfmt=${dq_main_data.charturlfmt}

    str.rendermode=${qdstr.rendermode:-final}
    str.pagemode=${prefqp.stattabpage:-all}

    str.sortorder=${qdstr.sortorder:-asset c1 ci c2}
    str.grouporder=${qdstr.grouporder:-asset c1 ci c2}
    str.metricorder=""

    str.stattabattr_bgcolor=${prefpp.bgcolor}
    str.stattabattr_color=${prefpp.fgcolor}
    str.stattabattr_fontname=${prefpp.fontfamily}
    str.stattabattr_fontsize=${prefpp.fontsize}
    str.stattabattr_timeformat=""
    str.stattabattr_timelabel=""
    for k in "${!qdstr.stattabattr.@}"; do
        typeset -n srcr=$k dstr=str.stattabattr_${k##*.}
        dstr=$srcr
    done

    str.titleattr_label="%(name)%"
    for k in "${!qdstr.titleattr.@}"; do
        typeset -n srcr=$k dstr=str.titleattr_${k##*.}
        dstr=$srcr
    done
    if [[ ${qdstr.titlenextqid} != '' ]] then
        str.titleurl=${curlfmt//___NEXTQID___/${qdstr.titlenextqid}}
    fi

    str.metricattr_label="%(stat_invlabel/stat_fulllabel)%"
    str.metricattr_bgcolor=${prefpp.bgcolor}
    str.metricattr_color=${prefpp.fgcolor}
    str.metricattr_fontname=${prefpp.fontfamily}
    str.metricattr_fontsize=${prefpp.fontsize}
    for k in "${!qdstr.metricattr.@}"; do
        typeset -n srcr=$k dstr=str.metricattr_${k##*.}
        dstr=$srcr
    done
    if [[ ${qdstr.metricnextqid} != '' ]] then
        str.metricurl=${curlfmt//___NEXTQID___/${qdstr.metricnextqid}}
    fi

    return 0
}

if [[ $SWIFTWARNLEVEL != "" ]] then
    PS4='${.sh.fun}-$LINENO: '
    typeset -ft dq_vt_stattab_init dq_vt_stattab_run dq_vt_stattab_setup
    typeset -ft dq_vt_stattab_emitbody dq_vt_stattab_emitheader
    typeset -ft dq_vt_stattab_emitpage
fi
