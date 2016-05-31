
rules=(
    [nova-api]=(
        counted=y
        anchor='@(TRACE|INFO|WARNING|ERROR)'
        aid=nova-api
        sev=3
        tmode=keep
        [exclude]=(
            [0]=( level=INFO txt='*' )
            [1]=( level=WARNING txt='*' )
            [2]=( level=TRACE txt='*' )
            [3]=( level=ERROR txt='*Trying again in*' )
            [4]=( level=ERROR txt='*Authentication required*' )
        )
        [include]=(
            [0]=( level=ERROR txt='*HTTPInternalServerError*' sev=1 tmode=keep )
        )
    )
    [nova-cert]=(
        counted=y
        anchor='@(TRACE|INFO|WARNING|ERROR)'
        aid=nova-cert
        sev=3
        tmode=keep
        [exclude]=(
            [0]=( level=INFO txt='*' )
            [1]=( level=WARNING txt='*' )
            [2]=( level=TRACE txt='*' )
            [3]=( level=ERROR txt='*Trying again in*' )
        )
        [include]=( [1]=1 )
    )
    [nova-compute]=(
        counted=y
        anchor='@(TRACE|INFO|WARNING|ERROR)'
        aid=nova-compute
        sev=3
        tmode=keep
        [exclude]=(
            [0]=( level=INFO txt='*' )
            [1]=( level=WARNING txt='*' )
            [2]=( level=TRACE txt='*' )
            [3]=( level=ERROR txt='*Trying again in*' )
        )
        [include]=(
            [0]=( level=ERROR txt='*model server*' sev=1 tmode=keep )
        )
    )
    [nova-conductor]=(
        counted=y
        anchor='@(TRACE|INFO|WARNING|ERROR)'
        aid=nova-conductor
        sev=3
        tmode=keep
        [exclude]=(
            [0]=( level=INFO txt='*' )
            [1]=( level=WARNING txt='*' )
            [2]=( level=TRACE txt='*' )
            [3]=( level=ERROR txt='*Trying again in*' )
        )
        [include]=( [1]=1 )
    )
    [nova-consoleauth]=(
        counted=y
        anchor='@(TRACE|INFO|WARNING|ERROR)'
        aid=nova-consoleauth
        sev=3
        tmode=keep
        [exclude]=(
            [0]=( level=INFO txt='*' )
            [1]=( level=WARNING txt='*' )
            [2]=( level=TRACE txt='*' )
            [3]=( level=ERROR txt='*Trying again in*' )
        )
        [include]=( [1]=1 )
    )
    [nova-manage]=(
        counted=y
        anchor='@(TRACE|INFO|WARNING|ERROR)'
        aid=nova-manage
        sev=3
        tmode=keep
        [exclude]=(
            [0]=( level=INFO txt='*' )
            [1]=( level=WARNING txt='*' )
            [2]=( level=TRACE txt='*' )
            [3]=( level=ERROR txt='*Trying again in*' )
        )
        [include]=( [1]=1 )
    )
    [nova-novncproxy]=(
        counted=y
        anchor='@(TRACE|INFO|WARNING|ERROR)'
        aid=nova-novncproxy
        sev=3
        tmode=keep
        [exclude]=(
            [0]=( level=INFO txt='*' )
            [1]=( level=WARNING txt='*' )
            [2]=( level=TRACE txt='*' )
            [3]=( level=ERROR txt='*Trying again in*' )
        )
        [include]=( [1]=1 )
    )
    [nova-scheduler]=(
        counted=y
        anchor='@(TRACE|INFO|WARNING|ERROR)'
        aid=nova-scheduler
        sev=3
        tmode=keep
        [exclude]=(
            [0]=( level=INFO txt='*' )
            [1]=( level=WARNING txt='*' )
            [2]=( level=TRACE txt='*' )
            [3]=( level=ERROR txt='*Trying again in*' )
        )
        [include]=( [1]=1 )
    )

    [cinder-api]=(
        counted=y
        anchor='@(TRACE|INFO|WARNING|ERROR)'
        aid=cinder-api
        sev=3
        tmode=keep
        [exclude]=(
            [0]=( level=INFO txt='*' )
            [1]=( level=WARNING txt='*' )
            [2]=( level=TRACE txt='*' )
            [3]=( level=ERROR txt='*Trying again in*' )
        )
        [include]=( [1]=1 )
    )
    [cinder-volume]=(
        counted=y
        anchor='@(TRACE|INFO|WARNING|ERROR)'
        aid=cinder-volume
        sev=3
        tmode=keep
        [exclude]=(
            [0]=( level=INFO txt='*' )
            [1]=( level=WARNING txt='*' )
            [2]=( level=TRACE txt='*' )
            [3]=( level=ERROR txt='*Trying again in*' )
        )
        [include]=( [1]=1 )
    )
    [cinder-scheduler]=(
        counted=y
        anchor='@(TRACE|INFO|WARNING|ERROR)'
        aid=cinder-scheduler
        sev=3
        tmode=keep
        [exclude]=(
            [0]=( level=INFO txt='*' )
            [1]=( level=WARNING txt='*' )
            [2]=( level=TRACE txt='*' )
            [3]=( level=ERROR txt='*Trying again in*' )
        )
        [include]=(
            [0]=(
                level=WARNING txt='*volume service is down*' sev=1 tmode=keep
            )
            [1]=(
                level=WARNING txt='*Insufficient free space*' sev=2 tmode=keep
            )
        )
    )

    [dhcp-agent]=(
        counted=y
        anchor='@(TRACE|INFO|WARNING|ERROR)'
        aid=neutron-dhcp-agent
        sev=3
        tmode=keep
        [exclude]=(
            [0]=( level=INFO txt='*' )
            [1]=( level=WARNING txt='*' )
            [2]=( level=TRACE txt='*' )
            [3]=( level=ERROR txt='*Trying again in*' )
        )
        [include]=( [1]=1 )
    )
    [l3-agent]=(
        counted=y
        anchor='@(TRACE|INFO|WARNING|ERROR)'
        aid=neutron-l3-agent
        sev=3
        tmode=keep
        [exclude]=(
            [0]=( level=INFO txt='*' )
            [1]=( level=WARNING txt='*' )
            [2]=( level=TRACE txt='*' )
            [3]=( level=ERROR txt='*Trying again in*' )
        )
        [include]=( [1]=1 )
    )
    [neutron-metadata-agent]=(
        counted=y
        anchor='@(TRACE|INFO|WARNING|ERROR)'
        aid=neutron-metadata-agent
        sev=3
        tmode=keep
        [exclude]=(
            [0]=( level=INFO txt='*' )
            [1]=( level=WARNING txt='*' )
            [2]=( level=TRACE txt='*' )
            [3]=( level=ERROR txt='*Trying again in*' )
        )
        [include]=( [1]=1 )
    )
    [neutron-server]=(
        counted=y
        anchor='@(TRACE|INFO|WARNING|ERROR)'
        aid=neutron-server
        sev=3
        tmode=keep
        [exclude]=(
            [0]=( level=INFO txt='*' )
            [1]=( level=WARNING txt='*' )
            [2]=( level=TRACE txt='*' )
            [3]=( level=ERROR txt='*Trying again in*' )
        )
        [include]=( [1]=1 )
    )
    [openvswitch-agent]=(
        counted=y
        anchor='@(TRACE|INFO|WARNING|ERROR)'
        aid=neutron-openvswitch-agent
        sev=3
        tmode=keep
        [exclude]=(
            [0]=( level=INFO txt='*' )
            [1]=( level=WARNING txt='*' )
            [2]=( level=TRACE txt='*' )
            [3]=( level=ERROR txt='*Trying again in*' )
        )
        [include]=( [1]=1 )
    )
    [ovs-cleanup]=(
        counted=y
        anchor='@(TRACE|INFO|WARNING|ERROR)'
        aid=neutron-ovs-cleanup
        sev=3
        tmode=keep
        [exclude]=(
            [0]=( level=INFO txt='*' )
            [1]=( level=WARNING txt='*' )
            [2]=( level=TRACE txt='*' )
            [3]=( level=ERROR txt='*Trying again in*' )
        )
        [include]=( [1]=1 )
    )

    [admin]=(
        counted=y
        anchor='@(TRACE|INFO|WARNING|ERROR)'
        aid=keystone-admin
        sev=3
        tmode=keep
        [exclude]=(
            [0]=( level=INFO txt='*' )
            [1]=( level=WARNING txt='*' )
            [2]=( level=TRACE txt='*' )
            [3]=( level=ERROR txt='*Trying again in*' )
            [4]=( level=ERROR txt='*takes exactly * arguments*' )
        )
        [include]=( [1]=1 )
    )
    [main]=(
        counted=y
        anchor='@(TRACE|INFO|WARNING|ERROR)'
        aid=keystone-main
        sev=3
        tmode=keep
        [exclude]=(
            [0]=( level=INFO txt='*' )
            [1]=( level=WARNING txt='*' )
            [2]=( level=TRACE txt='*' )
            [3]=( level=ERROR txt='*Trying again in*' )
            [4]=( level=ERROR txt='*takes exactly * arguments*' )
        )
        [include]=( [1]=1 )
    )
    [keystone-manage]=(
        counted=y
        anchor='@(TRACE|INFO|WARNING|ERROR)'
        aid=keystone-manage
        sev=3
        tmode=keep
        [exclude]=(
            [0]=( level=INFO txt='*' )
            [1]=( level=WARNING txt='*' )
            [2]=( level=TRACE txt='*' )
            [3]=( level=ERROR txt='*Trying again in*' )
            [4]=( level=ERROR txt='*takes exactly * arguments*' )
        )
        [include]=( [1]=1 )
    )

    [heat-engine]=(
        counted=y
        anchor='@(TRACE|INFO|WARNING|ERROR)'
        aid=heat-engine
        sev=3
        tmode=keep
        [exclude]=(
            [0]=( level=INFO txt='*' )
            [1]=( level=WARNING txt='*' )
            [2]=( level=TRACE txt='*' )
            [3]=( level=ERROR txt='*Trying again in*' )
        )
        [include]=( [1]=1 )
    )
    [heat-api]=(
        counted=y
        anchor='@(TRACE|INFO|WARNING|ERROR)'
        aid=heat-api
        sev=3
        tmode=keep
        [exclude]=(
            [0]=( level=INFO txt='*' )
            [1]=( level=WARNING txt='*' )
            [2]=( level=TRACE txt='*' )
            [3]=( level=ERROR txt='*Trying again in*' )
        )
        [include]=( [1]=1 )
    )
    [heat-api-cfn]=(
        counted=y
        anchor='@(TRACE|INFO|WARNING|ERROR)'
        aid=heat-api-cfn
        sev=3
        tmode=keep
        [exclude]=(
            [0]=( level=INFO txt='*' )
            [1]=( level=WARNING txt='*' )
            [2]=( level=TRACE txt='*' )
            [3]=( level=ERROR txt='*Trying again in*' )
        )
        [include]=( [1]=1 )
    )

    [glance-api]=(
        counted=y
        anchor='@(TRACE|INFO|WARNING|ERROR)'
        aid=glance-api
        sev=3
        tmode=keep
        [exclude]=(
            [0]=( level=INFO txt='*' )
            [1]=( level=WARNING txt='*' )
            [2]=( level=TRACE txt='*' )
            [3]=( level=ERROR txt='*Trying again in*' )
        )
        [include]=( [1]=1 )
    )
    [glance-registry]=(
        counted=y
        anchor='@(TRACE|INFO|WARNING|ERROR)'
        aid=glance-registry
        sev=3
        tmode=keep
        [exclude]=(
            [0]=( level=INFO txt='*' )
            [1]=( level=WARNING txt='*' )
            [2]=( level=TRACE txt='*' )
            [3]=( level=ERROR txt='*Trying again in*' )
        )
        [include]=( [1]=1 )
    )

    [ceilometer-agent-notification]=(
        counted=y
        anchor='@(TRACE|INFO|WARNING|ERROR)'
        aid=ceilometer-agent-notification
        sev=3
        tmode=keep
        [exclude]=(
            [0]=( level=INFO txt='*' )
            [1]=( level=WARNING txt='*' )
            [2]=( level=TRACE txt='*' )
            [3]=( level=ERROR txt='*Trying again in*' )
            [4]=( level=ERROR txt='*Couldn?t obtain IP address of instance*' )
            [5]=( level=ERROR txt='*object of type * has no len*' )
            [6]=( level=ERROR txt='*Exception during message handling*' )
        )
        [include]=( [1]=1 )
    )
    [ceilometer-alarm-evaluator]=(
        counted=y
        anchor='@(TRACE|INFO|WARNING|ERROR)'
        aid=ceilometer-alarm-evaluator
        sev=3
        tmode=keep
        [exclude]=(
            [0]=( level=INFO txt='*' )
            [1]=( level=WARNING txt='*' )
            [2]=( level=TRACE txt='*' )
            [3]=( level=ERROR txt='*Trying again in*' )
            [4]=( level=ERROR txt='*Couldn?t obtain IP address of instance*' )
            [5]=( level=ERROR txt='*object of type * has no len*' )
            [6]=( level=ERROR txt='*Exception during message handling*' )
        )
        [include]=( [1]=1 )
    )
    [ceilometer-agent-compute]=(
        counted=y
        anchor='@(TRACE|INFO|WARNING|ERROR)'
        aid=ceilometer-agent-compute
        sev=3
        tmode=keep
        [exclude]=(
            [0]=( level=INFO txt='*' )
            [1]=( level=WARNING txt='*' )
            [2]=( level=TRACE txt='*' )
            [3]=( level=ERROR txt='*Trying again in*' )
            [4]=( level=ERROR txt='*Couldn?t obtain IP address of instance*' )
            [5]=( level=ERROR txt='*object of type * has no len*' )
            [6]=( level=ERROR txt='*Exception during message handling*' )
        )
        [include]=( [1]=1 )
    )
    [ceilometer-agent-central]=(
        counted=y
        anchor='@(TRACE|INFO|WARNING|ERROR)'
        aid=ceilometer-agent-central
        sev=3
        tmode=keep
        [exclude]=(
            [0]=( level=INFO txt='*' )
            [1]=( level=WARNING txt='*' )
            [2]=( level=TRACE txt='*' )
            [3]=( level=ERROR txt='*Trying again in*' )
            [4]=( level=ERROR txt='*Couldn?t obtain IP address of instance*' )
            [5]=( level=ERROR txt='*object of type * has no len*' )
            [6]=( level=ERROR txt='*Exception during message handling*' )
        )
        [include]=( [1]=1 )
    )
    [ceilometer-collector]=(
        counted=y
        anchor='@(TRACE|INFO|WARNING|ERROR)'
        aid=ceilometer-collector
        sev=3
        tmode=keep
        [exclude]=(
            [0]=( level=INFO txt='*' )
            [1]=( level=WARNING txt='*' )
            [2]=( level=TRACE txt='*' )
            [3]=( level=ERROR txt='*Trying again in*' )
            [4]=( level=ERROR txt='*Couldn?t obtain IP address of instance*' )
            [5]=( level=ERROR txt='*object of type * has no len*' )
            [6]=( level=ERROR txt='*Exception during message handling*' )
        )
        [include]=( [1]=1 )
    )
    [ceilometer-alarm-notifier]=(
        counted=y
        anchor='@(TRACE|INFO|WARNING|ERROR)'
        aid=ceilometer-alarm-notifier
        sev=3
        tmode=keep
        [exclude]=(
            [0]=( level=INFO txt='*' )
            [1]=( level=WARNING txt='*' )
            [2]=( level=TRACE txt='*' )
            [3]=( level=ERROR txt='*Trying again in*' )
            [4]=( level=ERROR txt='*Couldn?t obtain IP address of instance*' )
            [5]=( level=ERROR txt='*object of type * has no len*' )
            [6]=( level=ERROR txt='*Exception during message handling*' )
        )
        [include]=( [1]=1 )
    )
    [ceilometer-api]=(
        counted=y
        anchor='@(TRACE|INFO|WARNING|ERROR)'
        aid=ceilometer-api
        sev=3
        tmode=keep
        [exclude]=(
            [0]=( level=INFO txt='*' )
            [1]=( level=WARNING txt='*' )
            [2]=( level=TRACE txt='*' )
            [3]=( level=ERROR txt='*Trying again in*' )
            [4]=( level=ERROR txt='*Couldn?t obtain IP address of instance*' )
            [5]=( level=ERROR txt='*object of type * has no len*' )
            [6]=( level=ERROR txt='*Exception during message handling*' )
        )
        [include]=( [1]=1 )
    )
)
