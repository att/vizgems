function sdpjobcntl { # $1: maxjobs
    typeset mj=$1

    typeset j l

    for (( j = mj; j >= mj; )) do
        set -A l $(jobs -p)
        j=${#l[@]}
        if (( j < mj )) then
            break
        fi
        sleep 1
    done
}
