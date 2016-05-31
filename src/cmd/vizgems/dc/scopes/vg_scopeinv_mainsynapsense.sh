
export cid=$CID aid=$AID ip=$IP user=$USER pass=$PASS cs=$CS
export targettype=$TARGETTYPE scopetype=$SCOPETYPE servicelevel=$SERVICELEVEL

set -o pipefail

IFS=$'\t\t'

typeset -A lids lidname lidloc lid2pid pid2lid sids sid2lid sidtype
typeset -A sidtypemap

typeset -l typeid
typeset -u U

today=$(printf '%(%Y-%m-%d 00:00:00)T')

firstline=y
mysql -h $ip -u$user -p$pass --database synap -e \
    "select * from tbl_realtime_data where data_time_stamp >= \"$today\"" \
2> /dev/null \
| while read -r sid data time; do
    [[ $firstline == y ]] && { firstline=n; continue; }
    sids[$sid]=$sid
done

firstline=y
mysql -h $ip -u$user -p$pass --database synap -e \
    "select * from tbl_sensor_node" \
2> /dev/null \
| while read -r \
    lid pid plid name descr loc type flag j1 freq funit x y ver rev \
    j2 j3 j4 j5 bcap bdate rdays v; \
do
    [[ $firstline == y ]] && { firstline=n; continue; }
    [[ $pid == '' || $lid == '' || ( $name == '' && $loc == '' ) ]] && continue
    pid=$(printf '%x' $pid)
    lid2pid[$lid]=$pid
    pid2lid[$pid]=$lid
    lidname[$lid]="$name"
    lidloc[$lid]="${loc:-$name}"
done

firstline=y
mysql -h $ip -u$user -p$pass --database synap -e \
    "select * from tbl_sensor_network_topology" \
2> /dev/null \
| while read jid netid lid sid v; do
    [[ $firstline == y ]] && { firstline=n; continue; }
    [[ $lid == NULL || $sid == NULL ]] && continue
    sid2lid[$sid]=$lid
done

firstline=y
mysql -h $ip -u$user -p$pass --database synap -e \
    "select * from tbl_sensor" \
2> /dev/null \
| while read sid j1 type j2; do
    [[ $firstline == y ]] && { firstline=n; continue; }
    [[ $sid == '' || $type == '' ]] && continue
    sidtype[$sid]=$type
done

exec 3> synap.list.tmp

for sid in "${!sid2lid[@]}"; do
    [[ ${sids[$sid]} == '' ]] && continue
    lid=${sid2lid[$sid]}
    pid=${lid2pid[$lid]}
    type=${sidtypemap[${sidtype[$sid]}]:-${sidtype[$sid]}}
    typeid=${type//' '/_}
    label=${lidloc[$lid]}
    for i in ${typeid//_/$'\t'}; do
        U=${i:0:1}
        label+=" $U${i:1}"
    done
    kind=sensor
    unit=
    case $typeid in
    *temp)
        kind=temp
        unit=F
        ;;
    *pressure)
        kind=pres
        unit=inH2O
        ;;
    *humidity)
        kind=humi
        unit=%
        ;;
    *battery)
        kind=batt
        unit=V
        ;;
    *)
        print -u2 ERROR unknown type $typeid
        ;;
    esac
    print "node|o|$AID|si_${kind}id${pid}_$typeid|${pid}_$typeid"
    print "node|o|$AID|si_${kind}label${pid}_$typeid|$label"
    lids[$lid]=$lid
    print -u3 "sid2lid['$sid']='$lid'"
    print -u3 "sid2pid['$sid']='$pid'"
    print -u3 "sidtypes['$sid']='$type'"
    print -u3 "sidlabels['$sid']='$label'"
    print -u3 "sidkinds['$sid']='$kind'"
    print -u3 "sidunits['$sid']='$unit'"
done

for lid in "${!lidloc[@]}"; do
    [[ ${lids[$lid]} == '' ]] && continue
    pid=${lid2pid[$lid]}
    print "node|r|synap$pid|name|Synap ${lidloc[$lid]%%?(' ')'('*}"
    print "node|r|synap$pid|nodename|Synap ${lidloc[$lid]%%?(' ')'('*}"
    print "edge|r|synap$pid|r|synap$pid|mode|virtual"
    print "map|r|synap$pid|o|$AID||"
done

exec 3>&-
mv synap.list.tmp synap.list
