
usage=$'
[-1p1?
@(#)$Id: vg_ndcollect (AT&T) 2007-03-25 $
]
[+NAME?vg_ndcollect - network discovery collector]
[+DESCRIPTION?\bvg_ndcollect\b collects the network information needed to
identify network assets and their connectivity.
]
[200:dir?specifies the directory to write data to.
]:[dir]
[201:modes?specifies the modes of collection to activate.
]:[modes]
[202:ipre?specifies an IP regular expression that specifies which of the
discovered IP addresses should be looked up on DNS.
]:[ipregexp]
[300:nf?specifies a file containing network specifications in the form
NNN.NNN.NNN.NNN/L.
The tool pings each IP address in these networks and records whether they
respond.
]:[file]
[301:af?specifies a file containing access information such as ssh accs
and snmp community strings.
The tool uses this information to collect data from these devices.
If the device matches a known type, the tool uses a specific set of
collection tools, otherwise it uses a default set.
]:[file]
[302:hf?specifies a file containing IP to hostname mappings in the format of
\b/etc/hosts\b to be used when the DNS mode is specified.
The default is to use \b/etc/hosts\b.
]:[file]
`[999:v?increases the verbosity level. May be specified multiple times.]
'

function runping { # $1 = net var
    typeset -n net=$1
    typeset base bits base2 min2 max2 min10 max10 ip10 ip16 ip n i

    base=${net.base}
    bits=${net.bits}

    print -u2 pinging network $base/$bits

    base2=$(printf '%08..2d%08..2d%08..2d%08..2d' ${base//./' '})
    base2=${base2:0:$bits}

    min2=${base2}00000000000000000000000000000000
    min2=${min2:0:32}
    max2=${base2}11111111111111111111111111111111
    max2=${max2:0:32}

    min10=$(printf '%d' 02#$min2)
    max10=$(printf '%d' 02#$max2)

    if [[ $bits != 32 ]] then
        # skip network ip and broadcast ip
        (( min10++ ))
        (( max10-- ))
    fi

    n=0
    for (( ip10 = min10; ip10 <= max10; ip10++ )) do
        ip16=$(printf '%08..16d' $ip10)
        ip=$(
            printf '%..10d.%..10d.%..10d.%..10d' \
            16#${ip16:0:2} 16#${ip16:2:2} 16#${ip16:4:2} 16#${ip16:6:2}
        )
        print -u2 pinging ip $ip
        (
            ping -q -i 0.2 -W 2 -n -c 2 $ip | egrep transmitted \
            | egrep -v 100% | read pingok
            [[ $pingok != '' ]] && print active[$ip]=Y
            # 22: ssh
            # 445: windows
            # 111: unix
            # 5600: linux
            vgport -t 1 $ip 22 80 445 111 5600 | while read -r line; do
                [[ $line == *result=N* ]] && continue
                if [[ $pingok == '' ]] then
                    print active[$ip]=Y
                    pingok=Y
                fi
                port=${line%%' '*}
                port=${port#port=}
                print ports[$ip:$port]=Y
            done
        ) > $dir/ping.$ip.tmp && mv $dir/ping.$ip.tmp $dir/ping.$ip &
        (( n++ ))
        sdpjobcntl 16
        for i in $dir/ping.+([0-9]); do
            [[ ! -f $i ]] && continue
            cat $i && rm $i
        done
    done
    wait
    for i in $dir/ping.+([0-9]); do
        [[ ! -f $i ]] && continue
        cat $i && rm $i
    done

    return 0
}

function runident { # $1 = acc var
    typeset -n acc=$1
    typeset ip type
    typeset -l idstr

    if [[ ${acc.type} != '' ]] then
        print "systype[${acc.name}]=\"${acc.type}\""
        return 0
    fi

    type=
    ip=${acc.ip}
    if [[ ${acc.cs} != '' ]] then
        print -u2 snmp ident query to ${acc.name}
        idstr=
        snmpwalk -LE 0 -v 1 -c ${acc.cs} $ip sysDescr \
        | while read -r line; do
            idstr+=" ${line//$'\r'/}"
        done
        id=
        case $idstr in
        *' ios '*|*cisco*internetwork*operating*)
            type=cisco
            ;;
        *cisco*catalyst*operating*)
            type=ciscocat
            ;;
        *foundry*)
            type=foundry
            ;;
        *powerconnect*)
            type=dell
            ;;
        *Dell*Operating*System*Version:*)
            type=dell2
            ;;
        *arista*)
            type=arista
            ;;
        "")
            type=
            ;;
        *)
            type=generic
            ;;
        esac
    fi
    if [[ $type != '' ]] then
        acc.type=$type
        print "systype[${acc.name}]=\"$type\""
        snmpwalk -LE 0 -v 1 -c ${acc.cs} $ip sysServices | read -r line
        layer=${line##*' '}
        print "layers[${acc.name}]=${layer:-0}"
        print "snmpcss[${acc.name}]=\"${acc.cs}\""
        return 0
    fi
    if [[ ${acc.user} != '' && ${acc.pass} != '' ]] then
        print -u2 ssh ident query to ${acc.name}
        idstr=
        print "uname -a" | vgssh $user@$ip | while read -r line; do
            idstr+=" ${line//$'\r'/}"
        done
        type=
        case $idstr in
        *linux*)
            type=linux.i386
            ;;
        "")
            type=
            ;;
        *)
            type=generic
            ;;
        esac
    fi
    if [[ $type != '' ]] then
        acc.type=$type
        print "systype[${acc.name}]=\"$type\""
        print "sshusers[${acc.name}]=\"${acc.user}\""
        print "sshpasss[${acc.name}]=\"${acc.pass}\""
    fi
    return 0
}

function runsnmp { # $1 = acc var
    typeset -n acc=$1
    typeset ip type

    [[ ${acc.cs} == '' ]] && continue
    ip=${acc.ip}
    type=${acc.type//./_}

    print "dev2ip[${acc.name}]=$ip"
    print "ip2dev[$ip]=\"${acc.name}\""

    print -u2 snmp to ${acc.name}
    if [[ -x $VG_SSCOPESDIR/current/netdisc/vg_nd_snmp_$type ]] then
        TOOLS=${acc.tools} NAME=${acc.name} CS=${acc.cs} SNMPIP=$ip \
        $SHELL $VG_SSCOPESDIR/current/netdisc/vg_nd_snmp_$type
    fi

    return 0
}

function runssh { # $1 = acc var
    typeset -n acc=$1
    typeset ip type

    [[ ${acc.user} == '' || ${acc.pass} == '' ]] && return 0
    ip=${acc.ip}
    type=${acc.type//./_}

    print "dev2ip[${acc.name}]=$ip"
    print "ip2dev[$ip]=${acc.name}"

    print -u2 ssh to ${acc.name}
    if [[ -x $VG_SSCOPESDIR/current/netdisc/vg_nd_ssh_$type ]] then
        TOOLS=${acc.tools} NAME=${acc.name} USER=${acc.user} PASS=$pass \
        $SHELL $VG_SSCOPESDIR/current/netdisc/vg_nd_ssh_$type
    fi

    return 0
}

function rundns {
    typeset i ip ls name
    typeset -A doneips ifips ip2dev ip2mac2 active

    for i in $dir/*.net $dir/*.acc; do
        cat $i
    done | egrep "^(ifips|ip2dev|ip2mac2|active)" > $dir/dns.1.tmp
    . $dir/dns.1.tmp
    for ip in "${ifips[@]}"; do
        doneips[$ip]=1
    done
    for ip in "${!ip2dev[@]}"; do
        doneips[$ip]=1
        print "dev2ip[${ip2dev[$ip]}]=$ip"
        print "ip2dev[$ip]=\"${ip2dev[$ip]}\""
    done
    for ip in "${!ip2mac2[@]}" "${!active[@]}"; do
        [[ ${doneips[$ip]} == 1 ]] && continue
        print -r "^${ip}[ \t]"
    done > $dir/dns.2.tmp
    egrep -f $dir/dns.2.tmp $hf | while read -r line; do
        set -A ls -- ${line}
        ls[1]=${ls[1]%%[().]*}
        print "dev2ip[${ls[1]}]=${ls[0]}"
        print "ip2dev[${ls[0]}]=\"${ls[1]}\""
        doneips[${ls[0]}]=1
    done
    for ip in "${!ip2mac2[@]}" "${!active[@]}"; do
        [[ ${doneips[$ip]} == 1 ]] && continue
        print -r "$ip"
    done > $dir/dns.3.tmp
    nslookup < $dir/dns.3.tmp 2> /dev/null \
    | egrep 'name = ' | egrep -v 'canonical name' | while read -r line; do
        name=${line##*'name = '}
        name=${name%.}
        name=${name%%.*}
        set -A ls -- ${line//./' '}
        ip=${ls[3]}.${ls[2]}.${ls[1]}.${ls[0]}
        print "dev2ip[$name]=$ip"
        print "ip2dev[$ip]=\"$name\""
    done
    rm -f $dir/dns.*.tmp

    return 0
}

function showusage {
    OPTIND=0
    getopts -a vg_ndcollect "$usage" opt '-?'
}

while getopts -a vg_ndcollect "$usage" opt; do
    case $opt in
    200) dir=$OPTARG ;;
    201) modes=$OPTARG ;;
    202) ipre=$OPTARG ;;
    300) nf=$OPTARG ;;
    301) af=$OPTARG ;;
    302) hf=$OPTARG ;;
    999) (( SWIFTWARNLEVEL++ )) ;;
    *) showusage; exit 1 ;;
    esac
done
shift $OPTIND-1

dir=${dir:-nddata}
ipre=${ipre:-'*.*.*.*'}
hf=${hf:-/etc/hosts}

if [[ ! -d $dir ]] then
    if ! mkdir -p $dir; then
        print -u2 ERROR: cannot create data directory $dir
        exit 1
    fi
fi
rm -f $dir/*.acc
rm -f $dir/*.net
rm -f $dir/*.dns
rm -f $dir/*.tmp

ifs="$IFS"
IFS='|'
nets=()
if [[ -f $nf ]] then
    while read -r spec; do
        typeset -n net=nets._${spec//['./']/_}
        base=${spec%%/*}
        bits=${spec##$base?(/)} bits=${bits:-32}
        net=( \
            base=$base bits=$bits
        )
    done < $nf
fi
accs=()
if [[ -f $af ]] then
    while read -r ip name type user pass cs tools; do
        typeset -n acc=accs._${ip//./_}
        acc=( \
            ip=$ip name=$name type=$type user=$user pass=$pass cs=$cs \
            tools=$tools
        )
    done < $af
fi
IFS="$ifs"

for neti in "${!nets.@}"; do
    [[ $neti == *.*.* ]] && continue
    [[ " $modes " == @(*' ping '*| ALL ) ]] && runping $neti > $dir/$neti.net &
    sdpjobcntl 2
done
wait

for acci in "${!accs.@}"; do
    [[ $acci == *.*.* ]] && continue
    {
        [[ " $modes " == @(*' ident '*| ALL ) ]] && runident $acci
        [[ " $modes " == @(*' snmp '*| ALL ) ]] && runsnmp $acci
        [[ " $modes " == @(*' ssh '*| ALL ) ]] && runssh $acci
    } > $dir/$acci.acc &
    sdpjobcntl 16
done
wait

[[ " $modes " == @(*' dns '*| ALL ) ]]  && rundns > $dir/names.dns

exit 0
