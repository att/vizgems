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

print "Content-type: text/html\n"
print "<html>"
print "<head></head>"

if [[ $qs_mname == '' || $qs_mport == '' ]] then
    exit 1
fi

if [[ $SWMADDRMAPFUNC != '' ]] then
    hname=$($SWMADDRMAPFUNC -rev)
    if [[ $hname != '' ]] then
        qs_mname=$hname
    fi
fi

(
    print "primaryoff"
    print "begin"
    for qsitem in $qslist; do
        typeset -n qsn=qs_$qsitem
        if (( ${#qsn[@]} > 1 )) || [[ " $qsarrays " == *\ $qsitem\ * ]] then
            print -r "$qsitem=["
            for i in "${qsn[@]}"; do
                print -r -- "$i"
            done
            print -r -- ']'
        else
            print -r -- "$qsitem=$qsn"
        fi
        typeset +n qsn
    done
    print "end"
) | suimclient $qs_mname/$qs_mport

print "</body>"
print "</html>"
