
typeset -i n=0
typeset -F03 s=0.0

# dellNetCpuUtil5Min
$SNMPCMD -On $SNMPARGS .1.3.6.1.4.1.6027.3.26.1.4.4.1.5 \
| while read line; do
    if [[ $line == *Gauge* ]] then
        v=${line##*': '}
        v=${v%%' '*}
        (( s += v ))
        (( n++ ))
    fi
done

if (( n > 0 )) then
    (( s = s / n ))
    print "rt=STAT name=cpu_used._total type=number num=$s unit=% label=\"CPU Used (All)\""
fi
