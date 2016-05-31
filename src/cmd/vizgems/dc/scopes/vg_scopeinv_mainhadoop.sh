
export cid=$CID aid=$AID ip=$IP user=$USER pass=$PASS cs=$CS
export targettype=$TARGETTYPE scopetype=$SCOPETYPE servicelevel=$SERVICELEVEL

set -o pipefail

export INVMODE=y

tools=vg_exec_cmd_mainhadoop

urlbase=http://$IP:8088/ws/v1/cluster
if [[ $1 == *urlbase=?* ]] then
    urlbase=${1#*urlbase=}
    urlbase=${urlbase%%'&'*}
fi

cat > hadoopinv.txt <<#EOF
si_id|number||EXEC:hadoop?topic=info&var=clusterInfo.id&urlbase=$urlbase
si_version._total|number||EXEC:hadoop?topic=info&var=clusterInfo.hadoopVersion&urlbase=$urlbase
si_rmversion|number||EXEC:hadoop?topic=info&var=clusterInfo.resourceManagerVersion&urlbase=$urlbase
si_startedon|number||EXEC:hadoop?topic=info&var=clusterInfo.startedOn&type=time&urlbase=$urlbase
si_state._total|number||EXEC:hadoop?topic=info&var=clusterInfo.state&urlbase=$urlbase
si_hastate|number||EXEC:hadoop?topic=info&var=clusterInfo.haState&urlbase=$urlbase
si_sz_nodes_active._total|number||EXEC:hadoop?topic=metrics&var=clusterMetrics.totalNodes&urlbase=$urlbase
si_sz_memory_used._total|number||EXEC:hadoop?topic=metrics&var=clusterMetrics.totalMB&urlbase=$urlbase
si_queue|number||EXEC:hadoop?topic=scheduler&var=scheduler.schedulerInfo.queues.*.queueName&id=queueName&urlbase=$urlbase
si_sz_queuecapacity_used.|number||EXEC:hadoop?topic=scheduler&var=scheduler.schedulerInfo.queues.*.absoluteMaxCapacity&id=queueName&evar=*.users.*&urlbase=$urlbase
si_sz_queueapps_used.|number||EXEC:hadoop?topic=scheduler&var=scheduler.schedulerInfo.queues.*.maxActiveApplications&evar=*.users.*&id=queueName&urlbase=$urlbase
si_nodeid|number||EXEC:hadoop?topic=nodes&var=nodes.node*.id&id=id&urlbase=$urlbase
si_nodename|number||EXEC:hadoop?topic=nodes&var=nodes.node*.nodeHostName&id=id&urlbase=$urlbase
si_version|number||EXEC:hadoop?topic=nodes&var=nodes.node*.version&id=id&urlbase=$urlbase
si_rack|number||EXEC:hadoop?topic=nodes&var=nodes.node*.rack&id=id&urlbase=$urlbase
si_state|number||EXEC:hadoop?topic=nodes&var=nodes.node*.state&id=id&urlbase=$urlbase
si_lastupdate|number||EXEC:hadoop?topic=nodes&var=nodes.node*.lastHealthUpdate&id=id&type=time&urlbase=$urlbase
EOF

for tool in $tools; do
    EXECUSER=$USER EXECPASS=$PASS $SHELL $VG_SSCOPESDIR/current/exec/${tool} \
        $aid $ip $TARGETTYPE \
    < hadoopinv.txt
done
