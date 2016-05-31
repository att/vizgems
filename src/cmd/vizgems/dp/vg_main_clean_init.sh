function vg_main_clean_init {
    typeset -n gv=$1

    gv.mainspace=16000
    gv.permspace=1000

    gv.cleanlogs=y
    gv.logremovedays=5
    gv.logcatdays=0

    gv.cleanrecs=y
    gv.recremovedays=2

    gv.cleaninc=y
    gv.incremovedays=2

    gv.cleancache=y
    gv.cacheremovedays=1

    gv.level1=y
    gv.level2=n
    gv.level3=n

    typeset +n gv
}
