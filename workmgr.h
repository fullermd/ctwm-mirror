#ifndef _WORKMGR_
#define _WORKMGR_

#define MAXWORKSPACE 32
#define MAPSTATE      0
#define BUTTONSSTATE  1

#if defined(ultrix) || defined(sequent) || defined(sony_news)
#   define strdup(s) ((char*) strcpy ((char*) malloc (strlen (s) + 1), s))
#endif

void CreateWorkSpaceManager ();

typedef struct WorkSpaceList WorkSpaceList;

typedef struct winList {
    Window		w;
    int			x, y;
    int			width, height;
    TwmWindow		*twm_win;
    ColorPair		cp;
    MyFont		font;
    struct winList	*next;
} *WinList;

typedef struct mapSubwindow {
    Window		w;
    Window		blanket;
    int			x, y;
    WinList		wl;
} MapSubwindow;

typedef struct WorkSpaceWindow {
    Window		w;
    TwmWindow		*twm_win;
    char		*geometry;
    int			x, y;
    char		*name;
    char		*icon_name;
    int			state;
    int			lines, columns;
    int			noshowoccupyall;

    int			width, height;
    int			bwidth, bheight;
    int			hspace, vspace;
    ColorPair		cp;
    MyFont		buttonFont;

    int			wwidth, wheight;
    name_list		*windowBackgroundL;
    name_list		*windowForegroundL;
    ColorPair		windowcp;
    MyFont		windowFont;

    ColorPair		curColors;
    Pixmap		curPixmap;
    unsigned long	curBorderColor;

    ColorPair		defColors;
    Pixmap		defPixmap;
    unsigned long	defBorderColor;
} WorkSpaceWindow;

typedef struct OccupyWindow {
    Window		w;
    TwmWindow		*twm_win;
    char		*geometry;
    Window		OK, cancel, allworkspc;
    int			x, y;
    int			width, height;
    char		*name;
    char		*icon_name;
    int			lines, columns;
    int			hspace, vspace;
    int			bwidth, bheight;
    int			owidth, oheight;
    ColorPair		cp;
    MyFont		font;
    int			tmpOccupation;
} OccupyWindow;

struct WorkSpaceList {
    Window		buttonw;
    Window		obuttonw;
    int			number;
    char		*name;
    char		*label;
    ColorPair		cp;
    IconMgr		*iconmgr;
    ColorPair		backcp;
    Pixmap		backpix;
    name_list		*clientlist;
    MapSubwindow	mapSubwindow;
    struct WorkSpaceList *next;
};

typedef struct WorkSpaceMgr {
    WorkSpaceList	*workSpaceList;
    WorkSpaceList	*activeWSPC;
    WorkSpaceWindow	workspaceWindow;
    OccupyWindow	occupyWindow;
    int			count;
} WorkSpaceMgr;


#endif
