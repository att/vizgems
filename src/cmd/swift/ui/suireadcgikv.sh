
function suireadcgikv {
    typeset qs ifs k v vi i j list tmpdir

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
            else
                v=${v//'+'/' '}
                v=${v//@([\'\\])/'\'\1}
                eval "v=\$'${v//'%'@(??)/'\x'\1"'\$'"}'"
                qsn="$v"
            fi
            qslist="$qslist $k"
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
}
