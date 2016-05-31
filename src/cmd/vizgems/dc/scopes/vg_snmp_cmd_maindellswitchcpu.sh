
$SNMPCMD -On $SNMPARGS .1.3.6.1.4.1.674.10895.5000.2.6132.1.1.1.1.4.9.0 \
| read line # agentSwitchCpuProcessTotalUtilization

if [[ $line == *300' Secs'* ]] then
    n=${line##*'300 Secs ('*(' ')}
    n=${n%%'%'*}
    print "rt=STAT name=cpu_used._total type=number num=$n unit=% label=\"CPU Used (All)\""
fi
