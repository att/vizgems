ilastats=(key=; typeset -A vars; statn=0; typeset -a stats)

function vg_ssh_fun_mainilastats_init {
    return 0
}

function vg_ssh_fun_mainilastats_term {
    return 0
}

function vg_ssh_fun_mainilastats_add {
    typeset var=${as[var]} inst=${as[inst]} key=${as[key]} dir=${as[dir]}
    typeset rival=${as[realstatinterval]}
    ilastats.dir=$dir
    ilastats.rival=$rival
    ilastats.vars[$var.$inst]=(
        name=$name
        type=$type
        var=$var
        inst=$inst
    )
    return 0
}

function vg_ssh_fun_mainilastats_run {
    typeset userhost=$1
    typeset i line rest dir pt ct

    if [[ ${ilastats.rival} != '' ]] then
        ct=$VG_JOBTS
        [[ -f ./ila_stats.pt ]] && . ./ila_stats.pt
        [[ $pt == '' ]] && pt=0
        if (( ct + 5 - pt < ${ilastats.rival} )) then
            return 0
        fi
        print "pt=$ct" > ./ila_stats.pt
    fi

    dir=${ilastats.dir:-vg}
    > got.stats

    for (( i = 0; i < 2; i++ )) do
        if vgscp $userhost:$dir/stats ila_stats.txt; then
            if vgscp got.stats $userhost:$dir/got.stats; then
                break
            fi
        else
            > ila_stats.txt
        fi
    done

    while read -r line; do
        typeset -A vals
        rest=${line##+([$' \t'])}; rest=${rest%%+([$' \t'])}
        while [[ $rest == *=* ]] do
            key=${rest%%=*}; rest=${rest#"$key"=}; key=${key//[!a-zA-Z0-9_]/_}
            if [[ ${rest:0:1} == \' ]] then
                rest=${rest#\'}
                val=${rest%%\'*}; rest=${rest##"$val"?(\')}
            elif [[ ${rest:0:1} == \" ]] then
                rest=${rest#\"}
                val=${rest%%\"*}; rest=${rest##"$val"?(\")}
            else
                val=${rest%%[$' \t']*}; rest=${rest##"$val"?([$' \t'])}
            fi
            rest=${rest##+([$' \t'])}
            [[ $key == '' || $key == [0-9]* ]] && continue
            vals[$key]=$val
        done
        ilastats.stats[${ilastats.statn}]=(
            name=${vals[name]} type=${vals[type]} rti=${vals[rti]}
            num=${vals[num]} unit=${vals[unit]} label=${vals[label]}
            rlevel=${vals[rlevel]} rid=${vals[rid]}
        )
        (( ilastats.statn++ ))
    done < ila_stats.txt

    return 0
}

function vg_ssh_fun_mainilastats_emit {
    typeset stati

    for (( stati = 0; stati < ilastats.statn; stati++ )) do
        typeset -n statr=ilastats.stats[$stati]
        typeset -n vref=vars._$varn
        (( varn++ ))
        vref.rt=STAT
        vref.name=${statr.name}
        vref.type=${statr.type}
        vref.num=${statr.num}
        vref.unit=${statr.unit}
        vref.label=${statr.label}
        [[ ${statr.rti} != '' ]] && vref.rti=${statr.rti}
        [[ ${statr.rlevel} != '' ]] && vref.rlevel=${statr.rlevel}
        [[ ${statr.rid} != '' ]] && vref.rlevel=${statr.rid}
    done
    typeset -n vref=vars._$varn
    (( varn++ ))
    vref.rt=STAT
    vref.name=stats
    vref.type=number
    vref.num=1
    vref.unit=
    vref.label=
    return 0
}

function vg_ssh_fun_mainilastats_invrun {
    return 0
}

if [[ $SWIFTWARNLEVEL != "" ]] then
    typeset -ft vg_ssh_fun_mainilastats_init vg_ssh_fun_mainilastats_term
    typeset -ft vg_ssh_fun_mainilastats_add vg_ssh_fun_mainilastats_run
    typeset -ft vg_ssh_fun_mainilastats_emit vg_ssh_fun_mainilastats_invrun
fi
