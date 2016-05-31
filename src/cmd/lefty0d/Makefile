.SOURCE: ../lefty ../lefty/examples

LICENSE = since=1996,author=ek

BINNAME = lefty0d
LIBNAME = gfx0d

if CC.HOSTTYPE == "sgi*.*" || "$(-mam)" == "static*"
	CCFLAGS =
end

FEATURE_DOT == 1
FEATURE_NONE == 1
FEATURE_CS == 1
FEATURE_MINTSIZE == 1

":OPTIONAL:" : .MAKE .OPERATOR
	$(<) := $(>)
	:: $(>)

src.bin.cs :OPTIONAL: cs2l/cs2l.h cs2l/cs2l.c

src.bin.dot :OPTIONAL: \
	dot2l/dot2l.c dot2l/dot2l.h dot2l/dotlex.c dot2l/dotparse.y \
	dot2l/dottrie.c

src.bin.ms :OPTIONAL: os/mswin32/io.c

src.bin.unix :OPTIONAL: os/unix/io.c

src.lib.win :OPTIONAL: \
	ws/mswin32/garray.c ws/mswin32/gbutton.c ws/mswin32/gcanvas.c \
	ws/mswin32/gcommon.c ws/mswin32/gcommon.h ws/mswin32/glabel.c \
	ws/mswin32/gmenu.c ws/mswin32/gpcanvas.c ws/mswin32/gquery.c \
	ws/mswin32/gscroll.c ws/mswin32/gtext.c ws/mswin32/gview.c \
	ws/mswin32/lefty.rc ws/mswin32/resource.h

src.lib.x11 :OPTIONAL: \
	ws/x11/garray.c ws/x11/gbutton.c ws/x11/gcanvas.c ws/x11/gcommon.c \
	ws/x11/gcommon.h ws/x11/glabel.c ws/x11/gmenu.c ws/x11/gpcanvas.c \
	ws/x11/gquery.c ws/x11/gscroll.c ws/x11/gtext.c ws/x11/gview.c \
	ws/x11/libfilereq/Dir.c ws/x11/libfilereq/Draw.c \
	ws/x11/libfilereq/Path.c ws/x11/libfilereq/README.selfile \
	ws/x11/libfilereq/SFinternal.h ws/x11/libfilereq/SelFile.c \
	ws/x11/libfilereq/xstat.h

src.lib.none :OPTIONAL: \
	ws/none/garray.c ws/none/gbutton.c ws/none/gcanvas.c ws/none/gcommon.c \
	ws/none/gcommon.h ws/none/glabel.c ws/none/gmenu.c ws/none/gpcanvas.c \
	ws/none/gquery.c ws/none/gscroll.c ws/none/gtext.c ws/none/gview.c

if FEATURE_DOT == 1
	.SOURCE: ../lefty/dot2l
	BINSRC += $(src.bin.dot)
end

if FEATURE_X11 == 1
	:PACKAGE: - X11
	.SOURCE: ../lefty/ws/x11 ../lefty/ws/x11/libfilereq
	LIBSRC += $(src.lib.x11)
	FEATURE_XAW3D == $(PACKAGES Xaw3d:N=1:?1??)
	if FEATURE_XAW3D == 1
		LIBLIBS += -lXaw3d
	else
		LIBLIBS += -lXaw
	end
	LIBLIBS += -lXmu -lXt -lXpm -lXext -lX11
elif FEATURE_NONE == 1
	.SOURCE: ../lefty/ws/none
	LIBSRC += $(src.lib.none)
else
	.SOURCE: ../lefty/ws/mswin32
	RCFLAGS += '-IC:\Program Files\DevStudio\vc'
	RCFLAGS += '-IC:\Program Files\DevStudio\vc\include'
	LIBSRC += $(src.lib.win)
	BINSRC += lefty.res
	LIBLIBS += -luser32 -lgdi32 -lwinspool -lcomdlg32 -ladvapi32 -lshell32
end

if FEATURE_MS == 1
	.SOURCE: ../lefty/os/mswin32
	BINSRC += $(src.bin.ms)
else
	.SOURCE: ../lefty/os/unix
	FEATURE_RUSAGE == 1
	BINSRC += $(src.bin.unix)
	if CC.HOSTTYPE == "linux.*"
		FEATURE_GNU == 1
	end
	if FEATURE_CS == 1
		:PACKAGE: - ast
		.SOURCE: ../lefty/cs2l
		BINSRC += $(src.bin.cs)
		BINLIBS += -lcs
	end
end

BINSRC += lefty.c gfxview.c txtview.c internal.c display.c str.c exec.c \
	parse.c lex.c tbl.c code.c io.c
LIBSRC += g.c mem.c common.c
LEFTYSCRIPTS += def.lefty fractal.lefty tree.lefty lefty.psp

:YYPREFIX: config.h dotparse.y

:ALL:

$(BINNAME) :: $(BINSRC) -l$(LIBNAME) $(BINLIBS) -lm lefty.1
$(LIBNAME) :LIBRARY: $(LIBSRC) $(LIBLIBS)
$(LIBDIR)/lefty :INSTALLDIR: $(LEFTYSCRIPTS)
