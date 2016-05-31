/* $XConsortium: SmeBSB.c,v 1.19 94/04/17 20:12:49 kaleb Exp $ */

/*
Copyright (c) 1989, 1994  X Consortium

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
X CONSORTIUM BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the X Consortium shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the X Consortium.
 */

/*
 * Portions Copyright (c) 2003 Brian V. Smith
 * Rights, permissions, and disclaimer per the above X Consortium license.
 */

/*
 * SmeBSB.c - Source code file for BSB Menu Entry object.
 *
 * Date:    September 26, 1989
 *
 * By:      Chris D. Peterson
 *          MIT X Consortium 
 *          kit@expo.lcs.mit.edu
 */

#include <ast.h>
#include "Xaw3dP.h"
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xos.h>
#include <X11/Xmu/Drawing.h>
#include <XawInit.h>
#include <ThreeDP.h>
#include <SimpleMenP.h>
#include <SmeBSBP.h>
#include <Cardinals.h>
#include <stdio.h>
#include "XawAlloc.h"

/* needed for abs() */
#ifndef X_NOT_STDC_ENV
#include <stdlib.h>
#else
extern int abs();
#endif

#define offset(field) XtOffsetOf(SmeBSBRec, sme_bsb.field)

static XtResource resources[] = {
  {XtNlabel,  XtCLabel, XtRString, sizeof(String),
     offset(label), XtRString, NULL},
  {XtNvertSpace,  XtCVertSpace, XtRInt, sizeof(int),
     offset(vert_space), XtRImmediate, (XtPointer) 25},
  {XtNleftBitmap, XtCLeftBitmap, XtRBitmap, sizeof(Pixmap),
     offset(left_bitmap), XtRImmediate, (XtPointer)None},
  {XtNjustify, XtCJustify, XtRJustify, sizeof(XtJustify),
     offset(justify), XtRImmediate, (XtPointer) XtJustifyLeft},
  {XtNrightBitmap, XtCRightBitmap, XtRBitmap, sizeof(Pixmap),
     offset(right_bitmap), XtRImmediate, (XtPointer)None},
  {XtNleftMargin,  XtCHorizontalMargins, XtRDimension, sizeof(Dimension),
     offset(left_margin), XtRImmediate, (XtPointer) 4},
  {XtNrightMargin,  XtCHorizontalMargins, XtRDimension, sizeof(Dimension),
     offset(right_margin), XtRImmediate, (XtPointer) 4},
  {XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel),
     offset(foreground), XtRString, XtDefaultForeground},
  {XtNfont,  XtCFont, XtRFontStruct, sizeof(XFontStruct *),
     offset(font), XtRString, XtDefaultFont},
#ifdef XAW_INTERNATIONALIZATION
  {XtNfontSet,  XtCFontSet, XtRFontSet, sizeof(XFontSet ),
     offset(fontset),XtRString, XtDefaultFontSet},
#endif
  {XtNmenuName, XtCMenuName, XtRString, sizeof(String),
     offset(menu_name), XtRImmediate, (XtPointer) NULL},
  {XtNunderline,  XtCIndex, XtRInt, sizeof(int),
     offset(underline), XtRImmediate, (XtPointer) -1},
};   
#undef offset

/*
 * Semi Public function definitions. 
 */

static void Redisplay(), Destroy(), Initialize();
static void Highlight(), Unhighlight();
static void ClassInitialize();
static Boolean SetValues();
static XtGeometryResult QueryGeometry();

/* 
 * Private Function Definitions.
 */

static void GetDefaultSize(), DrawBitmaps(), GetBitmapInfo();
static void CreateGCs(), DestroyGCs(), FlipColors();
    
#define superclass (&smeThreeDClassRec)
SmeBSBClassRec smeBSBClassRec = {
  {
    /* superclass         */    (WidgetClass) superclass,
    /* class_name         */    "SmeBSB",
    /* size               */    sizeof(SmeBSBRec),
    /* class_initializer  */	ClassInitialize,
    /* class_part_initialize*/	NULL,
    /* Class init'ed      */	FALSE,
    /* initialize         */    Initialize,
    /* initialize_hook    */	NULL,
    /* realize            */    NULL,
    /* actions            */    NULL,
    /* num_actions        */    ZERO,
    /* resources          */    resources,
    /* resource_count     */	XtNumber(resources),
    /* xrm_class          */    NULLQUARK,
    /* compress_motion    */    FALSE, 
    /* compress_exposure  */    FALSE,
    /* compress_enterleave*/ 	FALSE,
    /* visible_interest   */    FALSE,
    /* destroy            */    Destroy,
    /* resize             */    NULL,
    /* expose             */    Redisplay,
    /* set_values         */    SetValues,
    /* set_values_hook    */	NULL,
    /* set_values_almost  */	XtInheritSetValuesAlmost,  
    /* get_values_hook    */	NULL,			
    /* accept_focus       */    NULL,
    /* intrinsics version */	XtVersion,
    /* callback offsets   */    NULL,
    /* tm_table		  */    NULL,
    /* query_geometry	  */    QueryGeometry,
    /* display_accelerator*/    NULL,
    /* extension	  */    NULL
  },{
    /* SimpleMenuClass Fields */
    /* highlight          */	Highlight,
    /* unhighlight        */	Unhighlight,
    /* notify             */	XtInheritNotify,		
    /* extension	  */	NULL
  }, {
    /* ThreeDClass Fields */
    /* shadowdraw         */    XtInheritXawSme3dShadowDraw
  }, {
    /* BSBClass Fields */  
    /* extension	  */    NULL
  }
};

WidgetClass smeBSBObjectClass = (WidgetClass) &smeBSBClassRec;

/************************************************************
 *
 * Semi-Public Functions.
 *
 ************************************************************/

/*	Function Name: ClassInitialize
 *	Description: Initializes the SmeBSBObject. 
 *	Arguments: none.
 *	Returns: none.
 */

static void 
ClassInitialize()
{
    XawInitializeWidgetSet();
    XtAddConverter( XtRString, XtRJustify, XmuCvtStringToJustify, 
		    (XtConvertArgList)NULL, (Cardinal)0 );
}

/*      Function Name: Initialize
 *      Description: Initializes the simple menu widget
 *      Arguments: request - the widget requested by the argument list.
 *                 new     - the new widget with both resource and non
 *                           resource values.
 *      Returns: none.
 */

/* ARGSUSED */
static void
Initialize(request, new, args, num_args)
Widget request, new;
ArgList args;
Cardinal *num_args;
{
    SmeBSBObject entry = (SmeBSBObject) new;

    if (entry->sme_bsb.label == NULL) 
	entry->sme_bsb.label = XtName(new);
    else
	entry->sme_bsb.label = XtNewString( entry->sme_bsb.label );

    if (!entry->sme_bsb.font) XtError("Aborting: no font found\n");

    CreateGCs(new);

    GetBitmapInfo(new, TRUE);	/* Left Bitmap Info */
    GetBitmapInfo(new, FALSE);	/* Right Bitmap Info */

    entry->sme_bsb.left_stippled = entry->sme_bsb.right_stippled = None;

    GetDefaultSize(new, &(entry->rectangle.width), &(entry->rectangle.height));
}

/*      Function Name: Destroy
 *      Description: Called at destroy time, cleans up.
 *      Arguments: w - the simple menu widget.
 *      Returns: none.
 */

static void
Destroy(w)
Widget w;
{
    SmeBSBObject entry = (SmeBSBObject) w;

    DestroyGCs(w);
#ifdef XAW_MULTIPLANE_PIXMAPS
    if (entry->sme_bsb.left_stippled != None)
	XFreePixmap(XtDisplayOfObject(w), entry->sme_bsb.left_stippled);
    if (entry->sme_bsb.right_stippled != None)
	XFreePixmap(XtDisplayOfObject(w), entry->sme_bsb.right_stippled);
#endif
    if (entry->sme_bsb.label != XtName(w))
	XtFree(entry->sme_bsb.label);
}

/*      Function Name: Redisplay
 *      Description: Redisplays the contents of the widget.
 *      Arguments: w - the simple menu widget.
 *                 event - the X event that caused this redisplay.
 *                 region - the region the needs to be repainted. 
 *      Returns: none.
 */

/* ARGSUSED */
static void
Redisplay(w, event, region)
Widget w;
XEvent * event;
Region region;
{
    GC gc;
    SmeBSBObject entry = (SmeBSBObject) w;
    Dimension s = entry->sme_threeD.shadow_width;
    int	font_ascent = 0, font_descent = 0, y_loc;
#ifdef XAW_INTERNATIONALIZATION
    int	fontset_ascent = 0, fontset_descent = 0;
    XFontSetExtents *ext = XExtentsOfFontSet(entry->sme_bsb.fontset);
#endif

    entry->sme_bsb.set_values_area_cleared = FALSE;    
#ifdef XAW_INTERNATIONALIZATION
    if ( entry->sme.international == True ) {
        fontset_ascent = abs(ext->max_ink_extent.y);
        fontset_descent = ext->max_ink_extent.height - fontset_ascent;
    }
    else
#endif
    { /*else, compute size from font like R5*/
        font_ascent = entry->sme_bsb.font->max_bounds.ascent;
        font_descent = entry->sme_bsb.font->max_bounds.descent;
    }
    y_loc = entry->rectangle.y;

    if (XtIsSensitive(w) && XtIsSensitive( XtParent(w) ) ) {
	if ( w == XawSimpleMenuGetActiveEntry(XtParent(w)) ) {
	    XFillRectangle(XtDisplayOfObject(w), XtWindowOfObject(w), 
			   entry->sme_bsb.norm_gc, s, y_loc + s,
			   (unsigned int) entry->rectangle.width - 2 * s,
			   (unsigned int) entry->rectangle.height - 2 * s);
	    gc = entry->sme_bsb.rev_gc;
	}
	else
	    gc = entry->sme_bsb.norm_gc;
    }
    else
	gc = entry->sme_bsb.norm_gray_gc;
    
    if (entry->sme_bsb.label != NULL) {
	int x_loc = entry->sme_bsb.left_margin;
	int len = strlen(entry->sme_bsb.label);
	int width, t_width;
	char * label = entry->sme_bsb.label;

	switch(entry->sme_bsb.justify) {
	    case XtJustifyCenter:
#ifdef XAW_INTERNATIONALIZATION
		if ( entry->sme.international == True )
		    t_width = XmbTextEscapement(entry->sme_bsb.fontset,label,len);
		else
#endif
		    t_width = XTextWidth(entry->sme_bsb.font, label, len);

		width = entry->rectangle.width -
				(entry->sme_bsb.left_margin +
				entry->sme_bsb.right_margin);
		x_loc += (width - t_width)/2;
		break;
	    case XtJustifyRight:
#ifdef XAW_INTERNATIONALIZATION
		if ( entry->sme.international == True )
		    t_width = XmbTextEscapement(entry->sme_bsb.fontset,label,len);
		else
#endif
		    t_width = XTextWidth(entry->sme_bsb.font, label, len);

		x_loc = entry->rectangle.width -
				(entry->sme_bsb.right_margin + t_width);
		break;
	    case XtJustifyLeft:
	    default:
		break;
	}

	/* this will center the text in the gadget top-to-bottom */

#ifdef XAW_INTERNATIONALIZATION
        if ( entry->sme.international==True ) {
            y_loc += ((int)entry->rectangle.height - 
		  (fontset_ascent + fontset_descent)) / 2 + fontset_ascent;

            XmbDrawString(XtDisplayOfObject(w), XtWindowOfObject(w),
                entry->sme_bsb.fontset, gc, x_loc + s, y_loc, label, len);
        }
        else
#endif
        {
            y_loc += ((int)entry->rectangle.height - 
		  (font_ascent + font_descent)) / 2 + font_ascent;
	
            XDrawString(XtDisplayOfObject(w), XtWindowOfObject(w), gc,
		    x_loc + s, y_loc, label, len);
        }

	if (entry->sme_bsb.underline >= 0 && entry->sme_bsb.underline < len) {
	    int ul = entry->sme_bsb.underline;
	    int ul_x1_loc = x_loc + s;
	    int ul_wid;

	    if (ul != 0)
		ul_x1_loc += XTextWidth(entry->sme_bsb.font, label, ul);
	    ul_wid = XTextWidth(entry->sme_bsb.font, &label[ul], 1) - 2;
	    XDrawLine(XtDisplayOfObject(w), XtWindowOfObject(w), gc,
		      ul_x1_loc, y_loc + 1, ul_x1_loc + ul_wid, y_loc + 1);
	}
    }

    DrawBitmaps(w, gc);
}


/*      Function Name: SetValues
 *      Description: Relayout the menu when one of the resources is changed.
 *      Arguments: current - current state of the widget.
 *                 request - what was requested.
 *                 new - what the widget will become.
 *      Returns: none
 */

/* ARGSUSED */
static Boolean
SetValues(current, request, new, args, num_args)
Widget current, request, new;
ArgList args;
Cardinal *num_args;
{
    Widget parent = XtParent(new);
    SmeBSBObject entry = (SmeBSBObject) new;
    SmeBSBObject old_entry = (SmeBSBObject) current;
    Boolean ret_val = FALSE;

    if (old_entry->sme_bsb.label != entry->sme_bsb.label) {
        if (old_entry->sme_bsb.label != XtName( new ) )
	    XtFree( (char *) old_entry->sme_bsb.label );

	if (entry->sme_bsb.label != XtName(new) ) 
	    entry->sme_bsb.label = XtNewString( entry->sme_bsb.label );

	ret_val = TRUE;
    }

    if (entry->sme_bsb.underline != old_entry->sme_bsb.underline)
	ret_val = TRUE;

    if (entry->rectangle.sensitive != old_entry->rectangle.sensitive)
	ret_val = TRUE;

#ifdef XAW_INTERNATIONALIZATION
    if (  (	(old_entry->sme_bsb.font != entry->sme_bsb.font) &&
	(old_entry->sme.international == False )	          ) ||
	(old_entry->sme_bsb.foreground != entry->sme_bsb.foreground) ) {
#else
    if (  (old_entry->sme_bsb.font != entry->sme_bsb.font) ||
	(old_entry->sme_bsb.foreground != entry->sme_bsb.foreground) ) {
#endif
	DestroyGCs(current);
	CreateGCs(new);

	ret_val = TRUE;
    }

    if (entry->sme_bsb.left_bitmap != old_entry->sme_bsb.left_bitmap)
    {
	GetBitmapInfo(new, TRUE);

#ifdef XAW_MULTIPLANE_PIXMAPS
	entry->sme_bsb.left_stippled = None;
	if (old_entry->sme_bsb.left_stippled != None)
	    XFreePixmap(XtDisplayOfObject(current),
			old_entry->sme_bsb.left_stippled);
#endif

	ret_val = TRUE;
    }

    if (entry->sme_bsb.left_margin != old_entry->sme_bsb.left_margin)
	ret_val = TRUE;

    if (entry->sme_bsb.right_bitmap != old_entry->sme_bsb.right_bitmap)
    {
	GetBitmapInfo(new, FALSE);

#ifdef XAW_MULTIPLANE_PIXMAPS
	entry->sme_bsb.right_stippled = None;
	if (old_entry->sme_bsb.right_stippled != None)
	    XFreePixmap(XtDisplayOfObject(current),
			old_entry->sme_bsb.right_stippled);
#endif

	ret_val = TRUE;
    }

    if (entry->sme_bsb.right_margin != old_entry->sme_bsb.right_margin)
	ret_val = TRUE;

#ifdef XAW_INTERNATIONALIZATION
    if ( ( old_entry->sme_bsb.fontset != entry->sme_bsb.fontset) &&
				(old_entry->sme.international == True ) )
        /* don't change the GCs - the fontset is not in them */
        ret_val = TRUE;
#endif

    if (ret_val) {
	GetDefaultSize(new, 
		       &(entry->rectangle.width), &(entry->rectangle.height));
	entry->sme_bsb.set_values_area_cleared = TRUE;

	(parent->core.widget_class->core_class.resize)(new);
    }

    return(ret_val);
}

/*	Function Name: QueryGeometry.
 *	Description: Returns the preferred geometry for this widget.
 *	Arguments: w - the menu entry object.
 *                 itended, return_val - the intended and return geometry info.
 *	Returns: A Geometry Result.
 *
 * See the Intrinsics manual for details on what this function is for.
 * 
 * I just return the height and width of the label plus the margins.
 */

static XtGeometryResult
QueryGeometry(w, intended, return_val) 
Widget w;
XtWidgetGeometry *intended, *return_val;
{
    SmeBSBObject entry = (SmeBSBObject) w;
    Dimension width, height, h;
    XtGeometryResult ret_val = XtGeometryYes;
    XtGeometryMask mode = intended->request_mode;

    GetDefaultSize(w, &width, &height );

    if ( ((mode & CWWidth) && (intended->width != width)) ||
	 !(mode & CWWidth) ) {
	return_val->request_mode |= CWWidth;
	return_val->width = width;
	ret_val = XtGeometryAlmost;
    }

    if ( ((mode & CWHeight) && (intended->height != height)) ||
	 !(mode & CWHeight) ) {
	return_val->request_mode |= CWHeight;
	return_val->height = height;
	ret_val = XtGeometryAlmost;
    }

    if (ret_val == XtGeometryAlmost) {
	mode = return_val->request_mode;
	
	if ( ((mode & CWWidth) && (width == entry->rectangle.width)) &&
	     ((mode & CWHeight) && (height == entry->rectangle.height)) )
	    return(XtGeometryNo);
    }

    entry->rectangle.width = width;
    entry->rectangle.height = height;

    return(ret_val);
}
    
/*
 * FlipColors() used to be called directly, but it's blind
 * state toggling caused re-unhighlighting problems.
 */

static void
Highlight(w)
Widget w;
{
    SmeBSBObject entry = (SmeBSBObject) w;

    entry->sme_threeD.shadowed = True;
    FlipColors(w);
}

static void
Unhighlight(w)
Widget w;
{
    SmeBSBObject entry = (SmeBSBObject) w;

    entry->sme_threeD.shadowed = False;
    FlipColors(w);
}

/************************************************************
 *
 * Private Functions.
 *
 ************************************************************/

/*	Function Name: GetDefaultSize
 *	Description: Calculates the Default (preferred) size of
 *                   this menu entry.
 *	Arguments: w - the menu entry widget.
 *                 width, height - default sizes (RETURNED).
 *	Returns: none.
 */

static void
GetDefaultSize(w, width, height) 
Widget w;
Dimension * width, * height;
{
    SmeBSBObject entry = (SmeBSBObject) w;
    Dimension h;

#ifdef XAW_INTERNATIONALIZATION
    if ( entry->sme.international == True ) {
        XFontSetExtents *ext = XExtentsOfFontSet(entry->sme_bsb.fontset);

        if (entry->sme_bsb.label == NULL) 
	    *width = 0;
        else
	    *width = XmbTextEscapement(entry->sme_bsb.fontset, entry->sme_bsb.label,
			    strlen(entry->sme_bsb.label));

        *height = ext->max_ink_extent.height;
    }
    else
#endif
    {
        if (entry->sme_bsb.label == NULL) 
	    *width = 0;
        else
	    *width = XTextWidth(entry->sme_bsb.font, entry->sme_bsb.label,
			    strlen(entry->sme_bsb.label));

        *height = (entry->sme_bsb.font->max_bounds.ascent +
	       entry->sme_bsb.font->max_bounds.descent);
    }

    *width += entry->sme_bsb.left_margin + entry->sme_bsb.right_margin;
    *width += (2 * entry->sme_threeD.shadow_width);

    h = (entry->sme_bsb.left_bitmap_height > entry->sme_bsb.right_bitmap_height)
	    ? entry->sme_bsb.left_bitmap_height : entry->sme_bsb.right_bitmap_height;
    if (h > *height) *height = h;
    *height = ((int)*height * (100 + entry->sme_bsb.vert_space)) / 100;
    *height += (2 * entry->sme_threeD.shadow_width);
}

/*      Function Name: DrawBitmaps
 *      Description: Draws left and right bitmaps.
 *      Arguments: w - the simple menu widget.
 *                 gc - graphics context to use for drawing.
 *      Returns: none
 */

static void
DrawBitmaps(w, gc)
Widget w;
GC gc;
{
#ifdef XAW_MULTIPLANE_PIXMAPS
    Widget parent = XtParent(w);
#endif
    int x_loc, y_loc;
    SmeBSBObject entry = (SmeBSBObject) w;
    Pixmap pm;
    
    if ( (entry->sme_bsb.left_bitmap == None) && 
	 (entry->sme_bsb.right_bitmap == None) ) return;

/*
 * Draw Left Bitmap.
 */

  if (entry->sme_bsb.left_bitmap != None) {
    x_loc = entry->sme_threeD.shadow_width +
		(int)(entry->sme_bsb.left_margin -
		entry->sme_bsb.left_bitmap_width) / 2;

    y_loc = entry->rectangle.y + (int)(entry->rectangle.height -
		      entry->sme_bsb.left_bitmap_height) / 2;

    pm = entry->sme_bsb.left_bitmap;
#ifdef XAW_MULTIPLANE_PIXMAPS
    if (!XtIsSensitive(w)) {
	if (entry->sme_bsb.left_stippled == None)
	    entry->sme_bsb.left_stippled = stipplePixmap(w,
			entry->sme_bsb.left_bitmap,
			parent->core.colormap,
			parent->core.background_pixel,
			entry->sme_bsb.left_depth);
	if (entry->sme_bsb.left_stippled != None)
	    pm = entry->sme_bsb.left_stippled;
    }
#endif

    if (entry->sme_bsb.left_depth == 1)
	XCopyPlane(XtDisplayOfObject(w), pm,
		XtWindowOfObject(w), gc, 0, 0, 
		entry->sme_bsb.left_bitmap_width,
		entry->sme_bsb.left_bitmap_height, x_loc, y_loc, 1);
    else
	XCopyArea (XtDisplayOfObject(w), pm,
		XtWindowOfObject(w), gc, 0, 0, 
		entry->sme_bsb.left_bitmap_width,
		entry->sme_bsb.left_bitmap_height, x_loc, y_loc);
  }

/*
 * Draw Right Bitmap.
 */

  if (entry->sme_bsb.right_bitmap != None) {
    x_loc = entry->rectangle.width - entry->sme_threeD.shadow_width -
		(int)(entry->sme_bsb.right_margin +
		entry->sme_bsb.right_bitmap_width) / 2;

    y_loc = entry->rectangle.y + (int)(entry->rectangle.height -
		      entry->sme_bsb.right_bitmap_height) / 2;

     pm = entry->sme_bsb.right_bitmap;
#ifdef XAW_MULTIPLANE_PIXMAPS
    if (!XtIsSensitive(w)) {
	if (entry->sme_bsb.right_stippled == None)
	    entry->sme_bsb.right_stippled = stipplePixmap(w,
			entry->sme_bsb.right_bitmap,
			parent->core.colormap,
			parent->core.background_pixel,
			entry->sme_bsb.right_depth);
	if (entry->sme_bsb.right_stippled != None)
	    pm = entry->sme_bsb.right_stippled;
    }
#endif

    if (entry->sme_bsb.right_depth == 1)
	XCopyPlane(XtDisplayOfObject(w), pm,
		XtWindowOfObject(w), gc, 0, 0, 
		entry->sme_bsb.right_bitmap_width,
		entry->sme_bsb.right_bitmap_height, x_loc, y_loc, 1);
    else
	XCopyArea (XtDisplayOfObject(w), pm,
		XtWindowOfObject(w), gc, 0, 0, 
		entry->sme_bsb.right_bitmap_width,
		entry->sme_bsb.right_bitmap_height, x_loc, y_loc);
  }
}

/*      Function Name: GetBitmapInfo
 *      Description: Gets the bitmap information from either of the bitmaps.
 *      Arguments: w - the bsb menu entry widget.
 *                 is_left - TRUE if we are testing left bitmap,
 *                           FALSE if we are testing the right bitmap.
 *      Returns: none
 */

static void
GetBitmapInfo(w, is_left)
Widget w;
Boolean is_left;
{
    SmeBSBObject entry = (SmeBSBObject) w;    
    Window root;
    int x, y;
    unsigned int width, height, bw;
    char buf[BUFSIZ];
    char *pbuf;
    int len;
    
    if (is_left) {
	width = height = 0;

	if (entry->sme_bsb.left_bitmap != None) {
	    if (!XGetGeometry(XtDisplayOfObject(w), 
			    entry->sme_bsb.left_bitmap, &root, &x, &y,
			    &width, &height, &bw, &entry->sme_bsb.left_depth)) {
		char *err1 = "Xaw SmeBSB Object: Could not get Left Bitmap geometry information for menu entry ";
		len = strlen(err1) + strlen(XtName(w)) + 3 + 1;
		pbuf = XtStackAlloc(len, buf);
		if (pbuf == NULL) return;
		sprintf(pbuf, "%s\"%s\".", err1, XtName(w));
		XtAppError(XtWidgetToApplicationContext(w), pbuf);
		XtStackFree(pbuf, buf);
	    }
#ifdef NEVER
	    if (entry->sme_bsb.left_depth != 1) {
		char *err1 = "Xaw SmeBSB Object: Left Bitmap of entry ";
		char *err2 = " is not one bit deep.";
		len = strlen(err1) + strlen(err2) + strlen(XtName(w)) + 2 + 1;
		pbuf = XtStackAlloc(len, buf);
		if (pbuf == NULL) return;
		sprintf(pbuf, "%s\"%s\"%s", err1, XtName(w), err2);
		XtAppError(XtWidgetToApplicationContext(w), pbuf);
		XtStackFree(pbuf, buf);
	    }
#endif
	}

	entry->sme_bsb.left_bitmap_width = (Dimension) width; 
	entry->sme_bsb.left_bitmap_height = (Dimension) height;
    } else {
	width = height = 0;

	if (entry->sme_bsb.right_bitmap != None) {
	    if (!XGetGeometry(XtDisplayOfObject(w),
			    entry->sme_bsb.right_bitmap, &root, &x, &y,
			    &width, &height, &bw, &entry->sme_bsb.right_depth)) {
		char *err1 = "Xaw SmeBSB Object: Could not get Right Bitmap geometry information for menu entry ";
		len = strlen(err1) + strlen(XtName(w)) + 3 + 1;
		pbuf = XtStackAlloc(len, buf);
		if (pbuf == NULL) return;
		sprintf(pbuf, "%s\"%s\".", err1, XtName(w));
		XtAppError(XtWidgetToApplicationContext(w), pbuf);
		XtStackFree(pbuf, buf);
	    }
#ifdef NEVER
	    if (entry->sme_bsb.right_depth != 1) {
		char *err1 = "Xaw SmeBSB Object: Right Bitmap of entry ";
		char *err2 = " is not one bit deep.";
		len = strlen(err1) + strlen(err2) + strlen(XtName(w)) + 2 + 1;
		pbuf = XtStackAlloc(len, buf);
		if (pbuf == NULL) return;
		sprintf(pbuf, "%s\"%s\"%s", err1, XtName(w), err2);
		XtAppError(XtWidgetToApplicationContext(w), pbuf);
		XtStackFree(pbuf, buf);
	    }
#endif
	}

	entry->sme_bsb.right_bitmap_width = (Dimension) width; 
	entry->sme_bsb.right_bitmap_height = (Dimension) height;
    }
}      

/*      Function Name: CreateGCs
 *      Description: Creates all gc's for the simple menu widget.
 *      Arguments: w - the simple menu widget.
 *      Returns: none.
 */

static void
CreateGCs(w)
Widget w;
{
    SmeBSBObject entry = (SmeBSBObject) w;    
    XGCValues values;
    XtGCMask mask;
#ifdef XAW_INTERNATIONALIZATION
    XtGCMask mask_i18n;
#endif
    
    values.foreground = XtParent(w)->core.background_pixel;
    values.background = entry->sme_bsb.foreground;
    values.font = entry->sme_bsb.font->fid;
    values.graphics_exposures = FALSE;
    mask      = GCForeground | GCBackground | GCGraphicsExposures | GCFont;
#ifdef XAW_INTERNATIONALIZATION
    mask_i18n = GCForeground | GCBackground | GCGraphicsExposures;
    if ( entry->sme.international == True )
        entry->sme_bsb.rev_gc = XtAllocateGC(w, 0, mask_i18n, &values, GCFont, 0 );
    else
#endif
        entry->sme_bsb.rev_gc = XtGetGC(w, mask, &values);
    
    values.foreground = entry->sme_bsb.foreground;
    values.background = XtParent(w)->core.background_pixel;
#ifdef XAW_INTERNATIONALIZATION
    if ( entry->sme.international == True )
        entry->sme_bsb.norm_gc = XtAllocateGC(w, 0, mask_i18n, &values, GCFont, 0 );
    else
#endif
        entry->sme_bsb.norm_gc = XtGetGC(w, mask, &values);

    values.fill_style = FillTiled;
    values.tile   = XmuCreateStippledPixmap(XtScreenOfObject(w), 
					    entry->sme_bsb.foreground,
					    XtParent(w)->core.background_pixel,
					    XtParent(w)->core.depth);
    values.graphics_exposures = FALSE;
    mask |= GCTile | GCFillStyle;
#ifdef XAW_INTERNATIONALIZATION
    if ( entry->sme.international == True )
        entry->sme_bsb.norm_gray_gc = XtAllocateGC(w, 0, mask_i18n, &values, GCFont, 0 );
    else
#endif
        entry->sme_bsb.norm_gray_gc = XtGetGC(w, mask, &values);

    values.foreground ^= values.background;
    values.background = 0;
    values.function = GXxor;
    mask = GCForeground | GCBackground | GCGraphicsExposures | GCFunction;
    entry->sme_bsb.invert_gc = XtGetGC(w, mask, &values);
}

/*      Function Name: DestroyGCs
 *      Description: Removes all gc's for the simple menu widget.
 *      Arguments: w - the simple menu widget.
 *      Returns: none.
 */

static void
DestroyGCs(w)
Widget w;
{
    SmeBSBObject entry = (SmeBSBObject) w;    

    XtReleaseGC(w, entry->sme_bsb.norm_gc);
    XtReleaseGC(w, entry->sme_bsb.norm_gray_gc);
    XtReleaseGC(w, entry->sme_bsb.rev_gc);
    XtReleaseGC(w, entry->sme_bsb.invert_gc);
}

/*      Function Name: FlipColors
 *      Description: Invert the colors of the current entry.
 *      Arguments: w - the bsb menu entry widget.
 *      Returns: none.
 */

static void 
FlipColors(w)
Widget w;
{
    SmeBSBObject entry = (SmeBSBObject) w;
    SmeBSBObjectClass oclass = (SmeBSBObjectClass) XtClass (w);
    SimpleMenuWidget smw = (SimpleMenuWidget) XtParent (w);
    ThreeDWidget tdw = (ThreeDWidget) smw->simple_menu.threeD;
    Dimension s = tdw->threeD.shadow_width;

    if (entry->sme_bsb.set_values_area_cleared) {
	entry->sme_threeD.shadowed = False;
	return;
    }

    if (entry->sme_threeD.shadow_width > 0) {
	(*oclass->sme_threeD_class.shadowdraw) (w);
    } else
    	XFillRectangle(XtDisplayOfObject(w), XtWindowOfObject(w),
		   entry->sme_bsb.invert_gc, 
		   s, (int) entry->rectangle.y,
		   (unsigned int) entry->rectangle.width - 2 * s,
		   (unsigned int) entry->rectangle.height);
}

