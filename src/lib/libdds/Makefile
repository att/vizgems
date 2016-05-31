:PACKAGE: swift ast

LICENSE = since=1996,author=ek

CCFLAGS += $$(CC.DLL)

$(INCLUDEDIR) :INSTALLDIR: dds.h dds_int.h

dds $(SWIFTVERSION) :LIBRARY: libmain.c libload.c libfilter.c libsplit.c \
	libgroup.c libconv.c libcount.c libsort.c libextract.c libaggr.c \
	libprint.c libvczip.c dds.1 dds.3 -laggr -lvcodex -lrecsort -ldl
