
#
# args = (
#   type, name, index, width, height, color
#   areas[] = ()
# )
#

function vg_dq_vt_toolhelper { # $1 = args
    typeset -n as=$1

    typeset name=${as.name} index=${as.index}
    typeset iw=${as.width} ih=${as.height} bg=${as.backgroundcolor}

    typeset s

    if [[
        $name == '' || $index == '' || $iw == '' || $ih == '' || $bg == ''
    ]] then
        print -u2 SWIFT ERROR vg_dq_vt_toolhelper - incomplete spec
        return 1
    fi

    (
        case ${as.type} in
        image)
            imagebegin "$name" "$index" "$iw" "$ih" "$bg"
            ;;
        table)
            tablebegin "$name" "$index" "$iw" "$ih" "$bg"
            ;;
        esac

        for (( areai = 0; areai < ${#as.areas[@]}; areai++ )) do
            typeset -n arear=as.areas[$areai]
            draw${arear.tool} $1.areas[$areai]
        done

        case ${as.type} in
        image)
            imageend
            ;;
        table)
            tableend
            ;;
        esac
    ) | vg_dq_vt_tool > /dev/null
    case ${as.type} in
    image)
        if [[ -f $name.$index.gif ]] then
            s=
            if [[ -f $name.$index.imginfo ]] then
                set -A s $(< $name.$index.imginfo)
                if ((
                    s[0] > 0 && s[0] < 10000 && s[1] > 0 && s[1] < 10000
                )) then
                    s[0]="width=${s[0]}" s[1]="height=${s[1]}"
                fi
            fi
            s[${#s[@]}]='class=page'
            if [[ -f $name.$index.cmap ]] then
                print -n "<img"
                print -n " ${s[@]} src='$IMAGEPREFIX$name.$index.gif'"
                print -n " usemap='#$name.$index'"
                print ">"
                print "<map name='$name.$index'>"
                cat $name.$index.cmap
                print "</map>"
            else
                print "<img ${s[@]} src='$IMAGEPREFIX$name.$index.gif'>"
            fi
        fi > $name.$index.html.tmp && mv $name.$index.html.tmp $name.$index.html
        [[ -f $name.$index.html ]] && print $name.$index.html
        ;;
    table)
        [[ -f $name.$index.toc ]] && print $name.$index.toc
        [[ -f $name.$index.css ]] && print $name.$index.css
        [[ -f $name.$index.html ]] && print $name.$index.html
        ;;
    esac
}

function imagebegin {
    typeset name=$1 index=$2 iw=$3 ih=$4 bg=$5

    print "open image $name $index $iw $ih"
    print "box 0 0 $iw $ih 1 true $bg"
}

function imageend {
    print "close"
}

function tablebegin {
    typeset name=$1 index=$2 iw=$3 ih=$4 bg=$5

    print "open table $name $index $iw $ih"
    print "box 0 0 $iw $ih 1 true $bg"
}

function tableend {
    print "close"
}

# args = (
#   x, y, w, h
#   minv, maxv, unit, fontname, fontsize, fontcolor
#   legend = (
#     label, fontname, fontsize, fontcolor, object, url, info
#   )
#   dial = (
#     outlinecolor, backgroundcolor, color
#     ranges[] = (
#       minv, maxv, color
#     )
#   )
#   needles[] = (
#     v, color, label, fontname, fontsize, fontcolor, object, url, info
#   )
# )

function drawmeter {
    typeset -n as=$1

    integer ix=${as.x} iy=${as.y} iw=${as.w} ih=${as.h}
    float minv=${as.minv} maxv=${as.maxv}
    typeset unit=${as.unit} outlinecolor=${as.dial.outlinecolor}
    typeset backgroundcolor=${as.dial.backgroundcolor} color=${as.dial.color}

    float pi=3.14159265358979323846
    integer legx=$ix legy=$(( iy + ih - ih / 5 )) legw=$iw legh=$(( ih / 5 ))
    integer labw=$(( iw / 5 )) labh=$(( ih / 5 ))

    integer dialx=$(( ix + labw )) dialy=$(( iy + labh ))
    integer dialw=$(( iw - 2 * labw )) dialh=$(( ih - ih / 5  - 2 * labh ))

    integer cx=$(( dialx + dialw / 2.0 )) cy=$(( dialy + dialh / 2.0 ))
    integer outl=$(( dialw  / 2.0 ))
    integer inl=$((  dialw / 2.0 - dialw / 8 ))
    integer midl=$(( dialw / 2.0 - dialw / 16 ))

    typeset rangei needlei lab fn fs fc obj url info
    float r1 ang1 r2 ang2
    integer deg1 deg2 x1 y1 lx1 ly1 lx2 ly2

    for (( rangei = 0; rangei < ${#as.dial.ranges[@]}; rangei++ )) do
        typeset -n ranger=as.dial.ranges[$rangei]

        (( r1 = (${ranger.minv} - minv) / (maxv - minv) ))
        (( ang1 = (pi * 5.0 / 3.0) * (1.0 - r1) - pi / 3.0 ))
        (( deg1 = 360 - 180 * ang1 / pi ))
        (( r2 = (${ranger.maxv} - minv) / (maxv - minv) ))
        (( ang2 = (pi * 5.0 / 3.0) * (1.0 - r2) - pi / 3.0 ))
        (( deg2 = 360 - 180 * ang2 / pi ))
        print "ellipse $cx $cy $outl $outl $deg1 $deg2 1 true ${ranger.color}"
    done

    (( ang1 = (pi * 5.0 / 3.0) * (1.0 - 0) - pi / 3.0 ))
    (( ang2 = (pi * 5.0 / 3.0) * (1.0 - 1) - pi / 3.0 ))
    (( deg1 = 360 - 180 * ang1 / pi ))
    (( deg2 = 360 - 180 * ang2 / pi ))
    print "ellipse $cx $cy $inl $inl 0 360 1 true $backgroundcolor"
    print "ellipse $cx $cy $outl $outl $deg1 $deg2 1 false $outlinecolor"
    print "ellipse $cx $cy $inl $inl $deg1 $deg2 1 false $outlinecolor"

    for (( needlei = 0; needlei < ${#as.needles[@]}; needlei++ )) do
        typeset -n needler=as.needles[$needlei]

        (( r1 = (${needler.v} - minv) / (maxv - minv) ))
        (( ang1 = (pi * 5.0 / 3.0) * (1.0 - r1) - pi / 3.0 ))
        (( x1 = cx + midl * cos(2 * pi + ang1) ))
        (( y1 = cy + midl * sin(2 * pi + ang1) ))
        print "polyline $cx $cy $x1 $y1 2 ${needler.color}"
        (( x1 = cx + outl * cos(2 * pi + ang1) ))
        (( y1 = cy + outl * sin(2 * pi + ang1) ))
        if (( x1 < cx )) then
            (( lx1 = x1 - labw )); (( lx2 = x1 ))
        else
            (( lx1 = x1 )); (( lx2 = x1 + labw ))
        fi
        if (( y1 < cy )) then
            (( ly1 = y1 - labh )); (( ly2 = y1 ))
        else
            (( ly1 = y1 )); (( ly2 = y1 + labh ))
        fi
        lab="${needler.v}$unit\\\n${needler.label}"
        fn=${needler.fontname}
        fs=${needler.fontsize}
        fc=${needler.fontcolor}
        print -r "text '$lab' $fn $fs $lx1 $ly1 $lx2 $ly2 0 0 true $fc"
        obj=${needler.object}
        url=${needler.url}
        info=${needler.info}
        if [[ $url != '' ]] then
            print -r "area $lx1 $ly1 $lx2 $ly2 '$obj' '$url' '$info'"
        fi
    done
    print "ellipse $cx $cy 4 4 0 360 1 true $color"

    lx1=$(( dialx + dialw / 6.0 )) ly1=$(( dialy + dialh / 2.0 - outl ))
    lx2=$(( dialx + dialw * 5.0 / 6.0 )) ly2=$(( dialy + dialh / 2.0 - inl ))
    lab="$minv-$maxv $unit"
    fn=${as.fontname}
    fs=${as.fontsize}
    fc=${as.fontcolor}
    print -r "text '$lab' $fn $fs $lx1 $ly1 $lx2 $ly2 0 -1 true $fc"

    lx1=$(( legx )) ly1=$(( legy ))
    lx2=$(( legx + legw )) ly2=$(( legy + legh ))
    lab="${as.legend.label}"
    fn=${as.legend.fontname}
    fs=${as.legend.fontsize}
    fc=${as.legend.fontcolor}
    print -r "text '$lab' $fn $fs $lx1 $ly1 $lx2 $ly2 0 0 true $fc"
    obj=${as.legend.object}
    url=${as.legend.url}
    info=${as.legend.info}
    if [[ $url != '' ]] then
        print -r "area $lx1 $ly1 $lx2 $ly2 '$obj' '$url' '$info'"
    fi

    return 0
}

# args = (
#   x, y, w, h
#   sortby, unit, fontname, fontsize, fontcolor
#   legend = (
#     label, fontname, fontsize, fontcolor, object, url, info
#   )
#   pie = (
#     outlinecolor, backgroundcolor
#   )
#   other = (
#     minv, color, label, fontname, fontsize, fontcolor, object, url, info
#   )
#   slices[] = (
#     v, color, label, fontname, fontsize, fontcolor, object, url, info
#   )
# )

function drawpie {
    typeset -n as=$1

    integer ix=${as.x} iy=${as.y} iw=${as.w} ih=${as.h}
    typeset unit=${as.unit} outlinecolor=${as.pie.outlinecolor}
    typeset backgroundcolor=${as.pie.backgroundcolor}

    float pi=3.14159265358979323846
    integer legx=$ix legy=$(( iy + ih - ih / 5 )) legw=$iw legh=$(( ih / 5 ))
    integer labw=$(( iw / 5 )) labh=$(( ih / 5 ))

    integer piex=$(( ix + labw )) piey=$(( iy + labh ))
    integer piew=$(( iw - 2 * labw )) pieh=$(( ih - ih / 5  - 2 * labh ))

    integer cx=$(( piex + piew / 2.0 )) cy=$(( piey + pieh / 2.0 ))
    integer l=$(( piew  / 2.0 ))

    typeset slicei i n lab fn fs fc lines linei info obj url cl v map
    float r1 ang1 r2 ang2
    integer deg1 deg2 x1 y1 x2 y2 lx1 ly1 lx2 ly2 haveother other sum

    haveother=0
    if [[ ${as.other.minv} != '' ]] then
        haveother=1
    fi
    sum=0 other=0
    for (( slicei = 0; slicei < ${#as.slices[@]}; slicei++ )) do
        typeset -n slicer=as.slices[$slicei]
        (( sum += ${slicer.v} ))
        if (( haveother == 1 && ${slicer.v} <= ${as.other.minv} )) then
            (( other += ${slicer.v} ))
            slicer.inother=1
        fi
    done
    n=${#as.slices[@]}
    if (( haveother == 1 && other > 0 )) then
        (( n++ ))
        as.other.v=$other
    else
        haveother=0
    fi
    if [[ ${as.sortby} == val ]] then
        i=0
        for (( slicei = 0; slicei < ${#as.slices[@]}; slicei++ )) do
            typeset -n slicer=as.slices[$slicei]
            print "$slicei ${slicer.v}"
        done | sort -rn -k2,2 | while read slicei v; do
            map[$i]=$slicei
            (( i++ ))
        done
    else
        i=0
        for (( slicei = 0; slicei < ${#as.slices[@]}; slicei++ )) do
            map[$i]=$slicei
            (( i++ ))
        done
    fi

    ang1=0.0
    deg1=360
    (( x1 = cx + l * cos(ang1) ))
    (( y1 = cy + l * sin(ang1) ))
    lines[${#lines[@]}]="polyline $cx $cy $x1 $y1 1 $outlinecolor"
    for (( i = 0; i < n; i++ )) do
        if (( i == n - 1 && haveother == 1 )) then
            typeset -n slicer=as.other
        else
            typeset -n slicer=as.slices[${map[$i]}]
            if [[ ${slicer.inother} == 1 ]] then
                continue
            fi
        fi
        (( ang2 = ang1 + 2 * pi * ${slicer.v} / sum ))
        (( deg2 = 360 - 180 * ang2 / pi ))
        cl=${slicer.color:-$backgroundcolor}
        print "ellipse $cx $cy $l $l $deg2 $deg1 1 true $cl"
        (( x2 = cx + l * cos(ang2) ))
        (( y2 = cy + l * sin(ang2) ))
        lines[${#lines[@]}]="polyline $cx $cy $x2 $y2 1 $outlinecolor"

        (( x1 = cx + l * cos((ang1 + ang2) / 2.0) ))
        (( y1 = cy + l * sin((ang1 + ang2) / 2.0) ))
        if (( x1 < cx )) then
            (( lx1 = x1 - labw )); (( lx2 = x1 ))
        else
            (( lx1 = x1 )); (( lx2 = x1 + labw ))
        fi
        if (( y1 < cy )) then
            (( ly1 = y1 - labh )); (( ly2 = y1 ))
        else
            (( ly1 = y1 )); (( ly2 = y1 + labh ))
        fi
        lab="${slicer.v}$unit\\\n${slicer.label}"
        fn=${slicer.fontname}
        fs=${slicer.fontsize}
        fc=${slicer.fontcolor}
        print -r "text '$lab' $fn $fs $lx1 $ly1 $lx2 $ly2 0 -1 true $fc"
        obj=${slicer.object}
        url=${slicer.url}
        info=${slicer.info}
        if [[ $url != '' ]] then
            print -r "area $lx1 $ly1 $lx2 $ly2 '$obj' '$url' '$info'"
        fi

        (( ang1 = ang2 ))
        (( deg1 = deg2 ))
    done
    print "ellipse $cx $cy $l $l 0 360 1 false $outlinecolor"
    for (( linei = 0; linei < ${#lines[@]}; linei++ )) do
        print "${lines[$linei]}"
    done

    lx1=$(( legx )) ly1=$(( legy ))
    lx2=$(( legx + legw )) ly2=$(( legy + legh ))
    lab="${as.legend.label}"
    fn=${as.legend.fontname}
    fs=${as.legend.fontsize}
    fc=${as.legend.fontcolor}
    print -r "text '$lab' $fn $fs $lx1 $ly1 $lx2 $ly2 0 0 true $fc"
    obj=${as.legend.object}
    url=${as.legend.url}
    info=${as.legend.info}
    if [[ $url != '' ]] then
        print -r "area $lx1 $ly1 $lx2 $ly2 '$obj' '$url' '$info'"
    fi

    return 0
}

# args = (
#   x, y, w, h
#   legend = (
#     label, fontname, fontsize, fontcolor
#   )
#   outlinecolor, backgroundcolor, color
#   sections[] = (
#     v, backgroundcolor, outlinecolor, label, fontname, fontsize, fontcolor
#   )
# )

function drawstack {
    typeset -n as=$1

    integer ix=${as.x} iy=${as.y} iw=${as.w} ih=${as.h}
    float minv=${as.minv} maxv=${as.maxv}
    typeset unit=${as.unit} outlinecolor=${as.dial.outlinecolor}
    typeset backgroundcolor=${as.dial.backgroundcolor} color=${as.dial.color}

    integer legx=$ix legy=$(( iy + ih - ih / 5 )) legw=$iw legh=$(( ih / 5 ))

    integer gx=$(( ix + 5 )) gy=$(( iy + 5 ))
    integer gw=$(( iw / 3 )) gh=$(( ih - legh ))
    integer gew=$(( gw - 10 )) geh=$(( gh - 10 ))
    integer tx=$(( ix + gw + 5 )) ty=$(( iy + 5 ))
    integer tw=$(( iw * 2 / 3 )) th=$(( ih - legh ))
    integer tew=$(( tw - 10 )) teh=$(( th - 10 ))
    integer gx1 gy1 gx2 gy2 tx1 ty1 tx2 ty2 dgh dth lx1 lx2 ly1 ly2 sum

    typeset lab fn fs fc sectioni info obj url

    sum=0
    for (( sectioni = 0; sectioni < ${#as.sections[@]}; sectioni++ )) do
        typeset -n sectionr=as.sections[$sectioni]
        (( sum += ${sectionr.v} ))
    done

    (( gx1 = gx ))
    (( gx2 = gx + gew ))
    (( gy2 = gy + geh ))
    (( tx1 = tx ))
    (( tx2 = tx + tew ))
    (( ty2 = ty + teh ))
    for (( sectioni = 0; sectioni < ${#as.sections[@]}; sectioni++ )) do
        typeset -n sectionr=as.sections[$sectioni]
        (( dgh = geh * ${sectionr.v} / sum ))
        (( gy1 = gy2 - dgh ))
        print "box $gx1 $gy1 $gx2 $gy2 1 true ${sectionr.backgroundcolor}"
        print "box $gx1 $gy1 $gx2 $gy2 1 false ${sectionr.outlinecolor}"

        (( dth = teh / ${#as.sections[@]} ))
        (( ty1 = ty2 - dth ))
        lab="${sectionr.v} ${sectionr.label}"
        fn=${sectionr.fontname}
        fs=${sectionr.fontsize}
        fc=${sectionr.fontcolor}
        print "text '$lab' $fn $fs $tx1 $ty1 $tx2 $ty2 0 0 true $fc"
        print "polyline $gx2 $(( gy1 + dgh / 2 )) $tx1 $(( ty1 + dth / 2 )) 1 $fc"
        obj=${sectionr.object}
        url=${sectionr.url}
        info=${sectionr.info}
        if [[ $url != '' ]] then
            print -r "area $gx1 $gy1 $gx2 $gy2 '$obj' '$url' '$info'"
            print -r "area $tx1 $ty1 $tx2 $ty2 '$obj' '$url' '$info'"
        fi

        (( gy2 = gy1 ))
        (( ty2 = ty1 ))
    done

    lx1=$(( legx )) ly1=$(( legy ))
    lx2=$(( legx + legw )) ly2=$(( legy + legh ))
    lab="${as.legend.label}"
    fn=${as.legend.fontname}
    fs=${as.legend.fontsize}
    fc=${as.legend.fontcolor}
    print -r "text '$lab' $fn $fs $lx1 $ly1 $lx2 $ly2 0 0 true $fc"
    obj=${as.legend.object}
    url=${as.legend.url}
    info=${as.legend.info}
    if [[ $url != '' ]] then
        print -r "area $lx1 $ly1 $lx2 $ly2 '$obj' '$url' '$info'"
    fi

    return 0
}

# args = (
#   tables[] = (
#     fontname, fontsize, fontcolor, backgroundcolor
#     caption = (
#       contents, fontname, fontsize, fontcolor backgroundcolor, url, info
#     )
#     rows[] = (
#       fontname, fontsize, fontcolor, backgroundcolor
#       cells[] = (
#         contents, fontname, fontsize, fontcolor backgroundcolor, url, info
#       )
#     )
#   )
# )

function drawtable {
    typeset -n as=$1

    typeset tablei rowi celli fn fs fc bg lab url info cn j

    for (( tablei = 0; tablei < ${#as.tables[@]}; tablei++ )) do
        typeset -n tabler=as.tables[$tablei]
        fn=${tabler.fontname}
        fs=${tabler.fontsize}
        fc=${tabler.fontcolor}
        bg=${tabler.backgroundcolor}
        print "tablebegin $fn $fs $bg $fc"
        fn=${tabler.caption.fontname}
        fs=${tabler.caption.fontsize}
        fc=${tabler.caption.fontcolor}
        bg=${tabler.caption.backgroundcolor}
        lab=${tabler.caption.contents}
        url=${tabler.caption.url}
        info=${tabler.caption.info}
        print "caption '$lab' $fn $fs $bg $fc '$url' '$info'"
        for (( rowi = 0; rowi < ${#tabler.rows[@]}; rowi++ )) do
            typeset -n rowr=tabler.rows[$rowi]
            fn=${rowr.fontname}
            fs=${rowr.fontsize}
            fc=${rowr.fontcolor}
            bg=${rowr.backgroundcolor}
            print "rowbegin $fn $fs $bg $fc"
            for (( celli = 0; celli < ${#rowr.cells[@]}; celli++ )) do
                typeset -n cellr=rowr.cells[$celli]
                fn=${cellr.fontname}
                fs=${cellr.fontsize}
                fc=${cellr.fontcolor}
                bg=${cellr.backgroundcolor}
                lab=${cellr.contents}
                url=${cellr.url}
                info=${cellr.info}
                j=${cellr.justify}
                cn=${cellr.columns}
                print "cell '$lab' $fn $fs $bg $fc $j $cn '$url' '$info'"
            done
            print "rowend"
        done
        print "tableend"
    done

    return 0
}
