function vg_dserverhelper_init {
    [[ $SWMNAME == "" ]] && REMOTE_USER=$SWMID . swmgetinfo
    return 0
}
