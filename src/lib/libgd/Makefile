:PACKAGE: --prefix-include graphviz ast X11
.SOURCE.h: /usr/include/freetype2
.SOURCE.a: /usr/X11/lib64

if CC.HOSTTYPE == "darwin.i386*"
	.SOURCE.h: /usr/X11/include/freetype2
end

CCFLAGS += $$(CC.DLL)

gd 1.0 :LIBRARY: gd.c gd_gd.c gd_gd2.c gd_gif_in.c gd_gif_out.c gd_io.c \
	gd_io_dp.c gd_io_file.c gd_io_ss.c gd_jpeg.c gd_png.c gd_security.c \
	gd_ss.c gd_topal.c gd_wbmp.c gdcache.c gdfontg.c gdfontl.c gdfontmb.c \
	gdfonts.c gdfontt.c gdfont4.c gdfont5.c gdfont6.c gdfont7.c gdfont8.c \
	gdfont9.c gdfont10.c gdfont11.c gdfont12.c gdfont13.c gdfont14.c \
	gdfont15.c gdfont16.c gdfont17.c gdfont18.c gdfont19.c gdfont20.c \
	gdft.c gdfx.c gdhelpers.c gdkanji.c gdtables.c gdxpm.c wbmp.c gd.ign \
	-lX11 -lXpm -ljpeg -lpng -lttf -lz -lm

$(INCLUDEDIR) :INSTALLDIR: \
gd.h gd_io.h gdfx.h gdfontg.h gdfontl.h gdfontmb.h gdfonts.h gdfontt.h \
gdfont4.h gdfont5.h gdfont6.h gdfont7.h gdfont8.h gdfont9.h gdfont10.h \
gdfont11.h gdfont12.h gdfont13.h gdfont14.h gdfont15.h gdfont16.h gdfont17.h \
gdfont18.h gdfont19.h gdfont20.h

:: COPYING
