/* js_client.c */

/* Synchronet JavaScript "Client" Object */

/* $Id: js_client.c,v 1.17 2008/06/04 04:38:47 deuce Exp $ */

/****************************************************************************
 * @format.tab-size 4		(Plain Text/Source Code File Header)			*
 * @format.use-tabs true	(see http://www.synchro.net/ptsc_hdr.html)		*
 *																			*
 * Copyright 2005 Rob Swindell - http://www.synchro.net/copyright.html		*
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

/* Client Object Properites */
enum {
	 CLIENT_PROP_ADDR		/* IP address */
	,CLIENT_PROP_HOST		/* host name */
	,CLIENT_PROP_PORT		/* TCP port number */
	,CLIENT_PROP_TIME		/* connect time */
	,CLIENT_PROP_PROTOCOL	/* protocol description */
	,CLIENT_PROP_USER		/* user name */
};

#ifdef BUILD_JSDOCS
	static char* client_prop_desc[] = {
	 "client's IP address (in dotted-decimal format)"
	,"client's host name (up to 64 characters)"
	,"client's TCP or UDP port number"
	,"date/time of initial connection (in time_t format)"
	,"protocol description (e.g. 'Telnet', 'FTP', etc.)"
	,"user's name/alias (if logged in)"
	/* this next one must be last */
	,"instance of <a href=#Socket>Socket class</a> representing client's TCP/IP connection"
	,NULL
	};
#endif

static JSBool js_client_set(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	return(JS_FALSE);
}

static JSBool js_client_get(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
	const char*	p=NULL;
	ulong		val=0;
    jsint       tiny;
	JSString*	js_str;
	client_t*	client;

	if((client=(client_t*)JS_GetPrivate(cx,obj))==NULL)
		return(JS_FALSE);

    tiny = JSVAL_TO_INT(id);

	switch(tiny) {
		case CLIENT_PROP_ADDR: 
			p=client->addr;
			break;
		case CLIENT_PROP_HOST:
			p=client->host;
			break;
		case CLIENT_PROP_PORT:
			val=client->port;
			break;
		case CLIENT_PROP_TIME:
			val=client->time;
			break;
		case CLIENT_PROP_PROTOCOL:
			p=(char*)client->protocol;
			break;
		case CLIENT_PROP_USER:
			p=client->user;
			break;
		default:
			return(JS_TRUE);
	}
	if(p!=NULL) {
		if((js_str=JS_NewStringCopyZ(cx, p))==NULL)
			return(JS_FALSE);
		*vp = STRING_TO_JSVAL(js_str);
	} else
		*vp = INT_TO_JSVAL(val);

	return(JS_TRUE);
}

#define CLIENT_PROP_FLAGS JSPROP_ENUMERATE|JSPROP_READONLY

static jsSyncPropertySpec js_client_properties[] = {
/*		 name				,tinyid					,flags,					ver	*/

	{	"ip_address"		,CLIENT_PROP_ADDR 		,CLIENT_PROP_FLAGS,		310},
	{	"host_name"			,CLIENT_PROP_HOST		,CLIENT_PROP_FLAGS,		310},
	{	"port"				,CLIENT_PROP_PORT	 	,CLIENT_PROP_FLAGS,		310},
	{	"connect_time"		,CLIENT_PROP_TIME	 	,CLIENT_PROP_FLAGS,		310},
	{	"protocol"			,CLIENT_PROP_PROTOCOL 	,CLIENT_PROP_FLAGS,		310},
	{	"user_name"			,CLIENT_PROP_USER	 	,CLIENT_PROP_FLAGS,		310},
	{0}
};

static JSBool js_client_resolve(JSContext *cx, JSObject *obj, jsval id)
{
	char*			name=NULL;

	if(id != JSVAL_NULL)
		name=JS_GetStringBytes(JSVAL_TO_STRING(id));

	return(js_SyncResolve(cx, obj, name, js_client_properties, NULL, NULL, 0));
}

static JSBool js_client_enumerate(JSContext *cx, JSObject *obj)
{
	return(js_client_resolve(cx, obj, JSVAL_NULL));
}

static JSClass js_client_class = {
     "Client"				/* name			*/
    ,JSCLASS_HAS_PRIVATE	/* flags		*/
	,JS_PropertyStub		/* addProperty	*/
	,JS_PropertyStub		/* delProperty	*/
	,js_client_get			/* getProperty	*/
	,js_client_set			/* setProperty	*/
	,js_client_enumerate	/* enumerate	*/
	,js_client_resolve		/* resolve		*/
	,JS_ConvertStub			/* convert		*/
	,JS_FinalizeStub		/* finalize		*/
};

JSObject* DLLCALL js_CreateClientObject(JSContext* cx, JSObject* parent
										,char* name, client_t* client, SOCKET sock)
{
	JSObject*	obj;

	obj = JS_DefineObject(cx, parent, name, &js_client_class, NULL
		,JSPROP_ENUMERATE|JSPROP_READONLY);

	if(obj==NULL)
		return(NULL);

	JS_SetPrivate(cx, obj, client);	/* Store a pointer to client_t */

	js_CreateSocketObject(cx, obj, "socket", sock);

#ifdef BUILD_JSDOCS
	js_DescribeSyncObject(cx,obj,"Represents a TCP/IP client session",310);
	js_CreateArrayOfStrings(cx, obj, "_property_desc_list", client_prop_desc, JSPROP_READONLY);
#endif

	return(obj);
}

#endif	/* JAVSCRIPT */
