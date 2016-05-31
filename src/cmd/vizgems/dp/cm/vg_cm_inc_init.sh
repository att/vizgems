function vg_cm_inc_init {
    typeset -n gv=$1

    gv.filetype=REG
    gv.filepatincl='*'
    gv.filepatexcl='*.tmp'
    gv.filepatremove=''
    gv.filecheckuse=n
    gv.filesperround=512
    gv.filesinparallel=64
    gv.fileremove=y
    gv.filecollector=
    gv.filesync=n
    gv.filerecord=n

    gv.mainspace=500

    typeset +n gv
}
