
function suireadkv {
    # $1 = fd $2 = var $3 = optional output prefix
    typeset ifs fd line k v kv

    ifs="$IFS"
    IFS=''
    set -f
    fd=$1
    typeset -n x=$2
    x=()
    typeset +n x

    while read -r -u$fd line; do
        k=${line%%=*}
        v=${line#"$k"=}
        typeset -n x=$2.$k
        x[${#x[@]}]=$v
        if [[ $3 != '' ]] then
            print "$3$k=\"$v\""
        fi
        typeset +n x
    done
    set +f
    IFS="$ifs"
}
