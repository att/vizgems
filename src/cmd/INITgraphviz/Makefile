:PACKAGE: graphviz X11

if CC.HOSTTYPE == "sgi.mips3"
	IFFEFLAGS += -I/usr/freeware/include
	IFFEFLAGS += -I/usr/freeware/include/freetype2
	IFFEFLAGS += -L/usr/freeware/lib32
end
if CC.HOSTTYPE == "sgi.mips4"
	IFFEFLAGS += -I/usr/freeware/include
	IFFEFLAGS += -I/usr/freeware/include/freetype2
	IFFEFLAGS += -L/usr/freeware/lib64
end

dontcare -lgraphviz

all : pkg-graphviz.mk graphvizconfig.h

pkg-graphviz.mk : graphvizconfig.h
	{
	cat <<!
	if CC.HOSTTYPE == "sgi.mips3"
		.SOURCE.h: /usr/freeware/include
		.SOURCE.h: /usr/freeware/include/freetype2
		.SOURCE.a: /usr/freeware/lib32
	end
	if CC.HOSTTYPE == "sgi.mips4"
		.SOURCE.h: /usr/freeware/include
		.SOURCE.h: /usr/freeware/include/freetype2
		.SOURCE.a: /usr/freeware/lib64
	end
	PACKAGE_graphviz_DATE = "2010.03.10"
	HAVE_CONFIG_H == 1
	:PACKAGE: - ast
	dontcare -lgraphviz
	!
	sed -e '/^#define[ 	][ 	]*[A-Z][A-Z_]*[ 	][ 	]*[^ 	]/!d' -e 's/^#define[ 	][ 	]*\([^ 	]*\)[ 	][ 	]*\([^ 	]*\).*/\1 = \2/' $(*:N=*.h)
	} > $(<)

$(INCLUDEDIR) :INSTALLDIR: graphvizconfig.h

$(LIBDIR)/make :INSTALLDIR: pkg-graphviz.mk
