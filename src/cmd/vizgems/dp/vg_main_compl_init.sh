function vg_main_compl_init {
    typeset -n gv=$1

    gv.mainspace=100

    gv.completeday1=1.50h
    gv.completeday2=2.50h

    gv.level1=y
    gv.level2=y
    gv.level3=y

    typeset +n gv
}
