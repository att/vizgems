
dq_vt_invgrid_data=(
    deps='' rerun=''
    attrs=() embeds=''
)

function dq_vt_invgrid_init {
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
    for n in "${!vars.invgrid_@}"; do
        typeset -n vr=$n
        k=${n##*invgrid_}
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
            typeset -n attr=dq_vt_invgrid_data.attrs.$n1
            if [[ $attr != '' ]] then
                attr+=" $n2='${v//\'/\"}'"
            else
                attr="$n2='${v//\'/\"}'"
            fi
        else
            typeset -n attr=dq_vt_invgrid_data.attrs.$k
            attr=$v
        fi
        if [[ $v == *EMBED:*qimg* ]] then
            ev=$v
            while [[ $ev == *'<qimg>'* ]] do
                id=${ev%%'</qimg>'*}
                ev=${ev#*'</qimg>'}
                id=${id%%'</path>'}
                id=${id##*'<path>'}
                typeset -n embeds=dq_vt_invgrid_data.embeds
                embeds+=" $id"
                dq_vt_invgrid_data.deps+=" $id"
            done
        fi
    done
    dq_vt_invgrid_data.rendermode=${prefs[rendermode]:-final}
    dq_vt_invgrid_data.divmode=${prefs[divmode]:-max}
    dq_vt_invgrid_data.attrs.pagemode=${prefs[pagemode]:-all}
    dq_vt_invgrid_data.attrs.runid=invgrid

    return 0
}

function dq_vt_invgrid_run {
    typeset -n vt=$1
    typeset attri embed

    export INVGRIDATTRFILE=invgrid.attr INVGRIDEMBEDLISTFILE=invgrid.embedlist
    rm -f invgrid.*

    (
        print "maxpixel=${dq_main_data.maxpixel}"
        for attri in "${!dq_vt_invgrid_data.attrs.@}"; do
            typeset -n attr=$attri
            print -r "${attri##*.}=$attr"
        done
    ) > $INVGRIDATTRFILE

    (
        for embed in ${dq_vt_invgrid_data.embeds}; do
            print "$embed|$embed.embed"
        done
    ) | sort -u > $INVGRIDEMBEDLISTFILE

    ddsfilter -osm none -fso vg_dq_vt_invgrid.filter.so inv.dds \
    > invgrid.list
    return $?
}

function dq_vt_invgrid_emitheader {
    typeset file

    while read file; do
        [[ $file != *.css ]] && continue
        cat $file
    done < invgrid.list

    return 0
}

function dq_vt_invgrid_emitbody {
    typeset nodataflag=y id file pagen page pagei paget s
    typeset -A ids

    [[ ${dq_vt_invgrid_data.rendermode} == embed ]] && return 0

    while read file; do
        [[ $file != *.toc ]] && continue
        cat $file
    done < invgrid.list > invgrid.toc
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
    done < invgrid.list > invgrid.html
    [[ nodataflag == n ]] && print "<!--page 123456789-->" >> invgrid.html

    dq_main_emit_sdiv_begin \
        invgrid $SCREENWIDTH 200 "${dq_vt_invgrid_data.divmode}" \
    "Inv Grid View"
    if [[ ${dq_vt_invgrid_data.attrs.pagemode} == all ]] then
        pagen=
    else
        pagen=$(tail -1 invgrid.toc); pagen=${pagen%%'|'*}
        [[ $pagen == 1 ]] && pagen=
    fi
    if [[ $pagen == '' ]] then
        cat invgrid.html
    else
        typeset -n pi=dq_vt_invgrid_data.attrs.pageindex
        pi=${pi:-1}
        dq_main_emit_sdivpage_begin invgrid invgrid.html $pagen $pi
        while read page; do
            pagei=${page%%'|'*}
            paget=${page##*'|'}
            paget="$pagei/$pagen - ${paget:0:20}..."
            dq_main_emit_sdivpage_option "$pagei" "$paget"
        done < invgrid.toc
        dq_main_emit_sdivpage_middle invgrid invgrid.html $pagen $pi
        dq_vt_invgrid_emitpage invgrid.html $pi
        dq_main_emit_sdivpage_end invgrid invgrid.html $pagen $pi
    fi
    [[ $HIDEINFO == '' && -f invgrid.info ]] && cat invgrid.info
    if [[ $HIDEINFO == '' ]] then
        for id in ${dq_vt_invgrid_data.embeds}; do
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

function dq_vt_invgrid_emitpage { # $1 = file $2 = page
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

function dq_vt_invgrid_setup {
    dq_main_data.query.vtools.invgrid=(args=())
    typeset -n igr=dq_main_data.query.vtools.invgrid.args
    typeset -n qdigr=querydata.vt_invgrid
    typeset -n lvs=dq_main_data.levels
    typeset pnlv=${querydata.dt_inv.poutlevel}
    typeset snlv=${querydata.dt_inv.soutlevel}
    typeset glv pclv sclv k
    typeset nurlfmt=${dq_main_data.nodeurlfmt}

    igr.rendermode=${qdigr.rendermode:-final}
    igr.pagemode=${prefqp.invgridpage:-all}

    igr.graphlevel=${qdigr.graphlevel}
    igr.pclusterlevel=${qdigr.pclusterlevel}
    igr.sclusterlevel=${qdigr.sclusterlevel}
    glv=${igr.graphlevel}
    pclv=${igr.pclusterlevel}
    sclv=${igr.sclusterlevel}

    igr.invgridattr_bgcolor=${prefpp.bgcolor}
    for k in "${!qdigr.invgridattr.@}"; do
        typeset -n srcr=$k dstr=igr.invgridattr_${k##*.}
        dstr=$srcr
    done

    igr.titleattr_color=${prefpp.fgcolor}
    igr.titleattr_fontname=${prefpp.fontfamily}
    igr.titleattr_fontsize=$(fs2gd ${prefpp.fontsize})
    igr.titleattr_label=""
    igr.titleattr_info=""
    if [[ $glv != '' ]] then
        igr.titleattr_label="${lvs[$glv].fn}: %($glv.nodename)%"
        igr.titleattr_info="${lvs[$glv].fn}: %($glv.nodename)%"
    fi
    for k in "${!qdigr.titleattr.@}"; do
        typeset -n srcr=$k dstr=igr.titleattr_${k##*.}
        dstr=$srcr
    done
    igr.titleurl=""
    if [[ $glv != '' && ${qdigr.titlenextqid} != '' ]] then
        igr.titleurl=${nurlfmt//___NEXTQID___/${qdigr.titlenextqid}}
    fi

    igr.pclusterattr_color=${prefpp.fgcolor}
    igr.pclusterattr_fontname=${prefpp.fontfamily}
    igr.pclusterattr_fontsize=$(fs2gd ${prefpp.fontsize})
    igr.pclusterattr_linewidth=1
    igr.pclusterattr_label=""
    if [[ $pclv != '' ]] then
        igr.pclusterattr_label="${lvs[$pclv].fn}: %($pclv.nodename)%"
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
    igr.sclusterattr_fontname=${prefpp.fontfamily}
    igr.sclusterattr_fontsize=$(fs2gd ${prefpp.fontsize})
    igr.sclusterattr_linewidth=1
    igr.sclusterattr_label=""
    if [[ $sclv != '' ]] then
        igr.sclusterattr_label="${lvs[$sclv].fn}: %($sclv.nodename)%"
    fi
    for k in "${!qdigr.sclusterattr.@}"; do
        typeset -n srcr=$k dstr=igr.sclusterattr_${k##*.}
        dstr=$srcr
    done
    igr.sclusterurl=""
    if [[ $sclv != '' && ${qdigr.sclusternextqid} != '' ]] then
        igr.sclusterurl=${nurlfmt//___NEXTQID___/${qdigr.sclusternextqid}}
    fi

    igr.pnodeattr_color=${prefpp.fgcolor}
    igr.pnodeattr_fillcolor=${prefpp.bgcolor}
    igr.pnodeattr_width=125
    igr.pnodeattr_height=15
    igr.pnodeattr_linewidth=1
    igr.pnodeattr_fontname=${prefpp.fontfamily}
    igr.pnodeattr_fontsize=$(fs2gd ${prefpp.fontsize})
    igr.pnodeattr_label="${lvs[$pnlv].fn}: %(nodename)%"
    igr.pnodeattr_info="${lvs[$pnlv].fn}: %(name)%"
    for k in "${!qdigr.pnodeattr.@}"; do
        typeset -n srcr=$k dstr=igr.pnodeattr_${k##*.}
        dstr=$srcr
    done
    if [[ ${qdigr.pnodenextqid} != '' ]] then
        igr.pnodeurl=${nurlfmt//___NEXTQID___/${qdigr.pnodenextqid}}
    fi

    if [[ $snlv != '' ]] then
        igr.snodeattr_color=${prefpp.fgcolor}
        igr.snodeattr_fillcolor=${prefpp.bgcolor}
        igr.snodeattr_width=125
        igr.snodeattr_height=15
        igr.snodeattr_linewidth=1
        igr.snodeattr_fontname=${prefpp.fontfamily}
        igr.snodeattr_fontsize=$(fs2gd ${prefpp.fontsize})
        igr.snodeattr_label="${lvs[$snlv].fn}: %(nodename)%"
        igr.snodeattr_info="${lvs[$snlv].fn}: %(name)%"
        for k in "${!qdigr.snodeattr.@}"; do
            typeset -n srcr=$k dstr=igr.snodeattr_${k##*.}
            dstr=$srcr
        done
        if [[ ${qdigr.snodenextqid} != '' ]] then
            igr.snodeurl=${nurlfmt//___NEXTQID___/${qdigr.snodenextqid}}
        fi
    fi

    return 0
}

if [[ $SWIFTWARNLEVEL != "" ]] then
    PS4='${.sh.fun}-$LINENO: '
    typeset -ft dq_vt_invgrid_init dq_vt_invgrid_run dq_vt_invgrid_setup
    typeset -ft dq_vt_invgrid_emitbody dq_vt_invgrid_emitheader
    typeset -ft dq_vt_invgrid_emitpage
fi
