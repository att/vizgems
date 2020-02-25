
dq_vt_alarmstyle_data=(
    deps='' rerun=''
    attrs=() embeds=''
)

function dq_vt_alarmstyle_init {
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
    for n in "${!vars.alarmstyle_@}"; do
        typeset -n vr=$n
        k=${n##*alarmstyle_}
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
            typeset -n attr=dq_vt_alarmstyle_data.attrs.$n1
            if [[ $attr != '' ]] then
                attr+=" $n2='${v//\'/\"}'"
            else
                attr="$n2='${v//\'/\"}'"
            fi
        else
            typeset -n attr=dq_vt_alarmstyle_data.attrs.$k
            attr=$v
        fi
        if [[ $v == *EMBED:*qimg* ]] then
            ev=$v
            while [[ $ev == *'<qimg>'* ]] do
                id=${ev%%'</qimg>'*}
                ev=${ev#*'</qimg>'}
                id=${id%%'</path>'}
                id=${id##*'<path>'}
                typeset -n embeds=dq_vt_alarmstyle_data.embeds
                embeds+=" $id"
                dq_vt_alarmstyle_data.deps+=" $id"
            done
        fi
    done
    dq_vt_alarmstyle_data.rendermode=${prefs[rendermode]:-final}
    dq_vt_alarmstyle_data.divmode=${prefs[divmode]:-max}
    dq_vt_alarmstyle_data.attrs.runid=alarmstyle

    return 0
}

function dq_vt_alarmstyle_run {
    typeset -n vt=$1
    typeset attri embed

    export ALARMSTYLEATTRFILE=alarmstyle.attr
    export ALARMSTYLEEMBEDLISTFILE=alarmstyle.embedlist
    rm -f alarmstyle.*

    (
        print "alarmgroup=${dq_dt_alarm_data.group}"
        for attri in "${!dq_vt_alarmstyle_data.attrs.@}"; do
            typeset -n attr=$attri
            print -r "${attri##*.}=$attr"
        done
    ) > $ALARMSTYLEATTRFILE

    (
        for embed in ${dq_vt_alarmstyle_data.embeds}; do
            print "$embed|$embed.embed"
        done
    ) | sort -u > $ALARMSTYLEEMBEDLISTFILE

    ddsfilter -osm none -fso vg_dq_vt_alarmstyle.filter.so alarm.dds \
    > alarmstyle.list
    return $?
}

function dq_vt_alarmstyle_emitheader {
    typeset file

    while read file; do
        [[ $file != *.css ]] && continue
        cat $file
    done < alarmstyle.list

    return 0
}

function dq_vt_alarmstyle_emitbody {
    typeset nodataflag=y file

    [[ ${dq_vt_alarmstyle_data.rendermode} == embed ]] && return 0

    dq_main_emit_sdiv_begin \
        alarmstyle $SCREENWIDTH 200 "${dq_vt_alarmstyle_data.divmode}" \
    "Alarm View"
    while read file; do
        [[ $file != *.gif ]] && continue
        print "<img src='$IMAGEPREFIX$file'>"
        nodataflag=n
    done < alarmstyle.list
    [[ $HIDEINFO == '' && -f alarmstyle.info ]] && cat alarmstyle.info
    if [[ $nodataflag == y ]] then
        print "The query returned no alarm records."
    fi
    dq_main_emit_sdiv_end

    return 0
}

function dq_vt_alarmstyle_setup {
    dq_main_data.query.vtools.alarmstyle=(args=())
    typeset -n asr=dq_main_data.query.vtools.alarmstyle.args
    typeset -n qdasr=querydata.vt_alarmstyle
    typeset k

    asr.rendermode=${qdasr.rendermode:-final}
    asr.pagemode=${prefqp.alarmstylepage:-all}

    asr.alarmstyleattr_flags=""
    asr.alarmstyleattr_bgcolor=${prefpp.bgcolor}
    asr.alarmstyleattr_color=${prefpp.fgcolor}
    asr.alarmstyleattr_tmodenames=${dq_main_data.alarmattr.ticketlabel20}
    asr.alarmstyleattr_sevnames=${dq_main_data.alarmattr.severitylabel}
    for k in "${!qdasr.alarmstyleattr.@}"; do
        typeset -n srcr=$k dstr=asr.alarmstyleattr_${k##*.}
        dstr=$srcr
    done

    asr.alarmattr_color=${dq_main_data.alarmattr.color}
    for k in "${!qdasr.alarmattr.@}"; do
        typeset -n srcr=$k dstr=asr.alarmattr_${k##*.}
        dstr=$srcr
    done

    return 0
}

if [[ $SWIFTWARNLEVEL != "" ]] then
    PS4='${.sh.fun}-$LINENO: '
    typeset -ft dq_vt_alarmstyle_init dq_vt_alarmstyle_run
    typeset -ft dq_vt_alarmstyle_setup
    typeset -ft dq_vt_alarmstyle_emitbody dq_vt_alarmstyle_emitheader
fi
