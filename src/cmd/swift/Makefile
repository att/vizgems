:PACKAGE: swift ast X11 lefty:command lefty0d:command dotty:command

LICENSE = since=1996,author=ek

.SOURCE: ./dp ./tr ./ms ./pl ./wm ./ui ./gm ./ut

:ALL:

noxtst = 0
if CC.HOSTTYPE == "sol*.*"
	NETLIBS = -lsocket -lnsl
end
if CC.HOSTTYPE != "darwin.*"
	CRYPTLIB = -lcrypt
end

/* Data Processing */

:ALL: sdpincoming sdpprocessing sdpcomplete sdpcleanup sdpcontrol

$(INSTALLROOT)/fun :INSTALLDIR: \
sdpensuredirs sdpensuredualdirs sdpruncheck sdpspacecheck \
sdpmpjobcntl sdpjobcntl sdpmv


/* Data Transfer */

:ALL: strincoming stroutgoing strreport strcleanup strcontrol

$(INSTALLROOT)/fun :INSTALLDIR: \
strensuredirs strruncheck strspacecheck strjobcntl


/* Pipeline Builder */

$(INSTALLROOT)/bin :INSTALLDIR: splui splgen splprint
$(INSTALLROOT)/lib/lefty :INSTALLDIR: \
spl.lefty spl_check.lefty spl_gen.lefty spl_objects.lefty spl_proto.lefty \
spl_query.lefty spl_upgrade.lefty spl_view.lefty


/* WWW Member Tools */

$(INSTALLROOT)/bin :INSTALLDIR: swmhostname swmiplookup swmaccounts

$(INSTALLROOT)/fun :INSTALLDIR: swmuseringroups

$(INSTALLROOT)/www/bin :INSTALLDIR: \
swmenv_sample swmgetinfo swmlistfiles swmfilestats swmmimesplit

$(INSTALLROOT)/www/etc :INSTALLDIR: swmhb swmcleanup gateKeeper.jar PSE_Lite.jar

$(INSTALLROOT)/www/cgi-bin-members :INSTALLDIR: \
swmstart.cgi swmgreet.cgi swmcontrol.cgi swmuploadfile.cgi swmlistfiles.cgi \
swmdeletefile1.cgi swmdeletefile2.cgi swmsavefile.cgi \
swmcpasswd.cgi swmchangepasswd.cgi

$(INSTALLROOT)/www/htdocs/members :INSTALLDIR: swmuploadfile.html

swmmimesplit :: swmmimesplit.c

swmchangepasswd.cgi :: swmchangepasswd.c swmutil.c $(CRYPTLIB) +lswift +last

swmcheckpasswd :: swmcheckpasswd.c -lcrypt

/* User Interface */

:ALL: suiservices suiservicehb suiprefhb suidserverhelper

$(INSTALLROOT)/bin :INSTALLDIR: \
suiplot suigmap suigraph suitable suibrowser suiprefgen suiruncmd \
suigraphhelper suicoord2item suimhistory plotgen gmapgen

$(INSTALLROOT)/fun :INSTALLDIR: suireadcmd suireadkv suireadcgikv

$(INSTALLROOT)/lib/lefty :INSTALLDIR: \
suiutil.lefty suigmap.lefty suiplot.lefty suigraph.lefty suigraph_ds.lefty \
suixclient.lefty

$(INSTALLROOT)/lib/prefs :INSTALLDIR: \
suibrowser.prefs suigmap.prefs suigraph.prefs suiplot.prefs suipad.prefs \
suitable.prefs suidserver.prefs suistyle.prefs

$(INSTALLROOT)/www/cgi-bin-members :INSTALLDIR: \
suiprefshow.cgi suiprefproc.cgi suipreflist.cgi suitablehelper.cgi

:ALL: suipad.cgi suisetup.cgi suistart.cgi suidateselect.cgi \
suiviewselect.cgi suilss.cgi suilaunch.cgi \
suimserverhelper.cgi suidserverhelper.cgi

$(INSTALLROOT)/www/htdocs/swift :INSTALLDIR: suipad.js suilss.js

authkey = "maggie, spin to me maggie!!!"
key.h :
	echo "#define SWIFTAUTHKEY \"$$(swiftkeygen $(authkey))\"" > $(<)

suiencode :: suiencode.c

suirserver :: suirserver.c $(NETLIBS) suirserver.1
suirclient :: suirclient.c $(NETLIBS) suirclient.1

suimserver :: suimserver.c $(NETLIBS) suimserver.1
suimclient :: suimclient.c $(NETLIBS) suimclient.1

suidserver :: suidserver.c -lcmd $(NETLIBS) suidserver.1
suidclient :: suidclient.c -lcmd $(NETLIBS) suidclient.1

if noxtst == 0
suixserver :: suixserver.c -lXtst -lX11 -lXext $(NETLIBS) suixserver.1
suixclient :: suixclient.c $(NETLIBS) suixclient.1
suixwacom :: suixwacom.c $(NETLIBS) suixwacom.1
suixserver.1 :manual: suixserver
suixclient.1 :manual: suixclient
suixwacom.1 :manual: suixwacom
end

suicoord2item :: suicoord2item.c -lgeom suicoord2item.1

suinewpgrp :: suinewpgrp.c suinewpgrp.1

suirserver.1 :manual: suirserver
suirclient.1 :manual: suirclient

suimserver.1 :manual: suimserver
suimclient.1 :manual: suimclient

suidserver.1 :manual: suidserver
suidclient.1 :manual: suidclient

suicoord2item.1 :manual: suicoord2item

suinewpgrp.1 :manual: suinewpgrp

/* Image Server */

$(INSTALLROOT)/bin :INSTALLDIR: smsserver smsclient sms3dimg

$(INSTALLROOT)/fun :INSTALLDIR: smsruncheck smsspacecheck

/* Geometry Files */

GEOMFILES=border_polys.single world_polys.single

$(LIBDIR)/geoms :INSTALLDIR: $(GEOMFILES)

/* Utilities */

$(INSTALLROOT)/bin :INSTALLDIR: suthdrgen
$(INSTALLROOT)/fun :INSTALLDIR: sutawk sutgraph

sutendian :: sutendian.c

:: RELEASE
