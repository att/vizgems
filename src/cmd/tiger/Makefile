:PACKAGE: swift ast

LICENSE = since=1996,author=ek

if CC.HOSTTYPE != "win32.*"
CCFLAGS += $$(CC.DLL)
end

$(INCLUDEDIR) :INSTALLDIR: tiger.h

tiger $(SWIFTVERSION) :LIBRARY: \
tigerutils.c tigerfiles.c tigerrecords.c -lm tiger.1

tigera2b :: tigera2b.c -ltiger tigera2b.1
tigercat :: tigercat.c -ltiger tigercat.1
tigermerge :: tigermerge.c -ltiger tigermerge.1
tigerextract :: tigerextract.c -ltiger tigerextract.1
tigerattrmap :: tigerattrmap.c -ltiger tigerattrmap.1
tigerinfo :: tigerinfo.c -ltiger tigerinfo.1
tigerprint :: tigerprint.c -ltiger tigerprint.1
npanxx2tiger :: npanxx2tiger.c -ltiger npanxx2tiger.1
zip2tiger :: zip2tiger.c -ltiger zip2tiger.1

tigera2b.1 :manual: tigera2b
tigercat.1 :manual: tigercat
tigermerge.1 :manual: tigermerge
tigerextract.1 :manual: tigerextract
tigerattrmap.1 :manual: tigerattrmap
tigerinfo.1 :manual: tigerinfo
tigerprint.1 :manual: tigerprint
npanxx2tiger.1 :manual: npanxx2tiger
zip2tiger.1 :manual: zip2tiger
