/*
 * Parser routines called from yacc code (gram.y)
 *
 * This is very similar to the meaning of parse_be.c; the two may be
 * merged at some point.
 */

#include "ctwm.h"

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <strings.h>

#include "otp.h"
#include "menus.h"
#include "icons.h"
#include "windowbox.h"
#include "add_window.h"
#include "list.h"
#include "util.h"
#include "screen.h"
#include "parse.h"
#include "parse_be.h"
#include "parse_yacc.h"
#include "cursor.h"

char *Action = "";
char *Name = "";
MenuRoot *root, *pull = NULL;

name_list **curplist;
int cont = 0;
int color;
int mods = 0;
unsigned int mods_used = (ShiftMask | ControlMask | Mod1Mask);


void yyerror(char *s)
{
	twmrc_error_prefix();
	fprintf(stderr, "error in input file:  %s\n", s ? s : "");
	ParseError = 1;
}

void InitGramVariables(void)
{
	mods = 0;
}

void RemoveDQuote(char *str)
{
	char *i, *o;
	int n;
	int count;

	for(i = str + 1, o = str; *i && *i != '\"'; o++) {
		if(*i == '\\') {
			switch(*++i) {
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
					if(*++i == 'x') {
						goto hex;
					}
					else {
						--i;
					}
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
					n = 0;
					count = 0;
					while(*i >= '0' && *i <= '7' && count < 3) {
						n = (n << 3) + (*i++ - '0');
						count++;
					}
					*o = n;
					break;
hex:
				case 'x':
					n = 0;
					count = 0;
					while(i++, count++ < 2) {
						if(*i >= '0' && *i <= '9') {
							n = (n << 4) + (*i - '0');
						}
						else if(*i >= 'a' && *i <= 'f') {
							n = (n << 4) + (*i - 'a') + 10;
						}
						else if(*i >= 'A' && *i <= 'F') {
							n = (n << 4) + (*i - 'A') + 10;
						}
						else {
							break;
						}
					}
					*o = n;
					break;
				case '\n':
					i++;    /* punt */
					o--;    /* to account for o++ at end of loop */
					break;
				case '\"':
				case '\'':
				case '\\':
				default:
					*o = *i++;
					break;
			}
		}
		else {
			*o = *i++;
		}
	}
	*o = '\0';
}

MenuRoot *GetRoot(char *name, char *fore, char *back)
{
	MenuRoot *tmp;

	tmp = FindMenuRoot(name);
	if(tmp == NULL) {
		tmp = NewMenuRoot(name);
	}

	if(fore) {
		int save;

		save = Scr->FirstTime;
		Scr->FirstTime = TRUE;
		GetColor(COLOR, &tmp->highlight.fore, fore);
		GetColor(COLOR, &tmp->highlight.back, back);
		Scr->FirstTime = save;
	}

	return tmp;
}

void GotButton(int butt, int func)
{
	int i;
	MenuItem *item;

	for(i = 0; i < NUM_CONTEXTS; i++) {
		if((cont & (1 << i)) == 0) {
			continue;
		}

		if(func == F_MENU) {
			pull->prev = NULL;
			AddFuncButton(butt, i, mods, func, pull, (MenuItem *) 0);
		}
		else {
			root = GetRoot(TWM_ROOT, NULLSTR, NULLSTR);
			item = AddToMenu(root, "x", Action, NULL, func, NULLSTR, NULLSTR);
			AddFuncButton(butt, i, mods, func, (MenuRoot *) 0, item);
		}
	}

	Action = "";
	pull = NULL;
	cont = 0;
	mods_used |= mods;
	mods = 0;
}

void GotKey(char *key, int func)
{
	int i;

	for(i = 0; i < NUM_CONTEXTS; i++) {
		if((cont & (1 << i)) == 0) {
			continue;
		}

		if(func == F_MENU) {
			pull->prev = NULL;
			if(!AddFuncKey(key, i, mods, func, pull, Name, Action)) {
				break;
			}
		}
		else if(!AddFuncKey(key, i, mods, func, (MenuRoot *) 0, Name, Action)) {
			break;
		}
	}

	Action = "";
	pull = NULL;
	cont = 0;
	mods_used |= mods;
	mods = 0;
}


void GotTitleButton(char *bitmapname, int func, Bool rightside)
{
	if(!CreateTitleButton(bitmapname, func, Action, pull, rightside, True)) {
		twmrc_error_prefix();
		fprintf(stderr,
		        "unable to create %s titlebutton \"%s\"\n",
		        rightside ? "right" : "left", bitmapname);
	}
	Action = "";
	pull = NULL;
}

Bool CheckWarpScreenArg(char *s)
{
	if(strcasecmp(s,  WARPSCREEN_NEXT) == 0 ||
	                strcasecmp(s,  WARPSCREEN_PREV) == 0 ||
	                strcasecmp(s,  WARPSCREEN_BACK) == 0) {
		return True;
	}

	for(; *s && Isascii(*s) && Isdigit(*s); s++) ;  /* SUPPRESS 530 */
	return (*s ? False : True);
}


Bool CheckWarpRingArg(char *s)
{
	if(strcasecmp(s,  WARPSCREEN_NEXT) == 0 ||
	                strcasecmp(s,  WARPSCREEN_PREV) == 0) {
		return True;
	}

	return False;
}


Bool CheckColormapArg(char *s)
{
	if(strcasecmp(s, COLORMAP_NEXT) == 0 ||
	                strcasecmp(s, COLORMAP_PREV) == 0 ||
	                strcasecmp(s, COLORMAP_DEFAULT) == 0) {
		return True;
	}

	return False;
}
