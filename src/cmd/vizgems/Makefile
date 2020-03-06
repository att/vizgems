:PACKAGE: swift graphviz ast dds:command swift:command X11:nolibrary

LICENSE = since=1996,author=ek

CCFLAGS += $$(CC.DLL)

.SOURCE.mk: .INSERT dp/inv dp/alarm dp/stat
.SOURCE: dc dc/netdisc dc/port dc/sched dc/scopes dc/url dc/wmi dc/xml
.SOURCE: dp dp/cm dp/inv dp/inv/profile dp/inv/threshold dp/alarm dp/alarm/ce
.SOURCE: dp/alarm/ce/ee dp/alarm/ce/pm dp/alarm/db dp/alarm/email
.SOURCE: dp/alarm/filter dp/alarm/gce dp/stat dp/stat/pce dp/generic
.SOURCE: ui ui/account ui/cm ui/data ui/export ui/filter ui/fm ui/fonts ui/help
.SOURCE: ui/rest ui/wusage
.SOURCE: ../swift/dp ../swift/ui
.SOURCE.h: /usr/include/json-c /usr/include/json /usr/local/include/json-c

.SOURCE.h: /usr/include/libxml2 /usr/local/include/libxml2

if CC.HOSTTYPE == "darwin.i386*" || CC.HOSTTYPE == "freebsd12.amd64*"
	FEATURE_NO64FS == 1
end

/* Top Level SWIFT Tools */

if NOSWIFTTOOLS != "y"
	$(INSTALLROOT)/bin/vg_cm_incoming :INSTALL: sdpincoming
	$(INSTALLROOT)/bin/vg_cm_processing :INSTALL: sdpprocessing
	$(INSTALLROOT)/bin/vg_inv_incoming :INSTALL: sdpincoming
	$(INSTALLROOT)/bin/vg_inv_processing :INSTALL: sdpprocessing
	$(INSTALLROOT)/bin/vg_alarm_incoming :INSTALL: sdpincoming
	$(INSTALLROOT)/bin/vg_alarm_processing :INSTALL: sdpprocessing
	$(INSTALLROOT)/bin/vg_stat_incoming :INSTALL: sdpincoming
	$(INSTALLROOT)/bin/vg_stat_processing :INSTALL: sdpprocessing
	$(INSTALLROOT)/bin/vg_generic_incoming :INSTALL: sdpincoming
	$(INSTALLROOT)/bin/vg_generic_processing :INSTALL: sdpprocessing

	$(INSTALLROOT)/bin/vg_main_complete :INSTALL: sdpcomplete
	$(INSTALLROOT)/bin/vg_main_cleanup :INSTALL: sdpcleanup
	$(INSTALLROOT)/bin/vg_control :INSTALL: sdpcontrol

	$(INSTALLROOT)/bin/vg_dserverhelper :INSTALL: suidserverhelper

	$(INSTALLROOT)/www/proj/vg/bin/vg_services :INSTALL: suiservices

	$(INSTALLROOT)/www/proj/vg/etc/vg_servicehb :INSTALL: suiservicehb
	$(INSTALLROOT)/www/proj/vg/etc/vg_prefhb :INSTALL: suiprefhb

	$(INSTALLROOT)/www/proj/vg/cgi-bin-members/vg_mserverhelper.cgi \
	:INSTALL: suimserverhelper.cgi
	$(INSTALLROOT)/www/proj/vg/cgi-bin-members/vg_dserverhelper.cgi \
	:INSTALL: suidserverhelper.cgi
end


/* Common Tools */

$(INSTALLROOT)/bin :INSTALLDIR: vg_hdr vg_main_compl_compress

$(INSTALLROOT)/fun :INSTALLDIR: \
vg_init vg_cntl_init \
vg_main_compl_init vg_main_compl_run \
vg_main_clean_init vg_main_clean_run

vg_hdr.h vg_hdr.sh :JOINT: \
vg_hdr.defs vg_alarm.schema vg_inv_node.schema vg_stat.schema
	suthdrgen -vp VG -fp vg_hdr $(*)

vgprofile_pub.h : vgprofile.h
	cp $(>) $(<)

vgprofile 1.0 :LIBRARY: vgprofile.c -ldds

vgthreshold_pub.h : vgthreshold.h
	cp $(>) $(<)

vgthreshold 1.0 :LIBRARY: vgthreshold.c -ldds

vgdb_pub.h : vgdb.h
	cp $(>) $(<)

vgdb 1.0 :LIBRARY: vgdb.c

vgfilter_pub.h : vgfilter.h
	cp $(>) $(<)

vgfilter 1.0 :LIBRARY: vgfilter.c -ldds

vgemail_pub.h : vgemail.h
	cp $(>) $(<)

vgemail 1.0 :LIBRARY: vgemail.c -ldds

vgce :: \
vgce.c cc.c match.c rule.c alarm.c ticket.c misc.c \
eeval.c exec.c parse.c print.c lex.c code.c pmem.c -ldds

vggce :: \
vggce.c graph.c corr.c -ldds

vgpce :: \
vgpce.c pcealarm.c pcenode.c pcerule.c vgpce.c -ldds

vg_prop_filter :: vg_prop_filter.c -ldds

vg_dq_vt_util_pub.h : vg_dq_vt_util.h
	cp $(>) $(<)

vg_dq_vt_util 1.0 :LIBRARY: \
vg_dq_vt_util_alarmgrid.c vg_dq_vt_util_alarmstyle.c vg_dq_vt_util_alarmtab.c \
vg_dq_vt_util_em.c vg_dq_vt_util_invgraph.c vg_dq_vt_util_invgrid.c \
vg_dq_vt_util_invmap.c vg_dq_vt_util_invmapll.c vg_dq_vt_util_invtab.c \
vg_dq_vt_util_ri.c vg_dq_vt_util_statchart.c vg_dq_vt_util_stattab.c \
vg_dq_vt_util_ut.c \
-lgv -lgraph -lgd -lvgxml

authkey = "da visualizer"
key.h :
	echo "#define SWIFTAUTHKEY \"$$(swiftkeygen $(authkey))\"" > $(<)

vg_genkey :: vg_genkey.c


/* Incoming File Processing Tools */

$(INSTALLROOT)/fun :INSTALLDIR: \
vg_cm_inc_init vg_cm_inc_fileinfo vg_cm_proc_init vg_cm_update_proc_run \
vg_inv_inc_init vg_inv_inc_fileinfo vg_inv_proc_init vg_inv_raw2dds_proc_run \
vg_alarm_inc_init vg_alarm_inc_fileinfo vg_alarm_collector vg_alarm_proc_init \
vg_alarm_raw2dds_proc_run vg_alarm_monthly_proc_run vg_alarm_yearly_proc_run \
vg_stat_inc_init vg_stat_inc_fileinfo vg_stat_proc_init \
vg_stat_raw2dds_proc_run vg_stat_monthly_proc_run vg_stat_yearly_proc_run \
vg_generic_inc_init vg_generic_inc_fileinfo vg_generic_proc_init \
vg_generic_raw2dds_proc_run vg_generic_monthly_proc_run \
vg_generic_yearly_proc_run


/* Data Processing Tools */

$(INSTALLROOT)/bin :INSTALLDIR: \
vg_version vg_inv vg_level vg_alarm vg_alarm_ce vg_trans \
vg_stat vg_stat_prof vg_stat_ce vg_generic vg_generic_bsearch vg_generic_lsearch

$(INSTALLROOT)/fun :INSTALLDIR: \
vg_generic_loadspec

$(INSTALLROOT)/lib/make :INSTALLDIR: \
vg_inv_process.mk


/* User Interface Tools */

vg_dq_vt_tool :: vg_dq_vt_tool.c -lvg_dq_vt_util

$(INSTALLROOT)/bin :INSTALLDIR: \
vg_accountinfo vg_accountupdate vg_cm_profilehelper vg_cm_thresholdfilter \
vg_cm_thresholdhelper vg_cmaccountedit vg_cmdefcheck vg_cmdefedit \
vg_cmdeflist vg_cmdefprint vg_cmdefvaluehelper vg_cmprintmd5 vg_convgeom \
vg_dq vg_dq_datahelper vg_dq_dt_alarm vg_dq_dt_alarmstat vg_dq_dt_inv \
vg_dq_dt_invedit vg_dq_dt_stat vg_dq_dt_generic vg_dq_invhelper vg_dq_main \
vg_dq_run vg_dq_vt_alarmgrid vg_dq_vt_alarmstatchart vg_dq_vt_alarmstattab \
vg_dq_vt_alarmstyle vg_dq_vt_alarmtab vg_dq_vt_autoinvtab vg_dq_vt_generic \
vg_dq_vt_invgraph vg_dq_vt_invgrid vg_dq_vt_invmap vg_dq_vt_invtab \
vg_dq_vt_statchart vg_dq_vt_stattab vg_export_email vg_export_save \
vg_fileeditor vg_fileprinter vg_filter_run vg_prefshelper vg_valuehelper \
vg_wusage_run vg_dq_stathelper vg_cmprintbyaccount

$(INSTALLROOT)/fun :INSTALLDIR: \
vg_dserverhelper_init vg_dserverhelper_run \
vg_assetschecker vg_assetssplitter vg_bgpmonchecker vg_bgpmonsplitter \
vg_cpltabchecker vg_cpltabsplitter vg_halertchecker vg_halertsplitter \
vg_hbauditchecker vg_hbauditsplitter vg_hostschecker vg_hostssplitter \
vg_ivthroughputbechecker vg_ivthroughputbesplitter vg_logpatrolchecker \
vg_logpatrolsplitter vg_ospfmonchecker vg_ospfmonsplitter vg_pingchecker \
vg_pingsplitter vg_portlistchecker vg_portlistsplitter \
vg_portspeedoverridechecker vg_portspeedoverridesplitter \
vg_sosschecker vg_sosssplitter vg_stranschecker vg_stranssplitter \
vg_syslogsitechecker vg_syslogsitesplitter vg_ttranschecker \
vg_ttranssplitter vg_urlprobechecker vg_urlprobesplitter vg_dq_vt_toolhelper

$(INSTALLROOT)/lib/prefs :INSTALLDIR: vg.prefs

$(INSTALLROOT)/lib/vg :INSTALLDIR: \
vg_cmfilelist vg_filelist vg_cm_alarmemail vg_cm_business vg_cm_customer \
vg_cm_favorites vg_cm_filter vg_cm_location vg_cm_preferences vg_cm_scope \
vg_cm_profile vg_cm_threshold vg_cm_type vg_cm_view

$(INSTALLROOT)/lib/fonts :INSTALLDIR: \
LiberationMono-Bold.ttf LiberationMono-BoldItalic.ttf \
LiberationMono-Italic.ttf LiberationMono-Regular.ttf \
LiberationSans-Bold.ttf LiberationSans-BoldItalic.ttf \
LiberationSans-Italic.ttf LiberationSans-Regular.ttf \
LiberationSerif-Bold.ttf LiberationSerif-BoldItalic.ttf \
LiberationSerif-Italic.ttf LiberationSerif-Regular.ttf

$(INSTALLROOT)/lib/fonts/arial.ttf :INSTALL: LiberationSans-Regular.ttf
$(INSTALLROOT)/lib/fonts/courier.ttf :INSTALL: LiberationMono-Regular.ttf
$(INSTALLROOT)/lib/fonts/helvetica.ttf :INSTALL: LiberationSans-Regular.ttf
$(INSTALLROOT)/lib/fonts/times.ttf :INSTALL: LiberationSerif-Regular.ttf

$(INSTALLROOT)/www/conf/swift :INSTALLDIR: vg.conf_sample

$(INSTALLROOT)/www/cgi-bin :INSTALLDIR: \
vg_swmaccess.cgi vg_atteaccess.cgi vg_help.cgi

$(INSTALLROOT)/www/htdocs/proj/vg :INSTALLDIR: \
control.html vg.js vg_dq_main.js vg_prefseditor.js \
vg_banner1.gif vg_banner2.gif vg_banner3.gif \
vg_bdbanner1.gif vg_bdbanner2.gif vg_bdbanner3.gif \
vg_ralxpbtn_o.gif vg_ralxpbtn_p.gif

$(INSTALLROOT)/www/htdocs/proj/vg/rest :INSTALLDIR: \
services.html

$(INSTALLROOT)/www/proj/vg/bin :INSTALLDIR: vg_env_sample

$(INSTALLROOT)/www/proj/vg/cgi-bin-members :INSTALLDIR: \
vg_accountadmin.cgi vg_accounthelper.cgi vg_accountuser.cgi \
vg_cm_favoriteshelper.cgi vg_confmgr.cgi vg_dq_actionhelper.cgi \
vg_dq_datahelper.cgi vg_dq_imghelper.cgi vg_dq_invedit.cgi \
vg_dq_invhelper.cgi vg_dq_stathelper.cgi vg_exporthelper.cgi \
vg_filemanager.cgi vg_home.cgi vg_prefseditor.cgi vg_valuehelper.cgi \
vg_rest_confquery.cgi vg_rest_confupdate.cgi \
vg_rest_dataquery.cgi vg_rest_dataupload.cgi

vg_cm_profilefilter :: vg_cm_profilefilter.c -ldds

vg_cm_thresholdfilter :: vg_cm_thresholdfilter.c -ldds

/* collection files */

$(INSTALLROOT)/bin :INSTALLDIR: \
vg_snmp_compile vg_scopemgr vg_scheduler vgsched vg_collector vg_scopeinv \
vg_units vgnewsession vgscp vgssh vg_ndcollect vg_ndprocess vg_openstackinfo \
vg_smartctlcron

$(INSTALLROOT)/fun :INSTALLDIR: \
vg_expr_calc vg_expr_eval vg_expr_print \
vg_recgen_alarm vg_recgen_alarmwstate vg_recgen_stat

vgxml :LIBRARY: xmlread.c xmlfind.c xmlwrite.c xmlflatten.c
vgxml :: xmlmain.c -lvgxml

vgurl :: vgurl.c

vgport :: vgport.c

vgnewsession :: vgnewsession.c

vgjson2ksh :: vgjson2ksh.c -ljson-c
vgxml2ksh :: vgxml2ksh.c -lxml2

SNMPDIR = $(CC.STDINCLUDE:C!$!/net-snmp!:P=X)

if CC.HOSTTYPE == "*"
	if "$(SNMPDIR)" != ""
		vgsnmpwalk : vgsnmpwalk.c
			$(CC) $(CCFLAGS:C/-I.*//) -I$(SNMPDIR) $(LDFLAGS) -o $(<) $(*) -lnetsnmp -lcrypto

		vgsnmpbulkwalk : vgsnmpbulkwalk.c
			$(CC) $(CCFLAGS:C/-I.*//) -I$(SNMPDIR) $(LDFLAGS) -o $(<) $(*) -lnetsnmp -lcrypto
		$(INSTALLROOT)/bin :INSTALLDIR: vgsnmpwalk vgsnmpbulkwalk
	end
end

vgwmi :: vgwmi.sh

vgsched :: \
schedmain.c schedload.c schedupdate.c \
schedexec.c schedreap.c schedlog.c schedtimediff.c schedts.c -lvgxml

$(INSTALLROOT)/scopes/scopeinv :INSTALLDIR: \
vg_scopeinv_mainarista vg_scopeinv_mainavocentpm vg_scopeinv_mainbrocade \
vg_scopeinv_maincisco vg_scopeinv_maindarwin vg_scopeinv_mainila \
vg_scopeinv_mainlinux vg_scopeinv_mainnetbotz vg_scopeinv_mainraritanpm \
vg_scopeinv_mainraritanpx vg_scopeinv_mainsmee vg_scopeinv_mainsolaris \
vg_scopeinv_mainsynapsense vg_scopeinv_mainveris vg_scopeinv_mainwin32 \
vg_scopeinv_mainhadoop vg_scopeinv_mainopenstack vg_scopeinv_mainsmartctl \
vg_scopeinv_maindellswitch vg_scopeinv_mainnrpe vg_scopeinv_maincollectd \
vg_scopeinv_mainpurestorage vg_scopeinv_maindellswitch2 \
vg_scopeinv_mainjetstor vg_scopeinv_mainidrac vg_scopeinv_mainlinux_rcloud \
vg_scopeinv_mainfreebsd

$(INSTALLROOT)/scopes/scopeinv/vg_scopeinv_mainciscocat :INSTALL: \
vg_scopeinv_maincisco
$(INSTALLROOT)/scopes/scopeinv/vg_scopeinv_maininfortrend1 :INSTALL: \
vg_scopeinv_maininfortrend
$(INSTALLROOT)/scopes/scopeinv/vg_scopeinv_maininfortrend2 :INSTALL: \
vg_scopeinv_maininfortrend

$(INSTALLROOT)/scopes/ping :INSTALLDIR: vg_ping_driver

$(INSTALLROOT)/scopes/telnet :INSTALLDIR: \
vg_telnet_driver vg_telnet_driver_fun

$(INSTALLROOT)/scopes/ssh :INSTALLDIR: \
vg_ssh_driver \
vg_ssh_fun_maindf vg_ssh_fun_mainfree vg_ssh_fun_mainilaalarms \
vg_ssh_fun_mainilastats vg_ssh_fun_mainmegacli vg_ssh_fun_mainmpstat \
vg_ssh_fun_mainnetstat vg_ssh_fun_mainproc vg_ssh_fun_mainprstat \
vg_ssh_fun_mainps vg_ssh_fun_mainsar vg_ssh_fun_mainsmee vg_ssh_fun_mainswap \
vg_ssh_fun_maintail vg_ssh_fun_maintop vg_ssh_fun_mainuptime \
vg_ssh_fun_mainvmstat vg_ssh_fun_mainsmartctl vg_ssh_fun_maincollectd \
vg_ssh_fun_mainnvidia vg_ssh_fun_mainrcloud vg_ssh_fun_mainntp

$(INSTALLROOT)/scopes/wmi :INSTALLDIR: vg_wmi_driver

$(INSTALLROOT)/scopes/url :INSTALLDIR: vg_url_driver

$(INSTALLROOT)/scopes/port :INSTALLDIR: vg_port_driver

$(INSTALLROOT)/scopes/exec :INSTALLDIR: \
vg_exec_driver \
vg_exec_cmd_mainsynapsense vg_exec_cmd_mainveris vg_exec_cmd_mainhadoop \
vg_exec_cmd_mainopenstack vg_exec_cmd_mainweatherstation vg_exec_cmd_mainnrpe \
vg_exec_cmd_mainpurestorage vg_exec_cmd_mainjetstor vg_exec_cmd_mainidrac \
vg_exec_cmd_mainstorview

$(INSTALLROOT)/scopes/snmp :INSTALLDIR: \
vg_snmp_driver \
vg_snmp_cmd_mainaristaiface \
vg_snmp_cmd_mainavocentpm vg_snmp_cmd_mainbrocadefciface \
vg_snmp_cmd_mainciscocatiface vg_snmp_cmd_mainciscohw \
vg_snmp_cmd_mainciscoiface vg_snmp_cmd_mainciscotempsensor \
vg_snmp_cmd_maininfortrend1hdd vg_snmp_cmd_maininfortrend1lun \
vg_snmp_cmd_maininfortrend2hdd vg_snmp_cmd_maininfortrend2lun \
vg_snmp_cmd_mainnetbotzairsensor vg_snmp_cmd_mainnetbotzaudiosensor \
vg_snmp_cmd_mainnetbotzdewsensor vg_snmp_cmd_mainnetbotzdoorsensor \
vg_snmp_cmd_mainnetbotzhumisensor vg_snmp_cmd_mainnetbotztempsensor \
vg_snmp_cmd_mainraritanpm vg_snmp_cmd_mainraritanpmsensor \
vg_snmp_cmd_mainraritanpx vg_snmp_cmd_mainraritanpxsensor \
vg_snmp_cmd_maindellswitchcpu vg_snmp_cmd_maindellswitchmem \
vg_snmp_cmd_maindellswitchcpu2 vg_snmp_cmd_maindellswitchmem2

$(INSTALLROOT)/scopes/netdisc :INSTALLDIR: \
vg_nd_snmp_arista vg_nd_snmp_cisco vg_nd_snmp_ciscocat \
vg_nd_snmp_dell vg_nd_snmp_dell2 vg_nd_snmp_foundry vg_nd_snmp_generic \
vg_nd_snmp_cmd_arista_arp vg_nd_snmp_cmd_arista_dot \
vg_nd_snmp_cmd_arista_iface \
vg_nd_snmp_cmd_cisco_arp vg_nd_snmp_cmd_cisco_cdp vg_nd_snmp_cmd_cisco_dot \
vg_nd_snmp_cmd_cisco_iface vg_nd_snmp_cmd_cisco_trunk vg_nd_snmp_cmd_cisco_vtp \
vg_nd_snmp_cmd_ciscocat_arp vg_nd_snmp_cmd_ciscocat_cdp \
vg_nd_snmp_cmd_ciscocat_dot vg_nd_snmp_cmd_ciscocat_iface \
vg_nd_snmp_cmd_ciscocat_trunk vg_nd_snmp_cmd_ciscocat_vtp \
vg_nd_snmp_cmd_dell_arp vg_nd_snmp_cmd_dell_dot vg_nd_snmp_cmd_dell_iface \
vg_nd_snmp_cmd_foundry_arp vg_nd_snmp_cmd_foundry_dot \
vg_nd_snmp_cmd_foundry_iface \
vg_nd_snmp_cmd_generic_arp vg_nd_snmp_cmd_generic_dot \
vg_nd_snmp_cmd_generic_iface

%.sh : %.snmpspec vg_snmp_compile
	$(SHELL) $(*:O=2) $(*:O=1) > $(<)


/* DDS files */

$(INSTALLROOT)/lib/dds :INSTALLDIR: vgschema.info

%.schema : %.schema.tmpl vgschema.info vgmkschema
	$(SHELL) $(*:O=3) $(*:O=2) < $(*:O=1) > $(<)

vg_level_node :load: vg_level_node.schema
vg_level_map :load: vg_level_map.schema
vg_level_map :conv: \
vg_level_map.schema vg_level_map.schema vg_level_map.conv

vg_inv_node :load: vg_inv_node.schema
vg_inv_uniquenode :conv: \
vg_inv_node.schema vg_inv_node.schema vg_inv_uniquenode.conv
vg_inv_map :load: vg_inv_map.schema
vg_inv_edge :load: vg_inv_edge.schema
vg_inv_map :conv: vg_inv_node.schema vg_inv_map.schema vg_inv_map.conv
vg_inv_map_rev :conv: \
vg_inv_map.schema vg_inv_map.schema vg_inv_map_rev.conv
vg_inv_nd2cc :conv: vg_inv_edge.schema vg_inv_cc.schema vg_inv_nd2cc.conv
vg_inv_profile :filter: \
vg_inv_node.schema vg_inv_profile.filter -lvgprofile
vg_profile :load: vg_profile.schema
vg_inv_threshold :filter: \
vg_inv_node.schema vg_inv_threshold.filter -lvgthreshold

vg_alarm :load: vg_alarm.schema
vg_trans :load: vg_trans.schema
vg_alarm_bydate :split: vg_alarm.schema dateissued
vg_alarm_byhour :split: vg_alarm.schema hourissued
vg_alarm :filter: vg_alarm.schema vg_alarm.filter -lvgdb -lvgfilter
vg_alarm_prop :filter: vg_alarm.schema vg_alarm_prop.filter
vg_alarm_open :filter: vg_alarm.schema vg_alarm_open.filter
vg_alarm_email :filter: vg_alarm.schema vg_alarm_email.filter -lvgemail
vg_alarm_aggr :filter: vg_alarm.schema vg_alarm_aggr.filter

vg_stat :load: vg_stat.schema
vg_stat_bydate :split: vg_stat.schema dateissued
vg_stat_byframe :split: vg_stat.schema frame
vg_stat_prof :split: vg_stat.schema label
vg_stat :filter: vg_stat.schema vg_stat.filter
vg_stat_prop :filter: vg_stat.schema vg_stat_prop.filter
vg_stat_prof :conv: vg_stat.schema vg_stat_prof.conv
vg_stat_aggr :conv: vg_stat.schema vg_stat_aggr.conv

vg_dq_dt_inv_node :conv: \
vg_inv_node.schema vg_dq_dt_inv.schema vg_dq_dt_inv_node.conv
vg_dq_dt_inv_edge :conv: \
vg_inv_edge.schema vg_dq_dt_inv.schema vg_dq_dt_inv_edge.conv

vg_dq_dt_alarm :conv: \
vg_alarm.schema vg_dq_dt_alarm.schema vg_dq_dt_alarm.conv
vg_dq_dt_alarm_bs :conv: \
vg_alarm.schema vg_dq_dt_alarm.schema vg_dq_dt_alarm_bs.conv

vg_dq_dt_stat :conv: \
vg_stat.schema vg_dq_dt_stat.schema vg_dq_dt_stat.conv
vg_dq_dt_stat_one :conv: \
vg_dq_dt_stat.schema vg_dq_dt_stat.schema vg_dq_dt_stat_one.conv
vg_dq_dt_stat_gnp :conv: \
vg_dq_dt_stat.schema vg_dq_dt_stat.schema vg_dq_dt_stat_gnp.conv
vg_dq_dt_inv2statfilter :filter: \
vg_dq_dt_inv.schema vg_dq_dt_inv2statfilter.filter
vg_dq_dt_inv2invfilter :filter: \
vg_dq_dt_inv.schema vg_dq_dt_inv2invfilter.filter
vg_dq_dt_alarm2statfilter :filter: \
vg_dq_dt_alarm.schema vg_dq_dt_alarm2statfilter.filter

vg_dq_dt_alarmstat :conv: \
vg_dq_dt_alarm.schema vg_dq_dt_stat.schema vg_dq_dt_alarmstat.conv
vg_dq_dt_alarmstat :count: \
vg_dq_dt_stat.schema ilevel,iid,key,frame val

vg_dq_vt_alarmgrid :filter: \
vg_dq_dt_alarm.schema vg_dq_vt_alarmgrid.filter -lvg_dq_vt_util
vg_dq_vt_alarmtab :filter: \
vg_dq_dt_alarm.schema vg_dq_vt_alarmtab.filter -lvg_dq_vt_util
vg_dq_vt_alarmstyle :filter: \
vg_dq_dt_alarm.schema vg_dq_vt_alarmstyle.filter -lvg_dq_vt_util
vg_dq_vt_invgraph :filter: \
vg_dq_dt_inv.schema vg_dq_vt_invgraph.filter -lvg_dq_vt_util
vg_dq_vt_invgrid :filter: \
vg_dq_dt_inv.schema vg_dq_vt_invgrid.filter -lvg_dq_vt_util
vg_dq_vt_invmap :filter: \
vg_dq_dt_inv.schema vg_dq_vt_invmap.filter -lvg_dq_vt_util
vg_dq_vt_invmapll :filter: \
vg_dq_dt_inv.schema vg_dq_vt_invmapll.filter -lvg_dq_vt_util
vg_dq_vt_invtab :filter: \
vg_dq_dt_inv.schema vg_dq_vt_invtab.filter -lvg_dq_vt_util
vg_dq_vt_statchart :filter: \
vg_dq_dt_stat.schema vg_dq_vt_statchart.filter -lvg_dq_vt_util
vg_dq_vt_stattab :filter: \
vg_dq_dt_stat.schema vg_dq_vt_stattab.filter -lvg_dq_vt_util
vg_dq_invhelper :filter: \
vg_inv_node.schema vg_dq_invhelper.filter
vg_dq_invhelper_edge :filter: \
vg_inv_edge.schema vg_dq_invhelper_edge.filter
vg_dq_stathelper :filter: \
vg_stat.schema vg_dq_stathelper.filter
vg_dq_vt_autoinvtab :filter: \
vg_dq_dt_inv.schema vg_dq_vt_autoinvtab.filter \
-lvgfilter -lvgemail -lvgthreshold

vg_rest_data_level :printer: \
vg_level_node.schema vg_rest_data_level.printer
vg_rest_data_inv :printer: \
vg_dq_dt_inv.schema vg_rest_data_inv.printer
vg_rest_data_alarm :printer: \
vg_dq_dt_alarm.schema vg_rest_data_alarm.printer
vg_rest_data_stat :printer: \
vg_dq_dt_stat.schema vg_rest_data_stat.printer

vg_inv_map_check :filter: vg_inv_node.schema vg_inv_map_check.filter


vg_sevmap.c : vg_sevmap.tmpl.c
	cp $(>) $(<)

vg_statmap.c : vg_statmap.tmpl.c
	cp $(>) $(<)

vg_timeranges.c : vg_timeranges.tmpl.c
	cp $(>) $(<)

vg_inv_map_tc.c : vg_inv_map_tc.tmpl.c
	cp $(>) $(<)

vg_dq_dt_attrs.c : vg_dq_dt_attrs.tmpl.c
	cp $(>) $(<)

vg_dq_vt_attrs.c : vg_dq_vt_attrs.tmpl.c
	cp $(>) $(<)

sl_trans.c : vg_trans.schema
	$(SHELL) ddsmkslist -name trans \
		-schema $(*:O=1) -key inid \
	> $(<)

sl_transrev.c : vg_trans.schema
	$(SHELL) ddsmkslist -name transrev \
		-schema $(*:O=1) -mode all -key outid \
	> $(<)

sl_inv_node.c : vg_inv_node.schema
	$(SHELL) ddsmkslist -name inv_node \
		-schema $(*:O=1) -mode all -key 'level id' \
	> $(<)

sl_inv_nodeattr.c : vg_inv_node.schema
	$(SHELL) ddsmkslist -name inv_nodeattr \
		-schema $(*:O=1) -key 'level id key' \
	> $(<)

sl_inv_nodenameattr.c : vg_inv_node.schema
	$(SHELL) ddsmkslist -name inv_nodenameattr \
		-schema $(*:O=1) -key 'level id key' \
	> $(<)

sl_inv_edge.c : vg_inv_edge.schema
	$(SHELL) ddsmkslist -name inv_edge \
		-schema $(*:O=1) -mode all -key 'level1 id1' \
	> $(<)

sl_inv_edge2.c : vg_inv_edge.schema
	$(SHELL) ddsmkslist -name inv_edge2 \
		-schema $(*:O=1) -mode all -key 'level1 id1 level2 id2' \
	> $(<)

sl_inv_edgeattr.c : vg_inv_edge.schema
	$(SHELL) ddsmkslist -name inv_edgeattr \
		-schema $(*:O=1) -key 'level1 id1 level2 id2 key' \
	> $(<)

sl_level_node.c : vg_level_node.schema
	$(SHELL) ddsmkslist -name level_node \
		-schema $(*:O=1) -mode all -key id \
	> $(<)

sl_level_map.c : vg_level_map.schema
	$(SHELL) ddsmkslist -name level_map \
		-schema $(*:O=1) -mode all -key 'id1 id2' \
	> $(<)

sl_inv_map.c : vg_inv_map.schema
	$(SHELL) ddsmkslist -name inv_map \
		-schema $(*:O=1) -mode all -key 'level1 id1' \
	> $(<)

sl_inv_map1.c : vg_inv_map.schema vgmkmapfind
	$(SHELL) ddsmkslist -name inv_map1 -run vgmkmapfind \
		-schema $(*:O=1) -mode all -key 'level1 id1 level2' \
	> $(<)

sl_inv_map2.c : vg_inv_map.schema vgmkmapfind
	$(SHELL) ddsmkslist -name inv_map2 -run vgmkmapfind \
		-schema $(*:O=1) -mode all -key 'level1 id1 level2' \
	> $(<)

sl_inv_map3.c : vg_inv_map.schema vgmkmapfind
	$(SHELL) ddsmkslist -name inv_map3 -run vgmkmapfind \
		-schema $(*:O=1) -mode all -key 'level1 id1 level2' \
	> $(<)

sl_inv_cc2nd.c : vg_inv_cc.schema
	$(SHELL) ddsmkslist -name inv_cc2nd \
		-schema $(*:O=1) -mode all -key 'cclevel ccid' \
	> $(<)

sl_inv_nd2cc.c : vg_inv_cc.schema
	$(SHELL) ddsmkslist -name inv_nd2cc \
		-schema $(*:O=1) -key 'ndlevel ndid' \
	> $(<)

sl_profile.c : vg_profile.schema
	$(SHELL) ddsmkslist -name profile -warnonopen \
		-schema $(*:O=1) -key 'level id key' \
	> $(<)

sl_stat.c : vg_stat.schema
	$(SHELL) ddsmkslist -name stat -mode all -warnonopen \
		-schema $(*:O=1) -key 'level id key unit' \
	> $(<)

sl_mean.c : vg_stat.schema
	$(SHELL) ddsmkslist -name mean -mode all -warnonopen \
		-schema $(*:O=1) -key 'level id key' \
	> $(<)

sl_sdev.c : vg_stat.schema
	$(SHELL) ddsmkslist -name sdev -mode all -warnonopen \
		-schema $(*:O=1) -key 'level id key' \
	> $(<)

sl_alarm.c : vg_alarm.schema
	$(SHELL) ddsmkslist -name alarm -mode all -warnonopen \
		-schema $(*:O=1) -key 'level1 id1 level2 id2' \
	> $(<)

sl_dq_dt_inv.c : vg_dq_dt_inv.schema
	$(SHELL) ddsmkslist -name dq_dt_inv \
		-schema $(*:O=1) -key 'cat type level1 id1' \
	> $(<)

:: ptest.c wmi.h wmiarg.h wmiinv.cpp wmilog.cpp wmimain.cpp wmimisc.cpp \
wmiopn.cpp wmiv2s.cpp wmiv2s.h
