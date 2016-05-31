function vg_alarm_proc_init {
    typeset -n gv=$1

    gv.custdir=${gv.maindir}/custids
    if ! sdpensuredirs ${gv.custdir}; then
        exit 1
    fi

    gv.mainspace=500

    gv.level1procsteps=raw2dds
    gv.level2procsteps=monthly
    gv.level3procsteps=yearly

    gv.raw2ddsjobs=16

    typeset +n gv
}
