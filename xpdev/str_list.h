/* str_list.h */

/* Functions to deal with NULL-terminated string lists */

/* $Id: str_list.h,v 1.3 2004/05/11 19:28:30 rswindell Exp $ */

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

#ifndef _STR_LIST_H
#define _STR_LIST_H

#include "gen_defs.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef char** str_list_t;

/* Returns an allocated and terminated string list */
str_list_t	strListAlloc(void);

/* Frees the strings in the list (and the list itself) */
void		strListFree(str_list_t* list);

/* Pass a pointer to a string list, the string to add */
/* Returns the updated list or NULL on error */
str_list_t	strListAdd(str_list_t* list, char* str);

/* Adds a string into the list at a specific index */
str_list_t	strListAddAt(str_list_t* list, char* str, size_t index);

/* Count the number of strings in the list and returns the count */
size_t		strListCount(str_list_t list);

#if defined(__cplusplus)
}
#endif

#endif	/* Don't add anything after this line */
