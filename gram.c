#ifdef __cplusplus
   #include <stdio.h>
   extern "C" {
     extern void yyerror(char *);
     extern int yylex();
   }
#endif	/* __cplusplus */ 

# line 42 "gram.y"
#include <stdio.h>
#include <ctype.h>
#include "twm.h"
#include "menus.h"
#include "list.h"
#include "util.h"
#include "screen.h"
#include "parse.h"
#include <X11/Xos.h>
#include <X11/Xmu/CharSet.h>

static char *Action = "";
static char *Name = "";
static MenuRoot	*root, *pull = NULL;
static char *curWorkSpc;

static MenuRoot *GetRoot();

static Bool CheckWarpScreenArg(), CheckWarpRingArg();
static Bool CheckColormapArg();
static void GotButton(), GotKey(), GotTitleButton();
static char *ptr;
static name_list **list;
static int cont = 0;
static int color;
int mods = 0;
unsigned int mods_used = (ShiftMask | ControlMask | Mod1Mask);

extern int do_single_keyword(), do_string_keyword(), do_number_keyword();
extern name_list **do_colorlist_keyword();
extern int do_color_keyword(), do_string_savecolor();
extern int yylineno;

# line 76 "gram.y"
typedef union 
{
    int num;
    char *ptr;
} YYSTYPE;
# define LB 257
# define RB 258
# define LP 259
# define RP 260
# define MENUS 261
# define MENU 262
# define BUTTON 263
# define DEFAULT_FUNCTION 264
# define PLUS 265
# define MINUS 266
# define ALL 267
# define OR 268
# define CURSORS 269
# define PIXMAPS 270
# define ICONS 271
# define COLOR 272
# define SAVECOLOR 273
# define MONOCHROME 274
# define FUNCTION 275
# define ICONMGR_SHOW 276
# define ICONMGR 277
# define WINDOW_FUNCTION 278
# define ZOOM 279
# define ICONMGRS 280
# define ICONMGR_GEOMETRY 281
# define ICONMGR_NOSHOW 282
# define MAKE_TITLE 283
# define ICONIFY_BY_UNMAPPING 284
# define DONT_ICONIFY_BY_UNMAPPING 285
# define NO_TITLE 286
# define AUTO_RAISE 287
# define NO_HILITE 288
# define ICON_REGION 289
# define META 290
# define SHIFT 291
# define LOCK 292
# define CONTROL 293
# define WINDOW 294
# define TITLE 295
# define ICON 296
# define ROOT 297
# define FRAME 298
# define COLON 299
# define EQUALS 300
# define SQUEEZE_TITLE 301
# define DONT_SQUEEZE_TITLE 302
# define START_ICONIFIED 303
# define NO_TITLE_HILITE 304
# define TITLE_HILITE 305
# define MOVE 306
# define RESIZE 307
# define WAIT 308
# define SELECT 309
# define KILL 310
# define LEFT_TITLEBUTTON 311
# define RIGHT_TITLEBUTTON 312
# define NUMBER 313
# define KEYWORD 314
# define NKEYWORD 315
# define CKEYWORD 316
# define CLKEYWORD 317
# define FKEYWORD 318
# define FSKEYWORD 319
# define SKEYWORD 320
# define DKEYWORD 321
# define JKEYWORD 322
# define WINDOW_RING 323
# define WARP_CURSOR 324
# define ERRORTOKEN 325
# define NO_STACKMODE 326
# define WORKSPACES 327
# define WORKSPCMGR_GEOMETRY 328
# define OCCUPYALL 329
# define STRING 330
#define yyclearin yychar = -1
#define yyerrok yyerrflag = 0
extern int yychar;
#ifndef YYMAXDEPTH
#define YYMAXDEPTH 150
#endif

/* __YYSCLASS defines the scoping/storage class for global objects
 * that are NOT renamed by the -p option.  By default these names
 * are going to be 'static' so that multi-definition errors
 * will not occur with multiple parsers.
 * If you want (unsupported) access to internal names you need
 * to define this to be null so it implies 'extern' scope.
 * This should not be used in conjunction with -p.
 */
#ifndef __YYSCLASS
# define __YYSCLASS static
#endif
YYSTYPE yylval;
__YYSCLASS YYSTYPE yyval;
typedef int yytabelem;
# define YYERRCODE 256

# line 681 "gram.y"

yyerror(s) char *s;
{
    twmrc_error_prefix();
    fprintf (stderr, "error in input file:  %s\n", s ? s : "");
    ParseError = 1;
}
RemoveDQuote(str)
char *str;
{
    register char *i, *o;
    register n;
    register count;

    for (i=str+1, o=str; *i && *i != '\"'; o++)
    {
	if (*i == '\\')
	{
	    switch (*++i)
	    {
	    case 'n':
		*o = '\n';
		i++;
		break;
	    case 'b':
		*o = '\b';
		i++;
		break;
	    case 'r':
		*o = '\r';
		i++;
		break;
	    case 't':
		*o = '\t';
		i++;
		break;
	    case 'f':
		*o = '\f';
		i++;
		break;
	    case '0':
		if (*++i == 'x')
		    goto hex;
		else
		    --i;
	    case '1': case '2': case '3':
	    case '4': case '5': case '6': case '7':
		n = 0;
		count = 0;
		while (*i >= '0' && *i <= '7' && count < 3)
		{
		    n = (n<<3) + (*i++ - '0');
		    count++;
		}
		*o = n;
		break;
	    hex:
	    case 'x':
		n = 0;
		count = 0;
		while (i++, count++ < 2)
		{
		    if (*i >= '0' && *i <= '9')
			n = (n<<4) + (*i - '0');
		    else if (*i >= 'a' && *i <= 'f')
			n = (n<<4) + (*i - 'a') + 10;
		    else if (*i >= 'A' && *i <= 'F')
			n = (n<<4) + (*i - 'A') + 10;
		    else
			break;
		}
		*o = n;
		break;
	    case '\n':
		i++;	/* punt */
		o--;	/* to account for o++ at end of loop */
		break;
	    case '\"':
	    case '\'':
	    case '\\':
	    default:
		*o = *i++;
		break;
	    }
	}
	else
	    *o = *i++;
    }
    *o = '\0';
}

static MenuRoot *GetRoot(name, fore, back)
char *name;
char *fore, *back;
{
    MenuRoot *tmp;

    tmp = FindMenuRoot(name);
    if (tmp == NULL)
	tmp = NewMenuRoot(name);

    if (fore)
    {
	int save;

	save = Scr->FirstTime;
	Scr->FirstTime = TRUE;
	GetColor(COLOR, &tmp->hi_fore, fore);
	GetColor(COLOR, &tmp->hi_back, back);
	Scr->FirstTime = save;
    }

    return tmp;
}

static void GotButton(butt, func)
int butt, func;
{
    int i;

    for (i = 0; i < NUM_CONTEXTS; i++)
    {
	if ((cont & (1 << i)) == 0)
	    continue;

	Scr->Mouse[butt][i][mods].func = func;
	if (func == F_MENU)
	{
	    pull->prev = NULL;
	    Scr->Mouse[butt][i][mods].menu = pull;
	}
	else
	{
	    root = GetRoot(TWM_ROOT, NULLSTR, NULLSTR);
	    Scr->Mouse[butt][i][mods].item = AddToMenu(root,"x",Action,
		    NULLSTR, func, NULLSTR, NULLSTR);
	}
    }
    Action = "";
    pull = NULL;
    cont = 0;
    mods_used |= mods;
    mods = 0;
}

static void GotKey(key, func)
char *key;
int func;
{
    int i;

    for (i = 0; i < NUM_CONTEXTS; i++)
    {
	if ((cont & (1 << i)) == 0) 
	  continue;
	if (!AddFuncKey(key, i, mods, func, Name, Action)) 
	  break;
    }

    Action = "";
    pull = NULL;
    cont = 0;
    mods_used |= mods;
    mods = 0;
}


static void GotTitleButton (bitmapname, func, rightside)
    char *bitmapname;
    int func;
    Bool rightside;
{
    if (!CreateTitleButton (bitmapname, func, Action, pull, rightside, True)) {
	twmrc_error_prefix();
	fprintf (stderr, 
		 "unable to create %s titlebutton \"%s\"\n",
		 rightside ? "right" : "left", bitmapname);
    }
    Action = "";
    pull = NULL;
}

static Bool CheckWarpScreenArg (s)
    register char *s;
{
    XmuCopyISOLatin1Lowered (s, s);

    if (strcmp (s,  WARPSCREEN_NEXT) == 0 ||
	strcmp (s,  WARPSCREEN_PREV) == 0 ||
	strcmp (s,  WARPSCREEN_BACK) == 0)
      return True;

    for (; *s && isascii(*s) && isdigit(*s); s++) ; /* SUPPRESS 530 */
    return (*s ? False : True);
}


static Bool CheckWarpRingArg (s)
    register char *s;
{
    XmuCopyISOLatin1Lowered (s, s);

    if (strcmp (s,  WARPSCREEN_NEXT) == 0 ||
	strcmp (s,  WARPSCREEN_PREV) == 0)
      return True;

    return False;
}


static Bool CheckColormapArg (s)
    register char *s;
{
    XmuCopyISOLatin1Lowered (s, s);

    if (strcmp (s, COLORMAP_NEXT) == 0 ||
	strcmp (s, COLORMAP_PREV) == 0 ||
	strcmp (s, COLORMAP_DEFAULT) == 0)
      return True;

    return False;
}


twmrc_error_prefix ()
{
    fprintf (stderr, "%s:  line %d:  ", ProgramName, yylineno);
}
__YYSCLASS yytabelem yyexca[] ={
-1, 1,
	0, -1,
	-2, 0,
-1, 2,
	0, 1,
	-2, 0,
-1, 15,
	257, 18,
	-2, 20,
-1, 23,
	257, 33,
	-2, 35,
-1, 26,
	257, 40,
	-2, 42,
-1, 27,
	257, 43,
	-2, 45,
-1, 28,
	257, 46,
	-2, 48,
-1, 29,
	257, 49,
	-2, 51,
-1, 41,
	257, 73,
	-2, 75,
-1, 46,
	257, 160,
	-2, 159,
-1, 47,
	257, 163,
	-2, 162,
-1, 202,
	257, 176,
	-2, 175,
-1, 263,
	257, 147,
	-2, 146,
	};
# define YYNPROD 211
# define YYLAST 471
__YYSCLASS yytabelem yyact[]={

     4,    49,   265,   139,    99,   107,    33,    48,    39,   297,
    65,    66,    54,    14,    13,    35,    36,    37,    38,    34,
    25,    67,    40,    12,    24,    10,    23,    30,    15,    20,
    29,    32,    27,     9,   176,   289,    54,   256,   212,    65,
    66,   106,   274,   259,   140,    46,    47,    31,    26,   216,
   209,    49,   203,    49,   200,    16,    17,   194,    43,    45,
    69,   293,   206,   301,    44,    19,   170,    42,    41,   272,
    28,    21,    11,    22,    49,    50,    51,    52,   126,    63,
   171,    49,    60,    61,    62,   250,   251,   278,   279,   285,
   269,   142,   173,   179,   105,   248,   215,   214,   137,    83,
    84,    65,    66,    65,    66,   132,    49,    49,   249,    49,
    94,   130,   243,   244,   245,   246,   247,   241,   117,    90,
    91,    49,    49,   143,    49,   113,    49,   153,   149,    49,
    88,   165,   108,    58,    56,    54,   131,   109,   257,   207,
   260,   208,   148,   210,   174,   195,   156,   290,    49,   280,
   268,   253,   201,   161,   162,   163,   164,   165,   167,   204,
   146,   147,   166,   145,   168,    97,   239,   240,   180,    96,
   298,   150,   151,   152,   154,   155,   237,   294,   177,   161,
   162,   163,   164,   133,   284,   275,   157,   158,   159,   238,
    53,   213,   169,   232,   233,   234,   235,   236,   230,   178,
   175,   144,   103,   141,   102,   242,   231,   110,   182,   160,
   199,   183,   184,   185,   186,   187,   188,   189,   190,   191,
   192,   193,   196,   197,   104,    93,   134,    92,    89,    87,
    86,   129,    85,   202,   205,   172,    95,   128,   127,    98,
   211,   282,   100,   101,    82,    81,   217,    80,    79,   219,
   220,   221,   222,   223,   224,   225,   226,   227,   228,   229,
    78,    77,    76,    75,   116,   252,    74,    73,    72,   112,
    71,   254,   255,   258,    70,    59,    57,   262,    55,     8,
   263,   264,     7,     6,     5,     3,     2,     1,   261,    68,
    64,   276,    18,     0,     0,   111,     0,   114,   115,     0,
   118,   119,   120,   121,   122,   123,   124,   125,     0,     0,
   266,     0,     0,     0,     0,     0,     0,   135,   136,     0,
   271,   267,   138,     0,     0,     0,     0,     0,     0,     0,
   181,     0,     0,     0,     0,     0,     0,     0,   273,     0,
   283,     0,     0,     0,     0,     0,   291,     0,     0,   292,
     0,     0,   198,     0,     0,     0,     0,   295,     0,   296,
   299,   300,     0,     0,     0,   302,   303,     0,     0,   305,
     0,     0,   218,     0,     0,     0,     0,     0,     0,     0,
     0,   304,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,   270,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,   277,     0,     0,     0,
     0,     0,   281,     0,     0,     0,     0,   286,     0,   287,
   288 };
__YYSCLASS yytabelem yypact[]={

 -3000, -3000,  -256, -3000, -3000, -3000, -3000, -3000, -3000,  -329,
  -329,  -329,  -301,  -123,  -124, -3000,  -329,  -329,  -279,  -240,
 -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000,
 -3000, -3000, -3000,  -329,  -329, -3000, -3000,  -127, -3000,  -308,
  -308, -3000, -3000, -3000,  -329,  -301, -3000, -3000,  -301, -3000,
  -317,  -301,  -301, -3000, -3000, -3000, -3000, -3000, -3000,  -163,
  -259,  -295, -3000, -3000, -3000, -3000,  -329, -3000, -3000, -3000,
  -163,  -132,  -163,  -163,  -139,  -163,  -163,  -163,  -163,  -163,
  -163,  -163,  -163,  -181, -3000,  -146,  -152, -3000, -3000,  -152,
 -3000, -3000,  -163,  -163, -3000, -3000,  -159,  -163, -3000,  -318,
 -3000, -3000,  -214,  -135, -3000, -3000,  -308,  -308, -3000,  -111,
  -137, -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000,
 -3000, -3000, -3000, -3000, -3000, -3000,  -329,  -177,  -165, -3000,
 -3000, -3000, -3000,  -224, -3000, -3000, -3000, -3000, -3000,  -301,
 -3000, -3000,  -329, -3000, -3000,  -329,  -329,  -329,  -329,  -329,
  -329,  -329,  -329,  -329,  -329,  -329,  -201, -3000, -3000, -3000,
 -3000,  -301, -3000, -3000, -3000, -3000, -3000,  -204,  -206,  -237,
 -3000, -3000, -3000, -3000,  -208,  -220, -3000, -3000, -3000, -3000,
  -209,  -301, -3000,  -329,  -329,  -329,  -329,  -329,  -329,  -329,
  -329,  -329,  -329,  -329, -3000, -3000, -3000,  -101, -3000,  -182,
 -3000, -3000, -3000, -3000, -3000,  -329,  -329,  -221,  -215, -3000,
 -3000,  -329, -3000, -3000,  -329,  -329, -3000,  -320, -3000, -3000,
 -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000,
  -308, -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000,
 -3000,  -308, -3000, -3000, -3000, -3000, -3000, -3000, -3000, -3000,
 -3000, -3000, -3000,  -167,  -277,  -191, -3000, -3000,  -217, -3000,
 -3000, -3000, -3000, -3000, -3000,  -178, -3000, -3000, -3000, -3000,
 -3000,  -301, -3000, -3000,  -329,  -168,  -301, -3000,  -301,  -301,
  -223, -3000,  -177,  -238, -3000, -3000, -3000, -3000, -3000, -3000,
 -3000,  -329, -3000,  -329,  -249,  -329,  -197, -3000, -3000,  -329,
  -329,  -308, -3000,  -329, -3000, -3000 };
__YYSCLASS yytabelem yypgo[]={

     0,    65,    79,   292,   190,   291,   290,   289,   287,   286,
   285,   284,   283,   282,   279,   278,   276,   275,   224,   274,
   270,   269,   268,   267,   266,   264,   263,   262,   261,   260,
   248,   247,   245,   244,   241,    66,   238,   237,   235,   232,
   231,   230,   136,   229,   228,   227,   225,   137,   223,   210,
   209,   206,   205,   204,   203,   202,   201,   200,   191,   185,
   184,   183,   178,   177,   170,   169,   168,   165,   164,   159,
   158,   152,   151,   150,   149,   147,   146,   145,   144,   143,
   141,   140,   139,   138 };
__YYSCLASS yytabelem yyr1[]={

     0,     8,     9,     9,    10,    10,    10,    10,    10,    10,
    10,    10,    10,    10,    10,    10,    10,    10,    17,    10,
    10,    10,    10,    10,    10,    10,    10,    19,    10,    20,
    10,    22,    10,    23,    10,    10,    24,    10,    26,    10,
    27,    10,    10,    28,    10,    10,    29,    10,    10,    30,
    10,    10,    31,    10,    32,    10,    33,    10,    34,    10,
    36,    10,    37,    10,    39,    10,    41,    10,    10,    44,
    10,    10,    10,    45,    10,    10,    46,    10,    11,    12,
    13,     6,     7,    47,    47,    50,    50,    50,    50,    50,
    50,    48,    48,    51,    51,    51,    51,    51,    51,    51,
    51,    51,    49,    49,    52,    52,    52,    52,    52,    52,
    52,    52,    52,    52,    15,    53,    53,    54,    16,    55,
    55,    56,    56,    56,    56,    56,    56,    56,    56,    56,
    56,    56,    56,    56,    56,    56,    56,    56,    56,    56,
    56,    56,    56,    42,    57,    57,    58,    59,    58,    58,
    43,    61,    61,    62,    62,    60,    63,    63,    64,    14,
    65,    14,    14,    67,    14,    66,    66,    25,    68,    68,
    69,    69,    21,    70,    70,    71,    72,    71,    73,    74,
    74,    75,    75,    75,    75,    75,    18,    76,    76,    77,
    40,    78,    78,    79,    38,    80,    80,    81,    35,    82,
    82,    83,    83,     2,     2,     5,     5,     5,     3,     1,
     4 };
__YYSCLASS yytabelem yyr2[]={

     0,     2,     0,     4,     2,     2,     2,     2,     2,    13,
     7,     5,     7,     5,     5,     3,     5,     5,     1,     6,
     3,     9,     9,     5,     5,     5,     5,     1,     6,     1,
     6,     1,     6,     1,     6,     3,     1,     6,     1,     6,
     1,     6,     3,     1,     6,     3,     1,     6,     3,     1,
     6,     3,     1,     6,     1,     6,     1,     6,     1,    19,
     1,     9,     1,     8,     1,     6,     1,     6,     4,     1,
     6,     5,     5,     1,     6,     3,     1,     6,     3,     5,
     5,    13,    13,     0,     4,     3,     3,     3,     3,     5,
     3,     0,     4,     3,     3,     3,     3,     3,     3,     3,
     3,     3,     0,     4,     3,     3,     3,     3,     3,     3,
     3,     3,     3,     3,     6,     0,     4,     5,     6,     0,
     4,     7,     5,     7,     5,     7,     5,     7,     5,     7,
     5,     7,     5,     7,     5,     7,     5,     7,     5,     7,
     5,     7,     5,     6,     0,     4,     5,     1,     8,     5,
     6,     0,     4,     3,     3,     6,     0,     4,     5,     3,
     1,    10,     3,     1,     6,     0,    11,     6,     0,     4,
     7,     9,     6,     0,     4,     3,     1,     6,     6,     0,
     4,     3,     5,     7,     9,    11,     6,     0,     4,     3,
     6,     0,     4,     5,     6,     0,     4,     3,     6,     0,
     4,     5,    15,     3,     5,     3,     5,     5,     5,     3,
     3 };
__YYSCLASS yytabelem yychk[]={

 -3000,    -8,    -9,   -10,   256,   -11,   -12,   -13,   -14,   289,
   281,   328,   279,   270,   269,   284,   311,   312,    -3,    -1,
   285,   327,   329,   282,   280,   276,   304,   288,   326,   286,
   283,   303,   287,   262,   275,   271,   272,   273,   274,   264,
   278,   324,   323,   314,   320,   315,   301,   302,   263,   330,
    -1,    -1,    -1,    -4,   313,   -15,   257,   -16,   257,   -17,
    -1,    -1,    -1,    -2,    -6,   318,   319,   300,    -7,   300,
   -19,   -20,   -22,   -23,   -24,   -26,   -27,   -28,   -29,   -30,
   -31,   -32,   -33,    -1,    -1,   -39,   -41,   -43,   257,   -44,
    -2,    -2,   -45,   -46,    -1,    -4,   -65,   -67,    -4,   321,
    -4,    -4,   -53,   -55,   -18,   257,   300,   300,    -1,   -47,
   -47,   -18,   -21,   257,   -18,   -18,   -25,   257,   -18,   -18,
   -18,   -18,   -18,   -18,   -18,   -18,   259,   -36,   -37,   -40,
   257,   -42,   257,   -61,   -42,   -18,   -18,   257,   -18,   321,
   258,   -54,   305,   258,   -56,   298,   295,   296,   277,   263,
   306,   307,   308,   262,   309,   310,   -76,    -2,    -2,   299,
   -50,   290,   291,   292,   293,   268,   299,   -70,   -68,    -1,
   -35,   257,   -38,   257,   -78,   -57,   258,   -62,    -1,   317,
   -66,    -4,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    -1,    -1,   258,   -77,    -1,   -48,    -4,   -49,
   258,   -71,    -1,   258,   -69,    -1,   299,   -82,   -80,   258,
   -79,    -1,   258,   -58,   317,   316,   258,    -1,    -4,    -1,
    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
   299,   -51,   294,   295,   296,   297,   298,   277,   290,   267,
   268,   299,   -52,   294,   295,   296,   297,   298,   277,   290,
   267,   268,    -1,   -72,    -1,    -1,   258,   -83,    -1,   258,
   -81,    -2,    -1,    -1,    -1,   322,    -2,    -2,   -73,   257,
    -4,    -1,   260,    -2,   259,   -59,    -5,    -4,   265,   266,
   -74,    -4,   -34,    -1,   -60,   257,    -4,    -4,    -4,   258,
   -75,    -1,   -35,   299,   -63,    -1,    -1,   258,   -64,    -1,
    -1,   260,    -1,    -1,    -2,    -1 };
__YYSCLASS yytabelem yydef[]={

     2,    -2,    -2,     3,     4,     5,     6,     7,     8,     0,
     0,     0,    15,     0,     0,    -2,     0,     0,     0,     0,
    27,    29,    31,    -2,    36,    38,    -2,    -2,    -2,    -2,
    52,    54,    56,     0,     0,    64,    66,     0,    69,     0,
     0,    -2,    76,    78,     0,     0,    -2,    -2,     0,   209,
     0,    11,    13,    14,   210,    16,   115,    17,   119,     0,
     0,     0,    23,    24,    26,   203,     0,    83,    25,    83,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,    60,    62,     0,     0,    68,   151,     0,
    71,    72,     0,     0,    79,    80,     0,     0,   208,     0,
    10,    12,     0,     0,    19,   187,     0,     0,   204,     0,
     0,    28,    30,   173,    32,    34,    37,   168,    39,    41,
    44,    47,    50,    53,    55,    57,     0,     0,     0,    65,
   191,    67,   144,     0,    70,    74,    77,   165,   164,     0,
   114,   116,     0,   118,   120,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,    21,    22,    91,
    84,    85,    86,    87,    88,    90,   102,     0,     0,     0,
    61,   199,    63,   195,     0,     0,   150,   152,   153,   154,
     0,     0,   117,   122,   124,   126,   128,   130,   132,   134,
   136,   138,   140,   142,   186,   188,   189,     0,    89,     0,
   172,   174,    -2,   167,   169,     0,     0,     0,     0,   190,
   192,     0,   143,   145,     0,     0,   161,     0,     9,   121,
   123,   125,   127,   129,   131,   133,   135,   137,   139,   141,
     0,    92,    93,    94,    95,    96,    97,    98,    99,   100,
   101,     0,   103,   104,   105,   106,   107,   108,   109,   110,
   111,   112,   113,     0,     0,     0,   198,   200,     0,   194,
   196,   197,   193,    -2,   149,     0,    81,    82,   177,   179,
   170,     0,    58,   201,     0,     0,     0,   205,     0,     0,
     0,   171,     0,     0,   148,   156,   166,   206,   207,   178,
   180,   181,    59,     0,     0,   182,     0,   155,   157,     0,
   183,     0,   158,   184,   202,   185 };
typedef struct { char *t_name; int t_val; } yytoktype;
#ifndef YYDEBUG
#	define YYDEBUG	0	/* don't allow debugging */
#endif

#if YYDEBUG

__YYSCLASS yytoktype yytoks[] =
{
	"LB",	257,
	"RB",	258,
	"LP",	259,
	"RP",	260,
	"MENUS",	261,
	"MENU",	262,
	"BUTTON",	263,
	"DEFAULT_FUNCTION",	264,
	"PLUS",	265,
	"MINUS",	266,
	"ALL",	267,
	"OR",	268,
	"CURSORS",	269,
	"PIXMAPS",	270,
	"ICONS",	271,
	"COLOR",	272,
	"SAVECOLOR",	273,
	"MONOCHROME",	274,
	"FUNCTION",	275,
	"ICONMGR_SHOW",	276,
	"ICONMGR",	277,
	"WINDOW_FUNCTION",	278,
	"ZOOM",	279,
	"ICONMGRS",	280,
	"ICONMGR_GEOMETRY",	281,
	"ICONMGR_NOSHOW",	282,
	"MAKE_TITLE",	283,
	"ICONIFY_BY_UNMAPPING",	284,
	"DONT_ICONIFY_BY_UNMAPPING",	285,
	"NO_TITLE",	286,
	"AUTO_RAISE",	287,
	"NO_HILITE",	288,
	"ICON_REGION",	289,
	"META",	290,
	"SHIFT",	291,
	"LOCK",	292,
	"CONTROL",	293,
	"WINDOW",	294,
	"TITLE",	295,
	"ICON",	296,
	"ROOT",	297,
	"FRAME",	298,
	"COLON",	299,
	"EQUALS",	300,
	"SQUEEZE_TITLE",	301,
	"DONT_SQUEEZE_TITLE",	302,
	"START_ICONIFIED",	303,
	"NO_TITLE_HILITE",	304,
	"TITLE_HILITE",	305,
	"MOVE",	306,
	"RESIZE",	307,
	"WAIT",	308,
	"SELECT",	309,
	"KILL",	310,
	"LEFT_TITLEBUTTON",	311,
	"RIGHT_TITLEBUTTON",	312,
	"NUMBER",	313,
	"KEYWORD",	314,
	"NKEYWORD",	315,
	"CKEYWORD",	316,
	"CLKEYWORD",	317,
	"FKEYWORD",	318,
	"FSKEYWORD",	319,
	"SKEYWORD",	320,
	"DKEYWORD",	321,
	"JKEYWORD",	322,
	"WINDOW_RING",	323,
	"WARP_CURSOR",	324,
	"ERRORTOKEN",	325,
	"NO_STACKMODE",	326,
	"WORKSPACES",	327,
	"WORKSPCMGR_GEOMETRY",	328,
	"OCCUPYALL",	329,
	"STRING",	330,
	"-unknown-",	-1	/* ends search */
};

__YYSCLASS char * yyreds[] =
{
	"-no such reduction-",
	"twmrc : stmts",
	"stmts : /* empty */",
	"stmts : stmts stmt",
	"stmt : error",
	"stmt : noarg",
	"stmt : sarg",
	"stmt : narg",
	"stmt : squeeze",
	"stmt : ICON_REGION string DKEYWORD DKEYWORD number number",
	"stmt : ICONMGR_GEOMETRY string number",
	"stmt : ICONMGR_GEOMETRY string",
	"stmt : WORKSPCMGR_GEOMETRY string number",
	"stmt : WORKSPCMGR_GEOMETRY string",
	"stmt : ZOOM number",
	"stmt : ZOOM",
	"stmt : PIXMAPS pixmap_list",
	"stmt : CURSORS cursor_list",
	"stmt : ICONIFY_BY_UNMAPPING",
	"stmt : ICONIFY_BY_UNMAPPING win_list",
	"stmt : ICONIFY_BY_UNMAPPING",
	"stmt : LEFT_TITLEBUTTON string EQUALS action",
	"stmt : RIGHT_TITLEBUTTON string EQUALS action",
	"stmt : button string",
	"stmt : button action",
	"stmt : string fullkey",
	"stmt : button full",
	"stmt : DONT_ICONIFY_BY_UNMAPPING",
	"stmt : DONT_ICONIFY_BY_UNMAPPING win_list",
	"stmt : WORKSPACES",
	"stmt : WORKSPACES workspc_list",
	"stmt : OCCUPYALL",
	"stmt : OCCUPYALL win_list",
	"stmt : ICONMGR_NOSHOW",
	"stmt : ICONMGR_NOSHOW win_list",
	"stmt : ICONMGR_NOSHOW",
	"stmt : ICONMGRS",
	"stmt : ICONMGRS iconm_list",
	"stmt : ICONMGR_SHOW",
	"stmt : ICONMGR_SHOW win_list",
	"stmt : NO_TITLE_HILITE",
	"stmt : NO_TITLE_HILITE win_list",
	"stmt : NO_TITLE_HILITE",
	"stmt : NO_HILITE",
	"stmt : NO_HILITE win_list",
	"stmt : NO_HILITE",
	"stmt : NO_STACKMODE",
	"stmt : NO_STACKMODE win_list",
	"stmt : NO_STACKMODE",
	"stmt : NO_TITLE",
	"stmt : NO_TITLE win_list",
	"stmt : NO_TITLE",
	"stmt : MAKE_TITLE",
	"stmt : MAKE_TITLE win_list",
	"stmt : START_ICONIFIED",
	"stmt : START_ICONIFIED win_list",
	"stmt : AUTO_RAISE",
	"stmt : AUTO_RAISE win_list",
	"stmt : MENU string LP string COLON string RP",
	"stmt : MENU string LP string COLON string RP menu",
	"stmt : MENU string",
	"stmt : MENU string menu",
	"stmt : FUNCTION string",
	"stmt : FUNCTION string function",
	"stmt : ICONS",
	"stmt : ICONS icon_list",
	"stmt : COLOR",
	"stmt : COLOR color_list",
	"stmt : SAVECOLOR save_color_list",
	"stmt : MONOCHROME",
	"stmt : MONOCHROME color_list",
	"stmt : DEFAULT_FUNCTION action",
	"stmt : WINDOW_FUNCTION action",
	"stmt : WARP_CURSOR",
	"stmt : WARP_CURSOR win_list",
	"stmt : WARP_CURSOR",
	"stmt : WINDOW_RING",
	"stmt : WINDOW_RING win_list",
	"noarg : KEYWORD",
	"sarg : SKEYWORD string",
	"narg : NKEYWORD number",
	"full : EQUALS keys COLON contexts COLON action",
	"fullkey : EQUALS keys COLON contextkeys COLON action",
	"keys : /* empty */",
	"keys : keys key",
	"key : META",
	"key : SHIFT",
	"key : LOCK",
	"key : CONTROL",
	"key : META number",
	"key : OR",
	"contexts : /* empty */",
	"contexts : contexts context",
	"context : WINDOW",
	"context : TITLE",
	"context : ICON",
	"context : ROOT",
	"context : FRAME",
	"context : ICONMGR",
	"context : META",
	"context : ALL",
	"context : OR",
	"contextkeys : /* empty */",
	"contextkeys : contextkeys contextkey",
	"contextkey : WINDOW",
	"contextkey : TITLE",
	"contextkey : ICON",
	"contextkey : ROOT",
	"contextkey : FRAME",
	"contextkey : ICONMGR",
	"contextkey : META",
	"contextkey : ALL",
	"contextkey : OR",
	"contextkey : string",
	"pixmap_list : LB pixmap_entries RB",
	"pixmap_entries : /* empty */",
	"pixmap_entries : pixmap_entries pixmap_entry",
	"pixmap_entry : TITLE_HILITE string",
	"cursor_list : LB cursor_entries RB",
	"cursor_entries : /* empty */",
	"cursor_entries : cursor_entries cursor_entry",
	"cursor_entry : FRAME string string",
	"cursor_entry : FRAME string",
	"cursor_entry : TITLE string string",
	"cursor_entry : TITLE string",
	"cursor_entry : ICON string string",
	"cursor_entry : ICON string",
	"cursor_entry : ICONMGR string string",
	"cursor_entry : ICONMGR string",
	"cursor_entry : BUTTON string string",
	"cursor_entry : BUTTON string",
	"cursor_entry : MOVE string string",
	"cursor_entry : MOVE string",
	"cursor_entry : RESIZE string string",
	"cursor_entry : RESIZE string",
	"cursor_entry : WAIT string string",
	"cursor_entry : WAIT string",
	"cursor_entry : MENU string string",
	"cursor_entry : MENU string",
	"cursor_entry : SELECT string string",
	"cursor_entry : SELECT string",
	"cursor_entry : KILL string string",
	"cursor_entry : KILL string",
	"color_list : LB color_entries RB",
	"color_entries : /* empty */",
	"color_entries : color_entries color_entry",
	"color_entry : CLKEYWORD string",
	"color_entry : CLKEYWORD string",
	"color_entry : CLKEYWORD string win_color_list",
	"color_entry : CKEYWORD string",
	"save_color_list : LB s_color_entries RB",
	"s_color_entries : /* empty */",
	"s_color_entries : s_color_entries s_color_entry",
	"s_color_entry : string",
	"s_color_entry : CLKEYWORD",
	"win_color_list : LB win_color_entries RB",
	"win_color_entries : /* empty */",
	"win_color_entries : win_color_entries win_color_entry",
	"win_color_entry : string string",
	"squeeze : SQUEEZE_TITLE",
	"squeeze : SQUEEZE_TITLE",
	"squeeze : SQUEEZE_TITLE LB win_sqz_entries RB",
	"squeeze : DONT_SQUEEZE_TITLE",
	"squeeze : DONT_SQUEEZE_TITLE",
	"squeeze : DONT_SQUEEZE_TITLE win_list",
	"win_sqz_entries : /* empty */",
	"win_sqz_entries : win_sqz_entries string JKEYWORD signed_number number",
	"iconm_list : LB iconm_entries RB",
	"iconm_entries : /* empty */",
	"iconm_entries : iconm_entries iconm_entry",
	"iconm_entry : string string number",
	"iconm_entry : string string string number",
	"workspc_list : LB workspc_entries RB",
	"workspc_entries : /* empty */",
	"workspc_entries : workspc_entries workspc_entry",
	"workspc_entry : string",
	"workspc_entry : string",
	"workspc_entry : string workapp_list",
	"workapp_list : LB workapp_entries RB",
	"workapp_entries : /* empty */",
	"workapp_entries : workapp_entries workapp_entry",
	"workapp_entry : string",
	"workapp_entry : string string",
	"workapp_entry : string string string",
	"workapp_entry : string string string string",
	"workapp_entry : string string string string string",
	"win_list : LB win_entries RB",
	"win_entries : /* empty */",
	"win_entries : win_entries win_entry",
	"win_entry : string",
	"icon_list : LB icon_entries RB",
	"icon_entries : /* empty */",
	"icon_entries : icon_entries icon_entry",
	"icon_entry : string string",
	"function : LB function_entries RB",
	"function_entries : /* empty */",
	"function_entries : function_entries function_entry",
	"function_entry : action",
	"menu : LB menu_entries RB",
	"menu_entries : /* empty */",
	"menu_entries : menu_entries menu_entry",
	"menu_entry : string action",
	"menu_entry : string LP string COLON string RP action",
	"action : FKEYWORD",
	"action : FSKEYWORD string",
	"signed_number : number",
	"signed_number : PLUS number",
	"signed_number : MINUS number",
	"button : BUTTON number",
	"string : STRING",
	"number : NUMBER",
};
#endif /* YYDEBUG */
#define YYFLAG  (-3000)
/* @(#) $Revision: 66.3 $ */    

/*
** Skeleton parser driver for yacc output
*/

#if defined(NLS) && !defined(NL_SETN)
#include <msgbuf.h>
#endif

#ifndef nl_msg
#define nl_msg(i,s) (s)
#endif

/*
** yacc user known macros and defines
*/
#define YYERROR		goto yyerrlab

#ifndef __RUNTIME_YYMAXDEPTH
#define YYACCEPT	return(0)
#define YYABORT		return(1)
#else
#define YYACCEPT	{free_stacks(); return(0);}
#define YYABORT		{free_stacks(); return(1);}
#endif

#define YYBACKUP( newtoken, newvalue )\
{\
	if ( yychar >= 0 || ( yyr2[ yytmp ] >> 1 ) != 1 )\
	{\
		yyerror( (nl_msg(30001,"syntax error - cannot backup")) );\
		goto yyerrlab;\
	}\
	yychar = newtoken;\
	yystate = *yyps;\
	yylval = newvalue;\
	goto yynewstate;\
}
#define YYRECOVERING()	(!!yyerrflag)
#ifndef YYDEBUG
#	define YYDEBUG	1	/* make debugging available */
#endif

/*
** user known globals
*/
int yydebug;			/* set to 1 to get debugging */

/*
** driver internal defines
*/
/* define for YYFLAG now generated by yacc program. */
/*#define YYFLAG		(FLAGVAL)*/

/*
** global variables used by the parser
*/
# ifndef __RUNTIME_YYMAXDEPTH
__YYSCLASS YYSTYPE yyv[ YYMAXDEPTH ];	/* value stack */
__YYSCLASS int yys[ YYMAXDEPTH ];		/* state stack */
# else
__YYSCLASS YYSTYPE *yyv;			/* pointer to malloc'ed value stack */
__YYSCLASS int *yys;			/* pointer to malloc'ed stack stack */
#ifdef __cplusplus
	extern char *malloc(int);
	extern char *realloc(char *, int);
	extern void free();
# else
	extern char *malloc();
	extern char *realloc();
	extern void free();
# endif /* __cplusplus */
static int allocate_stacks(); 
static void free_stacks();
# ifndef YYINCREMENT
# define YYINCREMENT (YYMAXDEPTH/2) + 10
# endif
# endif	/* __RUNTIME_YYMAXDEPTH */
long  yymaxdepth = YYMAXDEPTH;

__YYSCLASS YYSTYPE *yypv;			/* top of value stack */
__YYSCLASS int *yyps;			/* top of state stack */

__YYSCLASS int yystate;			/* current state */
__YYSCLASS int yytmp;			/* extra var (lasts between blocks) */

int yynerrs;			/* number of errors */
__YYSCLASS int yyerrflag;			/* error recovery flag */
int yychar;			/* current input token number */



/*
** yyparse - return 0 if worked, 1 if syntax error not recovered from
*/
int
yyparse()
{
	register YYSTYPE *yypvt;	/* top of value stack for $vars */

	/*
	** Initialize externals - yyparse may be called more than once
	*/
# ifdef __RUNTIME_YYMAXDEPTH
	if (allocate_stacks()) YYABORT;
# endif
	yypv = &yyv[-1];
	yyps = &yys[-1];
	yystate = 0;
	yytmp = 0;
	yynerrs = 0;
	yyerrflag = 0;
	yychar = -1;

	goto yystack;
	{
		register YYSTYPE *yy_pv;	/* top of value stack */
		register int *yy_ps;		/* top of state stack */
		register int yy_state;		/* current state */
		register int  yy_n;		/* internal state number info */

		/*
		** get globals into registers.
		** branch to here only if YYBACKUP was called.
		*/
	yynewstate:
		yy_pv = yypv;
		yy_ps = yyps;
		yy_state = yystate;
		goto yy_newstate;

		/*
		** get globals into registers.
		** either we just started, or we just finished a reduction
		*/
	yystack:
		yy_pv = yypv;
		yy_ps = yyps;
		yy_state = yystate;

		/*
		** top of for (;;) loop while no reductions done
		*/
	yy_stack:
		/*
		** put a state and value onto the stacks
		*/
#if YYDEBUG
		/*
		** if debugging, look up token value in list of value vs.
		** name pairs.  0 and negative (-1) are special values.
		** Note: linear search is used since time is not a real
		** consideration while debugging.
		*/
		if ( yydebug )
		{
			register int yy_i;

			printf( "State %d, token ", yy_state );
			if ( yychar == 0 )
				printf( "end-of-file\n" );
			else if ( yychar < 0 )
				printf( "-none-\n" );
			else
			{
				for ( yy_i = 0; yytoks[yy_i].t_val >= 0;
					yy_i++ )
				{
					if ( yytoks[yy_i].t_val == yychar )
						break;
				}
				printf( "%s\n", yytoks[yy_i].t_name );
			}
		}
#endif /* YYDEBUG */
		if ( ++yy_ps >= &yys[ yymaxdepth ] )	/* room on stack? */
		{
# ifndef __RUNTIME_YYMAXDEPTH
			yyerror( (nl_msg(30002,"yacc stack overflow")) );
			YYABORT;
# else
			/* save old stack bases to recalculate pointers */
			YYSTYPE * yyv_old = yyv;
			int * yys_old = yys;
			yymaxdepth += YYINCREMENT;
			yys = (int *) realloc(yys, yymaxdepth * sizeof(int));
			yyv = (YYSTYPE *) realloc(yyv, yymaxdepth * sizeof(YYSTYPE));
			if (yys==0 || yyv==0) {
			    yyerror( (nl_msg(30002,"yacc stack overflow")) );
			    YYABORT;
			    }
			/* Reset pointers into stack */
			yy_ps = (yy_ps - yys_old) + yys;
			yyps = (yyps - yys_old) + yys;
			yy_pv = (yy_pv - yyv_old) + yyv;
			yypv = (yypv - yyv_old) + yyv;
# endif

		}
		*yy_ps = yy_state;
		*++yy_pv = yyval;

		/*
		** we have a new state - find out what to do
		*/
	yy_newstate:
		if ( ( yy_n = yypact[ yy_state ] ) <= YYFLAG )
			goto yydefault;		/* simple state */
#if YYDEBUG
		/*
		** if debugging, need to mark whether new token grabbed
		*/
		yytmp = yychar < 0;
#endif
		if ( ( yychar < 0 ) && ( ( yychar = yylex() ) < 0 ) )
			yychar = 0;		/* reached EOF */
#if YYDEBUG
		if ( yydebug && yytmp )
		{
			register int yy_i;

			printf( "Received token " );
			if ( yychar == 0 )
				printf( "end-of-file\n" );
			else if ( yychar < 0 )
				printf( "-none-\n" );
			else
			{
				for ( yy_i = 0; yytoks[yy_i].t_val >= 0;
					yy_i++ )
				{
					if ( yytoks[yy_i].t_val == yychar )
						break;
				}
				printf( "%s\n", yytoks[yy_i].t_name );
			}
		}
#endif /* YYDEBUG */
		if ( ( ( yy_n += yychar ) < 0 ) || ( yy_n >= YYLAST ) )
			goto yydefault;
		if ( yychk[ yy_n = yyact[ yy_n ] ] == yychar )	/*valid shift*/
		{
			yychar = -1;
			yyval = yylval;
			yy_state = yy_n;
			if ( yyerrflag > 0 )
				yyerrflag--;
			goto yy_stack;
		}

	yydefault:
		if ( ( yy_n = yydef[ yy_state ] ) == -2 )
		{
#if YYDEBUG
			yytmp = yychar < 0;
#endif
			if ( ( yychar < 0 ) && ( ( yychar = yylex() ) < 0 ) )
				yychar = 0;		/* reached EOF */
#if YYDEBUG
			if ( yydebug && yytmp )
			{
				register int yy_i;

				printf( "Received token " );
				if ( yychar == 0 )
					printf( "end-of-file\n" );
				else if ( yychar < 0 )
					printf( "-none-\n" );
				else
				{
					for ( yy_i = 0;
						yytoks[yy_i].t_val >= 0;
						yy_i++ )
					{
						if ( yytoks[yy_i].t_val
							== yychar )
						{
							break;
						}
					}
					printf( "%s\n", yytoks[yy_i].t_name );
				}
			}
#endif /* YYDEBUG */
			/*
			** look through exception table
			*/
			{
				register int *yyxi = yyexca;

				while ( ( *yyxi != -1 ) ||
					( yyxi[1] != yy_state ) )
				{
					yyxi += 2;
				}
				while ( ( *(yyxi += 2) >= 0 ) &&
					( *yyxi != yychar ) )
					;
				if ( ( yy_n = yyxi[1] ) < 0 )
					YYACCEPT;
			}
		}

		/*
		** check for syntax error
		*/
		if ( yy_n == 0 )	/* have an error */
		{
			/* no worry about speed here! */
			switch ( yyerrflag )
			{
			case 0:		/* new error */
				yyerror( (nl_msg(30003,"syntax error")) );
				yynerrs++;
				goto skip_init;
			yyerrlab:
				/*
				** get globals into registers.
				** we have a user generated syntax type error
				*/
				yy_pv = yypv;
				yy_ps = yyps;
				yy_state = yystate;
				yynerrs++;
			skip_init:
			case 1:
			case 2:		/* incompletely recovered error */
					/* try again... */
				yyerrflag = 3;
				/*
				** find state where "error" is a legal
				** shift action
				*/
				while ( yy_ps >= yys )
				{
					yy_n = yypact[ *yy_ps ] + YYERRCODE;
					if ( yy_n >= 0 && yy_n < YYLAST &&
						yychk[yyact[yy_n]] == YYERRCODE)					{
						/*
						** simulate shift of "error"
						*/
						yy_state = yyact[ yy_n ];
						goto yy_stack;
					}
					/*
					** current state has no shift on
					** "error", pop stack
					*/
#if YYDEBUG
#	define _POP_ "Error recovery pops state %d, uncovers state %d\n"
					if ( yydebug )
						printf( _POP_, *yy_ps,
							yy_ps[-1] );
#	undef _POP_
#endif
					yy_ps--;
					yy_pv--;
				}
				/*
				** there is no state on stack with "error" as
				** a valid shift.  give up.
				*/
				YYABORT;
			case 3:		/* no shift yet; eat a token */
#if YYDEBUG
				/*
				** if debugging, look up token in list of
				** pairs.  0 and negative shouldn't occur,
				** but since timing doesn't matter when
				** debugging, it doesn't hurt to leave the
				** tests here.
				*/
				if ( yydebug )
				{
					register int yy_i;

					printf( "Error recovery discards " );
					if ( yychar == 0 )
						printf( "token end-of-file\n" );
					else if ( yychar < 0 )
						printf( "token -none-\n" );
					else
					{
						for ( yy_i = 0;
							yytoks[yy_i].t_val >= 0;
							yy_i++ )
						{
							if ( yytoks[yy_i].t_val
								== yychar )
							{
								break;
							}
						}
						printf( "token %s\n",
							yytoks[yy_i].t_name );
					}
				}
#endif /* YYDEBUG */
				if ( yychar == 0 )	/* reached EOF. quit */
					YYABORT;
				yychar = -1;
				goto yy_newstate;
			}
		}/* end if ( yy_n == 0 ) */
		/*
		** reduction by production yy_n
		** put stack tops, etc. so things right after switch
		*/
#if YYDEBUG
		/*
		** if debugging, print the string that is the user's
		** specification of the reduction which is just about
		** to be done.
		*/
		if ( yydebug )
			printf( "Reduce by (%d) \"%s\"\n",
				yy_n, yyreds[ yy_n ] );
#endif
		yytmp = yy_n;			/* value to switch over */
		yypvt = yy_pv;			/* $vars top of value stack */
		/*
		** Look in goto table for next state
		** Sorry about using yy_state here as temporary
		** register variable, but why not, if it works...
		** If yyr2[ yy_n ] doesn't have the low order bit
		** set, then there is no action to be done for
		** this reduction.  So, no saving & unsaving of
		** registers done.  The only difference between the
		** code just after the if and the body of the if is
		** the goto yy_stack in the body.  This way the test
		** can be made before the choice of what to do is needed.
		*/
		{
			/* length of production doubled with extra bit */
			register int yy_len = yyr2[ yy_n ];

			if ( !( yy_len & 01 ) )
			{
				yy_len >>= 1;
				yyval = ( yy_pv -= yy_len )[1];	/* $$ = $1 */
				yy_state = yypgo[ yy_n = yyr1[ yy_n ] ] +
					*( yy_ps -= yy_len ) + 1;
				if ( yy_state >= YYLAST ||
					yychk[ yy_state =
					yyact[ yy_state ] ] != -yy_n )
				{
					yy_state = yyact[ yypgo[ yy_n ] ];
				}
				goto yy_stack;
			}
			yy_len >>= 1;
			yyval = ( yy_pv -= yy_len )[1];	/* $$ = $1 */
			yy_state = yypgo[ yy_n = yyr1[ yy_n ] ] +
				*( yy_ps -= yy_len ) + 1;
			if ( yy_state >= YYLAST ||
				yychk[ yy_state = yyact[ yy_state ] ] != -yy_n )
			{
				yy_state = yyact[ yypgo[ yy_n ] ];
			}
		}
					/* save until reenter driver code */
		yystate = yy_state;
		yyps = yy_ps;
		yypv = yy_pv;
	}
	/*
	** code supplied by user is placed in this switch
	*/
	switch( yytmp )
	{
		
case 9:
# line 116 "gram.y"
{ AddIconRegion(yypvt[-4].ptr, yypvt[-3].num, yypvt[-2].num, yypvt[-1].num, yypvt[-0].num); } break;
case 10:
# line 117 "gram.y"
{ if (Scr->FirstTime)
						  {
						    Scr->iconmgr->geometry= yypvt[-1].ptr;
						    Scr->iconmgr->columns=yypvt[-0].num;
						  }
						} break;
case 11:
# line 123 "gram.y"
{ if (Scr->FirstTime)
						    Scr->iconmgr->geometry = yypvt[-0].ptr;
						} break;
case 12:
# line 126 "gram.y"
{ if (Scr->FirstTime)
						  {
						    Scr->workSpaceMgr.geometry= yypvt[-1].ptr;
						    Scr->workSpaceMgr.columns=yypvt[-0].num;
						  }
						} break;
case 13:
# line 132 "gram.y"
{ if (Scr->FirstTime)
						    Scr->workSpaceMgr.geometry = yypvt[-0].ptr;
						} break;
case 14:
# line 135 "gram.y"
{ if (Scr->FirstTime)
					  {
						Scr->DoZoom = TRUE;
						Scr->ZoomCount = yypvt[-0].num;
					  }
					} break;
case 15:
# line 141 "gram.y"
{ if (Scr->FirstTime) 
						Scr->DoZoom = TRUE; } break;
case 16:
# line 143 "gram.y"
{} break;
case 17:
# line 144 "gram.y"
{} break;
case 18:
# line 145 "gram.y"
{ list = &Scr->IconifyByUn; } break;
case 20:
# line 147 "gram.y"
{ if (Scr->FirstTime) 
		    Scr->IconifyByUnmapping = TRUE; } break;
case 21:
# line 149 "gram.y"
{ 
					  GotTitleButton (yypvt[-2].ptr, yypvt[-0].num, False);
					} break;
case 22:
# line 152 "gram.y"
{ 
					  GotTitleButton (yypvt[-2].ptr, yypvt[-0].num, True);
					} break;
case 23:
# line 155 "gram.y"
{ root = GetRoot(yypvt[-0].ptr, NULLSTR, NULLSTR);
					  Scr->Mouse[yypvt[-1].num][C_ROOT][0].func = F_MENU;
					  Scr->Mouse[yypvt[-1].num][C_ROOT][0].menu = root;
					} break;
case 24:
# line 159 "gram.y"
{ Scr->Mouse[yypvt[-1].num][C_ROOT][0].func = yypvt[-0].num;
					  if (yypvt[-0].num == F_MENU)
					  {
					    pull->prev = NULL;
					    Scr->Mouse[yypvt[-1].num][C_ROOT][0].menu = pull;
					  }
					  else
					  {
					    root = GetRoot(TWM_ROOT,NULLSTR,NULLSTR);
					    Scr->Mouse[yypvt[-1].num][C_ROOT][0].item = 
						AddToMenu(root,"x",Action,
							  NULLSTR,yypvt[-0].num,NULLSTR,NULLSTR);
					  }
					  Action = "";
					  pull = NULL;
					} break;
case 25:
# line 175 "gram.y"
{ GotKey(yypvt[-1].ptr, yypvt[-0].num); } break;
case 26:
# line 176 "gram.y"
{ GotButton(yypvt[-1].num, yypvt[-0].num); } break;
case 27:
# line 177 "gram.y"
{ list = &Scr->DontIconify; } break;
case 29:
# line 179 "gram.y"
{} break;
case 31:
# line 181 "gram.y"
{ list = &Scr->OccupyAll; } break;
case 33:
# line 183 "gram.y"
{ list = &Scr->IconMgrNoShow; } break;
case 35:
# line 185 "gram.y"
{ Scr->IconManagerDontShow = TRUE; } break;
case 36:
# line 186 "gram.y"
{ list = &Scr->IconMgrs; } break;
case 38:
# line 188 "gram.y"
{ list = &Scr->IconMgrShow; } break;
case 40:
# line 190 "gram.y"
{ list = &Scr->NoTitleHighlight; } break;
case 42:
# line 192 "gram.y"
{ if (Scr->FirstTime)
						Scr->TitleHighlight = FALSE; } break;
case 43:
# line 194 "gram.y"
{ list = &Scr->NoHighlight; } break;
case 45:
# line 196 "gram.y"
{ if (Scr->FirstTime)
						Scr->Highlight = FALSE; } break;
case 46:
# line 198 "gram.y"
{ list = &Scr->NoStackModeL; } break;
case 48:
# line 200 "gram.y"
{ if (Scr->FirstTime)
						Scr->StackMode = FALSE; } break;
case 49:
# line 202 "gram.y"
{ list = &Scr->NoTitle; } break;
case 51:
# line 204 "gram.y"
{ if (Scr->FirstTime)
						Scr->NoTitlebar = TRUE; } break;
case 52:
# line 206 "gram.y"
{ list = &Scr->MakeTitle; } break;
case 54:
# line 208 "gram.y"
{ list = &Scr->StartIconified; } break;
case 56:
# line 210 "gram.y"
{ list = &Scr->AutoRaise; } break;
case 58:
# line 212 "gram.y"
{
					root = GetRoot(yypvt[-5].ptr, yypvt[-3].ptr, yypvt[-1].ptr); } break;
case 59:
# line 214 "gram.y"
{ root->real_menu = TRUE;} break;
case 60:
# line 215 "gram.y"
{ root = GetRoot(yypvt[-0].ptr, NULLSTR, NULLSTR); } break;
case 61:
# line 216 "gram.y"
{ root->real_menu = TRUE; } break;
case 62:
# line 217 "gram.y"
{ root = GetRoot(yypvt[-0].ptr, NULLSTR, NULLSTR); } break;
case 64:
# line 219 "gram.y"
{ list = &Scr->IconNames; } break;
case 66:
# line 221 "gram.y"
{ color = COLOR; } break;
case 69:
# line 225 "gram.y"
{ color = MONOCHROME; } break;
case 71:
# line 227 "gram.y"
{ Scr->DefaultFunction.func = yypvt[-0].num;
					  if (yypvt[-0].num == F_MENU)
					  {
					    pull->prev = NULL;
					    Scr->DefaultFunction.menu = pull;
					  }
					  else
					  {
					    root = GetRoot(TWM_ROOT,NULLSTR,NULLSTR);
					    Scr->DefaultFunction.item = 
						AddToMenu(root,"x",Action,
							  NULLSTR,yypvt[-0].num, NULLSTR, NULLSTR);
					  }
					  Action = "";
					  pull = NULL;
					} break;
case 72:
# line 243 "gram.y"
{ Scr->WindowFunction.func = yypvt[-0].num;
					   root = GetRoot(TWM_ROOT,NULLSTR,NULLSTR);
					   Scr->WindowFunction.item = 
						AddToMenu(root,"x",Action,
							  NULLSTR,yypvt[-0].num, NULLSTR, NULLSTR);
					   Action = "";
					   pull = NULL;
					} break;
case 73:
# line 251 "gram.y"
{ list = &Scr->WarpCursorL; } break;
case 75:
# line 253 "gram.y"
{ if (Scr->FirstTime) 
					    Scr->WarpCursor = TRUE; } break;
case 76:
# line 255 "gram.y"
{ list = &Scr->WindowRingL; } break;
case 78:
# line 260 "gram.y"
{ if (!do_single_keyword (yypvt[-0].num)) {
					    twmrc_error_prefix();
					    fprintf (stderr,
					"unknown singleton keyword %d\n",
						     yypvt[-0].num);
					    ParseError = 1;
					  }
					} break;
case 79:
# line 270 "gram.y"
{ if (!do_string_keyword (yypvt[-1].num, yypvt[-0].ptr)) {
					    twmrc_error_prefix();
					    fprintf (stderr,
				"unknown string keyword %d (value \"%s\")\n",
						     yypvt[-1].num, yypvt[-0].ptr);
					    ParseError = 1;
					  }
					} break;
case 80:
# line 280 "gram.y"
{ if (!do_number_keyword (yypvt[-1].num, yypvt[-0].num)) {
					    twmrc_error_prefix();
					    fprintf (stderr,
				"unknown numeric keyword %d (value %d)\n",
						     yypvt[-1].num, yypvt[-0].num);
					    ParseError = 1;
					  }
					} break;
case 81:
# line 292 "gram.y"
{ yyval.num = yypvt[-0].num; } break;
case 82:
# line 295 "gram.y"
{ yyval.num = yypvt[-0].num; } break;
case 85:
# line 302 "gram.y"
{ mods |= Mod1Mask; } break;
case 86:
# line 303 "gram.y"
{ mods |= ShiftMask; } break;
case 87:
# line 304 "gram.y"
{ mods |= LockMask; } break;
case 88:
# line 305 "gram.y"
{ mods |= ControlMask; } break;
case 89:
# line 306 "gram.y"
{ if (yypvt[-0].num < 1 || yypvt[-0].num > 5) {
					     twmrc_error_prefix();
					     fprintf (stderr, 
				"bad modifier number (%d), must be 1-5\n",
						      yypvt[-0].num);
					     ParseError = 1;
					  } else {
					     mods |= (Mod1Mask << (yypvt[-0].num - 1));
					  }
					} break;
case 90:
# line 316 "gram.y"
{ } break;
case 93:
# line 323 "gram.y"
{ cont |= C_WINDOW_BIT; } break;
case 94:
# line 324 "gram.y"
{ cont |= C_TITLE_BIT; } break;
case 95:
# line 325 "gram.y"
{ cont |= C_ICON_BIT; } break;
case 96:
# line 326 "gram.y"
{ cont |= C_ROOT_BIT; } break;
case 97:
# line 327 "gram.y"
{ cont |= C_FRAME_BIT; } break;
case 98:
# line 328 "gram.y"
{ cont |= C_ICONMGR_BIT; } break;
case 99:
# line 329 "gram.y"
{ cont |= C_ICONMGR_BIT; } break;
case 100:
# line 330 "gram.y"
{ cont |= C_ALL_BITS; } break;
case 101:
# line 331 "gram.y"
{  } break;
case 104:
# line 338 "gram.y"
{ cont |= C_WINDOW_BIT; } break;
case 105:
# line 339 "gram.y"
{ cont |= C_TITLE_BIT; } break;
case 106:
# line 340 "gram.y"
{ cont |= C_ICON_BIT; } break;
case 107:
# line 341 "gram.y"
{ cont |= C_ROOT_BIT; } break;
case 108:
# line 342 "gram.y"
{ cont |= C_FRAME_BIT; } break;
case 109:
# line 343 "gram.y"
{ cont |= C_ICONMGR_BIT; } break;
case 110:
# line 344 "gram.y"
{ cont |= C_ICONMGR_BIT; } break;
case 111:
# line 345 "gram.y"
{ cont |= C_ALL_BITS; } break;
case 112:
# line 346 "gram.y"
{ } break;
case 113:
# line 347 "gram.y"
{ Name = yypvt[-0].ptr; cont |= C_NAME_BIT; } break;
case 117:
# line 358 "gram.y"
{ SetHighlightPixmap (yypvt[-0].ptr); } break;
case 121:
# line 369 "gram.y"
{
			NewBitmapCursor(&Scr->FrameCursor, yypvt[-1].ptr, yypvt[-0].ptr); } break;
case 122:
# line 371 "gram.y"
{
			NewFontCursor(&Scr->FrameCursor, yypvt[-0].ptr); } break;
case 123:
# line 373 "gram.y"
{
			NewBitmapCursor(&Scr->TitleCursor, yypvt[-1].ptr, yypvt[-0].ptr); } break;
case 124:
# line 375 "gram.y"
{
			NewFontCursor(&Scr->TitleCursor, yypvt[-0].ptr); } break;
case 125:
# line 377 "gram.y"
{
			NewBitmapCursor(&Scr->IconCursor, yypvt[-1].ptr, yypvt[-0].ptr); } break;
case 126:
# line 379 "gram.y"
{
			NewFontCursor(&Scr->IconCursor, yypvt[-0].ptr); } break;
case 127:
# line 381 "gram.y"
{
			NewBitmapCursor(&Scr->IconMgrCursor, yypvt[-1].ptr, yypvt[-0].ptr); } break;
case 128:
# line 383 "gram.y"
{
			NewFontCursor(&Scr->IconMgrCursor, yypvt[-0].ptr); } break;
case 129:
# line 385 "gram.y"
{
			NewBitmapCursor(&Scr->ButtonCursor, yypvt[-1].ptr, yypvt[-0].ptr); } break;
case 130:
# line 387 "gram.y"
{
			NewFontCursor(&Scr->ButtonCursor, yypvt[-0].ptr); } break;
case 131:
# line 389 "gram.y"
{
			NewBitmapCursor(&Scr->MoveCursor, yypvt[-1].ptr, yypvt[-0].ptr); } break;
case 132:
# line 391 "gram.y"
{
			NewFontCursor(&Scr->MoveCursor, yypvt[-0].ptr); } break;
case 133:
# line 393 "gram.y"
{
			NewBitmapCursor(&Scr->ResizeCursor, yypvt[-1].ptr, yypvt[-0].ptr); } break;
case 134:
# line 395 "gram.y"
{
			NewFontCursor(&Scr->ResizeCursor, yypvt[-0].ptr); } break;
case 135:
# line 397 "gram.y"
{
			NewBitmapCursor(&Scr->WaitCursor, yypvt[-1].ptr, yypvt[-0].ptr); } break;
case 136:
# line 399 "gram.y"
{
			NewFontCursor(&Scr->WaitCursor, yypvt[-0].ptr); } break;
case 137:
# line 401 "gram.y"
{
			NewBitmapCursor(&Scr->MenuCursor, yypvt[-1].ptr, yypvt[-0].ptr); } break;
case 138:
# line 403 "gram.y"
{
			NewFontCursor(&Scr->MenuCursor, yypvt[-0].ptr); } break;
case 139:
# line 405 "gram.y"
{
			NewBitmapCursor(&Scr->SelectCursor, yypvt[-1].ptr, yypvt[-0].ptr); } break;
case 140:
# line 407 "gram.y"
{
			NewFontCursor(&Scr->SelectCursor, yypvt[-0].ptr); } break;
case 141:
# line 409 "gram.y"
{
			NewBitmapCursor(&Scr->DestroyCursor, yypvt[-1].ptr, yypvt[-0].ptr); } break;
case 142:
# line 411 "gram.y"
{
			NewFontCursor(&Scr->DestroyCursor, yypvt[-0].ptr); } break;
case 146:
# line 423 "gram.y"
{ if (!do_colorlist_keyword (yypvt[-1].num, color,
								     yypvt[-0].ptr)) {
					    twmrc_error_prefix();
					    fprintf (stderr,
			"unhandled list color keyword %d (string \"%s\")\n",
						     yypvt[-1].num, yypvt[-0].ptr);
					    ParseError = 1;
					  }
					} break;
case 147:
# line 432 "gram.y"
{ list = do_colorlist_keyword(yypvt[-1].num,color,
								      yypvt[-0].ptr);
					  if (!list) {
					    twmrc_error_prefix();
					    fprintf (stderr,
			"unhandled color list keyword %d (string \"%s\")\n",
						     yypvt[-1].num, yypvt[-0].ptr);
					    ParseError = 1;
					  }
					} break;
case 149:
# line 443 "gram.y"
{ if (!do_color_keyword (yypvt[-1].num, color,
								 yypvt[-0].ptr)) {
					    twmrc_error_prefix();
					    fprintf (stderr,
			"unhandled color keyword %d (string \"%s\")\n",
						     yypvt[-1].num, yypvt[-0].ptr);
					    ParseError = 1;
					  }
					} break;
case 153:
# line 461 "gram.y"
{ do_string_savecolor(color, yypvt[-0].ptr); } break;
case 154:
# line 462 "gram.y"
{ do_var_savecolor(yypvt[-0].num); } break;
case 158:
# line 472 "gram.y"
{ if (Scr->FirstTime &&
					      color == Scr->Monochrome)
					    AddToList(list, yypvt[-1].ptr, yypvt[-0].ptr); } break;
case 159:
# line 477 "gram.y"
{ 
				    if (HasShape) Scr->SqueezeTitle = TRUE;
				} break;
case 160:
# line 480 "gram.y"
{ list = &Scr->SqueezeTitleL; 
				  if (HasShape && Scr->SqueezeTitle == -1)
				    Scr->SqueezeTitle = TRUE;
				} break;
case 162:
# line 485 "gram.y"
{ Scr->SqueezeTitle = FALSE; } break;
case 163:
# line 486 "gram.y"
{ list = &Scr->DontSqueezeTitleL; } break;
case 166:
# line 491 "gram.y"
{
				if (Scr->FirstTime) {
				   do_squeeze_entry (list, yypvt[-3].ptr, yypvt[-2].num, yypvt[-1].num, yypvt[-0].num);
				}
			} break;
case 170:
# line 506 "gram.y"
{ if (Scr->FirstTime)
					    AddToList(list, yypvt[-2].ptr, (char *)
						AllocateIconManager(yypvt[-2].ptr, NULLSTR,
							yypvt[-1].ptr,yypvt[-0].num));
					} break;
case 171:
# line 512 "gram.y"
{ if (Scr->FirstTime)
					    AddToList(list, yypvt[-3].ptr, (char *)
						AllocateIconManager(yypvt[-3].ptr,yypvt[-2].ptr,
						yypvt[-1].ptr, yypvt[-0].num));
					} break;
case 175:
# line 526 "gram.y"
{
			AddWorkSpace (yypvt[-0].ptr, NULLSTR, NULLSTR, NULLSTR, NULLSTR, NULLSTR);
		} break;
case 176:
# line 529 "gram.y"
{
			curWorkSpc = yypvt[-0].ptr;
		} break;
case 181:
# line 542 "gram.y"
{
			AddWorkSpace (curWorkSpc, yypvt[-0].ptr, NULLSTR, NULLSTR, NULLSTR, NULLSTR);
		} break;
case 182:
# line 545 "gram.y"
{
			AddWorkSpace (curWorkSpc, yypvt[-1].ptr, yypvt[-0].ptr, NULLSTR, NULLSTR, NULLSTR);
		} break;
case 183:
# line 548 "gram.y"
{
			AddWorkSpace (curWorkSpc, yypvt[-2].ptr, yypvt[-1].ptr, yypvt[-0].ptr, NULLSTR, NULLSTR);
		} break;
case 184:
# line 551 "gram.y"
{
			AddWorkSpace (curWorkSpc, yypvt[-3].ptr, yypvt[-2].ptr, yypvt[-1].ptr, yypvt[-0].ptr, NULLSTR);
		} break;
case 185:
# line 554 "gram.y"
{
			AddWorkSpace (curWorkSpc, yypvt[-4].ptr, yypvt[-3].ptr, yypvt[-2].ptr, yypvt[-1].ptr, yypvt[-0].ptr);
		} break;
case 189:
# line 566 "gram.y"
{ if (Scr->FirstTime)
					    AddToList(list, yypvt[-0].ptr, 0);
					} break;
case 193:
# line 578 "gram.y"
{ if (Scr->FirstTime) AddToList(list, yypvt[-1].ptr, yypvt[-0].ptr); } break;
case 197:
# line 588 "gram.y"
{ AddToMenu(root, "", Action, NULLSTR, yypvt[-0].num,
						NULLSTR, NULLSTR);
					  Action = "";
					} break;
case 201:
# line 601 "gram.y"
{ AddToMenu(root, yypvt[-1].ptr, Action, pull, yypvt[-0].num,
						NULLSTR, NULLSTR);
					  Action = "";
					  pull = NULL;
					} break;
case 202:
# line 606 "gram.y"
{
					  AddToMenu(root, yypvt[-6].ptr, Action, pull, yypvt[-0].num,
						yypvt[-4].ptr, yypvt[-2].ptr);
					  Action = "";
					  pull = NULL;
					} break;
case 203:
# line 614 "gram.y"
{ yyval.num = yypvt[-0].num; } break;
case 204:
# line 615 "gram.y"
{
				yyval.num = yypvt[-1].num;
				Action = yypvt[-0].ptr;
				switch (yypvt[-1].num) {
				  case F_MENU:
				    pull = GetRoot (yypvt[-0].ptr, NULLSTR,NULLSTR);
				    pull->prev = root;
				    break;
				  case F_WARPRING:
				    if (!CheckWarpRingArg (Action)) {
					twmrc_error_prefix();
					fprintf (stderr,
			"ignoring invalid f.warptoring argument \"%s\"\n",
						 Action);
					yyval.num = F_NOP;
				    }
				  case F_WARPTOSCREEN:
				    if (!CheckWarpScreenArg (Action)) {
					twmrc_error_prefix();
					fprintf (stderr, 
			"ignoring invalid f.warptoscreen argument \"%s\"\n", 
					         Action);
					yyval.num = F_NOP;
				    }
				    break;
				  case F_COLORMAP:
				    if (CheckColormapArg (Action)) {
					yyval.num = F_COLORMAP;
				    } else {
					twmrc_error_prefix();
					fprintf (stderr,
			"ignoring invalid f.colormap argument \"%s\"\n", 
						 Action);
					yyval.num = F_NOP;
				    }
				    break;
				} /* end switch */
				   } break;
case 205:
# line 656 "gram.y"
{ yyval.num = yypvt[-0].num; } break;
case 206:
# line 657 "gram.y"
{ yyval.num = yypvt[-0].num; } break;
case 207:
# line 658 "gram.y"
{ yyval.num = -(yypvt[-0].num); } break;
case 208:
# line 661 "gram.y"
{ yyval.num = yypvt[-0].num;
					  if (yypvt[-0].num == 0)
						yyerror("bad button 0");

					  if (yypvt[-0].num > MAX_BUTTONS)
					  {
						yyval.num = 0;
						yyerror("button number too large");
					  }
					} break;
case 209:
# line 673 "gram.y"
{ ptr = (char *)malloc(strlen(yypvt[-0].ptr)+1);
					  strcpy(ptr, yypvt[-0].ptr);
					  RemoveDQuote(ptr);
					  yyval.ptr = ptr;
					} break;
case 210:
# line 678 "gram.y"
{ yyval.num = yypvt[-0].num; } break;
	}
	goto yystack;		/* reset registers in driver code */
}

# ifdef __RUNTIME_YYMAXDEPTH

static int allocate_stacks() {
	/* allocate the yys and yyv stacks */
	yys = (int *) malloc(yymaxdepth * sizeof(int));
	yyv = (YYSTYPE *) malloc(yymaxdepth * sizeof(YYSTYPE));

	if (yys==0 || yyv==0) {
	   yyerror( (nl_msg(30004,"unable to allocate space for yacc stacks")) );
	   return(1);
	   }
	else return(0);

}


static void free_stacks() {
	if (yys!=0) free((char *) yys);
	if (yyv!=0) free((char *) yyv);
}

# endif  /* defined(__RUNTIME_YYMAXDEPTH) */

