function vg_cm_proc_init {
    typeset -n gv=$1

    gv.mainspace=500

    gv.level1procsteps=update
    gv.level2procsteps=''
    gv.level3procsteps=''

    gv.raw2ddsjobs=16

    typeset +n gv
}
