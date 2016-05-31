
function vg_cpltabsplitter {
    typeset rest i f v1 v2 v t

    if [[ $3 == '' ]] then
        print -u2 SWIFT ERROR missing mode
        return 1
    fi

    if [[ $4 == '' ]] then
        print -u2 SWIFT ERROR missing record
        return 1
    fi

    rest=$4
    for (( i = 0; i < 3; i++ )) do
        f=${rest%%:*}
        rest=${rest#"$f":}
        v1="$v1:$f"
        v2="$v2|+|$f"
        t="$t:$f"
    done
    v1=${v1#:}
    v2=${v2#\|+\|}
    t=${t#:}
    v="$v1|++|$v2"

    print -r "<option value='${v//\'/%27}'>$t"

    return 0
}
