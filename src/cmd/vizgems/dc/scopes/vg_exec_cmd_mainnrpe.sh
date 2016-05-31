
[[ -f ~/.vg_mainnrperc ]] && . ~/.vg_mainnrperc

targetname=$1
targetaddr=$2
targettype=$3

if [[ $INVMODE == y ]] then
    for inv2cmd in $VGINV2NRPECMDS; do
        if [[ $(check_nrpe -H $targetaddr -c "${inv2cmd#*:}") == *Command*not*defined* ]] then
            continue
        fi
        print "node|o|$AID|si_${inv2cmd%%:*}|${inv2cmd%%:*}"
    done
    exit 0
fi

typeset -A names args

ifs="$IFS"
IFS='|'
set -f
while read -r name type unit impl; do
    names[$name]=( name=$name type=$type unit=$unit impl=$impl typeset -A args )
    IFS='&'
    set -A al -- ${impl#*\?}
    unset as; typeset -A as
    for a in "${al[@]}"; do
        k=${a%%=*}
        v=${a#*=}
        v=${v//'+'/' '}
        v=${v//@([\'\\])/'\'\1}
        eval "v=\$'${v//'%'@(??)/'\x'\1"'\$'"}'"
        v=${v//%26/'&'}
        args[$k]=$v
        names[$name].args[$k]=$v
        case $k in
        aid) names[$name]._aid=${v//[!a-zA-Z0-9_:-]/_} ;;
        command) names[$name]._cmd=$v ;;
        esac
    done
    IFS='|'
done
set +f
IFS="$ifs"

for namei in "${!names[@]}"; do
    [[ ${names[$namei]._cmd} == '' ]] && continue
    typeset -n nr=names[$namei] ar=names[$namei].args
    cmd=${nr._cmd}
    check_nrpe -H $targetaddr -c $cmd | while read -r line; do
        apart=${line%%*( )'|'*}
        spart=${line#*'|'}
        spart=${spart##?( )}
        if [[ ${apart} == *CHECK_NRPE* ]] then
            print "rt=ALARM sev=1 alarmid=\"nrpe\" type=ALARM tech=NRPE txt=\"NRPE FAILURE\""
            continue
        fi
    
        if [[ ${apart%%[-:]*} != *OK* ]] then
            if [[ ${apart%%[-:]*} == *CRITICAL* ]] then
                sev=1
            elif [[ ${apart%%[-:]*} == *WARNING* ]] then
                sev=4
            else
                sev=3
            fi
            if [[ ! -f alarm.nrpe.${nr._aid} ]] then
                print "rt=ALARM sev=$sev alarmid=\"${ar[aid]}\" type=ALARM tech=NRPE txt=\"$apart\""
            fi
            touch alarm.nrpe.${nr._aid}
        elif [[ -f alarm.nrpe.${nr._aid} ]] then
            print "rt=ALARM sev=$sev alarmid=\"${ar[aid]}\" type=CLEAR tech=NRPE txt=\"$apart\""
            rm alarm.nrpe.${nr._aid}
        fi
        for kv in $spart; do
            k=${kv%%=*}
            [[ $k != "${ar[var]}" ]] && continue
            v=${kv#*=}
            v=${v%%';'*}
            print "rt=STAT name=${nr.name} type=number num=$v unit=${nr.unit} label=\"${ar[label]}\""
        done
    done
done
