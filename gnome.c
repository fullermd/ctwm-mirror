#include <stdio.h>
#include "twm.h"
#include "screen.h"
#ifdef VMS
#include <ctype.h>
#include <string.h>
#include <decw$include/Xos.h>
#include <decw$include/Xatom.h>
#include <X11Xmu/CharSet.h>
#include <decw$include/Xresource.h>
#else
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/Xmu/CharSet.h>
#include <X11/Xresource.h>
#endif

#define PROTOCOLS_COUNT 5

Atom _XA_WIN_WORKSPACE;
Atom _XA_WIN_STATE;
static Atom _XA_WIN_CLIENT_LIST;
static Window *ws;
static int wsSize;
static int numWins;

void InitGnome();
void GnomeAddClientList();
void GnomeDeleteClientWindow();


void InitGnome () {
  int curws = 1;
  virtualScreen *vs;
  Atom _XA_WIN_SUPPORTING_WM_CHECK, _XA_WIN_PROTOCOLS,
    _XA_WIN_PROTOCOLS_LIST[PROTOCOLS_COUNT], _XA_WIN_DESKTOP_BUTTON_PROXY;
  XWindowAttributes winattrs;
  unsigned long eventMask;

  XGetWindowAttributes(dpy, Scr->Root, &winattrs);
  eventMask = winattrs.your_event_mask;
  XSelectInput(dpy, Scr->Root, eventMask & ~PropertyChangeMask);
	
  _XA_WIN_SUPPORTING_WM_CHECK  = XInternAtom (dpy, "_WIN_SUPPORTING_WM_CHECK", False);
  _XA_WIN_DESKTOP_BUTTON_PROXY = XInternAtom (dpy, "_WIN_DESKTOP_BUTTON_PROXY", False);

  for (vs = Scr->vScreenList; vs != NULL; vs = vs->next) {
    XChangeProperty (dpy, vs->wsw->w, _XA_WIN_SUPPORTING_WM_CHECK, XA_CARDINAL, 32, 
		    PropModeReplace,  (unsigned char *) &(vs->wsw->w), 1);

    XChangeProperty (dpy, Scr->Root,  _XA_WIN_SUPPORTING_WM_CHECK, XA_CARDINAL, 32, 
		    PropModeReplace,  (unsigned char *) &(vs->wsw->w), 1);

    XChangeProperty (dpy, vs->wsw->w, _XA_WIN_DESKTOP_BUTTON_PROXY, XA_CARDINAL, 32, 
		    PropModeReplace,  (unsigned char *) &(vs->wsw->w), 1);

    XChangeProperty (dpy, Scr->Root,  _XA_WIN_DESKTOP_BUTTON_PROXY, XA_CARDINAL, 32, 
		    PropModeReplace,  (unsigned char *) &(vs->wsw->w), 1);
  }
  _XA_WIN_PROTOCOLS = XInternAtom (dpy, "_WIN_PROTOCOLS", False);
  _XA_WIN_PROTOCOLS_LIST[0] = XInternAtom(dpy, "_WIN_WORKSPACE", False);
  _XA_WIN_PROTOCOLS_LIST[1] = XInternAtom(dpy, "_WIN_WORKSPACE_COUNT", False);
  _XA_WIN_PROTOCOLS_LIST[2] = XInternAtom(dpy, "_WIN_WORKSPACE_NAMES", False);
  _XA_WIN_PROTOCOLS_LIST[3] = XInternAtom(dpy, "_WIN_CLIENT_LIST", False);
  _XA_WIN_PROTOCOLS_LIST[4] = XInternAtom(dpy, "_WIN_STATE", False);
  _XA_WIN_WORKSPACE = _XA_WIN_PROTOCOLS_LIST[0];
  _XA_WIN_CLIENT_LIST = _XA_WIN_PROTOCOLS_LIST[3];
  _XA_WIN_STATE = _XA_WIN_PROTOCOLS_LIST[4];
	
  XChangeProperty (dpy, Scr->Root, _XA_WIN_PROTOCOLS, XA_ATOM, 32, 
		   PropModeReplace, (unsigned char *) _XA_WIN_PROTOCOLS_LIST,
		   PROTOCOLS_COUNT);

  XChangeProperty (dpy, Scr->Root, _XA_WIN_PROTOCOLS_LIST[1], XA_CARDINAL, 32,
		   PropModeReplace, (unsigned char *) &(Scr->workSpaceMgr.count), 1);

  XChangeProperty (dpy, Scr->Root, _XA_WIN_PROTOCOLS_LIST[0], XA_CARDINAL, 32,
		   PropModeReplace, (unsigned char *) &curws, 1);

  XSelectInput (dpy, Scr->Root, eventMask);

  ws = malloc(sizeof(Window));
  if (ws) {
    wsSize = 1;
    numWins = 0;
  } else wsSize = numWins = 0;
  XChangeProperty (dpy, Scr->Root, _XA_WIN_CLIENT_LIST, XA_CARDINAL, 32, 
		   PropModeReplace, (unsigned char *)ws, numWins);
}


void GnomeAddClientWindow (TwmWindow *new_win) {
  XWindowAttributes winattrs;
  unsigned long eventMask;

  if ((!LookInList (Scr->IconMgrNoShow, new_win->full_name, &new_win->class)) && 
      (new_win->w != Scr->workSpaceMgr.occupyWindow->w) && 
      (!new_win->iconmgr)) {
    if (++numWins > wsSize) ws = realloc(ws, sizeof(Window) * (wsSize *= 2));
    if (ws) ws [numWins - 1] = new_win->w;
    else {
      fprintf (stderr, "Unable to allocate memory for GNOME client list.");
      return;
    }
    XGetWindowAttributes (dpy, Scr->Root, &winattrs);
    eventMask = winattrs.your_event_mask;
    XSelectInput(dpy, Scr->Root, eventMask & ~PropertyChangeMask);		
    XChangeProperty(dpy, Scr->Root, _XA_WIN_CLIENT_LIST, XA_CARDINAL, 32, 
		    PropModeReplace, (unsigned char *)ws, numWins);
    XSelectInput(dpy, Scr->Root, eventMask);
  }
}

void GnomeDeleteClientWindow (TwmWindow *new_win) {
  XWindowAttributes winattrs;
  unsigned long eventMask;
  int i;
  if ((!LookInList(Scr->IconMgrNoShow, new_win->full_name, &new_win->class)) && 
      (new_win->w != Scr->workSpaceMgr.occupyWindow->w) && 
      (!new_win->iconmgr)) {
    for (i = 0; i < numWins; i++){
      if(ws[i] == new_win->w){
	numWins--;
	ws[i] = ws[numWins];
	ws[numWins] = 0;
	if((numWins * 3) < wsSize)
	  ws = realloc (ws, sizeof (Window) * (wsSize /= 2));
	  /* memory shrinking, shouldn't have problems */
	break;
      }
    }
    XGetWindowAttributes (dpy, Scr->Root, &winattrs);
    eventMask = winattrs.your_event_mask;
    XSelectInput(dpy, Scr->Root, eventMask & ~PropertyChangeMask);		
    XChangeProperty(dpy, Scr->Root, _XA_WIN_CLIENT_LIST, XA_CARDINAL, 32, 
		    PropModeReplace, (unsigned char *)ws, numWins);
    XSelectInput(dpy, Scr->Root, eventMask);
  }
}
