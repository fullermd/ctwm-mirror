#ifndef _WORKMGR_
#define _WORKMGR_

#define MAXWORKSPACE 32

#ifdef ultrix
#   define strdup(s) ((char*) strcpy ((char*) malloc (strlen (s) + 1), s))
#endif

void CreateWorkSpaceManager ();
void PaintWorkSpaceManager  ();
void SetButtonLabel         ();

extern int workSpaceManagerActive;

typedef struct OccupyWindow {
    Window     w;
    TwmWindow  *twm_win;
    int        x, y;
    int        width, height;
    char       *name;
    char       *icon_name;
    int        hspace;
    Window     OK, cancel, allworkspc;
    int        tmpOccupation;
} OccupyWindow;

typedef struct ButtonList {
    Window	w;
    Window	ow;
    int		number;
    char	*label;
    ColorPair	cp;
    IconMgr	*iconmgr;
    ColorPair	backcp;
    Pixmap	backpix;
    name_list	*clientlist;
    struct ButtonList *next;
} ButtonList;

typedef struct WorkMgr {
    Window       w;
    ButtonList   *buttonList;
    ButtonList   *activeWSPC;
    TwmWindow    *twm_win;
    int          x, y;
    int          width, height;
    ColorPair    cp;
    MyFont       font;
    int          lines, columns;
    char         *name;
    char         *icon_name;
    char         *geometry;
    int          hspace, vspace;
    int          bwidth, bheight;
    int          count;
    OccupyWindow occupyWindow;
} WorkMgr;

#endif
