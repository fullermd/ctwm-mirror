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

#ifndef _EWMH_
#define _EWMH_

#ifdef EWMH

#include "types.h"

extern Atom NET_CURRENT_DESKTOP;

extern void EwmhInit(void);
extern int EwmhInitScreenEarly(ScreenInfo *scr);
extern void EwmhInitScreenLate(ScreenInfo *scr);
extern void EwmhInitVirtualRoots(ScreenInfo *scr);
extern void EwmhTerminate(void);
extern void EwhmSelectionClear(XSelectionClearEvent *sev);
extern int EwmhClientMessage(XClientMessageEvent *msg);
extern Image *EwhmGetIcon(ScreenInfo *scr, TwmWindow *twm_win);
extern int EwmhHandlePropertyNotify(XPropertyEvent *event, TwmWindow *twm_win);
extern void EwmhSet_NET_WM_DESKTOP(TwmWindow *twm_win);
extern void EwmhSet_NET_WM_DESKTOP_ws(TwmWindow *twm_win, WorkSpace *ws);
extern int EwmhGetOccupation(TwmWindow *twm_win);
extern void EwmhUnmapNotify(TwmWindow *twm_win);
extern void EwmhAddClientWindow(TwmWindow *new_win);
extern void EwmhDeleteClientWindow(TwmWindow *old_win);
extern void EwmhSet_NET_CLIENT_LIST_STACKING(void);

#endif /* EWMH */
#endif /* _EWMH_ */
