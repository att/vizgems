
dq_vt_invgraph_data=(
    deps='' rerun=''
    attrs=() embeds=''
)

function dq_vt_invgraph_init {
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
    for n in "${!vars.invgraph_@}"; do
        typeset -n vr=$n
        k=${n##*invgraph_}
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
            typeset -n attr=dq_vt_invgraph_data.attrs.$n1
            if [[ $attr != '' ]] then
                attr+=" $n2='${v//\'/\"}'"
            else
                attr="$n2='${v//\'/\"}'"
            fi
        else
            typeset -n attr=dq_vt_invgraph_data.attrs.$k
            attr=$v
        fi
        if [[ $v == *EMBED:*qimg* ]] then
            ev=$v
            while [[ $ev == *'<qimg>'* ]] do
                id=${ev%%'</qimg>'*}
                ev=${ev#*'</qimg>'}
                id=${id%%'</path>'}
                id=${id##*'<path>'}
                typeset -n embeds=dq_vt_invgraph_data.embeds
                embeds+=" $id"
                dq_vt_invgraph_data.deps+=" $id"
            done
        fi
    done
    dq_vt_invgraph_data.rendermode=${prefs[rendermode]:-final}
    dq_vt_invgraph_data.divmode=${prefs[divmode]:-max}
    dq_vt_invgraph_data.attrs.pagemode=${prefs[pagemode]:-all}
    dq_vt_invgraph_data.attrs.runid=invgraph

    return 0
}

function dq_vt_invgraph_run {
    typeset -n vt=$1
    typeset attri embed

    export INVGRAPHATTRFILE=invgraph.attr
    export INVGRAPHEMBEDLISTFILE=invgraph.embedlist
    rm -f invgraph.*

    (
        print "maxpixel=${dq_main_data.maxpixel}"
        for attri in "${!dq_vt_invgraph_data.attrs.@}"; do
            typeset -n attr=$attri
            print -r "${attri##*.}=$attr"
        done
    ) > $INVGRAPHATTRFILE

    (
        for embed in ${dq_vt_invgraph_data.embeds}; do
            print "$embed|$embed.embed"
        done
    ) | sort -u > $INVGRAPHEMBEDLISTFILE

    ddsfilter -osm none -fso vg_dq_vt_invgraph.filter.so inv.dds \
    > invgraph.list
    return $?
}

function dq_vt_invgraph_emitheader {
    typeset file

    while read file; do
        [[ $file != *.css ]] && continue
        cat $file
    done < invgraph.list

    return 0
}

function dq_vt_invgraph_emitbody {
    typeset nodataflag=y id file pagen page pagei paget s
    typeset -A ids

    [[ ${dq_vt_invgraph_data.rendermode} == embed ]] && return 0

    while read file; do
        [[ $file != *.toc ]] && continue
        cat $file
    done < invgraph.list > invgraph.toc
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
    done < invgraph.list > invgraph.html
    [[ nodataflag == n ]] && print "<!--page 123456789-->" >> invgraph.html

    dq_main_emit_sdiv_begin \
        invgraph $SCREENWIDTH 200 "${dq_vt_invgraph_data.divmode}" \
    "Inv Diagram View"
    if [[ ${dq_vt_invgraph_data.attrs.pagemode} == all ]] then
        pagen=
    else
        pagen=$(tail -1 invgraph.toc); pagen=${pagen%%'|'*}
        [[ $pagen == 1 ]] && pagen=
    fi
    if [[ $pagen == '' ]] then
        cat invgraph.html
    else
        typeset -n pi=dq_vt_invgraph_data.attrs.pageindex
        pi=${pi:-1}
        dq_main_emit_sdivpage_begin invgraph invgraph.html $pagen $pi
        while read page; do
            pagei=${page%%'|'*}
            paget=${page##*'|'}
            paget="$pagei/$pagen - ${paget:0:20}..."
            dq_main_emit_sdivpage_option "$pagei" "$paget"
        done < invgraph.toc
        dq_main_emit_sdivpage_middle invgraph invgraph.html $pagen $pi
        dq_vt_invgraph_emitpage invgraph.html $pi
        dq_main_emit_sdivpage_end invgraph invgraph.html $pagen $pi
    fi
    [[ $HIDEINFO == '' && -f invgraph.info ]] && cat invgraph.info
    if [[ $HIDEINFO == '' ]] then
        for id in ${dq_vt_invgraph_data.embeds}; do
            [[ ${ids[$id]} == '' && -f $id.info ]] && cat $id.info
            ids[$id]=1
        done
    fi
    if [[ $nodataflag == y ]] then
        print "The query returned no inventory records."
    fi
    dq_main_emit_sdiv_end

    return 0
}

function dq_vt_invgraph_emitpage { # $1 = file $2 = page
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

function dq_vt_invgraph_setup {
    dq_main_data.query.vtools.invgraph=(args=())
    typeset -n igr=dq_main_data.query.vtools.invgraph.args
    typeset -n qdigr=querydata.vt_invgraph
    typeset -n lvs=dq_main_data.levels
    typeset pnlv=${querydata.dt_inv.poutlevel}
    typeset snlv=${querydata.dt_inv.soutlevel}
    typeset glv pclv sclv rlv k
    typeset nurlfmt=${dq_main_data.nodeurlfmt}
    typeset eurlfmt=${dq_main_data.edgeurlfmt}

    igr.rendermode=${qdigr.rendermode:-final}
    igr.pagemode=${prefqp.invgraphpage:-all}

    igr.layoutstyle=${qdigr.layoutstyle:-dot}
    igr.graphlevel=${qdigr.graphlevel}
    igr.pclusterlevel=${qdigr.pclusterlevel}
    igr.sclusterlevel=${qdigr.sclusterlevel}
    igr.ranklevel=${qdigr.ranklevel}
    glv=${igr.graphlevel}
    pclv=${igr.pclusterlevel}
    sclv=${igr.sclusterlevel}
    rlv=${igr.ranklevel}

    if [[ ${igr.layoutstyle} == dot ]] then
        igr.graphattr_rankdir=LR
    fi
    igr.graphattr_ranksep=$(p2p 50)
    igr.graphattr_nodesep=$(p2p 5)
    igr.graphattr_labelloc=t
    igr.graphattr_color=${prefpp.fgcolor}
    igr.graphattr_bgcolor=${prefpp.bgcolor}
    igr.graphattr_fillcolor=${prefpp.bgcolor}
    igr.graphattr_fontname=${prefpp.fontfamily}
    igr.graphattr_fontsize=$(fs2gd ${prefpp.fontsize})
    igr.graphattr_fontcolor=${prefpp.fgcolor}
    igr.graphattr_label=""
    igr.graphattr_info=""
    if [[ $glv != '' ]] then
        igr.graphattr_label="${lvs[$glv].fn}: %($glv.nodename)%"
        igr.graphattr_info="${lvs[$glv].fn}: %($glv.nodename)%"
    fi
    for k in "${!qdigr.graphattr.@}"; do
        typeset -n srcr=$k dstr=igr.graphattr_${k##*.}
        dstr=$srcr
    done
    igr.graphurl=""
    if [[ $glv != '' && ${qdigr.graphnextqid} != '' ]] then
        igr.graphurl=${nurlfmt//___NEXTQID___/${qdigr.graphnextqid}}
    fi

    igr.sgraphattr_color=${prefpp.fgcolor}
    igr.sgraphattr_bgcolor=${prefpp.bgcolor}
    igr.sgraphattr_fillcolor=${prefpp.bgcolor}
    igr.sgraphattr_fontname=${prefpp.fontfamily}
    igr.sgraphattr_fontsize=$(fs2gd ${prefpp.fontsize})
    igr.sgraphattr_fontcolor=${prefpp.fgcolor}
    igr.sgraphattr_label=""
    igr.sgraphattr_info=""
    if [[ $glv != '' ]] then
        igr.sgraphattr_label="${lvs[$glv].fn}: %($glv.nodename)%"
        igr.sgraphattr_info="${lvs[$glv].fn}: %($glv.nodename)%"
    fi
    for k in "${!qdigr.sgraphattr.@}"; do
        typeset -n srcr=$k dstr=igr.sgraphattr_${k##*.}
        dstr=$srcr
    done
    igr.sgraphurl=""
    if [[ $glv != '' && ${qdigr.sgraphnextqid} != '' ]] then
        igr.sgraphurl=${nurlfmt//___NEXTQID___/${qdigr.sgraphnextqid}}
    fi

    igr.pclusterattr_color=${prefpp.fgcolor}
    igr.pclusterattr_bgcolor=${prefpp.bgcolor}
    igr.pclusterattr_fillcolor=${prefpp.bgcolor}
    igr.pclusterattr_fontname=${prefpp.fontfamily}
    igr.pclusterattr_fontsize=$(fs2gd ${prefpp.fontsize})
    igr.pclusterattr_fontcolor=${prefpp.fgcolor}
    igr.pclusterattr_label=""
    igr.pclusterattr_info=""
    if [[ $pclv != '' ]] then
        igr.pclusterattr_label="${lvs[$pclv].fn}: %($pclv.nodename)%"
        igr.pclusterattr_info="${lvs[$pclv].fn}: %($pclv.nodename)%"
    fi
    for k in "${!qdigr.pclusterattr.@}"; do
        typeset -n srcr=$k dstr=igr.pclusterattr_${k##*.}
        dstr=$srcr
    done
    igr.pclusterurl=""
    if [[ $pclv != '' && ${qdigr.pclusternextqid} != '' ]] then
        igr.pclusterurl=${nurlfmt//___NEXTQID___/${qdigr.pclusternextqid}}
    fi

    igr.sclusterattr_color=${prefpp.fgcolor}
    igr.sclusterattr_bgcolor=${prefpp.bgcolor}
    igr.sclusterattr_fillcolor=${prefpp.bgcolor}
    igr.sclusterattr_fontname=${prefpp.fontfamily}
    igr.sclusterattr_fontsize=$(fs2gd ${prefpp.fontsize})
    igr.sclusterattr_fontcolor=${prefpp.fgcolor}
    igr.sclusterattr_label=""
    igr.sclusterattr_info=""
    if [[ $sclv != '' ]] then
        igr.sclusterattr_label="${lvs[$sclv].fn}: %($sclv.nodename)%"
        igr.sclusterattr_info="${lvs[$sclv].fn}: %($sclv.nodename)%"
    fi
    for k in "${!qdigr.sclusterattr.@}"; do
        typeset -n srcr=$k dstr=igr.sclusterattr_${k##*.}
        dstr=$srcr
    done
    igr.sclusterurl=""
    if [[ $sclv != '' && ${qdigr.sclusternextqid} != '' ]] then
        igr.sclusterurl=${nurlfmt//___NEXTQID___/${qdigr.sclusternextqid}}
    fi

    igr.rankattr_rank=""
    if [[ $rlv != '' ]] then
        igr.rankattr_rank="same"
    fi
    for k in "${!qdigr.rankattr.@}"; do
        typeset -n srcr=$k dstr=igr.rankattr_${k##*.}
        dstr=$srcr
    done
    igr.rankurl=""
    if [[ $rlv != '' && ${qdigr.ranknextqid} != '' ]] then
        igr.rankurl=${nurlfmt//___NEXTQID___/${qdigr.ranknextqid}}
    fi

    igr.pnodeattr_shape=box
    igr.pnodeattr_width=$(p2p 125)
    igr.pnodeattr_height=$(p2p 15)
    igr.pnodeattr_color=${prefpp.fgcolor}
    igr.pnodeattr_fillcolor=${prefpp.bgcolor}
    igr.pnodeattr_style=filled
    igr.pnodeattr_fontname=${prefpp.fontfamily}
    igr.pnodeattr_fontsize=$(fs2gd ${prefpp.fontsize})
    igr.pnodeattr_fontcolor=${prefpp.fgcolor}
    igr.pnodeattr_label="${lvs[$pnlv].fn}: %(nodename)%"
    igr.pnodeattr_info="${lvs[$pnlv].fn}: %(name)%"
    for k in "${!qdigr.pnodeattr.@}"; do
        typeset -n srcr=$k dstr=igr.pnodeattr_${k##*.}
        dstr=$srcr
    done
    if [[ ${qdigr.pnodenextqid} != '' ]] then
        igr.pnodeurl=${nurlfmt//___NEXTQID___/${qdigr.pnodenextqid}}
    fi

    igr.snodeattr_shape=box
    igr.snodeattr_width=$(p2p 125)
    igr.snodeattr_height=$(p2p 15)
    igr.snodeattr_color=${prefpp.fgcolor}
    igr.snodeattr_fillcolor=${prefpp.bgcolor}
    igr.snodeattr_style=filled
    igr.snodeattr_fontname=${prefpp.fontfamily}
    igr.snodeattr_fontsize=$(fs2gd ${prefpp.fontsize})
    igr.snodeattr_fontcolor=${prefpp.fgcolor}
    igr.snodeattr_label="${lvs[$snlv].fn}: %(nodename)%"
    igr.snodeattr_info="${lvs[$snlv].fn}: %(name)%"
    for k in "${!qdigr.snodeattr.@}"; do
        typeset -n srcr=$k dstr=igr.snodeattr_${k##*.}
        dstr=$srcr
    done
    if [[ ${qdigr.snodenextqid} != '' ]] then
        igr.snodeurl=${nurlfmt//___NEXTQID___/${qdigr.snodenextqid}}
    fi

    igr.ppedgeattr_dir=none
    igr.ppedgeattr_color=black
    for k in "${!qdigr.ppedgeattr.@}"; do
        typeset -n srcr=$k dstr=igr.ppedgeattr_${k##*.}
        dstr=$srcr
    done
    if [[ ${qdigr.ppedgenextqid} != '' ]] then
        igr.ppedgeurl=${eurlfmt//___NEXTQID___/${qdigr.ppedgenextqid}}
    fi

    igr.psedgeattr_dir=none
    igr.psedgeattr_color=black
    for k in "${!qdigr.psedgeattr.@}"; do
        typeset -n srcr=$k dstr=igr.psedgeattr_${k##*.}
        dstr=$srcr
    done
    if [[ ${qdigr.psedgenextqid} != '' ]] then
        igr.psedgeurl=${eurlfmt//___NEXTQID___/${qdigr.psedgenextqid}}
    fi

    igr.spedgeattr_dir=none
    igr.spedgeattr_color=black
    for k in "${!qdigr.spedgeattr.@}"; do
        typeset -n srcr=$k dstr=igr.spedgeattr_${k##*.}
        dstr=$srcr
    done
    if [[ ${qdigr.spedgenextqid} != '' ]] then
        igr.spedgeurl=${eurlfmt//___NEXTQID___/${qdigr.spedgenextqid}}
    fi

    igr.ssedgeattr_dir=none
    igr.ssedgeattr_color=black
    for k in "${!qdigr.ssedgeattr.@}"; do
        typeset -n srcr=$k dstr=igr.ssedgeattr_${k##*.}
        dstr=$srcr
    done
    if [[ ${qdigr.ssedgenextqid} != '' ]] then
        igr.ssedgeurl=${eurlfmt//___NEXTQID___/${qdigr.ssedgenextqid}}
    fi

    return 0
}

if [[ $SWIFTWARNLEVEL != "" ]] then
    PS4='${.sh.fun}-$LINENO: '
    typeset -ft dq_vt_invgraph_init dq_vt_invgraph_run dq_vt_invgraph_setup
    typeset -ft dq_vt_invgraph_emitbody dq_vt_invgraph_emitheader
    typeset -ft dq_vt_invgraph_emitpage
fi
