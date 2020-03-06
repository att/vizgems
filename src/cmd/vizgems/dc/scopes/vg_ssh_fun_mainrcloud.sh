typeset -A rcloud rcloud_sizes

function vg_ssh_fun_mainrcloud_init {
    return 0
}

function vg_ssh_fun_mainrcloud_term {
    typeset -A sizes
    typeset k j siz d

    d=${SSHDEST##*'@'}
    d=${d//[!a-zA-Z0-9.]/_}
    for k in $VG_DSCOPESDIR/remotefiles/rcloud/common/*; do
        [[ ! -f $k ]] && continue
        stat $k | egrep Size: | read j siz j
        sizes[${k##*/}]=(k=$k s=$siz)
    done
    for k in $VG_DSCOPESDIR/remotefiles/rcloud/$targettype/*; do
        [[ ! -f $k ]] && continue
        stat $k | egrep Size: | read j siz j
        sizes[${k##*/}]=(k=$k s=$siz)
    done

    for k in "${!rcloud_sizes[@]}"; do
        if [[ ${sizes[$k].k} == '' ]] then
            continue
        fi
        if [[ ${sizes[$k].s} == ${rcloud_sizes[$k]} ]] then
            sizes[$k].done=1
            continue
        fi
        vgssh $SSHDEST \
            "mkdir -p ./vg/rcloud/$d.$VG_SYSNAME; cat > ./vg/rcloud/$d.$VG_SYSNAME/$k.tmp; chmod +x ./vg/rcloud/$d.$VG_SYSNAME/$k.tmp; mv ./vg/rcloud/$d.$VG_SYSNAME/$k.tmp ./vg/rcloud/$d.$VG_SYSNAME/$k" \
        < ${sizes[$k].k} 2>> ssh.err
        sizes[$k].done=1
    done
    for k in "${!sizes[@]}"; do
        if [[ ${sizes[$k].done} == 1 ]] then
            continue
        fi
        vgssh $SSHDEST \
            "mkdir -p ./vg/rcloud/$d.$VG_SYSNAME; cat > ./vg/rcloud/$d.$VG_SYSNAME/$k.tmp; chmod +x ./vg/rcloud/$d.$VG_SYSNAME/$k.tmp; mv ./vg/rcloud/$d.$VG_SYSNAME/$k.tmp ./vg/rcloud/$d.$VG_SYSNAME/$k" \
        < ${sizes[$k].k} 2>> ssh.err
    done

    return 0
}

function vg_ssh_fun_mainrcloud_add {
    typeset file=${as[file]}

    rcloud[$file]=(
        name=$name recn=0 errn=0 statn=0
    )
    typeset -A rcloud[$file].stats
    return 0
}

function vg_ssh_fun_mainrcloud_send {
    typeset file d

    d=${SSHDEST##*'@'}
    d=${d//[!a-zA-Z0-9.]/_}
    for file in "${!rcloud[@]}"; do
        typeset -n rcloudr=rcloud[$file]
        print -r "cd ./vg/rcloud/$d.$VG_SYSNAME && TAILHOST=$VG_TARGETNAME ./vgksh ./run $file"
    done
    return 0
}

function vg_ssh_fun_mainrcloud_receive {
    typeset line=$1
    typeset file i k v data id aid type sev tmode tech ti txt

    if [[ $line == *-INFO:* ]] then
        for i in $line; do
            [[ $i != *=* ]] && continue
            k=${i%%=*}
            v=${i#*=}
            rcloud_sizes[$k]=$v
        done
        return 0
    fi
    if [[ $line == *-ERR:* ]] then
        file=${line%%-ERR*}
        typeset -n rcloudr=rcloud[$file]
        if (( rcloudr.errn < 10 )) then
            print -u2 ${line#*ERR:}
            (( rcloudr.errn++ ))
        fi
        return 0
    fi
    [[ $line != *-DATA:* ]] && return 0

    file=${line%%-DATA*}
    data=:${line#*-DATA:}
    typeset -n rcloudr=rcloud[$file]

    id=${data#*:ID=}; id=${id%%:*}
    key=${data#*:KEY=}; key=${key%%:*}
    ti=${data#*:TI=}; ti=${ti%%:*}
    val=${data#*:VAL=}; val=${val%%:*}
    label=${data#*:LABEL=}
    rcloudr.stats[${rcloudr.statn}]=(
        id="$id" key="$key" ti=$ti val="$val" label="$label"
    )
    (( rcloudr.statn++ ))
    return 0
}

function vg_ssh_fun_mainrcloud_emit {
    typeset file stati

    for file in "${!rcloud[@]}"; do
        typeset -n rcloudr=rcloud[$file]
        for (( stati = 0; stati < rcloudr.statn; stati++ )) do
            typeset -n statr=rcloud[$file].stats[$stati]
            typeset -n vref=vars._$varn
            (( varn++ ))
            vref.rt=STAT
            vref.name=${statr.key}
            vref.type=number
            vref.num=${statr.val}
            vref.unit=s
            vref.label=${statr.label}
            [[ ${statr.ti} != '' ]] && vref.rti=${statr.ti}
        done
        typeset -n vref=vars._$varn
        (( varn++ ))
        vref.rt=STAT
        vref.name=${rcloudr.name}
        vref.type=number
        vref.num=${rcloudr.statn}
        vref.unit=
    done
    return 0
}

function vg_ssh_fun_mainrcloud_invsend {
    return 0
}

function vg_ssh_fun_mainrcloud_invreceive {
    return 0
}

if [[ $SWIFTWARNLEVEL != "" ]] then
    typeset -ft vg_ssh_fun_mainrcloud_init vg_ssh_fun_mainrcloud_term
    typeset -ft vg_ssh_fun_mainrcloud_add vg_ssh_fun_mainrcloud_send
    typeset -ft vg_ssh_fun_mainrcloud_receive vg_ssh_fun_mainrcloud_emit
    typeset -ft vg_ssh_fun_mainrcloud_invsend vg_ssh_fun_mainrcloud_invreceive
fi
