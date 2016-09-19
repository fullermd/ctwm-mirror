/*
 * Window util funcs
 */
#ifndef _CTWM_WIN_UTILS_H
#define _CTWM_WIN_UTILS_H


void GetWindowSizeHints(TwmWindow *tmp_win);
void FetchWmProtocols(TwmWindow *tmp);
void GetGravityOffsets(TwmWindow *tmp, int *xp, int *yp);
TwmWindow *GetTwmWindow(Window w);
unsigned char *GetWMPropertyString(Window w, Atom prop);
void FreeWMPropertyString(char *prop);
bool visible(const TwmWindow *tmp_win);
long mask_out_event(Window w, long ignore_event);
long mask_out_event_mask(Window w, long ignore_event, long curmask);
int restore_mask(Window w, long restore);
void SetMapStateProp(TwmWindow *tmp_win, int state);
bool GetWMState(Window w, int *statep, Window *iwp);
void DisplayPosition(const TwmWindow *_unused_tmp_win, int x, int y);


#endif /* _CTWM_WIN_UTILS_H */
