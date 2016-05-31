export SNMPCMD=vgsnmpbulkwalk
export SNMPARGS="-LE 0 -v 2c -t 5 -c $CS $SNMPIP"

tools='iface arp cdp vtp trunk'
for i in $tools; do
    [[ $TOOLS != '' && " $TOOLS " != *' '${i##*/}' '* ]] && continue
    print -u2 running ${i##*/} on $NAME
    $VG_SSCOPESDIR/current/netdisc/vg_nd_snmp_cmd_ciscocat_$i
done

for i in dot; do
    [[ $TOOLS != '' && " $TOOLS " != *' '${i##*/}' '* ]] && continue
    MODE=loop $VG_SSCOPESDIR/current/netdisc/vg_nd_snmp_cmd_ciscocat_vtp \
    | while read vid; do
        print -u2 running ${i##*/} for vlan $vid on $NAME
        SNMPARGS="-LE 0 -v 2c -t 5 -c $CS@$vid $SNMPIP" \
        $VG_SSCOPESDIR/current/netdisc/vg_nd_snmp_cmd_ciscocat_$i
    done
done
