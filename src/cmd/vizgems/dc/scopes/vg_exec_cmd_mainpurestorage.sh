
targetname=$1
targetaddr=$2
targettype=$3

typeset -A names args

ifs="$IFS"
IFS='|'
set -f
while read -r name type unit impl; do
    [[ $name != @(storageinv|storagestat)* ]] && continue
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
    done
    IFS='|'
done
set +f
IFS="$ifs"

user=$EXECUSER pass=$EXECPASS

u=''
[[ $INVMODE == y ]] && u='-u2'

if [[ ${args[urlbase]} == '' ]] then
    print $u "rt=ALARM sev=1 type=ALARM tech=purestorage txt=\"url base not specified for purestorage collection\""
    exit 0
fi
urlbase=${args[urlbase]}

apiversion=1.6
if [[ ${args[apiversion]} != '' ]] then
    apiversion=${args[apiversion]}
fi

authurl=$urlbase/api/$apiversion/auth/session
dataurl=$urlbase/api/$apiversion

aid=

typeset -A array emitted
typeset -A sevs=(
    [critical]=1 [major]=2 [minor]=3 [warning]=4
)

# authentication

function gettoken { # $1 = user, $2 = pass (apitoken)
    typeset user=$1 pass=$2

    typeset tk

    touch hdr.txt
    eval $(
        curl -D hdr.txt -k -s -X POST \
            -H "Content-type: application/json" -H "Accept: application/json" \
        $authurl -d "{
          \"api_token\": \"$pass\"
        }" | vgjson2ksh tk
    )

    aid=$( egrep '^Set-Cookie:.*session=' hdr.txt )
    aid=${aid#*' '}
    aid=${aid%$'\r'}
    rm -f hdr.txt
}

# inventory

function getarray { # no args
    typeset ar sr i cap

    eval $(
        curl -k -s \
            -b "$aid" -H "Accept: application/json" \
        "$dataurl/array" | vgjson2ksh ar
    )
    array=(
        name=${ar.array_name} version=${ar.version}
        revision=${ar.revision} id=${ar.id}
        typeset -A hosts luns disks
    )

    eval $(
        curl -k -s \
            -b "$aid" -H "Accept: application/json" \
        "$dataurl/array?space=true" | vgjson2ksh sr
    )
    cap=0
    for (( i = 0; ; i++ )) do
        typeset -n r=sr.[$i]
        [[ ${r.hostname} == '' ]] && break

        (( cap += ${r.capacity} ))
    done
    array.capacity=$cap
}

function gethosts { # no args
    typeset hr i

    eval $(
        curl -k -s \
            -b "$aid" -H "Accept: application/json" \
        "$dataurl/host" | vgjson2ksh hr
    )
    for (( i = 0; ; i++ )) do
        typeset -n r=hr[$i]
        [[ ${r.name} == '' ]] && break

        array.hosts[${r.name}]=(
            name=${r.name}
        )
    done
}

function getvolumes { # no args
    typeset vr i

    eval $(
        curl -k -s \
            -b "$aid" -H "Accept: application/json" \
        "$dataurl/volume" | vgjson2ksh vr
    )
    for (( i = 0; ; i++ )) do
        typeset -n r=vr.[$i]
        [[ ${r.name} == '' ]] && break

        array.luns[$(namecnv "${r.name}")]=(
            name=${r.name} created=$(datecnv "${r.created}") serial=${r.serial}
            size=${r.size} source=${r.source}
        )
    done
}

function getdisks { # no args
    typeset dr i dname

    eval $(
        curl -k -s \
            -b "$aid" -H "Accept: application/json" \
        "$dataurl/drive" | vgjson2ksh dr
    )
    for (( i = 0; ; i++ )) do
        typeset -n r=dr.[$i]
        [[ ${r.name} == '' ]] && break

        array.disks[$(namecnv "${r.name}")]=(
            name=${r.name} status=${r.status}
            last_failure=$(datecnv "${r.last_failure}") type=${r.type}
            capacity=${r.capacity} degraded=${r.degraded}
            details=${r.details}
        )
    done
}

function geninventory { # no args
    typeset hname lname dname

    print "node|o|$AID|si_name|${array.name}"
    print "node|o|$AID|si_version|${array.version}"
    print "node|o|$AID|si_revision|${array.revision}"
    print "node|o|$AID|si_id|${array.id}"
    print "node|o|$AID|si_capacity|$(spacecnv "${array.capacity}") GB"
    print "node|o|$AID|si_sz_space_used._total|$(spacecnv "${array.capacity}")"
    print "node|o|$AID|si_host_total|SUM"

    for hname in "${!array.hosts[@]}"; do
        typeset -n hr=array.hosts[$hname]
        print "node|o|$AID|si_host$hname|${hr.name}"
    done
    for lname in "${!array.luns[@]}"; do
        typeset -n lr=array.luns[$lname]
        print "node|o|$AID|si_lunname$lname|${lr.name}"
        print "node|o|$AID|si_lunsize$lname|$(spacecnv "${lr.capacity}") GB"
    done
    for dname in "${!array.disks[@]}"; do
        typeset -n dr=array.disks[$dname]
        print "node|o|$AID|si_disk$dname|${dr.name}"
        print "node|o|$AID|si_hwname$dname|$(spacecnv "${dr.capacity}") GB"
        print "node|o|$AID|si_hwinfo$dname|:FW::SN::MN:${dr.type}"
    done
}

# alarms and stats

function genstatsandalarms { # no args
    typeset cr sr mr vr dr ar i k v l amode use n t id f1 f2 sev

    f1='rt=STAT name="%s" num=%.3f unit="%s" type=%s label="%s"'
    f2='rt=STAT name="%s" rti=%ld num=%.3f unit="%s" type=%s label="%s"'

    eval $(
        curl -k -s \
            -b "$aid" -H "Accept: application/json" \
        "$dataurl/array?controllers=true" | vgjson2ksh cr
    )
    for (( i = 0; ; i++ )) do
        typeset -n r=cr.[$i]
        [[ ${r.name} == '' ]] && break

        l=${r.name}
        id=$(namecnv "${r.name}")
        amode=clear
        [[ ${r.status} != ready ]] && amode=set
        togglealarm $amode controller.$id "Controller $l status = ${r.status}"
    done

    eval $(
        curl -k -s \
            -b "$aid" -H "Accept: application/json" \
        "$dataurl/array?space=true" | vgjson2ksh sr
    )
    use=0
    for (( i = 0; ; i++ )) do
        typeset -n r=sr.[$i]
        [[ ${r.hostname} == '' ]] && break

        (( use += ${r.total} ))
    done
    n=$(spacecnv "$use")
    printf "$f1\n" "space_used._total" $n "GB" "number" "Space Used (SUM)"

    eval $(
        curl -k -s \
            -b "$aid" -H "Accept: application/json" \
        "$dataurl/array?action=monitor" | vgjson2ksh mr
    )
    id=_total
    for (( i = 0; ; i++ )) do
        typeset -n r=mr.[$i]
        [[ ${r.time} == '' ]] && break

        t=$(printf '%(%#)T\n' "${r.time}")
        n=${r.input_per_sec}
        printf "$f2\n" "io_input.$id" $t $n "" "number" "Input / sec (SUM)"
        n=${r.output_per_sec}
        printf "$f2\n" "io_output.$id" $t $n "" "number" "Output / sec (SUM)"
        n=${r.reads_per_sec}
        printf "$f2\n" "io_read.$id" $t $n "" "number" "Reads / sec (SUM)"
        n=${r.writes_per_sec}
        printf "$f2\n" "io_write.$id" $t $n "" "number" "Writes / sec (SUM)"
        n=${r.usec_per_read_op}
        printf "$f2\n" "io_readop.$id" $t $n "" "number" "usec / Read Op (SUM)"
        n=${r.usec_per_write_op}
        printf "$f2\n" "io_writeop.$id" $t $n "" "number" "usec / Write Op (SUM)"
        n=${r.queue_depth}
        printf "$f2\n" "io_queuedepth.$id" $t $n "" "number" "Queue Depth (SUM)"
    done

    eval $(
        curl -k -s \
            -b "$aid" -H "Accept: application/json" \
        "$dataurl/host?action=monitor" | vgjson2ksh mr
    )
    for (( i = 0; ; i++ )) do
        typeset -n r=mr.[$i]
        [[ ${r.time} == '' ]] && break

        l=${r.name}
        id=$(namecnv "${r.name}")
        t=$(printf '%(%#)T\n' "${r.time}")
        n=${r.input_per_sec}
        printf "$f2\n" "io_input.$id" $t $n "" "number" "Input / sec ($l)"
        n=${r.output_per_sec}
        printf "$f2\n" "io_output.$id" $t $n "" "number" "Output / sec ($l)"
        n=${r.reads_per_sec}
        printf "$f2\n" "io_read.$id" $t $n "" "number" "Reads / sec ($l)"
        n=${r.writes_per_sec}
        printf "$f2\n" "io_write.$id" $t $n "" "number" "Writes / sec ($l)"
        n=${r.usec_per_read_op}
        printf "$f2\n" "io_readop.$id" $t $n "" "number" "usec / Read Op ($l)"
        n=${r.usec_per_write_op}
        printf "$f2\n" "io_writeop.$id" $t $n "" "number" "usec / Write Op ($l)"
    done

    eval $(
        curl -k -s \
            -b "$aid" -H "Accept: application/json" \
        "$dataurl/volume?space=true" | vgjson2ksh vr
    )
    for (( i = 0; ; i++ )) do
        typeset -n r=vr.[$i]
        [[ ${r.name} == '' ]] && break

        l=${r.name}
        id=$(namecnv "${r.name}")
        n=$(spacecnv "${r.total}")
        printf "$f1\n" "space_used.$id" $n "GB" "number" "Space Used ($l)"
    done

    eval $(
        curl -k -s \
            -b "$aid" -H "Accept: application/json" \
        "$dataurl/drive" | vgjson2ksh dr
    )
    for (( i = 0; ; i++ )) do
        typeset -n r=dr.[$i]
        [[ ${r.name} == '' ]] && break

        l=${r.name}
        id=$(namecnv "${r.name}")
        amode=clear
        [[ ${r.status} != @(healthy|unused) ]] && amode=set
        togglealarm $amode disk.$id "Disk $l status = ${r.status}"
    done

    (( mint = VG_JOBTS - 2 * VG_JOBIV ))
    eval $(
        curl -k -s \
            -b "$aid" -H "Accept: application/json" \
        "$dataurl/message?recent=true" | vgjson2ksh ar
    )
    for (( i = 0; ; i++ )) do
        typeset -n r=ar.[$i]
        [[ ${r.opened} == '' ]] && break

        rti="rti=$(printf '%(%#)T' "${r.opened}")"
        sev="sev=1"
        if [[ ${r.current_severity} != '' ]] then
            sev="sev=${sevs[${r.current_severity}]:-1}"
        fi
        unset args; typeset -A args
        for k in "${!r.@}"; do
            typeset -n v=$k
            args[${k##*.}]=$v
            typeset +n v
        done
        if (( ${#args[@]} > 0 )) then
            v=''
            for k in "${!args[@]}"; do
                v+=" ${k##*.}=${args[$k]//\"/\'}"
            done
            v=${v##+(' ')}
            if [[ ${emitted[$v]} == '' ]] then
                print "rt=ALARM $sev $rti type=ALARM tech=purestorage txt=\"$v\""
                emitted[$v]=$VG_JOBTS
            fi
        fi
    done
    eval $(
        curl -k -s \
            -b "$aid" -H "Accept: application/json" \
        "$dataurl/message?audit=true&recent=true" | vgjson2ksh ar
    )
    for (( i = 0; ; i++ )) do
        typeset -n r=ar.[$i]
        [[ ${r.opened} == '' ]] && break

        rti="rti=$(printf '%(%#)T' "${r.opened}")"
        sev="sev=1"
        if [[ ${r.current_severity} != '' ]] then
            sev="sev=${sevs[${r.current_severity}]:-1}"
        fi
        unset args; typeset -A args
        for k in "${!r.@}"; do
            typeset -n v=$k
            args[${k##*.}]=$v
            typeset +n v
        done
        if (( ${#args[@]} > 0 )) then
            v=''
            for k in "${!args[@]}"; do
                v+=" ${k##*.}=${args[$k]//\"/\'}"
            done
            v=${v##+(' ')}
            if [[ ${emitted[$v]} == '' ]] then
                print "rt=ALARM $sev $rti type=ALARM tech=purestorage txt=\"$v\""
                emitted[$v]=$VG_JOBTS
            fi
        fi
    done

    (( t = VG_JOBTS - 25 * 60 * 60 ))
    for v in "${!emitted[@]}"; do
        (( emitted[$v] < t )) && unset emitted["$v"]
    done

    typeset -p emitted > emitted.sh.tmp && mv emitted.sh.tmp emitted.sh
    print rt=STAT name=storagestat nodata=2
}

# utilities

function datecnv { # $1 = date
    printf '%(%#)T\n' "$1"
}

function namecnv { # $1 = name
    typeset -l l=$1

    print -r "${l//[!a-z0-9_-]/_}"
}

function spacecnv { # $1 = bytes
    typeset -F03 s=$1

    print $(( s / (1024.0 * 1024.0 * 1024.0) ))
}

function togglealarm { # $1 = mode $2 = alarmid $3 = text
    typeset mode=$1 aid=$2 text=$3

    case $mode in
    set)
        if [[ ! -f alarm.purestorage.$aid ]] then
            print "rt=ALARM sev=1 alarmid=$aid type=ALARM tech=purestorage txt=\"$text\""
        fi
        touch alarm.purestorage.$aid
        ;;
    clear)
        if [[ -f alarm.purestorage.$aid ]] then
            print "rt=ALARM sev=5 alarmid=$aid type=CLEAR tech=purestorage txt=\"$text\""
            rm alarm.purestorage.$aid
        fi
        ;;
    esac
}

case $INVMODE in
y)
    gettoken "$user" "$pass"

    getarray
    gethosts
    getvolumes
    getdisks
    geninventory
    ;;
*)
    [[ -f emitted.sh ]] && . ./emitted.sh
    gettoken "$user" "$pass" || exit
    genstatsandalarms
    ;;
esac
