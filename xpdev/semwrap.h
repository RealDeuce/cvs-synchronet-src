/* semwrap.h */

/* Semaphore-related cross-platform development wrappers */

/* $Id: semwrap.h,v 1.5 2004/09/10 08:36:46 rswindell Exp $ */

/****************************************************************************
 * @format.tab-size 4		(Plain Text/Source Code File Header)			*
 * @format.use-tabs true	(see http://www.synchro.net/ptsc_hdr.html)		*
 *																			*
 * Copyright 2004 Rob Swindell - http://www.synchro.net/copyright.html		*
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

#ifndef _SEMWRAP_H
#define _SEMWRAP_H

#include "gen_defs.h"

#if defined(__cplusplus)
extern "C" {
#endif

#if defined(__unix__)
	#if defined(USE_XP_SEMAPHORES)
		#include "xpsem.h"
		#define 	sem_init(x, y, z)	xp_sem_init(x, y, z)
		#define 	sem_destroy(x)		xp_sem_destroy(x)
		#define 	sem_close(x)		xp_sem_close(x)
		#define 	sem_unlink(x)		xp_sem_unlink(x)
		#define 	sem_wait(x)			xp_sem_wait(x)
		#define 	sem_trywait(x)		xp_sem_trywait(x)
		#define 	sem_post(x)			xp_sem_post(x)
		#define 	sem_getvalue(x)		xp_sem_getvalue(x)
		#define		sem_timedwait(x,y)	xp_sem_timedwait(x,y)
		#define		sem_t				xp_sem_t
	#else
		#include <semaphore.h>	/* POSIX semaphores */
	#endif

	/* NOT POSIX */
	int 	sem_trywait_block(sem_t *sem, unsigned long timeout);

#elif defined(_WIN32)	

	#include <process.h>	/* _beginthread */
	#include <limits.h>		/* INT_MAX */
	#include <errno.h>		/* EAGAIN and EBUSY */

	/* POSIX semaphores */
	typedef HANDLE sem_t;
	#define sem_init(psem,ps,v)			*(psem)=CreateSemaphore(NULL,v,INT_MAX,NULL)
	#define sem_wait(psem)				WaitForSingleObject(*(psem),INFINITE)
	#define sem_trywait(psem)			(WaitForSingleObject(*(psem),0)==WAIT_OBJECT_0?0:(errno=EAGAIN,-1))
	#define sem_post(psem)				ReleaseSemaphore(*(psem),1,NULL)
	#define sem_destroy(psem)			CloseHandle(*(psem))
	/* No Win32 implementation for sem_getvalue() */

	/* NOT POSIX */
	#define sem_trywait_block(psem,t)	(WaitForSingleObject(*(psem),t)==WAIT_OBJECT_0?0:(errno=EAGAIN,-1))

#elif defined(__OS2__)	/* These have *not* been tested! */

	/* POSIX semaphores */
	typedef HEV sem_t;
	#define	sem_init(psem,ps,v)			DosCreateEventSem(NULL,psem,0,0);
	#define sem_wait(psem)				DosWaitEventSem(*(psem),-1)
	#define sem_post(psem)				DosPostEventSem(*(psem))
	#define sem_destroy(psem)			DosCloseEventSem(*(psem))

#else

	#error "Need semaphore wrappers."

#endif

/* Change semaphore to "unsignaled" (NOT POSIX) */
#define sem_reset(psem)					while(sem_trywait(psem)==0)

#if defined(__cplusplus)
}
#endif

#endif	/* Don't add anything after this line */
