:PACKAGE: graphviz X11

dontcare -lgraphviz

all : pkg-graphviz.mk graphvizconfig.h

pkg-graphviz.mk : graphvizconfig.h
	{
	cat <<!
	PACKAGE_graphviz_DATE = "2010.03.10"
	HAVE_CONFIG_H == 1
	:PACKAGE: - ast
	dontcare -lgraphviz
	!
	sed -e '/^#define[ 	][ 	]*[A-Z][A-Z_]*[ 	][ 	]*[^ 	]/!d' -e 's/^#define[ 	][ 	]*\([^ 	]*\)[ 	][ 	]*\([^ 	]*\).*/\1 = \2/' $(*:N=*.h)
	} > $(<)

$(INCLUDEDIR) :INSTALLDIR: graphvizconfig.h

$(LIBDIR)/make :INSTALLDIR: pkg-graphviz.mk
