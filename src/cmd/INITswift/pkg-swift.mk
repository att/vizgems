/*
 * swift common assertions
 */

SWIFTVERSION = 4.0
DDSLIB = $(INSTALLROOT)/lib/dds
DDSCC = ddscc
DDSCCFLAGS =


.ATTRIBUTE.%.aggr : .SCAN.c

.do.aggr : .USE
	$(DDSCC) $(DDSCCFLAGS) -so $(<) -aggregator -is $(*:N=*.schema:O=1) \
	-vt $(DDSAGGRVT) -af $(*:N=*.aggr:O=1)

":aggr:" : .MAKE .OPERATOR .PROBE.INIT
	local T
	T := $(<).aggr.so
	$(T) : .do.aggr $(>:N=*.schema) $(>:N=*.aggr) \
	DDSAGGRVT=$(>:N!=*.schema:N!=*.aggr)
	.ALL : $(T)
	$(DDSLIB) :INSTALLDIR: $(T) $(>:N=*.schema)


.ATTRIBUTE.%.conv : .SCAN.c

.do.conv : .USE
	DDSCONVIS=$(*:N=*.schema:O=1)
	DDSCONVOS=$(*:N=*.schema:O=2)
	DDSCONVOS=${DDSCONVOS:-$DDSCONVIS}
	$(DDSCC) $(DDSCCFLAGS) -so $(<) -converter \
	-is $DDSCONVIS -os $DDSCONVOS -cf $(*:N=*.conv:O=1)

":conv:" : .MAKE .OPERATOR .PROBE.INIT
	local T
	T := $(<).conv.so
	$(T) : .do.conv $(>)
	.ALL : $(T)
	$(DDSLIB) :INSTALLDIR: $(T) $(>:N=*.schema)


.do.count : .USE
	$(DDSCC) $(DDSCCFLAGS) -so $(<) -counter -is $(*:N=*.schema:O=1) \
	-ke "$(DDSCOUNTKEYFIELDS:/,/ /G)" -ce "$(DDSCOUNTCOUNTFIELDS:/,/ /G)"

":count:" : .MAKE .OPERATOR .PROBE.INIT
	local T
	T := $(<).count.so
	$(T) : .do.count $(>:N=*.schema) \
	    DDSCOUNTKEYFIELDS=$(>:O=2) DDSCOUNTCOUNTFIELDS=$(>:O=3)
	.ALL : $(T)
	$(DDSLIB) :INSTALLDIR: $(T) $(>:N=*.schema)


.do.extract : .USE
	ddscc -so $(<) -extractor \
	-is $(*:N=*.schema:O=1) -ee "$(DDSEXTRACTFIELDS:/,/ /G)"

":extract:" : .MAKE .OPERATOR .PROBE.INIT
	local T
	T := $(<).extract.so
	$(T) : .do.extract $(>:N=*.schema) DDSEXTRACTFIELDS=$(>:N!=*.schema)
	.ALL : $(T)
	$(DDSLIB) :INSTALLDIR: $(T) $(>:N=*.schema)


.ATTRIBUTE.%.filter : .SCAN.c

.do.filter : .USE
	$(DDSCC) $(DDSCCFLAGS) -so $(<) -filter \
	-is $(*:N=*.schema:O=1) -ff $(*:N=*.filter:O=1)

":filter:" : .MAKE .OPERATOR .PROBE.INIT
	local T
	T := $(<).filter.so
	$(T) : .do.filter $(>)
	.ALL : $(T)
	$(DDSLIB) :INSTALLDIR: $(T) $(>:N=*.schema)


.ATTRIBUTE.%.group : .SCAN.c

.do.group : .USE
	$(DDSCC) $(DDSCCFLAGS) -so $(<) -grouper \
	-is $(*:N=*.schema:O=1) -gf $(*:N=*.group:O=1)

":group:" : .MAKE .OPERATOR .PROBE.INIT
	local T
	T := $(<).group.so
	$(T) : .do.group $(>)
	.ALL : $(T)
	$(DDSLIB) :INSTALLDIR: $(T) $(>:N=*.schema)


.ATTRIBUTE.%.printer : .SCAN.c

.do.printer : .USE
	$(DDSCC) $(DDSCCFLAGS) -so $(<) -printer \
	-is $(*:N=*.schema:O=1) -pf $(*:N=*.printer:O=1)

":printer:" : .MAKE .OPERATOR .PROBE.INIT
	local T
	T := $(<).printer.so
	$(T) : .do.printer $(>)
	.ALL : $(T)
	$(DDSLIB) :INSTALLDIR: $(T) $(>:N=*.schema)


.do.load : .USE
	$(DDSCC) -so $(<) -loader -os $(*:N=*.schema:O=1)

":load:" : .MAKE .OPERATOR .PROBE.INIT
	local T
	T := $(<).load.so
	$(T) : .do.load $(>)
	.ALL : $(T)
	$(DDSLIB) :INSTALLDIR: $(T) $(>:N=*.schema)


.do.sort : .USE
	$(DDSCC) $(DDSCCFLAGS) -so $(<) -sorter -is $(*:N=*.schema:O=1) \
	-ke "$(DDSSORTFIELDS:/,/ /G)"

":sort:" : .MAKE .OPERATOR .PROBE.INIT
	local T
	T := $(<).sort.so
	$(T) : .do.sort $(>:N=*.schema) DDSSORTFIELDS=$(>:N!=*.schema)
	.ALL : $(T)
	$(DDSLIB) :INSTALLDIR: $(T) $(>:N=*.schema)


.do.split : .USE
	$(DDSCC) $(DDSCCFLAGS) -so $(<) -splitter -is $(*:N=*.schema:O=1) \
	-se "$(DDSSPLITFIELDS:/,/ /G)"

":split:" : .MAKE .OPERATOR .PROBE.INIT
	local T
	T := $(<).split.so
	$(T) : .do.split $(>:N=*.schema) DDSSPLITFIELDS=$(>:N!=*.schema)
	.ALL : $(T)
	$(DDSLIB) :INSTALLDIR: $(T) $(>:N=*.schema)

":manual:" : .MAKE .OPERATOR .PROBE.INIT
	.ALL : $(<)
	$(<) : $(*)
		ignore $(*) --??nroff 2> $(<)
