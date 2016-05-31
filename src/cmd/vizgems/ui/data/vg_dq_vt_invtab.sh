
dq_vt_invtab_data=(
    deps='' rerun=''
    attrs=() embeds=''
)

function dq_vt_invtab_init {
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
    for n in "${!vars.invtab_@}"; do
        typeset -n vr=$n
        k=${n##*invtab_}
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
            typeset -n attr=dq_vt_invtab_data.attrs.$n1
            if [[ $attr != '' ]] then
                attr+=" $n2='${v//\'/\"}'"
            else
                attr="$n2='${v//\'/\"}'"
            fi
        else
            typeset -n attr=dq_vt_invtab_data.attrs.$k
            attr=$v
        fi
        if [[ $v == *EMBED:*qimg* ]] then
            ev=$v
            while [[ $ev == *'<qimg>'* ]] do
                id=${ev%%'</qimg>'*}
                ev=${ev#*'</qimg>'}
                id=${id%%'</path>'}
                id=${id##*'<path>'}
                typeset -n embeds=dq_vt_invtab_data.embeds
                embeds+=" $id"
                dq_vt_invtab_data.deps+=" $id"
            done
        fi
    done
    dq_vt_invtab_data.rendermode=${prefs[rendermode]:-final}
    dq_vt_invtab_data.divmode=${prefs[divmode]:-max}
    dq_vt_invtab_data.attrs.pagemode=${prefs[pagemode]:-all}
    dq_vt_invtab_data.attrs.runid=invtab

    return 0
}

function dq_vt_invtab_run {
    typeset -n vt=$1
    typeset attri embed u soflag=n

    export INVTABATTRFILE=invtab.attr INVTABEMBEDLISTFILE=invtab.embedlist
    rm -f invtab.*

    (
        u=${dq_main_data.fullurl}
        u=${u//invtab_sortorder=*([!&])/}
        u=${u//'&&'/'&'}
        print -r "sorturl=$u"
        for attri in "${!dq_vt_invtab_data.attrs.@}"; do
            typeset -n attr=$attri
            print -r "${attri##*.}=$attr"
            [[ ${attri##*.} == sortorder ]] && soflag=y
        done
        [[ $soflag != y ]] && print -r "sortorder=0"
    ) > $INVTABATTRFILE

    (
        for embed in ${dq_vt_invtab_data.embeds}; do
            print "$embed|$embed.embed"
        done
    ) | sort -u > $INVTABEMBEDLISTFILE

    ddsfilter -osm none -fso vg_dq_vt_invtab.filter.so inv.dds \
    > invtab.list
    return $?
}

function dq_vt_invtab_emitheader {
    typeset file

    while read file; do
        [[ $file != *.css ]] && continue
        cat $file
    done < invtab.list

    return 0
}

function dq_vt_invtab_emitbody {
    typeset nodataflag=y file pagen page pagei paget findex

    [[ ${dq_vt_invtab_data.rendermode} == embed ]] && return 0

    dq_main_emit_sdiv_begin \
        invtab $SCREENWIDTH 200 "${dq_vt_invtab_data.divmode}" \
    "Inv Tabular View"
    while read file; do
        [[ $file != *.html ]] && continue
        if [[ ${dq_vt_invtab_data.attrs.pagemode} == all ]] then
            pagen=
        else
            pagen=$(tail -1 ${file%.html}.toc); pagen=${pagen%%'|'*}
            [[ $pagen == 1 ]] && pagen=
        fi
        if [[ $pagen == '' ]] then
            cat $file
        else
            findex=${file//*.+([0-9]).*/\1}
            typeset -n pi=dq_vt_invtab_data.attrs.pageindex$findex
            pi=${pi:-1}
            dq_main_emit_sdivpage_begin invtab $file $pagen $pi
            while read page; do
                pagei=${page%%'|'*}
                paget=${page##*'|'}
                paget="$pagei/$pagen - $paget..."
                dq_main_emit_sdivpage_option "$pagei" "$paget"
            done < ${file%.html}.toc
            dq_main_emit_sdivpage_middle invtab $file $pagen $pi
            dq_vt_invtab_emitpage $file $pi
            dq_main_emit_sdivpage_end invtab $file $pagen $pi
        fi
        print "<br>"
        nodataflag=n
    done < invtab.list
    [[ $HIDEINFO == '' && -f invtab.info ]] && cat invtab.info
    if [[ $nodataflag == y ]] then
        print "The query returned no inventory records."
    fi
    dq_main_emit_sdiv_end

    return 0
}

function dq_vt_invtab_emitpage { # $1 = file $2 = page
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

function dq_vt_invtab_setup {
    dq_main_data.query.vtools.invtab=(args=())
    typeset -n itr=dq_main_data.query.vtools.invtab.args
    typeset -n qditr=querydata.vt_invtab
    typeset k ci hdr row url sep
    typeset nurlfmt=${dq_main_data.nodeurlfmt}
    typeset eurlfmt=${dq_main_data.edgeurlfmt}
    typeset pnodedata=(
        c0=(
            hdr='C:Asset'
            row='RS:%(name/_id_)%'
            url=${nurlfmt//___NEXTQID___/${qdatr.pnodenextqid}}
        )
    )
    typeset snodedata=()
    typeset ppedgedata=() psedgedata=() spedgedata=() ssedgedata=()

    itr.rendermode=${qditr.rendermode:-final}
    itr.pagemode=${prefqp.invtabpage:-all}

    itr.sortorder=${qditr.sortorder:-0}

    itr.invtabattr_bgcolor=${prefpp.bgcolor}
    itr.invtabattr_color=${prefpp.fgcolor}
    itr.invtabattr_fontname=${prefpp.fontfamily}
    itr.invtabattr_fontsize=${prefpp.fontsize}
    for k in "${!qditr.invtabattr.@}"; do
        typeset -n srcr=$k dstr=itr.invtabattr_${k##*.}
        dstr=$srcr
    done

    itr.pnodeattr_bgcolor=${prefpp.bgcolor}
    itr.pnodeattr_color=${prefpp.fgcolor}
    itr.pnodeattr_fontname=${prefpp.fontfamily}
    itr.pnodeattr_fontsize=${ fssmaller ${prefpp.fontsize}; }
    itr.pnodeattr_label="%(name)%"
    hdr= row= url= sep=
    for (( ci = 0; ; ci++ )) do
        typeset -n cr=pnodedata.c$ci
        [[ ${cr.row} == '' ]] && break
        hdr+="$sep${cr.hdr}"
        row+="$sep${cr.row}"
        url+="$sep${cr.url}"
        sep='|'
    done
    if [[ ${querydata.vt_invtab.pnodedata.c0.row} != '' ]] then
        hdr= row= url= sep=
        for (( ci = 0; ; ci++ )) do
            typeset -n cr=querydata.vt_invtab.pnodedata.c$ci
            [[ ${cr.row} == '' ]] && break
            hdr+="$sep${cr.hdr}"
            row+="$sep${cr.row}"
            url+="$sep${cr.url}"
            sep='|'
        done
    fi
    itr.pnodeattr_hdr=$hdr
    itr.pnodeattr_row=$row
    for k in "${!qditr.pnodeattr.@}"; do
        typeset -n srcr=$k dstr=itr.pnodeattr_${k##*.}
        dstr=$srcr
    done
    if [[ $url != '' && ${qditr.pnodenextqid} != '' ]] then
        itr.pnodeurl=$url
    fi

    itr.snodeattr_bgcolor=${prefpp.bgcolor}
    itr.snodeattr_color=${prefpp.fgcolor}
    itr.snodeattr_fontname=${prefpp.fontfamily}
    itr.snodeattr_fontsize=${ fssmaller ${prefpp.fontsize}; }
    itr.snodeattr_label="_SKIP_"
    hdr= row= url= sep=
    for (( ci = 0; ; ci++ )) do
        typeset -n cr=snodedata.c$ci
        [[ ${cr.row} == '' ]] && break
        hdr+="$sep${cr.hdr}"
        row+="$sep${cr.row}"
        url+="$sep${cr.url}"
        sep='|'
    done
    if [[ ${querydata.vt_invtab.snodedata.c0.row} != '' ]] then
        hdr= row= url= sep=
        for (( ci = 0; ; ci++ )) do
            typeset -n cr=querydata.vt_invtab.snodedata.c$ci
            [[ ${cr.row} == '' ]] && break
            hdr+="$sep${cr.hdr}"
            row+="$sep${cr.row}"
            url+="$sep${cr.url}"
            sep='|'
        done
    fi
    itr.snodeattr_hdr=$hdr
    itr.snodeattr_row=$row
    for k in "${!qditr.snodeattr.@}"; do
        typeset -n srcr=$k dstr=itr.snodeattr_${k##*.}
        dstr=$srcr
    done
    if [[ $url != '' && ${qditr.snodenextqid} != '' ]] then
        itr.snodeurl=$url
    fi

    itr.ppedgeattr_bgcolor=${prefpp.bgcolor}
    itr.ppedgeattr_color=${prefpp.fgcolor}
    itr.ppedgeattr_fontname=${prefpp.fontfamily}
    itr.ppedgeattr_fontsize=${ fssmaller ${prefpp.fontsize}; }
    itr.ppedgeattr_label="_SKIP_"
    hdr= row= url= sep=
    for (( ci = 0; ; ci++ )) do
        typeset -n cr=ppedgedata.c$ci
        [[ ${cr.row} == '' ]] && break
        hdr+="$sep${cr.hdr}"
        row+="$sep${cr.row}"
        url+="$sep${cr.url}"
        sep='|'
    done
    if [[ ${querydata.vt_invtab.ppedgedata.c0.row} != '' ]] then
        hdr= row= url= sep=
        for (( ci = 0; ; ci++ )) do
            typeset -n cr=querydata.vt_invtab.ppedgedata.c$ci
            [[ ${cr.row} == '' ]] && break
            hdr+="$sep${cr.hdr}"
            row+="$sep${cr.row}"
            url+="$sep${cr.url}"
            sep='|'
        done
    fi
    itr.ppedgeattr_hdr=$hdr
    itr.ppedgeattr_row=$row
    for k in "${!qditr.ppedgeattr.@}"; do
        typeset -n srcr=$k dstr=itr.ppedgeattr_${k##*.}
        dstr=$srcr
    done
    if [[ $url != '' && ${qditr.ppedgenextqid} != '' ]] then
        itr.ppedgeurl=$url
    fi

    itr.psedgeattr_bgcolor=${prefpp.bgcolor}
    itr.psedgeattr_color=${prefpp.fgcolor}
    itr.psedgeattr_fontname=${prefpp.fontfamily}
    itr.psedgeattr_fontsize=${ fssmaller ${prefpp.fontsize}; }
    itr.psedgeattr_label="_SKIP_"
    hdr= row= url= sep=
    for (( ci = 0; ; ci++ )) do
        typeset -n cr=psedgedata.c$ci
        [[ ${cr.row} == '' ]] && break
        hdr+="$sep${cr.hdr}"
        row+="$sep${cr.row}"
        url+="$sep${cr.url}"
        sep='|'
    done
    if [[ ${querydata.vt_invtab.psedgedata.c0.row} != '' ]] then
        hdr= row= url= sep=
        for (( ci = 0; ; ci++ )) do
            typeset -n cr=querydata.vt_invtab.psedgedata.c$ci
            [[ ${cr.row} == '' ]] && break
            hdr+="$sep${cr.hdr}"
            row+="$sep${cr.row}"
            url+="$sep${cr.url}"
            sep='|'
        done
    fi
    itr.psedgeattr_hdr=$hdr
    itr.psedgeattr_row=$row
    for k in "${!qditr.psedgeattr.@}"; do
        typeset -n srcr=$k dstr=itr.psedgeattr_${k##*.}
        dstr=$srcr
    done
    if [[ $url != '' && ${qditr.psedgenextqid} != '' ]] then
        itr.psedgeurl=$url
    fi

    itr.spedgeattr_bgcolor=${prefpp.bgcolor}
    itr.spedgeattr_color=${prefpp.fgcolor}
    itr.spedgeattr_fontname=${prefpp.fontfamily}
    itr.spedgeattr_fontsize=${ fssmaller ${prefpp.fontsize}; }
    itr.spedgeattr_label="_SKIP_"
    hdr= row= url= sep=
    for (( ci = 0; ; ci++ )) do
        typeset -n cr=spedgedata.c$ci
        [[ ${cr.row} == '' ]] && break
        hdr+="$sep${cr.hdr}"
        row+="$sep${cr.row}"
        url+="$sep${cr.url}"
        sep='|'
    done
    if [[ ${querydata.vt_invtab.spedgedata.c0.row} != '' ]] then
        hdr= row= url= sep=
        for (( ci = 0; ; ci++ )) do
            typeset -n cr=querydata.vt_invtab.spedgedata.c$ci
            [[ ${cr.row} == '' ]] && break
            hdr+="$sep${cr.hdr}"
            row+="$sep${cr.row}"
            url+="$sep${cr.url}"
            sep='|'
        done
    fi
    itr.spedgeattr_hdr=$hdr
    itr.spedgeattr_row=$row
    for k in "${!qditr.spedgeattr.@}"; do
        typeset -n srcr=$k dstr=itr.spedgeattr_${k##*.}
        dstr=$srcr
    done
    if [[ $url != '' && ${qditr.spedgenextqid} != '' ]] then
        itr.spedgeurl=$url
    fi

    itr.ssedgeattr_bgcolor=${prefpp.bgcolor}
    itr.ssedgeattr_color=${prefpp.fgcolor}
    itr.ssedgeattr_fontname=${prefpp.fontfamily}
    itr.ssedgeattr_fontsize=${ fssmaller ${prefpp.fontsize}; }
    itr.ssedgeattr_label="_SKIP_"
    hdr= row= url= sep=
    for (( ci = 0; ; ci++ )) do
        typeset -n cr=ssedgedata.c$ci
        [[ ${cr.row} == '' ]] && break
        hdr+="$sep${cr.hdr}"
        row+="$sep${cr.row}"
        url+="$sep${cr.url}"
        sep='|'
    done
    if [[ ${querydata.vt_invtab.ssedgedata.c0.row} != '' ]] then
        hdr= row= url= sep=
        for (( ci = 0; ; ci++ )) do
            typeset -n cr=querydata.vt_invtab.ssedgedata.c$ci
            [[ ${cr.row} == '' ]] && break
            hdr+="$sep${cr.hdr}"
            row+="$sep${cr.row}"
            url+="$sep${cr.url}"
            sep='|'
        done
    fi
    itr.ssedgeattr_hdr=$hdr
    itr.ssedgeattr_row=$row
    for k in "${!qditr.ssedgeattr.@}"; do
        typeset -n srcr=$k dstr=itr.ssedgeattr_${k##*.}
        dstr=$srcr
    done
    if [[ $url != '' && ${qditr.ssedgenextqid} != '' ]] then
        itr.ssedgeurl=$url
    fi

    return 0
}

if [[ $SWIFTWARNLEVEL != "" ]] then
    PS4='${.sh.fun}-$LINENO: '
    typeset -ft dq_vt_invtab_init dq_vt_invtab_run dq_vt_invtab_setup
    typeset -ft dq_vt_invtab_emitbody dq_vt_invtab_emitheader
    typeset -ft dq_vt_invtab_emitpage
fi
