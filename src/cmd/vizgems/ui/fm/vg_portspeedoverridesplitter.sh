
function vg_portspeedoverridesplitter {
    typeset rec l name port speed rest v

    if [[ $3 == '' ]] then
        print -u2 SWIFT ERROR missing mode
        return 1
    fi

    if [[ $4 == '' ]] then
        print -u2 SWIFT ERROR missing record
        return 1
    fi

    rec=$4
    name=${rec%%\|*}
    rest=${rec#"$name"\|}
    port=${rest%%\|*}
    rest=${rest#"$port"\|}
    speed=$rest
    v="$rec|++|$name|+|$port|+|$speed"

    print -r "<option value='${v//\'/%27}'>$rec"

    return 0
}
