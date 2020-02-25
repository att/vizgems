
targetname=$1
targetaddr=$2
targettype=$3

typeset -A names args

ifs="$IFS"
IFS='|'
set -f
while read -r name type unit impl; do
    [[ $name != @(idracinv|idracstat)* ]] && continue
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

urlbase=https://$targetaddr
if [[ ${args[urlbase]} != '' ]] then
    urlbase=${args[urlbase]}
fi

authurl=$urlbase
dataurl=$urlbase
params='pwState,sysDesc,sysRev,svcTag,fans,powerSupplies,temperatures,voltages'
paramsplus="$params,RecentEventLogEntries"
aid=
st1=
st2=

host=()
typeset lastalarmti=0
typeset -A sevs=(
    [critical]=1 [major]=2 [minor]=3 [warning]=4
    [Critical]=1 [Normal]=4
)

# authentication

function gettoken { # $1 = user, $2 = pass (apitoken)
    typeset user=$1 pass=$2
    typeset page

    touch hdr.txt
    page=$(
        curl -D hdr.txt -k -s \
            -d "user=$user&password=$pass" \
        "$authurl/data/login"
    )
    st1=${page#*ST1=}; st1=${st1%%[,<]*}
    st2=${page#*ST2=}; st2=${st2%%[,<]*}

    aid=$( egrep '^Set-Cookie.*-http-session' hdr.txt )
    [[ $aid == '' ]] && aid=$( egrep '^Set-Cookie.*_appwebSessionId_' hdr.txt )
    aid=${aid#*e:\ }
    aid=${aid%%\;*}
    rm -f hdr.txt
}

function droptoken { # no args
    curl -L -k -s \
        -b "$aid" -H "ST1: $st1" -H "ST2: $st2" \
    "$authurl/data/logout" > /dev/null
}

# inventory

function getresources { # no args
    typeset dr i j k name status units v minwv maxwv minfv maxfv type
    typeset -l id

    eval $(
        curl -k -s \
            -b "$aid" -H "ST1: $st1" -H "ST2: $st2" \
            -H 'X_SYSMGMT_OPTIMIZE: true' \
        "$dataurl/data?get=$params" | vgxml2ksh dr
    )
    host=(
        model=${ findcsbyname dr sysDesc; }
        version=${ findcsbyname dr svcTag; }
        typeset -A ps fans temps
    )

    for (( i = 0; i < ${#dr.cs[@]}; i++ )) do
        if [[ ${dr.cs[$i].name} == psSensorList ]] then
            typeset -n pr=dr.cs[$i]
            for (( j = 0; j < ${#pr.cs[@]}; j++ )) do
                [[ ${pr.cs[$j].name} != sensor ]] && continue
                name=${ findcsbyname dr.cs[$i].cs[$j] name; }
                name=${name%' Status'}
                id=${name//[!A-Za-z0-9_]/_}
                status=${ findcsbyname dr.cs[$i].cs[$j] sensorHealth; }
                host.ps[$id]=( id=$id name=$name status=$status )
            done
        fi
    done

    for (( i = 0; i < ${#dr.cs[@]}; i++ )) do
        if [[ ${dr.cs[$i].name} == sensortype ]] then
            typeset -n lr=dr.cs[$i]
            for (( j = 0; j < ${#lr.cs[@]}; j++ )) do
                [[ ${lr.cs[$j].name} != thresholdSensorList ]] && continue
                typeset -n sr=lr.cs[$j]
                for (( k = 0; k < ${#sr.cs[@]}; k++ )) do
                    [[ ${sr.cs[$k].name} != sensor ]] && continue
                    name=${ findcsbyname dr.cs[$i].cs[$j].cs[$k] name; }
                    id=${name//[!A-Za-z0-9_]/_}
                    status=${ findcsbyname dr.cs[$i].cs[$j].cs[$k] sensorStatus; }
                    units=${ findcsbyname dr.cs[$i].cs[$j].cs[$k] units; }
                    v=${ findcsbyname dr.cs[$i].cs[$j].cs[$k] reading; }
                    minwv=${ findcsbyname dr.cs[$i].cs[$j].cs[$k] minWarning; }
                    maxwv=${ findcsbyname dr.cs[$i].cs[$j].cs[$k] maxWarning; }
                    minfv=${ findcsbyname dr.cs[$i].cs[$j].cs[$k] minFailure; }
                    maxfv=${ findcsbyname dr.cs[$i].cs[$j].cs[$k] maxFailure; }
                    case $units in
                    RPM)
                        host.fans[$id]=( id=$id name=$name status=$status )
                        type=fan
                        ;;
                    C)
                        host.temps[$id]=( id=$id name=$name status=$status )
                        type=temp
                        ;;
                    esac
                done
            done
        fi
    done
}

function geninventory { # no args
    typeset pid fid tid

    print "node|o|$AID|si_model|${host.model}"
    print "node|o|$AID|si_version|${host.version}"

    for pid in "${!host.ps[@]}"; do
        typeset -n pr=host.ps[$pid]
        print "node|o|$AID|si_psname$pid|${pr.name}"
    done
    for fid in "${!host.fans[@]}"; do
        typeset -n fr=host.fans[$fid]
        print "node|o|$AID|si_fanid$fid|$fid"
        print "node|o|$AID|si_fanlabel$fid|${fr.name}"
    done
    for tid in "${!host.temps[@]}"; do
        typeset -n tr=host.temps[$tid]
        print "node|o|$AID|si_tempid$tid|$tid"
        print "node|o|$AID|si_templabel$tid|${tr.name}"
    done
}

# alarms and stats

function genstatsandalarms { # no args
    typeset f1 dr i j k name status units v minwv maxwv minfv maxfv type amode
    typeset key label sev rti maxrti datestr text t
    typeset -l id
    typeset -F03 rv

    f1='rt=STAT name="%s" num=%.3f unit="%s" type=number label="%s"'

    eval $(
        curl -k -s \
            -b "$aid" -H "ST1: $st1" -H "ST2: $st2" \
            -H 'X_SYSMGMT_OPTIMIZE: true' \
        "$dataurl/data?get=$paramsplus" | vgxml2ksh dr
    )

    for (( i = 0; i < ${#dr.cs[@]}; i++ )) do
        if [[ ${dr.cs[$i].name} == psSensorList ]] then
            typeset -n pr=dr.cs[$i]
            for (( j = 0; j < ${#pr.cs[@]}; j++ )) do
                [[ ${pr.cs[$j].name} != sensor ]] && continue
                name=${ findcsbyname dr.cs[$i].cs[$j] name; }
                name=${name%' Status'}
                id=${name//[!A-Za-z0-9_]/_}
                status=${ findcsbyname dr.cs[$i].cs[$j] sensorHealth; }
                amode=clear
                [[ $status != 2 ]] && amode=set
                togglealarm 1 $amode ps.$id "$name health=$status"
            done
        fi
    done

    for (( i = 0; i < ${#dr.cs[@]}; i++ )) do
        if [[ ${dr.cs[$i].name} == sensortype ]] then
            typeset -n lr=dr.cs[$i]
            for (( j = 0; j < ${#lr.cs[@]}; j++ )) do
                [[ ${lr.cs[$j].name} != thresholdSensorList ]] && continue
                typeset -n sr=lr.cs[$j]
                for (( k = 0; k < ${#sr.cs[@]}; k++ )) do
                    [[ ${sr.cs[$k].name} != sensor ]] && continue
                    name=${ findcsbyname dr.cs[$i].cs[$j].cs[$k] name; }
                    id=${name//[!A-Za-z0-9_]/_}
                    units=${ findcsbyname dr.cs[$i].cs[$j].cs[$k] units; }
                    v=${ findcsbyname dr.cs[$i].cs[$j].cs[$k] reading; }
                    minwv=${ findcsbyname dr.cs[$i].cs[$j].cs[$k] minWarning; }
                    maxwv=${ findcsbyname dr.cs[$i].cs[$j].cs[$k] maxWarning; }
                    minfv=${ findcsbyname dr.cs[$i].cs[$j].cs[$k] minFailure; }
                    maxfv=${ findcsbyname dr.cs[$i].cs[$j].cs[$k] maxFailure; }
                    case $units in
                    RPM)
                        type=fan
                        key=sensor_fan
                        label='Fan Sensor'
                        rv=$v
                        ;;
                    C)
                        type=temp
                        key=sensor_temp
                        label='Temp Sensor'
                        units=F
                        (( rv = 32.0 + (9.0 / 5.0) * v ))
                        ;;
                    esac
                    printf "$f1\n" "$key.$id" $rv "$units" "$label ($name)"
                    amode=clear sev=5 text=
                    if [[ $minfv == +([0-9]) ]] && (( v < minfv )) then
                        amode=set sev=1 text=" in failure range ($rv)"
                    elif [[ $maxfv == +([0-9]) ]] && (( v > maxfv )) then
                        amode=set sev=1 text=" in failure range ($rv)"
                    elif [[ $minwv == +([0-9]) ]] && (( v < minwv )) then
                        amode=set sev=1 text=" in warning range ($rv)"
                    elif [[ $maxwv == +([0-9]) ]] && (( v > maxwv )) then
                        amode=set sev=1 text=" in warning range ($rv)"
                    fi
                    togglealarm $sev $amode $type.$id "$name$text"
                done
            done
        fi
    done

    maxrti=0
    for (( i = 0; i < ${#dr.cs[@]}; i++ )) do
        if [[ ${dr.cs[$i].name} == RecentEventLogEntries ]] then
            typeset -n er=dr.cs[$i]
            for (( j = 0; j < ${#er.cs[@]}; j++ )) do
                [[ ${er.cs[$j].name} != RecentEventLogEntry ]] && continue
                sev=${ findcsbyname dr.cs[$i].cs[$j] severity; }
                sev="sev=${sevs[$sev]:-4}"
                datestr=${ findcsbyname dr.cs[$i].cs[$j] dateTime; }
                text=${ findcsbyname dr.cs[$i].cs[$j] description; }
                rti=$(printf '%(%#)T' "$datestr" 2> /dev/null)
                [[ $? != 0 ]] && continue
                (( maxrti < rti )) && maxrti=$rti
                (( lastalarmti > rti )) && continue
                rti="rti=$rti"
                print "rt=ALARM $sev $rti type=ALARM tech=idrac txt=\"$text\""
            done
        fi
    done

    (( lastalarmti <= maxrti )) && (( lastalarmti = maxrti + 1 ))
    typeset -p lastalarmti > idracstate.sh.tmp \
    && mv idracstate.sh.tmp idracstate.sh
    print rt=STAT name=idracstat nodata=2
}

# utilities

csbynameindex=
function findcsbyname { # $1 = varref, $2 = name, $3 = y to just set index
    typeset -n cr=$1.cs
    typeset name=$2 saveindex=$3

    typeset i

    csbynameindex=
    for (( i = 0; i < ${#cr[@]}; i++ )) do
        if [[ ${cr[$i].name} == $name ]] then
            if [[ $saveindex != '' ]] then
                csbynameindex=$i
            else
                print -- ${cr[$i].cs[0].text}
            fi
            return 0
        fi
    done
    return 1
}

function togglealarm { # $1 = sev $2 = mode $3 = alarmid $4 = text
    typeset sev=$1 mode=$2 aid=$3 text=$4

    case $mode in
    set)
        if [[ ! -f alarm.idrac.$aid ]] then
            print "rt=ALARM sev=$sev alarmid=$aid type=ALARM tech=idrac txt=\"$text\""
        fi
        touch alarm.idrac.$aid
        ;;
    clear)
        if [[ -f alarm.idrac.$aid ]] then
            print "rt=ALARM sev=$sev alarmid=$aid type=CLEAR tech=idrac txt=\"$text\""
            rm alarm.idrac.$aid
        fi
        ;;
    esac
}

case $INVMODE in
y)
    gettoken "$user" "$pass"
    getresources
    geninventory
    droptoken
    ;;
*)
    [[ -f idracstate.sh ]] && . ./idracstate.sh
    gettoken "$user" "$pass" || exit
    genstatsandalarms
    droptoken
    ;;
esac

exit 0
