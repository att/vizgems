/*
* $KK: ThreeD.c,v 0.3 92/11/04 xx:xx:xx keithley Exp $ 
*/

/***********************************************************
Copyright 1987, 1988 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.
Copyright 1992 by Kaleb Keithley

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Digital, MIT, or Kaleb 
Keithley not be used in advertising or publicity pertaining to distribution 
of the software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

/*
 * Portions Copyright (c) 2003 David J. Hawkey Jr.
 * Rights, permissions, and disclaimer per the above DEC/MIT license.
 */

#include <ast.h>
#include "Xaw3dP.h"
#include <X11/Xlib.h>
#include <X11/StringDefs.h>
#include <X11/IntrinsicP.h>
#include <XawInit.h>
#include <ThreeDP.h>
#include <X11/Xosdefs.h>

/* Initialization of defaults */

#define XtNtopShadowPixmap "topShadowPixmap"
#define XtCTopShadowPixmap "TopShadowPixmap"
#define XtNbottomShadowPixmap "bottomShadowPixmap"
#define XtCBottomShadowPixmap "BottomShadowPixmap"

static char defRelief[] = "Raised";

#define offset(field) XtOffsetOf(ThreeDRec, field)

static XtResource resources[] = {
    {XtNshadowWidth, XtCShadowWidth, XtRDimension, sizeof(Dimension),
	offset(threeD.shadow_width), XtRImmediate, (XtPointer) 2},
    {XtNtopShadowPixel, XtCTopShadowPixel, XtRPixel, sizeof(Pixel),
	offset(threeD.top_shadow_pixel), XtRString, XtDefaultForeground},
    {XtNbottomShadowPixel, XtCBottomShadowPixel, XtRPixel, sizeof(Pixel),
	offset(threeD.bot_shadow_pixel), XtRString, XtDefaultForeground},
    {XtNtopShadowPixmap, XtCTopShadowPixmap, XtRPixmap, sizeof(Pixmap),
	offset(threeD.top_shadow_pxmap), XtRImmediate, (XtPointer) NULL},
    {XtNbottomShadowPixmap, XtCBottomShadowPixmap, XtRPixmap, sizeof(Pixmap),
	offset(threeD.bot_shadow_pxmap), XtRImmediate, (XtPointer) NULL},
    {XtNtopShadowContrast, XtCTopShadowContrast, XtRInt, sizeof(int),
	offset(threeD.top_shadow_contrast), XtRImmediate, (XtPointer) 20},
    {XtNbottomShadowContrast, XtCBottomShadowContrast, XtRInt, sizeof(int),
	offset(threeD.bot_shadow_contrast), XtRImmediate, (XtPointer) 40},
    {XtNuserData, XtCUserData, XtRPointer, sizeof(XtPointer),
	offset(threeD.user_data), XtRPointer, (XtPointer) NULL},
    {XtNbeNiceToColormap, XtCBeNiceToColormap, XtRBoolean, sizeof(Boolean),
	offset(threeD.be_nice_to_cmap), XtRImmediate, (XtPointer) True},
    {XtNborderWidth, XtCBorderWidth, XtRDimension, sizeof(Dimension),
	XtOffsetOf(RectObjRec,rectangle.border_width), XtRImmediate,
	(XtPointer)0},
    {XtNinvertBorder, XtCInvertBorder, XtRBoolean, sizeof(Boolean),
	offset(threeD.invert_border), XtRImmediate, (XtPointer) False},
    {XtNrelief, XtCRelief, XtRRelief, sizeof(XtRelief),
	offset(threeD.relief), XtRString, (XtPointer) defRelief}
};

#undef offset

static void ClassInitialize(), ClassPartInitialize(), Initialize(), Destroy();
static void Redisplay(), Realize(), _Xaw3dDrawShadows();
static Boolean SetValues();

ThreeDClassRec threeDClassRec = {
    { /* core fields */
    /* superclass		*/	(WidgetClass) &simpleClassRec,
    /* class_name		*/	"ThreeD",
    /* widget_size		*/	sizeof(ThreeDRec),
    /* class_initialize		*/	ClassInitialize,
    /* class_part_initialize	*/	ClassPartInitialize,
    /* class_inited		*/	FALSE,
    /* initialize		*/	Initialize,
    /* initialize_hook		*/	NULL,
    /* realize			*/	Realize,
    /* actions			*/	NULL,
    /* num_actions		*/	0,
    /* resources		*/	resources,
    /* resource_count		*/	XtNumber(resources),
    /* xrm_class		*/	NULLQUARK,
    /* compress_motion		*/	TRUE,
    /* compress_exposure	*/	TRUE,
    /* compress_enterleave	*/	TRUE,
    /* visible_interest		*/	FALSE,
    /* destroy			*/	Destroy,
    /* resize			*/	XtInheritResize,
    /* expose			*/	Redisplay,
    /* set_values		*/	SetValues,
    /* set_values_hook		*/	NULL,
    /* set_values_almost	*/	XtInheritSetValuesAlmost,
    /* get_values_hook		*/	NULL,
    /* accept_focus		*/	NULL,
    /* version			*/	XtVersion,
    /* callback_private		*/	NULL,
    /* tm_table			*/	NULL,
    /* query_geometry           */	XtInheritQueryGeometry,
    /* display_accelerator      */	XtInheritDisplayAccelerator,
    /* extension                */	NULL
    },
    { /* simple fields */
    /* change_sensitive         */      XtInheritChangeSensitive
    },
    { /* threeD fields */
    /* shadow draw              */      _Xaw3dDrawShadows
    }
};

WidgetClass threeDWidgetClass = (WidgetClass) &threeDClassRec;

/****************************************************************
 *
 * Private Procedures
 *
 ****************************************************************/

#define mbshadowpm_size 3
static char mbshadowpm_bits[] = {0x05, 0x03, 0x06};

#define mtshadowpm_size 3
static char mtshadowpm_bits[] = {0x02, 0x04, 0x01};

#define shadowpm_size 2
static char shadowpm_bits[] = {0x02, 0x01};

/* ARGSUSED */
static void AllocTopShadowGC (w)
    Widget w;
{
    ThreeDWidget	tdw = (ThreeDWidget) w;
    Screen		*scn = XtScreen (w);
    XtGCMask		valuemask;
    XGCValues		myXGCV;

    if (tdw->threeD.be_nice_to_cmap || DefaultDepthOfScreen (scn) == 1) {
	valuemask = GCTile | GCFillStyle;
	myXGCV.tile = tdw->threeD.top_shadow_pxmap;
	myXGCV.fill_style = FillTiled;
    } else {
	valuemask = GCForeground;
	myXGCV.foreground = tdw->threeD.top_shadow_pixel;
    }
    tdw->threeD.top_shadow_GC = XtGetGC(w, valuemask, &myXGCV);
}

/* ARGSUSED */
static void AllocBotShadowGC (w)
    Widget w;
{
    ThreeDWidget	tdw = (ThreeDWidget) w;
    Screen		*scn = XtScreen (w);
    XtGCMask		valuemask;
    XGCValues		myXGCV;

    if (tdw->threeD.be_nice_to_cmap || DefaultDepthOfScreen (scn) == 1) {
	valuemask = GCTile | GCFillStyle;
	myXGCV.tile = tdw->threeD.bot_shadow_pxmap;
	myXGCV.fill_style = FillTiled;
    } else {
	valuemask = GCForeground;
	myXGCV.foreground = tdw->threeD.bot_shadow_pixel;
    }
    tdw->threeD.bot_shadow_GC = XtGetGC(w, valuemask, &myXGCV);
}

/* ARGSUSED */
static void AllocTopShadowPixmap (new)
    Widget new;
{
    ThreeDWidget	tdw = (ThreeDWidget) new;
    Display		*dpy = XtDisplay (new);
    Screen		*scn = XtScreen (new);
    unsigned long	top_fg_pixel = 0, top_bg_pixel = 0;
    char		*pm_data = NULL;
    Boolean		create_pixmap = FALSE;
    unsigned int        pm_size;

    /*
     * I know, we're going to create two pixmaps for each and every
     * shadow'd widget.  Yeuck.  I'm semi-relying on server side
     * pixmap cacheing.
     */

    if (DefaultDepthOfScreen (scn) == 1) {
	top_fg_pixel = BlackPixelOfScreen (scn);
	top_bg_pixel = WhitePixelOfScreen (scn);
	pm_data = mtshadowpm_bits;
        pm_size = mtshadowpm_size;
	create_pixmap = TRUE;
    } else if (tdw->threeD.be_nice_to_cmap) {
	if (tdw->core.background_pixel == WhitePixelOfScreen (scn)) {
	    top_fg_pixel = WhitePixelOfScreen (scn);
	    top_bg_pixel = grayPixel( BlackPixelOfScreen (scn), dpy, scn);
	} else if (tdw->core.background_pixel == BlackPixelOfScreen (scn)) {
	    top_fg_pixel = grayPixel( BlackPixelOfScreen (scn), dpy, scn);
	    top_bg_pixel = WhitePixelOfScreen (scn);
	} else {
	    top_fg_pixel = tdw->core.background_pixel;
	    top_bg_pixel = WhitePixelOfScreen (scn);
	}
#ifndef XAW_GRAY_BLKWHT_STIPPLES
	if (tdw->core.background_pixel == WhitePixelOfScreen (scn) ||
	    tdw->core.background_pixel == BlackPixelOfScreen (scn)) {
	    pm_data = mtshadowpm_bits;
	    pm_size = mtshadowpm_size;
	} else 
#endif
	{
	    pm_data = shadowpm_bits;
	    pm_size = shadowpm_size;
	}
	create_pixmap = TRUE;
    } else {
	pm_size = 0; /* keep gcc happy */
    }

    if (create_pixmap)
	tdw->threeD.top_shadow_pxmap = XCreatePixmapFromBitmapData (dpy,
			RootWindowOfScreen (scn),
			pm_data,
			pm_size,
			pm_size,
			top_fg_pixel,
			top_bg_pixel,
			DefaultDepthOfScreen (scn));
}

/* ARGSUSED */
static void AllocBotShadowPixmap (new)
    Widget new;
{
    ThreeDWidget	tdw = (ThreeDWidget) new;
    Display		*dpy = XtDisplay (new);
    Screen		*scn = XtScreen (new);
    unsigned long	bot_fg_pixel = 0, bot_bg_pixel = 0;
    char		*pm_data = NULL;
    Boolean		create_pixmap = FALSE;
    unsigned int        pm_size;

    if (DefaultDepthOfScreen (scn) == 1) {
	bot_fg_pixel = BlackPixelOfScreen (scn);
	bot_bg_pixel = WhitePixelOfScreen (scn);
	pm_data = mbshadowpm_bits;
        pm_size = mbshadowpm_size;
	create_pixmap = TRUE;
    } else if (tdw->threeD.be_nice_to_cmap) {
	if (tdw->core.background_pixel == WhitePixelOfScreen (scn)) {
	    bot_fg_pixel = grayPixel( WhitePixelOfScreen (scn), dpy, scn);
	    bot_bg_pixel = BlackPixelOfScreen (scn);
	} else if (tdw->core.background_pixel == BlackPixelOfScreen (scn)) {
	    bot_fg_pixel = BlackPixelOfScreen (scn);
	    bot_bg_pixel = grayPixel( BlackPixelOfScreen (scn), dpy, scn);
	} else {
	    bot_fg_pixel = tdw->core.background_pixel;
	    bot_bg_pixel = BlackPixelOfScreen (scn);
	}
#ifndef XAW_GRAY_BLKWHT_STIPPLES
	if (tdw->core.background_pixel == WhitePixelOfScreen (scn) ||
	    tdw->core.background_pixel == BlackPixelOfScreen (scn)) {
	    pm_data = mbshadowpm_bits;
	    pm_size = mbshadowpm_size;
	} else 
#endif
        {
	    pm_data = shadowpm_bits;
	    pm_size = shadowpm_size;
        }
	create_pixmap = TRUE;
    } else {
	pm_size = 0; /* keep gcc happy */
    }

    if (create_pixmap)
	tdw->threeD.bot_shadow_pxmap = XCreatePixmapFromBitmapData (dpy,
			RootWindowOfScreen (scn),
			pm_data,
			pm_size,
			pm_size,
			bot_fg_pixel,
			bot_bg_pixel,
			DefaultDepthOfScreen (scn));
}

/* ARGSUSED */
void Xaw3dComputeTopShadowRGB (new, xcol_out)
    Widget new;
    XColor *xcol_out;
{
    if (XtIsSubclass (new, threeDWidgetClass)) {
	ThreeDWidget tdw = (ThreeDWidget) new;
	XColor get_c;
	double contrast;
	Display *dpy = XtDisplay (new);
	Screen *scn = XtScreen (new);
	Colormap cmap = new->core.colormap;

	get_c.pixel = tdw->core.background_pixel;
	if (get_c.pixel == WhitePixelOfScreen (scn) ||
	    get_c.pixel == BlackPixelOfScreen (scn)) {
	    contrast = (100 - tdw->threeD.top_shadow_contrast) / 100.0;
	    xcol_out->red   = contrast * 65535.0;
	    xcol_out->green = contrast * 65535.0;
	    xcol_out->blue  = contrast * 65535.0;
	} else {
	    contrast = 1.0 + tdw->threeD.top_shadow_contrast / 100.0;
	    XQueryColor (dpy, cmap, &get_c);
#define MIN(x,y) (unsigned short) (x < y) ? x : y
	    xcol_out->red   = MIN (65535, (int) (contrast * (double) get_c.red));
	    xcol_out->green = MIN (65535, (int) (contrast * (double) get_c.green));
	    xcol_out->blue  = MIN (65535, (int) (contrast * (double) get_c.blue));
#undef MIN
	}
    } else
	xcol_out->red = xcol_out->green = xcol_out->blue = 0;
}

/* ARGSUSED */
static void AllocTopShadowPixel (new)
    Widget new;
{
    XColor set_c;
    ThreeDWidget tdw = (ThreeDWidget) new;
    Display *dpy = XtDisplay (new);
    Screen *scn = XtScreen (new);
    Colormap cmap = new->core.colormap;

    Xaw3dComputeTopShadowRGB (new, &set_c);
    (void) XAllocColor (dpy, cmap, &set_c);
    tdw->threeD.top_shadow_pixel = set_c.pixel;
}

/* ARGSUSED */
void Xaw3dComputeBottomShadowRGB (new, xcol_out)
    Widget new;
    XColor *xcol_out;
{
    if (XtIsSubclass (new, threeDWidgetClass)) {
	ThreeDWidget tdw = (ThreeDWidget) new;
	XColor get_c;
	double contrast;
	Display *dpy = XtDisplay (new);
	Screen *scn = XtScreen (new);
	Colormap cmap = new->core.colormap;

	get_c.pixel = tdw->core.background_pixel;
	if (get_c.pixel == WhitePixelOfScreen (scn) ||
	    get_c.pixel == BlackPixelOfScreen (scn)) {
	    contrast = tdw->threeD.bot_shadow_contrast / 100.0;
	    xcol_out->red   = contrast * 65535.0;
	    xcol_out->green = contrast * 65535.0;
	    xcol_out->blue  = contrast * 65535.0;
	} else {
	    XQueryColor (dpy, cmap, &get_c);
	    contrast = (100 - tdw->threeD.bot_shadow_contrast) / 100.0;
	    xcol_out->red   = contrast * get_c.red;
	    xcol_out->green = contrast * get_c.green;
	    xcol_out->blue  = contrast * get_c.blue;
	}
    } else
	xcol_out->red = xcol_out->green = xcol_out->blue = 0;
}

/* ARGSUSED */
static void AllocBotShadowPixel (new)
    Widget new;
{
    XColor set_c;
    ThreeDWidget tdw = (ThreeDWidget) new;
    Display *dpy = XtDisplay (new);
    Screen *scn = XtScreen (new);
    Colormap cmap = new->core.colormap;

    Xaw3dComputeBottomShadowRGB (new, &set_c);
    (void) XAllocColor (dpy, cmap, &set_c);
    tdw->threeD.bot_shadow_pixel = set_c.pixel;
}


static XrmQuark	XtQReliefNone, XtQReliefRaised, XtQReliefSunken,
		XtQReliefRidge, XtQReliefGroove;

#define	done(address, type) { \
	  toVal->size = sizeof(type); \
	  toVal->addr = (XPointer) address; \
	  return; \
	}

/* ARGSUSED */
static void _CvtStringToRelief(args, num_args, fromVal, toVal)
    XrmValuePtr args;          /* unused */
    Cardinal    *num_args;      /* unused */
    XrmValuePtr fromVal;
    XrmValuePtr toVal;
{
    static XtRelief relief;
    XrmQuark q;
    char lowerName[1000];

    XmuCopyISOLatin1Lowered (lowerName, (char*)fromVal->addr);
    q = XrmStringToQuark(lowerName);
    if (q == XtQReliefNone) {
       relief = XtReliefNone;
       done(&relief, XtRelief);
    }
    if (q == XtQReliefRaised) {
       relief = XtReliefRaised;
       done(&relief, XtRelief);
    }
    if (q == XtQReliefSunken) {
       relief = XtReliefSunken;
       done(&relief, XtRelief);
    }    
    if (q == XtQReliefRidge) {
       relief = XtReliefRidge;
       done(&relief, XtRelief);
    }
    if (q == XtQReliefGroove) {
       relief = XtReliefGroove;
       done(&relief, XtRelief);
    }
    XtStringConversionWarning(fromVal->addr, "relief");
    toVal->addr = NULL;
    toVal->size = 0;
}

static void ClassInitialize()
{
    XawInitializeWidgetSet();
    XtQReliefNone   = XrmPermStringToQuark("none");
    XtQReliefRaised = XrmPermStringToQuark("raised");
    XtQReliefSunken = XrmPermStringToQuark("sunken");
    XtQReliefRidge  = XrmPermStringToQuark("ridge");
    XtQReliefGroove = XrmPermStringToQuark("groove");

    XtAddConverter( XtRString, XtRRelief, _CvtStringToRelief, 
                   (XtConvertArgList)NULL, 0 );
}


/* ARGSUSED */
static void ClassPartInitialize (wc)
    WidgetClass	wc;

{
    ThreeDClassRec *tdwc = (ThreeDClassRec*) wc;
    ThreeDClassRec *super = (ThreeDClassRec*) tdwc->core_class.superclass;

    if (tdwc->threeD_class.shadowdraw == XtInheritXaw3dShadowDraw)
	tdwc->threeD_class.shadowdraw = super->threeD_class.shadowdraw;
}


/* ARGSUSED */
static void Initialize (request, new, args, num_args)
    Widget request, new;
    ArgList args;
    Cardinal *num_args;
{
    ThreeDWidget 	tdw = (ThreeDWidget) new;
    Screen		*scr = XtScreen (new);

    if (tdw->threeD.be_nice_to_cmap || DefaultDepthOfScreen (scr) == 1) {
	AllocTopShadowPixmap (new);
	AllocBotShadowPixmap (new);
    } else {
	if (tdw->threeD.top_shadow_pixel == tdw->threeD.bot_shadow_pixel) {
	    /* 
		Eeek.  We're probably going to XQueryColor() twice 
		for each widget.  Necessary because you can set the
		top and bottom shadows independent of each other in
		SetValues.  Some cacheing would certainly help...
	    */
	    AllocTopShadowPixel (new);
	    AllocBotShadowPixel (new);
	}
	tdw->threeD.top_shadow_pxmap = tdw->threeD.bot_shadow_pxmap = (Pixmap) 0;
    }
    AllocTopShadowGC (new);
    AllocBotShadowGC (new);
}

static void Realize (gw, valueMask, attrs)
    Widget gw;
    XtValueMask *valueMask;
    XSetWindowAttributes *attrs;
{
 /* 
  * This is necessary because Simple doesn't have a realize method
  * XtInheritRealize in the ThreeD class record doesn't work.  This
  * daisychains through Simple to the Core class realize method
  */
    (*threeDWidgetClass->core_class.superclass->core_class.realize)
	 (gw, valueMask, attrs);
}

static void Destroy (w)
     Widget w;
{
    ThreeDWidget tdw = (ThreeDWidget) w;
    XtReleaseGC (w, tdw->threeD.top_shadow_GC);
    XtReleaseGC (w, tdw->threeD.bot_shadow_GC);
    if (tdw->threeD.top_shadow_pxmap)
	XFreePixmap (XtDisplay (w), tdw->threeD.top_shadow_pxmap);
    if (tdw->threeD.bot_shadow_pxmap)
	XFreePixmap (XtDisplay (w), tdw->threeD.bot_shadow_pxmap);
}

/* ARGSUSED */
static void Redisplay (w, event, region)
    Widget w;
    XEvent *event;		/* unused */
    Region region;		/* unused */
{
    ThreeDWidget tdw = (ThreeDWidget) w;

    _Xaw3dDrawShadows (w, event, region, tdw->threeD.relief, True);
}

/* ARGSUSED */
static Boolean SetValues (gcurrent, grequest, gnew, args, num_args)
    Widget gcurrent, grequest, gnew;
    ArgList args;
    Cardinal *num_args;
{
    ThreeDWidget current = (ThreeDWidget) gcurrent;
    ThreeDWidget new = (ThreeDWidget) gnew;
    Boolean redisplay = FALSE;
    Boolean alloc_top_pixel = FALSE;
    Boolean alloc_bot_pixel = FALSE;
    Boolean alloc_top_pxmap = FALSE;
    Boolean alloc_bot_pxmap = FALSE;

    (*threeDWidgetClass->core_class.superclass->core_class.set_values) 
	(gcurrent, grequest, gnew, NULL, 0);
    if (new->threeD.relief != current->threeD.relief)
	redisplay = TRUE;
    if (new->threeD.shadow_width != current->threeD.shadow_width)
	redisplay = TRUE;
    if (new->threeD.be_nice_to_cmap != current->threeD.be_nice_to_cmap) {
	if (new->threeD.be_nice_to_cmap) {
	    alloc_top_pxmap = TRUE;
	    alloc_bot_pxmap = TRUE;
	} else {
	    alloc_top_pixel = TRUE;
	    alloc_bot_pixel = TRUE;
	}
	redisplay = TRUE;
    }
    if (!new->threeD.be_nice_to_cmap &&
	new->threeD.top_shadow_contrast != current->threeD.top_shadow_contrast)
	alloc_top_pixel = TRUE;
    if (!new->threeD.be_nice_to_cmap &&
	new->threeD.bot_shadow_contrast != current->threeD.bot_shadow_contrast)
	alloc_bot_pixel = TRUE;
    if (alloc_top_pixel)
	AllocTopShadowPixel (gnew);
    if (alloc_bot_pixel)
	AllocBotShadowPixel (gnew);
    if (alloc_top_pxmap)
	AllocTopShadowPixmap (gnew);
    if (alloc_bot_pxmap)
	AllocBotShadowPixmap (gnew);
    if (!new->threeD.be_nice_to_cmap &&
	new->threeD.top_shadow_pixel != current->threeD.top_shadow_pixel)
	alloc_top_pixel = TRUE;
    if (!new->threeD.be_nice_to_cmap &&
	new->threeD.bot_shadow_pixel != current->threeD.bot_shadow_pixel)
	alloc_bot_pixel = TRUE;
    if (new->threeD.be_nice_to_cmap) {
	if (alloc_top_pxmap) {
	    XtReleaseGC (gcurrent, current->threeD.top_shadow_GC);
	    AllocTopShadowGC (gnew);
	    redisplay = True;
	}
	if (alloc_bot_pxmap) {
	    XtReleaseGC (gcurrent, current->threeD.bot_shadow_GC);
	    AllocBotShadowGC (gnew);
	    redisplay = True;
	}
    } else {
	if (alloc_top_pixel) {
	    if (new->threeD.top_shadow_pxmap) {
		XFreePixmap (XtDisplay (gnew), new->threeD.top_shadow_pxmap);
		new->threeD.top_shadow_pxmap = (Pixmap) NULL;
	    }
	    XtReleaseGC (gcurrent, current->threeD.top_shadow_GC);
	    AllocTopShadowGC (gnew);
	    redisplay = True;
	}
	if (alloc_bot_pixel) {
	    if (new->threeD.bot_shadow_pxmap) {
		XFreePixmap (XtDisplay (gnew), new->threeD.bot_shadow_pxmap);
		new->threeD.bot_shadow_pxmap = (Pixmap) NULL;
	    }
	    XtReleaseGC (gcurrent, current->threeD.bot_shadow_GC);
	    AllocBotShadowGC (gnew);
	    redisplay = True;
	}
    }
    return (redisplay);
}

/* ARGSUSED */
static void
_Xaw3dDrawShadows (gw, event, region, relief, out)
    Widget gw;
    XEvent *event;
    Region region;
    XtRelief relief;
    Boolean out;
{
    XPoint	pt[6];
    ThreeDWidget tdw = (ThreeDWidget) gw;
    Dimension	s = tdw->threeD.shadow_width;

    /* 
     * Draw the shadows using the core part width and height, 
     * and the threeD part relief and shadow_width.
     * No point to do anything if the shadow_width is 0 or the
     * widget has not been realized.
     */ 
    if ((s > 0) && XtIsRealized (gw)) {
	Dimension	h = tdw->core.height;
	Dimension	w = tdw->core.width;
	Dimension	hms = h - s;
	Dimension	wms = w - s;
	Display		*dpy = XtDisplay (gw);
	Window		win = XtWindow (gw);
	GC		realtop = tdw->threeD.top_shadow_GC;
	GC		realbot = tdw->threeD.bot_shadow_GC;
	GC		top, bot;

	if ((out ^ tdw->threeD.invert_border)) {
	    top = tdw->threeD.top_shadow_GC;
	    bot = tdw->threeD.bot_shadow_GC;
	} else {
	    top = tdw->threeD.bot_shadow_GC;
	    bot = tdw->threeD.top_shadow_GC;
	}

	if (relief == XtReliefRaised || relief == XtReliefSunken) {
	    /* top-left shadow */
	    if ((region == NULL) ||
		    (XRectInRegion (region, 0, 0, w, s) != RectangleOut) ||
		    (XRectInRegion (region, 0, 0, s, h) != RectangleOut)) {
		pt[0].x = 0;	pt[0].y = h;
		pt[1].x =	pt[1].y = 0;
		pt[2].x = w;	pt[2].y = 0;
		pt[3].x = wms;	pt[3].y = s;
		pt[4].x =	pt[4].y = s;
		pt[5].x = s;	pt[5].y = hms;
		XFillPolygon (dpy, win,
			(relief == XtReliefRaised) ? top : bot,
			pt, 6, Complex, CoordModeOrigin);
	    }

	    /* bottom-right shadow */
	    if ((region == NULL) ||
		    (XRectInRegion (region, 0, hms, w, s) != RectangleOut) ||
		    (XRectInRegion (region, wms, 0, s, h) != RectangleOut)) {
		pt[0].x = 0;	pt[0].y = h;
		pt[1].x = w;	pt[1].y = h;
		pt[2].x = w;	pt[2].y = 0; 
		pt[3].x = wms;	pt[3].y = s;
		pt[4].x = wms;	pt[4].y = hms;
		pt[5].x = s;	pt[5].y = hms; 
		XFillPolygon (dpy, win,
			(relief == XtReliefRaised) ? bot : top,
			pt, 6, Complex, CoordModeOrigin);
	    }
	} else if (relief == XtReliefRidge || relief == XtReliefGroove) {
	    /* split the shadow width */
	    s /= 2;	hms = h - s;	wms = w - s;

	    /* outer top-left shadow */
	    if ((region == NULL) ||
		    (XRectInRegion (region, 0, 0, w, s) != RectangleOut) ||
		    (XRectInRegion (region, 0, 0, s, h) != RectangleOut)) {
		pt[0].x = 0;	pt[0].y = h;
		pt[1].x =	pt[1].y = 0;
		pt[2].x = w;	pt[2].y = 0;
		pt[3].x = wms;	pt[3].y = s;
		pt[4].x =	pt[4].y = s;
		pt[5].x = s;	pt[5].y = hms;
		XFillPolygon (dpy, win,
			(relief == XtReliefRidge) ? realtop : realbot,
			pt, 6, Complex, CoordModeOrigin);
	    }

	    /* outer bottom-right shadow */
	    if ((region == NULL) ||
		    (XRectInRegion (region, 0, hms, w, s) != RectangleOut) ||
		    (XRectInRegion (region, wms, 0, s, h) != RectangleOut)) {
		pt[0].x = 0;	pt[0].y = h;
		pt[1].x = w;	pt[1].y = h;
		pt[2].x = w;	pt[2].y = 0; 
		pt[3].x = wms;	pt[3].y = s;
		pt[4].x = wms;	pt[4].y = hms;
		pt[5].x = s;	pt[5].y = hms; 
		XFillPolygon (dpy, win,
			(relief == XtReliefRidge) ? realbot : realtop,
			pt, 6, Complex, CoordModeOrigin);
	    }

	    /* inner top-left shadow */
	    if ((region == NULL) ||
		    (XRectInRegion (region, 0, 0, w, s) != RectangleOut) ||
		    (XRectInRegion (region, 0, 0, s, h) != RectangleOut)) {
		pt[0].x = s;		pt[0].y = h;
		pt[1].x =		pt[1].y = s;
		pt[2].x = w;		pt[2].y = s;
		pt[3].x = wms;		pt[3].y = s * 2;
		pt[4].x =		pt[4].y = s * 2;
		pt[5].x = s * 2;	pt[5].y = hms;
		XFillPolygon (dpy, win,
			(relief == XtReliefRidge) ? bot: top,
			pt, 6, Complex, CoordModeOrigin);
	    }

	    /* inner bottom-right shadow */
	    if ((region == NULL) ||
		    (XRectInRegion (region, 0, hms, w, s) != RectangleOut) ||
		    (XRectInRegion (region, wms, 0, s, h) != RectangleOut)) {
		pt[0].x = s;		pt[0].y = hms;
		pt[1].x = wms;		pt[1].y = hms;
		pt[2].x = wms;		pt[2].y = s; 
		pt[3].x = wms - s;	pt[3].y = s * 2;
		pt[4].x = wms - s;	pt[4].y = hms - s;
		pt[5].x = s * 2;	pt[5].y = hms - s; 
		XFillPolygon (dpy, win,
			(relief == XtReliefRidge) ? top : bot,
			pt, 6, Complex, CoordModeOrigin);
	    }
	}
    }
}

/*
 * _ShadowSurroundedBox() is somewhat redundant with _Xaw3dDrawShadows(),
 * but has a more explicit interface, and ignores threeD part relief.
 */

/* ARGSUSED */
void
_ShadowSurroundedBox(gw, tdw, x0, y0, x1, y1, relief, out)
Widget gw;
ThreeDWidget tdw;
Position x0, y0, x1, y1;
XtRelief relief;	/* unused */
Boolean out;
{
    XPoint pt[6];
    Dimension s = tdw->threeD.shadow_width;

    /*
     * Draw the shadows using the core part width and height,
     * and the threeD part shadow_width.
     * No point to do anything if the shadow_width is 0 or the
     * widget has not been realized.
     */
    if ((s > 0) && XtIsRealized(gw))
    {
	Dimension h = y1 - y0;
	Dimension w = x1 - x0;
	Dimension wms = w - s;
	Dimension hms = h - s;
	Dimension sm = (s > 1 ? s / 2 : 1);
	Dimension wmsm = w - sm;
	Dimension hmsm = h - sm;
	Display *dpy = XtDisplay(gw);
	Window win = XtWindow(gw);
	GC top, bot;

	if ((out ^ tdw->threeD.invert_border))
	{
	    top = tdw->threeD.top_shadow_GC;
	    bot = tdw->threeD.bot_shadow_GC;
	}
	else
	{
	    top = tdw->threeD.bot_shadow_GC;
	    bot = tdw->threeD.top_shadow_GC;
	}

	/* top-left shadow */
	pt[0].x = x0;		pt[0].y = y0 + h;
	pt[1].x = x0;		pt[1].y = y0;
	pt[2].x = x0 + w;	pt[2].y = y0;
	pt[3].x = x0 + wmsm;	pt[3].y = y0 + sm - 1;
	pt[4].x = x0 + sm;	pt[4].y = y0 + sm;
	pt[5].x = x0 + sm - 1;	pt[5].y = y0 + hmsm;
	XFillPolygon(dpy, win, top, pt, 6, Complex, CoordModeOrigin);
	if (s > 1)
	{
	    pt[0].x = x0 + s - 1;	pt[0].y = y0 + hms;
	    pt[1].x = x0 + s;		pt[1].y = y0 + s;
	    pt[2].x = x0 + wms;		pt[2].y = y0 + s - 1;
	    XFillPolygon(dpy, win, top, pt, 6, Complex, CoordModeOrigin);
	}

	/* bottom-right shadow */
	pt[0].x = x0;		pt[0].y = y0 + h;
	pt[1].x = x0 + w;	pt[1].y = y0 + h;
	pt[2].x = x0 + w;	pt[2].y = y0;
	pt[3].x = x0 + wmsm;	pt[3].y = y0 + sm - 1;
	pt[4].x = x0 + wmsm;	pt[4].y = y0 + hmsm;
	pt[5].x = x0 + sm - 1;	pt[5].y = y0 + hmsm;
	XFillPolygon(dpy, win, bot, pt, 6, Complex, CoordModeOrigin);
	if (s > 1)
	{
	    pt[0].x = x0 + s - 1;	pt[0].y = y0 + hms;
	    pt[1].x = x0 + wms;		pt[1].y = y0 + hms;
	    pt[2].x = x0 + wms;		pt[2].y = y0 + s - 1;
	    XFillPolygon(dpy, win, bot, pt, 6, Complex, CoordModeOrigin);
	}
    }
}

