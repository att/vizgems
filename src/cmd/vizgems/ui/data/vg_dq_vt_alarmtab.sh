
dq_vt_alarmtab_data=(
    deps='' rerun=''
    attrs=() embeds=''
)

function dq_vt_alarmtab_init {
    typeset -n vt=$1
    typeset -A prefs
    typeset n n1 n2 k v id ev showclear
    typeset -n qprefs=$1.args

    if swmuseringroups "vg_att_admin|vg_hevalarmclear"; then
        if [[ ${dq_dt_alarm_data.allowclear} == y ]] then
            showclear=y
        fi
    fi

    for k in "${!qprefs.@}"; do
        n=${k##*$1.args?(.)}
        [[ $n == '' || $n == *.* ]] && continue
        typeset -n vr=$k
        prefs[$n]=$vr
    done
    for n in "${!vars.alarmtab_@}"; do
        typeset -n vr=$n
        k=${n##*alarmtab_}
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
        if [[ $v == *_SHOWCLEAR_* ]] then
            if [[ $showclear == y ]] then
                v=${v//_SHOWCLEAR_/}
            else
                v=${v//_SHOWCLEAR_*_SHOWCLEAR_/}
            fi
        fi
        if [[ $k == *_* ]] then
            n1=${k%%_*}
            n2=${k#*_}
            typeset -n attr=dq_vt_alarmtab_data.attrs.$n1
            if [[ $attr != '' ]] then
                attr+=" $n2='${v//\'/\"}'"
            else
                attr="$n2='${v//\'/\"}'"
            fi
        else
            typeset -n attr=dq_vt_alarmtab_data.attrs.$k
            attr=$v
        fi
        if [[ $v == *EMBED:*qimg* ]] then
            ev=$v
            while [[ $ev == *'<qimg>'* ]] do
                id=${ev%%'</qimg>'*}
                ev=${ev#*'</qimg>'}
                id=${id%%'</path>'}
                id=${id##*'<path>'}
                typeset -n embeds=dq_vt_alarmtab_data.embeds
                embeds+=" $id"
                dq_vt_alarmtab_data.deps+=" $id"
            done
        fi
    done
    dq_vt_alarmtab_data.rendermode=${prefs[rendermode]:-final}
    dq_vt_alarmtab_data.divmode=${prefs[divmode]:-max}
    dq_vt_alarmtab_data.attrs.pagemode=${prefs[pagemode]:-all}
    dq_vt_alarmtab_data.attrs.runid=alarmtab

    return 0
}

function dq_vt_alarmtab_run {
    typeset -n vt=$1
    typeset attri embed u soflag=n

    export ALARMTABATTRFILE=alarmtab.attr
    export ALARMTABEMBEDLISTFILE=alarmtab.embedlist
    rm -f alarmtab.*

    (
        u=${dq_main_data.fullurl}
        u=${u//alarmtab_sortorder=*([!&])/}
        u=${u//'&&'/'&'}
        print -r "sorturl=$u"
        for attri in "${!dq_vt_alarmtab_data.attrs.@}"; do
            typeset -n attr=$attri
            print -r "${attri##*.}=$attr"
            [[ ${attri##*.} == sortorder ]] && soflag=y
        done
        [[ $soflag != y ]] && print -r "sortorder=0"
    ) > $ALARMTABATTRFILE

    (
        for embed in ${dq_vt_alarmtab_data.embeds}; do
            print "$embed|$embed.embed"
        done
    ) | sort -u > $ALARMTABEMBEDLISTFILE

    TZ=$PHTZ ddsfilter -osm none -fso vg_dq_vt_alarmtab.filter.so alarm.dds \
    > alarmtab.list
    return $?
}

function dq_vt_alarmtab_emitheader {
    typeset file

    while read file; do
        [[ $file != *.css ]] && continue
        cat $file
    done < alarmtab.list

    return 0
}

function dq_vt_alarmtab_emitbody {
    typeset nodataflag=y file pagen page pagei paget findex

    [[ ${dq_vt_alarmtab_data.rendermode} == embed ]] && return 0

    dq_main_emit_sdiv_begin \
        alarmtab $SCREENWIDTH 200 "${dq_vt_alarmtab_data.divmode}" \
    "Alarm View"
    while read file; do
        [[ $file != *.html ]] && continue
        if [[ ${dq_vt_alarmtab_data.attrs.pagemode} == all ]] then
            pagen=
        else
            pagen=$(tail -1 ${file%.html}.toc); pagen=${pagen%%'|'*}
            [[ $pagen == 1 ]] && pagen=
        fi
        if [[ $pagen == '' ]] then
            cat $file
        else
            findex=${file//*.+([0-9]).*/\1}
            typeset -n pi=dq_vt_alarmtab_data.attrs.pageindex$findex
            pi=${pi:-1}
            dq_main_emit_sdivpage_begin alarmtab $file $pagen $pi
            while read page; do
                pagei=${page%%'|'*}
                paget=${page##*'|'}
                paget="$pagei/$pagen - $paget..."
                dq_main_emit_sdivpage_option "$pagei" "$paget"
            done < ${file%.html}.toc
            dq_main_emit_sdivpage_middle alarmtab $file $pagen $pi
            dq_vt_alarmtab_emitpage $file $pi
            dq_main_emit_sdivpage_end alarmtab $file $pagen $pi
        fi
        print "<br>"
        nodataflag=n
    done < alarmtab.list
    [[ $HIDEINFO == '' && -f alarmtab.info ]] && cat alarmtab.info
    if [[ $nodataflag == y ]] then
        print "The query returned no alarm records."
    fi
    dq_main_emit_sdiv_end

    return 0
}

function dq_vt_alarmtab_emitpage { # $1 = file $2 = page
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

function dq_vt_alarmtab_setup {
    dq_main_data.query.vtools.alarmtab=(args=())
    typeset -n atr=dq_main_data.query.vtools.alarmtab.args
    typeset -n qdatr=querydata.vt_alarmtab
    typeset k ci hdr row url sep
    typeset nurlfmt=${dq_main_data.nodeurlfmt}
    typeset aurlfmt=${dq_main_data.alarmurlfmt}
    typeset pnodedata=(
        c0=(
            hdr='C:Time'
            row='LS:%(alarm_timeissued/)%'
            url=''
        )
        c1=(
            hdr='C:Sev'
            row='LS:%(alarm_severity/)%'
            url=''
        )
        c2=(
            hdr='C:Asset'
            row='LS:%(name/_id_)%'
            url=${nurlfmt//___NEXTQID___/${qdatr.pnodenextqid}}
        )
        c3=(
            hdr='C:AlarmID'
            row='LS:%(alarm_alarmid/)%'
            url=''
        )
        c4=(
            hdr='C:CCID'
            row='LS:%(alarm_ccid/)%'
            url=''
        )
        c5=(
            hdr='C:Text'
            row='%(alarm_text/)%'
            url=''
        )
        c6=(
            hdr='C:Comment'
            row='%(alarm_comment/)%'
            url=''
        )
        c7=(
            hdr='_SHOWCLEAR_C:Clear_SHOWCLEAR_'
            row='_SHOWCLEAR_Clear_SHOWCLEAR_'
            url="_SHOWCLEAR_${aurlfmt}_SHOWCLEAR_"
        )
    )
    typeset snodedata=()
    typeset ppedgedata=() psedgedata=() spedgedata=() ssedgedata=()

    atr.rendermode=${qdatr.rendermode:-final}
    atr.pagemode=${prefqp.alarmtabpage:-all}

    atr.sortorder=${qdatr.sortorder:-0r 1 2 3 4}
    atr.grouporder=${qdatr.grouporder}

    atr.alarmtabattr_flags="map_filtered_deferred"
    atr.alarmtabattr_bgcolor=${prefpp.bgcolor}
    atr.alarmtabattr_color=${prefpp.fgcolor}
    atr.alarmtabattr_fontname=${prefpp.fontfamily}
    atr.alarmtabattr_fontsize=${prefpp.fontsize}
    atr.alarmtabattr_sevmap=${dq_main_data.alarmattr.color}
    atr.alarmtabattr_sevnames=${dq_main_data.alarmattr.severitylabel}
    atr.alarmtabattr_tmodenames=${dq_main_data.alarmattr.ticketlabel21}
    for k in "${!qdatr.alarmtabattr.@}"; do
        typeset -n srcr=$k dstr=atr.alarmtabattr_${k##*.}
        dstr=$srcr
    done

    atr.pnodeattr_label="%(name)%"
    atr.pnodeattr_fontname=${prefpp.fontfamily}
    atr.pnodeattr_fontsize=${ fssmaller ${prefpp.fontsize}; }
    hdr= row= url= sep=
    for (( ci = 0; ; ci++ )) do
        typeset -n cr=pnodedata.c$ci
        [[ ${cr.row} == '' ]] && break
        hdr+="$sep${cr.hdr}"
        row+="$sep${cr.row}"
        url+="$sep${cr.url}"
        sep='|'
    done
    if [[ ${querydata.vt_alarmtab.pnodedata.c0.row} != '' ]] then
        hdr= row= url= sep=
        for (( ci = 0; ; ci++ )) do
            typeset -n cr=querydata.vt_alarmtab.pnodedata.c$ci
            [[ ${cr.row} == '' ]] && break
            hdr+="$sep${cr.hdr}"
            row+="$sep${cr.row}"
            url+="$sep${cr.url}"
            sep='|'
        done
    fi
    atr.pnodeattr_hdr=$hdr
    atr.pnodeattr_row=$row
    for k in "${!qdatr.pnodeattr.@}"; do
        typeset -n srcr=$k dstr=atr.pnodeattr_${k##*.}
        dstr=$srcr
    done
    if [[ ${qdatr.pnodeurl} != '' ]] then
        atr.pnodeurl=${qdatr.pnodeurl}
    elif [[ $url != '' && ${qdatr.pnodenextqid} != '' ]] then
        atr.pnodeurl=$url
    fi

    atr.snodeattr_label="_SKIP_"
    atr.snodeattr_fontname=${prefpp.fontfamily}
    atr.snodeattr_fontsize=${ fssmaller ${prefpp.fontsize}; }
    hdr= row= url= sep=
    for (( ci = 0; ; ci++ )) do
        typeset -n cr=snodedata.c$ci
        [[ ${cr.row} == '' ]] && break
        hdr+="$sep${cr.hdr}"
        row+="$sep${cr.row}"
        url+="$sep${cr.url}"
        sep='|'
    done
    if [[ ${querydata.vt_alarmtab.snodedata.c0.row} != '' ]] then
        hdr= row= url= sep=
        for (( ci = 0; ; ci++ )) do
            typeset -n cr=querydata.vt_alarmtab.snodedata.c$ci
            [[ ${cr.row} == '' ]] && break
            hdr+="$sep${cr.hdr}"
            row+="$sep${cr.row}"
            url+="$sep${cr.url}"
            sep='|'
        done
    fi
    atr.snodeattr_hdr=$hdr
    atr.snodeattr_row=$row
    for k in "${!qdatr.snodeattr.@}"; do
        typeset -n srcr=$k dstr=atr.snodeattr_${k##*.}
        dstr=$srcr
    done
    if [[ ${qdatr.snodeurl} != '' ]] then
        atr.snodeurl=${qdatr.snodeurl}
    elif [[ $url != '' && ${qdatr.snodenextqid} != '' ]] then
        atr.snodeurl=$url
    fi

    atr.ppedgeattr_label="_SKIP_"
    atr.ppedgeattr_fontname=${prefpp.fontfamily}
    atr.ppedgeattr_fontsize=${ fssmaller ${prefpp.fontsize}; }
    hdr= row= url= sep=
    for (( ci = 0; ; ci++ )) do
        typeset -n cr=ppedgedata.c$ci
        [[ ${cr.row} == '' ]] && break
        hdr+="$sep${cr.hdr}"
        row+="$sep${cr.row}"
        url+="$sep${cr.url}"
        sep='|'
    done
    if [[ ${querydata.vt_alarmtab.ppedgedata.c0.row} != '' ]] then
        hdr= row= url= sep=
        for (( ci = 0; ; ci++ )) do
            typeset -n cr=querydata.vt_alarmtab.ppedgedata.c$ci
            [[ ${cr.row} == '' ]] && break
            hdr+="$sep${cr.hdr}"
            row+="$sep${cr.row}"
            url+="$sep${cr.url}"
            sep='|'
        done
    fi
    atr.ppedgeattr_hdr=$hdr
    atr.ppedgeattr_row=$row
    for k in "${!qdatr.ppedgeattr.@}"; do
        typeset -n srcr=$k dstr=atr.ppedgeattr_${k##*.}
        dstr=$srcr
    done
    if [[ ${qdatr.ppedgeurl} != '' ]] then
        atr.ppedgeurl=${qdatr.ppedgeurl}
    elif [[ $url != '' && ${qdatr.ppedgenextqid} != '' ]] then
        atr.ppedgeurl=$url
    fi

    atr.psedgeattr_label="_SKIP_"
    atr.psedgeattr_fontname=${prefpp.fontfamily}
    atr.psedgeattr_fontsize=${ fssmaller ${prefpp.fontsize}; }
    hdr= row= url= sep=
    for (( ci = 0; ; ci++ )) do
        typeset -n cr=psedgedata.c$ci
        [[ ${cr.row} == '' ]] && break
        hdr+="$sep${cr.hdr}"
        row+="$sep${cr.row}"
        url+="$sep${cr.url}"
        sep='|'
    done
    if [[ ${querydata.vt_alarmtab.psedgedata.c0.row} != '' ]] then
        hdr= row= url= sep=
        for (( ci = 0; ; ci++ )) do
            typeset -n cr=querydata.vt_alarmtab.psedgedata.c$ci
            [[ ${cr.row} == '' ]] && break
            hdr+="$sep${cr.hdr}"
            row+="$sep${cr.row}"
            url+="$sep${cr.url}"
            sep='|'
        done
    fi
    atr.psedgeattr_hdr=$hdr
    atr.psedgeattr_row=$row
    for k in "${!qdatr.psedgeattr.@}"; do
        typeset -n srcr=$k dstr=atr.psedgeattr_${k##*.}
        dstr=$srcr
    done
    if [[ ${qdatr.psedgeurl} != '' ]] then
        atr.psedgeurl=${qdatr.psedgeurl}
    elif [[ $url != '' && ${qdatr.psedgenextqid} != '' ]] then
        atr.psedgeurl=$url
    fi

    atr.spedgeattr_label="_SKIP_"
    atr.spedgeattr_fontname=${prefpp.fontfamily}
    atr.spedgeattr_fontsize=${ fssmaller ${prefpp.fontsize}; }
    hdr= row= url= sep=
    for (( ci = 0; ; ci++ )) do
        typeset -n cr=spedgedata.c$ci
        [[ ${cr.row} == '' ]] && break
        hdr+="$sep${cr.hdr}"
        row+="$sep${cr.row}"
        url+="$sep${cr.url}"
        sep='|'
    done
    if [[ ${querydata.vt_alarmtab.spedgedata.c0.row} != '' ]] then
        hdr= row= url= sep=
        for (( ci = 0; ; ci++ )) do
            typeset -n cr=querydata.vt_alarmtab.spedgedata.c$ci
            [[ ${cr.row} == '' ]] && break
            hdr+="$sep${cr.hdr}"
            row+="$sep${cr.row}"
            url+="$sep${cr.url}"
            sep='|'
        done
    fi
    atr.spedgeattr_hdr=$hdr
    atr.spedgeattr_row=$row
    for k in "${!qdatr.spedgeattr.@}"; do
        typeset -n srcr=$k dstr=atr.spedgeattr_${k##*.}
        dstr=$srcr
    done
    if [[ ${qdatr.spedgeurl} != '' ]] then
        atr.spedgeurl=${qdatr.spedgeurl}
    elif [[ $url != '' && ${qdatr.spedgenextqid} != '' ]] then
        atr.spedgeurl=$url
    fi

    atr.ssedgeattr_label="_SKIP_"
    atr.ssedgeattr_fontname=${prefpp.fontfamily}
    atr.ssedgeattr_fontsize=${ fssmaller ${prefpp.fontsize}; }
    hdr= row= url= sep=
    for (( ci = 0; ; ci++ )) do
        typeset -n cr=ssedgedata.c$ci
        [[ ${cr.row} == '' ]] && break
        hdr+="$sep${cr.hdr}"
        row+="$sep${cr.row}"
        url+="$sep${cr.url}"
        sep='|'
    done
    if [[ ${querydata.vt_alarmtab.ssedgedata.c0.row} != '' ]] then
        hdr= row= url= sep=
        for (( ci = 0; ; ci++ )) do
            typeset -n cr=querydata.vt_alarmtab.ssedgedata.c$ci
            [[ ${cr.row} == '' ]] && break
            hdr+="$sep${cr.hdr}"
            row+="$sep${cr.row}"
            url+="$sep${cr.url}"
            sep='|'
        done
    fi
    atr.ssedgeattr_hdr=$hdr
    atr.ssedgeattr_row=$row
    for k in "${!qdatr.ssedgeattr.@}"; do
        typeset -n srcr=$k dstr=atr.ssedgeattr_${k##*.}
        dstr=$srcr
    done
    if [[ ${qdatr.ssedgeurl} != '' ]] then
        atr.ssedgeurl=${qdatr.ssedgeurl}
    elif [[ $url != '' && ${qdatr.ssedgenextqid} != '' ]] then
        atr.ssedgeurl=$url
    fi

    return 0
}

if [[ $SWIFTWARNLEVEL != "" ]] then
    PS4='${.sh.fun}-$LINENO: '
    typeset -ft dq_vt_alarmtab_init dq_vt_alarmtab_run dq_vt_alarmtab_setup
    typeset -ft dq_vt_alarmtab_emitbody dq_vt_alarmtab_emitheader
    typeset -ft dq_vt_alarmtab_emitpage
fi
