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
 * Implements some of the Extended Window Manager Hints, as (poorly)
 * documented at
 * http://standards.freedesktop.org/wm-spec/wm-spec-1.3.html .
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

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "ewmh.h"
#include "twm.h"
#include "screen.h"
#include "events.h"

#define DEBUG_EWMH	1

static Atom MANAGER;
       Atom NET_CURRENT_DESKTOP;
static Atom NET_DESKTOP_GEOMETRY;
static Atom NET_DESKTOP_VIEWPORT;
static Atom NET_NUMBER_OF_DESKTOPS;
static Atom NET_SUPPORTED;
static Atom NET_SUPPORTING_WM_CHECK;
static Atom NET_VIRTUAL_ROOTS;
static Atom NET_WM_NAME;
static Atom UTF8_STRING;

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
    MANAGER           	= XInternAtom(dpy, "MANAGER", False);
    NET_CURRENT_DESKTOP = XInternAtom(dpy, "_NET_CURRENT_DESKTOP", False);
    NET_DESKTOP_GEOMETRY    = XInternAtom(dpy, "_NET_DESKTOP_GEOMETRY", False);
    NET_DESKTOP_VIEWPORT    = XInternAtom(dpy, "_NET_DESKTOP_VIEWPORT", False);
    NET_NUMBER_OF_DESKTOPS  = XInternAtom(dpy, "_NET_NUMBER_OF_DESKTOPS", False);
    NET_SUPPORTED     	= XInternAtom(dpy, "_NET_SUPPORTED", False);
    NET_SUPPORTING_WM_CHECK = XInternAtom(dpy, "_NET_SUPPORTING_WM_CHECK", False);
    NET_VIRTUAL_ROOTS   = XInternAtom(dpy, "_NET_VIRTUAL_ROOTS", False);
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
    int timeout = 200;		/* 0.2 seconds in ms */
    int found;

    if (lastTimestamp > 0) {
	return;
    }

    XChangeProperty(dpy, scr->icccm_Window,
                    XA_WM_CLASS, XA_STRING,
                    8, PropModeAppend, NULL, 0);

    while (timeout > 0) {
	found = XCheckTypedWindowEvent(dpy, scr->icccm_Window, PropertyNotify, &event);
	if (found) {
	    break;
	}
	usleep(10000);		/* sleep 10 ms */
	timeout -= 10;
    }

    if (found) {
#if DEBUG_EWMH
	fprintf(stderr, "GenerateTimestamp: time = %ld, timeout left = %d\n", event.xproperty.time, timeout);
#endif
	if (lastTimestamp < event.xproperty.time) {
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

#if DEBUG_EWMH
    fprintf(stderr, "EwmhReplaceWM: Atom %s = %d\n", atomname, (int)wmAtom);
#endif
    selectionOwner = XGetSelectionOwner(dpy, wmAtom);
    fprintf(stderr, "EwmhReplaceWM: selectionOwner = %x\n", (unsigned)selectionOwner);
    if (selectionOwner == scr->icccm_Window) {
	selectionOwner = None;
    }
#if DEBUG_EWMH
    fprintf(stderr, "EwmhReplaceWM: selectionOwner = %x\n", (unsigned)selectionOwner);
#endif

    if (selectionOwner != None) {
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

	if (caughtError) {
	    selectionOwner = None;
	}
    }

    if (selectionOwner != None) {
	if (!ewmh_replace) {
	    fprintf(stderr, "A window manager is already running on screen %d\n",
		    scr->screen);
	    return False;
	}
    }

#if DEBUG_EWMH
    fprintf(stderr, "EwmhReplaceWM: call XSetSelectionOwner\n");
#endif
    XSetSelectionOwner(dpy, wmAtom, scr->icccm_Window, CurrentTime);

    if (XGetSelectionOwner(dpy, wmAtom) != scr->icccm_Window) {
	fprintf(stderr, "Did not get window manager selection on screen %d\n",
		scr->screen);
	return False;
    }
#if DEBUG_EWMH
    fprintf(stderr, "EwmhReplaceWM: we are XSetSelectionOwner\n");
#endif

    /*
     * If there was a previous selection owner, wait for it
     * to go away.
     */

    if (selectionOwner != None) {
	int timeout = 10 * 1000;	/* 10 seconds in ms */
	XEvent event;

	while (timeout > 0) {

	    int found = XCheckTypedWindowEvent(dpy, selectionOwner, DestroyNotify, &event);
	    if (found) {
		break;
	    }
	    usleep(100000);		/* sleep 100 ms */
	    timeout -= 100;
	}

	if (timeout <= 0) {
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

#if DEBUG_EWMH
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

#if DEBUG_EWMH
    fprintf(stderr, "EwmhInitScreenEarly: call EwmhReplaceWM\n");
#endif
    if (!EwmhReplaceWM(scr)) {
	XDestroyWindow(dpy, scr->icccm_Window);
	scr->icccm_Window = None;

#if DEBUG_EWMH
	fprintf(stderr, "EwmhInitScreenEarly: return False\n");
#endif
	return False;
    }

#if DEBUG_EWMH
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

    if (scr->workSpaceManagerActive) {
	data[0] = scr->workSpaceMgr.count;
    } else {
	data[0] = 1;
    }

    XChangeProperty(dpy, scr->XineramaRoot,
			NET_NUMBER_OF_DESKTOPS, XA_CARDINAL,
			32, PropModeReplace,
			(unsigned char *)data, 1);

    if (scr->workSpaceManagerActive) {
	/* TODO: this is for the first Virtual Screen only... */
	//data[0] = scr->workSpaceMgr.workSpaceWindowList->currentwspc->number;
	data[0] = 0;
    } else {
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
void EwmhInitVirtualRoots(ScreenInfo *scr) { int numVscreens =
    scr->numVscreens;

    if (numVscreens > 1) {
	long *data;
	long d0;
	VirtualScreen *vs;
	int i;

	data = calloc(numVscreens, sizeof(long));

	for (vs = scr->vScreenList, i = 0;
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

    for (scrnum = 0; scrnum < NumScreens; scrnum++) {
	if ((scr = ScreenList[scrnum]) == NULL) {
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
#if DEBUG_EWMH
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
    if (msg->window != Scr->XineramaRoot &&
	msg->window != Scr->Root) {
	return False;
    }

    if (msg->message_type == NET_CURRENT_DESKTOP) {
	GotoWorkSpaceByNumber (Scr->currentvs, msg->data.l[0]);
	return True;
    }

    return False;
}
