function swmuseringroups {
    if [[ " $SWMGROUPS " == @(*\ ($1)\ *) ]] then
        return 0
    fi
    return 1
}
