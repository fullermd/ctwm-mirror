/*
 * Core event handling
 */
/*****************************************************************************/
/**       Copyright 1988 by Evans & Sutherland Computer Corporation,        **/
/**                          Salt Lake City, Utah                           **/
/**  Portions Copyright 1989 by the Massachusetts Institute of Technology   **/
/**                        Cambridge, Massachusetts                         **/
/**                                                                         **/
/**                           All Rights Reserved                           **/
/**                                                                         **/
/**    Permission to use, copy, modify, and distribute this software and    **/
/**    its documentation  for  any  purpose  and  without  fee is hereby    **/
/**    granted, provided that the above copyright notice appear  in  all    **/
/**    copies and that both  that  copyright  notice  and  this  permis-    **/
/**    sion  notice appear in supporting  documentation,  and  that  the    **/
/**    names of Evans & Sutherland and M.I.T. not be used in advertising    **/
/**    in publicity pertaining to distribution of the  software  without    **/
/**    specific, written prior permission.                                  **/
/**                                                                         **/
/**    EVANS & SUTHERLAND AND M.I.T. DISCLAIM ALL WARRANTIES WITH REGARD    **/
/**    TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES  OF  MERCHANT-    **/
/**    ABILITY  AND  FITNESS,  IN  NO  EVENT SHALL EVANS & SUTHERLAND OR    **/
/**    M.I.T. BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL  DAM-    **/
/**    AGES OR  ANY DAMAGES WHATSOEVER  RESULTING FROM LOSS OF USE, DATA    **/
/**    OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER    **/
/**    TORTIOUS ACTION, ARISING OUT OF OR IN  CONNECTION  WITH  THE  USE    **/
/**    OR PERFORMANCE OF THIS SOFTWARE.                                     **/
/*****************************************************************************/
/*
 *  [ ctwm ]
 *
 *  Copyright 1992 Claude Lecommandeur.
 *
 * Permission to use, copy, modify  and distribute this software  [ctwm] and
 * its documentation for any purpose is hereby granted without fee, provided
 * that the above  copyright notice appear  in all copies and that both that
 * copyright notice and this permission notice appear in supporting documen-
 * tation, and that the name of  Claude Lecommandeur not be used in adverti-
 * sing or  publicity  pertaining to  distribution of  the software  without
 * specific, written prior permission. Claude Lecommandeur make no represen-
 * tations  about the suitability  of this software  for any purpose.  It is
 * provided "as is" without express or implied warranty.
 *
 * Claude Lecommandeur DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL  IMPLIED WARRANTIES OF  MERCHANTABILITY AND FITNESS.  IN NO
 * EVENT SHALL  Claude Lecommandeur  BE LIABLE FOR ANY SPECIAL,  INDIRECT OR
 * CONSEQUENTIAL  DAMAGES OR ANY  DAMAGES WHATSOEVER  RESULTING FROM LOSS OF
 * USE, DATA  OR PROFITS,  WHETHER IN AN ACTION  OF CONTRACT,  NEGLIGENCE OR
 * OTHER  TORTIOUS ACTION,  ARISING OUT OF OR IN  CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Claude Lecommandeur [ lecom@sic.epfl.ch ][ April 1992 ]
 */


/***********************************************************************
 *
 * $XConsortium: events.c,v 1.182 91/07/17 13:59:14 dave Exp $
 *
 * twm event handling
 *
 * 17-Nov-87 Thomas E. LaStrange                File created
 *
 * Do the necessary modification to be integrated in ctwm.
 * Can no longer be used for the standard twm.
 *
 * 22-April-92 Claude Lecommandeur.
 *
 *
 ***********************************************************************/

#include "ctwm.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <X11/extensions/shape.h>

#include "add_window.h"
#include "animate.h"
#include "captive.h"
#include "colormaps.h"
#include "events.h"
#include "event_handlers.h"
#include "event_internal.h"
#include "event_names.h"
#include "functions.h"
#include "iconmgr.h"
#include "image.h"
#include "screen.h"
#include "version.h"
#ifdef SOUNDS
#include "sound.h"
#endif


static void CtwmNextEvent(Display *display, XEvent  *event);
static bool StashEventTime(XEvent *ev);
static void dumpevent(const XEvent *e);

FILE *tracefile = NULL;

#define MAX_X_EVENT 256
event_proc EventHandler[MAX_X_EVENT]; /* event handler jump table */
int Context = C_NO_CONTEXT;     /* current button press context */
XEvent Event;                   /* the current event */

Window DragWindow;              /* variables used in moving windows */
int origDragX;
int origDragY;
int DragX;
int DragY;
unsigned int DragWidth;
unsigned int DragHeight;
unsigned int DragBW;
int CurrentDragX;
int CurrentDragY;

Time EventTime = CurrentTime;       /* until Xlib does this for us */

/* Maybe more staticizable later? */
bool enter_flag;
bool leave_flag;
TwmWindow *enter_win, *raise_win, *leave_win, *lower_win;
TwmWindow *ButtonWindow; /* button press window structure */

/*
 * Not static because shared with colormaps.c and other events files, but
 * not listed in events.h since nowhere else needs it.
 */
bool ColortableThrashing;
TwmWindow *Tmp_win; // the current twm window; shared with other event code

int ButtonPressed = -1;
bool Cancel = false;



/*
 * Initialize the event handling bits.  Mainly the jump table for event
 * handling, plus various event vars.  Called during startup.
 */
void
InitEvents(void)
{
	/* Clear out vars */
	ResizeWindow = (Window) 0;
	DragWindow = (Window) 0;
	enter_flag = false;
	enter_win = raise_win = NULL;
	leave_flag = false;
	leave_win = lower_win = NULL;

	/* Set everything to unknown to start */
	for(int i = 0; i < MAX_X_EVENT; i++) {
		EventHandler[i] = HandleUnknown;
	}

#define STDH(evt) EventHandler[evt] = Handle##evt

	/* Init the standard events */
	STDH(Expose);
	STDH(CreateNotify);
	STDH(DestroyNotify);
	STDH(MapRequest);
	STDH(MapNotify);
	STDH(UnmapNotify);
	STDH(MotionNotify);
	STDH(ButtonRelease);
	STDH(ButtonPress);
	STDH(EnterNotify);
	STDH(LeaveNotify);
	STDH(ConfigureRequest);
	STDH(ClientMessage);
	STDH(PropertyNotify);
	STDH(KeyPress);
	STDH(KeyRelease);
	STDH(ColormapNotify);
	STDH(VisibilityNotify);
	STDH(CirculateNotify);

	/* Focus handlers are special; see comment on HandleFocusChange() */
	EventHandler[FocusIn] = HandleFocusChange;
	EventHandler[FocusOut] = HandleFocusChange;

	/* Some more conditional bits */
#ifdef EWMH
	STDH(SelectionClear);
#endif

	/*
	 * Optional extensions are registered dynamically, so their events
	 * are put by the X server at some offset into the event number tree
	 * when it starts.
	 */
	if(HasShape) {
		EventHandler[ShapeEventBase + ShapeNotify] = HandleShapeNotify;
	}

#undef STDH

	/* And done */
	return;
}



/*
 * Main event loop.
 *
 * This is the last thing called during ctwm startup, and never returns.
 * So in essence, this is everything ctwm does except starting up and
 * shutting down (and that latter winds up called through us as well).
 */
void
HandleEvents(void)
{
	while(1) {
		if(enter_flag && !QLength(dpy)) {
			if(enter_win && enter_win != raise_win) {
				AutoRaiseWindow(enter_win);   /* sets enter_flag T */
			}
			else {
				enter_flag = false;
			}
		}
		if(leave_flag && !QLength(dpy)) {
			if(leave_win && leave_win != lower_win) {
				AutoLowerWindow(leave_win);  /* sets leave_flag T */
			}
			else {
				leave_flag = false;
			}
		}
		if(ColortableThrashing && !QLength(dpy) && Scr) {
			InstallColormaps(ColormapNotify, NULL);
		}
		WindowMoved = false;

		CtwmNextEvent(dpy, &Event);

		if(Event.type < 0 || Event.type >= MAX_X_EVENT) {
			XtDispatchEvent(&Event);
		}
		else {
			(void) DispatchEvent();
		}
	}

	/* NOTREACHED */
	fprintf(stderr, "Error: Should never reach the end of HandleEvents()\n");
	exit(1);
}


/*
 * Grab the next event in the queue to process.
 */
static void
CtwmNextEvent(Display *display, XEvent *event)
{
	int         fd;
	struct timeval timeout, *tout = NULL;
	const bool animate = (AnimationActive && MaybeAnimate);

#define NEXTEVENT XtAppNextEvent(appContext, event)

	if(RestartFlag) {
		DoRestart(CurrentTime);
	}
	if(XEventsQueued(display, QueuedAfterFlush) != 0) {
		NEXTEVENT;
		return;
	}
	fd = ConnectionNumber(display);

	if(animate) {
		TryToAnimate();
	}
	if(RestartFlag) {
		DoRestart(CurrentTime);
	}
	if(! MaybeAnimate) {
		NEXTEVENT;
		return;
	}
	if(animate) {
		tout = (AnimationSpeed > 0) ? &timeout : NULL;
	}
	while(1) {
		fd_set mask;
		int found;

		FD_ZERO(&mask);
		FD_SET(fd, &mask);
		if(animate) {
			timeout = AnimateTimeout;
		}
		found = select(fd + 1, &mask, NULL, NULL, tout);
		if(RestartFlag) {
			DoRestart(CurrentTime);
		}
		if(found < 0) {
			if(errno != EINTR) {
				perror("select");
			}
			continue;
		}
		if(FD_ISSET(fd, &mask)) {
			NEXTEVENT;
			return;
		}
		if(found == 0) {
			if(animate) {
				TryToAnimate();
			}
			if(RestartFlag) {
				DoRestart(CurrentTime);
			}
			if(! MaybeAnimate) {
				NEXTEVENT;
				return;
			}
			continue;
		}
	}

#undef NEXTEVENT

	/* NOTREACHED */
}



/*
 * And dispatchers.  These look at the global Event and run with it from
 * there.
 *
 * There are slight differences between the two, and it's not at all
 * clear to what extent there should be or why they're different.  At
 * least some of the difference seem gratuitous; this requires further
 * research.  They probably can and should be collapsed together.
 */
/* Main dispatcher, from the loop above and a few other places */
bool
DispatchEvent(void)
{
	Window w = Event.xany.window;
	ScreenInfo *thisScr;

	StashEventTime(&Event);
	Tmp_win = GetTwmWindow(w);
	thisScr = GetTwmScreen(&Event);

	dumpevent(&Event);

	if(!thisScr) {
		return false;
	}
	Scr = thisScr;

	if(CLarg.is_captive) {
		if((Event.type == ConfigureNotify)
		                && (Event.xconfigure.window == Scr->CaptiveRoot)) {
			ConfigureCaptiveRootWindow(&Event);
			return false;
		}
	}
	FixRootEvent(&Event);
	if(Event.type >= 0 && Event.type < MAX_X_EVENT) {
#ifdef SOUNDS
		play_sound(Event.type);
#endif
		(*EventHandler[Event.type])();
	}
	return true;
}


/* Alternate; called from various function and menu code */
bool
DispatchEvent2(void)
{
	Window w = Event.xany.window;
	ScreenInfo *thisScr;

	StashEventTime(&Event);
	Tmp_win = GetTwmWindow(w);
	thisScr = GetTwmScreen(&Event);

	dumpevent(&Event);

	if(!thisScr) {
		return false;
	}
	Scr = thisScr;

	FixRootEvent(&Event);

#ifdef SOUNDS
	play_sound(Event.type);
#endif

	if(menuFromFrameOrWindowOrTitlebar) {
		if(Event.type == Expose) {
			HandleExpose();
		}
	}
	else {
		if(Event.type >= 0 && Event.type < MAX_X_EVENT) {
			(*EventHandler[Event.type])();
		}
	}

	return true;
}




/*
 * Stash the time of the given event in our EventTime global.  Called
 * during dispatching the event.
 */
static bool
StashEventTime(XEvent *ev)
{
	switch(ev->type) {
		case KeyPress:
		case KeyRelease:
			EventTime = ev->xkey.time;
			return true;
		case ButtonPress:
		case ButtonRelease:
			EventTime = ev->xbutton.time;
			return true;
		case MotionNotify:
			EventTime = ev->xmotion.time;
			return true;
		case EnterNotify:
		case LeaveNotify:
			EventTime = ev->xcrossing.time;
			return true;
		case PropertyNotify:
			EventTime = ev->xproperty.time;
			return true;
		case SelectionClear:
			EventTime = ev->xselectionclear.time;
			return true;
		case SelectionRequest:
			EventTime = ev->xselectionrequest.time;
			return true;
		case SelectionNotify:
			EventTime = ev->xselection.time;
			return true;
	}
	return false;
}


/*
 * Debugging: output details of an event.  Or at least, details of a few
 * events, presumably those which somebody doing debugging needed at the
 * time.
 */
static void
dumpevent(const XEvent *e)
{
	const char *name;

	if(! tracefile) {
		return;
	}

	/* Whatsit? */
	name = event_name_by_num(e->type);
	if(!name) {
		name = "Unknown event";
	}

	/* Tell about it */
	fprintf(tracefile, "event:  %s in window 0x%x\n", name,
	        (unsigned int)e->xany.window);
	switch(e->type) {
		case KeyPress:
		case KeyRelease:
			fprintf(tracefile, "     :  +%d,+%d (+%d,+%d)  state=%d, keycode=%d\n",
			        e->xkey.x, e->xkey.y,
			        e->xkey.x_root, e->xkey.y_root,
			        e->xkey.state, e->xkey.keycode);
			break;
		case ButtonPress:
		case ButtonRelease:
			fprintf(tracefile, "     :  +%d,+%d (+%d,+%d)  state=%d, button=%d\n",
			        e->xbutton.x, e->xbutton.y,
			        e->xbutton.x_root, e->xbutton.y_root,
			        e->xbutton.state, e->xbutton.button);
			break;
	}
}
