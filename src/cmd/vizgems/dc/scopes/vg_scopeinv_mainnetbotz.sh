
export cid=$CID aid=$AID ip=$IP user=$USER pass=$PASS cs=$CS
export targettype=$TARGETTYPE scopetype=$SCOPETYPE servicelevel=$SERVICELEVEL
if [[ $ip == *:*:* ]] then
    snmpip=udp6:$ip
else
    snmpip=$ip
fi

set -o pipefail

. vg_units

export INVMODE=y INV_TARGET=$aid
export SNMPCMD=vgsnmpbulkwalk
export SNMPARGS="-v 2c -t 2 -r 2 -c $cs $snmpip"
export SNMPIP=$snmpip

tools='vg_snmp_cmd_mainnetbotzairsensor vg_snmp_cmd_mainnetbotzaudiosensor vg_snmp_cmd_mainnetbotzdewsensor vg_snmp_cmd_mainnetbotzdoorsensor vg_snmp_cmd_mainnetbotzhumisensor vg_snmp_cmd_mainnetbotztempsensor'

for tool in $tools; do
    $SHELL $VG_SSCOPESDIR/current/snmp/${tool}
done
