
function vg_cpltabchecker {
    typeset -A fs
    typeset rest i f v t
    integer errn=0 warnn=0

    if [[ $3 == \#* ]] then
        print -r -- "rec=${3//'|'+'|'/'|'}"
        print WARNING entry is commented out
        print "errors=0\nwarnings=1"
        return 0
    fi

    rest=$3
    for (( i = 0; i < 3; i++ )) do
        f=${rest%%\|+\|*}
        rest=${rest#"$f"\|+\|}
        fs[$i]=$f
        t="$t:$f"
        v="$v:$f"
    done
    t=${t#:}
    v=${v#:}

    if (( ${#fs[2]} < 1 )) then
        print ERROR process field may not be empty
        ((errn++))
    fi

    print -r -- "rec=$v"
    print "errors=$errn\nwarnings=$warnn"

    return 0
}
