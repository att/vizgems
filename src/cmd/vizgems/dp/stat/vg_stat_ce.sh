
if [[ $SWIFTWARNLEVEL != '' ]] then
    set -x
fi

mode=$1
shift

dir=${PWD%/processed/*}
dir=${dir##*main/}
date=${dir//\//.}

dir1=$VGMAINDIR/dpfiles/stat

if [[ ! -d $dir1 ]] then
    print -u2 SWIFT ERROR cannot find correlation directory $dir1
    exit 1
fi

if [[ ! -f $dir1/profile.txt ]] then
    print -u2 SWIFT ERROR cannot find correlation rules $dir1/profile.txt
    exit 1
fi

args="-date $date -af $dir1/pce.rules -pf $dir1/pce.files -sf $dir1/pce.state"

case $mode in
check)
    if [[ $VG_SYSMODE != ems ]] then
        exit 0
    fi
    if [[ $PCEPHASEON == y ]] then
        if [[
            -f $dir1/pce.rules.tmp || -f $dir1/pce.files.tmp ||
            -f $dir1/pce.state.tmp
        ]] then
            print -u2 SWIFT WARNING previous invocation of pce ended abnormally
        fi
        if [[
            ! -f $dir1/pce.rules || inv-nodes.dds -nt $dir1/pce.rules ||
            $dir1/profile.txt -nt $dir1/pce.rules
        ]] then
            export PROFILEFILE=$dir1/profile.txt
            export DEFAULTFILE=$dir1/parameter.txt
            export ACCOUNTFILE=$dir1/../account.filter
            export LEVELMAPFILE=LEVELS-maps.dds
            export INVNODEATTRFILE=inv-nodes.dds
            export INVMAPFILE=inv-maps-uniq.dds
            if [[ -f $LEVELMAPFILE && -f $INVMAPFILE ]] then
                ddsfilter -osm none -fso vg_inv_profile.filter.so \
                    inv-nodes-uniq.dds \
                | ddsload -os vg_profile.schema -le '|' -type ascii \
                    -lnts -lso vg_profile.load.so \
                | ddssort -fast -ke 'level id key' > $dir1/pce.rules.tmp && \
                mv $dir1/pce.rules.tmp $dir1/pce.rules
            elif [[ ! -f $dir1/pce.rules ]] then
                print -u2 MESSAGE waiting for inventory files
                ddscat -is vg_prof_alarm_rule.schema > $dir1/pce.rules.tmp && \
                mv $dir1/pce.rules.tmp $dir1/pce.rules
            fi
        fi
        if [[
            ! -f $dir1/pce.files || ! -s $dir1/pce.files ||
            $VGMAINDIR/dpfiles/config.sh -nt $dir1/pce.files
        ]] then
            (( filen = (24 * 60 * 60) / STATINTERVAL ))
            typeset -Z2 hh
            for (( i = 0; i < filen; i++ )) do
                (( hh = i / 12 ))
                print -n "$i|../avg/total/mean.$hh.stats.dds"
                print "|../avg/total/sdev.$hh.stats.dds"
            done > $dir1/pce.files.tmp && \
            mv $dir1/pce.files.tmp $dir1/pce.files
        fi
        if [[ ! -f $dir1/pce.state ]] then
            touch $dir1/pce.state
        fi
    fi
    ;;
run)
    if [[ $VG_SYSMODE != ems ]] then
        cat > /dev/null
        exit 0
    fi
    if [[ $PCEPHASEON == y ]] then
        vgpce $args
    fi
    ;;
esac
