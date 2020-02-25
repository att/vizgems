
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
[[ $1 == *version=3* ]] && SNMPARGS="-v 3 -t 2 -r 2 $cs $snmpip"
export SNMPIP=$snmpip

$SNMPCMD $SNMPARGS .1.3.6.1.2.1.1.1.0 | {
    mss=
    while read -r ms; do
        ms=${ms//$'\r'/}
        if [[ $ms == .+([0-9]).* ]] then
            [[ $mss != "" ]] && print -r ""
            mss=
        fi
        print -rn "$mss$ms"
        mss=" "
    done
    [[ $mss != "" ]] && print -r ""
} | read -r a b c d
if [[ $d != '' ]] then
    print "node|o|$aid|si_ident|$d"
fi
$SNMPCMD $SNMPARGS .1.3.6.1.2.1.1.3.0 | read -r a b c d e
if [[ $e != '' ]] then
    print "node|o|$aid|si_uptime|$e"
fi
$SNMPCMD $SNMPARGS .1.3.6.1.2.1.1.5 | read -r a b c d
if [[ $d != '' ]] then
    print "node|o|$aid|si_sysname|$d"
fi

tools='vg_snmp_cmd_mainciscoiface vg_snmp_cmd_maindellswitchmem2'

for tool in $tools; do
    $SHELL $VG_SSCOPESDIR/current/snmp/${tool}
done
