
dq_dt_alarmstat_data=(
    deps='alarm' rerun=''
    xlabels=() xlabeln=0
    fframe=0 lframe=0 ft=0 lt=0
    alwaysreruninv=
    label=''
)

function dq_dt_alarmstat_init { # $1 = dt query prefix
    export ALARMSTATMAPFILE=${dq_main_data.rdir}/stat.list
    egrep -v '^#|^$' $SWIFTDATADIR/uifiles/dataqueries/statorder.txt \
    | sort -t'|' -k2,2 -k4,4 -k3.3 > alarmstatorder.txt
    export ALARMSTATORDERFILE=alarmstatorder.txt
    export ALARMSTATORDERSIZE=$(wc -l < alarmstatorder.txt)

    return 0
}

function dq_dt_alarmstat_info_open { # $1 = ft $2 = lt
    typeset ft=$1 lt=$2
    typeset ds ct mt
    typeset fyyyymmdd fhh fmm lyyyymmdd lhh lmm cyyyymmdd chh cmm tzhh
    typeset -Z2 xlmm
    typeset fro frph tfr ifr ofr xlfr xlevel xl
    typeset -n xlabeln=dq_dt_alarmstat_data.xlabeln

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
        for (( xlfr = 0; xlfr < frph; xlfr++ )) do
            xl=
            (( xlmm = xlfr * STATINTERVAL / 60 ))
            tzhh=$(TZ=$PHTZ printf '%(%H)T' \#$mt)
            if (( ofr + xlfr < 0 )) then
                :
            elif (( xlmm == 0 )) then
                xl=$(( ofr + xlfr )):1:$tzhh:$xlmm
            elif (( xlmm % 30 == 0 && xlevel > 1 )) then
                xl=$(( ofr + xlfr )):2:$tzhh:$xlmm
            elif (( xlmm % 15 == 0 && xlevel > 1 )) then
                xl=$(( ofr + xlfr )):3:$tzhh:$xlmm
            fi
            if [[ $xl != '' ]] then
                typeset -n xlabel=dq_dt_alarmstat_data.xlabels._$xlabeln
                xlabel=$xl
                (( xlabeln++ ))
            fi
        done
        (( ct += 60 * 60 ))
        ct=$(printf '%(%#)T' $(printf '%(%Y/%m/%d-%H:00:00)T' "#$ct"))
    done
    (( lmm > 0 )) && (( tfr -= (frph - (lmm * 60 / STATINTERVAL)) ))
    (( tfr < 0 )) && tfr=0

    typeset -n lframe=dq_dt_alarmstat_data.lframe
    lframe=$tfr

    return 0
}

function dq_dt_alarmstat_info_ymd { # $1 = ft $2 = lt
    typeset ft=$1 lt=$2
    typeset ds ct mt
    typeset fyyyymmdd fhh fmm lyyyymmdd lhh lmm cyyyymmdd chh cmm tzhh
    typeset -Z2 xlmm
    typeset fro frph tfr ifr ofr xlfr xlevel xl
    typeset -n xlabeln=dq_dt_alarmstat_data.xlabeln

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
                typeset -n xlabel=dq_dt_alarmstat_data.xlabels._$xlabeln
                xlabel=$xl
                (( xlabeln++ ))
            fi
        done
        (( ct += 60 * 60 ))
        ct=$(printf '%(%#)T' $(printf '%(%Y/%m/%d-%H:00:00)T' "#$ct"))
    done
    (( lmm > 0 )) && (( tfr -= (frph - (lmm * 60 / STATINTERVAL)) ))
    (( tfr < 0 )) && tfr=0

    typeset -n lframe=dq_dt_alarmstat_data.lframe
    lframe=$tfr

    return 0
}

function dq_dt_alarmstat_info_ym { # $1 = ft $2 = lt
    typeset ft=$1 lt=$2
    typeset ds tzds ct mt
    typeset fyyyymm fdd fhh lyyyymm ldd lhh cyyyymm cdd chh tzyyyymm tzdd tzhh
    typeset -Z2 xlhh
    typeset fro frpd tfr ifr ofr xl
    typeset -n xlabeln=dq_dt_alarmstat_data.xlabeln

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
                typeset -n xlabel=dq_dt_alarmstat_data.xlabels._$xlabeln
                xlabel=$xl
                (( xlabeln++ ))
            fi
        done
        ct=$(printf '%(%#)T' "#$ct midnight tomorrow")
    done
    (( lhh > 0 )) && (( tfr -= (24 - lhh) ))
    (( tfr < 0 )) && tfr=0

    typeset -n lframe=dq_dt_alarmstat_data.lframe
    lframe=$tfr

    return 0
}

function dq_dt_alarmstat_info_y { # $1 = ft $2 = lt
    typeset ft=$1 lt=$2
    typeset ds ct mt
    typeset fyyyy fmm fdd fjjj lyyyy lmm ldd ljjj cyyyy cmm cdd cjjj ncjjj
    typeset -Z2 xldd
    typeset fro frpm tfr ifr ofr xl
    typeset -n xlabeln=dq_dt_alarmstat_data.xlabeln

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

    for (( ct = ft; ct < lt; )) do
        mt=$(printf '%(%#)T' $(printf '%(%Y/%m/01-00:00:00)T' "#$ct"))
        set -A ds -- $(printf '%(%Y %m %d %J)T' \#$mt)
        cyyyy=${ds[0]}
        cmm=${ds[1]}
        cdd=${ds[2]}
        cjjj=${ds[3]}
        (( ifr = cjjj ))
        (( ofr = tfr ))
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
                typeset -n xlabel=dq_dt_alarmstat_data.xlabels._$xlabeln
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
        (( tfr += ncjjj - cjjj ))
    done
    (( ldd > 1 )) && (( tfr -= (ncjjj - (ljjj + 1)) ))
    (( tfr < 0 )) && tfr=0

    typeset -n lframe=dq_dt_alarmstat_data.lframe
    lframe=$tfr

    return 0
}

function dq_dt_alarmstat_run { # $1 = dt query prefix
    typeset -n dt=$1 idt=${1%.stat}.inv
    typeset fileffr filelfr fileft filelt ft lt group lod mode

    group=${dq_main_data.alarmgroup}
    lod=${dq_main_data.alarmlod}
    case $group in
    open) mode=0 ;;
    all)
        case $lod in
        ymd) mode=0 ;;
        ym)  mode=1 ;;
        y)   mode=2 ;;
        esac
        ;;
    esac

    ft=${dq_main_data.ft}
    lt=${dq_main_data.lt}

    ddssort -fast -ke timeissued alarm.dds \
    | STATINTERVAL=$STATINTERVAL MODE=$mode FT=$ft LT=$lt ddsconv \
        -os vg_dq_dt_stat.schema -cso vg_dq_dt_alarmstat.conv.so \
    3> alarmstat.info \
    | ddscount -cso vg_dq_dt_alarmstat.count.so > alarmstat.dds
    read fileffr filelfr fileft filelt < alarmstat.info

    case $group in
    open)
        ft=$fileft
        lt=$filelt
        dq_dt_alarmstat_info_open $ft $lt
        ;;
    all)
        case $lod in
        ymd)
            dq_dt_alarmstat_info_ymd $ft $lt
            ;;
        ym)
            dq_dt_alarmstat_info_ym $ft $lt
            ;;
        y)
            dq_dt_alarmstat_info_y $ft $lt
            ;;
        esac
        ;;
    esac
    dq_dt_alarmstat_data.ft=$ft
    dq_dt_alarmstat_data.lt=$lt

    return 0
}

function dq_dt_alarmstat_emitlabel { # $1 = dt query prefix
    return 0
}

function dq_dt_alarmstat_emitparams { # $1 = dt query prefix
    return 0
}

function dq_dt_alarmstat_setup {
    dq_main_data.query.dtools.alarmstat=(args=())
    typeset -n asr=dq_main_data.query.dtools.alarmstat.args
    typeset -n qdasr=querydata.dt_alarmstat

    asr.inlevels=${qdasr.inlevels}
    if [[ ${qdasr.reruninv} != '' ]] then
        asr.reruninv=${qdasr.reruninv}
    fi
    asr.runinparallel=${qdasr.runinparallel:-n}

    return 0
}

if [[ $SWIFTWARNLEVEL != "" ]] then
    PS4='${.sh.fun}-$LINENO: '
    typeset -ft dq_dt_alarmstat_init dq_dt_alarmstat_run dq_dt_alarmstat_setup
    typeset -ft dq_dt_alarmstat_info_open dq_dt_alarmstat_info_ymd
    typeset -ft dq_dt_alarmstat_info_ym dq_dt_alarmstat_info_y
    typeset -ft dq_dt_alarmstat_emitlabel dq_dt_alarmstat_emitparams
fi
