
function prepfiles { # $1 = context
    typeset -u context
    typeset ci

    context=${1:-ALL}
    contexts=${contextmap[$context]:-$context}
    if [[ $contexts == '' ]] then
        print -u2 ERROR unknown context $context
        exit 1
    fi
    for ci in $contexts; do
        if [[ ${ofs[$ci]} != '' ]] then
            continue
        fi
        eval exec $ofn\> $fprefix.${suffixmap[$ci]}
        ofs[$ci]=$ofn
        case $ci in
        C)
            print -u$ofn "#ifndef _$vprefix"
            print -u$ofn "#define _$vprefix"
            ;;
        esac
        ((ofn++))
    done
}

function termfiles { # $1 = context
    typeset ci

    for ci in $contexts; do
        case $ci in
        C)
            print -u${ofs[$ci]} "#endif"
            ;;
        esac
    done
}


function output { # $1=state $2=mode $3=text
    typeset ci

    for ci in $contexts; do
        case $ci in
        C)
            output_C "$1" "$2" "$3"
            ;;
        KSH)
            output_KSH "$1" "$2" "$3"
            ;;
        esac
    done
}

function output_C { # $1=state $2=mode $3=text
    typeset fd seenparen s

    fd=${ofs[C]}
    case $1 in
    BEGIN)
        case $2 in
        VAR)
            if [[ "$3" == \(* ]] then
                print -u$fd "#define ${vprefix}_$name$3"
            else
                print -u$fd "#define ${vprefix}_$name $3"
            fi
            ;;
        FUN)
            seenparen=0
            s=
            for ((i = 0; i < ${#3}; i++)) do
                if [[ $seenparen == 0 && ${3:$i:1} == '(' ]] then
                    seenparen=1
                    s="$s ${vprefix}_$name "
                fi
                s="$s${3:$i:1}"
            done
            print -u$fd "#ifndef ${vprefix}_DEFS_MAIN"
            print -u$fd "extern ${s%%\{*};"
            print -u$fd "#else"
            print -u$fd "$s"
            ;;
        esac
        ;;
    BODY)
        print -u$fd "$3"
        ;;
    END)
        if [[ $2 == FUN ]] then
            print -u$fd "#endif"
        fi
        print -u$fd ""
        ;;
    RAW)
        print -u$fd "$3"
    esac
}

function output_KSH { # $1=state $2=mode $3=text
    typeset fd seenparen s

    fd=${ofs[KSH]}
    case $1 in
    BEGIN)
        case $2 in
        VAR)
            print -u$fd "${vprefix}_$name=$3"
            ;;
        FUN)
            print -u$fd "function ${vprefix}_$name $3"
            ;;
        esac
        ;;
    BODY)
        print -u$fd "$3"
        ;;
    END)
        print -u$fd ""
        ;;
    RAW)
        print -u$fd "$3"
    esac
}

function showusage {
    print -u2 "$0 [optional flags] <name>.defs"
    print -u2 "    [-fp <file prefix>]  (default: <name>)"
    print -u2 "    [-vp <var prefix>]  (default: 'SUT')"
}

typeset -A contextmap
contextmap[ALL]='C KSH'

typeset -A suffixmap
suffixmap[C]=h
suffixmap[KSH]=sh

typeset -A ofs
ofn=3

if [[ $SWIFTWARNLEVEL != '' ]] then
    typeset -ft prepfiles output output_C output_KSH
    set -x
fi

fprefix=
vprefix=SUI

while (( $# > 0 )) do
    if [[ $1 == '-fp' ]] then
        if (( $# < 2 )) then
            showusage
            exit 1
        fi
        fprefix=$2
        shift; shift
    elif [[ $1 == '-vp' ]] then
        if (( $# < 2 )) then
            showusage
            exit 1
        fi
        vprefix=$2
        shift; shift
    elif [[ $1 == -* ]] then
        showusage
        exit 1
    else
        break
    fi
done

if [[ ! -f $1 ]] then
    print -u2 ERROR file $1 does not exist
    exit 1
fi

ifile=$1

if [[ $fprefix == '' ]] then
    if [[ $ifile == *.defs ]] then
        fprefix=${ifile%.defs}
    else
        print -u2 "ERROR a file prefix must be specified when file name\c"
        print -u2 " is not of the form <name>.defs"
        exit 1
    fi
fi

ifs="$IFS"

name=
mode=
while read -r line; do
    case $line in
    \#*)
        ;;
    VAR\|*)
        if [[ $name != '' ]] then
            output END $mode
        fi
        IFS='|'
        set -A l -- ${line}
        IFS="$ifs"
        mode=VAR
        name=${l[1]}
        prepfiles "${l[2]}"
        output BEGIN $mode "${l[3]}"
        ;;
    FUN\|*)
        if [[ $name != '' ]] then
            output END $mode
        fi
        IFS='|'
        set -A l -- ${line}
        IFS="$ifs"
        mode=FUN
        name=${l[1]}
        prepfiles "${l[2]}"
        output BEGIN $mode "${l[3]}"
        ;;
    *)
        if [[ $name != '' ]] then
            output BODY $mode "$line"
        else
            print -u2 ERROR unrecognised directive "$line"
        fi
        ;;
    esac
done < $ifile
if [[ $name != '' && mode != '' ]] then
    output END $mode
fi

typeset -A typemap
typemap[char]=1
typemap[short]=2
typemap[int]=4
typemap[long]=4
typemap[float]=4
typemap[double]=8
typemap["long long"]=8

shift

for file in "$@"; do
    while read a b c; do
        case $a in
        name)
            fname=$b
            ;;
        field)
            d=${c##*\ }
            if [[ $d == '' || $d != [0-9]* ]] then
                d=1
            else
                c=${c%\ $d}
            fi
            c2=${c#unsigned\ }
            c2=${c2#signed\ }
            ((n = ${typemap[$c2]} * $d))
            prepfiles all
            name=${fname}_${b}_N
            output BEGIN VAR $d
            output END VAR
            name=${fname}_${b}_L
            output BEGIN VAR $n
            output END VAR
            ;;
        esac
    done < $file
done

fd=${ofs[C]}
for file in "$@"; do
    print -u$fd ""
    while read a b c; do
        case $a in
        name)
            name=$b
            print -u$fd "typedef struct ${vprefix}_${name}_s {"
            ;;
        field)
            d=${c##*\ }
            if [[ $d == '' || $d != [0-9]* ]] then
                d=1
            else
                c=${c%\ $d}
            fi
            if [[ $d == 1 ]] then
                print -u$fd "    $c s_$b;"
            else
                print -u$fd "    $c s_${b}[$d];"
            fi
            ;;
        esac
    done < $file
    print -u$fd "} ${vprefix}_${name}_t;"
done

termfiles
