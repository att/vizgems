if [[ ${vars.date} == '' && ${vars.fdate} == '' && ${vars.ldate} == '' ]] then
    vars.fdate=$(printf '%(%Y.%m.%d-00:00:00)T')
    vars.ldate=$(printf '%(%Y.%m.%d-23:59:59)T')
fi
querydata=(
    args=(
        dtools='inv alarm stat'
        vtools='alarmtab stattab'
    )
    dt_inv=(
        poutlevel=c
        runinparallel=y
    )
    dt_alarm=(
        inlevels=o
    )
    dt_stat=(
        inlevels=o
    )
    vt_alarmtab=(
        pnodeattr=(
            label='Unknown Assets'
        )
        pnodenextqid='nextqid/_str_detailed'
    )
    vt_stattab=(
        grouporder='asset c1'
        metricattr=(
            bgcolor='#CCCCCC|#EEEEEE'
        )
        stattabattr=(
            bgcolor='#333333'
            color='#FFFFFF'
        )
        titleattr=(
            label='%(stat_id)%'
        )
    )
)
