/*
 * Xaw3dP.c
 *
 * Global functions that don't really belong anywhere else.
 */

/*********************************************************************
Copyright (C) 1992 Kaleb Keithley
Copyright (C) 2000, 2003 David J. Hawkey Jr.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and
its documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and
that both that copyright notice and this permission notice appear in
supporting documentation, and that the names of the copyright holders
not be used in advertising or publicity pertaining to distribution
of the software without specific, written prior permission.

THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS. IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*********************************************************************/

#include <ast.h>
#include "Xaw3dP.h"
#ifdef XAW_MULTIPLANE_PIXMAPS
#include <stdio.h>
#include <X11/xpm.h>
#endif

#ifdef XAW_GRAY_BLKWHT_STIPPLES
/* ARGSUSED */
unsigned long
grayPixel(p, dpy, scn)
unsigned long p;	/* unused */
Display *dpy;
Screen	*scn;
{
    static XColor Gray =
    {
	0,		/* pixel */
	0, 0, 0,	/* red, green, blue */
        0,		/* flags */
        0		/* pad */
    };

    if (!Gray.pixel)
    {
	XColor exact;

	(void)XAllocNamedColor(dpy, DefaultColormapOfScreen(scn), 
			       "gray", &Gray, &exact);  /* Blindflug */
    }

    return Gray.pixel;
}
#endif

#ifdef XAW_MULTIPLANE_PIXMAPS
#define IS_EVEN(x)	(((x) % 2) == 0)
#define IS_ODD(x)	(((x) % 2) == 1)

/* ARGSUSED */
Pixmap
stipplePixmap(w, pm, cm, bg, d)
Widget w;
Pixmap pm;
Colormap cm;
Pixel bg;
unsigned int d;
{
    static Pixmap pixmap;
    Display *dpy;
    XpmImage image;
    XpmAttributes attr;
    XpmColor *src_table, *dst_table;
    int i, j, index = -1;

    if (pm == None)
	return (None);
    if (XtIsRealized(w) == False)
	return (None);

    dpy = XtDisplayOfObject(w);

    attr.colormap = cm;
    attr.closeness = 32768;	/* might help on 8-bpp displays? */
    attr.valuemask = XpmColormap | XpmCloseness;

    if (XpmCreateXpmImageFromPixmap(dpy, pm, None,
				    &image, &attr) != XpmSuccess)
	return (None);
    if (image.height == 0 || image.width == 0)
    {
	XpmFreeXpmImage(&image);
	return (None);
    }

    if (d > 1)
    {
	XColor x_color;
	XpmColor *dst_color;
	char dst_rgb[14];

	/*
	 * Multi-plane (XPM) pixmap. Don't bother scanning the color table
	 * for the background color, it might not be there. Copy the color
	 * table, add an entry for the background color, and set the index
	 * to that.
	 */

	x_color.pixel = bg;
	XQueryColor(dpy, cm, &x_color);
	sprintf(dst_rgb, "#%04X%04X%04X",
		x_color.red, x_color.green, x_color.blue);

	dst_table = (XpmColor *) XtCalloc(sizeof(XpmColor),
					  image.ncolors + 1);
	memcpy(dst_table, image.colorTable, image.ncolors * sizeof(XpmColor));

	dst_color = &dst_table[image.ncolors];
	switch (w->core.depth)
	{
	    case 1:
		dst_color->m_color = dst_rgb;
		break;
	    case 4:
		dst_color->g4_color = dst_rgb;
		break;
	    case 6:
		dst_color->g_color = dst_rgb;
		break;
	    case 8:
	    default:
		dst_color->c_color = dst_rgb;
		break;
	}
	dst_color->string = "\x01";	/* ! */

	src_table = image.colorTable;
	image.colorTable = dst_table;

	index = image.ncolors++;
    }
    else
    {
	XpmColor *src_color;
	char *src_rgb;

	/*
	 * Single-plane (XBM) pixmap. Set the index to the white color.
	 */

	for (i = 0, src_color = image.colorTable; i < image.ncolors;
		i++, src_color++)
	{
	    switch (w->core.depth)
	    {
		case 1:
		    src_rgb = src_color->m_color;
		    break;
		case 4:
		    src_rgb = src_color->g4_color;
		    break;
		case 6:
		    src_rgb = src_color->g_color;
		    break;
		case 8:
		default:
		    src_rgb = src_color->c_color;
		    break;
	    }
	    if (strcmp(src_rgb, "#000000000000") == 0)
	    {
		index = i;
		break;
	    }
	}

	if (index == -1)
	{
	    XpmFreeXpmImage(&image);
	    return (None);
	}
    }

    for (i = 0; i < image.height; i++)
	for (j = 0; j < image.width; j++)
	    if ((IS_ODD(i) && IS_EVEN(j)) || (IS_EVEN(i) && IS_ODD(j)))
		image.data[(i * image.width) + j] = index;

    attr.depth = d;
    attr.valuemask |= XpmDepth;

    i = XpmCreatePixmapFromXpmImage(dpy, pm, &image, &pixmap, NULL, &attr);

    if (d > 1)
    {
	XtFree((void *)image.colorTable);	/* dst_table */
	image.colorTable = src_table;
	image.ncolors--;
    }
    XpmFreeXpmImage(&image);

    return ((i == XpmSuccess) ? pixmap : None);
}
#endif
