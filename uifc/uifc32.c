/* uifc32.c */

/* Curses implementation of UIFC (user interface) library based on uifc.c */

/****************************************************************************
 * @format.tab-size 4		(Plain Text/Source Code File Header)			*
 * @format.use-tabs true	(see http://www.synchro.net/ptsc_hdr.html)		*
 *																			*
 * Copyright 2003 Rob Swindell - http://www.synchro.net/copyright.html		*
 *																			*
 * This library is free software; you can redistribute it and/or			*
 * modify it under the terms of the GNU Lesser General Public License		*
 * as published by the Free Software Foundation; either version 2			*
 * of the License, or (at your option) any later version.					*
 * See the GNU Lesser General Public License for more details: lgpl.txt or	*
 * http://www.fsf.org/copyleft/lesser.html									*
 *																			*
 * Anonymous FTP access to the most recent released source is available at	*
 * ftp://vert.synchro.net, ftp://cvs.synchro.net and ftp://ftp.synchro.net	*
 *																			*
 * Anonymous CVS access to the development source and modification history	*
 * is available at cvs.synchro.net:/cvsroot/sbbs, example:					*
 * cvs -d :pserver:anonymous@cvs.synchro.net:/cvsroot/sbbs login			*
 *     (just hit return, no password is necessary)							*
 * cvs -d :pserver:anonymous@cvs.synchro.net:/cvsroot/sbbs checkout src		*
 *																			*
 * For Synchronet coding style and modification guidelines, see				*
 * http://www.synchro.net/source.html										*
 *																			*
 * You are encouraged to submit any modifications (preferably in Unix diff	*
 * format) via e-mail to mods@synchro.net									*
 *																			*
 * Note: If this box doesn't appear square, then you need to fix your tabs.	*
 ****************************************************************************/

#ifdef __unix__
	#include <stdio.h>
	#include <unistd.h>
	#ifdef __QNX__
		#include <strings.h>
	#endif
	#include "ciowrap.h"
    #define mswait(x) delay(x)
    #define putch(x)	_putch(x,TRUE)
    #define clreol()	clrtoeol()
#elif defined(_WIN32)
	#include <share.h>
	#include <conio.h>
	#include <windows.h>
	#include "keys.h"
	#define mswait(x) Sleep(x)
#endif

#include "uifc.h"

							/* Bottom line elements */
#define BL_INS      (1<<0)  /* INS key */
#define BL_DEL      (1<<1)  /* DEL key */
#define BL_GET      (1<<2)  /* Get key */
#define BL_PUT      (1<<3)  /* Put key */

#ifdef __unix__
enum {
	 BLACK
	,BLUE	
	,GREEN	
	,CYAN
	,RED
	,MAGENTA
	,BROWN	
	,LIGHTGRAY	
	,DARKGRAY
	,LIGHTBLUE	
	,LIGHTGREEN	
	,LIGHTCYAN
	,LIGHTRED
	,LIGHTMAGENTA
	,YELLOW
	,WHITE
};
#endif

#define BLINK	128

static char  hclr,lclr,bclr,cclr,lbclr;
static int   cursor;
static char* helpfile=0;
static uint  helpline=0;
static char  blk_scrn[MAX_BFLN];
static win_t sav[MAX_BUFS];
static uifcapi_t* api;

/* Prototypes */
static int   uprintf(int x, int y, unsigned char attr, char *fmt,...);
static void  bottomline(int line);
static char  *utimestr(time_t *intime);
static void  help(void);
static int   ugetstr(int left, int top, char *outstr, int max, long mode, int *lastkey);
static void  timedisplay(void);

/* API routines */
static void uifcbail(void);
static int  uscrn(char *str);
static int  ulist(int mode, int left, int top, int width, int *dflt, int *bar
	,char *title, char **option);
static int  uinput(int imode, int left, int top, char *prompt, char *str
	,int len ,int kmode);
static void umsg(char *str);
static void upop(char *str);
static void sethelp(int line, char* file);
static void showbuf(int mode, int left, int top, int width, int height, char *title
	, char *hbuf, int *curp, int *barp);

/* Dynamic menu support */
static int *last_menu_cur=NULL;
static int *last_menu_bar=NULL;
static int save_menu_cur=-1;
static int save_menu_bar=-1;

static void reset_dynamic(void) {
	last_menu_cur=NULL;
	last_menu_bar=NULL;
	save_menu_cur=-1;
	save_menu_bar=-1;
}

/****************************************************************************/
/* Initialization function, see uifc.h for details.							*/
/* Returns 0 on success.													*/
/****************************************************************************/

int kbwait(void) {
	int timeout=0;
	while(timeout++<500) {
		if(kbhit)
			return(TRUE);
		mswait(1);
	}
	return(FALSE);
}

int inkey(int mode)
{
	int c;

	if(mode)
		return(kbwait());
	c=getch();
#ifdef _WIN32
	if(!c)
		c=(getch()<<8);
#endif
	return(c);
}

int uifcini32(uifcapi_t* uifcapi)
{
	int 	i;
	struct	text_info txtinfo;

    if(uifcapi==NULL || uifcapi->size!=sizeof(uifcapi_t))
        return(-1);

    api=uifcapi;

    /* install function handlers */            
    api->bail=uifcbail;
    api->scrn=uscrn;
    api->msg=umsg;
    api->pop=upop;
    api->list=ulist;
    api->input=uinput;
    api->sethelp=sethelp;
    api->showhelp=help;
	api->showbuf=showbuf;
	api->timedisplay=timedisplay;
	api->getstrxy=ugetstr;

#ifdef __unix__
	initciowrap(api->mode);
	#ifdef NCURSES_VERSION_MAJOR
		ESCDELAY=api->esc_delay;
	#endif
#else
    if(api->scrn_len!=0) {
        switch(api->scrn_len) {
            case 14:
                textmode(C80X14);
                break;
            case 21:
                textmode(C80X21);
                break;
            case 25:
                textmode(C80);
                break;
            case 28:
                textmode(C80X28);
                break;
            case 43:
                textmode(C80X43);
                break;
            case 50:
                textmode(C80X50);
                break;
            case 60:
                textmode(C80X60);
                break;
            default:
                textmode(C4350);
                break;
        }
    }
#endif

    clrscr();

    gettextinfo(&txtinfo);
#ifdef _WIN32
    /* unsupported mode? */
    if(txtinfo.screenheight<MIN_LINES
        || txtinfo.screenheight>MAX_LINES
        || txtinfo.screenwidth<80) {
        textmode(C80);  /* set mode to 80x25*/
        gettextinfo(&txtinfo);
    }
#endif

    api->scrn_len=txtinfo.screenheight;
    if(api->scrn_len<MIN_LINES || api->scrn_len>MAX_LINES) {
        cprintf("\7UIFC: Screen length (%u) must be between %d and %d lines\r\n"
            ,api->scrn_len,MIN_LINES,MAX_LINES);
        return(-2);
    }
    api->scrn_len--; /* account for status line */

    if(txtinfo.screenwidth<80) {
        cprintf("\7UIFC: Screen width (%u) must be at least 80 characters\r\n"
            ,txtinfo.screenwidth);
        return(-3);
    }
	api->scrn_width=txtinfo.screenwidth;

    if(!(api->mode&UIFC_COLOR)
        && (api->mode&UIFC_MONO
            || txtinfo.currmode==MONO || txtinfo.currmode==BW80)) {
        bclr=BLACK;
        hclr=WHITE;
        lclr=LIGHTGRAY;
        cclr=LIGHTGRAY;
#ifdef __unix__
		if(txtinfo.currmode==MONO)
			lbclr=WHITE|(BLACK<<4);		/* no color on curses means no inverse either */
		else
#endif
			lbclr=BLACK|(LIGHTGRAY<<4);	/* lightbar color */
    } else {
        bclr=BLUE;
        hclr=YELLOW;
        lclr=WHITE;
        cclr=CYAN;
		lbclr=BLUE|(LIGHTGRAY<<4);	/* lightbar color */
    }

    for(i=0;i<MAX_BFLN;i+=2) {
        blk_scrn[i]='�';
        blk_scrn[i+1]=cclr|(bclr<<4);
    }

    cursor=_NOCURSOR;
    _setcursortype(cursor);

    return(0);
}

void uifcbail(void)
{
	_setcursortype(_NORMALCURSOR);
	textattr(LIGHTGRAY);
	clrscr();
#ifdef __unix__
	nl();
	nocbreak();
	noraw();
	refresh();
	endwin();
#endif
}

/****************************************************************************/
/* Clear screen, fill with background attribute, display application title.	*/
/* Returns 0 on success.													*/
/****************************************************************************/
int uscrn(char *str)
{
    textattr(bclr|(cclr<<4));
    gotoxy(1,1);
    clreol();
    gotoxy(3,1);
	cputs(str);
    if(!puttext(1,2,api->scrn_width,api->scrn_len,blk_scrn))
        return(-1);
    gotoxy(1,api->scrn_len+1);
    clreol();
	reset_dynamic();
    return(0);
}

/****************************************************************************/
/****************************************************************************/
static void scroll_text(int x1, int y1, int x2, int y2, int down)
{
	uchar buf[MAX_BFLN];

	gettext(x1,y1,x2,y2,buf);
	if(down)
		puttext(x1,y1+1,x2,y2,buf);
	else
		puttext(x1,y1,x2,y2-1,buf+(((x2-x1)+1)*2));
}

/****************************************************************************/
/* Updates time in upper left corner of screen with current time in ASCII/  */
/* Unix format																*/
/****************************************************************************/
static void timedisplay()
{
	static time_t savetime;
	time_t now;

	now=time(NULL);
	if(difftime(now,savetime)>=60) {
		uprintf(api->scrn_width-25,1,bclr|(cclr<<4),utimestr(&now));
		savetime=now; 
	}
}

/****************************************************************************/
/* Truncates white-space chars off end of 'str'								*/
/****************************************************************************/
static void truncsp(char *str)
{
	uint c;

	c=strlen(str);
	while(c && (uchar)str[c-1]<=SP) c--;
	if(str[c]!=0)	/* don't write to string constants */
		str[c]=0;
}

/****************************************************************************/
/* General menu function, see uifc.h for details.							*/
/****************************************************************************/
int ulist(int mode, int left, int top, int width, int *cur, int *bar
	, char *title, char **option)
{
	uchar line[256],shade[256],win[MAX_BFLN],*ptr,a,b,c,longopt
		,search[MAX_OPLN],bline=0;
	int height,y;
	int i,j,opts=0,s=0; /* s=search index into options */
	int	is_redraw=0;
	uint s_top=SCRN_TOP;
	uint s_left=SCRN_LEFT;
	uint s_right=SCRN_RIGHT;
	uint s_bottom=api->scrn_len-3;

	if(mode&WIN_FAT) {
		s_top=1;
		s_left=0;
		s_right=api->scrn_width-3;  /* Leave space for the shadow */
		s_bottom=api->scrn_len-1;   /* Leave one for the shadow */
	}
	if(mode&WIN_SAV && api->savnum>=MAX_BUFS-1)
		putch(7);
	if(mode&WIN_INS) bline|=BL_INS;
	if(mode&WIN_DEL) bline|=BL_DEL;
	if(mode&WIN_GET) bline|=BL_GET;
	if(mode&WIN_PUT) bline|=BL_PUT;
	bottomline(bline);
	while(opts<MAX_OPTS)
		if(option[opts][0]==0)
			break;
		else opts++;
	if(mode&WIN_XTR && opts<MAX_OPTS)
		opts++;
	height=opts+4;
	if(top+height>s_bottom)
		height=(s_bottom)-top;
	if(!width || width<strlen(title)+6) {
		width=strlen(title)+6;
		for(i=0;i<opts;i++) {
			truncsp(option[i]);
			if((j=strlen(option[i])+5)>width)
				width=j;
		}
	}
	if(width>(s_right+1)-s_left) {
		width=(s_right+1)-s_left;
		if(strlen(title)>(width-4)) {
			*(title+width-7)='.';
			*(title+width-6)='.';
			*(title+width-5)='.';
			*(title+width-4)=0;
		}
	}
	if(mode&WIN_L2R)
		left=(api->scrn_width-width+1)/2-s_left;
	else if(mode&WIN_RHT)
		left=s_right-(width+left+2);
	if(mode&WIN_T2B)
		top=(api->scrn_len-height+1)/2-2;
	else if(mode&WIN_BOT)
		top=s_bottom-height-top;

	/* Dynamic Menus */
	if(mode&WIN_DYN
			&& cur != NULL
			&& bar != NULL
			&& last_menu_cur==cur
			&& last_menu_bar==bar
			&& save_menu_cur==*cur
			&& save_menu_bar==*bar)
		is_redraw=1;

	if(!is_redraw) {
		if(mode&WIN_SAV && api->savdepth==api->savnum) {
			if((sav[api->savnum].buf=(char *)MALLOC((width+3)*(height+2)*2))==NULL) {
				cprintf("UIFC line %d: error allocating %u bytes."
					,__LINE__,(width+3)*(height+2)*2);
				return(-1);
			}
			gettext(s_left+left,s_top+top,s_left+left+width+1
				,s_top+top+height,sav[api->savnum].buf);
			sav[api->savnum].left=s_left+left;
			sav[api->savnum].top=s_top+top;
			sav[api->savnum].right=s_left+left+width+1;
			sav[api->savnum].bot=s_top+top+height;
			api->savdepth++;
		}
		else if(mode&WIN_SAV
			&& (sav[api->savnum].left!=s_left+left
			|| sav[api->savnum].top!=s_top+top
			|| sav[api->savnum].right!=s_left+left+width+1
			|| sav[api->savnum].bot!=s_top+top+height)) { /* dimensions have changed */
			puttext(sav[api->savnum].left,sav[api->savnum].top,sav[api->savnum].right,sav[api->savnum].bot
				,sav[api->savnum].buf);	/* put original window back */
			FREE(sav[api->savnum].buf);
			if((sav[api->savnum].buf=(char *)MALLOC((width+3)*(height+2)*2))==NULL) {
				cprintf("UIFC line %d: error allocating %u bytes."
					,__LINE__,(width+3)*(height+2)*2);
				return(-1);
			}
			gettext(s_left+left,s_top+top,s_left+left+width+1
				,s_top+top+height,sav[api->savnum].buf);	  /* save again */
			sav[api->savnum].left=s_left+left;
			sav[api->savnum].top=s_top+top;
			sav[api->savnum].right=s_left+left+width+1;
			sav[api->savnum].bot=s_top+top+height;
		}
	}

	if(!is_redraw) {
		if(mode&WIN_ORG) { /* Clear around menu */
			if(top)
				puttext(s_left,s_top,s_right+2,s_top+top-1,blk_scrn);
			if(s_top+height+top<=api->scrn_len)
				puttext(s_left,s_top+height+top,s_right+2,api->scrn_len,blk_scrn);
			if(left)
				puttext(s_left,s_top+top,s_left+left-1,s_top+height+top
					,blk_scrn);
			if(s_left+left+width<=s_right)
				puttext(s_left+left+width,s_top+top,s_right+2
					,s_top+height+top,blk_scrn);
		}
		ptr=win;
		*(ptr++)='�';
		*(ptr++)=hclr|(bclr<<4);

		i=0;
		for(;i<width-2;i++) {
			*(ptr++)='�';
			*(ptr++)=hclr|(bclr<<4);
		}
		*(ptr++)='�';
		*(ptr++)=hclr|(bclr<<4);
		*(ptr++)='�';
		*(ptr++)=hclr|(bclr<<4);
		a=strlen(title);
		b=(width-a-1)/2;
		for(i=0;i<b;i++) {
			*(ptr++)=' ';
			*(ptr++)=hclr|(bclr<<4);
		}
		for(i=0;i<a;i++) {
			*(ptr++)=title[i];
			*(ptr++)=hclr|(bclr<<4);
		}
		for(i=0;i<width-(a+b)-2;i++) {
			*(ptr++)=' ';
			*(ptr++)=hclr|(bclr<<4);
		}
		*(ptr++)='�';
		*(ptr++)=hclr|(bclr<<4);
		*(ptr++)='�';
		*(ptr++)=hclr|(bclr<<4);
		for(i=0;i<width-2;i++) {
			*(ptr++)='�';
			*(ptr++)=hclr|(bclr<<4);
		}
		*(ptr++)='�';
		*(ptr++)=hclr|(bclr<<4);

		if((*cur)>=opts)
			(*cur)=opts-1;			/* returned after scrolled */

		if(!bar) {
			if((*cur)>height-5)
				(*cur)=height-5;
			i=0;
		}
		else {
			if((*bar)>=opts)
				(*bar)=opts-1;
			if((*bar)>height-5)
				(*bar)=height-5;
			if((*cur)==opts-1)
				(*bar)=height-5;
			if((*bar)<0)
				(*bar)=0;
			if((*cur)<(*bar))
				(*cur)=(*bar);
			i=(*cur)-(*bar);
			if(i+(height-5)>=opts) {
				i=opts-(height-4);
				(*cur)=i+(*bar);
				}
			}
			if((*cur)<0)
				(*cur)=0;

			j=0;
			if(i<0) i=0;
			longopt=0;
			while(j<height-4 && i<opts) {
				*(ptr++)='�';
				*(ptr++)=hclr|(bclr<<4);
				*(ptr++)=' ';
				*(ptr++)=hclr|(bclr<<4);
				*(ptr++)='�';
				*(ptr++)=lclr|(bclr<<4);
				if(i==(*cur))
					a=lbclr;
				else
					a=lclr|(bclr<<4);
				b=strlen(option[i]);
				if(b>longopt)
					longopt=b;
				if(b+4>width)
					b=width-4;
				for(c=0;c<b;c++) {
					*(ptr++)=option[i][c];
					*(ptr++)=a; 
				}
				while(c<width-4) {
					*(ptr++)=' ';
					*(ptr++)=a;
					c++;
				}
				*(ptr++)='�';
				*(ptr++)=hclr|(bclr<<4);
				i++;
				j++; 
			}
			*(ptr++)='�';
			*(ptr++)=hclr|(bclr<<4);
			for(i=0;i<width-2;i++) {
				*(ptr++)='�';
				*(ptr++)=hclr|(bclr<<4); 
			}
			*(ptr++)='�';
			*(ptr)=hclr|(bclr<<4);	/* Not incremented to shut ot BCC */
			puttext(s_left+left,s_top+top,s_left+left+width-1
				,s_top+top+height-1,win);
			if(bar)
				y=top+3+(*bar);
			else
				y=top+3+(*cur);
			if(opts+4>height && ((!bar && (*cur)!=opts-1)
				|| (bar && ((*cur)-(*bar))+(height-4)<opts))) {
				gotoxy(s_left+left+1,s_top+top+height-2);
				textattr(lclr|(bclr<<4));
				putch(31);	   /* put down arrow */
				textattr(hclr|(bclr<<4)); 
			}

			if(bar && (*bar)!=(*cur)) {
				gotoxy(s_left+left+1,s_top+top+3);
				textattr(lclr|(bclr<<4));
				putch(30);	   /* put the up arrow */
				textattr(hclr|(bclr<<4)); 
			}

			if(bclr==BLUE) {
				gettext(s_left+left+width,s_top+top+1,s_left+left+width+1
					,s_top+top+height-1,shade);
				for(i=1;i<height*4;i+=2)
					shade[i]=DARKGRAY;
				puttext(s_left+left+width,s_top+top+1,s_left+left+width+1
					,s_top+top+height-1,shade);
				gettext(s_left+left+2,s_top+top+height,s_left+left+width+1
					,s_top+top+height,shade);
				for(i=1;i<width*2;i+=2)
					shade[i]=DARKGRAY;
				puttext(s_left+left+2,s_top+top+height,s_left+left+width+1
					,s_top+top+height,shade);
			}
	}
	else {	/* Is a redraw */
		i=(*cur)-(*bar);
		j=2;

		longopt=0;
		while(j<height-2 && i<opts) {
			ptr=win;
			if(i==(*cur))
				a=lbclr;
			else
				a=lclr|(bclr<<4);
			b=strlen(option[i]);
			if(b>longopt)
				longopt=b;
			if(b+4>width)
				b=width-4;
			for(c=0;c<b;c++) {
				*(ptr++)=option[i][c];
				*(ptr++)=a; 
			}
			while(c<width-4) {
				*(ptr++)=' ';
				*(ptr++)=a;
				c++; 
			}
			i++;
			j++; 
			puttext(s_left+left+3,s_top+top+j,s_left+left+width-2
				,s_top+top+j,win); 
		}
		if(bar)
			y=top+3+(*bar);
		else
			y=top+3+(*cur);
	}

	last_menu_cur=cur;
	last_menu_bar=bar;
	if(mode&WIN_IMM)
		return(-2);
	while(1) {
	#if 0					/* debug */
		gotoxy(30,1);
		cprintf("y=%2d h=%2d c=%2d b=%2d s=%2d o=%2d"
			,y,height,*cur,bar ? *bar :0xff,api->savdepth,opts);
	#endif
		timedisplay();
		i=0;
		if(inkey(1)) {
			i=inkey(0);
			if(i==KEY_BACKSPACE || i==BS)
				i=ESC;
			if(i>255) {
				s=0;
				switch(i) {
					/* ToDo extended keys */
					case KEY_HOME:	/* home */
						if(!opts)
							break;
						if(opts+4>height) {
							gotoxy(s_left+left+1,s_top+top+3);
							textattr(lclr|(bclr<<4));
							putch(' ');    /* Delete the up arrow */
							gotoxy(s_left+left+1,s_top+top+height-2);
							putch(31);	   /* put the down arrow */
							uprintf(s_left+left+3,s_top+top+3
								,lbclr
								,"%-*.*s",width-4,width-4,option[0]);
							for(i=1;i<height-4;i++)    /* re-display options */
								uprintf(s_left+left+3,s_top+top+3+i
									,lclr|(bclr<<4)
									,"%-*.*s",width-4,width-4,option[i]);
							(*cur)=0;
							if(bar)
								(*bar)=0;
							y=top+3;
							break; 
						}
						gettext(s_left+3+left,s_top+y
							,s_left+left+width-2,s_top+y,line);
						for(i=1;i<width*2;i+=2)
							line[i]=lclr|(bclr<<4);
						puttext(s_left+3+left,s_top+y
							,s_left+left+width-2,s_top+y,line);
						(*cur)=0;
						if(bar)
							(*bar)=0;
						y=top+3;
						gettext(s_left+3+left,s_top+y
							,s_left+left+width-2,s_top+y,line);
						for(i=1;i<width*2;i+=2)
							line[i]=lbclr;
						puttext(s_left+3+left,s_top+y
							,s_left+left+width-2,s_top+y,line);
						break;
					case KEY_UP:	/* up arrow */
						if(!opts)
							break;
						if(!(*cur) && opts+4>height) {
							gotoxy(s_left+left+1,s_top+top+3); /* like end */
							textattr(lclr|(bclr<<4));
							putch(30);	   /* put the up arrow */
							gotoxy(s_left+left+1,s_top+top+height-2);
							putch(' ');    /* delete the down arrow */
							for(i=(opts+4)-height,j=0;i<opts;i++,j++)
								uprintf(s_left+left+3,s_top+top+3+j
									,i==opts-1 ? lbclr
										: lclr|(bclr<<4)
									,"%-*.*s",width-4,width-4,option[i]);
							(*cur)=opts-1;
							if(bar)
								(*bar)=height-5;
							y=top+height-2;
							break; 
						}
						gettext(s_left+3+left,s_top+y
							,s_left+left+width-2,s_top+y,line);
						for(i=1;i<width*2;i+=2)
							line[i]=lclr|(bclr<<4);
						puttext(s_left+3+left,s_top+y
							,s_left+left+width-2,s_top+y,line);
						if(!(*cur)) {
							y=top+height-2;
							(*cur)=opts-1;
							if(bar)
								(*bar)=height-5; 
						}
						else {
							(*cur)--;
							y--;
							if(bar && *bar)
								(*bar)--; 
						}
						if(y<top+3) {	/* scroll */
							if(!(*cur)) {
								gotoxy(s_left+left+1,s_top+top+3);
								textattr(lclr|(bclr<<4));
								putch(' '); /* delete the up arrow */
							}  
							if((*cur)+height-4==opts-1) {
								gotoxy(s_left+left+1,s_top+top+height-2);
								textattr(lclr|(bclr<<4));
								putch(31);	/* put the dn arrow */
							}
							y++;
							scroll_text(s_left+left+2,s_top+top+3
								,s_left+left+width-3,s_top+top+height-2,1);
							uprintf(s_left+left+3,s_top+top+3
								,lbclr
								,"%-*.*s",width-4,width-4,option[*cur]);
						}
						else {
							gettext(s_left+3+left,s_top+y
								,s_left+left+width-2,s_top+y,line);
							for(i=1;i<width*2;i+=2)
								line[i]=lbclr;
							puttext(s_left+3+left,s_top+y
								,s_left+left+width-2,s_top+y,line);
						}
						break;
#if 0
					case KEY_PPAGE;	/* PgUp */
					case KEY_NPAGE;	/* PgDn */
						if(!opts || (*cur)==(opts-1))
							break;
						(*cur)+=(height-4);
						if((*cur)>(opts-1))
							(*cur)=(opts-1);

						gettext(s_left+3+left,s_top+y
							,s_left+left+width-2,s_top+y,line);
						for(i=1;i<width*2;i+=2)
							line[i]=lclr|(bclr<<4);
						puttext(s_left+3+left,s_top+y
							,s_left+left+width-2,s_top+y,line);

						for(i=(opts+4)-height,j=0;i<opts;i++,j++)
							uprintf(s_left+left+3,s_top+top+3+j
								,i==(*cur) lbclr : lclr|(bclr<<4)
								,"%-*.*s",width-4,width-4,option[i]);
						y=top+height-2;
						if(bar)
							(*bar)=height-5;
						gettext(s_left+3+left,s_top+y
							,s_left+left+width-2,s_top+y,line);
						for(i=1;i<148;i+=2)
							line[i]=lbclr;
						puttext(s_left+3+left,s_top+y
							,s_left+left+width-2,s_top+y,line);
						break;
#endif
					case KEY_END:	/* end */
						if(!opts)
							break;
						if(opts+4>height) {	/* Scroll mode */
							gotoxy(s_left+left+1,s_top+top+3);
							textattr(lclr|(bclr<<4));
							putch(30);	   /* put the up arrow */
							gotoxy(s_left+left+1,s_top+top+height-2);
							putch(' ');    /* delete the down arrow */
							for(i=(opts+4)-height,j=0;i<opts;i++,j++)
								uprintf(s_left+left+3,s_top+top+3+j
									,i==opts-1 ? lbclr
										: lclr|(bclr<<4)
									,"%-*.*s",width-4,width-4,option[i]);
							(*cur)=opts-1;
							y=top+height-2;
							if(bar)
								(*bar)=height-5;
							break; 
						}
						gettext(s_left+3+left,s_top+y
							,s_left+left+width-2,s_top+y,line);
						for(i=1;i<width*2;i+=2)
							line[i]=lclr|(bclr<<4);
						puttext(s_left+3+left,s_top+y
							,s_left+left+width-2,s_top+y,line);
						(*cur)=opts-1;
						y=top+height-2;
						if(bar)
							(*bar)=height-5;
						gettext(s_left+3+left,s_top+y
							,s_left+left+width-2,s_top+y,line);
						for(i=1;i<148;i+=2)
							line[i]=lbclr;
						puttext(s_left+3+left,s_top+y
							,s_left+left+width-2,s_top+y,line);
						break;
					case KEY_DOWN:	/* dn arrow */
						if(!opts)
							break;
						if((*cur)==opts-1 && opts+4>height) { /* like home */
							gotoxy(s_left+left+1,s_top+top+3);
							textattr(lclr|(bclr<<4));
							putch(' ');    /* Delete the up arrow */
							gotoxy(s_left+left+1,s_top+top+height-2);
							putch(31);	   /* put the down arrow */
							uprintf(s_left+left+3,s_top+top+3
								,lbclr
								,"%-*.*s",width-4,width-4,option[0]);
							for(i=1;i<height-4;i++)    /* re-display options */
								uprintf(s_left+left+3,s_top+top+3+i
									,lclr|(bclr<<4)
									,"%-*.*s",width-4,width-4,option[i]);
							(*cur)=0;
							y=top+3;
							if(bar)
								(*bar)=0;
							break; 
						}
						gettext(s_left+3+left,s_top+y
							,s_left+left+width-2,s_top+y,line);
						for(i=1;i<width*2;i+=2)
							line[i]=lclr|(bclr<<4);
						puttext(s_left+3+left,s_top+y
							,s_left+left+width-2,s_top+y,line);
						if((*cur)==opts-1) {
							(*cur)=0;
							y=top+3;
							if(bar) {
								/* gotoxy(1,1); cprintf("bar=%08lX ",bar); */
								(*bar)=0; 
							} 
						}
						else {
							(*cur)++;
							y++;
							if(bar && (*bar)<height-5) {
								/* gotoxy(1,1); cprintf("bar=%08lX ",bar); */
								(*bar)++; 
							} 
						}
						if(y==top+height-1) {	/* scroll */
							if(*cur==opts-1) {
								gotoxy(s_left+left+1,s_top+top+height-2);
								textattr(lclr|(bclr<<4));
								putch(' ');	/* delete the down arrow */
							}
							if((*cur)+4==height) {
								gotoxy(s_left+left+1,s_top+top+3);
								textattr(lclr|(bclr<<4));
								putch(30);	/* put the up arrow */
							}
							y--;
							/* gotoxy(1,1); cprintf("\rdebug: %4d ",__LINE__); */
							scroll_text(s_left+left+2,s_top+top+3
								,s_left+left+width-3,s_top+top+height-2,0);
							/* gotoxy(1,1); cprintf("\rdebug: %4d ",__LINE__); */
							uprintf(s_left+left+3,s_top+top+height-2
								,lbclr
								,"%-*.*s",width-4,width-4,option[*cur]);
						}
						else {
							gettext(s_left+3+left,s_top+y
								,s_left+left+width-2,s_top+y
								,line);
							for(i=1;i<width*2;i+=2)
								line[i]=lbclr;
							puttext(s_left+3+left,s_top+y
								,s_left+left+width-2,s_top+y
								,line);
						}
						break;
					case KEY_F(1):	/* F1 */
						help();
						break;
					case KEY_F(5):	/* F5 */
						if(mode&WIN_GET && !(mode&WIN_XTR && (*cur)==opts-1)) {
							return((*cur)|MSK_GET);
						}
						break;
					case KEY_F(6):	/* F6 */
						if(mode&WIN_PUT && !(mode&WIN_XTR && (*cur)==opts-1)) {
							return((*cur)|MSK_PUT);
						}
						break;
					case KEY_IC:	/* insert */
						if(mode&WIN_INS) {
							if(mode&WIN_INSACT) {
								gettext(s_left+left,s_top+top,s_left
									+left+width-1,s_top+top+height-1,win);
								for(i=1;i<(width*height*2);i+=2)
									win[i]=lclr|(cclr<<4);
								if(opts) {
									j=(((y-top)*width)*2)+7+((width-4)*2);
									for(i=(((y-top)*width)*2)+7;i<j;i+=2)
										win[i]=hclr|(cclr<<4); 
								}
								puttext(s_left+left,s_top+top,s_left
									+left+width-1,s_top+top+height-1,win);
							}
							if(!opts) {
								return(MSK_INS); 
							}
							return((*cur)|MSK_INS); 
						}
						break;
					case KEY_DC:	/* delete */
						if(mode&WIN_XTR && (*cur)==opts-1)	/* can't delete */
							break;							/* extra line */
						if(mode&WIN_DEL) {
							if(mode&WIN_DELACT) {
								gettext(s_left+left,s_top+top,s_left
									+left+width-1,s_top+top+height-1,win);
								for(i=1;i<(width*height*2);i+=2)
									win[i]=lclr|(cclr<<4);
								j=(((y-top)*width)*2)+7+((width-4)*2);
								for(i=(((y-top)*width)*2)+7;i<j;i+=2)
									win[i]=hclr|(cclr<<4);
								puttext(s_left+left,s_top+top,s_left
									+left+width-1,s_top+top+height-1,win);
							}
							return((*cur)|MSK_DEL); 
						}
						break;
				} 
			}
			else {
				i&=0xff;
				if(isalnum(i) && opts && option[0][0]) {
					search[s]=i;
					search[s+1]=0;
					for(j=(*cur)+1,a=b=0;a<2;j++) {   /* a = search count */
						if(j==opts) {					/* j = option count */
							j=-1;						/* b = letter count */
							continue; 
						}
						if(j==(*cur)) {
							b++;
							continue; 
						}
						if(b>=longopt) {
							b=0;
							a++; 
						}
						if(a==1 && !s)
							break;
						if(strlen(option[j])>b
							&& ((!a && s && !strnicmp(option[j]+b,search,s+1))
							|| ((a || !s) && toupper(option[j][b])==toupper(i)))) {
							if(a) s=0;
							else s++;
							if(y+(j-(*cur))+2>height+top) {
								(*cur)=j;
								gotoxy(s_left+left+1,s_top+top+3);
								textattr(lclr|(bclr<<4));
								putch(30);	   /* put the up arrow */
								if((*cur)==opts-1) {
									gotoxy(s_left+left+1,s_top+top+height-2);
									putch(' ');	/* delete the down arrow */
								}
								for(i=((*cur)+5)-height,j=0;i<(*cur)+1;i++,j++)
									uprintf(s_left+left+3,s_top+top+3+j
										,i==(*cur) ? lbclr
											: lclr|(bclr<<4)
										,"%-*.*s",width-4,width-4,option[i]);
								y=top+height-2;
								if(bar)
									(*bar)=height-5;
								break; 
							}
							if(y-((*cur)-j)<top+3) {
								(*cur)=j;
								gotoxy(s_left+left+1,s_top+top+3);
								textattr(lclr|(bclr<<4));
								if(!(*cur))
									putch(' ');    /* Delete the up arrow */
								gotoxy(s_left+left+1,s_top+top+height-2);
								putch(31);	   /* put the down arrow */
								uprintf(s_left+left+3,s_top+top+3
									,lbclr
									,"%-*.*s",width-4,width-4,option[(*cur)]);
								for(i=1;i<height-4;i++) 	/* re-display options */
									uprintf(s_left+left+3,s_top+top+3+i
										,lclr|(bclr<<4)
										,"%-*.*s",width-4,width-4
										,option[(*cur)+i]);
								y=top+3;
								if(bar)
									(*bar)=0;
								break; 
							}
							gettext(s_left+3+left,s_top+y
								,s_left+left+width-2,s_top+y,line);
							for(i=1;i<width*2;i+=2)
								line[i]=lclr|(bclr<<4);
							puttext(s_left+3+left,s_top+y
								,s_left+left+width-2,s_top+y,line);
							if((*cur)>j)
								y-=(*cur)-j;
							else
								y+=j-(*cur);
							if(bar) {
								if((*cur)>j)
									(*bar)-=(*cur)-j;
								else
									(*bar)+=j-(*cur); 
							}
							(*cur)=j;
							gettext(s_left+3+left,s_top+y
								,s_left+left+width-2,s_top+y,line);
							for(i=1;i<width*2;i+=2)
								line[i]=lbclr;
							puttext(s_left+3+left,s_top+y
								,s_left+left+width-2,s_top+y,line);
							break; 
						} 
					}
					if(a==2)
						s=0;
				}
				else
					switch(i) {
						case CR:
							if(!opts || (mode&WIN_XTR && (*cur)==opts-1))
								break;
							if(mode&WIN_ACT) {
								gettext(s_left+left,s_top+top,s_left
									+left+width-1,s_top+top+height-1,win);
								for(i=1;i<(width*height*2);i+=2)
									win[i]=lclr|(cclr<<4);
								j=(((y-top)*width)*2)+7+((width-4)*2);
								for(i=(((y-top)*width)*2)+7;i<j;i+=2)
									win[i]=hclr|(cclr<<4);

								puttext(s_left+left,s_top+top,s_left
									+left+width-1,s_top+top+height-1,win);
							}
							else if(mode&WIN_SAV) {
								puttext(sav[api->savnum].left,sav[api->savnum].top
									,sav[api->savnum].right,sav[api->savnum].bot
									,sav[api->savnum].buf);
								FREE(sav[api->savnum].buf);
								api->savdepth--; 
							}
							return(*cur);
						case 3:
						case ESC:
							if((mode&WIN_ESC || (mode&WIN_CHE && api->changes))
								&& !(mode&WIN_SAV)) {
								gettext(s_left+left,s_top+top,s_left
									+left+width-1,s_top+top+height-1,win);
								for(i=1;i<(width*height*2);i+=2)
									win[i]=lclr|(cclr<<4);
								puttext(s_left+left,s_top+top,s_left
									+left+width-1,s_top+top+height-1,win);
							}
							else if(mode&WIN_SAV) {
								puttext(sav[api->savnum].left,sav[api->savnum].top
									,sav[api->savnum].right,sav[api->savnum].bot
									,sav[api->savnum].buf);
								FREE(sav[api->savnum].buf);
								api->savdepth--; 
							}
							return(-1);
				} 
			} 
		}
		else
			mswait(1);
		if(mode&WIN_DYN) {
			save_menu_cur=*cur;
			save_menu_bar=*bar;
			return(-2-i);
		}
	}
}


/*************************************************************************/
/* This function is a windowed input string input routine.               */
/*************************************************************************/
int uinput(int mode, int left, int top, char *prompt, char *str,
	int max, int kmode)
{
	unsigned char c,save_buf[2048],in_win[2048]
		,shade[160],width,height=3;
	int i,plen,slen;

	reset_dynamic();
	plen=strlen(prompt);
	if(!plen)
		slen=4;
	else
		slen=6;
	width=plen+slen+max;
	if(mode&WIN_T2B)
		top=(api->scrn_len-height+1)/2-2;
	if(mode&WIN_L2R)
		left=(api->scrn_width-width+1)/2-SCRN_LEFT;
	if(mode&WIN_SAV)
		gettext(SCRN_LEFT+left,SCRN_TOP+top,SCRN_LEFT+left+width+1
			,SCRN_TOP+top+height,save_buf);
	i=0;
	in_win[i++]='�';
	in_win[i++]=hclr|(bclr<<4);
	for(c=1;c<width-1;c++) {
		in_win[i++]='�';
		in_win[i++]=hclr|(bclr<<4); 
	}
	in_win[i++]='�';
	in_win[i++]=hclr|(bclr<<4);
	in_win[i++]='�';
	in_win[i++]=hclr|(bclr<<4);

	if(plen) {
		in_win[i++]=SP;
		in_win[i++]=lclr|(bclr<<4); 
	}

	for(c=0;prompt[c];c++) {
		in_win[i++]=prompt[c];
		in_win[i++]=lclr|(bclr<<4); 
	}

	if(plen) {
		in_win[i++]=':';
		in_win[i++]=lclr|(bclr<<4);
	}

	for(c=0;c<max+2;c++) {
		in_win[i++]=SP;
		in_win[i++]=lclr|(bclr<<4); 
	}

	in_win[i++]='�';
	in_win[i++]=hclr|(bclr<<4);
	in_win[i++]='�';
	in_win[i++]=hclr|(bclr<<4);
	for(c=1;c<width-1;c++) {
		in_win[i++]='�';
		in_win[i++]=hclr|(bclr<<4); 
	}
	in_win[i++]='�';
	in_win[i]=hclr|(bclr<<4);	/* I is not incremented to shut up BCC */
	puttext(SCRN_LEFT+left,SCRN_TOP+top,SCRN_LEFT+left+width-1
		,SCRN_TOP+top+height-1,in_win);

	if(bclr==BLUE) {
		gettext(SCRN_LEFT+left+width,SCRN_TOP+top+1,SCRN_LEFT+left+width+1
			,SCRN_TOP+top+(height-1),shade);
		for(c=1;c<12;c+=2)
			shade[c]=DARKGRAY;
		puttext(SCRN_LEFT+left+width,SCRN_TOP+top+1,SCRN_LEFT+left+width+1
			,SCRN_TOP+top+(height-1),shade);
		gettext(SCRN_LEFT+left+2,SCRN_TOP+top+3,SCRN_LEFT+left+width+1
			,SCRN_TOP+top+height,shade);
		for(c=1;c<width*2;c+=2)
			shade[c]=DARKGRAY;
		puttext(SCRN_LEFT+left+2,SCRN_TOP+top+3,SCRN_LEFT+left+width+1
			,SCRN_TOP+top+height,shade); 
	}

	textattr(lclr|(bclr<<4));
	if(!plen)
		i=ugetstr(SCRN_LEFT+left+2,SCRN_TOP+top+1,str,max,kmode,NULL);
	else
		i=ugetstr(SCRN_LEFT+left+plen+4,SCRN_TOP+top+1,str,max,kmode,NULL);
	if(mode&WIN_SAV)
		puttext(SCRN_LEFT+left,SCRN_TOP+top,SCRN_LEFT+left+width+1
			,SCRN_TOP+top+height,save_buf);
	return(i);
}

/****************************************************************************/
/* Displays the message 'str' and waits for the user to select "OK"         */
/****************************************************************************/
void umsg(char *str)
{
	int i=0;
	char *ok[2]={"OK",""};

	if(api->mode&UIFC_INMSG)	/* non-cursive */
		return;
	api->mode|=UIFC_INMSG;
	if(api->savdepth) api->savnum++;
	ulist(WIN_SAV|WIN_MID,0,0,0,&i,0,str,ok);
	if(api->savdepth) api->savnum--;
	api->mode&=~UIFC_INMSG;
}

/****************************************************************************/
/* Gets a string of characters from the user. Turns cursor on. Allows 	    */
/* Different modes - K_* macros. ESC aborts input.                          */
/****************************************************************************/
int ugetstr(int left, int top, char *outstr, int max, long mode, int *lastkey)
{
	uchar   str[256],ins=0,buf[256],y;
	int		ch;
	int     i,j,k,f=0;	/* i=offset, j=length */
	BOOL	gotdecimal=FALSE;

	gotoxy(left,top);
	cursor=_NORMALCURSOR;
	_setcursortype(cursor);
	y=wherey();
	if(mode&K_EDIT && outstr[0]) {
	/***
		truncsp(outstr);
	***/
		outstr[max]=0;
		textattr(lbclr);
		cputs(outstr);
		textattr(lclr|(bclr<<4));
		strcpy(str,outstr);
		i=j=strlen(str);
#if 0
		while(inkey(1)==0) {
			mswait(1);
		}
#endif
		f=inkey(0);
		gotoxy(wherex()-i,y);
		if(f == CR 
				|| (f >= 0xff && f != KEY_DC) 
				|| (f == '\t' && mode&K_TABEXIT) 
				|| (f == '%' && mode&K_SCANNING))
		{
			cputs(outstr);
		}
		else
		{
			cprintf("%*s",i,"");
			gotoxy(wherex()-i,y);
			i=j=0;
		}
	}
	else
		i=j=0;

	ch=0;
	while(ch!=CR)
	{
		if(i>j) j=i;
		if(f || inkey(1))
		{
			if(f)
				ch=f;
			else
				ch=inkey(0);
			if(lastkey != NULL)
				*lastkey=ch;
			f=0;
			switch(ch)
			{
				case KEY_F(1):	/* F1 Help */
					help();
					continue;
				case KEY_LEFT:	/* left arrow */
					if(i)
					{
						gotoxy(wherex()-1,y);
						i--;
					}
					continue;
				case KEY_RIGHT:	/* right arrow */
					if(i<j)
					{
						gotoxy(wherex()+1,y);
						i++;
					}
					continue;
				case KEY_HOME:	/* home */
					if(i)
					{
						gotoxy(wherex()-i,y);
						i=0;
					}
					continue;
				case KEY_END:	/* end */
					if(i<j)
					{
						gotoxy(wherex()+(j-i),y);
						i=j;
					}
					continue;
				case KEY_IC:	/* insert */
					ins=!ins;
					if(ins)
						cursor=_SOLIDCURSOR;
					else
						cursor=_NORMALCURSOR;
					_setcursortype(cursor);
					continue;
				case BS:
				case KEY_BACKSPACE:
					if(i)
					{
						if(i==j)
						{
							cputs("\b \b");
							j--;
							i--;
						}
						else {
							gettext(wherex(),y,wherex()+(j-i),y,buf);
							puttext(wherex()-1,y,wherex()+(j-i)-1,y,buf);
							gotoxy(wherex()+(j-i),y);
							putch(SP);
							gotoxy(wherex()-((j-i)+2),y);
							i--;
							j--;
							if(str[i]=='.')
								gotdecimal=FALSE;
							for(k=i;k<j;k++)
								str[k]=str[k+1]; 
						}
						continue; 
					}
				case KEY_DC:	/* delete */
					if(i<j)
					{
						gettext(wherex()+1,y,wherex()+(j-i),y,buf);
						puttext(wherex(),y,wherex()+(j-i)-1,y,buf);
						gotoxy(wherex()+(j-i),y);
						putch(SP);
						gotoxy(wherex()-((j-i)+1),y);
						for(k=i;k<j;k++)
							str[k]=str[k+1];
						j--;
					}
					continue;
				case 3:
				case ESC:
					{
						cursor=_NOCURSOR;
						_setcursortype(cursor);
						return(-1);
					}
				case CR:
					break;
				case '\t':	/* '%' indicates that a UPC is coming next */
					if(mode&K_TABEXIT)
						ch=CR;
					break;
				case '%':	/* '%' indicates that a UPC is coming next */
					if(mode&K_SCANNING)
						ch=CR;
					break;
				case KEY_UP:
				case KEY_DOWN:
					if(mode&K_DEUCEEXIT)
						ch=CR;
					break;
				case 24:   /* ctrl-x  */
					if(j)
					{
						gotoxy(wherex()-i,y);
						cprintf("%*s",j,"");
						gotoxy(wherex()-j,y);
						i=j=0;
					}
					continue;
				case 25:   /* ctrl-y */
					if(i<j)
					{
						cprintf("%*s",(j-i),"");
						gotoxy(wherex()-(j-i),y);
						j=i;
					}
					continue;
			}
			if(mode&K_NUMBER && !isdigit(ch))
				continue;
			if(mode&K_DECIMAL && !isdigit(ch)) {
				if(ch!='.')
					continue;
				if(gotdecimal)
					continue;
				gotdecimal=TRUE;
			}
			if(mode&K_ALPHA && !isalpha(ch))
				continue;
			if((ch>=SP || (ch==1 && mode&K_MSG)) && i<max && (!ins || j<max) && isprint(ch))
			{
				if(mode&K_UPPER)
					ch=toupper(ch);
				if(ins)
				{
					gettext(wherex(),y,wherex()+(j-i),y,buf);
					puttext(wherex()+1,y,wherex()+(j-i)+1,y,buf);
					for(k=++j;k>i;k--)
						str[k]=str[k-1];
				}
				putch(ch);
				str[i++]=ch; 
			} 
		}
		else
			mswait(1);
	}


	str[j]=0;
	if(mode&K_EDIT)
	{
		truncsp(str);
		if(strcmp(outstr,str))
			api->changes=1;
	}
	else
	{
		if(j)
			api->changes=1;
	}
	strcpy(outstr,str);
	cursor=_NOCURSOR;
	_setcursortype(cursor);
	return(j);
}

/****************************************************************************/
/* Performs printf() through puttext() routine								*/
/****************************************************************************/
static int uprintf(int x, int y, unsigned char attr, char *fmat, ...)
{
	va_list argptr;
	char str[256],buf[512];
	int i,j;

    va_start(argptr,fmat);
    vsprintf(str,fmat,argptr);
    va_end(argptr);
    for(i=j=0;str[i];i++) {
        buf[j++]=str[i];
        buf[j++]=attr; 
	}
    puttext(x,y,x+(i-1),y,buf);
    return(i);
}


/****************************************************************************/
/* Display bottom line of screen in inverse                                 */
/****************************************************************************/
void bottomline(int line)
{
	int i=4;

	uprintf(i,api->scrn_len+1,bclr|(cclr<<4),"F1 ");
	i+=3;
	uprintf(i,api->scrn_len+1,BLACK|(cclr<<4),"Help  ");
	i+=6;
	if(line&BL_GET) {
		uprintf(i,api->scrn_len+1,bclr|(cclr<<4),"F5 ");
		i+=3;
		uprintf(i,api->scrn_len+1,BLACK|(cclr<<4),"Copy Item  ");
		i+=11; 
	}
	if(line&BL_PUT) {
		uprintf(i,api->scrn_len+1,bclr|(cclr<<4),"F6 ");
		i+=3;
		uprintf(i,api->scrn_len+1,BLACK|(cclr<<4),"Paste  ");
		i+=7; 
	}
	if(line&BL_INS) {
		uprintf(i,api->scrn_len+1,bclr|(cclr<<4),"INS ");
		i+=4;
		uprintf(i,api->scrn_len+1,BLACK|(cclr<<4),"Add Item  ");
		i+=10; 
	}
	if(line&BL_DEL) {
		uprintf(i,api->scrn_len+1,bclr|(cclr<<4),"DEL ");
		i+=4;
		uprintf(i,api->scrn_len+1,BLACK|(cclr<<4),"Delete Item  ");
		i+=13; 
	}
	uprintf(i,api->scrn_len+1,bclr|(cclr<<4),"ESC ");	/* Backspace is no good no way to abort editing */
	i+=4;
	uprintf(i,api->scrn_len+1,BLACK|(cclr<<4),"Exit");
	i+=4;
	gotoxy(i,api->scrn_len+1);
	textattr(BLACK|(cclr<<4));
	clreol();
}


/*****************************************************************************/
/* Generates a 24 character ASCII string that represents the time_t pointer  */
/* Used as a replacement for ctime()                                         */
/*****************************************************************************/
char *utimestr(time_t *intime)
{
	static char str[25];
	char wday[4],mon[4],mer[3],hour;
	struct tm *gm;

	gm=localtime(intime);
	switch(gm->tm_wday) {
		case 0:
			strcpy(wday,"Sun");
			break;
		case 1:
			strcpy(wday,"Mon");
			break;
		case 2:
			strcpy(wday,"Tue");
			break;
		case 3:
			strcpy(wday,"Wed");
			break;
		case 4:
			strcpy(wday,"Thu");
			break;
		case 5:
			strcpy(wday,"Fri");
			break;
		case 6:
			strcpy(wday,"Sat");
			break; 
	}
	switch(gm->tm_mon) {
		case 0:
			strcpy(mon,"Jan");
			break;
		case 1:
			strcpy(mon,"Feb");
			break;
		case 2:
			strcpy(mon,"Mar");
			break;
		case 3:
			strcpy(mon,"Apr");
			break;
		case 4:
			strcpy(mon,"May");
			break;
		case 5:
			strcpy(mon,"Jun");
			break;
		case 6:
			strcpy(mon,"Jul");
			break;
		case 7:
			strcpy(mon,"Aug");
			break;
		case 8:
			strcpy(mon,"Sep");
			break;
		case 9:
			strcpy(mon,"Oct");
			break;
		case 10:
			strcpy(mon,"Nov");
			break;
		case 11:
			strcpy(mon,"Dec");
			break; 
	}
	if(gm->tm_hour>12) {
		strcpy(mer,"pm");
		hour=gm->tm_hour-12; 
	}
	else {
		if(!gm->tm_hour)
			hour=12;
		else
			hour=gm->tm_hour;
		strcpy(mer,"am"); 
	}
	sprintf(str,"%s %s %02d %4d %02d:%02d %s",wday,mon,gm->tm_mday,1900+gm->tm_year
		,hour,gm->tm_min,mer);
	return(str);
}

/****************************************************************************/
/* Status popup/down function, see uifc.h for details.						*/
/****************************************************************************/
void upop(char *str)
{
	static char sav[26*3*2];
	char buf[26*3*2];
	int i,j,k;

	if(!str) {
		puttext(28,12,53,14,sav);
		return;
	}
	gettext(28,12,53,14,sav);
	memset(buf,SP,25*3*2);
	for(i=1;i<26*3*2;i+=2)
		buf[i]=(hclr|(bclr<<4));
	buf[0]='�';
	for(i=2;i<25*2;i+=2)
		buf[i]='�';
	buf[i]='�'; i+=2;
	buf[i]='�'; i+=2;
	i+=2;
	k=strlen(str);
	i+=(((23-k)/2)*2);
	for(j=0;j<k;j++,i+=2) {
		buf[i]=str[j];
		buf[i+1]|=BLINK;
	}
	i=((25*2)+1)*2;
	buf[i]='�'; i+=2;
	buf[i]='�'; i+=2;
	for(;i<((26*3)-1)*2;i+=2)
		buf[i]='�';
	buf[i]='�';

	puttext(28,12,53,14,buf);
}

/****************************************************************************/
/* Sets the current help index by source code file and line number.			*/
/****************************************************************************/
void sethelp(int line, char* file)
{
    helpline=line;
    helpfile=file;
}

/****************************************************************************/
/* Shows a scrollable text buffer - optionally parsing "help markup codes"	*/
/****************************************************************************/
void showbuf(int mode, int left, int top, int width, int height, char *title, char *hbuf, int *curp, int *barp)
{
	char *savscrn,inverse=0,high=0;
	char *buf;
	char *textbuf;
    char *p;
	int i,j,k,len;
	int	 lines;
	int pad=1;
	int	is_redraw=0;

	_setcursortype(_NOCURSOR);

	if(top+height>api->scrn_len-3)
		height=(api->scrn_len-3)-top;
	if(!width || width<strlen(title)+6)
		width=strlen(title)+6;
	if(width>api->scrn_width)
		width=api->scrn_width;
	if(mode&WIN_L2R)
		left=(api->scrn_width-width+1)/2;
	else if(mode&WIN_RHT)
		left=api->scrn_width-(width+4+left);
	if(mode&WIN_T2B)
		top=(api->scrn_len-height+1)/2;
	else if(mode&WIN_BOT)
		top=api->scrn_len-height-3-top;

	if(mode&WIN_PACK)
		pad=0;

	/* Dynamic Menus */
	if(mode&WIN_DYN
			&& curp != NULL
			&& barp != NULL
			&& last_menu_cur==curp
			&& last_menu_bar==barp
			&& save_menu_cur==*curp
			&& save_menu_bar==*barp)
		is_redraw=1;

	if((savscrn=(char *)MALLOC(api->scrn_width*api->scrn_len*2))==NULL) {
		cprintf("UIFC line %d: error allocating %u bytes\r\n"
			,__LINE__,api->scrn_width*25*2);
		_setcursortype(cursor);
		return; 
	}
	if((buf=(char *)MALLOC(width*height*2))==NULL) {
		cprintf("UIFC line %d: error allocating %u bytes\r\n"
			,__LINE__,width*21*2);
		FREE(savscrn);
		_setcursortype(cursor);
		return;
	}
	gettext(1,1,api->scrn_width,api->scrn_len,savscrn);

	if(!is_redraw) {
		memset(buf,SP,width*height*2);
		for(i=1;i<width*height*2;i+=2)
			buf[i]=(hclr|(bclr<<4));
	    buf[0]='�';
		j=strlen(title);
		if(j>width-6)
			*(title+width-6)=0;
		for(i=2;i<((width-6)/2-(j/2))*2;i+=2)
   		      buf[i]='�';
	    buf[i]='�'; i+=4;
		for(p=title;*p;p++) {
			buf[i]=*p;
			i+=2;
		}
		i+=2;
   		buf[i]='�'; i+=2;
		for(j=i;j<((width-1)*2);j+=2)
   		    buf[j]='�';
		i=j;
    	buf[i]='�'; i+=2;
		j=i;	/* leave i alone */
		for(k=0;k<(height-2);k++) { 		/* the sides of the box */
	        buf[j]='�'; j+=2;
			j+=((width-2)*2);
        	buf[j]='�'; j+=2; 
		}
	    buf[j]='�'; j+=2;
		if(!(mode&WIN_DYN) && (width>31)) {
			for(k=j;k<j+(((width-4)/2-13)*2);k+=2)
				buf[k]='�';
			buf[k]='�'; k+=4;
			buf[k]='H'; k+=2;
			buf[k]='i'; k+=2;
			buf[k]='t'; k+=4;
			buf[k]='a'; k+=2;
			buf[k]='n'; k+=2;
			buf[k]='y'; k+=4;
			buf[k]='k'; k+=2;
			buf[k]='e'; k+=2;
			buf[k]='y'; k+=4;
			buf[k]='t'; k+=2;
			buf[k]='o'; k+=4;
			buf[k]='c'; k+=2;
			buf[k]='o'; k+=2;
			buf[k]='n'; k+=2;
			buf[k]='t'; k+=2;
			buf[k]='i'; k+=2;
			buf[k]='n'; k+=2;
			buf[k]='u'; k+=2;
			buf[k]='e'; k+=4;
	    	buf[k]='�'; k+=2;
			for(j=k;j<k+(((width-4)/2-12)*2);j+=2)
		        buf[j]='�';
		}	
		else {
			for(k=j;k<j+((width-2)*2);k+=2)
				buf[k]='�';
			j=k;
		}
	    buf[j]='�';
		puttext(left,top+1,left+width-1,top+height,buf);
	}
	len=strlen(hbuf);

	lines=1;		/* The first one is free */
	k=0;
	for(j=0;j<len;j++) {
		k++;
		if(hbuf[j]==LF)
			lines++;
		if(k>72) {
			k=0;
			lines++;
		}
	}
	if(lines < height-2-pad-pad)
		lines=height-2-pad-pad;

	if((buf=(char *)textbuf=MALLOC((width-2-pad-pad)*lines*2))==NULL) {
		cprintf("UIFC line %d: error allocating %u bytes\r\n"
			,__LINE__,(width-2-pad-pad)*lines*2);
		FREE(savscrn);
		FREE(buf);
		_setcursortype(cursor);
		return; 
	}
	memset(textbuf,SP,(width-2-pad-pad)*lines*2);
	for(i=1;i<(width-2-pad-pad)*lines*2;i+=2)
		buf[i]=(hclr|(bclr<<4));
	i=0;
	for(j=0;j<len;j++,i+=2) {
		if(hbuf[j]==LF) {
			i+=2;
			while(i%((width-2-pad-pad)*2)) i++; i-=2; 
		}
		else if(mode&WIN_HLP && (hbuf[j]==2 || hbuf[j]=='~')) {		 /* Ctrl-b toggles inverse */
			inverse=!inverse;
			i-=2; 
		}
		else if(mode&WIN_HLP && (hbuf[j]==1 || hbuf[j]=='`')) {		 /* Ctrl-a toggles high intensity */
			high=!high;
			i-=2; 
		}
		else if(hbuf[j]!=CR) {
			textbuf[i]=hbuf[j];
			textbuf[i+1]=inverse ? (bclr|(cclr<<4))
				: high ? (hclr|(bclr<<4)) : (lclr|(bclr<<4)); 
		} 
	}
	i=0;
	p=textbuf;
	if(mode&WIN_DYN) {
		puttext(left+1+pad,top+2+pad,left+width-2-pad,top+height-1-pad,p);
	}
	else {
		while(i==0) {
			puttext(left+1+pad,top+2+pad,left+width-2-pad,top+height-1-pad,p);
			if(inkey(1)) {
				switch(inkey(0)) {
					case KEY_HOME:	/* home */
						p=textbuf;
						break;

					case KEY_UP:	/* up arrow */
						p = p-((width-4)*2);
						if(p<textbuf)
							p=textbuf;
						break;
					
					case KEY_PPAGE:	/* PgUp */
						p = p-((width-4)*2*(height-5));
						if(p<textbuf)
							p=textbuf;
						break;

					case KEY_NPAGE:	/* PgDn */
						p=p+(width-4)*2*(height-5);
						if(p > textbuf+(lines-height+1)*(width-4)*2)
							p=textbuf+(lines-height+1)*(width-4)*2;
						if(p<textbuf)
							p=textbuf;
						break;

					case KEY_END:	/* end */
						p=textbuf+(lines-height+1)*(width-4)*2;
						if(p<textbuf)
							p=textbuf;
						break;

					case KEY_DOWN:	/* dn arrow */
						p = p+((width-4)*2);
						if(p > textbuf+(lines-height+1)*(width-4)*2)
							p=textbuf+(lines-height+1)*(width-4)*2;
						if(p<textbuf)
							p=textbuf;
						break;

					default:
						i=1;
				}
			}
			mswait(1);
		}

		puttext(1,1,api->scrn_width,api->scrn_len,savscrn);
	}
	if(is_redraw)			/* Force redraw of menu also. */
		reset_dynamic();
	FREE(savscrn);
	FREE(buf);
	_setcursortype(cursor);
}

/************************************************************/
/* Help (F1) key function. Uses helpbuf as the help input.	*/
/************************************************************/
static void help(void)
{
	char hbuf[HELPBUF_SIZE],str[256];
    char *p;
	unsigned int line;
	long l;
	FILE *fp;

	_setcursortype(_NOCURSOR);

	if(!api->helpbuf) {
		if((fp=fopen(api->helpixbfile,"rb"))==NULL) {
			sprintf(hbuf," ERROR  Cannot open help index:\r\n          %s"
				,api->helpixbfile);
		}
		else {
			p=strrchr(helpfile,'/');
			if(p==NULL)
				p=strrchr(helpfile,'\\');
			if(p==NULL)
				p=helpfile;
			else
				p++;
			l=-1L;
			while(!feof(fp)) {
				if(!fread(str,12,1,fp))
					break;
				str[12]=0;
				fread(&line,2,1,fp);
				if(stricmp(str,p) || line!=helpline) {
					fseek(fp,4,SEEK_CUR);
					continue; 
				}
				fread(&l,4,1,fp);
				break;
			}
			fclose(fp);
			if(l==-1L)
				sprintf(hbuf," ERROR  Cannot locate help key (%s:%u) in:\r\n"
					"         %s",p,helpline,api->helpixbfile);
			else {
				if((fp=fopen(api->helpdatfile,"rb"))==NULL)
					sprintf(hbuf," ERROR  Cannot open help file:\r\n          %s"
						,api->helpdatfile);
				else {
					fseek(fp,l,SEEK_SET);
					fread(hbuf,HELPBUF_SIZE,1,fp);
					fclose(fp); 
				} 
			}
		}
	}
	else
		strcpy(hbuf,api->helpbuf);

	showbuf(WIN_MID|WIN_HLP, 0, 0, 76, api->scrn_len, "Online Help", hbuf, NULL, NULL);
}
