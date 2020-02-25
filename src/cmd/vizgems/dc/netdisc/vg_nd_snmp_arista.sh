export SNMPCMD=vgsnmpbulkwalk
export SNMPARGS="-LE 0 -v 2c -t 5 -c $CS $SNMPIP"

tools='iface arp dot'
for i in $tools; do
    [[ $TOOLS != '' && " $TOOLS " != *' '${i##*/}' '* ]] && continue
    print -u2 running ${i##*/} on $NAME
    $VG_SSCOPESDIR/current/netdisc/vg_nd_snmp_cmd_arista_$i
done
