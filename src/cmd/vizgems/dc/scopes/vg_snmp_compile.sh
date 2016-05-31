
usage=$'
[-1p1?
@(#)$Id: vg_snmp_compile (AT&T Labs Research) 2006-03-12 $
]
'$USAGE_LICENSE$'
[+NAME?vg_snmp_compile - generate snmpwalk scripts based on specifications]
[+DESCRIPTION?\bvg_snmp_compile\b generates a script that performs a multi-part
snmpwalk operation and correlates the results based on the input specification.
]
[999:v?increases the verbosity level. May be specified multiple times.]

input_file

[+SEE ALSO?\bVizGEMS\b(1)]
'

function getvars {
    typeset rest=$1

    while [[ $rest == *\$\{*\}* ]] do
        rest=${rest#*\$\{}
        var=${rest%%\}*}
        rest=${rest#"$var"}
        print $var
    done
}

while getopts -a vg_snmp_compile "$usage" opt; do
    case $opt in
    999) (( SWIFTWARNLEVEL++ )) ;;
    *) OPTIND=0 getopts -a vg_snmp_compile "$usage" opt '-?'; exit 1 ;;
    esac
done
shift $OPTIND-1
if [[ $1 == '' ]] then
    OPTIND=0 getopts -a vg_snmp_compile "$usage" opt '-?'
    exit 1
fi

. $1

SNMPCMD=${SNMPCMD:-vgsnmpwalk}

typeset -A nodes keys consts done todo

hexmapcmd='{
    hls=
    while read -r hl; do
        if [[ $hl == *Hex-STRING* ]] then
            [[ $hls != "" ]] && print -r ""
            hls=
        fi
        print -rn "$hls$hl"
        hls=" "
    done
    [[ $hls != "" ]] && print -r ""
}'

mstringcmd='{
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
}'

for ni in ${rec.keys}; do
    keys[$ni]=1
done

for ni in ${rec.consts}; do
    consts[$ni]=1
done

print "set -o pipefail"

if [[ ${rec.prepend} != '' ]] then
    print -r "${rec.prepend}"
fi

simplevars=
print "{"
for (( i = 0; ; i++ )) do
    typeset -n w=rec.walk$i
    if [[ ${w} == '' ]] then
        break
    fi
    typeset -n n=rec.walk$i.name
    typeset -n v=rec.walk$i.value
    typeset -n t=rec.walk$i.type
    typeset -n ie=rec.walk$i.ifexpr
    if [[ ${n} == *\$\{*\}* ]] then
        set -f
        set -A nl -- $(getvars "${n}")
        set +f
        snmpvar=${n%%\$*}
        snmpvar=${snmpvar%.}
    else
        snmpvar=${n}
    fi
    if [[ $snmpvar == *[a-zA-Z]* ]] then
        nsnmpvar=$(snmptranslate -On -IR $snmpvar)
        n=${n//$snmpvar/$nsnmpvar}
        snmpvar=$nsnmpvar
    fi
    [[ ${ie} != '' ]] && print "    if $ie then"
    if [[ $t == HEXMAP ]] then
        print "    \$SNMPCMD -On \$SNMPARGS $snmpvar | $hexmapcmd"
    elif [[ $t == MSTRING ]] then
        print "    \$SNMPCMD -On \$SNMPARGS $snmpvar | $mstringcmd"
    elif [[ ${ie} != '' ]] then
        print "    \$SNMPCMD -On \$SNMPARGS $snmpvar || return 1"
    else
        simplevars+=" $snmpvar"
    fi
    [[ ${ie} != '' ]] && print "    fi"
    if [[ ${v} == *\$\{*\}* ]] then
        set -f
        set -A vl -- $(getvars "${v}")
        set +f
    fi
    for ni in "${nl[@]}" "${vl[@]}"; do
        #print -u2 done $ni
        done[$ni-$ni]=$ni-$ni
        nodes[$ni]=$ni
        [[ ${keys[$ni]} == '' ]] && continue
        for nj in "${nl[@]}" "${vl[@]}"; do
            [[ $ni == $nj ]] && continue
            #print -u2 done $ni $nj
            done[$ni-$nj]=$ni-$nj
        done
    done
done
if [[ $simplevars != '' ]] then
    print "    \$SNMPCMD -On \$SNMPARGS ${simplevars#' '} || return 1"
fi
print "} | {"
for ni in "${nodes[@]}"; do
    print typeset -A ${ni}_vals
    [[ ${keys[$ni]} == '' ]] && continue
    for nj in "${nodes[@]}"; do
        [[ $ni == $nj ]] && continue
        [[ ${consts[$nj]} != '' ]] && continue
        print typeset -A ${ni}_2_${nj}
        if [[ ${done[$ni-$nj]} != '' ]] then
            continue
        fi
        #print -u2 todo $ni $nj
        todo[$ni-$nj]=$ni-$nj
    done
done
print "while read -r name eq type value; do"
print '    case $name in'
for (( i = 0; ; i++ )) do
    typeset -n w=rec.walk$i
    if [[ ${w} == '' ]] then
        break
    fi
    typeset -n n=rec.walk$i.name
    typeset -n v=rec.walk$i.value
    typeset -n t=rec.walk$i.type
    typeset -n vra=rec.walk$i.varappend
    print "    ${n//@(~(-g)'${'*'}')/@(*)})"
    set -A nl j1 $(getvars "${n}")
    print "        [[ \$name == ${n//@(~(-g)'${'*'}')/@(*)} ]]"
    print '        set -A dl -- "${.sh.match[@]}"'
    for (( ni = 1; ni < ${#nl[@]}; ni++ )) do
        print "        ${nl[$ni]}="'${dl['$ni']}'
    done
    if [[ ${vra} != '' ]] then
        print "        ${vra}"
    fi
    set -A vl j1 $(getvars "${v}")
    print "        [[ \$value == ${v//@(~(-g)'${'*'}')/@(*)} ]]"
    print '        set -A dl -- "${.sh.match[@]}"'
    if [[ $t == HEXMAP ]] then
        print "        base=1"
        print "        for di in \${dl[0]}; do"
        print "            byte=\$(printf '%08.0.2d' \$(( 0x\$di )))"
        print "            for (( dj = 0; dj < 8; dj++ )) do"
        print "                bit=\${byte:\$dj:1}"
        print "                if [[ \$bit == 1 ]] then"
        print "                    ${vl[1]}=\$(( base + dj ))"
        for ni in "${nl[@]}" "${vl[@]}"; do
            [[ $ni == j1 ]] && continue
            print "                    [[ \$$ni == '' ]] && continue"
            print "                    ${ni}_vals[\$$ni]=\$$ni"
            [[ ${keys[$ni]} == '' ]] && continue
            for nj in "${nl[@]}" "${vl[@]}"; do
                [[ $ni == $nj ]] && continue
                [[ ${done[$ni-$nj]} == '' ]] && continue
                print "                    ${ni}_2_${nj}[\$$ni]+=\" \$$nj\""
            done
        done
        print "                fi"
        print "            done"
        print "            (( base += 8 ))"
        print "        done"
    else
        for (( ni = 1; ni < ${#vl[@]}; ni++ )) do
            print "        ${vl[$ni]}="'${dl['$ni']}'
            if [[ $t == SSTRING ]] then
                print "        ${vl[$ni]}=\${${vl[$ni]}//'|'/' '}"
            fi
        done
        for ni in "${nl[@]}" "${vl[@]}"; do
            [[ $ni == j1 ]] && continue
            print "        [[ \$$ni == '' ]] && continue"
            print "        ${ni}_vals[\$$ni]=\$$ni"
            [[ ${keys[$ni]} == '' ]] && continue
            for nj in "${nl[@]}" "${vl[@]}"; do
                [[ $ni == $nj ]] && continue
                [[ ${done[$ni-$nj]} == '' ]] && continue
                print "        ${ni}_2_${nj}[\$$ni]=\$$nj"
            done
        done
    fi
    print "        ;;"
done
print "    esac"
print "done"
print "}"

print "if [[ \$? != 0 ]] then"
print "    print -u2 vg_snmp_cmd_cooked: snmpwalk failed"
print "    exit 1"
print "fi"

while (( ${#todo[@]} > 0 )) do
    flag=n
    for ei in "${todo[@]}"; do
        ni=${ei%%-*}
        nj=${ei#*-}
        for nk in "${nodes[@]}"; do
            [[ $nk == $ni || $nk == $nj ]] && continue
            if [[ ${done[$ni-$nk]} != '' && ${done[$nk-$nj]} != '' ]] then
                if [[ ${done[$ni-$nj]} == '' ]] then
                    done[$ni-$nj]=${done[$ni-$nk]%-*}-$nk-${done[$nk-$nj]#*-}
                else
                    ndone=${done[$ni-$nk]%-*}-$nk-${done[$nk-$nj]#*-}
                    nlen=${ndone//+([!-])/}
                    olen=${done[$ni-$nj]//+([!-])/}
                    (( ${#nlen} < ${#olen} )) && done[$ni-$nj]=$ndone
                fi
                unset todo[$ei]
                flag=y
            fi
        done
        [[ $flag == y ]] && break
    done
    [[ $flag == n ]] && break
done

if (( ${#todo[@]} > 0 )) then
    print -u2 ERROR cannot map ${#todo[@]} items: ${todo[@]}
fi

#print -u2 ${done[@]}

function getvals {
    typeset -i pi pj
    typeset v
    eval set -A pl ${done[$1-$2]//-/' '}
    v=$3
    for (( pi = 0; pi < ${#pl[@]} - 1; pi++ )) do
        (( pj = pi + 1 ))
        v="\${${pl[$pi]}_2_${pl[$pj]}[$v]}"
    done
    print "$v"
}

outkey=${rec.outkey}
print "for $outkey in \"\${${outkey}_vals[@]}\"; do"
fs=${rec.output}
set -A nl $(getvars "${rec.output}")
for ni in "${nl[@]}"; do
    if [[ ${consts[$ni]} != '' ]] then
        v='${'${ni}_vals'[@]}'
        fs=${fs//'${'$ni'}'/$v}
    elif [[ $ni != $outkey ]] then
        v=$(getvals ${outkey} ${ni} \$$outkey)
        fs=${fs//'${'$ni'}'/$v}
    fi
done
print "    print \"${fs//\"/\\\"}\""
outfilter=${rec.outfilter}
if [[ $outfilter == '' ]] then
    print "done"
else
    print "done | $outfilter"
fi

print "exit 0"
