
function suipackkv { # $1 = var $rest = optional exclude list
    typeset _var skip k v i j s c n

    set -f

    _var=$1
    typeset -n x=$_var
    shift

    s=
    c='?'
    for i in "${!x.@}"; do
        k=${i#$_var.}
        skip=n
        for j in "$@"; do
            if [[ $j == $k ]] then
                skip=y
                break
            fi
        done
        if [[ $skip == n ]] then
            typeset -n y=$_var.$k
            if [[ ${y[1]} != '' ]] then
                s="$s$c$k=["
                for (( k = 0; ; k++ )) do
                    if [[ ${y[$k]} == '' ]] then
                        break
                    fi
                    v=${y[$k]//'%'/%25}
                    v=${v//';'/%3B}
                    v=${v//'&'/%26}
                    v=${v//'+'/%2B}
                    v=${v//' '/'+'}
                    s="$s;$k=$v"
                done
                s="${s#';'}]"
            else
                v=${y//'%'/%25}
                v=${v//';'/%3B}
                v=${v//'&'/%26}
                v=${v//'+'/%2B}
                v=${v//' '/'+'}
                s="$s$c$k=$v"
            fi
            if [[ $k == name ]] then
                n=${y//'%'/%25}
                n=${n//';'/%3B}
                n=${n//'&'/%26}
                n=${n//'+'/%2B}
                n=${n//' '/'+'}
            fi
            c='&'
            typeset +n y
        fi
    done
    print "$n$s"
    typeset +n x
}
