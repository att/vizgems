function sdpmv { # fromfile tofile
    if [[ $1 -ef $2 ]] then
        return 0
    fi
    if cmp $1 $2 > /dev/null 2>&1; then
        rm $1
    else
        mv $1 $2
    fi
    return 0
}
