/* UIFC.H */

/* Rob Swindell's Text-mode User Interface Library */

/* Developed 1990-1997 by Rob Swindell; PO Box 501, Yorba Linda, CA 92885 */

#include <io.h>
#include <dos.h>
#include <time.h>
#include <fcntl.h>
#include <alloc.h>
#include <ctype.h>
#include <conio.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

/* OS Specific */
#if defined(__FLAT__)
	#define far
#endif

#if !defined(__FLAT__)
    #include <bios.h>
#endif


/****************************************************************************/
/* MALLOC/FREE Macros for various compilers and environments				*/
/* MALLOC is used for allocations of 64k or less							*/
/* FREE is used to free buffers allocated with MALLOC						*/
/* LMALLOC is used for allocations of possibly larger than 64k				*/
/* LFREE is used to free buffers allocated with LMALLOC 					*/
/* REALLOC is used to re-size a previously MALLOCed or LMALLOCed buffer 	*/
/****************************************************************************/
#if defined(__COMPACT__) || defined(__LARGE__) || defined(__HUGE__)
	#if defined(__TURBOC__)
		#define REALLOC(x,y) farrealloc(x,y)
		#define LMALLOC(x) farmalloc(x)
		#define MALLOC(x) farmalloc(x)
		#define LFREE(x) farfree(x)
		#define FREE(x) farfree(x)
	#elif defined(__WATCOMC__)
		#define REALLOC realloc
		#define LMALLOC(x) halloc(x,1)	/* far heap, but slow */
		#define MALLOC malloc			/* far heap, but 64k max */
		#define LFREE hfree
		#define FREE free
	#else	/* Other 16-bit Compiler */
		#define REALLOC realloc
		#define LMALLOC malloc
		#define MALLOC malloc
		#define LFREE free
		#define FREE free
	#endif
#else		/* 32-bit Compiler or Small Memory Model */
	#define REALLOC realloc
	#define LMALLOC malloc
	#define MALLOC malloc
	#define LFREE free
	#define FREE free
#endif

#ifdef __DPMI32__
	#define INT_86(i,j,k) int386(i,j,k)
#else
	#define INT_86(i,j,k) int86(i,j,k)
#endif

#ifdef __FLAT__
	#define MAX_OPTS	1000
	#define MSK_ON		0xf0000000
	#define MSK_OFF 	0x0fffffff
	#define MSK_INS 	0x10000000
	#define MSK_DEL 	0x20000000
	#define MSK_GET 	0x30000000
	#define MSK_PUT 	0x40000000
#else
	#define MAX_OPTS	500 	/* Maximum number of options per menu call */
	#define MSK_ON		0xf000
	#define MSK_OFF 	0x0fff
	#define MSK_INS 	0x1000
	#define MSK_DEL 	0x2000
	#define MSK_GET 	0x3000
	#define MSK_PUT 	0x4000
#endif
#define MAX_OPLN	75		/* Maximum length of each option per menu call */
#define MAX_BUFS	7		/* Maximum number of screen buffers to save */
#define MAX_BFLN	8000	/* Maximum size of screen buffers - 80x50	*/

#ifndef uint
#define uint unsigned int
#endif

#define UIFC_INMSG	(1<<0)	/* Currently in Message Routine non-recursive */
#define UIFC_MOUSE	(1<<1)	/* Mouse installed and available */
#define UIFC_MONO	(1<<2)	/* Force monochrome mode */
#define UIFC_COLOR	(1<<3)	/* Force color mode */

#define WIN_ORG 	(1<<0)	/* Original menu - destroy valid screen area */
#define WIN_SAV 	(1<<1)	/* Save existing text and replace when finished */
#define WIN_ACT 	(1<<2)	/* Menu remains active after a selection */
#define WIN_L2R 	(1<<3)	/* Center the window based on 'width'   */
#define WIN_T2B 	(1<<4)	/* Center the window based on 'height'  */
#define WIN_INS 	(1<<5)	/* Allows user to user insert key */
#define WIN_INSACT	(1<<6)	/* Remains active after insert key */
#define WIN_DEL 	(1<<7)	/* Allows user to use delete key */
#define WIN_DELACT	(1<<8)	/* Remains active after delete key */
#define WIN_ESC 	(1<<9)	/* Screen is active when escape is hit			 */
#define WIN_RHT 	(1<<10) /* Place window against right side of screen */
#define WIN_BOT 	(1<<11) /* Place window against botton of screen */
#define WIN_GET 	(1<<12) /* Allows F5 to Get a menu item */
#define WIN_PUT 	(1<<13) /* Allows F6 to Put a menu item */
#define WIN_CHE 	(1<<14) /* Stay active after escape if changes */
#define WIN_XTR 	(1<<15) /* Add extra line at end for inserting at end */

#define WIN_MID WIN_L2R|WIN_T2B  /* Place window in middle of screen */

#define SCRN_TOP	3
#define SCRN_LEFT	5
#define SCRN_RIGHT  76

								/* Bits in 'mode' for getkey and getstr     */
#define K_UPPER 	(1L<<0) 	/* Converts all letters to upper case		*/
#define K_UPRLWR	(1L<<1) 	/* Upper/Lower case automatically			*/
#define K_NUMBER	(1L<<2) 	/* Allow numbers only						*/
#define K_WRAP		(1L<<3) 	/* Allows word wrap 						*/
#define K_MSG		(1L<<4) 	/* Allows ANSI, ^N ^A ^G					*/
#define K_SPIN		(1L<<5) 	/* Spinning cursor (same as SPIN)			*/
#define K_LINE		(1L<<6) 	/* Input line (inverse color)				*/
#define K_EDIT		(1L<<7) 	/* Edit string passed						*/
#define K_CHAT		(1L<<8) 	/* In chat multi-chat						*/
#define K_NOCRLF	(1L<<9) 	/* Don't print CRLF after string input      */
#define K_ALPHA 	(1L<<10)	/* Only allow alphabetic characters 		*/

#define HELPBUF_SIZE 4000

#define SETHELP(where)	helpline=__LINE__; helpfile=__FILE__

#ifndef TAB
									/* Control characters */
#define STX 	0x02				/* Start of text			^B	*/
#define ETX 	0x03				/* End of text				^C	*/
#define BS		'\b'				/* Back space				^H	*/
#define TAB 	'\t'				/* Horizontal tabulation	^I	*/
#define LF		'\n'				/* Line feed				^J	*/
#define FF		0x0c				/* Form feed				^L	*/
#define CR		'\r'				/* Carriage return			^M	*/
#define ESC 	0x1b				/* Escape					^[	*/
#define SP      ' '                 /* Space                        */

#endif

#define CLREOL 256

#ifndef uchar				/* Short-hand for unsigned data types */
#define uchar unsigned char
#endif
#ifndef uint
#define uint unsigned int
#endif
#ifndef ulong
#define ulong unsigned long
#endif


typedef struct {
	char	left,top,right,bot,*buf;
	} win_t;


/* LCLOLL.ASM */
int lclini(int);
void lclxy(int,int);
int lclwx(void);
int lclwy(void);
int lclatr(int);
void lputc(int);
long lputs(char far *);

#if defined(__OS2__) || !defined(__FLAT__)
void mswait(int msecs);
extern mswtyp;
#endif

extern char scrn_len,lclr,hclr,bclr,cclr,blk_scrn[MAX_BFLN],savdepth
	,changes,show_free_mem,savnum,uifc_status,*helpfile,*helpbuf
	,helpdatfile[256]
	,helpixbfile[256];
extern win_t sav[MAX_BUFS];
extern uint cursor,helpline;

void uifcini(void);
int uscrn(char *str);
int ulist(int mode, char left, int top, char width, int *dflt, int *bar
	,char *title, char **option);
int uinput(int imode, char left, char top, char *prompt, char *str
	,char len ,int kmode);
int  uprintf(char x, char y, char attr, char *fmt,...);
void umsg(char *str);
void upop(char *str);
int  getstr(char *str, int maxlen, long mode);
void timedisplay();
void puttextinfo(struct text_info txt);
int  lprintf(char *fmt,...);
char *timestr(time_t *intime);
void help(void);
void truncsp(char *str);
void uifcbail(void);
