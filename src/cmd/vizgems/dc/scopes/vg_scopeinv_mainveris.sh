
export cid=$CID aid=$AID ip=$IP user=$USER pass=$PASS cs=$CS
export targettype=$TARGETTYPE scopetype=$SCOPETYPE servicelevel=$SERVICELEVEL

function triml {
    typeset val=$1

    val=${val//\'/' '}
    val=${val//\"/' '}
    val=${val//'('/' ('}
    val=${val//')'/') '}
    val=${val//+(' ')/' '}
    val=${val##+(' ')}
    val=${val%%+(' ')}
    print -r -- "$val"
}

function trimi {
    typeset -l val

    for i in "$@"; do
        i=${i//[!a-zA-Z0-9]/}
        val+=_$i
    done
    val=${val//_+(_)/_}
    val=${val#_}
    val=${val%_}
    print -r -- "$val"
}

set -o pipefail

rm -f mb*.ini
(
    print "open $ip"
    print "user $user $pass"
    print "cd /mnt/main/sysconfig/modbus"
    print "prompt off"
    print "mget mb*.ini"
) | ftp -n > /dev/null 2>&1

typeset -A fid2name fid2plabel fid2psid mid2fidname
typeset -A mid2metric mid2sname mid2units
typeset -A mid2csid mid2clabel mid2rlabel mid2dlabel
typeset -A mid2psid mid2plabel

typeset -A csid2rsid rsid2dsid csid2vid rsid2vid csid2phase rsid2phase

typeset -A psid2psid

typeset -A vmap
vmap[1]=a_n vmap[2]=b_n vmap[3]=c_n
vmap[ab]=a_b
vmap[bc]=b_c vmap[cb]=b_c
vmap[ac]=c_a
vmap[abc]=l_l

typeset -A shorts
shorts['Energy']=Energy
shorts['Apparent Power']=A.Power
shorts['Power']=Power
shorts['Power Factor']=P.Factor
shorts['Current']=Current
shorts['Volts']=Volts

for file in mb-*.ini; do
    [[ $file == mb-250*.ini ]] && continue
    [[ ! -f $file ]] && continue
    [[ ! -s $file ]] && continue
    fid=${file##*-}
    fid=${fid%.ini}
    fid=${fid##+(0)}
    [[ $fid == '' ]] && fid=0
    while read -r -u5 line; do
        if [[ $line == TYPE=Veris* ]] then
            [[ $line != *=CP* && $line != *=[A-E][0-9]* ]] && break
            line=${line//CP-/CP}
            fid2name[$fid]=${ triml "${line##*=}"; }
            fid2plabel[$fid]=${ triml "P-${fid2name[$fid]}"; }
            fid2psid[$fid]=${ trimi P "${fid2name[$fid]}"; }
        elif [[ $line == POINT*NAME=* ]] then
            mid=${line#POINT}
            mid=${mid%%NAME=*}
            mid=${mid##+(0)}
            [[ $mid == '' ]] && mid=0
            mid=${fid}_${mid}
            val=${ triml "${line#*=}"; }
            case $val in
            Channel*|-|*-NL*|Max' '*|*' 'Max*|*Power' 'Demand*|*-Free*)
                continue
                ;;
            Current' 'Phase*|*Veris_Ref*|*VerisRef*|*-Spare*)
                continue
                ;;
            Ch+([0-9])-CP?(-)[0-9]*)
                fidname=${val#Ch*-CP}
                fidname=${fidname#-}
                fidname=CP${fidname%%[!0-9A-Z]*}
                mid2fidname[$mid]=$fidname
                continue
                ;;
            Ch[0-9]*)
                chid=${val#Ch}
                chid=${chid%%[!0-9]*}
                chid=${chid##+(0)}
                [[ $chid == '' ]] && chid=0
                chid=${fid}_${chid}
                case $val in
                *Energy)
                    sname=sensor_energy units=kWh metric='Energy'
                    ;;
                *Apparent' 'Power)
                    sname=sensor_apower units=kVA metric='Apparent Power'
                    ;;
                *Power)
                    sname=sensor_power units=kW metric='Power'
                    ;;
                *Factor)
                    sname=sensor_pf units=factor metric='Power Factor'
                    ;;
                *Demand)
                    val=${val%Demand}Current
                    sname=sensor_current units=Amps metric='Current'
                    ;;
                *)
                    continue
                    ;;
                esac
                val=${val%%?( )"$metric"}
                rval=${val#Ch*-}
                dval=${val#Ch*-}
                dval=${dval%%'('*}
                dval=${dval%%_*}
                clabel=${ triml "C-${fid2name[$fid]}-$val"; }
                rlabel=${ triml "R-${fid2name[$fid]}-$rval"; }
                dlabel=${ triml "D-$dval"; }
                csid=${ trimi C "${fid2name[$fid]}" "$val"; }
                rsid=${ trimi R "${fid2name[$fid]}" "$rval"; }
                dsid=${ trimi D "$dval"; }
                mid2csid[$mid]=$csid
                mid2sname[$mid]=$sname
                mid2units[$mid]=$units
                mid2metric[$mid]=$metric
                mid2clabel[$mid]=$clabel
                mid2rlabel[$mid]=$rlabel
                mid2dlabel[$mid]=$dlabel
                csid2rsid[$csid]=$rsid
                rsid2dsid[$rsid]=$dsid
                (( vmod = 1 + ((((${chid#*_} + 1) / 2) - 1) % 3) ))
                csid2phase[$csid]=${vmap[$vmod]}
                vid=${ trimi P "${fid2name[$fid]}" "${vmap[$vmod]}"; }
                csid2vid[$csid]=$vid
                vid=${ trimi P "${fid2name[$fid]}"; }
                rsid2vid[$rsid]=$vid
                ;;
            *)
                case $val in
                "3ph total KVA")
                    val='3ph total Apparent Power'
                    sname=sensor_apower units=kVA metric='Apparent Power'
                    ;;
                "KVA Phase 1")
                    val='Phase 1 Apparent Power'
                    sname=sensor_apower units=kVA metric='Apparent Power'
                    ;;
                "KVA Phase 2")
                    val='Phase 2 Apparent Power'
                    sname=sensor_apower units=kVA metric='Apparent Power'
                    ;;
                "KVA Phase 3")
                    val='Phase 3 Apparent Power'
                    sname=sensor_apower units=kVA metric='Apparent Power'
                    ;;
                "3ph Ave Current")
                    val='3ph Ave Current'
                    sname=sensor_current units=Amps metric='Current'
                    ;;
                "Current Demand Phase 1")
                    val='Phase 1 Current'
                    sname=sensor_current units=Amps metric='Current'
                    ;;
                "Current Demand Phase 2")
                    val='Phase 2 Current'
                    sname=sensor_current units=Amps metric='Current'
                    ;;
                "Current Demand Phase 3")
                    val='Phase 3 Current'
                    sname=sensor_current units=Amps metric='Current'
                    ;;
                "Current Demand Phase 4")
                    val='Phase 4 Current'
                    sname=sensor_current units=Amps metric='Current'
                    ;;
                "3ph kWh")
                    val='3ph Energy'
                    sname=sensor_energy units=kWh metric='Energy'
                    ;;
                "Frequency")
                    sname=sensor_freq units=Hz metric=''
                    ;;
                "3ph Total PF")
                    val='3ph Total Power Factor'
                    sname=sensor_pf units=factor metric='Power Factor'
                    ;;
                "PF Phase 1")
                    val='Phase 1 Power Factor'
                    sname=sensor_pf units=factor metric='Power Factor'
                    ;;
                "PF Phase 2")
                    val='Phase 2 Power Factor'
                    sname=sensor_pf units=factor metric='Power Factor'
                    ;;
                "PF Phase 3")
                    val='Phase 3 Power Factor'
                    sname=sensor_pf units=factor metric='Power Factor'
                    ;;
                "3ph Total kW")
                    val='3ph Total Power'
                    sname=sensor_power units=kW metric='Power'
                    ;;
                "kW Phase 1")
                    val='Phase 1 Power'
                    sname=sensor_power units=kW metric='Power'
                    ;;
                "kW Phase 2")
                    val='Phase 3 Power'
                    sname=sensor_power units=kW metric='Power'
                    ;;
                "kW Phase 3")
                    val='Phase 3 Power'
                    sname=sensor_power units=kW metric='Power'
                    ;;
                "Volts A-B")
                    val='A-B Volts'
                    sname=sensor_volts units=Volts metric='Volts'
                    ;;
                "Volts A-N")
                    val='A-N Volts'
                    sname=sensor_volts units=Volts metric='Volts'
                    ;;
                "Volts B-C")
                    val='B-C Volts'
                    sname=sensor_volts units=Volts metric='Volts'
                    ;;
                "Volts B-N")
                    val='B-N Volts'
                    sname=sensor_volts units=Volts metric='Volts'
                    ;;
                "Volts C-A")
                    val='C-A Volts'
                    sname=sensor_volts units=Volts metric='Volts'
                    ;;
                "Volts C-N")
                    val='C-N Volts'
                    sname=sensor_volts units=Volts metric='Volts'
                    ;;
                "Volts L-L 3ph Ave")
                    val='L-L Volts'
                    sname=sensor_volts units=Volts metric='Volts'
                    ;;
                "Volts L-N 3ph Ave")
                    val='L-N Volts'
                    sname=sensor_volts units=Volts metric='Volts'
                    ;;
                "3ph Present KW Total Demand"|Input*|Output*)
                    continue
                    ;;
                *)
                    #print -u2 ERROR unknown label $val
                    continue
                    ;;
                esac
                val=${val%%?( )"$metric"}
                plabel=${ triml "P-${fid2name[$fid]}-$val"; }
                psid=${ trimi P "${fid2name[$fid]}" "$val"; }
                mid2psid[$mid]=$psid
                mid2sname[$mid]=$sname
                mid2units[$mid]=$units
                mid2metric[$mid]=$metric
                mid2plabel[$mid]=$plabel
                ;;
            esac
        fi
    done 5< $file
done

typeset -A rsid2volts

for csid in "${!csid2phase[@]}"; do
    rsid=${csid2rsid[$csid]}
    rsid2phase[$rsid]+=${csid2phase[$csid]%_*}
done

for rsid in "${!rsid2phase[@]}"; do
    phase=${rsid2phase[$rsid]}
    phaselist=
    [[ $phase == *a* ]] && phaselist+=a
    [[ $phase == *b* ]] && phaselist+=b
    [[ $phase == *c* ]] && phaselist+=c
    rsid2phase[$rsid]=${vmap[$phaselist]}
    rsid2vid[$rsid]+=_${vmap[$phaselist]//_/}
done

for mid in "${!mid2fidname[@]}"; do
    fida=${mid%_*}
    labela=${fid2name[$fida]}
    labelb=${mid2fidname[$mid]}
    for fidb in "${!fid2name[@]}"; do
        if [[ ${fid2name[$fidb]} == $labelb ]] then
            psida=${ trimi P "${fid2name[$fida]}"; }
            psidb=${ trimi P "${fid2name[$fidb]}"; }
            psid2psid[$psidb]=$psida
        fi
    done
done

exec 3> veris.list.tmp

typeset -A seen

for mid in "${!mid2csid[@]}"; do
    print -u3 "mid2metric['$mid']='${shorts[${mid2metric[$mid]}]}'"
    print -u3 "mid2sname['$mid']='${mid2sname[$mid]}'"
    print -u3 "mid2units['$mid']='${mid2units[$mid]}'"
    print -u3 "mid2csid['$mid']='${mid2csid[$mid]}'"
    print -u3 "mid2clabel['$mid']='${mid2clabel[$mid]}'"
    print -u3 "mid2rlabel['$mid']='${mid2rlabel[$mid]}'"
    print -u3 "mid2dlabel['$mid']='${mid2dlabel[$mid]}'"

    csid=${mid2csid[$mid]}
    rsid=${csid2rsid[$csid]}
    dsid=${rsid2dsid[$rsid]}

    sname=${mid2sname[$mid]#sensor_}
    metric=${mid2metric[$csid]}
    clabel=${mid2clabel[$mid]}
    rlabel=${mid2rlabel[$mid]}
    dlabel=${mid2dlabel[$mid]}

    if [[ ${seen[$csid]} == '' ]] then
        print "node|r|${AID}_$csid|name|$clabel"
        print "node|r|${AID}_$csid|nodename|$clabel"
        print "map|r|${AID}_$csid|o|$AID||"
        print "edge|r|${AID}_$csid|r|${AID}_$rsid||"
        print -u3 "csid2rsid['$csid']='$rsid'"
        if [[ ${csid2vid[$csid]} != '' ]] then
            print -u3 "csid2vid['$csid']='${csid2vid[$csid]}'"
        fi
    fi
    seen[$csid]=1

    if [[ ${seen[$rsid]} == '' ]] then
        print "node|r|${AID}_$rsid|name|$rlabel"
        print "node|r|${AID}_$rsid|nodename|$rlabel"
        print "map|r|${AID}_$rsid|o|$AID||"
        print "edge|r|${AID}_$rsid|r|${AID}_$dsid||"
        print -u3 "rsid2dsid['$rsid']='$dsid'"
        if [[ ${rsid2vid[$rsid]} != '' ]] then
            print -u3 "rsid2vid['$rsid']='${rsid2vid[$rsid]}'"
        fi
    fi
    seen[$rsid]=1

    if [[ ${seen[$dsid]} == '' ]] then
        print "node|r|${AID}_$dsid|name|$dlabel"
        print "node|r|${AID}_$dsid|nodename|$dlabel"
        print "map|r|${AID}_$dsid|o|${AID}||"
    fi
    seen[$dsid]=1
    if [[ ${seen[$dsid:$sname]} == '' ]] then
        print "node|o|$AID|si_${sname}id${dsid}|$dsid"
        print "node|o|$AID|si_${sname}label${dsid}|$dlabel"
    fi
    seen[$dsid:$sname]=1
done

for mid in "${!mid2psid[@]}"; do
    print -u3 "mid2metric['$mid']='${mid2metric[$mid]}'"
    print -u3 "mid2sname['$mid']='${mid2sname[$mid]}'"
    print -u3 "mid2units['$mid']='${mid2units[$mid]}'"
    print -u3 "mid2psid['$mid']='${mid2psid[$mid]}'"
    print -u3 "mid2plabel['$mid']='${mid2plabel[$mid]}'"

    psid=${mid2psid[$mid]}
    print -u3 "psid2panel['$psid']='${psid%_*}'"

    sname=${mid2sname[$mid]#sensor_}
    metric=${mid2metric[$psid]}
    plabel=${mid2plabel[$mid]}

    if [[ ${seen[$psid:$sname]} == '' ]] then
        print "node|o|$AID|si_${sname}id${psid}|$psid"
        print "node|o|$AID|si_${sname}label${psid}|$plabel"
    fi
    seen[$psid:$sname]=1
done

for fid in "${!fid2name[@]}"; do
    name=${fid2name[$fid]}
    label=${fid2plabel[$fid]}
    psid=${fid2psid[$fid]}

    print -u3 "panelid['$psid']='${psid}'"
    print -u3 "panellabel['$psid']='${label}'"

    if [[ ${seen[$psid]} == '' ]] then
        print "node|r|${AID}_$psid|name|$label"
        print "node|r|${AID}_$psid|nodename|$label"
        print "map|r|${AID}_$psid|o|$AID||"
        if [[ ${psid2psid[$psid]} != '' ]] then
            print "edge|r|${AID}_$psid|r|${AID}_${psid2psid[$psid]}||"
        else
            print "edge|r|${AID}_$psid|r|${AID}_$psid|mode|virtual"
        fi
    fi
    seen[$psid]=1
done

print "node|o|$AID|nextqid|resource"

exec 3>&-
mv veris.list.tmp veris.list
