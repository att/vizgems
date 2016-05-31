
if [[ $SWIFTWARNLEVEL != '' ]] then
    set -x
fi

mode=$1
shift

dir=${PWD%/processed/*}
dir=${dir##*main/}
date=${dir//\//.}

dir1=$VGMAINDIR/dpfiles/alarm
dir2=$VGMAINDIR/incoming/alarm

if [[ ! -d $dir1 ]] then
    print -u2 SWIFT ERROR cannot find correlation directory $dir1
    exit 1
fi

if [[ ! -f $dir1/ce.rules ]] then
    print -u2 SWIFT ERROR cannot find correlation rules $dir1/ce.rules
    exit 1
fi

if [[ -f $dir1/ce.prefix ]] then
    . $dir1/ce.prefix
    CCIDPREFIX=${CCIDPREFIX%%.*}
    if (( ${#CCIDPREFIX} > 0 && ${#CCIDPREFIX} < 24 )) then
        prefix="-prefix $CCIDPREFIX"
    fi
fi

args="-date $date -rf $dir1/ce.rules -sf $dir1/ce.state $prefix"
gargs="-date $date -rf $dir1/gce.rules -sf $dir1/gce.state"

case $mode in
check)
    if [[ $VG_SYSMODE != ems ]] then
        exit 0
    fi
    if [[ -f $dir1/ce.state.tmp ]] then
        print -u2 SWIFT WARNING previous invocation of ce ended abnormally
    fi
    if [[ ! -f $dir1/empty.dds ]] then
        ddscat -is vg_alarm.schema > $dir1/empty.dds
        if [[ ! -f $dir1/empty.dds ]] then
            print -u2 SWIFT ERROR cannot create alarm file $dir1/empty.dds
            exit 1
        fi
    fi
    if [[ $GCEPHASEON == y && ! -f $dir1/gce.state ]] then
        vggce -new $gargs < $dir1/empty.dds > /dev/null
    fi
    if [[ $GCEPHASEON == y ]] then
        if \
            ! vggce -checkrules $gargs < $dir1/empty.dds > /dev/null
        then
            print -u2 SWIFT ERROR cannot parse rules file $dir1/gce.rules
            exit 1
        fi
    fi
    if [[ ! -f $dir1/ce.state ]] then
        vgce -new 0 $args < $dir1/empty.dds > /dev/null
    fi
    if \
        ! vgce -checkrules $args < $dir1/empty.dds > /dev/null
    then
        print -u2 SWIFT ERROR cannot parse rules file $dir1/ce.rules
        exit 1
    fi
    if [[ -f $dir1/ce.docompact ]] then
        rm $dir1/ce.docompact
        print -u2 SWIFT MESSAGE compacting correlation data
        if \
            ! vgce -compact $args < $dir1/empty.dds > /dev/null
        then
            print -u2 SWIFT ERROR cannot compact file $dir1/ce.state
            exit 1
        fi
    fi
    if [[ -f $dir1/ce.dodump ]] then
        rm $dir1/ce.dodump
        print -u2 SWIFT MESSAGE dumping correlation data
        if \
            ! vgce -dump $args < $dir1/empty.dds > /dev/null 2> $dir1/ce.dump
        then
            print -u2 SWIFT ERROR cannot dump file $dir1/ce.state
            exit 1
        fi
    fi
    if [[ -f $dir1/ce.doflush ]] then
        rm $dir1/ce.doflush
        print -u2 SWIFT MESSAGE flushing correlation data
        if \
            ! vgce -flush $args < $dir1/empty.dds > /dev/null
        then
            print -u2 SWIFT ERROR cannot flush
            exit 1
        fi
        rm -f $dir1/ce.state $dir1/empty.dds
    fi
    ;;
update)
#    if [[ $VG_SYSMODE != ems ]] then
#        exit 0
#    fi
    tim=$(printf '%(%#)T')
    file=$dir2/$(printf 'alarms.%(%Y.%m.%d.%H.%M.%S)T.swift.xml')
    (
        print "<alarm>"
        print "<v>$VG_S_VERSION</v><aid></aid>"
        print "<ccid></ccid><st></st><sm></sm>"
        print "<vars></vars><di></di>"
        print "<hi></hi>"
        print "<tc>$tim</tc><ti>$tim</ti>"
        print "<tp>CLEAR</tp><so></so>"
        print "<pm>$VG_ALARM_N_PMODE_PASSTHROUGH</pm>"
        print "<lv1></lv1><id1>__ce_svc_upd__</id1>"
        print "<lv2></lv2><id2></id2>"
        print "<tm></tm><sev>5</sev>"
        print "<txt>VG CLEAR heartbeat</txt><com></com>"
        print "</alarm>"
    ) > $file.tmp
    mv $file.tmp $file
    ;;
run)
    if [[ $VG_SYSMODE != ems ]] then
        cat
        exit 0
    fi
    if [[ $GCEPHASEON == y ]] then
        { vggce $gargs || mv $dir1/gce.state $dir1/gce.state.tmp; } \
        | { vgce $args || mv $dir1/ce.state $dir1/ce.state.tmp; }
    else
        vgce $args || mv $dir1/ce.state $dir1/ce.state.tmp
    fi
    ;;
compact)
    if [[ $VG_SYSMODE != ems ]] then
        exit 0
    fi
    touch $dir1/ce.docompact
    ;;
flush)
    if [[ $VG_SYSMODE != ems ]] then
        exit 0
    fi
    if \
        ! vgce -flush $args < $dir1/empty.dds > /dev/null
    then
        print -u2 SWIFT ERROR cannot flush
        exit 1
    fi
    rm -f $dir1/ce.state $dir1/empty.dds
    ;;
esac
