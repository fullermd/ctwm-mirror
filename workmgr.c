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
#include "twm.h"
#include "util.h"
#include "parse.h"
#include "screen.h"
#include "resize.h"
#include "add_window.h"
#include "events.h"
#include "siconify.bm"
#include <X11/Xos.h>
#include <X11/Xmu/CharSet.h>
#include <X11/Xresource.h>
#ifdef macII
int strcmp(); /* missing from string.h in AUX 2.0 */
#endif

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
unsigned int GetMaskFromNameList ();


typedef enum {on, off} ButtonState;
static GC rootGC;
int workSpaceManagerActive = 0;
int fullOccupation         = 0;
int useBackgroundInfo      = False;

void InitWorkSpaceManager ()
{
    Scr->workSpaceMgr.buttonList = NULL;
    Scr->workSpaceMgr.name       = "WorkSpaceManager";
    Scr->workSpaceMgr.icon_name  = "WSManager Icon";
    Scr->workSpaceMgr.geometry   = NULL;
    Scr->workSpaceMgr.columns    = 0;
    Scr->workSpaceMgr.vspace     = 5;
    Scr->workSpaceMgr.hspace     = 5;
    Scr->workSpaceMgr.twm_win    = (TwmWindow*) 0;
    Scr->workSpaceMgr.occupyWindow.twm_win = (TwmWindow*) 0;
    XrmInitialize ();
}

void CreateWorkSpaceManager ()
{
    int        mask;
    int        lines, vspace, hspace, count, columns;
    int        width, height, bwidth, bheight;
    char       *name, *icon_name, *geometry;
    int        i, j;
    Window     w;
    ColorPair  cp;
    MyFont     font;
    ButtonList *blist;
    Window     junkW;
    int        x, y, strWid, wid;
    XGCValues  gcvalues;

    if (! workSpaceManagerActive) return;

    name      = Scr->workSpaceMgr.name;
    icon_name = Scr->workSpaceMgr.icon_name;
    geometry  = Scr->workSpaceMgr.geometry;
    columns   = Scr->workSpaceMgr.columns;
    vspace    = Scr->workSpaceMgr.vspace;
    hspace    = Scr->workSpaceMgr.hspace;
    font      = Scr->IconManagerFont;

    count = 0;
    for (blist = Scr->workSpaceMgr.buttonList; blist != NULL; blist = blist->next) count++;
    Scr->workSpaceMgr.count = count;

    if (columns == 0) {
	lines   = 2;
	columns = ((count - 1) / lines) + 1;
    }
    else {
	lines   = ((count - 1) / columns) + 1;
    }
    strWid = 0;
    for (blist = Scr->workSpaceMgr.buttonList; blist != NULL; blist = blist->next) {
	wid = XTextWidth (font.font, blist->label, strlen (blist->label));
	if (wid > strWid) strWid = wid;
    }
    bwidth  = strWid + 10;
    width   = (columns * bwidth) + ((columns + 1) * hspace);
    bheight = 22;
    height  = (lines * bheight) + ((lines + 1) * vspace);
    x       = (Scr->MyDisplayWidth - width) / 2;
    y       = Scr->MyDisplayHeight - height;

    if (geometry != NULL) {
	mask = XParseGeometry (geometry, &x, &y, (unsigned int *) &width, (unsigned int *) &height);
	if (mask & XNegative) x += Scr->MyDisplayWidth  - width  - (2 * Scr->BorderWidth);
	if (mask & YNegative) y += Scr->MyDisplayHeight - height - (2 * Scr->BorderWidth);
	if (!(mask & XValue)) x  = (Scr->MyDisplayWidth -  width) / 2;
	if (!(mask & YValue)) y  = Scr->MyDisplayHeight - height;
	bwidth  = (width  - ((columns + 1) * hspace)) / columns;
	bheight = (height - ((lines   + 1) * vspace)) / lines;
    }

    cp = Scr->IconManagerC;
    Scr->workSpaceMgr.w = XCreateSimpleWindow (dpy, Scr->Root, x, y, width, height, 0, Scr->Black, cp.back);

    i = 0;
    j = 0;
    for (blist = Scr->workSpaceMgr.buttonList; blist != NULL; blist = blist->next) {
	blist->w = XCreateSimpleWindow(dpy, Scr->workSpaceMgr.w,
		       i * (bwidth  + hspace) + hspace,
		       j * (bheight + vspace) + vspace, bwidth, bheight, 0,
		       Scr->Black, blist->cp.back);
	XMapWindow (dpy, blist->w);
	i++;
	if (i == columns) {i = 0; j++;};
    }

    XSetStandardProperties (dpy, Scr->workSpaceMgr.w, name, icon_name, None, NULL, 0, NULL);
    Scr->workSpaceMgr.twm_win = AddWindow (Scr->workSpaceMgr.w, FALSE, Scr->iconmgr);
    Scr->workSpaceMgr.twm_win->occupation = fullOccupation;
    XSelectInput (dpy, Scr->workSpaceMgr.w, ButtonPressMask |
					    ButtonReleaseMask |
					    KeyPressMask |
					    KeyReleaseMask |
					    ExposureMask);

    SetMapStateProp (Scr->workSpaceMgr.twm_win, WithdrawnState);

    Scr->workSpaceMgr.lines     = lines;
    Scr->workSpaceMgr.columns   = columns;
    Scr->workSpaceMgr.bwidth    = bwidth;
    Scr->workSpaceMgr.bheight   = bheight;
    Scr->workSpaceMgr.width     = width;
    Scr->workSpaceMgr.height    = height;
    Scr->workSpaceMgr.cp        = cp;
    Scr->workSpaceMgr.font      = font;

    CreateOccupyWindow ();

    if (useBackgroundInfo) {
	if (Scr->workSpaceMgr.buttonList->backpix == None)
	    XSetWindowBackground (dpy, Scr->Root, Scr->workSpaceMgr.buttonList->backcp.back);
	else
	    XSetWindowBackgroundPixmap (dpy, Scr->Root, Scr->workSpaceMgr.buttonList->backpix);
    }
    XClearWindow (dpy, Scr->Root);
}

ChangeWorkSpace (buttonW)
Window buttonW;
{
    ButtonList *blist;

    if (buttonW == 0) return; /* icon */
    blist = Scr->workSpaceMgr.buttonList;
    while (blist != NULL) {
	if (blist->w == buttonW) break;
	blist = blist->next;
    }
    if (blist == NULL) return; /* someone told me it happens */
    GotoWorkSpace (blist);
}

GotoWorkSpaceByName (bname)
char *bname;
{
    ButtonList *blist;

    blist = Scr->workSpaceMgr.buttonList;
    while (blist != NULL) {
	if (strcmp (blist->label, bname) == 0) break;
	blist = blist->next;
    }
    if (blist == NULL) return;
    GotoWorkSpace (blist);
}

GotoWorkSpace (blist)
ButtonList *blist;
{
    TwmWindow  *twmWin;
    ButtonList *oldscr, *newscr;
    WList      *wl, *wl1;

    oldscr = Scr->workSpaceMgr.activeWSPC;
    newscr = blist;
    if (oldscr == newscr) return;

    if (useBackgroundInfo) {
	if (newscr->backpix == None)
	    XSetWindowBackground (dpy, Scr->Root, newscr->backcp.back);
	else
	    XSetWindowBackgroundPixmap (dpy, Scr->Root, newscr->backpix);

	XClearWindow (dpy, Scr->Root);
    }
    for (twmWin = &(Scr->TwmRoot); twmWin != NULL; twmWin = twmWin->next) {
	if (OCCUPY (twmWin, oldscr)) {
	    if (twmWin->mapped == FALSE) {
		if ((twmWin->icon_on == TRUE) && (twmWin->icon_w)) {
		    XUnmapWindow(dpy, twmWin->icon_w);
		    IconDown (twmWin);
		}
	    }
	    else
	    if (! OCCUPY (twmWin, newscr)) {
		Vanish (twmWin);
	    }
	}
    }
    for (twmWin = &(Scr->TwmRoot); twmWin != NULL; twmWin = twmWin->next) {
	if (OCCUPY (twmWin, newscr)) {
	    if (twmWin->mapped == FALSE) {
		if ((twmWin->icon_on == TRUE) && (twmWin->icon_w)) {
		    IconUp (twmWin);
		    XMapRaised(dpy, twmWin->icon_w);
		}
	    }
	    else
	    if (! OCCUPY (twmWin, oldscr)) {
		DisplayWin (twmWin);
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
    SetButton  (Scr->workSpaceMgr.activeWSPC->w, off);
    Scr->workSpaceMgr.activeWSPC = blist;
    SetButton (Scr->workSpaceMgr.activeWSPC->w, on);
    oldscr->iconmgr = Scr->iconmgr;
    Scr->iconmgr = newscr->iconmgr;
    XFlush (dpy);
}

AddWorkSpace (name, background, foreground, backback, backfore, backpix)
char *name, *background, *foreground, *backback, *backfore, *backpix;
{
    ButtonList   *blist, *b;
    static int   scrnum = 0;
    Pixmap       pix;
    unsigned int width, height, depth;
    XGCValues    gcvalues;

    if (scrnum == MAXWORKSPACE) return;
    fullOccupation |= (1 << scrnum);
    blist = (ButtonList*) malloc (sizeof (ButtonList));
    blist->label = strdup (name);
    blist->clientlist = NULL;

    if (background == NULL)
	blist->cp.back = Scr->IconManagerC.back;
    else
	GetColor(Scr->Monochrome, &(blist->cp.back), background);

    if (foreground == NULL)
	blist->cp.fore = Scr->IconManagerC.fore;
    else
	GetColor (Scr->Monochrome, &(blist->cp.fore), foreground);

    if (backback == NULL)
	GetColor (Scr->Monochrome, &(blist->backcp.back), "Black");
    else {
	GetColor (Scr->Monochrome, &(blist->backcp.back), backback);
	useBackgroundInfo = True;
    }

    if (backfore == NULL)
	GetColor (Scr->Monochrome, &(blist->backcp.fore), "White");
    else {
	GetColor (Scr->Monochrome, &(blist->backcp.fore), backfore);
	useBackgroundInfo = True;
    }

    if (scrnum == 0) rootGC = XCreateGC (dpy, Scr->Root, 0, &gcvalues);

    if (backpix == NULL)
	blist->backpix = None;
    else
    if (*backpix == '/') {
	blist->backpix = None;
    }
    else
    if ((pix = FindBitmap (backpix, &width, &height)) == None) {
	blist->backpix = None;
    }
    else {
	depth = (unsigned int) DefaultDepth (dpy, Scr->screen);
	blist->backpix = XCreatePixmap (dpy, Scr->Root, width, height, depth);
	gcvalues.background = blist->backcp.back;
	gcvalues.foreground = blist->backcp.fore;
	XChangeGC (dpy, rootGC, GCForeground | GCBackground, &gcvalues);
	XCopyPlane (dpy, pix, blist->backpix, rootGC, 0, 0, width, height, 0, 0, (unsigned long) 1);
	XFreePixmap (dpy, pix);
	useBackgroundInfo = True;
    }
    blist->next        = NULL;
    blist->number      = scrnum++;
    Scr->workSpaceMgr.count = scrnum;

    if (Scr->workSpaceMgr.buttonList == NULL) {
	Scr->workSpaceMgr.buttonList = blist;
    }
    else {
	b = Scr->workSpaceMgr.buttonList;
	while (b->next != NULL) {b = b->next;}
	b->next = blist;
    }
    workSpaceManagerActive = 1;
}

static XrmDatabase db;
static XrmOptionDescRec table [] = {
    {"-workspace",	"*workspace",	XrmoptionSepArg, (caddr_t) NULL},
    {"-xrm",		NULL,		XrmoptionResArg, (caddr_t) NULL},
};

SetupOccupation (twm_win)
TwmWindow *twm_win;
{
    unsigned char *prop;
    unsigned long  nitems, bytesafter;
    Atom   actual_type;
    int    actual_format;
    int    state;
    Window icon;
    char   **cliargv = NULL;
    int    cliargc;
    Bool   status;
    char   *str_type;
    XrmValue value;
    char   wrkSpcList [256];
    ButtonList   *blist;

    if (! workSpaceManagerActive) {
	twm_win->occupation = 1;
	return;
    }

    twm_win->occupation = 0;

    for (blist = Scr->workSpaceMgr.buttonList; blist != NULL; blist = blist->next) {
	if (LookInList (blist->clientlist, twm_win->full_name, &twm_win->class)) {
            twm_win->occupation |= 1 << blist->number;
	}
    }

    if (LookInList (Scr->OccupyAll, twm_win->full_name, &twm_win->class)) {
        twm_win->occupation = fullOccupation;
    }

    nitems = 0;
    if (XGetWindowProperty (dpy, twm_win->w, _XA_WM_WORKSPACES, 0L, 1L, False,
			    _XA_WM_WORKSPACES, &actual_type, &actual_format, &nitems,
			    &bytesafter, (unsigned char **)&prop) == Success) {
	if (nitems != 0) {
	    twm_win->occupation = *((int*) prop);
	}
    }

    if (XGetCommand (dpy, twm_win->w, &cliargv, &cliargc)) {
	XrmParseCommand (&db, table, 2, "ctwm", &cliargc, cliargv);
	status = XrmGetResource (db, "ctwm.workspace", "Ctwm.Workspace", &str_type, &value);
	if ((status == True) && (value.size != 0)) {
	    strncpy (wrkSpcList, value.addr, value.size);
	    twm_win->occupation = GetMaskFromNameList (wrkSpcList);
	}
	XrmDestroyDatabase (db);
	db = NULL;
    }

    if (twm_win->iconmgr) /* someone try to modify occupation of icon managers */
	twm_win->occupation = 1 << Scr->workSpaceMgr.activeWSPC->number;

    if ((twm_win->occupation & fullOccupation) == 0)
	twm_win->occupation = 1 << Scr->workSpaceMgr.activeWSPC->number;

    XChangeProperty (dpy, twm_win->w, _XA_WM_WORKSPACES, _XA_WM_WORKSPACES, 32, 
		     PropModeReplace, (unsigned char *) &twm_win->occupation, 1);

/* kludge */
    if (OCCUPY (twm_win, Scr->workSpaceMgr.activeWSPC)) {
	if (GetWMState (twm_win->w, &state, &icon) != 0) {
	    if (state == InactiveState) SetMapStateProp (twm_win, NormalState);
	}
    }
    else {
	if (GetWMState (twm_win->w, &state, &icon) != 0) {
	    if (state == NormalState) SetMapStateProp (twm_win, InactiveState);
	}
	else
	if (twm_win->wmhints->initial_state == NormalState) {
	    SetMapStateProp (twm_win, InactiveState);
	}
    }
}

static TwmWindow *occupyWin = (TwmWindow*) 0;

Occupy (twm_win)
TwmWindow *twm_win;
{
    int x, y, junkX, junkY;
    unsigned int junkB, junkD;
    unsigned int width, height;
    int xoffset, yoffset;
    Window junkW, w;
    unsigned int junkK;
    ButtonList *blist;

    if (! workSpaceManagerActive) return;
    if (occupyWin != (TwmWindow*) 0) return;
    if (twm_win->iconmgr) return;

    for (blist = Scr->workSpaceMgr.buttonList; blist != NULL; blist = blist->next) {
	if (OCCUPY (twm_win, blist)) {
	    SetButton (blist->ow, on);
	    Scr->workSpaceMgr.occupyWindow.tmpOccupation |= (1 << blist->number);
	}
	else {
	    SetButton (blist->ow, off);
	    Scr->workSpaceMgr.occupyWindow.tmpOccupation &= ~(1 << blist->number);
	}
    }
    w = Scr->workSpaceMgr.occupyWindow.w;
    XGetGeometry  (dpy, w, &junkW, &x, &y, &width, &height, &junkB, &junkD);
    XQueryPointer (dpy, Scr->Root, &junkW, &junkW, &x, &y, &junkX, &junkY, &junkK);
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
    occupyWin = twm_win;
}

EndOccupy (buttonW)
Window buttonW;
{
    TwmWindow    *twmWin;
    ButtonList   *blist;
    OccupyWindow *occupyW;

    if (! workSpaceManagerActive) return;
    if (occupyWin == (TwmWindow*) 0) return;
    if (buttonW == 0) return; /* icon */

    XGrabPointer (dpy, Scr->Root, True,
		  ButtonPressMask | ButtonReleaseMask,
		  GrabModeAsync, GrabModeAsync,
		  Scr->Root, Scr->WaitCursor, CurrentTime);

    occupyW = &Scr->workSpaceMgr.occupyWindow;
    for (blist = Scr->workSpaceMgr.buttonList; blist != NULL; blist = blist->next) {
	if (blist->ow == buttonW) break;
    }
    if (blist != NULL) {
	if ((occupyW->tmpOccupation & (1 << blist->number)) == 0) {
	    SetButton (blist->ow, on);
	    occupyW->tmpOccupation |= (1 << blist->number);
	}
	else {
	    SetButton (blist->ow, off);
	    occupyW->tmpOccupation &= ~(1 << blist->number);
	}
    }
    else
    if (buttonW == occupyW->OK) {
	if (occupyW->tmpOccupation == 0) return;
	occupyWin->occupation = occupyW->tmpOccupation & ~occupyWin->occupation;
	AddIconManager (occupyWin);
	occupyWin->occupation = occupyW->tmpOccupation;
	RemoveIconManager (occupyWin);
	if (! OCCUPY (occupyWin, Scr->workSpaceMgr.activeWSPC)) Vanish (occupyWin);
	Vanish (occupyW->twm_win);
	XChangeProperty (dpy, occupyWin->w, _XA_WM_WORKSPACES, _XA_WM_WORKSPACES, 32, 
			 PropModeReplace, (unsigned char *) &occupyWin->occupation, 1);

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
	for (blist = Scr->workSpaceMgr.buttonList; blist != NULL; blist = blist->next) {
	    SetButton (blist->ow, on);
	}
	occupyW->tmpOccupation = fullOccupation;
    }
    if (ButtonPressed == -1) XUngrabPointer (dpy, CurrentTime);
}

ChangeLabel (event)
XEvent *event;
{
    ButtonList *blist;
    Window     buttonW;
    int        len, i, lname;
    char       key [16], k;
    char       name [128];

    buttonW = event->xkey.subwindow;

    if (buttonW == 0) return; /* icon */
    blist = Scr->workSpaceMgr.buttonList;
    while (blist != NULL) {
	if (blist->w == buttonW) break;
	blist = blist->next;
    }
    if (blist == NULL) return;
    strcpy (name, blist->label);
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
    SetButtonLabel (blist, name);
}

OccupyAll (twm_win)
TwmWindow *twm_win;
{
    IconMgr *save;

    if (! workSpaceManagerActive) return;
    if (twm_win->iconmgr) return;
    save = Scr->iconmgr;
    Scr->iconmgr = Scr->workSpaceMgr.buttonList->iconmgr;
    twm_win->occupation = fullOccupation & ~twm_win->occupation;
    AddIconManager (twm_win);
    twm_win->occupation = fullOccupation;
    Scr->iconmgr = save;
    XChangeProperty (dpy, twm_win->w, _XA_WM_WORKSPACES, _XA_WM_WORKSPACES, 32, 
			 PropModeReplace, (unsigned char *) &twm_win->occupation, 1);
}

AllocateOthersIconManagers () {
    IconMgr    *p, *ip, *oldp, *oldv;
    ButtonList *blist;

    if (! workSpaceManagerActive) return;

    oldp = Scr->iconmgr;
    for (blist = Scr->workSpaceMgr.buttonList->next; blist != NULL; blist = blist->next) {
	blist->iconmgr  = (IconMgr *) malloc (sizeof (IconMgr));
	*blist->iconmgr = *oldp;
	oldv = blist->iconmgr;
	oldp->nextv = blist->iconmgr;
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
	for (ip = blist->iconmgr; ip != NULL; ip = ip->next) {
	    ip->lasti = p;
        }
	oldp = blist->iconmgr;
    }
    Scr->workSpaceMgr.buttonList->iconmgr = Scr->iconmgr;
}

Vanish(tmp_win)
TwmWindow *tmp_win;
{
    TwmWindow *t;
    int iconify;
    XWindowAttributes winattrs;
    unsigned long eventMask;

    XGetWindowAttributes(dpy, tmp_win->w, &winattrs);
    eventMask = winattrs.your_event_mask;

    /*
     * Prevent the receipt of an UnmapNotify, since that would
     * cause a transition to the Withdrawn state.
     */
    XSelectInput(dpy, tmp_win->w, eventMask & ~StructureNotifyMask);
    XUnmapWindow(dpy, tmp_win->w);
    XSelectInput(dpy, tmp_win->w, eventMask);
    XUnmapWindow(dpy, tmp_win->frame);
    SetMapStateProp (tmp_win, InactiveState);
}

DisplayWin(tmp_win)
TwmWindow *tmp_win;
{
    TwmWindow *t;
    int iconify;
    XWindowAttributes winattrs;
    unsigned long eventMask;

    XMapWindow(dpy, tmp_win->w);
    XMapWindow(dpy, tmp_win->frame);
    SetMapStateProp (tmp_win, NormalState);
}

SetButton (w, state)
Window      w;
ButtonState state;
{
    int       bwidth, bheight;

    bwidth  = Scr->workSpaceMgr.bwidth;
    bheight = Scr->workSpaceMgr.bheight;

    if (state == on) {FB (Scr->Black, Scr->White);}
    else {FB (Scr->White, Scr->Black);}
    XDrawLine (dpy, w, Scr->NormalGC, 0, 0, bwidth,     0);
    XDrawLine (dpy, w, Scr->NormalGC, 0, 1, bwidth - 1, 1);
    XDrawLine (dpy, w, Scr->NormalGC, 0, 0, 0,          bheight);
    XDrawLine (dpy, w, Scr->NormalGC, 1, 0, 1,          bheight - 1);
    if (state == on) {FB (Scr->White, Scr->Black);}
    else {FB (Scr->Black, Scr->White);}
    XDrawLine (dpy, w, Scr->NormalGC, bwidth - 1,     1, bwidth - 1, bheight);
    XDrawLine (dpy, w, Scr->NormalGC, bwidth - 2,     2, bwidth - 2, bheight);
    XDrawLine (dpy, w, Scr->NormalGC, 1, bheight - 1, bwidth, bheight - 1);
    XDrawLine (dpy, w, Scr->NormalGC, 2, bheight - 2, bwidth, bheight - 2);
}

ColorPair occupyButtoncp;

CreateOccupyWindow () {
    int width, height, lines, columns, x, y;
    int bwidth, bheight, hspace, vspace, bspace;
    int i, j;
    Window w, OK, cancel, allworkspc;
    ColorPair cp;
    TwmWindow *twm_win;
    ButtonList *blist;

    lines   = Scr->workSpaceMgr.lines;
    columns = Scr->workSpaceMgr.columns;
    bwidth  = Scr->workSpaceMgr.bwidth;
    bheight = Scr->workSpaceMgr.bheight;
    vspace  = Scr->workSpaceMgr.vspace;
    hspace  = Scr->workSpaceMgr.hspace;
    cp      = Scr->workSpaceMgr.cp;

    width  = (bwidth * columns) + (hspace * (columns + 1));
    height = ((bheight + vspace) * lines) + bheight + (3 * vspace);
    if (width < ((3 * bwidth) + (4 * hspace))) width = (3 * bwidth) + (4 * hspace);

    w = XCreateSimpleWindow (dpy, Scr->Root, 0, 0, width, height, 1, Scr->Black, cp.back);
    Scr->workSpaceMgr.occupyWindow.w = w;
    i = 0;
    j = 0;
    for (blist = Scr->workSpaceMgr.buttonList; blist != NULL; blist = blist->next) {
	blist->ow = XCreateSimpleWindow(dpy, w,
		       i * (bwidth  + hspace) + hspace,
		       j * (bheight + vspace) + vspace, bwidth, bheight, 0,
		       Scr->Black, blist->cp.back);
	XMapWindow (dpy, blist->ow);
	i++;
	if (i == columns) {i = 0; j++;}
    }
    GetColor (Scr->Monochrome, &(occupyButtoncp.back), "gray50");
    occupyButtoncp.fore = Scr->White;

    bspace = (width - (3 * bwidth)) / 4;
    x = bspace;
    y = ((bheight + vspace) * lines) + ( 2 * vspace);
    OK         = XCreateSimpleWindow(dpy, w, x, y, bwidth, bheight, 0, Scr->Black, occupyButtoncp.back);
    XMapWindow (dpy, OK);
    x += bwidth + bspace;
    cancel     = XCreateSimpleWindow(dpy, w, x, y, bwidth, bheight, 0, Scr->Black, occupyButtoncp.back);
    XMapWindow (dpy, cancel);
    x += bwidth + bspace;
    allworkspc = XCreateSimpleWindow(dpy, w, x, y, bwidth, bheight, 0, Scr->Black, occupyButtoncp.back);
    XMapWindow (dpy, allworkspc);

    XSetStandardProperties (dpy, w, "Occupy Window", "Occupy Window Icon", None, NULL, 0, NULL);
    twm_win = AddWindow (w, FALSE, Scr->iconmgr);
    XGrabButton (dpy, AnyButton, AnyModifier, w, 
		True, ButtonPressMask | ButtonReleaseMask,
		GrabModeAsync, GrabModeAsync, None, 
		Scr->WaitCursor);

    XSelectInput (dpy, w, ButtonPressMask | ButtonReleaseMask | ExposureMask);
    SetMapStateProp (Scr->workSpaceMgr.twm_win, WithdrawnState);
    twm_win->occupation = 0;

    Scr->workSpaceMgr.occupyWindow.OK         = OK;
    Scr->workSpaceMgr.occupyWindow.cancel     = cancel;
    Scr->workSpaceMgr.occupyWindow.allworkspc = allworkspc;
    Scr->workSpaceMgr.occupyWindow.twm_win    = twm_win;
}

void PaintOccupyWindow (twm_win)
TwmWindow *twm_win;
{
    Window       w;
    int          bwidth, bheight;
    ColorPair    cp;
    MyFont       font;
    ButtonList   *blist;
    char         *name;
    int          strWid, strHei, hspace, vspace;
    OccupyWindow *occupyW;

    bwidth  = Scr->workSpaceMgr.bwidth;
    bheight = Scr->workSpaceMgr.bheight;
    font    = Scr->workSpaceMgr.font;

    strHei = font.font->max_bounds.ascent + font.font->max_bounds.descent;
    vspace = ((bheight + strHei) / 2) - font.font->max_bounds.descent;

    occupyW = &Scr->workSpaceMgr.occupyWindow;
    for (blist = Scr->workSpaceMgr.buttonList; blist != NULL; blist = blist->next) {
	w    = blist->ow;
	name = blist->label;
	cp   = blist->cp;

	if (occupyW->tmpOccupation & (1 << blist->number))
	    SetButton (w, on);
	else
	    SetButton (w, off);

	FBF (cp.fore, cp.back, font.font->fid);
	strWid = XTextWidth (font.font, name, strlen (name));
	hspace  = (bwidth - strWid - 4) / 2 + 2;
	if (hspace < 3) hspace = 3;
	XDrawString (dpy, w, Scr->NormalGC, hspace, vspace, name, strlen (name));
    }

    w = Scr->workSpaceMgr.occupyWindow.OK;
    SetButton (w,  off);
    FBF (occupyButtoncp.fore, occupyButtoncp.back, font.font->fid);
    strWid = XTextWidth (font.font, "OK", 2);
    hspace  = ((bwidth - strWid - 4) / 2) + 2;
    if (hspace < 3) hspace = 3;
    XDrawString (dpy, w, Scr->NormalGC, hspace, vspace, "OK", 2);

    w = Scr->workSpaceMgr.occupyWindow.cancel;
    SetButton (w, off);
    FBF (occupyButtoncp.fore, occupyButtoncp.back, font.font->fid);
    strWid = XTextWidth (font.font, "Cancel", 6);
    hspace  = ((bwidth - strWid - 4) / 2) + 2;
    if (hspace < 3) hspace = 3;
    XDrawString (dpy, w, Scr->NormalGC, hspace, vspace, "Cancel", 6);

    w = Scr->workSpaceMgr.occupyWindow.allworkspc;
    SetButton (w, off);
    FBF (occupyButtoncp.fore, occupyButtoncp.back, font.font->fid);
    strWid = XTextWidth (font.font, "Everywhere", 10);
    hspace  = ((bwidth - strWid - 4) / 2) + 2;
    if (hspace < 3) hspace = 3;
    XDrawString (dpy, w, Scr->NormalGC, hspace, vspace, "Everywhere", 10);
}

void PaintWorkSpaceManager ()
{
    Window     w;
    int        bwidth, bheight, width, height;
    ColorPair  cp;
    MyFont     font;
    ButtonList *blist;
    char       *name;
    int        strWid, strHei, hspace, vspace;

    width   = Scr->workSpaceMgr.width;
    height  = Scr->workSpaceMgr.height;
    bwidth  = Scr->workSpaceMgr.bwidth;
    bheight = Scr->workSpaceMgr.bheight;
    font    = Scr->workSpaceMgr.font;

    w = Scr->workSpaceMgr.w;
    FB (Scr->White, Scr->Black);
    XDrawLine (dpy, w, Scr->NormalGC, 0, 0, width - 1, 0);
    XDrawLine (dpy, w, Scr->NormalGC, 0, 1, width - 2, 1);
    XDrawLine (dpy, w, Scr->NormalGC, 0, 0, 0, height - 1);
    XDrawLine (dpy, w, Scr->NormalGC, 1, 0, 1, height - 2);
    FB (Scr->Black, Scr->White);
    XDrawLine (dpy, w, Scr->NormalGC, width - 1, 1, width - 1, height - 1);
    XDrawLine (dpy, w, Scr->NormalGC, width - 2, 2, width - 2, height - 1);
    XDrawLine (dpy, w, Scr->NormalGC, 1, height - 1, width, height - 1);
    XDrawLine (dpy, w, Scr->NormalGC, 2, height - 2, width, height - 2);

    strHei = font.font->max_bounds.ascent + font.font->max_bounds.descent;
    vspace = ((bheight + strHei) / 2) - font.font->max_bounds.descent;
    for (blist = Scr->workSpaceMgr.buttonList; blist != NULL; blist = blist->next) {
	w    = blist->w;
	name = blist->label;
	cp   = blist->cp;

	if (blist == Scr->workSpaceMgr.activeWSPC) SetButton (blist->w, on);
	else SetButton (blist->w, off);
	FBF (cp.fore, cp.back, font.font->fid);
	strWid = XTextWidth (font.font, name, strlen (name));
	hspace = (bwidth - strWid) / 2;
	if (hspace < 3) hspace = 3;
	XDrawString (dpy, w, Scr->NormalGC, hspace, vspace, name, strlen (name));
    }
}

void SetButtonLabel (blist, label)
ButtonList *blist;
char       *label;
{
    Window     w;
    int        bwidth, bheight, width, height;
    ColorPair  cp;
    MyFont     font;
    int        strWid, strHei, hspace, vspace;

    width   = Scr->workSpaceMgr.width;
    height  = Scr->workSpaceMgr.height;
    bwidth  = Scr->workSpaceMgr.bwidth;
    bheight = Scr->workSpaceMgr.bheight;
    font    = Scr->workSpaceMgr.font;

    w    = blist->w;
    blist->label = realloc (blist->label, (strlen (label) + 1));
    strcpy (blist->label, label);
    cp   = blist->cp;

    strHei = font.font->max_bounds.ascent + font.font->max_bounds.descent;
    vspace = ((bheight + strHei) / 2) - font.font->max_bounds.descent;
    XClearWindow (dpy, blist->w);
    if (blist == Scr->workSpaceMgr.activeWSPC) SetButton (blist->w, on);
    else SetButton (blist->w, off);
    FBF (cp.fore, cp.back, font.font->fid);
    strWid = XTextWidth (font.font, label, strlen (label));
    hspace = (bwidth - strWid) / 2;
    if (hspace < 3) hspace = 3;
    XDrawString (dpy, w, Scr->NormalGC, hspace, vspace, label, strlen (label));
}

unsigned int GetMaskFromNameList (res)
char *res;
{
    char       *name;
    char       wrkSpcName [64];
    ButtonList *bl;
    int        mask, num;

    mask = 0;
    while (*res != '\0') {
	while (*res == ' ') res++;
	if (*res == '\0') break;
	name = wrkSpcName;
	while ((*res != '\0') && (*res != ' ')) {
	    *name = *res;
	    name++; res++;
	}
	*name = '\0';
	if (strcmp (wrkSpcName, "all") == 0) {
	    mask = fullOccupation;
	    break;
	}
	num  = 0;
	for (bl = Scr->workSpaceMgr.buttonList; bl != NULL; bl = bl->next) {
	    if (strcmp (wrkSpcName, bl->label) == 0) break;
	    num++;
	}
	if (bl != NULL) mask |= (1 << num);
	else {
	    twmrc_error_prefix();
	    fprintf (stderr, "unknown workspace : %s\n", wrkSpcName);
	}
    }
    return (mask);
}

AddToClientsList (workspace, client)
char *workspace, *client;
{
    ButtonList *blist;
    name_list  *nl;

    if (strcmp (workspace, "all") == 0) {
	blist = Scr->workSpaceMgr.buttonList;
	while (blist != NULL) {
	    AddToList (&blist->clientlist, client, "");
	    blist = blist->next;
	}
	
	return;
    }

    blist = Scr->workSpaceMgr.buttonList;
    while (blist != NULL) {
	if (strcmp (blist->label, workspace) == 0) break;
	blist = blist->next;
    }
    if (blist == NULL) return;
    AddToList (&blist->clientlist, client, "");    
}

MakeListFromMask (mask, list)
int  mask;
char *list;
{
    ButtonList *bl;
    int        num;

    num = 0;
    for (bl = Scr->workSpaceMgr.buttonList; bl != NULL; bl = bl->next) {
	if (mask & (1 << num)) strcat (list, bl->label);
	num++;
    }
}   

