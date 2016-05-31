function vg_alarm_inc_init {
    typeset -n gv=$1

    gv.filetype=REG
    gv.filepatincl='*'
    gv.filepatexcl='*.tmp'
    gv.filepatremove=''
    gv.filecheckuse=n
    gv.filesperround=512
    gv.filesinparallel=64
    gv.fileremove=y
    gv.filecollector=vg_alarm_collector
    gv.filesync=n
    gv.filerecord=n

    gv.mainspace=500

    typeset +n gv

    if [[ $ACCEPTOLDINCOMING == '' ]] then
        if [[ -f $VGMAINDIR/dpfiles/config.sh ]] then
            . $VGMAINDIR/dpfiles/config.sh
        fi
        export ACCEPTOLDINCOMING=${ACCEPTOLDINCOMING:-n}
        if [[ $ACCEPTOLDINCOMING == +([0-9]) ]] then
            ACCEPTOLDINCOMING=${ACCEPTOLDINCOMING}d
        fi
    fi
}
