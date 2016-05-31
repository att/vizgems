
function vg_hbauditchecker {
    typeset rest name
    integer errn=0 warnn=0

    if [[ $3 == \#* ]] then
        print -r -- "rec=${3//'|'+'|'/'|'}"
        print WARNING entry is commented out
        print "errors=0\nwarnings=1"
        return 0
    fi

    rest=$3
    name=${rest%%\|+\|*}

    print -r -- "rec=$name"
    print "errors=$errn\nwarnings=$warnn"

    return 0
}
