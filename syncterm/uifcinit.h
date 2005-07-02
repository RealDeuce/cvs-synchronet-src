/* $Id: uifcinit.h,v 1.5 2005/06/18 23:26:20 deuce Exp $ */

#ifndef _UIFCINIT_H_
#define _UIFCINIT_H_

#include <uifc.h>

extern	uifcapi_t uifc; /* User Interface (UIFC) Library API */
extern int uifc_initialized;
int	init_uifc(BOOL scrn, BOOL bottom);
void uifcbail(void);
void uifcmsg(char *msg, char *helpbuf);

#endif
