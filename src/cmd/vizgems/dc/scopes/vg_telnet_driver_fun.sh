
function tn_wait { # $1=timeout $2=string to match
    typeset s

    while read -p -r -t$1 s; do
        [[ $s == $2 ]] && break
    done
}

function tn_drain { # $1=timeout
    typeset s

    while read -p -r -t$1 s; do
        :
    done
}

function tn_read { # $1=timeout $2=variable to use
    typeset -n s=$2

    read -p -r -t$1 s
}

function tn_write { # $1=string to write
    print -p -- "$1"
}
