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
#include <stdio.h>
#if defined(sony_news) || defined __QNX__
#  include <ctype.h>
#endif
#include "twm.h"
#include "util.h"
#include "parse.h"
#include "screen.h"
#include "icons.h"
#include "resize.h"
#include "add_window.h"
#include "events.h"
#include "gram.h"
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
#ifdef macII
int strcmp(); /* missing from string.h in AUX 2.0 */
#endif
#ifdef BUGGY_HP700_SERVER
static void fakeRaiseLower ();
#endif

extern int twmrc_error_prefix (); /* in ctwm.c */
extern int match ();		/* in list.c */
extern char *captivename;

/***********************************************************************
 *
 *  Procedure:
 *	CreateWorkSpaceManager - create teh workspace manager window
 *		for this screen.
 *
 *  Returned Value:
 *	none
 *
 *  Inputs:
 *	none
 *
 ***********************************************************************
 */
#define WSPCWINDOW    0
#define OCCUPYWINDOW  1
#define OCCUPYBUTTON  2

static void Vanish			();
static void DisplayWin			();
static void CreateWorkSpaceManagerWindow ();
static void CreateOccupyWindow		();
static unsigned int GetMaskFromResource	();
unsigned int GetMaskFromProperty	();
static   int GetPropertyFromMask	();
static void PaintWorkSpaceManagerBorder	();
static void PaintButton			();
static void WMapRemoveFromList		();
static void WMapAddToList		();
static void ResizeWorkSpaceManager	();
static void ResizeOccupyWindow		();
static WorkSpaceList *GetWorkspace	();
static void WMapRedrawWindow		();

Atom _XA_WM_OCCUPATION;
Atom _XA_WM_CURRENTWORKSPACE;
Atom _XA_WM_WORKSPACESLIST;
Atom _XA_WM_CTWMSLIST;
Atom _OL_WIN_ATTR;

int       fullOccupation    = 0;
int       useBackgroundInfo = False;
XContext  MapWListContext = (XContext) 0;
static Cursor handCursor  = (Cursor) 0;
static Bool DontRedirect ();

extern Bool MaybeAnimate;
extern FILE *tracefile;

void InitWorkSpaceManager ()
{
    Scr->workSpaceMgr.count			= 0;
    Scr->workSpaceMgr.workSpaceList		= NULL;

    Scr->workSpaceMgr.workspaceWindow.name		= "WorkSpaceManager";
    Scr->workSpaceMgr.workspaceWindow.icon_name		= "WorkSpaceManager Icon";
    Scr->workSpaceMgr.workspaceWindow.geometry		= NULL;
    Scr->workSpaceMgr.workspaceWindow.columns		= 0;
    Scr->workSpaceMgr.workspaceWindow.twm_win		= (TwmWindow*) 0;
    Scr->workSpaceMgr.workspaceWindow.state		= BUTTONSSTATE;
    Scr->workSpaceMgr.workspaceWindow.buttonStyle	= STYLE_NORMAL;
    Scr->workSpaceMgr.workspaceWindow.noshowoccupyall	= FALSE;
#ifdef I18N
    Scr->workSpaceMgr.workspaceWindow.windowFont.basename	=
		"-adobe-courier-medium-r-normal--10-100-75-75-m-60-iso8859-1";
		/*"-adobe-courier-bold-r-normal--8-80-75-75-m-50-iso8859-1";*/
#else
    Scr->workSpaceMgr.workspaceWindow.windowFont.name	=
		"-adobe-times-*-r-*--10-*-*-*-*-*-*-*";
		/*"-adobe-courier-bold-r-normal--8-80-75-75-m-50-iso8859-1";*/
#endif    
    Scr->workSpaceMgr.workspaceWindow.windowcp.back	= Scr->White;
    Scr->workSpaceMgr.workspaceWindow.windowcp.fore	= Scr->Black;
    Scr->workSpaceMgr.workspaceWindow.windowcpgiven	= False;

    Scr->workSpaceMgr.workspaceWindow.curImage		= None;
    Scr->workSpaceMgr.workspaceWindow.curColors.back	= Scr->Black;
    Scr->workSpaceMgr.workspaceWindow.curColors.fore	= Scr->White;
    Scr->workSpaceMgr.workspaceWindow.curPaint		= False;

    Scr->workSpaceMgr.workspaceWindow.defImage		= None;
    Scr->workSpaceMgr.workspaceWindow.defColors.back	= Scr->White;
    Scr->workSpaceMgr.workspaceWindow.defColors.fore	= Scr->Black;


    Scr->workSpaceMgr.occupyWindow.name		= "Occupy Window";
    Scr->workSpaceMgr.occupyWindow.icon_name	= "Occupy Window Icon";
    Scr->workSpaceMgr.occupyWindow.geometry	= NULL;
    Scr->workSpaceMgr.occupyWindow.columns	= 0;
    Scr->workSpaceMgr.occupyWindow.twm_win	= (TwmWindow*) 0;

    XrmInitialize ();
    if (MapWListContext == (XContext) 0) MapWListContext = XUniqueContext ();
}

ConfigureWorkSpaceManager () {
    Scr->workSpaceMgr.workspaceWindow.vspace	= Scr->WMgrVertButtonIndent;
    Scr->workSpaceMgr.workspaceWindow.hspace	= Scr->WMgrHorizButtonIndent;
    Scr->workSpaceMgr.occupyWindow.vspace	= Scr->WMgrVertButtonIndent;
    Scr->workSpaceMgr.occupyWindow.hspace	= Scr->WMgrHorizButtonIndent;
}

void CreateWorkSpaceManager () {
    WorkSpaceList *wlist;
    char wrkSpcList [512];
    int len;

    if (! Scr->workSpaceManagerActive) return;

    _XA_WM_OCCUPATION       = XInternAtom (dpy, "WM_OCCUPATION",       False);
    _XA_WM_CURRENTWORKSPACE = XInternAtom (dpy, "WM_CURRENTWORKSPACE", False);
    _XA_WM_WORKSPACESLIST   = XInternAtom (dpy, "WM_WORKSPACESLIST",   False);
    _OL_WIN_ATTR            = XInternAtom (dpy, "_OL_WIN_ATTR",        False);

    NewFontCursor (&handCursor, "hand2");

    Scr->workSpaceMgr.activeWSPC = Scr->workSpaceMgr.workSpaceList;
    CreateWorkSpaceManagerWindow ();
    CreateOccupyWindow ();

    wlist = Scr->workSpaceMgr.activeWSPC;
    if (Scr->workSpaceMgr.workspaceWindow.curImage == None) {
	if (Scr->workSpaceMgr.workspaceWindow.curPaint) {
	    XSetWindowBackground (dpy, wlist->mapSubwindow.w,
		Scr->workSpaceMgr.workspaceWindow.curColors.back);
	}
    } else {
	XSetWindowBackgroundPixmap (dpy, wlist->mapSubwindow.w,
		Scr->workSpaceMgr.workspaceWindow.curImage->pixmap);
    }
    XSetWindowBorder (dpy, wlist->mapSubwindow.w,
		Scr->workSpaceMgr.workspaceWindow.curBorderColor);
    XClearWindow (dpy, wlist->mapSubwindow.w);

    len = GetPropertyFromMask (0xFFFFFFFF, wrkSpcList);
    XChangeProperty (dpy, Scr->Root, _XA_WM_WORKSPACESLIST, XA_STRING, 8, 
		     PropModeReplace, (unsigned char *) wrkSpcList, len);
}

void GotoWorkSpaceByName (wname)
char *wname;
{
    WorkSpaceList *wlist;

    if (! Scr->workSpaceManagerActive) return;
    for (wlist = Scr->workSpaceMgr.workSpaceList; wlist != NULL; wlist = wlist->next) {
	if (strcmp (wlist->label, wname) == 0) break;
    }
    if (wlist == NULL) {
	for (wlist = Scr->workSpaceMgr.workSpaceList; wlist != NULL; wlist = wlist->next) {
	    if (strcmp (wlist->name, wname) == 0) break;
	}
    }
    if (wlist == NULL) return;
    GotoWorkSpace (wlist);
}

void GotoPrevWorkSpace () {
    WorkSpaceList *wlist1, *wlist2;

    if (! Scr->workSpaceManagerActive) return;
    wlist1 = Scr->workSpaceMgr.workSpaceList;
    if (wlist1 == NULL) return;

    wlist2 = wlist1->next;
    while ((wlist2 != Scr->workSpaceMgr.activeWSPC) && (wlist2 != NULL)) {
	wlist1 = wlist2;
	wlist2 = wlist2->next;
    }
    GotoWorkSpace (wlist1);
}

void GotoNextWorkSpace () {
    WorkSpaceList *wlist;

    if (! Scr->workSpaceManagerActive) return;
    wlist = Scr->workSpaceMgr.activeWSPC;
    wlist = (wlist->next != NULL) ? wlist->next : Scr->workSpaceMgr.workSpaceList;
    GotoWorkSpace (wlist);
}

void GotoRightWorkSpace () {
    WorkSpaceList *wlist;
    int number, columns, count, i;

    if (! Scr->workSpaceManagerActive) return;
    wlist   = Scr->workSpaceMgr.activeWSPC;
    if (! wlist) return;
    number  = wlist->number;
    columns = Scr->workSpaceMgr.workspaceWindow.columns;
    count   = Scr->workSpaceMgr.count;
    number += ((number + 1) % columns) ? 1 : (1 - columns);
    if (number >= count) number = (number / columns) * columns;
    wlist = Scr->workSpaceMgr.workSpaceList;
    for (i = 0; i < number; i++) wlist = wlist->next;
    GotoWorkSpace (wlist);
}

void GotoLeftWorkSpace () {
    WorkSpaceList *wlist;
    int number, columns, count, i;

    if (! Scr->workSpaceManagerActive) return;
    wlist   = Scr->workSpaceMgr.activeWSPC;
    if (! wlist) return;
    number  = wlist->number;
    columns = Scr->workSpaceMgr.workspaceWindow.columns;
    count   = Scr->workSpaceMgr.count;
    number += (number % columns) ? -1 : (columns - 1);
    if (number >= count) number = count - 1;
    wlist = Scr->workSpaceMgr.workSpaceList;
    for (i = 0; i < number; i++) wlist = wlist->next;
    GotoWorkSpace (wlist);
}

void GotoUpWorkSpace () {
    WorkSpaceList *wlist;
    int number, lines, columns, count, i;

    if (! Scr->workSpaceManagerActive) return;
    wlist   = Scr->workSpaceMgr.activeWSPC;
    if (! wlist) return;
    number  = wlist->number;
    lines   = Scr->workSpaceMgr.workspaceWindow.lines;
    columns = Scr->workSpaceMgr.workspaceWindow.columns;
    count   = Scr->workSpaceMgr.count;
    number += (number >= columns) ? - columns : ((lines - 1) * columns);
    while (number >= count) number -= columns;
    wlist = Scr->workSpaceMgr.workSpaceList;
    for (i = 0; i < number; i++) wlist = wlist->next;
    GotoWorkSpace (wlist);
}

void GotoDownWorkSpace () {
    WorkSpaceList *wlist;
    int number, lines, columns, count, i;

    if (! Scr->workSpaceManagerActive) return;
    wlist   = Scr->workSpaceMgr.activeWSPC;
    if (! wlist) return;
    number  = wlist->number;
    lines   = Scr->workSpaceMgr.workspaceWindow.lines;
    columns = Scr->workSpaceMgr.workspaceWindow.columns;
    count   = Scr->workSpaceMgr.count;
    number += (number < ((lines - 1) * columns)) ? columns : ((1 - lines) * columns);
    if (number >= count) number = number % columns;
    wlist = Scr->workSpaceMgr.workSpaceList;
    for (i = 0; i < number; i++) wlist = wlist->next;
    GotoWorkSpace (wlist);
}

void GotoWorkSpace (wlist)
WorkSpaceList *wlist;
{
    TwmWindow	  *twmWin;
    WorkSpaceList *oldscr, *newscr;
    WList	  *wl, *wl1;
    WinList	  winl;
    XSetWindowAttributes attr;
    XWindowAttributes    winattrs;
    unsigned long	 eventMask;
    IconMgr		*iconmgr;
    Window		w;
    unsigned long	valuemask;
    TwmWindow		*focuswindow;
    TwmWindow		*last_twmWin;

    if (! Scr->workSpaceManagerActive) return;
    oldscr = Scr->workSpaceMgr.activeWSPC;
    newscr = wlist;
    if (oldscr == newscr) return;

    valuemask = (CWBackingStore | CWSaveUnder);
    attr.backing_store = NotUseful;
    attr.save_under    = False;
    w = XCreateWindow (dpy, Scr->Root, 0, 0,
			(unsigned int) Scr->MyDisplayWidth,
			(unsigned int) Scr->MyDisplayHeight,
			(unsigned int) 0,
			CopyFromParent, (unsigned int) CopyFromParent,
			(Visual *) CopyFromParent, valuemask,
			&attr);
    XMapWindow (dpy, w);

    if (useBackgroundInfo && ! Scr->DontPaintRootWindow) {
	if (newscr->image == None)
	    XSetWindowBackground (dpy, Scr->Root, newscr->backcp.back);
	else
	    XSetWindowBackgroundPixmap (dpy, Scr->Root, newscr->image->pixmap);

	XClearWindow (dpy, Scr->Root);
    }
    focuswindow = (TwmWindow*)0;
    for (twmWin = &(Scr->TwmRoot); twmWin != NULL; twmWin = twmWin->next) {
	if (OCCUPY (twmWin, oldscr)) {
	    if (twmWin->mapped == FALSE) {
		if ((twmWin->icon_on == TRUE) && (twmWin->icon) && (twmWin->icon->w)) {
		    XUnmapWindow(dpy, twmWin->icon->w);
		    IconDown (twmWin);
		}
		if (twmWin->UnmapByMovingFarAway && !OCCUPY (twmWin, newscr))
		    XMoveWindow (dpy, twmWin->frame,
				Scr->MyDisplayWidth  + 1,
				Scr->MyDisplayHeight + 1);
	    }
	    else {
		if (! OCCUPY (twmWin, newscr)) Vanish (twmWin);
		else
		if (twmWin->hasfocusvisible) {
		    focuswindow = twmWin;
		    SetFocusVisualAttributes (focuswindow, False);
		}
	    }
	}
    }
    /* Move to the end of the twmWin list */
    for (twmWin = &(Scr->TwmRoot); twmWin != NULL; twmWin = twmWin->next) {
	last_twmWin = twmWin;
    }
    /* Iconise in reverse order */
    for (twmWin = last_twmWin; twmWin != NULL; twmWin = twmWin->prev) {
	if (OCCUPY (twmWin, newscr)) {
	    if (twmWin->mapped == FALSE) {
		if ((twmWin->icon_on == TRUE) && (twmWin->icon) && (twmWin->icon->w)) {
		    IconUp (twmWin);
		    XMapWindow (dpy, twmWin->icon->w);
		}
		if (twmWin->UnmapByMovingFarAway && !OCCUPY (twmWin, oldscr))
		    XMoveWindow (dpy, twmWin->frame,
			twmWin->frame_x,
			twmWin->frame_y);
	    }
	    else {
		if (! OCCUPY (twmWin, oldscr))  DisplayWin (twmWin);
	    }
	}
    }
/*
   Reorganize window lists
*/
    for (twmWin = &(Scr->TwmRoot); twmWin != NULL; twmWin = twmWin->next) {
	wl = twmWin->list;
	if (wl == NULL) continue;
	if (OCCUPY (wl->iconmgr->twm_win, newscr)) continue;
	wl1 = wl;
	wl  = wl->nextv;
	while (wl != NULL) {
	    if (OCCUPY (wl->iconmgr->twm_win, newscr)) break;
	    wl1 = wl;
	    wl  = wl->nextv;
	}
	if (wl != NULL) {
	    wl1->nextv = wl->nextv;
	    wl->nextv  = twmWin->list;
	    twmWin->list = wl;
	}
    }
    wl = (WList*)0;
    for (iconmgr = newscr->iconmgr; iconmgr; iconmgr = iconmgr->next) {
	if (iconmgr->first) {
	    wl = iconmgr->first;
	    break;
	}
    }	
    CurrentIconManagerEntry (wl);
    if (focuswindow) {
	SetFocusVisualAttributes (focuswindow, True);
    }
    Scr->workSpaceMgr.activeWSPC = newscr;
    if (Scr->ReverseCurrentWorkspace &&
	Scr->workSpaceMgr.workspaceWindow.state == MAPSTATE) {
	for (winl = oldscr->mapSubwindow.wl; winl != NULL; winl = winl->next) {
	    WMapRedrawName (winl);
	}
	for (winl = newscr->mapSubwindow.wl; winl != NULL; winl = winl->next) {
	    WMapRedrawName (winl);
	}
    } else
    if (Scr->workSpaceMgr.workspaceWindow.state == BUTTONSSTATE) {
	PaintButton (WSPCWINDOW, oldscr->buttonw, oldscr->label, oldscr->cp, off);
	PaintButton (WSPCWINDOW, newscr->buttonw, newscr->label, newscr->cp,  on);
    }
    oldscr->iconmgr = Scr->iconmgr;
    Scr->iconmgr = newscr->iconmgr;

    if (useBackgroundInfo) {
	if (oldscr->image == None)
	    XSetWindowBackground (dpy, oldscr->mapSubwindow.w, oldscr->backcp.back);
	else
	    XSetWindowBackgroundPixmap (dpy, oldscr->mapSubwindow.w, oldscr->image->pixmap);
    }
    else {
	if (Scr->workSpaceMgr.workspaceWindow.defImage == None)
	    XSetWindowBackground (dpy, oldscr->mapSubwindow.w,
			Scr->workSpaceMgr.workspaceWindow.defColors.back);
	else
	    XSetWindowBackgroundPixmap (dpy, oldscr->mapSubwindow.w,
			Scr->workSpaceMgr.workspaceWindow.defImage->pixmap);
    }
    attr.border_pixel = Scr->workSpaceMgr.workspaceWindow.defBorderColor;
    XChangeWindowAttributes (dpy, oldscr->mapSubwindow.w, CWBorderPixel, &attr);

    if (Scr->workSpaceMgr.workspaceWindow.curImage == None) {
	if (Scr->workSpaceMgr.workspaceWindow.curPaint) {
	    XSetWindowBackground (dpy, newscr->mapSubwindow.w,
			Scr->workSpaceMgr.workspaceWindow.curColors.back);
	}
    }
    else {
	XSetWindowBackgroundPixmap (dpy, newscr->mapSubwindow.w,
			Scr->workSpaceMgr.workspaceWindow.curImage->pixmap);
    }
    attr.border_pixel = Scr->workSpaceMgr.workspaceWindow.curBorderColor;
    XChangeWindowAttributes (dpy, newscr->mapSubwindow.w, CWBorderPixel, &attr);

    XClearWindow (dpy, oldscr->mapSubwindow.w);
    XClearWindow (dpy, newscr->mapSubwindow.w);

    XGetWindowAttributes(dpy, Scr->Root, &winattrs);
    eventMask = winattrs.your_event_mask;
    XSelectInput(dpy, Scr->Root, eventMask & ~PropertyChangeMask);

    XChangeProperty (dpy, Scr->Root, _XA_WM_CURRENTWORKSPACE, XA_STRING, 8, 
		     PropModeReplace, (unsigned char *) newscr->name, strlen (newscr->name));
    XSelectInput(dpy, Scr->Root, eventMask);

    XDestroyWindow (dpy, w);
    if (Scr->ChangeWorkspaceFunction.func != 0) {
	char *action;
	XEvent event;

	action = Scr->ChangeWorkspaceFunction.item ?
		Scr->ChangeWorkspaceFunction.item->action : NULL;
	ExecuteFunction (Scr->ChangeWorkspaceFunction.func, action,
			   (Window) 0, (TwmWindow*) 0, &event, C_ROOT, FALSE);
    }
    XSync (dpy, 0);
    MaybeAnimate = True;
}

char *GetCurrentWorkSpaceName ()
{
    WorkSpaceList *wlist;

    if (! Scr->workSpaceManagerActive) return (NULL);
    wlist   = Scr->workSpaceMgr.activeWSPC;
    if (! wlist) return (NULL);
    return (wlist->name);
}

void AddWorkSpace (name, background, foreground, backback, backfore, backpix)
char *name, *background, *foreground, *backback, *backfore, *backpix;
{
    WorkSpaceList *wlist, *wl;
    int	  	  scrnum;
    Image	  *image;

    scrnum = Scr->workSpaceMgr.count;
    if (scrnum == MAXWORKSPACE) return;

    fullOccupation     |= (1 << scrnum);
    wlist		= (WorkSpaceList*) malloc (sizeof (WorkSpaceList));
    wlist->FirstWindowRegion = NULL;
#ifdef VMS
    {
       char *ftemp;
       ftemp = (char *) malloc((strlen(name)+1)*sizeof(char));
       wlist->name = strcpy (ftemp,name);
       ftemp = (char *) malloc((strlen(name)+1)*sizeof(char));
       wlist->label = strcpy (ftemp,name);
    }
#else
    wlist->name		= (char*) strdup (name);
    wlist->label	= (char*) strdup (name);
#endif
    wlist->clientlist	= NULL;

    if (background == NULL)
	wlist->cp.back = Scr->IconManagerC.back;
    else
	GetColor (Scr->Monochrome, &(wlist->cp.back), background);

    if (foreground == NULL)
	wlist->cp.fore = Scr->IconManagerC.fore;
    else
	GetColor (Scr->Monochrome, &(wlist->cp.fore), foreground);

#ifdef COLOR_BLIND_USER
    wlist->cp.shadc   = Scr->White;
    wlist->cp.shadd   = Scr->Black;
#else
    if (!Scr->BeNiceToColormap) GetShadeColors (&wlist->cp);
#endif

    if (backback == NULL)
	GetColor (Scr->Monochrome, &(wlist->backcp.back), "Black");
    else {
	GetColor (Scr->Monochrome, &(wlist->backcp.back), backback);
	useBackgroundInfo = True;
    }

    if (backfore == NULL)
	GetColor (Scr->Monochrome, &(wlist->backcp.fore), "White");
    else {
	GetColor (Scr->Monochrome, &(wlist->backcp.fore), backfore);
	useBackgroundInfo = True;
    }
    if ((image = GetImage (backpix, wlist->backcp)) != None) {
	wlist->image = image;
	useBackgroundInfo = True;
    }
    else {
	wlist->image = None;
    }
    wlist->next        = NULL;
    wlist->number      = scrnum;
    Scr->workSpaceMgr.count++;

    if (Scr->workSpaceMgr.workSpaceList == NULL) {
	Scr->workSpaceMgr.workSpaceList = wlist;
    }
    else {
	wl = Scr->workSpaceMgr.workSpaceList;
	while (wl->next != NULL) {wl = wl->next;}
	wl->next = wlist;
    }
    Scr->workSpaceManagerActive = 1;
}

static XrmDatabase db;
static XrmOptionDescRec table [] = {
    {"-xrm",		NULL,		XrmoptionResArg, (XPointer) NULL},
};

void SetupOccupation (twm_win, occupation_hint)
TwmWindow *twm_win;
int occupation_hint; /* <== [ Matthew McNeill Feb 1997 ] == */
{
    TwmWindow		*t;
    unsigned char	*prop;
    unsigned long	nitems, bytesafter;
    Atom		actual_type;
    int			actual_format;
    int			state;
    Window		icon;
    char		**cliargv = NULL;
    int			cliargc;
    Bool		status;
    char		*str_type;
    XrmValue		value;
    char		wrkSpcList [512];
    int			len;
    WorkSpaceList	*wlist;
    XWindowAttributes   winattrs;
    unsigned long	eventMask;

    if (! Scr->workSpaceManagerActive) {
	twm_win->occupation = 1;
	return;
    }
    twm_win->occupation = 0;

    for (wlist = Scr->workSpaceMgr.workSpaceList; wlist != NULL; wlist = wlist->next) {
	if (LookInList (wlist->clientlist, twm_win->full_name, &twm_win->class)) {
            twm_win->occupation |= 1 << wlist->number;
	}
    }

    if (LookInList (Scr->OccupyAll, twm_win->full_name, &twm_win->class)) {
        twm_win->occupation = fullOccupation;
    }

    if (XGetCommand (dpy, twm_win->w, &cliargv, &cliargc)) {
	XrmParseCommand (&db, table, 1, "ctwm", &cliargc, cliargv);
	status = XrmGetResource (db, "ctwm.workspace", "Ctwm.Workspace", &str_type, &value);
	if ((status == True) && (value.size != 0)) {
	    strncpy (wrkSpcList, value.addr, value.size);
	    twm_win->occupation = GetMaskFromResource (twm_win, wrkSpcList);
	}
	XrmDestroyDatabase (db);
	XFreeStringList (cliargv);
	db = NULL;
    }

    if (RestartPreviousState) {
	if (XGetWindowProperty (dpy, twm_win->w, _XA_WM_OCCUPATION, 0L, 2500, False,
				XA_STRING, &actual_type, &actual_format, &nitems,
				&bytesafter, &prop) == Success) {
	    if (nitems != 0) {
		twm_win->occupation = GetMaskFromProperty (prop, nitems);
		XFree ((char *) prop);
	    }
	}
    }

    if (twm_win->iconmgr) /* someone tried to modify occupation of icon managers */
	twm_win->occupation = 1 << Scr->workSpaceMgr.activeWSPC->number;

    if (! Scr->TransientHasOccupation) {
	if (twm_win->transient) {
	    for (t = Scr->TwmRoot.next; t != NULL; t = t->next) {
		if (twm_win->transientfor == t->w) break;
	    }
	    if (t != NULL) twm_win->occupation = t->occupation;
	}
	else
	if (twm_win->group != twm_win->w) {
	    for (t = Scr->TwmRoot.next; t != NULL; t = t->next) {
		if (t->w == twm_win->group) break;
	    }
	    if (t != NULL) twm_win->occupation = t->occupation;
	}
    }

    /*============[ Matthew McNeill Feb 1997 ]========================*
     * added in functionality of specific occupation state. The value 
     * should be a valid occupation bit-field or 0 for the default action
     */

    if (occupation_hint != 0)
      twm_win->occupation = occupation_hint;

    /*================================================================*/

    if ((twm_win->occupation & fullOccupation) == 0)
	twm_win->occupation = 1 << Scr->workSpaceMgr.activeWSPC->number;

    len = GetPropertyFromMask (twm_win->occupation, wrkSpcList);

    if (!XGetWindowAttributes(dpy, twm_win->w, &winattrs)) return;
    eventMask = winattrs.your_event_mask;
    XSelectInput(dpy, twm_win->w, eventMask & ~PropertyChangeMask);

    XChangeProperty (dpy, twm_win->w, _XA_WM_OCCUPATION, XA_STRING, 8, 
		     PropModeReplace, (unsigned char *) wrkSpcList, len);
    XSelectInput(dpy, twm_win->w, eventMask);

/* kludge */
    state = NormalState;
    if (!(RestartPreviousState && GetWMState (twm_win->w, &state, &icon) &&
	 (state == NormalState || state == IconicState || state == InactiveState))) {
	if (twm_win->wmhints && (twm_win->wmhints->flags & StateHint))
	    state = twm_win->wmhints->initial_state;
    }
    if (OCCUPY (twm_win, Scr->workSpaceMgr.activeWSPC)) {
	if (state == InactiveState) SetMapStateProp (twm_win, NormalState);
    }
    else {
	if (state == NormalState) SetMapStateProp (twm_win, InactiveState);
    }
}

Bool RedirectToCaptive (window)
Window window;
{
    unsigned char	*prop;
    unsigned long	nitems, bytesafter;
    Atom		actual_type;
    int			actual_format;
    int			state;
    char		**cliargv = NULL;
    int			cliargc;
    Bool		status;
    char		*str_type;
    XrmValue		value;
    int			len;
    int			ret;
    Atom		_XA_WM_CTWMSLIST, _XA_WM_CTWM_ROOT;
    char		*atomname;
    Window		newroot;
    XWindowAttributes	wa;

    if (DontRedirect (window)) return (False);
    if (!XGetCommand (dpy, window, &cliargv, &cliargc)) return (False);
    XrmParseCommand (&db, table, 1, "ctwm", &cliargc, cliargv);

    ret = False;
    status = XrmGetResource (db, "ctwm.redirect", "Ctwm.Redirect", &str_type, &value);
    if ((status == True) && (value.size != 0)) {
	char cctwm [64];
	strncpy (cctwm, value.addr, value.size);
	atomname = (char*) malloc (strlen ("WM_CTWM_ROOT_") + strlen (cctwm) + 1);
	sprintf (atomname, "WM_CTWM_ROOT_%s", cctwm);
	_XA_WM_CTWM_ROOT = XInternAtom (dpy, atomname, False);
	
	if (XGetWindowProperty (dpy, Scr->Root, _XA_WM_CTWM_ROOT,
		0L, 1L, False, AnyPropertyType, &actual_type, &actual_format,
		&nitems, &bytesafter, &prop) == Success) {
	    if (actual_type == XA_WINDOW && actual_format == 32 &&
			nitems == 1 /*&& bytesafter == 0*/) {
		newroot = *((Window*) prop);
		if (XGetWindowAttributes (dpy, newroot, &wa)) {
		    XReparentWindow (dpy, window, newroot, 0, 0);
		    XMapWindow (dpy, window);
		    ret = True;
		}
	    }
	}
    }
    status = XrmGetResource (db, "ctwm.rootWindow", "Ctwm.RootWindow", &str_type, &value);
    if ((status == True) && (value.size != 0)) {
	char rootw [32];
	strncpy (rootw, value.addr, value.size);
	if (sscanf (rootw, "%x", &newroot) == 1) {
	    if (XGetWindowAttributes (dpy, newroot, &wa)) {
		XReparentWindow (dpy, window, newroot, 0, 0);
		XMapWindow (dpy, window);
		ret = True;
	    }
	}
    }
    XrmDestroyDatabase (db);
    XFreeStringList (cliargv);
    db = NULL;
    return (ret);
}

static TwmWindow *occupyWin = (TwmWindow*) 0;

void Occupy (twm_win)
TwmWindow *twm_win;
{
    int			x, y, junkX, junkY;
    unsigned int	junkB, junkD;
    unsigned int	width, height;
    int			xoffset, yoffset;
    Window		junkW, w;
    unsigned int	junkK;
    WorkSpaceList	*wlist;

    if (! Scr->workSpaceManagerActive) return;
    if (occupyWin != (TwmWindow*) 0) return;
    if (twm_win->iconmgr) return;
    if (! Scr->TransientHasOccupation) {
	if (twm_win->transient) return;
	if ((twm_win->group != (Window) 0) && (twm_win->group != twm_win->w)) return;
    }
    for (wlist = Scr->workSpaceMgr.workSpaceList; wlist != NULL; wlist = wlist->next) {
	if (OCCUPY (twm_win, wlist)) {
	    PaintButton (OCCUPYWINDOW, wlist->obuttonw, wlist->label, wlist->cp, on);
	    Scr->workSpaceMgr.occupyWindow.tmpOccupation |= (1 << wlist->number);
	}
	else {
	    PaintButton (OCCUPYWINDOW, wlist->obuttonw, wlist->label, wlist->cp, off);
	    Scr->workSpaceMgr.occupyWindow.tmpOccupation &= ~(1 << wlist->number);
	}
    }
    w = Scr->workSpaceMgr.occupyWindow.w;
    XGetGeometry  (dpy, w, &junkW, &x, &y, &width, &height, &junkB, &junkD);
    XQueryPointer (dpy, Scr->Root, &junkW, &junkW, &junkX, &junkY, &x, &y, &junkK);
    x -= (width  / 2);
    y -= (height / 2);
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    xoffset = width  + 2 * Scr->BorderWidth;
    yoffset = height + 2 * Scr->BorderWidth + Scr->TitleHeight;
    if ((x + xoffset) > Scr->MyDisplayWidth)  x = Scr->MyDisplayWidth  - xoffset;
    if ((y + yoffset) > Scr->MyDisplayHeight) y = Scr->MyDisplayHeight - yoffset;

    Scr->workSpaceMgr.occupyWindow.twm_win->occupation = twm_win->occupation;
    XMoveWindow  (dpy, Scr->workSpaceMgr.occupyWindow.twm_win->frame, x, y);
    SetMapStateProp (Scr->workSpaceMgr.occupyWindow.twm_win, NormalState);
    XMapWindow      (dpy, Scr->workSpaceMgr.occupyWindow.w);
    XMapRaised      (dpy, Scr->workSpaceMgr.occupyWindow.twm_win->frame);
    Scr->workSpaceMgr.occupyWindow.twm_win->mapped = TRUE;
    Scr->workSpaceMgr.occupyWindow.x = x;
    Scr->workSpaceMgr.occupyWindow.y = y;
    occupyWin = twm_win;
}

void OccupyHandleButtonEvent (event)
XEvent *event;
{
    WorkSpaceList	*wlist;
    OccupyWindow	*occupyW;
    Window		buttonW;

    if (! Scr->workSpaceManagerActive) return;
    if (occupyWin == (TwmWindow*) 0) return;

    buttonW = event->xbutton.window;
    if (buttonW == 0) return; /* icon */

    XGrabPointer (dpy, Scr->Root, True,
		  ButtonPressMask | ButtonReleaseMask,
		  GrabModeAsync, GrabModeAsync,
		  Scr->Root, None, CurrentTime);

    occupyW = &Scr->workSpaceMgr.occupyWindow;
    for (wlist = Scr->workSpaceMgr.workSpaceList; wlist != NULL; wlist = wlist->next) {
	if (wlist->obuttonw == buttonW) break;
    }
    if (wlist != NULL) {
	if ((occupyW->tmpOccupation & (1 << wlist->number)) == 0) {
	    PaintButton (OCCUPYWINDOW, wlist->obuttonw, wlist->label, wlist->cp, on);
	    occupyW->tmpOccupation |= (1 << wlist->number);
	}
	else {
	    PaintButton (OCCUPYWINDOW, wlist->obuttonw, wlist->label, wlist->cp, off);
	    occupyW->tmpOccupation &= ~(1 << wlist->number);
	}
    }
    else
    if (buttonW == occupyW->OK) {
	if (occupyW->tmpOccupation == 0) return;
	ChangeOccupation (occupyWin, occupyW->tmpOccupation);
	Vanish (occupyW->twm_win);
	occupyW->twm_win->mapped = FALSE;
	occupyW->twm_win->occupation = 0;
	occupyWin = (TwmWindow*) 0;
	XSync (dpy, 0);
    }
    else
    if (buttonW == occupyW->cancel) {
	Vanish (occupyW->twm_win);
	occupyW->twm_win->mapped = FALSE;
	occupyW->twm_win->occupation = 0;
	occupyWin = (TwmWindow*) 0;
	XSync (dpy, 0);
    }
    else
    if (buttonW == occupyW->allworkspc) {
	for (wlist = Scr->workSpaceMgr.workSpaceList; wlist != NULL; wlist = wlist->next) {
	    PaintButton (OCCUPYWINDOW, wlist->obuttonw, wlist->label, wlist->cp, on);
	}
	occupyW->tmpOccupation = fullOccupation;
    }
    if (ButtonPressed == -1) XUngrabPointer (dpy, CurrentTime);
}

void OccupyAll (twm_win)
TwmWindow *twm_win;
{
    IconMgr *save;

    if (! Scr->workSpaceManagerActive) return;
    if (twm_win->iconmgr)   return;
    if ((! Scr->TransientHasOccupation) && twm_win->transient) return;
    save = Scr->iconmgr;
    Scr->iconmgr = Scr->workSpaceMgr.workSpaceList->iconmgr;
    ChangeOccupation (twm_win, fullOccupation);
    Scr->iconmgr = save;
}

void AddToWorkSpace (wname, twm_win)
char *wname;
TwmWindow *twm_win;
{
    WorkSpaceList *wlist;
    int newoccupation;

    if (! Scr->workSpaceManagerActive) return;
    if (twm_win->iconmgr)   return;
    if ((! Scr->TransientHasOccupation) && twm_win->transient) return;
    wlist = GetWorkspace (wname);
    if (!wlist) return;

    if (twm_win->occupation & (1 << wlist->number)) return;
    newoccupation = twm_win->occupation | (1 << wlist->number);
    ChangeOccupation (twm_win, newoccupation);
}

void RemoveFromWorkSpace (wname, twm_win)
char *wname;
TwmWindow *twm_win;
{
    WorkSpaceList *wlist;
    int newoccupation;

    if (! Scr->workSpaceManagerActive) return;
    if (twm_win->iconmgr)   return;
    if ((! Scr->TransientHasOccupation) && twm_win->transient) return;
    wlist = GetWorkspace (wname);
    if (!wlist) return;

    newoccupation = twm_win->occupation & ~(1 << wlist->number);
    if (!newoccupation) return;
    ChangeOccupation (twm_win, newoccupation);
}

void ToggleOccupation (wname, twm_win)
char *wname;
TwmWindow *twm_win;
{
    WorkSpaceList *wlist;
    int newoccupation;

    if (! Scr->workSpaceManagerActive) return;
    if (twm_win->iconmgr)   return;
    if ((! Scr->TransientHasOccupation) && twm_win->transient) return;
    wlist = GetWorkspace (wname);
    if (!wlist) return;

    if (twm_win->occupation & (1 << wlist->number)) {
	newoccupation = twm_win->occupation & ~(1 << wlist->number);
    } else {
	newoccupation = twm_win->occupation | (1 << wlist->number);
    }
    if (!newoccupation) return;
    ChangeOccupation (twm_win, newoccupation);
}

static WorkSpaceList *GetWorkspace (wname)
char *wname;
{
    WorkSpaceList *wlist;

    if (!wname) return (NULL);
    for (wlist = Scr->workSpaceMgr.workSpaceList; wlist != NULL; wlist = wlist->next) {
	if (strcmp (wlist->label, wname) == 0) break;
    }
    if (wlist == NULL) {
	for (wlist = Scr->workSpaceMgr.workSpaceList; wlist != NULL; wlist = wlist->next) {
	    if (strcmp (wlist->name, wname) == 0) break;
	}
    }
    return (wlist);
}

void AllocateOthersIconManagers () {
    IconMgr		*p, *ip, *oldp, *oldv;
    WorkSpaceList	*wlist;

    if (! Scr->workSpaceManagerActive) return;

    oldp = Scr->iconmgr;
    for (wlist = Scr->workSpaceMgr.workSpaceList->next; wlist != NULL; wlist = wlist->next) {
	wlist->iconmgr  = (IconMgr *) malloc (sizeof (IconMgr));
	*wlist->iconmgr = *oldp;
	oldv = wlist->iconmgr;
	oldp->nextv = wlist->iconmgr;
	oldv->nextv = NULL;

	for (ip = oldp->next; ip != NULL; ip = ip->next) {
	    p  = (IconMgr *) malloc (sizeof (IconMgr));
	    *p = *ip;
	    ip->nextv  = p;
	    p->next    = NULL;
	    p->prev    = oldv;
	    p->nextv   = NULL;
	    oldv->next = p;
	    oldv = p;
        }
	for (ip = wlist->iconmgr; ip != NULL; ip = ip->next) {
	    ip->lasti = p;
        }
	oldp = wlist->iconmgr;
    }
    Scr->workSpaceMgr.workSpaceList->iconmgr = Scr->iconmgr;
}

static void Vanish (tmp_win)
TwmWindow *tmp_win;
{
    TwmWindow		*t;
    XWindowAttributes	winattrs;
    unsigned long	eventMask;

    if (tmp_win->UnmapByMovingFarAway) {
	XMoveWindow (dpy, tmp_win->frame, Scr->MyDisplayWidth + 1, Scr->MyDisplayHeight + 1);
    } else {
	XGetWindowAttributes(dpy, tmp_win->w, &winattrs);
	eventMask = winattrs.your_event_mask;
	/*
	* Prevent the receipt of an UnmapNotify, since that would
	* cause a transition to the Withdrawn state.
	*/
	XSelectInput    (dpy, tmp_win->w, eventMask & ~StructureNotifyMask);
	XUnmapWindow    (dpy, tmp_win->w);
	XUnmapWindow    (dpy, tmp_win->frame);
	XSelectInput    (dpy, tmp_win->w, eventMask);
	if (!tmp_win->DontSetInactive) SetMapStateProp (tmp_win, InactiveState);
    }
    if (tmp_win->icon_on && tmp_win->icon && tmp_win->icon->w) {
	XUnmapWindow (dpy, tmp_win->icon->w);
	IconDown (tmp_win);
    }
}

static void DisplayWin (tmp_win)
TwmWindow *tmp_win;
{
    XWindowAttributes	winattrs;
    unsigned long	eventMask;

    if (tmp_win->icon_on) {
	if (tmp_win->icon && tmp_win->icon->w) {
	    IconUp (tmp_win);
	    XMapWindow (dpy, tmp_win->icon->w);
	    return;
	}
    }
    if (tmp_win->UnmapByMovingFarAway) {
	XMoveWindow (dpy, tmp_win->frame, tmp_win->frame_x, tmp_win->frame_y);
    } else {
	if (!tmp_win->squeezed) {
	    XGetWindowAttributes(dpy, tmp_win->w, &winattrs);
	    eventMask = winattrs.your_event_mask;
	    XSelectInput (dpy, tmp_win->w, eventMask & ~StructureNotifyMask);
	    XMapWindow   (dpy, tmp_win->w);
	    XSelectInput (dpy, tmp_win->w, eventMask);
	}
	XMapWindow      (dpy, tmp_win->frame);
	SetMapStateProp (tmp_win, NormalState);
    }
}

void ChangeOccupation (tmp_win, newoccupation)
TwmWindow *tmp_win;
int newoccupation;
{
    TwmWindow	  *t;
    WorkSpaceList *wlist;
    int		  oldoccupation;
    char	  namelist [512];
    int		  len;
    int		  final_x, final_y;
    XWindowAttributes    winattrs;
    unsigned long	 eventMask;

    if ((newoccupation == 0) || /* in case the property has been broken by another client */
	(newoccupation == tmp_win->occupation)) {
	len = GetPropertyFromMask (tmp_win->occupation, namelist);
	XGetWindowAttributes(dpy, tmp_win->w, &winattrs);
	eventMask = winattrs.your_event_mask;
	XSelectInput(dpy, tmp_win->w, eventMask & ~PropertyChangeMask);

	XChangeProperty (dpy, tmp_win->w, _XA_WM_OCCUPATION, XA_STRING, 8, 
		     PropModeReplace, (unsigned char *) namelist, len);
	XSelectInput(dpy, tmp_win->w, eventMask);
	return;
    }
    oldoccupation = tmp_win->occupation;
    tmp_win->occupation = newoccupation & ~oldoccupation;
    AddIconManager (tmp_win);
    tmp_win->occupation = newoccupation;
    RemoveIconManager (tmp_win);
    if ((oldoccupation & (1 << Scr->workSpaceMgr.activeWSPC->number)) &&
        (! OCCUPY (tmp_win, Scr->workSpaceMgr.activeWSPC))) Vanish (tmp_win);
    if (! (oldoccupation & (1 << Scr->workSpaceMgr.activeWSPC->number)) &&
        OCCUPY (tmp_win, Scr->workSpaceMgr.activeWSPC)) DisplayWin (tmp_win);

    for (wlist = Scr->workSpaceMgr.workSpaceList; wlist != NULL; wlist = wlist->next) {
	if (oldoccupation & (1 << wlist->number)) {
	    if (!(newoccupation & (1 << wlist->number))) {
		RemoveWindowFromRegion (tmp_win);
		if (PlaceWindowInRegion (tmp_win, &final_x, &final_y))
		    XMoveWindow (dpy, tmp_win->frame, final_x, final_y);
	    }
	    break;
	}
    }
    len = GetPropertyFromMask (newoccupation, namelist);
    XGetWindowAttributes(dpy, tmp_win->w, &winattrs);
    eventMask = winattrs.your_event_mask;
    XSelectInput(dpy, tmp_win->w, eventMask & ~PropertyChangeMask);

    XChangeProperty (dpy, tmp_win->w, _XA_WM_OCCUPATION, XA_STRING, 8, 
		     PropModeReplace, (unsigned char *) namelist, len);
    XSelectInput(dpy, tmp_win->w, eventMask);

    if (Scr->workSpaceMgr.workspaceWindow.noshowoccupyall) {
	if (newoccupation == fullOccupation) {
	    for (wlist = Scr->workSpaceMgr.workSpaceList; wlist != NULL; wlist = wlist->next) {
		if ((oldoccupation & (1 << wlist->number)) != 0) {
		    WMapRemoveFromList (tmp_win, wlist);
		}
	    }
	    goto trans;
	}
	else
	if (oldoccupation == fullOccupation) {
	    for (wlist = Scr->workSpaceMgr.workSpaceList; wlist != NULL; wlist = wlist->next) {
		if ((newoccupation & (1 << wlist->number)) != 0) {
		    WMapAddToList (tmp_win, wlist);
		}
	    }
	    goto trans;
	}
    }
    for (wlist = Scr->workSpaceMgr.workSpaceList; wlist != NULL; wlist = wlist->next) {
	if (((oldoccupation & (1 << wlist->number)) != 0) &&
	    ((newoccupation & (1 << wlist->number)) == 0)) {
	    WMapRemoveFromList (tmp_win, wlist);
	}
	else
	if (((newoccupation & (1 << wlist->number)) != 0) &&
	    ((oldoccupation & (1 << wlist->number)) == 0)) {
	    WMapAddToList (tmp_win, wlist);
	}
    }
trans:
    if (! Scr->TransientHasOccupation) {
	for (t = Scr->TwmRoot.next; t != NULL; t = t->next) {
	    if ((t->transient && t->transientfor == tmp_win->w) ||
		((tmp_win->group == tmp_win->w) && (tmp_win->group == t->group) &&
		(tmp_win->group != t->w))) {
		ChangeOccupation (t, newoccupation);
	    }
	}
    }
}

void WmgrRedoOccupation (win)
TwmWindow *win;
{
    WorkSpaceList	*wlist;
    int			newoccupation;

    if (LookInList (Scr->OccupyAll, win->full_name, &win->class)) {
	newoccupation = fullOccupation;
    }
    else {
	newoccupation = 0;
	for (wlist = Scr->workSpaceMgr.workSpaceList; wlist != NULL; wlist = wlist->next) {
	    if (LookInList (wlist->clientlist, win->full_name, &win->class)) {
		newoccupation |= 1 << wlist->number;
	    }
	}
    }
    if (newoccupation != 0) ChangeOccupation (win, newoccupation);
}

void WMgrRemoveFromCurrentWosksace (win)
TwmWindow *win;
{
    WorkSpaceList *wlist;
    int		  newoccupation;

    wlist = Scr->workSpaceMgr.activeWSPC;
    if (! OCCUPY (win, wlist)) return;

    newoccupation = win->occupation & ~(1 << wlist->number);
    if (newoccupation == 0) return;

    ChangeOccupation (win, newoccupation);
}

void WMgrAddToCurrentWosksaceAndWarp (winname)
char *winname;
{
    TwmWindow *tw;
    int       newoccupation;

    for (tw = Scr->TwmRoot.next; tw != NULL; tw = tw->next) {
	if (match (winname, tw->full_name)) break;
    }
    if (!tw) {
	for (tw = Scr->TwmRoot.next; tw != NULL; tw = tw->next) {
	    if (match (winname, tw->class.res_name)) break;
	}
	if (!tw) {
	    for (tw = Scr->TwmRoot.next; tw != NULL; tw = tw->next) {
		if (match (winname, tw->class.res_class)) break;
	    }
	}
    }
    if (!tw) {
	XBell (dpy, 0);
	return;
    }
    if ((! Scr->WarpUnmapped) && (! tw->mapped)) {
	XBell (dpy, 0);
	return;
    }
    if (! OCCUPY (tw, Scr->workSpaceMgr.activeWSPC)) {
	newoccupation = tw->occupation | (1 << Scr->workSpaceMgr.activeWSPC->number);
	ChangeOccupation (tw, newoccupation);
    }

    if (! tw->mapped) DeIconify (tw);
    if (! Scr->NoRaiseWarp) RaiseWindow (tw);
    WarpToWindow (tw);
}

static void CreateWorkSpaceManagerWindow ()
{
    int		  mask;
    int		  lines, vspace, hspace, count, columns;
    int		  width, height, bwidth, bheight, wwidth, wheight;
    char	  *name, *icon_name, *geometry;
    int		  i, j;
    ColorPair	  cp;
    MyFont	  font;
    WorkSpaceList *wlist;
    int		  x, y, strWid, wid;
    unsigned long border;
    TwmWindow	  *tmp_win;
    XSetWindowAttributes	attr;
    XWindowAttributes		wattr;
    unsigned long		attrmask;
    XSizeHints	  sizehints;
    XWMHints	  wmhints;
    int		  gravity;
#ifdef I18N
	XRectangle inc_rect;
	XRectangle logical_rect;
#endif
    Scr->workSpaceMgr.workspaceWindow.buttonFont = Scr->IconManagerFont;
    Scr->workSpaceMgr.workspaceWindow.cp	 = Scr->IconManagerC;
#ifdef COLOR_BLIND_USER
    Scr->workSpaceMgr.workspaceWindow.cp.shadc   = Scr->White;
    Scr->workSpaceMgr.workspaceWindow.cp.shadd   = Scr->Black;
#else
    if (!Scr->BeNiceToColormap) GetShadeColors (&Scr->workSpaceMgr.workspaceWindow.cp);
#endif

    name      = Scr->workSpaceMgr.workspaceWindow.name;
    icon_name = Scr->workSpaceMgr.workspaceWindow.icon_name;
    geometry  = Scr->workSpaceMgr.workspaceWindow.geometry;
    columns   = Scr->workSpaceMgr.workspaceWindow.columns;
    vspace    = Scr->workSpaceMgr.workspaceWindow.vspace;
    hspace    = Scr->workSpaceMgr.workspaceWindow.hspace;
    font      = Scr->workSpaceMgr.workspaceWindow.buttonFont;
    cp        = Scr->workSpaceMgr.workspaceWindow.cp;
    border    = Scr->workSpaceMgr.workspaceWindow.defBorderColor;

    count = 0;
    for (wlist = Scr->workSpaceMgr.workSpaceList; wlist != NULL; wlist = wlist->next) count++;
    Scr->workSpaceMgr.count = count;

    if (columns == 0) {
	lines   = 2;
	columns = ((count - 1) / lines) + 1;
    }
    else {
	lines   = ((count - 1) / columns) + 1;
    }
    Scr->workSpaceMgr.workspaceWindow.lines     = lines;
    Scr->workSpaceMgr.workspaceWindow.columns   = columns;

    strWid = 0;
    for (wlist = Scr->workSpaceMgr.workSpaceList; wlist != NULL; wlist = wlist->next) {
#ifdef I18N
	XmbTextExtents(font.font_set, wlist->label, strlen (wlist->label),
		       &inc_rect, &logical_rect);
	wid = logical_rect.width;
#else
	wid = XTextWidth (font.font, wlist->label, strlen (wlist->label));
	if (wid > strWid) strWid = wid;
#endif
    }
    if (geometry != NULL) {
	mask = XParseGeometry (geometry, &x, &y, (unsigned int *) &width,
						 (unsigned int *) &height);
	bwidth  = (mask & WidthValue)  ?
			((width - (columns * hspace)) / columns) : strWid + 10;
	bheight = (mask & HeightValue) ?
			((height - (lines  * vspace)) / lines) : 22;
	width   = columns * (bwidth  + hspace);
	height  = lines   * (bheight + vspace);

	if (! (mask & YValue)) {
	    y = 0;
	    mask |= YNegative;
	}
	if (mask & XValue) {
	    if (mask & XNegative) {
		x += Scr->MyDisplayWidth  - width;
		gravity = (mask & YNegative) ? SouthEastGravity :
					       NorthEastGravity;
	    }
	    else {
		gravity = (mask & YNegative) ? SouthWestGravity :
					       NorthWestGravity;
	    }
	}
	else {
	    x  = (Scr->MyDisplayWidth - width) / 2;
	    gravity = (mask & YValue) ? ((mask & YNegative) ?
			SouthGravity : NorthGravity) : SouthGravity;
	}
	if (mask & YNegative) y += Scr->MyDisplayHeight - height;
    }
    else {
	bwidth  = strWid + 2 * Scr->WMgrButtonShadowDepth + 6;
	bheight = 22;
	width   = columns * (bwidth  + hspace);
	height  = lines   * (bheight + vspace);
	x       = (Scr->MyDisplayWidth - width) / 2;
	y       = Scr->MyDisplayHeight - height;
	gravity = NorthWestGravity;
    }
    wwidth  = width  / columns;
    wheight = height / lines;

    Scr->workSpaceMgr.workspaceWindow.width     = width;
    Scr->workSpaceMgr.workspaceWindow.height    = height;
    Scr->workSpaceMgr.workspaceWindow.bwidth    = bwidth;
    Scr->workSpaceMgr.workspaceWindow.bheight   = bheight;
    Scr->workSpaceMgr.workspaceWindow.wwidth    = wwidth;
    Scr->workSpaceMgr.workspaceWindow.wheight   = wheight;

    Scr->workSpaceMgr.workspaceWindow.w		= XCreateSimpleWindow (dpy, Scr->Root,
						x, y, width, height, 0, Scr->Black, cp.back);
    i = 0;
    j = 0;
    for (wlist = Scr->workSpaceMgr.workSpaceList; wlist != NULL; wlist = wlist->next) {
	wlist->buttonw = XCreateSimpleWindow (dpy, Scr->workSpaceMgr.workspaceWindow.w,
			i * (bwidth  + hspace) + (hspace / 2),
			j * (bheight + vspace) + (vspace / 2),
			bwidth, bheight, 0,
			Scr->Black, wlist->cp.back);

	wlist->mapSubwindow.x = i * wwidth;
	wlist->mapSubwindow.y = j * wheight;
	wlist->mapSubwindow.w = XCreateSimpleWindow (dpy, Scr->workSpaceMgr.workspaceWindow.w,
			wlist->mapSubwindow.x, wlist->mapSubwindow.y,
			wwidth - 2, wheight - 2, 1, border, wlist->cp.back);

	if (Scr->workSpaceMgr.workspaceWindow.state == BUTTONSSTATE)
	    XMapWindow (dpy, wlist->buttonw);
	else
	    XMapWindow (dpy, wlist->mapSubwindow.w);

	wlist->mapSubwindow.wl = NULL;
	if (useBackgroundInfo) {
	    if (wlist->image == None)
		XSetWindowBackground (dpy, wlist->mapSubwindow.w, wlist->backcp.back);
	    else
		XSetWindowBackgroundPixmap (dpy, wlist->mapSubwindow.w, wlist->image->pixmap);
	}
	else {
	    if (Scr->workSpaceMgr.workspaceWindow.defImage == None)
		XSetWindowBackground (dpy, wlist->mapSubwindow.w,
			Scr->workSpaceMgr.workspaceWindow.defColors.back);
	    else
		XSetWindowBackgroundPixmap (dpy, wlist->mapSubwindow.w,
			Scr->workSpaceMgr.workspaceWindow.defImage->pixmap);
	}
	XClearWindow (dpy, wlist->mapSubwindow.w);
	i++;
	if (i == columns) {i = 0; j++;};
    }

    sizehints.flags       = USPosition | PBaseSize | PMinSize | PResizeInc | PWinGravity;
    sizehints.x           = x;
    sizehints.y           = y;
    sizehints.base_width  = columns * hspace;
    sizehints.base_height = lines   * vspace;
    sizehints.width_inc   = columns;
    sizehints.height_inc  = lines;
    sizehints.min_width   = columns  * (hspace + 2);
    sizehints.min_height  = lines    * (vspace + 2);
    sizehints.win_gravity = gravity;

    XSetStandardProperties (dpy, Scr->workSpaceMgr.workspaceWindow.w,
			name, icon_name, None, NULL, 0, NULL);
    XSetWMSizeHints (dpy, Scr->workSpaceMgr.workspaceWindow.w, &sizehints, XA_WM_NORMAL_HINTS);

    wmhints.flags	  = InputHint | StateHint;
    wmhints.input         = True;
    wmhints.initial_state = NormalState;
    XSetWMHints (dpy, Scr->workSpaceMgr.workspaceWindow.w, &wmhints);
    tmp_win = AddWindow (Scr->workSpaceMgr.workspaceWindow.w, FALSE, Scr->iconmgr);
    if (! tmp_win) {
	fprintf (stderr, "cannot create workspace manager window, exiting...\n");
	exit (1);
    }
    tmp_win->occupation = fullOccupation;

    attrmask = 0;
    attr.cursor = Scr->ButtonCursor;
    attrmask |= CWCursor;
    attr.win_gravity = gravity;
    attrmask |= CWWinGravity;
    XChangeWindowAttributes (dpy, Scr->workSpaceMgr.workspaceWindow.w, attrmask, &attr);

    XGetWindowAttributes (dpy, Scr->workSpaceMgr.workspaceWindow.w, &wattr);
    attrmask = wattr.your_event_mask | KeyPressMask | KeyReleaseMask | ExposureMask;
    XSelectInput (dpy, Scr->workSpaceMgr.workspaceWindow.w, attrmask);

    for (wlist = Scr->workSpaceMgr.workSpaceList; wlist != NULL; wlist = wlist->next) {
	XSelectInput (dpy, wlist->buttonw, ButtonPressMask | ButtonReleaseMask | ExposureMask);
	XSaveContext (dpy, wlist->buttonw, TwmContext,    (XPointer) tmp_win);
	XSaveContext (dpy, wlist->buttonw, ScreenContext, (XPointer) Scr);

	XSelectInput (dpy, wlist->mapSubwindow.w, ButtonPressMask | ButtonReleaseMask);
	XSaveContext (dpy, wlist->mapSubwindow.w, TwmContext,    (XPointer) tmp_win);
	XSaveContext (dpy, wlist->mapSubwindow.w, ScreenContext, (XPointer) Scr);
    }
    SetMapStateProp (tmp_win, WithdrawnState);
    Scr->workSpaceMgr.workspaceWindow.twm_win = tmp_win;

    wlist = Scr->workSpaceMgr.workSpaceList;
    if (useBackgroundInfo && ! Scr->DontPaintRootWindow) {
	if (wlist->image == None)
	    XSetWindowBackground (dpy, Scr->Root, wlist->backcp.back);
	else
	    XSetWindowBackgroundPixmap (dpy, Scr->Root, wlist->image->pixmap);
	XClearWindow (dpy, Scr->Root);
    }
    PaintWorkSpaceManager ();
}

void WMgrHandleExposeEvent (event)
XEvent *event;
{
    WorkSpaceList *wlist;

    if (Scr->workSpaceMgr.workspaceWindow.state == BUTTONSSTATE) {
	for (wlist = Scr->workSpaceMgr.workSpaceList; wlist != NULL; wlist = wlist->next) {
	    if (event->xexpose.window == wlist->buttonw) break;
	}
	if (wlist == NULL) {
	    PaintWorkSpaceManagerBorder ();
	}
	else
	if (wlist == Scr->workSpaceMgr.activeWSPC)
	    PaintButton (WSPCWINDOW, wlist->buttonw, wlist->label, wlist->cp, on);
	else
	    PaintButton (WSPCWINDOW, wlist->buttonw, wlist->label, wlist->cp, off);
    }
    else {
	WinList	  wl;

        if (XFindContext (dpy, event->xexpose.window, MapWListContext,
		(XPointer *) &wl) == XCNOENT) return;
	if (wl && wl->twm_win && wl->twm_win->mapped) {
	    WMapRedrawName (wl);
	}
    }
}

void PaintWorkSpaceManager () {
    WorkSpaceList *wlist;

    PaintWorkSpaceManagerBorder ();
    for (wlist = Scr->workSpaceMgr.workSpaceList; wlist != NULL; wlist = wlist->next) {
	if (wlist == Scr->workSpaceMgr.activeWSPC)
	    PaintButton (WSPCWINDOW, wlist->buttonw, wlist->label, wlist->cp, on);
	else
	    PaintButton (WSPCWINDOW, wlist->buttonw, wlist->label, wlist->cp, off);
    }
}

static void PaintWorkSpaceManagerBorder () {
    int           width, height;

    width   = Scr->workSpaceMgr.workspaceWindow.width;
    height  = Scr->workSpaceMgr.workspaceWindow.height;

    Draw3DBorder (Scr->workSpaceMgr.workspaceWindow.w, 0, 0, width, height, 2,
			Scr->workSpaceMgr.workspaceWindow.cp, off, True, False);
}

ColorPair occupyButtoncp;

char *ok_string		= "OK",
     *cancel_string	= "Cancel",
     *everywhere_string	= "All";

static void CreateOccupyWindow () {
    int		  width, height, lines, columns, x, y;
    int		  bwidth, bheight, owidth, oheight, hspace, vspace;
    int		  min_bwidth, min_width;
    int		  i, j;
    Window	  w, OK, cancel, allworkspc;
    char	  *name, *icon_name;
    ColorPair	  cp;
    TwmWindow	  *tmp_win;
    WorkSpaceList *wlist;
    XSizeHints	  sizehints;
    XWMHints      wmhints;
    MyFont	  font;
    XSetWindowAttributes attr;
    XWindowAttributes	 wattr;
    unsigned long attrmask;
#ifdef I18N
    XRectangle inc_rect;
    XRectangle logical_rect;
#endif

    Scr->workSpaceMgr.occupyWindow.font     = Scr->IconManagerFont;
    Scr->workSpaceMgr.occupyWindow.cp       = Scr->IconManagerC;
#ifdef COLOR_BLIND_USER
    Scr->workSpaceMgr.occupyWindow.cp.shadc = Scr->White;
    Scr->workSpaceMgr.occupyWindow.cp.shadd = Scr->Black;
#else
    if (!Scr->BeNiceToColormap) GetShadeColors (&Scr->workSpaceMgr.occupyWindow.cp);
#endif
    name      = Scr->workSpaceMgr.occupyWindow.name;
    icon_name = Scr->workSpaceMgr.occupyWindow.icon_name;
    lines     = Scr->workSpaceMgr.workspaceWindow.lines;
    columns   = Scr->workSpaceMgr.workspaceWindow.columns;
    bwidth    = Scr->workSpaceMgr.workspaceWindow.bwidth;
    bheight   = Scr->workSpaceMgr.workspaceWindow.bheight;
    oheight   = Scr->workSpaceMgr.workspaceWindow.bheight;
    vspace    = Scr->workSpaceMgr.occupyWindow.vspace;
    hspace    = Scr->workSpaceMgr.occupyWindow.hspace;
    cp        = Scr->workSpaceMgr.occupyWindow.cp;

    height = ((bheight + vspace) * lines) + oheight + (2 * vspace);

    font       = Scr->workSpaceMgr.occupyWindow.font;
#ifdef I18N
    XmbTextExtents(font.font_set, ok_string, strlen(ok_string),
		       &inc_rect, &logical_rect);
    min_bwidth = logical_rect.width;
    XmbTextExtents(font.font_set, cancel_string, strlen (cancel_string),
		   &inc_rect, &logical_rect);
    i = logical_rect.width;
    if (i > min_bwidth) min_bwidth = i;
    XmbTextExtents(font.font_set,everywhere_string, strlen (everywhere_string),
		   &inc_rect, &logical_rect);
    i = logical_rect.width;
    if (i > min_bwidth) min_bwidth = i;
#else
    min_bwidth = XTextWidth (font.font, ok_string, strlen (ok_string));
    i = XTextWidth (font.font, cancel_string, strlen (cancel_string));
    if (i > min_bwidth) min_bwidth = i;
    i = XTextWidth (font.font, everywhere_string, strlen (everywhere_string));
    if (i > min_bwidth) min_bwidth = i;
#endif    
    min_bwidth = (min_bwidth + hspace); /* normal width calculation */
    width = columns * (bwidth  + hspace); 
    min_width = 3 * (min_bwidth + hspace); /* width by text width */

    if (columns < 3) {
	owidth = min_bwidth;
	if (width < min_width) width = min_width;
	bwidth = (width - columns * hspace) / columns;
    }
    else {
	owidth = min_bwidth + 2 * Scr->WMgrButtonShadowDepth + 2;
	width  = columns * (bwidth  + hspace);
    }
    Scr->workSpaceMgr.occupyWindow.lines      = lines;
    Scr->workSpaceMgr.occupyWindow.columns    = columns;
    Scr->workSpaceMgr.occupyWindow.width      = width;
    Scr->workSpaceMgr.occupyWindow.height     = height;
    Scr->workSpaceMgr.occupyWindow.bwidth     = bwidth;
    Scr->workSpaceMgr.occupyWindow.bheight    = bheight;
    Scr->workSpaceMgr.occupyWindow.owidth     = owidth;
    Scr->workSpaceMgr.occupyWindow.oheight    = oheight;

    w = XCreateSimpleWindow (dpy, Scr->Root, 0, 0, width, height, 1,
			Scr->Black, cp.back);
    Scr->workSpaceMgr.occupyWindow.w = w;
    i = 0;
    j = 0;
    for (wlist = Scr->workSpaceMgr.workSpaceList; wlist != NULL; wlist = wlist->next) {
	wlist->obuttonw = XCreateSimpleWindow(dpy, w,
		       i * (bwidth  + hspace) + (hspace / 2),
		       j * (bheight + vspace) + (vspace / 2), bwidth, bheight, 0,
		       Scr->Black, wlist->cp.back);
	XMapWindow (dpy, wlist->obuttonw);
	i++;
	if (i == columns) {i = 0; j++;}
    }
    GetColor (Scr->Monochrome, &(occupyButtoncp.back), "gray50");
    occupyButtoncp.fore = Scr->White;
    if (!Scr->BeNiceToColormap) GetShadeColors (&occupyButtoncp);

    hspace = (width - 3 * owidth) / 3;
    x = hspace / 2;
    y = ((bheight + vspace) * lines) + ((3 * vspace) / 2);
    OK         = XCreateSimpleWindow(dpy, w, x, y, owidth, oheight, 0,
			Scr->Black, occupyButtoncp.back);
    XMapWindow (dpy, OK);
    x += owidth + vspace;
    cancel     = XCreateSimpleWindow(dpy, w, x, y, owidth, oheight, 0,
			Scr->Black, occupyButtoncp.back);
    XMapWindow (dpy, cancel);
    x += owidth + vspace;
    allworkspc = XCreateSimpleWindow(dpy, w, x, y, owidth, oheight, 0,
			Scr->Black, occupyButtoncp.back);
    XMapWindow (dpy, allworkspc);

    Scr->workSpaceMgr.occupyWindow.OK         = OK;
    Scr->workSpaceMgr.occupyWindow.cancel     = cancel;
    Scr->workSpaceMgr.occupyWindow.allworkspc = allworkspc;

    sizehints.flags       = PBaseSize | PMinSize | PResizeInc;
    sizehints.base_width  = columns;
    sizehints.base_height = lines;
    sizehints.width_inc   = columns;
    sizehints.height_inc  = lines;
    sizehints.min_width   = 2 * columns;
    sizehints.min_height  = 2 * lines;
    XSetStandardProperties (dpy, w, name, icon_name, None, NULL, 0, &sizehints);

    wmhints.flags	  = InputHint | StateHint;
    wmhints.input         = True;
    wmhints.initial_state = NormalState;
    XSetWMHints (dpy, w, &wmhints);
    tmp_win = AddWindow (w, FALSE, Scr->iconmgr);
    if (! tmp_win) {
	fprintf (stderr, "cannot create occupy window, exiting...\n");
	exit (1);
    }
    tmp_win->occupation = 0;

    attrmask = 0;
    attr.cursor = Scr->ButtonCursor;
    attrmask |= CWCursor;
    XChangeWindowAttributes (dpy, w, attrmask, &attr);

    XGetWindowAttributes (dpy, w, &wattr);
    attrmask = wattr.your_event_mask | KeyPressMask | KeyReleaseMask | ExposureMask;
    XSelectInput (dpy, w, attrmask);

    for (wlist = Scr->workSpaceMgr.workSpaceList; wlist != NULL; wlist = wlist->next) {
	XSelectInput (dpy, wlist->obuttonw, ButtonPressMask | ButtonReleaseMask);
	XSaveContext (dpy, wlist->obuttonw, TwmContext,    (XPointer) tmp_win);
	XSaveContext (dpy, wlist->obuttonw, ScreenContext, (XPointer) Scr);
    }
    XSelectInput (dpy, Scr->workSpaceMgr.occupyWindow.OK, ButtonPressMask | ButtonReleaseMask);
    XSaveContext (dpy, Scr->workSpaceMgr.occupyWindow.OK, TwmContext,    (XPointer) tmp_win);
    XSaveContext (dpy, Scr->workSpaceMgr.occupyWindow.OK, ScreenContext, (XPointer) Scr);
    XSelectInput (dpy, Scr->workSpaceMgr.occupyWindow.cancel, ButtonPressMask | ButtonReleaseMask);
    XSaveContext (dpy, Scr->workSpaceMgr.occupyWindow.cancel, TwmContext,    (XPointer) tmp_win);
    XSaveContext (dpy, Scr->workSpaceMgr.occupyWindow.cancel, ScreenContext, (XPointer) Scr);
    XSelectInput (dpy, Scr->workSpaceMgr.occupyWindow.allworkspc, ButtonPressMask | ButtonReleaseMask);
    XSaveContext (dpy, Scr->workSpaceMgr.occupyWindow.allworkspc, TwmContext,    (XPointer) tmp_win);
    XSaveContext (dpy, Scr->workSpaceMgr.occupyWindow.allworkspc, ScreenContext, (XPointer) Scr);

    SetMapStateProp (tmp_win, WithdrawnState);
    Scr->workSpaceMgr.occupyWindow.twm_win = tmp_win;
}

void PaintOccupyWindow () {
    WorkSpaceList *wlist;
    OccupyWindow  *occupyW;
    int 	  width, height;

    width   = Scr->workSpaceMgr.occupyWindow.width;
    height  = Scr->workSpaceMgr.occupyWindow.height;

    Draw3DBorder (Scr->workSpaceMgr.occupyWindow.w, 0, 0, width, height, 2,
			Scr->workSpaceMgr.occupyWindow.cp, off, True, False);

    occupyW = &Scr->workSpaceMgr.occupyWindow;
    for (wlist = Scr->workSpaceMgr.workSpaceList; wlist != NULL; wlist = wlist->next) {
	if (occupyW->tmpOccupation & (1 << wlist->number))
	    PaintButton (OCCUPYWINDOW, wlist->obuttonw, wlist->label, wlist->cp, on);
	else
	    PaintButton (OCCUPYWINDOW, wlist->obuttonw, wlist->label, wlist->cp, off);
    }
    PaintButton (OCCUPYBUTTON, Scr->workSpaceMgr.occupyWindow.OK,
		ok_string, occupyButtoncp, off);
    PaintButton (OCCUPYBUTTON, Scr->workSpaceMgr.occupyWindow.cancel,
		cancel_string, occupyButtoncp, off);
    PaintButton (OCCUPYBUTTON, Scr->workSpaceMgr.occupyWindow.allworkspc,
		everywhere_string, occupyButtoncp, off);
}

static void PaintButton (which, w, label, cp, state)
int       which;
Window    w;
char      *label;
ColorPair cp;
int       state;
{
    int        bwidth, bheight;
    MyFont     font;
    int        strWid, strHei, hspace, vspace;
#ifdef I18N
    XFontSetExtents *font_extents;
    XFontStruct **xfonts;
    char **font_names;
    register int i;
    int descent;
    XRectangle inc_rect;
    XRectangle logical_rect;
    int fnum;
#endif
    
    if (which == WSPCWINDOW) {
	bwidth  = Scr->workSpaceMgr.workspaceWindow.bwidth;
	bheight = Scr->workSpaceMgr.workspaceWindow.bheight;
	font    = Scr->workSpaceMgr.workspaceWindow.buttonFont;
    }
    else
    if (which == OCCUPYWINDOW) {
	bwidth  = Scr->workSpaceMgr.occupyWindow.bwidth;
	bheight = Scr->workSpaceMgr.occupyWindow.bheight;
	font    = Scr->workSpaceMgr.occupyWindow.font;
    }
    else
    if (which == OCCUPYBUTTON) {
	bwidth  = Scr->workSpaceMgr.occupyWindow.owidth;
	bheight = Scr->workSpaceMgr.occupyWindow.oheight;
	font    = Scr->workSpaceMgr.occupyWindow.font;
    }
    else return;

#ifdef I18N
    font_extents = XExtentsOfFontSet(font.font_set);

    strHei = font_extents->max_logical_extent.height;

    vspace = ((bheight + strHei) / 2) - font.descent;

    XmbTextExtents(font.font_set, label, strlen (label),
		   &inc_rect, &logical_rect);
    strWid = logical_rect.width;
#else
    strHei = font.font->max_bounds.ascent + font.font->max_bounds.descent;
    vspace = ((bheight + strHei) / 2) - font.font->max_bounds.descent;
    strWid = XTextWidth (font.font, label, strlen (label));
#endif
    hspace = (bwidth - strWid) / 2;
    if (hspace < (Scr->WMgrButtonShadowDepth + 1)) hspace = Scr->WMgrButtonShadowDepth + 1;
    XClearWindow (dpy, w);

    if (Scr->Monochrome == COLOR) {
	Draw3DBorder (w, 0, 0, bwidth, bheight, Scr->WMgrButtonShadowDepth,
			cp, state, True, False);

	switch (Scr->workSpaceMgr.workspaceWindow.buttonStyle) {
	    case STYLE_NORMAL :
		break;

	    case STYLE_STYLE1 :
		Draw3DBorder (w,
			Scr->WMgrButtonShadowDepth - 1,
			Scr->WMgrButtonShadowDepth - 1,
			bwidth  - 2 * Scr->WMgrButtonShadowDepth + 2,
			bheight - 2 * Scr->WMgrButtonShadowDepth + 2,
			1, cp, (state == on) ? off : on, True, False);
		break;

	    case STYLE_STYLE2 :
		Draw3DBorder (w,
			Scr->WMgrButtonShadowDepth / 2,
			Scr->WMgrButtonShadowDepth / 2,
			bwidth  - Scr->WMgrButtonShadowDepth,
			bheight - Scr->WMgrButtonShadowDepth,
			1, cp, (state == on) ? off : on, True, False);
		break;

	    case STYLE_STYLE3 :
		Draw3DBorder (w,
			1,
			1,
			bwidth  - 2,
			bheight - 2,
			1, cp, (state == on) ? off : on, True, False);
		break;
	}
#ifdef I18N
	FB (cp.fore, cp.back);
	XmbDrawString (dpy, w, font.font_set, Scr->NormalGC, hspace, vspace, label, strlen (label));
#else
	FBF (cp.fore, cp.back, font.font->fid);
	XDrawString (dpy, w, Scr->NormalGC, hspace, vspace, label, strlen (label));
#endif
    }
    else {
	Draw3DBorder (w, 0, 0, bwidth, bheight, Scr->WMgrButtonShadowDepth,
		cp, state, True, False);
	if (state == on) {
#ifdef I18N
	    FB (cp.fore, cp.back);
	    XmbDrawImageString (dpy, w, font.font_set, Scr->NormalGC, hspace, vspace, label, strlen (label));
#else
	    FBF (cp.fore, cp.back, font.font->fid);
	    XDrawImageString (dpy, w, Scr->NormalGC, hspace, vspace, label, strlen (label));
#endif
	}
	else {
#ifdef I18N
	    FB (cp.back, cp.fore);
	    XmbDrawImageString (dpy, w, font.font_set, Scr->NormalGC, hspace, vspace, label, strlen (label));
#else
	    FBF (cp.back, cp.fore, font.font->fid);
	    XDrawImageString (dpy, w, Scr->NormalGC, hspace, vspace, label, strlen (label));
#endif
	}
    }
}

static unsigned int GetMaskFromResource (win, res)
TwmWindow *win;
char      *res;
{
    char          *name;
    char          wrkSpcName [64];
    WorkSpaceList *wlist;
    int           mask, num, mode;

    mode = 0;
    if (*res == '+') {
	mode = 1;
	res++;
    }
    else
    if (*res == '-') {
	mode = 2;
	res++;
    }
    mask = 0;
    while (*res != '\0') {
	while (*res == ' ') res++;
	if (*res == '\0') break;
	name = wrkSpcName;
	while ((*res != '\0') && (*res != ' ')) {
	    if (*res == '\\') res++;
	    *name = *res;
	    name++; res++;
	}
	*name = '\0';
	if (strcmp (wrkSpcName, "all") == 0) {
	    mask = fullOccupation;
	    break;
	}
	if (strcmp (wrkSpcName, "current") == 0) {
	    mask |= (1 << Scr->workSpaceMgr.activeWSPC->number);
	    continue;
	}
	num  = 0;
	for (wlist = Scr->workSpaceMgr.workSpaceList; wlist != NULL; wlist = wlist->next) {
	    if (strcmp (wrkSpcName, wlist->label) == 0) break;
	    num++;
	}
	if (wlist != NULL) mask |= (1 << num);
	else {
	    twmrc_error_prefix ();
	    fprintf (stderr, "unknown workspace : %s\n", wrkSpcName);
	}
    }
    switch (mode) {
	case 0 :
	    return (mask);
	case 1 :
	    return (mask | win->occupation);
	case 2 :
	    return (win->occupation & ~mask);
    }
}

unsigned int GetMaskFromProperty (prop, len)
char *prop;
unsigned long len;
{
    char          wrkSpcName [256];
    WorkSpaceList *wlist;
    unsigned int  mask;
    int           num, l;

    mask = 0;
    l = 0;
    while (l < len) {
	strcpy (wrkSpcName, prop);
	l    += strlen (prop) + 1;
	prop += strlen (prop) + 1;
	if (strcmp (wrkSpcName, "all") == 0) {
	    mask = fullOccupation;
	    break;
	}
	num = 0;
	for (wlist = Scr->workSpaceMgr.workSpaceList; wlist != NULL; wlist = wlist->next) {
	    if (strcmp (wrkSpcName, wlist->label) == 0) break;
	    num++;
	}
	if (wlist == NULL) {
	    fprintf (stderr, "unknown workspace : %s\n", wrkSpcName);
	}
	else {
	    mask |= (1 << num);
	}
    }
    return (mask);
}

static int GetPropertyFromMask (mask, prop)
unsigned int mask;
char *prop;
{
    WorkSpaceList *wlist;
    int           len;
    char	  *p;

    if (mask == fullOccupation) {
	strcpy (prop, "all");
	return (3);
    }
    len = 0;
    p = prop;
    for (wlist = Scr->workSpaceMgr.workSpaceList; wlist != NULL; wlist = wlist->next) {
	if (mask & (1 << wlist->number)) {
	    strcpy (p, wlist->label);
	    p   += strlen (wlist->label) + 1;
	    len += strlen (wlist->label) + 1;
	}
    }
    return (len);
}

void AddToClientsList (workspace, client)
char *workspace, *client;
{
    WorkSpaceList *wlist;

    if (strcmp (workspace, "all") == 0) {
	for (wlist = Scr->workSpaceMgr.workSpaceList; wlist != NULL; wlist = wlist->next) {
	    AddToList (&wlist->clientlist, client, "");
	}
	return;
    }

    for (wlist = Scr->workSpaceMgr.workSpaceList; wlist != NULL; wlist = wlist->next) {
	if (strcmp (wlist->label, workspace) == 0) break;
    }
    if (wlist == NULL) return;
    AddToList (&wlist->clientlist, client, "");    
}

void WMapToggleState () {
    if (Scr->workSpaceMgr.workspaceWindow.state == BUTTONSSTATE) {
	WMapSetMapState ();
    }
    else {
	WMapSetButtonsState ();
    }
}

void WMapSetMapState () {
    WorkSpaceList *wlist;

    if (Scr->workSpaceMgr.workspaceWindow.state == MAPSTATE) return;
    for (wlist = Scr->workSpaceMgr.workSpaceList; wlist != NULL; wlist = wlist->next) {
	XUnmapWindow (dpy, wlist->buttonw);
	XMapWindow   (dpy, wlist->mapSubwindow.w);
    }
    Scr->workSpaceMgr.workspaceWindow.state = MAPSTATE;
    MaybeAnimate = True;
}

void WMapSetButtonsState () {
    WorkSpaceList *wlist;

    if (Scr->workSpaceMgr.workspaceWindow.state == BUTTONSSTATE) return;
    for (wlist = Scr->workSpaceMgr.workSpaceList; wlist != NULL; wlist = wlist->next) {
	XUnmapWindow (dpy, wlist->mapSubwindow.w);
	XMapWindow   (dpy, wlist->buttonw);
    }
    Scr->workSpaceMgr.workspaceWindow.state = BUTTONSSTATE;
}

void WMapAddWindow (win)
TwmWindow *win;
{
    WorkSpaceList *wlist;

    if (win->iconmgr) return;
    if (strcmp (win->name, Scr->workSpaceMgr.workspaceWindow.name) == 0) return;
    if (strcmp (win->name, Scr->workSpaceMgr.occupyWindow.name) == 0) return;
    if ((Scr->workSpaceMgr.workspaceWindow.noshowoccupyall) &&
	(win->occupation == fullOccupation)) return;
    for (wlist = Scr->workSpaceMgr.workSpaceList; wlist != NULL; wlist = wlist->next) {
	if (OCCUPY (win, wlist)) WMapAddToList (win, wlist);
    }
}

void WMapDestroyWindow (win)
TwmWindow *win;
{
    WorkSpaceList *wlist;

    for (wlist = Scr->workSpaceMgr.workSpaceList; wlist != NULL; wlist = wlist->next) {
	if (OCCUPY (win, wlist)) WMapRemoveFromList (win, wlist);
    }
    if (win == occupyWin) {
	OccupyWindow *occupyW = &Scr->workSpaceMgr.occupyWindow;

	Vanish (occupyW->twm_win);
	occupyW->twm_win->mapped = FALSE;
	occupyW->twm_win->occupation = 0;
	occupyWin = (TwmWindow*) 0;
    }
}

void WMapMapWindow (win)
TwmWindow *win;
{
    WorkSpaceList *wlist;
    WinList wl;

    for (wlist = Scr->workSpaceMgr.workSpaceList; wlist != NULL; wlist = wlist->next) {
	for (wl = wlist->mapSubwindow.wl; wl != NULL; wl = wl->next) {
	    if (wl->twm_win == win) {
		XMapWindow (dpy, wl->w);
		WMapRedrawName (wl);
		break;
	    }
	}
    }
}

void WMapSetupWindow (win, x, y, w, h)
TwmWindow *win;
int x, y, w, h;
{
    WorkSpaceList *wlist;
    WinList	  wl;
    float	  wf, hf;

    if (win->iconmgr) return;
    if (strcmp (win->name, Scr->workSpaceMgr.workspaceWindow.name) == 0) {
	Scr->workSpaceMgr.workspaceWindow.x = x;
	Scr->workSpaceMgr.workspaceWindow.y = y;
	if (w == -1) return;
	ResizeWorkSpaceManager (win);
	return;
    }
    if (strcmp (win->name, Scr->workSpaceMgr.occupyWindow.name) == 0) {
	Scr->workSpaceMgr.occupyWindow.x = x;
	Scr->workSpaceMgr.occupyWindow.y = y;
	if (w == -1) return;
	ResizeOccupyWindow (win);
	return;
    }
    wf = (float) (Scr->workSpaceMgr.workspaceWindow.wwidth  - 2) /
		(float) Scr->MyDisplayWidth;
    hf = (float) (Scr->workSpaceMgr.workspaceWindow.wheight - 2) /
		(float) Scr->MyDisplayHeight;
    for (wlist = Scr->workSpaceMgr.workSpaceList; wlist != NULL; wlist = wlist->next) {
	for (wl = wlist->mapSubwindow.wl; wl != NULL; wl = wl->next) {
	    if (win == wl->twm_win) {
		wl->x = (int) (x * wf);
		wl->y = (int) (y * hf);
		if (w == -1) {
		    XMoveWindow (dpy, wl->w, wl->x, wl->y);
		}
		else {
		    wl->width  = (unsigned int) ((w * wf) + 0.5);
		    wl->height = (unsigned int) ((h * hf) + 0.5);
		    if (!Scr->use3Dwmap) {
			wl->width  -= 2;
			wl->height -= 2;
		    }
		    if (wl->width  < 1) wl->width  = 1;
		    if (wl->height < 1) wl->height = 1;
		    XMoveResizeWindow (dpy, wl->w, wl->x, wl->y, wl->width, wl->height);
		}
		break;
	    }
	}
    }
}

void WMapIconify (win)
TwmWindow *win;
{
    WorkSpaceList *wlist;
    WinList    wl;

    for (wlist = Scr->workSpaceMgr.workSpaceList; wlist != NULL; wlist = wlist->next) {
	for (wl = wlist->mapSubwindow.wl; wl != NULL; wl = wl->next) {
	    if (win == wl->twm_win) {
		XUnmapWindow (dpy, wl->w);
		break;
	    }
	}
    }
}

void WMapDeIconify (win)
TwmWindow *win;
{
    WorkSpaceList *wlist;
    WinList    wl;

    for (wlist = Scr->workSpaceMgr.workSpaceList; wlist != NULL; wlist = wlist->next) {
	for (wl = wlist->mapSubwindow.wl; wl != NULL; wl = wl->next) {
	    if (win == wl->twm_win) {
		if (Scr->NoRaiseDeicon)
		    XMapWindow (dpy, wl->w);
		else
		    XMapRaised (dpy, wl->w);
		WMapRedrawName (wl);
		break;
	    }
	}
    }
}

void WMapRaiseLower (win)
TwmWindow *win;
{
    WorkSpaceList *wlist;

    for (wlist = Scr->workSpaceMgr.workSpaceList; wlist != NULL; wlist = wlist->next) {
	if (OCCUPY (win, wlist)) WMapRestack (wlist);
    }
}

void WMapLower (win)
TwmWindow *win;
{
    WorkSpaceList *wlist;

    for (wlist = Scr->workSpaceMgr.workSpaceList; wlist != NULL; wlist = wlist->next) {
	if (OCCUPY (win, wlist)) WMapRestack (wlist);
    }
}

void WMapRaise (win)
TwmWindow *win;
{
    WorkSpaceList *wlist;

    for (wlist = Scr->workSpaceMgr.workSpaceList; wlist != NULL; wlist = wlist->next) {
	if (OCCUPY (win, wlist)) WMapRestack (wlist);
    }
}

void WMapRestack (wlist)
WorkSpaceList *wlist;
{
    TwmWindow	*win;
    WinList	wl;
    Window	root;
    Window	parent;
    Window	*children, *smallws;
    unsigned int number;
    int		i, j;

    number = 0;
    XQueryTree (dpy, Scr->Root, &root, &parent, &children, &number);
    smallws = (Window*) malloc (number * sizeof (Window));

    j = 0;
    for (i = number - 1; i >= 0; i--) {
	if (XFindContext (dpy, children [i], TwmContext, (XPointer *) &win) == XCNOENT) {
	    continue;
	}
	if (win->frame != children [i]) continue; /* skip icons */
	if (! OCCUPY (win, wlist)) continue;
	if (tracefile) {
	    fprintf (tracefile, "WMapRestack : w = %x, win = %x\n", children [i], win);
	    fflush (tracefile);
	}
	for (wl = wlist->mapSubwindow.wl; wl != NULL; wl = wl->next) {
	    if (win == wl->twm_win) {
		smallws [j++] = wl->w;
		break;
	    }
	}
    }
    XRestackWindows (dpy, smallws, j);

    XFree ((char *) children);
    free  (smallws);
    return;
}

void WMapUpdateIconName (win)
TwmWindow *win;
{
    WorkSpaceList *wlist;
    WinList    wl;

    for (wlist = Scr->workSpaceMgr.workSpaceList; wlist != NULL; wlist = wlist->next) {
	for (wl = wlist->mapSubwindow.wl; wl != NULL; wl = wl->next) {
	    if (win == wl->twm_win) {
		WMapRedrawName (wl);
		break;
	    }
	}
    }
}

void WMgrHandleKeyReleaseEvent (event)
XEvent *event;
{
    char	*keyname;
    KeySym	keysym;

    keysym  = XLookupKeysym ((XKeyEvent*) event, 0);
    if (! keysym) return;
    keyname = XKeysymToString (keysym);
    if (! keyname) return;
    if ((strcmp (keyname, "Control_R") == 0) || (strcmp (keyname, "Control_L") == 0)) {
	WMapToggleState ();
	return;
    }
}

void WMgrHandleKeyPressEvent (event)
XEvent *event;
{
    WorkSpaceList	*wlist;
    int			len, i, lname;
    char		key [16], k;
    char		name [128];
    char		*keyname;
    KeySym		keysym;

    keysym  = XLookupKeysym   ((XKeyEvent*) event, 0);
    if (! keysym) return;
    keyname = XKeysymToString (keysym);
    if (! keyname) return;
    if ((strcmp (keyname, "Control_R") == 0) || (strcmp (keyname, "Control_L") == 0)) {
	WMapToggleState ();
	return;
    }
    if (Scr->workSpaceMgr.workspaceWindow.state == MAPSTATE) return;

    for (wlist = Scr->workSpaceMgr.workSpaceList; wlist != NULL; wlist = wlist->next) {
	if (wlist->buttonw == event->xkey.subwindow) break;
    }
    if (wlist == NULL) return;

    strcpy (name, wlist->label);
    lname = strlen (name);
    len   = XLookupString (&(event->xkey), key, 16, NULL, NULL);
    for (i = 0; i < len; i++) {
        k = key [i];
	if (isprint (k)) {
	    name [lname++] = k;
	}
	else
	if ((k == 127) || (k == 8)) {
	    if (lname != 0) lname--;
	}
	else
	    break;
    }
    name [lname] = '\0';
    wlist->label = realloc (wlist->label, (strlen (name) + 1));
    strcpy (wlist->label, name);
    if (wlist == Scr->workSpaceMgr.activeWSPC)
	PaintButton (WSPCWINDOW, wlist->buttonw, wlist->label, wlist->cp,  on);
    else
	PaintButton (WSPCWINDOW, wlist->buttonw, wlist->label, wlist->cp, off);
}

void WMgrHandleButtonEvent (event)
XEvent *event;
{
    WorkSpaceWindow	*mw;
    WorkSpaceList	*wlist, *oldwlist, *newwlist, *cwlist;
    WinList		wl;
    TwmWindow		*win;
    int			occupation;
    unsigned int	W0, H0, bw;
    int			cont;
    XEvent		ev;
    Window		w, sw, parent;
    int			X0, Y0, X1, Y1, XW, YW, XSW, YSW;
    Position		newX, newY, winX, winY;
    Window		junkW;
    unsigned int	junk;
    unsigned int	button;
    unsigned int	modifier;
    XSetWindowAttributes attrs;
    unsigned long	eventMask;
    float		wf, hf;
    Boolean		alreadyvivible, realmovemode, startincurrent;
    Time		etime;

    parent   = event->xbutton.window;
    sw       = event->xbutton.subwindow;
    mw       = &(Scr->workSpaceMgr.workspaceWindow);
    button   = event->xbutton.button;
    modifier = event->xbutton.state;
    etime    = event->xbutton.time;

    if (Scr->workSpaceMgr.workspaceWindow.state == BUTTONSSTATE) {
	for (wlist = Scr->workSpaceMgr.workSpaceList; wlist != NULL; wlist = wlist->next) {
	    if (wlist->buttonw == parent) break;
	}
	if (wlist == NULL) return;
	GotoWorkSpace (wlist);
	return;
    }

    for (wlist = Scr->workSpaceMgr.workSpaceList; wlist != NULL; wlist = wlist->next) {
	if (wlist->mapSubwindow.w == parent) break;
    }
    if (wlist == NULL) return;
    if (sw == (Window) 0) {
	GotoWorkSpace (wlist);
	return;
    }
    oldwlist = wlist;

    if (XFindContext (dpy, sw, MapWListContext, (XPointer *) &wl) == XCNOENT) return;
    win = wl->twm_win;
    if ((! Scr->TransientHasOccupation) && win->transient) return;

    XTranslateCoordinates (dpy, Scr->Root, sw, event->xbutton.x_root, event->xbutton.y_root,
				&XW, &YW, &junkW);
    realmovemode = ( Scr->ReallyMoveInWorkspaceManager && !(modifier & ShiftMask)) ||
		   (!Scr->ReallyMoveInWorkspaceManager &&  (modifier & ShiftMask));
    startincurrent = (oldwlist == Scr->workSpaceMgr.activeWSPC);
    if (win->OpaqueMove) {
	int sw, ss;

	sw = win->frame_width * win->frame_height;
	ss = Scr->MyDisplayWidth  * Scr->MyDisplayHeight;
	if (sw > ((ss * Scr->OpaqueMoveThreshold) / 100))
	    Scr->OpaqueMove = FALSE;
	else
	    Scr->OpaqueMove = TRUE;
    } else {
	Scr->OpaqueMove = FALSE;
    }
    switch (button) {
	case 1 :
	    XUnmapWindow (dpy, sw);

	case 2 :
	    XGetGeometry (dpy, sw, &junkW, &X0, &Y0, &W0, &H0, &bw, &junk);
	    XTranslateCoordinates (dpy, oldwlist->mapSubwindow.w,
				mw->w, X0, Y0, &X1, &Y1, &junkW);

	    attrs.event_mask       = ExposureMask;
	    attrs.background_pixel = wl->cp.back;
	    attrs.border_pixel     = wl->cp.back;
	    w = XCreateWindow (dpy, mw->w, X1, Y1, W0, H0, bw,
				CopyFromParent,
				(unsigned int) CopyFromParent,
				(Visual *) CopyFromParent,
				CWEventMask | CWBackPixel | CWBorderPixel, &attrs);

	    XMapRaised (dpy, w);
	    WMapRedrawWindow (w, W0, H0, wl->cp, wl->twm_win->icon_name);
	    if (realmovemode && Scr->ShowWinWhenMovingInWmgr) {
		if (Scr->OpaqueMove) {
		    DisplayWin (win);
		} else {
		    MoveOutline (Scr->Root,
			win->frame_x - win->frame_bw,
			win->frame_y - win->frame_bw,
			win->frame_width  + 2 * win->frame_bw,
			win->frame_height + 2 * win->frame_bw,
			win->frame_bw,
			win->title_height + win->frame_bw3D);
		}
	    }
	    break;

	case 3 :
	    occupation = win->occupation & ~(1 << oldwlist->number);
	    ChangeOccupation (win, occupation);
	    return;
	default :
	    return;
    }

    wf = (float) (mw->wwidth  - 1) / (float) Scr->MyDisplayWidth;
    hf = (float) (mw->wheight - 1) / (float) Scr->MyDisplayHeight;
    XGrabPointer (dpy, mw->w, False, ButtonPressMask | ButtonMotionMask | ButtonReleaseMask,
		  GrabModeAsync, GrabModeAsync, mw->w, Scr->MoveCursor, CurrentTime);

    alreadyvivible = False;
    cont = TRUE;
    while (cont) {
	XMaskEvent (dpy, ButtonPressMask | ButtonMotionMask |
			 ButtonReleaseMask | ExposureMask, &ev);
	switch (ev.xany.type) {
	    case ButtonPress :
	    case ButtonRelease :
		if (ev.xbutton.button != button) break;
		cont = FALSE;
		newX = ev.xbutton.x;
		newY = ev.xbutton.y;

	    case MotionNotify :
		if (cont) {
		    newX = ev.xmotion.x;
		    newY = ev.xmotion.y;
		}
		if (realmovemode) {
		    for (cwlist = Scr->workSpaceMgr.workSpaceList;
			 cwlist != NULL;
			 cwlist = cwlist->next) {
			if ((newX >= cwlist->mapSubwindow.x) &&
			    (newX <  cwlist->mapSubwindow.x + mw->wwidth) &&
			    (newY >= cwlist->mapSubwindow.y) &&
			    (newY <  cwlist->mapSubwindow.y + mw->wheight)) {
			    break;
			}
		    }
		    if (!cwlist) break;
		    winX = newX - XW;
		    winY = newY - YW;
		    XTranslateCoordinates (dpy, mw->w, cwlist->mapSubwindow.w,
					winX, winY, &XSW, &YSW, &junkW);
		    winX = (int) (XSW / wf);
		    winY = (int) (YSW / hf);
		    if (Scr->DontMoveOff) {
			int w = win->frame_width;
			int h = win->frame_height;

			if ((winX < Scr->BorderLeft) &&
                            ((Scr->MoveOffResistance < 0) ||
                             (winX > Scr->BorderLeft - Scr->MoveOffResistance)))
                        {
			    winX = Scr->BorderLeft;
			    newX = cwlist->mapSubwindow.x + XW +
                                Scr->BorderLeft * mw->wwidth /
                                Scr->MyDisplayWidth;
			}
			if (((winX + w) >
                             Scr->MyDisplayWidth - Scr->BorderRight) &&
			    ((Scr->MoveOffResistance < 0) ||
                             ((winX + w) < Scr->MyDisplayWidth -
                              Scr->BorderRight + Scr->MoveOffResistance)))
                        {
			    winX = Scr->MyDisplayWidth - Scr->BorderRight - w;
			    newX = cwlist->mapSubwindow.x + mw->wwidth *
                                (1 - Scr->BorderRight / (double) Scr->MyDisplayWidth) -
                                wl->width + XW - 2;
			}
			if ((winY < Scr->BorderTop) &&
                            ((Scr->MoveOffResistance < 0) ||
                             (winY > Scr->BorderTop - Scr->MoveOffResistance)))
                        {
			    winY = Scr->BorderTop;
			    newY = cwlist->mapSubwindow.y + YW +
                                Scr->BorderTop * mw->height /
                                Scr->MyDisplayHeight;
			}
			if (((winY + h) > Scr->MyDisplayHeight -
                             Scr->BorderBottom) &&
			    ((Scr->MoveOffResistance < 0) ||
                             ((winY + h) < Scr->MyDisplayHeight -
                              Scr->BorderBottom + Scr->MoveOffResistance)))
                        {
			    winY = Scr->MyDisplayHeight - Scr->BorderBottom - h;
			    newY = cwlist->mapSubwindow.y + mw->wheight *
                                (1 - Scr->BorderBottom / (double) Scr->MyDisplayHeight) -
                                wl->height + YW - 2;
			}
		    }
		    WMapSetupWindow (win, winX, winY, -1, -1);
		    if (Scr->ShowWinWhenMovingInWmgr) goto movewin;
		    if (cwlist == Scr->workSpaceMgr.activeWSPC) {
			if (alreadyvivible) goto movewin;
			if (Scr->OpaqueMove) {
			    XMoveWindow (dpy, win->frame, winX, winY);
			    DisplayWin (win);
			} else {
			    MoveOutline (Scr->Root,
				winX - win->frame_bw, winY - win->frame_bw,
				win->frame_width  + 2 * win->frame_bw,
				win->frame_height + 2 * win->frame_bw,
				win->frame_bw,
				win->title_height + win->frame_bw3D);
			}
			alreadyvivible = True;
			goto move;
		    }
		    if (!alreadyvivible) goto move;
		    if (!OCCUPY (win, Scr->workSpaceMgr.activeWSPC) ||
			(startincurrent && (button == 1))) {
			if (Scr->OpaqueMove) {
			    Vanish (win);
			    XMoveWindow (dpy, win->frame, winX, winY);
			} else {
			    MoveOutline (Scr->Root, 0, 0, 0, 0, 0, 0);
			}
			alreadyvivible = False;
			goto move;
		    }
movewin:	    if (Scr->OpaqueMove) {
			XMoveWindow (dpy, win->frame, winX, winY);
		    } else {
			MoveOutline (Scr->Root,
				winX - win->frame_bw, winY - win->frame_bw,
				win->frame_width  + 2 * win->frame_bw,
				win->frame_height + 2 * win->frame_bw,
				win->frame_bw,
				win->title_height + win->frame_bw3D);
		    }
		}
move:		XMoveWindow (dpy, w, newX - XW, newY - YW);
		break;
	    case Expose :
		if (ev.xexpose.window == w) {
		    WMapRedrawWindow (w, W0, H0, wl->cp, wl->twm_win->icon_name);
		    break;
		}
		Event = ev;
		DispatchEvent ();
		break;
	}
    }
    if (realmovemode) {
	if (Scr->ShowWinWhenMovingInWmgr || alreadyvivible) {
	    if (Scr->OpaqueMove && !OCCUPY (win, Scr->workSpaceMgr.activeWSPC)) {
		Vanish (win);
	    }
	    if (!Scr->OpaqueMove) {
		MoveOutline (Scr->Root, 0, 0, 0, 0, 0, 0);
		WMapRedrawName (wl);
	    }
	}
	SetupWindow (win, winX, winY, win->frame_width, win->frame_height, -1);
    }
    ev.xbutton.subwindow = (Window) 0;
    ev.xbutton.window = parent;
    XPutBackEvent   (dpy, &ev);
    XUngrabPointer  (dpy, CurrentTime);

    if ((ev.xbutton.time - etime) < 250) {
	KeyCode control_L_code, control_R_code;
	KeySym  control_L_sym,  control_R_sym;
	char keys [32];

	XMapWindow (dpy, sw);
	XDestroyWindow (dpy, w);
	GotoWorkSpace (wlist);
	if (!Scr->DontWarpCursorInWMap) WarpToWindow (win);
	control_L_sym  = XStringToKeysym  ("Control_L");
	control_R_sym  = XStringToKeysym  ("Control_R");
	control_L_code = XKeysymToKeycode (dpy, control_L_sym);
	control_R_code = XKeysymToKeycode (dpy, control_R_sym);
	XQueryKeymap (dpy, keys);
	if ((keys [control_L_code / 8] & ((char) 0x80 >> (control_L_code % 8))) ||
	     keys [control_R_code / 8] & ((char) 0x80 >> (control_R_code % 8))) {
	    WMapToggleState ();
	}
	return;
    }

    for (wlist = Scr->workSpaceMgr.workSpaceList; wlist != NULL; wlist = wlist->next) {
	if ((newX >= wlist->mapSubwindow.x) && (newX < wlist->mapSubwindow.x + mw->wwidth) &&
	    (newY >= wlist->mapSubwindow.y) && (newY < wlist->mapSubwindow.y + mw->wheight)) {
	    break;
	}
    }
    newwlist = wlist;
    switch (button) {
	case 1 :
	    if ((newwlist == NULL) || (newwlist == oldwlist) || OCCUPY (wl->twm_win, newwlist)) {
		XMapWindow (dpy, sw);
		break;
	    }
	    occupation = (win->occupation | (1 << newwlist->number)) & ~(1 << oldwlist->number);
	    ChangeOccupation (win, occupation);
	    if (newwlist == Scr->workSpaceMgr.activeWSPC) {
		RaiseWindow (win);
		WMapRaise (win);
	    }
	    else WMapRestack (newwlist);
	    break;

	case 2 :
	    if ((newwlist == NULL) || (newwlist == oldwlist) ||
		OCCUPY (wl->twm_win, newwlist)) break;

	    occupation = win->occupation | (1 << newwlist->number);
	    ChangeOccupation (win, occupation);
	    if (newwlist == Scr->workSpaceMgr.activeWSPC) {
		RaiseWindow (win);
		WMapRaise (win);
	    }
	    else WMapRestack (newwlist);
	    break;

	default :
	    return;
    }
    XDestroyWindow (dpy, w);
}

void InvertColorPair (cp)
ColorPair *cp;
{
    Pixel save;

    save = cp->fore;
    cp->fore = cp->back;
    cp->back = save;
    save = cp->shadc;
    cp->shadc = cp->shadd;
    cp->shadd = save;
}

void WMapRedrawName (wl)
WinList   wl;
{
    int       w = wl->width;
    int       h = wl->height;
    ColorPair cp;
    char      *label;

    label  = wl->twm_win->icon_name;
    cp     = wl->cp;

    if (Scr->ReverseCurrentWorkspace &&
	wl->wlist == Scr->workSpaceMgr.activeWSPC) {
	InvertColorPair (&cp);
    }
    WMapRedrawWindow (wl->w, w, h, cp, label);
}

static void WMapRedrawWindow (window, width, height, cp, label)
Window	window;
int	width, height;
ColorPair cp;
char 	*label;
{
    int		x, y, strhei, strwid;
    MyFont	font;
#ifdef I18N
    XFontSetExtents *font_extents;
    XRectangle inc_rect;
    XRectangle logical_rect;
    XFontStruct **xfonts;
    char **font_names;
    register int i;
    int descent;
    int fnum;
#endif

    XClearWindow (dpy, window);
    font   = Scr->workSpaceMgr.workspaceWindow.windowFont;

#ifdef I18N
    font_extents = XExtentsOfFontSet(font.font_set);
    strhei = font_extents->max_logical_extent.height;

    if (strhei > height) return;

    XmbTextExtents(font.font_set, label, strlen (label),
		   &inc_rect, &logical_rect);
    strwid = logical_rect.width;
    x = (width  - strwid) / 2;
    if (x < 1) x = 1;

    fnum = XFontsOfFontSet(font.font_set, &xfonts, &font_names);
    for( i = 0, descent = 0; i < fnum; i++){
	/* xf = xfonts[i]; */
	descent = ((descent < (xfonts[i]->max_bounds.descent)) ? (xfonts[i]->max_bounds.descent): descent);
    }
    
    y = ((height + strhei) / 2) - descent;

    if (Scr->use3Dwmap) {
	Draw3DBorder (window, 0, 0, width, height, 1, cp, off, True, False);
	FB(cp.fore, cp.back);
    } else {
	FB (cp.back, cp.fore);
	XFillRectangle (dpy, window, Scr->NormalGC, 0, 0, width, height);
	FB (cp.fore, cp.back);
    }
    if (Scr->Monochrome != COLOR) {
	XmbDrawImageString (dpy, window, font.font_set, Scr->NormalGC, x, y, label, strlen (label));
    } else {
	XmbDrawString (dpy, window, font.font_set,Scr->NormalGC, x, y, label, strlen (label));
    }

#else
    strhei = font.font->max_bounds.ascent + font.font->max_bounds.descent;
    if (strhei > height) return;

    strwid = XTextWidth (font.font, label, strlen (label));
    x = (width  - strwid) / 2;
    if (x < 1) x = 1;
    y = ((height + strhei) / 2) - font.font->max_bounds.descent;

    if (Scr->use3Dwmap) {
	Draw3DBorder (window, 0, 0, width, height, 1, cp, off, True, False);
	FBF(cp.fore, cp.back, font.font->fid);
    } else {
	FBF (cp.back, cp.fore, font.font->fid);
	XFillRectangle (dpy, window, Scr->NormalGC, 0, 0, width, height);
	FBF (cp.fore, cp.back, font.font->fid);
    }
    if (Scr->Monochrome != COLOR) {
	XDrawImageString (dpy, window, Scr->NormalGC, x, y, label, strlen (label));
    } else {
	XDrawString (dpy, window, Scr->NormalGC, x, y, label, strlen (label));
    }
#endif    
}

static void WMapAddToList (win, wlist)
TwmWindow     *win;
WorkSpaceList *wlist;
{
    WinList wl;
    float   wf, hf;
    ColorPair cp;
    XSetWindowAttributes attr;
    unsigned long attrmask;
    unsigned int bw;

    cp.back = win->title.back;
    cp.fore = win->title.fore;
    if (Scr->workSpaceMgr.workspaceWindow.windowcpgiven) {
	cp.back = Scr->workSpaceMgr.workspaceWindow.windowcp.back;
	GetColorFromList (Scr->workSpaceMgr.workspaceWindow.windowBackgroundL,
			win->full_name, &win->class, &cp.back);
	cp.fore = Scr->workSpaceMgr.workspaceWindow.windowcp.fore;
	GetColorFromList (Scr->workSpaceMgr.workspaceWindow.windowForegroundL,
		      win->full_name, &win->class, &cp.fore);
    }
    if (Scr->use3Dwmap && !Scr->BeNiceToColormap) {
	GetShadeColors (&cp);
    }
    wf = (float) (Scr->workSpaceMgr.workspaceWindow.wwidth  - 2) /
		(float) Scr->MyDisplayWidth;
    hf = (float) (Scr->workSpaceMgr.workspaceWindow.wheight - 2) /
		(float) Scr->MyDisplayHeight;
    wl = (WinList) malloc (sizeof (struct winList));
    wl->wlist  = wlist;
    wl->x      = (int) (win->frame_x * wf);
    wl->y      = (int) (win->frame_y * hf);
    wl->width  = (unsigned int) ((win->frame_width  * wf) + 0.5);
    wl->height = (unsigned int) ((win->frame_height * hf) + 0.5);
    bw = 0;
    if (!Scr->use3Dwmap) {
	bw = 1;
	wl->width  -= 2;
	wl->height -= 2;
    }
    if (wl->width  < 1) wl->width  = 1;
    if (wl->height < 1) wl->height = 1;
    wl->w = XCreateSimpleWindow (dpy, wlist->mapSubwindow.w, wl->x, wl->y,
			wl->width, wl->height, bw, Scr->Black, cp.back);
    attrmask = 0;
    if (Scr->BackingStore) {
	attr.backing_store = WhenMapped;
	attrmask |= CWBackingStore;
    }
    attr.cursor = handCursor;
    attrmask |= CWCursor;
    XChangeWindowAttributes (dpy, wl->w, attrmask, &attr);
    XSelectInput (dpy, wl->w, ExposureMask);
    XSaveContext (dpy, wl->w, TwmContext, (XPointer) Scr->workSpaceMgr.workspaceWindow.twm_win);
    XSaveContext (dpy, wl->w, ScreenContext, (XPointer) Scr);
    XSaveContext (dpy, wl->w, MapWListContext, (XPointer) wl);
    wl->twm_win = win;
    wl->cp      = cp;
    wl->next    = wlist->mapSubwindow.wl;
    wlist->mapSubwindow.wl = wl;
    if (win->mapped) XMapWindow (dpy, wl->w);
}

static void WMapRemoveFromList (win, wlist)
TwmWindow *win;
WorkSpaceList *wlist;
{
    WinList wl, wl1;

    wl = wlist->mapSubwindow.wl;
    if (wl == NULL) return;
    if (win == wl->twm_win) {
	wlist->mapSubwindow.wl = wl->next;
	XDeleteContext (dpy, wl->w, TwmContext);
	XDeleteContext (dpy, wl->w, ScreenContext);
	XDeleteContext (dpy, wl->w, MapWListContext);
	XDestroyWindow (dpy, wl->w);
	free (wl);
	return;
    }
    wl1 = wl;
    wl  = wl->next;
    while (wl != NULL) {
	if (win == wl->twm_win) {
	    wl1->next = wl->next;
	    XDeleteContext (dpy, wl->w, TwmContext);
	    XDeleteContext (dpy, wl->w, ScreenContext);
	    XDeleteContext (dpy, wl->w, MapWListContext);
	    XDestroyWindow (dpy, wl->w);
	    free (wl);
	    break;
	}
	wl1 = wl;
	wl  = wl->next;
    }
}

static void ResizeWorkSpaceManager (win)
TwmWindow *win;
{
    int           bwidth, bheight;
    int		  wwidth, wheight;
    int           hspace, vspace;
    int           lines, columns;
    int		  neww, newh;
    WorkSpaceList *wlist;
    TwmWindow	  *tmp_win;
    WinList	  wl;
    int           i, j;
    static int    oldw = 0;
    static int    oldh = 0;
    float	  wf, hf;

    neww = win->attr.width;
    newh = win->attr.height;
    if ((neww == oldw) && (newh == oldh)) return;
    oldw = neww; oldh = newh;

    hspace  = Scr->workSpaceMgr.workspaceWindow.hspace;
    vspace  = Scr->workSpaceMgr.workspaceWindow.vspace;
    lines   = Scr->workSpaceMgr.workspaceWindow.lines;
    columns = Scr->workSpaceMgr.workspaceWindow.columns;
    bwidth  = (neww - (columns * hspace)) / columns;
    bheight = (newh - (lines   * vspace)) / lines;
    wwidth  = neww / columns;
    wheight = newh / lines;
    wf = (float) (wwidth  - 2) / (float) Scr->MyDisplayWidth;
    hf = (float) (wheight - 2) / (float) Scr->MyDisplayHeight;

    i = 0;
    j = 0;
    for (wlist = Scr->workSpaceMgr.workSpaceList; wlist != NULL; wlist = wlist->next) {
	XMoveResizeWindow (dpy, wlist->buttonw,
				i * (bwidth  + hspace) + (hspace / 2),
				j * (bheight + vspace) + (vspace / 2),
				bwidth, bheight);
	wlist->mapSubwindow.x = i * wwidth;
	wlist->mapSubwindow.y = j * wheight;
	XMoveResizeWindow (dpy, wlist->mapSubwindow.w, wlist->mapSubwindow.x,
				wlist->mapSubwindow.y, wwidth - 2, wheight - 2);
	for (wl = wlist->mapSubwindow.wl; wl != NULL; wl = wl->next) {
	    tmp_win    = wl->twm_win;
	    wl->x      = (int) (tmp_win->frame_x * wf);
	    wl->y      = (int) (tmp_win->frame_y * hf);
	    wl->width  = (unsigned int) ((tmp_win->frame_width  * wf) + 0.5);
	    wl->height = (unsigned int) ((tmp_win->frame_height * hf) + 0.5);
	    XMoveResizeWindow (dpy, wl->w, wl->x, wl->y, wl->width, wl->height);
	}
	i++;
	if (i == columns) {i = 0; j++;};
    }
    Scr->workSpaceMgr.workspaceWindow.bwidth    = bwidth;
    Scr->workSpaceMgr.workspaceWindow.bheight   = bheight;
    Scr->workSpaceMgr.workspaceWindow.width     = neww;
    Scr->workSpaceMgr.workspaceWindow.height    = newh;
    Scr->workSpaceMgr.workspaceWindow.wwidth	= wwidth;
    Scr->workSpaceMgr.workspaceWindow.wheight	= wheight;
    PaintWorkSpaceManager ();
}

static void ResizeOccupyWindow (win)
TwmWindow *win;
{
    int           bwidth, bheight, owidth, oheight;
    int           hspace, vspace;
    int           lines, columns;
    int           neww, newh;
    WorkSpaceList *wlist;
    int           i, j, x, y;
    static int    oldw = 0;
    static int    oldh = 0;

    neww = win->attr.width;
    newh = win->attr.height;
    if ((neww == oldw) && (newh == oldh)) return;
    oldw = neww; oldh = newh;

    hspace  = Scr->workSpaceMgr.occupyWindow.hspace;
    vspace  = Scr->workSpaceMgr.occupyWindow.vspace;
    lines   = Scr->workSpaceMgr.occupyWindow.lines;
    columns = Scr->workSpaceMgr.occupyWindow.columns;
    bwidth  = (neww -  columns    * hspace) / columns;
    bheight = (newh - (lines + 2) * vspace) / (lines + 1);
    owidth  = Scr->workSpaceMgr.occupyWindow.owidth;
    oheight = bheight;

    i = 0;
    j = 0;
    for (wlist = Scr->workSpaceMgr.workSpaceList; wlist != NULL; wlist = wlist->next) {
	XMoveResizeWindow (dpy, wlist->obuttonw,
				i * (bwidth  + hspace) + (hspace / 2),
				j * (bheight + vspace) + (vspace / 2),
				bwidth, bheight);
	i++;
	if (i == columns) {i = 0; j++;}
    }
    hspace = (neww - 3 * owidth) / 3;
    x = hspace / 2;
    y = ((bheight + vspace) * lines) + ((3 * vspace) / 2);
    XMoveResizeWindow (dpy, Scr->workSpaceMgr.occupyWindow.OK, x, y, owidth, oheight);
    x += owidth + hspace;
    XMoveResizeWindow (dpy, Scr->workSpaceMgr.occupyWindow.cancel, x, y, owidth, oheight);
    x += owidth + hspace;
    XMoveResizeWindow (dpy, Scr->workSpaceMgr.occupyWindow.allworkspc, x, y, owidth, oheight);
    Scr->workSpaceMgr.occupyWindow.width      = neww;
    Scr->workSpaceMgr.occupyWindow.height     = newh;
    Scr->workSpaceMgr.occupyWindow.bwidth     = bwidth;
    Scr->workSpaceMgr.occupyWindow.bheight    = bheight;
    Scr->workSpaceMgr.occupyWindow.owidth     = owidth;
    Scr->workSpaceMgr.occupyWindow.oheight    = oheight;
    PaintOccupyWindow ();
}

void WMapCreateCurrentBackGround (border, background, foreground, pixmap)
char *border, *background, *foreground, *pixmap;
{
    Image	 *image;

    Scr->workSpaceMgr.workspaceWindow.curBorderColor = Scr->Black;
    Scr->workSpaceMgr.workspaceWindow.curColors.back = Scr->White;
    Scr->workSpaceMgr.workspaceWindow.curColors.fore = Scr->Black;
    Scr->workSpaceMgr.workspaceWindow.curImage       = None;

    if (border == NULL) return;
    GetColor (Scr->Monochrome, &(Scr->workSpaceMgr.workspaceWindow.curBorderColor), border);
    if (background == NULL) return;
    Scr->workSpaceMgr.workspaceWindow.curPaint = True;
    GetColor (Scr->Monochrome, &(Scr->workSpaceMgr.workspaceWindow.curColors.back), background);
    if (foreground == NULL) return;
    GetColor (Scr->Monochrome, &(Scr->workSpaceMgr.workspaceWindow.curColors.fore), foreground);

    if (pixmap == NULL) return;
    if ((image = GetImage (pixmap, Scr->workSpaceMgr.workspaceWindow.curColors)) == None) {
	fprintf (stderr, "Can't find pixmap %s\n", pixmap);
	return;
    }
    Scr->workSpaceMgr.workspaceWindow.curImage = image;
}

void WMapCreateDefaultBackGround (border, background, foreground, pixmap)
char *border, *background, *foreground, *pixmap;
{
    Image	 *image;

    Scr->workSpaceMgr.workspaceWindow.defBorderColor = Scr->Black;
    Scr->workSpaceMgr.workspaceWindow.defColors.back = Scr->White;
    Scr->workSpaceMgr.workspaceWindow.defColors.fore = Scr->Black;
    Scr->workSpaceMgr.workspaceWindow.defImage       = None;

    if (border == NULL) return;
    GetColor (Scr->Monochrome, &(Scr->workSpaceMgr.workspaceWindow.defBorderColor), border);
    if (background == NULL) return;
    GetColor (Scr->Monochrome, &(Scr->workSpaceMgr.workspaceWindow.defColors.back), background);
    if (foreground == NULL) return;
    GetColor (Scr->Monochrome, &(Scr->workSpaceMgr.workspaceWindow.defColors.fore), foreground);

    if (pixmap == NULL) return;
    if ((image = GetImage (pixmap, Scr->workSpaceMgr.workspaceWindow.defColors)) == None)
	return;
    Scr->workSpaceMgr.workspaceWindow.defImage = image;
}

Bool AnimateRoot () {
    ScreenInfo	  *scr;
    int		  scrnum;
    Image	  *image;
    WorkSpaceList *wlist;
    Bool	  maybeanimate;

    maybeanimate = False;
    for (scrnum = 0; scrnum < NumScreens; scrnum++) {
	if ((scr = ScreenList [scrnum]) == NULL) continue;
	if (! scr->workSpaceManagerActive) continue;
	if (! scr->workSpaceMgr.activeWSPC) continue; /* une securite de plus */

	image = scr->workSpaceMgr.activeWSPC->image;
	if ((image == None) || (image->next == None)) continue;
	if (scr->DontPaintRootWindow) continue;

	XSetWindowBackgroundPixmap (dpy, scr->Root, image->pixmap);
	XClearWindow (dpy, scr->Root);
	scr->workSpaceMgr.activeWSPC->image = image->next;
	maybeanimate = True;
    }
    for (scrnum = 0; scrnum < NumScreens; scrnum++) {
	if ((scr = ScreenList [scrnum]) == NULL) continue;
	if (scr->workSpaceMgr.workspaceWindow.state == BUTTONSSTATE) continue;

	for (wlist = scr->workSpaceMgr.workSpaceList; wlist != NULL; wlist = wlist->next) {
	    image = wlist->image;

	    if ((image == None) || (image->next == None)) continue;
	    if (wlist == scr->workSpaceMgr.activeWSPC) continue;
	    XSetWindowBackgroundPixmap (dpy, wlist->mapSubwindow.w, image->pixmap);
	    XClearWindow (dpy, wlist->mapSubwindow.w);
	    wlist->image = image->next;
	    maybeanimate = True;
	}
    }
    return (maybeanimate);
}

static char **GetCaptivesList (scrnum)
int scrnum;
{
    unsigned char	*prop, *p;
    unsigned long	bytesafter;
    unsigned long	len;
    Atom		actual_type;
    int			actual_format;
    char		**ret;
    int			count;
    int			i, l;
    Window		root;

    _XA_WM_CTWMSLIST = XInternAtom (dpy, "WM_CTWMSLIST", True);
    if (_XA_WM_CTWMSLIST == None) return ((char**)0);

    root = RootWindow (dpy, scrnum);
    if (XGetWindowProperty (dpy, root, _XA_WM_CTWMSLIST, 0L, 512,
			False, XA_STRING, &actual_type, &actual_format, &len,
			&bytesafter, &prop) != Success) return ((char**) 0);
    if (len == 0) return ((char**) 0);

    count = 0;
    p = prop;
    l = 0;
    while (l < len) {
	l += strlen ((char*)p) + 1;
	p += strlen ((char*)p) + 1;
	count++;
    }
    ret = (char**) malloc ((count + 1) * sizeof (char*));

    p = prop;
    l = 0;
    i = 0;
    while (l < len) {
	ret [i++] = (char*) strdup ((char*) p);
	l += strlen ((char*)p) + 1;
	p += strlen ((char*)p) + 1;
    }
    ret [i] = (char*) 0;
    return (ret);
}

static void SetCaptivesList (scrnum, clist)
int scrnum;
char **clist;
{
    unsigned char	*prop, *p;
    unsigned long	bytesafter;
    unsigned long	len;
    Atom		actual_type;
    int			actual_format;
    char		**cl;
    char		*s, *slist;
    int			i;
    Window		root = RootWindow (dpy, scrnum);

    _XA_WM_CTWMSLIST = XInternAtom (dpy, "WM_CTWMSLIST", False);
    cl  = clist; len = 0;
    while (*cl) { len += strlen (*cl++) + 1; }
    if (len == 0) {
	XDeleteProperty (dpy, root, _XA_WM_CTWMSLIST);
	return;
    }
    slist = (char*) malloc (len * sizeof (char));
    cl = clist; s  = slist;
    while (*cl) {
	strcpy (s, *cl);
	s += strlen (*cl);
	*s++ = '\0';
	cl++;
    }
    XChangeProperty (dpy, root, _XA_WM_CTWMSLIST, XA_STRING, 8, 
		     PropModeReplace, (unsigned char *) slist, len);
}

static void freeCaptiveList (clist)
char **clist;
{
    while (clist && *clist) { free (*clist++); }
}

void AddToCaptiveList ()
{
    int		i, count;
    char	**clist, **cl, **newclist;
    int		busy [32];
    Atom	_XA_WM_CTWM_ROOT;
    char	*atomname;
    int		scrnum = Scr->screen;
    Window	croot  = Scr->Root;
    Window	root   = RootWindow (dpy, scrnum);

    for (i = 0; i < 32; i++) { busy [i] = 0; }
    clist = GetCaptivesList (scrnum);
    cl = clist;
    count = 0;
    while (cl && *cl) {
	count++;
	if (!captivename) {
	    if (!strncmp (*cl, "ctwm-", 5)) {
		int r, n;
		r = sscanf (*cl, "ctwm-%d", &n);
		cl++;
		if (r != 1) continue;
		if ((n < 0) || (n > 31)) continue;
		busy [n] = 1;
	    } else cl++;
	    continue;
	}
	if (!strcmp (*cl, captivename)) {
	    fprintf (stderr, "A captive ctwm with name %s is already running\n", captivename);
	    exit (1);
	}
	cl++;
    }
    if (!captivename) {
	for (i = 0; i < 32; i++) {
	    if (!busy [i]) break;
	}
	if (i == 32) { /* no one can tell we didn't try hard */
	    fprintf (stderr, "Cannot find a suitable name for captive ctwm\n");
	    exit (1);
	}
	captivename = (char*) malloc (8);
	sprintf (captivename, "ctwm-%d", i);
    }
    newclist = (char**) malloc ((count + 2) * sizeof (char*));
    for (i = 0; i < count; i++) {
	newclist [i] = (char*) strdup (clist [i]);
    }
    newclist [count] = (char*) strdup (captivename);
    newclist [count + 1] = (char*) 0;
    SetCaptivesList (scrnum, newclist);
    freeCaptiveList (clist);
    freeCaptiveList (newclist);
    free (clist); free (newclist);

    root = RootWindow (dpy, scrnum);
    atomname = (char*) malloc (strlen ("WM_CTWM_ROOT_") + strlen (captivename) +1);
    sprintf (atomname, "WM_CTWM_ROOT_%s", captivename);
    _XA_WM_CTWM_ROOT = XInternAtom (dpy, atomname, False);
    XChangeProperty (dpy, root, _XA_WM_CTWM_ROOT, XA_WINDOW, 32, 
		     PropModeReplace, (unsigned char *) &croot, 4);
}

void RemoveFromCaptiveList () {
    int	 i, count;
    char **clist, **cl, **newclist;
    Atom _XA_WM_CTWM_ROOT;
    char *atomname;
    int scrnum = Scr->screen;
    Window root = RootWindow (dpy, scrnum);

    if (!captivename) return;
    clist = GetCaptivesList (scrnum);
    cl = clist; count = 0;
    while (*cl) { count++, cl++; }
    newclist = (char**) malloc (count * sizeof (char*));
    cl = clist; count = 0;
    while (*cl) {
	if (!strcmp (*cl, captivename)) { cl++; continue; }
	newclist [count++] = *cl;
	cl++;
    }
    newclist [count] = (char*) 0;
    SetCaptivesList (scrnum, newclist);
    freeCaptiveList (clist);
    free (clist); free (newclist);

    atomname = (char*) malloc (strlen ("WM_CTWM_ROOT_") + strlen (captivename) +1);
    sprintf (atomname, "WM_CTWM_ROOT_%s", captivename);
    _XA_WM_CTWM_ROOT = XInternAtom (dpy, atomname, True);
    if (_XA_WM_CTWM_ROOT == None) return;
    XDeleteProperty (dpy, root, _XA_WM_CTWM_ROOT);
}

void SetPropsIfCaptiveCtwm (win)
TwmWindow *win;
{
    Window	window = win->w;
    Window	frame  = win->frame;
    Atom	_XA_WM_CTWM_ROOT;

    if (!CaptiveCtwmRootWindow (window)) return;
    _XA_WM_CTWM_ROOT = XInternAtom (dpy, "WM_CTWM_ROOT", True);
    if (_XA_WM_CTWM_ROOT == None) return;

    XChangeProperty (dpy, frame, _XA_WM_CTWM_ROOT, XA_WINDOW, 32, 
		     PropModeReplace, (unsigned char *) &window, 4);
}

Window CaptiveCtwmRootWindow (window)
Window window;
{
    unsigned char	*prop;
    unsigned long	bytesafter;
    unsigned long	len;
    Atom		actual_type;
    int			actual_format;
    int			scrnum = Scr->screen;
    Window		root = RootWindow (dpy, scrnum);
    Window		ret;
    Atom		_XA_WM_CTWM_ROOT;

    _XA_WM_CTWM_ROOT = XInternAtom (dpy, "WM_CTWM_ROOT", True);
    if (_XA_WM_CTWM_ROOT == None) return ((Window)0);

    if (XGetWindowProperty (dpy, window, _XA_WM_CTWM_ROOT, 0L, 1L,
			False, XA_WINDOW, &actual_type, &actual_format, &len,
			&bytesafter, &prop) != Success) return ((Window)0);
    if (len == 0) return ((Window)0);
    ret = *((Window*) prop);
    return (ret);
}

CaptiveCTWM GetCaptiveCTWMUnderPointer () {
    int		scrnum = Scr->screen;
    Window	root;
    Window	child, croot;
    CaptiveCTWM	cctwm;

    root = RootWindow (dpy, scrnum);
    while (1) {
	XQueryPointer (dpy, root, &JunkRoot, &child,
			&JunkX, &JunkY, &JunkX, &JunkY, &JunkMask);
	if (child && (croot = CaptiveCtwmRootWindow (child))) {
	    root = croot;
	    continue;
	}
	cctwm.root = root;
	XFetchName (dpy, root, &cctwm.name);
	if (!cctwm.name) cctwm.name = (char*) strdup ("Root");
	return (cctwm);
    }
}

void SetNoRedirect (window)
Window window;
{
    Atom	_XA_WM_NOREDIRECT;

    _XA_WM_NOREDIRECT = XInternAtom (dpy, "WM_NOREDIRECT", False);
    if (_XA_WM_NOREDIRECT == None) return;

    XChangeProperty (dpy, window, _XA_WM_NOREDIRECT, XA_STRING, 8, 
		     PropModeReplace, (unsigned char *) "Yes", 4);
}

static Bool DontRedirect (window)
Window window;
{
    unsigned char	*prop;
    unsigned long	bytesafter;
    unsigned long	len;
    Atom		actual_type;
    int			actual_format;
    int			scrnum = Scr->screen;
    Window		root = RootWindow (dpy, scrnum);
    Atom		_XA_WM_NOREDIRECT;

    _XA_WM_NOREDIRECT = XInternAtom (dpy, "WM_NOREDIRECT", True);
    if (_XA_WM_NOREDIRECT == None) return (False);

    if (XGetWindowProperty (dpy, window, _XA_WM_NOREDIRECT, 0L, 1L,
			False, XA_STRING, &actual_type, &actual_format, &len,
			&bytesafter, &prop) != Success) return (False);
    if (len == 0) return (False);
    return (True);
}

#ifdef BUGGY_HP700_SERVER
static void fakeRaiseLower (display, window)
Display *display;
Window   window;
{
    Window          root;
    Window          parent;
    Window          grandparent;
    Window         *children;
    unsigned int    number;
    XWindowChanges  changes;

    number = 0;
    XQueryTree (display, window, &root, &parent, &children, &number);
    XFree ((char *) children);
    XQueryTree (display, parent, &root, &grandparent, &children, &number);

    changes.stack_mode = (children [number-1] == window) ? Below : Above;
    XFree ((char *) children);
    XConfigureWindow (display, window, CWStackMode, &changes);
}
#endif



