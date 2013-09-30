/* Copyright (C), 2007 by Stephen Hurd */

/* $Id: uifcinit.h,v 1.10 2008/02/18 01:55:20 deuce Exp $ */

#ifndef _UIFCINIT_H_
#define _UIFCINIT_H_

#include <uifc.h>

extern	uifcapi_t uifc; /* User Interface (UIFC) Library API */
extern	int uifc_old_font;	/* Font to return to after exiting uifc */
int	init_uifc(BOOL scrn, BOOL bottom);
void uifcbail(void);
void uifcmsg(char *msg, char *helpbuf);
void uifcinput(char *title, int len, char *msg, int mode, char *helpbuf);
int confirm(char *msg, char *helpbuf);

#endif
