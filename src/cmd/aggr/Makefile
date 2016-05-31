:PACKAGE: swift ast

LICENSE = since=1996,author=ek

authkey = "maggie, spin to me maggie!!!"
key.h :
	print "#define SWIFTAUTHKEY \"$$(swiftkeygen $(authkey))\"" > $(<)

aggrload :: aggrload.c -laggr aggrload.1
aggrcombine :: aggrcombine.c -laggr aggrcombine.1
aggrcat :: aggrcat.c -laggr aggrcat.1
aggrsum :: aggrsum.c -laggr aggrsum.1
aggrreaggr :: aggrreaggr.c -laggr aggrreaggr.1
aggrextract :: aggrextract.c -laggr aggrextract.1
aggrmean :: aggrmean.c -laggr aggrmean.1
aggrtopn :: aggrtopn.c -laggr aggrtopn.1
aggrprint :: aggrprint.c -laggr aggrprint.1
aggrkeys :: aggrkeys.c -laggr aggrkeys.1
aggrmap :: aggrmap.c -laggr aggrmap.1
aggrserver :: aggrserver.c -laggr aggrserver.1

aggrload.1 :manual: aggrload
aggrload.1 :manual: aggrload
aggrcombine.1 :manual: aggrcombine
aggrcat.1 :manual: aggrcat
aggrsum.1 :manual: aggrsum
aggrreaggr.1 :manual: aggrreaggr
aggrextract.1 :manual: aggrextract
aggrmean.1 :manual: aggrmean
aggrtopn.1 :manual: aggrtopn
aggrprint.1 :manual: aggrprint
aggrkeys.1 :manual: aggrkeys
aggrmap.1 :manual: aggrmap
aggrserver.1 :manual: aggrserver

:INSTALLDIR: fix1 fix2 fix3 t
fix1 :: fix1.c -laggr
fix2 :: fix2.c -laggr
fix3 :: fix3.sh
t :: t.c -laggr
