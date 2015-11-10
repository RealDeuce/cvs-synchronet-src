/* ver.cpp */

/* Synchronet version display */

/* $Id: ver.cpp,v 1.27 2015/11/10 22:53:28 deuce Exp $ */

/****************************************************************************
 * @format.tab-size 4		(Plain Text/Source Code File Header)			*
 * @format.use-tabs true	(see http://www.synchro.net/ptsc_hdr.html)		*
 *																			*
 * Copyright 2010 Rob Swindell - http://www.synchro.net/copyright.html		*
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

#include "sbbs.h"

const char* beta_version = " "; /* Space if non-beta, " beta" otherwise */

#if defined(_WINSOCKAPI_)
	extern WSADATA WSAData;
	#define SOCKLIB_DESC WSAData.szDescription
#else
	#define	SOCKLIB_DESC NULL
#endif

#if defined(__unix__)
	#include <sys/utsname.h>	/* uname() */
#endif

char* socklib_version(char* str, char* winsock_ver)
{
#if defined(_WINSOCKAPI_)

	strcpy(str,winsock_ver);

#elif defined(__GLIBC__)

	sprintf(str,"GLIBC %u.%u",__GLIBC__,__GLIBC_MINOR__);

#else
	
	strcpy(str,"No socket library version available");

#endif

	return(str);
}

#ifndef JSDOOR
void sbbs_t::ver()
{
	char str[128],compiler[32];

	CRLF;
	strcpy(str,VERSION_NOTICE);
#if defined(_DEBUG)
	strcat(str,"  Debug");
#endif
	center(str);
	CRLF;

	DESCRIBE_COMPILER(compiler);

	sprintf(str,"Revision %c%s %s %.5s  "
		"SMBLIB %s  %s"
		,toupper(REVISION)
		,beta_version
		,__DATE__,__TIME__
		,smb_lib_ver(),compiler);

	center(str);
	CRLF;

	sprintf(str,"%s - http://www.synchro.net", COPYRIGHT_NOTICE);
	center(str);
	CRLF;

#ifdef JAVASCRIPT
	if(!(startup->options&BBS_OPT_NO_JAVASCRIPT)) {
		center((char *)JS_GetImplementationVersion());
		CRLF;
	}
#endif

	center(socklib_version(str,SOCKLIB_DESC));
	CRLF;

	center(os_version(str));
}
#endif
