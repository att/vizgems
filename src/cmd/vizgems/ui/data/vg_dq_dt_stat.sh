
dq_dt_stat_data=(
    deps='inv' rerun=''
    statfilters=() statfiltern=0
    estats=n astats=n
    files=() filen=0 xlabels=() xlabeln=0
    fframe=0 lframe=0 projft=0 projlt=0
    lod=''
    alwaysreruninv= statgroup='' runinparallel=n
    groupmode=0 projmode=0
    filterstr='' label=''
)

function dq_dt_stat_init { # $1 = dt query prefix
    typeset dtstr=$1
    typeset -n dt=$dtstr
    typeset -n sfilters=dq_dt_stat_data.statfilters
    typeset -n sfiltern=dq_dt_stat_data.statfiltern
    typeset sfilteri
    typeset -n lod=dq_dt_stat_data.lod
    typeset -n alwaysreruninv=dq_dt_stat_data.alwaysreruninv
    typeset -n statgroup=dq_dt_stat_data.statgroup
    typeset -n filterstr=dq_dt_stat_data.filterstr
    typeset ifs param vs vi s1 k v fgroup found rdir ft lt

    ifs="$IFS"
    IFS='|'
    for param in key label; do
        typeset -n vs=vars.stat_$param
        if (( ${#vs[@]} > 0 )) && [[ ${vs[0]} != '' ]] then
            s1=
            for vi in "${vs[@]}"; do
                s1+="|$vi"
            done
            s1=${s1#\|}
            case $param in
            *)
                k="$param" v="${s1}"
                ;;
            esac
            typeset -n sfilter=dq_dt_stat_data.statfilters._$sfiltern
            sfilter=(t=S k="$k" v="$v")
            (( sfiltern++ ))
        fi
    done
    for fgroup in onempty always; do
        if [[ $fgroup == onempty ]] && (( sfiltern > 0 )) then
            continue
        fi
        for param in key label; do
            typeset -n vs=$dtstr.filters.$fgroup.stat_$param
            [[ $vs == '' ]] && continue
            found=n
            for (( sfilteri = 0; sfilteri < sfiltern; sfilteri++ )) do
                typeset -n sfilter=dq_dt_stat_data.statfilters._$sfilteri
                [[ ${sfilter.k} != $param ]] && continue
                s1=${sfilter.v}
                found=y
            done
            if [[ $found == n ]] then
                typeset -n sfilter=dq_dt_stat_data.statfilters._$sfiltern
                sfilter=(t=S k="$param" v="" f=trans)
                s1=
                (( sfiltern++ ))
            fi
            case $param in
            *)
                s1+="|$vs"
                ;;
            esac
            sfilter.v=${s1#\|}
        done
    done
    IFS="$ifs"

    if [[ ${dt.args.statgroup} != '' ]] then
        statgroup=${dt.statgroup}
    elif [[ ${vars.statgroup} != '' ]] then
        statgroup=${vars.statgroup}
    else
        statgroup=both
    fi
    if [[ $statgroup == proj ]] then
        statgroup=curr
        dq_dt_stat_data.projmode=1
    fi
    if [[ ${dt.args.projmode} == 1 ]] then
        statgroup=curr
        dq_dt_stat_data.projmode=1
    fi

    if [[ ${dt.args.groupmode} == 1 ]] then
        dq_dt_stat_data.groupmode=1
    fi

    if [[ ${dt.args.runinparallel} == y ]] then
        dq_dt_stat_data.runinparallel=y
    else
        dq_dt_stat_data.runinparallel=n
    fi

    if [[ ${dt.args.essentialstats} == y ]] then
        dq_dt_stat_data.estats=y
    fi

    if [[ ${dt.args.alarmedstats} == y ]] then
        dq_dt_stat_data.astats=y
        dq_dt_stat_data.deps+=" alarm"
    fi

    rdir=${dq_main_data.rdir}
    ft=${dq_main_data.ft}
    lt=${dq_main_data.lt}
    if [[ ${dq_dt_stat_data.projmode} == 1 ]] then
        (( lt += (lt - ft) ))
        dq_dt_stat_data.projft=$ft
        dq_dt_stat_data.projlt=$lt
    fi
    [[ $ft == '' || $lt == '' ]] && return 1
    lod=${dq_main_data.statlod}
    case $lod in
    ymd)
        dq_dt_stat_info_ymd $ft $lt
        ;;
    ym)
        dq_dt_stat_info_ym $ft $lt
        ;;
    y)
        dq_dt_stat_info_y $ft $lt
        ;;
    esac
    if [[ ${dt.args.reruninv} != '' ]] then
        alwaysreruninv=${dt.args.reruninv}
    elif [[ ${vars.reruninv} == onstats ]] then
        alwaysreruninv=y
    else
        alwaysreruninv=n
    fi

    for (( sfilteri = 0; sfilteri < sfiltern; sfilteri++ )) do
        typeset -n sfilter=dq_dt_stat_data.statfilters._$sfilteri
        [[ ${sfilter.f} == trans ]] && continue
        filterstr+="&stat_${sfilter.k}=$(printf '%#H' "${sfilter.v}")"
    done
    filterstr=${filterstr#'&'}

    export STATMAPFILE=$rdir/stat.list
    egrep -v '^#|^$' $SWIFTDATADIR/uifiles/dataqueries/statorder.txt \
    | sort -t'|' -k2,2 -k4,4 -k3.3 > statorder.txt
    export STATORDERFILE=statorder.txt
    export STATORDERSIZE=$(wc -l < statorder.txt)

    return 0
}

function dq_dt_stat_info_ymd { # $1 = ft $2 = lt
    typeset ft=$1 lt=$2
    typeset ds ct mt dir cfile
    typeset fyyyymmdd fhh fmm lyyyymmdd lhh lmm cyyyymmdd chh cmm tzhh
    typeset -Z2 xlmm hmm
    typeset fro frph tfr ifr ofr xlfr xlevel xl mm

    typeset -n filen=dq_dt_stat_data.filen xlabeln=dq_dt_stat_data.xlabeln
    filen=0 xlabeln=0

    set -A ds -- $(printf '%(%Y/%m/%d %H %M)T' \#$ft)
    fyyyymmdd=${ds[0]}
    fhh=${ds[1]}
    fmm=${ds[2]}
    set -A ds -- $(printf '%(%Y/%m/%d %H %M)T' \#$lt)
    lyyyymmdd=${ds[0]}
    lhh=${ds[1]}
    lmm=${ds[2]}

    (( fro = fmm * 60 / STATINTERVAL ))
    (( frph = 60 * 60 / STATINTERVAL ))
    tfr=0
    (( fmm > 0 )) && (( tfr -= fro ))
    xlevel=1
    if (( lt - ft <= 3 * 60 * 60 )) then
        xlevel=2
    fi

    for (( ct = ft; ct < lt; )) do
        mt=$(printf '%(%#)T' $(printf '%(%Y/%m/%d-%H:00:00)T' "#$ct"))
        set -A ds -- $(printf '%(%Y/%m/%d %H %M)T' \#$mt)
        cyyyymmdd=${ds[0]}
        chh=${ds[1]}
        cmm=${ds[2]}
        (( ifr = chh * frph ))
        (( ofr = tfr ))
        (( tfr += frph ))
        dir=$SWIFTDATADIR/data/main/$cyyyymmdd/processed/total
        cfile=$dir/all.$chh.stats.dds
        if [[ -f $cfile ]] then
            typeset -n file=dq_dt_stat_data.files._$filen
            file="$(( ofr - ifr )):$cfile"
            (( filen++ ))
        fi
        if [[ ${dq_dt_stat_data.statgroup} == both ]] then
            dir=$SWIFTDATADIR/data/main/$cyyyymmdd/processed/avg/total
            cfile=$dir/mean.$chh.stats.dds
            if [[ -f $cfile ]] then
                typeset -n file=dq_dt_stat_data.files._$filen
                file="$(( ofr - ifr )):$cfile"
                (( filen++ ))
            fi
        fi
        dir=$SWIFTDATADIR/data/main/$cyyyymmdd/processed/total
        for cfile in $dir/all.$chh.*.stats.dds; do
            [[ ! -f $cfile ]] && continue
            mm=${cfile##*/}
            mm=${mm%.stats.dds}
            hmm=${mm#all.$chh.}
            (( mt + hmm * 60 >= lt )) && continue
            typeset -n file=dq_dt_stat_data.files._$filen
            file="$(( ofr - ifr )):$cfile"
            (( filen++ ))
        done
        for (( xlfr = 0; xlfr < frph; xlfr++ )) do
            xl=
            (( xlmm = xlfr * STATINTERVAL / 60 ))
            tzhh=$(TZ=$PHTZ printf '%(%H)T' \#$mt)
            if (( ofr + xlfr < 0 )) then
                :
            elif (( xlmm == 0 && tzhh % 3 == 0 )) then
                xl=$(( ofr + xlfr )):1:$tzhh:$xlmm
            elif (( xlmm == 0 && tzhh % 2 == 0 )) then
                xl=$(( ofr + xlfr )):2:$tzhh:$xlmm
            elif (( xlmm == 0 )) then
                xl=$(( ofr + xlfr )):3:$tzhh:$xlmm
            elif (( xlmm % 30 == 0 && xlevel > 1 )) then
                xl=$(( ofr + xlfr )):4:$tzhh:$xlmm
            elif (( xlmm % 15 == 0 && xlevel > 1 )) then
                xl=$(( ofr + xlfr )):5:$tzhh:$xlmm
            fi
            if [[ $xl != '' ]] then
                typeset -n xlabel=dq_dt_stat_data.xlabels._$xlabeln
                xlabel=$xl
                (( xlabeln++ ))
            fi
        done
        (( ct += 60 * 60 ))
        ct=$(printf '%(%#)T' $(printf '%(%Y/%m/%d-%H:00:00)T' "#$ct"))
    done
    (( lmm > 0 )) && (( tfr -= (frph - (lmm * 60 / STATINTERVAL)) ))

    typeset -n lframe=dq_dt_stat_data.lframe
    lframe=$tfr

    return 0
}

function dq_dt_stat_info_ym { # $1 = ft $2 = lt
    typeset ft=$1 lt=$2
    typeset ds tzds ct mt dir cfile
    typeset fyyyymm fdd fhh lyyyymm ldd lhh cyyyymm cdd chh tzyyyymm tzdd tzhh
    typeset -Z2 xlhh
    typeset fro frpd tfr ifr ofr xl

    typeset -n filen=dq_dt_stat_data.filen xlabeln=dq_dt_stat_data.xlabeln
    filen=0 xlabeln=0

    set -A ds -- $(printf '%(%Y/%m %d %H)T' \#$ft)
    fyyyymm=${ds[0]}
    fdd=${ds[1]}
    fhh=${ds[2]}
    set -A ds -- $(printf '%(%Y/%m %d %H)T' \#$lt)
    lyyyymm=${ds[0]}
    ldd=${ds[1]}
    lhh=${ds[2]}

    (( fro = fhh ))
    (( frpd = 24 ))
    tfr=0
    (( fhh > 0 )) && (( tfr -= fro ))

    for (( ct = ft; ct < lt; )) do
        mt=$(printf '%(%#)T' "#$ct midnight")
        set -A ds -- $(printf '%(%Y/%m %d %H)T' \#$mt)
        cyyyymm=${ds[0]}
        cdd=${ds[1]}
        chh=${ds[2]}
        (( ifr = (cdd - 1) * frpd ))
        (( ofr = tfr ))
        (( tfr += frpd ))
        dir=$SWIFTDATADIR/data/main/$cyyyymm/processed/total
        cfile=$dir/all.$cdd.stats.dds
        if [[ -f $cfile ]] then
            typeset -n file=dq_dt_stat_data.files._$filen
            file="$(( ofr - ifr )):$cfile"
            (( filen++ ))
        fi
        if [[ ${dq_dt_stat_data.statgroup} == both ]] then
            dir=$SWIFTDATADIR/data/main/$cyyyymm/processed/avg/total
            cfile=$dir/mean.$cdd.stats.dds
            if [[ -f $cfile ]] then
                typeset -n file=dq_dt_stat_data.files._$filen
                file="$(( ofr - ifr )):$cfile"
                (( filen++ ))
            fi
        fi
        for (( xlhh = 0; xlhh < 24; xlhh++ )) do
            xl=
            set -A tzds -- $(TZ=$PHTZ printf '%(%Y/%m %d %H)T' \#$mt)
            tzyyyymm=${tzds[0]}
            tzdd=${tzds[1]}
            tzhh=${tzds[2]}
            if (( ofr + tzhh < 0 )) then
                :
            elif (( tzhh == 0 )) then
                xl=$(( ofr + xlhh )):1:$tzyyyymm/$tzdd
            elif (( tzhh % 12 == 0 )) then
                xl=$(( ofr + xlhh )):2:$tzhh:00
            elif (( tzhh % 6 == 0 )) then
                xl=$(( ofr + xlhh )):3:$tzhh:00
            elif (( tzhh % 3 == 0 )) then
                xl=$(( ofr + xlhh )):4:$tzhh:00
            else
                xl=$(( ofr + xlhh )):5:$tzhh:00
            fi
            if [[ $xl != '' ]] then
                typeset -n xlabel=dq_dt_stat_data.xlabels._$xlabeln
                xlabel=$xl
                (( xlabeln++ ))
            fi
        done
        ct=$(printf '%(%#)T' "#$ct midnight tomorrow")
    done
    (( lhh > 0 )) && (( tfr -= (24 - lhh) ))

    typeset -n lframe=dq_dt_stat_data.lframe
    lframe=$tfr

    return 0
}

function dq_dt_stat_info_y { # $1 = ft $2 = lt
    typeset ft=$1 lt=$2
    typeset ds ct mt dir cfile
    typeset fyyyy fmm fdd fjjj lyyyy lmm ldd ljjj cyyyy cmm cdd cjjj ncjjj
    typeset tzyyyy tzmm tzdd
    typeset -Z2 xldd
    typeset fro frpm tfr ifr ofr xl

    typeset -n filen=dq_dt_stat_data.filen xlabeln=dq_dt_stat_data.xlabeln
    filen=0 xlabeln=0

    set -A ds -- $(printf '%(%Y %m %d %J)T' \#$ft)
    fyyyy=${ds[0]}
    fmm=${ds[1]}
    fdd=${ds[2]}
    fjjj=${ds[3]}
    set -A ds -- $(printf '%(%Y %m %d %J)T' \#$lt)
    lyyyy=${ds[0]}
    lmm=${ds[1]}
    ldd=${ds[2]}
    ljjj=${ds[3]}

    (( fro = fdd - 1 ))
    tfr=0
    (( fdd > 1 )) && (( tfr -= fro ))

    for (( ct = ft; ct <= lt; )) do
        mt=$(printf '%(%#)T' $(printf '%(%Y/%m/01-00:00:00)T' "#$ct"))
        set -A ds -- $(printf '%(%Y %m %d %J)T' \#$mt)
        cyyyy=${ds[0]}
        cmm=${ds[1]}
        cdd=${ds[2]}
        cjjj=${ds[3]}
        (( ifr = cjjj ))
        (( ofr = tfr ))
        dir=$SWIFTDATADIR/data/main/$cyyyy/processed/total
        cfile=$dir/all.$cmm.stats.dds
        if [[ -f $cfile ]] then
            typeset -n file=dq_dt_stat_data.files._$filen
            file="$(( ofr - ifr )):$cfile"
            (( filen++ ))
        fi
        if [[ ${dq_dt_stat_data.statgroup} == both ]] then
            dir=$SWIFTDATADIR/data/main/$cyyyy/processed/avg/total
            cfile=$dir/mean.$cmm.stats.dds
            if [[ -f $cfile ]] then
                typeset -n file=dq_dt_stat_data.files._$filen
                file="$(( ofr - ifr )):$cfile"
                (( filen++ ))
            fi
        fi
        for (( xldd = 1; xldd <= 31; xldd += 3 )) do
            xl=
            set -A tzds -- $(printf '%(%Y %m %d)T' \#$mt)
            tzyyyy=${tzds[0]}
            tzmm=${tzds[1]}
            tzdd=${tzds[2]}
            if (( ofr + tzdd < 0 )) then
                :
            elif (( tzdd == 1 )) then
                xl=$(( ofr + xldd )):1:$tzyyyy/$tzmm
            elif (( tzdd % 15 == 0 )) then
                xl=$(( ofr + xldd )):2:$tzdd
            else
                xl=$(( ofr + xldd )):3:$tzdd
            fi
            if [[ $xl != '' ]] then
                typeset -n xlabel=dq_dt_stat_data.xlabels._$xlabeln
                xlabel=$xl
                (( xlabeln++ ))
            fi
        done
        ct=$(printf '%(%#)T' $(printf '%(%Y/%m/01-00:00:00)T' "#$ct"))
        (( ct += 32 * 24 * 60 * 60 ))
        ct=$(printf '%(%#)T' $(printf '%(%Y/%m/01-00:00:00)T' "#$ct"))
        ncjjj=$(printf '%(%J)T' \#$ct)
        if (( ncjjj < cjjj )) then
            (( ncjjj += $(printf '%(%j)T' \#$(( ct - 1 ))) ))
        fi
        (( tfr += (ncjjj - cjjj) ))
    done
    (( tfr -= (ncjjj - ljjj) ))

    typeset -n lframe=dq_dt_stat_data.lframe
    lframe=$tfr

    return 0
}

function dq_dt_stat_run { # $1 = dt query prefix
    typeset -n dt=$1 idt=${1%.stat}.inv
    typeset -n sfiltern=dq_dt_stat_data.statfiltern
    typeset sfilteri
    typeset -n ffr=dq_dt_stat_data.fframe
    typeset -n lfr=dq_dt_stat_data.lframe
    typeset -n filen=dq_dt_stat_data.filen
    typeset -n infiltern=dq_dt_inv_data.invnodefiltern
    typeset -n iefiltern=dq_dt_inv_data.invedgefiltern
    typeset -n alwaysreruninv=dq_dt_stat_data.alwaysreruninv
    typeset rdir argi filei ofiles

    export STATFILTERFILE=stat.filter STATFILTERSIZE=$sfiltern
    export STATFILEFILE=stat.files STATFILESIZE=$filen
    export STATATTRFILE=stat.attr

    rm -f $STATFILTERFILE $STATFILEFILE $STATATTRFILE
    rm -f stat.dds statinv.filter stat_u.dds stat_[0-9]*.dds

    rdir=${dq_main_data.rdir}
    (
        print -r -- "fframe=$ffr"
        print -r -- "lframe=$lfr"
        for argi in "${!idt.args.@}"; do
            typeset -n arg=$argi
            [[ $argi != *.?outlevel ]] && continue
            print -r -- "${argi##*.}=$arg"
        done
        for argi in "${!dt.args.@}"; do
            typeset -n arg=$argi
            print -r -- "${argi##*.}=$arg"
        done
    ) > $STATATTRFILE

    for (( sfilteri = 0; sfilteri < sfiltern; sfilteri++ )) do
        typeset -n sfilter=dq_dt_stat_data.statfilters._$sfilteri
        print -r "${sfilter.t}|${sfilter.k}|${sfilter.v}"
    done > $STATFILTERFILE

    for (( filei = 0; filei < filen; filei++ )) do
        typeset -n file=dq_dt_stat_data.files._$filei
        print -r -- "$file"
    done > $STATFILEFILE

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
        export STATALWAYSRERUNINV=1
    fi

    export ISFILTERSIZE=0
    export ISFILTERFILE=is.filter
    if [[
        ${dq_dt_stat_data.estats} == y || ${dq_dt_stat_data.astats} == y
    ]] then
        typeset -A isdata
        typeset islv isid islvid issname isrest
        if [[ ${dq_dt_stat_data.estats} == y ]] then
            DEFAULTFILE=$SWIFTDATADIR/dpfiles/stat/parameter.txt \
            ddsfilter -osm none \
                -fso vg_dq_dt_inv2statfilter.filter.so inode.dds \
            | while read -r line; do
                islv=${line%%'|'*}
                isrest=${line##"$islv"?('|')}
                isid=${isrest%%'|'*}
                issname=${isrest##"$isid"?('|')}
                isdata[$islv:$isid]+="|$issname"
            done
        fi
        if [[ ${dq_dt_stat_data.astats} == y ]] then
            ddsfilter -osm none \
                -fso vg_dq_dt_alarm2statfilter.filter.so alarm.dds \
            | while read -r line; do
                islv=${line%%'|'*}
                isrest=${line##"$islv"?('|')}
                isid=${isrest%%'|'*}
                issname=${isrest##"$isid"?('|')}
                isdata[$islv:$isid]+="|$issname"
            done
        fi
        if (( ${#isdata[@]} > 0 )) then
            ISFILTERSIZE=${#isdata[@]}
            for islvid in "${!isdata[@]}"; do
                print "${islvid/:/'|'}${isdata[$islvid]}"
            done > $ISFILTERFILE
        else
            ISFILTERSIZE=1
            print "_|_|_" > $ISFILTERFILE
        fi
    fi

    if [[ ${dq_dt_stat_data.runinparallel} == n ]] then
        RUNINPARALLEL=0 \
        ddsconv $UIZFLAG -os vg_dq_dt_stat.schema -cso vg_dq_dt_stat.conv.so \
            $rdir/uniq.stats.dds \
        > stat.dds 3> statinv.filter
        if [[
            ${dq_dt_stat_data.projmode} == 1 ||
            ${dq_dt_stat_data.groupmode} == 1
        ]] then
            FT=${dq_dt_stat_data.projft} \
            LT=${dq_dt_stat_data.projlt} \
            FFRAME=${dq_dt_stat_data.fframe} \
            LFRAME=${dq_dt_stat_data.lframe} \
            PROJMODE=${dq_dt_stat_data.projmode} \
            GROUPMODE=${dq_dt_stat_data.groupmode} \
            ddsconv $UIZFLAG -os vg_dq_dt_stat.schema \
                -cso vg_dq_dt_stat_gnp.conv.so stat.dds \
            > stat.dds.tmp
            mv stat.dds.tmp stat.dds
        fi
    else
        RUNINPARALLEL=1 \
        ddsconv $UIZFLAG -os vg_dq_dt_stat.schema -cso vg_dq_dt_stat.conv.so \
            $rdir/uniq.stats.dds \
        > stat_u.dds 3> statinv.filter
        ofiles=
        for (( filei = 0; filei < filen; filei++ )) do
            typeset -n file=dq_dt_stat_data.files._$filei
            FILENAME=${file#*:} FILEOFFSET=${file%%:*} \
            FFRAME=$ffr LFRAME=$lfr \
            ddsconv $UIZFLAG -os vg_dq_dt_stat.schema \
                -cso vg_dq_dt_stat_one.conv.so stat_u.dds \
            > stat_${filei}.dds &
            ofiles+=" stat_${filei}.dds"
            sdpjobcntl 30
        done
        wait
        if [[
            ${dq_dt_stat_data.projmode} == 1 ||
            ${dq_dt_stat_data.groupmode} == 1
        ]] then
            ddscat $ofiles \
            | FT=${dq_dt_stat_data.projft} \
            LT=${dq_dt_stat_data.projlt} \
            FFRAME=${dq_dt_stat_data.fframe} \
            LFRAME=${dq_dt_stat_data.lframe} \
            PROJMODE=${dq_dt_stat_data.projmode} \
            GROUPMODE=${dq_dt_stat_data.groupmode} \
            ddsconv $UIZFLAG -os vg_dq_dt_stat.schema \
                -cso vg_dq_dt_stat_gnp.conv.so \
            > stat.dds.tmp
            mv stat.dds.tmp stat.dds
            rm -f $ofiles
        fi
    fi

    if (( infiltern + iefiltern == 0 )) || [[ $alwaysreruninv == y ]] then
        if [[ ! -s statinv.filter ]] then
            print "I|_|___________" > statinv.filter
        fi
        typeset -n idtt=dq_dt_inv_data
        idtt.appendfilterfiles+=" statinv.filter"
        idtt.rerun=y
    fi

    return 0
}

function dq_dt_stat_emitlabel { # $1 = dt query prefix
    typeset -n dt=$1 idt=${1%.stat}.inv
    typeset -n sfiltern=dq_dt_stat_data.statfiltern
    typeset sfilteri

    for (( sfilteri = 0; sfilteri < sfiltern; sfilteri++ )) do
        typeset -n sfilter=dq_dt_stat_data.statfilters._$sfilteri
        dq_dt_stat_data.label+=" ${sfilter.k}:${sfilter.v}"
    done
    dq_dt_stat_data.label=${dq_dt_stat_data.label#' '}

    return 0
}

function dq_dt_stat_emitparams { # $1 = dt query prefix
    return 0
}

function dq_dt_stat_setup {
    dq_main_data.query.dtools.stat=(args=())
    typeset -n sr=dq_main_data.query.dtools.stat.args
    typeset -n qdsr=querydata.dt_stat

    sr.inlevels=${qdsr.inlevels}
    if [[ ${qdsr.reruninv} != '' ]] then
        sr.reruninv=${qdsr.reruninv}
    fi
    if [[ ${qdsr.alarmedstats} != '' ]] then
        sr.alarmedstats=${qdsr.alarmedstats}
    fi
    if [[ ${qdsr.essentialstats} != '' ]] then
        sr.essentialstats=${qdsr.essentialstats}
    fi
    sr.runinparallel=${qdsr.runinparallel:-n}

    if [[ ${qdsr.filters} != '' ]] then
        typeset -C dq_main_data.query.dtools.stat.filters=qdsr.filters
    fi

    return 0
}

if [[ $SWIFTWARNLEVEL != "" ]] then
    PS4='${.sh.fun}-$LINENO: '
    typeset -ft dq_dt_stat_init dq_dt_stat_run dq_dt_stat_setup
    typeset -ft dq_dt_stat_info_ymd dq_dt_stat_info_ym dq_dt_stat_info_y
    typeset -ft dq_dt_stat_emitlabel dq_dt_stat_emitparams
fi
