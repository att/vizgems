#!/bin/ksh

export SWMROOT=${DOCUMENT_ROOT%/htdocs}
. $SWMROOT/bin/swmenv
[[ $SECONDS != *.* ]] && exec $SHELL $0 "$@"
. $SWMROOT/bin/swmgetinfo

argv0=${0##*/}
instance=${argv0%%_*}
rest=${argv0#$instance}
rest=${rest#_}
if [[ $instance == '' || $rest == '' ]] then
    print -u2 ERROR program name not in correct form
    exit 1
fi
. $SWMROOT/proj/$instance/bin/${instance}_env || exit 1

suireadcgikv

if [[ $qs_prefprefix == '' ]] then
    print -u2 "ERROR configuration not specified"
    exit 1
fi
. $SWIFTDATADIR$qs_prefprefix.sh

typeset -n x=$instance.dserver.default.transmission
case ${x} in
Uncompressed) dtype=u ;;
Compressed) dtype=c ;;
esac
typeset +n x

${instance}_services start
${instance}_services list r | read j1 j2 j3 rport j4
${instance}_services list d-$dtype | read j1 j2 j3 dport j4

tools2launch=0
for tool in $tools; do
    lang=${tool#*:}
    tool=${tool%:*}
    if [[ $lang == java || $lang == nolaunch || $lang == none ]] then
        continue
    fi
    tools2launch=1
done

hname=$(swmhostname)
if [[ $SWMADDRMAPFUNC != '' ]] then
    mhname=$($SWMADDRMAPFUNC)
    if [[ $mhname == '' ]] then
        mhname=$hname
    fi
else
    mhname=$hname
fi

dir=$SWMROOT/${qs_dir}
rkey=${qs_sessionid##*\;}
mname=${qs_sessionid%%\;*}
mport=${qs_sessionid#*\;}
mport=${mport%%\;*}

if [[ $SWIFTWEBMODE == secure ]] then
    mname=$mhname
    mport=
    secarg="-ipdir $SWMROOT/ipdir"
    ipfile=$SWMROOT/ipdir/$REMOTE_ADDR
else
    secarg=
    ipfile=/dev/null
fi

if [[ $tools2launch != 1 && $mname == "" ]] then
    mname=$mhname
fi
if [[ $mname == $hname ]] then
    mname=$mhname
fi
if [[ $mname == '' ]] then
    tools2launch=1
elif [[ $mport == '' && $mname == $mhname ]] then
    mport=$(
        suimserver $secarg -regname $hname/$rport -regkey $rkey \
        3> $ipfile \
        | egrep port | sed 's/^.* //'
    )
    $SHELL suimhistory \
        $instance $mname $mport $mhname $dport \
        $qs_prefprefix $dir \
    < /dev/null > /dev/null 2> /dev/null &
fi

case $tools2launch in
1)
    print "Content-type: application/swift-cmd\n"
    print "begin"
    print "cmd=launch"
    print "instance=$instance"
    print "instancename=$SWIFTINSTANCENAME"
    print "tools=$tools"
    print "rname=$mhname"
    print "rport=$rport"
    print "rkey=$rkey"
    print "dname=$mhname"
    print "dport=$dport"
    print "webserver=$SWIFTWEBSERVER"
    print "mname=$mname"
    print "mport=$mport"
    print "prefprefix=$qs_prefprefix"
    print "end"
    ;;
*)
    print "Content-type: text/html\n"
    print "<html></html>"
    ;;
esac
