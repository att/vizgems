
if [[ $SWIFTWARNLEVEL != '' ]] then
    set -x
fi

. vg_hdr
ver=$VG_S_VERSION

dl=$DEFAULTLEVEL
if [[ $CASEINSENSITIVE == y ]] then
    typeset -l l1 l2 id1 id2
fi
if [[ $ALWAYSCOMPRESSDP == y ]] then
    zflag='-ozm rtb'
fi

mode=$1
shift

dir=${PWD%/processed/*}
dir=${dir##*main/}
date=${dir//\//.}

if [[ $mode == @(monthly|yearly) ]] then
    exec 4>&2
    lockfile=lock.alarm
    set -o noclobber
    while ! command exec 3> $lockfile; do
        if kill -0 $(< $lockfile); then
            [[ $WAITLOCK != y ]] && exit 0
        elif [[ $(fuser $lockfile) != '' ]] then
            [[ $WAITLOCK != y ]] && exit 0
        else
            rm -f $lockfile
        fi
        print -u4 SWIFT MESSAGE waiting for instance $(< ${lockfile}) to exit
        sleep 1
    done 2> /dev/null
    print -u3 $$
    set +o noclobber
    trap "rm -f $lockfile" HUP QUIT TERM KILL EXIT
    exec 4>&-
fi

case $mode in
sl)
    typeset -A sevmap

    ifs="$IFS"
    IFS='|'
    maxid=0
    defid=0
    sevmap[0]=none
    egrep '^[0-9][0-9]*\|.*' $1 | sed 's/\r//g' | tr A-Z a-z \
    | while read id name; do
        if [[ $name != _default_ ]] then
            sevmap[$id]=$name
        else
            defid=$id
        fi
        if (( maxid < id )) then
            maxid=$id
        fi
    done
    IFS="$ifs"

    if [[ ${sevmap[$defid]} == '' ]] then
        defid=0
    fi
    (
        print $(( maxid + 1 ))
        print $defid
        for id in "${!sevmap[@]}"; do
            print "$id|${sevmap[$id]}"
        done | sort -t'|' -k1,1n
    ) > $2.tmp && sdpmv $2.tmp $2
    ;;
xml)
    [[ -f ${4%.done}.dds ]] && touch $4
    [[ -f $4 ]] && exit 0

    if (( ALARMSPLITFILESIZE >= 100000 )) then
        if (( $(wc -c < $1) > ALARMSPLITFILESIZE )) then
            print -u2 SWIFT WARNING splitting large alarm file $1 $(ls -lh $1)
            d=${1%/*}
            [[ $1 != */* ]] && d=.
            split -C $(( $ALARMSPLITFILESIZE / 2 )) -a 4 $1 $1.
            if [[ $? == 0 ]] then
                n=0
                for i in $1.*; do
                    if [[ -f $d/$n.${1##*/} ]] then
                        (( n++ ))
                        continue
                    fi
                    mv $i $d/$n.${1##*/}
                    (( n++ ))
                done
                if [[ $ALARMSAVEDIR != '' ]] then
                    mv $1 $ALARMSAVEDIR
                else
                    touch $4
                fi
                exit 0
            else
                rm -f $1.*
            fi
        fi
    fi

    d1="$date-00:00:00"
    d2="$date-23:59:59"
    export MINTIME=$(( $(printf '%(%#)T' "$d1") - 6 * 30 * 24 * 60 * 60 ))
    export MAXTIME=$(( $(printf '%(%#)T' "$d2") + 10 * 60 ))
    if (( MAXTIME > $(printf '%(%#)T') + 15 * 60 )) then
        (( MAXTIME = $(printf '%(%#)T') + 15 * 60 ))
    fi

    export LEVELMAPFILE=LEVELS-maps.dds
    export INVMAPFILE=inv-maps.dds
    export INVNODEATTRFILE=inv-nodes.dds
    export ACCOUNTFILE=$VGMAINDIR/dpfiles/account.filter
    if [[ -f $LEVELMAPFILE && -f $INVMAPFILE ]] then
        if [[ ! -f $ACCOUNTFILE ]] then
            print -u2 SWIFT ERROR creating empty account filter file
            touch -d 200001010000.00 $ACCOUNTFILE
        fi
    else
        print -u2 SWIFT MESSAGE waiting for inventory files
        exit 0
    fi

    typeset -l state smode type sorder pmode tmode
    typeset -A statemap
    statemap[$VG_ALARM_S_STATE_NONE]=$VG_ALARM_N_STATE_NONE
    statemap[$VG_ALARM_S_STATE_INFO]=$VG_ALARM_N_STATE_INFO
    statemap[$VG_ALARM_S_STATE_DEG]=$VG_ALARM_N_STATE_DEG
    statemap[$VG_ALARM_S_STATE_DOWN]=$VG_ALARM_N_STATE_DOWN
    statemap[$VG_ALARM_S_STATE_UP]=$VG_ALARM_N_STATE_UP
    typeset -A modemap
    modemap[$VG_ALARM_S_MODE_NONE]=$VG_ALARM_N_MODE_NONE
    modemap[$VG_ALARM_S_MODE_KEEP]=$VG_ALARM_N_MODE_KEEP
    modemap[$VG_ALARM_S_MODE_DEFER]=$VG_ALARM_N_MODE_DEFER
    modemap[$VG_ALARM_S_MODE_DROP]=$VG_ALARM_N_MODE_DROP
    typeset -A typemap
    typemap[$VG_ALARM_S_TYPE_NONE]=$VG_ALARM_N_TYPE_NONE
    typemap[$VG_ALARM_S_TYPE_ALARM]=$VG_ALARM_N_TYPE_ALARM
    typemap[$VG_ALARM_S_TYPE_CLEAR]=$VG_ALARM_N_TYPE_CLEAR
    typeset -A sordermap
    sordermap[$VG_ALARM_S_SO_NONE]=$VG_ALARM_N_SO_NONE
    sordermap[$VG_ALARM_S_SO_NORMAL]=$VG_ALARM_N_SO_NORMAL
    sordermap[$VG_ALARM_S_SO_OVERRIDE]=$VG_ALARM_N_SO_OVERRIDE
    typeset -A pmodemap
    pmodemap[$VG_ALARM_S_PMODE_NONE]=$VG_ALARM_N_PMODE_NONE
    pmodemap[$VG_ALARM_S_PMODE_PROCESS]=$VG_ALARM_N_PMODE_PROCESS
    pmodemap[$VG_ALARM_S_PMODE_PASSTHROUGH]=$VG_ALARM_N_PMODE_PASSTHROUGH

    export ALARMDBFILE=$VGMAINDIR/dpfiles/alarm/alarm.db
    export ALARMFILTERFILE=$VGMAINDIR/dpfiles/alarm/alarm.filter
    export OBJECTTRANSFILE=$2
    export SEVMAPFILE=$3
    if [[ ! -f $ALARMDBFILE || ! -f $ALARMFILTERFILE ]] then
        print -u2 SWIFT ERROR cannot find alarm db and filter files
        if [[ ! -f $ALARMDBFILE ]] then
            print -u2 SWIFT ERROR creating empty db file
            > $ALARMDBFILE
        fi
        if [[ ! -f $ALARMFILTERFILE ]] then
            print -u2 SWIFT ERROR creating empty filter file
            > $ALARMFILTERFILE
        fi
    fi
    rm -f ${4%.done}.dds.fwd.[0-9][0-9][0-9][0-9].[0-9][0-9].[0-9][0-9]
    vgxml -flatten2 '' < $1 | while read -u3 -r -C alarm; do
        typeset -n aref=alarm
        [[ $aref == '' ]] && break

        sid=${aref.sid:-${aref.scope}}
        aid=${aref.aid:-${aref.alarmid}}
        ccid=${aref.ccid}
        state=${aref.st:-${aref.state}}
        state=${statemap[$state]:-$VG_ALARM_N_STATE_INFO}
        smode=${aref.sm:-${aref.smode}}
        smode=${modemap[$smode]:-$VG_ALARM_N_MODE_KEEP}
        vars=${aref.vars:-${aref.variables}}
        tc=${aref.tc:-${aref.timecleared}}
        ti=${aref.ti:-${aref.timeissued}}
        type=${aref.tp:-${aref.type}}
        type=${typemap[$type]:-$VG_ALARM_N_TYPE_ALARM}
        sorder=${aref.so:-${aref.sortorder}}
        sorder=${sordermap[$sorder]:-$VG_ALARM_N_SO_NORMAL}
        pmode=${aref.pm:-${aref.pmode}}
        pmode=${pmodemap[$pmode]:-$VG_ALARM_N_PMODE_PROCESS}
        l1=${aref.lv1:-${aref.level1:-$dl}}
        id1=${aref.id1:-${aref.id:-${aref.target}}}
        id1=${id1//[!.a-zA-Z0-9_:-]/}
        l2=${aref.lv2:-${aref.level2}}
        id2=${aref.id2}
        id2=${id2//[!.a-zA-Z0-9_:-]/}
        tmode=${aref.tm:-${aref.tmode}}
        tmode=${modemap[$tmode]:-$VG_ALARM_N_MODE_KEEP}
        sev=${aref.sev:-${aref.severity:-5}}
        text=${aref.txt:-${aref.text}}
        com=${aref.com:-${aref.comment}}
        origmsg=${aref.origmsg}
        print -u4 -nr "$sid|$aid|$ccid|$state|$smode|$vars||"
        print -u4 -nr "|$tc|$ti|$type|$sorder|$pmode"
        print -u4 -r "|$l1|$id1|$l2|$id2|$tmode|$sev|$text|$com|$origmsg"
    done 3<&0- 4>&1- \
    | ddsload -os vg_alarm.schema -le '|' -type ascii -dec url \
        -lnts -lso vg_alarm.load.so \
    | ddsfilter -fso vg_alarm.filter.so \
    | ddssort -fast -ke 'timeissued type' \
    | ddssplit -sso vg_alarm_bydate.split.so -p ${4%.done}.dds.fwd.%s
    if [[ -f ${4%.done}.dds.fwd.$date ]] then
        mv ${4%.done}.dds.fwd.$date ${4%.done}.dds
    fi
    if [[ $ALARMSAVEDIR != '' ]] then
        mv $1 $ALARMSAVEDIR
    else
        touch $4
    fi
    ;;
txt)
    [[ -f ${4%.done}.dds ]] && touch $4
    [[ -f $4 ]] && exit 0

    d1="$date-00:00:00"
    d2="$date-23:59:59"
    export MINTIME=$(( $(printf '%(%#)T' "$d1") - 6 * 30 * 24 * 60 * 60 ))
    export MAXTIME=$(( $(printf '%(%#)T' "$d2") + 1 * 24 * 60 * 60 ))

    export LEVELMAPFILE=LEVELS-maps.dds
    export INVMAPFILE=inv-maps.dds
    export INVNODEATTRFILE=inv-nodes.dds
    export ACCOUNTFILE=$VGMAINDIR/dpfiles/account.filter
    if [[ -f $LEVELMAPFILE && -f $INVMAPFILE ]] then
        if [[ ! -f $ACCOUNTFILE ]] then
            print -u2 SWIFT ERROR creating empty account filter file
            touch -d 200001010000.00 $ACCOUNTFILE
        fi
    else
        print -u2 SWIFT MESSAGE waiting for inventory files
        exit 0
    fi

    dnt=${1##*/}
    dnt=${dnt%.alarms.txt}
    dnt=${dnt:0:2}:${dnt:3:2}:${dnt:6:2}
    tim=$(printf '%(%#)T' ${date}-$dnt)

    export ALARMDBFILE=$VGMAINDIR/dpfiles/alarm/alarm.db
    export ALARMFILTERFILE=$VGMAINDIR/dpfiles/alarm/alarm.filter
    export OBJECTTRANSFILE=$2
    export SEVMAPFILE=$3
    if [[ ! -f $ALARMDBFILE || ! -f $ALARMFILTERFILE ]] then
        print -u2 SWIFT ERROR cannot find alarm db and filter files
        if [[ ! -f $ALARMDBFILE ]] then
            print -u2 SWIFT ERROR creating empty db file
            > $ALARMDBFILE
        fi
        if [[ ! -f $ALARMFILTERFILE ]] then
            print -u2 SWIFT ERROR creating empty filter file
            > $ALARMFILTERFILE
        fi
    fi
    rm -f ${4%.done}.dds.fwd.[0-9][0-9][0-9][0-9].[0-9][0-9].[0-9][0-9]
    while read -r line; do
        l1= l2= id1= id2= sev= aid= st= ctim= ccid=
        case $line in
        .iso.*severity*|iso.*severity*|ccit*severity*)
            if [[ $line == *object=* ]] then
                id1=${line#*object=}
                id1=${id1%%\ *}
            elif [[ $line == *object1=* ]] then
                id1=${line#*object1=}
                id1=${id1%%\ *}
            else
                VG_warning 0 'DATA ERROR' "not an alarm record: '$line'"
                continue
            fi
            if [[ $line == *object2=* ]] then
                id2=${line#*object=}
                id2=${id2%%\ *}
            fi
            id1=${id1//[!.a-zA-Z0-9_:-]/}
            id2=${id2//[!.a-zA-Z0-9_:-]/}
            if [[ $line == *SWIFTlevel=* ]] then
                l1=${line#*SWIFTlevel=}
                l1=${l1%%\ *}
            elif [[ $line == *SWIFTlevel1=* ]] then
                l1=${line#*SWIFTlevel1=}
                l1=${l1%%\ *}
            else
                l1=$dl
            fi
            if [[ $line == *SWIFTlevel2=* ]] then
                l2=${line#*SWIFTlevel2=}
                l2=${l2%%\ *}
            elif [[ $id2 != '' ]] then
                l2=$dl
            fi
            if [[ $line == *severity=* ]] then
                sev=${line#*severity=}
                sev=${sev%%\ *}
                case $sev in
                critical) sev=1 ;;
                major) sev=2 ;;
                minor) sev=3 ;;
                warning) sev=4 ;;
                normal) sev=5 ;;
                *) sev=5 ;;
                esac
            else
                sev=5
            fi
            if [[ $line == *state=* ]] then
                type=${line#*state=}
                type=${type%%\ *}
                if [[ $type == UP ]] then
                    type=$VG_ALARM_N_TYPE_CLEAR
                else
                    type=$VG_ALARM_N_TYPE_ALARM
                fi
            else
                type=$VG_ALARM_N_TYPE_ALARM
            fi
            if [[ $line == *SWIFTalarmid=* ]] then
                aid=${line#*SWIFTalarmid=}
                aid=${aid%%\ *}
                case $aid in
                __CLEAR_ALL__!*)
                    id1=__all_clear__
                    aid=
                    ;;
                __CLEAR_CCID__!*)
                    ccid=${aid#*!}
                    aid=
                    ;;
                __CLEAR_ALARM__!*)
                    ctim=${aid#*!}
                    aid=
                    ;;
                __CLEAR_OBJECT__!*)
                    aid=
                    ;;
                esac
            fi
            text=${line##*msg_text=?('"')}
            if [[ $line == *tmode=* ]] then
                tmode=${line#*tmode=}
                tmode=${tmode%%\ *}
                if [[ $tmode != yes ]] then
                    tmode=$VG_ALARM_N_MODE_DROP
                else
                    tmode=$VG_ALARM_N_MODE_KEEP
                fi
            elif [[ $text == -* ]] then
                tmode=$VG_ALARM_N_MODE_DROP
            else
                tmode=$VG_ALARM_N_MODE_KEEP
            fi
            text=${text%'"'}
            text=${text//'|'/\\'|'}
            [[ $type == $VG_ALARM_N_TYPE_CLEAR ]] && (( tim++ ))
            print -nr "|$aid|$ccid|$VG_ALARM_N_STATE_INFO"
            print -nr "|$VG_ALARM_N_MODE_KEEP||||$ctim|$tim|$type"
            print -nr "|$VG_ALARM_N_SO_NORMAL|$VG_ALARM_N_PMODE_PROCESS|$l1"
            print -r "|$id1|$l2|$id2|$tmode|$sev|$text||"
            ;;
        /*)
            if [[ $line == *object=* ]] then
                id1=${line#*object=}
                id1=${id1%%\ *}
            elif [[ $line == *object1=* ]] then
                id1=${line#*object1=}
                id1=${id1%%\ *}
            else
                VG_warning 0 'DATA ERROR' "not an alarm record: '$line'"
                continue
            fi
            if [[ $line == *object2=* ]] then
                id2=${line#*object=}
                id2=${id2%%\ *}
            fi
            id1=${id1//[!.a-zA-Z0-9_:-]/}
            id2=${id2//[!.a-zA-Z0-9_:-]/}
            if [[ $line == *SWIFTlevel=* ]] then
                l1=${line#*SWIFTlevel=}
                l1=${l1%%\ *}
            elif [[ $line == *SWIFTlevel1=* ]] then
                l1=${line#*SWIFTlevel1=}
                l1=${l1%%\ *}
            else
                l1=$dl
            fi
            if [[ $line == *SWIFTlevel2=* ]] then
                l2=${line#*SWIFTlevel2=}
                l2=${l2%%\ *}
            elif [[ $id2 != '' ]] then
                l2=$dl
            fi
            if [[ $line == *severity=* ]] then
                sev=${line#*severity=}
                sev=${sev%%\ *}
                case $sev in
                critical) sev=1 ;;
                major) sev=2 ;;
                minor) sev=3 ;;
                warning) sev=4 ;;
                normal) sev=5 ;;
                *) sev=5 ;;
                esac
            else
                sev=5
            fi
            if [[ $line == *msg_text=*:UP* || $line == *msg_text=*:DOWN* ]] then
                type=${line#*msg_text=}
                if [[ $type == *:UP\ * ]] then
                    type=$VG_ALARM_N_TYPE_CLEAR
                else
                    type=$VG_ALARM_N_TYPE_ALARM
                fi
            else
                type=$VG_ALARM_N_TYPE_ALARM
            fi
            if [[ $line == *SWIFTalarmid=* ]] then
                aid=${line#*SWIFTalarmid=}
                aid=${aid%%\ *}
                case $aid in
                __CLEAR_ALL__!*)
                    id1=__all_clear__
                    aid=
                    ;;
                __CLEAR_CCID__!*)
                    ccid=${aid#*!}
                    aid=
                    ;;
                __CLEAR_ALARM__!*)
                    ctim=${aid#*!}
                    aid=
                    ;;
                __CLEAR_OBJECT__!*)
                    aid=
                    ;;
                esac
            fi
            text=${line#*msg_text=\"}
            if [[ $line == *tmode=* ]] then
                tmode=${line#*tmode=}
                tmode=${tmode%%\ *}
                if [[ $tmode != yes ]] then
                    tmode=$VG_ALARM_N_MODE_DROP
                else
                    tmode=$VG_ALARM_N_MODE_KEEP
                fi
            elif [[ $text == -* ]] then
                tmode=$VG_ALARM_N_MODE_DROP
            else
                tmode=$VG_ALARM_N_MODE_KEEP
            fi
            text=${text%'"'}
            text=${text//'|'/\\'|'}
            [[ $type == CLEAR ]] && (( tim++ ))
            print -nr "|$aid|$ccid|$VG_ALARM_N_STATE_INFO"
            print -nr "|$VG_ALARM_N_MODE_KEEP||||$ctim|$tim|$type"
            print -nr "|$VG_ALARM_N_SO_NORMAL|$VG_ALARM_N_PMODE_PROCESS|$l1"
            print -r "|$id1|$l2|$id2|$tmode|$sev|$text||"
            ;;
        iso.*|*TRAP,*SNMP*v*)
            ;;
        *)
            VG_warning 0 'DATA ERROR' "not an alarm record: '$line'"
            continue
            ;;
        esac
    done < $1 \
    | ddsload -os vg_alarm.schema -le '|' -type ascii -dec simple \
        -lnts -lso vg_alarm.load.so \
    | ddsfilter -fso vg_alarm.filter.so \
    | ddssort -fast -ke 'timeissued type' \
    | ddssplit -sso vg_alarm_bydate.split.so -p ${4%.done}.dds.fwd.%s
    if [[ -f ${4%.done}.dds.fwd.$date ]] then
        mv ${4%.done}.dds.fwd.$date ${4%.done}.dds
    fi
    if [[ $ALARMSAVEDIR != '' ]] then
        mv $1 $ALARMSAVEDIR
    else
        touch $4
    fi
    ;;
pptxt)
    [[ -f ${4%.done}.dds ]] && touch $4
    [[ -f $4 ]] && exit 0

    d1="$date-00:00:00"
    d2="$date-23:59:59"
    export MINTIME=$(( $(printf '%(%#)T' "$d1") - 6 * 30 * 24 * 60 * 60 ))
    export MAXTIME=$(( $(printf '%(%#)T' "$d2") + 1 * 24 * 60 * 60 ))

    export LEVELMAPFILE=LEVELS-maps.dds
    export INVMAPFILE=inv-maps.dds
    export INVNODEATTRFILE=inv-nodes.dds
    export ACCOUNTFILE=$VGMAINDIR/dpfiles/account.filter
    if [[ -f $LEVELMAPFILE && -f $INVMAPFILE ]] then
        if [[ ! -f $ACCOUNTFILE ]] then
            print -u2 SWIFT ERROR creating empty account filter file
            touch -d 200001010000.00 $ACCOUNTFILE
        fi
    else
        print -u2 SWIFT MESSAGE waiting for inventory files
        exit 0
    fi

    export ALARMDBFILE=$VGMAINDIR/dpfiles/alarm/alarm.db
    export ALARMFILTERFILE=$VGMAINDIR/dpfiles/alarm/alarm.filter
    export OBJECTTRANSFILE=$2
    export SEVMAPFILE=$3
    if [[ ! -f $ALARMDBFILE || ! -f $ALARMFILTERFILE ]] then
        print -u2 SWIFT ERROR cannot find alarm db and filter files
        if [[ ! -f $ALARMDBFILE ]] then
            print -u2 SWIFT ERROR creating empty db file
            > $ALARMDBFILE
        fi
        if [[ ! -f $ALARMFILTERFILE ]] then
            print -u2 SWIFT ERROR creating empty filter file
            > $ALARMFILTERFILE
        fi
    fi
    rm -f ${4%.done}.dds.fwd.[0-9][0-9][0-9][0-9].[0-9][0-9].[0-9][0-9]
    IFS='|'
    set -f
    sed -E -e 's/^.*SWIFTSNMP (.*)"$/\1/' $1 \
    | while read -r line; do
        case $line in
        5.0\|*)
            set -A list -- $line
            list[19]=${list[19]##*msg_text=\"}
            list[19]=${list[19]%\"}
            case ${list[1]} in
            __CLEAR_ALL__!*)
                list[14]=__all_clear__
                list[1]= list[2]= list[8]=0
                ;;
            __CLEAR_CCID__!*)
                list[2]=${list[0]#*!}
                list[1]= list[8]=0
                ;;
            __CLEAR_ALARM__!*)
                list[8]=${list[0]#*!}
                list[1]= list[2]=
                ;;
            __CLEAR_OBJECT__!*)
                list[1]= list[2]= list[8]=0
                ;;
            esac
            line="|${list[1]}|${list[2]}|${list[4]}|${list[5]}|${list[6]}"
            line+="|${list[7]}||${list[8]}|${list[9]}|${list[10]}|${list[11]}"
            line+="|${list[12]}|${list[13]}|${list[14]}|${list[15]}|${list[16]}"
            line+="|${list[17]}|${list[18]}|${list[19]}|${list[22]}"
            print -r "$line}||"
            ;;
        *)
            set -A list -- $line
            list[15]=${list[15]##*msg_text=\"}
            list[15]=${list[15]%\"}
            [[ ${list[1]} != *.* ]] && list[1]=unknown.${list[1]}
            case ${list[0]} in
            __CLEAR_ALL__!*)
                list[12]=__all_clear__
                list[0]= list[1]= list[7]=0
                ;;
            __CLEAR_CCID__!*)
                list[1]=${list[0]#*!}
                list[0]= list[7]=0
                ;;
            __CLEAR_ALARM__!*)
                list[7]=${list[0]#*!}
                list[0]= list[1]=
                ;;
            __CLEAR_OBJECT__!*)
                list[0]= list[1]= list[7]=0
                ;;
            esac
            line="|${list[0]}|${list[1]}|${list[3]}|${list[4]}|${list[5]}"
            line+="|${list[6]}||${list[7]}|${list[8]}|${list[9]}|${list[10]}"
            line+="|${list[11]}|$dl|${list[12]}||"
            line+="|${list[13]}|${list[14]}|${list[15]}|${list[18]}"
            print -r "$line}||"
            ;;
        esac
    done \
    | ddsload -os vg_alarm.schema -le '|' -type ascii -dec simple \
        -lnts -lso vg_alarm.load.so \
    | ddsfilter -fso vg_alarm.filter.so \
    | ddssort -fast -ke 'timeissued type' \
    | ddssplit -sso vg_alarm_bydate.split.so -p ${4%.done}.dds.fwd.%s
    set +f
    if [[ -f ${4%.done}.dds.fwd.$date ]] then
        mv ${4%.done}.dds.fwd.$date ${4%.done}.dds
    fi
    if [[ $ALARMSAVEDIR != '' ]] then
        mv $1 $ALARMSAVEDIR
    else
        touch $4
    fi
    ;;
all)
    export LEVELMAPFILE=LEVELS-maps.dds
    export INVMAPFILE=inv-maps.dds
    export INVNODEATTRFILE=inv-nodes.dds
    export INVEDGEATTRFILE=inv-edges.dds
    export INVEDGEFILE=inv-edges.dds
    export INVND2CCFILE=inv-cc-nd2cc.dds
    export INVCC2NDFILE=inv-cc-cc2nd.dds
    export ACCOUNTFILE=$VGMAINDIR/dpfiles/account.filter
    if [[ -f $LEVELMAPFILE && -f $INVMAPFILE ]] then
        if [[ ! -f $ACCOUNTFILE ]] then
            print -u2 SWIFT ERROR creating empty account filter file
            touch -d 200001010000.00 $ACCOUNTFILE
        fi
    else
        print -u2 SWIFT MESSAGE waiting for inventory files
        exit 0
    fi

    export SEVMAPFILE=$1
    $SHELL vg_alarm_ce check || exit 1

    if ! ddscat -checksf alarm.state; then
        print -u2 SWIFT ERROR open.alarms.dds corrupted - recomputing
        rm -f alarm.state open.alarms.dds* all.*.alarms.dds*
        for i in [0-9][0-9]*.alarms.done; do
            rm -f $i
        done
    fi

    hms=$(printf '%(%H.%M.%S)T')

    set -x
    args=${ print -r [0-9]*.alarms.dds; }
    argcur=${#args}
    filen=${args//[! ]/}; filen=${#filen}
    argmax=${ getconf ARG_MAX; }
    filemax=${ ulimit -n; }
    if [[ $args == '[0-9]*.alarms.dds' ]] then
        argcur=$argmax
    fi
    if (( argmax - argcur > 1024 && filemax - filen > 16 )) then
        for file in $args; do
            [[ ! -f $file ]] && continue
            print -r -- "./$file"
        done > alarm.state
        ddssort -m -fast -ke 'timeissued type' $args
    else
        ddscat -is vg_alarm.schema -sf alarm.state \
            -fp '[0-9]*.alarms.dds' \
        | ddssort -fast -ke 'timeissued type'
    fi \
    | $SHELL vg_alarm_ce run \
    3> alarms.$date.$hms.$$.1 \
    4> tickets.$date.$hms.$$.1 \
    | case $PROPALARMS in
    y) ddsfilter -fso vg_alarm_prop.filter.so ;;
    *) cat ;;
    esac \
    3> alarms.$date.$hms.$$.2 \
    | ddssort -fast -r -ke 'timeissued type' \
    | ddssplit $zflag -sso vg_alarm_byhour.split.so -p new.alarms.byhour.%s
    set +x
    for i in new.alarms.byhour.*; do
        [[ ! -f $i ]] && continue
        h=${i##*.byhour.}
        if [[ ! -f all.$h.alarms.dds ]] then
            ddscat -is vg_alarm.schema \
            > all.$h.alarms.dds
        fi
        set -x
        ddscat all.$h.alarms.dds $i \
        | ddssort $zflag -fast -r -ke 'timeissued type sortorder' \
        > all.$h.alarms.dds.tmp && mv all.$h.alarms.dds.tmp all.$h.alarms.dds
        case $PROPEMAILS in
        y)
            ename=email.alarms.$$.$RANDOM.$RANDOM.dds
            while [[ -f $ename ]] do
                ename=${ename%.dds}.$RANDOM.dds
            done
            mv $i $ename
            ;;
        *)
            rm $i
            ;;
        esac
        set +x
    done
    typeset -Z2 hh
    (( maxh = (MAXALARMKEEPMIN / 60) + 1 ))
    n=
    files=
    for (( h = 23; h >= 0 && n < maxh; h-- )) do
        hh=$h
        if [[ -f all.$hh.alarms.dds ]] then
            files+=" all.$hh.alarms.dds"
            (( n++ ))
        fi
    done
    if [[ $PREVOPENALARMS != '' && -f $PREVOPENALARMS ]] && (( n < maxh )) then
        files+=" $PREVOPENALARMS"
    fi
    set -x
    if [[ $files != '' ]] then
        ddscat $files \
        | ddsfilter $zflag -fso vg_alarm_open.filter.so \
        > $2.tmp && mv $2.tmp $2
    elif [[ ! -f $2 ]] then
        ddscat -is vg_alarm.schema \
        > $2.tmp && mv $2.tmp $2
    fi
    if [[ -s alarms.$date.$hms.$$.1 ]] then
        file=$hms.$$.ticketed
        while [[ -f $file.alarms.xml ]] do
            file=$file.0
        done
        mv alarms.$date.$hms.$$.1 $file.alarms.xml
    else
        rm alarms.$date.$hms.$$.1
    fi
    if [[ -s alarms.$date.$hms.$$.2 ]] then
        file=alarms.$date.$hms.$$
        while [[ -f $VGMAINDIR/outgoing/alarm/$file.xml ]] do
            file=$file.0
        done
        mv alarms.$date.$hms.$$.2 $VGMAINDIR/outgoing/alarm/$file.xml
    else
        rm alarms.$date.$hms.$$.2
    fi
    if [[ -s tickets.$date.$hms.$$.1 ]] then
        file=tickets.$date.$hms.$$.1
        while [[ -f $VGMAINDIR/outgoing/ticket/$file.xml ]] do
            file=$file.0
        done
        mv tickets.$date.$hms.$$.1 $VGMAINDIR/outgoing/ticket/$file.xml
    else
        rm tickets.$date.$hms.$$.1
    fi
    if [[ $PROPEMAILS == y ]] then
        export ALARMEMAILFILE=$VGMAINDIR/dpfiles/alarm/alarmemail.filter
        if [[ ! -f $ALARMEMAILFILE ]] then
            print -u2 SWIFT ERROR creating empty alarm email file
            > $ALARMEMAILFILE
        fi
        for i in email.alarms.*.dds; do
            [[ ! -f $i ]] && continue
            o=${i%.dds}.xml
            ddsfilter -osm none -fso vg_alarm_email.filter.so $i > $o
            while [[ -f $VGMAINDIR/outgoing/email/$o ]] do
                o=${o%.txt}.$RANDOM.xml
            done
            mv ${i%.dds}.xml $VGMAINDIR/outgoing/email/$o
            rm $i
        done
    fi
    set +x
    egrep / alarm.state | while read i; do
        rm $i
    done
    rm alarm.state
    ;;
monthly)
    [[ $ALARMNOAGGR == [yY] ]] && exit 0

    typeset -Z2 hh

    cymdh=$(printf '%(%Y.%m.%d.%H)T' "$AGGRDELAY ago")
    ts=$(printf '%(%Y.%m.%d.%H:%M:%S)T')
    f=n
    for d1 in ../../[0-9][0-9]; do
        dd=${d1##*/}
        d2=$d1/processed/total
        f2=n
        files=
        for (( hh = 23; hh >= 0; hh-- )) do
            [[ ! -f $d2/all.$hh.alarms.dds ]] && continue
            files+=" $d2/all.$hh.alarms.dds"
            [[ $date.$dd.$hh > $cymdh && $WAITLOCK != y ]] && continue
            [[ all.$dd.alarms.dds -nt $d2/all.$hh.alarms.dds ]] && continue
            f2=y
        done
        [[ $f2 == n ]] && continue
        set -x
        ddscat $files \
        | ddsfilter -fso vg_alarm_aggr.filter.so \
        | ddssort -fast -ozm rtb -ke 'level1 id1 level2 id2' \
        > all.$dd.alarms.dds.tmp \
        && mv all.$dd.alarms.dds.tmp all.$dd.alarms.dds
        touch -d $ts all.$dd.alarms.dds
        ddssort -fast -ozm rtb -m -u -ke 'level1 id1 level2 id2' \
            all.$dd.alarms.dds \
        > uniq.$dd.alarms.dds.tmp \
        && mv uniq.$dd.alarms.dds.tmp uniq.$dd.alarms.dds
        set +x
        f=y
    done
    if [[ $f == y ]] then
        files=
        for file in uniq.*.alarms.dds; do
            [[ ! -f $file ]] && continue
            files+=" $file"
        done
        if [[ $files != '' ]] then
            set -x
            ddssort -fast -ozm rtb -m -u -ke 'level1 id1 level2 id2' $files \
            > uniq.alarms.dds.tmp && mv uniq.alarms.dds.tmp uniq.alarms.dds
            set +x
        fi
    elif [[ ! -f uniq.alarms.dds ]] then
        file=
        for m1 in ../../../[0-9][0-9]; do
            mm=${m1##*/}
            [[ $mm == $dir ]] && continue
            m2=$m1/processed/total
            [[ -f $m2/uniq.alarms.dds ]] && file=$m2/uniq.alarms.dds
        done
        if [[ $file != '' ]] then
            set -x
            cp $file uniq.alarms.dds.tmp \
            && mv uniq.alarms.dds.tmp uniq.alarms.dds
            set +x
        fi
    fi
    ;;
yearly)
    [[ $ALARMNOAGGR == [yY] ]] && exit 0

    typeset -Z2 dd

    f=n
    for d1 in ../../[0-9][0-9]; do
        mm=${d1##*/}
        d2=$d1/processed/total
        [[ ! -f $d2/uniq.alarms.dds ]] && continue
        [[ all.$mm.alarms.dds -nt $d2/uniq.alarms.dds ]] && continue
        files=
        for (( dd = 31; dd >= 1; dd-- )) do
            [[ ! -f $d2/all.$dd.alarms.dds ]] && continue
            files+=" $d2/all.$dd.alarms.dds"
        done
        [[ $files == '' ]] && continue
        set -x
        ddssort -fast -ozm rtb -m -ke 'level1 id1 level2 id2' $files \
        > all.$mm.alarms.dds.tmp \
        && mv all.$mm.alarms.dds.tmp all.$mm.alarms.dds
        ddssort -fast -ozm rtb -m -u -ke 'level1 id1 level2 id2' \
            all.$mm.alarms.dds \
        > uniq.$mm.alarms.dds.tmp \
        && mv uniq.$mm.alarms.dds.tmp uniq.$mm.alarms.dds
        set +x
        f=y
    done
    if [[ $f == y ]] then
        files=
        for file in uniq.*.alarms.dds; do
            [[ ! -f $file ]] && continue
            files+=" $file"
        done
        if [[ $files != '' ]] then
            set -x
            ddssort -fast -ozm rtb -m -u -ke 'level1 id1 level2 id2' $files \
            > uniq.alarms.dds.tmp && mv uniq.alarms.dds.tmp uniq.alarms.dds
            set +x
        fi
    elif [[ ! -f uniq.alarms.dds ]] then
        file=
        for y1 in ../../../[0-9][0-9][0-9][0-9]; do
            yy=${y1##*/}
            [[ $yy == $dir ]] && continue
            y2=$y1/processed/total
            [[ -f $y2/uniq.alarms.dds ]] && file=$y2/uniq.alarms.dds
        done
        if [[ $file != '' ]] then
            set -x
            cp $file uniq.alarms.dds.tmp \
            && mv uniq.alarms.dds.tmp uniq.alarms.dds
            set +x
        fi
    fi
    ;;
esac
