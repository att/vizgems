#!/bin/ksh

function showusage {
    print usage: procviewer
}

while (( $# > 0 )) do
    if [[ $1 == -ps ]] then
        if (( $# < 2 )) then
            showusage
            exit
        fi
        psargs=$2
        shift; shift
    else
        break
    fi
done

sleep=${1:-5}

typeset -A cmap
cmap[0]=gray0
cmap[1]=gray10
cmap[2]=gray20
cmap[3]=gray30
cmap[4]=gray40
cmap[5]=gray50
cmap[6]=gray60
cmap[7]=gray70
cmap[8]=gray80
cmap[9]=gray90
cmap[10]=white
typeset -A fcmap
fcmap[0]=white
fcmap[1]=white
fcmap[2]=white
fcmap[3]=white
fcmap[4]=white
fcmap[5]=black
fcmap[6]=black
fcmap[7]=black
fcmap[8]=black
fcmap[9]=black
fcmap[10]=black

export VPMPID=$$

while sleep $sleep; do
    (
        print "digraph a {"
        print "rankdir = LR"
        print "edge [ len = 2 ]"
        print 'node [ shape=box style=filled ]'
        ps -h $psargs \
            --format 'X %(pid)d %(ppid)d %(rss)d %(start)s %(time)d %(args)s' \
        | egrep ^X | while read junk pid ppid rss start time args; do
            args=${args:0:100}
            if [[ $time != +([0-9]) ]] then
                continue
            fi
            # until ast_ps fixed %(start)d
            if [[ $start == *-*-* ]] then
                start=$(date -s -f '%#' $start-00:00:00)
            else
                start=$(date -s -f '%#' $start)
            fi
            (( dt = $(date -s -f '%#') - start ))
            label=
            args=${args//\"/\'}
            n=${#args}
            while (( n > 20 )) do
                label="${label}${args:0:20}\n"
                args=${args:20}
                n=${#args}
            done
            label="${label}${args}"
            color=black
            fontcolor=white
            if (( (colori = dt / 60) > 10 )) then
                colori=10
            elif (( colori < 0 )) then
                colori=0
            fi
            color=${cmap[$colori]}
            fontcolor=${fcmap[$colori]}
            if (( rss > 1024 * 256 )) then
                color=red
                fontcolor=black
            fi
            if (( time > 10 )) then
                color=red
                fontcolor=black
            fi
            printf \
                '%s [label="%s\\n%s\\n%dk/u %ds/e %ds" %s]\n' \
                $pid "${label//\"/\'}" "$pid" "$rss" "$time" "$dt" \
            "color=$color fontcolor=$fontcolor"
            printf '%s -> %s\n' $ppid $pid
        done
        print "}"
    ) | ${VPMLAYOUT:-twopi} -Txdot | {
        read line
        print read graph
        print -r -- $line
        cat
    }
done | dotty -f vpm.lefty -lt ${VPMLAYOUT:-twopi}
