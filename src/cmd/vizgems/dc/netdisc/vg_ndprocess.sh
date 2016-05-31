
usage=$'
[-1p1?
@(#)$Id: vg_ndprocess (AT&T) 2007-03-25 $
]
[+NAME?vg_ndprocess - network topology tool]
[+DESCRIPTION?\bvg_ndprocess\b builds a topology map of all the assets
in a network.
]
[200:dir?specifies the directory to read data from.
]:[dir]
[201:modes?specifies the modes of connectivity to activate.
]:[modes]
[202:ipre?specifies a regular expression that matches only the set of IP
addresses that should be used for generating the topology relationships.
]:[ipregexp]
[203:out?specifies the types of output to generate.
]:[types]
[301:map?specifies the set of group, site, location, and customer to map
the discovered assets to.
The argument must be a quoted, space separated string of the form:
g=g1 s=s1234 l=l1234 c=c1234 b=b1234
]:[maps]
[999:v?increases the verbosity level. May be specified multiple times.]
'

function insertnode { # $1 = id
    typeset dev ip n

    if [[ ${dev2ip[$1]} != '' || ${ip2dev[$1]} != '' ]] then
        if [[ ${dev2ip[$1]} != '' ]] then
            dev=$1
            ip=${dev2ip[$dev]}
        else
            ip=$1
            dev=${ip2dev[$ip]}
        fi
        nodes[$dev]="$dev\nsnmp=$ip"
        ip2node[$ip]=$dev
        n=0
        for ip in ${id2ip[$ip]}; do
            [[ $ip != $ipre || ${iptype[$ip]} != [1m]if ]] && continue
            ip2node[$ip]=$dev
            (( n++ ))
            if (( n < 10 )) then
                nodes[$dev]+="\nip=$ip"
            elif (( n == 10 )) then
                nodes[$dev]+="\n..."
            fi
        done
        [[ ${systype[$dev]} != '' ]] && nodeprops[$dev]=edge
    else
        ip=$1
        nodes[$ip]="$ip"
        ip2node[$ip]=$ip
    fi
}

function insertedge { # $1 = id1 $2 = id2 $3 = type
    typeset dev ip n

    if [[ ${dev2ip[$1]} != '' ]] then
        (( l1 = layers[$1] ))
    else
        (( l1 = -1 ))
    fi
    if [[ ${dev2ip[$2]} != '' ]] then
        (( l2 = layers[$2] ))
    else
        (( l2 = -1 ))
    fi
    if (( l1 > l2 )) then
        edges[$1"|"$2]+=" $3"
    elif (( l1 == l2 )) && [[ $1 < $2 ]] then
        edges[$1"|"$2]+=" $3"
    else
        edges[$2"|"$1]+=" $3"
    fi
}

function showusage {
    OPTIND=0
    getopts -a vg_ndprocess "$usage" opt '-?'
}

dir=$PWD/inv
ipre='*.*.*.*'
outtypes=ALL
mapstr='g=g1 s=s1 l=l1 c=c1 b=b1'
while getopts -a vg_ndprocess "$usage" opt; do
    case $opt in
    200) dir=$OPTARG ;;
    201) modes=$OPTARG ;;
    202) ipre=$OPTARG ;;
    203) outtypes=$OPTARG ;;
    301) mapstr=$OPTARG ;;
    999) (( SWIFTWARNLEVEL++ )) ;;
    *) showusage; exit 1 ;;
    esac
done
shift $OPTIND-1

typeset -A maps
for i in $mapstr; do
    k=${i%%=*}
    v=${i#*=}
    maps[$k]=$v
done

typeset -A layers systype snmpcss sshusers sshpasss
typeset -A active ports dev2ip ip2dev vtpid2name
typeset -A cdpdev2dev cdpport2iface
typeset -A ifaliass ifdescrs ifips ifmacs ifnames
typeset -A ifspeeds ifstates ifmasks ifistrunk
typeset -A ip2mac1 ip2mac2 ip2riface mac2riface
typeset -A mac2ip1 mac2ip2 riface2ip riface2mac

typeset -A ips iptype ip2id id2ip ip2iface
typeset -A macs mactype mac2id id2mac mac2iface
typeset -A ip2mac mac2ip dev2dev2iface net2ip net2mask
typeset -A nodes edges nodeprops ip2node

for file in $dir/*; do
    [[ ! -f $file ]] && continue
    [[ $file == *.tmp ]] && continue
    print -u2 loading $file
    . $file
done

for ip in "${!active[@]}"; do
    [[ $ip == 127.* ]] && continue
    ips[$ip]=1
    iptype[$ip]=sa
done

for mac in "${!mac2riface[@]}"; do
    [[ $mac == 00:00:00:00:00:00 ]] && continue
    macs[$mac]=1
    mactype[$mac]=sa
done

for devdev in "${!cdpdev2dev[@]}"; do
    dev=${devdev#*:}
    ipidescr=${cdpdev2dev[$devdev]}
    ip=${ipidescr%%:*}
    if [[ ${ip2dev[$ip]} == '' ]] then
        dev2ip[$dev]=$ip
        ip2dev[$ip]=$dev
    fi
    if [[ ${ips[$ip]} == '' ]] then
        ips[$ip]=1
        iptype[$ip]=sa
    fi
done

for ip in "${!ip2dev[@]}"; do
    [[ $ip == 127.* ]] && continue
    ip2id[$ip]+=" $ip"
done

for idiface in "${!ifips[@]}"; do
    ip=${ifips[$idiface]}
    [[ $ip == 127.* || $ip != $ipre ]] && continue
    if [[ ${ips[$ip]} == '' ]] then
        ips[$ip]=1
        iptype[$ip]=sa
    fi
    if [[ ${ifmasks[$idiface]} != '' ]] then
        mask=${ifmasks[$idiface]}
        set -A is -- $(printf '%08..2d %08..2d %08..2d %08..2d' ${ip//./' '})
        set -A ms -- $(printf '%08..2d %08..2d %08..2d %08..2d' ${mask//./' '})
        net=
        for (( i = 0; i < 4; i++ )) do
            net+=" $(printf '%08..2d' $(( 2#${is[$i]} & 2#${ms[$i]} )))"
        done
        net2ip[$net]+=" $ip"
        if [[ ${net2mask[$net]} != " $mask" ]] then
            net2mask[$net]+=" $mask"
        fi
    fi
    id=${idiface%%:*}
    iface=${idiface#*:}
    if [[
        ${iptype[$ip]} == mid ||
        ( ${ip2id[$ip]} != '' && ${ip2id[$ip]} != " $id" )
    ]] then
        iptype[$ip]=mid
        ip2iface[$ip]=''
        ip2id[$ip]+=" $id"
    elif [[
        ${iptype[$ip]} == mif ||
        ( ${ip2iface[$ip]} != '' && ${ip2iface[$ip]} != " $iface" )
    ]] then
        iptype[$ip]=mif
        ip2iface[$ip]+=" $iface"
    else
        iptype[$ip]=1if
        ip2iface[$ip]+=" $iface"
        ip2id[$ip]+=" $id"
    fi
done
for ip in "${!ip2iface[@]}"; do
    unset tab; typeset -A tab
    for iface in ${ip2iface[$ip]}; do
        tab[$iface]=1
    done
    ip2iface[$ip]=${!tab[@]}
done
for ip in "${!ip2id[@]}"; do
    unset tab; typeset -A tab
    for id in ${ip2id[$ip]}; do
        tab[$id]=1
    done
    ip2id[$ip]=${!tab[@]}
    for id in "${!tab[@]}"; do
        id2ip[$id]+=" $ip"
    done
done

for idiface in "${!ifmacs[@]}"; do
    mac=${ifmacs[$idiface]}
    [[ $mac == 00:00:00:00:00:00 ]] && continue
    if [[ ${macs[$mac]} == '' ]] then
        macs[$mac]=1
        mactype[$mac]=sa
    fi
    id=${idiface%%:*}
    iface=${idiface#*:}
    if [[
        ${mactype[$mac]} == mid ||
        ( ${mac2id[$mac]} != '' && ${mac2id[$mac]} != " $id" )
    ]] then
        mactype[$mac]=mid
        mac2iface[$mac]=''
        mac2id[$mac]+=" $id"
    elif [[
        ${mactype[$mac]} == mif ||
        ( ${mac2iface[$mac]} != '' && ${mac2iface[$mac]} != " $iface" )
    ]] then
        mactype[$mac]=mif
        mac2iface[$mac]+=" $iface"
    else
        mactype[$mac]=1if
        mac2iface[$mac]+=" $iface"
        mac2id[$mac]+=" $id"
    fi
done
for mac in "${!mac2iface[@]}"; do
    unset tab; typeset -A tab
    for iface in ${mac2iface[$mac]}; do
        tab[$iface]=1
    done
    mac2iface[$mac]=${!tab[@]}
done
for mac in "${!mac2id[@]}"; do
    unset tab; typeset -A tab
    for id in ${mac2id[$mac]}; do
        tab[$id]=1
    done
    mac2id[$mac]=${!tab[@]}
    for id in "${!tab[@]}"; do
        id2mac[$id]+=" $mac"
    done
done

for ip in "${!ip2mac1[@]}"; do
    for mac in ${ip2mac1[$ip]}; do
        ip2mac[$ip]+=" $mac"
        mac2ip[$mac]+=" $ip"
    done
done
for ip in "${!ip2mac2[@]}"; do
    for mac in ${ip2mac2[$ip]}; do
        [[ ${ip2mac1[$ip]} != '' ]] && continue
        [[ ${mac2ip1[$mac]} != '' ]] && continue
        ip2mac[$ip]+=" $mac"
        mac2ip[$mac]+=" $ip"
        [[ $ip == 127.* || $ip != $ipre ]] && continue
        if [[ ${ips[$ip]} == '' ]] then
            ips[$ip]=1
            iptype[$ip]=sa
        fi
    done
done
for ip in "${!ip2mac[@]}"; do
    unset tab; typeset -A tab
    for mac in ${ip2mac[$ip]}; do
        tab[$mac]=1
    done
    ip2mac[$ip]=${!tab[@]}
    if (( ${#tab[@]} == 1 )) then
        ip2mac[$ip]=${!tab[@]}
        if [[ ${mactype[${tab[0]}]} == '' ]] then
            mactype[${tab[0]}]=sa
        fi
        if [[ ${iptype[${ip}]} == '' ]] then
            iptype[${ip}]=sa
        fi
    else
        unset ip2mac[$ip]
    fi
done
for mac in "${!mac2ip[@]}"; do
    unset tab; typeset -A tab
    for ip in ${mac2ip[$mac]}; do
        tab[$ip]=1
    done
    mac2ip[$mac]=${!tab[@]}
    if (( ${#tab[@]} == 1 )) then
        mac2ip[$mac]=${!tab[@]}
        if [[ ${iptype[${tab[0]}]} == '' ]] then
            iptype[${tab[0]}]=sa
        fi
        if [[ ${mactype[${mac}]} == '' ]] then
            mactype[${mac}]=sa
        fi
    else
        unset mac2ip[$mac]
    fi
done

for dev in "${!dev2ip[@]}"; do
    insertnode $dev
done

for ip in "${!iptype[@]}"; do
    [[ $ip != $ipre ]] && continue
    [[ ${iptype[$ip]} != @([1m]if|sa) ]] && continue
    [[ ${ip2node[$ip]} != '' ]] && continue
    insertnode $ip
done

if [[ " $modes " == @(*' cdp '*| ALL ) ]] then
    for dev2dev in "${!cdpdev2dev[@]}"; do
        ipport=${cdpdev2dev[$dev2dev]}
        ip=${dev2ip[${dev2dev#*:}]}
        port=${ipport#*:}
        if [[ ${cdpport2iface[$ip:$port]} == '' ]] then
            cdpport2iface[$ip:$port]=$port
        fi
        if [[ ${dev2ip[${dev2dev#*:}]} == '' ]] then
             dev2ip[${dev2dev#*:}]=$ip
             ip2dev[$ip]=${dev2dev#*:}
        fi
        if [[
            ${cdpport2iface[$ip:$port]} == '' ||
            (${dev2ip[${dev2dev%%:*}]} == '' && ${dev2ip[${dev2dev#*:}]} == '')
        ]] then
            continue
        fi
        idiface=$ip:${cdpport2iface[$ip:$port]}
        ifistrunk[$idiface]=1
        dev2dev2iface[$dev2dev]=$idiface
    done
    for dev2dev in "${!dev2dev2iface[@]}"; do
        lidiface=${dev2dev2iface[${dev2dev#*:}:${dev2dev%:*}]}
        ridiface=${dev2dev2iface[$dev2dev]}
#        [[ $lidiface == '' || $ridiface == '' ]] && continue
        lport=${ifnames[$lidiface]:-${lidiface#*:}}
        rport=${ifnames[$ridiface]:-${ridiface#*:}}
        insertedge "${dev2dev%:*}" "${dev2dev#*:}" "CDP:$lport:$rport"
    done
fi

if [[ " $modes " == @(*' hw '*| ALL ) ]] then
    for idiface in "${!riface2mac[@]}"; do
        [[ ${ifistrunk[$idiface]} != '' ]] && continue
        id=${idiface%%:*}
        iface=${idiface#*:}
#        [[ ${riface2mac[$idiface]} == ?*' '*? ]] && continue
        for mac in ${riface2mac[$idiface]}; do
            [[ $mac == 00:00:00:00:00:00 ]] && continue
#[[ ${mac2riface[$mac]} == ?*' '*? ]] && continue
            lmac=$mac lid= liface=
            if [[ ${mactype[$lmac]} == 1if ]] then
                lid=${mac2id[$lmac]#' '}
                liface=${mac2iface[$lmac]#' '}
                ltype=1if
                lnode=${ip2node[$lid]}
                linfo="${ifnames[$lid:$liface]}"
            elif [[ ${mactype[$lmac]} == mif ]] then
                lid=${mac2id[$lmac]#' '}
                liface=
                ltype=mif
                lnode=${ip2node[$lid]}
                linfo=
            elif [[ ${mactype[$lmac]} == sa ]] then
                lid=
                liface=
                ltype=sa
                lnode=${ip2node[${mac2ip[$lmac]}]}
                linfo=""
            else
                continue
            fi
            rmac=${ifmacs[$id:$iface]}
            rid=$id riface=$iface
            if [[ ${mactype[$rmac]} == 1if ]] then
                rid=${mac2id[$rmac]#' '}
                riface=${mac2iface[$rmac]#' '}
                rtype=1if
                rnode=${ip2node[$rid]}
                rinfo="${ifnames[$rid:$riface]}"
            elif [[ ${mactype[$rmac]} == mif ]] then
                rid=${mac2id[$rmac]#' '}
                riface=
                rtype=mif
                rnode=${ip2node[$rid]}
                rinfo=
            elif [[ ${mactype[$rmac]} == sa ]] then
                rid=
                riface=
                rtype=sa
                rnode=${ip2node[${mac2ip[$rmac]}]}
                rinfo=""
            else
                continue
            fi
            [[ ${nodes[$lnode]} == '' || ${nodes[$rnode]} == '' ]] && continue
            insertedge "$lnode" "$rnode" "P2P:$linfo:$rinfo"
            nodeprops[$lnode]=edge
            nodeprops[$rnode]=edge
        done
    done
fi

if [[ " $modes " == @(*' p2p '*| ALL ) ]] then
    for net in "${!net2ip[@]}"; do
        [[ ${net2mask[$net]} != " 255.255.255.252" ]] && continue
        set -A nips -- ${net2ip[$net]}
        [[ ${#nips[@]} != 2 ]] && continue
        lnode=${ip2node[${nips[0]}]}
        rnode=${ip2node[${nips[1]}]}
        lport=${ifnames[${ip2id[${nips[0]}]}:${ip2iface[${nips[0]}]}]}
        rport=${ifnames[${ip2id[${nips[1]}]}:${ip2iface[${nips[1]}]}]}
        [[ ${nodes[$lnode]} == '' || ${nodes[$rnode]} == '' ]] && continue
        insertedge "$lnode" "$rnode" "R2R:$lport:$rport"
        nodeprops[$lnode]=edge
        nodeprops[$rnode]=edge
    done
fi

if [[ " $outtypes " == @(*' view '*| ALL ) ]] then
    exec 5> view-inv.txt
pr=nd_
    for node in "${!nodes[@]}"; do
        id=${node//[![:alnum:]]/_}
        print -r -u5 "node|o|${pr}$id|name|$node"
        print -r -u5 "node|o|${pr}$id|nodename|$node"
        print -r -u5 "node|o|${pr}$id|shname|$node"
        print -r -u5 "node|o|${pr}$id|shsize|-1"

        print -r -u5 "node|g|${pr}${maps[g]:-g1}|name|${maps[g]:-g1}"
        print -r -u5 "node|g|${pr}${maps[g]:-g1}|nodename|${maps[g]:-g1}"
        print -r -u5 "node|g|${pr}${maps[g]:-g1}|groupname|${maps[g]:-g1}"

        print -r -u5 "map|g|${pr}${maps[g]:-g1}|s|${pr}${maps[s]:-s1}||"
        print -r -u5 "map|o|${pr}$id|g|${pr}${maps[g]:-g1}||"

        print -r -u5 "map|o|${pr}$id|b|${maps[b]:-b1}||"
        print -r -u5 "map|o|${pr}$id|c|${pr}${maps[c]:-c1}||"
        print -r -u5 "map|s|${pr}${maps[s]:-s1}|l|${maps[l]:-l1}||"
        print -r -u5 "map|s|${pr}${maps[s]:-s1}|c|${pr}${maps[c]:-c1}||"
    done
    for edge in "${!edges[@]}"; do
        lnode=${edge%%'|'*}
        lid=${lnode//[![:alnum:]]/_}
        rnode=${edge##*'|'}
        rid=${rnode//[![:alnum:]]/_}
        print -r -u5 "edge|o|${pr}$lid|o|${pr}$rid|type|${edges[$edge]#' '}"
    done
    exec 5>&-
fi

if [[ " $outtypes " == @(*' scope '*| ALL ) ]] then
    exec 5> scope-inv.txt
    for node in "${!nodes[@]}"; do
        if [[ ${dev2ip[$node]} != '' ]] then
            id=${node//[![:alnum:]]/_}
            print -r -u5 "node|o|$id|scope_ip|${dev2ip[$node]}"
            print -r -u5 "node|o|$id|scope_systype|${systype[$node]}"
            print -r -u5 "node|o|$id|scope_user|${sshusers[$node]}"
            print -r -u5 "node|o|$id|scope_pass|${sshpasss[$node]}"
            print -r -u5 "node|o|$id|scope_snmpcommunity|${snmpcss[$node]}"
            print -r -u5 "node|o|$id|scope_sysfunc|client"
            print -r -u5 "node|o|$id|scope_servicelevel|os"
        else
            id=${node//[![:alnum:]]/_}
            print -r -u5 "node|o|$id|scope_ip|$node"
            print -r -u5 "node|o|$id|scope_systype|netdev"
            print -r -u5 "node|o|$id|scope_user|${sshusers[$node]}"
            print -r -u5 "node|o|$id|scope_pass|${sshpasss[$node]}"
            print -r -u5 "node|o|$id|scope_snmpcommunity|${snmpcss[$node]}"
            print -r -u5 "node|o|$id|scope_sysfunc|client"
            print -r -u5 "node|o|$id|scope_servicelevel|os"
        fi
    done
    exec 5>&-
fi

if [[ " $outtypes " == @(*' graph '*| ALL ) ]] then
    exec 5> graph.dot
    print -r -u5 "digraph g {"
    print -r -u5 "rankdir=LR"
    print -r -u5 "node [ shape=box ]"
    for node in "${!nodes[@]}"; do
        [[ ${nodeprops[$node]} != *edge* ]] && continue
        id=${node//[![:alnum:]]/_}
        print -r -u5 "\"$id\" [ label=\"${nodes[$node]}\" ]"
    done
    for edge in "${!edges[@]}"; do
        lnode=${edge%%'|'*}
        lid=${lnode//[![:alnum:]]/_}
        rnode=${edge##*'|'}
        rid=${rnode//[![:alnum:]]/_}
        label=${edges[$edge]#' '}
        label=${label//' '/'\\n'}
        print -r -u5 "\"$lid\" -> \"$rid\" [ label=\"${label}\" ]"
    done
    print -r -u5 "}"
    exec 5>&-
fi
