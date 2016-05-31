
usage=$'
[-1p1?
@(#)$Id: vg_collector (AT&T Labs Research) 2006-03-12 $
]
'$USAGE_LICENSE$'
[+NAME?vg_collector - collect information from target system]
[+DESCRIPTION?\bvg_collector\b collects information using several protocols
such as SNMP, WMI, PROC, EXEC.
it uses a spec file that is derived from the main XML schedule.
]
[999:v?increases the verbosity level. May be specified multiple times.]

run_dir job_id spec_file time interval

[+SEE ALSO?\bVizGEMS\b(1)]
'

while getopts -a vg_collector "$usage" opt; do
    case $opt in
    999) (( SWIFTWARNLEVEL++ )) ;;
    *) OPTIND=0 getopts -a vg_collector "$usage" opt '-?'; exit 1 ;;
    esac
done
shift $OPTIND-1

jdir=$1
jid=$2
jspec=$3
jts=$4
jiv=$5
jlog=$6

cd $jdir || { print -u2 vg_collector: cannot cd to dir $jdir; exit 1; }
[[ $jid != '' ]] || { print -u2 vg_collector: missing id; exit 1; }
[[ -f $jspec ]] || { print -u2 vg_collector: missing spec $jspec; exit 1; }
. ./$jspec || { print -u2 vg_collector: error reading spec $jspec; exit 1; }
[[ $jts != '' ]] || { print -u2 vg_collector: missing time stamp; exit 1; }
[[ $jiv != '' ]] || { print -u2 vg_collector: missing time interval; exit 1; }
[[ $jlog != '' ]] || { print -u2 vg_collector: missing log file name; exit 1; }

exec > $jlog 2>&1

if [[ -f debug ]] then
    PS4='${.sh.file}-${.sh.fun}-$LINENO: '
    export SWIFTWARNLEVEL=-1
    exec 2> debug.log
    set -x
fi

export VG_JOBID=$jid VG_JOBTS=$jts VG_JOBIV=$jiv

export VG_SCOPENAME=${cfg.scope.name}
export VG_SCOPEADDR=${cfg.scope.addr}
export VG_SCOPETYPE=${cfg.scope.type}
target=${cfg.target.name}
export VG_TARGETNAME=$target

set -o pipefail

autoload vg_expr_eval vg_expr_print vg_expr_calc
autoload vg_recgen_stat vg_recgen_alarm vg_recgen_alarmwstate

. vg_units

# run autoinv if needed

ispec=${cfg.inv}
if [[ $ispec != '' ]] then
    ispec=${ispec//'+'/' '}
    ispec=${ispec//@([\'\\])/'\'\1}
    eval "ispec=\$'${ispec//'%'@(??)/'\x'\1"'\$'"}'"
    (( its = $jts - ${VG_INVSECS:-86400} ))
    if [[ ! -f inv.out ]] || (( $(date -f '%#' -m inv.out) <= $its )) then
        print -r "$ispec" > inv.in
        $SHELL vg_scopeinv < inv.in > inv.out.tmp
        mv inv.out.tmp inv.out
        date=$(printf '%(%Y.%m.%d.%H.%M.%S)T' \#$VG_JOBTS) || exit 1
        file=$VG_DSCOPESDIR/outgoing/autoinv
        file+=/autoinv.$date.$VG_JOBID.$VG_SCOPENAME.xml
        cp inv.out $file.tmp && mv $file.tmp $file
    fi
fi

typeset -A prots n2is n2us pts pas
typeset -l prot
typeset -u PROT
typeset -A stats
typeset -A alarms

statflags=

# group stats by protocol, except for CALC

for (( vi = 0; ; vi++ )) do
    vistr=_$vi
    (( vi == 0 )) && vistr=''
    typeset -n vref=cfg.vars.var$vistr
    [[ $vref == '' ]] && break

    if [[
        ${vref.name} == '' || ${vref.impl} == '' || ${vref.type} == ''
    ]] then
        print -u2 vg_collector: incomplete spec for variable $vi
        continue
    fi

    vref.impl=${vref.impl//'+'/' '}
    vref.impl=${vref.impl//@([\'\\])/'\'\1}
    eval "vref.impl=\$'${vref.impl//'%'@(??)/'\x'\1"'\$'"}'"
    n2is[${vref.name}]=$vi
    n2us[${vref.name}]=${vref.unit}
    prot=${vref.impl%%:*}
    stats[$vi]=( name=${vref.name} nodata=1 )
    [[ ${vref.rep} == n ]] && stats[$vi].norep=y
    (( pts[$prot]++ ))
    (( pas[$prot] = 0 ))
    [[ $prot == calc ]] && continue

    prots[$prot]+="\n${vref.name}|${vref.type}"
    prots[$prot]+="|${vref.unit}|${vref.impl//\\/\\\\}"
done
vn=$vi


# run each protocol and save results

ai=0
for prot in "${!prots[@]}"; do
    unset protrets
    print "${prots[$prot]#\\n}" > driver.$prot
    eval protrets=$(
        $SHELL $VG_SSCOPESDIR/current/$prot/vg_${prot}_driver \
            ${cfg.target.name} ${cfg.target.addr} ${cfg.target.type} \
        < driver.$prot
    )
    if [[ $? != 0 ]] then
        print -u2 "vg_collector: failed to execute protocol $prot on $target"
    fi
    for (( pi = 0; ; pi++ )) do
        typeset -n pref=protrets._$pi
        [[ ${pref} == '' ]] && break

        if [[ ${pref.rt} == ALARM ]] then
            eval alarms[$ai]="${pref}"
            (( ai++ ))
        elif [[ ${pref.rt} == STAT ]] then
            if [[ ${pref.nodata} == 2 ]] then
                vi=${n2is[${pref.name}]}
                [[ $vi != '' ]] && stats[$vi].nodata=2
                (( pas[$prot]++ ))
                continue
            fi
            if [[ ${pref.num} == '' ]] then
                continue
            fi

            if [[ ${n2is[${pref.name}]} == '' ]] then
                (( n2is[${pref.name}] = vn++ ))
                n2us[${pref.name}]=
                (( pts[$prot]++ ))
                stats[${n2is[${pref.name}]}]=()
            fi
            vi=${n2is[${pref.name}]}
            unset stats[$vi].nodata

            if [[ ${pref.rid} != '' || ${pref.alvids} != '' ]] then
                statflags=havereals
            fi
            if [[ ${pref.rti} != '' && ${pref.rti} != $VG_JOBTS ]] then
                num=${pref.num}
                unset pref.num
                eval stats[$vi]+="${pref}"
                stats[$vi].rtis+=" ${pref.rti}"
                typeset -n numref=stats[$vi].num_${pref.rti}
                numref=$num
                statflags=havereals
            else
                unset pref.rti
                eval stats[$vi]+="${pref}"
                typeset -n numref=stats[$vi].num
            fi
            typeset -n sref=stats[$vi]
            if [[
                ${n2us[${sref.name}]} != '' && ${sref.unit} != '' &&
                ${n2us[${sref.name}]} != ${sref.unit}
            ]] then
                vg_unitconv $numref ${sref.unit} ${n2us[${sref.name}]}
                if [[ $? == 0 ]] then
                    numref=$vg_ucnum
                    sref.unit=$vg_ucunit
                fi
            fi
            (( pas[$prot]++ ))
        else
            print -u2 vg_collector: unknown record type ${pref}
        fi
    done
done


# calculate CALC stats

for (( vi = 0; ; vi++ )) do
    vistr=_$vi
    (( vi == 0 )) && vistr=''
    typeset -n vref=cfg.vars.var$vistr
    [[ $vref == '' ]] && break

    if [[
        ${vref.name} == '' || ${vref.impl} == '' || ${vref.type} == ''
    ]] then
        continue
    fi

    prot=${vref.impl%%:*}
    [[ $prot != calc ]] && continue

    impl=${vref.impl}
    impl=${impl#calc:}
    argstr=${impl#*\?}
    argstr=${argstr//'&&'/_AND_DNA_}
    ifs="$IFS"
    IFS='&'
    set -f
    set -A al -- ${argstr}
    set +f
    IFS="$ifs"
    unset as
    typeset -A as
    for ai in "${al[@]}"; do
        v=${ai#*=}
        v=${v//'+'/' '}
        v=${v//@([\'\\])/'\'\1}
        eval "v=\$'${v//'%'@(??)/'\x'\1"'\$'"}'"
        as[${ai%%=*}]=${v//_AND_DNA_/'&&'}
    done
    if [[ ${as[val]} == '' ]] then
        print -u2 vg_collector: no value expression specified for $name
        continue
    fi
    if ! vg_expr_calc stats n2is "${as[val]}"; then
        continue
    fi

    val=$vg_ecresult
    unset stats[$vi].nodata
    stats[$vi]=(
        rt=STAT name=${vref.name} type=${vref.type}
        num=${val} unit=${as[unit]}
    )
    typeset -n sref=stats[$vi]
    if [[
        ${n2us[${sref.name}]} != '' && ${sref.unit} != '' &&
        ${n2us[${sref.name}]} != ${sref.unit}
    ]] then
        vg_unitconv ${sref.num} ${sref.unit} ${n2us[${sref.name}]}
        if [[ $? == 0 ]] then
            sref.num=$vg_ucnum
            sref.unit=$vg_ucunit
        fi
    fi
    if [[ ${as[label]} != '' ]] then
        sref.label=${as[label]}
    fi
    (( pas[calc]++ ))
done


# generate stats

if [[ ${#stats[@]} != 0 ]] && ! vg_recgen_stat cfg stats "$statflags"; then
    print -u2 "vg_collector: cannot create stats records"
fi


# generate alarms

for (( ai = 0; ; ai++ )) do
    aistr=_$ai
    (( ai == 0 )) && aistr=''
    typeset -n aref=cfg.alarms.alarm$aistr
    [[ $aref == '' ]] && break

    aflag=n
    estr=
    for (( ei = 0; ; ei++ )) do
        eistr=_$ei
        (( ei == 0 )) && eistr=''
        typeset -n eref=cfg.alarms.alarm$aistr.expr$eistr
        [[ $eref == '' ]] && break

        astr=cfg.alarms.alarm$aistr
        estr=cfg.alarms.alarm$aistr.expr$eistr
        vg_expr_eval stats n2is $estr || continue

        val=$vg_eeresult
        [[ $val == 0 ]] && continue

        vg_expr_print stats n2is "$estr" || continue

        str=$vg_epresult
        vg_recgen_alarmwstate alarm cfg alarms.alarm$aistr expr$eistr " $str"
        if [[ $? != 0 ]] then
            print -u2 "vg_collector: cannot create SET alarm $ai"
            continue
        fi

        aflag=y
        break
    done
    if [[ $aflag == n ]] then
        typeset -n cref=cfg.alarms.alarm$aistr.clear
        [[ $cref == '' ]] && continue

        if [[ $estr == '' && ${aref.alarmid} != '' ]] then
            estrval=( var=${aref.alarmid} )
            estr=estrval
        fi
        vg_expr_print stats n2is "$estr" _varonly_ || continue

        str=$vg_epresult
        vg_recgen_alarmwstate clear cfg alarms.alarm$aistr clear " $str"
        if [[ $? != 0 ]] then
            print -u2 "vg_collector: cannot create CLEAR alarm $ai"
        fi
    fi
done

if [[ ${#alarms[@]} != 0 ]] && ! vg_recgen_alarm cfg alarms; then
    print -u2 "vg_collector: cannot create alarm records"
fi


# generate collection alarms

vn=${#stats[@]}
vm=0
str=""
for prot in "${!pts[@]}"; do
    PROT=$prot
    (( vm += pas[$prot] ))
    str+=" $PROT: $(( ${pts[$prot]} - ${pas[$prot]} ))/${pts[$prot]}"
done
if (( vm < vn )) then
    str=" - Missing: $(( vn - vm ))/$vn (${str#' '})"
    str+=" vars:"
    for (( vi = 0; vi < vn; vi++ )) do
        typeset -n sref=stats[$vi]
        if [[ ${sref.nodata} == 1 ]] then
            str+=" ${sref.name}"
            if (( ${#str} > 1000 )) then
                str+=" ..."
                break
            fi
        fi
    done
fi

typeset -n aref=cfg.errors
if [[ $aref != '' ]] then
    if (( vn > 0 )) then
        (( vloss = 100.0 * ((vn - vm) / (vn + 0.0)) ))
    else
        (( vloss = 0.0 ))
    fi
    typeset -A cstats
    cstats[0]=( rt=STAT name=collection type=number num=$vloss unit=% )
    typeset -A cn2is
    cn2is[collection]=0
    aflag=n
    for (( ei = 0; ; ei++ )) do
        eistr=_$ei
        (( ei == 0 )) && eistr=''
        typeset -n eref=cfg.errors.expr$eistr
        [[ $eref == '' ]] && break

        estr=cfg.errors.expr$eistr
        vg_expr_eval cstats cn2is $estr || continue

        val=$vg_eeresult
        if [[ $val == 0 ]] then
            continue
        fi

        vg_recgen_alarmwstate alarm cfg errors expr$eistr "$str"
        if [[ $? != 0 ]] then
            print -u2 "vg_collector: cannot create SET collection alarm"
            continue
        fi

        aflag=y
        break
    done
    if [[ $aflag == n ]] then
        typeset -n cref=cfg.errors.clear
        [[ $cref == '' ]] && continue

        vg_recgen_alarmwstate clear cfg errors clear
        if [[ $? != 0 ]] then
            print -u2 "vg_collector: cannot create CLEAR collection alarm"
        fi
    fi
fi
