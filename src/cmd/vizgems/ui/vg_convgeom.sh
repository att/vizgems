
if [[ $SWIFTWARNLEVEL != '' ]] then
    set -x
fi

typeset -A names coords polys itemps

case $1 in
fg)
    n=0
    while read -r line; do
        names[$n]=${line%\|*}
        coords[$n]=${line##*\|}
        coords[$n]=${coords[$n]%%\\*}
        (( n++ ))
    done
    print $n
    for (( id = 0; id < n; id++ )) do
        print ${names[$id]}
        print 1
        print -- ${coords[$id]:-0.0 0.0}
    done
    ;;
bg)
    polyn=0
    while read -r line; do
        case $line in
        item:*)
            rest=${line#item:\ }
            id=${rest%%\ *}
            rest=${rest#"$id"\ }
            name=$rest
            names[$id]=$name
            ;;
        point:*)
            rest=${line#point:\ }
            pi=${rest%%\ *}
            rest=${rest#"$pi"\ }
            coord=${rest%,*}
            coord=${coord#\(}
            coord=${coord//,/" "}
            coords[$pi]=$coord
            ;;
        ind:*)
            rest=${line#ind:\ }
            polyid=${rest%%\ *}
            rest=${rest#"$polyid"\ }
            pn=$rest
            polys[$polyid]=$pn
            if (( pn > 0 )) then
                (( polyn++ ))
            fi
            ;;
        p2i:*)
            rest=${line#p2i:\ }
            pi=${rest%%\ *}
            rest=${rest#"$pi"\ }
            id=$rest
            (( itemps[$id]++ ))
            ;;
        esac
    done
    polyi=0
    pi=0
    print $polyn
    for (( id = 0; id < ${#names[@]}; id++ )) do
        pn=${itemps[$id]}
        for (( polyj = polyi; polyj < ${#polys[@]}; polyj++ )) do
            (( pn -= ${polys[$polyj]} ))
            if (( pn == 0 )) then
                break
            elif (( pn < 0 )) then
                print -u2 OOPS
                exit
            fi
        done
        for (( polyk = polyi; polyk <= polyj; polyk++ )) do
            (( ${polys[$polyk]} > 0 )) && print ${names[$id]}
            (( ${polys[$polyk]} > 0 )) && print ${polys[$polyk]}
            for (( pj = 0; pj < ${polys[$polyk]}; pj++ )) do
                (( ${polys[$polyk]} > 0 )) && print -- ${coords[$(( pi + pj ))]}
            done
            (( pi += pj ))
        done
        (( polyi = polyj + 1 ))
    done
    ;;
esac
