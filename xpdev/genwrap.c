/* genwrap.c */

/* General cross-platform development wrappers */

/* $Id: genwrap.c,v 1.12 2002/04/25 22:47:23 rswindell Exp $ */

/****************************************************************************
 * @format.tab-size 4		(Plain Text/Source Code File Header)			*
 * @format.use-tabs true	(see http://www.synchro.net/ptsc_hdr.html)		*
 *																			*
 * Copyright 2002 Rob Swindell - http://www.synchro.net/copyright.html		*
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

#include <string.h>     /* strlen() */
#include <stdlib.h>		/* RAND_MAX */
#include <fcntl.h>		/* O_NOCTTY */
#include <time.h>		/* clock() */
#include <errno.h>		/* errno */
#include <ctype.h>		/* toupper/tolower */

#if defined(__unix__)
	#include <sys/ioctl.h>		/* ioctl() */
	#include <sys/utsname.h>	/* uname() */
	/* KIOCSOUND */
	#if defined(__FreeBSD__)
		#include <sys/kbio.h>
	#else
		#include <sys/kd.h>	
	#endif
#endif	/* __unix__ */

#include "genwrap.h"	/* Verify prototypes */

/****************************************************************************/
/* Convert ASCIIZ string to upper case										*/
/****************************************************************************/
#if defined(__unix__)
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
#if defined(__unix__)
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
int DLLCALL xp_random(int n)
{
	float f;
	static BOOL initialized;

	if(!initialized) {
		srand(time(NULL));	/* seed random number generator */
		rand();				/* throw away first result */
		initialized=TRUE;
	}
	if(n<2)
		return(0);
	f=(float)rand()/(float)RAND_MAX;

	return((int)(n*f));
}

/****************************************************************************/
/* Return ASCII string representation of ulong								*/
/* There may be a native GNU C Library function to this...					*/
/****************************************************************************/
#if !defined _MSC_VER && !defined __BORLANDC__
char* DLLCALL ultoa(ulong val, char* str, int radix)
{
	switch(radix) {
		case 8:
			sprintf(str,"%lo",val);
			break;
		case 10:
			sprintf(str,"%lu",val);
			break;
		case 16:
			sprintf(str,"%lx",val);
			break;
		default:
			sprintf(str,"bad radix: %d",radix);
			break;
	}
	return(str);
}
#endif

/****************************************************************************/
/* Write the version details of the current operating system into str		*/
/****************************************************************************/
char* DLLCALL os_version(char *str)
{
#if defined(__OS2__) && defined(__BORLANDC__)

	sprintf(str,"OS/2 %u.%u (%u.%u)",_osmajor/10,_osminor/10,_osmajor,_osminor);

#elif defined(_WIN32)

	/* Windows Version */
	char*			winflavor="";
	OSVERSIONINFO	winver;

	winver.dwOSVersionInfoSize=sizeof(winver);
	GetVersionEx(&winver);

	switch(winver.dwPlatformId) {
		case VER_PLATFORM_WIN32_NT:
			winflavor="NT ";
			break;
		case VER_PLATFORM_WIN32s:
			winflavor="Win32s ";
			break;
		case VER_PLATFORM_WIN32_WINDOWS:
			winver.dwBuildNumber&=0xffff;
			break;
	}

	sprintf(str,"Windows %sVersion %u.%02u (Build %u) %s"
			,winflavor
			,winver.dwMajorVersion, winver.dwMinorVersion
			,winver.dwBuildNumber,winver.szCSDVersion);

#elif defined(__unix__)

	struct utsname unixver;

	if(uname(&unixver)!=0)
		sprintf(str,"Unix (uname errno: %d)",errno);
	else
		sprintf(str,"%s %s %s"
			,unixver.sysname	/* e.g. "Linux" */
			,unixver.release	/* e.g. "2.2.14-5.0" */
			,unixver.machine	/* e.g. "i586" */
			);

#else	/* DOS */

	sprintf(str,"DOS %u.%02u",_osmajor,_osminor);

#endif

	return(str);
}
