files=(
    ttrans=(
        name=trans
        location=$VGFILES_TTRANSFILE
        logfile=$VGFILES_TTRANSFILE.log
        group="vg_att_admin|vg_att_user"
        fields=(
            field1=(
                name=name1
                text="From Name"
                type=text
                info="the asset name to translate"
                size=100
            )
            field2=(
                name=name2
                text="To Name"
                type=text
                info="the translated asset name"
                size=100
            )
        )
        bulkinsertfield=(
            name=insertfile
            text="Bulk Insert File"
            info="specifies a file of entries to insert to the main file - must be in the same format as the main file"
            format="orig_name|+|standard_name\n('|+|' is the field delimiter)"
            example="mchmts97|+|g06589mts0097"
        )
        recordchecker=vg_ttranschecker
        recordsplitter=vg_ttranssplitter
        fileeditor=vg_fileeditor
        fileprinter=vg_fileprinter
        headersep=""
        footersep=""
        summary="The Asset Translation File"
        description="this file is used to translate client aliases, non-conforming asset names, and ip addresses to a standard naming convention that maps to the Ticketing Database"
    )
    strans=(
        name=hev_trans
        location=$VGFILES_STRANSFILE
        logfile=$VGFILES_STRANSFILE.log
        group="vg_att_admin|vg_att_user"
        fields=(
            field1=(
                name=name1
                text="From Name"
                type=text
                info="the asset name to translate"
                size=100
            )
            field2=(
                name=name2
                text="To Name"
                type=text
                info="the translated asset name"
                size=100
            )
        )
        bulkinsertfield=(
            name=insertfile
            text="Bulk Insert File"
            info="specifies a file of entries to insert to the main file - must be in the same format as the main file"
            format="orig_name|+|standard_name\n('|+|' is the field delimiter)"
            example="s98|+|j08501web98"
        )
        recordchecker=vg_stranschecker
        recordsplitter=vg_stranssplitter
        fileeditor=vg_fileeditor
        fileprinter=vg_fileprinter
        headersep=""
        footersep=""
        summary="The HEV Asset Translation File"
        description="this file is used by the Hosting Element Visualizer, it is used to translate non-conforming asset names to the AT&T Hosting naming conventions required by the Visualizer"
    )
    assets=(
        name=assets
        location=$VGFILES_ASSETSFILE
        logfile=$VGFILES_ASSETSFILE.log
        group="vg_att_admin|vg_att_user"
        fields=(
            field1=(
                name=name
                text="Asset Name"
                type=text
                info="the asset name"
            )
        )
        bulkinsertfield=(
            name=insertfile
            text="Bulk Insert File"
            info="specifies a file of entries to insert to the main file - must be in the same format as the main file"
            format="asset_name"
            example="j08501web98"
        )
        recordchecker=vg_assetschecker
        recordsplitter=vg_assetssplitter
        fileeditor=vg_fileeditor
        fileprinter=vg_fileprinter
        headersep=""
        footersep=""
        summary="The Asset Database File"
        description="this file contains all known assets being processed by the EMS, data is derived from the CompuLert Entity Database, BMC Patrol PET configuration files, RuniGEMS Ping List, LWKM audit List, and locally maintained asset_list_addons files"
    )
    ping=(
        name=ping
        location=$VGFILES_PINGFILE
        logfile=$VGFILES_PINGFILE.log
        group="vg_att_admin|vg_att_user"
        fields=(
            field1=(
                name=ip
                text="IP Address"
                type=text
                info="the ip address to ping"
            )
            field2=(
                name=object
                text="Name"
                type=text
                info="the asset name"
                helper=vg_valuehelper
            )
        )
        bulkinsertfield=(
            name=insertfile
            text="Bulk Insert File"
            info="specifies a file of entries to insert to the main file - must be in the same format as the main file"
            format="ip_address|+|asset_name\n('|+|' is the field delimiter)"
            example="10.17.24.53|+|nbawa027"
        )
        recordchecker=vg_pingchecker
        recordsplitter=vg_pingsplitter
        fileeditor=vg_fileeditor
        fileprinter=vg_fileprinter
        headersep=""
        footersep=""
        summary="The Ping List File"
        description="this files contains ip and host information for the RuniGEMS probing module, please use the hosting naming convention when updating this file"
    )
    hbaudit=(
        name=hbaudit
        location=$VGFILES_HBAUDITFILE
        logfile=$VGFILES_HBAUDITFILE.log
        group="vg_att_admin|vg_att_user"
        fields=(
            field1=(
                name=object
                text="Name"
                type=text
                info="the asset name"
                helper=vg_valuehelper
            )
        )
        bulkinsertfield=(
            name=insertfile
            text="Bulk Insert File"
            info="specifies a file of entries to insert to the main file - must be in the same format as the main file"
            format="asset_name"
            example="nbawa027"
        )
        recordchecker=vg_hbauditchecker
        recordsplitter=vg_hbauditsplitter
        fileeditor=vg_fileeditor
        fileprinter=vg_fileprinter
        headersep=""
        footersep=""
        summary="The Heartbeat Audit List File"
        description="this files contains host information for assets that are being monitored with the Light Weight Knowledge Module, it must be populated to perform heartbeat monitoring"
    )
    portlist=(
        name=portlist
        location=$VGFILES_PORTLISTFILE
        logfile=$VGFILES_PORTLISTFILE.log
        group="vg_att_admin|vg_att_user"
        fields=(
            field1=(
                name=target
                text="Target"
                type=text
                info="a URL, a server name, or an IP address"
            )
            field2=(
                name=port
                text="Port Number"
                type=text
                info="the port number to probe"
            )
            field3=(
                name=timeout
                text="Timeout (1-60)"
                type=text
                info="the number of seconds to wait for a response"
            )
            field4=(
                name=assetname
                text="Asset Name"
                type=text
                info="the asset name"
            )
        )
        recordchecker=vg_portlistchecker
        recordsplitter=vg_portlistsplitter
        fileeditor=vg_fileeditor
        fileprinter=vg_fileprinter
        headersep=""
        footersep=""
        summary="The Port List File"
        description="remote probing file for target elements, used to ping port 80 or 443"
    )
    urlprobe=(
        name=urlprobe
        location=$VGFILES_URLPROBEFILE
        logfile=$VGFILES_URLPROBEFILE.log
        group="vg_att_admin|vg_att_user"
        fields=(
            field1=(
                name=assetname
                text="Asset Name"
                type=text
                info="the asset name"
            )
            field2=(
                name=urlstring
                text="URL"
                type=text
                info="the URL"
            )
            field3=(
                name=retries
                text="Retries (1-2)"
                type=text
                info="the number of retries"
            )
            field4=(
                name=waittime
                text="Wait Time (1-30)"
                type=text
                info="the number of seconds to wait for a response"
            )
        )
        recordchecker=vg_urlprobechecker
        recordsplitter=vg_urlprobesplitter
        fileeditor=vg_fileeditor
        fileprinter=vg_fileprinter
        headersep=""
        footersep=""
        summary="The URL Probe File"
        description="list of URLs to probe"
    )

#    hosts=(
#        name=hosts
#        location=$VGFILES_HOSTSFILE
#        logfile=$VGFILES_HOSTSFILE.log
#        group="vg_att_admin"
#        fields=(
#            field1=(
#                name=ip
#                text="IP Address"
#                type=ipaddr
#                info="the IP address of the system, eg: 10.19.20.180"
#            )
#            field2=(
#                name=names
#                text="System Name"
#                type=text
#                info="the official name and (optional) aliases of the system: eg: n08774apd0005 artemis"
#            )
#        )
#        recordchecker=vg_hostschecker
#        recordsplitter=vg_hostssplitter
#        fileeditor=vg_fileeditor
#        fileprinter=vg_fileprinter
#        postprocess=$VGFILES_HOSTSPOSTPROCESS
#        headersep=""
#        footersep=""
#        summary="The Hosts File"
#        description="Update this file when performing Client Implementations, please follow the Hosting naming convention standards"
#    )
#    cpltab=(
#        name=cpltab
#        location=$VGFILES_CPLTABFILE
#        logfile=$VGFILES_CPLTABFILE.log
#        group="vg_att_admin"
#        fields=(
#            field1=(
#                name=id
#                text="DAP Number"
#                type=text
#                info="The DAP number"
#            )
#            field2=(
#                name=amode
#                text="Action"
#                type=choice
#                choice1="respawn|respawn"
#                choice2="off|off"
#                info="This defines the run-level in which this entry is to be processed"
#            )
#            field3=(
#                name=process
#                text="Process"
#                type=text
#                info="Specify the command to be executed"
#                size=100
#            )
#        )
#        recordchecker=vg_cpltabchecker
#        recordsplitter=vg_cpltabsplitter
#        fileeditor=vg_fileeditor
#        fileprinter=vg_fileprinter
#        headersep="#CLERT DAPS - OPS USE ONLY"
#        footersep=""
#        summary="The DAP File"
#        description="This file is used to add or remove Compulert Data Acquisiton Points DAP's. This file is normally updated when assigning dap to Cisco Terminal Servers"
#    )
    log_cfg_bmc=(
        name=log_cfg_bmc
        location=$VGFILES_LOGBMCFILE
        logfile=$VGFILES_LOGBMCFILE.log
        group="vg_att_admin"
        fields=(
            field1=(
                name=conffile
                text="Configuration File"
                type=text
                info="the name of a RuniGEMS configuration file"
            )
        )
        recordchecker=vg_logpatrolchecker
        recordsplitter=vg_logpatrolsplitter
        fileeditor=vg_fileeditor
        fileprinter=vg_fileprinter
        headersep=""
        footersep=""
        summary="The BMC Log Configuration File"
        description="file that contains all log configuration files for the iGEMS agent (BMC Patrol Classic)"
    )
    log_cfg_clert=(
        name=log_cfg_clert
        location=$VGFILES_LOGCLERTFILE
        logfile=$VGFILES_LOGCLERTFILE.log
        group="vg_att_admin"
        fields=(
            field1=(
                name=conffile
                text="Configuration File"
                type=text
                info="the name of a RuniGEMS configuration file"
            )
        )
        recordchecker=vg_logpatrolchecker
        recordsplitter=vg_logpatrolsplitter
        fileeditor=vg_fileeditor
        fileprinter=vg_fileprinter
        headersep=""
        footersep=""
        summary="The CLERT Log Configuration File"
        description="file that contains all log configuration files for the iGEMS console agent (CompuLert)"
    )
    log_cfg_health=(
        name=log_cfg_health
        location=$VGFILES_LOGHEALTHFILE
        logfile=$VGFILES_LOGHEALTHFILE.log
        group="vg_att_admin"
        fields=(
            field1=(
                name=conffile
                text="Configuration File"
                type=text
                info="the name of a RuniGEMS configuration file"
            )
        )
        recordchecker=vg_logpatrolchecker
        recordsplitter=vg_logpatrolsplitter
        fileeditor=vg_fileeditor
        fileprinter=vg_fileprinter
        headersep=""
        footersep=""
        summary="The Health Log Configuration File"
        description="file that contains all log configuration files for the iGEMS server health statistics"
    )
    log_cfg_ito=(
        name=log_cfg_ito
        location=$VGFILES_LOGITOFILE
        logfile=$VGFILES_LOGITOFILE.log
        group="vg_att_admin"
        fields=(
            field1=(
                name=conffile
                text="Configuration File"
                type=text
                info="the name of a RuniGEMS configuration file"
            )
        )
        recordchecker=vg_logpatrolchecker
        recordsplitter=vg_logpatrolsplitter
        fileeditor=vg_fileeditor
        fileprinter=vg_fileprinter
        headersep=""
        footersep=""
        summary="The ITO Log Configuration File"
        description="file that contains all log configuration files for the iGEMS server ito statistics"
    )
    log_cfg_probe=(
        name=log_cfg_probe
        location=$VGFILES_LOGPROBEFILE
        logfile=$VGFILES_LOGPROBEFILE.log
        group="vg_att_admin"
        fields=(
            field1=(
                name=conffile
                text="Configuration File"
                type=text
                info="the name of a RuniGEMS configuration file"
            )
        )
        recordchecker=vg_logpatrolchecker
        recordsplitter=vg_logpatrolsplitter
        fileeditor=vg_fileeditor
        fileprinter=vg_fileprinter
        headersep=""
        footersep=""
        summary="The PROBE Log Configuration File"
        description="file that contains all log configuration files for the iGEMS remote probing module"
    )
    log_cfg_snmp=(
        name=log_cfg_snmp
        location=$VGFILES_LOGSNMPFILE
        logfile=$VGFILES_LOGSNMPFILE.log
        group="vg_att_admin"
        fields=(
            field1=(
                name=conffile
                text="Configuration File"
                type=text
                info="the name of a RuniGEMS configuration file"
            )
        )
        recordchecker=vg_logpatrolchecker
        recordsplitter=vg_logpatrolsplitter
        fileeditor=vg_fileeditor
        fileprinter=vg_fileprinter
        headersep=""
        footersep=""
        summary="The SNMP Log Configuration File"
        description="file that contains all log configuration files for SNMP based surveillance technologies (iGEMS express, LWKM, iGEMS Probe=OVIS, Network devices, Custom agents, Brick Firewalls)"
    )
    log_cfg_syslog=(
        name=log_cfg_syslog
        location=$VGFILES_LOGSYSLOGFILE
        logfile=$VGFILES_LOGSYSLOGFILE.log
        group="vg_att_admin"
        fields=(
            field1=(
                name=conffile
                text="Configuration File"
                type=text
                info="the name of a RuniGEMS configuration file"
            )
        )
        recordchecker=vg_logpatrolchecker
        recordsplitter=vg_logpatrolsplitter
        fileeditor=vg_fileeditor
        fileprinter=vg_fileprinter
        headersep=""
        footersep=""
        summary="The SYSLOG Log Configuration File"
        description="file that contains all log configuration files for SYSLOG based surveillance technologies (Network devices, Syslog Hosts, & Firewalls)"
    )
    log_cfg_tmpfiles=(
        name=log_cfg_tmpfiles
        location=$VGFILES_LOGTMPFILESFILE
        logfile=$VGFILES_LOGTMPFILESFILE.log
        group="vg_att_admin"
        fields=(
            field1=(
                name=conffile
                text="Configuration File"
                type=text
                info="the name of a RuniGEMS configuration file"
            )
        )
        recordchecker=vg_logpatrolchecker
        recordsplitter=vg_logpatrolsplitter
        fileeditor=vg_fileeditor
        fileprinter=vg_fileprinter
        headersep=""
        footersep=""
        summary="The TMPFILES Log Configuration File"
        description="file that contains all log configuration files for the Post Processed alarms"
    )
    soss=(
        name=soss
        location=$VGFILES_SOSSFILE
        logfile=$VGFILES_SOSSFILE.log
        group="vg_att_admin|vg_att_user"
        fields=(
            field1=(
                name=custname
                text="SOSS Asset"
                type=text
                info="the name of an SOSS asset"
            )
        )
        bulkinsertfield=(
            name=insertfile
            text="Bulk Insert File"
            info="specifies a file of entries to insert to the main file - must be in the same format as the main file"
            format="cust_name"
            example="acme"
        )
        recordchecker=vg_sosschecker
        recordsplitter=vg_sosssplitter
        fileeditor=vg_fileeditor
        fileprinter=vg_fileprinter
        headersep=""
        footersep=""
        summary="The SOSS File"
        description="This file controls SOSS assets the GCSC will receive tickets for. Update this file with the hostname or IP of the SOSS managed asset, exactly as defined on the Express SIP"
    )
    syslog_siteids=(
        name=syslog_siteids
        location=$VGFILES_SYSLOGSITEFILE
        logfile=$VGFILES_SYSLOGSITEFILE.log
        group="vg_att_admin|vg_att_user"
        fields=(
            field1=(
                name=sitename
                text="Site and Device Acronym"
                type=text
                info="the 6 character site id followed by the device acronym (usually 3 chars)"
            )
        )
        recordchecker=vg_syslogsitechecker
        recordsplitter=vg_syslogsitesplitter
        fileeditor=vg_fileeditor
        fileprinter=vg_fileprinter
        headersep=""
        footersep=""
        summary="The SYSLOG Siteids File"
        description="file that contains the prefixes (siteid and device acronym) of all devices allowed to send SYSLOG data to the EMS"
    )
    highalert=(
        name=highalert
        location=$VGFILES_HIGHALERTFILE
        logfile=$VGFILES_HIGHALERTFILE.log
        group="vg_att_admin|vg_att_user"
        fields=(
            field1=(
                name=customer
                text="Customer"
                type=text
                info="the customer name"
                helper=vg_valuehelper
            )
        )
        recordchecker=vg_halertchecker
        recordsplitter=vg_halertsplitter
        fileeditor=vg_fileeditor
        fileprinter=vg_fileprinter
        headersep=""
        footersep=""
        summary="The High Alert Customer File"
        description="this file contains the customers that should appear in the High Alert Dashboard"
    )
    bgpmon=(
        name=bgpmon
        location=$VGFILES_BGPMONFILE
        logfile=$VGFILES_BGPMONFILE.log
        group="vg_att_admin|vg_att_user"
        fields=(
            field1=(
                name=ip
                text="IP Address"
                type=text
                info="ip address of device"
            )
        )
        bulkinsertfield=(
            name=insertfile
            text="Bulk Insert File"
            info="specifies a file of entries to insert to the main file - must be in the same format as the main file"
            format="ip_address"
            example="10.17.24.53"
        )
        recordchecker=vg_bgpmonchecker
        recordsplitter=vg_bgpmonsplitter
        fileeditor=vg_fileeditor
        fileprinter=vg_fileprinter
        headersep=""
        footersep=""
        summary="The BGPmon List File"
        description="this file contains the ip addresses for BGP routing events"
    )
    ospfmon=(
        name=ospfmon
        location=$VGFILES_OSPFMONFILE
        logfile=$VGFILES_OSPFMONFILE.log
        group="vg_att_admin|vg_att_user"
        fields=(
            field1=(
                name=ip
                text="IP Address"
                type=text
                info="ip address of device"
            )
        )
        bulkinsertfield=(
            name=insertfile
            text="Bulk Insert File"
            info="specifies a file of entries to insert to the main file - must be in the same format as the main file"
            format="ip_address"
            example="10.17.24.53"
        )
        recordchecker=vg_ospfmonchecker
        recordsplitter=vg_ospfmonsplitter
        fileeditor=vg_fileeditor
        fileprinter=vg_fileprinter
        headersep=""
        footersep=""
        summary="The OSPFmon List File"
        description="this file contains the ip addresses for OSPF events"
    )
    ivthroughputbe=(
        name=iv_throughput_be
        location=$VGFILES_IVTPFILE
        logfile=$VGFILES_IVTPFILE.log
        group="vg_att_admin|vg_att_user"
        fields=(
            field1=(
                name=device
                text="Network Device"
                type=text
                info="the name of the backend device"
                size=100
            )
            field2=(
                name=port
                text="Port Number"
                type=text
                info="the port number"
                size=100
            )
        )
        recordchecker=vg_ivthroughputbechecker
        recordsplitter=vg_ivthroughputbesplitter
        fileeditor=vg_fileeditor
        fileprinter=vg_fileprinter
        headersep=""
        footersep=""
        summary="The Backend IV Throughput Ticketing Include File"
        description="this file contains the port numbers on backend network devices that may generate tickets"
    )
    portspeedoverride=(
        name=port_speed_override
        location=$VGFILES_PSOFILE
        logfile=$VGFILES_PSOFILE.log
        group="vg_att_admin|vg_att_user"
        fields=(
            field1=(
                name=vgasset
                text="Asset Name"
                type=text
                info="the name of the asset"
                helper=vg_valuehelper
            )
            field2=(
                name=vgswport
                text="Interface Name"
                type=text
                info="the interface name to override"
                helper=vg_valuehelper
            )
            field3=(
                name=vgswspeed
                text="Port Speed (bps)"
                type=text
                info="this is the current port speed used for this interface"
                helper=vg_valuehelper
            )
        )
        recordchecker=vg_portspeedoverridechecker
        recordsplitter=vg_portspeedoverridesplitter
        fileeditor=vg_fileeditor
        fileprinter=vg_fileprinter
        headersep=""
        footersep=""
        summary="The Port Speed Override File"
        description="This file contains the modified interface port speed that will override the discovered port speed."
    )
)
