/* js_global.c */

/* Synchronet JavaScript "global" object properties/methods for all servers */

/* $Id$ */

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

#define JS_THREADSAFE	/* needed for JS_SetContextThread */

#include "sbbs.h"
#include "md5.h"
#include "base64.h"
#include "htmlansi.h"
#include "ini_file.h"

#define MAX_ANSI_SEQ	16
#define MAX_ANSI_PARAMS	8

#ifdef JAVASCRIPT

/* Global Object Properites */
enum {
	 GLOB_PROP_ERRNO
	,GLOB_PROP_ERRNO_STR
};

static JSBool js_system_get(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    jsint       tiny;
	JSString*	js_str;

    tiny = JSVAL_TO_INT(id);

	switch(tiny) {
		case GLOB_PROP_ERRNO:
			JS_NewNumberValue(cx,errno,vp);
			break;
		case GLOB_PROP_ERRNO_STR:
			if((js_str=JS_NewStringCopyZ(cx, strerror(errno)))==NULL)
				return(JS_FALSE);
	        *vp = STRING_TO_JSVAL(js_str);
			break;
	}
	return(JS_TRUE);
}

#define GLOBOBJ_FLAGS JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_SHARED

static struct JSPropertySpec js_global_properties[] = {
/*		 name,		tinyid,				flags */

	{	"errno",	GLOB_PROP_ERRNO,	GLOBOBJ_FLAGS },
	{	"errno_str",GLOB_PROP_ERRNO_STR,GLOBOBJ_FLAGS },
	{0}
};

typedef struct {
	JSRuntime*		runtime;
	JSContext*		cx;
	JSContext*		parent_cx;
	JSObject*		obj;
	JSScript*		script;
	msg_queue_t*	msg_queue;
	js_branch_t		branch;
	JSErrorReporter error_reporter;
} background_data_t;

static void background_thread(void* arg)
{
	background_data_t* bg = (background_data_t*)arg;
	jsval result=JSVAL_VOID;

	msgQueueAttach(bg->msg_queue);
	JS_SetContextThread(bg->cx);
	if(JS_ExecuteScript(bg->cx, bg->obj, bg->script, &result) && result!=JSVAL_VOID)
		js_enqueue_value(bg->cx, bg->msg_queue, result, NULL);
	JS_DestroyScript(bg->cx, bg->script);
	JS_DestroyContext(bg->cx);
	JS_DestroyRuntime(bg->runtime);
	free(bg);
}

static void
js_ErrorReporter(JSContext *cx, const char *message, JSErrorReport *report)
{
	background_data_t* bg;

	if((bg=(background_data_t*)JS_GetContextPrivate(cx))==NULL)
		return;

	/* Use parent's context private data */
	JS_SetContextPrivate(cx, JS_GetContextPrivate(bg->parent_cx));

	/* Call parent's error reporter */
	bg->error_reporter(cx, message, report);

	/* Restore our context private data */
	JS_SetContextPrivate(cx, bg);
}

static JSBool js_BranchCallback(JSContext *cx, JSScript* script)
{
	background_data_t* bg;

	if((bg=(background_data_t*)JS_GetContextPrivate(cx))==NULL)
		return(JS_FALSE);

	if(bg->parent_cx!=NULL && !JS_IsRunning(bg->parent_cx)) 	/* die when parent dies */
		return(JS_FALSE);

	return js_CommonBranchCallback(cx,&bg->branch);
}

static JSBool
js_load(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char		path[MAX_PATH+1];
    uintN		i;
	uintN		argn=0;
    const char*	filename;
    JSScript*	script;
	scfg_t*		cfg;
	jsval		val;
	JSObject*	js_argv;
	JSObject*	exec_obj;
	JSContext*	exec_cx=cx;
	JSBool		success;
	JSBool		background=JS_FALSE;
	background_data_t* bg;

	*rval=JSVAL_VOID;

	if((cfg=(scfg_t*)JS_GetPrivate(cx,obj))==NULL)
		return(JS_FALSE);

	exec_obj=JS_GetScopeChain(cx);

	if(JSVAL_IS_BOOLEAN(argv[argn]))
		background=JSVAL_TO_BOOLEAN(argv[argn++]);

	if(background) {

		if((bg=(background_data_t*)malloc(sizeof(background_data_t)))==NULL)
			return(JS_FALSE);
		memset(bg,0,sizeof(background_data_t));

		bg->parent_cx = cx;

		/* Setup default values for branch settings */
		bg->branch.limit=JAVASCRIPT_BRANCH_LIMIT;
		bg->branch.gc_interval=JAVASCRIPT_GC_INTERVAL;
		bg->branch.yield_interval=JAVASCRIPT_YIELD_INTERVAL;
		if(JS_GetProperty(cx, obj,"js",&val))	/* copy branch settings from parent */
			memcpy(&bg->branch,JS_GetPrivate(cx,JSVAL_TO_OBJECT(val)),sizeof(bg->branch));
		bg->branch.terminated=NULL;	/* could be bad pointer at any time */
		bg->branch.counter=0;
		bg->branch.gc_attempts=0;

		if((bg->runtime = JS_NewRuntime(JAVASCRIPT_MAX_BYTES))==NULL)
			return(JS_FALSE);

	    if((bg->cx = JS_NewContext(bg->runtime, JAVASCRIPT_CONTEXT_STACK))==NULL)
			return(JS_FALSE);

		if((bg->obj=js_CreateCommonObjects(bg->cx
				,cfg			/* common config */
				,NULL			/* node-specific config */
				,NULL			/* additional global methods */
				,0				/* uptime */
				,""				/* hostname */
				,""				/* socklib_desc */
				,&bg->branch	/* js */
				,NULL			/* client */
				,INVALID_SOCKET	/* client_socket */
				,NULL			/* server props */
				))==NULL) 
			return(JS_FALSE);

		bg->msg_queue = msgQueueInit(NULL,MSG_QUEUE_BIDIR);

		js_CreateQueueObject(bg->cx, bg->obj, "parent_queue", bg->msg_queue);

		/* Save parent's error reporter (for later use by our error reporter) */
		bg->error_reporter=JS_SetErrorReporter(cx,NULL);
		JS_SetErrorReporter(cx,bg->error_reporter);
		JS_SetErrorReporter(bg->cx,js_ErrorReporter);

		/* Set our branch callback (which calls the generic branch callback) */
		JS_SetContextPrivate(bg->cx, bg);
		JS_SetBranchCallback(bg->cx, js_BranchCallback);

		exec_cx = bg->cx;
		exec_obj = bg->obj;
		
	} else if(JSVAL_IS_OBJECT(argv[argn]))	/* Scope specified */
		obj=JSVAL_TO_OBJECT(argv[argn++]);

	if((filename=JS_GetStringBytes(JS_ValueToString(cx, argv[argn++])))==NULL)
		return(JS_FALSE);

	if(argc>argn) {

		if((js_argv=JS_NewArrayObject(exec_cx, 0, NULL)) == NULL)
			return(JS_FALSE);

		JS_DefineProperty(exec_cx, exec_obj, "argv", OBJECT_TO_JSVAL(js_argv)
			,NULL,NULL,JSPROP_ENUMERATE|JSPROP_READONLY);

		for(i=argn; i<argc; i++)
			JS_SetElement(exec_cx, js_argv, i-argn, &argv[i]);

		JS_DefineProperty(exec_cx, exec_obj, "argc", INT_TO_JSVAL(argc-argn)
			,NULL,NULL,JSPROP_ENUMERATE|JSPROP_READONLY);
	}

	errno = 0;
	if(isfullpath(filename))
		strcpy(path,filename);
	else {
		sprintf(path,"%s%s",cfg->mods_dir,filename);
		if(cfg->mods_dir[0]==0 || !fexistcase(path))
			sprintf(path,"%s%s",cfg->exec_dir,filename);
	}

	JS_ClearPendingException(exec_cx);

	if((script=JS_CompileFile(exec_cx, exec_obj, path))==NULL)
		return(JS_FALSE);

	if(background) {

		bg->script = script;
		*rval = OBJECT_TO_JSVAL(js_CreateQueueObject(cx, obj, NULL, bg->msg_queue));
		success = _beginthread(background_thread,0,bg)!=-1;

	} else {

		success = JS_ExecuteScript(exec_cx, exec_obj, script, rval);
		JS_DestroyScript(exec_cx, script);
	}

    return(success);
}

static JSBool
js_format(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char*		p;
	char*		fmt;
    uintN		i;
    JSString *	str;
	va_list		arglist[64];

	if((fmt=JS_GetStringBytes(JS_ValueToString(cx, argv[0])))==NULL)
		return(JS_FALSE);

	memset(arglist,0,sizeof(arglist));	/* Initialize arglist to NULLs */

    for (i = 1; i < argc && i<sizeof(arglist)/sizeof(arglist[0]); i++) {
		if(JSVAL_IS_DOUBLE(argv[i]))
			arglist[i-1]=(char*)(unsigned long)*JSVAL_TO_DOUBLE(argv[i]);
		else if(JSVAL_IS_INT(argv[i]))
			arglist[i-1]=(char *)JSVAL_TO_INT(argv[i]);
		else {
			if((str=JS_ValueToString(cx, argv[i]))==NULL) {
				JS_ReportError(cx,"JS_ValueToString failed");
			    return(JS_FALSE);
			}
			arglist[i-1]=JS_GetStringBytes(str);
		}
	}
	
	if((p=JS_vsmprintf(fmt,(char*)arglist))==NULL)
		return(JS_FALSE);

	str = JS_NewStringCopyZ(cx, p);
	JS_smprintf_free(p);

	if(str==NULL)
		return(JS_FALSE);

	*rval = STRING_TO_JSVAL(str);
    return(JS_TRUE);
}

static JSBool
js_yield(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	BOOL forced=TRUE;

	if(argc)
		JS_ValueToBoolean(cx, argv[0], &forced);
	if(forced) {
		YIELD();
	} else {
		MAYBE_YIELD();
	}

	*rval = JSVAL_VOID;
	return(JS_TRUE);
}

static JSBool
js_mswait(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	int32 val=1;

	if(argc)
		JS_ValueToInt32(cx,argv[0],&val);
	mswait(val);

	*rval = JSVAL_VOID;
	return(JS_TRUE);
}

static JSBool
js_random(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	int32 val=100;

	if(argc)
		JS_ValueToInt32(cx,argv[0],&val);

	JS_NewNumberValue(cx,sbbs_random(val),rval);
	return(JS_TRUE);
}

static JSBool
js_time(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	JS_NewNumberValue(cx,time(NULL),rval);
	return(JS_TRUE);
}


static JSBool
js_beep(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	int32 freq=500;
	int32 dur=500;

	if(argc)
		JS_ValueToInt32(cx,argv[0],&freq);
	if(argc>1)
		JS_ValueToInt32(cx,argv[1],&dur);

	sbbs_beep(freq,dur);
	*rval = JSVAL_VOID;
	return(JS_TRUE);
}

static JSBool
js_exit(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	*rval = JSVAL_VOID;
	if(argc)
		JS_DefineProperty(cx, obj, "exit_code", argv[0]
			,NULL,NULL,JSPROP_ENUMERATE|JSPROP_READONLY);

	return(JS_FALSE);
}

static JSBool
js_crc16(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char*		str;

	if((str=JS_GetStringBytes(JS_ValueToString(cx, argv[0])))==NULL) 
		return(JS_FALSE);

	*rval = INT_TO_JSVAL(crc16(str,0));
	return(JS_TRUE);
}

static JSBool
js_crc32(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char*		str;

	if((str=JS_GetStringBytes(JS_ValueToString(cx, argv[0])))==NULL) 
		return(JS_FALSE);

	JS_NewNumberValue(cx,crc32(str,strlen(str)),rval);
	return(JS_TRUE);
}

static JSBool
js_chksum(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	ulong		sum=0;
	char*		p;

	if((p=JS_GetStringBytes(JS_ValueToString(cx, argv[0])))==NULL) 
		return(JS_FALSE);

	while(*p) sum+=*(p++);

	JS_NewNumberValue(cx,sum,rval);
	return(JS_TRUE);
}

static JSBool
js_ascii(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char*		p;
	char		str[2];
	int32		i=0;
	JSString*	js_str;

	if(JSVAL_IS_STRING(argv[0])) {	/* string to ascii-int */

		if((p=JS_GetStringBytes(JSVAL_TO_STRING(argv[0])))==NULL) 
			return(JS_FALSE);

		*rval=INT_TO_JSVAL(*p);
		return(JS_TRUE);
	}

	/* ascii-int to str */
	JS_ValueToInt32(cx,argv[0],&i);
	str[0]=(uchar)i;
	str[1]=0;

	if((js_str = JS_NewStringCopyZ(cx, str))==NULL)
		return(JS_FALSE);

	*rval = STRING_TO_JSVAL(js_str);
	return(JS_TRUE);
}

static JSBool
js_ctrl(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char		ch;
	char*		p;
	char		str[2];
	int32		i=0;
	JSString*	js_str;

	if(JSVAL_IS_STRING(argv[0])) {	

		if((p=JS_GetStringBytes(JSVAL_TO_STRING(argv[0])))==NULL) 
			return(JS_FALSE);
		ch=*p;
	} else {
		JS_ValueToInt32(cx,argv[0],&i);
		ch=(char)i;
	}

	str[0]=toupper(ch)&~0x20;
	str[1]=0;

	if((js_str = JS_NewStringCopyZ(cx, str))==NULL)
		return(JS_FALSE);

	*rval = STRING_TO_JSVAL(js_str);
	return(JS_TRUE);
}

static JSBool
js_ascii_str(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char*		p;
	char*		str;
	JSString*	js_str;

	if((str=JS_GetStringBytes(JS_ValueToString(cx, argv[0])))==NULL) 
		return(JS_FALSE);

	if((p=strdup(str))==NULL)
		return(JS_FALSE);

	ascii_str(p);

	js_str = JS_NewStringCopyZ(cx, p);
	free(p);
	if(js_str==NULL)
		return(JS_FALSE);

	*rval = STRING_TO_JSVAL(js_str);
	return(JS_TRUE);
}


static JSBool
js_strip_ctrl(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char*		p;
	char*		str;
	JSString*	js_str;

	if((str=JS_GetStringBytes(JS_ValueToString(cx, argv[0])))==NULL) 
		return(JS_FALSE);

	if((p=strdup(str))==NULL)
		return(JS_FALSE);

	strip_ctrl(p);

	js_str = JS_NewStringCopyZ(cx, p);
	free(p);
	if(js_str==NULL)
		return(JS_FALSE);

	*rval = STRING_TO_JSVAL(js_str);
	return(JS_TRUE);
}

static JSBool
js_strip_exascii(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char*		p;
	char*		str;
	JSString*	js_str;

	if((str=JS_GetStringBytes(JS_ValueToString(cx, argv[0])))==NULL) 
		return(JS_FALSE);

	if((p=strdup(str))==NULL)
		return(JS_FALSE);

	strip_exascii(p);

	js_str = JS_NewStringCopyZ(cx, p);
	free(p);
	if(js_str==NULL)
		return(JS_FALSE);

	*rval = STRING_TO_JSVAL(js_str);
	return(JS_TRUE);
}

static JSBool
js_lfexpand(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	ulong		i,j;
	char*		inbuf;
	char*		outbuf;
	JSString*	js_str;

	if((inbuf=JS_GetStringBytes(JS_ValueToString(cx, argv[0])))==NULL) 
		return(JS_FALSE);

	if((outbuf=(char*)malloc((strlen(inbuf)*2)+1))==NULL)
		return(JS_FALSE);

	for(i=j=0;inbuf[i];i++) {
		if(inbuf[i]=='\n' && (!i || inbuf[i-1]!='\r'))
			outbuf[j++]='\r';
		outbuf[j++]=inbuf[i];
	}
	outbuf[j]=0;

	js_str = JS_NewStringCopyZ(cx, outbuf);
	free(outbuf);
	if(js_str==NULL)
		return(JS_FALSE);

	*rval = STRING_TO_JSVAL(js_str);
	return(JS_TRUE);
}

static JSBool
js_word_wrap(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	int32		l,len=79;
	ulong		i,k;
	int			col=1;
	uchar*		inbuf;
	char*		outbuf;
	char*		linebuf;
	JSString*	js_str;

	if((inbuf=JS_GetStringBytes(JS_ValueToString(cx, argv[0])))==NULL) 
		return(JS_FALSE);

	if((outbuf=(char*)malloc((strlen(inbuf)*2)+1))==NULL)
		return(JS_FALSE);

	if(argc>1)
		JS_ValueToInt32(cx,argv[1],&len);

	if((linebuf=(char*)malloc((len*2)+2))==NULL) /* room for ^A codes */
		return(JS_FALSE);

	outbuf[0]=0;
	for(i=l=0;inbuf[i];) {
		if(inbuf[i]=='\r' || inbuf[i]==FF) {
			strncat(outbuf,linebuf,l);
			l=0;
			col=1;
		}
		else if(inbuf[i]=='\t') {
			if((col%8)==0)
				col++;
			col+=(col%8);
		} else if(inbuf[i]==CTRL_A && inbuf[i+1]!=0) {
			if(l<(len*2)) {
				strncpy(linebuf+l,inbuf+i,2);
				l+=2;
			}
			i+=2;
			continue;
		} else if(inbuf[i]>=' ')
			col++;
		linebuf[l]=inbuf[i++];
		if(col<=len) {
			l++;
			continue;
		}
		/* wrap line here */
		k=l;
		while(k && linebuf[k]>' ' && linebuf[k-1]!=CTRL_A) k--;
		if(k==0)	/* continuous printing chars, no word wrap possible */
			strncat(outbuf,linebuf,l+1);
		else {
			i-=(l-k);	/* rewind to start of next word */
			linebuf[k]=0;
			truncsp(linebuf);
			strcat(outbuf,linebuf);
		}
		strcat(outbuf,"\r\n");
		/* skip white space (but no more than one LF) for starting of new line */
		while(inbuf[i] && inbuf[i]<=' ' && inbuf[i]!='\n' && inbuf[i]!=CTRL_A) i++;	
		if(inbuf[i]=='\n') i++;
		l=0;
		col=1;
	}
	if(l)	/* remainder */
		strncat(outbuf,linebuf,l);

	js_str = JS_NewStringCopyZ(cx, outbuf);
	free(outbuf);
	if(js_str==NULL)
		return(JS_FALSE);

	*rval = STRING_TO_JSVAL(js_str);
	return(JS_TRUE);
}

static JSBool
js_quote_msg(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	int32		len=79;
	int			i,l;
	uchar*		inbuf;
	char*		outbuf;
	char*		linebuf;
	char*		prefix=" > ";
	JSString*	js_str;

	if((inbuf=JS_GetStringBytes(JS_ValueToString(cx, argv[0])))==NULL) 
		return(JS_FALSE);

	if(argc>1)
		JS_ValueToInt32(cx,argv[1],&len);

	if(argc>2)
		prefix=JS_GetStringBytes(JS_ValueToString(cx, argv[2]));

	if((outbuf=(char*)malloc((strlen(inbuf)*strlen(prefix))+1))==NULL)
		return(JS_FALSE);

	if((linebuf=(char*)malloc(len+1))==NULL)
		return(JS_FALSE);

	outbuf[0]=0;
	for(i=l=0;inbuf[i];i++) {
		if(l==0)
			strcat(outbuf,prefix);
		if(l<len)
			linebuf[l++]=inbuf[i];
		if(inbuf[i]=='\n') {
			strncat(outbuf,linebuf,l);
			l=0;
		}
	}
	if(l)	/* remainder */
		strncat(outbuf,linebuf,l);

	js_str = JS_NewStringCopyZ(cx, outbuf);
	free(outbuf);
	if(js_str==NULL)
		return(JS_FALSE);

	*rval = STRING_TO_JSVAL(js_str);
	return(JS_TRUE);
}

static JSBool
js_netaddr_type(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char*	str;

	if((str=JS_GetStringBytes(JS_ValueToString(cx, argv[0])))==NULL) 
		return(JS_FALSE);

	*rval = INT_TO_JSVAL(smb_netaddr_type(str));

	return(JS_TRUE);
}

static JSBool
js_rot13(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char*		p;
	char*		str;
	JSString*	js_str;

	if((str=JS_GetStringBytes(JS_ValueToString(cx, argv[0])))==NULL) 
		return(JS_FALSE);

	if((p=strdup(str))==NULL)
		return(JS_FALSE);

	js_str = JS_NewStringCopyZ(cx, rot13(p));
	free(p);
	if(js_str==NULL)
		return(JS_FALSE);

	*rval = STRING_TO_JSVAL(js_str);
	return(JS_TRUE);
}

/* This table is used to convert between IBM ex-ASCII and HTML character entities */
/* Much of this table supplied by Deuce (thanks!) */
static struct { 
	int		value;
	char*	name;
} exasctbl[128] = {
/*  HTML val,name             ASCII  description */
	{ 199	,"Ccedil"	}, /* 128 C, cedilla */
	{ 252	,"uuml"		}, /* 129 u, umlaut */
	{ 233	,"eacute"	}, /* 130 e, acute accent */
	{ 226	,"acirc"	}, /* 131 a, circumflex accent */
	{ 228	,"auml"		}, /* 132 a, umlaut */
	{ 224	,"agrave"	}, /* 133 a, grave accent */
	{ 229	,"aring"	}, /* 134 a, ring */
	{ 231	,"ccedil"	}, /* 135 c, cedilla */
	{ 234	,"ecirc"	}, /* 136 e, circumflex accent */
	{ 235	,"euml"		}, /* 137 e, umlaut */
	{ 232	,"egrave"	}, /* 138 e, grave accent */
	{ 239	,"iuml"		}, /* 139 i, umlaut */
	{ 238	,"icirc"	}, /* 140 i, circumflex accent */
	{ 236	,"igrave"	}, /* 141 i, grave accent */
	{ 196	,"Auml"		}, /* 142 A, umlaut */
	{ 197	,"Aring"	}, /* 143 A, ring */
	{ 201	,"Eacute"	}, /* 144 E, acute accent */
	{ 230	,"aelig"	}, /* 145 ae ligature */
	{ 198	,"AElig"	}, /* 146 AE ligature */
	{ 244	,"ocirc"	}, /* 147 o, circumflex accent */
	{ 246	,"ouml"		}, /* 148 o, umlaut */
	{ 242	,"ograve"	}, /* 149 o, grave accent */
	{ 251	,"ucirc"	}, /* 150 u, circumflex accent */
	{ 249	,"ugrave"	}, /* 151 u, grave accent */
	{ 255	,"yuml"		}, /* 152 y, umlaut */
	{ 214	,"Ouml"		}, /* 153 O, umlaut */
	{ 220	,"Uuml"		}, /* 154 U, umlaut */
	{ 162	,"cent"		}, /* 155 Cent sign */
	{ 163	,"pound"	}, /* 156 Pound sign */
	{ 165	,"yen"		}, /* 157 Yen sign */
	{ 8359	,NULL		}, /* 158 Pt (unicode) */
	{ 402	,NULL		}, /* 402 Florin (non-standard alsi 159?) */
	{ 225	,"aacute"	}, /* 160 a, acute accent */
	{ 237	,"iacute"	}, /* 161 i, acute accent */
	{ 243	,"oacute"	}, /* 162 o, acute accent */
	{ 250	,"uacute"	}, /* 163 u, acute accent */
	{ 241	,"ntilde"	}, /* 164 n, tilde */
	{ 209	,"Ntilde"	}, /* 165 N, tilde */
	{ 170	,"ordf"		}, /* 166 Feminine ordinal */
	{ 186	,"ordm"		}, /* 167 Masculine ordinal */
	{ 191	,"iquest"	}, /* 168 Inverted question mark */
	{ 8976	,NULL		}, /* 169 Inverse "Not sign" (unicode) */
	{ 172	,"not"		}, /* 170 Not sign */
	{ 189	,"frac12"	}, /* 171 Fraction one-half */
	{ 188	,"frac14"	}, /* 172 Fraction one-fourth */
	{ 161	,"iexcl"	}, /* 173 Inverted exclamation point */
	{ 171	,"laquo"	}, /* 174 Left angle quote */
	{ 187	,"raquo"	}, /* 175 Right angle quote */
	{ 9617	,NULL		}, /* 176 drawing symbol (unicode) */
	{ 9618	,NULL		}, /* 177 drawing symbol (unicode) */
	{ 9619	,NULL		}, /* 178 drawing symbol (unicode) */
	{ 9474	,NULL		}, /* 179 drawing symbol (unicode) */
	{ 9508	,NULL		}, /* 180 drawing symbol (unicode) */
	{ 9569	,NULL		}, /* 181 drawing symbol (unicode) */
	{ 9570	,NULL		}, /* 182 drawing symbol (unicode) */
	{ 9558	,NULL		}, /* 183 drawing symbol (unicode) */
	{ 9557	,NULL		}, /* 184 drawing symbol (unicode) */
	{ 9571	,NULL		}, /* 185 drawing symbol (unicode) */
	{ 9553	,NULL		}, /* 186 drawing symbol (unicode) */
	{ 9559	,NULL		}, /* 187 drawing symbol (unicode) */
	{ 9565	,NULL		}, /* 188 drawing symbol (unicode) */
	{ 9564	,NULL		}, /* 189 drawing symbol (unicode) */
	{ 9563	,NULL		}, /* 190 drawing symbol (unicode) */
	{ 9488	,NULL		}, /* 191 drawing symbol (unicode) */
	{ 9492	,NULL		}, /* 192 drawing symbol (unicode) */
	{ 9524	,NULL		}, /* 193 drawing symbol (unicode) */
	{ 9516	,NULL		}, /* 194 drawing symbol (unicode) */
	{ 9500	,NULL		}, /* 195 drawing symbol (unicode) */
	{ 9472	,NULL		}, /* 196 drawing symbol (unicode) */
	{ 9532	,NULL		}, /* 197 drawing symbol (unicode) */
	{ 9566	,NULL		}, /* 198 drawing symbol (unicode) */
	{ 9567	,NULL		}, /* 199 drawing symbol (unicode) */
	{ 9562	,NULL		}, /* 200 drawing symbol (unicode) */
	{ 9556	,NULL		}, /* 201 drawing symbol (unicode) */
	{ 9577	,NULL		}, /* 202 drawing symbol (unicode) */
	{ 9574	,NULL		}, /* 203 drawing symbol (unicode) */
	{ 9568	,NULL		}, /* 204 drawing symbol (unicode) */
	{ 9552	,NULL		}, /* 205 drawing symbol (unicode) */
	{ 9580	,NULL		}, /* 206 drawing symbol (unicode) */
	{ 9575	,NULL		}, /* 207 drawing symbol (unicode) */
	{ 9576	,NULL		}, /* 208 drawing symbol (unicode) */
	{ 9572	,NULL		}, /* 209 drawing symbol (unicode) */
	{ 9573	,NULL		}, /* 210 drawing symbol (unicode) */
	{ 9561	,NULL		}, /* 211 drawing symbol (unicode) */
	{ 9560	,NULL		}, /* 212 drawing symbol (unicode) */
	{ 9554	,NULL		}, /* 213 drawing symbol (unicode) */
	{ 9555	,NULL		}, /* 214 drawing symbol (unicode) */
	{ 9579	,NULL		}, /* 215 drawing symbol (unicode) */
	{ 9578	,NULL		}, /* 216 drawing symbol (unicode) */
	{ 9496	,NULL		}, /* 217 drawing symbol (unicode) */
	{ 9484	,NULL		}, /* 218 drawing symbol (unicode) */
	{ 9608	,NULL		}, /* 219 drawing symbol (unicode) */
	{ 9604	,NULL		}, /* 220 drawing symbol (unicode) */
	{ 9612	,NULL		}, /* 221 drawing symbol (unicode) */
	{ 9616	,NULL		}, /* 222 drawing symbol (unicode) */
	{ 9600	,NULL		}, /* 223 drawing symbol (unicode) */
	{ 945	,NULL		}, /* 224 alpha symbol */
	{ 223	,"szlig"	}, /* 225 sz ligature (beta symbol) */
	{ 915	,NULL		}, /* 226 omega symbol */
	{ 960	,NULL		}, /* 227 pi symbol*/
	{ 931	,NULL		}, /* 228 epsilon symbol */
	{ 963	,NULL		}, /* 229 o with stick */
	{ 181	,"micro"	}, /* 230 Micro sign (Greek mu) */
	{ 964	,NULL		}, /* 231 greek char? */
	{ 934	,NULL		}, /* 232 greek char? */
	{ 920	,NULL		}, /* 233 greek char? */
	{ 937	,NULL		}, /* 234 greek char? */
	{ 948	,NULL		}, /* 235 greek char? */
	{ 8734	,NULL		}, /* 236 infinity symbol (unicode) */
	{ 966	,"oslash"	}, /* 237 Greek Phi */
	{ 949	,NULL		}, /* 238 rounded E */
	{ 8745	,NULL		}, /* 239 unside down U (unicode) */
	{ 8801	,NULL		}, /* 240 drawing symbol (unicode) */
	{ 177	,"plusmn"	}, /* 241 Plus or minus */
	{ 8805	,NULL		}, /* 242 drawing symbol (unicode) */
	{ 8804	,NULL		}, /* 243 drawing symbol (unicode) */
	{ 8992	,NULL		}, /* 244 drawing symbol (unicode) */
	{ 8993	,NULL		}, /* 245 drawing symbol (unicode) */
	{ 247	,"divide"	}, /* 246 Division sign */
	{ 8776	,NULL		}, /* 247 two squiggles (unicode) */
	{ 176	,"deg"		}, /* 248 Degree sign */
	{ 8729	,NULL		}, /* 249 drawing symbol (unicode) */
	{ 183	,"middot"	}, /* 250 Middle dot */
	{ 8730	,NULL		}, /* 251 check mark (unicode) */
	{ 8319	,NULL		}, /* 252 superscript n (unicode) */
	{ 178	,"sup2"		}, /* 253 superscript 2 */
	{ 9632	,NULL		}, /* 254 drawing symbol (unicode) */
	{ 160	,"nbsp"		}  /* 255 non-breaking space */
};

static struct { 
	int		value;
	char*	name;
} lowasctbl[32] = {
	{ 160	,"nbsp"		}, /* NULL non-breaking space */
	{ 9786	,NULL		}, /* white smiling face */
	{ 9787	,NULL		}, /* black smiling face */
	{ 9829	,"hearts"	}, /* black heart suit */
	{ 9830	,"diams"	}, /* black diamond suit */
	{ 9827	,"clubs"	}, /* black club suit */
	{ 9824	,"spades"	}, /* black spade suit */
	{ 8226	,"bull"		}, /* bullet */
	{ 9688	,NULL		}, /* inverse bullet */
	{ 9702	,NULL		}, /* white bullet */
	{ 9689	,NULL		}, /* inverse white circle */
	{ 9794	,NULL		}, /* male sign */
	{ 9792	,NULL		}, /* female sign */
	{ 9834	,NULL		}, /* eighth note */
	{ 9835	,NULL		}, /* beamed eighth notes */
	{ 9788	,NULL		}, /* white sun with rays */
	{ 9654	,NULL		}, /* black right-pointing triangle */
	{ 9664	,NULL		}, /* black left-pointing triangle */
	{ 8597	,NULL		}, /* up down arrow */
	{ 8252	,NULL		}, /* double exclamation mark */
	{ 182	,"para"		}, /* pilcrow sign */
	{ 167	,"sect"		}, /* section sign */
	{ 9644	,NULL		}, /* black rectangle */
	{ 8616	,NULL		}, /* up down arrow with base */
	{ 8593	,"uarr"		}, /* upwards arrow */
	{ 8595	,"darr"		}, /* downwards arrow */
	{ 8594	,"rarr"		}, /* rightwards arrow */
	{ 8592	,"larr"		}, /* leftwards arrow */
	{ 8985	,NULL		}, /* turned not sign */
	{ 8596	,"harr"		}, /* left right arrow */
	{ 9650	,NULL		}, /* black up-pointing triangle */
	{ 9660	,NULL		}  /* black down-pointing triangle */
};

static JSBool
js_html_encode(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	int			ch;
	ulong		i,j;
	uchar*		inbuf;
	uchar*		tmpbuf;
	uchar*		outbuf;
	uchar*		param;
	char*		lastparam;
	JSBool		exascii=JS_TRUE;
	JSBool		wsp=JS_TRUE;
	JSBool		ansi=JS_TRUE;
	JSBool		ctrl_a=JS_TRUE;
	JSString*	js_str;
	int			fg=7;
	int			bg=0;
	BOOL		blink=FALSE;
	BOOL		bold=FALSE;
	int			esccount=0;
	char		ansi_seq[MAX_ANSI_SEQ+1];
	int			ansi_param[MAX_ANSI_PARAMS];
	int			k,l;
	ulong		savepos=0;
	int			hpos=0;
	int			currrow=0;
	int			savehpos=0;
	int			savevpos=0;
	int			wraphpos=-2;
	int			wrapvpos=-2;
	ulong		wrappos=0;
	BOOL		extchar=FALSE;
	ulong		obsize;
	int			lastcolor=7;
	char		tmp1[128];
	struct		tm tm;
	time_t		now;
	BOOL		nodisplay=FALSE;
	scfg_t*		cfg;
	uchar   	attr_stack[64]; /* Saved attributes (stack) */
	int     	attr_sp=0;                /* Attribute stack pointer */
	ulong		clear_screen=0;

	if((cfg=(scfg_t*)JS_GetPrivate(cx,obj))==NULL)		/* Will this work?  Ask DM */
		return(JS_FALSE);

	if((inbuf=JS_GetStringBytes(JS_ValueToString(cx, argv[0])))==NULL)
		return(JS_FALSE);

	if(argc>1 && JSVAL_IS_BOOLEAN(argv[1]))
		exascii=JSVAL_TO_BOOLEAN(argv[1]);

	if(argc>2 && JSVAL_IS_BOOLEAN(argv[2]))
		wsp=JSVAL_TO_BOOLEAN(argv[2]);

	if(argc>3 && JSVAL_IS_BOOLEAN(argv[3]))
		ansi=JSVAL_TO_BOOLEAN(argv[3]);

	if(argc>4 && JSVAL_IS_BOOLEAN(argv[4]))
	{
		ctrl_a=JSVAL_TO_BOOLEAN(argv[4]);
		if(ctrl_a)
			ansi=ctrl_a;
	}

	if((tmpbuf=(char*)malloc((strlen(inbuf)*10)+1))==NULL)
		return(JS_FALSE);

	for(i=j=0;inbuf[i];i++) {
		switch(inbuf[i]) {
			case TAB:
			case LF:
			case CR:
				if(wsp)
					j+=sprintf(tmpbuf+j,"&#%u;",inbuf[i]);
				else
					tmpbuf[j++]=inbuf[i];
				break;
			case '"':
				j+=sprintf(tmpbuf+j,"&quot;");
				break;
			case '&':
				j+=sprintf(tmpbuf+j,"&amp;");
				break;
			case '<':
				j+=sprintf(tmpbuf+j,"&lt;");
				break;
			case '>':
				j+=sprintf(tmpbuf+j,"&gt;");
				break;
			case '\b':
				j--;
				break;
			default:
				if(inbuf[i]&0x80) {
					if(exascii) {
						ch=inbuf[i]^0x80;
						if(exasctbl[ch].name!=NULL)
							j+=sprintf(tmpbuf+j,"&%s;",exasctbl[ch].name);
						else
							j+=sprintf(tmpbuf+j,"&#%u;",exasctbl[ch].value);
					} else
						tmpbuf[j++]=inbuf[i];
				}
				else if(inbuf[i]>=' ' && inbuf[i]<DEL)
					tmpbuf[j++]=inbuf[i];
#if 0		/* ASCII 127 - Not displayed? */
				else if(inbuf[i]==DEL && exascii)
					j+=sprintf(tmpbuf+j,"&#8962;",exasctbl[ch].value);
#endif
				else if(inbuf[i]<' ') /* unknown control chars */
				{
					if(ansi && inbuf[i]==ESC)
					{
						esccount++;
						tmpbuf[j++]=inbuf[i];
					}
					else if(ctrl_a && inbuf[i]==1)
					{
						esccount++;
						tmpbuf[j++]=inbuf[i];
						tmpbuf[j++]=inbuf[++i];
					}
					else if(exascii) {
						ch=inbuf[i];
						if(lowasctbl[ch].name!=NULL)
							j+=sprintf(tmpbuf+j,"&%s;",lowasctbl[ch].name);
						else
							j+=sprintf(tmpbuf+j,"&#%u;",lowasctbl[ch].value);
					} else
						j+=sprintf(tmpbuf+j,"&#%u;",inbuf[i]);
				}
				break;
		}
	}
	tmpbuf[j]=0;

	if(ansi)
	{
		obsize=(strlen(tmpbuf)+(esccount+1)*MAX_COLOR_STRING)+1;
		if(obsize<2048)
			obsize=2048;
		if((outbuf=(uchar*)malloc(obsize))==NULL)
		{
			free(tmpbuf);
			return(JS_FALSE);
		}
		j=sprintf(outbuf,"<span style=\"%s\">",htmlansi[7]);
		clear_screen=j;
		for(i=0;tmpbuf[i];i++) {
			if(j>(obsize/2))		/* Completely arbitrary here... must be carefull with this eventually ToDo */
			{
				obsize+=(obsize/2);
				if((param=realloc(outbuf,obsize))==NULL)
				{
					free(tmpbuf);
					free(outbuf);
					return(JS_FALSE);
				}
				outbuf=param;
			}
			if(tmpbuf[i]==ESC && tmpbuf[i+1]=='[')
			{
				if(nodisplay)
					continue;
				k=0;
				memset(&ansi_param,0xff,sizeof(int)*MAX_ANSI_PARAMS);
				strncpy(ansi_seq, tmpbuf+i+2, MAX_ANSI_SEQ);
				ansi_seq[MAX_ANSI_SEQ]=0;
				for(lastparam=ansi_seq;*lastparam;lastparam++)
				{
					if(isalpha(*lastparam))
					{
						*(++lastparam)=0;
						break;
					}
				}
				k=0;
				param=ansi_seq;
				if(*param=='?')		/* This is to fix ESC[?7 whatever that is */
					param++;
				if(isdigit(*param))
					ansi_param[k++]=atoi(ansi_seq);
				while(isspace(*param) || isdigit(*param))
					param++;
				lastparam=param;
				while((param=strchr(param,';'))!=NULL)
				{
					param++;
					ansi_param[k++]=atoi(param);
					while(isspace(*param) || isdigit(*param))
						param++;
					lastparam=param;
				}
				switch(*lastparam)
				{
					case 'm':	/* Colour */
						for(k=0;ansi_param[k]>=0;k++)
						{
							switch(ansi_param[k])
							{
								case 0:
									fg=7;
									bg=0;
									blink=FALSE;
									bold=FALSE;
									break;
								case 1:
									bold=TRUE;
									break;
								case 2:
									bold=FALSE;
									break;
								case 5:
									blink=TRUE;
									break;
								case 6:
									blink=TRUE;
									break;
								case 7:
									l=fg;
									fg=bg;
									bg=l;
									break;
								case 8:
									fg=bg;
									blink=FALSE;
									bold=FALSE;
									break;
								case 30:
								case 31:
								case 32:
								case 33:
								case 34:
								case 35:
								case 36:
								case 37:
									fg=ansi_param[k]-30;
									break;
								case 40:
								case 41:
								case 42:
								case 43:
								case 44:
								case 45:
								case 46:
								case 47:
									bg=ansi_param[k]-40;
									break;
							}
						}
						break;
					case 'C': /* Move right */
						j+=sprintf(outbuf+j,"%s%s%s",HTML_COLOR_PREFIX,htmlansi[0],HTML_COLOR_SUFFIX);
						lastcolor=0;
						l=ansi_param[0]>0?ansi_param[0]:1;
						if(wrappos==0 && wrapvpos==currrow)  {
							/* j+=sprintf(outbuf+j,"<!-- \r\nC after A l=%d hpos=%d -->",l,hpos); */
							l=l-hpos;
							wrapvpos=-2;	/* Prevent additional move right */
						}
						if(l>81-hpos)
							l=81-hpos;
						for(k=0; k<l; k++)
						{
							j+=sprintf(outbuf+j,"%s","&nbsp;");
							hpos++;
						}
						break;
					case 's': /* Save position */
						savepos=j;
						savehpos=hpos;
						savevpos=currrow;
						break;
					case 'u': /* Restore saved position */
						j=savepos;
						hpos=savehpos;
						currrow=savevpos;
						break;
					case 'H': /* Move */
						k=ansi_param[0];
						if(k<=0)
							k=1;
						k--;
						l=ansi_param[1];
						if(l<=0)
							l=1;
						l--;
						while(k>currrow)
						{
							hpos=0;
							currrow++;
							outbuf[j++]='\r';
							outbuf[j++]='\n';
						}
						if(l>hpos)
						{
							j+=sprintf(outbuf+j,"%s%s%s",HTML_COLOR_PREFIX,htmlansi[0],HTML_COLOR_SUFFIX);
							lastcolor=0;
							while(l>hpos)
							{
								j+=sprintf(outbuf+j,"%s","&nbsp;");
								hpos++;
							}
						}
						break;
					case 'B': /* Move down */
						l=ansi_param[0];
						if(l<=0)
							l=1;
						for(k=0; k < l; k++)
						{
							currrow++;
							outbuf[j++]='\r';
							outbuf[j++]='\n';
						}
						if(hpos!=0 && tmpbuf[i+1]!=CR)
						{
							j+=sprintf(outbuf+j,"%s%s%s",HTML_COLOR_PREFIX,htmlansi[0],HTML_COLOR_SUFFIX);
							lastcolor=0;
							for(k=0; k<hpos ; k++)
							{
								j+=sprintf(outbuf+j,"%s","&nbsp;");
							}
							break;
						}
						break;
					case 'A': /* Move up */
						l=wrappos;
						if(j > wrappos && hpos==0 && currrow==wrapvpos+1 && ansi_param[0]<=1)  {
							hpos=wraphpos;
							currrow=wrapvpos;
							j=wrappos;
							wrappos=0; /* Prevent additional move up */
						}
						break;
					case 'J': /* Clear */
						if(ansi_param[0]==2)  {
							j=clear_screen;
							hpos=0;
							currrow=0;
							wraphpos=-2;
							wrapvpos=-2;
							wrappos=0;
						}
						break;
				}
				i+=(int)(lastparam-ansi_seq)+2;
			}
			else if(ctrl_a && tmpbuf[i]==1)		/* CTRL-A codes */
			{
/*				j+=sprintf(outbuf+j,"<!-- CTRL-A-%c (%u) -->",tmpbuf[i+1],tmpbuf[i+1]); */
				if(nodisplay && tmpbuf[i+1] != ')')
					continue;
				if(tmpbuf[i+1]>0x7f)
				{
					j+=sprintf(outbuf+j,"%s%s%s",HTML_COLOR_PREFIX,htmlansi[0],HTML_COLOR_SUFFIX);
					lastcolor=0;
					l=tmpbuf[i+1]-0x7f;
					if(l>81-hpos)
						l=81-hpos;
					for(k=0; k<l; k++)
					{
						j+=sprintf(outbuf+j,"%s","&nbsp;");
						hpos++;
					}
				}
				else switch(toupper(tmpbuf[i+1]))
				{
					case 'K':
						fg=0;
						break;
					case 'R':
						fg=1;
						break;
					case 'G':
						fg=2;
						break;
					case 'Y':
						fg=3;
						break;
					case 'B':
						fg=4;
						break;
					case 'M':
						fg=5;
						break;
					case 'C':
						fg=6;
						break;
					case 'W':
						fg=7;
						break;
					case '0':
						bg=0;
						break;
					case '1':
						bg=1;
						break;
					case '2':
						bg=2;
						break;
					case '3':
						bg=3;
						break;
					case '4':
						bg=4;
						break;
					case '5':
						bg=5;
						break;
					case '6':
						bg=6;
						break;
					case '7':
						bg=7;
						break;
					case 'H':
						bold=TRUE;
						break;
					case 'I':
						blink=TRUE;
						break;
					case '+':
						if(attr_sp<(int)sizeof(attr_stack))
							attr_stack[attr_sp++]=(blink?(1<<7):0) | (bg << 4) | (bold?(1<<3):0) | fg;
						break;
					case '-':
						if(attr_sp>0)
						{
							blink=(attr_stack[--attr_sp]&(1<<7))?TRUE:FALSE;
							bg=(attr_stack[attr_sp] >> 4) & 7;
							blink=(attr_stack[attr_sp]&(1<<3))?TRUE:FALSE;
							fg=attr_stack[attr_sp] & 7;
						}
						else if(bold || blink || bg)
						{
							bold=FALSE;
							blink=FALSE;
							fg=7;
							bg=0;
						}
						break;
					case '_':
						if(blink || bg)
						{
							bold=FALSE;
							blink=FALSE;
							fg=7;
							bg=0;
						}
						break;
					case 'N':
						bold=FALSE;
						blink=FALSE;
						fg=7;
						bg=0;
						break;
					case 'P':
					case 'Q':
					case ',':
					case ';':
					case '.':
					case 'S':
					case '>':
					case '<':
						break;

					case '!':		/* This needs to be fixed! (Somehow) */
					case '@':
					case '#':
					case '$':
					case '%':
					case '^':
					case '&':
					case '*':
					case '(':
						nodisplay=TRUE;
						break;
					case ')':
						nodisplay=FALSE;
						break;

					case 'D':
						now=time(NULL);
						j+=sprintf(outbuf+j,"%s",unixtodstr(cfg,now,tmp1));
						break;
					case 'T':
						now=time(NULL);
						localtime_r(&now,&tm);
						j+=sprintf(outbuf+j,"%02d:%02d %s"
							,tm.tm_hour==0 ? 12
							: tm.tm_hour>12 ? tm.tm_hour-12
							: tm.tm_hour, tm.tm_min, tm.tm_hour>11 ? "pm":"am");
						break;
						
					case 'L':
						currrow=0;
						hpos=0;
						outbuf[j++]='\r';
						outbuf[j++]='\n';
						break;
					case ']':
						currrow++;
						if(hpos!=0 && tmpbuf[i+2]!=CR && !(tmpbuf[i+2]==1 && tmpbuf[i+3]=='['))
						{
							outbuf[j++]='\r';
							outbuf[j++]='\n';
							j+=sprintf(outbuf+j,"%s%s%s",HTML_COLOR_PREFIX,htmlansi[0],HTML_COLOR_SUFFIX);
							lastcolor=0;
							for(k=0; k<hpos ; k++)
							{
								j+=sprintf(outbuf+j,"%s","&nbsp;");
							}
							break;
						}
						outbuf[j++]='\n';
						break;
					case '[':
						outbuf[j++]='\r';
						hpos=0;
						break;
					case 'Z':
						outbuf[j++]=0;
						break;
					case 'A':
					default:
						if(exascii) {
							ch=tmpbuf[i];
							if(lowasctbl[ch].name!=NULL)
								j+=sprintf(outbuf+j,"&%s;",lowasctbl[ch].name);
							else
								j+=sprintf(outbuf+j,"&#%u;",lowasctbl[ch].value);
						} else
							j+=sprintf(outbuf+j,"&#%u;",inbuf[i]);
						i--;
				}
				i++;
			}
			else
			{
				if(nodisplay)
					continue;
				switch(tmpbuf[i])
				{
					case TAB:			/* This assumes that tabs do NOT use the current background. */
						l=hpos%8;
						if(l==0)
							l=8;
						j+=sprintf(outbuf+j,"%s%s%s",HTML_COLOR_PREFIX,htmlansi[0],HTML_COLOR_SUFFIX);
						lastcolor=0;
						for(k=0; k<l ; k++)
						{
							j+=sprintf(outbuf+j,"%s","&nbsp;");
							hpos++;
						}
						break;
					case LF:
						wrapvpos=currrow;
						if(wrappos<j-3)
							wrappos=j;
						currrow++;
						if(hpos!=0 && tmpbuf[i+1]!=CR)
						{
							outbuf[j++]='\r';
							outbuf[j++]='\n';
							j+=sprintf(outbuf+j,"%s%s%s",HTML_COLOR_PREFIX,htmlansi[0],HTML_COLOR_SUFFIX);
							lastcolor=0;
							for(k=0; k<hpos ; k++)
							{
								j+=sprintf(outbuf+j,"%s","&nbsp;");
							}
							break;
						}
					case CR:
						if(wraphpos==-2 || hpos!=0)
							wraphpos=hpos;
						if(wrappos<j-3)
							wrappos=j;
						outbuf[j++]=tmpbuf[i];
						hpos=0;
						break;
					default:
						if(lastcolor != ((blink?(1<<7):0) | (bg << 4) | (bold?(1<<3):0) | fg))
						{
							lastcolor=(blink?(1<<7):0) | (bg << 4) | (bold?(1<<3):0) | fg;
							j+=sprintf(outbuf+j,"%s%s%s",HTML_COLOR_PREFIX,htmlansi[lastcolor],HTML_COLOR_SUFFIX);
						}
						if(hpos>=80 && tmpbuf[i+1] != '\r' && tmpbuf[i+1] != '\n' && tmpbuf[i+1] != ESC)
						{
							wrapvpos=-2;
							wraphpos=-2;
							wrappos=0;
							hpos=0;
							currrow++;
							outbuf[j++]='\r';
							outbuf[j++]='\n';
						}
						outbuf[j++]=tmpbuf[i];
						if(tmpbuf[i]=='&')
							extchar=TRUE;
						if(tmpbuf[i]==';')
							extchar=FALSE;
						if(!extchar)
							hpos++;
				}
			}
		}
		strcpy(outbuf+j,"</span>");

		js_str = JS_NewStringCopyZ(cx, outbuf);
		free(outbuf);
	}
	else
		js_str = JS_NewStringCopyZ(cx, tmpbuf);

	free(tmpbuf);
	if(js_str==NULL)
		return(JS_FALSE);

	*rval = STRING_TO_JSVAL(js_str);
	return(JS_TRUE);
}

static JSBool
js_html_decode(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	int			ch;
	int			val;
	ulong		i,j;
	uchar*		inbuf;
	uchar*		outbuf;
	char		token[16];
	size_t		t;
	JSString*	js_str;

	if((inbuf=JS_GetStringBytes(JS_ValueToString(cx, argv[0])))==NULL) 
		return(JS_FALSE);

	if((outbuf=(char*)malloc(strlen(inbuf)+1))==NULL)
		return(JS_FALSE);

	for(i=j=0;inbuf[i];i++) {
		if(inbuf[i]!='&') {
			outbuf[j++]=inbuf[i];
			continue;
		}
		for(i++,t=0; inbuf[i]!=0 && inbuf[i]!=';' && t<sizeof(token)-1; i++, t++)
			token[t]=inbuf[i];
		if(inbuf[i]==0)
			break;
		token[t]=0;

		/* First search the ex-ascii table for a name match */
		for(ch=0;ch<128;ch++)
			if(exasctbl[ch].name!=NULL && strcmp(token,exasctbl[ch].name)==0)
				break;
		if(ch<128) {
			outbuf[j++]=ch|0x80;
			continue;
		}
		if(token[0]=='#') {		/* numeric constant */
			val=atoi(token+1);

			/* search ex-ascii table for a value match */
			for(ch=0;ch<128;ch++)
				if(exasctbl[ch].value==val)
					break;
			if(ch<128) {
				outbuf[j++]=ch|0x80;
				continue;
			}

			if((val>=' ' && val<=0xff) || val=='\r' || val=='\n' || val=='\t') {
				outbuf[j++]=val;
				continue;
			}
		}
		if(strcmp(token,"quot")==0) {
			outbuf[j++]='"';
			continue;
		}
		if(strcmp(token,"amp")==0) {
			outbuf[j++]='&';
			continue;
		}
		if(strcmp(token,"lt")==0) {
			outbuf[j++]='<';
			continue;
		}
		if(strcmp(token,"gt")==0) {
			outbuf[j++]='>';
			continue;
		}
		if(strcmp(token,"curren")==0) {
			outbuf[j++]=CTRL_O;
			continue;
		}
		if(strcmp(token,"para")==0) {
			outbuf[j++]=CTRL_T;
			continue;
		}
		if(strcmp(token,"sect")==0) {
			outbuf[j++]=CTRL_U;
			continue;
		}
		/* Unknown character entity, leave intact */
		j+=sprintf(outbuf+j,"&%s;",token);
		
	}
	outbuf[j]=0;

	js_str = JS_NewStringCopyZ(cx, outbuf);
	free(outbuf);
	if(js_str==NULL)
		return(JS_FALSE);

	*rval = STRING_TO_JSVAL(js_str);
	return(JS_TRUE);
}

static JSBool
js_b64_encode(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	int			res;
	size_t		len;
	uchar*		inbuf;
	uchar*		outbuf;
	JSString*	js_str;

	*rval = JSVAL_NULL;

	if((inbuf=JS_GetStringBytes(JS_ValueToString(cx, argv[0])))==NULL) 
		return(JS_FALSE);

	len=(strlen(inbuf)*10)+1;
	if((outbuf=(char*)malloc(len))==NULL)
		return(JS_FALSE);

	res=b64_encode(outbuf,len,inbuf,strlen(inbuf));

	if(res<1) {
		free(outbuf);
		return(JS_TRUE);
	}

	js_str = JS_NewStringCopyZ(cx, outbuf);
	free(outbuf);
	if(js_str==NULL)
		return(JS_FALSE);

	*rval = STRING_TO_JSVAL(js_str);
	return(JS_TRUE);
}

static JSBool
js_b64_decode(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	int			res;
	size_t		len;
	uchar*		inbuf;
	uchar*		outbuf;
	JSString*	js_str;

	*rval = JSVAL_NULL;

	if((inbuf=JS_GetStringBytes(JS_ValueToString(cx, argv[0])))==NULL) 
		return(JS_FALSE);

	len=strlen(inbuf)+1;
	if((outbuf=(char*)malloc(len))==NULL)
		return(JS_FALSE);

	res=b64_decode(outbuf,len,inbuf,strlen(inbuf));

	if(res<1) {
		free(outbuf);
		return(JS_TRUE);
	}

	js_str = JS_NewStringCopyN(cx, outbuf, res);
	free(outbuf);
	if(js_str==NULL)
		return(JS_FALSE);

	*rval = STRING_TO_JSVAL(js_str);
	return(JS_TRUE);
}

static JSBool
js_md5_calc(JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval)
{
	BYTE		digest[MD5_DIGEST_SIZE];
	JSBool		hex=JS_FALSE;
	char*		inbuf;
	char		outbuf[64];
	JSString*	js_str;

	*rval = JSVAL_NULL;

	if((inbuf=JS_GetStringBytes(JS_ValueToString(cx, argv[0])))==NULL) 
		return(JS_FALSE);

	if(argc>1 && JSVAL_IS_BOOLEAN(argv[1]))
		hex=JSVAL_TO_BOOLEAN(argv[1]);

	MD5_calc(digest,inbuf,strlen(inbuf));

	if(hex)
		MD5_hex(outbuf,digest);
	else
		b64_encode(outbuf,sizeof(outbuf),digest,sizeof(digest));

	js_str = JS_NewStringCopyZ(cx, outbuf);
	if(js_str==NULL)
		return(JS_FALSE);

	*rval = STRING_TO_JSVAL(js_str);
	return(JS_TRUE);
}

static JSBool
js_truncsp(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char*		p;
	char*		str;
	JSString*	js_str;

	if((str=JS_GetStringBytes(JS_ValueToString(cx, argv[0])))==NULL) 
		return(JS_FALSE);

	if((p=strdup(str))==NULL)
		return(JS_FALSE);

	truncsp(p);

	js_str = JS_NewStringCopyZ(cx, p);
	free(p);
	if(js_str==NULL)
		return(JS_FALSE);

	*rval = STRING_TO_JSVAL(js_str);
	return(JS_TRUE);
}

static JSBool
js_truncstr(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char*		p;
	char*		str;
	char*		set;
	JSString*	js_str;

	if((str=JS_GetStringBytes(JS_ValueToString(cx, argv[0])))==NULL) 
		return(JS_FALSE);

	if((set=JS_GetStringBytes(JS_ValueToString(cx, argv[1])))==NULL) 
		return(JS_FALSE);

	if((p=strdup(str))==NULL)
		return(JS_FALSE);

	truncstr(p,set);

	js_str = JS_NewStringCopyZ(cx, p);
	free(p);
	if(js_str==NULL)
		return(JS_FALSE);

	*rval = STRING_TO_JSVAL(js_str);
	return(JS_TRUE);
}

static JSBool
js_backslash(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char		path[MAX_PATH+1];
	char*		str;
	JSString*	js_str;

	if((str=JS_GetStringBytes(JS_ValueToString(cx, argv[0])))==NULL) 
		return(JS_FALSE);
	
	SAFECOPY(path,str);
	backslash(path);

	if((js_str = JS_NewStringCopyZ(cx, path))==NULL)
		return(JS_FALSE);

	*rval = STRING_TO_JSVAL(js_str);
	return(JS_TRUE);
}


static JSBool
js_getfname(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char*		str;
	JSString*	js_str;

	if((str=JS_GetStringBytes(JS_ValueToString(cx, argv[0])))==NULL) 
		return(JS_FALSE);

	js_str = JS_NewStringCopyZ(cx, getfname(str));
	if(js_str==NULL)
		return(JS_FALSE);

	*rval = STRING_TO_JSVAL(js_str);
	return(JS_TRUE);
}

static JSBool
js_getfext(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char*		str;
	char*		p;
	JSString*	js_str;

	if((str=JS_GetStringBytes(JS_ValueToString(cx, argv[0])))==NULL) 
		return(JS_FALSE);

	*rval = JSVAL_VOID;

	if((p=getfext(str))==NULL)
		return(JS_TRUE);

	js_str = JS_NewStringCopyZ(cx, p);
	if(js_str==NULL)
		return(JS_FALSE);

	*rval = STRING_TO_JSVAL(js_str);
	return(JS_TRUE);
}

static JSBool
js_getfcase(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char*		str;
	char		path[MAX_PATH+1];
	JSString*	js_str;

	if((str=JS_GetStringBytes(JS_ValueToString(cx, argv[0])))==NULL) 
		return(JS_FALSE);

	*rval = JSVAL_VOID;

	SAFECOPY(path,str);
	if(fexistcase(path)) {
		js_str = JS_NewStringCopyZ(cx, path);
		if(js_str==NULL)
			return(JS_FALSE);

		*rval = STRING_TO_JSVAL(js_str);
	}
	return(JS_TRUE);
}

static JSBool
js_cfgfname(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char*		path;
	char*		fname;
	char		result[MAX_PATH+1];

	if((path=JS_GetStringBytes(JS_ValueToString(cx, argv[0])))==NULL) 
		return(JS_FALSE);

	if((fname=JS_GetStringBytes(JS_ValueToString(cx, argv[1])))==NULL) 
		return(JS_FALSE);

	*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,iniFileName(result,sizeof(result),path,fname)));

	return(JS_TRUE);
}

static JSBool
js_fexist(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char*		p;

	if((p=JS_GetStringBytes(JS_ValueToString(cx, argv[0])))==NULL) {
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
		return(JS_TRUE);
	}

	*rval = BOOLEAN_TO_JSVAL(fexist(p));
	return(JS_TRUE);
}

static JSBool
js_remove(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char*		p;

	if((p=JS_GetStringBytes(JS_ValueToString(cx, argv[0])))==NULL) {
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
		return(JS_TRUE);
	}

	*rval = BOOLEAN_TO_JSVAL(remove(p)==0);
	return(JS_TRUE);
}

static JSBool
js_rename(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char*		oldname;
	char*		newname;

	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
	if((oldname=JS_GetStringBytes(JS_ValueToString(cx, argv[0])))==NULL)
		return(JS_TRUE);
	if((newname=JS_GetStringBytes(JS_ValueToString(cx, argv[1])))==NULL)
		return(JS_TRUE);

	*rval = BOOLEAN_TO_JSVAL(rename(oldname,newname)==0);
	return(JS_TRUE);
}

static JSBool
js_fcopy(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char*		src;
	char*		dest;

	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
	if((src=JS_GetStringBytes(JS_ValueToString(cx, argv[0])))==NULL)
		return(JS_TRUE);
	if((dest=JS_GetStringBytes(JS_ValueToString(cx, argv[1])))==NULL)
		return(JS_TRUE);

	*rval = BOOLEAN_TO_JSVAL(fcopy(src,dest));
	return(JS_TRUE);
}

static JSBool
js_backup(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char*		fname;
	int32		level=5;
	BOOL		ren=FALSE;

	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
	if((fname=JS_GetStringBytes(JS_ValueToString(cx, argv[0])))==NULL)
		return(JS_TRUE);

	if(argc>1)
		JS_ValueToInt32(cx,argv[1],&level);
	if(argc>2)
		JS_ValueToBoolean(cx,argv[2],&ren);

	*rval = BOOLEAN_TO_JSVAL(backup(fname,level,ren));
	return(JS_TRUE);
}

static JSBool
js_isdir(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char*		p;

	if((p=JS_GetStringBytes(JS_ValueToString(cx, argv[0])))==NULL) {
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
		return(JS_TRUE);
	}

	*rval = BOOLEAN_TO_JSVAL(isdir(p));
	return(JS_TRUE);
}

static JSBool
js_fattr(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char*		p;

	if((p=JS_GetStringBytes(JS_ValueToString(cx, argv[0])))==NULL) {
		*rval = INT_TO_JSVAL(-1);
		return(JS_TRUE);
	}

	JS_NewNumberValue(cx,getfattr(p),rval);
	return(JS_TRUE);
}

static JSBool
js_fdate(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char*		p;

	if((p=JS_GetStringBytes(JS_ValueToString(cx, argv[0])))==NULL) {
		*rval = INT_TO_JSVAL(-1);
		return(JS_TRUE);
	}

	JS_NewNumberValue(cx,fdate(p),rval);
	return(JS_TRUE);
}

static JSBool
js_utime(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char*			fname;
	int32			actime;
	int32			modtime;
	struct utimbuf	tbuf;
	struct utimbuf*	t=NULL;

	*rval = JSVAL_FALSE;

	if((fname=JS_GetStringBytes(JS_ValueToString(cx, argv[0])))==NULL)
		return(JS_TRUE);

	if(argc>1) {
		memset(&tbuf,0,sizeof(tbuf));
		actime=modtime=time(NULL);
		JS_ValueToInt32(cx,argv[1],&actime);
		JS_ValueToInt32(cx,argv[2],&modtime);
		tbuf.actime=actime;
		tbuf.modtime=modtime;
		t=&tbuf;
	}

	*rval = BOOLEAN_TO_JSVAL(utime(fname,t)==0);

	return(JS_TRUE);
}


static JSBool
js_flength(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char*		p;

	if((p=JS_GetStringBytes(JS_ValueToString(cx, argv[0])))==NULL) {
		*rval = INT_TO_JSVAL(-1);
		return(JS_TRUE);
	}

	JS_NewNumberValue(cx,flength(p),rval);
	return(JS_TRUE);
}


static JSBool
js_ftouch(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char*		fname;

	if((fname=JS_GetStringBytes(JS_ValueToString(cx, argv[0])))==NULL) {
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
		return(JS_TRUE);
	}

	*rval = BOOLEAN_TO_JSVAL(ftouch(fname));
	return(JS_TRUE);
}

static JSBool
js_fmutex(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char*		fname;
	char*		text=NULL;

	if((fname=JS_GetStringBytes(JS_ValueToString(cx, argv[0])))==NULL) {
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
		return(JS_TRUE);
	}
	if(argc>1)
		text=JS_GetStringBytes(JS_ValueToString(cx,argv[1]));

	*rval = BOOLEAN_TO_JSVAL(fmutex(fname,text));
	return(JS_TRUE);
}
		
static JSBool
js_sound(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char*		p;

	if(!argc) {	/* Stop playing sound */
#ifdef _WIN32
		PlaySound(NULL,NULL,0);
#endif
		*rval = BOOLEAN_TO_JSVAL(JS_TRUE);
		return(JS_TRUE);
	}

	if((p=JS_GetStringBytes(JS_ValueToString(cx, argv[0])))==NULL) {
		*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
		return(JS_TRUE);
	}

#ifdef _WIN32
	*rval = BOOLEAN_TO_JSVAL(PlaySound(p, NULL, SND_ASYNC|SND_FILENAME));
#else
	*rval = BOOLEAN_TO_JSVAL(JS_FALSE);
#endif

	return(JS_TRUE);
}

static JSBool
js_directory(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	int			i;
	int32		flags=GLOB_MARK;
	char*		p;
	glob_t		g;
	JSObject*	array;
	JSString*	js_str;
    jsint       len=0;
	jsval		val;

	*rval = JSVAL_NULL;

	if((p=JS_GetStringBytes(JS_ValueToString(cx, argv[0])))==NULL) 
		return(JS_TRUE);

	if(argc>1)
		JS_ValueToInt32(cx,argv[1],&flags);

    if((array = JS_NewArrayObject(cx, 0, NULL))==NULL)
		return(JS_FALSE);

	glob(p,flags,NULL,&g);
	for(i=0;i<(int)g.gl_pathc;i++) {
		if((js_str=JS_NewStringCopyZ(cx,g.gl_pathv[i]))==NULL)
			break;
		val=STRING_TO_JSVAL(js_str);
        if(!JS_SetElement(cx, array, len++, &val))
			break;
	}
	globfree(&g);

    *rval = OBJECT_TO_JSVAL(array);

    return(JS_TRUE);
}

static JSBool
js_freediskspace(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	int32		unit=0;
	char*		p;

	*rval = JSVAL_VOID;

	if((p=JS_GetStringBytes(JS_ValueToString(cx, argv[0])))==NULL) 
		return(JS_TRUE);

	if(argc>1)
		JS_ValueToInt32(cx,argv[1],&unit);

	JS_NewNumberValue(cx,getfreediskspace(p,unit),rval);

    return(JS_TRUE);
}


static JSBool
js_socket_select(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	JSObject*	inarray=NULL;
	JSObject*	rarray;
	BOOL		poll_for_write=FALSE;
	fd_set		socket_set;
	fd_set*		rd_set=NULL;
	fd_set*		wr_set=NULL;
	uintN		argn;
	SOCKET		sock;
	SOCKET		maxsock=0;
	struct		timeval tv = {0, 0};
	jsuint		i;
    jsuint      limit;
	SOCKET*		index;
	jsval		val;
	int			len=0;

	*rval = JSVAL_NULL;

	for(argn=0;argn<argc;argn++) {
		if(JSVAL_IS_BOOLEAN(argv[argn]))
			poll_for_write=JSVAL_TO_BOOLEAN(argv[argn]);
		else if(JSVAL_IS_OBJECT(argv[argn]))
			inarray = JSVAL_TO_OBJECT(argv[argn]);
		else if(JSVAL_IS_NUMBER(argv[argn]))
			js_timeval(cx,argv[argn],&tv);
	}

    if(inarray==NULL || !JS_IsArrayObject(cx, inarray))
		return(JS_TRUE);	/* This not a fatal error */

    if(!JS_GetArrayLength(cx, inarray, &limit))
		return(JS_TRUE);

	/* Return array */
    if((rarray = JS_NewArrayObject(cx, 0, NULL))==NULL)
		return(JS_FALSE);

	if((index=(SOCKET *)MALLOC(sizeof(SOCKET)*limit))==NULL)
		return(JS_FALSE);

	FD_ZERO(&socket_set);
	if(poll_for_write)
		wr_set=&socket_set;
	else
		rd_set=&socket_set;

    for(i=0;i<limit;i++) {
        if(!JS_GetElement(cx, inarray, i, &val))
			break;
		sock=js_socket(cx,val);
		index[i]=sock;
		if(sock!=INVALID_SOCKET) {
			FD_SET(sock,&socket_set);
			if(sock>maxsock)
				maxsock=sock;
		}
    }

	if(select(maxsock+1,rd_set,wr_set,NULL,&tv)<0)
		lprintf(LOG_DEBUG,"Error in socket_select()  %s (%d)",strerror(errno),errno);

	for(i=0;i<limit;i++) {
		if(index[i]!=INVALID_SOCKET && FD_ISSET(index[i],&socket_set)) {
			val=INT_TO_JSVAL(i);
   			if(!JS_SetElement(cx, rarray, len++, &val))
				break;
		}
	}
	free(index);

    *rval = OBJECT_TO_JSVAL(rarray);

    return(JS_TRUE);
}

static JSBool
js_mkdir(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char*		p;

	if((p=JS_GetStringBytes(JS_ValueToString(cx, argv[0])))==NULL) {
		*rval = INT_TO_JSVAL(-1);
		return(JS_TRUE);
	}

	*rval = BOOLEAN_TO_JSVAL(MKDIR(p)==0);
	return(JS_TRUE);
}

static JSBool
js_rmdir(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char*		p;

	if((p=JS_GetStringBytes(JS_ValueToString(cx, argv[0])))==NULL) {
		*rval = INT_TO_JSVAL(-1);
		return(JS_TRUE);
	}

	*rval = BOOLEAN_TO_JSVAL(rmdir(p)==0);
	return(JS_TRUE);
}


static JSBool
js_strftime(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char		str[128];
	char*		fmt;
	int32		i=time(NULL);
	time_t		t;
	struct tm	tm;
	JSString*	js_str;

	fmt=JS_GetStringBytes(JS_ValueToString(cx, argv[0]));
	if(argc>1)
		JS_ValueToInt32(cx,argv[1],&i);

	strcpy(str,"-Invalid time-");
	t=i;
	if(localtime_r(&t,&tm)==NULL)
		memset(&tm,0,sizeof(tm));
	strftime(str,sizeof(str),fmt,&tm);

	if((js_str=JS_NewStringCopyZ(cx, str))==NULL)
		return(JS_FALSE);

	*rval = STRING_TO_JSVAL(js_str);
	return(JS_TRUE);
}

static JSBool
js_resolve_ip(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	struct in_addr addr;
	JSString*	str;

	*rval = JSVAL_NULL;

	if(argv[0]==JSVAL_VOID)
		return(JS_TRUE);

	if((addr.s_addr=resolve_ip(JS_GetStringBytes(JS_ValueToString(cx, argv[0]))))
		==INADDR_NONE)
		return(JS_TRUE);
	
	if((str=JS_NewStringCopyZ(cx, inet_ntoa(addr)))==NULL)
		return(JS_FALSE);

	*rval = STRING_TO_JSVAL(str);
	return(JS_TRUE);
}


static JSBool
js_resolve_host(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	struct in_addr addr;
	HOSTENT*	h;

	*rval = JSVAL_NULL;

	if(argv[0]==JSVAL_VOID)
		return(JS_TRUE);

	addr.s_addr=inet_addr(JS_GetStringBytes(JS_ValueToString(cx, argv[0])));
	h=gethostbyaddr((char *)&addr,sizeof(addr),AF_INET);

	if(h!=NULL && h->h_name!=NULL)
		*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,h->h_name));

	return(JS_TRUE);

}

extern link_list_t named_queues;	/* js_queue.c */

static JSBool
js_list_named_queues(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	JSObject*	array;
    jsint       len=0;
	jsval		val;
	list_node_t* node;
	msg_queue_t* q;

    if((array = JS_NewArrayObject(cx, 0, NULL))==NULL)
		return(JS_FALSE);

	for(node=listFirstNode(&named_queues);node!=NULL;node=listNextNode(node)) {
		if((q=listNodeData(node))==NULL)
			continue;
		val=STRING_TO_JSVAL(JS_NewStringCopyZ(cx,q->name));
        if(!JS_SetElement(cx, array, len++, &val))
			break;
	}

    *rval = OBJECT_TO_JSVAL(array);

    return(JS_TRUE);
}
	
static JSClass js_global_class = {
     "Global"				/* name			*/
    ,JSCLASS_HAS_PRIVATE	/* flags		*/
	,JS_PropertyStub		/* addProperty	*/
	,JS_PropertyStub		/* delProperty	*/
	,js_system_get			/* getProperty	*/
	,JS_PropertyStub		/* setProperty	*/
	,JS_EnumerateStub		/* enumerate	*/
	,JS_ResolveStub			/* resolve		*/
	,JS_ConvertStub			/* convert		*/
	,JS_FinalizeStub		/* finalize		*/
};

static jsSyncMethodSpec js_global_functions[] = {
	{"exit",			js_exit,			0,	JSTYPE_VOID,	"[number exit_code]"
	,JSDOCSTR("stop script execution, "
		"optionally setting the global property <tt>exit_code</tt> to the specified numeric value")
	,311
	},		
	{"load",            js_load,            1,	JSTYPE_UNDEF
	,JSDOCSTR("[<i>bool</i> background or <i>object</i> scope,] <i>string</i> filename [,args]")
	,JSDOCSTR("load and execute a JavaScript module (<i>filename</i>), "
		"optionally specifying a target <i>scope</i> object (default: <i>this</i>) "
		"and a list of arguments to pass to the module (as <i>argv</i>). "
		"Returns the result (last executed statement) of the executed script "
		"or a newly created <i>Queue</i> object if <i>background</i> is <i>true</i>).<br><br>"
		"<b>Background</b> (added in v3.12):<br>"
		"If <i>background</i> is <i>true</i>, the loaded script runs in the background "
		"(in a child thread) but may communicate with the parent "
		"script/thread by reading from and/or writing to the <i>parent_queue</i> "
		"(an automatically created <i>Queue</i> object). " 
		"The result (last executed statement) of the executed script will also be automatically "
		"written to the <i>parent_queue</i> "
		"which may be read later by the parent script.")
	,312
	},		
	{"sleep",			js_mswait,			0,	JSTYPE_ALIAS },
	{"mswait",			js_mswait,			0,	JSTYPE_VOID,	JSDOCSTR("[number milliseconds]")
	,JSDOCSTR("millisecond wait/sleep routine (AKA sleep)")
	,310
	},
	{"yield",			js_yield,			0,	JSTYPE_VOID,	JSDOCSTR("[bool forced]")
	,JSDOCSTR("release current thread time-slice, "
		"a <i>forced</i> yield will yield to all other pending tasks (lowering CPU utilization), "
		"a non-<i>forced</i> yield will yield only to pending tasks of equal or higher priority. "
		"<i>forced</i> defaults to <i>true</i>")
	,311
	},
	{"random",			js_random,			1,	JSTYPE_NUMBER,	JSDOCSTR("number max")
	,JSDOCSTR("return random integer between 0 and max-1")
	,310
	},		
	{"time",			js_time,			0,	JSTYPE_NUMBER,	""
	,JSDOCSTR("return current time in Unix (time_t) format (number of seconds since Jan-01-1970)")
	,310
	},		
	{"beep",			js_beep,			0,	JSTYPE_VOID,	JSDOCSTR("[number freq, duration]")
	,JSDOCSTR("produce a tone on the local speaker at specified frequency for specified duration (in milliseconds)")
	,310
	},		
	{"sound",			js_sound,			0,	JSTYPE_BOOLEAN,	JSDOCSTR("[string filename]")
	,JSDOCSTR("play a waveform (.wav) sound file (currently, on Windows platforms only)")
	,310
	},		
	{"ctrl",			js_ctrl,			1,	JSTYPE_STRING,	JSDOCSTR("number or string")
	,JSDOCSTR("return ASCII control character representing character passed - Example: <tt>ctrl('C') returns '\3'</tt>")
	,311
	},
	{"ascii",			js_ascii,			1,	JSTYPE_UNDEF,	JSDOCSTR("[string text] or [number value]")
	,JSDOCSTR("convert string to ASCII value or vice-versa (returns number OR string)")
	,310
	},		
	{"ascii_str",		js_ascii_str,		1,	JSTYPE_STRING,	JSDOCSTR("string text")
	,JSDOCSTR("convert extended-ASCII in string to plain ASCII")
	,310
	},		
	{"strip_ctrl",		js_strip_ctrl,		1,	JSTYPE_STRING,	JSDOCSTR("string text")
	,JSDOCSTR("strip control characters from string")
	,310
	},		
	{"strip_exascii",	js_strip_exascii,	1,	JSTYPE_STRING,	JSDOCSTR("string text")
	,JSDOCSTR("strip extended-ASCII characters from string")
	,310
	},		
	{"truncsp",			js_truncsp,			1,	JSTYPE_STRING,	JSDOCSTR("string text")
	,JSDOCSTR("truncate (trim) white-space characters off end of string")
	,310
	},
	{"truncstr",		js_truncstr,		2,	JSTYPE_STRING,	JSDOCSTR("string text, charset")
	,JSDOCSTR("truncate (trim) string at first char in <i>charset</i>")
	,310
	},		
	{"lfexpand",		js_lfexpand,		1,	JSTYPE_STRING,	JSDOCSTR("string text")
	,JSDOCSTR("expand line-feeds (LF) to carriage-return/line-feeds (CRLF)")
	,310
	},
	{"backslash",		js_backslash,		1,	JSTYPE_STRING,	JSDOCSTR("string path")
	,JSDOCSTR("returns directory path with trailing (platform-specific) path delimeter "
		"(i.e. \"slash\" or \"backslash\")")
	,312
	},
	{"file_getname",	js_getfname,		1,	JSTYPE_STRING,	JSDOCSTR("string path")
	,JSDOCSTR("returns filename portion of passed path string")
	,311
	},
	{"file_getext",		js_getfext,			1,	JSTYPE_STRING,	JSDOCSTR("string path")
	,JSDOCSTR("returns file extension portion of passed path/filename string (including '.') "
		"or <i>undefined</i> if no extension is found")
	,311
	},
	{"file_getcase",	js_getfcase,		1,	JSTYPE_STRING,	JSDOCSTR("string filename")
	,JSDOCSTR("returns correct case of filename (long version of filename on Win32) "
		"or <i>undefined</i> if the file doesn't exist")
	,311
	},
	{"file_cfgname",	js_cfgfname,		2,	JSTYPE_STRING,	JSDOCSTR("string path, filename")
	,JSDOCSTR("returns completed configuration filename from supplied <i>path</i> and <i>filename</i>, "
	"optionally including the local hostname (e.g. <tt>path/file.<i>host</i>.<i>domain</i>.ext</tt> "
	"or <tt>path/file.<i>host</i>.ext</tt>) if such a variation of the filename exists")
	,312
	},
	{"file_exists",		js_fexist,			1,	JSTYPE_BOOLEAN,	JSDOCSTR("string filename")
	,JSDOCSTR("verify a file's existence")
	,310
	},		
	{"file_remove",		js_remove,			1,	JSTYPE_BOOLEAN,	JSDOCSTR("string filename")
	,JSDOCSTR("delete a file")
	,310
	},		
	{"file_rename",		js_rename,			2,	JSTYPE_BOOLEAN,	JSDOCSTR("oldname, newname")
	,JSDOCSTR("rename a file, possibly moving it to another directory in the process")
	,311
	},
	{"file_copy",		js_fcopy,			2,	JSTYPE_BOOLEAN,	JSDOCSTR("source, destination")
	,JSDOCSTR("copy a file from one directory or filename to another")
	,311
	},
	{"file_backup",		js_backup,			1,	JSTYPE_BOOLEAN,	JSDOCSTR("string filename [,number level] [,bool rename]")
	,JSDOCSTR("backup the specified <i>filename</i> as <tt>filename.<i>number</i>.extension</tt> "
		"where <i>number</i> is the backup number 0 through <i>level</i>-1 "
		"(default backup <i>level</i> is 5), "
		"if <i>rename</i> is <i>true</i>, the original file is renamed instead of copied "
		"(default is <i>false</i>)")
	,311
	},
	{"file_isdir",		js_isdir,			1,	JSTYPE_BOOLEAN,	JSDOCSTR("string filename")
	,JSDOCSTR("check if specified <i>filename</i> is a directory")
	,310
	},		
	{"file_attrib",		js_fattr,			1,	JSTYPE_NUMBER,	JSDOCSTR("string filename")
	,JSDOCSTR("get a file's permissions/attributes")
	,310
	},		
	{"file_date",		js_fdate,			1,	JSTYPE_NUMBER,	JSDOCSTR("string filename")
	,JSDOCSTR("get a file's last modified date/time (in time_t format)")
	,310
	},
	{"file_size",		js_flength,			1,	JSTYPE_NUMBER,	JSDOCSTR("string filename")
	,JSDOCSTR("get a file's length (in bytes)")
	,310
	},
	{"file_utime",		js_utime,			3,	JSTYPE_BOOLEAN,	JSDOCSTR("string filename [,access_time] [,mod_time]")
	,JSDOCSTR("change a file's last accessed and modification date/time (in time_t format), "
		"or change to current time")
	,311
	},
	{"file_touch",		js_ftouch,			1,	JSTYPE_BOOLEAN,	JSDOCSTR("string filename")
	,JSDOCSTR("updates a file's last modification date/time to current time, "
		"creating an empty file if it doesn't already exist")
	,311
	},
	{"file_mutex",		js_fmutex,			1,	JSTYPE_BOOLEAN,	JSDOCSTR("string filename [,text]")
	,JSDOCSTR("attempts to create an exclusive (e.g. lock) file, "
		"optionally with the contents of <i>text</i>")
	,312
	},
	{"directory",		js_directory,		1,	JSTYPE_ARRAY,	JSDOCSTR("string pattern [,flags]")
	,JSDOCSTR("returns an array of directory entries, "
		"<i>pattern</i> is the path and filename or wildcards to search for (e.g. '/subdir/*.txt'), "
		"<i>flags</i> is a bitfield of optional <tt>glob</tt> flags (default is <tt>GLOB_MARK</tt>)")
	,310
	},
	{"dir_freespace",	js_freediskspace,	2,	JSTYPE_NUMBER,	JSDOCSTR("string directory [,unit_size]")
	,JSDOCSTR("returns the amount of available disk space in the specified <i>directory</i> "
		"using the specified <i>unit_size</i> in bytes (default: 1), "
		"specify a <i>unit_size</i> of <tt>1024</tt> to return the available space in <i>kilobytes</i>.")
	,311
	},
	{"socket_select",	js_socket_select,	0,	JSTYPE_ARRAY,	JSDOCSTR("[array of socket objects or descriptors] [,number timeout] [,bool write]")
	,JSDOCSTR("checks an array of socket objects or descriptors for read or write ability (default is <i>read</i>), "
		"default timeout value is 0.0 seconds (immediate timeout), "
		"returns an array of 0-based index values into the socket array, representing the sockets that were ready for reading or writing")
	,311
	},
	{"mkdir",			js_mkdir,			1,	JSTYPE_BOOLEAN,	JSDOCSTR("string directory")
	,JSDOCSTR("make a directory")
	,310
	},		
	{"rmdir",			js_rmdir,			1,	JSTYPE_BOOLEAN,	JSDOCSTR("string directory")
	,JSDOCSTR("remove a directory")
	,310
	},		
	{"strftime",		js_strftime,		1,	JSTYPE_STRING,	JSDOCSTR("string format [,number time]")
	,JSDOCSTR("return a formatted time string (ala C strftime)")
	,310
	},		
	{"format",			js_format,			1,	JSTYPE_STRING,	JSDOCSTR("string format [,args]")
	,JSDOCSTR("return a formatted string (ala sprintf) - "
		"<small>CAUTION: for experienced C programmers ONLY</small>")
	,310
	},
	{"html_encode",		js_html_encode,		1,	JSTYPE_STRING,	JSDOCSTR("string text [,bool ex_ascii] [,bool white_space] [,bool ansi] [,bool ctrl_a]")
	,JSDOCSTR("return an HTML-encoded text string (using standard HTML character entities), "
		"escaping IBM extended-ASCII, white-space characters, ANSI codes, and CTRL-A codes by default")
	,311
	},
	{"html_decode",		js_html_decode,		1,	JSTYPE_STRING,	JSDOCSTR("string text")
	,JSDOCSTR("return a decoded HTML-encoded text string")
	,311
	},
	{"word_wrap",		js_word_wrap,		1,	JSTYPE_STRING,	JSDOCSTR("string text [,line_length]")
	,JSDOCSTR("returns a word-wrapped version of the text string argument, <i>line_length</i> defaults to <i>79</i>")
	,311
	},
	{"quote_msg",		js_quote_msg,		1,	JSTYPE_STRING,	JSDOCSTR("string text [,line_length] [,prefix]")
	,JSDOCSTR("returns a quoted version of the message text string argument, <i>line_length</i> defaults to <i>79</i>, "
		"<i>prefix</i> defaults to <tt>\" > \"</tt>")
	,311
	},
	{"rot13_translate",	js_rot13,			1,	JSTYPE_STRING,	JSDOCSTR("string text")
	,JSDOCSTR("returns ROT13-translated version of text string (will encode or decode text)")
	,311
	},
	{"base64_encode",	js_b64_encode,		1,	JSTYPE_STRING,	JSDOCSTR("string text")
	,JSDOCSTR("returns base64-encoded version of text string or <i>null</i> on error")
	,311
	},
	{"base64_decode",	js_b64_decode,		1,	JSTYPE_STRING,	JSDOCSTR("string text")
	,JSDOCSTR("returns base64-decoded text string or <i>null</i> on error")
	,311
	},
	{"crc16_calc",		js_crc16,			1,	JSTYPE_NUMBER,	JSDOCSTR("string text")
	,JSDOCSTR("calculate and return 16-bit CRC of text string")
	,311
	},		
	{"crc32_calc",		js_crc32,			1,	JSTYPE_NUMBER,	JSDOCSTR("string text")
	,JSDOCSTR("calculate and return 32-bit CRC of text string")
	,311
	},		
	{"chksum_calc",		js_chksum,			1,	JSTYPE_NUMBER,	JSDOCSTR("string text")
	,JSDOCSTR("calculate and return 32-bit checksum of text string")
	,311
	},
	{"md5_calc",		js_md5_calc,		1,	JSTYPE_STRING,	JSDOCSTR("string text [,bool hex]")
	,JSDOCSTR("calculate and return 128-bit MD5 digest of text string, result encoded in base64 (default) or hexadecimal")
	,311
	},
	{"gethostbyname",	js_resolve_ip,		1,	JSTYPE_ALIAS },
	{"resolve_ip",		js_resolve_ip,		1,	JSTYPE_STRING,	JSDOCSTR("string hostname")
	,JSDOCSTR("resolve IP address of specified hostname (AKA gethostbyname)")
	,311
	},
	{"gethostbyaddr",	js_resolve_host,	1,	JSTYPE_ALIAS },
	{"resolve_host",	js_resolve_host,	1,	JSTYPE_STRING,	JSDOCSTR("string ip_address")
	,JSDOCSTR("resolve hostname of specified IP address (AKA gethostbyaddr)")
	,311
	},
	{"netaddr_type",	js_netaddr_type,	1,	JSTYPE_NUMBER,	JSDOCSTR("string email_address")
	,JSDOCSTR("returns the proper message <i>net_type</i> for the specified <i>email_address</i>, "
		"(e.g. <tt>NET_INTERNET</tt> for Internet e-mail or <tt>NET_NONE</tt> for local e-mail)")
	,312
	},
	{"list_named_queues",js_list_named_queues,0,JSTYPE_ARRAY,	JSDOCSTR("")
	,JSDOCSTR("returns an array of <i>named queues</i> (created with the <i>Queue</i> constructor)")
	,312
	},

	{0}
};

static jsConstIntSpec js_global_const_ints[] = {
	/* Numeric error constants from errno.h (platform-dependant) */
	{"EPERM"		,EPERM			},
	{"ENOENT"		,ENOENT			},
	{"ESRCH"		,ESRCH			},
	{"EIO"			,EIO			},
	{"ENXIO"		,ENXIO			},
	{"E2BIG"		,E2BIG			},
	{"ENOEXEC"		,ENOEXEC		},
	{"EBADF"		,EBADF			},
	{"ECHILD"		,ECHILD			},
	{"EAGAIN"		,EAGAIN			},
	{"ENOMEM"		,ENOMEM			},
	{"EACCES"		,EACCES			},
	{"EFAULT"		,EFAULT			},
	{"EBUSY"		,EBUSY			},
	{"EEXIST"		,EEXIST			},
	{"EXDEV"		,EXDEV			},
	{"ENODEV"		,ENODEV			},
	{"ENOTDIR"		,ENOTDIR		},
	{"EISDIR"		,EISDIR			},
	{"EINVAL"		,EINVAL			},
	{"ENFILE"		,ENFILE			},
	{"EMFILE"		,EMFILE			},
	{"ENOTTY"		,ENOTTY			},
	{"EFBIG"		,EFBIG			},
	{"ENOSPC"		,ENOSPC			},
	{"ESPIPE"		,ESPIPE			},
	{"EROFS"		,EROFS			},
	{"EMLINK"		,EMLINK			},
	{"EPIPE"		,EPIPE			},
	{"EDOM"			,EDOM			},
	{"ERANGE"		,ERANGE			},
	{"EDEADLOCK"	,EDEADLOCK		},
	{"ENAMETOOLONG"	,ENAMETOOLONG	},
	{"ENOTEMPTY"	,ENOTEMPTY		},

	/* Socket errors */
	{"EINTR"			,EINTR			},
	{"ENOTSOCK"			,ENOTSOCK		},
	{"EMSGSIZE"			,EMSGSIZE		},
	{"EWOULDBLOCK"		,EWOULDBLOCK	},
	{"EPROTOTYPE"		,EPROTOTYPE		},
	{"ENOPROTOOPT"		,ENOPROTOOPT	},
	{"EPROTONOSUPPORT"	,EPROTONOSUPPORT},
	{"ESOCKTNOSUPPORT"	,ESOCKTNOSUPPORT},
	{"EOPNOTSUPP"		,EOPNOTSUPP		},
	{"EPFNOSUPPORT"		,EPFNOSUPPORT	},
	{"EAFNOSUPPORT"		,EAFNOSUPPORT	},
	{"EADDRINUSE"		,EADDRINUSE		},
	{"EADDRNOTAVAIL"	,EADDRNOTAVAIL	},
	{"ECONNABORTED"		,ECONNABORTED	},
	{"ECONNRESET"		,ECONNRESET		},
	{"ENOBUFS"			,ENOBUFS		},
	{"EISCONN"			,EISCONN		},
	{"ENOTCONN"			,ENOTCONN		},
	{"ESHUTDOWN"		,ESHUTDOWN		},
	{"ETIMEDOUT"		,ETIMEDOUT		},
	{"ECONNREFUSED"		,ECONNREFUSED	},
	{"EINPROGRESS"		,EINPROGRESS	},

	/* Log priority values from syslog.h/sbbsdefs.h (possibly platform-dependant) */
	{"LOG_EMERG"		,LOG_EMERG		},
	{"LOG_ALERT"		,LOG_ALERT		},
	{"LOG_CRIT"			,LOG_CRIT		},
	{"LOG_ERR"			,LOG_ERR		},
	{"LOG_ERROR"		,LOG_ERR		},
	{"LOG_WARNING"		,LOG_WARNING	},
	{"LOG_NOTICE"		,LOG_NOTICE		},
	{"LOG_INFO"			,LOG_INFO		},
	{"LOG_DEBUG"		,LOG_DEBUG		},

	/* Other useful constants */
	{"INVALID_SOCKET"	,INVALID_SOCKET	},

	/* Terminator (Governor Arnold) */
	{0}
};

JSObject* DLLCALL js_CreateGlobalObject(JSContext* cx, scfg_t* cfg, jsSyncMethodSpec* methods)
{
	JSObject*	glob;

	if((glob = JS_NewObject(cx, &js_global_class, NULL, NULL)) ==NULL)
		return(NULL);

	if (!JS_InitStandardClasses(cx, glob))
		return(NULL);

	if(methods!=NULL && !js_DefineSyncMethods(cx, glob, methods, TRUE)) 
		return(NULL);

	if(!js_DefineSyncMethods(cx, glob, js_global_functions, TRUE)) 
		return(NULL);

	if(!JS_DefineProperties(cx, glob, js_global_properties))
		return(NULL);

	if(!JS_SetPrivate(cx, glob, cfg))	/* Store a pointer to scfg_t */
		return(NULL);

#ifdef _DEBUG
	js_DescribeSyncObject(cx,glob
		,"Top-level functions and properties (common to all servers and services)",310);
#endif

	if(!js_DefineConstIntegers(cx, glob, js_global_const_ints, JSPROP_READONLY))
		return(NULL);

	return(glob);
}

JSObject* DLLCALL js_CreateCommonObjects(JSContext* js_cx
										,scfg_t* cfg				/* common */
										,scfg_t* node_cfg			/* node-specific */
										,jsSyncMethodSpec* methods	/* global */
										,time_t uptime				/* system */
										,char* host_name			/* system */
										,char* socklib_desc			/* system */
										,js_branch_t* branch		/* js */
										,client_t* client			/* client */
										,SOCKET client_socket		/* client */
										,js_server_props_t* props	/* server */
										)
{
	JSObject*	js_glob;

	if(node_cfg==NULL)
		node_cfg=cfg;

	/* Global Object */
	if((js_glob=js_CreateGlobalObject(js_cx, cfg, methods))==NULL)
		return(NULL);

	/* System Object */
	if(js_CreateSystemObject(js_cx, js_glob, node_cfg, uptime, host_name, socklib_desc)==NULL)
		return(NULL);

	/* Internal JS Object */
	if(branch!=NULL 
		&& js_CreateInternalJsObject(js_cx, js_glob, branch)==NULL)
		return(NULL);

	/* Client Object */
	if(client!=NULL 
		&& js_CreateClientObject(js_cx, js_glob, "client", client, client_socket)==NULL)
		return(NULL);

	/* Server */
	if(props!=NULL
		&& js_CreateServerObject(js_cx, js_glob, props)==NULL)
		return(NULL);

	/* Socket Class */
	if(js_CreateSocketClass(js_cx, js_glob)==NULL)
		return(NULL);

	/* Queue Class */
	if(js_CreateQueueClass(js_cx, js_glob)==NULL)
		return(NULL);

	/* MsgBase Class */
	if(js_CreateMsgBaseClass(js_cx, js_glob, cfg)==NULL)
		return(NULL);

	/* File Class */
	if(js_CreateFileClass(js_cx, js_glob)==NULL)
		return(NULL);

	/* User class */
	if(js_CreateUserClass(js_cx, js_glob, cfg)==NULL) 
		return(NULL);

	/* Area Objects */
	if(!js_CreateUserObjects(js_cx, js_glob, cfg, NULL, NULL, NULL)) 
		return(NULL);

	return(js_glob);
}


#endif	/* JAVSCRIPT */
