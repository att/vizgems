function smsspacecheck { # $1: filesystem, $2: minspace
    typeset left

    left=$(df -P -m $1 | tail -1 | sutawk '{ print $4 }')
    if (( left > $2 )) then
        return 0
    else
        print ERROR less than $2 MB in $1
        return 1
    fi
}
