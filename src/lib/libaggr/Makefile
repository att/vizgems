:PACKAGE: swift ast

LICENSE = since=1996,author=ek

CCFLAGS += $$(CC.DLL)

$(INCLUDEDIR) :INSTALLDIR: aggr.h aggr_int.h

authkey = "maggie, spin to me maggie!!!"
key.h :
	echo "#define SWIFTAUTHKEY \"$$(swiftkeygen $(authkey))\"" > $(<)

lib%.inst.c : gen%hdr.sh
	$(SHELL) $(>) > $(<)

aggr $(SWIFTVERSION) :LIBRARY: libmain.c libcombine.c gencombinehdr.sh \
	libcat.c gencathdr.sh libsum.c gensumhdr.sh libreaggr.c \
	genreaggrhdr.sh libextract.c genextracthdr.sh libmean.c genmeanhdr.sh \
	libtopn.c gentopnhdr.sh aggr.1 aggr.3 -lm -lsocket -lnsl
