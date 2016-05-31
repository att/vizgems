
dq_vt_alarmgrid_data=(
    deps='' rerun=''
    attrs=() embeds=''
)

function dq_vt_alarmgrid_init {
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
    for n in "${!vars.alarmgrid_@}"; do
        typeset -n vr=$n
        k=${n##*alarmgrid_}
        prefs[$k]=$vr
    done

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
            typeset -n attr=dq_vt_alarmgrid_data.attrs.$n1
            if [[ $attr != '' ]] then
                attr+=" $n2='${v//\'/\"}'"
            else
                attr="$n2='${v//\'/\"}'"
            fi
        else
            typeset -n attr=dq_vt_alarmgrid_data.attrs.$k
            attr=$v
        fi
        if [[ $v == *EMBED:*qimg* ]] then
            ev=$v
            while [[ $ev == *'<qimg>'* ]] do
                id=${ev%%'</qimg>'*}
                ev=${ev#*'</qimg>'}
                id=${id%%'</path>'}
                id=${id##*'<path>'}
                typeset -n embeds=dq_vt_alarmgrid_data.embeds
                embeds+=" $id"
                dq_vt_alarmgrid_data.deps+=" $id"
            done
        fi
    done
    dq_vt_alarmgrid_data.rendermode=${prefs[rendermode]:-final}
    dq_vt_alarmgrid_data.divmode=${prefs[divmode]:-max}
    dq_vt_alarmgrid_data.attrs.pagemode=${prefs[pagemode]:-all}
    dq_vt_alarmgrid_data.attrs.runid=alarmgrid

    return 0
}

function dq_vt_alarmgrid_run {
    typeset -n vt=$1
    typeset attri embed

    export ALARMGRIDATTRFILE=alarmgrid.attr
    export ALARMGRIDEMBEDLISTFILE=alarmgrid.embedlist
    rm -f alarmgrid.*

    (
        print "maxpixel=${dq_main_data.maxpixel}"
        print "alarmgroup=${dq_dt_alarm_data.group}"
        for attri in "${!dq_vt_alarmgrid_data.attrs.@}"; do
            typeset -n attr=$attri
            print -r "${attri##*.}=$attr"
        done
    ) > $ALARMGRIDATTRFILE

    (
        for embed in ${dq_vt_alarmgrid_data.embeds}; do
            print "$embed|$embed.embed"
        done
    ) | sort -u > $ALARMGRIDEMBEDLISTFILE

    ddsfilter -osm none -fso vg_dq_vt_alarmgrid.filter.so alarm.dds \
    > alarmgrid.list
    return $?
}

function dq_vt_alarmgrid_emitheader {
    typeset file

    while read file; do
        [[ $file != *.css ]] && continue
        cat $file
    done < alarmgrid.list

    return 0
}

function dq_vt_alarmgrid_emitbody {
    typeset nodataflag=y file pagen page pagei paget s

    [[ ${dq_vt_alarmgrid_data.rendermode} == embed ]] && return 0

    while read file; do
        [[ $file != *.toc ]] && continue
        cat $file
    done < alarmgrid.list > alarmgrid.toc
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
    done < alarmgrid.list > alarmgrid.html
    [[ nodataflag == n ]] && print "<!--page 123456789-->" >> alarmgrid.html

    dq_main_emit_sdiv_begin \
        alarmgrid $SCREENWIDTH 200 "${dq_vt_alarmgrid_data.divmode}" \
    "Alarm View"
    if [[ ${dq_vt_alarmgrid_data.attrs.pagemode} == all ]] then
        pagen=
    else
        pagen=$(tail -1 alarmgrid.toc); pagen=${pagen%%'|'*}
        [[ $pagen == 1 ]] && pagen=
    fi
    if [[ $pagen == '' ]] then
        cat alarmgrid.html
    else
        typeset -n pi=dq_vt_alarmgrid_data.attrs.pageindex
        pi=${pi:-1}
        dq_main_emit_sdivpage_begin alarmgrid alarmgrid.html $pagen $pi
        while read page; do
            pagei=${page%%'|'*}
            paget=${page##*'|'}
            paget="$pagei/$pagen - ${paget:0:20}..."
            dq_main_emit_sdivpage_option "$pagei" "$paget"
        done < alarmgrid.toc
        dq_main_emit_sdivpage_middle alarmgrid alarmgrid.html $pagen $pi
        dq_vt_alarmgrid_emitpage alarmgrid.html $pi
        dq_main_emit_sdivpage_end alarmgrid alarmgrid.html $pagen $pi
    fi
    [[ $HIDEINFO == '' && -f alarmgrid.info ]] && cat alarmgrid.info
    if [[ $nodataflag == y ]] then
        print "The query returned no alarm records."
    fi
    dq_main_emit_sdiv_end

    return 0
}

function dq_vt_alarmgrid_emitpage { # $1 = file $2 = page
    typeset htmlfile=$1 pageindex=$2
    typeset tocfile=${htmlfile%.html}.toc
    typeset pagen

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

function dq_vt_alarmgrid_setup {
    dq_main_data.query.vtools.alarmgrid=(args=())
    typeset -n agr=dq_main_data.query.vtools.alarmgrid.args
    typeset -n qdagr=querydata.vt_alarmgrid
    typeset -n lvs=dq_main_data.levels
    typeset pnlv=${querydata.dt_inv.poutlevel}
    typeset k
    typeset nurlfmt=${dq_main_data.nodeurlfmt}

    agr.rendermode=${qdagr.rendermode:-final}
    agr.pagemode=${prefqp.alarmgridpage:-all}

    agr.alarmgridattr_flags="map_filtered_deferred"
    agr.alarmgridattr_bgcolor=${prefpp.bgcolor}
    agr.alarmgridattr_color=${prefpp.fgcolor}
    agr.alarmgridattr_width=125
    agr.alarmgridattr_height=32
    for k in "${!qdagr.alarmgridattr.@}"; do
        typeset -n srcr=$k dstr=agr.alarmgridattr_${k##*.}
        dstr=$srcr
    done
    if [[ ${qdagr.alarmgridnextqid} != '' ]] then
        scr.alarmgridurl=${nurlfmt//___NEXTQID___/${qdagr.alarmgridnextqid}}
    fi

    agr.titleattr_color=${prefpp.fgcolor}
    agr.titleattr_fontname=${prefpp.fontfamily}
    agr.titleattr_fontsize=$(fs2gd ${prefpp.fontsize})
    agr.titleattr_label="${lvs[$pnlv].fn}: %(nodename)%"
    agr.titleattr_info="${lvs[$pnlv].fn}: %(name)%"
    for k in "${!qdagr.titleattr.@}"; do
        typeset -n srcr=$k dstr=agr.titleattr_${k##*.}
        dstr=$srcr
    done
    if [[ ${qdagr.titlenextqid} != '' ]] then
        scr.titleurl=${nurlfmt//___NEXTQID___/${qdagr.titlenextqid}}
    fi

    agr.alarmattr_flags="filled outline text"
    agr.alarmattr_color=${dq_main_data.alarmattr.color}
    agr.alarmattr_linewidth=1
    agr.alarmattr_fontname=${prefpp.fontfamily}
    agr.alarmattr_fontsize=$(fs2gd ${prefpp.fontsize})
    for k in "${!qdagr.alarmattr.@}"; do
        typeset -n srcr=$k dstr=agr.alarmattr_${k##*.}
        dstr=$srcr
    done
    if [[ ${qdagr.alarmnextqid} != '' ]] then
        scr.alarmurl=${nurlfmt//___NEXTQID___/${qdagr.alarmnextqid}}
    fi

    agr.xaxisattr_flags=""
    agr.xaxisattr_label=${dq_main_data.alarmattr.severitylabel}
    agr.xaxisattr_color=${prefpp.fgcolor}
    agr.xaxisattr_fontname=${prefpp.fontfamily}
    agr.xaxisattr_fontsize=$(fs2gd ${prefpp.fontsize})
    for k in "${!qdagr.xaxisattr.@}"; do
        typeset -n srcr=$k dstr=agr.xaxisattr_${k##*.}
        dstr=$srcr
    done
    if [[ ${qdagr.xaxisnextqid} != '' ]] then
        scr.xaxisurl=${nurlfmt//___NEXTQID___/${qdagr.xaxisnextqid}}
    fi

    typeset -n tlr=dq_main_data.alarmattr.ticketlabel1${prefqp.alarmgridmode}
    agr.yaxisattr_flags=""
    agr.yaxisattr_label=$tlr
    agr.yaxisattr_color=${prefpp.fgcolor}
    agr.yaxisattr_fontname=${prefpp.fontfamily}
    agr.yaxisattr_fontsize=$(fs2gd ${prefpp.fontsize})
    for k in "${!qdagr.yaxisattr.@}"; do
        typeset -n srcr=$k dstr=agr.yaxisattr_${k##*.}
        dstr=$srcr
    done
    if [[ ${qdagr.yaxisnextqid} != '' ]] then
        scr.yaxisurl=${nurlfmt//___NEXTQID___/${qdagr.yaxisnextqid}}
    fi

    return 0
}

if [[ $SWIFTWARNLEVEL != "" ]] then
    PS4='${.sh.fun}-$LINENO: '
    typeset -ft dq_vt_alarmgrid_init dq_vt_alarmgrid_run dq_vt_alarmgrid_setup
    typeset -ft dq_vt_alarmgrid_emitbody dq_vt_alarmgrid_emitheader
    typeset -ft dq_vt_alarmgrid_emitpage
fi
