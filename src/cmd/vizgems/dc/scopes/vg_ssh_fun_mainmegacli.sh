megacli=(
    typeset -Z2 dgi di
    typeset -A sizes
)
typeset -A megaclivs megaclius

function vg_ssh_fun_mainmegacli_init {
    megacli.varn=0
    return 0
}

function vg_ssh_fun_mainmegacli_term {
    return 0
}

function vg_ssh_fun_mainmegacli_add {
    typeset var=${as[var]} inst=${as[inst]}

    typeset -n megaclir=megacli._${megacli.varn}
    megaclir.name=$name
    megaclir.unit=$unit
    megaclir.type=$type
    megaclir.var=$var
    megaclir.inst=$inst
    (( megacli.varn++ ))
    return 0
}

function vg_ssh_fun_mainmegacli_send {
    print "cat /var/log/MegaSAS.log"
    return 0
}

function vg_ssh_fun_mainmegacli_receive {
    typeset state sev temp unit

    case $1 in
    "DISK GROUP: "*)
        megacli.dgi=${1#"DISK GROUP: "}
        megacli.currname=lun
        megacli.currid=${megacli.dgi}
        ;;
    "Physical Disk: "*)
        megacli.di=${1#"Physical Disk: "}
        megacli.currname=hw
        megacli.currid=${megacli.dgi}_${megacli.di}
        ;;
    "State"*:*)
        state=${1##*'State'*:?( )}
        if [[ $state != Optimal ]] then
            sev=1
            [[ $state == Degraded ]] && sev=2
            megacli.alarms[${#megacli.alarms[@]}]=(
                type=ALARM sev=$sev tech='MegaCLI'
                txt="Lun ${megacli.currid} State is $state"
            )
        fi
        ;;
    "Firmware state"*:*)
        state=${1##*'Firmware state'*:?( )}
        if [[ $state != 'Online, Spun Up' ]] then
            sev=2
            [[ $state == Failed ]] && sev=1
            megacli.alarms[${#megacli.alarms[@]}]=(
                type=ALARM sev=$sev tech='MegaCLI'
                txt="Disk ${megacli.currid} State is $state"
            )
        fi
        ;;
    "Drive Temperature :"*)
        temp=${1##*'Drive Temperature :'?( )}
        temp=${temp#*'('}
        temp=${temp%')'}
        unit=${temp#*' '}
        temp=${temp%' '*}
        megaclius[sensor_temp.${megacli.currid}]=$unit
        megaclivs[sensor_temp.${megacli.currid}]=$temp
        ;;
    esac

    return 0
}

function vg_ssh_fun_mainmegacli_emit {
    typeset megaclii

    for (( megaclii = 0; megaclii < megacli.varn; megaclii++ )) do
        typeset -n megaclir=megacli._$megaclii
        [[ ${megaclivs[${megaclir.var}.${megaclir.inst}]} == '' ]] && continue
        typeset -n vref=vars._$varn
        (( varn++ ))
        vref.rt=STAT
        vref.name=${megaclir.name}
        vref.type=${megaclir.type}
        vref.num=${megaclivs[${megaclir.var}.${megaclir.inst}]}
        vref.unit=${megaclir.unit}
        vref.label="Temp Sensor (Disk ${megaclir.inst})"
    done
    for (( alarmi = 0; alarmi < ${#megacli.alarms[@]}; alarmi++ )) do
        typeset -n alarmr=megacli.alarms[$alarmi]
        typeset -n vref=vars._$varn
        (( varn++ ))
        vref.rt=ALARM
        vref.alarmid=${alarmr.alarmid}
        vref.type=${alarmr.type}
        vref.sev=${alarmr.sev}
        vref.tech=${alarmr.tech}
        vref.tmode=${alarmr.tmode}
        vref.txt=${alarmr.txt}
    done
    return 0
}

function vg_ssh_fun_mainmegacli_invsend {
    print "cat /var/log/MegaSAS.log"
    return 0
}

function vg_ssh_fun_mainmegacli_invreceive {
    typeset name size unit
    typeset -A map=(
        [TB]=1024.0 [GB]=1 [MB]=$(( 1 / 1024.0 ))
    )

    case $1 in
    "Exit Code"*)
        for name in "${!megacli.sizes[@]}"; do
            print "node|o|$aid|$name|${megacli.sizes[$name]} GB"
        done
        unset megacli.sizes
        typeset -A megacli.sizes
        ;;
    "Product Name: "*)
        print "node|o|$aid|si_pname|${1#"Product Name: "}"
        ;;
    "Memory: "*)
        print "node|o|$aid|si_memory|${1#"Memory: "}"
        ;;
    "Serial No: "*)
        print "node|o|$aid|si_serialnum|${1#"Serial No: "}"
        ;;
    "DISK GROUP: "*)
        for name in "${!megacli.sizes[@]}"; do
            print "node|o|$aid|$name|${megacli.sizes[$name]} GB"
        done
        unset megacli.sizes
        typeset -A megacli.sizes
        megacli.dgi=${1#"DISK GROUP: "}
        megacli.currname=lun
        megacli.currid=${megacli.dgi}
        ;;
    "Physical Disk: "*)
        megacli.di=${1#"Physical Disk: "}
        megacli.currname=hw
        megacli.currid=${megacli.dgi}_${megacli.di}
        ;;
    "Name"*:*)
        name=${1##*'Name'*:?( )}
        [[ $name == '' ]] && name=${megacli.currname}_${megacli.currid}
        print "node|o|$aid|si_${megacli.currname}name${megacli.currid}|$name"
        ;;
    "Size"*:*)
        size=${1##*'Size'*:?( )}
        unit=${size##*' '}
        size=${size%' '*}
        ((
            megacli.sizes[si_${megacli.currname}size${megacli.currid}] +=
            size * map[$unit]
        ))
        ;;
    "Inquiry Data: "*)
        name=${1#*'Inquiry Data: '}
        name=${name//+([ 	])/' '}
        print "node|o|$aid|si_${megacli.currname}name${megacli.currid}|$name"
        ;;
    "Raw Size: "*)
        size=${1#*'Raw Size: '}
        size=${size%%?( )'['*}
        print "node|o|$aid|si_${megacli.currname}size${megacli.currid}|$size"
        ;;
    esac

    return 0
}

if [[ $SWIFTWARNLEVEL != "" ]] then
    typeset -ft vg_ssh_fun_mainmegacli_init vg_ssh_fun_mainmegacli_term
    typeset -ft vg_ssh_fun_mainmegacli_add vg_ssh_fun_mainmegacli_send
    typeset -ft vg_ssh_fun_mainmegacli_receive vg_ssh_fun_mainmegacli_emit
    typeset -ft vg_ssh_fun_mainmegacli_invsend vg_ssh_fun_mainmegacli_invreceive
fi
