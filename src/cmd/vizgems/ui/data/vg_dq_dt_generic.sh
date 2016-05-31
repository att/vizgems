
dq_dt_generic_data=(
    deps='inv' rerun=''
    genericfilters=() genericfiltern=0
    files=() filen=0
    rdir=''
    alwaysreruninv=''
    filterstr=''
    label=''
)

function dq_dt_generic_init { # $1 = dt query prefix
    typeset dtstr=$1
    typeset -n dt=$dtstr
    typeset -n gfilters=dq_dt_generic_data.genericfilters
    typeset -n gfiltern=dq_dt_generic_data.genericfiltern
    typeset gfilteri
    typeset -n alwaysreruninv=dq_dt_generic_data.alwaysreruninv
    typeset -n filterstr=dq_dt_generic_data.filterstr
    typeset ifs param vs vi s1 k v fgroup found rdir ft lt lod

    export GENERICSPEC=${dt.args.spec}
    if ! vg_generic_loadspec $GENERICSPEC; then
        print -u2 SWIFT ERROR failed to load spec $GENERICSPEC
        return 1
    fi
    if [[ -f $GENERICPATH/bin/spec.sh ]] then
        . $GENERICPATH/bin/spec.sh
    fi

    ifs="$IFS"
    IFS='|'
    for param in ${spec.queryparams}; do
        typeset -n vs=vars.generic_$param
        if (( ${#vs[@]} > 0 )) && [[ ${vs[0]} != '' ]] then
            s1=
            for vi in "${vs[@]}"; do
                s1+="|$vi"
            done
            s1=${s1#\|}
            k="$param" v="${s1}"
            typeset -n gfilter=dq_dt_generic_data.genericfilters._$gfiltern
            gfilter=(t=G k="$k" v="$v")
            (( gfiltern++ ))
        fi
    done
    for fgroup in onempty always; do
        if [[ $fgroup == onempty ]] && (( gfiltern > 0 )) then
            continue
        fi
        for param in ${spec.queryparams}; do
            typeset -n vs=$dtstr.filters.$fgroup.generic_$param
            [[ $vs == '' ]] && continue
            found=n
            for (( gfilteri = 0; gfilteri < gfiltern; gfilteri++ )) do
                typeset -n gfilter=dq_dt_generic_data.genericfilters._$gfilteri
                [[ ${gfilter.k} != $param ]] && continue
                s1=${gfilter.v}
                found=y
            done
            if [[ $found == n ]] then
                typeset -n gfilter=dq_dt_generic_data.genericfilters._$gfiltern
                gfilter=(t=G k="$param" v="" f=trans)
                s1=
                (( gfiltern++ ))
            fi
            case $param in
            *)
                s1+="|$vs"
                ;;
            esac
            gfilter.v=${s1#\|}
        done
    done
    IFS="$ifs"

    rdir=${dq_main_data.rdir}
    ft=${dq_main_data.ft}
    lt=${dq_main_data.lt}
    lod=${dq_main_data.genericlod:-ymd}
    case $lod in
    ymd)
        dq_dt_generic_info_ymd $ft $lt
        ;;
    ym)
        dq_dt_generic_info_ym $ft $lt
        ;;
    y)
        dq_dt_generic_info_y $ft $lt
        ;;
    esac

    if [[ ${dt.args.reruninv} != '' ]] then
        alwaysreruninv=${dt.args.reruninv}
    elif [[ ${vars.reruninv} == ongenerics ]] then
        alwaysreruninv=y
    else
        alwaysreruninv=n
    fi

    for (( gfilteri = 0; gfilteri < gfiltern; gfilteri++ )) do
        typeset -n gfilter=dq_dt_generic_data.genericfilters._$gfilteri
        [[ ${gfilter.f} == trans ]] && continue
        filterstr+="&generic_${gfilter.k}=$(printf '%#H' "${gfilter.v}")"
    done
    filterstr=${filterstr#'&'}

    return 0
}

function dq_dt_generic_info_ymd { # $1 = ft $2 = lt
    typeset ft=$1 lt=$2
    typeset ds ct mt dir cfile
    typeset fyyyymmdd fhh lyyyymmdd lhh cyyyymmdd chh

    typeset -n filen=dq_dt_generic_data.filen
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
        cfile=$dir/all.$chh.${spec.name}
        if [[ -f $cfile ]] then
            typeset -n file=dq_dt_generic_data.files._$filen
            file="$cfile"
            (( filen++ ))
        fi
        (( ct += 60 * 60 ))
        ct=$(printf '%(%#)T' $(printf '%(%Y/%m/%d-%H:00:00)T' "#$ct"))
    done

    return 0
}

function dq_dt_generic_info_ym { # $1 = ft $2 = lt
    typeset ft=$1 lt=$2
    typeset ds ct mt dir cfile
    typeset fyyyymm fdd lyyyymm ldd cyyyymm cdd

    typeset -n filen=dq_dt_generic_data.filen
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
        cfile=$dir/all.$cdd.${spec.name}
        if [[ -f $cfile ]] then
            typeset -n file=dq_dt_generic_data.files._$filen
            file="$cfile"
            (( filen++ ))
        fi
        if [[ -f ${cfile//all.$cdd/uniq} ]] then
            dq_dt_generic_data.rdir=${cfile%/*}
        fi
        ct=$(printf '%(%#)T' "#$ct midnight tomorrow")
    done

    return 0
}

function dq_dt_generic_info_y { # $1 = ft $2 = lt
    typeset ft=$1 lt=$2
    typeset ds ct mt dir cfile
    typeset fyyyy fmm lyyyy lmm cyyyy cmm

    typeset -n filen=dq_dt_generic_data.filen
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
        cfile=$dir/all.$cmm.${spec.name}
        if [[ -f $cfile ]] then
            typeset -n file=dq_dt_generic_data.files._$filen
            file="$cfile"
            (( filen++ ))
        fi
        if [[ -f ${cfile//all.$cmm/uniq} ]] then
            dq_dt_generic_data.rdir=${cfile%/*}
        fi
        ct=$(printf '%(%#)T' $(printf '%(%Y/%m/01-00:00:00)T' "#$ct"))
        (( ct += 32 * 24 * 60 * 60 ))
        ct=$(printf '%(%#)T' $(printf '%(%Y/%m/01-00:00:00)T' "#$ct"))
    done

    return 0
}

function dq_dt_generic_run { # $1 = dt query prefix
    typeset -n dt=$1 idt=${1%.generic}.inv
    typeset -n gfiltern=dq_dt_generic_data.genericfiltern
    typeset gfilteri
    typeset -n filen=dq_dt_generic_data.filen
    typeset -n infiltern=dq_dt_inv_data.invnodefiltern
    typeset -n iefiltern=dq_dt_inv_data.invedgefiltern
    typeset -n alwaysreruninv=dq_dt_generic_data.alwaysreruninv
    typeset ft lt args argi filei

    export GENERICFILTERFILE=generic.filter GENERICFILTERSIZE=$gfiltern
    export GENERICFILEFILE=generic.files GENERICFILESIZE=$filen
    export GENERICATTRFILE=generic.attr

    rm -f $GENERICFILTERFILE $GENERICATTRFILE $GENERICFILEFILE
    rm -f generic.dds genericinv.filter

    ft=${dq_main_data.ft}
    lt=${dq_main_data.lt}
    export GENERICFT=$ft GENERICLT=$lt
    (
        print -r -- "ftime=$ft"
        print -r -- "ltime=$lt"
        for argi in "${!idt.args.@}"; do
            typeset -n arg=$argi
            [[ $argi != *.?outlevel ]] && continue
            print -r -- "${argi##*.}=$arg"
        done
        for argi in "${!dt.args.@}"; do
            typeset -n arg=$argi
            print -r -- "${argi##*.}=$arg"
        done
    ) > $GENERICATTRFILE

    for (( gfilteri = 0; gfilteri < gfiltern; gfilteri++ )) do
        typeset -n gfilter=dq_dt_generic_data.genericfilters._$gfilteri
        print -r "${gfilter.t}|${gfilter.k}|${gfilter.v}"
    done > $GENERICFILTERFILE

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
        export GENERICALWAYSRERUNINV=1
    fi

    ${spec.querydtrun}

    if (( infiltern + iefiltern == 0 )) || [[ $alwaysreruninv == y ]] then
        if [[ ! -s genericinv.filter ]] then
            print "I|_|___________" > genericinv.filter
        fi
        typeset -n idtt=dq_dt_inv_data
        idtt.appendfilterfiles+=" genericinv.filter"
        idtt.rerun=y
    fi

    return 0
}

function dq_dt_generic_emitlabel { # $1 = dt query prefix
    typeset -n dt=$1 idt=${1%.generic}.inv
    typeset -n gfiltern=dq_dt_generic_data.genericfiltern
    typeset gfilteri

    for (( gfilteri = 0; gfilteri < gfiltern; gfilteri++ )) do
        typeset -n gfilter=dq_dt_generic_data.genericfilters._$gfilteri
        dq_dt_generic_data.label+=" ${gfilter.k}:${gfilter.v}"
    done
    dq_dt_generic_data.label=${dq_dt_generic_data.label#' '}

    return 0
}

function dq_dt_generic_emitparams { # $1 = dt query prefix
    return 0
}

function dq_dt_generic_setup {
    dq_main_data.query.dtools.generic=(args=())
    typeset -n ar=dq_main_data.query.dtools.generic.args
    typeset -n qdar=querydata.dt_generic

    ar.spec=${qdar.spec}
    ar.inlevels=${qdar.inlevels}
    if [[ ${qdar.reruninv} != '' ]] then
        ar.reruninv=${qdar.reruninv}
    fi
    ar.runinparallel=${qdar.runinparallel:-n}

    return 0
}

if [[ $SWIFTWARNLEVEL != "" ]] then
    PS4='${.sh.fun}-$LINENO: '
    typeset -ft dq_dt_generic_init dq_dt_generic_run dq_dt_generic_setup
    typeset -ft dq_dt_generic_info_ymd
    typeset -ft dq_dt_generic_info_ym dq_dt_generic_info_y
    typeset -ft dq_dt_generic_emitlabel dq_dt_generic_emitparams
fi
