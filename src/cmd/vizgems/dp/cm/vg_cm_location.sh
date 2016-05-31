
function vg_cm_location_check {
    typeset spec=$1

    typeset -A ofs nfs
    typeset kvi str kv k v

    eval $(vgxml -topmark edit -ksh "$spec")
    case ${edit.a} in
    remove)
        v=${edit.o.r.l//'+'/' '}
        v=${v//@([\'\\])/'\'\1}
        eval "v=\$'${v//'%'@(??)/'\x'\1"'\$'"}'"
        print -r "OLD RECORD|$v"
        print -r "REC|$spec"
        return
        ;;
    modify)
        v=${edit.o.r.l//'+'/' '}
        v=${v//@([\'\\])/'\'\1}
        eval "v=\$'${v//'%'@(??)/'\x'\1"'\$'"}'"
        print -r "OLD RECORD|$v"
        v=${edit.n.r.l//'+'/' '}
        v=${v//@([\'\\])/'\'\1}
        eval "v=\$'${v//'%'@(??)/'\x'\1"'\$'"}'"
        print -r "NEW RECORD|$v"
        # continue with new record check
        ;;
    insert)
        v=${edit.n.r.l//'+'/' '}
        v=${v//@([\'\\])/'\'\1}
        eval "v=\$'${v//'%'@(??)/'\x'\1"'\$'"}'"
        print -r "NEW RECORD|$v"
        # continue with new record check
        ;;
    *)
        print "ERROR|unknown action ${edit.a}"
        return
        ;;
    esac

    n=7
    for (( kvi = 0; kvi < n; kvi++ )) do
        if (( kvi == 0 )) then
            str=f
        else
            str=f_$kvi
        fi
        typeset -n kv=edit.n.r.$str
        k=${kv%%'|'*}
        v=${kv#"$k"'|'}
        v=${v//'+'/' '}
        v=${v//@([\'\\])/'\'\1}
        eval "v=\$'${v//'%'@(??)/'\x'\1"'\$'"}'"
        ofs[$kvi]=$v
        if (( kvi == 0 )) then
            if [[ $v == node ]] then
                n=5
            fi
        fi
        nfs[$kvi]=$v
    done

    # check type field
    if [[ ${nfs[0]} != node && ${nfs[0]} != edge && ${nfs[0]} != map ]] then
        print "ERROR|the record type must be node, edge, or map"
    fi

    print -r "REC|$spec"

    return 0
}

function vg_cm_location_pack {
    typeset spec=$1

    typeset or n kvi str kv k v t

    eval $(vgxml -topmark edit -ksh "$spec")
    print -r -u5 "${edit.u}"
    case ${edit.a} in
    remove)
        or=${edit.o.r.t//'+'/' '}
        or=${or//@([\'\\])/'\'\1}
        eval "or=\$'${or//'%'@(??)/'\x'\1"'\$'"}'"
        print -r -u3 "$or"
        return
        ;;
    modify)
        or=${edit.o.r.t//'+'/' '}
        or=${or//@([\'\\])/'\'\1}
        eval "or=\$'${or//'%'@(??)/'\x'\1"'\$'"}'"
        print -r -u3 "$or"
        # continue with new record pack
        ;;
    insert)
        # continue with new record pack
        ;;
    *)
        print "ERROR|unknown action ${edit.a}"
        return
        ;;
    esac

    t=
    n=7
    for (( kvi = 0; kvi < n; kvi++ )) do
        if (( kvi == 0 )) then
            str=f
        else
            str=f_$kvi
        fi
        typeset -n kv=edit.n.r.$str
        k=${kv%%'|'*}
        v=${kv#"$k"'|'}
        v=${v//'+'/' '}
        v=${v//@([\'\\])/'\'\1}
        eval "v=\$'${v//'%'@(??)/'\x'\1"'\$'"}'"
        if (( kvi == 0 )) then
            if [[ $v == node ]] then
                n=5
            fi
        fi
        t+="|$v"
    done
    t=${t#'|'}
    print -r -u4 "$t"

    return 0
}

function vg_cm_location_split {
    typeset fmt=$1 rec=$2

    typeset x l n c rest i f k

    [[ $rec != *'|'* ]] && return 0

    x='<r>'
    l=
    case $rec in
    node*) n=5; c=0 ;;
    edge*) n=7; c=1 ;;
    map*)  n=7; c=2 ;;
    esac
    rest=$rec
    for (( i = 0; i < n; i++ )) do
        f=${rest%%'|'*}
        rest=${rest##"$f"?('|')}
        f=$(printf '%#H' "$f")
        if (( i == 0 )) then
            k=$i
        else
            k="$(( i - 1 )):$c"
        fi
        x+="<f>$k|$f</f>"
        l+=" - $f"
    done
    l=${l#' - '}
    x+="<l>$l</l><t>$(printf '%#H' "$rec")</t></r>"

    print -r "$x"

    return 0
}
