/* websrvr.h */

/* Synchronet Web Server */

/* $Id: websrvr.h,v 1.22 2004/10/13 19:58:38 rswindell Exp $ */

/****************************************************************************
 * @format.tab-size 4		(Plain Text/Source Code File Header)			*
 * @format.use-tabs true	(see http://www.synchro.net/ptsc_hdr.html)		*
 *																			*
 * Copyright 2004 Rob Swindell - http://www.synchro.net/copyright.html		*
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

#ifndef _WEBSRVR_H_
#define _WEBSRVR_H_

#include "client.h"				/* client_t */
#include "startup.h"			/* BBS_OPT_* */
#include "semwrap.h"			/* sem_t */

typedef struct {
	DWORD	size;				/* sizeof(web_startup_t) */
	WORD	port;
	WORD	max_clients;
	WORD	max_inactivity;
	WORD	max_cgi_inactivity;
	WORD	sem_chk_freq;		/* semaphore file checking frequency (in seconds) */
    DWORD   interface_addr;
    DWORD	options;
    DWORD	js_max_bytes;
	DWORD	js_cx_stack;

	void*	cbdata;				/* Private data passed to callbacks */ 

	/* Callbacks (NULL if unused) */
	int 	(*lputs)(void*, int, char*);
	void	(*status)(void*, char*);
    void	(*started)(void*);
    void	(*terminated)(void*, int code);
    void	(*clients)(void*, int active);
    void	(*thread_up)(void*, BOOL up, BOOL setuid);
	void	(*socket_open)(void*, BOOL open);
    void	(*client_on)(void*, BOOL on, int sock, client_t*, BOOL update);
    BOOL	(*seteuid)(BOOL user);
	BOOL	(*setuid)(BOOL);

	/* Paths */
	char	ssjs_ext[16];			/* Server-Side JavaScript file extension */
	char**	cgi_ext;				/* CGI Extensions */
	char	cgi_dir[128];			/* relative to root_dir (all files executable) */
    char    ctrl_dir[128];
    char	root_dir[128];			/* HTML root directory */
    char	error_dir[128];			/* relative to root_dir */
    char	cgi_temp_dir[128];
    char**	index_file_name;		/* Index filenames */
	char	logfile_base[128];	/* Logfile base name (date is appended) */

	/* Misc */
    char	host_name[128];
	BOOL	recycle_now;
	sem_t	recycle_sem;
	DWORD	log_mask;

} web_startup_t;

#define WEB_OPT_DEBUG_RX			(1<<0)	/* Log all received requests		*/
#define WEB_OPT_DEBUG_TX			(1<<1)	/* Log all transmitted responses	*/
#define WEB_OPT_VIRTUAL_HOSTS		(1<<4)	/* Use virutal host html subdirs	*/
#define WEB_OPT_NO_CGI				(1<<5)	/* Disable CGI support				*/
#define WEB_OPT_HTTP_LOGGING		(1<<6)	/* Create/write-to HttpLogFile		*/

#define WEB_DEFAULT_ROOT_DIR		"../html"
#define WEB_DEFAULT_ERROR_DIR		"error"
#define WEB_DEFAULT_CGI_DIR			"cgi-bin"

#ifdef DLLEXPORT
#undef DLLEXPORT
#endif
#ifdef DLLCALL
#undef DLLCALL
#endif

#ifdef _WIN32
	#ifdef WEBSRVR_EXPORTS
		#define DLLEXPORT __declspec(dllexport)
	#else
		#define DLLEXPORT __declspec(dllimport)
	#endif
	#ifdef __BORLANDC__
		#define DLLCALL __stdcall
	#else
		#define DLLCALL
	#endif
#else
	#define DLLEXPORT
	#define DLLCALL
#endif

#ifdef __cplusplus
extern "C" {
#endif
DLLEXPORT void			DLLCALL web_server(void* arg);
DLLEXPORT void			DLLCALL web_terminate(void);
DLLEXPORT const char*	DLLCALL web_ver(void);
#ifdef __cplusplus
}
#endif

#endif /* Don't add anything after this line */
