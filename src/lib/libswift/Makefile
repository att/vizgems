:PACKAGE: swift ast

LICENSE = since=1996,author=ek

CCFLAGS += $$(CC.DLL)

$(INCLUDEDIR) :INSTALLDIR: swift.h

swift $(SWIFTVERSION) :LIBRARY: libsfdisc.c libauth.c libenc.c libmsg.c

swiftkeygen :: swiftkeygen.c -lswift
swiftencode :: swiftencode.c -lswift

swiftsh $(SWIFTVERSION) :LIBRARY: swiftshenc.c -lshell -lswift
