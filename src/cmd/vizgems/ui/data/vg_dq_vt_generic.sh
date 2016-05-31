
dq_vt_generic_data=(
    deps='' rerun=''
    attrs=() embeds=''
    funcs=( init='' run='' emitheader='' emitbody='' emitpage='' setup='' )
)

function dq_vt_generic_init {
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
    for n in "${!vars.generic_@}"; do
        typeset -n vr=$n
        k=${n##*generic_}
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
            typeset -n attr=dq_vt_generic_data.attrs.$n1
            if [[ $attr != '' ]] then
                attr+=" $n2='${v//\'/\"}'"
            else
                attr="$n2='${v//\'/\"}'"
            fi
        else
            typeset -n attr=dq_vt_generic_data.attrs.$k
            attr=$v
        fi
        if [[ $v == *EMBED:*qimg* ]] then
            ev=$v
            while [[ $ev == *'<qimg>'* ]] do
                id=${ev%%'</qimg>'*}
                ev=${ev#*'</qimg>'}
                id=${id%%'</path>'}
                id=${id##*'<path>'}
                typeset -n embeds=dq_vt_generic_data.embeds
                embeds+=" $id"
                dq_vt_generic_data.deps+=" $id"
            done
        fi
    done
    dq_vt_generic_data.rendermode=${prefs[rendermode]:-final}
    dq_vt_generic_data.divmode=${prefs[divmode]:-max}
    dq_vt_generic_data.attrs.pagemode=${prefs[pagemode]:-all}
    dq_vt_generic_data.attrs.runid=generic

    dq_vt_generic_data.funcs.init=${qprefs.funcs.init}
    dq_vt_generic_data.funcs.run=${qprefs.funcs.run}
    dq_vt_generic_data.funcs.emitheader=${qprefs.funcs.emitheader}
    dq_vt_generic_data.funcs.emitbody=${qprefs.funcs.emitbody}
    dq_vt_generic_data.funcs.emitpage=${qprefs.funcs.emitpage}
    dq_vt_generic_data.funcs.setup=${qprefs.funcs.setup}

    [[ ${dq_vt_generic_data.funcs.init} != '' ]] && \
    { ${dq_vt_generic_data.funcs.init} "$@" || return 1; }

    return 0
}

function dq_vt_generic_run {
    typeset -n vt=$1

    rm -f generic.*

    [[ ${dq_vt_generic_data.funcs.run} != '' ]] && \
    { ${dq_vt_generic_data.funcs.run} "$@" || return 1; }

    [[ ! -f generic.list ]] && touch generic.list

    return 0
}

function dq_vt_generic_emitheader {
    typeset file

    while read file; do
        [[ $file != *.css ]] && continue
        cat $file
    done < generic.list

    [[ ${dq_vt_generic_data.funcs.emitheader} != '' ]] && \
    { ${dq_vt_generic_data.funcs.emitheader} "$@" || return 1; }

    return 0
}

function dq_vt_generic_emitbody {
    typeset nodataflag=y file pagen page pagei paget findex

    [[ ${dq_vt_generic_data.rendermode} == embed ]] && return 0

    dq_main_emit_sdiv_begin \
        generic $SCREENWIDTH 200 "${dq_vt_generic_data.divmode}" \
    "Generic View"
    while read file; do
        if [[ $file == '<'*'>' ]] then
            print -r "$file"
            continue
        fi
        [[ $file != *.html ]] && continue
        if [[ ${dq_vt_generic_data.attrs.pagemode} == all ]] then
            pagen=
        else
            pagen=$(tail -1 ${file%.html}.toc); pagen=${pagen%%'|'*}
            [[ $pagen == 1 ]] && pagen=
        fi
        if [[ $pagen == '' ]] then
            cat $file
        else
            findex=${file//*.+([0-9]).*/\1}
            typeset -n pi=dq_vt_generic_data.attrs.pageindex$findex
            pi=${pi:-1}
            dq_main_emit_sdivpage_begin generic $file $pagen $pi
            while read page; do
                pagei=${page%%'|'*}
                paget=${page##*'|'}
                paget="$pagei/$pagen - $paget..."
                dq_main_emit_sdivpage_option "$pagei" "$paget"
            done < ${file%.html}.toc
            dq_main_emit_sdivpage_middle generic $file $pagen $pi
            dq_vt_generic_emitpage $file $pi
            dq_main_emit_sdivpage_end generic $file $pagen $pi
        fi
        nodataflag=n
    done < generic.list
    [[ $HIDEINFO == '' && -f generic.info ]] && cat generic.info
    if [[ $nodataflag == y ]] then
        print "The query returned no data records."
    fi

    [[ ${dq_vt_generic_data.funcs.emitbody} != '' ]] && \
    { ${dq_vt_generic_data.funcs.emitbody} "$@" || return 1; }

    dq_main_emit_sdiv_end

    return 0
}

function dq_vt_generic_emitpage { # $1 = file $2 = page
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

    [[ ${dq_vt_generic_data.funcs.emitpage} != '' ]] && \
    { ${dq_vt_generic_data.funcs.emitpage} "$@" || return 1; }

}

function dq_vt_generic_setup {
    dq_main_data.query.vtools.generic=(args=())
    typeset -n itr=dq_main_data.query.vtools.generic.args
    typeset -n qditr=querydata.vt_generic
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
    itr.pagemode=${prefqp.genericpage:-all}

    itr.funcs.init=${qditr.funcs.init}
    itr.funcs.run=${qditr.funcs.run}
    itr.funcs.emitheader=${qditr.funcs.emitheader}
    itr.funcs.emitbody=${qditr.funcs.emitbody}
    itr.funcs.emitpage=${qditr.funcs.emitpage}
    itr.funcs.setup=${qditr.funcs.setup}

    itr.genericattr_bgcolor=${prefpp.bgcolor}
    itr.genericattr_color=${prefpp.fgcolor}
    itr.genericattr_fontname=${prefpp.fontfamily}
    itr.genericattr_fontsize=${prefpp.fontsize}
    for k in "${!qditr.genericattr.@}"; do
        typeset -n srcr=$k dstr=itr.genericattr_${k##*.}
        dstr=$srcr
    done

    [[ ${dq_vt_generic_data.funcs.setup} != '' ]] && \
    { ${dq_vt_generic_data.funcs.setup} "$@" || return 1; }

    return 0
}

if [[ $SWIFTWARNLEVEL != "" ]] then
    PS4='${.sh.fun}-$LINENO: '
    typeset -ft dq_vt_generic_init dq_vt_generic_run dq_vt_generic_setup
    typeset -ft dq_vt_generic_emitbody dq_vt_generic_emitheader
    typeset -ft dq_vt_generic_emitpage
fi
