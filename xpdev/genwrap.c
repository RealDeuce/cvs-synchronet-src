/* genwrap.c */

/* General cross-platform development wrappers */

/* $Id: genwrap.c,v 1.5 2002/04/06 08:46:51 rswindell Exp $ */

/****************************************************************************
 * @format.tab-size 4		(Plain Text/Source Code File Header)			*
 * @format.use-tabs true	(see http://www.synchro.net/ptsc_hdr.html)		*
 *																			*
 * Copyright 2000 Rob Swindell - http://www.synchro.net/copyright.html		*
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

#include <string.h>     /* strlen() */
#include <stdlib.h>	/* RAND_MAX */
#include <fcntl.h>	/* O_NOCTTY */

#ifdef __unix__
#ifndef __FreeBSD__
#include <sys/kd.h>	/* KIOCSOUND */
#endif
#ifdef __FreeBSD__
#include <sys/kbio.h>
#endif
#endif	/* __unix__ */

#include "genwrap.h"	/* Verify prototypes */

/****************************************************************************/
/* Convert ASCIIZ string to upper case										*/
/****************************************************************************/
#ifdef __unix__
char* DLLCALL strupr(char* str)
{
	char*	p=str;

	while(*p) {
		*p=toupper(*p);
		p++;
	}
	return(str);
}
/****************************************************************************/
/* Convert ASCIIZ string to lower case										*/
/****************************************************************************/
char* DLLCALL strlwr(char* str)
{
	char*	p=str;

	while(*p) {
		*p=tolower(*p);
		p++;
	}
	return(str);
}
/****************************************************************************/
/* Reverse characters of a string (provided by amcleod)						*/
/****************************************************************************/
char* strrev(char* str)
{
    char t, *i=str, *j=str+strlen(str);

    while (i<j) {
        t=*i; *(i++)=*(--j); *j=t;
    }
    return str;
}
#endif

/****************************************************************************/
/* Generate a tone at specified frequency for specified milliseconds		*/
/* Thanks to Casey Martin for this code										*/
/****************************************************************************/
#ifdef __unix__
void DLLCALL unix_beep(int freq, int dur)
{
	static int console_fd=-1;

	if(console_fd == -1) 
  		console_fd = open("/dev/console", O_NOCTTY);
	
	if(console_fd != -1) {
		ioctl(console_fd, KIOCSOUND, (int) (1193180 / freq));
		SLEEP(dur);
		ioctl(console_fd, KIOCSOUND, 0);	/* turn off tone */
	}
}
#endif

/****************************************************************************/
/* Return random number between 0 and n-1									*/
/****************************************************************************/
#ifndef __BORLANDC__
int DLLCALL xp_random(int n)
{
	float f;

	if(n<2)
		return(0);
	f=(float)rand()/(float)RAND_MAX;

	return((int)(n*f));
}
#endif

