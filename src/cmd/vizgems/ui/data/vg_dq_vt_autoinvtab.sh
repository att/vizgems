
dq_vt_autoinvtab_data=(
    deps='' rerun=''
    attrs=() embeds=''
    fgs=() bgs=() fgn=0 bgn=0 fgis=() bgis=() styles=() stylei=0
    is=() es=() fs=() ts=() ps=() tbli=-1 roottbli=0 lv='' id=''
)

function dq_vt_autoinvtab_init {
    typeset -n vt=$1
    typeset -A prefs
    typeset n k v id ev
    typeset -n qprefs=$1.args

    for k in "${!qprefs.@}"; do
        n=${k##*$1.args?(.)}
        [[ $n == '' || $n == *.* ]] && continue
        typeset -n vr=$k
        prefs[$n]=$vr
    done
    for n in "${!vars.autoinvtab_@}"; do
        typeset -n vr=$n
        k=${n##*autoinvtab_}
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
        typeset -n attr=dq_vt_autoinvtab_data.attrs.$k
        attr=$v
        if [[ $v == *EMBED:*qimg* ]] then
            ev=$v
            while [[ $ev == *'<qimg>'* ]] do
                id=${ev%%'</qimg>'*}
                ev=${ev#*'</qimg>'}
                id=${id%%'</path>'}
                id=${id##*'<path>'}
                typeset -n embeds=dq_vt_autoinvtab_data.embeds
                embeds+=" $id"
                dq_vt_autoinvtab_data.deps+=" $id"
            done
        fi
    done
    dq_vt_autoinvtab_data.rendermode=${prefs[rendermode]:-final}
    dq_vt_autoinvtab_data.sections=${prefs[sections]:-I}
    dq_vt_autoinvtab_data.divmode=${prefs[divmode]:-max}
    dq_vt_autoinvtab_data.attrs.runid=autoinvtab

    return 0
}

function dq_vt_autoinvtab_run {
    typeset -n vt=$1
    typeset -A toolsbyst toolsloaded
    typeset attri embed ifs users groups st stre tool fg bg
    typeset u t lv id rest kv n en fn tn pn style class rdir

    export AUTOINVTABATTRFILE=autoinvtab.attr
    export AUTOINVTABEMBEDLISTFILE=autoinvtab.embedlist
    rm -f autoinvtab.*

    (
        u=${dq_main_data.fullurl}
        u=${u//autoinvtab_sortorder=*([!&])/}
        u=${u//'&&'/'&'}
        print -r "sorturl=$u"
        for attri in "${!dq_vt_autoinvtab_data.attrs.@}"; do
            typeset -n attr=$attri
            print -r "${attri##*.}=$attr"
        done
    ) > $AUTOINVTABATTRFILE

    (
        for embed in ${dq_vt_autoinvtab_data.embeds}; do
            print "$embed|$embed.embed"
        done
    ) | sort -u > $AUTOINVTABEMBEDLISTFILE

    ifs="$IFS"
    IFS='|'
    egrep -v '^#' $SWIFTDATADIR/uifiles/autoinv/tools.txt \
    | while read -r line; do
        users=${line%%'|'*}; rest=${line##"$users"?('|')}
        groups=${rest%%'|'*}; rest=${rest##"$groups"?('|')}
        tool=${rest%%'|'*}; rest=${rest##"$tool"?('|')}
        st=${rest}
        if [[ $users != '' && " $SWMID " != @(*\ (${users//:/'|'})\ *) ]] then
            continue
        fi
        if [[ $groups != '' ]] && ! swmuseringroups "${groups//:/'|'}"; then
            continue
        fi
        [[ ${toolsbyst[$st]} != '' ]] && continue
        toolsbyst[$st]=$tool
    done
    if [[ -f $SWIFTDATADIR/uifiles/autoinv/default.sh ]] then
        . $SWIFTDATADIR/uifiles/autoinv/default.sh
        toolsloaded[default]=1
    else
        print -u2 ERROR cannot find default autoinv tool
        return 1
    fi
    typeset -n attrs=dq_vt_autoinvtab_data.attrs
    for fg in ${attrs.nodeattr_color:-black}; do
        dq_vt_autoinvtab_data.fgs.[${dq_vt_autoinvtab_data.fgn}]=$fg
        (( dq_vt_autoinvtab_data.fgn++ ))
    done
    for bg in ${attrs.nodeattr_bgcolor:-white}; do
        dq_vt_autoinvtab_data.bgs.[${dq_vt_autoinvtab_data.bgn}]=$bg
        (( dq_vt_autoinvtab_data.bgn++ ))
    done

    rdir=${dq_main_data.rdir}
    export LEVELMAPFILE=$rdir/LEVELS-maps.dds
    export INVMAPFILE=$rdir/inv-maps-uniq.dds
    export INVNODEATTRFILE=$rdir/inv-nodes.dds
    export OBJECTTRANSREVFILE=$rdir/objecttrans-rev.dds
    print "o" > autoinvtablevellist.txt
    export LEVELLISTFILE=autoinvtablevellist.txt
    export LEVELLISTSIZE=$(wc -l < $LEVELLISTFILE)
    export SEVMAPFILE=$rdir/sev.list
    export STATMAPFILE=$rdir/stat.list
    export ACCOUNTFILE=$SWIFTDATADIR/dpfiles/account.filter
    export ALARMFILTERFILE=$SWIFTDATADIR/dpfiles/alarm/alarm.filter
    export ALARMEMAILFILE=$SWIFTDATADIR/dpfiles/alarm/alarmemail.filter
    export PROFILEFILE=$SWIFTDATADIR/dpfiles/stat/profile.txt
    export THRESHOLDFILE=$SWIFTDATADIR/dpfiles/stat/threshold.txt
    export SHOWEMAILS=0 SHOWFILTERS=0 SHOWTHRESHOLDS=0 SHOWPROFILES=0
    [[ ${dq_vt_autoinvtab_data.sections} == *F* ]] && SHOWFILTERS=1
    [[ ${dq_vt_autoinvtab_data.sections} == *E* ]] && SHOWEMAILS=1
    [[ ${dq_vt_autoinvtab_data.sections} == *P* ]] && SHOWPROFILES=1
    [[ ${dq_vt_autoinvtab_data.sections} == *T* ]] && SHOWTHRESHOLDS=1
    ddsfilter -osm none -fso vg_dq_vt_autoinvtab.filter.so inv.dds \
    2>&1 > autoinvtab.recs | egrep -v 'account not found' 1>&2

    n=0 en=0 fn=0 tn=0 pn=0
    sort -u -t'|' -k2,3 autoinvtab.recs | while read t lv id rest; do
        dq_vt_autoinvtab_data.lv=$lv
        dq_vt_autoinvtab_data.id=$id
        unset dq_vt_autoinvtab_data.is; dq_vt_autoinvtab_data.is=()
        unset dq_vt_autoinvtab_data.es; dq_vt_autoinvtab_data.es=()
        unset dq_vt_autoinvtab_data.fs; dq_vt_autoinvtab_data.fs=()
        unset dq_vt_autoinvtab_data.ps; dq_vt_autoinvtab_data.ps=()
        unset dq_vt_autoinvtab_data.ts; dq_vt_autoinvtab_data.ts=()
        set -f
        egrep "^.\|$lv\|$id\|" autoinvtab.recs | while read -r t lv id rest; do
            case $t in
            I)
                set -A kv -- $rest
                typeset -n ip=dq_vt_autoinvtab_data.is.[${kv[0]}]
                ip=${kv[1]}
                ;;
            E)
                typeset -n ep=dq_vt_autoinvtab_data.es.[$en]
                ep=$rest
                (( en++ ))
                ;;
            F)
                typeset -n fp=dq_vt_autoinvtab_data.fs.[$fn]
                fp=$rest
                (( fn++ ))
                ;;
            P)
                typeset -n pp=dq_vt_autoinvtab_data.ps.[$pn]
                pp=$rest
                (( pn++ ))
                ;;
            T)
                typeset -n tp=dq_vt_autoinvtab_data.ts.[$tn]
                tp=$rest
                (( tn++ ))
                ;;
            esac
        done
        set +f
        dq_vt_autoinvtab_data.en=$en
        dq_vt_autoinvtab_data.fn=$fn
        dq_vt_autoinvtab_data.pn=$pn
        dq_vt_autoinvtab_data.tn=$tn
        typeset -n ip=dq_vt_autoinvtab_data.is.name
        dq_vt_autoinvtab_data.name=$ip
        typeset -n ip=dq_vt_autoinvtab_data.is.scope_systype
        st=$ip
        dq_vt_autoinvtab_data.st=$st
        typeset -n ip=dq_vt_autoinvtab_data.is.scope_servicelevel
        dq_vt_autoinvtab_data.sl=$ip
        if [[ $st == '' ]] then
            tool=default
        elif [[ ${toolsbyst[$st]} == '' ]] then
            tool=
            for stre in "${!toolsbyst[@]}"; do
                if [[ $st == $stre ]] then
                    tool=${toolsbyst[$stre]}
                    break
                fi
            done
            [[ $tool == '' ]] && tool=default
        else
            tool=${toolsbyst[$st]}
        fi
        if [[ ${toolsloaded[$tool]} == '' ]] then
            if [[ -f $SWIFTDATADIR/uifiles/autoinv/$tool.sh ]] then
                . $SWIFTDATADIR/uifiles/autoinv/$tool.sh
                toolsloaded[$tool]=1
            else
                print -u2 ERROR cannot find $tool autoinv tool
                return 1
            fi
        fi
        dq_vt_autoinvtab_tool_$tool > autoinvtab.$n.html
        print autoinvtab.$n.html
        (( n++ ))
    done > autoinvtab.list

    (
        print -r "<style type='text/css'>"
        for style in "${!dq_vt_autoinvtab_data.styles.@}"; do
            typeset -n class=$style
            style=${style#'dq_vt_autoinvtab_data.styles.['*:}
            style=${style%']'}
            style="$style; padding:2px 2px 2px 2px"
            if [[ $style != *text-align* ]] then
                style="$style; text-align:left"
            fi
            print -r "${class} { ${style} }"
            if [[ $class == tr.* ]] then
                print -r "${class/tr/td} { ${style} }"
            fi
            if [[ $class == caption.* ]] then
                print -r "${class/caption/a} { ${style} }"
            fi
        done
        print -r "</style>"
    ) > autoinvtab.css

    print autoinvtab.css >> autoinvtab.list
    IFS="$ifs"
}

function dq_vt_autoinvtab_emitheader {
    typeset file

    while read file; do
        [[ $file != *.css ]] && continue
        cat $file
    done < autoinvtab.list

    return 0
}

function dq_vt_autoinvtab_emitbody {
    typeset nodataflag=y file

    [[ ${dq_vt_autoinvtab_data.rendermode} == embed ]] && return 0

    dq_main_emit_sdiv_begin \
        autoinvtab $SCREENWIDTH 200 "${dq_vt_autoinvtab_data.divmode}" \
    "Inv Tabular View"
    while read file; do
        [[ $file != *.html ]] && continue
        cat $file
        print "<br>"
        nodataflag=n
    done < autoinvtab.list
    if [[ $nodataflag == y ]] then
        print "The query returned no inventory records."
    fi
    dq_main_emit_sdiv_end

    return 0
}

function dq_vt_autoinvtab_begintable { # $1=extra table attr
    typeset tbli bg fg fn fs st class

    (( dq_vt_autoinvtab_data.tbli++ ))
    tbli=${dq_vt_autoinvtab_data.tbli}
    typeset -n attrs=dq_vt_autoinvtab_data.attrs
    fn=${attrs.autoinvtabattr_fontname:-'Times Roman'}
    fs=${attrs.autoinvtabattr_fontsize:-10pt}
    bg=${attrs.autoinvtabattr_bgcolor:-black}
    fg=${attrs.autoinvtabattr_color:-white}
    st="font-family:$fn; font-size:$fs; background-color:$bg; color:$fg"
    typeset -n style=dq_vt_autoinvtab_data.styles.[table:$st]
    if [[ $style != '' ]] then
        class=$style
    else
        class="table.autoinvtab_${dq_vt_autoinvtab_data.stylei}"
        style=$class
        (( dq_vt_autoinvtab_data.stylei++ ))
    fi
    dq_vt_autoinvtab_data.fgis.[$tbli]=0
    dq_vt_autoinvtab_data.bgis.[$tbli]=0
    print -r "<table class='${class#*.}' $1>"
}

function dq_vt_autoinvtab_endtable { # $1=tbli
    print -r "</table>"
}

function dq_vt_autoinvtab_beginsubtable { # $1=tbli $2=row $3=extra table attr
    typeset tbli=$1 row="$2|"
    typeset tblj bg fg fn fs st class tds tdi cl1

    typeset -n attrs=dq_vt_autoinvtab_data.attrs
    fn=${attrs.nodeattr_fontname:-'Times Roman'}
    fs=${attrs.nodeattr_fontsize:-10pt}
    typeset -n fgi=dq_vt_autoinvtab_data.fgis.[$tbli]
    typeset -n fg=dq_vt_autoinvtab_data.fgs.[$fgi]
    (( fgi = (fgi + 1) % dq_vt_autoinvtab_data.fgn ))
    typeset -n bgi=dq_vt_autoinvtab_data.bgis.[$tbli]
    typeset -n bg=dq_vt_autoinvtab_data.bgs.[$bgi]
    (( bgi = (bgi + 1) % dq_vt_autoinvtab_data.bgn ))
    st="font-family:$fn; font-size:$fs; background-color:$bg; color:$fg"
    typeset -n style=dq_vt_autoinvtab_data.styles.[tr:$st]
    if [[ $style != '' ]] then
        class=$style
    else
        class="tr.autoinvtab_${dq_vt_autoinvtab_data.stylei}"
        style=$class
        (( dq_vt_autoinvtab_data.stylei++ ))
    fi
    print -r "<tr class='${class#*.}'>"
    cl1=$class
    set -f
    set -A tds -- $row
    set +f
    for tdi in "${tds[@]}"; do
        if [[ $tdi == @(left|right|center):* ]] then
            print -rn "<td class='${class#*.}' style='text-align:${tdi%:*}'>"
            print -rn "${tdi#*:}"
            print -r "</td>"
        else
            print -r "<td class='${class#*.}'>$tdi</td>"
        fi
    done
    typeset +n fg bg

    (( dq_vt_autoinvtab_data.tbli++ ))
    tblj=${dq_vt_autoinvtab_data.tbli}
    typeset -n attrs=dq_vt_autoinvtab_data.attrs
    fn=${attrs.autoinvtabattr_fontname:-'Times Roman'}
    fs=${attrs.autoinvtabattr_fontsize:-10pt}
    bg=${attrs.autoinvtabattr_bgcolor:-black}
    fg=${attrs.autoinvtabattr_color:-white}
    st="font-family:$fn; font-size:$fs; background-color:$bg; color:$fg"
    typeset -n style=dq_vt_autoinvtab_data.styles.[table:$st]
    if [[ $style != '' ]] then
        class=$style
    else
        class="table.autoinvtab_${dq_vt_autoinvtab_data.stylei}"
        style=$class
        (( dq_vt_autoinvtab_data.stylei++ ))
    fi
    dq_vt_autoinvtab_data.fgis.[$tblj]=0
    dq_vt_autoinvtab_data.bgis.[$tblj]=0
    print -r "<td class='${cl1#*.}'><table class='${class#*.}' $1>"
}

function dq_vt_autoinvtab_endsubtable { # $1=tbli
    print -r "</table></td>"
    print -r "</tr>"
}

function dq_vt_autoinvtab_addcaption { # $1=tbli $2=caption
    typeset tbli=$1
    typeset bg fg fn fs st class u obj l

    typeset -n attrs=dq_vt_autoinvtab_data.attrs
    fn=${attrs.autoinvtabattr_fontname:-'Times Roman'}
    fs=${attrs.autoinvtabattr_fontsize:-10pt}
    bg=${attrs.autoinvtabattr_bgcolor:-black}
    fg=${attrs.autoinvtabattr_color:-white}
    st="font-family:$fn; font-size:$fs; background-color:$bg; color:$fg"
    st="$st; text-align:center"
    typeset -n style=dq_vt_autoinvtab_data.styles.[caption:$st]
    if [[ $style != '' ]] then
        class=$style
    else
        class="caption.autoinvtab_${dq_vt_autoinvtab_data.stylei}"
        style=$class
        (( dq_vt_autoinvtab_data.stylei++ ))
    fi
    if [[
        $tbli == ${dq_vt_autoinvtab_data.roottbli} &&
        ${attrs.autoinvtaburl} != ''
    ]] then
        u=${attrs.autoinvtaburl}
        obj=n:${dq_vt_autoinvtab_data.lv}:${dq_vt_autoinvtab_data.id}
        u=${u//'%(_object_)%'/$obj}
        u=${u//'%(_level_)%'/${dq_vt_autoinvtab_data.lv}}
        u=${u//'%(name)%'/${dq_vt_autoinvtab_data.name}}
        if [[ ${dq_vt_autoinvtab_data.sections} == I ]] then
            u+="&autoinvtab_sections=IFEPT"
            l='&nbsp;also show filters emails profiles and thresholds&nbsp;'
        else
            u+="&autoinvtab_sections=I"
            l='&nbsp;only show inventory&nbsp;'
        fi
        print -rn "<caption class='${class#*.}'>"
        print -rn "<a class='${class#*.}' href='$u' title='$l'>$2</a>"
        print -r "</caption>"
    else
        print -r "<caption class='${class#*.}'>$2</caption>"
    fi
}

function dq_vt_autoinvtab_addheaderrow { # $1=tbli $2=row
    typeset tbli=$1 row="$2|"
    typeset bg fg fn fs st class tds tdi

    typeset -n attrs=dq_vt_autoinvtab_data.attrs
    fn=${attrs.autoinvtabattr_fontname:-'Times Roman'}
    fs=${attrs.autoinvtabattr_fontsize:-10pt}
    bg=${attrs.autoinvtabattr_bgcolor:-black}
    fg=${attrs.autoinvtabattr_color:-white}
    st="font-family:$fn; font-size:$fs; background-color:$bg; color:$fg"
    typeset -n style=dq_vt_autoinvtab_data.styles.[tr:$st]
    if [[ $style != '' ]] then
        class=$style
    else
        class="tr.autoinvtab_${dq_vt_autoinvtab_data.stylei}"
        style=$class
        (( dq_vt_autoinvtab_data.stylei++ ))
    fi
    print -r "<tr class='${class#*.}'>"
    set -f
    set -A tds -- $row
    set +f
    for tdi in "${tds[@]}"; do
        if [[ $tdi == @(left|right|center):* ]] then
            print -rn "<td class='${class#*.}' style='text-align:${tdi%:*}'>"
            print -rn "${tdi#*:}"
            print -r "</td>"
        else
            print -r "<td class='${class#*.}'>$tdi</td>"
        fi
    done
    print -r "</tr>"
}

function dq_vt_autoinvtab_adddatarow { # $1=tbli $2=row
    typeset tbli=$1 row="$2|"
    typeset bg fg fn fs st class tds tdi

    typeset -n attrs=dq_vt_autoinvtab_data.attrs
    fn=${attrs.nodeattr_fontname:-'Times Roman'}
    fs=${attrs.nodeattr_fontsize:-10pt}
    typeset -n fgi=dq_vt_autoinvtab_data.fgis.[$tbli]
    typeset -n fg=dq_vt_autoinvtab_data.fgs.[$fgi]
    (( fgi = (fgi + 1) % dq_vt_autoinvtab_data.fgn ))
    typeset -n bgi=dq_vt_autoinvtab_data.bgis.[$tbli]
    typeset -n bg=dq_vt_autoinvtab_data.bgs.[$bgi]
    (( bgi = (bgi + 1) % dq_vt_autoinvtab_data.bgn ))
    st="font-family:$fn; font-size:$fs; background-color:$bg; color:$fg"
    typeset -n style=dq_vt_autoinvtab_data.styles.[tr:$st]
    if [[ $style != '' ]] then
        class=$style
    else
        class="tr.autoinvtab_${dq_vt_autoinvtab_data.stylei}"
        style=$class
        (( dq_vt_autoinvtab_data.stylei++ ))
    fi
    print -r "<tr class='${class#*.}'>"
    set -f
    set -A tds -- $row
    set +f
    for tdi in "${tds[@]}"; do
        if [[ $tdi == @(left|right|center):* ]] then
            print -rn "<td class='${class#*.}' style='text-align:${tdi%:*}'>"
            print -rn "${tdi#*:}"
            print -r "</td>"
        else
            print -r "<td class='${class#*.}'>$tdi</td>"
        fi
    done
    print -r "</tr>"
}

function dq_vt_autoinvtab_setup {
    dq_main_data.query.vtools.autoinvtab=(args=())
    typeset -n aitr=dq_main_data.query.vtools.autoinvtab.args
    typeset -n qdaitr=querydata.vt_autoinvtab
    typeset k
    typeset nurlfmt=${dq_main_data.nodeurlfmt}

    aitr.rendermode=${qdaitr.rendermode:-final}
    aitr.pagemode=${prefqp.autoinvtabpage:-all}

    aitr.autoinvtabattr_bgcolor=${prefpp.bgcolor}
    aitr.autoinvtabattr_color=${prefpp.fgcolor}
    aitr.autoinvtabattr_fontname=${prefpp.fontfamily}
    aitr.autoinvtabattr_fontsize=${prefpp.fontsize}
    for k in "${!qdaitr.autoinvtabattr.@}"; do
        typeset -n srcr=$k dstr=aitr.autoinvtabattr_${k##*.}
        dstr=$srcr
    done
    aitr.autoinvtaburl=${nurlfmt//\%(___NEXTQID___)\%/autoinvtab}

    aitr.nodeattr_bgcolor='#CCCCCC|#EEEEEE'
    aitr.nodeattr_color=${prefpp.fgcolor}
    aitr.nodeattr_fontname=${prefpp.fontfamily}
    aitr.nodeattr_fontsize=${ fssmaller ${prefpp.fontsize}; }
    for k in "${!qdaitr.nodeattr.@}"; do
        typeset -n srcr=$k dstr=aitr.nodeattr_${k##*.}
        dstr=$srcr
    done

    return 0
}

if [[ $SWIFTWARNLEVEL != "" ]] then
    PS4='${.sh.fun}-$LINENO: '
    typeset -ft dq_vt_autoinvtab_init dq_vt_autoinvtab_run
    typeset -ft dq_vt_autoinvtab_setup
    typeset -ft dq_vt_autoinvtab_emitheader dq_vt_autoinvtab_emitbody
    typeset -ft dq_vt_autoinvtab_begintable dq_vt_autoinvtab_endtable
    typeset -ft dq_vt_autoinvtab_beginsubtable dq_vt_autoinvtab_endsubtable
    typeset -ft dq_vt_autoinvtab_addcaption
    typeset -ft dq_vt_autoinvtab_addheaderrow dq_vt_autoinvtab_adddatarow
fi
