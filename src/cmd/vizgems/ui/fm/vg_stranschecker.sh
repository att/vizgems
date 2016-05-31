
function vg_stranschecker {
    typeset rest name1 name2
    integer errn=0 warnn=0

    if [[ $3 == \#* ]] then
        print -r -- "rec=${3//'|'+'|'/'|'}"
        print WARNING entry is commented out
        print "errors=0\nwarnings=1"
        return 0
    fi

    rest=$3
    name1=${rest%%\|+\|*}
    rest=${rest#"$name1"\|+\|}
    name2=$rest

    name1=${name1//\ /}
    name2=${name2//\ /}

    print -r -- "rec=$name1	$name2"
    print "errors=$errn\nwarnings=$warnn"

    return 0
}
