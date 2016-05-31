
function vg_urlprobesplitter {
    typeset rec l aname url retries waittime v

    if [[ $3 == '' ]] then
        print -u2 SWIFT ERROR missing mode
        return 1
    fi

    if [[ $4 == '' ]] then
        print -u2 SWIFT ERROR missing record
        return 1
    fi

    rec=$4
    aname=${rec%%\|*}
    rest=${rec#"$aname"\|}
    url=${rest%%\|*}
    rest=${rest#"$url"\|}
    retries=${rest%%\|*}
    rest=${rest#"$retries"\|}
    waittime=$rest
    v="$rec|++|$aname|+|$url|+|$retries|+|$waittime"

    print -r "<option value='${v//\'/%27}'>$rec"

    return 0
}
