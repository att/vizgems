
export LC_ALL=C LANG=C
export TZ=SWIFT_CFG_SYSTZ
export SWIFTINSTANCE=vg
export SWIFTINSTANCENAME=VG

export SWIFTDATADIR=SWIFT_CFG_DSYSTEMDIR
export SWIFTDATEDIR=data/main
export SWIFTDSERVERHELPER=vg_dserverhelper

export SWIFTWEBSERVER='SWIFT_CFG_WTYPE://SWIFT_CFG_WHOST:SWIFT_CFG_WPORT'
export SWIFTWEBHOME=SWIFT_CFG_DWWWDIR
export SWIFTWEBPREFIX='SWIFT_CFG_WPREFIX'
export SWIFTCGIPLAINKEYS='query|qid|kind|pid|tryasid|winw|winh|name|secret|prev|updaterate|reruninv|tzoverride|update|statdisp|statgroup|alarm_severity|alarm_tmode'

export SWIFTLLTILESERVER='SWIFT_CFG_WOSMSERVER'
export SWIFTLLTILEATTR='Data CC BY-SA by <a href="http://openstreetmap.org">OpenStreetMap</a>'

export SWMADDRMAPFUNC='echo localhost'

export SWIFTSYSMODE=SWIFT_CFG_SYSMODE

export SWIFTTOOLS="suistyle"

export SWIFTDSERVERCACHEMAX=0

function add2path {
    export PATH=$1/bin:$PATH
    export FPATH=$1/fun:$FPATH
    export SWIFT_CFG_LDLIBRARYPATH=$1/lib:$SWIFT_CFG_LDLIBRARYPATH
}

add2path $SWIFTWEBHOME/proj/$SWIFTINSTANCE
add2path $SWIFTWEBHOME

umask 002

export VG_SYSNAME=SWIFT_CFG_SYSNAME
export VG_SYSMODE=SWIFT_CFG_SYSMODE

export VG_SSYSTEMDIR=SWIFT_CFG_SSYSTEMDIR
export VG_SSCOPESDIR=SWIFT_CFG_SSCOPESDIR
export VG_SWWWDIR=SWIFT_CFG_SWWWDIR
export VG_SSNMPDIR=SWIFT_CFG_SSNMPDIR
export VG_DSYSTEMDIR=SWIFT_CFG_DSYSTEMDIR
export VG_DSCOPESDIR=SWIFT_CFG_DSCOPESDIR
export VG_DWWWDIR=SWIFT_CFG_DWWWDIR
export VG_DSNMPDIR=SWIFT_CFG_DSNMPDIR

case $(hostname) in
*.research.att.com)
    pref=SWIFT_CFG_DSYSTEMDIR
    export VGFILES_TTRANSFILE=$pref/files/trans_file
    export VGFILES_STRANSFILE=$pref/dpfiles/objecttrans.txt
    export VGFILES_ASSETSFILE=$pref/files/asset_list
    export VGFILES_PINGFILE=$pref/files/ping_this_list
    export VGFILES_HBAUDITFILE=$pref/files/hb_audit_list
    export VGFILES_HOSTSFILE=$pref/files/hosts_file
    export VGFILES_CPLTABFILE=$pref/files/cpltab
    export VGFILES_SOSSFILE=$pref/files/soss_file
    export VGFILES_LOGBMCFILE=$pref/files/log_cfg_bmc
    export VGFILES_LOGCLERTFILE=$pref/files/log_cfg_clert
    export VGFILES_LOGHEALTHFILE=$pref/files/log_cfg_health
    export VGFILES_LOGITOFILE=$pref/files/log_cfg_ito
    export VGFILES_LOGPROBEFILE=$pref/files/log_cfg_probe
    export VGFILES_LOGSNMPFILE=$pref/files/log_cfg_snmp
    export VGFILES_LOGSYSLOGFILE=$pref/files/log_cfg_syslog
    export VGFILES_LOGTMPFILESFILE=$pref/files/log_cfg_tmpfiles
    export VGFILES_PORTLISTFILE=$pref/files/portmon_list
    export VGFILES_SYSLOGSITEFILE=$pref/files/syslog_siteids
    export VGFILES_URLPROBEFILE=$pref/files/urlmonitor.cfg
    export VGFILES_HIGHALERTFILE=$pref/uifiles/highalertcusts.txt
    export VGFILES_BGPMONFILE=$pref/files/iplist
    export VGFILES_OSPFMONFILE=$pref/files/ospf_iplist
    export VGFILES_IVTPFILE=$pref/files/iv_throughput-be_tick
    export VGFILES_PSOFILE=$pref/files/portspeedoverride.txt
    ;;
*)
    pref1=SWIFT_CFG_DSYSTEMDIR
    pref2=/export/home/swift/igems
    export VGFILES_TTRANSFILE=$pref2/cfg/trans_file
    export VGFILES_STRANSFILE=$pref1/dpfiles/objecttrans.txt
    export VGFILES_ASSETSFILE=$pref2/cfg/asset_list
    export VGFILES_PINGFILE=$pref2/cfg/ping_this_list
    export VGFILES_HBAUDITFILE=$pref2/cfg/hb_audit_list
    export VGFILES_HOSTSFILE=$pref2/cfg/hosts_file
    export VGFILES_CPLTABFILE=/tone/cpl/cdata/camsetc/cpltab
    export VGFILES_SOSSFILE=$pref2/cfg/soss_file
    export VGFILES_LOGBMCFILE=$pref2/cfg/log_cfg_bmc
    export VGFILES_LOGCLERTFILE=$pref2/cfg/log_cfg_clert
    export VGFILES_LOGHEALTHFILE=$pref2/cfg/log_cfg_health
    export VGFILES_LOGITOFILE=$pref2/cfg/log_cfg_ito
    export VGFILES_LOGPROBEFILE=$pref2/cfg/log_cfg_probe
    export VGFILES_LOGSNMPFILE=$pref2/cfg/log_cfg_snmp
    export VGFILES_LOGSYSLOGFILE=$pref2/cfg/log_cfg_syslog
    export VGFILES_LOGTMPFILESFILE=$pref2/cfg/log_cfg_tmpfiles
    export VGFILES_PORTLISTFILE=$pref2/cfg/portmon_list
    export VGFILES_SYSLOGSITEFILE=$pref2/cfg/syslog_siteids
    export VGFILES_URLPROBEFILE=$pref2/cfg/urlmonitor.cfg
    export VGFILES_HOSTSPOSTPROCESS=$pref2/bin/host_edit.sh
    export VGFILES_HIGHALERTFILE=$pref1/uifiles/highalertcusts.txt
    export VGFILES_BGPMONFILE=$pref2/cfg/BGPmon/iplist
    export VGFILES_OSPFMONFILE=$pref2/cfg/ospf_iplist
    export VGFILES_IVTPFILE=$pref2/cfg/iv_throughput-be_tick
    export VGFILES_PSOFILE=$pref2/cfg/portspeedoverride.txt
    ;;
esac
