/* js_internal.c */

/* Synchronet "js" object, for internal JavaScript callback and GC control */

/* $Id: js_internal.c,v 1.78 2013/02/08 02:41:45 deuce Exp $ */

/****************************************************************************
 * @format.tab-size 4		(Plain Text/Source Code File Header)			*
 * @format.use-tabs true	(see http://www.synchro.net/ptsc_hdr.html)		*
 *																			*
 * Copyright 2011 Rob Swindell - http://www.synchro.net/copyright.html		*
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
#include "js_request.h"

enum {
	 PROP_VERSION
	,PROP_TERMINATED
	,PROP_AUTO_TERMINATE
	,PROP_COUNTER
	,PROP_TIME_LIMIT
	,PROP_YIELD_INTERVAL
	,PROP_GC_INTERVAL
	,PROP_GC_ATTEMPTS
#ifdef jscntxt_h___
	,PROP_GC_COUNTER
	,PROP_GC_LASTBYTES
	,PROP_BYTES
	,PROP_MAXBYTES
#endif
	,PROP_GLOBAL
};

static JSBool js_get(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
	jsval idval;
    jsint			tiny;
	js_callback_t*	cb;

	if((cb=(js_callback_t*)JS_GetPrivate(cx,obj))==NULL)
		return(JS_FALSE);

    JS_IdToValue(cx, id, &idval);
    tiny = JSVAL_TO_INT(idval);

	switch(tiny) {
		case PROP_VERSION:
			*vp=STRING_TO_JSVAL(JS_NewStringCopyZ(cx,(char *)JS_GetImplementationVersion()));
			break;
		case PROP_TERMINATED:
			if(cb->terminated==NULL)
				*vp=JSVAL_FALSE;
			else
				*vp=BOOLEAN_TO_JSVAL(*cb->terminated);
			break;
		case PROP_AUTO_TERMINATE:
			*vp=BOOLEAN_TO_JSVAL(cb->auto_terminate);
			break;
		case PROP_COUNTER:
			*vp=DOUBLE_TO_JSVAL((double)cb->counter);
			break;
		case PROP_TIME_LIMIT:
			*vp=DOUBLE_TO_JSVAL(cb->limit);
			break;
		case PROP_YIELD_INTERVAL:
			*vp=DOUBLE_TO_JSVAL((double)cb->yield_interval);
			break;
		case PROP_GC_INTERVAL:
			*vp=DOUBLE_TO_JSVAL((double)cb->gc_interval);
			break;
		case PROP_GC_ATTEMPTS:
			*vp=DOUBLE_TO_JSVAL((double)cb->gc_attempts);
			break;
#ifdef jscntxt_h___
		case PROP_GC_COUNTER:
			*vp=UINT_TO_JSVAL(cx->runtime->gcNumber);
			break;
		case PROP_GC_LASTBYTES:
			*vp=DOUBLE_TO_JSVAL((double)cx->runtime->gcLastBytes);
			break;
		case PROP_BYTES:
			*vp=DOUBLE_TO_JSVAL((double)cx->runtime->gcBytes);
			break;
		case PROP_MAXBYTES:
			*vp=DOUBLE_TO_JSVAL((double)cx->runtime->gcMaxBytes);
			break;
#endif
		case PROP_GLOBAL:
			*vp = OBJECT_TO_JSVAL(JS_GetGlobalObject(cx));	
			break;
	}

	return(JS_TRUE);
}

static JSBool js_set(JSContext *cx, JSObject *obj, jsid id, JSBool strict, jsval *vp)
{
	jsval idval;
    jsint			tiny;
	js_callback_t*	cb;

	if((cb=(js_callback_t*)JS_GetPrivate(cx,obj))==NULL)
		return(JS_FALSE);

    JS_IdToValue(cx, id, &idval);
    tiny = JSVAL_TO_INT(idval);

	switch(tiny) {
		case PROP_TERMINATED:
			if(cb->terminated!=NULL)
				JS_ValueToBoolean(cx, *vp, (int *)cb->terminated);
			break;
		case PROP_AUTO_TERMINATE:
			JS_ValueToBoolean(cx,*vp,&cb->auto_terminate);
			break;
		case PROP_COUNTER:
			if(!JS_ValueToInt32(cx, *vp, (int32*)&cb->counter))
				return JS_FALSE;
			break;
		case PROP_TIME_LIMIT:
			if(!JS_ValueToInt32(cx, *vp, (int32*)&cb->limit))
				return JS_FALSE;
			break;
		case PROP_GC_INTERVAL:
			if(!JS_ValueToInt32(cx, *vp, (int32*)&cb->gc_interval))
				return JS_FALSE;
			break;
		case PROP_YIELD_INTERVAL:
			if(!JS_ValueToInt32(cx, *vp, (int32*)&cb->yield_interval))
				return JS_FALSE;
			break;
#ifdef jscntxt_h___
		case PROP_MAXBYTES:
			if(!JS_ValueToInt32(cx, *vp, (int32*)&cx->runtime->gcMaxBytes))
				return JS_FALSE;
			break;
#endif
	}

	return(JS_TRUE);
}

#define PROP_FLAGS	JSPROP_ENUMERATE|JSPROP_READONLY

static jsSyncPropertySpec js_properties[] = {
/*		 name,				tinyid,						flags,		ver	*/

	{	"version",			PROP_VERSION,		PROP_FLAGS,			311 },
	{	"auto_terminate",	PROP_AUTO_TERMINATE,JSPROP_ENUMERATE,	311 },
	{	"terminated",		PROP_TERMINATED,	JSPROP_ENUMERATE,	311 },
	{	"branch_counter",	PROP_COUNTER,		0,					311 },
	{	"counter",			PROP_COUNTER,		JSPROP_ENUMERATE,	316 },
	{	"branch_limit",		PROP_TIME_LIMIT,	0,					311 },
	{	"time_limit",		PROP_TIME_LIMIT,	JSPROP_ENUMERATE,	316 },
	{	"yield_interval",	PROP_YIELD_INTERVAL,JSPROP_ENUMERATE,	311 },
	{	"gc_interval",		PROP_GC_INTERVAL,	JSPROP_ENUMERATE,	311 },
	{	"gc_attempts",		PROP_GC_ATTEMPTS,	PROP_FLAGS,			311 },
#ifdef jscntxt_h___
	{	"gc_counter",		PROP_GC_COUNTER,	PROP_FLAGS,			311 },
	{	"gc_last_bytes",	PROP_GC_LASTBYTES,	PROP_FLAGS,			311 },
	{	"bytes",			PROP_BYTES,			PROP_FLAGS,			311 },
	{	"max_bytes",		PROP_MAXBYTES,		JSPROP_ENUMERATE,	311 },
#endif
	{	"global",			PROP_GLOBAL,		PROP_FLAGS,			314 },
	{0}
};

#ifdef BUILD_JSDOCS
static char* prop_desc[] = {
	 "JavaScript engine version information (AKA system.js_version)"
	,"set to <i>false</i> to disable the automatic termination of the script upon external request"
	,"termination has been requested (stop execution as soon as possible)"
	,"number of branch operations performed in this runtime"
	,"maximum number of branches, used for infinite-loop detection (0=disabled)"
	,"interval of periodic time-slice yields (lower number=higher frequency, 0=disabled)"
	,"interval of periodic garbage collection attempts (lower number=higher frequency, 0=disabled)"
	,"number of garbage collections attempted in this runtime - <small>READ ONLY</small>"
#ifdef jscntxt_h___
	,"number of garbage collections performed in this runtime - <small>READ ONLY</small>"
	,"number of heap bytes in use after last garbage collection - <small>READ ONLY</small>"
	,"number of heap bytes currently in use - <small>READ ONLY</small>"
	,"maximum number of bytes available for heap"
#endif
	,"global (top level) object - <small>READ ONLY</small>"
	/* New properties go here... */
	,"load() search path array.<br>For relative load paths (e.g. not beginning with '/' or '\'), "
		"the path is assumed to be a sub-directory of the (configurable) mods or exec directories "
		"and is searched accordingly. "
		"So, by default, load(\"somefile.js\") will search in this order:<br>"
		"mods/load/somefile.js<br>"
		"exec/load/somefile.js<br>"
		"mods/somefile.js<br>"
		"exec/somefile.js<br>"
	,"full path and filename of JS file executed"
	,"directory of executed JS file"
	,"JS filename executed (with no path)"
	,"Either the configure startup directory in SCFG (for externals) or the cwd when jsexec is started"
	,NULL
};
#endif

JSBool DLLCALL
js_CommonOperationCallback(JSContext *cx, js_callback_t* cb)
{
	cb->counter++;

	/* Terminated? */
	if(cb->auto_terminate &&
		(cb->terminated!=NULL && *cb->terminated)) {
		JS_ReportWarning(cx,"Terminated");
		cb->counter=0;
		return(JS_FALSE);
	}

	/* Infinite loop? */
	if(cb->limit && cb->counter > cb->limit) {
		JS_ReportError(cx,"Infinite loop (%lu branches) detected",cb->counter);
		cb->counter=0;
		return(JS_FALSE);
	}

	/* Give up timeslices every once in a while */
	if(cb->yield_interval && (cb->counter%cb->yield_interval)==0) {
		jsrefcount	rc;

		rc=JS_SUSPENDREQUEST(cx);
		YIELD();
		JS_RESUMEREQUEST(cx, rc);
	}

	/* Permit other contexts to run GC */
	JS_YieldRequest(cx);

	/* Periodic Garbage Collection */
	if(cb->gc_interval && (cb->counter%cb->gc_interval)==0)
		JS_MaybeGC(cx), cb->gc_attempts++;

    return(JS_TRUE);
}

static JSClass eval_class = {
    "Global",  /* name */
    JSCLASS_GLOBAL_FLAGS,  /* flags */
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_StrictPropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

/* Execute a string in its own context (away from Synchronet objects) */
static JSBool
js_eval(JSContext *parent_cx, uintN argc, jsval *arglist)
{
	jsval *argv=JS_ARGV(parent_cx, arglist);
	char*			buf;
	size_t			buflen;
	JSString*		str;
    JSObject*		script;
	JSContext*		cx;
	JSObject*		obj;
	JSErrorReporter	reporter;

	JS_SET_RVAL(cx, arglist, JSVAL_VOID);

	if(argc<1)
		return(JS_TRUE);

	if((str=JS_ValueToString(parent_cx, argv[0]))==NULL)
		return(JS_FALSE);
	JSSTRING_TO_MSTRING(parent_cx, str, buf, &buflen);
	HANDLE_PENDING(parent_cx);
	if(buf==NULL)
		return(JS_TRUE);

	if((cx=JS_NewContext(JS_GetRuntime(parent_cx),JAVASCRIPT_CONTEXT_STACK))==NULL) {
		free(buf);
		return(JS_FALSE);
	}

	/* Use the error reporter from the parent context */
	reporter=JS_SetErrorReporter(parent_cx,NULL);
	JS_SetErrorReporter(parent_cx,reporter);
	JS_SetErrorReporter(cx,reporter);

	/* Use the branch callback from the parent context */
	JS_SetContextPrivate(cx, JS_GetContextPrivate(parent_cx));
	JS_SetOperationCallback(cx, JS_GetOperationCallback(parent_cx));

	if((obj=JS_NewCompartmentAndGlobalObject(cx, &eval_class, NULL))==NULL
		|| !JS_InitStandardClasses(cx,obj)) {
		JS_DestroyContext(cx);
		free(buf);
		return(JS_FALSE);
	}

	if((script=JS_CompileScript(cx, obj, buf, buflen, NULL, 0))!=NULL) {
		jsval	rval;

		JS_ExecuteScript(cx, obj, script, &rval);
		JS_SET_RVAL(cx, arglist, rval);
	}
	free(buf);

	JS_DestroyContext(cx);

    return(JS_TRUE);
}

static JSBool
js_gc(JSContext *cx, uintN argc, jsval *arglist)
{
	JSObject *obj=JS_THIS_OBJECT(cx, arglist);
	jsval *argv=JS_ARGV(cx, arglist);
	JSBool			forced=JS_TRUE;
	js_callback_t*	cb;

	JS_SET_RVAL(cx, arglist, JSVAL_VOID);

	if((cb=(js_callback_t*)JS_GetPrivate(cx,obj))==NULL)
		return(JS_FALSE);

	if(argc)
		JS_ValueToBoolean(cx,argv[0],&forced);

	if(forced)
		JS_GC(cx);
	else
		JS_MaybeGC(cx);

	cb->gc_attempts++;

	return(JS_TRUE);
}

static JSBool
js_report_error(JSContext *cx, uintN argc, jsval *arglist)
{
	jsval *argv=JS_ARGV(cx, arglist);
	char	*p;

	JSVALUE_TO_MSTRING(cx, argv[0], p, NULL);
	HANDLE_PENDING(cx);
	if(p==NULL)
		JS_ReportError(cx,"NULL");
	else {
		JS_ReportError(cx,"%s",p);
		free(p);
	}

	JS_SET_RVAL(cx, arglist, JSVAL_VOID);

	if(argc>1 && argv[1]==JSVAL_TRUE)
		return(JS_FALSE);	/* fatal */

	return(JS_TRUE);
}

static JSBool
js_on_exit(JSContext *cx, uintN argc, jsval *arglist)
{
	JSObject *scope=JS_GetScopeChain(cx);
	JSObject *glob=JS_GetGlobalObject(cx);
	jsval *argv=JS_ARGV(cx, arglist);
	global_private_t*	pd;
	str_list_t	list;
	str_list_t	oldlist;
	char		*p;

	JS_SET_RVAL(cx, arglist, JSVAL_VOID);

	if(glob==scope) {
		if((pd=(global_private_t*)JS_GetPrivate(cx,glob))==NULL)
			return(JS_FALSE);
		if(pd->exit_func==NULL)
			pd->exit_func=strListInit();
		list=pd->exit_func;
	}
	else {
		list=(str_list_t)JS_GetPrivate(cx,scope);
		if(list==NULL) {
			list=strListInit();
			JS_SetPrivate(cx,scope,list);
		}
	}

	JSVALUE_TO_MSTRING(cx, argv[0], p, NULL);
	HANDLE_PENDING(cx);
	if(!p)
		return JS_TRUE;
	oldlist=list;
	strListPush(&list,p);
	free(p);
	if(oldlist != list) {
		if(glob==scope)
			pd->exit_func=list;
		else
			JS_SetPrivate(cx,scope,list);
	}

	return(JS_TRUE);
}

static JSBool
js_get_parent(JSContext *cx, uintN argc, jsval *arglist)
{
	jsval *argv=JS_ARGV(cx, arglist);
	JSObject* child=NULL;
	JSObject* parent;

	if(JS_ValueToObject(cx, argv[0], &child)
		&& child!=NULL
		&& (parent=JS_GetParent(cx,child))!=NULL)
		JS_SET_RVAL(cx, arglist, OBJECT_TO_JSVAL(parent));

	return(JS_TRUE);
}

static jsSyncMethodSpec js_functions[] = {
	{"eval",            js_eval,            0,	JSTYPE_UNDEF,	JSDOCSTR("script")
	,JSDOCSTR("evaluate a JavaScript string in its own (secure) context, returning the result")
	,311
	},		
	{"gc",				js_gc,				0,	JSTYPE_VOID,	JSDOCSTR("forced=<tt>true</tt>")
	,JSDOCSTR("perform a garbage collection operation (freeing memory for unused allocated objects), "
		"if <i>forced</i> is <i>true</i> (the default) a garbage collection is always performed, "
		"otherwise it is only performed if deemed appropriate by the JavaScript engine")
	,311
	},
	{"on_exit",			js_on_exit,			1,	JSTYPE_VOID,	JSDOCSTR("to_eval")
	,JSDOCSTR("add a string to evaluate/execute (LIFO stack) upon script's termination")
	,313
	},
	{"report_error",	js_report_error,	1,	JSTYPE_VOID,	JSDOCSTR("error [,fatal=<tt>false</tt>]")
	,JSDOCSTR("report an error using the standard JavaScript error reporting mechanism "
	"(including script filename and line number), "
	"if <i>fatal</i> is <i>true</i>, immediately terminates script")
	,313
	},
	{"get_parent",		js_get_parent,		1,	JSTYPE_OBJECT,	JSDOCSTR("object")
	,JSDOCSTR("return the parent of the specified object")
	,314
	},
	{0}
};

static JSBool js_internal_resolve(JSContext *cx, JSObject *obj, jsid id)
{
	char*			name=NULL;
	JSBool			ret;

	if(id != JSID_VOID && id != JSID_EMPTY) {
		jsval idval;
		
		JS_IdToValue(cx, id, &idval);
		if(JSVAL_IS_STRING(idval)) {
			JSSTRING_TO_MSTRING(cx, JSVAL_TO_STRING(idval), name, NULL);
			HANDLE_PENDING(cx);
		}
	}

	ret=js_SyncResolve(cx, obj, name, js_properties, js_functions, NULL, 0);
	if(name)
		free(name);
	return(ret);
}

static JSBool js_internal_enumerate(JSContext *cx, JSObject *obj)
{
	return(js_internal_resolve(cx, obj, JSID_VOID));
}

static JSClass js_internal_class = {
     "JsInternal"				/* name			*/
    ,JSCLASS_HAS_PRIVATE	/* flags		*/
	,JS_PropertyStub		/* addProperty	*/
	,JS_PropertyStub		/* delProperty	*/
	,js_get					/* getProperty	*/
	,js_set					/* setProperty	*/
	,js_internal_enumerate	/* enumerate	*/
	,js_internal_resolve	/* resolve		*/
	,JS_ConvertStub			/* convert		*/
	,JS_FinalizeStub		/* finalize		*/
};

void DLLCALL js_EvalOnExit(JSContext *cx, JSObject *obj, js_callback_t* cb)
{
	char*	p;
	jsval	rval;
	JSObject* script;
	BOOL	auto_terminate=cb->auto_terminate;
	JSObject	*glob=JS_GetGlobalObject(cx);
	global_private_t *pt;
	str_list_t	list;

	if(glob==obj) {
		pt=(global_private_t *)JS_GetPrivate(cx,JS_GetGlobalObject(cx));		
		list=pt->exit_func;
	}
	else
		list=JS_GetPrivate(cx,obj);

	cb->auto_terminate=FALSE;

	while((p=strListPop(&list))!=NULL) {
		if((script=JS_CompileScript(cx, obj, p, strlen(p), NULL, 0))!=NULL) {
			JS_ExecuteScript(cx, obj, script, &rval);
		}
		free(p);
	}

	strListFree(&list);
	if(glob != obj)
		JS_SetPrivate(cx,obj,NULL);
	else
		pt->exit_func=NULL;

	if(auto_terminate)
		cb->auto_terminate = TRUE;
}

JSObject* DLLCALL js_CreateInternalJsObject(JSContext* cx, JSObject* parent, js_callback_t* cb, js_startup_t* startup)
{
	JSObject*	obj;

	if((obj = JS_DefineObject(cx, parent, "js", &js_internal_class, NULL
		,JSPROP_ENUMERATE|JSPROP_READONLY))==NULL)
		return(NULL);

	if(!JS_SetPrivate(cx, obj, cb))	/* Store a pointer to js_callback_t */
		return(NULL);

	if(startup!=NULL) {
		JSObject*	load_path_list;
		jsval		val;
		str_list_t	load_path;

		if((load_path_list=JS_NewArrayObject(cx, 0, NULL))==NULL) 
			return(NULL);
		val=OBJECT_TO_JSVAL(load_path_list);
		if(!JS_SetProperty(cx, obj, JAVASCRIPT_LOAD_PATH_LIST, &val)) 
			return(NULL);

		if((load_path=strListSplitCopy(NULL, startup->load_path, ",")) != NULL) {
			JSString*	js_str;
			unsigned	i;

			for(i=0; load_path[i]!=NULL; i++) {
				if((js_str=JS_NewStringCopyZ(cx, load_path[i]))==NULL)
					break;
				val=STRING_TO_JSVAL(js_str);
				if(!JS_SetElement(cx, load_path_list, i, &val))
					break;
			}
			strListFree(&load_path);
		}
	}

#ifdef BUILD_JSDOCS
	js_DescribeSyncObject(cx,obj,"JavaScript execution and garbage collection control object",311);
	js_CreateArrayOfStrings(cx, obj, "_property_desc_list", prop_desc, JSPROP_READONLY);
#endif

	return(obj);
}

void DLLCALL js_PrepareToExecute(JSContext *cx, JSObject *obj, const char *filename, const char* startup_dir)
{
	JSString*	str;
	jsval		val;

	if(JS_GetProperty(cx, obj, "js", &val) && JSVAL_IS_OBJECT(val)) {
		JSObject* js = JSVAL_TO_OBJECT(val);
		char	dir[MAX_PATH+1];

		if(filename!=NULL) {
			char* p;

			if((str=JS_NewStringCopyZ(cx, filename)) != NULL)
				JS_DefineProperty(cx, js, "exec_path", STRING_TO_JSVAL(str)
					,NULL,NULL,JSPROP_ENUMERATE|JSPROP_READONLY);
			if((str=JS_NewStringCopyZ(cx, getfname(filename))) != NULL)
				JS_DefineProperty(cx, js, "exec_file", STRING_TO_JSVAL(str)
					,NULL,NULL,JSPROP_ENUMERATE|JSPROP_READONLY);
			SAFECOPY(dir,filename);
			p=getfname(dir);
			*p=0;
			if((str=JS_NewStringCopyZ(cx, dir)) != NULL)
				JS_DefineProperty(cx, js, "exec_dir", STRING_TO_JSVAL(str)
					,NULL,NULL,JSPROP_ENUMERATE|JSPROP_READONLY);
		}
		if(startup_dir==NULL)
			startup_dir="";
		if((str=JS_NewStringCopyZ(cx, startup_dir)) != NULL)
			JS_DefineProperty(cx, js, "startup_dir", STRING_TO_JSVAL(str)
				,NULL,NULL,JSPROP_ENUMERATE|JSPROP_READONLY);
	}
}
