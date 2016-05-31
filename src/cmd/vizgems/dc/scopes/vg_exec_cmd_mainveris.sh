
targetname=$1
targetaddr=$2
targettype=$3

IFS='|'
typeset -A names
while read -r name type unit impl; do
    names[$name]=1
done

IFS='&'
set -f
set -A al -- $impl
unset as; typeset -A as
for a in "${al[@]}"; do
    k=${a%%=*}
    v=${a#*=}
    v=${v//'+'/' '}
    v=${v//@([\'\\])/'\'\1}
    eval "v=\$'${v//'%'@(??)/'\x'\1"'\$'"}'"
    v=${v//%26/'&'}
    as[$k]=$v
done
set +f

rm -f mb*.log
(
    print "open $targetaddr"
    print "user $EXECUSER $EXECPASS"
    print "cd /mnt/main/log/modbus"
    print "prompt off"
    print "mget mb*.log"
) | ftp -n > /dev/null 2>&1

typeset -A aggrmode
aggrmode[Amps]=sum
aggrmode[Hz]=
aggrmode[Volts]=
aggrmode[factor]=avg
aggrmode[kVA]=sum
aggrmode[kW]=sum
aggrmode[kWh]=sum

fmt1="rt=%s rlevel=r rid=%s name=%s label='%s' num=%s unit='%s'\n"
fmt2="rt=%s alvids='%s' name=%s label='%s' num=%s unit='%s'\n"

typeset -A mid2metric mid2sname mid2units
typeset -A mid2csid mid2clabel mid2rlabel mid2dlabel
typeset -A mid2psid mid2plabel

typeset -A csid2rsid rsid2dsid csid2vid rsid2vid

typeset -A psid2pabel

typeset -A panelid panellabel

[[ -f ./veris.list ]] && . ./veris.list

IFS=,

for file in mb-*.log; do
    [[ $file == mb-250*.log ]] && continue
    [[ ! -f $file ]] && continue
    [[ ! -s $file ]] && continue
    fid=${file##*-}
    fid=${fid%.log}
    fid=${fid##+(0)}
    [[ $fid == '' ]] && fid=0
    typeset -A raggrv raggrn raggrl raggru daggrv daggrn daggrl daggru
    typeset -A vid2volts vid2sname vid2units
    typeset -A seen

    tail -1 $file | while read -r -u5 line; do
        set -f
        set -A vs -- $line
        set +f
        vn=${#vs[@]}
        for (( vi = 4; vi < vn; vi++ )) do
            mid=${fid}_$(( vi - 4 ))
            if [[ ${mid2csid[$mid]} != '' ]] then
                csid=${mid2csid[$mid]}
                [[ $csid == '' ]] && continue

                sname=${mid2sname[$mid]}
                units=${mid2units[$mid]}
                metric=${mid2metric[$mid]}
                clabel="${mid2clabel[$mid]} $metric"
                rlabel="${mid2rlabel[$mid]} $metric"
                dlabel="${mid2dlabel[$mid]} $metric"
                rid=${targetname}_$csid
                print -f \
                    "$fmt1" \
                STAT $rid $sname.${csid} "$clabel" ${vs[$vi]} "$units"
                rsid=${csid2rsid[$csid]}
                [[ $rsid == '' || ${aggrmode[$units]} == '' ]] && continue
                (( raggrv[$sname.$rsid] += vs[$vi] ))
                (( raggrn[$sname.$rsid]++ ))
                raggrl[$sname.$rsid]=$rlabel
                raggru[$sname.$rsid]=$units
                dsid=${rsid2dsid[$rsid]}
                [[ $dsid == '' || ${aggrmode[$units]} == '' ]] && continue
                (( daggrv[$sname.$dsid] += vs[$vi] ))
                (( daggrn[$sname.$dsid]++ ))
                daggrl[$sname.$dsid]=$dlabel
                daggru[$sname.$dsid]=$units
            else
                psid=${mid2psid[$mid]}
                [[ $psid == '' ]] && continue

                sname=${mid2sname[$mid]}
                units=${mid2units[$mid]}
                metric=${mid2metric[$mid]}
                plabel="${mid2plabel[$mid]} $metric"
                if [[ $units == Volts ]] then
                    vid2volts[$psid]=${vs[$vi]}
                    vid2sname[$psid]=$sname
                    vid2units[$psid]=$units
                fi
                rid=${targetname}_${psid%_*}
                print -f \
                    "$fmt2" \
                STAT r:$rid $sname.${psid} "$plabel" ${vs[$vi]} "$units"
            fi
        done
    done 5<&0-
    for rsnamesid in "${!raggrv[@]}"; do
        rsid=${rsnamesid#*.}
        sname=${rsnamesid%%.*}
        v=${raggrv[$rsnamesid]}
        n=${raggrn[$rsnamesid]}
        l=${raggrl[$rsnamesid]}
        u=${raggru[$rsnamesid]}
        rid=${targetname}_$rsid
        if [[ ${aggrmode[$u]} == avg ]] then
            (( v = v / n ))
        fi
        print -f \
            "$fmt1" \
        STAT $rid $sname.${rsid} "$l" $v "$u"
    done
    for dsnamesid in "${!daggrv[@]}"; do
        dsid=${dsnamesid#*.}
        sname=${dsnamesid%%.*}
        v=${daggrv[$dsnamesid]}
        n=${daggrn[$dsnamesid]}
        l=${daggrl[$dsnamesid]}
        u=${daggru[$dsnamesid]}
        rid=${targetname}_$dsid
        if [[ ${aggrmode[$u]} == avg ]] then
            (( v = v / n ))
        fi
        print -f \
            "$fmt2" \
        STAT r:$rid $sname.${dsid} "$l" $v "$u"
    done
    for mid in "${!mid2csid[@]}"; do
        csid=${mid2csid[$mid]}
        [[ ${seen[$csid]} != '' ]] && continue
        vid=${csid2vid[$csid]}
        [[ $vid == '' ]] && continue
        v=${vid2volts[$vid]}
        [[ $v == '' ]] && continue

        s=${vid2sname[$vid]}
        u=${vid2units[$vid]}
        rid=${targetname}_$csid
        clabel="${mid2clabel[$mid]} Volts"
        print -f \
            "$fmt1" \
        STAT $rid $s.${csid} "$clabel" $v "$u"
        rsid=${csid2rsid[$csid]}
        seen[$csid]=y

        [[ ${seen[$rsid]} != '' ]] && continue

        vid=${rsid2vid[$rsid]}
        [[ $vid == '' ]] && continue
        v=${vid2volts[$vid]}
        [[ $v == '' ]] && continue
        s=${vid2sname[$vid]}
        u=${vid2units[$vid]}
        rid=${targetname}_$rsid
        rlabel="${mid2rlabel[$mid]} Volts"
        print -f \
            "$fmt1" \
        STAT $rid $s.${rsid} "$rlabel" $v "$u"
        seen[$rsid]=y
    done

    unset seen
    unset vid2volts vid2sname vid2units
    unset raggrv raggrn raggrl raggru daggrv daggrn daggrl daggru
done
