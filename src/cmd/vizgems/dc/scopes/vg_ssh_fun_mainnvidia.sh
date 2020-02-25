nvidia=()
typeset -A nvidiavs nvidiaus nvidials nvidians=(
    [nvfan_util]='NV Fan Speed'
    [nvmemory_used]='NV Mem Used'
    [nvgpu_util]='NV GPU Util'
    [nvmem_util]='NV Mem Util'
    [nvsensor_temp]='NV Temp'
    [nvsensor_power]='NV Power'
)

function vg_ssh_fun_mainnvidia_init {
    nvidia.varn=0
    return 0
}

function vg_ssh_fun_mainnvidia_term {
    return 0
}

function vg_ssh_fun_mainnvidia_add {
    typeset var=${as[var]} inst=${as[inst]}

    typeset -n nvidiar=nvidia._${nvidia.varn}
    nvidiar.name=$name
    nvidiar.unit=$unit
    nvidiar.type=$type
    nvidiar.var=$var
    nvidiar.inst=$inst
    (( nvidia.varn++ ))
    return 0
}

function vg_ssh_fun_mainnvidia_send {
    typeset cmd

    cmd="nvidia-smi --format=csv --query-gpu=index,pci.bus_id,name,fan.speed,memory.used,utilization.gpu,utilization.memory,temperature.gpu,power.draw"
    print -r "$cmd"
    return 0
}

function vg_ssh_fun_mainnvidia_receive {
    typeset line=$1
    typeset ifs vn vs id val unit

    [[ $line != [0-9]* ]] && return 0

    ifs="$IFS"
    IFS=','
    set -f
    set -A vs -- ${line//, /,}
    set +f
    id=${vs[0]}
    name="${vs[1]} ${vs[2]}"
    if [[ ${vs[3]} == [0-9]* ]] then
        val=${vs[3]%' '*} unit=${vs[3]##*' '}
        nvidiavs[nvfan_util.$id]=$val
        nvidiaus[nvfan_util.$id]=$unit
        nvidials[nvfan_util.$id]="${nvidians[nvfan_util]} ($name)"
    else
        nvidiavs[nvfan_util.$id]=nodata
    fi
    if [[ ${vs[4]} == [0-9]* ]] then
        val=${vs[4]%' '*} unit=${vs[4]##*' '}
        unit=${unit//i/}
        vg_unitconv "$val" $unit MB
        val=$vg_ucnum
        nvidiavs[nvmemory_used.$id]=$val
        nvidiaus[nvmemory_used.$id]=$unit
        nvidials[nvmemory_used.$id]="${nvidians[nvmemory_used]} ($name)"
    else
        nvidiavs[nvmemory_used.$id]=nodata
    fi
    if [[ ${vs[5]} == [0-9]* ]] then
        val=${vs[5]%' '*} unit=${vs[5]##*' '}
        nvidiavs[nvgpu_util.$id]=$val
        nvidiaus[nvgpu_util.$id]=$unit
        nvidials[nvgpu_util.$id]="${nvidians[nvgpu_util]} ($name)"
    else
        nvidiavs[nvgpu_util.$id]=nodata
    fi
    if [[ ${vs[6]} == [0-9]* ]] then
        val=${vs[6]%' '*} unit=${vs[6]##*' '}
        nvidiavs[nvmem_util.$id]=$val
        nvidiaus[nvmem_util.$id]=$unit
        nvidials[nvmem_util.$id]="${nvidians[nvmem_util]} ($name)"
    else
        nvidiavs[nvmem_util.$id]=nodata
    fi
    if [[ ${vs[7]} == [0-9]* ]] then
        val=${vs[7]%' '*} unit=${vs[7]##*' '}
        (( val = val * 1.8 + 32.0 ))
        nvidiavs[nvsensor_temp.$id]=$val
        nvidiaus[nvsensor_temp.$id]=F
        nvidials[nvsensor_temp.$id]="${nvidians[nvsensor_temp]} ($name)"
    else
        nvidiavs[nvsensor_temp.$id]=nodata
    fi
    if [[ ${vs[8]} == [0-9]* ]] then
        val=${vs[8]%' '*} unit=${vs[8]##*' '}
        nvidiavs[nvsensor_power.$id]=$val
        nvidiaus[nvsensor_power.$id]=${unit}att
        nvidials[nvsensor_power.$id]="${nvidians[nvsensor_power]} ($name)"
    else
        nvidiavs[nvsensor_power.$id]=nodata
    fi
    IFS="$ifs"

    return 0
}

function vg_ssh_fun_mainnvidia_emit {
    typeset nvidiai

    for (( nvidiai = 0; nvidiai < nvidia.varn; nvidiai++ )) do
        typeset -n nvidiar=nvidia._$nvidiai
        [[ ${nvidiavs[${nvidiar.var}.${nvidiar.inst}]} == '' ]] && continue
        typeset -n vref=vars._$varn
        (( varn++ ))
        vref.rt=STAT
        vref.name=${nvidiar.name}
        if [[ ${nvidiavs[${nvidiar.var}.${nvidiar.inst}]} == nodata ]] then
            vref.nodata=2
        else
            vref.type=${nvidiar.type}
            vref.num=${nvidiavs[${nvidiar.var}.${nvidiar.inst}]}
            vref.unit=${nvidiaus[${nvidiar.var}.${nvidiar.inst}]}
            vref.label=${nvidials[${nvidiar.var}.${nvidiar.inst}]}
        fi
    done
    return 0
}

function vg_ssh_fun_mainnvidia_invsend {
    typeset cmd

    cmd="nvidia-smi --format=csv --query-gpu=index,pci.bus_id,name,memory.total 2> /dev/null"
    print -r "$cmd"
    return 0
}

function vg_ssh_fun_mainnvidia_invreceive {
    typeset line=$1
    typeset ifs vn vs id name val unit

    [[ $line != [0-9]* ]] && return 0

    ifs="$IFS"
    IFS=','
    set -f
    set -A vs -- ${line//, /,}
    set +f
    vn=${#vs[@]}
    id=${vs[0]}
    name="${vs[1]} ${vs[2]}"
    print "node|o|$aid|si_nvidia$id|$name"
    val=${vs[3]%' '*B}
    unit=${vs[3]##*' '}
    unit=${unit//i/}
    vg_unitconv "$val" $unit MB
    val=$vg_ucnum
    if [[ $val != '' ]] then
        print "node|o|$aid|si_sz_nvmemory_used.$id|$val"
    fi
    IFS="$ifs"

    return 0
}

if [[ $SWIFTWARNLEVEL != "" ]] then
    typeset -ft vg_ssh_fun_mainnvidia_init vg_ssh_fun_mainnvidia_term
    typeset -ft vg_ssh_fun_mainnvidia_add vg_ssh_fun_mainnvidia_send
    typeset -ft vg_ssh_fun_mainnvidia_receive vg_ssh_fun_mainnvidia_emit
    typeset -ft vg_ssh_fun_mainnvidia_invsend vg_ssh_fun_mainnvidia_invreceive
fi
