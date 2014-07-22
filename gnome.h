#include "ctwm.h"

#ifndef _GNOME_
#define _GNOME_

typedef struct {
	Window *ws;
	int wsSize;
	int numWins;
} GnomeData;

extern Atom _XA_WIN_WORKSPACE;
extern Atom _XA_WIN_STATE;

void InitGnome(void);
void GnomeAddClientWindow(TwmWindow *new_win);
void GnomeDeleteClientWindow(TwmWindow *new_win);

#endif /* _GNOME_ */
