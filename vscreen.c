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

#include <X11/Xatom.h>
#include <stdio.h>
#include "twm.h"
#include "screen.h"

void InitVirtualScreens (ScreenInfo *scr) {
  Cursor cursor;
  unsigned long valuemask, attrmask;
  XSetWindowAttributes attributes;
  name_list *nptr;
  Atom _XA_WM_VIRTUALROOT = XInternAtom (dpy, "WM_VIRTUALROOT", False);
  Bool userealroot = True;

  NewFontCursor (&cursor, "X_cursor");

  if (scr->VirtualScreens == NULL) {
    if (userealroot) {
      virtualScreen *vs = (virtualScreen*) malloc (sizeof (virtualScreen));
      vs->x      = 0;
      vs->y      = 0;
      vs->w      = scr->rootw;
      vs->h      = scr->rooth;
      vs->window = scr->Root;
      vs->next   = NULL;
      scr->vScreenList = vs;
      scr->currentvs   = vs;
      return;
    } else {
      scr->VirtualScreens = (name_list*) malloc (sizeof (name_list));
      scr->VirtualScreens->next = NULL;
      scr->VirtualScreens->name = (char*) malloc (64);
      sprintf (scr->VirtualScreens->name, "%dx%d+0+0", scr->rootw, scr->rooth);
    }
  }
  attrmask  = ColormapChangeMask | EnterWindowMask | PropertyChangeMask | 
              SubstructureRedirectMask | KeyPressMask | ButtonPressMask |
              ButtonReleaseMask;

  valuemask = CWBackingStore | CWSaveUnder | CWBackPixel | CWOverrideRedirect |
              CWEventMask | CWCursor;
  attributes.backing_store     = NotUseful;
  attributes.save_under	       = False;
  attributes.override_redirect = True;
  attributes.event_mask	       = attrmask;
  attributes.cursor    	       = cursor;
  attributes.background_pixel  = Scr->Black;

  scr->vScreenList = NULL;
  for (nptr = Scr->VirtualScreens; nptr != NULL; nptr = nptr->next) {
    virtualScreen *vs;
    char *geometry = (char*) nptr->name;
    int x, y, w, h;
    unsigned int mask = XParseGeometry (geometry, &x, &y, &w, &h);

    if ((x < 0) || (y < 0) || (w > scr->rootw) || (h > scr->rooth)) {
      twmrc_error_prefix ();
      fprintf (stderr, "InitVirtualScreens : invalid geometry : %s\n", geometry);
      continue;
    }
    vs = (virtualScreen*) malloc (sizeof (virtualScreen));
    vs->x = x;
    vs->y = y;
    vs->w = w;
    vs->h = h;
    vs->window = XCreateWindow (dpy, Scr->Root, x, y, w, h,
			       0, CopyFromParent, (unsigned int) CopyFromParent,
			       (Visual *) CopyFromParent, valuemask, &attributes);
    XSync (dpy, 0);
    XMapWindow (dpy, vs->window);
    XChangeProperty (dpy, vs->window, _XA_WM_VIRTUALROOT, XA_STRING, 8, 
		     PropModeReplace, (unsigned char *) "Yes", 4);

    vs->next = scr->vScreenList;
    scr->vScreenList = vs;
  }
  if (scr->vScreenList == NULL) {
    twmrc_error_prefix ();
    fprintf (stderr, "no valid VirtualScreens found, exiting...\n");
    exit (1);
  }
}

virtualScreen *getVScreenOf (x, y)
int x, y;
{
  virtualScreen *vs;
  for (vs = Scr->vScreenList; vs != NULL; vs = vs->next) {
    if ((x >= vs->x) && ((x - vs->x) < vs->w) &&
	(y >= vs->y) && ((y - vs->y) < vs->h)) {
        return vs;
    }
  }
  return Scr->vScreenList;
}
