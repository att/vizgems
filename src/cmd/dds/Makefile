:PACKAGE: swift ast

LICENSE = since=1996,author=ek

if CC.HOSTTYPE == "darwin.i386*" || CC.HOSTTYPE == "freebsd12.amd64*"
	FEATURE_NO64FS == 1
end

ddsinfo :: ddsinfo.c -ldds -lvcodex ddsinfo.1
ddsload :: ddsload.c -ldds -lvcodex ddsload.1
ddsfilter :: ddsfilter.c -ldds -lvcodex ddsfilter.1
ddschoose :: ddschoose.c -ldds -lvcodex ddschoose.1
ddssplit :: ddssplit.c -ldds -lvcodex ddssplit.1
ddsgroup :: ddsgroup.c -ldds -lvcodex ddsgroup.1
ddscount :: ddscount.c -ldds -lvcodex ddscount.1
ddssort :: ddssort.c -ldds -lrecsort -lvcodex ddssort.1
ddsconv :: ddsconv.c -ldds -lvcodex ddsconv.1
ddsextract :: ddsextract.c -ldds -lvcodex ddsextract.1
ddscat :: ddscat.c -ldds -lvcodex ddscat.1
ddsaggr :: ddsaggr.c -laggr -ldds -lvcodex -lm ddsaggr.1
ddssplitnaggr :: ddssplitnaggr.c -laggr -ldds -lvcodex -lm ddssplitnaggr.1
ddsprint :: ddsprint.c -ldds -lvcodex ddsprint.1
ddscc :: ddscc.c -laggr -ldds -lrecsort -lvcodex ddscc.1
ddsmkdict :: ddsmkdict.sh ddsmkdict.1
ddsmkarray :: ddsmkarray.sh ddsmkarray.1
ddsmkslist :: ddsmkslist.sh ddsmkslist.1

ddsinfo.1 :manual: ddsinfo
ddsload.1 :manual: ddsload
ddsfilter.1 :manual: ddsfilter
ddschoose.1 :manual: ddschoose
ddssplit.1 :manual: ddssplit
ddsgroup.1 :manual: ddsgroup
ddscount.1 :manual: ddscount
ddssort.1 :manual: ddssort
ddsconv.1 :manual: ddsconv
ddsextract.1 :manual: ddsextract
ddscat.1 :manual: ddscat
ddsaggr.1 :manual: ddsaggr
ddssplitnaggr.1 :manual: ddssplitnaggr
ddsprint.1 :manual: ddsprint
ddscc.1 :manual: ddscc
ddsmkdict.1 :manual: $(SHELL) ddsmkdict
ddsmkarray.1 :manual: $(SHELL) ddsmkarray
ddsmkslist.1 :manual: $(SHELL) ddsmkslist

$(INSTALLROOT)/bin :INSTALLDIR: ddspp

":gen:" : .MAKE .OPERATOR
	$(<) :: $(>:N=*.sh)
		$(MAKE) -f $(*:N=*.mk) nativepp=-1 $(<)
	$(<) : gen$(>:B:S=.mk)

ddsmkso :gen: ddsmkso.sh
