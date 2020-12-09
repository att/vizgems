
function suireadcgikv {
    typeset qs ifs k v vi i j list tmpdir
    typeset ill='+(@(\<|%3c)@([a-z][a-z0-9]|a)*|\`*\`|\$*\(*\)|\$*\{*\})'
    typeset plain='+([a-zA-Z0-9_:/\.-])'
    typeset -l vl

    set -f
    qslist=
    qsarrays=

    case $REQUEST_METHOD in
    GET)
        qs="$QUERY_STRING"
        ifs="$IFS"
        IFS='&'
        set -A list $qs
        for i in "${list[@]}"; do
            [[ $i == '' ]] && continue
            k=${i%%=*}
            [[ $k == '' ]] && continue
            v=${i#"$k"=}
            k=${k#_}
            if [[ ${k//+([!a-zA-Z0-9_])/} != $k ]] then
                print -u2 ERROR: bad key "$k"
                continue
            fi
            if [[ $v == *$'\r'* ]] then
                v="[${v//+([$'\r\n'])/';'}]"
            fi
            typeset -n qsn=qs_$k
            if [[ $v == %5B*%5D ]] then
                v=${v//'+'/' '}
                v=${v//@([\'\\])/'\'\1}
                eval "v=\$'${v//'%'@(??)/'\x'\1"'\$'"}'"
            fi
            if [[ " $qslist " == *\ $k\ * ]] then
                if [[ $qsn != "$v" ]] then
                    v=${v//'+'/' '}
                    v=${v//@([\'\\])/'\'\1}
                    eval "v=\$'${v//'%'@(??)/'\x'\1"'\$'"}'"
                    qsn[${#qsn[@]}]=$v
                    qsarrays="$qsarrays $k"
                fi
            elif [[ $v == \[*\] ]] then
                v=${v#\[}
                v=${v%\]}
                v=${v#\;}
                while [[ $v != '' ]] do
                    vi=${v%%\;*}
                    v=${v#"$vi"}
                    v=${v#\;}
                    vi=${vi//'+'/' '}
                    vi=${vi//@([\'\\])/'\'\1}
                    eval "vi=\$'${vi//'%'@(??)/'\x'\1"'\$'"}'"
                    qsn[${#qsn[@]}]=$vi
                done
                qsarrays="$qsarrays $k"
                qslist="$qslist $k"
            else
                v=${v//'+'/' '}
                v=${v//@([\'\\])/'\'\1}
                eval "v=\$'${v//'%'@(??)/'\x'\1"'\$'"}'"
                qsn="$v"
                qslist="$qslist $k"
            fi
            typeset +n qsn
        done
        IFS="$ifs"
        ;;
    POST)
        ifs="$IFS"
        tmpdir=${TMPDIR:-/tmp}/sui.${SWMID}.$$
        trap "rm -rf $tmpdir" HUP QUIT TERM KILL EXIT
        mkdir $tmpdir || exit 1
        ( cd $tmpdir && $SWMROOT/bin/swmmimesplit )
        set +f
        list="$(print $tmpdir/*)"
        set -f
        for i in $list; do
            [[ ! -f $i ]] && continue
            k=${i##*/}
            k=${k#_}
            if [[ ${k//+([!a-zA-Z0-9_])/} != $k ]] then
                print -u2 ERROR: bad key "$k"
                continue
            fi
            typeset -n qsn=qs_$k
            j=0
            IFS=''
            while read -r v; do
                if ((j == 0)) then
                    qsn=${v%%$'\r'}
                else
                    qsn[${#qsn[@]}]=${v%%$'\r'}
                fi
                ((j++))
            done < $i
            IFS="$ifs"
            qslist="$qslist $k"
            typeset +n qsn
        done
        IFS="$ifs"
        ;;
    esac
    set +f

    if [[ $SWIFTCGINOFILTER != y ]] then
        qslist=" $qslist "
        qsarrays=" $qsarrays "
        for i in $qslist; do
            k=qs_$i
            if [[ $k == *+([!a-zA-Z0-9_])* ]] then
                print -r -u2 "SWIFT ERROR illegal key in parameters"
                unset $k
                qslist=${qslist//' '$i' '/' '}
                qsarrays=${qsarrays//' '$i' '/' '}
                continue
            fi
            typeset -n v=$k
            if [[ " "$qsarrays" " == *' '$i' '* ]] then
                for (( j = 0; j < ${#v[@]}; j++ )) do
                    vl=${v[$j]}
                    vl=${vl//+([[:space:]])/}
                    if [[ $vl == *$ill* ]] then
                        print -r -u2 "SWIFT ERROR illegal value for key $i"
                        unset $k
                        qslist=${qslist//' '$i' '/' '}
                        qsarrays=${qsarrays//' '$i' '/' '}
                        continue
                    elif [[ $k == qs_@($SWIFTCGIPLAINKEYS) && $vl != $plain ]] then
                        print -r -u2 "SWIFT ERROR non-plain value for key $i"
                        unset $k
                        qslist=${qslist//' '$i' '/' '}
                        qsarrays=${qsarrays//' '$i' '/' '}
                        continue
                    fi
                done
            else
                vl=$v
                vl=${vl//+([[:space:]])/}
                if [[ $vl == *$ill* ]] then
                    print -r -u2 "SWIFT ERROR illegal value for key $i"
                    unset $k
                    qslist=${qslist//' '$i' '/' '}
                    qsarrays=${qsarrays//' '$i' '/' '}
                    continue
                elif [[ $k == qs_@($SWIFTCGIPLAINKEYS) && $vl != $plain ]] then
                    print -r -u2 "SWIFT ERROR non-plain value for key $i"
                    unset $k
                    qslist=${qslist//' '$i' '/' '}
                    qsarrays=${qsarrays//' '$i' '/' '}
                    continue
                fi
            fi
        done
        qslist=${qslist//' '+(' ')/' '}
        qslist=${qslist#' '}
        qslist=${qslist%' '}
        qsarrays=${qsarrays//' '+(' ')/' '}
        qsarrays=${qsarrays#' '}
        qsarrays=${qsarrays%' '}
    fi
}
