
function vg_portspeedoverridechecker {
    typeset rest name port speed
    integer errn=0 warnn=0

    if [[ $3 == \#* ]] then
        print -r -- "rec=${3//'|'+'|'/'|'}"
        print WARNING entry is commented out
        print "errors=0\nwarnings=1"
        return 0
    fi

    rest=$3
    name=${rest%%\|+\|*}
    rest=${rest#"$name"\|+\|}
    port=${rest%%\|+\|*}
    rest=${rest#"$port"\|+\|}
    speed=$rest

    if [[ $name == "" ]] then
        print ERROR asset name field must be populated
        print "errors=1\nwarnings=0"
        return 1
    fi
    if [[ $port == "" ]] then
        print ERROR interface name field must be populated
        print "errors=1\nwarnings=0"
        return 1
    fi
    if [[ $speed == "" ]] then
        print ERROR port speed field must be populated
        print "errors=1\nwarnings=0"
        return 1
    fi
    if [[ $speed != +([0-9]) ]] then
        print ERROR port speed field must be an integer
        print "errors=1\nwarnings=0"
        return 1
    fi

    print -r -- "rec=$name|$port|$speed"
    print "errors=$errn\nwarnings=$warnn"

    return 0
}
