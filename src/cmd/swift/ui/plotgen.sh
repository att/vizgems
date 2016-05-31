#!/bin/ksh

rmode=view
cmode=color
title="PlotGenerator"

typeset -A cmap
cmap["white"]=7
cmap["cyan"]=3
cmap["red"]=4
cmap["green"]=2
cmap["blue"]=1

typeset -A ticks
ticks[tenmin]='"00:00" 0, "02:00" 12, "04:00" 24, "06:00" 36, \
    "08:00" 48, "10:00" 60, "12:00" 72, "14:00" 84, "16:00" 96, \
    "18:00" 108, "20:00" 120, "22:00" 132 \
'

function process {
    if [[ $rmode == view ]] then
        gnuplot
    else
        gnuplot | ppmtogif > $outfile
    fi
}

tmp=/tmp/plotgen
tmpfiles=
trap '[[ $tmpfiles != "" ]] && eval rm $tmpfiles' HUP QUIT TERM KILL EXIT

count=0
while (( $# > 0 )) do
    case $1 in
    -o)
        outfile=$2
        rmode=save
        shift 2
        ;;
    -title)
        title=$2
        shift 2
        ;;
    -item)
        onlyitem=$2
        shift 2
        ;;
    -key)
        onlykey=$2
        shift 2
        ;;
    -xticks)
        if [[ ${ticks[$2]} != '' ]] then
            xticks=${ticks[$2]}
        else
            xticks=$2
        fi
        shift 2
        ;;
    -yscale)
        yscale=$2
        shift 2
        ;;
    -cmode)
        cmode=$2
        shift 2
        ;;
    legend=*)
        legends[$count]=${1##*=}
        shift
        ;;
    color=*)
        colors[$count]=${1##*=}
        cid=$((count + 2))
        export GNUPLOT_COLOR_$cid=${cmap[${1##*=}]}
        shift
        ;;
    style=*)
        styles[$count]=${1##*=}
        shift
        ;;
    *)
        if [[ $onlykey != '' ]] then
            tmpfile=$tmp.$RANDOM.$RANDOM
            aggrprint -key $onlykey $1 > $tmpfile
            tmpfiles="$tmpfiles $tmpfile"
        elif [[ $onlyitem != '' ]] then
            tmpfile=$tmp.$RANDOM.$RANDOM
            aggrprint -item $onlyitem $1 > $tmpfile
            tmpfiles="$tmpfiles $tmpfile"
        else
            tmpfile1=$tmp.$RANDOM.$RANDOM
            tmpfiles="$tmpfiles $tmpfile1"
            aggrsum -items -o $tmpfile1 $1
            tmpfile=$tmp.$RANDOM.$RANDOM
            aggrprint $tmpfile1 > $tmpfile
            tmpfiles="$tmpfiles $tmpfile"
        fi
        if [[ $yscale != '' ]] then
            tmpfile1=$tmpfile
            tmpfile=$tmp.$RANDOM.$RANDOM
            tmpfiles="$tmpfiles $tmpfile"
            awk "{ print \$1 / $yscale }" $tmpfile1 > $tmpfile
        fi
        export maxvalues[$count]=$(sort -n $tmpfile | tail -1)
        files[$count]=$tmpfile
        (( count++ ))
        shift
        ;;
    -?)
        print "usage: plotgen [flags] <var1=val1> ... <file1> ..."
        print "    flags:"
        print "        -o <gif file>"
        print "        -title <title text>"
        print "        -item <item number>"
        print "        -key <key>"
        print "        -xticks '<text1> <pos1>, ...'"
        print "        -yscale <scale>"
        print "        -cmode <mono|color>"
        print "    variables:"
        print "        [(color=<color>|style=<style>|legend=<text>]"
        print "    example:"
        print "        plotgen -o a.gif -title "ABC" \\"
        print "            legend='123' color=green style=lines a.aggr"
        exit
        ;;
    esac
done
export MAXVALUE=$maxvalues
for v in "${maxvalues[@]}"; do
    if (( MAXVALUE < v )) then
        MAXVALUE=$v
    fi
done

if [[ $rmode == save ]] then
    term="pbm $cmode"
else
    term=x11
fi

(
    print "set terminal $term"
    print "set title \"$(eval print $title)\""
    print "set tics out"
    if [[ $xticks != '' ]] then
        print "set xtics ($xticks)"
    fi
    print 'set format y "%.0f"'
    if [[ $rmode == save ]] then
        print 'set size 0.9375,0.3125'
    fi
    for (( count = 0; ; count++ )) do
        if [[ ${files[count]} == "" ]] then
            print ""
            break
        fi
        if (( count != 0 )) then
            print ", \c"
        else
            print "plot \c"
        fi
        MAXVALUE=${maxvalues[$count]}
        print "\"${files[$count]}\" \c"
        print "title \"$(eval print ${legends[$count]})\" \c"
        if [[ ${styles[$count]} != '' ]] then
            print "with ${styles[$count]}\c"
        fi
    done
    if [[ $rmode == view ]] then
        read a?'Press Enter to Exit '
    fi
) | process
