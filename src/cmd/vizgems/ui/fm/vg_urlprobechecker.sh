
function vg_urlprobechecker {
    typeset rest aname url retries waittime
    integer errn=0 warnn=0

    if [[ $3 == \#* ]] then
        print -r -- "rec=${3//'|'+'|'/'|'}"
        print WARNING entry is commented out
        print "errors=0\nwarnings=1"
        return 0
    fi

    rest=$3
    aname=${rest%%\|+\|*}
    rest=${rest#"$aname"\|+\|}
    url=${rest%%\|+\|*}
    rest=${rest#"$url"\|+\|}
    retries=${rest%%\|+\|*}
    rest=${rest#"$retries"\|+\|}
    waittime=$rest

    if [[ $retries != '' && $retries != [12] ]] then
        print ERROR retries field must be between 1 - 2
        print "errors=1\nwarnings=0"
        return 1
    fi

    if [[ $waittime != '' && $waittime != +([0-9]) ]] then
        print ERROR waittime field must be an integer
        print "errors=1\nwarnings=0"
        return 1
    fi

    if [[ $waittime != '' ]] && (( waittime < 1 || waittime > 30 )) then
        print ERROR waittime value must be between 1 - 30
        print "errors=1\nwarnings=0"
        return 1
    fi

    aname="${aname//[ 	]/_}"
    url="${url//[ 	]/_}"

    print -r -- "rec=$aname|$url|$retries|$waittime"
    print "errors=$errn\nwarnings=$warnn"

    return 0
}
