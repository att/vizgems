/*
 * Copyright (c) 1999 by The XFree86 Project, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE XFREE86 PROJECT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of the XFree86 Project shall
 * not be used in advertising or otherwise to promote the sale, use or other
 * dealings in this Software without prior written authorization from the
 * XFree86 Project.
 *
 * Author: Paulo Csar Pereira de Andrade
 */

/*
 * Portions Copyright (c) 2003 David J. Hawkey Jr.
 * Rights, permissions, and disclaimer per the above XFree86 license.
 */

/* $XFree86: xc/lib/Xaw/Tip.c,v 1.5 2000/05/18 16:29:53 dawes Exp $ */

#include <ast.h>
#include "Xaw3dP.h"
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xos.h>
#include <X11/Xmu/Converters.h>
#include <TipP.h>
#include <XawInit.h>

#define	TIP_EVENT_MASK (ButtonPressMask	  |	\
			ButtonReleaseMask |	\
			PointerMotionMask |	\
			ButtonMotionMask  |	\
			KeyPressMask	  |	\
			KeyReleaseMask	  |	\
			EnterWindowMask	  |	\
			LeaveWindowMask)

/*
 * Types
 */
typedef struct _WidgetInfo {
    Widget widget;
    String label;
    struct _WidgetInfo *next;
} WidgetInfo;

typedef struct _XawTipInfo {
    Screen *screen;
    TipWidget tip;
    Bool mapped;
    WidgetInfo *widgets;
    struct _XawTipInfo *next;
} XawTipInfo;

typedef struct {
    XawTipInfo *info;
    WidgetInfo *winfo;
} TimeoutInfo;

/*
 * Class Methods
 */
static void XawTipClassInitialize();
static void XawTipInitialize();
static void XawTipDestroy();
static void XawTipExpose();
static void XawTipRealize();
static Boolean XawTipSetValues();

/*
 * Prototypes
 */
static void TipEventHandler();
static void TipShellEventHandler();
static WidgetInfo *CreateWidgetInfo();
static WidgetInfo *FindWidgetInfo();
static XawTipInfo *CreateTipInfo();
static XawTipInfo *FindTipInfo();
static void ResetTip();
static void TipTimeoutCallback();
static void TipLayout();
static void TipPosition();

/*
 * Initialization
 */
#define offset(field) XtOffsetOf(TipRec, tip.field)
static XtResource resources[] = {
  {XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel),
    offset(foreground), XtRString, XtDefaultForeground},
  {XtNfont, XtCFont, XtRFontStruct, sizeof(XFontStruct*),
    offset(font), XtRString, XtDefaultFont},
#ifdef XAW_INTERNATIONALIZATION
  {XtNfontSet, XtCFontSet, XtRFontSet, sizeof(XFontSet),
    offset(fontset), XtRString, XtDefaultFontSet},
#endif
  {XtNlabel, XtCLabel, XtRString, sizeof(String),
    offset(label), XtRString, NULL},
  {XtNencoding, XtCEncoding, XtRUnsignedChar, sizeof(unsigned char),
    offset(encoding), XtRImmediate, (XtPointer)XawTextEncoding8bit},
  {XtNinternalHeight, XtCHeight, XtRDimension, sizeof(Dimension),
    offset(internal_height), XtRImmediate, (XtPointer)2},
  {XtNinternalWidth, XtCWidth, XtRDimension, sizeof(Dimension),
    offset(internal_width), XtRImmediate, (XtPointer)2},
  {XtNbackingStore, XtCBackingStore, XtRBackingStore, sizeof(int),
    offset(backing_store), XtRImmediate,
    (XtPointer)(Always + WhenMapped + NotUseful)},
  {XtNtimeout, XtCTimeout, XtRInt, sizeof(int),
    offset(timeout), XtRImmediate, (XtPointer)500},
};
#undef offset

TipClassRec tipClassRec = {
  /* core */
  {
    (WidgetClass)&widgetClassRec,	/* superclass */
    "Tip",				/* class_name */
    sizeof(TipRec),			/* widget_size */
    XawTipClassInitialize,		/* class_initialize */
    NULL,				/* class_part_initialize */
    False,				/* class_inited */
    XawTipInitialize,			/* initialize */
    NULL,				/* initialize_hook */
    XawTipRealize,			/* realize */
    NULL,				/* actions */
    0,					/* num_actions */
    resources,				/* resources */
    XtNumber(resources),		/* num_resources */
    NULLQUARK,				/* xrm_class */
    True,				/* compress_motion */
    True,				/* compress_exposure */
    True,				/* compress_enterleave */
    False,				/* visible_interest */
    XawTipDestroy,			/* destroy */
    NULL,				/* resize */
    XawTipExpose,			/* expose */
    XawTipSetValues,			/* set_values */
    NULL,				/* set_values_hook */
    XtInheritSetValuesAlmost,		/* set_values_almost */
    NULL,				/* get_values_hook */
    NULL,				/* accept_focus */
    XtVersion,				/* version */
    NULL,				/* callback_private */
    NULL,				/* tm_table */
    XtInheritQueryGeometry,		/* query_geometry */
    XtInheritDisplayAccelerator,	/* display_accelerator */
    NULL,				/* extension */
  },
  /* tip */
  {
    NULL,				/* extension */
  },
};

WidgetClass tipWidgetClass = (WidgetClass)&tipClassRec;

static XawTipInfo *TipInfoList = NULL;
static TimeoutInfo TimeoutData;

/*
 * Implementation
 */

/*
 * XmuCvtBackingStoreToString() from XFree86's distribution, because
 * X.Org's distribution doesn't have it.
 */

/*ARGSUSED*/
static Boolean
XawCvtBackingStoreToString(dpy, args, num_args, fromVal, toVal, data)
Display *dpy;
XrmValuePtr args;
Cardinal *num_args;
XrmValuePtr fromVal, toVal;
XtPointer *data;
{
  static String buffer;
  Cardinal size;

  switch (*(int *)fromVal->addr)
  {
    case NotUseful:
      buffer = XtEnotUseful;
      break;
    case WhenMapped:
      buffer = XtEwhenMapped;
      break;
    case Always:
      buffer = XtEalways;
      break;
    case (Always + WhenMapped + NotUseful):
      buffer = XtEdefault;
      break;
    default:
      XtWarning("Cannot convert BackingStore to String");
      toVal->addr = NULL;
      toVal->size = 0;
      return (False);
  }

  size = strlen(buffer) + 1;
  if (toVal->addr != NULL)
  {
      if (toVal->size < size)
      {
	  toVal->size = size;
	  return (False);
      }
      strcpy((char *)toVal->addr, buffer);
  }
  else
    toVal->addr = (XPointer)buffer;
  toVal->size = sizeof(String);

  return (True);
}

static void
XawTipClassInitialize()
{
    XawInitializeWidgetSet();
    XtAddConverter(XtRString, XtRBackingStore, XmuCvtStringToBackingStore,
		   NULL, 0);
    XtSetTypeConverter(XtRBackingStore, XtRString, XawCvtBackingStoreToString,
		       NULL, 0, XtCacheNone, NULL);
}

/*ARGSUSED*/
static void
XawTipInitialize(req, w, args, num_args)
Widget req, w;
ArgList args;
Cardinal *num_args;
{
    TipWidget tip = (TipWidget)w;
    XGCValues values;

    if (!tip->tip.font) XtError("Aborting: no font found\n");
#ifdef XAW_INTERNATIONALIZATION
    if (tip->tip.international && !tip->tip.fontset)
	XtError("Aborting: no fontset found\n");
#endif

    tip->tip.timer = 0;

    values.foreground = tip->tip.foreground;
    values.background = tip->core.background_pixel;
    values.font = tip->tip.font->fid;
    values.graphics_exposures = False;

    tip->tip.gc = XtAllocateGC(w, 0, GCForeground | GCBackground | GCFont |
			       GCGraphicsExposures, &values, GCFont, 0);
}

static void
XawTipDestroy(w)
Widget w;
{
    XawTipInfo *info = FindTipInfo(w);
    WidgetInfo *winfo;
    TipWidget tip = (TipWidget)w;

    if (tip->tip.timer)
	XtRemoveTimeOut(tip->tip.timer);

    XtReleaseGC(w, tip->tip.gc);

    XtRemoveEventHandler(XtParent(w), KeyPressMask, False,
			 TipShellEventHandler, (XtPointer)NULL);

    while (info->widgets) {
	winfo = info->widgets->next;
	XtFree((char *)info->widgets->label);
	XtFree((char *)info->widgets);
	info->widgets = winfo;
    }

    if (info == TipInfoList)
	TipInfoList = TipInfoList->next;
    else {
	XawTipInfo *p = TipInfoList;

	while (p && p->next != info)
	    p = p->next;
	if (p)
	    p->next = info->next;
    }

    XtFree((char *)info);
}

static void
XawTipRealize(w, mask, attr)
Widget w;
Mask *mask;
XSetWindowAttributes *attr;
{
    TipWidget tip = (TipWidget)w;

    if (tip->tip.backing_store == Always ||
		tip->tip.backing_store == NotUseful ||
		tip->tip.backing_store == WhenMapped) {
	*mask |= CWBackingStore;
	attr->backing_store = tip->tip.backing_store;
    }
    else
	*mask &= ~CWBackingStore;
    *mask |= CWOverrideRedirect;
    attr->override_redirect = True;

    XtWindow(w) = XCreateWindow(DisplayOfScreen(XtScreen(w)),
				RootWindowOfScreen(XtScreen(w)),
				XtX(w), XtY(w),
				XtWidth(w) ? XtWidth(w) : 1,
				XtHeight(w) ? XtHeight(w) : 1,
				XtBorderWidth(w),
				DefaultDepthOfScreen(XtScreen(w)),
				InputOutput, CopyFromParent, *mask, attr);
}

static void
XawTipExpose(w, event, region)
Widget w;
XEvent *event;
Region region;
{
    TipWidget tip = (TipWidget)w;
    GC gc = tip->tip.gc;
    char *nl, *label = tip->tip.label;
    Position y = tip->tip.internal_height + tip->tip.font->max_bounds.ascent;
    int len;

#ifdef XAW_INTERNATIONALIZATION
    if (tip->tip.international == True) {
	Position ksy = tip->tip.internal_height;
	XFontSetExtents *ext;

        if (!tip->tip.fontset || !(ext = XExtentsOfFontSet(tip->tip.fontset)))
	   XtError("Aborting: no fontset found\n");

	ksy += abs(ext->max_ink_extent.y);

	while ((nl = (char *) index(label, '\n')) != NULL) {
	    XmbDrawString(XtDisplay(w), XtWindow(w), tip->tip.fontset,
			  gc, tip->tip.internal_width, ksy, label,
			  (int)(nl - label));
	    ksy += ext->max_ink_extent.height;
	    label = nl + 1;
	}
	len = strlen(label);
	if (len)
	    XmbDrawString(XtDisplay(w), XtWindow(w), tip->tip.fontset, gc,
			  tip->tip.internal_width, ksy, label, len);
    }
    else
#endif
    {
	while ((nl = (char *) index(label, '\n')) != NULL) {
	    if (tip->tip.encoding)
		XDrawString16(XtDisplay(w), XtWindow(w), gc,
			      tip->tip.internal_width, y,
			      (XChar2b*)label, (int)(nl - label) >> 1);
	    else
		XDrawString(XtDisplay(w), XtWindow(w), gc,
			    tip->tip.internal_width, y,
			    label, (int)(nl - label));
	    y += tip->tip.font->max_bounds.ascent + 
		 tip->tip.font->max_bounds.descent;
	    label = nl + 1;
	}
	len = strlen(label);
	if (len) {
	    if (tip->tip.encoding)
		XDrawString16(XtDisplay(w), XtWindow(w), gc,
			      tip->tip.internal_width, y,
			      (XChar2b*)label, len >> 1);
	    else
		XDrawString(XtDisplay(w), XtWindow(w), gc,
			    tip->tip.internal_width, y, label, len);
	}
    }
}

/*ARGSUSED*/
static Boolean
XawTipSetValues(current, request, cnew, args, num_args)
Widget current, request, cnew;
ArgList args;
Cardinal *num_args;
{
    TipWidget curtip = (TipWidget)current;
    TipWidget newtip = (TipWidget)cnew;
    Boolean redisplay = False;

    if (curtip->tip.font->fid != newtip->tip.font->fid ||
		curtip->tip.foreground != newtip->tip.foreground) {
	XGCValues values;

	values.foreground = newtip->tip.foreground;
	values.background = newtip->core.background_pixel;
	values.font = newtip->tip.font->fid;
	values.graphics_exposures = False;
	XtReleaseGC(cnew, curtip->tip.gc);
	newtip->tip.gc = XtAllocateGC(cnew, 0, GCForeground | GCBackground |
				      GCFont | GCGraphicsExposures, &values,
				      GCFont, 0);
	redisplay = True;
    }

    return (redisplay);
}

static void
TipLayout(info)
XawTipInfo *info;
{
    XFontStruct	*fs = info->tip->tip.font;
    int width = 0, height;
    char *nl, *label = info->tip->tip.label;

#ifdef XAW_INTERNATIONALIZATION
    if (info->tip->tip.international == True) {
	XFontSet fset = info->tip->tip.fontset;
	XFontSetExtents *ext;

	if (!fset || !(ext = XExtentsOfFontSet(fset)))
	   XtError("Aborting: no fontset found\n");

	height = ext->max_ink_extent.height;
	if ((nl = (char *) index(label, '\n')) != NULL) {
	    /*CONSTCOND*/
	    while (True) {
		int w = XmbTextEscapement(fset, label, (int)(nl - label));

		if (w > width)
		    width = w;
		if (*nl == '\0')
		    break;
		label = nl + 1;
		if (*label)
		    height += ext->max_ink_extent.height;
		if ((nl = (char *) index(label, '\n')) == NULL)
		    nl = (char *) index(label, '\0');
	    }
	}
	else
	    width = XmbTextEscapement(fset, label, strlen(label));
    }
    else
#endif
    {
	height = fs->max_bounds.ascent + fs->max_bounds.descent;
	if ((nl = (char *) index(label, '\n')) != NULL) {
	    /*CONSTCOND*/
	    while (True) {
		int w = info->tip->tip.encoding ?
			XTextWidth16(fs, (XChar2b*)label,
				     (int)(nl - label) >> 1) :
			XTextWidth(fs, label, (int)(nl - label));

		if (w > width)
		    width = w;
		if (*nl == '\0')
		    break;
		label = nl + 1;
		if (*label)
		    height += fs->max_bounds.ascent + fs->max_bounds.descent;
		if ((nl = (char *) index(label, '\n')) == NULL)
		    nl = (char *) index(label, '\0');
	    }
	}
	else
	    width = info->tip->tip.encoding ?
		    XTextWidth16(fs, (XChar2b*)label, strlen(label) >> 1) :
		    XTextWidth(fs, label, strlen(label));
    }
    XtWidth(info->tip) = width + info->tip->tip.internal_width * 2;
    XtHeight(info->tip) = height + info->tip->tip.internal_height * 2;
}

#define	DEFAULT_TIP_OFFSET	12

static void
TipPosition(info)
XawTipInfo *info;
{
    Window r, c;
    int rx, ry, wx, wy;
    unsigned mask;
    Position x, y;
    int bw2 = XtBorderWidth(info->tip) * 2;
    int scr_width = WidthOfScreen(XtScreen(info->tip));
    int scr_height = HeightOfScreen(XtScreen(info->tip));
    int win_width = XtWidth(info->tip) + bw2;
    int win_height = XtHeight(info->tip) + bw2;

    XQueryPointer(XtDisplay((Widget)info->tip), XtWindow((Widget)info->tip),
		  &r, &c, &rx, &ry, &wx, &wy, &mask);
    x = rx + DEFAULT_TIP_OFFSET;
    y = ry + DEFAULT_TIP_OFFSET;

    if (x + win_width > scr_width)
	x = scr_width - win_width;
    if (x < 0)
	x = 0;

    if (y + win_height > scr_height)
	y -= win_height + (DEFAULT_TIP_OFFSET << 1);
    if (y < 0)
	y = 0;

    XMoveResizeWindow(XtDisplay(info->tip), XtWindow(info->tip),
		      (int)(XtX(info->tip) = x), (int)(XtY(info->tip) = y),
		      (unsigned)XtWidth(info->tip),
		      (unsigned)XtHeight(info->tip));
}

static WidgetInfo *
CreateWidgetInfo(w)
Widget w;
{
    WidgetInfo *winfo = XtNew(WidgetInfo);

    winfo->widget = w;
    winfo->label = NULL;
    winfo->next = NULL;

    return (winfo);
}

static WidgetInfo *
FindWidgetInfo(info, w)
XawTipInfo *info;
Widget w;
{
    WidgetInfo *winfo, *wlist = info->widgets;

    if (wlist == NULL)
	return (info->widgets = CreateWidgetInfo(w));

    for (winfo = wlist; wlist; winfo = wlist, wlist = wlist->next)
	if (wlist->widget == w)
	    return (wlist);

    return (winfo->next = CreateWidgetInfo(w));
}

static XawTipInfo *
CreateTipInfo(w)
Widget w;
{
    XawTipInfo *info = XtNew(XawTipInfo);
    Widget shell = w;

    while (XtParent(shell))
	shell = XtParent(shell);

    info->tip = (TipWidget)XtCreateWidget("tip", tipWidgetClass,
					  shell, NULL, 0);
    XtRealizeWidget((Widget)info->tip);
    info->screen = XtScreen(w);
    info->mapped = False;
    info->widgets = NULL;
    info->next = NULL;
    XtAddEventHandler(shell, KeyPressMask, False, TipShellEventHandler,
		      (XtPointer)NULL);

    return (info);
}

static XawTipInfo *
FindTipInfo(w)
Widget w;
{
    XawTipInfo *info, *list = TipInfoList;
    Screen *screen;

    if (list == NULL)
	return (TipInfoList = CreateTipInfo(w));

    screen = XtScreen(w);
    for (info = list; list; info = list, list = list->next)
	if (list->screen == screen)
	    return (list);

    return (info->next = CreateTipInfo(w));
}

static void
ResetTip(info, winfo, add_timeout)
XawTipInfo *info;
WidgetInfo *winfo;
Bool add_timeout;
{
    if (info->tip->tip.timer) {
	XtRemoveTimeOut(info->tip->tip.timer);
	info->tip->tip.timer = 0;
    }
    if (info->mapped) {
	XtRemoveGrab(XtParent((Widget)info->tip));
	XUnmapWindow(XtDisplay((Widget)info->tip), XtWindow((Widget)info->tip));
	info->mapped = False;
    }
    if (add_timeout) {
	TimeoutData.info = info;
	TimeoutData.winfo = winfo;
	info->tip->tip.timer =
	    XtAppAddTimeOut(XtWidgetToApplicationContext((Widget)info->tip),
			    info->tip->tip.timeout, TipTimeoutCallback,
			    (XtPointer)&TimeoutData);
    }
}

static void
TipTimeoutCallback(closure, id)
XtPointer closure;
XtIntervalId *id;
{
    TimeoutInfo *cinfo = (TimeoutInfo *)closure;
    XawTipInfo *info = cinfo->info;
    WidgetInfo *winfo = cinfo->winfo;
    Arg args[2];

    info->tip->tip.label = winfo->label;
    info->tip->tip.encoding = 0;
    XtSetArg(args[0], XtNencoding, &info->tip->tip.encoding);
#ifdef XAW_INTERNATIONALIZATION
    info->tip->tip.international = False;
    XtSetArg(args[1], XtNinternational, &info->tip->tip.international);
#endif
    XtGetValues(winfo->widget, args, 2);

    TipLayout(info);
    TipPosition(info);
    XMapRaised(XtDisplay((Widget)info->tip), XtWindow((Widget)info->tip));
    XtAddGrab(XtParent((Widget)info->tip), True, True);
    info->mapped = True;
}

/*ARGSUSED*/
static void
TipShellEventHandler(w, client_data, event, continue_to_dispatch)
Widget w;
XtPointer client_data;
XEvent *event;
Boolean *continue_to_dispatch;
{
    XawTipInfo *info = FindTipInfo(w);

    ResetTip(info, FindWidgetInfo(info, w), False);
}

/*ARGSUSED*/
static void
TipEventHandler(w, client_data, event, continue_to_dispatch)
Widget w;
XtPointer client_data;
XEvent *event;
Boolean *continue_to_dispatch;
{
    XawTipInfo *info = FindTipInfo(w);
    Boolean add_timeout;

    switch (event->type) {
	case EnterNotify:
	    add_timeout = True;
	    break;
	case MotionNotify:
	    /* If any button is pressed, timer is 0 */
	    if (info->mapped)
		return;
	    add_timeout = info->tip->tip.timer != 0;
	    break;
	default:
	    add_timeout = False;
	    break;
    }
    ResetTip(info, FindWidgetInfo(info, w), add_timeout);
}

/*
 * Public routines
 */
void
XawTipEnable(w, label)
Widget w;
String label;
{
    if (XtIsWidget(w) && label && *label) {
	XawTipInfo *info = FindTipInfo(w);
	WidgetInfo *winfo = FindWidgetInfo(info, w);

	if (winfo->label)
	    XtFree((char *)winfo->label);
	winfo->label = XtNewString(label);

	XtAddEventHandler(w, TIP_EVENT_MASK, False, TipEventHandler,
			  (XtPointer)NULL);
    }
}

void
XawTipDisable(w)
Widget w;
{
    if (XtIsWidget(w)) {
	XawTipInfo *info = FindTipInfo(w);

	XtRemoveEventHandler(w, TIP_EVENT_MASK, False, TipEventHandler,
			     (XtPointer)NULL);
	ResetTip(info, FindWidgetInfo(info, w), False);
    }
}
