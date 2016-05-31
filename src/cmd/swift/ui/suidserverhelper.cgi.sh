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

pgrpfile=$SWMROOT/tmp/pgrp.$$
> $pgrpfile
trap '/bin/kill -15 -$(< $pgrpfile); exit 0' PIPE TERM

if [[ $QUERY_STRING == *_ct_=xml* ]] then
    print "Content-type: text/xml\n"
else
    print "Content-type: text/html\n"
fi

print -r "$QUERY_STRING" \
| suinewpgrp -log $pgrpfile ${instance}_dserverhelper \
| while true; do
    SECONDS=0; line=; read -t5 line; sec=$SECONDS
    if [[ $line == status:* ]] then
        if [[ $senthdr == '' ]] then
            senthdr=1
            print "<html><head><script>"
            print "var a = 0"
            print "function f () {"
            print "  if (a == 0)"
            print "    window.stop ()"
            print "  else"
            print "    window.close ()"
            print "}"
            print "</script>"
            print "</head><body>"
        fi
        print "<script>window.status='$line'</script>"
        pline=$line
    elif [[ $line == "" ]] then
        print "<script>window.status='$pline'</script>"
    else
        if [[ $senthdr == 1 ]] then
            print "</body></html>"
        fi
        /bin/cat $line 2> /dev/null || cat $line
        break
    fi
    if (( $sec < 4 )) then
        if [[ $line == "" ]] then
            break
        fi
    fi
done
rm -f $pgrpfile
