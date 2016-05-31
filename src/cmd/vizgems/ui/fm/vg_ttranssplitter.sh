
function vg_ttranssplitter {
    typeset rec name1 name2 v

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
    name1=$1
    shift
    name2="$@"
    v="$rec|++|$name1|+|$name2"

    print -r "<option value='${v//\'/%27}'>$rec"

    return 0
}
