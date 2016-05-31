#!/bin/ksh

export SWMROOT=${DOCUMENT_ROOT%/htdocs}
. $SWMROOT/bin/swmenv
[[ $KSHREC != 1 && ${KSH_VERSION##*' '} < $SHELLVERSION ]] && \
KSHREC=1 exec $SHELL $0 "$@"
. $SWMROOT/bin/swmgetinfo

argv0=${0##*/}
instance=${argv0%%_*}
rest=${argv0#$instance}
rest=${rest#_}
if [[ $instance == '' || $rest == '' ]] then
    print -u2 SWIFT ERROR program name not in correct form
    exit 1
fi
. $SWMROOT/proj/$instance/bin/${instance}_env || exit 1

wdir=$SWIFTDATADIR/cache/main/$SWMID/data.$$.$RANDOM.$RANDOM
mkdir -p $wdir
cd $wdir || {
    print -u2 ERROR cannot cd to $wdir
    exit 1
}
rm -f *

if ! sdpmpjobcntl rest ${SWIFTRESTJOBMAX:-16} || ! sdpmpstartjob; then
    print -u2 ERROR unable to start job
    exit 1
fi

typeset -l lfmt
rest=$QUERY_STRING
kind=${rest%%'/'*}
rest=${rest#"$kind"'/'}
fmt=${rest%%'/'*}
lfmt=${fmt:-xml}
rest=${rest#"$fmt"'/'}
ifs="$IFS"
IFS='&'
set -f
set -A qs -- $rest
set +f
IFS="$ifs"

if ! swmuseringroups 'vg_att_admin|vg_restdataquery'; then
    errors[${#errors[@]}]="you are not authorized to query data on this system"
fi

dqlod=n
canshowsecretattrs=n
if swmuseringroups 'vg_att_admin'; then
    dqlod=y
    canshowsecretattrs=y
fi

case $kind in
level) qid=         ;;
inv)   qid=invtab   ;;
stat)  qid=stattab  ;;
alarm) qid=alarmtab ;;
*)
    errors[${#errors[@]}]="unknown data kind $kind"
    qid=
    ;;
esac

typeset -A args
mode=default
showsecretattrs=n

for qi in "${!qs[@]}"; do
    v=${ printf '%H' "${qs[$qi]}"; }
    k=${v%%=*}
    v=${v#"$k"=}
    case $k in
    @(date|[fl]date|lod))
        typeset -n kr=$k
        kr=$v
        ;;
    poutlevel|soutlevel|showattrs|showmaps)
        typeset -n kr=$k
        kr=$v
        ;;
    showsecretattrs)
        if [[ $v == 1 && $canshowsecretattrs == y ]] then
            showsecretattrs=y
        fi
        ;;
    level*|lname*|stat*|alarm*)
        args[$k]=$v
        ;;
    *)
        warnings[${#warnings[@]}]="unknown parameter $k"
        ;;
    esac
done
args[statgroup]=${args[statgroup]:-curr}

case $lfmt in
csv)  type=text/plain       style=csv ;;
xml)  type=text/xml         style=xml ;;
json) type=application/json style=jsn ;;
*)
    errors[${#errors[@]}]="unknown format $fmt"
    type=text/html
    ;;
esac

print "Content-type: $type\n"

if (( ${#errors[@]} > 0 )) then
    case $type in
    text/xml)
        print "<response><errors>"
        for error in "${errors[@]}"; do
            print "<error>$error</error>"
        done
        print "</errors></response>"
        ;;
    application/json)
        print "{\"response\":{\"errors\":["
        sep=
        for error in "${errors[@]}"; do
            print "$sep\"$error\""
            sep=,
        done
        print "]}}"
        ;;
    text/plain)
        for error in "${errors[@]}"; do
            print "ERROR: $error"
        done
        ;;
    esac
    sdpmpendjob
    exit 0
fi

{
    havew=n
    haveh=n
    print "pid=default"
    print "name=${wdir##*/}"
    print "query=dataquery"
    print "qid=$qid"
    [[ $lod != '' ]] && print "lod=$lod"

    if [[ $fdate != '' ]] then
        print "fdate=$fdate"
        [[ $ldate != '' ]] && print "ldate=$ldate"
    elif [[ $date != '' ]] then
        print "date=$date"
    else
        print "date=latest"
    fi

    for k in "${!args[@]}"; do
        print -r "$k=${args[$k]}"
        [[ $k == winw=* ]] && havew=y
        [[ $k == winh=* ]] && haveh=y
    done
    [[ $havew == n ]] && print "winw=1280"
    [[ $haveh == n ]] && print "winh=1024"
} > vars.lst

tsep=''
case $lfmt in
csv)                          ;;
xml)  print "<response>"      ;;
json) print "{\"response\":{" ;;
esac
if (( ${#warnings[@]} > 0 )) then
    case $type in
    text/xml)
        print "<warnings>"
        for warning in "${warnings[@]}"; do
            print "<warning>$warning</warning>"
        done
        print "</warnings>"
        ;;
    application/json)
        print "\"warnings\":["
        sep=
        for warning in "${warnings[@]}"; do
            print "$sep\"$warning\""
            sep=,
        done
        print "]"
        tsep=,
        ;;
    text/plain)
        for warning in "${warnings[@]}"; do
            print "WARNING: $warning"
        done
        ;;
    esac
fi

export DQVTOOLS=n DQSRC=rest DQMODE=$mode DQLOD=$dqlod
case $kind in
level)
    file=$SWIFTDATADIR/data/main/latest/processed/total/LEVELS-nodes.dds
    MODE=$style ddsprint -pso vg_rest_data_level.printer.so $file
    ;;
inv)
    export SHOWATTRS=${showattrs:-0} SHOWMAPS=${showmaps:-0}
    export POUTLEVEL=${poutlevel:-o} SOUTLEVEL=${soutlevel}
    SAVEDATA=y $SHELL vg_dq_run < vars.lst > out
    if [[ -f iedge.dds ]] then
        file=iedge.dds
    else
        file=inode.dds
    fi
    print "$tsep"
    . ./dq_main_data.sh
    rdir=${dq_main_data.rdir}
    export INVEDGEATTRFILE=$rdir/inv-edges.dds
    export INVNODEATTRFILE=$rdir/inv-nodes.dds
    export INVMAPFILE=$rdir/inv-maps-uniq.dds
    if [[ $showsecretattrs != y ]] then
        export HIDEATTRRE='(scope_user|scope_pass|scope_snmpcommunity)'
    else
        unset HIDEATTRRE
    fi
    MODE=$style ddsprint -pso vg_rest_data_inv.printer.so $file
    ;;
stat)
    $SHELL vg_dq_run < vars.lst > out
    for file in stat.dds stat_[0-9]*.dds; do
        [[ ! -f $file ]] && continue
        print "$tsep"
        MODE=$style ddsprint -pso vg_rest_data_stat.printer.so $file
    done
    ;;
alarm)
    $SHELL vg_dq_run < vars.lst > out
    for file in alarm.dds; do
        [[ ! -f $file ]] && continue
        print "$tsep"
        MODE=$style ddsprint -pso vg_rest_data_alarm.printer.so $file
    done
    ;;
esac

case $lfmt in
csv)                      ;;
xml)  print "</response>" ;;
json) print "}}"          ;;
esac

sdpmpendjob
