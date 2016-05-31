:PACKAGE: swift ast

LICENSE = since=1996,author=ek

FUSE_VERSION_MIN = 2.8.0
FUSE_VERSION = $(CC.STDLIB:L=libfuse.so.*.*.*:C/libfuse.so.//) $(FUSE_VERSION_MIN)

if CC.HOSTTYPE == "*"
	if "$(FUSE_VERSION:H=V:O=1)" != "$(FUSE_VERSION_MIN)"
		CCFLAGS += -pthread
		LDFLAGS += -pthread
		ndfsserver :: ndfsserver.c ndfsops.c ndfsutils.c -lfuse
	else
		:NOTHING:
	end
end
