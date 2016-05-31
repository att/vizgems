
function vg_cm_type_check {
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

    for (( kvi = 0; kvi < 3; kvi++ )) do
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
        nfs[$kvi]=$v
    done

    # check type id field
    if [[ ${nfs[0]} == '' ]] then
        print "ERROR|type id is mandatory"
    fi
    if [[ ${nfs[0]} != +([.a-zA-Z0-9_:-]) ]] then
        print "ERROR|the type id can only contain: . a-z A-Z 0-9 _ : -"
    fi
    if [[
        ${edit.a} == insert &&
        $(egrep "^${nfs[0]}\|" $VGCM_TYPEFILE) != ''
    ]] then
        print "ERROR|type id ${nfs[0]} already exists"
    fi

    # check type name field
    if [[ ${nfs[1]} == '' ]] then
        print "ERROR|type name is mandatory"
    fi

    # check formatted name field
    if [[ ${nfs[2]} == '' ]] then
        print "ERROR|formatted name is mandatory"
    fi

    print -r "REC|$spec"

    return 0
}

function vg_cm_type_pack {
    typeset spec=$1

    typeset or kvi str kv k v t

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
    for (( kvi = 0; kvi < 3; kvi++ )) do
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
        t+="|$v"
    done
    t=${t#'|'}
    print -r -u4 "$t"

    return 0
}

function vg_cm_type_split {
    typeset fmt=$1 rec=$2

    typeset x l rest i f

    [[ $rec != *'|'* ]] && return 0

    x='<r>'
    l=
    rest=$rec
    for (( i = 0; i < 3; i++ )) do
        f=${rest%%'|'*}
        rest=${rest##"$f"?('|')}
        f=$(printf '%#H' "$f")
        x+="<f>$i|$f</f>"
        l+=" - $f"
    done
    l=${l#' - '}
    x+="<l>$l</l><t>$(printf '%#H' "$rec")</t></r>"

    print -r "$x"

    return 0
}
