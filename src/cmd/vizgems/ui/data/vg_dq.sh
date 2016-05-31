#!/bin/ksh

usage=$'
[-1p1?
@(#)$Id: vg_dq (AT&T Labs Research) 2009-02-18 $
]
'$USAGE_LICENSE$'
[+NAME?vg_dq - command line tool for running a data query]
[+DESCRIPTION?\bvg_dq\b runs a data query.
It can run in several modes, to produce html pages and images
or simple CSV files.
]
[100:mode?specifies the output mode for the query.
\bnone\b runs the query and generates the results in the default mode.
\bembed\b outputs html that can be embeded in another page.
]:[(default|embed)]
[101:user?specifies the user id to run the query under.
The default id uses the main system account id.
]:[userid]
[200:date?specifies a single date for the query.
it is interpreted as the full 24 hours of that day.
]:[date]
[201:fdate?specifies the begin date for the query.
]:[date]
[202:ldate?specifies the end date for the query.
]:[date]
[210:alarmgroup?specifies the group of alarms to extract.
]:[(open|all)]
[211:statlod?specifies the level of detail
for the statistical piece of the query.
]:[(ymd|ym|y)]
[220:qid?specifies the query id.
]:[qid]
[230:exp?specifies an export format to use.
The default is not to export to any format.
]:[(csv|xml|zip)]
[231:raw?specifies that no display tools should be ran but instead the data
extracted by the query (inv, stat, alarm) will be
printed out as pipe-delimited records.
]
[232:remote?specifies that the query is from a remote VizGEMS system.
All the parameters are read from stdin.
The display tools are not executed.
The data results are packaged in a .pax file and printed on stdout.
]
[999:v?increases the verbosity level. May be specified multiple times.]

level_o=abc level_c=ABC ...

'

function showusage {
    OPTIND=0
    getopts -a vg_dq "$usage" opt '-?'
}

mode=default

while getopts -a vg_dq "$usage" opt; do
    case $opt in
    100) mode=$OPTARG ;;
    101) SWMID=$OPTARG ;;
    200) date=$OPTARG ;;
    201) fdate=$OPTARG ;;
    202) ldate=$OPTARG ;;
    210) ag=$OPTARG ;;
    211) lod=$OPTARG ;;
    220) qid=$OPTARG ;;
    230) exp=$OPTARG ;;
    231) raw=y ;;
    232) remote=y; export REMOTEQUERY=y ;;
    999) (( SWIFTWARNLEVEL++ )) ;;
    *) showusage; exit 1 ;;
    esac
done
shift $OPTIND-1

if [[ $SWIFTDATADIR == '' ]] then
    print -u2 ERROR SWIFTDATADIR not valid
    exit 1
fi

if [[ $remote != y && $qid == '' ]] then
    print -u2 ERROR qid not defined
    exit 1
fi

if [[ $SWMID == '' ]] then
    print -u2 ERROR SWMID not defined
    exit 1
fi
SWMIDOVERRIDE=$SWMID
. $SWMROOT/bin/swmgetinfo

wdir=$SWIFTDATADIR/cache/main/$SWMID/data.$$.$RANDOM.$RANDOM
mkdir -p $wdir
cd $wdir || {
    print -u2 ERROR cannot cd to $wdir
    exit 1
}
rm -f *

{
    havew=n
    haveh=n
    print "pid=default"
    print "name=${wdir##*/}"
    print "query=dataquery"

    if [[ $remote == y ]] then
        cat
    else
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

        for i in "$@"; do
            print -r "$i"
            [[ $i == winw=* ]] && havew=y
            [[ $i == winh=* ]] && haveh=y
        done
        [[ $havew == n ]] && print "winw=1280"
        [[ $haveh == n ]] && print "winh=1024"
    fi
} > vars.lst

[[ $raw != '' || $remote == y ]] && export DQVTOOLS=n

DQMODE=$mode DQLOD=y $SHELL vg_dq_run < vars.lst > rep.out
if [[ $exp != '' && $remote != y ]] then
    $SHELL vg_export_save $exp exp ""
fi

[[ $remote != y ]] && print data directory: $wdir

if [[ $remote == y ]] then
    rm inv.dds
    pax -wf - . 2> /dev/null
elif [[ $raw != '' ]] then
    for file in alarm.dds stat.dds stat_[0-9]*.dds; do
        [[ ! -f $file ]] && continue
        ddspp $file
    done
fi
