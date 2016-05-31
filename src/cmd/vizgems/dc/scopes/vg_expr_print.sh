
vg_epvals=
vg_epn2is=
vg_epunit=
vg_epresult=

function vg_expr_print_var { # $1: variable name $2: capacity
    typeset name=$1
    typeset -F2 valperc
    typeset v

    typeset -n epn2is=$vg_epn2is
    typeset -n epvref=${vg_epvals}[${epn2is[${name}]:-_}]
    if [[ ${epvref} == '' ]] then
        print -u2 vg_expr_print_var: cannot find variable $name
        return 1
    fi
    [[ ${epvref.nodata} != '' ]] && return 1
    if [[ ${epvref.rti} != '' ]] then
        typeset -n vr=epvref.num_${epvref.rti}
        v=$vr
    else
        v=${epvref.num}
    fi
    if [[ ${epvref.alarmlabel} != '' && ${epvref.alarmlabel} != \+* ]] then
        print -rn -- "(${epvref.alarmlabel}=${v}${epvref.unit}"
    elif [[ ${epvref.label} != '' && ${epvref.label} != \+* ]] then
        print -rn -- "(${epvref.label}=${v}${epvref.unit}"
    else
        print -rn -- "(${epvref.name}=${v}${epvref.unit}"
    fi
    if [[ $2 != '' && ${epvref.unit} != '%' ]] then
        (( valperc = 100.0 * (${v} / ($2 + 0.0)) ))
        print -rn -- " ($valperc%)"
    fi
    print -r -- ")|(${epvref.unit})"
    return 0
}

function vg_expr_print_val { # $1: ref to value $2: capacity
    typeset val
    typeset -F2 valperc

    typeset -n epvref=$1
    val=${epvref.num}
    if [[ $2 != '' && $vg_epunit != '%' ]] then
        (( valperc = 100.0 * (val / ($2 + 0.0)) ))
        print -r -- "$val$vg_epunit ($valperc%)"
    else
        print -r -- "$val$vg_epunit"
    fi
    return 0
}

function vg_expr_print_node { # $1: expression ref $2: var only flag
    typeset cap var minval maxval minmode1 maxmode1 minmode2 maxmode2

    if [[ $1 == '' ]] then
        print -r -- ""
        return 0
    fi

    typeset -n epnref=$1
    cap=${epnref.cap}
    if [[ $2 == _varonly_ ]] then
        var=$(vg_expr_print_var "${epnref.var}" "$cap") || return 1
        var=${var%'|('*')'}
        print -r "$var"
        return 0
    fi
    if [[ ${epnref.type} != '' ]] then
        case ${epnref.type} in
        nrange)
            var=$(vg_expr_print_var "${epnref.var}" "$cap") || return 1
            vg_epunit=${var##*'|'}
            vg_epunit=${vg_epunit//[()]/}
            var=${var%'|('*')'}
            minmode1=
            if [[ ${epnref.emin} != '' ]] then
                minval=$(vg_expr_print_val "$1.emin" "$cap") || return 1
                minmode1='>'
                minmode2='('
            elif [[ ${epnref.imin} != '' ]] then
                minval=$(vg_expr_print_val "$1.imin" "$cap") || return 1
                minmode1='>='
                minmode2='['
            fi
            maxmode1=
            if [[ ${epnref.emax} != '' ]] then
                maxval=$(vg_expr_print_val "$1.emax" "$cap") || return 1
                maxmode1='<'
                maxmode2=')'
            elif [[ ${epnref.imax} != '' ]] then
                maxval=$(vg_expr_print_val "$1.imax" "$cap") || return 1
                maxmode1='<='
                maxmode2=']'
            fi
            if [[ $minmode1 != '' && $maxmode1 != '' ]] then
                print -rn -- "$var in range $minmode2$minval, $maxval$maxmode2"
            elif [[ $minmode1 != '' ]] then
                print -rn -- "$var $minmode1 $minval"
            elif [[ $maxmode1 != '' ]] then
                print -rn -- "$var $maxmode1 $maxval"
            fi
            if [[ ${epnref.min.count} != '' ]] then
                print -r -- \
                " for ${epnref.min.count}/${epnref.min.per} cycles"
            else
                print -r -- ""
            fi
            return 0
            ;;
        *)
            print -u2 vg_expr_print_node: unknown type ${epnref.type}
            return 1
            ;;
        esac
    else
        print -u2 vg_expr_print_node: unknown expression ${epnref}
        return 1
    fi
    return 0
}

function vg_expr_print { # $1: var ref, $2: var map, $3: expr ref, $4: extra
    vg_epvals=$1
    vg_epn2is=$2
    vg_epunit=
    vg_epresult=$(vg_expr_print_node "$3" "$4") || return 1
}
