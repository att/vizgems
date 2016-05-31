function strensuredirs { # [-c] dir1, ...
    typeset cflag i

    if [[ $1 == -c ]] then
        cflag=1
        shift
    fi
    for i in "$@"; do
        if [[ ! -d $i ]] then
            if [[ $cflag == 1 ]] then
                mkdir -p $i
                if [[ ! -d $i ]] then
                    print -u2 ERROR cannot create $i
                    return 1
                fi
            else
                print -u2 ERROR $i does not exist
                return 1
            fi
        fi
    done
    return 0
}
