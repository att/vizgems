#!/bin/ksh

rmode=view
title="GMAPGenerator"

x=640
y=480

defmap='p 0 0 c black_0 black_0 0 100 h 0 3 c green green'
defcolor=gray
defbggeom=border_polys.multi
defbgcolor=gray

function cnvmap {
    integer count

    count=0
    while (( $# > 0 )) do
        case $1 in
        p) print "'percent' = 'total';"; shift ;;
        c) print "'minc' = '${2//_/ }'; 'maxc' = '${3//_/ }';"; shift 3 ;;
        h) print "'minh' = $2; 'maxh' = $3;"; shift 3 ;;
        *)
            if (( count > 0 )) then
                print "];"
            fi
            print "$count = ["
            print "'minv' = $1; 'maxv' = $2;"
            (( count++ ))
            shift 2
            ;;
        esac
    done
    if (( count > 0 )) then
        print "];"
    fi
}

tmp=/tmp/gmapgen
tmp=bar
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
    -size)
        x=${2%,*}; y=${2#*,}
        shift 2
        ;;
    -bggeom)
        bggeoms="$bggeoms ${2%:*}"
        if [[ ${2%:*} != "" ]] then
            bgcolors="$bgcolors ${2##*:}"
        else
            bgcolors="$bgcolors $defbgcolor"
        fi
        shift 2
        ;;
    color=*)
        colors[$count]=${1##*=}
        shift
        ;;
    style=*)
        styles[$count]=${1##*=}
        shift
        ;;
    geom=*)
        geoms[$count]=${1##*=}
        shift
        ;;
    map=*)
        maps[$count]=${1##*=}
        shift
        ;;
    *)
        framen=$(aggrprint -V -V -nodata $1 | egrep framen)
        framen=${framen##* }
        if [[ $framen != 1 ]] then
            class=$(aggrprint -V -V -nodata $1 | egrep class)
            class=${class##*: }
            tmpfile=$tmp.$RANDOM.$RANDOM
            aggrsum -cl $class -frames -o $tmpfile $1
            tmpfiles="$tmpfiles $tmpfile"
            files[$count]=$tmpfile
        else
            files[$count]=$1
        fi
        if [[ ${styles[$count]} == "" ]] then
            styles[$count]=lines
        fi
        if [[ ${maps[$count]} == "" ]] then
            maps[$count]=$defmap
        fi
        if [[ ${colors[$count]} == "" ]] then
            colors[$count]=$defcolor
        fi
        if [[ ${geoms[$count]} == "" ]] then
            class=$(aggrprint -V -V -nodata ${files[$count]} | egrep class)
            class=${class##*: }
            geom=${class}_${styles}.single
            if [[ $class == npanxxloc ]] then
                for i in ${PATH//:/ }; do
                    if [[ -f $i/../lib/geoms/$geom ]] then
                        geoms[$count]=$geom
                        break
                    fi
                done
                if [[ ${geoms[$count]} == "" ]] then
                    geoms[$count]=/fs/swift/lerg/daily/latest/$geom
                fi
            else
                geoms[$count]=$geom
            fi
        fi
        (( count++ ))
        shift
        ;;
    -?)
        print "usage: gmapgen [flags] <var1=val1> ... <file1> ..."
        print "    flags:"
        print "        -o <gif file>"
        print "        -size <x>,<y>"
        print "    variables:"
        print "        [(geom=<geom>|style=<style>|map=<map>]"
        print "    example:"
        print "        gmapgen -o a.jpg map='$defmap' a.aggr"
        exit
        ;;
    esac
done

if [[ $rmode == save ]] then
    colorscheme=paper
    largs=-x
    if [[ -f lefty3d.0000.rgb ]] then
        print -u2 moving lefty3d.0000.rgb to lefty3d.0000.rgb.old
        mv lefty3d.0000.rgb lefty3d.0000.rgb.old
    fi
else
    colorscheme=screen
    largs=
fi

if [[ $bggeoms == '' ]] then
    bggeoms=$defbggeom
    bgcolors=$defbgcolor
fi

(
    IFS=$ifs
    print "
    txtview ('off');
    load ('gmap2l.lefty');
    gmap2l.init ();
    gmap2l.main ();
    gmap2l.viewattr.$colorscheme.size=['x' = $x; 'y' = $y; ];
    viewt = gmap2l.createview (gmap2l.viewattr.$colorscheme);
    gmap2l.createchannel (viewt, gmap2l.viewattr.$colorscheme);
    "
    eval set -A glist ${bggeoms}
    eval set -A clist ${bgcolors}
    for (( count = 0; count < ${#glist[@]}; count++ )) do
        print "
        geomt = gmap2l.loadgeom ('${glist[$count]}', [
            'pickmask' = 1; 'color' = '${clist[$count]//_/ }';
        ]);
        gmap2l.attachgeom2view (viewt, geomt);
        "
        if [[ $count == 0 ]] then
            print "
            gmap2l.seteye (viewt, 'setreset', '', gmap2l.bbox2eye (geomt.bbox));
            "
        fi
    done

    for (( count = 0; ; count++ )) do
        if [[ ${files[count]} == "" ]] then
            break
        fi
        print "
            load = function () {
                if (~(geom1t = gmap2l.loadgeom ('${geoms[$count]}', [
                    'pickmask' = 0; 'color' = '${colors[$count]}';
                ])))
                    exit ();
                gmap2l.attachgeom2view (viewt, geom1t);
                if (~(val1t = gmap2l.loadval ('${files[$count]}')))
                    exit ();
            };
            load ();
            val2geom1t = gmap2l.mapval2geom (val1t, geom1t, [
                'map' = [
        "
        eval cnvmap ${maps[$count]}
        print "];"
        print "]);"
        if [[ ${styles[$count]} == lines ]] then
            print "
                gmap2l.seteye (viewt, 'tilt', 1, [
                    'angxy' = 0; 'angz' = 50;
                ]);
            "
        fi
    done
    if [[ $rmode == save ]] then
        print "gmap2l.snap (viewt);"
    fi
) | lefty3d $largs -

if [[ $rmode == save ]] then
    mv lefty3d.0000.rgb $outfile
fi
