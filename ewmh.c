/*
 *  [ ctwm ]
 *
 *  Copyright 2014 Olaf Seibert
 *
 * Permission to use, copy, modify and distribute this software [ctwm]
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of Olaf Seibert not be
 * used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission. Olaf Seibert
 * makes no representations about the suitability of this software for
 * any purpose. It is provided "as is" without express or implied
 * warranty.
 *
 * Olaf Seibert DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL Olaf Seibert BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE
 * USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Olaf Seibert [ rhialto@falu.nl ][ May 2014 ]
 */

/*
 * Implements some of the Extended Window Manager Hints, as (extremely
 * poorly) documented at
 * http://standards.freedesktop.org/wm-spec/wm-spec-1.3.html .
 * In fact, the wiki page that refers to that as being the current version
 * (http://www.freedesktop.org/wiki/Specifications/wm-spec/)
 * neglects to tell us there are newer versions 1.4 and 1.5 at
 * http://standards.freedesktop.org/wm-spec/wm-spec-1.5.html
 * (which has a listable directory that I discovered by accident).
 * The same wiki page also has lots of dead links to a CVS repository.
 * Nevertheless, it is where one ends up if one starts at
 * http://www.freedesktop.org/wiki/Specifications/ .
 *
 * EWMH is an extension to the ICCCM (Inter-Client Communication
 * Conventions Manual).
 * http://tronche.com/gui/x/icccm/
 *
 * To fill in lots of details, the source code of other window managers
 * has been consulted.
 */

#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include <assert.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "ewmh.h"
#include "twm.h"
#include "screen.h"
#include "events.h"
#include "icons.h"
#include "util.h"

#define DEBUG_EWMH      1

static Atom MANAGER;
Atom NET_CURRENT_DESKTOP;
static Atom NET_CLIENT_LIST;
static Atom NET_CLIENT_LIST_STACKING;
static Atom NET_DESKTOP_GEOMETRY;
static Atom NET_DESKTOP_VIEWPORT;
static Atom NET_NUMBER_OF_DESKTOPS;
static Atom NET_SUPPORTED;
static Atom NET_SUPPORTING_WM_CHECK;
static Atom NET_VIRTUAL_ROOTS;
static Atom NET_WM_DESKTOP;
static Atom NET_WM_ICON;
static Atom NET_WM_NAME;
static Atom UTF8_STRING;

static Image *ExtractIcon(ScreenInfo *scr, unsigned long *prop, int width,
                          int height);

static void SendPropertyMessage(Window to, Window about,
                                Atom messagetype,
                                long l0, long l1, long l2, long l3, long l4,
                                long mask)
{
	XEvent e;

	e.xclient.type = ClientMessage;
	e.xclient.message_type = messagetype;
	e.xclient.display = dpy;
	e.xclient.window = about;
	e.xclient.format = 32;
	e.xclient.data.l[0] = l0;
	e.xclient.data.l[1] = l1;
	e.xclient.data.l[2] = l2;
	e.xclient.data.l[3] = l3;
	e.xclient.data.l[4] = l4;

	XSendEvent(dpy, to, False, mask, &e);
}

static void EwmhInitAtoms(void)
{
	MANAGER             = XInternAtom(dpy, "MANAGER", False);
	NET_CLIENT_LIST     = XInternAtom(dpy, "_NET_CLIENT_LIST", False);
	NET_CLIENT_LIST_STACKING = XInternAtom(dpy, "_NET_CLIENT_LIST_STACKING", False);
	NET_CURRENT_DESKTOP = XInternAtom(dpy, "_NET_CURRENT_DESKTOP", False);
	NET_DESKTOP_GEOMETRY    = XInternAtom(dpy, "_NET_DESKTOP_GEOMETRY", False);
	NET_DESKTOP_VIEWPORT    = XInternAtom(dpy, "_NET_DESKTOP_VIEWPORT", False);
	NET_NUMBER_OF_DESKTOPS  = XInternAtom(dpy, "_NET_NUMBER_OF_DESKTOPS", False);
	NET_SUPPORTED       = XInternAtom(dpy, "_NET_SUPPORTED", False);
	NET_SUPPORTING_WM_CHECK = XInternAtom(dpy, "_NET_SUPPORTING_WM_CHECK", False);
	NET_VIRTUAL_ROOTS   = XInternAtom(dpy, "_NET_VIRTUAL_ROOTS", False);
	NET_WM_DESKTOP      = XInternAtom(dpy, "_NET_WM_DESKTOP", False);
	NET_WM_ICON         = XInternAtom(dpy, "_NET_WM_ICON", False);
	NET_WM_NAME         = XInternAtom(dpy, "_NET_WM_NAME", False);
	UTF8_STRING         = XInternAtom(dpy, "UTF8_STRING", False);
}

static int caughtError;

static int CatchError(Display *display, XErrorEvent *event)
{
	caughtError = True;
	return 0;
}

void EwmhInit(void)
{
	EwmhInitAtoms();
}

/*
 * Force-generate some event, so that we know the current time.
 *
 * Suggested in the ICCCM:
 * http://tronche.com/gui/x/icccm/sec-2.html#s-2.1
 */

static void GenerateTimestamp(ScreenInfo *scr)
{
	XEvent event;
	int timeout = 200;          /* 0.2 seconds in ms */
	int found;

	if(lastTimestamp > 0) {
		return;
	}

	XChangeProperty(dpy, scr->icccm_Window,
	                XA_WM_CLASS, XA_STRING,
	                8, PropModeAppend, NULL, 0);

	while(timeout > 0) {
		found = XCheckTypedWindowEvent(dpy, scr->icccm_Window, PropertyNotify, &event);
		if(found) {
			break;
		}
		usleep(10000);          /* sleep 10 ms */
		timeout -= 10;
	}

	if(found) {
#ifdef DEBUG_EWMH
		fprintf(stderr, "GenerateTimestamp: time = %ld, timeout left = %d\n",
		        event.xproperty.time, timeout);
#endif /* DEBUG_EWMH */
		if(lastTimestamp < event.xproperty.time) {
			lastTimestamp = event.xproperty.time;
		}
	}
}

/*
 * Perform the "replace the window manager" protocol, as vaguely hinted
 * at by the ICCCM section 4.3.
 * http://tronche.com/gui/x/icccm/sec-4.html#s-4.3
 *
 * TODO: convert the selection to atom VERSION.
 */
static int EwmhReplaceWM(ScreenInfo *scr)
{
	char atomname[32];
	Atom wmAtom;
	Window selectionOwner;

	snprintf(atomname, sizeof(atomname), "WM_S%d", scr->screen);
	wmAtom = XInternAtom(dpy, atomname, False);

	selectionOwner = XGetSelectionOwner(dpy, wmAtom);
	if(selectionOwner == scr->icccm_Window) {
		selectionOwner = None;
	}

	if(selectionOwner != None) {
		XErrorHandler oldHandler;

		/*
		 * Check if that owner still exists, and if it does, we want
		 * StructureNotify-kind events from it.
		 */
		caughtError = False;
		oldHandler = XSetErrorHandler(CatchError);

		XSelectInput(dpy, selectionOwner, StructureNotifyMask);
		XSync(dpy, False);

		XSetErrorHandler(oldHandler);

		if(caughtError) {
			selectionOwner = None;
		}
	}

	if(selectionOwner != None) {
		if(!ewmh_replace) {
			fprintf(stderr, "A window manager is already running on screen %d\n",
			        scr->screen);
			return False;
		}
	}

	XSetSelectionOwner(dpy, wmAtom, scr->icccm_Window, CurrentTime);

	if(XGetSelectionOwner(dpy, wmAtom) != scr->icccm_Window) {
		fprintf(stderr, "Did not get window manager selection on screen %d\n",
		        scr->screen);
		return False;
	}

	/*
	 * If there was a previous selection owner, wait for it
	 * to go away.
	 */

	if(selectionOwner != None) {
		int timeout = 10 * 1000;        /* 10 seconds in ms */
		XEvent event;

		while(timeout > 0) {

			int found = XCheckTypedWindowEvent(dpy, selectionOwner, DestroyNotify, &event);
			if(found) {
				break;
			}
			usleep(100000);             /* sleep 100 ms */
			timeout -= 100;
		}

		if(timeout <= 0) {
			fprintf(stderr, "Timed out waiting for other window manager "
			        "on screen %d to quit\n",
			        scr->screen);
			return False;
		}
	}

	/*
	 * Send a message to confirm we're now managing the screen.
	 * (Seen in OpenBox but not in docs)
	 */

	GenerateTimestamp(scr);

	SendPropertyMessage(scr->XineramaRoot, scr->XineramaRoot,
	                    MANAGER, lastTimestamp, wmAtom, scr->icccm_Window, 0, 0,
	                    SubstructureNotifyMask);

	return True;
}

/*
 * This function is called very early in initialisation.
 *
 * Only scr->screen and scr->XineramaRoot are valid: we want to know if
 * it makes sense to continue with the full initialisation.
 *
 * Create the ICCCM window that owns the WM_Sn selection.
 */
int EwmhInitScreenEarly(ScreenInfo *scr)
{
	XSetWindowAttributes attrib;

	scr->ewmh_CLIENT_LIST_used = 0;
	scr->ewmh_CLIENT_LIST_size = 16;
	scr->ewmh_CLIENT_LIST = malloc(scr->ewmh_CLIENT_LIST_size * sizeof(
	                                       scr->ewmh_CLIENT_LIST[0]));

#ifdef DEBUG_EWMH
	fprintf(stderr, "EwmhInitScreenEarly: XCreateWindow\n");
#endif
	attrib.event_mask = PropertyChangeMask;
	attrib.override_redirect = True;
	scr->icccm_Window = XCreateWindow(dpy, scr->XineramaRoot,
	                                  -100, -100, 1, 1, 0,
	                                  CopyFromParent, InputOutput,
	                                  CopyFromParent,
	                                  CWEventMask | CWOverrideRedirect,
	                                  &attrib);

	XMapWindow(dpy, scr->icccm_Window);
	XLowerWindow(dpy, scr->icccm_Window);

#ifdef DEBUG_EWMH
	fprintf(stderr, "EwmhInitScreenEarly: call EwmhReplaceWM\n");
#endif
	if(!EwmhReplaceWM(scr)) {
		XDestroyWindow(dpy, scr->icccm_Window);
		scr->icccm_Window = None;

#ifdef DEBUG_EWMH
		fprintf(stderr, "EwmhInitScreenEarly: return False\n");
#endif
		return False;
	}

#ifdef DEBUG_EWMH
	fprintf(stderr, "EwmhInitScreenEarly: return True\n");
#endif
	return True;
}

/*
 * This initialisation is called late, when scr has been set up
 * completely.
 */
void EwmhInitScreenLate(ScreenInfo *scr)
{
	long data[2];

	/* Set _NET_SUPPORTING_WM_CHECK on root window */
	data[0] = scr->icccm_Window;
	XChangeProperty(dpy, scr->XineramaRoot,
	                NET_SUPPORTING_WM_CHECK, XA_WINDOW,
	                32, PropModeReplace,
	                (unsigned char *)data, 1);

	/*
	 * Set properties on the window;
	 * this also belongs with _NET_SUPPORTING_WM_CHECK
	 */
	XChangeProperty(dpy, scr->icccm_Window,
	                NET_WM_NAME, UTF8_STRING,
	                8, PropModeReplace,
	                (unsigned char *)"ctwm", 4);

	data[0] = scr->icccm_Window;
	XChangeProperty(dpy, scr->icccm_Window,
	                NET_SUPPORTING_WM_CHECK, XA_WINDOW,
	                32, PropModeReplace,
	                (unsigned char *)data, 1);

	/*
	 * Add supported properties to the root window.
	 */
	data[0] = 0;
	data[1] = 0;
	XChangeProperty(dpy, scr->XineramaRoot,
	                NET_DESKTOP_VIEWPORT, XA_CARDINAL,
	                32, PropModeReplace,
	                (unsigned char *)data, 2);

	data[0] = scr->rootw;
	data[1] = scr->rooth;
	XChangeProperty(dpy, scr->XineramaRoot,
	                NET_DESKTOP_GEOMETRY, XA_CARDINAL,
	                32, PropModeReplace,
	                (unsigned char *)data, 2);

	if(scr->workSpaceManagerActive) {
		data[0] = scr->workSpaceMgr.count;
	}
	else {
		data[0] = 1;
	}

	XChangeProperty(dpy, scr->XineramaRoot,
	                NET_NUMBER_OF_DESKTOPS, XA_CARDINAL,
	                32, PropModeReplace,
	                (unsigned char *)data, 1);

	if(scr->workSpaceManagerActive) {
		/* TODO: this is for the first Virtual Screen only... */
		/*data[0] = scr->workSpaceMgr.workSpaceWindowList->currentwspc->number; */
		data[0] = 0;
	}
	else {
		data[0] = 0;
	}
	XChangeProperty(dpy, scr->XineramaRoot,
	                NET_CURRENT_DESKTOP, XA_CARDINAL,
	                32, PropModeReplace,
	                (unsigned char *)data, 1);

	long supported[10];
	int i = 0;

	supported[i++] = NET_SUPPORTING_WM_CHECK;
	supported[i++] = NET_DESKTOP_VIEWPORT;
	supported[i++] = NET_NUMBER_OF_DESKTOPS;
	supported[i++] = NET_CURRENT_DESKTOP;
	supported[i++] = NET_DESKTOP_GEOMETRY;
	supported[i++] = NET_WM_ICON;
	supported[i++] = NET_WM_DESKTOP;
	supported[i++] = NET_CLIENT_LIST;

	XChangeProperty(dpy, scr->XineramaRoot,
	                NET_SUPPORTED, XA_ATOM,
	                32, PropModeReplace,
	                (unsigned char *)supported, i);
}

/*
 * Set up the _NET_VIRTUAL_ROOTS property, which indicates that we're
 * using virtual root windows.
 * This applies only if we have multiple virtual screens.
 *
 * Also record this as a supported atom in _NET_SUPPORTED.
 *
 * Really, our virtual screens (with their virtual root windows) don't quite
 * fit in the EWMH idiom. Several root window properties (such as
 * _NET_CURRENT_DESKTOP) are more appropriate on the virtual root windows. But
 * that is not where other clients would look for them.
 *
 * The idea seems to be that the virtual roots as used for workspaces (desktops
 * in EWMH terminology) are only mapped one at a time.
 */
void EwmhInitVirtualRoots(ScreenInfo *scr)
{
	int numVscreens = scr->numVscreens;

	if(numVscreens > 1) {
		long *data;
		long d0;
		VirtualScreen *vs;
		int i;

		data = calloc(numVscreens, sizeof(long));

		for(vs = scr->vScreenList, i = 0;
		                vs != NULL && i < numVscreens;
		                vs = vs->next, i++) {
			data[i] = vs->window;
		}

		XChangeProperty(dpy, scr->XineramaRoot,
		                NET_VIRTUAL_ROOTS, XA_WINDOW,
		                32, PropModeReplace,
		                (unsigned char *)data, numVscreens);

		/* This might fail, but what can we do about it? */

		free(data);

		d0 = NET_VIRTUAL_ROOTS;
		XChangeProperty(dpy, scr->XineramaRoot,
		                NET_SUPPORTED, XA_ATOM,
		                32, PropModeAppend,
		                (unsigned char *)&d0, 1);
	}
}

static void EwmhTerminateScreen(ScreenInfo *scr)
{
	XDeleteProperty(dpy, scr->XineramaRoot, NET_SUPPORTED);

	/*
	 * Don't delete scr->icccm_Window; let it be deleted automatically
	 * when we terminate the X server connection. A replacement window
	 * manager may want to start working immediately after it has
	 * disappeared.
	 */
}

/*
 * Clear everything that needs to be cleared before we exit.
 */

void EwmhTerminate(void)
{
	int scrnum;
	ScreenInfo *scr;

	for(scrnum = 0; scrnum < NumScreens; scrnum++) {
		if((scr = ScreenList[scrnum]) == NULL) {
			continue;
		}
		EwmhTerminateScreen(scr);
	}
}

/*
 * Event handler: lost the WM_Sn selection
 * (that's the only selection we have).
 */

void EwhmSelectionClear(XSelectionClearEvent *sev)
{
#ifdef DEBUG_EWMH
	fprintf(stderr, "sev->window = %x\n", (unsigned)sev->window);
#endif
	Done(0);
}

/*
 * When accepting client messages to the root window,
 * be accepting and accept both the real root window and the
 * current virtual screen.
 *
 * Should perhaps also accept any other virtual screen.
 */
int EwmhClientMessage(XClientMessageEvent *msg)
{
	if(msg->window != Scr->XineramaRoot &&
	                msg->window != Scr->Root) {
		return False;
	}

	if(msg->message_type == NET_CURRENT_DESKTOP) {
		GotoWorkSpaceByNumber(Scr->currentvs, msg->data.l[0]);
		return True;
	}

	return False;
}

/*
 * The format of the _NET_WM_ICON property is
 *
 * [0] width
 * [1] height
 *     height repetitions of
 *         row, which is
 *              width repetitions of
 *                      pixel: ARGB
 * repeat for next size.
 *
 * Some icons can be 256x256 CARDINALs which is 65536 CARDINALS!
 * Therefore we fetch in pieces and skip the pixels of large icons
 * until needed.
 *
 * First scan all sizes. Keep a record of the closest smaller and larger
 * size. At the end, choose from one of those.
 * FInally, go and fetch the pixel data.
 */

Image *EwhmGetIcon(ScreenInfo *scr, TwmWindow *twm_win)
{
	int fetch_offset;
	Atom actual_type;
	int actual_format;
	unsigned long nitems, bytes_after;
	unsigned long *prop;

	int wanted_area;
	int smaller, larger;
	int offset;
	int smaller_offset, larger_offset;
	int i;

	int area, width, height;

	fetch_offset = 0;
	if(XGetWindowProperty(dpy, twm_win->w, NET_WM_ICON,
	                      fetch_offset, 8 * 1024, False, XA_CARDINAL,
	                      &actual_type, &actual_format, &nitems,
	                      &bytes_after, (unsigned char **)&prop) != Success || nitems == 0) {
		return NULL;
	}

	if(actual_format != 32) {
		XFree(prop);
		return NULL;
	}

#ifdef DEBUG_EWMH
	fprintf(stderr, "_NET_WM_ICON data fetched\n");
#endif
	/*
	 * Usually the icons are square, but that is not a rule.
	 * So we measure the area instead.
	 *
	 * Approach wanted size from both directions and at the end,
	 * choose the "nearest".
	 */
	wanted_area = Scr->PreferredIconWidth * Scr->PreferredIconHeight;
	smaller = 0;
	larger = 999999;
	offset = 0;
	smaller_offset = -1;
	larger_offset = -1;
	i = 0;

	for(;;) {
		offset = i;

		int w = prop[i++];
		int h = prop[i++];
		int size = w * h;

		area = w * h;

#ifdef DEBUG_EWMH
		fprintf(stderr, "[%d+%d] w=%d h=%d\n", fetch_offset, offset, w, h);
#endif


		if(area == wanted_area) {
#ifdef DEBUG_EWMH
			fprintf(stderr, "exact match [%d+%d=%d] w=%d h=%d\n", fetch_offset, offset,
			        fetch_offset + offset, w, h);
#endif /* DEBUG_EWMH */
			smaller_offset = fetch_offset + offset;
			smaller = area;
			larger_offset = -1;
			break;
		}
		else if(area < wanted_area) {
			if(area > smaller) {
#ifdef DEBUG_EWMH
				fprintf(stderr, "increase smaller, was [%d]\n", smaller_offset);
#endif /* DEBUG_EWMH */
				smaller = area;
				smaller_offset = fetch_offset + offset;
			}
		}
		else {   /* area > wanted_area */
			if(area < larger) {
#ifdef DEBUG_EWMH
				fprintf(stderr, "decrease larger, was [%d]\n", larger_offset);
#endif /* DEBUG_EWMH */
				larger = area;
				larger_offset = fetch_offset + offset;
			}
		}

		if(i + size + 2 > nitems) {
#ifdef DEBUG_EWMH
			fprintf(stderr, "not enough data: %d + %d > %ld \n", i, size, nitems);
#endif /* DEBUG_EWMH */

			if(i + size + 2 <= nitems + bytes_after / 4) {
				/* we can fetch some more... */
				XFree(prop);
				fetch_offset += i + size;
				if(XGetWindowProperty(dpy, twm_win->w, NET_WM_ICON,
				                      fetch_offset, 8 * 1024, False, XA_CARDINAL,
				                      &actual_type, &actual_format, &nitems,
				                      &bytes_after, (unsigned char **)&prop) != Success) {
					continue;
				}
				i = 0;
				continue;
			}
			break;
		}
		i += size;
	}

	/*
	 * Choose which icon approximates our desired size best.
	 */
	area = 0;

	if(smaller_offset >= 0) {
		if(larger_offset >= 0) {
			/* choose the nearest */
#ifdef DEBUG_EWMH
			fprintf(stderr, "choose nearest %d %d\n", smaller, larger);
#endif /* DEBUG_EWMH */
			if((double)larger / wanted_area > (double)wanted_area / smaller) {
				offset = smaller_offset;
				area = smaller;
			}
			else {
				offset = larger_offset;
				area = larger;
			}
		}
		else {
			/* choose smaller */
#ifdef DEBUG_EWMH
			fprintf(stderr, "choose smaller (only) %d\n", smaller);
#endif /* DEBUG_EWMH */
			offset = smaller_offset;
			area = smaller;
		}
	}
	else if(larger_offset >= 0) {
		/* choose larger */
#ifdef DEBUG_EWMH
		fprintf(stderr, "choose larger (only) %d\n", larger);
#endif /* DEBUG_EWMH */
		offset = larger_offset;
		area = larger;
	}
	else {
		/* no icons found at all? */
#ifdef DEBUG_EWMH
		fprintf(stderr, "nothing to choose from\n");
#endif /* DEBUG_EWMH */
		XFree(prop);
		return NULL;
	}

	/*
	 * Now fetch the pixels.
	 */

#ifdef DEBUG_EWMH
	fprintf(stderr, "offset = %d fetch_offset = %d\n", offset, fetch_offset);
	fprintf(stderr, "offset + 2 + area = %d fetch_offset + nitems = %ld\n",
	        offset + 2 + area, fetch_offset + nitems);
#endif /* DEBUG_EWMH */
	if(offset < fetch_offset ||
	                offset + 2 + area > fetch_offset + nitems) {
		XFree(prop);
		fetch_offset = offset;
#ifdef DEBUG_EWMH
		fprintf(stderr, "refetching from %d\n", fetch_offset);
#endif /* DEBUG_EWMH */
		if(XGetWindowProperty(dpy, twm_win->w, NET_WM_ICON,
		                      fetch_offset, 2 + area, False, XA_CARDINAL,
		                      &actual_type, &actual_format, &nitems,
		                      &bytes_after, (unsigned char **)&prop) != Success) {
			return NULL;
		}
	}

	i = offset - fetch_offset;
	width = prop[i++];
	height = prop[i++];
#ifdef DEBUG_EWMH
	fprintf(stderr, "Chosen [%d] w=%d h=%d area=%d\n", offset, width, height, area);
#endif /* DEBUG_EWMH */
	assert(width * height == area);

	Image *image = ExtractIcon(scr, &prop[i], width, height);

	XFree(prop);

	return image;
}

static uint16_t *buffer_16bpp;
static uint32_t *buffer_32bpp;

static void convert_for_16(int w, int x, int y, int argb)
{
	int r = (argb >> 16) & 0xFF;
	int g = (argb >>  8) & 0xFF;
	int b = (argb >>  0) & 0xFF;
	buffer_16bpp [y * w + x] = ((r >> 3) << 11) + ((g >> 2) << 5) + (b >> 3);
}

static void convert_for_32(int w, int x, int y, int argb)
{
	buffer_32bpp [y * w + x] = argb & 0x00FFFFFF;
}

static Image *ExtractIcon(ScreenInfo *scr, unsigned long *prop, int width,
                          int height)
{
	XImage *ximage;
	void (*store_data)(int w, int x, int y, int argb);
	int x, y, transparency;
	int rowbytes;
	unsigned char *maskbits;

	GC gc;
	Pixmap pixret;
	Pixmap mask;
	Image *image;
	int i;

	ximage = NULL;

	/** XXX sort of duplicated from util.c:LoadJpegImage() */
	if(scr->d_depth == 16) {
		store_data = convert_for_16;
		buffer_16bpp = (uint16_t *) malloc(width * height * 2);
		buffer_32bpp = NULL;
		ximage = XCreateImage(dpy, CopyFromParent, scr->d_depth, ZPixmap, 0,
		                      (char *) buffer_16bpp, width, height, 16, width * 2);
	}
	else if(scr->d_depth == 24 || scr->d_depth == 32) {
		store_data = convert_for_32;
		buffer_32bpp = malloc(width * height * sizeof(buffer_32bpp[0]));
		buffer_16bpp = NULL;
		ximage = XCreateImage(dpy, CopyFromParent, scr->d_depth, ZPixmap, 0,
		                      (char *) buffer_32bpp, width, height, 32, width * 4);
	}
	else {
#ifdef DEBUG_EWMH
		fprintf(stderr, "Screen unsupported depth for 32-bit icon: %d\n", scr->d_depth);
#endif /* DEBUG_EWMH */
		XFree(prop);
		return NULL;
	}
	if(ximage == NULL) {
#ifdef DEBUG_EWMH
		fprintf(stderr, "cannot create image for icon\n");
#endif /* DEBUG_EWMH */
		XFree(prop);
		return NULL;
	}

	transparency = 0;
	rowbytes = (width + 7) / 8;
	maskbits = (unsigned char *) calloc(height, rowbytes);

	/*
	 * Copy all ARGB pixels to the pixmap (the RGB part), and the bitmap (the
	 * Alpha, or opaqueness part). If any pixels are transparent, we're going
	 * to need a shape.
	 */
	i = 0;
	for(y = 0; y < height; y++) {
		for(x = 0; x < width; x++) {
			unsigned long argb = prop[i++];
			store_data(width, x, y, argb);
			int opaque = ((argb >> 24) & 0xFF) >= 0x80; /* arbitrary cutoff */
			if(opaque) {
				maskbits [rowbytes * y + (x / 8)] |= 0x01 << (x % 8);
			}
			else {
				transparency = 1;
			}
		}
	}

	gc = DefaultGC(dpy, scr->screen);
	pixret = XCreatePixmap(dpy, scr->Root, width, height, scr->d_depth);
	XPutImage(dpy, pixret, gc, ximage, 0, 0, 0, 0, width, height);
	XDestroyImage(ximage);  /* also frees buffer_{16,32}bpp */
	ximage = NULL;

	mask = None;
	if(transparency) {
		mask = XCreatePixmapFromBitmapData(dpy, scr->Root, (char *)maskbits,
		                                   width, height, 1, 0, 1);
	}
	free(maskbits);

	image = calloc(1, sizeof(Image));

	image->width  = width;
	image->height = height;
	image->pixmap = pixret;
	image->mask   = mask;
	image->next   = None;

	return image;
}

int EwmhHandlePropertyNotify(XPropertyEvent *event, TwmWindow *twm_win)
{
	if(event->atom == NET_WM_ICON) {
		unsigned long valuemask;                /* mask for create windows */
		XSetWindowAttributes attributes;        /* attributes for create windows */
		Icon *icon = twm_win->icon;
		int x;

#ifdef DEBUG_EWMH
		fprintf(stderr, "EwmhHandlePropertyNotify: NET_WM_ICON\n");
#endif /* DEBUG_EWMH */
		/*
		 * If there is no icon yet, we'll look at this property
		 * later, if and when we do create an icon.
		 */
		if(!icon || icon->match != match_net_wm_icon) {
#ifdef DEBUG_EWMH
			fprintf(stderr, "no icon, or not match_net_wm_icon\n");
#endif /* DEBUG_EWMH */
			return 1;
		}

		Image *image = EwhmGetIcon(Scr, twm_win);

		/* TODO: de-duplicate with handling of XA_WM_HINTS */
		{
			Image *old_image = icon->image;
			icon->image = image;
			FreeImage(old_image);
		}


		if(twm_win->icon->bm_w) {
			XDestroyWindow(dpy, twm_win->icon->bm_w);
		}

		valuemask = CWBackPixmap;
		attributes.background_pixmap = image->pixmap;

		x = GetIconOffset(twm_win->icon);
		twm_win->icon->bm_w =
		        XCreateWindow(dpy, twm_win->icon->w, x, 0,
		                      (unsigned int) twm_win->icon->width,
		                      (unsigned int) twm_win->icon->height,
		                      (unsigned int) 0, Scr->d_depth,
		                      (unsigned int) CopyFromParent, Scr->d_visual,
		                      valuemask, &attributes);

		if(image->mask) {
			XShapeCombineMask(dpy, twm_win->icon->bm_w, ShapeBounding, 0, 0, image->mask,
			                  ShapeSet);
			XShapeCombineMask(dpy, twm_win->icon->w,    ShapeBounding, x, 0, image->mask,
			                  ShapeSet);
		}
		else {
			XRectangle rect;

			rect.x      = x;
			rect.y      = 0;
			rect.width  = twm_win->icon->width;
			rect.height = twm_win->icon->height;
			XShapeCombineRectangles(dpy, twm_win->icon->w, ShapeBounding, 0,
			                        0, &rect, 1, ShapeUnion, 0);
		}
		XMapSubwindows(dpy, twm_win->icon->w);
		RedoIconName();

		return 1;
	}
	return 0;
}

void EwmhSet_NET_WM_DESKTOP(TwmWindow *twm_win)
{
	WorkSpace *ws;

	VirtualScreen *vs = twm_win->vs;
	if(vs != NULL) {
		ws = vs->wsw->currentwspc;
	}
	else {
		ws = NULL;
	}

	EwmhSet_NET_WM_DESKTOP_ws(twm_win, ws);
}

void EwmhSet_NET_WM_DESKTOP_ws(TwmWindow *twm_win, WorkSpace *ws)
{
	long workspaces[MAXWORKSPACE];
	int n = 0;

	if(!Scr->workSpaceManagerActive) {
		workspaces[n++] = 0;
	}
	else if(twm_win->occupation == fullOccupation) {
		workspaces[n++] = 0xFFFFFFFF;
	}
	else {
		/*
		 * Our windows can occupy multiple workspaces ("virtual desktops" in
		 * EWMH terminology) at once. Extend the _NET_WM_DESKTOP property
		 * by setting it to multiple CARDINALs if this occurs.
		 * Put the currently visible workspace (if any) first.
		 */
		int occupation = twm_win->occupation;

		/*
		 * Set visible workspace number.
		 */
		if(ws != NULL) {
			int wsn = ws->number;

			workspaces[n++] = wsn;
			occupation &= ~(1 << wsn);
		}

		/*
		 * Set any other workspace numbers.
		 */
		if(occupation != 0) {
			int i;
			int mask = 1;

			for(i = 0; i < MAXWORKSPACE; i++) {
				if(occupation & mask) {
					workspaces[n++] = i;
				}
				mask <<= 1;
			}
		}
	}

	XChangeProperty(dpy, twm_win->w,
	                NET_WM_DESKTOP, XA_CARDINAL,
	                32, PropModeReplace,
	                (unsigned char *)workspaces, n);
}

/*
 * Add a new window to _NET_CLIENT_LIST.
 * Newer windows are always added at the end.
 *
 * Look at new_win->iconmanagerlist as an optimization for
 * !LookInList(Scr->IconMgrNoShow, new_win->full_name, &new_win->class)).
 */
void EwmhAddClientWindow(TwmWindow *new_win)
{
	if(Scr->ewmh_CLIENT_LIST_size == 0) {
		return;
	}
	if(new_win->iconmanagerlist != NULL &&
	                !new_win->wspmgr &&
	                !new_win->iconmgr) {
		Scr->ewmh_CLIENT_LIST_used++;
		if(Scr->ewmh_CLIENT_LIST_used > Scr->ewmh_CLIENT_LIST_size) {
			Scr->ewmh_CLIENT_LIST_size *= 2;
			Scr->ewmh_CLIENT_LIST = realloc(Scr->ewmh_CLIENT_LIST,
			                                sizeof(long) * Scr->ewmh_CLIENT_LIST_size);
		}
		if(Scr->ewmh_CLIENT_LIST) {
			Scr->ewmh_CLIENT_LIST[Scr->ewmh_CLIENT_LIST_used - 1] = new_win->w;
		}
		else {
			Scr->ewmh_CLIENT_LIST_size = 0;
			fprintf(stderr, "Unable to allocate memory for GNOME client list.\n");
			return;
		}
		XChangeProperty(dpy, Scr->Root, NET_CLIENT_LIST, XA_WINDOW, 32,
		                PropModeReplace, (unsigned char *)Scr->ewmh_CLIENT_LIST,
		                Scr->ewmh_CLIENT_LIST_used);
	}
}

void EwmhDeleteClientWindow(TwmWindow *old_win)
{
	int i;

	if(Scr->ewmh_CLIENT_LIST_size == 0) {
		return;
	}
	for(i = Scr->ewmh_CLIENT_LIST_used - 1; i >= 0; i--) {
		if(Scr->ewmh_CLIENT_LIST[i] == old_win->w) {
			Scr->ewmh_CLIENT_LIST_used--;
			memmove(&Scr->ewmh_CLIENT_LIST[i],
			        &Scr->ewmh_CLIENT_LIST[i + 1],
			        (Scr->ewmh_CLIENT_LIST_used - 1 - i) * sizeof(Scr->ewmh_CLIENT_LIST[0]));
			if(Scr->ewmh_CLIENT_LIST_used &&
			                (Scr->ewmh_CLIENT_LIST_used * 3) < Scr->ewmh_CLIENT_LIST_size) {
				Scr->ewmh_CLIENT_LIST_size /= 2;
				Scr->ewmh_CLIENT_LIST = realloc(Scr->ewmh_CLIENT_LIST,
				                                sizeof((Scr->ewmh_CLIENT_LIST[0])) * Scr->ewmh_CLIENT_LIST_size);
				/* memory shrinking, shouldn't have problems */
			}
			break;
		}
	}
	/* If window was not found, there is no need to update the property. */
	if(i >= 0) {
		XChangeProperty(dpy, Scr->Root, NET_CLIENT_LIST, XA_WINDOW, 32,
		                PropModeReplace, (unsigned char *)Scr->ewmh_CLIENT_LIST,
		                Scr->ewmh_CLIENT_LIST_used);
	}
}
