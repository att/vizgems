
if [[ $SWIFTWARNLEVEL != "" ]] then
    PS4='${.sh.file}-${.sh.fun}-$LINENO: '
    exec 2> snmp_driver.log
    set -x
fi

usage=$'
[-1p1?
@(#)$Id: vg_snmp_driver (AT&T Labs Research) 2006-03-12 $
]
'$USAGE_LICENSE$'
[+NAME?vg_snmp_driver - extract system information using the SNMP interface]
[+DESCRIPTION?\bvg_snmp_driver\b extracts information by running snmpwalk.
The host name and type are specified on the command line.
The parameters to extract appear in stdin, one per line.
The first word in each line is the standard parameter name.
The rest of the text in the line is a CGI-style string.
The path name of the CGI can be \braw\b, to indicate that a single parameter
is to be collected.
When the path name is not \braw\b, it is assumed to indicate the suffix of the
name of a script.
The full script name will be \bvg_snmp_cmd_\bname, eg \bvg_snmp_cmd_xyz\b.
The output of that script must be zero or more lines of the form:
\brt=STAT ... stat param key=value pairs ...\b or
\brt=ALARM ... alarm param key=value pairs ...\b
The output of \bvg_snmp_driver\b is a \bksh\b compound variable containing
all the data from all the raw and scripted executions.
]
[999:v?increases the verbosity level. May be specified multiple times.]

targetname targetaddr targettype

[+SEE ALSO?\bVizGEMS\b(1)]
'

while getopts -a vg_snmp_driver "$usage" opt; do
    case $opt in
    999) (( SWIFTWARNLEVEL++ )) ;;
    *) OPTIND=0 getopts -a vg_snmp_driver "$usage" opt '-?'; exit 1 ;;
    esac
done
shift $OPTIND-1

targetname=$1
targetaddr=$2
targettype=$3

set -o pipefail

if [[ $targetaddr == *:*:* ]] then
    target=udp6:$targetaddr
else
    target=$targetaddr
fi

vars=()
varn=0
typeset -A js
rawi=0 otheri=0

ifs="$IFS"
IFS='|'
while read -r name type unit impl; do
    impl=${impl#SNMP:}
    mode=${impl%%\?*}
    impl=${impl##"$mode"?(\?)}

    IFS='&'
    set -f
    set -A al -- $impl
    set +f
    IFS='|'
    unset as; typeset -A as
    for a in "${al[@]}"; do
        k=${a%%=*}
        v=${a#*=}
        v=${v//'+'/' '}
        v=${v//@([\'\\])/'\'\1}
        eval "v=\$'${v//'%'@(??)/'\x'\1"'\$'"}'"
        v=${v//%26/'&'}
        as[$k]=$v
    done

    case $mode in
    raw)
        if [[ ${as[var]} == '' ]] then
            print -u2 vg_snmp_driver: no oid specified for $name
            continue
        fi
        ji=raw.$rawi
        (( rawi++ ))
        ;;
    cooked)
        if [[ ${as[helper]} == '' ]] then
            print -u2 vg_snmp_driver: no helper specified for $name
            continue
        fi
        ji=${as[helper]}
        ;;
    *)
        ji=other.$otheri
        (( otheri++ ))
        ;;
    esac

    if [[ ${js[$ji].mode} == '' ]] then
        js[$ji]=( mode=$mode; typeset -A ms; typeset -A as )
    fi
    js[$ji].ms[$name]=(
        name=$name type=$type unit=$unit impl=$impl typeset -A as
    )
    for ai in "${!as[@]}"; do
        js[$ji].ms[$name].as[$ai]=${as[$ai]}
        [[ $ai == @(*_exp_*|*_sto_*_*) ]] && havestoorexp=y
    done

    if [[ ${as[port]} != '' ]] then
        js[$ji].as[target]=$target:${as[port]}
    else
        js[$ji].as[target]=$target
    fi
    if [[ ${as[version]} != 1* ]] then
        js[$ji].as[walk]=vgsnmpbulkwalk
    else
        js[$ji].as[walk]=vgsnmpwalk
    fi
    js[$ji].as[timeout]=${as[timeout]:-2}
    js[$ji].as[retries]=${as[retries]:-2}
    js[$ji].as[version]=${as[version]:-1}
    js[$ji].as[community]=${as[community]:-public}
done
IFS="$ifs"

typeset -A umap
umap[octets]=B
umap[milliseconds]=ms
umap[Bytes]=B
umap[KBytes]=KB

for ji in "${!js[@]}"; do
    typeset -n jr=js[$ji]

    if [[ ${jr.as[community]} == SWENC*SWENC*SWENC ]] then
        cs=${jr.as[community]}
        builtin -f swiftsh swiftshenc swiftshdec
        builtin -f codex codex
        phrase=${cs#SWENC*SWENC}; phrase=${phrase%SWENC}
        swiftshdec phrase code
        cs=${cs#SWENC}; cs=${cs%%SWENC*}
        print -r "$cs" \
        | codex --passphrase="$code" "<uu-base64-string<aes" \
        | read -r cs
        jr.as[community]=$cs
    fi
    if [[ ${jr.as[community]} == SWMUL*SWMUL && -f snmp.lastmulti ]] then
        touch snmp.lastmulti
    fi
    if [[ ${jr.as[version]} != 3 ]] then
        jr.as[community]="-c ${jr.as[community]}"
    fi

    case ${jr.mode} in
    raw)
        name="${!jr.ms[@]}"
        typeset -n mr=jr.ms[$name]
        sf=${mr.as[func]:-sum}
        sv=0
        sn=0

        ${jr.as[walk]} \
            -Oef -t ${jr.as[timeout]} -r ${jr.as[retries]} \
            -v ${jr.as[version]} ${jr.as[community]} \
            ${jr.as[target]} ${mr.as[var]} \
        | while read -r line; do
            if [[ $line != *' = '*': '* ]] then
                print -u2 vg_snmp_driver: bad response for ${mr.as[var]}: $line
                continue
            fi
            line=${line#*' = '}
            t=${line%%:*}
            v=${line##"$t: "}
            u=
            case $t in
            Counter*)
                t=number; u= ;;
            "Network Address"|IpAddress|OID)
                t=string; u= ;;
            *STRING*)
                t=string; u=; v=${v//'"'/}
                [[ $v == +([0-9]) ]] && t=number
                ;;
            Gauge*|INTEGER)
                t=number
                if [[ $v == *' '* ]] then
                    u=${v#*' '}
                    v=${v%" $u"}
                    u=${umap[$u]:-$u}
                fi
                ;;
            Timeticks)
                t=string; u= ;;
            '""')
                t=string; v=; u= ;;
            *)
                t=string; u= ;;
            esac
            if [[ $v == '' && $t != string ]] then
                print -u2 vg_snmp_driver: empty value for ${mr.as[var]}
                continue
            fi
            case $sf in
            sum|avg) (( sv += v ))            ;;
            min)     (( sv > v )) && sv=$v    ;;
            max)     (( sv < v )) && sv=$v    ;;
            first)   [[ $sv == '' ]] && sv=$v ;;
            last)    sv=$v                    ;;
            esac
            (( sn++ ))
        done

        if (( sn > 0 )) then
            [[ $sf == avg && $sn != 0 ]] && (( sv /= $sn.0 ))
            typeset -n vr=vars._$varn
            vr.rt=STAT
            vr.name=$name
            vr.type=${mr.type}
            case $t in
            number) vr.num=$sv ;;
            *)      vr.str=$sv ;;
            esac
            vr.unit=${u:-${mr.as[unit]}}
            [[ ${mr.as[label]} != '' ]] && vr.label=${mr.as[label]}
            (( varn++ ))
        fi
        ;;
    cooked)
        if [[ $havestoorexp == y ]] then
            exps=
            seensto=n
            for name in "${!jr.ms[@]}"; do
                typeset -n mr=jr.ms[$name]
                for ai in "${!mr.as[@]}"; do
                    case $ai in
                    *_exp_*)
                        k=${ai#"_exp_"}
                        v=${mr.as[$ai]}
                        export VGEXP_$k=$v
                        exps+=" VGEXP_$k"
                        ;;
                    *_sto_*_*)
                        k=${ai#"_sto_"}; i=${k#*_}; k=${k%%_*}
                        v=${mr.as[$ai]}
                        [[ $seensto == n ]] && > $ji.data
                        seensto=y
                        print -r "${k}[\"$i\"]=\"$v\"" >> $ji.data
                        ;;
                    esac
                done
            done
        fi

        args="-t ${jr.as[timeout]} -r ${jr.as[retries]}"
        args+=" -v ${jr.as[version]} ${jr.as[community]} ${jr.as[target]}"
        SNMPCMD=${jr.as[walk]} SNMPARGS=$args SNMPIP=${jr.as[target]} \
            $SHELL $VG_SSCOPESDIR/current/snmp/vg_snmp_cmd_$ji \
        | while read -r line; do
            if [[ $line == *rt=STAT* ]] then
                name=${line##*'name='}; name=${name%%' '*}; name=${name//'"'/}
                [[ ${jr.ms[$name]} == '' ]] && continue
            fi
            typeset -n vr=vars._$varn
            (( varn++ ))
            eval vr=\( $line \)
        done
        if [[ $? != 0 ]] then
            print -u2 "vg_snmp_driver: failed to execute helper $ji"
        fi

        if [[ $havestoorexp == y ]] then
            [[ $exps != '' ]] && unset $exps
        fi
        ;;
    *)
        name="${!jr.ms[@]}"
        typeset -n mr=jr.ms[$name]
        print "${mr.name}|${mr.type}|${mr.unit}|${mr.impl}" \
        | $VG_SSCOPESDIR/current/snmp/vg_snmp_cmd_${jr.mode} \
            $targetname $targetaddr $targettype \
        | while read -r line; do
            typeset -n vr=vars._$varn
            (( varn++ ))
            eval vr=\( $line \)
        done
        if [[ $? != 0 ]] then
            print -u2 "vg_snmp_driver: failed to execute command ${jr.mode}"
        fi
        ;;
    esac
done

print "${vars}"
