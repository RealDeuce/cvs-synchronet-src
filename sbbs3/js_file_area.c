/* js_file_area.c */

/* Synchronet JavaScript "File Area" Object */

/* $Id: js_file_area.c,v 1.8 2001/12/20 13:05:44 rswindell Exp $ */

/****************************************************************************
 * @format.tab-size 4		(Plain Text/Source Code File Header)			*
 * @format.use-tabs true	(see http://www.synchro.net/ptsc_hdr.html)		*
 *																			*
 * Copyright 2001 Rob Swindell - http://www.synchro.net/copyright.html		*
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

#ifdef JAVASCRIPT

enum {	/* file_area Object Properties */
	 PROP_MIN_DSPACE
	,PROP_MIN_LEECH_PCT
	,PROP_MIN_LEECH_SEC
};

static JSClass js_file_area_class = {
     "FileArea"				/* name			*/
    ,JSCLASS_HAS_PRIVATE	/* flags		*/
	,JS_PropertyStub		/* addProperty	*/
	,JS_PropertyStub		/* delProperty	*/
	,JS_PropertyStub		/* getProperty	*/
	,JS_PropertyStub		/* setProperty	*/
	,JS_EnumerateStub		/* enumerate	*/
	,JS_ResolveStub			/* resolve		*/
	,JS_ConvertStub			/* convert		*/
	,JS_FinalizeStub		/* finalize		*/
};

JSObject* DLLCALL js_CreateFileAreaObject(JSContext* cx, JSObject* parent, scfg_t* cfg
										  ,user_t* user, char* html_index_file)
{
	char		vpath[MAX_PATH+1];
	JSObject*	areaobj;
	JSObject*	libobj;
	JSObject*	dirobj;
	JSObject*	lib_list;
	JSObject*	dir_list;
	jsval		val;
	jsuint		index;
	uint		l,d;
	JSBool		found;

	areaobj = JS_DefineObject(cx, parent, "file_area", &js_file_area_class, NULL, 0);

	if(areaobj==NULL)
		return(NULL);

	/* lib_list[] */
	if((lib_list=JS_NewArrayObject(cx, 0, NULL))==NULL) 
		return(NULL);

	val=OBJECT_TO_JSVAL(lib_list);
	if(!JS_SetProperty(cx, areaobj, "lib_list", &val)) 
		return(NULL);
	JS_SetPropertyAttributes(cx, areaobj, "lib_list", 0, &found);

	for(l=0;l<cfg->total_libs;l++) {

		if(user!=NULL && !chk_ar(cfg,cfg->lib[l]->ar,user))
			continue;

		if((libobj=JS_NewObject(cx, &js_file_area_class, NULL, NULL))==NULL)
			return(NULL);

		val=STRING_TO_JSVAL(JS_NewStringCopyZ(cx, cfg->lib[l]->sname));
		if(!JS_SetProperty(cx, libobj, "name", &val))
			return(NULL);

		val=STRING_TO_JSVAL(JS_NewStringCopyZ(cx, cfg->lib[l]->lname));
		if(!JS_SetProperty(cx, libobj, "description", &val))
			return(NULL);

		sprintf(vpath,"/%s/%s",cfg->lib[l]->sname,html_index_file);
		val=STRING_TO_JSVAL(JS_NewStringCopyZ(cx, vpath));
		if(!JS_SetProperty(cx, libobj, "link", &val))
			return(NULL);

		/* dir_list[] */
		if((dir_list=JS_NewArrayObject(cx, 0, NULL))==NULL) 
			return(NULL);

		val=OBJECT_TO_JSVAL(dir_list);
		if(!JS_SetProperty(cx, libobj, "dir_list", &val)) 
			return(NULL);
		JS_SetPropertyAttributes(cx, libobj, "dir_list", 0, &found);

		for(d=0;d<cfg->total_dirs;d++) {
			if(cfg->dir[d]->lib!=l)
				continue;
			if(user!=NULL && !chk_ar(cfg,cfg->dir[d]->ar,user))
				continue;

			if((dirobj=JS_NewObject(cx, &js_file_area_class, NULL, NULL))==NULL)
				return(NULL);

			val=STRING_TO_JSVAL(JS_NewStringCopyZ(cx, cfg->dir[d]->code));
			if(!JS_SetProperty(cx, dirobj, "code", &val))
				return(NULL);

			val=STRING_TO_JSVAL(JS_NewStringCopyZ(cx, cfg->dir[d]->sname));
			if(!JS_SetProperty(cx, dirobj, "name", &val))
				return(NULL);

			val=STRING_TO_JSVAL(JS_NewStringCopyZ(cx, cfg->dir[d]->lname));
			if(!JS_SetProperty(cx, dirobj, "description", &val))
				return(NULL);

			val=INT_TO_JSVAL(cfg->dir[d]->misc);
			if(!JS_SetProperty(cx, dirobj, "settings", &val))
				return(NULL);

			sprintf(vpath,"/%s/%s/%s"
				,cfg->lib[l]->sname
				,cfg->dir[d]->code
				,html_index_file);

			val=STRING_TO_JSVAL(JS_NewStringCopyZ(cx, vpath));
			if(!JS_SetProperty(cx, dirobj, "link", &val))
				return(NULL);

			if(user==NULL || chk_ar(cfg,cfg->dir[d]->ul_ar,user))
				val=BOOLEAN_TO_JSVAL(JS_TRUE);
			else
				val=BOOLEAN_TO_JSVAL(JS_FALSE);
			if(!JS_SetProperty(cx, dirobj, "can_upload", &val))
				return(NULL);

			if(user==NULL || chk_ar(cfg,cfg->dir[d]->dl_ar,user))
				val=BOOLEAN_TO_JSVAL(JS_TRUE);
			else
				val=BOOLEAN_TO_JSVAL(JS_FALSE);
			if(!JS_SetProperty(cx, dirobj, "can_download", &val))
				return(NULL);

			if(user==NULL || chk_ar(cfg,cfg->dir[d]->ex_ar,user))
				val=BOOLEAN_TO_JSVAL(JS_TRUE);
			else
				val=BOOLEAN_TO_JSVAL(JS_FALSE);
			if(!JS_SetProperty(cx, dirobj, "is_exempt", &val))
				return(NULL);

			if(user==NULL || chk_ar(cfg,cfg->dir[d]->op_ar,user))
				val=BOOLEAN_TO_JSVAL(JS_TRUE);
			else
				val=BOOLEAN_TO_JSVAL(JS_FALSE);
			if(!JS_SetProperty(cx, dirobj, "is_operator", &val))
				return(NULL);

			if(!JS_GetArrayLength(cx, dir_list, &index))	/* inexplicable exception here on Jul-6-2001 */
				return(NULL);								/* and again on Aug-7-2001 and Oct-21-2001 */

			val=OBJECT_TO_JSVAL(dirobj);
			if(!JS_SetElement(cx, dir_list, index, &val))
				return(NULL);
		}

		if(!JS_GetArrayLength(cx, lib_list, &index))
			return(NULL);

		val=OBJECT_TO_JSVAL(libobj);
		if(!JS_SetElement(cx, lib_list, index, &val))
			return(NULL);
	}

	return(areaobj);
}

#endif	/* JAVSCRIPT */