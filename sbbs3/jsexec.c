/* jsexec.c */

/* Execute a Synchronet JavaScript module from the command-line */

/* $Id: jsexec.c,v 1.8 2003/07/08 05:00:38 rswindell Exp $ */

/****************************************************************************
 * @format.tab-size 4		(Plain Text/Source Code File Header)			*
 * @format.use-tabs true	(see http://www.synchro.net/ptsc_hdr.html)		*
 *																			*
 * Copyright 2003 Rob Swindell - http://www.synchro.net/copyright.html		*
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

#ifndef JAVASCRIPT
#define JAVASCRIPT
#endif

#include "sbbs.h"

JSRuntime*	js_runtime;
JSContext*	js_cx;
JSObject*	js_glob;
js_branch_t	branch;
scfg_t		scfg;
ulong		js_max_bytes=JAVASCRIPT_MAX_BYTES;
ulong		js_context_stack=JAVASCRIPT_CONTEXT_STACK;
FILE*		confp;
FILE*		errfp;
char		revision[16];
BOOL		pause_on_exit=FALSE;

void banner(FILE* fp)
{
	fprintf(fp,"\nJSexec v%s%c-%s (rev %s) - "
		"Execute Synchronet JavaScript Module\n"
		,VERSION,REVISION
		,PLATFORM_DESC
		,revision
		);
}

void usage(FILE* fp)
{
	banner(fp);

	fprintf(fp,"\nusage: jsexec [-opts] [path]module[.js] [args]\n"
		"\navailable opts:\n\n"
		"\t-m <bytes>      set maximum heap size (default=%u bytes)\n"
		"\t-s <bytes>      set context stack size (default=%u bytes)\n"
		"\t-b <limit>      set branch limit (default=%u, 0=unlimited)\n"
		"\t-y <freq>       set yield frequency (default=%u, 0=never)\n"
		"\t-g <freq>       set garbage collection frequency (default=%u, 0=never)\n"
		"\t-t <filename>   send console output to stdout and filename\n"
		"\t-q              send console output to %s (quiet mode)\n"
		"\t-e              send error messages to console instead of stderr\n"
		"\t-p              wait for keypress (pause) on exit\n"
		,JAVASCRIPT_MAX_BYTES
		,JAVASCRIPT_CONTEXT_STACK
		,JAVASCRIPT_BRANCH_LIMIT
		,JAVASCRIPT_YIELD_FREQUENCY
		,JAVASCRIPT_GC_FREQUENCY
		,_PATH_DEVNULL
		);
}

void bail(int code)
{
	if(pause_on_exit) {
		fprintf(errfp,"\nHit enter to continue...");
		getchar();
	}
	exit(code);
}

static JSBool
js_log(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    uintN		i;
    JSString*	str;

    for (i = 0; i < argc; i++) {
		if((str=JS_ValueToString(cx, argv[i]))==NULL)
		    return(JS_FALSE);
		fprintf(errfp,JS_GetStringBytes(str));
	}

	*rval = JSVAL_VOID;
    return(JS_TRUE);
}

static JSBool
js_write(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    uintN		i;
    JSString*	str=NULL;

    for (i = 0; i < argc; i++) {
		if((str=JS_ValueToString(cx, argv[i]))==NULL)
		    return(JS_FALSE);
		fprintf(confp,"%s",JS_GetStringBytes(str));
	}

	if(str==NULL)
		*rval = JSVAL_VOID;
	else
		*rval = STRING_TO_JSVAL(str);
    return(JS_TRUE);
}

static JSBool
js_print(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	if(!js_write(cx,obj,argc,argv,rval))
		return(JS_FALSE);

	fprintf(confp,"\n");
    return(JS_TRUE);
}

static JSBool
js_printf(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char*		p;
    uintN		i;
	JSString *	fmt;
    JSString *	str;
	va_list		arglist[64];

	if((fmt = JS_ValueToString(cx, argv[0]))==NULL)
		return(JS_FALSE);

	memset(arglist,0,sizeof(arglist));	// Initialize arglist to NULLs

    for (i = 1; i < argc && i<sizeof(arglist)/sizeof(arglist[0]); i++) {
		if(JSVAL_IS_STRING(argv[i])) {
			if((str=JS_ValueToString(cx, argv[i]))==NULL)
			    return(JS_FALSE);
			arglist[i-1]=JS_GetStringBytes(str);
		} 
		else if(JSVAL_IS_DOUBLE(argv[i]))
			arglist[i-1]=(char*)(unsigned long)*JSVAL_TO_DOUBLE(argv[i]);
		else if(JSVAL_IS_INT(argv[i]) || JSVAL_IS_BOOLEAN(argv[i]))
			arglist[i-1]=(char *)JSVAL_TO_INT(argv[i]);
		else
			arglist[i-1]=NULL;
	}
	
	if((p=JS_vsmprintf(JS_GetStringBytes(fmt),(char*)arglist))==NULL)
		return(JS_FALSE);

	fprintf(confp,"%s",p);

	*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx, p));

	JS_smprintf_free(p);

    return(JS_TRUE);
}

static JSBool
js_alert(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSString *	str;

	if((str=JS_ValueToString(cx, argv[0]))==NULL)
	    return(JS_FALSE);

	fprintf(confp,"!%s\n",JS_GetStringBytes(str));

	*rval = JSVAL_VOID;
    return(JS_TRUE);
}

static JSBool
js_confirm(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSString *	str;

	if((str=JS_ValueToString(cx, argv[0]))==NULL)
	    return(JS_FALSE);

	*rval = BOOLEAN_TO_JSVAL(FALSE);
	return(JS_TRUE);
}

static JSBool
js_prompt(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
	char		instr[81];
    JSString *	prompt;
    JSString *	str;

	if((prompt=JS_ValueToString(cx, argv[0]))==NULL)
	    return(JS_FALSE);

	if(argc>1) {
		if((str=JS_ValueToString(cx, argv[1]))==NULL)
		    return(JS_FALSE);
		SAFECOPY(instr,JS_GetStringBytes(str));
	} else
		instr[0]=0;

	fprintf(confp,"%s: ",JS_GetStringBytes(prompt));

	if(!fgets(instr,sizeof(instr),stdin)) {
		*rval = JSVAL_VOID;
		return(JS_TRUE);
	}

	if((str=JS_NewStringCopyZ(cx, instr))==NULL)
	    return(JS_FALSE);

	*rval = STRING_TO_JSVAL(str);
    return(JS_TRUE);
}

static jsMethodSpec js_global_functions[] = {
	{"log",				js_log,				1},
    {"write",           js_write,           0},
    {"writeln",         js_print,           0},
    {"print",           js_print,           0},
    {"printf",          js_printf,          1},	
	{"alert",			js_alert,			1},
	{"prompt",			js_prompt,			1},
	{"confirm",			js_confirm,			1},
    {0}
};

static void
js_ErrorReporter(JSContext *cx, const char *message, JSErrorReport *report)
{
	char	line[64];
	char	file[MAX_PATH+1];
	const char*	warning;

	if(report==NULL) {
		fprintf(errfp,"!JavaScript: %s", message);
		return;
    }

	if(report->filename)
		sprintf(file," %s",report->filename);
	else
		file[0]=0;

	if(report->lineno)
		sprintf(line," line %d",report->lineno);
	else
		line[0]=0;

	if(JSREPORT_IS_WARNING(report->flags)) {
		if(JSREPORT_IS_STRICT(report->flags))
			warning="strict warning";
		else
			warning="warning";
	} else
		warning="";

	fprintf(errfp,"!JavaScript %s%s%s: %s",warning,file,line,message);
}

static JSBool
js_BranchCallback(JSContext *cx, JSScript *script)
{
	branch.counter++;

	/* Infinite loop? */
	if(branch.limit && branch.counter > branch.limit) {
		JS_ReportError(cx,"Infinite loop (%lu branches) detected",branch.counter);
		branch.counter=0;
		return(JS_FALSE);
	}
	/* Give up timeslices every once in a while */
	if(branch.yield_freq && (branch.counter%branch.yield_freq)==0)
		YIELD();

	if(branch.gc_freq && (branch.counter%branch.gc_freq)==0)
		JS_MaybeGC(cx);

    return(JS_TRUE);
}

static BOOL js_CreateEnvObject(JSContext* cx, JSObject* glob, char** env)
{
	char		name[256];
	char*		val;
	uint		i;
	JSObject*	js_env;

	if((js_env=JS_NewObject(js_cx, NULL, NULL, glob))==NULL)
		return(FALSE);

	for(i=0;env[i]!=NULL;i++) {
		SAFECOPY(name,env[i]);
		truncstr(name,"=");
		val=strchr(env[i],'=');
		if(val==NULL)
			val="";
		else
			val++;
		if(!JS_DefineProperty(cx, js_env, name, STRING_TO_JSVAL(JS_NewStringCopyZ(cx,val))
			,NULL,NULL,JSPROP_READONLY|JSPROP_ENUMERATE))
			return(FALSE);
	}

	if(!JS_DefineProperty(cx, glob, "env", OBJECT_TO_JSVAL(js_env)
		,NULL,NULL,JSPROP_READONLY|JSPROP_ENUMERATE))
		return(FALSE);

	return(TRUE);
}

static BOOL js_init(char** environ)
{
	fprintf(errfp,"JavaScript: Creating runtime: %lu bytes\n"
		,js_max_bytes);

	if((js_runtime = JS_NewRuntime(js_max_bytes))==NULL)
		return(FALSE);

	fprintf(errfp,"JavaScript: Initializing context (stack: %lu bytes)\n"
		,js_context_stack);

    if((js_cx = JS_NewContext(js_runtime, js_context_stack))==NULL)
		return(FALSE);

	JS_SetErrorReporter(js_cx, js_ErrorReporter);

	/* Global Object */
	if((js_glob=js_CreateGlobalObject(js_cx, &scfg, js_global_functions))==NULL)
		return(FALSE);

	/* Branch Object */
	if(js_CreateBranchObject(js_cx, js_glob, &branch)==NULL)
		return(FALSE);

	/* System Object */
	if(js_CreateSystemObject(js_cx, js_glob, &scfg, time(NULL), scfg.sys_inetaddr)==NULL)
		return(FALSE);

	/* Socket Class */
	if(js_CreateSocketClass(js_cx, js_glob)==NULL)
		return(FALSE);

	/* MsgBase Class */
	if(js_CreateMsgBaseClass(js_cx, js_glob, &scfg)==NULL)
		return(FALSE);

	/* File Class */
	if(js_CreateFileClass(js_cx, js_glob)==NULL)
		return(FALSE);

	/* User class */
	if(js_CreateUserClass(js_cx, js_glob, &scfg)==NULL) 
		return(FALSE);

	/* Area Objects */
	if(!js_CreateUserObjects(js_cx, js_glob, &scfg, NULL, NULL, NULL)) 
		return(FALSE);

	/* Environment Object (associative array) */
	if(!js_CreateEnvObject(js_cx, js_glob, environ))
		return(FALSE);

	return(TRUE);
}

long js_exec(const char *fname, char** args)
{
	int			argc=0;
	char		path[MAX_PATH+1];
	JSObject*	js_scope=NULL;
	JSScript*	js_script=NULL;
	JSString*	arg;
	JSObject*	argv;
	jsval		val;
	jsval		rval;
	
	if(strcspn(fname,"/\\")==strlen(fname)) {
		sprintf(path,"%s%s",scfg.mods_dir,fname);
		if(scfg.mods_dir[0]==0 || !fexistcase(path))
			sprintf(path,"%s%s",scfg.exec_dir,fname);
	} else
		sprintf(path,"%.*s",(int)sizeof(path)-4,fname);
	/* Add extension if not specified */
	if(!strchr(path,'.'))
		strcat(path,".js");

	if(!fexistcase(path)) {
		fprintf(errfp,"!Module file (%s) doesn't exist\n",path);
		return(-1); 
	}

	js_scope=JS_NewObject(js_cx, NULL, NULL, js_glob);

	if(js_scope==NULL) {
		fprintf(errfp,"!Error creating JS scope\n");
		return(-1);
	}

	argv=JS_NewArrayObject(js_cx, 0, NULL);

	for(argc=0;args[argc];argc++) {
		arg = JS_NewStringCopyZ(js_cx, args[argc]);
		if(arg==NULL)
			break;
		val=STRING_TO_JSVAL(arg);
		if(!JS_SetElement(js_cx, argv, argc, &val))
			break;
	}
	JS_DefineProperty(js_cx, js_scope, "argv", OBJECT_TO_JSVAL(argv)
		,NULL,NULL,JSPROP_READONLY|JSPROP_ENUMERATE);
	JS_DefineProperty(js_cx, js_scope, "argc", INT_TO_JSVAL(argc)
		,NULL,NULL,JSPROP_READONLY|JSPROP_ENUMERATE);

	if((js_script=JS_CompileFile(js_cx, js_scope, path))==NULL) {
		fprintf(errfp,"!Error executing %s\n",fname);
		return(-1);
	}

	JS_SetBranchCallback(js_cx, js_BranchCallback);

	JS_ExecuteScript(js_cx, js_scope, js_script, &rval);

	JS_DestroyScript(js_cx, js_script);

	JS_ClearScope(js_cx, js_scope);

	JS_GC(js_cx);

	return(JSVAL_TO_INT(rval));
}

/*********************/
/* Entry point (duh) */
/*********************/
int main(int argc, char **argv, char** environ)
{
	char	error[512];
	char*	module=NULL;
	char*	p;
	int		argn;

	confp=stdout;
	errfp=stderr;

	branch.limit=JAVASCRIPT_BRANCH_LIMIT;
	branch.yield_freq=JAVASCRIPT_YIELD_FREQUENCY;
	branch.gc_freq=JAVASCRIPT_GC_FREQUENCY;

	sscanf("$Revision: 1.8 $", "%*s %s", revision);

	p=getenv("SBBSCTRL");
	if(p==NULL) {
		fprintf(errfp,"\nSBBSCTRL environment variable not set.\n");
		fprintf(errfp,"\nExample: SET SBBSCTRL=/sbbs/ctrl\n");
		bail(1); 
	}

	memset(&scfg,0,sizeof(scfg));
	scfg.size=sizeof(scfg);
	SAFECOPY(scfg.ctrl_dir,p);

	for(argn=1;argn<argc;argn++) {
		if(argv[argn][0]=='-') {
			switch(argv[argn][1]) {
				case 'm':
					js_max_bytes=strtoul(argv[++argn],NULL,0);
					break;
				case 's':
					js_context_stack=strtoul(argv[++argn],NULL,0);
					break;
				case 'b':
					branch.limit=strtoul(argv[++argn],NULL,0);
					break;
				case 'y':
					branch.yield_freq=strtoul(argv[++argn],NULL,0);
					break;
				case 'g':
					branch.gc_freq=strtoul(argv[++argn],NULL,0);
					break;
				case 'e':
					errfp=confp;
					break;
				case 'q':
					confp=fopen(_PATH_DEVNULL,"w");
					break;
				case 'p':
					pause_on_exit=TRUE;
					break;
				default:
					usage(errfp);
					bail(1);
			}
			continue;
		}
		module=argv[argn];
	}

	if(module==NULL) {
		usage(errfp);
		bail(1); 
	}

	banner(errfp);

	if(chdir(scfg.ctrl_dir)!=0)
		fprintf(errfp,"!ERROR changing directory to: %s", scfg.ctrl_dir);

	printf("\nLoading configuration files from %s\n",scfg.ctrl_dir);
	if(!load_cfg(&scfg,NULL,TRUE,error)) {
		fprintf(errfp,"!ERROR loading configuration files: %s\n",error);
		bail(1);
	}
	prep_dir(scfg.data_dir, scfg.temp_dir, sizeof(scfg.temp_dir));

	if(!(scfg.sys_misc&SM_LOCAL_TZ))
		putenv("TZ=UTC0");

	if(!js_init(environ)) {
		fprintf(errfp,"!JavaScript initialization failure\n");
		bail(1);
	}
	printf("\n");

	bail(js_exec(module,&argv[argn]));

	return(-1);	/* never gets here */
}

