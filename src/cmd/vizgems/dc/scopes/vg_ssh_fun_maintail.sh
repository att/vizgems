typeset -A tail tail_sizes

function vg_ssh_fun_maintail_init {
    return 0
}

function vg_ssh_fun_maintail_term {
    typeset -A sizes
    typeset k j siz d

    d=${SSHDEST##*'@'}
    d=${d//[!a-zA-Z0-9.]/_}
    for k in $VG_DSCOPESDIR/remotefiles/tail/common/*; do
        [[ ! -f $k ]] && continue
        stat $k | egrep Size: | read j siz j
        sizes[${k##*/}]=(k=$k s=$siz)
    done
    for k in $VG_DSCOPESDIR/remotefiles/tail/$targettype/*; do
        [[ ! -f $k ]] && continue
        stat $k | egrep Size: | read j siz j
        sizes[${k##*/}]=(k=$k s=$siz)
    done

    for k in "${!tail_sizes[@]}"; do
        if [[ ${sizes[$k].k} == '' ]] then
            continue
        fi
        if [[ ${sizes[$k].s} == ${tail_sizes[$k]} ]] then
            sizes[$k].done=1
            continue
        fi
        vgssh $SSHDEST \
            "mkdir -p ./vg/tail/$d.$VG_SYSNAME; cat > ./vg/tail/$d.$VG_SYSNAME/$k.tmp; chmod +x ./vg/tail/$d.$VG_SYSNAME/$k.tmp; mv ./vg/tail/$d.$VG_SYSNAME/$k.tmp ./vg/tail/$d.$VG_SYSNAME/$k" \
        < ${sizes[$k].k} 2>> ssh.err
        sizes[$k].done=1
    done
    for k in "${!sizes[@]}"; do
        if [[ ${sizes[$k].done} == 1 ]] then
            continue
        fi
        vgssh $SSHDEST \
            "mkdir -p ./vg/tail/$d.$VG_SYSNAME; cat > ./vg/tail/$d.$VG_SYSNAME/$k.tmp; chmod +x ./vg/tail/$d.$VG_SYSNAME/$k.tmp; mv ./vg/tail/$d.$VG_SYSNAME/$k.tmp ./vg/tail/$d.$VG_SYSNAME/$k" \
        < ${sizes[$k].k} 2>> ssh.err
    done

    return 0
}

function vg_ssh_fun_maintail_add {
    typeset file=${as[file]}

    tail[$file]=(
        name=$name recn=0 errn=0 alarmn=0
    )
    typeset -A tail[$file].alarms
    return 0
}

function vg_ssh_fun_maintail_send {
    typeset file d

    d=${SSHDEST##*'@'}
    d=${d//[!a-zA-Z0-9.]/_}
    for file in "${!tail[@]}"; do
        typeset -n tailr=tail[$file]
        print -r "export TAILHOST=$VG_TARGETNAME; if [[ -x /bin/ksh ]]; then /bin/ksh ./vg/tail/$d.$VG_SYSNAME/run $file; else ./vg/tail/$d.$VG_SYSNAME/ksh.$targettype ./vg/tail/$d.$VG_SYSNAME/run $file; fi"
    done
    return 0
}

function vg_ssh_fun_maintail_receive {
    typeset line=$1
    typeset file i k v data id aid type sev tmode tech ti txt

    if [[ $line == *-INFO:* ]] then
        for i in $line; do
            [[ $i != *=* ]] && continue
            k=${i%%=*}
            v=${i#*=}
            tail_sizes[$k]=$v
        done
        return 0
    fi
    if [[ $line == *-ERR:* ]] then
        file=${line%%-ERR*}
        typeset -n tailr=tail[$file]
        if (( tailr.errn < 10 )) then
            print -u2 ${line#*ERR:}
            (( tailr.errn++ ))
        fi
        return 0
    fi
    [[ $line != *-DATA:* ]] && return 0

    file=${line%%-DATA*}
    data=:${line#*-DATA:}
    typeset -n tailr=tail[$file]

    id=${data#*:ID=}; id=${id%%:*}
    aid=${data#*:AID=}; aid=${aid%%:*}
    type=${data#*:TYPE=}; type=${type%%:*}
    sev=${data#*:SEV=}; sev=${sev%%:*}
    tmode=${data#*:TMODE=}; tmode=${tmode%%:*}
    tech=${data#*:TECH=}; tech=${tech%%:*}
    ti=${data#*:TI=}; ti=${ti%%:*}
    txt=${data#*:TXT=}
    tailr.alarms[${tailr.alarmn}]=(
        id1="$id" alarmid="$aid" type="$type" sev="$sev" tech="$tech"
        tmode="$tmode" ti=$ti txt="$txt"
    )
    (( tailr.alarmn++ ))
    return 0
}

function vg_ssh_fun_maintail_emit {
    typeset file alarmi

    for file in "${!tail[@]}"; do
        typeset -n tailr=tail[$file]
        for (( alarmi = 0; alarmi < tailr.alarmn; alarmi++ )) do
            typeset -n alarmr=tail[$file].alarms[$alarmi]
            typeset -n vref=vars._$varn
            (( varn++ ))
            vref.rt=ALARM
            vref.id1=${alarmr.id1}
            vref.alarmid=${alarmr.alarmid}
            vref.type=${alarmr.type}
            vref.sev=${alarmr.sev}
            vref.tech=${alarmr.tech}
            vref.tmode=${alarmr.tmode}
            [[ ${alarmr.ti} != '' ]] && vref.rti=${alarmr.ti}
            vref.txt=${alarmr.txt}
        done
        typeset -n vref=vars._$varn
        (( varn++ ))
        vref.rt=STAT
        vref.name=${tailr.name}
        vref.type=number
        vref.num=${tailr.alarmn}
        vref.unit=
    done
    return 0
}

function vg_ssh_fun_maintail_invsend {
    return 0
}

function vg_ssh_fun_maintail_invreceive {
    return 0
}

if [[ $SWIFTWARNLEVEL != "" ]] then
    typeset -ft vg_ssh_fun_maintail_init vg_ssh_fun_maintail_term
    typeset -ft vg_ssh_fun_maintail_add vg_ssh_fun_maintail_send
    typeset -ft vg_ssh_fun_maintail_receive vg_ssh_fun_maintail_emit
    typeset -ft vg_ssh_fun_maintail_invsend vg_ssh_fun_maintail_invreceive
fi
