
function vg_cm_favorites_isallowed {
    typeset ids id

    ids=$SWMID
    [[ " $SWMGROUPS " == *' vg_att_admin '* ]] && ids=__ALL__
    id=${VGCM_RFILE%%.txt}
    [[ $ids == __ALL__ || :$ids: == *:$id:* ]] && return 0

    return 1
}

function vg_cm_favorites_list {
    typeset ids id i

    ids=$SWMID
    [[ " $SWMGROUPS " == *' vg_att_admin '*  ]] && ids=__ALL__
    for i in $VGCM_FAVORITESDIR/*.txt; do
        [[ ! -f $i ]] && continue
        id=${i##*/}
        id=${id%.txt}
        [[ $ids != __ALL__ && :$ids: != *:$id:* ]] && continue
        print -r "${i##*/}|$id"
    done

    return 0
}

function vg_cm_favorites_check {
    typeset spec=$1

    typeset -A ofs nfs ts
    typeset kvi str kv k v g err i ire ore rw x l

    rw=n
    eval $(vgxml -topmark edit -ksh "$spec")
    case ${edit.a} in
    remove)
        v=${edit.o.r.l//'+'/' '}
        v=${v//@([\'\\])/'\'\1}
        eval "v=\$'${v//'%'@(??)/'\x'\1"'\$'"}'"
        print -r "OLD RECORD|$v"
        typeset -n x=edit.o.r
        if [[ $EDITALL != 1 && ${x.f_2} != *'|'$SWMID ]] then
            print "ERROR|Account IDs do not match - cannot remove record"
            return
        fi
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
        typeset -n x=edit.o.r
        if [[ $EDITALL != 1 && ${x.f_2} != *'|'$SWMID ]] then
            print "ERROR|Account IDs do not match - cannot modify record"
            return
        fi
        typeset -n x=edit.n.r
        if [[ ${x.f_2} != "2|$SWMID" ]] then
            x.f_2="2|$SWMID"
            rw=y
        fi
        # continue with new record check
        ;;
    insert)
        v=${edit.n.r.l//'+'/' '}
        v=${v//@([\'\\])/'\'\1}
        eval "v=\$'${v//'%'@(??)/'\x'\1"'\$'"}'"
        print -r "NEW RECORD|$v"
        typeset -n x=edit.n.r
        if [[ ${x.f_2} != "2|$SWMID" ]] then
            x.f_2="2|$SWMID"
            rw=y
        fi
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
        if [[ $v == '' ]] then
            print "ERROR|all fields are mandatory"
        fi
        nfs[$kvi]=$v
    done

    # check account id
    if [[
        $(egrep "^${nfs[2]};" $VG_DWWWDIR/accounts/current) == ''
    ]] then
        print "ERROR|account id ${nfs[2]} does not exist"
    fi

    if [[ $rw == y ]] then
        x=${spec%%'<n>'*}
        x+='<n><r>'
        l=
        for (( kvi = 0; kvi < 3; kvi++ )) do
            f=$(printf '%#H' "${nfs[$kvi]}")
            x+="<f>$kvi|$f</f>"
            l+=" - $f"
        done
        l=${l#' - '}
        x+="<l>$l</l></r></n>"
        print -r "REC|$x"
    else
        print -r "REC|$spec"
    fi

    return 0
}

function vg_cm_favorites_pack {
    typeset spec=$1

    typeset or kvi str kv k v f v g t

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
        t+="|+|$v"
    done
    t=${t#'|+|'}
    print -r -u4 "$t"

    return 0
}

function vg_cm_favorites_split {
    typeset fmt=$1 rec=$2

    typeset x l rest i f id

    [[ $rec != *'|+|'* ]] && return 0

    x='<r>'
    l=
    rest=$rec
    for (( i = 0; i < 3; i++ )) do
        f=${rest%%'|+|'*}
        rest=${rest##"$f"?('|+|')}
        if (( i == 2 )) then
            id=$f
        fi
        f=$(printf '%#H' "$f")
        x+="<f>$i|$f</f>"
        l+=" - $f"
    done
    l=${l#' - '}
    x+="<l>$l</l><t>$(printf '%#H' "$rec")</t></r>"

    print -r "$x"

    return 0
}
