:PACKAGE: dot:command lefty:command

LICENSE = since=1996,author=ek

$(BINDIR) :INSTALLDIR: dotty
$(LIBDIR)/lefty :INSTALLDIR: dotty.lefty dotty_draw.lefty dotty_edit.lefty \
	dotty_layout.lefty dotty_ui.lefty

if "$(CC.HOSTTYPE:N=win32.*)" && "$(CC:N=ncc|*/ncc)"
dotty :: dotty.c -luser32 -lgdi32 -lwinspool -lcomdlg32 -ladvapi32 -lshell32
else
dotty :: dotty.sh dotty.1
end

:: dotty.bsh dotty.c
