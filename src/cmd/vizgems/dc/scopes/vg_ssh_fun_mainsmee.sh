smee=()
typeset -A smeevs smeels

function vg_ssh_fun_mainsmee_init {
    smee.varn=0
    smee.state=''
    smee.spares=''
    smee.errors=0
    smee.ip=''
    return 0
}

function vg_ssh_fun_mainsmee_term {
    return 0
}

function vg_ssh_fun_mainsmee_add {
    typeset var=${as[var]} inst=${as[inst]}

    typeset -n smeer=smee._${smee.varn}
    smeer.name=$name
    smeer.unit=KB
    smeer.type=$type
    smeer.var=$var
    smeer.inst=$inst
    smee.ip=${as[addr]}
    (( smee.varn++ ))
    return 0
}

function vg_ssh_fun_mainsmee_send {
    print -r "/usr/bin/smeecli ${smee.ip} -c 'show storageArray;'"
    return 0
}

function vg_ssh_fun_mainsmee_receive {
    typeset vn sev typ txt did

    set -f
    set -A vs -- $1
    set +f
    vn=${#vs[@]}

    case $1 in
    *[A-Z]*----------*)
        smee.state=${1%%'----------'*}
        smee.state=${smee.state##+( )}
        ;;
    *'Total hot spare '*)
        smee.state=${smee.state}1
        smee.spares=${1##*' '}
        ;;
    *'In use:'*)
        if [[ ${smee.state} == @(SUMMARY1|STORAGE ARRAY1) ]] then
            v=${1#*:}; v=${v##+([ 	])}
            if [[ $v == ${smee.spares} ]] then
                sev=1 typ=ALARM txt='All hot spare drives in use'
            elif [[ $v != 0 ]] then
                sev=2 typ=ALARM txt='A hot spare drive is in use'
            else
                typ=
            fi
            if [[ $typ == ALARM ]] then
                typeset -n vref=vars._$varn
                (( varn++ ))
                vref.rt=ALARM
                vref.alarmid=disk
                vref.sev=$sev
                vref.type=$typ
                vref.tech=''
                vref.cond=''
                vref.txt="$txt"
            fi
        fi
        ;;
    *'SUMMARY'*)
        if [[ ${smee.state} == @('STANDARD VOLUMES'*|'STANDARD VIRTUAL DISKS'*) ]] then
            smee.state=${smee.state}1
        fi
        if [[ ${smee.state} == 'DRIVES'* ]] then
            smee.state=${smee.state}1
        fi
        ;;
    *'DETAILS'*)
        if [[ ${smee.state} == @('STANDARD VOLUMES'*|'STANDARD VIRTUAL DISKS'*) ]] then
            smee.state=${smee.state}2
        fi
        ;;
    *'DRIVE CHANNELS'*)
        if [[ ${smee.state} == 'DRIVES'* ]] then
            smee.state=${smee.state}2
        fi
        ;;
    *(' ')+([0-9])+(' ')*+(' ')+([0-9,.])' '@([A-Z]B)+(' ')+([0-9])+(' ')+([0-9])+(' ')+([0-9])+(' ')*Group*Drive*)
        if [[ ${smee.state} == 'STANDARD VOLUMES1' ]] then
            if [[ $1 == *(' ')+([0-9])+(' ')*+(' ')+([0-9,.])' '@([A-Z]B)+(' ')+([0-9])+(' ')+([0-9])+(' ')+([0-9])+(' ')*Group*Drive* ]] then
                set -f
                set -A res -- "${.sh.match[@]}"
                set +f
                v=${1##+(' ')}; v=${v##+([0-9])+(' ')}
                v=${v%%+(' ')+([0-9,.])' '[A-Z]B*}
                if [[ $v != Optimal ]] then
                    typeset -n vref=vars._$varn
                    (( varn++ ))
                    vref.rt=ALARM
                    vref.alarmid=lun
                    vref.sev=1
                    vref.type=ALARM
                    vref.tech=''
                    vref.cond=''
                    vref.txt="Lun ${res[12]} not optimal: $v"
                fi
            fi
        fi
        ;;
    *(' ')+([0-9])+(' ')*+(' ')+([0-9,.])' '@([A-Z]B)+(' ')*Disk*Group*)
        if [[ ${smee.state} == 'STANDARD VIRTUAL DISKS1' ]] then
            if [[ $1 == *(' ')+([0-9])+(' ')*+(' ')+([0-9,.])' '@([A-Z]B)+(' ')*Disk*Group* ]] then
                set -f
                set -A res -- "${.sh.match[@]}"
                set +f
                v=${1##+(' ')}; v=${v##+([0-9])+(' ')}
                v=${v%%+(' ')+([0-9,.])' '[A-Z]B*}
                if [[ $v != *Optimal* ]] then
                    typeset -n vref=vars._$varn
                    (( varn++ ))
                    vref.rt=ALARM
                    vref.alarmid=lun
                    vref.sev=1
                    vref.type=ALARM
                    vref.tech=''
                    vref.cond=''
                    vref.txt="Lun ${res[12]} not optimal: $v"
                fi
            fi
        fi
        ;;
    *(' ')+([0-9]),+(' ')+([0-9]),+(' ')+([0-9])+(' ')*+(' ')+([0-9,.])' '@([A-Z]B)+(' ')*@(Drive|Physical Disk)*)
        if [[ ${smee.state} == DRIVES1 ]] then
            if [[ $1 == *(' ')+([0-9]),+(' ')+([0-9]),+(' ')+([0-9])+(' ')*+(' ')+([0-9,.])' '@([A-Z]B)+(' ')*@(Drive|Physical Disk)* ]] then
                set -f
                set -A res -- "${.sh.match[@]}"
                set +f
                did=${res[2]}_${res[4]}_${res[6]}
                v=${1#*,*,}; v=${v##+(' ')+([0-9])+(' ')}
                v=${v%%+(' ')+([0-9,.])' '[A-Z]B*}
                if [[ $v != Optimal ]] then
                    typeset -n vref=vars._$varn
                    (( varn++ ))
                    vref.rt=ALARM
                    vref.alarmid=disk
                    vref.sev=1
                    vref.type=ALARM
                    vref.tech=''
                    vref.cond=''
                    vref.txt="Disk $did not optimal: $v"
                fi
            fi
        fi
        ;;
    esac
    smee.errors=$varn
    return 0
}

function vg_ssh_fun_mainsmee_emit {
    typeset smeei

    smeevs[errors._total]=${smee.errors}
    smeels[errors._total]='Errors'
    for (( smeei = 0; smeei < smee.varn; smeei++ )) do
        typeset -n smeer=smee._$smeei
        [[ ${smeevs[${smeer.var}.${smeer.inst}]} == '' ]] && continue
        typeset -n vref=vars._$varn
        (( varn++ ))
        vref.rt=STAT
        vref.name=${smeer.name}
        vref.type=${smeer.type}
        vref.num=${smeevs[${smeer.var}.${smeer.inst}]}
        vref.unit=${smeer.unit}
        vref.label=${smeels[${smeer.var}.${smeer.inst}]}
    done
    return 0
}

function vg_ssh_fun_mainsmee_invsend {
    print -r "/usr/bin/smeecli $SMEEADDR -c 'show storageArray;'"
    return 0
}

function vg_ssh_fun_mainsmee_invreceive {
    typeset vn v did mn fw

    set -f
    set -A vs -- $1
    set +f
    vn=${#vs[@]}

    case $1 in
    *'PROFILE FOR STORAGE ARRAY:'*)
        v=${1#*:}; v=${v##+([ 	])}
        v=${v%%' ('*}
        print "node|o|$aid|si_sysname|$v"
        ;;
    *[A-Z]*----------*)
        smee.state=${1%%'----------'*}
        smee.state=${smee.state##+( )}
        ;;
    *'Total hot spare '*)
        smee.state=${smee.state}1
        smee.spares=${1##*' '}
        ;;
    *'Storage array world-wide identifier (ID):'*)
        if [[ ${smee.state} == @(SUMMARY*|STORAGE ARRAY*) ]] then
            v=${1#*:}; v=${v##+([ 	])}
            print "node|o|$aid|si_sysid|$v"
        fi
        ;;
    *'Vendor:'*)
        if [[ ${smee.state} == CONTROLLERS* ]] then
            v=${1#*:}; v=${v##+([ 	])}
            [[ $v != 'Not Available' ]] && print "node|o|$aid|si_vendor|$v"
        fi
        ;;
    *'Product ID:'*)
        if [[ ${smee.state} == CONTROLLERS* ]] then
            v=${1#*:}; v=${v##+([ 	])}
            [[ $v != 'Not Available' ]] && print "node|o|$aid|si_model|$v"
        fi
        ;;
    *'SUMMARY'*)
        if [[ ${smee.state} == @('STANDARD VOLUMES'*|'STANDARD VIRTUAL DISKS'*) ]] then
            smee.state=${smee.state}1
        fi
        if [[ ${smee.state} == 'DRIVES'* ]] then
            smee.state=${smee.state}1
        fi
        ;;
    *'DETAILS'*)
        if [[ ${smee.state} == @('STANDARD VOLUMES'*|'STANDARD VIRTUAL DISKS'*) ]] then
            smee.state=${smee.state}2
        fi
        ;;
    *'DRIVE CHANNELS'*)
        if [[ ${smee.state} == 'DRIVES'* ]] then
            smee.state=${smee.state}2
        fi
        ;;
    *(' ')+([0-9])+(' ')*+(' ')+([0-9,.])' '@([A-Z]B)+(' ')+([0-9])+(' ')+([0-9])+(' ')+([0-9])+(' ')*Group*Drive*)
        if [[ ${smee.state} == 'STANDARD VOLUMES1' ]] then
            if [[ $1 == *(' ')+([0-9])+(' ')*+(' ')+([0-9,.])' '@([A-Z]B)+(' ')+([0-9])+(' ')+([0-9])+(' ')+([0-9])+(' ')*Group*Drive* ]] then
                set -f
                set -A res -- "${.sh.match[@]}"
                set +f
                print "node|o|$aid|si_lunname${res[12]}|${res[12]}"
                v="${res[5]//,/} ${res[6]}"
                print "node|o|$aid|si_lunsize${res[12]}|$v"
            fi
        fi
        ;;
    *(' ')+([0-9])+(' ')*+(' ')+([0-9,.])' '@([A-Z]B)+(' ')*Disk*Group*)
        if [[ ${smee.state} == 'STANDARD VIRTUAL DISKS1' ]] then
            if [[ $1 == *(' ')+([0-9])+(' ')*+(' ')+([0-9,.])' '@([A-Z]B)+(' ')*Disk*Group* ]] then
                set -f
                set -A res -- "${.sh.match[@]}"
                set +f
                print "node|o|$aid|si_lunname${res[2]}|${res[2]}"
                v="${res[5]//,/} ${res[6]}"
                print "node|o|$aid|si_lunsize${res[2]}|$v"
            fi
        fi
        ;;
    *(' ')+([0-9]),+(' ')+([0-9]),+(' ')+([0-9])+(' ')*+(' ')+([0-9,.])' '@([A-Z]B)+(' ')*@(Drive|Physical Disk)*)
        if [[ ${smee.state} == DRIVES1 ]] then
            if [[ $1 == *(' ')+([0-9]),+(' ')+([0-9]),+(' ')+([0-9])+(' ')*+(' ')+([0-9,.])' '@([A-Z]B)+(' ')*@(Drive|Physical Disk)* ]] then
                set -f
                set -A res -- "${.sh.match[@]}"
                set +f
                did=${res[2]}_${res[4]}_${res[6]}
                v="${res[9]//,/} ${res[10]}"
                print "node|o|$aid|si_hwname$did|$v"
                v="${1##*bps+(' ')}"
                mn=${v%%' '*}; v=${v#"$mn"}; v=${v##+(' ')}; fw=${v%%' '*}
                print "node|o|$aid|si_hwinfo$did|:FW:$fw:SN::MN:$mn"
                print "node|o|$aid|si_disk$did|$did"
            fi
        fi
        ;;
    esac
    return 0
}

if [[ $SWIFTWARNLEVEL != "" ]] then
    typeset -ft vg_ssh_fun_mainsmee_init vg_ssh_fun_mainsmee_term
    typeset -ft vg_ssh_fun_mainsmee_add vg_ssh_fun_mainsmee_send
    typeset -ft vg_ssh_fun_mainsmee_receive vg_ssh_fun_mainsmee_emit
    typeset -ft vg_ssh_fun_mainsmee_invsend vg_ssh_fun_mainsmee_invreceive
fi
