#if (defined(__MACH__) && defined(__APPLE__))
#include <Carbon/Carbon.h>
#define USE_PASTEBOARD
#include "pasteboard.h"
#endif

#include <stdarg.h>
#include <stdio.h>		/* NULL */
#include <stdlib.h>
#include <string.h>

#include "gen_defs.h"
#include "genwrap.h"
#include "dirwrap.h"
#include "xpbeep.h"
#include "threadwrap.h"
#ifdef __unix__
#include <xp_dl.h>
#endif

#if (defined CIOLIB_IMPORTS)
 #undef CIOLIB_IMPORTS
#endif
#if (defined CIOLIB_EXPORTS)
 #undef CIOLIB_EXPORTS
#endif

#include "ciolib.h"
#include "vidmodes.h"
#define BITMAP_CIOLIB_DRIVER
#include "bitmap_con.h"

#include "SDL.h"
#include "SDL_thread.h"

#include "sdlfuncs.h"

int bitmap_width,bitmap_height;

/* 256 bytes so I can cheat */
unsigned char		sdl_keybuf[256];		/* Keyboard buffer */
unsigned char		sdl_key=0;				/* Index into keybuf for next key in buffer */
unsigned char		sdl_keynext=0;			/* Index into keybuf for next free position */

int sdl_exitcode=0;

SDL_Window	*win=NULL;
SDL_Renderer	*renderer=NULL;
SDL_Texture	*texture=NULL;
SDL_mutex	*win_mutex;
SDL_Surface	*sdl_icon=NULL;

/* *nix copy/paste stuff */
int paste_needs_events;
int copy_needs_events;
SDL_sem	*sdl_pastebuf_set;
SDL_sem	*sdl_pastebuf_copied;
SDL_mutex	*sdl_copybuf_mutex;
char *sdl_copybuf=NULL;
char *sdl_pastebuf=NULL;

SDL_sem *sdl_ufunc_ret;
SDL_sem *sdl_ufunc_rec;
SDL_mutex *sdl_ufunc_mtx;
int sdl_ufunc_retval;

SDL_sem	*sdl_flush_sem;
int pending_updates=0;

int fullscreen=0;

int	sdl_init_good=0;
SDL_mutex *sdl_keylock;
SDL_sem *sdl_key_pending;
static unsigned int sdl_pending_mousekeys=0;
static int sdl_using_directx=0;
static int sdl_using_quartz=0;
static int sdl_using_x11=0;
static int sdl_x11available=0;

static struct video_stats cvstat;

struct sdl_keyvals {
	int	keysym
		,key
		,shift
		,ctrl
		,alt;
};

static SDL_mutex *sdl_headlock;
static struct rectlist *update_list = NULL;
static struct rectlist *update_list_tail = NULL;

enum {
	 SDL_USEREVENT_FLUSH
	,SDL_USEREVENT_SETTITLE
	,SDL_USEREVENT_SETNAME
	,SDL_USEREVENT_SETICON
	,SDL_USEREVENT_SETVIDMODE
	,SDL_USEREVENT_SHOWMOUSE
	,SDL_USEREVENT_HIDEMOUSE
	,SDL_USEREVENT_INIT
	,SDL_USEREVENT_COPY
	,SDL_USEREVENT_PASTE
	,SDL_USEREVENT_QUIT
};

const struct sdl_keyvals sdl_keyval[] =
{
	{SDLK_BACKSPACE, 0x08, 0x08, 0x7f, 0x0e00},
	{SDLK_TAB, 0x09, 0x0f00, 0x9400, 0xa500},
	{SDLK_RETURN, 0x0d, 0x0d, 0x0a, 0xa600},
	{SDLK_ESCAPE, 0x1b, 0x1b, 0x1b, 0x0100},
	{SDLK_SPACE, 0x20, 0x20, 0x0300, 0x20},
	{SDLK_0, '0', ')', 0, 0x8100},
	{SDLK_1, '1', '!', 0, 0x7800},
	{SDLK_2, '2', '@', 0x0300, 0x7900},
	{SDLK_3, '3', '#', 0, 0x7a00},
	{SDLK_4, '4', '$', 0, 0x7b00},
	{SDLK_5, '5', '%', 0, 0x7c00},
	{SDLK_6, '6', '^', 0x1e, 0x7d00},
	{SDLK_7, '7', '&', 0, 0x7e00},
	{SDLK_8, '8', '*', 0, 0x7f00},
	{SDLK_9, '9', '(', 0, 0x8000},
	{SDLK_a, 'a', 'A', 0x01, 0x1e00},
	{SDLK_b, 'b', 'B', 0x02, 0x3000},
	{SDLK_c, 'c', 'C', 0x03, 0x2e00},
	{SDLK_d, 'd', 'D', 0x04, 0x2000},
	{SDLK_e, 'e', 'E', 0x05, 0x1200},
	{SDLK_f, 'f', 'F', 0x06, 0x2100},
	{SDLK_g, 'g', 'G', 0x07, 0x2200},
	{SDLK_h, 'h', 'H', 0x08, 0x2300},
	{SDLK_i, 'i', 'I', 0x09, 0x1700},
	{SDLK_j, 'j', 'J', 0x0a, 0x2400},
	{SDLK_k, 'k', 'K', 0x0b, 0x2500},
	{SDLK_l, 'l', 'L', 0x0c, 0x2600},
	{SDLK_m, 'm', 'M', 0x0d, 0x3200},
	{SDLK_n, 'n', 'N', 0x0e, 0x3100},
	{SDLK_o, 'o', 'O', 0x0f, 0x1800},
	{SDLK_p, 'p', 'P', 0x10, 0x1900},
	{SDLK_q, 'q', 'Q', 0x11, 0x1000},
	{SDLK_r, 'r', 'R', 0x12, 0x1300},
	{SDLK_s, 's', 'S', 0x13, 0x1f00},
	{SDLK_t, 't', 'T', 0x14, 0x1400},
	{SDLK_u, 'u', 'U', 0x15, 0x1600},
	{SDLK_v, 'v', 'V', 0x16, 0x2f00},
	{SDLK_w, 'w', 'W', 0x17, 0x1100},
	{SDLK_x, 'x', 'X', 0x18, 0x2d00},
	{SDLK_y, 'y', 'Y', 0x19, 0x1500},
	{SDLK_z, 'z', 'Z', 0x1a, 0x2c00},
	{SDLK_PAGEUP, 0x4900, 0x4900, 0x8400, 0x9900},
	{SDLK_PAGEDOWN, 0x5100, 0x5100, 0x7600, 0xa100},
	{SDLK_END, 0x4f00, 0x4f00, 0x7500, 0x9f00},
	{SDLK_HOME, 0x4700, 0x4700, 0x7700, 0x9700},
	{SDLK_LEFT, 0x4b00, 0x4b00, 0x7300, 0x9b00},
	{SDLK_UP, 0x4800, 0x4800, 0x8d00, 0x9800},
	{SDLK_RIGHT, 0x4d00, 0x4d00, 0x7400, 0x9d00},
	{SDLK_DOWN, 0x5000, 0x5000, 0x9100, 0xa000},
	{SDLK_INSERT, 0x5200, 0x5200, 0x9200, 0xa200},
	{SDLK_DELETE, 0x5300, 0x5300, 0x9300, 0xa300},
	{SDLK_KP_0, 0x5200, 0x5200, 0x9200, 0},
	{SDLK_KP_1, 0x4f00, 0x4f00, 0x7500, 0},
	{SDLK_KP_2, 0x5000, 0x5000, 0x9100, 0},
	{SDLK_KP_3, 0x5100, 0x5100, 0x7600, 0},
	{SDLK_KP_4, 0x4b00, 0x4b00, 0x7300, 0},
	{SDLK_KP_5, 0x4c00, 0x4c00, 0x8f00, 0},
	{SDLK_KP_6, 0x4d00, 0x4d00, 0x7400, 0},
	{SDLK_KP_7, 0x4700, 0x4700, 0x7700, 0},
	{SDLK_KP_8, 0x4800, 0x4800, 0x8d00, 0},
	{SDLK_KP_9, 0x4900, 0x4900, 0x8400, 0},
	{SDLK_KP_MULTIPLY, '*', '*', 0x9600, 0x3700},
	{SDLK_KP_PLUS, '+', '+', 0x9000, 0x4e00},
	{SDLK_KP_MINUS, '-', '-', 0x8e00, 0x4a00},
	{SDLK_KP_PERIOD, 0x7f, 0x7f, 0x5300, 0x9300},
	{SDLK_KP_DIVIDE, '/', '/', 0x9500, 0xa400},
	{SDLK_KP_ENTER, 0x0d, 0x0d, 0x0a, 0xa600},
	{SDLK_F1, 0x3b00, 0x5400, 0x5e00, 0x6800},
	{SDLK_F2, 0x3c00, 0x5500, 0x5f00, 0x6900},
	{SDLK_F3, 0x3d00, 0x5600, 0x6000, 0x6a00},
	{SDLK_F4, 0x3e00, 0x5700, 0x6100, 0x6b00},
	{SDLK_F5, 0x3f00, 0x5800, 0x6200, 0x6c00},
	{SDLK_F6, 0x4000, 0x5900, 0x6300, 0x6d00},
	{SDLK_F7, 0x4100, 0x5a00, 0x6400, 0x6e00},
	{SDLK_F8, 0x4200, 0x5b00, 0x6500, 0x6f00},
	{SDLK_F9, 0x4300, 0x5c00, 0x6600, 0x7000},
	{SDLK_F10, 0x4400, 0x5d00, 0x6700, 0x7100},
	{SDLK_F11, 0x8500, 0x8700, 0x8900, 0x8b00},
	{SDLK_F12, 0x8600, 0x8800, 0x8a00, 0x8c00},
	{SDLK_BACKSLASH, '\\', '|', 0x1c, 0x2b00},
	{SDLK_SLASH, '/', '?', 0, 0x3500},
	{SDLK_MINUS, '-', '_', 0x1f, 0x8200},
	{SDLK_EQUALS, '=', '+', 0, 0x8300},
	{SDLK_LEFTBRACKET, '[', '{', 0x1b, 0x1a00},
	{SDLK_RIGHTBRACKET, ']', '}', 0x1d, 0x1b00},
	{SDLK_SEMICOLON, ';', ':', 0, 0x2700},
	{SDLK_QUOTE, '\'', '"', 0, 0x2800},
	{SDLK_COMMA, ',', '<', 0, 0x3300},
	{SDLK_PERIOD, '.', '>', 0, 0x3400},
	{SDLK_BACKQUOTE, '`', '~', 0, 0x2900},
	{0, 0, 0, 0, 0}	/** END **/
};

void sdl_setscaling(int new_value);

#if !defined(NO_X) && defined(__unix__)
#include "SDL_syswm.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>

#define CONSOLE_CLIPBOARD	XA_PRIMARY

/* X functions */
struct x11 {
	int		(*XFree)		(void *data);
	Window	(*XGetSelectionOwner)	(Display*, Atom);
	int		(*XConvertSelection)	(Display*, Atom, Atom, Atom, Window, Time);
	int		(*XGetWindowProperty)	(Display*, Window, Atom, long, long, Bool, Atom, Atom*, int*, unsigned long *, unsigned long *, unsigned char **);
	int		(*XChangeProperty)		(Display*, Window, Atom, Atom, int, int, _Xconst unsigned char*, int);
	Status	(*XSendEvent)	(Display*, Window, Bool, long, XEvent*);
	int		(*XSetSelectionOwner)	(Display*, Atom, Window, Time);
};
struct x11 sdl_x11;
#endif

static void sdl_video_event_thread(void *data);

static void sdl_user_func(int func, ...)
{
	va_list argptr;
	SDL_Event	ev;
	int rv;

	ev.type=SDL_USEREVENT;
	ev.user.data1=NULL;
	ev.user.data2=NULL;
	ev.user.code=func;
	sdl.mutexP(sdl_ufunc_mtx);
	/* Drain the swamp */
	if(sdl_x11available && sdl_using_x11)
		while(sdl.SemWaitTimeout(sdl_ufunc_rec, 0)==0);
	while (1) {
		va_start(argptr, func);
		switch(func) {
			case SDL_USEREVENT_SETICON:
				ev.user.data1=va_arg(argptr, void *);
				if((ev.user.data2=(unsigned long *)malloc(sizeof(unsigned long)))==NULL) {
					sdl.mutexV(sdl_ufunc_mtx);
					va_end(argptr);
					return;
				}
				*(unsigned long *)ev.user.data2=va_arg(argptr, unsigned long);
				break;
			case SDL_USEREVENT_SETNAME:
			case SDL_USEREVENT_SETTITLE:
				if((ev.user.data1=strdup(va_arg(argptr, char *)))==NULL) {
					sdl.mutexV(sdl_ufunc_mtx);
					va_end(argptr);
					return;
				}
				break;
			case SDL_USEREVENT_COPY:
			case SDL_USEREVENT_PASTE:
			case SDL_USEREVENT_SHOWMOUSE:
			case SDL_USEREVENT_HIDEMOUSE:
			case SDL_USEREVENT_FLUSH:
				break;
			default:
				va_end(argptr);
				return;
		}
		va_end(argptr);
		while((rv = sdl.PeepEvents(&ev, 1, SDL_ADDEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT))!=1)
			YIELD();
		if (func != SDL_USEREVENT_FLUSH) {
			if(sdl_x11available && sdl_using_x11)
				if ((rv = sdl.SemWaitTimeout(sdl_ufunc_rec, 2000)) != 0)
					continue;
		}
		break;
	}
	sdl.mutexV(sdl_ufunc_mtx);
}

/* Called from main thread only */
static int sdl_user_func_ret(int func, ...)
{
	int rv;
	va_list argptr;
	SDL_Event	ev;

	ev.type=SDL_USEREVENT;
	ev.user.data1=NULL;
	ev.user.data2=NULL;
	ev.user.code=func;
	va_start(argptr, func);
	sdl.mutexP(sdl_ufunc_mtx);
	/* Drain the swamp */
	if(sdl_x11available && sdl_using_x11)
		while(sdl.SemWaitTimeout(sdl_ufunc_rec, 0)==0);
	while(1) {
		switch(func) {
			case SDL_USEREVENT_SETVIDMODE:
			case SDL_USEREVENT_INIT:
			case SDL_USEREVENT_QUIT:
				while(sdl.PeepEvents(&ev, 1, SDL_ADDEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT)!=1)
					YIELD();
				break;
			default:
				sdl.mutexV(sdl_ufunc_mtx);
				va_end(argptr);
				return -1;
		}
		rv = sdl.SemWait(sdl_ufunc_ret);
		if(rv==0)
			break;
	}
	sdl.mutexV(sdl_ufunc_mtx);
	va_end(argptr);
	return(sdl_ufunc_retval);
}

void exit_sdl_con(void)
{
	// Avoid calling exit(0) from an atexit() function...
	ciolib_reaper = 0;
	sdl_user_func_ret(SDL_USEREVENT_QUIT);
}

void sdl_copytext(const char *text, size_t buflen)
{
#if (defined(__MACH__) && defined(__APPLE__))
	if(!sdl_using_x11) {
#if defined(USE_PASTEBOARD)
		if (text && buflen)
			OSX_copytext(text, buflen);
		return;
#endif
#if defined(USE_SCRAP_MANAGER)
		ScrapRef	scrap;
		if(text && buflen) {
			if(!ClearCurrentScrap()) {		/* purge the current contents of the scrap. */
				if(!GetCurrentScrap(&scrap)) {		/* obtain a reference to the current scrap. */
					PutScrapFlavor(scrap, kScrapFlavorTypeText, /* kScrapFlavorMaskTranslated */ kScrapFlavorMaskNone, buflen, text); 		/* write the data to the scrap */
				}
			}
		}
		return;
#endif
	}
#endif

#if !defined(NO_X) && defined(__unix__)
	if(sdl_x11available && sdl_using_x11) {
		sdl.mutexP(sdl_copybuf_mutex);
		FREE_AND_NULL(sdl_copybuf);

		sdl_copybuf=(char *)malloc(buflen+1);
		if(sdl_copybuf!=NULL) {
			strcpy(sdl_copybuf, text);
			sdl_user_func(SDL_USEREVENT_COPY,0,0,0,0);
		}
		sdl.mutexV(sdl_copybuf_mutex);
		return;
	}
#endif

	sdl.mutexP(sdl_copybuf_mutex);
	FREE_AND_NULL(sdl_copybuf);

	sdl_copybuf=strdup(text);
	sdl.mutexV(sdl_copybuf_mutex);
	return;
}

char *sdl_getcliptext(void)
{
	char *ret=NULL;

#if (defined(__MACH__) && defined(__APPLE__))
	if(!sdl_using_x11) {
#if defined(USE_PASTEBOARD)
		return OSX_getcliptext();
#endif
#if defined(USE_SCRAP_MANAGER)
		ScrapRef	scrap;
		UInt32	fl;
		Size		scraplen;

		if(!GetCurrentScrap(&scrap)) {		/* obtain a reference to the current scrap. */
			if(!GetScrapFlavorFlags(scrap, kScrapFlavorTypeText, &fl) /* && (fl & kScrapFlavorMaskTranslated) */) {
				if(!GetScrapFlavorSize(scrap, kScrapFlavorTypeText, &scraplen)) {
					ret=(char *)malloc(scraplen+1);
					if(ret!=NULL) {
						if(GetScrapFlavorData(scrap, kScrapFlavorTypeText, &scraplen, sdl_pastebuf))
							ret[scraplen]=0;
					}
				}
			}
		}
		return ret;
#endif
	}
#endif

#if !defined(NO_X) && defined(__unix__)
	if(sdl_x11available && sdl_using_x11) {
		sdl_user_func(SDL_USEREVENT_PASTE,0,0,0,0);
		sdl.SemWait(sdl_pastebuf_set);
		if(sdl_pastebuf!=NULL) {
			ret=(char *)malloc(strlen(sdl_pastebuf)+1);
			if(ret!=NULL)
				strcpy(ret,sdl_pastebuf);
		}
		else
			ret=NULL;
		sdl.SemPost(sdl_pastebuf_copied);
		return(ret);
	}
#endif
	sdl.mutexP(sdl_copybuf_mutex);
	if(sdl_copybuf)
		ret=strdup(sdl_copybuf);
	sdl.mutexV(sdl_copybuf_mutex);
	return(ret);
}

void sdl_drawrect(struct rectlist *data)
{
	if(sdl_init_good) {
		data->next = NULL;
		sdl.mutexP(sdl_headlock);
		if (update_list == NULL)
			update_list = update_list_tail = data;
		else {
			update_list_tail->next = data;
			update_list_tail = data;
		}
		sdl.mutexV(sdl_headlock);
	}
	else
		bitmap_drv_free_rect(data);
}

void sdl_flush(void)
{
	sdl_user_func(SDL_USEREVENT_FLUSH);
}

static int sdl_init_mode(int mode)
{
	int oldcols;

	if (mode != CIOLIB_MODE_CUSTOM) {
		pthread_mutex_lock(&vstatlock);
		if (mode == vstat.mode) {
			pthread_mutex_unlock(&vstatlock);
			return 0;
		}
		pthread_mutex_unlock(&vstatlock);
	}

	oldcols = cvstat.cols;

	sdl_user_func(SDL_USEREVENT_FLUSH);

	pthread_mutex_lock(&blinker_lock);
	pthread_mutex_lock(&vstatlock);
	bitmap_drv_init_mode(mode, &bitmap_width, &bitmap_height);
	if(oldcols != vstat.cols) {
		if(oldcols == 40)
			vstat.scaling /= 2;
		if(vstat.cols == 40)
			vstat.scaling *= 2;
	}
	if(vstat.scaling < 1)
		vstat.scaling = 1;
	if(vstat.vmultiplier < 1)
		vstat.vmultiplier = 1;

	cvstat = vstat;
	pthread_mutex_unlock(&vstatlock);
	pthread_mutex_unlock(&blinker_lock);

	sdl_user_func_ret(SDL_USEREVENT_SETVIDMODE);

	return(0);
}

/* Called from main thread only (Passes Event) */
int sdl_init(int mode)
{
#if !defined(NO_X) && defined(__unix__)
	dll_handle	dl;
	const char *libnames[2]={"X11", NULL};
#endif

	bitmap_drv_init(sdl_drawrect, sdl_flush);

	if(mode==CIOLIB_MODE_SDL_FULLSCREEN)
		fullscreen=1;
	// Needs to be *after* bitmap_drv_init()
	_beginthread(sdl_video_event_thread, 0, NULL);
	sdl_user_func_ret(SDL_USEREVENT_INIT);
	sdl_init_mode(3);

	if(sdl_init_good) {
		cio_api.mode=fullscreen?CIOLIB_MODE_SDL_FULLSCREEN:CIOLIB_MODE_SDL;
#ifdef _WIN32
		FreeConsole();
#endif
#if !defined(NO_X) && defined(__unix__)
		dl=xp_dlopen(libnames,RTLD_LAZY|RTLD_GLOBAL,7);
		if(dl!=NULL) {
			sdl_x11available=TRUE;
			if(sdl_x11available && (sdl_x11.XFree=xp_dlsym(dl,XFree))==NULL) {
				xp_dlclose(dl);
				sdl_x11available=FALSE;
			}
			if(sdl_x11available && (sdl_x11.XGetSelectionOwner=xp_dlsym(dl,XGetSelectionOwner))==NULL) {
				xp_dlclose(dl);
				sdl_x11available=FALSE;
			}
			if(sdl_x11available && (sdl_x11.XConvertSelection=xp_dlsym(dl,XConvertSelection))==NULL) {
				xp_dlclose(dl);
				sdl_x11available=FALSE;
			}
			if(sdl_x11available && (sdl_x11.XGetWindowProperty=xp_dlsym(dl,XGetWindowProperty))==NULL) {
				xp_dlclose(dl);
				sdl_x11available=FALSE;
			}
			if(sdl_x11available && (sdl_x11.XChangeProperty=xp_dlsym(dl,XChangeProperty))==NULL) {
				xp_dlclose(dl);
				sdl_x11available=FALSE;
			}
			if(sdl_x11available && (sdl_x11.XSendEvent=xp_dlsym(dl,XSendEvent))==NULL) {
				xp_dlclose(dl);
				sdl_x11available=FALSE;
			}
			if(sdl_x11available && (sdl_x11.XSetSelectionOwner=xp_dlsym(dl,XSetSelectionOwner))==NULL) {
				xp_dlclose(dl);
				sdl_x11available=FALSE;
			}
		}
#else
		sdl_x11available=FALSE;
#endif
		cio_api.options |= CONIO_OPT_PALETTE_SETTING | CONIO_OPT_SET_TITLE | CONIO_OPT_SET_NAME | CONIO_OPT_SET_ICON;
		return(0);
	}

	ciolib_reaper = 0;
	sdl_user_func_ret(SDL_USEREVENT_QUIT);
	return(-1);
}

void sdl_setscaling(int new_value)
{
	pthread_mutex_lock(&vstatlock);
	cvstat.scaling = vstat.scaling = new_value;
	pthread_mutex_unlock(&vstatlock);
}

int sdl_getscaling(void)
{
	return cvstat.scaling;
}

/* Called from main thread only */
int sdl_kbhit(void)
{
	int ret;

	sdl.mutexP(sdl_keylock);
	ret=(sdl_key!=sdl_keynext);
	sdl.mutexV(sdl_keylock);
	return(ret);
}

/* Called from main thread only */
int sdl_getch(void)
{
	int ch;

	sdl.SemWait(sdl_key_pending);
	sdl.mutexP(sdl_keylock);

	/* This always frees up space in keybuf for one more char */
	ch=sdl_keybuf[sdl_key++];
	/* If we have missed mouse keys, tack them on to the end of the buffer now */
	if(sdl_pending_mousekeys) {
		if(sdl_pending_mousekeys & 1)	/* Odd number... second char */
	       	sdl_keybuf[sdl_keynext++]=CIO_KEY_MOUSE >> 8;
		else							/* Even number... first char */
	        sdl_keybuf[sdl_keynext++]=CIO_KEY_MOUSE & 0xff;
        sdl.SemPost(sdl_key_pending);
		sdl_pending_mousekeys--;
	}
	sdl.mutexV(sdl_keylock);
	return(ch);
}

/* Called from main thread only */
void sdl_textmode(int mode)
{
	sdl_init_mode(mode);
}

/* Called from main thread only (Passes Event) */
int sdl_setname(const char *name)
{
	sdl_user_func(SDL_USEREVENT_SETNAME,name);
	return(0);
}

/* Called from main thread only (Passes Event) */
int sdl_seticon(const void *icon, unsigned long size)
{
	sdl_user_func(SDL_USEREVENT_SETICON,icon,size);
	return(0);
}

/* Called from main thread only (Passes Event) */
int sdl_settitle(const char *title)
{
	sdl_user_func(SDL_USEREVENT_SETTITLE,title);
	return(0);
}

int sdl_showmouse(void)
{
	sdl_user_func(SDL_USEREVENT_SHOWMOUSE);
	return(1);
}

int sdl_hidemouse(void)
{
	sdl_user_func(SDL_USEREVENT_HIDEMOUSE);
	return(0);
}

int sdl_get_window_info(int *width, int *height, int *xpos, int *ypos)
{
	int ww, wh;

	sdl.mutexP(win_mutex);
	sdl.GetWindowSize(win, &ww, &wh);
	if(width)
		*width=ww;
	if(height)
		*height=wh;
	if(xpos)
		*xpos=-1;
	if(ypos)
		*ypos=-1;
	sdl.mutexV(win_mutex);

	return(1);
}

static void setup_surfaces(void)
{
	int		char_width;
	int		char_height;
	int		flags=0;
	SDL_Event	ev;
	int charwidth, charheight, cols, scaling, rows, vmultiplier;

	if(fullscreen)
		flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
	else
		flags |= SDL_WINDOW_RESIZABLE;

	sdl.mutexP(win_mutex);
	charwidth = cvstat.charwidth;
	charheight = cvstat.charheight;
	cols = cvstat.cols;
	scaling = cvstat.scaling;
	rows = cvstat.rows;
	vmultiplier = cvstat.vmultiplier;
	
	char_width=charwidth*cols*scaling;
	char_height=charheight*rows*scaling*vmultiplier;

	if (win == NULL) {
		// SDL2: This is slow sometimes... not sure why.
		if (sdl.CreateWindowAndRenderer(char_width, char_height, flags, &win, &renderer) == 0) {
			sdl.RenderClear(renderer);
			texture = sdl.CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, charwidth*cols, charheight*rows);
		}
		else {
			win = NULL;
			renderer = NULL;
		}
	}
	else {
		sdl.SetWindowSize(win, char_width, char_height);
	}

	if(win!=NULL) {
		bitmap_drv_request_pixels();
	}
	else if(sdl_init_good) {
		ev.type=SDL_QUIT;
		sdl_exitcode=1;
		sdl.PeepEvents(&ev, 1, SDL_ADDEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT);
	}
	sdl.mutexV(win_mutex);
}

/* Called from event thread only */
static void sdl_add_key(unsigned int keyval)
{
	if(keyval==0xa600) {
		fullscreen=!fullscreen;
		cio_api.mode=fullscreen?CIOLIB_MODE_SDL_FULLSCREEN:CIOLIB_MODE_SDL;
		setup_surfaces();
		return;
	}
	if(keyval <= 0xffff) {
		sdl.mutexP(sdl_keylock);
		if(sdl_keynext+1==sdl_key) {
			beep();
			sdl.mutexV(sdl_keylock);
			return;
		}
		if((sdl_keynext+2==sdl_key) && keyval > 0xff) {
			if(keyval==CIO_KEY_MOUSE)
				sdl_pending_mousekeys+=2;
			else
				beep();
			sdl.mutexV(sdl_keylock);
			return;
		}
		sdl_keybuf[sdl_keynext++]=keyval & 0xff;
		sdl.SemPost(sdl_key_pending);
		if(keyval>0xff) {
			sdl_keybuf[sdl_keynext++]=keyval >> 8;
			sdl.SemPost(sdl_key_pending);
		}
		sdl.mutexV(sdl_keylock);
	}
}

static unsigned int cp437_convert(unsigned int unicode)
{
	if(unicode < 0x80)
		return(unicode);
	switch(unicode) {
		case 0x00c7:
			return(0x80);
		case 0x00fc:
			return(0x81);
		case 0x00e9:
			return(0x82);
		case 0x00e2:
			return(0x83);
		case 0x00e4:
			return(0x84);
		case 0x00e0:
			return(0x85);
		case 0x00e5:
			return(0x86);
		case 0x00e7:
			return(0x87);
		case 0x00ea:
			return(0x88);
		case 0x00eb:
			return(0x89);
		case 0x00e8:
			return(0x8a);
		case 0x00ef:
			return(0x8b);
		case 0x00ee:
			return(0x8c);
		case 0x00ec:
			return(0x8d);
		case 0x00c4:
			return(0x8e);
		case 0x00c5:
			return(0x8f);
		case 0x00c9:
			return(0x90);
		case 0x00e6:
			return(0x91);
		case 0x00c6:
			return(0x92);
		case 0x00f4:
			return(0x93);
		case 0x00f6:
			return(0x94);
		case 0x00f2:
			return(0x95);
		case 0x00fb:
			return(0x96);
		case 0x00f9:
			return(0x97);
		case 0x00ff:
			return(0x98);
		case 0x00d6:
			return(0x99);
		case 0x00dc:
			return(0x9a);
		case 0x00a2:
			return(0x9b);
		case 0x00a3:
			return(0x9c);
		case 0x00a5:
			return(0x9d);
		case 0x20a7:
			return(0x9e);
		case 0x0192:
			return(0x9f);
		case 0x00e1:
			return(0xa0);
		case 0x00ed:
			return(0xa1);
		case 0x00f3:
			return(0xa2);
		case 0x00fa:
			return(0xa3);
		case 0x00f1:
			return(0xa4);
		case 0x00d1:
			return(0xa5);
		case 0x00aa:
			return(0xa6);
		case 0x00ba:
			return(0xa7);
		case 0x00bf:
			return(0xa8);
		case 0x2310:
			return(0xa9);
		case 0x00ac:
			return(0xaa);
		case 0x00bd:
			return(0xab);
		case 0x00bc:
			return(0xac);
		case 0x00a1:
			return(0xad);
		case 0x00ab:
			return(0xae);
		case 0x00bb:
			return(0xaf);
		case 0x2591:
			return(0xb0);
		case 0x2592:
			return(0xb1);
		case 0x2593:
			return(0xb2);
		case 0x2502:
			return(0xb3);
		case 0x2524:
			return(0xb4);
		case 0x2561:
			return(0xb5);
		case 0x2562:
			return(0xb6);
		case 0x2556:
			return(0xb7);
		case 0x2555:
			return(0xb8);
		case 0x2563:
			return(0xb9);
		case 0x2551:
			return(0xba);
		case 0x2557:
			return(0xbb);
		case 0x255d:
			return(0xbc);
		case 0x255c:
			return(0xbd);
		case 0x255b:
			return(0xbe);
		case 0x2510:
			return(0xbf);
		case 0x2514:
			return(0xc0);
		case 0x2534:
			return(0xc1);
		case 0x252c:
			return(0xc2);
		case 0x251c:
			return(0xc3);
		case 0x2500:
			return(0xc4);
		case 0x253c:
			return(0xc5);
		case 0x255e:
			return(0xc6);
		case 0x255f:
			return(0xc7);
		case 0x255a:
			return(0xc8);
		case 0x2554:
			return(0xc9);
		case 0x2569:
			return(0xca);
		case 0x2566:
			return(0xcb);
		case 0x2560:
			return(0xcc);
		case 0x2550:
			return(0xcd);
		case 0x256c:
			return(0xce);
		case 0x2567:
			return(0xcf);
		case 0x2568:
			return(0xd0);
		case 0x2564:
			return(0xd1);
		case 0x2565:
			return(0xd2);
		case 0x2559:
			return(0xd3);
		case 0x2558:
			return(0xd4);
		case 0x2552:
			return(0xd5);
		case 0x2553:
			return(0xd6);
		case 0x256b:
			return(0xd7);
		case 0x256a:
			return(0xd8);
		case 0x2518:
			return(0xd9);
		case 0x250c:
			return(0xda);
		case 0x2588:
			return(0xdb);
		case 0x2584:
			return(0xdc);
		case 0x258c:
			return(0xdd);
		case 0x2590:
			return(0xde);
		case 0x2580:
			return(0xdf);
		case 0x03b1:
			return(0xe0);
		case 0x00df:
			return(0xe1);
		case 0x0393:
			return(0xe2);
		case 0x03c0:
			return(0xe3);
		case 0x03a3:
			return(0xe4);
		case 0x03c3:
			return(0xe5);
		case 0x00b5:
			return(0xe6);
		case 0x03c4:
			return(0xe7);
		case 0x03a6:
			return(0xe8);
		case 0x0398:
			return(0xe9);
		case 0x03a9:
			return(0xea);
		case 0x03b4:
			return(0xeb);
		case 0x221e:
			return(0xec);
		case 0x03c6:
			return(0xed);
		case 0x03b5:
			return(0xee);
		case 0x2229:
			return(0xef);
		case 0x2261:
			return(0xf0);
		case 0x00b1:
			return(0xf1);
		case 0x2265:
			return(0xf2);
		case 0x2264:
			return(0xf3);
		case 0x2320:
			return(0xf4);
		case 0x2321:
			return(0xf5);
		case 0x00f7:
			return(0xf6);
		case 0x2248:
			return(0xf7);
		case 0x00b0:
			return(0xf8);
		case 0x2219:
			return(0xf9);
		case 0x00b7:
			return(0xfa);
		case 0x221a:
			return(0xfb);
		case 0x207f:
			return(0xfc);
		case 0x00b2:
			return(0xfd);
		case 0x25a0:
			return(0xfe);
		case 0x00a0:
			return(0xff);
	}
	return(0x01ffff);
}

/* Called from event thread only */
static unsigned int sdl_get_char_code(unsigned int keysym, unsigned int mod)
{
	int expect;
	int i;

#ifdef __DARWIN__
	if(keysym==0x7f && !(mod & KMOD_CTRL)) {
		keysym=0x08;
		keysym=SDLK_BACKSPACE;
	}
#endif

	/*
	 * No Unicode translation available.
	 * Or there *IS* an SDL keysym.
	 * Or ALT (GUI) pressed
	 */
	// SDL2: This needs to be replaced with... betterness.
	if((keysym > SDLK_UNKNOWN) || (mod & (KMOD_GUI|KMOD_ALT))) {

		/* Find the SDL keysym */
		for(i=0;sdl_keyval[i].keysym;i++) {
			if(sdl_keyval[i].keysym==keysym) {
				/* KeySym found in table */

				/*
				 * Using the modifiers, look up the expected scan code.
				 * Under windows, this is what unicode will be set to
				 * if the ALT key is not AltGr
				 */

				if(mod & KMOD_CTRL)
					expect=sdl_keyval[i].ctrl;
				else if(mod & KMOD_SHIFT) {
					if(mod & KMOD_CAPS)
						expect=sdl_keyval[i].key;
					else
						expect=sdl_keyval[i].shift;
				}
				else {
					if(mod & KMOD_CAPS && (toupper(sdl_keyval[i].key) == sdl_keyval[i].shift))
						expect=sdl_keyval[i].shift;
					else
						expect=sdl_keyval[i].key;
				}

				/*
				 * Now handle the ALT case so that expect will
				 * be what we expect to return
				 */
				if(mod & (KMOD_GUI|KMOD_ALT)) {

					/* Yes, this is a "normal" ALT combo */
					if(keysym==expect || keysym == 0)
						return(sdl_keyval[i].alt);

					/* AltGr apparently... translate unicode or give up */
					return(cp437_convert(keysym));
				}

				/*
				 * If the keysym is a keypad one
				 * AND numlock is locked
				 * AND none of Control, Shift, ALT, or GUI are pressed
				 */
				if(keysym >= SDLK_KP_0 && keysym <= SDLK_KP_EQUALS && 
						(!(mod & (KMOD_CTRL|KMOD_SHIFT|KMOD_ALT|KMOD_GUI) ))) {
#if defined(_WIN32)
					/*
					 * Apparently, Win32 SDL doesn't interpret keypad with numlock...
					 * and doesn't know the state of numlock either...
					 * So, do that here. *sigh*
					 */

					mod &= ~KMOD_NUM;	// Ignore "current" mod state
					if (GetKeyState(VK_NUMLOCK) & 1)
						mod |= KMOD_NUM;
#endif
					if (mod & KMOD_NUM) {
						switch(keysym) {
							case SDLK_KP_0:
								return('0');
							case SDLK_KP_1:
								return('1');
							case SDLK_KP_2:
								return('2');
							case SDLK_KP_3:
								return('3');
							case SDLK_KP_4:
								return('4');
							case SDLK_KP_5:
								return('5');
							case SDLK_KP_6:
								return('6');
							case SDLK_KP_7:
								return('7');
							case SDLK_KP_8:
								return('8');
							case SDLK_KP_9:
								return('9');
							case SDLK_KP_PERIOD:
								return('.');
							case SDLK_KP_DIVIDE:
								return('/');
							case SDLK_KP_MULTIPLY:
								return('*');
							case SDLK_KP_MINUS:
								return('-');
							case SDLK_KP_PLUS:
								return('+');
							case SDLK_KP_ENTER:
								return('\r');
							case SDLK_KP_EQUALS:
								return('=');
						}
					}
				}

				/*
				 * "Extended" keys are always right since we can't compare to unicode
				 * This does *NOT* mean ALT-x, it means things like F1 and Print Screen
				 */
				if(sdl_keyval[i].key > 255)			/* Extended regular key */
					return(expect);

				/*
				 * If there is no unicode translation available,
				 * we *MUST* use our table since we have
				 * no other data to use.  This is apparently
				 * never true on OS X.
				 */
				if(!keysym)
					return(expect);

				/*
				 * At this point, we no longer have a reason to distrust the
				 * unicode mapping.  If we can coerce it into CP437, we will.
				 * If we can't, just give up.
				 */
				return(cp437_convert(expect));
			}
		}
	}
	/*
	 * Well, we can't find it in our table...
	 * If there's a unicode character, use that if possible.
	 */
	if(keysym)
		return(cp437_convert(keysym));

	/*
	 * No unicode... perhaps it's ASCII?
	 * Most likely, this would be a strangely
	 * entered control character.
	 *
	 * If *any* modifier key is down though
	 * we're not going to trust the keysym
	 * value since we can't.
	 */
	if(keysym <= 127 && !(mod & (KMOD_GUI|KMOD_ALT|KMOD_CTRL|KMOD_SHIFT)))
		return(keysym);

	/* Give up.  It's not working out for us. */
	return(0x0001ffff);
}

/* Mouse event/keyboard thread */
static void sdl_mouse_thread(void *data)
{
	SetThreadName("SDL Mouse");
	while(1) {
		if(mouse_wait())
			sdl_add_key(CIO_KEY_MOUSE);
	}
}

static int win_to_text_xpos(int winpos)
{
	int ret;

	ret = winpos/(cvstat.charwidth*cvstat.scaling)+1;
	return ret;
}

static int win_to_text_ypos(int winpos)
{
	int ret;

	ret = winpos/(cvstat.charheight*cvstat.scaling*cvstat.vmultiplier)+1;
	return ret;
}

static void sdl_video_event_thread(void *data)
{
	SDL_Event	ev;
	int			new_scaling = -1;
	int			old_scaling;

	old_scaling = cvstat.scaling;

	char	*driver;
	if((driver = sdl.GetCurrentVideoDriver())!=NULL) {
#if defined(_WIN32)
		if(!strcmp(driver,"directx"))
			sdl_using_directx=TRUE;
#else
		sdl_using_directx=FALSE;
#endif
#if (defined(__MACH__) && defined(__APPLE__))
		if(!strcmp(driver,"Quartz"))
			sdl_using_quartz=TRUE;
#else
		sdl_using_quartz=FALSE;
#endif
#if !defined(NO_X) && defined(__unix__)
		if(!strcmp(driver,"x11"))
			sdl_using_x11=TRUE;
		if(!strcmp(driver,"dga"))
			sdl_using_x11=TRUE;
#else
		sdl_using_x11=FALSE;
#endif
	}

	while(1) {
		if(sdl.WaitEventTimeout(&ev, 1)!=1) {
			if (new_scaling != -1 || cvstat.scaling != old_scaling) {
				if (new_scaling == -1)
					new_scaling = cvstat.scaling;
				sdl_setscaling(new_scaling);
				new_scaling = -1;
				if(cvstat.scaling < 1)
					sdl_setscaling(1);
				setup_surfaces();
				old_scaling = cvstat.scaling;
			}
		}
		else {
			switch (ev.type) {
				case SDL_KEYDOWN:			/* Keypress */
					if ((ev.key.keysym.mod & KMOD_GUI) &&
					    (ev.key.keysym.sym == SDLK_LEFT ||
					     ev.key.keysym.sym == SDLK_RIGHT ||
					     ev.key.keysym.sym == SDLK_UP ||
					     ev.key.keysym.sym == SDLK_DOWN)) {
						int w, h;
						sdl.mutexP(win_mutex);
						sdl.GetWindowSize(win, &w, &h);
						switch(ev.key.keysym.sym) {
							case SDLK_LEFT:
								if (w % (cvstat.charwidth * cvstat.cols)) {
									w = w - w % (cvstat.charwidth * cvstat.cols);
								}
								else {
									w -= (cvstat.charwidth * cvstat.cols);
									if (w < (cvstat.charwidth * cvstat.cols))
										w = cvstat.charwidth * cvstat.cols;
								}
								break;
							case SDLK_RIGHT:
								w = (w - w % (cvstat.charwidth * cvstat.cols)) + (cvstat.charwidth * cvstat.cols);
								break;
							case SDLK_UP:
								if (h % (cvstat.charheight * cvstat.rows * cvstat.vmultiplier)) {
									h = h - h % (cvstat.charheight * cvstat.rows);
								}
								else {
									h -= (cvstat.charheight * cvstat.rows * cvstat.vmultiplier);
									if (h < (cvstat.charheight * cvstat.rows * cvstat.vmultiplier))
										h = cvstat.charheight * cvstat.rows * cvstat.vmultiplier;
								}
								break;
							case SDLK_DOWN:
								h = (h - h % (cvstat.charheight * cvstat.rows * cvstat.vmultiplier)) + (cvstat.charheight * cvstat.rows * cvstat.vmultiplier);
								break;
						}
						sdl.SetWindowSize(win, w, h);
						sdl.mutexP(win_mutex);
					}
					else
						sdl_add_key(sdl_get_char_code(ev.key.keysym.sym, ev.key.keysym.mod));
					break;
				case SDL_KEYUP:				/* Ignored (handled in KEYDOWN event) */
					break;
				case SDL_MOUSEMOTION:
					if(!ciolib_mouse_initialized)
						break;
					ciomouse_gotevent(CIOLIB_MOUSE_MOVE,win_to_text_xpos(ev.motion.x),win_to_text_ypos(ev.motion.y));
					break;
				case SDL_MOUSEBUTTONDOWN:
					if(!ciolib_mouse_initialized)
						break;
					switch(ev.button.button) {
						case SDL_BUTTON_LEFT:
							ciomouse_gotevent(CIOLIB_BUTTON_PRESS(1),win_to_text_xpos(ev.button.x),win_to_text_ypos(ev.button.y));
							break;
						case SDL_BUTTON_MIDDLE:
							ciomouse_gotevent(CIOLIB_BUTTON_PRESS(2),win_to_text_xpos(ev.button.x),win_to_text_ypos(ev.button.y));
							break;
						case SDL_BUTTON_RIGHT:
							ciomouse_gotevent(CIOLIB_BUTTON_PRESS(3),win_to_text_xpos(ev.button.x),win_to_text_ypos(ev.button.y));
							break;
					}
					break;
				case SDL_MOUSEBUTTONUP:
					if(!ciolib_mouse_initialized)
						break;
					switch(ev.button.button) {
						case SDL_BUTTON_LEFT:
							ciomouse_gotevent(CIOLIB_BUTTON_RELEASE(1),win_to_text_xpos(ev.button.x),win_to_text_ypos(ev.button.y));
							break;
						case SDL_BUTTON_MIDDLE:
							ciomouse_gotevent(CIOLIB_BUTTON_RELEASE(2),win_to_text_xpos(ev.button.x),win_to_text_ypos(ev.button.y));
							break;
						case SDL_BUTTON_RIGHT:
							ciomouse_gotevent(CIOLIB_BUTTON_RELEASE(3),win_to_text_xpos(ev.button.x),win_to_text_ypos(ev.button.y));
							break;
					}
					break;
				case SDL_QUIT:
					/*
					 * SDL2: Do we still need the reaper?
					 * This is what exit()s programs when the
					 * X is hit.
					 */
					if (ciolib_reaper)
						sdl_user_func(SDL_USEREVENT_QUIT);
					else
						sdl_add_key(CIO_KEY_QUIT);
					break;
				case SDL_WINDOWEVENT:
					switch(ev.window.event) {
						case SDL_WINDOWEVENT_SIZE_CHANGED:
							// SDL2: User resized window
						case SDL_WINDOWEVENT_RESIZED:
							{
								// SDL2: Something resized window
								const char *newh;
								if(ev.window.data1 > 0 && ev.window.data2 > 0) {
									new_scaling = (int)(ev.window.data1/(cvstat.charwidth*cvstat.cols));
								}
								if ((ev.window.data1 % (cvstat.charwidth * cvstat.cols)) || (ev.window.data2 % (cvstat.charheight * cvstat.rows)))
									newh = "2";
								else
									newh = "0";
								sdl.mutexP(win_mutex);
								if (strcmp(newh, sdl.GetHint(SDL_HINT_RENDER_SCALE_QUALITY))) {
									sdl.SetHint(SDL_HINT_RENDER_SCALE_QUALITY, newh );
									sdl.DestroyTexture(texture);
									texture = sdl.CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, cvstat.charwidth*cvstat.cols, cvstat.charheight*cvstat.rows);
									bitmap_drv_request_pixels();
								}
								sdl.mutexV(win_mutex);
								break;
							}
						case SDL_WINDOWEVENT_EXPOSED:
							{
								sdl.mutexP(win_mutex);
								sdl.RenderCopy(renderer, texture, NULL, NULL);
								sdl.RenderPresent(renderer);
								sdl.mutexV(win_mutex);
							}
							break;
					}
					break;
				case SDL_USEREVENT: {
					struct rectlist *list;
					struct rectlist *old_next;
					/* Tell SDL to do various stuff... */
					if (ev.user.code != SDL_USEREVENT_FLUSH)
						if(sdl_x11available && sdl_using_x11)
							sdl.SemPost(sdl_ufunc_rec);
					switch(ev.user.code) {
						case SDL_USEREVENT_QUIT:
							sdl_ufunc_retval=0;
							if (ciolib_reaper)
								exit(0);
							sdl.SemPost(sdl_ufunc_ret);
							return;
						case SDL_USEREVENT_FLUSH:
							sdl.mutexP(win_mutex);
							if (win != NULL) {
								sdl.mutexP(sdl_headlock);
								list = update_list;
								update_list = update_list_tail = NULL;
								sdl.mutexV(sdl_headlock);
								for (; list; list = old_next) {
									SDL_Rect src;

									old_next = list->next;
									if (list->next == NULL) {
										src.x = 0;
										src.y = 0;
										src.w = list->rect.width;
										src.h = list->rect.height;
										sdl.UpdateTexture(texture, &src, list->data, list->rect.width * sizeof(uint32_t));
										sdl.RenderCopy(renderer, texture, &src, NULL);
									}
									bitmap_drv_free_rect(list);
								}
								sdl.RenderPresent(renderer);
							}
							sdl.mutexV(win_mutex);
							break;
						case SDL_USEREVENT_SETNAME:
							sdl.mutexP(win_mutex);
							sdl.SetWindowTitle(win, (char *)ev.user.data1);
							sdl.mutexV(win_mutex);
							free(ev.user.data1);
							break;
						case SDL_USEREVENT_SETICON:
							if(sdl_icon != NULL)
								sdl.FreeSurface(sdl_icon);
							sdl_icon=sdl.CreateRGBSurfaceFrom(ev.user.data1
									, *(unsigned long *)ev.user.data2
									, *(unsigned long *)ev.user.data2
									, 32
									, *(unsigned long *)ev.user.data2*4
									, *(DWORD *)"\377\0\0\0"
									, *(DWORD *)"\0\377\0\0"
									, *(DWORD *)"\0\0\377\0"
									, *(DWORD *)"\0\0\0\377"
							);
							sdl.mutexP(win_mutex);
							sdl.SetWindowIcon(win, sdl_icon);
							sdl.mutexV(win_mutex);
							free(ev.user.data2);
							break;
						case SDL_USEREVENT_SETTITLE:
							sdl.mutexP(win_mutex);
							sdl.SetWindowTitle(win, (char *)ev.user.data1);
							sdl.mutexV(win_mutex);
							free(ev.user.data1);
							break;
						case SDL_USEREVENT_SETVIDMODE:
							new_scaling = -1;
							old_scaling = cvstat.scaling;
							setup_surfaces();
							sdl_ufunc_retval=0;
							sdl.SemPost(sdl_ufunc_ret);
							break;
						case SDL_USEREVENT_HIDEMOUSE:
							sdl.ShowCursor(SDL_DISABLE);
							break;
						case SDL_USEREVENT_SHOWMOUSE:
							sdl.ShowCursor(SDL_ENABLE);
							break;
						case SDL_USEREVENT_INIT:
							if(!sdl_init_good) {
								if(sdl.WasInit(SDL_INIT_VIDEO)==SDL_INIT_VIDEO) {
									sdl.mutexP(win_mutex);
									_beginthread(sdl_mouse_thread, 0, NULL);
									sdl_init_good=1;
									sdl.mutexV(win_mutex);
								}
							}
							sdl_ufunc_retval=0;
							sdl.SemPost(sdl_ufunc_ret);
							break;
						case SDL_USEREVENT_COPY:
#if !defined(NO_X) && defined(__unix__) && defined(SDL_VIDEO_DRIVER_X11)
							if(sdl_x11available && sdl_using_x11) {
								SDL_SysWMinfo	wmi;

								SDL_VERSION(&(wmi.version));
								sdl.mutexP(win_mutex);
								sdl.GetWindowWMInfo(win, &wmi);
								sdl.mutexV(win_mutex);
								sdl.EventState(SDL_SYSWMEVENT, SDL_ENABLE);
								copy_needs_events = 1;
								sdl_x11.XSetSelectionOwner(wmi.info.x11.display, CONSOLE_CLIPBOARD, wmi.info.x11.window, CurrentTime);
							}
#endif
							break;
						case SDL_USEREVENT_PASTE:
#if !defined(NO_X) && defined(__unix__) && defined(SDL_VIDEO_DRIVER_X11)
							if(sdl_x11available && sdl_using_x11) {
								Window sowner=None;
								SDL_SysWMinfo	wmi;

								SDL_VERSION(&(wmi.version));
								sdl.mutexP(win_mutex);
								sdl.GetWindowWMInfo(win, &wmi);
								sdl.mutexV(win_mutex);

								paste_needs_events = 1;
								sdl.EventState(SDL_SYSWMEVENT, SDL_ENABLE);
								sowner=sdl_x11.XGetSelectionOwner(wmi.info.x11.display, CONSOLE_CLIPBOARD);
								if(sowner==wmi.info.x11.window) {
									/* Get your own primary selection */
									if(sdl_copybuf==NULL) {
										FREE_AND_NULL(sdl_pastebuf);
									}
									else
										sdl_pastebuf=(char *)malloc(strlen(sdl_copybuf)+1);
									if(sdl_pastebuf!=NULL)
										strcpy(sdl_pastebuf,sdl_copybuf);
									/* Set paste buffer */
									sdl.SemPost(sdl_pastebuf_set);
									sdl.SemWait(sdl_pastebuf_copied);
									FREE_AND_NULL(sdl_pastebuf);
									paste_needs_events = 0;
									if (!copy_needs_events)
										sdl.EventState(SDL_SYSWMEVENT, SDL_DISABLE);
								}
								else if(sowner!=None) {
									sdl_x11.XConvertSelection(wmi.info.x11.display, CONSOLE_CLIPBOARD, XA_STRING, XA_STRING, wmi.info.x11.window, CurrentTime);
								}
								else {
									/* Set paste buffer */
									FREE_AND_NULL(sdl_pastebuf);
									sdl.SemPost(sdl_pastebuf_set);
									sdl.SemWait(sdl_pastebuf_copied);
									paste_needs_events = 0;
									if (!copy_needs_events)
										sdl.EventState(SDL_SYSWMEVENT, SDL_DISABLE);
								}
							}
#endif
							break;
					}
					break;
				}
				case SDL_SYSWMEVENT:			/* ToDo... This is where Copy/Paste needs doing */
				// SDL2: There's a better way...
#if !defined(NO_X) && defined(__unix__) && defined(SDL_VIDEO_DRIVER_X11)
					if(sdl_x11available && sdl_using_x11) {
						XEvent *e;
						e=&ev.syswm.msg->msg.x11.event;
						switch(e->type) {
							case SelectionClear: {
									XSelectionClearEvent *req;

									req=&(e->xselectionclear);
									sdl.mutexP(sdl_copybuf_mutex);
									if(req->selection==CONSOLE_CLIPBOARD) {
										FREE_AND_NULL(sdl_copybuf);
									}
									sdl.mutexV(sdl_copybuf_mutex);
									copy_needs_events = 0;
									if (!paste_needs_events)
										sdl.EventState(SDL_SYSWMEVENT, SDL_DISABLE);
									break;
							}
							case SelectionNotify: {
									int format=0;
									unsigned long len, bytes_left, dummy;
									Atom type;
									XSelectionEvent *req;
									SDL_SysWMinfo	wmi;

									SDL_VERSION(&(wmi.version));
									sdl.mutexP(win_mutex);
									sdl.GetWindowWMInfo(win, &wmi);
									sdl.mutexV(win_mutex);
									req=&(e->xselection);
									if(req->requestor!=wmi.info.x11.window)
										break;
									if(req->property) {
										sdl_x11.XGetWindowProperty(wmi.info.x11.display, wmi.info.x11.window, req->property, 0, 0, 0, AnyPropertyType, &type, &format, &len, &bytes_left, (unsigned char **)(&sdl_pastebuf));
										if(bytes_left > 0 && format==8)
											sdl_x11.XGetWindowProperty(wmi.info.x11.display, wmi.info.x11.window, req->property,0,bytes_left,0,AnyPropertyType,&type,&format,&len,&dummy,(unsigned char **)&sdl_pastebuf);
										else {
											FREE_AND_NULL(sdl_pastebuf);
										}
									}
									else {
										FREE_AND_NULL(sdl_pastebuf);
									}

									/* Set paste buffer */
									sdl.SemPost(sdl_pastebuf_set);
									sdl.SemWait(sdl_pastebuf_copied);
									paste_needs_events = 0;
									if (!copy_needs_events)
										sdl.EventState(SDL_SYSWMEVENT, SDL_DISABLE);
									if(sdl_pastebuf!=NULL) {
										sdl_x11.XFree(sdl_pastebuf);
										sdl_pastebuf=NULL;
									}
									break;
							}
							case SelectionRequest: {
									XSelectionRequestEvent *req;
									XEvent respond;

									req=&(e->xselectionrequest);
									sdl.mutexP(sdl_copybuf_mutex);
									if(sdl_copybuf==NULL) {
										respond.xselection.property=None;
									}
									else {
										if(req->target==XA_STRING) {
											sdl_x11.XChangeProperty(req->display, req->requestor, req->property, XA_STRING, 8, PropModeReplace, (unsigned char *)sdl_copybuf, strlen(sdl_copybuf));
											respond.xselection.property=req->property;
										}
										else
											respond.xselection.property=None;
									}
									sdl.mutexV(sdl_copybuf_mutex);
									respond.xselection.type=SelectionNotify;
									respond.xselection.display=req->display;
									respond.xselection.requestor=req->requestor;
									respond.xselection.selection=req->selection;
									respond.xselection.target=req->target;
									respond.xselection.time=req->time;
									sdl_x11.XSendEvent(req->display,req->requestor,0,0,&respond);
									break;
							}
						}	/* switch */
					}	/* usingx11 */
#endif

				/* Ignore this stuff */
				case SDL_JOYAXISMOTION:
				case SDL_JOYBALLMOTION:
				case SDL_JOYHATMOTION:
				case SDL_JOYBUTTONDOWN:
				case SDL_JOYBUTTONUP:
				default:
					break;
			}
		}
	}
	return;
}

int sdl_initciolib(int mode)
{
	if(init_sdl_video()) {
		fprintf(stderr,"SDL Video Initialization Failed\n");
		return(-1);
	}
	sdl_key_pending=sdl.SDL_CreateSemaphore(0);
	sdl_ufunc_ret=sdl.SDL_CreateSemaphore(0);
	sdl_ufunc_rec=sdl.SDL_CreateSemaphore(0);
	sdl_ufunc_mtx=sdl.SDL_CreateMutex();
	sdl_headlock=sdl.SDL_CreateMutex();
	win_mutex=sdl.SDL_CreateMutex();
	sdl_keylock=sdl.SDL_CreateMutex();
#if !defined(NO_X) && defined(__unix__)
	sdl_pastebuf_set=sdl.SDL_CreateSemaphore(0);
	sdl_pastebuf_copied=sdl.SDL_CreateSemaphore(0);
	sdl_copybuf_mutex=sdl.SDL_CreateMutex();
#endif
	return(sdl_init(mode));
}
