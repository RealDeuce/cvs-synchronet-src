/* uifcc.c */

/* Curses implementation of UIFC (user interface) library based on uifc.c */

/****************************************************************************
 * @format.tab-size 4		(Plain Text/Source Code File Header)			*
 * @format.use-tabs true	(see http://www.synchro.net/ptsc_hdr.html)		*
 *																			*
 * Copyright 2002 Rob Swindell - http://www.synchro.net/copyright.html		*
 *																			*
 * This program is free software; you can redistribute it and/or			*
 * modify it under the terms of the GNU General Public License				*
 * as published by the Free Software Foundation; either version 2			*
 * of the License, or (at your option) any later version.					*
 * See the GNU General Public License for more details: gpl.txt or			*
 * http://www.fsf.org/copyleft/gpl.html										*
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

#include "uifc.h"
#include <curses.h>
#include <unistd.h>
#include <sys/time.h>

#if defined(__OS2__)

#define INCL_BASE
#include <os2.h>

void mswait(int msec)
{
DosSleep(msec ? msec : 1);
}

#elif defined(_WIN32)
	#include <windows.h>
	#define mswait(x) Sleep(x)
#elif defined(__FLAT__)
    #define mswait(x) delay(x)
#endif

							/* Bottom line elements */
#define BL_INS      (1<<0)  /* INS key */
#define BL_DEL      (1<<1)  /* DEL key */
#define BL_GET      (1<<2)  /* Get key */
#define BL_PUT      (1<<3)  /* Put key */

#define BLACK	0
#define BLUE	1
#define GREEN	2
#define	CYAN	3
#define	RED		4
#define MAGENTA	5
#define BROWN	6
#define	LIGHTGRAY	7
#define DARKGRAY	8
#define LIGHTBLUE	9
#define LIGHTGREEN	10
#define	LIGHTCYAN	11
#define	LIGHTRED		12
#define LIGHTMAGENTA	13
#define YELLOW	14
#define	WHITE	15
#define BLINK	128
#define SH_DENYWR	1
#define SH_DENYRW	2
#define O_BINARY	0

static char hclr,lclr,bclr,cclr,show_free_mem=0;
static char* helpfile=0;
static uint helpline=0;
static char blk_scrn[MAX_BFLN];
static win_t sav[MAX_BUFS];
static uint max_opts=MAX_OPTS;
static uifcapi_t* api;
static int lastattr=0;

/* Prototypes */
static int  uprintf(int x, int y, unsigned char attr, char *fmt,...);
static void bottomline(int line);
static char *utimestr(time_t *intime);
static void help();
static int puttext(int sx, int sy, int ex, int ey, unsigned char *fill);
static int gettext(int sx, int sy, int ex, int ey, unsigned char *fill);
static short curses_color(short color);
static void textattr(unsigned char attr);
static int kbhit(void);
static void delay(int msec);
static int ugetstr(char *outstr, int max, long mode);
static int wherey(void);
static int wherex(void);
static FILE * _fsopen(char *pathname, char *mode, int flags);
static int cprintf(char *fmat, ...);
static void putch(unsigned char ch);
static void cputs(char *str);
static void gotoxy(int x, int y);

/* API routines */
static void uifcbail(void);
static int uscrn(char *str);
static int ulist(int mode, int left, int top, int width, int *dflt, int *bar
	,char *title, char **option);
static int uinput(int imode, int left, int top, char *prompt, char *str
	,int len ,int kmode);
static void umsg(char *str);
static void upop(char *str);
static void sethelp(int line, char* file);

int inkey(int mode)
{
	if(mode)
		return(kbhit());
	return(getch());
}

/****************************************************************************/
/* Initialization function, see uifc.h for details.							*/
/* Returns 0 on success.													*/
/****************************************************************************/
int uifcinic(uifcapi_t* uifcapi)
{
	int 	i;
	int		height, width;
	short	fg, bg, pair=0;
#ifndef __FLAT__
	union	REGS r;
#endif

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

	initscr();
	start_color();
	cbreak();
	noecho();
	nonl();
//	intrflush(stdscr, FALSE);
	keypad(stdscr, TRUE);
	scrollok(stdscr,FALSE);

	// Set up color pairs
	for(bg=0;bg<8;bg++)  {
		for(fg=0;fg<8;fg++) {
			init_pair(++pair,curses_color(fg),curses_color(bg));
		}
	}
//	resizeterm(25,80);		/* set mode to 80x25 if possible */
    clear();
	getmaxyx(stdscr,height,width);
    /* unsupported mode? */
    if(height<MIN_LINES
        || height>MAX_LINES
        || width<80) {
	}
	
    api->scrn_len=height;
    if(api->scrn_len<MIN_LINES || api->scrn_len>MAX_LINES) {
        cprintf("\7UIFC: Screen length (%u) must be between %d and %d lines\r\n"
            ,api->scrn_len,MIN_LINES,MAX_LINES);
        return(-2);
    }
    api->scrn_len--; /* account for status line */

    if(width<80) {
        cprintf("\7UIFC: Screen width (%u) must be at least 80 characters\r\n"
            ,width);
        return(-3);
    }


/* ToDo - Set up mousemask() here */

    if(!has_colors()) {
        bclr=BLACK;
        hclr=WHITE;
        lclr=LIGHTGRAY;
        cclr=LIGHTGRAY;
    } else {
        bclr=BLUE;
        hclr=YELLOW;
        lclr=WHITE;
        cclr=CYAN;
    }
    for(i=0;i<MAX_BFLN;i+=2) {
        blk_scrn[i]='�';
        blk_scrn[i+1]=cclr|(bclr<<4);
    }

	curs_set(0);
	refresh();

    return(0);
}

short curses_color(short color)
{
	switch(color)
	{
		case 0 :
			return(COLOR_BLACK);
		case 1 :
			return(COLOR_BLUE);
		case 2 :
			return(COLOR_GREEN);
		case 3 :
			return(COLOR_CYAN);
		case 4 :
			return(COLOR_RED);
		case 5 :
			return(COLOR_MAGENTA);
		case 6 :
			return(COLOR_YELLOW);
		case 7 :
			return(COLOR_WHITE);
		case 8 :
			return(COLOR_BLACK);
		case 9 :
			return(COLOR_BLUE);
		case 10 :
			return(COLOR_GREEN);
		case 11 :
			return(COLOR_CYAN);
		case 12 :
			return(COLOR_RED);
		case 13 :
			return(COLOR_MAGENTA);
		case 14 :
			return(COLOR_YELLOW);
		case 15 :
			return(COLOR_WHITE);
	}
	return(0);
}

static void hidemouse(void)
{
/* ToDo - Mouse stuff here */
}

static void showmouse(void)
{
/* ToDo - Mouse stuff here */
}


void uifcbail(void)
{
curs_set(1);
clear();
refresh();
}

/****************************************************************************/
/* Clear screen, fill with background attribute, display application title.	*/
/* Returns 0 on success.													*/
/****************************************************************************/
int uscrn(char *str)
{
    textattr(bclr|(cclr<<4));
    gotoxy(1,1);
    clrtoeol();
    gotoxy(3,1);
	cputs(str);
    if(!puttext(1,2,80,api->scrn_len,blk_scrn))
        return(-1);
    gotoxy(1,api->scrn_len+1);
    clrtoeol();
	refresh();
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
	uprintf(55,1,bclr|(cclr<<4),utimestr(&now));
	savetime=now; }
}

/****************************************************************************/
/* Truncates white-space chars off end of 'str'								*/
/****************************************************************************/
static void truncsp(char *str)
{
	uint c;

	c=strlen(str);
	while(c && (uchar)str[c-1]<=SP) c--;
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

#ifndef __FLAT__
/* ToDo Mouse stuff */
#endif

if(mode&WIN_SAV && api->savnum>=MAX_BUFS-1)
	putch(7);
i=0;
if(mode&WIN_INS) bline|=BL_INS;
if(mode&WIN_DEL) bline|=BL_DEL;
if(mode&WIN_GET) bline|=BL_GET;
if(mode&WIN_PUT) bline|=BL_PUT;
bottomline(bline);
while(opts<max_opts && opts<MAX_OPTS)
	if(option[opts][0]==0)
		break;
	else opts++;
if(mode&WIN_XTR && opts<max_opts && opts<MAX_OPTS)
	option[opts++][0]=0;
height=opts+4;
if(top+height>api->scrn_len-3)
	height=(api->scrn_len-3)-top;
if(!width || width<strlen(title)+6) {
	width=strlen(title)+6;
	for(i=0;i<opts;i++) {
		truncsp(option[i]);
		if((j=strlen(option[i])+5)>width)
			width=j; } }
if(width>(SCRN_RIGHT+1)-SCRN_LEFT)
	width=(SCRN_RIGHT+1)-SCRN_LEFT;
if(mode&WIN_L2R)
	left=36-(width/2);
else if(mode&WIN_RHT)
	left=SCRN_RIGHT-(width+4+left);
if(mode&WIN_T2B)
	top=(api->scrn_len/2)-(height/2)-2;
else if(mode&WIN_BOT)
	top=api->scrn_len-height-3-top;
if(mode&WIN_SAV && api->savdepth==api->savnum) {
	if((sav[api->savnum].buf=(char *)MALLOC((width+3)*(height+2)*2))==NULL) {
		cprintf("UIFC line %d: error allocating %u bytes."
            ,__LINE__,(width+3)*(height+2)*2);
		return(-1); }
	gettext(SCRN_LEFT+left,SCRN_TOP+top,SCRN_LEFT+left+width+1
		,SCRN_TOP+top+height,sav[api->savnum].buf);
	sav[api->savnum].left=SCRN_LEFT+left;
	sav[api->savnum].top=SCRN_TOP+top;
	sav[api->savnum].right=SCRN_LEFT+left+width+1;
	sav[api->savnum].bot=SCRN_TOP+top+height;
	api->savdepth++; }
else if(mode&WIN_SAV
	&& (sav[api->savnum].left!=SCRN_LEFT+left
	|| sav[api->savnum].top!=SCRN_TOP+top
	|| sav[api->savnum].right!=SCRN_LEFT+left+width+1
	|| sav[api->savnum].bot!=SCRN_TOP+top+height)) { /* dimensions have changed */
	puttext(sav[api->savnum].left,sav[api->savnum].top,sav[api->savnum].right,sav[api->savnum].bot
		,sav[api->savnum].buf);	/* put original window back */
	FREE(sav[api->savnum].buf);
	if((sav[api->savnum].buf=(char *)MALLOC((width+3)*(height+2)*2))==NULL) {
		cprintf("UIFC line %d: error allocating %u bytes."
            ,__LINE__,(width+3)*(height+2)*2);
		return(-1); }
	gettext(SCRN_LEFT+left,SCRN_TOP+top,SCRN_LEFT+left+width+1
		,SCRN_TOP+top+height,sav[api->savnum].buf);	  /* save again */
	sav[api->savnum].left=SCRN_LEFT+left;
	sav[api->savnum].top=SCRN_TOP+top;
	sav[api->savnum].right=SCRN_LEFT+left+width+1;
	sav[api->savnum].bot=SCRN_TOP+top+height; }


#ifndef __FLAT__
if(show_free_mem) {
/* ToDo Show free memory */
//	uprintf(58,1,bclr|(cclr<<4),"%10u bytes free",coreleft());
	}
#endif


if(mode&WIN_ORG) { /* Clear around menu */
	if(top)
		puttext(SCRN_LEFT,SCRN_TOP,SCRN_RIGHT+2,SCRN_TOP+top-1,blk_scrn);
	if(SCRN_TOP+height+top<=api->scrn_len)
		puttext(SCRN_LEFT,SCRN_TOP+height+top,SCRN_RIGHT+2,api->scrn_len,blk_scrn);
	if(left)
		puttext(SCRN_LEFT,SCRN_TOP+top,SCRN_LEFT+left-1,SCRN_TOP+height+top
			,blk_scrn);
	if(SCRN_LEFT+left+width<=SCRN_RIGHT)
		puttext(SCRN_LEFT+left+width,SCRN_TOP+top,SCRN_RIGHT+2
			,SCRN_TOP+height+top,blk_scrn); }
ptr=win;
*(ptr++)='�';
*(ptr++)=hclr|(bclr<<4);

if(api->mode&UIFC_MOUSE) {
	/* ToDo Mouse stuff */
	i=0;
}
else
	i=0;
for(;i<width-2;i++) {
	*(ptr++)='�';
	*(ptr++)=hclr|(bclr<<4); }
*(ptr++)='�';
*(ptr++)=hclr|(bclr<<4);
*(ptr++)='�';
*(ptr++)=hclr|(bclr<<4);
a=strlen(title);
b=(width-a-1)/2;
for(i=0;i<b;i++) {
	*(ptr++)=' ';
	*(ptr++)=hclr|(bclr<<4); }
for(i=0;i<a;i++) {
	*(ptr++)=title[i];
	*(ptr++)=hclr|(bclr<<4); }
for(i=0;i<width-(a+b)-2;i++) {
	*(ptr++)=' ';
	*(ptr++)=hclr|(bclr<<4); }
*(ptr++)='�';
*(ptr++)=hclr|(bclr<<4);
*(ptr++)='�';
*(ptr++)=hclr|(bclr<<4);
for(i=0;i<width-2;i++) {
	*(ptr++)='�';
	*(ptr++)=hclr|(bclr<<4); }
*(ptr++)='�';
*(ptr++)=hclr|(bclr<<4);

if((*cur)>=opts)
	(*cur)=opts-1;			/* returned after scrolled */

if(!bar) {
	if((*cur)>height-5)
		(*cur)=height-5;
	i=0; }
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
//
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
		a=bclr|(LIGHTGRAY<<4);
	else
		a=lclr|(bclr<<4);
	b=strlen(option[i]);
	if(b>longopt)
		longopt=b;
	if(b+4>width)
		b=width-4;
	for(c=0;c<b;c++) {
		*(ptr++)=option[i][c];
		*(ptr++)=a; }
	while(c<width-4) {
		*(ptr++)=' ';
		*(ptr++)=a;
		c++; }
	*(ptr++)='�';
	*(ptr++)=hclr|(bclr<<4);
	i++;
	j++; }
*(ptr++)='�';
*(ptr++)=hclr|(bclr<<4);
for(i=0;i<width-2;i++) {
	*(ptr++)='�';
	*(ptr++)=hclr|(bclr<<4); }
*(ptr++)='�';
*(ptr++)=hclr|(bclr<<4);
puttext(SCRN_LEFT+left,SCRN_TOP+top,SCRN_LEFT+left+width-1
	,SCRN_TOP+top+height-1,win);
if(bar)
	y=top+3+(*bar);
else
	y=top+3+(*cur);
if(opts+4>height && ((!bar && (*cur)!=opts-1)
	|| (bar && ((*cur)-(*bar))+(height-4)<opts))) {
	gotoxy(SCRN_LEFT+left+1,SCRN_TOP+top+height-2);
	textattr(lclr|(bclr<<4));
	putch(31);	   /* put down arrow */
	textattr(hclr|(bclr<<4)); }

if(bar && (*bar)!=(*cur)) {
	gotoxy(SCRN_LEFT+left+1,SCRN_TOP+top+3);
	textattr(lclr|(bclr<<4));
	putch(30);	   /* put the up arrow */
	textattr(hclr|(bclr<<4)); }

if(bclr==BLUE) {
	gettext(SCRN_LEFT+left+width,SCRN_TOP+top+1,SCRN_LEFT+left+width+1
		,SCRN_TOP+top+height-1,shade);
	for(i=1;i<height*4;i+=2)
		shade[i]=DARKGRAY;
	puttext(SCRN_LEFT+left+width,SCRN_TOP+top+1,SCRN_LEFT+left+width+1
		,SCRN_TOP+top+height-1,shade);
	gettext(SCRN_LEFT+left+2,SCRN_TOP+top+height,SCRN_LEFT+left+width+1
		,SCRN_TOP+top+height,shade);
	for(i=1;i<width*2;i+=2)
		shade[i]=DARKGRAY;
	puttext(SCRN_LEFT+left+2,SCRN_TOP+top+height,SCRN_LEFT+left+width+1
		,SCRN_TOP+top+height,shade); }
showmouse();
while(1) {
#if 0					/* debug */
	gotoxy(30,1);
	cprintf("y=%2d h=%2d c=%2d b=%2d s=%2d o=%2d"
		,y,height,*cur,bar ? *bar :0xff,api->savdepth,opts);
#endif
	if(!show_free_mem)
		timedisplay();
#ifndef __FLAT__
	if(api->mode&UIFC_MOUSE) {
	/* ToDo Serious mouse stuff here */
	}
#endif

	if(inkey(1)) {
		i=inkey(0);
		if(i>255) {
			s=0;
			switch(i) {
				/* ToDo extended keys */
				case KEY_HOME:	/* home */
					if(!opts)
						break;
					if(opts+4>height) {
						hidemouse();
						gotoxy(SCRN_LEFT+left+1,SCRN_TOP+top+3);
						textattr(lclr|(bclr<<4));
						putch(' ');    /* Delete the up arrow */
						gotoxy(SCRN_LEFT+left+1,SCRN_TOP+top+height-2);
						putch(31);	   /* put the down arrow */
						uprintf(SCRN_LEFT+left+3,SCRN_TOP+top+3
							,bclr|(LIGHTGRAY<<4)
							,"%-*.*s",width-4,width-4,option[0]);
						for(i=1;i<height-4;i++)    /* re-display options */
							uprintf(SCRN_LEFT+left+3,SCRN_TOP+top+3+i
								,lclr|(bclr<<4)
								,"%-*.*s",width-4,width-4,option[i]);
						(*cur)=0;
						if(bar)
							(*bar)=0;
						y=top+3;
						showmouse();
						break; }
					hidemouse();
					gettext(SCRN_LEFT+3+left,SCRN_TOP+y
						,SCRN_LEFT+left+width-2,SCRN_TOP+y,line);
					for(i=1;i<width*2;i+=2)
						line[i]=lclr|(bclr<<4);
					puttext(SCRN_LEFT+3+left,SCRN_TOP+y
						,SCRN_LEFT+left+width-2,SCRN_TOP+y,line);
					(*cur)=0;
					if(bar)
						(*bar)=0;
					y=top+3;
					gettext(SCRN_LEFT+3+left,SCRN_TOP+y
						,SCRN_LEFT+left+width-2,SCRN_TOP+y,line);
					for(i=1;i<width*2;i+=2)
						line[i]=bclr|(LIGHTGRAY<<4);
					puttext(SCRN_LEFT+3+left,SCRN_TOP+y
						,SCRN_LEFT+left+width-2,SCRN_TOP+y,line);
					showmouse();
					break;
				case KEY_UP:	/* up arrow */
					if(!opts)
						break;
					if(!(*cur) && opts+4>height) {
						hidemouse();
						gotoxy(SCRN_LEFT+left+1,SCRN_TOP+top+3); /* like end */
						textattr(lclr|(bclr<<4));
						putch(30);	   /* put the up arrow */
						gotoxy(SCRN_LEFT+left+1,SCRN_TOP+top+height-2);
						putch(' ');    /* delete the down arrow */
						for(i=(opts+4)-height,j=0;i<opts;i++,j++)
							uprintf(SCRN_LEFT+left+3,SCRN_TOP+top+3+j
								,i==opts-1 ? bclr|(LIGHTGRAY<<4)
									: lclr|(bclr<<4)
								,"%-*.*s",width-4,width-4,option[i]);
						(*cur)=opts-1;
						if(bar)
							(*bar)=height-5;
						y=top+height-2;
						showmouse();
                        break; }
					hidemouse();
					gettext(SCRN_LEFT+3+left,SCRN_TOP+y
						,SCRN_LEFT+left+width-2,SCRN_TOP+y,line);
					for(i=1;i<width*2;i+=2)
						line[i]=lclr|(bclr<<4);
					puttext(SCRN_LEFT+3+left,SCRN_TOP+y
						,SCRN_LEFT+left+width-2,SCRN_TOP+y,line);
					showmouse();
					if(!(*cur)) {
						y=top+height-2;
						(*cur)=opts-1;
						if(bar)
							(*bar)=height-5; }
					else {
						(*cur)--;
						y--;
						if(bar && *bar)
							(*bar)--; }
					if(y<top+3) {	/* scroll */
						hidemouse();
						if(!(*cur)) {
							gotoxy(SCRN_LEFT+left+1,SCRN_TOP+top+3);
							textattr(lclr|(bclr<<4));
							putch(' '); }  /* delete the up arrow */
						if((*cur)+height-4==opts-1) {
							gotoxy(SCRN_LEFT+left+1,SCRN_TOP+top+height-2);
							textattr(lclr|(bclr<<4));
							putch(31); }   /* put the dn arrow */
						y++;
						scroll_text(SCRN_LEFT+left+2,SCRN_TOP+top+3
							,SCRN_LEFT+left+width-3,SCRN_TOP+top+height-2,1);
						uprintf(SCRN_LEFT+left+3,SCRN_TOP+top+3
							,bclr|(LIGHTGRAY<<4)
							,"%-*.*s",width-4,width-4,option[*cur]);
						showmouse(); }
					else {
						hidemouse();
						gettext(SCRN_LEFT+3+left,SCRN_TOP+y
							,SCRN_LEFT+left+width-2,SCRN_TOP+y,line);
						for(i=1;i<width*2;i+=2)
							line[i]=bclr|(LIGHTGRAY<<4);
						puttext(SCRN_LEFT+3+left,SCRN_TOP+y
							,SCRN_LEFT+left+width-2,SCRN_TOP+y,line);
						showmouse(); }
					break;
#if 0
				case KEY_PPAGE;	/* PgUp */
				case KEY_NPAGE;	/* PgDn */
					if(!opts || (*cur)==(opts-1))
						break;
					(*cur)+=(height-4);
					if((*cur)>(opts-1))
						(*cur)=(opts-1);

					hidemouse();
					gettext(SCRN_LEFT+3+left,SCRN_TOP+y
						,SCRN_LEFT+left+width-2,SCRN_TOP+y,line);
					for(i=1;i<width*2;i+=2)
						line[i]=lclr|(bclr<<4);
					puttext(SCRN_LEFT+3+left,SCRN_TOP+y
						,SCRN_LEFT+left+width-2,SCRN_TOP+y,line);

					for(i=(opts+4)-height,j=0;i<opts;i++,j++)
						uprintf(SCRN_LEFT+left+3,SCRN_TOP+top+3+j
							,i==(*cur) bclr|(LIGHTGRAY<<4) : lclr|(bclr<<4)
							,"%-*.*s",width-4,width-4,option[i]);
					y=top+height-2;
					if(bar)
						(*bar)=height-5;
					gettext(SCRN_LEFT+3+left,SCRN_TOP+y
						,SCRN_LEFT+left+width-2,SCRN_TOP+y,line);
					for(i=1;i<148;i+=2)
						line[i]=bclr|(LIGHTGRAY<<4);
					puttext(SCRN_LEFT+3+left,SCRN_TOP+y
						,SCRN_LEFT+left+width-2,SCRN_TOP+y,line);
					showmouse();
                    break;
#endif
				case KEY_END:	/* end */
					if(!opts)
						break;
					if(opts+4>height) {	/* Scroll mode */
						hidemouse();
						gotoxy(SCRN_LEFT+left+1,SCRN_TOP+top+3);
						textattr(lclr|(bclr<<4));
						putch(30);	   /* put the up arrow */
						gotoxy(SCRN_LEFT+left+1,SCRN_TOP+top+height-2);
						putch(' ');    /* delete the down arrow */
						for(i=(opts+4)-height,j=0;i<opts;i++,j++)
							uprintf(SCRN_LEFT+left+3,SCRN_TOP+top+3+j
								,i==opts-1 ? bclr|(LIGHTGRAY<<4)
									: lclr|(bclr<<4)
								,"%-*.*s",width-4,width-4,option[i]);
						(*cur)=opts-1;
						y=top+height-2;
						if(bar)
							(*bar)=height-5;
						showmouse();
						break; }
					hidemouse();
					gettext(SCRN_LEFT+3+left,SCRN_TOP+y
						,SCRN_LEFT+left+width-2,SCRN_TOP+y,line);
					for(i=1;i<width*2;i+=2)
						line[i]=lclr|(bclr<<4);
					puttext(SCRN_LEFT+3+left,SCRN_TOP+y
						,SCRN_LEFT+left+width-2,SCRN_TOP+y,line);
					(*cur)=opts-1;
					y=top+height-2;
					if(bar)
						(*bar)=height-5;
					gettext(SCRN_LEFT+3+left,SCRN_TOP+y
						,SCRN_LEFT+left+width-2,SCRN_TOP+y,line);
					for(i=1;i<148;i+=2)
						line[i]=bclr|(LIGHTGRAY<<4);
					puttext(SCRN_LEFT+3+left,SCRN_TOP+y
						,SCRN_LEFT+left+width-2,SCRN_TOP+y,line);
					showmouse();
					break;
				case KEY_DOWN:	/* dn arrow */
					if(!opts)
						break;
					if((*cur)==opts-1 && opts+4>height) { /* like home */
						hidemouse();
						gotoxy(SCRN_LEFT+left+1,SCRN_TOP+top+3);
						textattr(lclr|(bclr<<4));
						putch(' ');    /* Delete the up arrow */
						gotoxy(SCRN_LEFT+left+1,SCRN_TOP+top+height-2);
						putch(31);	   /* put the down arrow */
						uprintf(SCRN_LEFT+left+3,SCRN_TOP+top+3
							,bclr|(LIGHTGRAY<<4)
							,"%-*.*s",width-4,width-4,option[0]);
						for(i=1;i<height-4;i++)    /* re-display options */
							uprintf(SCRN_LEFT+left+3,SCRN_TOP+top+3+i
								,lclr|(bclr<<4)
								,"%-*.*s",width-4,width-4,option[i]);
						(*cur)=0;
						y=top+3;
						if(bar)
							(*bar)=0;
						showmouse();
                        break; }
					hidemouse();
					gettext(SCRN_LEFT+3+left,SCRN_TOP+y
						,SCRN_LEFT+left+width-2,SCRN_TOP+y,line);
					for(i=1;i<width*2;i+=2)
						line[i]=lclr|(bclr<<4);
					puttext(SCRN_LEFT+3+left,SCRN_TOP+y
						,SCRN_LEFT+left+width-2,SCRN_TOP+y,line);
					showmouse();
					if((*cur)==opts-1) {
						(*cur)=0;
						y=top+3;
						if(bar) {
							/* gotoxy(1,1); cprintf("bar=%08lX ",bar); */
							(*bar)=0; } }
					else {
						(*cur)++;
						y++;
						if(bar && (*bar)<height-5) {
							/* gotoxy(1,1); cprintf("bar=%08lX ",bar); */
							(*bar)++; } }
					if(y==top+height-1) {	/* scroll */
						hidemouse();
						if(*cur==opts-1) {
							gotoxy(SCRN_LEFT+left+1,SCRN_TOP+top+height-2);
							textattr(lclr|(bclr<<4));
							putch(' '); }  /* delete the down arrow */
						if((*cur)+4==height) {
							gotoxy(SCRN_LEFT+left+1,SCRN_TOP+top+3);
							textattr(lclr|(bclr<<4));
							putch(30); }   /* put the up arrow */
						y--;
						/* gotoxy(1,1); cprintf("\rdebug: %4d ",__LINE__); */
						scroll_text(SCRN_LEFT+left+2,SCRN_TOP+top+3
							,SCRN_LEFT+left+width-3,SCRN_TOP+top+height-2,0);
						/* gotoxy(1,1); cprintf("\rdebug: %4d ",__LINE__); */
						uprintf(SCRN_LEFT+left+3,SCRN_TOP+top+height-2
							,bclr|(LIGHTGRAY<<4)
							,"%-*.*s",width-4,width-4,option[*cur]);
						showmouse(); }
					else {
						hidemouse();
						gettext(SCRN_LEFT+3+left,SCRN_TOP+y
							,SCRN_LEFT+left+width-2,SCRN_TOP+y
							,line);
						for(i=1;i<width*2;i+=2)
							line[i]=bclr|(LIGHTGRAY<<4);
						puttext(SCRN_LEFT+3+left,SCRN_TOP+y
							,SCRN_LEFT+left+width-2,SCRN_TOP+y
							,line);
						showmouse(); }
					break;
				case KEY_F(1):	/* F1 */
					help();
					break;
				case KEY_F(5):	/* F5 */
					if(mode&WIN_GET && !(mode&WIN_XTR && (*cur)==opts-1))
						return((*cur)|MSK_GET);
					break;
				case KEY_F(6):	/* F6 */
					if(mode&WIN_PUT && !(mode&WIN_XTR && (*cur)==opts-1))
						return((*cur)|MSK_PUT);
					break;
				case KEY_IC:	/* insert */
					if(mode&WIN_INS) {
						if(mode&WIN_INSACT) {
							hidemouse();
							gettext(SCRN_LEFT+left,SCRN_TOP+top,SCRN_LEFT
								+left+width-1,SCRN_TOP+top+height-1,win);
							for(i=1;i<(width*height*2);i+=2)
								win[i]=lclr|(cclr<<4);
							if(opts) {
								j=(((y-top)*width)*2)+7+((width-4)*2);
								for(i=(((y-top)*width)*2)+7;i<j;i+=2)
									win[i]=hclr|(cclr<<4); }
							puttext(SCRN_LEFT+left,SCRN_TOP+top,SCRN_LEFT
								+left+width-1,SCRN_TOP+top+height-1,win);
							showmouse(); }
						if(!opts)
							return(MSK_INS);
						return((*cur)|MSK_INS); }
					break;
				case KEY_DC:	/* delete */
					if(mode&WIN_XTR && (*cur)==opts-1)	/* can't delete */
						break;							/* extra line */
					if(mode&WIN_DEL) {
						if(mode&WIN_DELACT) {
							hidemouse();
							gettext(SCRN_LEFT+left,SCRN_TOP+top,SCRN_LEFT
								+left+width-1,SCRN_TOP+top+height-1,win);
							for(i=1;i<(width*height*2);i+=2)
								win[i]=lclr|(cclr<<4);
							j=(((y-top)*width)*2)+7+((width-4)*2);
							for(i=(((y-top)*width)*2)+7;i<j;i+=2)
								win[i]=hclr|(cclr<<4);
							puttext(SCRN_LEFT+left,SCRN_TOP+top,SCRN_LEFT
								+left+width-1,SCRN_TOP+top+height-1,win);
							showmouse(); }
						return((*cur)|MSK_DEL); }
					break;	} }
		else {
			i&=0xff;
			if(isalnum(i) && opts && option[0][0]) {
				search[s]=i;
				search[s+1]=0;
				for(j=(*cur)+1,a=b=0;a<2;j++) {   /* a = search count */
					if(j==opts) {					/* j = option count */
						j=-1;						/* b = letter count */
						continue; }
					if(j==(*cur)) {
						b++;
						continue; }
					if(b>=longopt) {
                        b=0;
                        a++; }
					if(a==1 && !s)
                        break;
					if(strlen(option[j])>b
						&& ((!a && s && !strncasecmp(option[j]+b,search,s+1))
						|| ((a || !s) && toupper(option[j][b])==toupper(i)))) {
						if(a) s=0;
						else s++;
						if(y+(j-(*cur))+2>height+top) {
							(*cur)=j;
							gotoxy(SCRN_LEFT+left+1,SCRN_TOP+top+3);
							textattr(lclr|(bclr<<4));
							putch(30);	   /* put the up arrow */
							if((*cur)==opts-1) {
								gotoxy(SCRN_LEFT+left+1,SCRN_TOP+top+height-2);
								putch(' '); }  /* delete the down arrow */
							for(i=((*cur)+5)-height,j=0;i<(*cur)+1;i++,j++)
								uprintf(SCRN_LEFT+left+3,SCRN_TOP+top+3+j
									,i==(*cur) ? bclr|(LIGHTGRAY<<4)
										: lclr|(bclr<<4)
									,"%-*.*s",width-4,width-4,option[i]);
							y=top+height-2;
							if(bar)
								(*bar)=height-5;
							break; }
						if(y-((*cur)-j)<top+3) {
							(*cur)=j;
							gotoxy(SCRN_LEFT+left+1,SCRN_TOP+top+3);
							textattr(lclr|(bclr<<4));
							if(!(*cur))
								putch(' ');    /* Delete the up arrow */
							gotoxy(SCRN_LEFT+left+1,SCRN_TOP+top+height-2);
							putch(31);	   /* put the down arrow */
							uprintf(SCRN_LEFT+left+3,SCRN_TOP+top+3
								,bclr|(LIGHTGRAY<<4)
								,"%-*.*s",width-4,width-4,option[(*cur)]);
							for(i=1;i<height-4;i++) 	/* re-display options */
								uprintf(SCRN_LEFT+left+3,SCRN_TOP+top+3+i
									,lclr|(bclr<<4)
									,"%-*.*s",width-4,width-4
									,option[(*cur)+i]);
							y=top+3;
							if(bar)
								(*bar)=0;
							break; }
						hidemouse();
						gettext(SCRN_LEFT+3+left,SCRN_TOP+y
							,SCRN_LEFT+left+width-2,SCRN_TOP+y,line);
						for(i=1;i<width*2;i+=2)
							line[i]=lclr|(bclr<<4);
						puttext(SCRN_LEFT+3+left,SCRN_TOP+y
							,SCRN_LEFT+left+width-2,SCRN_TOP+y,line);
						if((*cur)>j)
							y-=(*cur)-j;
						else
							y+=j-(*cur);
						if(bar) {
							if((*cur)>j)
								(*bar)-=(*cur)-j;
							else
								(*bar)+=j-(*cur); }
						(*cur)=j;
                        gettext(SCRN_LEFT+3+left,SCRN_TOP+y
							,SCRN_LEFT+left+width-2,SCRN_TOP+y,line);
						for(i=1;i<width*2;i+=2)
							line[i]=bclr|(LIGHTGRAY<<4);
						puttext(SCRN_LEFT+3+left,SCRN_TOP+y
							,SCRN_LEFT+left+width-2,SCRN_TOP+y,line);
						showmouse();
						break; } }
				if(a==2)
					s=0; }
			else
				switch(i) {
					case CR:
						if(!opts || (mode&WIN_XTR && (*cur)==opts-1))
							break;
						if(mode&WIN_ACT) {
							hidemouse();
							gettext(SCRN_LEFT+left,SCRN_TOP+top,SCRN_LEFT
								+left+width-1,SCRN_TOP+top+height-1,win);
							for(i=1;i<(width*height*2);i+=2)
								win[i]=lclr|(cclr<<4);
							j=(((y-top)*width)*2)+7+((width-4)*2);
							for(i=(((y-top)*width)*2)+7;i<j;i+=2)
                                win[i]=hclr|(cclr<<4);

							puttext(SCRN_LEFT+left,SCRN_TOP+top,SCRN_LEFT
								+left+width-1,SCRN_TOP+top+height-1,win);
							showmouse(); }
						else if(mode&WIN_SAV) {
							hidemouse();
							puttext(sav[api->savnum].left,sav[api->savnum].top
								,sav[api->savnum].right,sav[api->savnum].bot
								,sav[api->savnum].buf);
							showmouse();
							FREE(sav[api->savnum].buf);
							api->savdepth--; }
						return(*cur);
					case ESC:
						if((mode&WIN_ESC || (mode&WIN_CHE && api->changes))
							&& !(mode&WIN_SAV)) {
							hidemouse();
							gettext(SCRN_LEFT+left,SCRN_TOP+top,SCRN_LEFT
								+left+width-1,SCRN_TOP+top+height-1,win);
							for(i=1;i<(width*height*2);i+=2)
								win[i]=lclr|(cclr<<4);
							puttext(SCRN_LEFT+left,SCRN_TOP+top,SCRN_LEFT
								+left+width-1,SCRN_TOP+top+height-1,win);
							showmouse(); }
						else if(mode&WIN_SAV) {
							hidemouse();
							puttext(sav[api->savnum].left,sav[api->savnum].top
								,sav[api->savnum].right,sav[api->savnum].bot
								,sav[api->savnum].buf);
							showmouse();
							FREE(sav[api->savnum].buf);
							api->savdepth--; }
						return(-1); } } }
	else
		mswait(1);
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

hidemouse();
plen=strlen(prompt);
if(!plen)
	slen=4;
else
	slen=6;
width=plen+slen+max;
if(mode&WIN_T2B)
	top=(api->scrn_len/2)-(height/2)-2;
if(mode&WIN_L2R)
	left=36-(width/2);
if(mode&WIN_SAV)
	gettext(SCRN_LEFT+left,SCRN_TOP+top,SCRN_LEFT+left+width+1
		,SCRN_TOP+top+height,save_buf);
i=0;
in_win[i++]='�';
in_win[i++]=hclr|(bclr<<4);
for(c=1;c<width-1;c++) {
	in_win[i++]='�';
	in_win[i++]=hclr|(bclr<<4); }
in_win[i++]='�';
in_win[i++]=hclr|(bclr<<4);
in_win[i++]='�';
in_win[i++]=hclr|(bclr<<4);

if(plen) {
	in_win[i++]=SP;
	in_win[i++]=lclr|(bclr<<4); }

for(c=0;prompt[c];c++) {
	in_win[i++]=prompt[c];
	in_win[i++]=lclr|(bclr<<4); }

if(plen) {
	in_win[i++]=':';
	in_win[i++]=lclr|(bclr<<4);
	c++; }

for(c=0;c<max+2;c++) {
	in_win[i++]=SP;
	in_win[i++]=lclr|(bclr<<4); }

in_win[i++]='�';
in_win[i++]=hclr|(bclr<<4);
in_win[i++]='�';
in_win[i++]=hclr|(bclr<<4);
for(c=1;c<width-1;c++) {
	in_win[i++]='�';
	in_win[i++]=hclr|(bclr<<4); }
in_win[i++]='�';
in_win[i++]=hclr|(bclr<<4);
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
		,SCRN_TOP+top+height,shade); }

textattr(lclr|(bclr<<4));
if(!plen)
	gotoxy(SCRN_LEFT+left+2,SCRN_TOP+top+1);
else
	gotoxy(SCRN_LEFT+left+plen+4,SCRN_TOP+top+1);
i=ugetstr(str,max,kmode);
if(mode&WIN_SAV)
	puttext(SCRN_LEFT+left,SCRN_TOP+top,SCRN_LEFT+left+width+1
		,SCRN_TOP+top+height,save_buf);
showmouse();
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
/* Cursor should be at END of where string prompt will be placed.           */
/****************************************************************************/
static int ugetstr(char *outstr, int max, long mode)
{
	uchar   str[256],ins=0,buf[256],y;
	int		ch;
	int     i,j,k,f=0;	/* i=offset, j=length */
#ifndef __FLAT__
	union  REGS r;
#endif

curs_set(1);
y=wherey();
if(mode&K_EDIT) {
/***
	truncsp(outstr);
***/
	outstr[max]=0;
	textattr(bclr|(LIGHTGRAY<<4));
	cputs(outstr);
	textattr(lclr|(bclr<<4));
	strcpy(str,outstr);
	i=j=strlen(str);
	while(inkey(1)==0) {
#ifndef __FLAT__
		if(api->mode&UIFC_MOUSE) {
		/* ToDo More mouse stuff */
		}
#endif
		mswait(1);
	}
	f=inkey(0);
	gotoxy(wherex()-i,y);
	if(f != KEY_DC && f != KEY_BACKSPACE)
	{
		cputs(outstr);
		if(isprint(f))
		{
			putch(f);
			i++;
			j++;
		}
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
#ifndef __FLAT__
	if(api->mode&UIFC_MOUSE)
	{
		/* ToDo More Mouse Stuff */
	}
#endif
	if(inkey(1))
	{
		ch=inkey(0);
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
				{
					curs_set(2);
					refresh();
				}
				else
				{
					curs_set(1);
					refresh();
				}
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
			case 03:
			case ESC:
				{
					curs_set(0);
					refresh();
					return(-1);
				}
			case CR:
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
		if(mode&K_ALPHA && !isalpha(ch))
			continue;
		if((ch>=SP || (ch==1 && mode&K_MSG)) && i<max && (!ins || j<max))
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
	curs_set(0);
	refresh();
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
        buf[j++]=attr; }
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
	i+=11; }
if(line&BL_PUT) {
	uprintf(i,api->scrn_len+1,bclr|(cclr<<4),"F6 ");
	i+=3;
	uprintf(i,api->scrn_len+1,BLACK|(cclr<<4),"Paste  ");
    i+=7; }
if(line&BL_INS) {
	uprintf(i,api->scrn_len+1,bclr|(cclr<<4),"INS ");
	i+=4;
	uprintf(i,api->scrn_len+1,BLACK|(cclr<<4),"Add Item  ");
	i+=10; }
if(line&BL_DEL) {
	uprintf(i,api->scrn_len+1,bclr|(cclr<<4),"DEL ");
    i+=4;
	uprintf(i,api->scrn_len+1,BLACK|(cclr<<4),"Delete Item  ");
	i+=13; }
uprintf(i,api->scrn_len+1,bclr|(cclr<<4),"ESC ");
i+=4;
uprintf(i,api->scrn_len+1,BLACK|(cclr<<4),"Exit");
i+=4;
gotoxy(i,api->scrn_len+1);
textattr(BLACK|(cclr<<4));
clrtoeol();
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
		break; }
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
		break; }
if(gm->tm_hour>12) {
	strcpy(mer,"pm");
	hour=gm->tm_hour-12; }
else {
	if(!gm->tm_hour)
		hour=12;
	else
		hour=gm->tm_hour;
	strcpy(mer,"am"); }
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

	hidemouse();
	if(!str) {
		puttext(28,12,53,14,sav);
		showmouse();
		return; }
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
		buf[i+1]|=BLINK; }
	i=((25*2)+1)*2;
	buf[i]='�'; i+=2;
	buf[i]='�'; i+=2;
	for(;i<((26*3)-1)*2;i+=2)
		buf[i]='�';
	buf[i]='�';

	puttext(28,12,53,14,buf);
	showmouse();
}

/****************************************************************************/
/* Sets the current help index by source code file and line number.			*/
/****************************************************************************/
void sethelp(int line, char* file)
{
    helpline=line;
    helpfile=file;
}

/************************************************************/
/* Help (F1) key function. Uses helpbuf as the help input.	*/
/************************************************************/
void help()
{
	char *savscrn,*buf,inverse=0,high=0
		,hbuf[HELPBUF_SIZE],str[256];
    char *p;
	uint i,j,k,len;
	unsigned short line;
	long l;
	FILE *fp;
#ifndef __FLAT__
	union  REGS r;
#endif

	curs_set(0);

	if((savscrn=(char *)MALLOC(80*25*2))==NULL) {
		cprintf("UIFC line %d: error allocating %u bytes\r\n"
			,__LINE__,80*25*2);
		curs_set(1);
		return; }
	if((buf=(char *)MALLOC(76*21*2))==NULL) {
		cprintf("UIFC line %d: error allocating %u bytes\r\n"
			,__LINE__,76*21*2);
		FREE(savscrn);
		curs_set(1);
		return; }
	hidemouse();
	gettext(1,1,80,25,savscrn);
	memset(buf,SP,76*21*2);
	for(i=1;i<76*21*2;i+=2)
		buf[i]=(hclr|(bclr<<4));
	buf[0]='�';
	for(i=2;i<30*2;i+=2)
		buf[i]='�';
	buf[i]='�'; i+=4;
	buf[i]='O'; i+=2;
	buf[i]='n'; i+=2;
	buf[i]='l'; i+=2;
	buf[i]='i'; i+=2;
	buf[i]='n'; i+=2;
	buf[i]='e'; i+=4;
	buf[i]='H'; i+=2;
	buf[i]='e'; i+=2;
	buf[i]='l'; i+=2;
	buf[i]='p'; i+=4;
	buf[i]='�'; i+=2;
	for(j=i;j<i+(30*2);j+=2)
		buf[j]='�';
	i=j;
	buf[i]='�'; i+=2;
	j=i;	/* leave i alone */
	for(k=0;k<19;k++) { 		/* the sides of the box */
		buf[j]='�'; j+=2;
		j+=(74*2);
		buf[j]='�'; j+=2; }
	buf[j]='�'; j+=2;
	for(k=j;k<j+(23*2);k+=2)
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
	for(j=k;j<k+(24*2);j+=2)
		buf[j]='�';
	buf[j]='�';

	if(!api->helpbuf) {
		if((fp=_fsopen(api->helpixbfile,"rb",SH_DENYWR))==NULL)
			sprintf(hbuf," ERROR  Cannot open help index:\r\n          %s"
				,api->helpixbfile);
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
					continue; }
				fread(&l,4,1,fp);
				break; }
			fclose(fp);
			if(l==-1L)
				sprintf(hbuf," ERROR  Cannot locate help key (%s:%u) in:\r\n"
					"         %s",p,helpline,api->helpixbfile);
			else {
				if((fp=_fsopen(api->helpdatfile,"rb",SH_DENYWR))==NULL)
					sprintf(hbuf," ERROR  Cannot open help file:\r\n          %s"
						,api->helpdatfile);
				else {
					fseek(fp,l,SEEK_SET);
					fread(hbuf,HELPBUF_SIZE,1,fp);
					fclose(fp); } } } }
	else
		strcpy(hbuf,api->helpbuf);

	len=strlen(hbuf);

	i+=78*2;
	for(j=0;j<len;j++,i+=2) {
		if(hbuf[j]==LF) {
			while(i%(76*2)) i++;
			i+=2; }
		else if(hbuf[j]==2) {		 /* Ctrl-b toggles inverse */
			inverse=!inverse;
			i-=2; }
		else if(hbuf[j]==1) {		 /* Ctrl-a toggles high intensity */
			high=!high;
			i-=2; }
		else if(hbuf[j]!=CR) {
			buf[i]=hbuf[j];
			buf[i+1]=inverse ? (bclr|(cclr<<4))
				: high ? (hclr|(bclr<<4)) : (lclr|(bclr<<4)); } }
	puttext(3,3,78,23,buf);
	showmouse();
	while(1) {
		if(inkey(1)) {
			inkey(0);
			break; }
	#ifndef __FLAT__
		if(api->mode&UIFC_MOUSE) {
			/* ToDo more mouse stiff */
		}
	#endif
		mswait(1);
		}

	hidemouse();
	puttext(1,1,80,25,savscrn);
	showmouse();
	FREE(savscrn);
	FREE(buf);
	curs_set(1);
}

static int puttext(int sx, int sy, int ex, int ey, unsigned char *fill)
{
	int x,y;
	int fillpos=0;
	unsigned char attr;
	unsigned char fill_char;
	unsigned char orig_attr;
	int oldx, oldy;

	getyx(stdscr,oldy,oldx);	
	orig_attr=lastattr;
	for(y=sy-1;y<=ey-1;y++)
	{
		for(x=sx-1;x<=ex-1;x++)
		{
			fill_char=fill[fillpos++];
			attr=fill[fillpos++];
			textattr(attr);
			mvaddch(y, x, fill_char);
		}
	}
	textattr(orig_attr);
	move(oldy, oldx);
	refresh();
	return(1);
}

static int gettext(int sx, int sy, int ex, int ey, unsigned char *fill)
{
	int x,y;
	int fillpos=0;
	chtype attr;
	unsigned char attrib;
	unsigned char colour;
	int oldx, oldy;

	getyx(stdscr,oldy,oldx);	
	for(y=sy-1;y<=ey-1;y++)
	{
		for(x=sx-1;x<=ex-1;x++)
		{
			attr=mvinch(y, x);
			fill[fillpos++]=(unsigned char)(attr&255);
			attrib=0;
			if (attr & A_BOLD)  
			{
				attrib |= 8;
			}
			if (attr & A_BLINK)
			{
				attrib |= 128;
			}
			colour=PAIR_NUMBER(attr&A_COLOR)-1;
			colour=((colour&56)<<1)|(colour&7);
			fill[fillpos++]=colour|attrib;
		}
	}
	move(oldy, oldx);
	return(1);
}

static void textattr(unsigned char attr)
{
	int   attrs=A_NORMAL;
	short	colour;

	if (lastattr==attr)
		return;

	lastattr=attr;
	
	if (attr & 8)  {
		attrs |= A_BOLD;
	}
	if (attr & 128)
	{
		attrs |= A_BLINK;
	}
	attrset(attrs);
	colour = COLOR_PAIR( ((attr&7)|((attr>>1)&56))+1 );
	color_set(colour,NULL);
	bkgdset(colour);
}

static int kbhit(void)
{
	struct timeval timeout;
	fd_set	rfds;
	
	timeout.tv_sec=5;
	timeout.tv_usec=0;
	FD_ZERO(&rfds);
	FD_SET(fileno(stdin),&rfds);

	return(select(fileno(stdin)+1,&rfds,NULL,NULL,&timeout));
}

static void delay(msec)
{
	usleep(msec*1000);
}

static int wherey(void)
{
	int x,y;
	getyx(stdscr,y,x);
	return(y+1);
}

static int wherex(void)
{
	int x,y;
	getyx(stdscr,y,x);
	return(x+1);
}

static FILE * _fsopen(char *pathname, char *mode, int flags)
{
	FILE *thefile;
	
	thefile = fopen(pathname, mode);
	if (thefile != NULL)  {
		if (flags&SH_DENYWR) {
			flock(fileno(thefile), LOCK_SH);
			return(thefile);
		}
		if (flags&SH_DENYRW) {
			flock(fileno(thefile), LOCK_EX);
			return(thefile);
		}
	}
	return(thefile);
}

static void putch(unsigned char ch)
{
	addch(ch);
	refresh();
}

static int cprintf(char *fmat, ...)
{
    va_list argptr;

    va_start(argptr,fmat);
    vwprintw(stdscr,fmat,argptr);
    va_end(argptr);
    refresh();
    return(1);
}

static void cputs(char *str)
{
	addstr(str);
	refresh();
}

static void gotoxy(int x, int y)
{
	move(y-1,x-1);
	refresh();
}
