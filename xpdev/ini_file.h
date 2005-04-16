/* ini_file.h */

/* Functions to parse ini files */

/* $Id: ini_file.h,v 1.28 2005/04/16 01:22:34 rswindell Exp $ */

/****************************************************************************
 * @format.tab-size 4		(Plain Text/Source Code File Header)			*
 * @format.use-tabs true	(see http://www.synchro.net/ptsc_hdr.html)		*
 *																			*
 * Copyright 2005 Rob Swindell - http://www.synchro.net/copyright.html		*
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

#ifndef _INI_FILE_H
#define _INI_FILE_H

#include "genwrap.h"
#include "str_list.h"	/* strList_t */

#define INI_MAX_VALUE_LEN	1024		/* Maximum value length, includes '\0' */
#define ROOT_SECTION		NULL

typedef struct {
	ulong		bit;
	const char*	name;
} ini_bitdesc_t;

typedef struct {
	int			key_len;
	const char* key_prefix;
	const char* section_separator;
	const char* value_separator;
	const char*	bit_separator;
} ini_style_t;

#if defined(__cplusplus)
extern "C" {
#endif

/* Read all section names and return as an allocated string list */
/* Optionally (if prefix!=NULL), returns a subset of section names */
str_list_t	iniReadSectionList(FILE*, const char* prefix);
/* Read all key names and return as an allocated string list */
str_list_t	iniReadKeyList(FILE*, const char* section);
/* Read all key and value pairs and return as a named string list */
named_string_t**
			iniReadNamedStringList(FILE*, const char* section);

/* These functions read a single key of the specified type */
char*		iniReadString(FILE*, const char* section, const char* key
					,const char* deflt, char* value);
str_list_t	iniReadStringList(FILE*, const char* section, const char* key
					,const char* sep, const char* deflt);
long		iniReadInteger(FILE*, const char* section, const char* key
					,long deflt);
ushort		iniReadShortInt(FILE*, const char* section, const char* key
					,ushort deflt);
ulong		iniReadIpAddress(FILE*, const char* section, const char* key
					,ulong deflt);
double		iniReadFloat(FILE*, const char* section, const char* key
					,double deflt);
BOOL		iniReadBool(FILE*, const char* section, const char* key
					,BOOL deflt);
ulong		iniReadBitField(FILE*, const char* section, const char* key
					,ini_bitdesc_t* bitdesc, ulong deflt);

/* Free string list returned from iniRead*List functions */
void*		iniFreeStringList(str_list_t list);

/* Free named string list returned from iniReadNamedStringList */
void*		iniFreeNamedStringList(named_string_t** list);


/* File I/O Functions */
char*		iniFileName(char* dest, size_t maxlen, const char* dir, const char* fname);
FILE*		iniOpenFile(const char* fname, BOOL create);
str_list_t	iniReadFile(FILE*);
BOOL		iniWriteFile(FILE*, const str_list_t);
BOOL		iniCloseFile(FILE*);

/* StringList functions */
str_list_t	iniGetSectionList(str_list_t* list, const char* prefix);
str_list_t	iniGetKeyList(str_list_t* list, const char* section);
named_string_t**
			iniGetNamedStringList(str_list_t* list, const char* section);

char*		iniGetString(str_list_t*, const char* section, const char* key
					,const char* deflt, char* value);
str_list_t	iniGetStringList(str_list_t*, const char* section, const char* key
					,const char* sep, const char* deflt);
long		iniGetInteger(str_list_t*, const char* section, const char* key
					,long deflt);
ushort		iniGetShortInt(str_list_t*, const char* section, const char* key
					,ushort deflt);
ulong		iniGetIpAddress(str_list_t*, const char* section, const char* key
					,ulong deflt);
double		iniGetFloat(str_list_t*, const char* section, const char* key
					,double deflt);
BOOL		iniGetBool(str_list_t*, const char* section, const char* key
					,BOOL deflt);
ulong		iniGetBitField(str_list_t*, const char* section, const char* key
					,ini_bitdesc_t* bitdesc, ulong deflt);


char*		iniSetString(str_list_t*, const char* section, const char* key, const char* value
					,ini_style_t*);
char*		iniSetInteger(str_list_t*, const char* section, const char* key, long value
					,ini_style_t*);
char*		iniSetShortInt(str_list_t*, const char* section, const char* key, ushort value
					,ini_style_t*);
char*		iniSetHexInt(str_list_t*, const char* section, const char* key, ulong value
					,ini_style_t*);
char*		iniSetFloat(str_list_t*, const char* section, const char* key, double value
					,ini_style_t*);
char*		iniSetIpAddress(str_list_t*, const char* section, const char* key, ulong value
					,ini_style_t*);
char*		iniSetBool(str_list_t*, const char* section, const char* key, BOOL value
					,ini_style_t*);
char*		iniSetBitField(str_list_t*, const char* section, const char* key, ini_bitdesc_t*, ulong value
					,ini_style_t*);
char*		iniSetStringList(str_list_t*, const char* section, const char* key
					,const char* sep, str_list_t value, ini_style_t*);

size_t		iniAddSection(str_list_t*, const char* section
					,ini_style_t*);

BOOL		iniSectionExists(str_list_t*, const char* section);
BOOL		iniKeyExists(str_list_t*, const char* section, const char* key);
BOOL		iniValueExists(str_list_t*, const char* section, const char* key);
BOOL		iniRemoveKey(str_list_t*, const char* section, const char* key);
BOOL		iniRemoveValue(str_list_t*, const char* section, const char* key);
BOOL		iniRemoveSection(str_list_t*, const char* section);
BOOL		iniRenameSection(str_list_t*, const char* section, const char* newname);

#if defined(__cplusplus)
}
#endif

#endif	/* Don't add anything after this line */
