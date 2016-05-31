:PACKAGE: swift ast tiger:command

LICENSE = since=1996,author=ek

$(INCLUDEDIR) :INSTALLDIR: geom.h

cnvlki :: cnvlki.c -lm cnvlki.1
ascii2geom :: ascii2geom.c -lgeom -lm ascii2geom.1
shapefile2geom :: shapefile2geom.c -lgeom -lm shapefile2geom.1
cnvnpanxx :: cnvnpanxx.c -lm cnvnpanxx.1
tiger2geom :: tiger2geom.c -ltiger -lgeom tiger2geom.1
tiger2ps :: tiger2ps.c -ltiger tiger2ps.1
tiger2off :: tiger2off.c -ltiger tiger2off.1
tiger2ascii :: tiger2ascii.c -ltiger tiger2ascii.1
geomprint :: geomprint.c -lgeom geomprint.1

geom :LIBRARY: libgeom.c

cnvlki.1 :manual: cnvlki
ascii2geom.1 :manual: ascii2geom
shapefile2geom.1 :manual: shapefile2geom
cnvnpanxx.1 :manual: cnvnpanxx
tiger2geom.1 :manual: tiger2geom
tiger2ps.1 :manual: tiger2ps
tiger2off.1 :manual: tiger2off
tiger2ascii.1 :manual: tiger2ascii
geomprint.1 :manual: geomprint
