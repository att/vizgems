function vg_cntl_init { # $1 = gvars
    typeset -n gv=$1

    gv.files="
        inc.inv
        inc.alarm
        inc.stat
        inc.generic
        inc.cm
        proc.inv
        proc.alarm
        proc.alarm2
        proc.alarm3
        proc.stat
        proc.stat2
        proc.stat3
        proc.generic
        proc.generic2
        proc.generic3
        proc.cm
        prop.group1 prop.group2 prop.all
        compl.main
        clean.main
    "

    typeset +n gv
}
