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

#include <stdio.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "ewmh.h"
#include "twm.h"
#include "screen.h"

static Atom NET_VIRTUAL_ROOTS;


static void EwmhInitAtoms(void)
{
    if (NET_VIRTUAL_ROOTS == (Atom)None) {
	NET_VIRTUAL_ROOTS = XInternAtom(dpy, "_NET_VIRTUAL_ROOTS", True);
    }
}

void EwmhInit(void)
{
    EwmhInitAtoms();
}

void EwmhInitScreen(ScreenInfo *scr)
{
}

void EwmhInitVirtualRoots(ScreenInfo *scr)
{
    int numVscreens = scr->numVscreens;

    if (numVscreens > 1) {
	long *data;
	VirtualScreen *vs;
	int i;

	data = calloc(numVscreens, sizeof(long));

	for (vs = scr->vScreenList, i = 0;
		vs != NULL && i < numVscreens;
		vs = vs->next, i++) {
	    data[i] = vs->window;
	}

	XChangeProperty(dpy, Scr->XineramaRoot,
			NET_VIRTUAL_ROOTS, XA_WINDOW,
			32,
			PropModeReplace,
			(unsigned char *)data,
			numVscreens);

	/* This might fail, but what can we do about it? */

	free(data);
    }
}
