function sutawk {
    typeset awk
    awk=$(whence -p nawk || whence -p awk)
    $awk "$@"
}
