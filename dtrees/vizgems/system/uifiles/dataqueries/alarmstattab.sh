querydata=(
    args=(
        dtools='inv alarm alarmstat'
        vtools='alarmstattab'
    )
    dt_inv=(
        poutlevel=o
        runinparallel=y
    )
    dt_alarm=(
        inlevels=o
    )
    dt_alarmstat=(
        inlevels=o
    )
    vt_alarmstattab=(
        grouporder='asset c1'
        metricattr=(
            bgcolor='#CCCCCC|#EEEEEE'
        )
        stattabattr=(
            bgcolor='#333333'
            color='#FFFFFF'
        )
    )
)
