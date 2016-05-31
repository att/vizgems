function sutgraph {
    typeset lt=dot sz=0 tm=60

    while (( $# > 0 )) do
        case $1 in
        -lt)
            lt=$2
            shift 2
            ;;
        -sz)
            sz=$2
            shift 2
            ;;
        -tm)
            tm=$2
            shift 2
            ;;
        *)
            break
            ;;
        esac
    done

    if (( tm > 0 )) then
        { ulimit -t $tm; $lt "$@"; }
    else
        $lt "$@"
    fi
}
