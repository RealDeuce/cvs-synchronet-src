/* sbbs_ini.h */

/* Synchronet console configuration (.ini) file routines */

/* $Id: sbbs_ini.h,v 1.8 2004/05/28 23:43:31 rswindell Exp $ */

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

#ifndef _SBBS_INI_H
#define _SBBS_INI_H

#include "startup.h"	/* bbs_startup_t */
#include "ftpsrvr.h"	/* ftp_startup_t */
#include "mailsrvr.h"	/* mail_startup_t */
#include "services.h"	/* services_startup_t */
#include "websrvr.h"	/* services_startup_t */
#include "ini_file.h"

#define strJavaScriptMaxBytes		"JavaScriptMaxBytes"
#define strJavaScriptContextStack	"JavaScriptContextStack"
#define strJavaScriptBranchLimit	"JavaScriptBranchLimit"
#define strJavaScriptGcInterval		"JavaScriptGcInterval"
#define strJavaScriptYieldInterval	"JavaScriptYieldInterval"
#define strSemFileCheckFrequency	"SemFileCheckFrequency"

#if defined(__cplusplus)
extern "C" {
#endif

void sbbs_get_ini_fname(
	 char* ini_file
	,char* ctrl_dir
	,char* host_name
	);

void sbbs_read_ini(
	 FILE*					fp
	,global_startup_t*		global
	,BOOL*					run_bbs
	,bbs_startup_t*			bbs_startup
	,BOOL*					run_ftp
	,ftp_startup_t*			ftp_startup
	,BOOL*					run_web
	,web_startup_t*			web_startup
	,BOOL*					run_mail		
	,mail_startup_t*		mail_startup
	,BOOL*					run_services
	,services_startup_t*	services_startup
	);

BOOL sbbs_write_ini(
	 FILE*					fp
	,global_startup_t*		global
	,BOOL					run_bbs
	,bbs_startup_t*			bbs
	,BOOL					run_ftp
	,ftp_startup_t*			ftp
	,BOOL					run_web
	,web_startup_t*			web
	,BOOL					run_mail		
	,mail_startup_t*		mail
	,BOOL					run_services
	,services_startup_t*	services
	);


#if defined(__cplusplus)
}
#endif

#endif	/* Don't add anything after this line */
