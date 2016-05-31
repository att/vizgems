function vg_cm_inc_fileinfo {
    typeset -n gv=$1
    typeset -n ov=$2
    typeset inname=$3

    case $inname in
    cm.[0-9]*.xml)
        ov.date=0000.00.00
        ov.outname=$inname
        ;;
    esac

    typeset +n gv
    typeset +n ov
}
