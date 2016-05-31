
function vg_assetssplitter {
    typeset rec name v

    if [[ $3 == '' ]] then
        print -u2 SWIFT ERROR missing mode
        return 1
    fi

    if [[ $4 == '' ]] then
        print -u2 SWIFT ERROR missing record
        return 1
    fi

    rec=$4
    v="$rec|++|$rec"

    print -r "<option value='${v//\'/%27}'>$rec"

    return 0
}
