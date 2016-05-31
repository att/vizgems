
dq_dt_alarm_data=(
    deps='inv' rerun='' binarysearch=n
    alarmfilters=() alarmfiltern=0
    files=() filen=0
    group='' rdir=''
    alwaysreruninv=''
    filterstr=''
    allowclear='' label=''
)

function dq_dt_alarm_init { # $1 = dt query prefix
    typeset dtstr=$1
    typeset -n dt=$dtstr
    typeset -n afilters=dq_dt_alarm_data.alarmfilters
    typeset -n afiltern=dq_dt_alarm_data.alarmfiltern
    typeset afilteri
    typeset -n group=dq_dt_alarm_data.group
    typeset -n alwaysreruninv=dq_dt_alarm_data.alwaysreruninv
    typeset -n filterstr=dq_dt_alarm_data.filterstr
    typeset ifs param vs vi s1 k v fgroup found rdir ft lt lod

    ifs="$IFS"
    IFS='|'
    for param in ccid severity tmode msgtxt comment; do
        typeset -n vs=vars.alarm_$param
        if (( ${#vs[@]} > 0 )) && [[ ${vs[0]} != '' ]] then
            s1=
            for vi in "${vs[@]}"; do
                s1+="|$vi"
            done
            s1=${s1#\|}
            case $param in
            ccid|severity|tmode)
                k="$param" v="${s1}"
                ;;
            msgtxt|comment)
                k="$param" v=".*(${s1}).*"
                ;;
            esac
            typeset -n afilter=dq_dt_alarm_data.alarmfilters._$afiltern
            afilter=(t=A k="$k" v="$v")
            (( afiltern++ ))
        fi
    done
    for fgroup in onempty always; do
        if [[ $fgroup == onempty ]] && (( afiltern > 0 )) then
            continue
        fi
        for param in ccid severity tmode msgtxt comment; do
            typeset -n vs=$dtstr.filters.$fgroup.alarm_$param
            [[ $vs == '' ]] && continue
            found=n
            for (( afilteri = 0; afilteri < afiltern; afilteri++ )) do
                typeset -n afilter=dq_dt_alarm_data.alarmfilters._$afilteri
                [[ ${afilter.k} != $param ]] && continue
                s1=${afilter.v}
                found=y
            done
            if [[ $found == n ]] then
                typeset -n afilter=dq_dt_alarm_data.alarmfilters._$afiltern
                afilter=(t=A k="$param" v="" f=trans)
                s1=
                (( afiltern++ ))
            fi
            case $param in
            *)
                s1+="|$vs"
                ;;
            esac
            afilter.v=${s1#\|}
        done
    done
    IFS="$ifs"

    rdir=${dq_main_data.rdir}
    ft=${dq_main_data.ft}
    lt=${dq_main_data.lt}
    group=${dq_main_data.alarmgroup}
    case $group in
    open)
        dq_dt_alarm_openinfo $ft $lt
        ;;
    all)
        lod=${dq_main_data.alarmlod}
        case $lod in
        ymd)
            dq_dt_alarm_allinfo_ymd $ft $lt
            ;;
        ym)
            dq_dt_alarm_allinfo_ym $ft $lt
            ;;
        y)
            dq_dt_alarm_allinfo_y $ft $lt
            ;;
        esac
        ;;
    esac
    if [[ ${dt.args.reruninv} != '' ]] then
        alwaysreruninv=${dt.args.reruninv}
    elif [[ ${vars.reruninv} == onalarms ]] then
        alwaysreruninv=y
    else
        alwaysreruninv=n
    fi

    for (( afilteri = 0; afilteri < afiltern; afilteri++ )) do
        typeset -n afilter=dq_dt_alarm_data.alarmfilters._$afilteri
        [[ ${afilter.f} == trans ]] && continue
        filterstr+="&alarm_${afilter.k}=$(printf '%#H' "${afilter.v}")"
    done
    filterstr=${filterstr#'&'}

    export SEVMAPFILE=$rdir/sev.list

    return 0
}

function dq_dt_alarm_openinfo { # $1 = ft $2 = lt
    typeset ft=$1 lt=$2
    typeset rdir cfile

    typeset -n filen=dq_dt_alarm_data.filen
    filen=0

    rdir=${dq_main_data.rdir}
    cfile=$rdir/open.alarms.dds
    [[ ! -f $cfile ]] && return 1
    typeset -n file=dq_dt_alarm_data.files._$filen
    file="$cfile"
    (( filen++ ))
    dq_dt_alarm_data.allowclear=y

    return 0
}

function dq_dt_alarm_allinfo_ymd { # $1 = ft $2 = lt
    typeset ft=$1 lt=$2
    typeset ds ct mt dir cfile
    typeset fyyyymmdd fhh lyyyymmdd lhh cyyyymmdd chh

    typeset -n filen=dq_dt_alarm_data.filen
    filen=0

    set -A ds -- $(printf '%(%Y/%m/%d %H)T' \#$ft)
    fyyyymmdd=${ds[0]}
    fhh=${ds[1]}
    set -A ds -- $(printf '%(%Y/%m/%d %H)T' \#$lt)
    lyyyymmdd=${ds[0]}
    lhh=${ds[1]}

    for (( ct = ft; ct <= lt; )) do
        mt=$(printf '%(%#)T' $(printf '%(%Y/%m/%d-%H:00:00)T' "#$ct"))
        set -A ds -- $(printf '%(%Y/%m/%d %H)T' \#$mt)
        cyyyymmdd=${ds[0]}
        chh=${ds[1]}
        dir=$SWIFTDATADIR/data/main/$cyyyymmdd/processed/total
        cfile=$dir/all.$chh.alarms.dds
        if [[ -f $cfile ]] then
            typeset -n file=dq_dt_alarm_data.files._$filen
            file="$cfile"
            (( filen++ ))
        fi
        (( ct += 60 * 60 ))
        ct=$(printf '%(%#)T' $(printf '%(%Y/%m/%d-%H:00:00)T' "#$ct"))
    done

    return 0
}

function dq_dt_alarm_allinfo_ym { # $1 = ft $2 = lt
    typeset ft=$1 lt=$2
    typeset ds ct mt dir cfile
    typeset fyyyymm fdd lyyyymm ldd cyyyymm cdd

    typeset -n filen=dq_dt_alarm_data.filen
    filen=0

    set -A ds -- $(printf '%(%Y/%m %d %H)T' \#$ft)
    fyyyymm=${ds[0]}
    fdd=${ds[1]}
    set -A ds -- $(printf '%(%Y/%m %d %H)T' \#$lt)
    lyyyymm=${ds[0]}
    ldd=${ds[1]}

    for (( ct = ft; ct <= lt; )) do
        mt=$(printf '%(%#)T' "#$ct midnight")
        set -A ds -- $(printf '%(%Y/%m %d)T' \#$mt)
        cyyyymm=${ds[0]}
        cdd=${ds[1]}
        dir=$SWIFTDATADIR/data/main/$cyyyymm/processed/total
        cfile=$dir/all.$cdd.alarms.dds
        if [[ -f $cfile ]] then
            typeset -n file=dq_dt_alarm_data.files._$filen
            file="$cfile"
            (( filen++ ))
        fi
        if [[ -f ${cfile//all.$cdd/uniq} ]] then
            dq_dt_alarm_data.rdir=${cfile%/*}
        fi
        ct=$(printf '%(%#)T' "#$ct midnight tomorrow")
    done
    dq_dt_alarm_data.binarysearch=y

    return 0
}

function dq_dt_alarm_allinfo_y { # $1 = ft $2 = lt
    typeset ft=$1 lt=$2
    typeset ds ct mt dir cfile
    typeset fyyyy fmm lyyyy lmm cyyyy cmm

    typeset -n filen=dq_dt_alarm_data.filen
    filen=0

    set -A ds -- $(printf '%(%Y %m %d %H)T' \#$ft)
    fyyyy=${ds[0]}
    fmm=${ds[1]}
    set -A ds -- $(printf '%(%Y %m %d %H)T' \#$lt)
    lyyyy=${ds[0]}
    lmm=${ds[1]}

    for (( ct = ft; ct <= lt; )) do
        mt=$(printf '%(%#)T' $(printf '%(%Y/%m/01-00:00:00)T' "#$ct"))
        set -A ds -- $(printf '%(%Y %m)T' \#$mt)
        cyyyy=${ds[0]}
        cmm=${ds[1]}
        dir=$SWIFTDATADIR/data/main/$cyyyy/processed/total
        cfile=$dir/all.$cmm.alarms.dds
        if [[ -f $cfile ]] then
            typeset -n file=dq_dt_alarm_data.files._$filen
            file="$cfile"
            (( filen++ ))
        fi
        if [[ -f ${cfile//all.$cmm/uniq} ]] then
            dq_dt_alarm_data.rdir=${cfile%/*}
        fi
        ct=$(printf '%(%#)T' $(printf '%(%Y/%m/01-00:00:00)T' "#$ct"))
        (( ct += 32 * 24 * 60 * 60 ))
        ct=$(printf '%(%#)T' $(printf '%(%Y/%m/01-00:00:00)T' "#$ct"))
    done
    dq_dt_alarm_data.binarysearch=y

    return 0
}

function dq_dt_alarm_run { # $1 = dt query prefix
    typeset -n dt=$1 idt=${1%.alarm}.inv
    typeset -n afiltern=dq_dt_alarm_data.alarmfiltern
    typeset afilteri
    typeset -n filen=dq_dt_alarm_data.filen
    typeset -n infiltern=dq_dt_inv_data.invnodefiltern
    typeset -n iefiltern=dq_dt_inv_data.invedgefiltern
    typeset -n alwaysreruninv=dq_dt_alarm_data.alwaysreruninv
    typeset ft lt args argi filei

    export ALARMFILTERFILE=alarm.filter ALARMFILTERSIZE=$afiltern
    export ALARMFILEFILE=alarm.files ALARMFILESIZE=$filen
    export ALARMATTRFILE=alarm.attr

    rm -f $ALARMFILTERFILE $ALARMATTRFILE $ALARMFILEFILE
    rm -f alarm.dds alarminv.filter

    ft=${dq_main_data.ft}
    lt=${dq_main_data.lt}
    (
        print -r -- "ftime=$ft"
        print -r -- "ltime=$lt"
        case ${dq_main_data.alarmgroup} in
        open)
            print -r -- "usetimerange=0"
            ;;
        *)
            print -r -- "usetimerange=1"
            ;;
        esac
        for argi in "${!idt.args.@}"; do
            typeset -n arg=$argi
            [[ $argi != *.?outlevel ]] && continue
            print -r -- "${argi##*.}=$arg"
        done
        for argi in "${!dt.args.@}"; do
            typeset -n arg=$argi
            print -r -- "${argi##*.}=$arg"
        done
    ) > $ALARMATTRFILE

    for (( afilteri = 0; afilteri < afiltern; afilteri++ )) do
        typeset -n afilter=dq_dt_alarm_data.alarmfilters._$afilteri
        print -r "${afilter.t}|${afilter.k}|${afilter.v}"
    done > $ALARMFILTERFILE

    unset UNKNOWNMODE
    if (( infiltern + iefiltern == 0 )) then
        unset INVOUTFILE
    else
        if [[
            $infiltern == 1 && $iefiltern == 1 &&
            ${dq_dt_inv_data.invnodefilters._0.v} == UNKNOWN &&
            ${dq_dt_inv_data.invedgefilters._0.v} == \
            @(UNKNOWN_swsep_.*|.*_swsep_UNKNOWN)
        ]] && swmuseringroups 'vg_att_admin'; then
            export UNKNOWNMODE=1
        fi
        export INVOUTFILE=inv.dds
    fi
    export INVNODEFILTERFILE=invnode.filter INVNODEFILTERSIZE=$infiltern
    if [[ $alwaysreruninv == y ]] then
        export ALARMALWAYSRERUNINV=1
    fi

    if [[ ${dq_dt_alarm_data.binarysearch} == n ]] then
        args=
        for (( filei = filen - 1; filei >= 0; filei-- )) do
            typeset -n file=dq_dt_alarm_data.files._$filei
            ddscat $args $file
            args='-osm none'
        done | TZ=$PHTZ ddsconv $UIZFLAG \
            -os vg_dq_dt_alarm.schema -cso vg_dq_dt_alarm.conv.so \
        > alarm.dds 3> alarminv.filter
    else
        for (( filei = filen - 1; filei >= 0; filei-- )) do
            typeset -n file=dq_dt_alarm_data.files._$filei
        print -r -- "$file"
        done > $ALARMFILEFILE
        TZ=$PHTZ ddsconv $UIZFLAG \
            -os vg_dq_dt_alarm.schema -cso vg_dq_dt_alarm_bs.conv.so \
            ${dq_dt_alarm_data.rdir}/uniq.alarms.dds \
        > alarm.dds 3> alarminv.filter
    fi

    if (( infiltern + iefiltern == 0 )) || [[ $alwaysreruninv == y ]] then
        if [[ ! -s alarminv.filter ]] then
            print "I|_|___________" > alarminv.filter
        fi
        typeset -n idtt=dq_dt_inv_data
        idtt.appendfilterfiles+=" alarminv.filter"
        idtt.rerun=y
    fi

    return 0
}

function dq_dt_alarm_emitlabel { # $1 = dt query prefix
    typeset -n dt=$1 idt=${1%.alarm}.inv
    typeset -n afiltern=dq_dt_alarm_data.alarmfiltern
    typeset afilteri

    for (( afilteri = 0; afilteri < afiltern; afilteri++ )) do
        typeset -n afilter=dq_dt_alarm_data.alarmfilters._$afilteri
        dq_dt_alarm_data.label+=" ${afilter.k}:${afilter.v}"
    done
    dq_dt_alarm_data.label=${dq_dt_alarm_data.label#' '}

    return 0
}

function dq_dt_alarm_emitparams { # $1 = dt query prefix
    return 0
}

function dq_dt_alarm_setup {
    dq_main_data.query.dtools.alarm=(args=())
    typeset -n ar=dq_main_data.query.dtools.alarm.args
    typeset -n qdar=querydata.dt_alarm

    ar.inlevels=${qdar.inlevels}
    if [[ ${qdar.reruninv} != '' ]] then
        ar.reruninv=${qdar.reruninv}
    fi
    ar.runinparallel=${qdar.runinparallel:-n}

    return 0
}

if [[ $SWIFTWARNLEVEL != "" ]] then
    PS4='${.sh.fun}-$LINENO: '
    typeset -ft dq_dt_alarm_init dq_dt_alarm_run dq_dt_alarm_setup
    typeset -ft dq_dt_alarm_openinfo dq_dt_alarm_allinfo_ymd
    typeset -ft dq_dt_alarm_allinfo_ym dq_dt_alarm_allinfo_y
    typeset -ft dq_dt_alarm_emitlabel dq_dt_alarm_emitparams
fi
