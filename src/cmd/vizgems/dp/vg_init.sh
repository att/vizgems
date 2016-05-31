function vg_init { # gvars
    typeset -n gv=$1

    gv.maindir=${VGMAINDIR:-UNSET}
    gv.permdir=${VGPERMDIR}
    gv.mindate=2000.01.01
    gv.maxdate=9999.12.31
    gv.mainspace=1000
    gv.permspace=1000

    gv.datadir=main
    case $2 in
    inv)
        gv.feeddir=inv
        ;;
    alarm)
        gv.feeddir=alarm
        ;;
    stat)
        gv.feeddir=stat
        ;;
    generic)
        gv.feeddir=generic
        ;;
    main)
        gv.feeddir='@(inv|alarm|stat|autoinv|cm|health)'
        ;;
    cm)
        gv.feeddir=cm
        gv.datadir=cm
        gv.mindate=0000.00.00
        gv.maxdate=0000.00.00
        gv.cmfileremovedays=7
        ;;
    esac

    typeset +n gv
}
