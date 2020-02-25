:PACKAGE: graphviz ast
.SOURCE.a: /usr/X11/lib64

CCFLAGS += $$(CC.DLL)

dot :: dot.c dot_builtins.c -lgv -lgraph -lgd -last

$(INSTALLROOT)/bin/circo :INSTALL: dot
$(INSTALLROOT)/bin/fdp :INSTALL: dot
$(INSTALLROOT)/bin/neato :INSTALL: dot
$(INSTALLROOT)/bin/sfdp :INSTALL: dot
$(INSTALLROOT)/bin/twopi :INSTALL: dot
$(INSTALLROOT)/bin/osage :INSTALL: dot
