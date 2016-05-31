
vg_eevals=
vg_een2is=
vg_eeresult=

function vg_expr_eval_var { # $1: variable name
    typeset name=$1
    typeset -n een2is=$vg_een2is
    typeset -lF3 v

    typeset -n eevref=${vg_eevals}[${een2is[${name}]:-_}]
    if [[ ${eevref} == '' ]] then
        print -u2 vg_expr_eval_var: cannot find variable $name
        return 1
    fi
    [[ ${eevref.nodata} != '' ]] && return 1
    if [[ ${eevref.rti} != '' ]] then
        typeset -n vr=eevref.num_${eevref.rti}
        v=$vr
    else
        v=${eevref.num}
    fi
    print -r -- $v
    return 0
}

function vg_expr_eval_val { # $1: ref to value
    typeset val
    typeset -lF3 v

    typeset -n eevref=$1
    val=${eevref.num}
    print -r -- $val
    return 0
}

function vg_expr_eval_node { # $1: expression ref
    typeset -lF3 var val
    typeset minflag maxflag

    typeset -n eenref=$1
    if [[ ${eenref.type} != '' ]] then
        case ${eenref.type} in
        nrange)
            var=$(vg_expr_eval_var ${eenref.var}) || return 1
            minflag=1
            if [[ ${eenref.emin} != '' ]] then
                val=$(vg_expr_eval_val $1.emin) || return 1
                (( minflag = (var > val) ))
            elif [[ ${eenref.imin} != '' ]] then
                val=$(vg_expr_eval_val $1.imin) || return 1
                (( minflag = (var >= val) ))
            fi
            maxflag=1
            if [[ ${eenref.emax} != '' ]] then
                val=$(vg_expr_eval_val $1.emax) || return 1
                (( maxflag = (var < val) ))
            elif [[ ${eenref.imax} != '' ]] then
                val=$(vg_expr_eval_val $1.imax) || return 1
                (( maxflag = (var <= val) ))
            fi
            if (( !minflag || !maxflag )) then
                print 0
                return 0
            fi
            print 1
            return 0
            ;;
        *)
            print -u2 vg_expr_eval_node: unknown type ${eenref.type}
            return 1
            ;;
        esac
    else
        print -u2 vg_expr_eval_node: unknown expression ${eenref}
        return 1
    fi
    return 0
}

function vg_expr_eval { # $1: var ref, $2: var map, $3: expression ref
    vg_eevals=$1
    vg_een2is=$2
    vg_eeresult=$(vg_expr_eval_node $3) || return 1
}
