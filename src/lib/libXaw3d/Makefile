:PACKAGE: X11 ast

VERSION = 8.0

if CC.HOSTTYPE == "sol*.*"
	NOX11EXT == 1
end

if CC.HOSTTYPE == "linux.*"
	YACC = bison
end

if CC.HOSTTYPE == "win*.*"
	X_NOT_STDC_ENV == 1
end

GRAY_BLKWHT_STIPPLES == 1
INTERNATIONALIZATION == 1

Xaw3d $(VERSION) :LIBRARY: \
	AllWidgets.c AsciiSink.c AsciiSrc.c AsciiText.c Box.c Command.c \
	Dialog.c Form.c Grip.c Label.c Layout.c List.c MenuButton.c MultiSrc.c \
	MultiSink.c Paned.c Panner.c Porthole.c Repeater.c Scrollbar.c \
	Simple.c SimpleMenu.c Sme.c SmeBSB.c SmeLine.c SmeThreeD.c \
	StripChart.c Text.c TextSink.c TextSrc.c TextAction.c TextPop.c \
	TextTr.c ThreeD.c Tip.c Toggle.c Tree.c Vendor.c Viewport.c XawIm.c \
	XawI18n.c XawInit.c Xaw3dP.c laygram.y laylex.l \
	$(CC.REQUIRE.Xaw:N!=-lXaw)

Xaw3dP.h : Xaw3dP.h.sed
	sed \
	    -e 's/\/\* I18n support \*\//#define XAW_INTERNATIONALIZATION/' \
	    -e 's/\/\* XPM support \*\//#undef XAW_MULTIPLANE_PIXMAPS/' \
	    -e 's/\/\* gray stipples \*\//#define XAW_GRAY_BLKWHT_STIPPLES/' \
	    -e 's/\/\* arrow scrollbars \*\//#undef XAW_ARROW_SCROLLBARS/' \
	$(*) > $(<)

:: README.XAW3D config.iffe \
	AllWidgets.h Template.c Template.h TemplateP.h sharedlib.c
