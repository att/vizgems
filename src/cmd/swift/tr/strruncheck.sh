function strruncheck { # exitfile
    if [[ -f $1 ]] then
        return 1
    fi
    return 0
}
