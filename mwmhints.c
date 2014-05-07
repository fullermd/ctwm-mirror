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
 * Code to look at a few Motif Window Manager hints.
 *
 * Only the bits marked [v] are actually looked at.
 * For the rest, ctwm has no concept, really.
 *
 * Lots of information gleaned from the Motif header <Xm/MwmUtil.h> and
 * manual page VendorShell(3motif).
 */

#include <stdio.h>

#include "twm.h"
#include "mwmhints.h"

/*
 * Contents of the _MOTIF_WM_HINTS property.
 */

typedef struct
{
    int	         flags;
    int		 functions;
    int		 decorations;
    int		 input_mode;
    int		 status;
} MotifWmHints;

/* bit definitions for MwmHints.flags */
#define MWM_HINTS_FUNCTIONS	(1L << 0)
#define MWM_HINTS_DECORATIONS	(1L << 1)
#define MWM_HINTS_INPUT_MODE	(1L << 2)
#define MWM_HINTS_STATUS	(1L << 3)

/* bit definitions for MwmHints.functions */
#define MWM_FUNC_ALL		(1L << 0)
#define MWM_FUNC_RESIZE		(1L << 1)
#define MWM_FUNC_MOVE		(1L << 2)
#define MWM_FUNC_MINIMIZE	(1L << 3)
#define MWM_FUNC_MAXIMIZE	(1L << 4)
#define MWM_FUNC_CLOSE		(1L << 5)

/* bit definitions for MwmHints.decorations */
#define MWM_DECOR_ALL		(1L << 0)	/* [v] */
#define MWM_DECOR_BORDER	(1L << 1)	/* [v] */
#define MWM_DECOR_RESIZEH	(1L << 2)
#define MWM_DECOR_TITLE		(1L << 3)	/* [v] */
#define MWM_DECOR_MENU		(1L << 4)
#define MWM_DECOR_MINIMIZE	(1L << 5)
#define MWM_DECOR_MAXIMIZE	(1L << 6)

/* values for MwmHints.input_mode */
#define MWM_INPUT_MODELESS			0
#define MWM_INPUT_PRIMARY_APPLICATION_MODAL	1
#define MWM_INPUT_SYSTEM_MODAL			2
#define MWM_INPUT_FULL_APPLICATION_MODAL	3

/* bit definitions for MwmHints.status */
#define MWM_TEAROFF_WINDOW	(1L << 0)

static Atom MOTIF_WM_HINTS;

static int GetMWMHints(Display *dpy, Window w, MotifWmHints *mwmHints)
{
    mwmHints->flags = 0;	/* if not found, there are none */
    mwmHints->functions = 0;
    mwmHints->decorations = 0;
    mwmHints->input_mode = 0;
    mwmHints->status = 0;

    if (MOTIF_WM_HINTS == 0) {
	MOTIF_WM_HINTS = XInternAtom(dpy, "_MOTIF_WM_HINTS", True);
    }

    int success;
    Atom actual_type;
    int actual_format;
    unsigned long nitems;
    unsigned long bytes_after;
    unsigned char *prop = NULL;

    success = XGetWindowProperty(
			dpy, w, MOTIF_WM_HINTS,
			0, 5, 		/* long_offset, long long_length, */
			False,		/* Bool delete, */
			AnyPropertyType,/* Atom req_type */
			&actual_type, 	/* Atom *actual_type_return, */
			&actual_format, /* int *actual_format_return, */
			&nitems, 	/* unsigned long *nitems_return,  */
			&bytes_after, 	/* unsigned long * */
			&prop);		/* unsigned char ** */

    if (success == Success &&
	    actual_type == MOTIF_WM_HINTS &&
	    actual_format == 32 &&
	    nitems >= 3) {
	mwmHints->flags = ((unsigned long *)prop)[0];
	mwmHints->functions = ((unsigned long *)prop)[1];
	mwmHints->decorations = ((unsigned long *)prop)[2];
	if (nitems >= 4) {
	    mwmHints->input_mode = ((unsigned long *)prop)[3];
	    if (nitems >= 5) {
		mwmHints->status = ((unsigned long *)prop)[4];
	    }
	}

	success = True;
    } else {
	success = False;
    }

    if (prop != NULL) {
    	XFree(prop);
    }

    return success;
}

/*
 * Apply the MWM hints, if possible.
 *
 * The only ones we support are "without title" and "without border".
 *
 * If other configuration decides to have no title or border,
 * these options won't reverse that.
 */

void ApplyMWMHints(Display *dpy, TwmWindow *twmWin)
{
    MotifWmHints mwmHints;

    if (GetMWMHints(dpy, twmWin->w, &mwmHints)) {
	if (mwmHints.flags & MWM_HINTS_DECORATIONS) {
	    if (mwmHints.decorations & MWM_DECOR_ALL) {
		mwmHints.decorations ^= ~0;
	    }

	    if ((mwmHints.decorations & MWM_DECOR_BORDER) == 0) {
		twmWin->frame_bw = 0;
		twmWin->frame_bw3D = 0;
	    }

	    if ((mwmHints.decorations & MWM_DECOR_TITLE) == 0) {
		twmWin->title_height = 0;
	    }
	}
    }
}
