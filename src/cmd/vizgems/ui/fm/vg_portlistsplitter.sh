
function vg_portlistsplitter {
    typeset rec l target port timeout aname v

    if [[ $3 == '' ]] then
        print -u2 SWIFT ERROR missing mode
        return 1
    fi

    if [[ $4 == '' ]] then
        print -u2 SWIFT ERROR missing record
        return 1
    fi

    rec=$4
    set -- $4
    target=$1
    port=$2
    timeout=$3
    aname=$4
    v="$rec|++|$target|+|$port|+|$timeout|+|$aname"

    print -r "<option value='${v//\'/%27}'>$rec"

    return 0
}
