/* ftpsrvr.c */

/* Synchronet FTP server */

/* $Id: ftpsrvr.c,v 1.424 2016/05/27 07:44:46 rswindell Exp $ */

/****************************************************************************
 * @format.tab-size 4		(Plain Text/Source Code File Header)			*
 * @format.use-tabs true	(see http://www.synchro.net/ptsc_hdr.html)		*
 *																			*
 * Copyright Rob Swindell - http://www.synchro.net/copyright.html			*
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

/* ANSI C Library headers */
#include <stdio.h>
#include <stdlib.h>			/* ltoa in GNU C lib */
#include <stdarg.h>			/* va_list, varargs */
#include <string.h>			/* strrchr */
#include <fcntl.h>			/* O_WRONLY, O_RDONLY, etc. */
#include <errno.h>			/* EACCES */
#include <ctype.h>			/* toupper */
#include <sys/stat.h>		/* S_IWRITE */

/* Synchronet-specific headers */
#undef SBBS	/* this shouldn't be defined unless building sbbs.dll/libsbbs.so */
#include "sbbs.h"
#include "text.h"			/* TOTAL_TEXT */
#include "ftpsrvr.h"
#include "telnet.h"
#include "js_rtpool.h"
#include "js_request.h"
#include "multisock.h"

/* Constants */

#define FTP_SERVER				"Synchronet FTP Server"

#define STATUS_WFC				"Listening"
#define ANONYMOUS				"anonymous"

#define BBS_VIRTUAL_PATH		"bbs:/""/"	/* this is actually bbs:<slash><slash> */
#define LOCAL_FSYS_DIR			"local:"
#define BBS_FSYS_DIR			"bbs:"
#define BBS_HIDDEN_ALIAS		"hidden"

#define TIMEOUT_THREAD_WAIT		60		/* Seconds */

#define TIMEOUT_SOCKET_LISTEN	30		/* Seconds */

#define XFER_REPORT_INTERVAL	60		/* Seconds */

#define INDEX_FNAME_LEN			15

#define	NAME_LEN				15		/* User name length for listings */

static ftp_startup_t*	startup=NULL;
static scfg_t	scfg;
static struct xpms_set *ftp_set = NULL;
static protected_uint32_t active_clients;
static protected_uint32_t thread_count;
static volatile time_t	uptime=0;
static volatile ulong	served=0;
static volatile BOOL	terminate_server=FALSE;
static char		revision[16];
static char 	*text[TOTAL_TEXT];
static str_list_t recycle_semfiles;
static str_list_t shutdown_semfiles;

#ifdef SOCKET_DEBUG
	static BYTE 	socket_debug[0x10000]={0};

	#define	SOCKET_DEBUG_CTRL		(1<<0)	/* 0x01 */
	#define SOCKET_DEBUG_SEND		(1<<1)	/* 0x02 */
	#define SOCKET_DEBUG_READLINE	(1<<2)	/* 0x04 */
	#define SOCKET_DEBUG_ACCEPT		(1<<3)	/* 0x08 */
	#define SOCKET_DEBUG_SENDTHREAD	(1<<4)	/* 0x10 */
	#define SOCKET_DEBUG_TERMINATE	(1<<5)	/* 0x20 */
	#define SOCKET_DEBUG_RECV_CHAR	(1<<6)	/* 0x40 */
	#define SOCKET_DEBUG_FILEXFER	(1<<7)	/* 0x80 */
#endif

char* genvpath(int lib, int dir, char* str);

typedef struct {
	SOCKET			socket;
	union xp_sockaddr	client_addr;
	socklen_t		client_addr_len;
} ftp_t;


static const char *ftp_mon[]={"Jan","Feb","Mar","Apr","May","Jun"
            ,"Jul","Aug","Sep","Oct","Nov","Dec"};

BOOL direxist(char *dir)
{
	if(access(dir,0)==0)
		return(TRUE);
	else
		return(FALSE);
}

BOOL dir_op(scfg_t* cfg, user_t* user, client_t* client, uint dirnum)
{
	return(user->level>=SYSOP_LEVEL
		|| (cfg->dir[dirnum]->op_ar[0] && chk_ar(cfg,cfg->dir[dirnum]->op_ar,user,client)));
}

static int lprintf(int level, const char *fmt, ...)
{
	va_list argptr;
	char sbuf[1024];

    va_start(argptr,fmt);
    vsnprintf(sbuf,sizeof(sbuf),fmt,argptr);
	sbuf[sizeof(sbuf)-1]=0;
    va_end(argptr);

	if(level <= LOG_ERR) {
		char errmsg[sizeof(sbuf)+16];
		SAFEPRINTF(errmsg, "ftp %s", sbuf);
		errorlog(&scfg, startup==NULL ? NULL:startup->host_name, errmsg);
		if(startup!=NULL && startup->errormsg!=NULL)
			startup->errormsg(startup->cbdata,level,errmsg);
	}

    if(startup==NULL || startup->lputs==NULL || level > startup->log_level)
		return(0);

#if defined(_WIN32)
	if(IsBadCodePtr((FARPROC)startup->lputs))
		return(0);
#endif

    return startup->lputs(startup->cbdata,level,sbuf);
}

#ifdef _WINSOCKAPI_

static WSADATA WSAData;
#define SOCKLIB_DESC WSAData.szDescription

static BOOL WSAInitialized=FALSE;

static BOOL winsock_startup(void)
{
	int		status;             /* Status Code */

    if((status = WSAStartup(MAKEWORD(1,1), &WSAData))==0) {
		lprintf(LOG_DEBUG,"%s %s",WSAData.szDescription, WSAData.szSystemStatus);
		WSAInitialized=TRUE;
		return (TRUE);
	}

    lprintf(LOG_CRIT,"!WinSock startup ERROR %d", status);
	return (FALSE);
}

#else /* No WINSOCK */

#define winsock_startup()	(TRUE)
#define SOCKLIB_DESC		NULL

#endif

static void status(char* str)
{
	if(startup!=NULL && startup->status!=NULL)
	    startup->status(startup->cbdata,str);
}

static void update_clients(void)
{
	if(startup!=NULL && startup->clients!=NULL)
		startup->clients(startup->cbdata,protected_uint32_value(active_clients));
}

static void client_on(SOCKET sock, client_t* client, BOOL update)
{
	if(startup!=NULL && startup->client_on!=NULL)
		startup->client_on(startup->cbdata,TRUE,sock,client,update);
}

static void client_off(SOCKET sock)
{
	if(startup!=NULL && startup->client_on!=NULL)
		startup->client_on(startup->cbdata,FALSE,sock,NULL,FALSE);
}

static void thread_up(BOOL setuid)
{
	if(startup!=NULL && startup->thread_up!=NULL)
		startup->thread_up(startup->cbdata,TRUE, setuid);
}

static int32_t thread_down(void)
{
	int32_t count = protected_uint32_adjust(&thread_count,-1);
	if(startup!=NULL && startup->thread_up!=NULL)
		startup->thread_up(startup->cbdata,FALSE, FALSE);
	return count;
}

static void ftp_open_socket_cb(SOCKET sock, void *cbdata)
{
	char	error[256];

	if(startup!=NULL && startup->socket_open!=NULL)
		startup->socket_open(startup->cbdata,TRUE);
	if(set_socket_options(&scfg, sock, "FTP", error, sizeof(error)))
		lprintf(LOG_ERR,"%04d !ERROR %s",sock, error);
}

static void ftp_close_socket_cb(SOCKET sock, void *cbdata)
{
	if(startup!=NULL && startup->socket_open!=NULL)
		startup->socket_open(startup->cbdata,FALSE);
}

static SOCKET ftp_open_socket(int domain, int type)
{
	SOCKET	sock;

	sock=socket(domain, type, IPPROTO_IP);
	if(sock != INVALID_SOCKET)
		ftp_open_socket_cb(sock, NULL);
	return(sock);
}

#ifdef __BORLANDC__
#pragma argsused
#endif
static int ftp_close_socket(SOCKET* sock, int line)
{
	int		result;

	if((*sock)==INVALID_SOCKET) {
		lprintf(LOG_WARNING,"0000 !INVALID_SOCKET in close_socket from line %u",line);
		return(-1);
	}

	shutdown(*sock,SHUT_RDWR);	/* required on Unix */

	result=closesocket(*sock);
	if(startup!=NULL && startup->socket_open!=NULL) 
		startup->socket_open(startup->cbdata,FALSE);

	if(result!=0) {
		if(ERROR_VALUE!=ENOTSOCK)
			lprintf(LOG_WARNING,"%04d !ERROR %d closing socket from line %u",*sock,ERROR_VALUE,line);
	}
	*sock=INVALID_SOCKET;

	return(result);
}

static int sockprintf(SOCKET sock, char *fmt, ...)
{
	int		len;
	int		maxlen;
	int		result;
	va_list argptr;
	char	sbuf[1024];
	fd_set	socket_set;
	struct timeval tv;

    va_start(argptr,fmt);
    len=vsnprintf(sbuf,maxlen=sizeof(sbuf)-2,fmt,argptr);
    va_end(argptr);

	if(len<0 || len>maxlen) /* format error or output truncated */
		len=maxlen;
	if(startup!=NULL && startup->options&FTP_OPT_DEBUG_TX)
		lprintf(LOG_DEBUG,"%04d TX: %.*s", sock, len, sbuf);
	memcpy(sbuf+len,"\r\n",2);
	len+=2;

	if(sock==INVALID_SOCKET) {
		lprintf(LOG_WARNING,"!INVALID SOCKET in call to sockprintf");
		return(0);
	}

	/* Check socket for writability (using select) */
	tv.tv_sec=300;
	tv.tv_usec=0;

	FD_ZERO(&socket_set);
	FD_SET(sock,&socket_set);

	if((result=select(sock+1,NULL,&socket_set,NULL,&tv))<1) {
		if(result==0)
			lprintf(LOG_WARNING,"%04d !TIMEOUT selecting socket for send"
				,sock);
		else
			lprintf(LOG_WARNING,"%04d !ERROR %d selecting socket for send"
				,sock, ERROR_VALUE);
		return(0);
	}
	while((result=sendsocket(sock,sbuf,len))!=len) {
		if(result==SOCKET_ERROR) {
			if(ERROR_VALUE==EWOULDBLOCK) {
				YIELD();
				continue;
			}
			if(ERROR_VALUE==ECONNRESET) 
				lprintf(LOG_WARNING,"%04d Connection reset by peer on send",sock);
			else if(ERROR_VALUE==ECONNABORTED)
				lprintf(LOG_WARNING,"%04d Connection aborted by peer on send",sock);
			else
				lprintf(LOG_WARNING,"%04d !ERROR %d sending",sock,ERROR_VALUE);
			return(0);
		}
		lprintf(LOG_WARNING,"%04d !ERROR: short send: %u instead of %u",sock,result,len);
	}
	return(len);
}


/* Returns the directory index of a virtual lib/dir path (e.g. main/games/filename) */
int getdir(char* p, user_t* user, client_t* client)
{
	char*	tp;
	char	path[MAX_PATH+1];
	uint	dir;
	uint	lib;

	SAFECOPY(path,p);
	p=path;

	if(*p=='/') 
		p++;
	else if(!strncmp(p,"./",2))
		p+=2;

	tp=strchr(p,'/');
	if(tp) *tp=0;
	for(lib=0;lib<scfg.total_libs;lib++) {
		if(!chk_ar(&scfg,scfg.lib[lib]->ar,user,client))
			continue;
		if(!stricmp(scfg.lib[lib]->sname,p))
			break;
	}
	if(lib>=scfg.total_libs) 
		return(-1);

	if(tp!=NULL)
		p=tp+1;

	tp=strchr(p,'/');
	if(tp) *tp=0;
	for(dir=0;dir<scfg.total_dirs;dir++) {
		if(scfg.dir[dir]->lib!=lib)
			continue;
		if(dir!=scfg.sysop_dir && dir!=scfg.upload_dir 
			&& !chk_ar(&scfg,scfg.dir[dir]->ar,user,client))
			continue;
		if(!stricmp(scfg.dir[dir]->code_suffix,p))
			break;
	}
	if(dir>=scfg.total_dirs) 
		return(-1);

	return(dir);
}

/*********************************/
/* JavaScript Data and Functions */
/*********************************/
#ifdef JAVASCRIPT

js_server_props_t js_server_props;

static JSBool
js_write(JSContext *cx, uintN argc, jsval *arglist)
{
	jsval *argv=JS_ARGV(cx, arglist);
    uintN		i;
    JSString*	str=NULL;
	FILE*	fp;
	jsrefcount	rc;
	char		*p;
	size_t		len;

	JS_SET_RVAL(cx, arglist, JSVAL_VOID);

	if((fp=(FILE*)JS_GetContextPrivate(cx))==NULL)
		return(JS_FALSE);

    for (i = 0; i < argc; i++) {
		str = JS_ValueToString(cx, argv[i]);
		if (!str)
		    return JS_FALSE;
		JSSTRING_TO_MSTRING(cx, str, p, &len);
		HANDLE_PENDING(cx);
		rc=JS_SUSPENDREQUEST(cx);
		if(p) {
			fwrite(p, len, 1, fp);
			free(p);
		}
		JS_RESUMEREQUEST(cx, rc);
	}

	if(str==NULL)
		JS_SET_RVAL(cx, arglist, JSVAL_VOID);
	else
		JS_SET_RVAL(cx, arglist, STRING_TO_JSVAL(str));
	return(JS_TRUE);
}

static JSBool
js_writeln(JSContext *cx, uintN argc, jsval *arglist)
{
	FILE*	fp;
	jsrefcount	rc;

	JS_SET_RVAL(cx, arglist, JSVAL_VOID);

	if((fp=(FILE*)JS_GetContextPrivate(cx))==NULL)
		return(JS_FALSE);

	js_write(cx,argc,arglist);
	rc=JS_SUSPENDREQUEST(cx);
	fprintf(fp,"\r\n");
	JS_RESUMEREQUEST(cx, rc);
	return(JS_TRUE);
}

static JSFunctionSpec js_global_functions[] = {
	{"write",           js_write,           1},		/* write to HTML file */
	{"writeln",         js_writeln,         1},		/* write to HTML file */
	{"print",			js_writeln,         1},		/* alias for writeln */
	{0}
};

static void
js_ErrorReporter(JSContext *cx, const char *message, JSErrorReport *report)
{
	char	line[64];
	char	file[MAX_PATH+1];
	char*	warning;
	FILE*	fp;
	int		log_level;

	fp=(FILE*)JS_GetContextPrivate(cx);
	
	if(report==NULL) {
		lprintf(LOG_ERR,"!JavaScript: %s", message);
		if(fp!=NULL)
			fprintf(fp,"!JavaScript: %s", message);
		return;
    }

	if(report->filename)
		sprintf(file," %s",report->filename);
	else
		file[0]=0;

	if(report->lineno)
		sprintf(line," line %u",report->lineno);
	else
		line[0]=0;

	if(JSREPORT_IS_WARNING(report->flags)) {
		if(JSREPORT_IS_STRICT(report->flags))
			warning="strict warning";
		else
			warning="warning";
		log_level=LOG_WARNING;
	} else {
		log_level=LOG_ERR;
		warning="";
	}

	lprintf(log_level,"!JavaScript %s%s%s: %s",warning,file,line,message);
	if(fp!=NULL)
		fprintf(fp,"!JavaScript %s%s%s: %s",warning,file,line,message);
}

static JSContext* 
js_initcx(JSRuntime* runtime, SOCKET sock, JSObject** glob, JSObject** ftp, js_callback_t* cb)
{
	JSContext*	js_cx;
	BOOL		success=FALSE;
	BOOL		rooted=FALSE;

	lprintf(LOG_DEBUG,"%04d JavaScript: Initializing context (stack: %lu bytes)"
		,sock,startup->js.cx_stack);

    if((js_cx = JS_NewContext(runtime, startup->js.cx_stack))==NULL)
		return(NULL);
	JS_BEGINREQUEST(js_cx);

	lprintf(LOG_DEBUG,"%04d JavaScript: Context created",sock);

    JS_SetErrorReporter(js_cx, js_ErrorReporter);

	memset(cb, 0, sizeof(js_callback_t));

	/* ToDo: call js_CreateCommonObjects() instead */

	do {

		lprintf(LOG_DEBUG,"%04d JavaScript: Initializing Global object",sock);
		if(!js_CreateGlobalObject(js_cx, &scfg, NULL, &startup->js, glob))
			break;
		rooted=TRUE;

		if(!JS_DefineFunctions(js_cx, *glob, js_global_functions)) 
			break;

		/* Internal JS Object */
		if(js_CreateInternalJsObject(js_cx, *glob, cb, &startup->js)==NULL)
			break;

		lprintf(LOG_DEBUG,"%04d JavaScript: Initializing System object",sock);
		if(js_CreateSystemObject(js_cx, *glob, &scfg, uptime, startup->host_name, SOCKLIB_DESC)==NULL) 
			break;

		if((*ftp=JS_DefineObject(js_cx, *glob, "ftp", NULL
			,NULL,JSPROP_ENUMERATE|JSPROP_READONLY))==NULL)
			break;

		if(js_CreateServerObject(js_cx,*glob,&js_server_props)==NULL)
			break;

		success=TRUE;

	} while(0);

	if(!success) {
		if(rooted)
			JS_RemoveObjectRoot(js_cx, glob);
		JS_ENDREQUEST(js_cx);
		JS_DestroyContext(js_cx);
		return(NULL);
	}

	return(js_cx);
}

BOOL js_add_file(JSContext* js_cx, JSObject* array, 
				 char* name, char* desc, char* ext_desc,
				 ulong size, ulong credits, 
				 time_t time, time_t uploaded, time_t last_downloaded, 
				 ulong times_downloaded, ulong misc, 
				 char* uploader, char* link)
{
	JSObject*	file;
	JSString*	js_str;
	jsval		val;
	jsuint		index;

	if((file=JS_NewObject(js_cx, NULL, NULL, NULL))==NULL)
		return(FALSE);

	if((js_str=JS_NewStringCopyZ(js_cx, name))==NULL)
		return(FALSE);
	val=STRING_TO_JSVAL(js_str);
	if(!JS_SetProperty(js_cx, file, "name", &val))
		return(FALSE);

	if((js_str=JS_NewStringCopyZ(js_cx, desc))==NULL)
		return(FALSE);
	val=STRING_TO_JSVAL(js_str);
	if(!JS_SetProperty(js_cx, file, "description", &val))
		return(FALSE);

	if((js_str=JS_NewStringCopyZ(js_cx, ext_desc))==NULL)
		return(FALSE);
	val=STRING_TO_JSVAL(js_str);
	if(!JS_SetProperty(js_cx, file, "extended_description", &val))
		return(FALSE);

	val=INT_TO_JSVAL(size);
	if(!JS_SetProperty(js_cx, file, "size", &val))
		return(FALSE);

	val=INT_TO_JSVAL(credits);
	if(!JS_SetProperty(js_cx, file, "credits", &val))
		return(FALSE);

	val=DOUBLE_TO_JSVAL((double)time);
	if(!JS_SetProperty(js_cx, file, "time", &val))
		return(FALSE);

	val=INT_TO_JSVAL((int32)uploaded);
	if(!JS_SetProperty(js_cx, file, "uploaded", &val))
		return(FALSE);

	val=INT_TO_JSVAL((int32)last_downloaded);
	if(!JS_SetProperty(js_cx, file, "last_downloaded", &val))
		return(FALSE);

	val=INT_TO_JSVAL(times_downloaded);
	if(!JS_SetProperty(js_cx, file, "times_downloaded", &val))
		return(FALSE);

	if((js_str=JS_NewStringCopyZ(js_cx, uploader))==NULL)
		return(FALSE);
	val=STRING_TO_JSVAL(js_str);
	if(!JS_SetProperty(js_cx, file, "uploader", &val))
		return(FALSE);

	val=INT_TO_JSVAL(misc);
	if(!JS_SetProperty(js_cx, file, "settings", &val))
		return(FALSE);

	if((js_str=JS_NewStringCopyZ(js_cx, link))==NULL)
		return(FALSE);
	val=STRING_TO_JSVAL(js_str);
	if(!JS_SetProperty(js_cx, file, "link", &val))
		return(FALSE);

	if(!JS_GetArrayLength(js_cx, array, &index))
		return(FALSE);

	val=OBJECT_TO_JSVAL(file);
	return(JS_SetElement(js_cx, array, index, &val));
}

BOOL js_generate_index(JSContext* js_cx, JSObject* parent, 
					   SOCKET sock, FILE* fp, int lib, int dir, user_t* user, client_t* client)
{
	char		str[256];
	char		path[MAX_PATH+1];
	char		spath[MAX_PATH+1];	/* script path */
	char		vpath[MAX_PATH+1];	/* virtual path */
	char		aliasfile[MAX_PATH+1];
	char		extdesc[513];
	char*		p;
	char*		tp;
	char*		np;
	char*		dp;
	char		aliasline[512];
	BOOL		alias_dir;
	BOOL		success=FALSE;
	FILE*		alias_fp;
	uint		i;
	file_t		f;
	glob_t		g;
	jsval		val;
	jsval		rval;
	JSObject*	lib_obj=NULL;
	JSObject*	dir_obj=NULL;
	JSObject*	file_array=NULL;
	JSObject*	dir_array=NULL;
	JSObject*	js_script=NULL;
	JSString*	js_str;
	long double		start=xp_timer();
	jsrefcount	rc;

	lprintf(LOG_DEBUG,"%04d JavaScript: Generating HTML Index for %s"
		,sock, genvpath(lib,dir,str));

	JS_SetContextPrivate(js_cx, fp);

	do {	/* pseudo try/catch */

		if((file_array=JS_NewArrayObject(js_cx, 0, NULL))==NULL) {
			lprintf(LOG_ERR,"%04d !JavaScript FAILED to create file_array",sock);
			break;
		}

		/* file[] */
		val=OBJECT_TO_JSVAL(file_array);
		if(!JS_SetProperty(js_cx, parent, "file_list", &val)) {
			lprintf(LOG_ERR,"%04d !JavaScript FAILED to set file property",sock);
			break;
		}

		if((dir_array=JS_NewArrayObject(js_cx, 0, NULL))==NULL) {
			lprintf(LOG_ERR,"%04d !JavaScript FAILED to create dir_array",sock);
			break;
		}

		/* dir[] */
		val=OBJECT_TO_JSVAL(dir_array);
		if(!JS_SetProperty(js_cx, parent, "dir_list", &val)) {
			lprintf(LOG_ERR,"%04d !JavaScript FAILED to set dir property",sock);
			break;
		}

		rc=JS_SUSPENDREQUEST(js_cx);
		if(strcspn(startup->html_index_script,"/\\")==strlen(startup->html_index_script)) {
			sprintf(spath,"%s%s",scfg.mods_dir,startup->html_index_script);
			if(scfg.mods_dir[0]==0 || !fexist(spath))
				sprintf(spath,"%s%s",scfg.exec_dir,startup->html_index_script);
		} else
			sprintf(spath,"%.*s",(int)sizeof(spath)-4,startup->html_index_script);
		/* Add extension if not specified */
		if(!strchr(spath,'.'))
			strcat(spath,".js");

		if(!fexist(spath)) {
			JS_RESUMEREQUEST(js_cx, rc);
			lprintf(LOG_ERR,"%04d !HTML JavaScript (%s) doesn't exist",sock,spath);
			break;
		}
		JS_RESUMEREQUEST(js_cx, rc);

		if((js_str=JS_NewStringCopyZ(js_cx, startup->html_index_file))==NULL)
			break;
		val=STRING_TO_JSVAL(js_str);
		if(!JS_SetProperty(js_cx, parent, "html_index_file", &val)) {
			lprintf(LOG_ERR,"%04d !JavaScript FAILED to set html_index_file property",sock);
			break;
		}

		/* curlib */
		if((lib_obj=JS_NewObject(js_cx, NULL, 0, NULL))==NULL) {
			lprintf(LOG_ERR,"%04d !JavaScript FAILED to create lib_obj",sock);
			break;
		}

		val=OBJECT_TO_JSVAL(lib_obj);
		if(!JS_SetProperty(js_cx, parent, "curlib", &val)) {
			lprintf(LOG_ERR,"%04d !JavaScript FAILED to set curlib property",sock);
			break;
		}

		/* curdir */
		if((dir_obj=JS_NewObject(js_cx, NULL, 0, NULL))==NULL) {
			lprintf(LOG_ERR,"%04d !JavaScript FAILED to create dir_obj",sock);
			break;
		}

		val=OBJECT_TO_JSVAL(dir_obj);
		if(!JS_SetProperty(js_cx, parent, "curdir", &val)) {
			lprintf(LOG_ERR,"%04d !JavaScript FAILED to set curdir property",sock);
			break;
		}

		SAFECOPY(vpath,"/");

		if(lib>=0) { /* root */

			strcat(vpath,scfg.lib[lib]->sname);
			strcat(vpath,"/");

			if((js_str=JS_NewStringCopyZ(js_cx, scfg.lib[lib]->sname))==NULL)
				break;
			val=STRING_TO_JSVAL(js_str);
			if(!JS_SetProperty(js_cx, lib_obj, "name", &val)) {
				lprintf(LOG_ERR,"%04d !JavaScript FAILED to set curlib.name property",sock);
				break;
			}

			if((js_str=JS_NewStringCopyZ(js_cx, scfg.lib[lib]->lname))==NULL)
				break;
			val=STRING_TO_JSVAL(js_str);
			if(!JS_SetProperty(js_cx, lib_obj, "description", &val)) {
				lprintf(LOG_ERR,"%04d !JavaScript FAILED to set curlib.desc property",sock);
				break;
			}
		}

		if(dir>=0) { /* 1st level */
			strcat(vpath,scfg.dir[dir]->code_suffix);
			strcat(vpath,"/");

			if((js_str=JS_NewStringCopyZ(js_cx, scfg.dir[dir]->code))==NULL)
				break;
			val=STRING_TO_JSVAL(js_str);
			if(!JS_SetProperty(js_cx, dir_obj, "code", &val)) {
				lprintf(LOG_ERR,"%04d !JavaScript FAILED to set curdir.code property",sock);
				break;
			}

			if((js_str=JS_NewStringCopyZ(js_cx, scfg.dir[dir]->sname))==NULL)
				break;
			val=STRING_TO_JSVAL(js_str);
			if(!JS_SetProperty(js_cx, dir_obj, "name", &val)) {
				lprintf(LOG_ERR,"%04d !JavaScript FAILED to set curdir.name property",sock);
				break;
			}

			if((js_str=JS_NewStringCopyZ(js_cx, scfg.dir[dir]->lname))==NULL)
				break;
			val=STRING_TO_JSVAL(js_str);
			if(!JS_SetProperty(js_cx, dir_obj, "description", &val)) {
				lprintf(LOG_ERR,"%04d !JavaScript FAILED to set curdir.desc property",sock);
				break;
			}

			val=INT_TO_JSVAL(scfg.dir[dir]->misc);
			if(!JS_SetProperty(js_cx, dir_obj, "settings", &val)) {
				lprintf(LOG_ERR,"%04d !JavaScript FAILED to set curdir.misc property",sock);
				break;
			}
		}

		if((js_str=JS_NewStringCopyZ(js_cx, vpath))==NULL)
			break;
		val=STRING_TO_JSVAL(js_str);
		if(!JS_SetProperty(js_cx, parent, "path", &val)) {
			lprintf(LOG_ERR,"%04d !JavaScript FAILED to set curdir property",sock);
			break;
		}

		if(lib<0) {	/* root dir */

			rc=JS_SUSPENDREQUEST(js_cx);
			/* File Aliases */
			sprintf(path,"%sftpalias.cfg",scfg.ctrl_dir);
			if((alias_fp=fopen(path,"r"))!=NULL) {

				while(!feof(alias_fp)) {
					if(!fgets(aliasline,sizeof(aliasline),alias_fp))
						break;

					p=aliasline;	/* alias pointer */
					SKIP_WHITESPACE(p);

					if(*p==';')	/* comment */
						continue;

					tp=p;		/* terminator pointer */
					FIND_WHITESPACE(tp);
					if(*tp) *tp=0;

					np=tp+1;	/* filename pointer */
					SKIP_WHITESPACE(np);

					tp=np;		/* terminator pointer */
					FIND_WHITESPACE(tp);
					if(*tp) *tp=0;

					dp=tp+1;	/* description pointer */
					SKIP_WHITESPACE(dp);
					truncsp(dp);

					if(stricmp(dp,BBS_HIDDEN_ALIAS)==0)
						continue;

					alias_dir=FALSE;

					/* Virtual Path? */
					if(!strnicmp(np,BBS_VIRTUAL_PATH,strlen(BBS_VIRTUAL_PATH))) {
						if((dir=getdir(np+strlen(BBS_VIRTUAL_PATH),user,client))<0)
							continue; /* No access or invalid virtual path */
						tp=strrchr(np,'/');
						if(tp==NULL) 
							continue;
						tp++;
						if(*tp) {
							SAFEPRINTF2(aliasfile,"%s%s",scfg.dir[dir]->path,tp);
							np=aliasfile;
						}
						else 
							alias_dir=TRUE;
					}

					if(!alias_dir && !fexist(np))
						continue;

					if(alias_dir) {
						if(!chk_ar(&scfg,scfg.dir[dir]->ar,user,client))
							continue;
						SAFEPRINTF2(vpath,"/%s/%s",p,startup->html_index_file);
					} else
						SAFECOPY(vpath,p);
					JS_RESUMEREQUEST(js_cx, rc);
					js_add_file(js_cx
						,alias_dir ? dir_array : file_array
						,p				/* filename */
						,dp				/* description */
						,NULL			/* extdesc */
						,flength(np)	/* size */
						,0				/* credits */
						,fdate(np)		/* time */
						,fdate(np)		/* uploaded */
						,0				/* last downloaded */
						,0				/* times downloaded */
						,0				/* misc */
						,scfg.sys_id	/* uploader */
						,vpath			/* link */
						);
					rc=JS_SUSPENDREQUEST(js_cx);
				}

				fclose(alias_fp);
			}
			JS_RESUMEREQUEST(js_cx, rc);

			/* QWK Packet */
			if(startup->options&FTP_OPT_ALLOW_QWK /* && fexist(qwkfile) */) {
				sprintf(str,"%s.qwk",scfg.sys_id);
				SAFEPRINTF(vpath,"/%s",str);
				js_add_file(js_cx
					,file_array 
					,str						/* filename */
					,"QWK Message Packet"		/* description */
					,NULL						/* extdesc */
					,10240						/* size */
					,0							/* credits */
					,time(NULL)					/* time */
					,0							/* uploaded */
					,0							/* last downloaded */
					,0							/* times downloaded */
					,0							/* misc */
					,scfg.sys_id				/* uploader */
					,str						/* link */
					);
			}

			/* Library Folders */
			for(i=0;i<scfg.total_libs;i++) {
				if(!chk_ar(&scfg,scfg.lib[i]->ar,user,client))
					continue;
				SAFEPRINTF2(vpath,"/%s/%s",scfg.lib[i]->sname,startup->html_index_file);
				js_add_file(js_cx
					,dir_array 
					,scfg.lib[i]->sname			/* filename */
					,scfg.lib[i]->lname			/* description */
					,NULL						/* extdesc */
					,0,0,0,0,0,0,0				/* unused */
					,scfg.sys_id				/* uploader */
					,vpath						/* link */
					);
			}
		} else if(dir<0) {
			/* Directories */
			for(i=0;i<scfg.total_dirs;i++) {
				if(scfg.dir[i]->lib!=lib)
					continue;
				if(/* i!=scfg.sysop_dir && i!=scfg.upload_dir && */
					!chk_ar(&scfg,scfg.dir[i]->ar,user,client))
					continue;
				SAFEPRINTF3(vpath,"/%s/%s/%s"
					,scfg.lib[scfg.dir[i]->lib]->sname
					,scfg.dir[i]->code_suffix
					,startup->html_index_file);
				js_add_file(js_cx
					,dir_array 
					,scfg.dir[i]->sname			/* filename */
					,scfg.dir[i]->lname			/* description */
					,NULL						/* extdesc */
					,getfiles(&scfg,i)			/* size */
					,0,0,0,0,0					/* unused */
					,scfg.dir[i]->misc			/* misc */
					,scfg.sys_id				/* uploader */
					,vpath						/* link */
					);

			}
		} else if(chk_ar(&scfg,scfg.dir[dir]->ar,user,client)){
			SAFEPRINTF(path,"%s*",scfg.dir[dir]->path);
			rc=JS_SUSPENDREQUEST(js_cx);
			glob(path,0,NULL,&g);
			for(i=0;i<(int)g.gl_pathc;i++) {
				if(isdir(g.gl_pathv[i]))
					continue;
	#ifdef _WIN32
				GetShortPathName(g.gl_pathv[i], str, sizeof(str));
	#else
				SAFECOPY(str,g.gl_pathv[i]);
	#endif
				padfname(getfname(str),f.name);
				f.dir=dir;
				if(getfileixb(&scfg,&f)) {
					f.size=0; /* flength(g.gl_pathv[i]); */
					getfiledat(&scfg,&f);
					if(f.misc&FM_EXTDESC) {
						extdesc[0]=0;
						getextdesc(&scfg, dir, f.datoffset, extdesc);
						/* Remove Ctrl-A Codes and Ex-ASCII code */
						remove_ctrl_a(extdesc,extdesc);
					}
					SAFEPRINTF3(vpath,"/%s/%s/%s"
						,scfg.lib[scfg.dir[dir]->lib]->sname
						,scfg.dir[dir]->code_suffix
						,getfname(g.gl_pathv[i]));
					JS_RESUMEREQUEST(js_cx, rc);
					js_add_file(js_cx
						,file_array 
						,getfname(g.gl_pathv[i])	/* filename */
						,f.desc						/* description */
						,f.misc&FM_EXTDESC ? extdesc : NULL
						,f.size						/* size */
						,f.cdt						/* credits */
						,f.date						/* time */
						,f.dateuled					/* uploaded */
						,f.datedled					/* last downloaded */
						,f.timesdled				/* times downloaded */
						,f.misc						/* misc */
						,f.uler						/* uploader */
						,getfname(g.gl_pathv[i])	/* link */
						);
					rc=JS_SUSPENDREQUEST(js_cx);
				}
			}
			globfree(&g);
			JS_RESUMEREQUEST(js_cx, rc);
		}


		/* RUN SCRIPT */
		JS_ClearPendingException(js_cx);

		if((js_script=JS_CompileFile(js_cx, parent, spath))==NULL) {
			lprintf(LOG_ERR,"%04d !JavaScript FAILED to compile script (%s)",sock,spath);
			break;
		}

		js_PrepareToExecute(js_cx, parent, spath, /* startup_dir: */NULL, parent);
		if((success=JS_ExecuteScript(js_cx, parent, js_script, &rval))!=TRUE) {
			lprintf(LOG_ERR,"%04d !JavaScript FAILED to execute script (%s)",sock,spath);
			break;
		}

		lprintf(LOG_DEBUG,"%04d JavaScript: Done executing script: %s (%.2Lf seconds)"
			,sock,spath,xp_timer()-start);

	} while(0);


	JS_DeleteProperty(js_cx, parent, "path");
	JS_DeleteProperty(js_cx, parent, "sort");
	JS_DeleteProperty(js_cx, parent, "reverse");
	JS_DeleteProperty(js_cx, parent, "file_list");
	JS_DeleteProperty(js_cx, parent, "dir_list");
	JS_DeleteProperty(js_cx, parent, "curlib");
	JS_DeleteProperty(js_cx, parent, "curdir");
	JS_DeleteProperty(js_cx, parent, "html_index_file");

	return(success);
}


#endif	/* ifdef JAVASCRIPT */

BOOL upload_stats(ulong bytes)
{
	char	str[MAX_PATH+1];
	int		file;
	uint32_t	val;

	sprintf(str,"%sdsts.dab",scfg.ctrl_dir);
	if((file=nopen(str,O_RDWR))==-1) 
		return(FALSE);

	lseek(file,20L,SEEK_SET);   /* Skip timestamp, logons and logons today */
	read(file,&val,4);        /* Uploads today         */
	val++;
	lseek(file,-4L,SEEK_CUR);
	write(file,&val,4);
	read(file,&val,4);        /* Upload bytes today    */
	val+=bytes;
	lseek(file,-4L,SEEK_CUR);
	write(file,&val,4);
	close(file);
	return(TRUE);
}

BOOL download_stats(ulong bytes)
{
	char	str[MAX_PATH+1];
	int		file;
	uint32_t	val;

	sprintf(str,"%sdsts.dab",scfg.ctrl_dir);
	if((file=nopen(str,O_RDWR))==-1) 
		return(FALSE);

	lseek(file,28L,SEEK_SET);   /* Skip timestamp, logons and logons today */
	read(file,&val,4);        /* Downloads today         */
	val++;
	lseek(file,-4L,SEEK_CUR);
	write(file,&val,4);
	read(file,&val,4);        /* Download bytes today    */
	val+=bytes;
	lseek(file,-4L,SEEK_CUR);
	write(file,&val,4);
	close(file);
	return(TRUE);
}

void recverror(SOCKET socket, int rd, int line)
{
	if(rd==0) 
		lprintf(LOG_NOTICE,"%04d Socket closed by peer on receive (line %u)"
			,socket, line);
	else if(rd==SOCKET_ERROR) {
		if(ERROR_VALUE==ECONNRESET) 
			lprintf(LOG_NOTICE,"%04d Connection reset by peer on receive (line %u)"
				,socket, line);
		else if(ERROR_VALUE==ECONNABORTED) 
			lprintf(LOG_NOTICE,"%04d Connection aborted by peer on receive (line %u)"
				,socket, line);
		else
			lprintf(LOG_NOTICE,"%04d !ERROR %d receiving on socket (line %u)"
				,socket, ERROR_VALUE, line);
	} else
		lprintf(LOG_WARNING,"%04d !ERROR: recv on socket returned unexpected value: %d (line %u)"
			,socket, rd, line);
}

int sockreadline(SOCKET socket, char* buf, int len, time_t* lastactive)
{
	char	ch;
	int		i,rd=0;
	fd_set	socket_set;
	struct timeval	tv;

	buf[0]=0;

	if(socket==INVALID_SOCKET) {
		lprintf(LOG_WARNING,"INVALID SOCKET in call to sockreadline");
		return(0);
	}
	
	while(rd<len-1) {

		tv.tv_sec=startup->max_inactivity;
		tv.tv_usec=0;

		FD_ZERO(&socket_set);
		FD_SET(socket,&socket_set);

		i=select(socket+1,&socket_set,NULL,NULL,&tv);

		if(ftp_set==NULL || terminate_server) {
			sockprintf(socket,"421 Server downed, aborting.");
			lprintf(LOG_WARNING,"%04d Server downed, aborting",socket);
			return(0);
		}
		if(i<1) {
			if(i==0) {
				if((time(NULL)-(*lastactive))>startup->max_inactivity) {
					lprintf(LOG_WARNING,"%04d Disconnecting due to to inactivity",socket);
					sockprintf(socket,"421 Disconnecting due to inactivity (%u seconds)."
						,startup->max_inactivity);
					return(0);
				}
				continue;
			}
			recverror(socket,i,__LINE__);
			return(i);
		}
#ifdef SOCKET_DEBUG_RECV_CHAR
		socket_debug[socket]|=SOCKET_DEBUG_RECV_CHAR;
#endif
		i=recv(socket, &ch, 1, 0);
#ifdef SOCKET_DEBUG_RECV_CHAR
		socket_debug[socket]&=~SOCKET_DEBUG_RECV_CHAR;
#endif
		if(i<1) {
			recverror(socket,i,__LINE__);
			return(i);
		}
		if(ch=='\n' /* && rd>=1 */) { /* Mar-9-2003: terminate on sole LF */
			break;
		}	
		buf[rd++]=ch;
	}
	if(rd>0 && buf[rd-1]=='\r')
		buf[rd-1]=0;
	else
		buf[rd]=0;
	
	return(rd);
}

void DLLCALL ftp_terminate(void)
{
   	lprintf(LOG_INFO,"FTP Server terminate");
	terminate_server=TRUE;
}

int ftp_remove(SOCKET sock, int line, const char* fname)
{
	int ret=0;

	if(fexist(fname) && (ret=remove(fname))!=0)
		lprintf(LOG_ERR,"%04d !ERROR %d (line %d) removing file: %s", sock, ret, line, fname);
	return ret;
}

typedef struct {
	SOCKET		ctrl_sock;
	SOCKET*		data_sock;
	BOOL*		inprogress;
	BOOL*		aborted;
	BOOL		delfile;
	BOOL		tmpfile;
	BOOL		credits;
	BOOL		append;
	long		filepos;
	char		filename[MAX_PATH+1];
	time_t*		lastactive;
	user_t*		user;
	client_t*	client;
	int			dir;
	char*		desc;
} xfer_t;

static void send_thread(void* arg)
{
	char		buf[8192];
	char		fname[MAX_PATH+1];
	char		str[128];
	char		tmp[128];
	char		username[128];
	char		host_ip[INET6_ADDRSTRLEN];
	int			i;
	int			rd;
	int			wr;
	long		mod;
	ulong		l;
	ulong		total=0;
	ulong		last_total=0;
	ulong		dur;
	ulong		cps;
	ulong		length;
	BOOL		error=FALSE;
	FILE*		fp;
	file_t		f;
	xfer_t		xfer;
	time_t		now;
	time_t		start;
	time_t		last_report;
	user_t		uploader;
	union xp_sockaddr	addr;
	socklen_t	addr_len;
	fd_set		socket_set;
	struct timeval tv;

	xfer=*(xfer_t*)arg;
	free(arg);

	SetThreadName("FTP Send");
	thread_up(TRUE /* setuid */);

	length=flength(xfer.filename);

	if((fp=fnopen(NULL,xfer.filename,O_RDONLY|O_BINARY))==NULL	/* non-shareable open failed */
		&& (fp=fopen(xfer.filename,"rb"))==NULL) {				/* shareable open failed */
		lprintf(LOG_ERR,"%04d !DATA ERROR %d opening %s",xfer.ctrl_sock,errno,xfer.filename);
		sockprintf(xfer.ctrl_sock,"450 ERROR %d opening %s.",errno,xfer.filename);
		if(xfer.tmpfile && !(startup->options&FTP_OPT_KEEP_TEMP_FILES))
			ftp_remove(xfer.ctrl_sock, __LINE__, xfer.filename);
		ftp_close_socket(xfer.data_sock,__LINE__);
		*xfer.inprogress=FALSE;
		thread_down();
		return;
	}

#ifdef SOCKET_DEBUG_SENDTHREAD
			socket_debug[xfer.ctrl_sock]|=SOCKET_DEBUG_SENDTHREAD;
#endif

	*xfer.aborted=FALSE;
	if(startup->options&FTP_OPT_DEBUG_DATA || xfer.filepos)
		lprintf(LOG_DEBUG,"%04d DATA socket %d sending %s from offset %lu"
			,xfer.ctrl_sock,*xfer.data_sock,xfer.filename,xfer.filepos);

	fseek(fp,xfer.filepos,SEEK_SET);
	last_report=start=time(NULL);
	while((xfer.filepos+total)<length) {

		now=time(NULL);

		/* Periodic progress report */
		if(total && now>=last_report+XFER_REPORT_INTERVAL) {
			if(xfer.filepos)
				sprintf(str," from offset %lu",xfer.filepos);
			else
				str[0]=0;
			lprintf(LOG_INFO,"%04d Sent %lu bytes (%lu total) of %s (%lu cps)%s"
				,xfer.ctrl_sock,total,length,xfer.filename
				,(ulong)((total-last_total)/(now-last_report))
				,str);
			last_total=total;
			last_report=now;
		}

		if(*xfer.aborted==TRUE) {
			lprintf(LOG_WARNING,"%04d !DATA Transfer aborted",xfer.ctrl_sock);
			sockprintf(xfer.ctrl_sock,"426 Transfer aborted.");
			error=TRUE;
			break;
		}
		if(ftp_set==NULL || terminate_server) {
			lprintf(LOG_WARNING,"%04d !DATA Transfer locally aborted",xfer.ctrl_sock);
			sockprintf(xfer.ctrl_sock,"426 Transfer locally aborted.");
			error=TRUE;
			break;
		}

		/* Check socket for writability (using select) */
		tv.tv_sec=1;
		tv.tv_usec=0;

		FD_ZERO(&socket_set);
		FD_SET(*xfer.data_sock,&socket_set);

		i=select((*xfer.data_sock)+1,NULL,&socket_set,NULL,&tv);
		if(i==SOCKET_ERROR) {
			lprintf(LOG_WARNING,"%04d !DATA ERROR %d selecting socket %d for send"
				,xfer.ctrl_sock, ERROR_VALUE, *xfer.data_sock);
			sockprintf(xfer.ctrl_sock,"426 Transfer error.");
			error=TRUE;
			break;
		}
		if(i<1)
			continue;

		fseek(fp,xfer.filepos+total,SEEK_SET);
		rd=fread(buf,sizeof(char),sizeof(buf),fp);
		if(rd<1) /* EOF or READ error */
			break;

#ifdef SOCKET_DEBUG_SEND
		socket_debug[xfer.ctrl_sock]|=SOCKET_DEBUG_SEND;
#endif
		wr=sendsocket(*xfer.data_sock,buf,rd);
#ifdef SOCKET_DEBUG_SEND
		socket_debug[xfer.ctrl_sock]&=~SOCKET_DEBUG_SEND;
#endif
		if(wr<1) {
			if(wr==SOCKET_ERROR) {
				if(ERROR_VALUE==EWOULDBLOCK) {
					/*lprintf(LOG_WARNING,"%04d DATA send would block, retrying",xfer.ctrl_sock);*/
					YIELD();
					continue;
				}
				else if(ERROR_VALUE==ECONNRESET) 
					lprintf(LOG_WARNING,"%04d DATA Connection reset by peer, sending on socket %d"
						,xfer.ctrl_sock,*xfer.data_sock);
				else if(ERROR_VALUE==ECONNABORTED) 
					lprintf(LOG_WARNING,"%04d DATA Connection aborted by peer, sending on socket %d"
						,xfer.ctrl_sock,*xfer.data_sock);
				else
					lprintf(LOG_WARNING,"%04d !DATA ERROR %d sending on data socket %d"
						,xfer.ctrl_sock,ERROR_VALUE,*xfer.data_sock);
				/* Send NAK */
				sockprintf(xfer.ctrl_sock,"426 Error %d sending on DATA channel"
					,ERROR_VALUE);
				error=TRUE;
				break;
			}
			if(wr==0) {
				lprintf(LOG_WARNING,"%04d !DATA socket %d disconnected",xfer.ctrl_sock, *xfer.data_sock);
				sockprintf(xfer.ctrl_sock,"426 DATA channel disconnected");
				error=TRUE;
				break;
			}
			lprintf(LOG_ERR,"%04d !DATA SEND ERROR %d (%d) on socket %d"
				,xfer.ctrl_sock, wr, ERROR_VALUE, *xfer.data_sock);
			sockprintf(xfer.ctrl_sock,"451 DATA send error");
			error=TRUE;
			break;
		}
		total+=wr;
		*xfer.lastactive=time(NULL);
		YIELD();
	}

	if((i=ferror(fp))!=0) 
		lprintf(LOG_ERR,"%04d !FILE ERROR %d (%d)",xfer.ctrl_sock,i,errno);

	ftp_close_socket(xfer.data_sock,__LINE__);	/* Signal end of file */
	if(startup->options&FTP_OPT_DEBUG_DATA)
		lprintf(LOG_DEBUG,"%04d DATA socket closed",xfer.ctrl_sock);
	
	if(!error) {
		dur=(long)(time(NULL)-start);
		cps=dur ? total/dur : total*2;
		lprintf(LOG_INFO,"%04d Transfer successful: %lu bytes sent in %lu seconds (%lu cps)"
			,xfer.ctrl_sock
			,total,dur,cps);
		sockprintf(xfer.ctrl_sock,"226 Download complete (%lu cps).",cps);

		if(xfer.dir>=0) {
			memset(&f,0,sizeof(f));
#ifdef _WIN32
			GetShortPathName(xfer.filename,fname,sizeof(fname));
#else
			SAFECOPY(fname,xfer.filename);
#endif
			padfname(getfname(fname),f.name);
			f.dir=xfer.dir;
			f.size=total;
			if(getfileixb(&scfg,&f)==TRUE && getfiledat(&scfg,&f)==TRUE) {
				f.timesdled++;
				putfiledat(&scfg,&f);
				f.datedled=time32(NULL);
				putfileixb(&scfg,&f);

				lprintf(LOG_INFO,"%04d %s downloaded: %s (%lu times total)"
					,xfer.ctrl_sock
					,xfer.user->alias
					,xfer.filename
					,f.timesdled);
				/**************************/
				/* Update Uploader's Info */
				/**************************/
				uploader.number=matchuser(&scfg,f.uler,TRUE /*sysop_alias*/);
				if(uploader.number
					&& uploader.number!=xfer.user->number 
					&& getuserdat(&scfg,&uploader)==0
					&& uploader.firston<f.dateuled) {
					l=f.cdt;
					if(!(scfg.dir[f.dir]->misc&DIR_CDTDL))	/* Don't give credits on d/l */
						l=0;
					if(scfg.dir[f.dir]->misc&DIR_CDTMIN && cps) { /* Give min instead of cdt */
						mod=((ulong)(l*(scfg.dir[f.dir]->dn_pct/100.0))/cps)/60;
						adjustuserrec(&scfg,uploader.number,U_MIN,10,mod);
						sprintf(tmp,"%lu minute",mod);
					} else {
						mod=(ulong)(l*(scfg.dir[f.dir]->dn_pct/100.0));
						adjustuserrec(&scfg,uploader.number,U_CDT,10,mod);
						ultoac(mod,tmp);
					}
					if(!(scfg.dir[f.dir]->misc&DIR_QUIET)) {
						addr_len = sizeof(addr);
						if(uploader.level>=SYSOP_LEVEL
							&& getpeername(xfer.ctrl_sock,&addr.addr,&addr_len)==0
							&& inet_addrtop(&addr, host_ip, sizeof(host_ip))!=NULL)
							SAFEPRINTF2(username,"%s [%s]",xfer.user->alias,host_ip);
						else
							SAFECOPY(username,xfer.user->alias);
						/* Inform uploader of downloaded file */
						safe_snprintf(str,sizeof(str),text[DownloadUserMsg]
							,getfname(xfer.filename)
							,xfer.filepos ? "partially FTP-" : "FTP-"
							,username,tmp); 
						putsmsg(&scfg,uploader.number,str); 
					}
				}
			}
			if(!xfer.tmpfile && !xfer.delfile && !(scfg.dir[f.dir]->misc&DIR_NOSTAT))
				download_stats(total);
		}	

		if(xfer.credits) {
			user_downloaded(&scfg, xfer.user, 1, total);
			if(xfer.dir>=0 && !is_download_free(&scfg,xfer.dir,xfer.user,xfer.client))
				subtract_cdt(&scfg, xfer.user, xfer.credits);
		}
	}

	fclose(fp);
	if(ftp_set!=NULL && !terminate_server)
		*xfer.inprogress=FALSE;
	if(xfer.tmpfile) {
		if(!(startup->options&FTP_OPT_KEEP_TEMP_FILES))
			ftp_remove(xfer.ctrl_sock, __LINE__, xfer.filename);
	} 
	else if(xfer.delfile && !error)
		ftp_remove(xfer.ctrl_sock, __LINE__, xfer.filename);

#if defined(SOCKET_DEBUG_SENDTHREAD)
			socket_debug[xfer.ctrl_sock]&=~SOCKET_DEBUG_SENDTHREAD;
#endif

	thread_down();
}

static void receive_thread(void* arg)
{
	char*		p;
	char		str[128];
	char		buf[8192];
	char		ext[F_EXBSIZE+1];
	char		desc[F_EXBSIZE+1];
	char		cmd[MAX_PATH*2];
	char		tmp[MAX_PATH+1];
	char		fname[MAX_PATH+1];
	int			i;
	int			rd;
	int			file;
	ulong		total=0;
	ulong		last_total=0;
	ulong		dur;
	ulong		cps;
	BOOL		error=FALSE;
	BOOL		filedat;
	FILE*		fp;
	file_t		f;
	xfer_t		xfer;
	time_t		now;
	time_t		start;
	time_t		last_report;
	fd_set		socket_set;
	struct timeval tv;

	xfer=*(xfer_t*)arg;
	free(arg);

	SetThreadName("FTP RECV");
	thread_up(TRUE /* setuid */);

	if((fp=fopen(xfer.filename,xfer.append ? "ab" : "wb"))==NULL) {
		lprintf(LOG_ERR,"%04d !DATA ERROR %d opening %s",xfer.ctrl_sock,errno,xfer.filename);
		sockprintf(xfer.ctrl_sock,"450 ERROR %d opening %s.",errno,xfer.filename);
		ftp_close_socket(xfer.data_sock,__LINE__);
		*xfer.inprogress=FALSE;
		thread_down();
		return;
	}

	if(xfer.append)
		xfer.filepos=filelength(fileno(fp));

	*xfer.aborted=FALSE;
	if(xfer.filepos || startup->options&FTP_OPT_DEBUG_DATA)
		lprintf(LOG_DEBUG,"%04d DATA socket %d receiving %s from offset %lu"
			,xfer.ctrl_sock,*xfer.data_sock,xfer.filename,xfer.filepos);

	fseek(fp,xfer.filepos,SEEK_SET);
	last_report=start=time(NULL);
	while(1) {

		now=time(NULL);

		/* Periodic progress report */
		if(total && now>=last_report+XFER_REPORT_INTERVAL) {
			if(xfer.filepos)
				sprintf(str," from offset %lu",xfer.filepos);
			else
				str[0]=0;
			lprintf(LOG_INFO,"%04d Received %lu bytes of %s (%lu cps)%s"
				,xfer.ctrl_sock,total,xfer.filename
				,(ulong)((total-last_total)/(now-last_report))
				,str);
			last_total=total;
			last_report=now;
		}
		if(startup->max_fsize && (xfer.filepos+total) > startup->max_fsize) {
			lprintf(LOG_WARNING,"%04d !DATA received %lu bytes of %s exceeds maximum allowed (%lu bytes)"
				,xfer.ctrl_sock, xfer.filepos+total, xfer.filename, startup->max_fsize);
			sockprintf(xfer.ctrl_sock,"552 File size exceeds maximum allowed (%lu bytes)", startup->max_fsize);
			error=TRUE;
			break;
		}
		if(*xfer.aborted==TRUE) {
			lprintf(LOG_WARNING,"%04d !DATA Transfer aborted",xfer.ctrl_sock);
			/* Send NAK */
			sockprintf(xfer.ctrl_sock,"426 Transfer aborted.");
			error=TRUE;
			break;
		}
		if(ftp_set==NULL || terminate_server) {
			lprintf(LOG_WARNING,"%04d !DATA Transfer locally aborted",xfer.ctrl_sock);
			/* Send NAK */
			sockprintf(xfer.ctrl_sock,"426 Transfer locally aborted.");
			error=TRUE;
			break;
		}

		/* Check socket for readability (using select) */
		tv.tv_sec=1;
		tv.tv_usec=0;

		FD_ZERO(&socket_set);
		FD_SET(*xfer.data_sock,&socket_set);

		i=select((*xfer.data_sock)+1,&socket_set,NULL,NULL,&tv);
		if(i==SOCKET_ERROR) {
			lprintf(LOG_WARNING,"%04d !DATA ERROR %d selecting socket %d for receive"
				,xfer.ctrl_sock, ERROR_VALUE, *xfer.data_sock);
			sockprintf(xfer.ctrl_sock,"426 Transfer error.");
			error=TRUE;
			break;
		}
		if(i<1)
			continue;

#if defined(SOCKET_DEBUG_RECV_BUF)
		socket_debug[xfer.ctrl_sock]|=SOCKET_DEBUG_RECV_BUF;
#endif
		rd=recv(*xfer.data_sock,buf,sizeof(buf),0);
#if defined(SOCKET_DEBUG_RECV_BUF)
		socket_debug[xfer.ctrl_sock]&=~SOCKET_DEBUG_RECV_BUF;
#endif
		if(rd<1) {
			if(rd==0) { /* Socket closed */
				if(startup->options&FTP_OPT_DEBUG_DATA)
					lprintf(LOG_DEBUG,"%04d DATA socket %d closed by client"
						,xfer.ctrl_sock,*xfer.data_sock);
				break;
			}
			if(rd==SOCKET_ERROR) {
				if(ERROR_VALUE==EWOULDBLOCK) {
					/*lprintf(LOG_WARNING,"%04d DATA recv would block, retrying",xfer.ctrl_sock);*/
					YIELD();
					continue;
				}
				else if(ERROR_VALUE==ECONNRESET) 
					lprintf(LOG_WARNING,"%04d DATA Connection reset by peer, receiving on socket %d"
						,xfer.ctrl_sock,*xfer.data_sock);
				else if(ERROR_VALUE==ECONNABORTED) 
					lprintf(LOG_WARNING,"%04d DATA Connection aborted by peer, receiving on socket %d"
						,xfer.ctrl_sock,*xfer.data_sock);
				else
					lprintf(LOG_WARNING,"%04d !DATA ERROR %d receiving on data socket %d"
						,xfer.ctrl_sock,ERROR_VALUE,*xfer.data_sock);
				/* Send NAK */
				sockprintf(xfer.ctrl_sock,"426 Error %d receiving on DATA channel"
					,ERROR_VALUE);
				error=TRUE;
				break;
			}
			lprintf(LOG_ERR,"%04d !DATA ERROR recv returned %d on socket %d"
				,xfer.ctrl_sock,rd,*xfer.data_sock);
			/* Send NAK */
			sockprintf(xfer.ctrl_sock,"451 Unexpected socket error: %d",rd);
			error=TRUE;
			break;
		}
		fwrite(buf,1,rd,fp);
		total+=rd;
		*xfer.lastactive=time(NULL);
		YIELD();
	}

	fclose(fp);

	ftp_close_socket(xfer.data_sock,__LINE__);
	if(error && startup->options&FTP_OPT_DEBUG_DATA)
		lprintf(LOG_DEBUG,"%04d DATA socket %d closed",xfer.ctrl_sock,*xfer.data_sock);
	
	if(xfer.filepos+total < startup->min_fsize) {
		lprintf(LOG_WARNING,"%04d DATA received %lu bytes for %s, less than minimum required (%lu bytes)"
			,xfer.ctrl_sock, xfer.filepos+total, xfer.filename, startup->min_fsize);
		sockprintf(xfer.ctrl_sock,"550 File size less than minimum required (%lu bytes)"
			,startup->min_fsize);
		error=TRUE;
	}
	if(error) {
		if(!xfer.append)
			ftp_remove(xfer.ctrl_sock, __LINE__, xfer.filename);
	} else {
		dur=(long)(time(NULL)-start);
		cps=dur ? total/dur : total*2;
		lprintf(LOG_INFO,"%04d Transfer successful: %lu bytes received in %lu seconds (%lu cps)"
			,xfer.ctrl_sock
			,total,dur,cps);

		if(xfer.dir>=0) {
			memset(&f,0,sizeof(f));
#ifdef _WIN32
			GetShortPathName(xfer.filename,fname,sizeof(fname));
#else
			SAFECOPY(fname,xfer.filename);
#endif
			padfname(getfname(fname),f.name);
			f.dir=xfer.dir;
			filedat=getfileixb(&scfg,&f);
			if(scfg.dir[f.dir]->misc&DIR_AONLY)  /* Forced anonymous */
				f.misc|=FM_ANON;
			f.cdt=flength(xfer.filename);
			f.dateuled=time32(NULL);

			/* Description specified with DESC command? */
			if(xfer.desc!=NULL && *xfer.desc!=0)	
				SAFECOPY(f.desc,xfer.desc);

			/* Necessary for DIR and LIB ARS keyword support in subsequent chk_ar()'s */
			SAFECOPY(xfer.user->curdir, scfg.dir[f.dir]->code);

			/* FILE_ID.DIZ support */
			p=strrchr(f.name,'.');
			if(p!=NULL && scfg.dir[f.dir]->misc&DIR_DIZ) {
				for(i=0;i<scfg.total_fextrs;i++)
					if(!stricmp(scfg.fextr[i]->ext,p+1) 
						&& chk_ar(&scfg,scfg.fextr[i]->ar,xfer.user,xfer.client))
						break;
				if(i<scfg.total_fextrs) {
					sprintf(tmp,"%sFILE_ID.DIZ",scfg.temp_dir);
					if(fexistcase(tmp))
						ftp_remove(xfer.ctrl_sock, __LINE__, tmp);
					cmdstr(&scfg,xfer.user,scfg.fextr[i]->cmd,fname,"FILE_ID.DIZ",cmd);
					lprintf(LOG_DEBUG,"%04d Extracting DIZ: %s",xfer.ctrl_sock,cmd);
					system(cmd);
					if(!fexistcase(tmp)) {
						sprintf(tmp,"%sDESC.SDI",scfg.temp_dir);
						if(fexistcase(tmp))
							ftp_remove(xfer.ctrl_sock, __LINE__, tmp);
						cmdstr(&scfg,xfer.user,scfg.fextr[i]->cmd,fname,"DESC.SDI",cmd);
						lprintf(LOG_DEBUG,"%04d Extracting DIZ: %s",xfer.ctrl_sock,cmd);
						system(cmd); 
						fexistcase(tmp);	/* fixes filename case */
					}
					if((file=nopen(tmp,O_RDONLY))!=-1) {
						lprintf(LOG_DEBUG,"%04d Parsing DIZ: %s",xfer.ctrl_sock,tmp);
						memset(ext,0,sizeof(ext));
						read(file,ext,sizeof(ext)-1);
						for(i=sizeof(ext)-1;i;i--)	/* trim trailing spaces */
							if(ext[i-1]>' ')
								break;
						ext[i]=0;
						if(!f.desc[0]) {			/* use for normal description */
							SAFECOPY(desc,ext);
							strip_exascii(desc, desc);	/* strip extended ASCII chars */
							prep_file_desc(desc, desc);	/* strip control chars and dupe chars */
							for(i=0;desc[i];i++)	/* find approprate first char */
								if(isalnum(desc[i]))
									break;
							SAFECOPY(f.desc,desc+i); 
						}
						close(file);
						ftp_remove(xfer.ctrl_sock, __LINE__, tmp);
						f.misc|=FM_EXTDESC; 
					} else
						lprintf(LOG_DEBUG,"%04d DIZ Does not exist: %s",xfer.ctrl_sock,tmp);
				} 
			} /* FILE_ID.DIZ support */

			if(f.desc[0]==0) 	/* no description given, use (long) filename */
				SAFECOPY(f.desc,getfname(xfer.filename));

			SAFECOPY(f.uler,xfer.user->alias);	/* exception here, Aug-27-2002 */
			if(filedat) {
				if(!putfiledat(&scfg,&f))
					lprintf(LOG_ERR,"%04d !ERROR updating file (%s) in database",xfer.ctrl_sock,f.name);
				/* need to update the index here */
			} else {
				if(!addfiledat(&scfg,&f))
					lprintf(LOG_ERR,"%04d !ERROR adding file (%s) to database",xfer.ctrl_sock,f.name);
			}

			if(f.misc&FM_EXTDESC)
				putextdesc(&scfg,f.dir,f.datoffset,ext);

			if(scfg.dir[f.dir]->upload_sem[0])
				ftouch(scfg.dir[f.dir]->upload_sem);
			/**************************/
			/* Update Uploader's Info */
			/**************************/
			user_uploaded(&scfg, xfer.user, (!xfer.append && xfer.filepos==0) ? 1:0, total);
			if(scfg.dir[f.dir]->up_pct && scfg.dir[f.dir]->misc&DIR_CDTUL) { /* credit for upload */
				if(scfg.dir[f.dir]->misc&DIR_CDTMIN && cps)    /* Give min instead of cdt */
					xfer.user->min=adjustuserrec(&scfg,xfer.user->number,U_MIN,10
						,((ulong)(total*(scfg.dir[f.dir]->up_pct/100.0))/cps)/60);
				else
					xfer.user->cdt=adjustuserrec(&scfg,xfer.user->number,U_CDT,10
						,(ulong)(f.cdt*(scfg.dir[f.dir]->up_pct/100.0))); 
			}
			if(!(scfg.dir[f.dir]->misc&DIR_NOSTAT))
				upload_stats(total);
		}
		/* Send ACK */
		sockprintf(xfer.ctrl_sock,"226 Upload complete (%lu cps).",cps);
	}

	if(ftp_set!=NULL && !terminate_server)
		*xfer.inprogress=FALSE;

	thread_down();
}



static void filexfer(union xp_sockaddr* addr, SOCKET ctrl_sock, SOCKET pasv_sock, SOCKET* data_sock
					,char* filename, long filepos, BOOL* inprogress, BOOL* aborted
					,BOOL delfile, BOOL tmpfile
					,time_t* lastactive
					,user_t* user
					,client_t* client
					,int dir
					,BOOL receiving
					,BOOL credits
					,BOOL append
					,char* desc)
{
	int			result;
	ulong		l;
	socklen_t	addr_len;
	union xp_sockaddr	server_addr;
	BOOL		reuseaddr;
	xfer_t*		xfer;
	struct timeval	tv;
	fd_set			socket_set;
	char		host_ip[INET6_ADDRSTRLEN];

	if((*inprogress)==TRUE) {
		lprintf(LOG_WARNING,"%04d !TRANSFER already in progress",ctrl_sock);
		sockprintf(ctrl_sock,"425 Transfer already in progress.");
		if(tmpfile && !(startup->options&FTP_OPT_KEEP_TEMP_FILES))
			ftp_remove(ctrl_sock, __LINE__, filename);
		return;
	}
	*inprogress=TRUE;

	if(*data_sock!=INVALID_SOCKET)
		ftp_close_socket(data_sock,__LINE__);

	inet_addrtop(addr, host_ip, sizeof(host_ip));
	if(pasv_sock==INVALID_SOCKET) {	/* !PASV */

		if((*data_sock=socket(addr->addr.sa_family, SOCK_STREAM, IPPROTO_IP)) == INVALID_SOCKET) {
			lprintf(LOG_ERR,"%04d !DATA ERROR %d opening socket", ctrl_sock, ERROR_VALUE);
			sockprintf(ctrl_sock,"425 Error %d opening socket",ERROR_VALUE);
			if(tmpfile && !(startup->options&FTP_OPT_KEEP_TEMP_FILES))
				ftp_remove(ctrl_sock, __LINE__, filename);
			*inprogress=FALSE;
			return;
		}
		if(startup->socket_open!=NULL)
			startup->socket_open(startup->cbdata,TRUE);
		if(startup->options&FTP_OPT_DEBUG_DATA)
			lprintf(LOG_DEBUG,"%04d DATA socket %d opened",ctrl_sock,*data_sock);

		/* Use port-1 for all data connections */
		reuseaddr=TRUE;
		setsockopt(*data_sock,SOL_SOCKET,SO_REUSEADDR,(char*)&reuseaddr,sizeof(reuseaddr));

		addr_len = sizeof(server_addr);
		if((result=getsockname(ctrl_sock, &server_addr.addr,&addr_len))!=0) {
			lprintf(LOG_ERR,"%04d !ERROR %d (%d) getting address/port of command socket (%u)"
				,ctrl_sock,result,ERROR_VALUE,pasv_sock);
			return;
		}

		inet_setaddrport(&server_addr, inet_addrport(&server_addr)-1);	/* 20? */

		result=bind(*data_sock, &server_addr.addr,addr_len);
		if(result!=0) {
			inet_setaddrport(&server_addr, 0);	/* any user port */
			result=bind(*data_sock, &server_addr.addr,addr_len);
		}
		if(result!=0) {
			lprintf(LOG_ERR,"%04d !DATA ERROR %d (%d) binding socket %d"
				,ctrl_sock, result, ERROR_VALUE, *data_sock);
			sockprintf(ctrl_sock,"425 Error %d binding socket",ERROR_VALUE);
			if(tmpfile && !(startup->options&FTP_OPT_KEEP_TEMP_FILES))
				ftp_remove(ctrl_sock, __LINE__, filename);
			*inprogress=FALSE;
			ftp_close_socket(data_sock,__LINE__);
			return;
		}

		result=connect(*data_sock, &addr->addr,xp_sockaddr_len(addr));
		if(result!=0) {
			lprintf(LOG_WARNING,"%04d !DATA ERROR %d (%d) connecting to client %s port %u on socket %d"
					,ctrl_sock,result,ERROR_VALUE
					,host_ip,inet_addrport(addr),*data_sock);
			sockprintf(ctrl_sock,"425 Error %d connecting to socket",ERROR_VALUE);
			if(tmpfile && !(startup->options&FTP_OPT_KEEP_TEMP_FILES))
				ftp_remove(ctrl_sock, __LINE__, filename);
			*inprogress=FALSE;
			ftp_close_socket(data_sock,__LINE__);
			return;
		}
		if(startup->options&FTP_OPT_DEBUG_DATA)
			lprintf(LOG_DEBUG,"%04d DATA socket %d connected to %s port %u"
				,ctrl_sock,*data_sock,host_ip,inet_addrport(addr));

	} else {	/* PASV */

		if(startup->options&FTP_OPT_DEBUG_DATA) {
			addr_len=sizeof(*addr);
			if((result=getsockname(pasv_sock, &addr->addr,&addr_len))!=0)
				lprintf(LOG_ERR,"%04d !ERROR %d (%d) getting address/port of passive socket (%u)"
					,ctrl_sock,result,ERROR_VALUE,pasv_sock);
			else
				lprintf(LOG_DEBUG,"%04d PASV DATA socket %d listening on %s port %u"
					,ctrl_sock,pasv_sock,host_ip,inet_addrport(addr));
		}

		/* Setup for select() */
		tv.tv_sec=TIMEOUT_SOCKET_LISTEN;
		tv.tv_usec=0;

		FD_ZERO(&socket_set);
		FD_SET(pasv_sock,&socket_set);

#if defined(SOCKET_DEBUG_SELECT)
		socket_debug[ctrl_sock]|=SOCKET_DEBUG_SELECT;
#endif
		result=select(pasv_sock+1,&socket_set,NULL,NULL,&tv);
#if defined(SOCKET_DEBUG_SELECT)
		socket_debug[ctrl_sock]&=~SOCKET_DEBUG_SELECT;
#endif
		if(result<1) {
			lprintf(LOG_WARNING,"%04d !PASV select returned %d (error: %d)",ctrl_sock,result,ERROR_VALUE);
			sockprintf(ctrl_sock,"425 Error %d selecting socket for connection",ERROR_VALUE);
			if(tmpfile && !(startup->options&FTP_OPT_KEEP_TEMP_FILES))
				ftp_remove(ctrl_sock, __LINE__, filename);
			*inprogress=FALSE;
			return;
		}
			
		addr_len=sizeof(*addr);
#ifdef SOCKET_DEBUG_ACCEPT
		socket_debug[ctrl_sock]|=SOCKET_DEBUG_ACCEPT;
#endif
		*data_sock=accept(pasv_sock,&addr->addr,&addr_len);
#ifdef SOCKET_DEBUG_ACCEPT
		socket_debug[ctrl_sock]&=~SOCKET_DEBUG_ACCEPT;
#endif
		if(*data_sock==INVALID_SOCKET) {
			lprintf(LOG_WARNING,"%04d !PASV DATA ERROR %d accepting connection on socket %d"
				,ctrl_sock,ERROR_VALUE,pasv_sock);
			sockprintf(ctrl_sock,"425 Error %d accepting connection",ERROR_VALUE);
			if(tmpfile && !(startup->options&FTP_OPT_KEEP_TEMP_FILES))
				ftp_remove(ctrl_sock, __LINE__, filename);
			*inprogress=FALSE;
			return;
		}
		if(startup->socket_open!=NULL)
			startup->socket_open(startup->cbdata,TRUE);
		if(startup->options&FTP_OPT_DEBUG_DATA)
			lprintf(LOG_DEBUG,"%04d PASV DATA socket %d connected to %s port %u"
				,ctrl_sock,*data_sock,host_ip,inet_addrport(addr));
	}

	do {

		l=1;

		if(ioctlsocket(*data_sock, FIONBIO, &l)!=0) {
			lprintf(LOG_ERR,"%04d !DATA ERROR %d disabling socket blocking"
				,ctrl_sock, ERROR_VALUE);
			sockprintf(ctrl_sock,"425 Error %d disabling socket blocking"
				,ERROR_VALUE);
			break;
		}

		if((xfer=malloc(sizeof(xfer_t)))==NULL) {
			lprintf(LOG_CRIT,"%04d !MALLOC FAILURE LINE %d",ctrl_sock,__LINE__);
			sockprintf(ctrl_sock,"425 MALLOC FAILURE");
			break;
		}
		memset(xfer,0,sizeof(xfer_t));
		xfer->ctrl_sock=ctrl_sock;
		xfer->data_sock=data_sock;
		xfer->inprogress=inprogress;
		xfer->aborted=aborted;
		xfer->delfile=delfile;
		xfer->tmpfile=tmpfile;
		xfer->append=append;
		xfer->filepos=filepos;
		xfer->credits=credits;
		xfer->lastactive=lastactive;
		xfer->user=user;
		xfer->client=client;
		xfer->dir=dir;
		xfer->desc=desc;
		SAFECOPY(xfer->filename,filename);
		protected_uint32_adjust(&thread_count,1);
		if(receiving)
			result=_beginthread(receive_thread,0,(void*)xfer);
		else
			result=_beginthread(send_thread,0,(void*)xfer);

		if(result!=-1)
			return;	/* success */

	} while(0);

	/* failure */
	if(tmpfile && !(startup->options&FTP_OPT_KEEP_TEMP_FILES))
		ftp_remove(ctrl_sock, __LINE__, filename);
	*inprogress=FALSE;
}

/* convert "user name" to "user.name" or "mr. user" to "mr._user" */
char* dotname(char* in, char* out)
{
	char	ch;
	int		i;

	if(strchr(in,'.')==NULL)
		ch='.';
	else
		ch='_';
	for(i=0;in[i];i++)
		if(in[i]<=' ')
			out[i]=ch;
		else
			out[i]=in[i];
	out[i]=0;
	return(out);
}

void parsepath(char** pp, user_t* user, client_t* client, int* curlib, int* curdir)
{
	char*	p;
	char*	tp;
	char	path[MAX_PATH+1];
	int		dir=*curdir;
	int		lib=*curlib;

	SAFECOPY(path,*pp);
	p=path;

	if(*p=='/') {
		p++;
		lib=-1;
	}
	else if(!strncmp(p,"./",2))
		p+=2;

	if(!strncmp(p,"..",2)) {
		p+=2;
		if(dir>=0)
			dir=-1;
		else if(lib>=0)
			lib=-1;
		if(*p=='/')
			p++;
	}

	if(*p==0) {
		*curlib=lib;
		*curdir=dir;
		return;
	}

	if(lib<0) { /* root */
		tp=strchr(p,'/');
		if(tp)
			*(tp++)=0;
		else
			tp=p+strlen(p);
		for(lib=0;lib<scfg.total_libs;lib++) {
			if(!chk_ar(&scfg,scfg.lib[lib]->ar,user,client))
				continue;
			if(!stricmp(scfg.lib[lib]->sname,p))
				break;
		}
		if(lib>=scfg.total_libs) { /* not found */
			*curlib=-1;
			return;
		}
		*curlib=lib;

		if(*(tp)==0) {
			*curdir=-1;
			*pp+=tp-path;	/* skip "lib" or "lib/" */
			return;
		}

		p=tp;
	}

	tp=strchr(p,'/');
	if(tp!=NULL) {
		*tp=0;
		tp++;
	} else 
		tp=p+strlen(p);

	for(dir=0;dir<scfg.total_dirs;dir++) {
		if(scfg.dir[dir]->lib!=lib)
			continue;
		if(dir!=scfg.sysop_dir && dir!=scfg.upload_dir 
			&& !chk_ar(&scfg,scfg.dir[dir]->ar,user,client))
			continue;
		if(!stricmp(scfg.dir[dir]->code_suffix,p))
			break;
	}

	if(dir>=scfg.total_dirs) {  /* not found */
		*pp+=p-path;			/* skip /lib/filespec */
		return;
	}

	*curdir=dir;

	*pp+=tp-path;	/* skip "lib/dir/" */
}

static BOOL ftpalias(char* fullalias, char* filename, user_t* user, client_t* client, int* curdir)
{
	char*	p;
	char*	tp;
	char*	fname="";
	char	line[512];
	char	alias[512];
	char	aliasfile[MAX_PATH+1];
	int		dir=-1;
	FILE*	fp;
	BOOL	result=FALSE;

	sprintf(aliasfile,"%sftpalias.cfg",scfg.ctrl_dir);
	if((fp=fopen(aliasfile,"r"))==NULL) 
		return(FALSE);

	SAFECOPY(alias,fullalias);
	p=strrchr(alias+1,'/');
	if(p) {
		*p=0;
		fname=p+1;
	}

	if(filename==NULL /* directory */ && *fname /* filename specified */) {
		fclose(fp);
		return(FALSE);
	}

	while(!feof(fp)) {
		if(!fgets(line,sizeof(line),fp))
			break;

		p=line;	/* alias */
		SKIP_WHITESPACE(p);
		if(*p==';')	/* comment */
			continue;

		tp=p;		/* terminator */
		FIND_WHITESPACE(tp);
		if(*tp) *tp=0;

		if(stricmp(p,alias))	/* Not a match */
			continue;

		p=tp+1;		/* filename */
		SKIP_WHITESPACE(p);

		tp=p;		/* terminator */
		FIND_WHITESPACE(tp);
		if(*tp) *tp=0;

		if(!strnicmp(p,BBS_VIRTUAL_PATH,strlen(BBS_VIRTUAL_PATH))) {
			if((dir=getdir(p+strlen(BBS_VIRTUAL_PATH),user,client))<0)	{
				lprintf(LOG_WARNING,"0000 !Invalid virtual path (%s) for %s",p,user->alias);
				/* invalid or no access */
				continue;
			}
			p=strrchr(p,'/');
			if(p!=NULL) p++;
			if(p!=NULL && filename!=NULL) {
				if(*p)
					sprintf(filename,"%s%s",scfg.dir[dir]->path,p);
				else
					sprintf(filename,"%s%s",scfg.dir[dir]->path,fname);
			}
		} else if(filename!=NULL)
			strcpy(filename,p);

		result=TRUE;	/* success */
		break;
	}
	fclose(fp);
	if(curdir!=NULL)
		*curdir=dir;
	return(result);
}

char* root_dir(char* path)
{
	char*	p;
	static char	root[MAX_PATH+1];

	SAFECOPY(root,path);

	if(!strncmp(root,"\\\\",2)) {	/* network path */
		p=strchr(root+2,'\\');
		if(p) p=strchr(p+1,'\\');
		if(p) *(p+1)=0;				/* truncate at \\computer\sharename\ */
	} 
	else if(!strncmp(root+1,":/",2) || !strncmp(root+1,":\\",2))
		root[3]=0;
	else if(*root=='/' || *root=='\\')
		root[1]=0;

	return(root);
}

char* genvpath(int lib, int dir, char* str)
{
	strcpy(str,"/");
	if(lib<0)
		return(str);
	strcat(str,scfg.lib[lib]->sname);
	strcat(str,"/");
	if(dir<0)
		return(str);
	strcat(str,scfg.dir[dir]->code_suffix);
	strcat(str,"/");
	return(str);
}

void ftp_printfile(SOCKET sock, const char* name, unsigned code)
{
	char	path[MAX_PATH+1];
	char	buf[512];
	FILE*	fp;
	unsigned i;

	SAFEPRINTF2(path,"%sftp%s.txt",scfg.text_dir,name);
	if((fp=fopen(path,"rb"))!=NULL) {
		i=0;
		while(!feof(fp)) {
			if(!fgets(buf,sizeof(buf),fp))
				break;
			truncsp(buf);
			if(!i)
				sockprintf(sock,"%u-%s",code,buf);
			else
				sockprintf(sock," %s",buf);
			i++;
		}
		fclose(fp);
	}
}

static BOOL ftp_hacklog(char* prot, char* user, char* text, char* host, union xp_sockaddr* addr)
{
#ifdef _WIN32
	if(startup->hack_sound[0] && !(startup->options&FTP_OPT_MUTE)) 
		PlaySound(startup->hack_sound, NULL, SND_ASYNC|SND_FILENAME);
#endif

	return hacklog(&scfg, prot, user, text, host, addr);
}

/****************************************************************************/
/* Consecutive failed login (possible password hack) attempt tracking		*/
/****************************************************************************/

static BOOL badlogin(SOCKET sock, ulong* login_attempts, char* user, char* passwd, char* host, union xp_sockaddr* addr)
{
	ulong count;
	char	host_ip[INET6_ADDRSTRLEN];

	if(addr!=NULL) {
		count=loginFailure(startup->login_attempt_list, addr, "FTP", user, passwd);
		if(startup->login_attempt.hack_threshold && count>=startup->login_attempt.hack_threshold)
			ftp_hacklog("FTP LOGIN", user, passwd, host, addr);
		if(startup->login_attempt.filter_threshold && count>=startup->login_attempt.filter_threshold) {
			inet_addrtop(addr, host_ip, sizeof(host_ip));
			filter_ip(&scfg, "FTP", "- TOO MANY CONSECUTIVE FAILED LOGIN ATTEMPTS"
				,host, host_ip, user, /* fname: */NULL);
		}
		if(count > *login_attempts)
			*login_attempts=count;
	} else
		(*login_attempts)++;

	mswait(startup->login_attempt.delay);	/* As recommended by RFC2577 */

	if((*login_attempts)>=3) {
		sockprintf(sock,"421 Too many failed login attempts.");
		return(TRUE);
	}
	ftp_printfile(sock,"badlogin",530);
	sockprintf(sock,"530 Invalid login.");
	return(FALSE);
}

static char* ftp_tmpfname(char* fname, char* ext, SOCKET sock)
{
	safe_snprintf(fname,MAX_PATH,"%sSBBS_FTP.%x%x%x%lx.%s"
		,scfg.temp_dir,getpid(),sock,rand(),clock(),ext);
	return(fname);
}

static void ctrl_thread(void* arg)
{
	char		buf[512];
	char		str[128];
	char*		cmd;
	char*		p;
	char*		np;
	char*		tp;
	char*		dp;
	char*		ap;
	char*		filespec;
	char*		mode="active";
	char		old_char;
	char		password[64];
	char		fname[MAX_PATH+1];
	char		qwkfile[MAX_PATH+1];
	char		aliasfile[MAX_PATH+1];
	char		aliasline[512];
	char		desc[501]="";
	char		sys_pass[128];
	char		host_name[256];
	char		host_ip[INET6_ADDRSTRLEN];
	char		data_ip[INET6_ADDRSTRLEN];
	uint16_t	data_port;
	char		path[MAX_PATH+1];
	char		local_dir[MAX_PATH+1];
	char		ren_from[MAX_PATH+1]="";
	char		html_index_ext[MAX_PATH+1];
	WORD		port;
	uint32_t	ip_addr;
	socklen_t	addr_len;
	unsigned	h1,h2,h3,h4;
	u_short		p1,p2;	/* For PORT command */
	int			i;
	int			rd;
	int			result;
	int			lib;
	int			dir;
	int			curlib=-1;
	int			curdir=-1;
	int			orglib;
	int			orgdir;
	long		filepos=0L;
	long		timeleft;
	ulong		l;
	ulong		login_attempts=0;
	ulong		avail;	/* disk space */
	ulong		count;
	BOOL		detail;
	BOOL		success;
	BOOL		getdate;
	BOOL		getsize;
	BOOL		delecmd;
	BOOL		delfile;
	BOOL		tmpfile;
	BOOL		credits;
	BOOL		filedat=FALSE;
	BOOL		transfer_inprogress;
	BOOL		transfer_aborted;
	BOOL		sysop=FALSE;
	BOOL		local_fsys=FALSE;
	BOOL		alias_dir;
	BOOL		append;
	BOOL		reuseaddr;
	FILE*		fp;
	FILE*		alias_fp;
	SOCKET		sock;
	SOCKET		tmp_sock;
	SOCKET		pasv_sock=INVALID_SOCKET;
	SOCKET		data_sock=INVALID_SOCKET;
	HOSTENT*	host;
	union xp_sockaddr	addr;
	union xp_sockaddr	data_addr;
	union xp_sockaddr	pasv_addr;
	ftp_t		ftp=*(ftp_t*)arg;
	user_t		user;
	time_t		t;
	time_t		now;
	time_t		logintime=0;
	time_t		lastactive;
	time_t		file_date;
	file_t		f;
	glob_t		g;
	node_t		node;
	client_t	client;
	struct tm	tm;
	struct tm 	cur_tm;
#ifdef JAVASCRIPT
	jsval		js_val;
	JSRuntime*	js_runtime=NULL;
	JSContext*	js_cx=NULL;
	JSObject*	js_glob;
	JSObject*	js_ftp;
	JSString*	js_str;
	js_callback_t	js_callback;
#endif
	login_attempt_t attempted;

	SetThreadName("FTP CTRL");
	thread_up(TRUE /* setuid */);

	lastactive=time(NULL);

	sock=ftp.socket;
	memcpy(&data_addr, &ftp.client_addr, ftp.client_addr_len);
	/* Default data port is ctrl port-1 */
	data_port = inet_addrport(&data_addr)-1;
	
	lprintf(LOG_DEBUG,"%04d CTRL thread started", sock);

	free(arg);

#ifdef _WIN32
	if(startup->answer_sound[0] && !(startup->options&FTP_OPT_MUTE)) 
		PlaySound(startup->answer_sound, NULL, SND_ASYNC|SND_FILENAME);
#endif

	transfer_inprogress = FALSE;
	transfer_aborted = FALSE;

	l=1;

	if((i=ioctlsocket(sock, FIONBIO, &l))!=0) {
		lprintf(LOG_ERR,"%04d !ERROR %d (%d) disabling socket blocking"
			,sock, i, ERROR_VALUE);
		sockprintf(sock,"425 Error %d disabling socket blocking"
			,ERROR_VALUE);
		ftp_close_socket(&sock,__LINE__);
		thread_down();
		return;
	}

	memset(&user,0,sizeof(user));

	inet_addrtop(&ftp.client_addr, host_ip, sizeof(host_ip));

	lprintf(LOG_INFO,"%04d CTRL connection accepted from: %s port %u"
		,sock, host_ip, inet_addrport(&ftp.client_addr));

	if(startup->options&FTP_OPT_NO_HOST_LOOKUP)
		strcpy(host_name,"<no name>");
	else {
		if(getnameinfo(&ftp.client_addr.addr, sizeof(ftp.client_addr), host_name, sizeof(host_name), NULL, 0, NI_NAMEREQD)!=0)
			strcpy(host_name,"<no name>");
	}

	if(!(startup->options&FTP_OPT_NO_HOST_LOOKUP))
		lprintf(LOG_INFO,"%04d Hostname: %s", sock, host_name);

	ulong banned = loginBanned(&scfg, startup->login_attempt_list, sock,  startup->login_attempt, &attempted);
	if(banned || trashcan(&scfg,host_ip,"ip")) {
		if(banned) {
			char ban_duration[128];
			lprintf(LOG_NOTICE, "%04d !TEMPORARY BAN of %s (%u login attempts, last: %s) - remaining: %s"
				,sock, host_ip, attempted.count, attempted.user, seconds_to_str(banned, ban_duration));
		} else
			lprintf(LOG_NOTICE,"%04d !CLIENT BLOCKED in ip.can: %s", sock, host_ip);
		sockprintf(sock,"550 Access denied.");
		ftp_close_socket(&sock,__LINE__);
		thread_down();
		return;
	}

	if(trashcan(&scfg,host_name,"host")) {
		lprintf(LOG_NOTICE,"%04d !CLIENT BLOCKED in host.can: %s", sock, host_name);
		sockprintf(sock,"550 Access denied.");
		ftp_close_socket(&sock,__LINE__);
		thread_down();
		return;
	}

	/* For PASV mode */
	addr_len=sizeof(pasv_addr);
	if((result=getsockname(sock, &pasv_addr.addr,&addr_len))!=0) {
		lprintf(LOG_ERR,"%04d !ERROR %d (%d) getting address/port", sock, result, ERROR_VALUE);
		sockprintf(sock,"425 Error %d getting address/port",ERROR_VALUE);
		ftp_close_socket(&sock,__LINE__);
		thread_down();
		return;
	} 

	protected_uint32_adjust(&active_clients, 1), 
	update_clients();

	/* Initialize client display */
	client.size=sizeof(client);
	client.time=time32(NULL);
	SAFECOPY(client.addr,host_ip);
	SAFECOPY(client.host,host_name);
	client.port=inet_addrport(&ftp.client_addr);
	client.protocol="FTP";
	client.user="<unknown>";
	client_on(sock,&client,FALSE /* update */);

	if(startup->login_attempt.throttle
		&& (login_attempts=loginAttempts(startup->login_attempt_list, &ftp.client_addr)) > 1) {
		lprintf(LOG_DEBUG,"%04d Throttling suspicious connection from: %s (%u login attempts)"
			,sock, host_ip, login_attempts);
		mswait(login_attempts*startup->login_attempt.throttle);
	}

	sockprintf(sock,"220-%s (%s)",scfg.sys_name, startup->host_name);
	sockprintf(sock," Synchronet FTP Server %s-%s Ready"
		,revision,PLATFORM_DESC);
	sprintf(str,"%sftplogin.txt",scfg.text_dir);
	if((fp=fopen(str,"rb"))!=NULL) {
		while(!feof(fp)) {
			if(!fgets(buf,sizeof(buf),fp))
				break;
			truncsp(buf);
			sockprintf(sock," %s",buf);
		}
		fclose(fp);
	}
	sockprintf(sock,"220 Please enter your user name.");

#ifdef SOCKET_DEBUG_CTRL
	socket_debug[sock]|=SOCKET_DEBUG_CTRL;
#endif
	while(1) {

#ifdef SOCKET_DEBUG_READLINE
		socket_debug[sock]|=SOCKET_DEBUG_READLINE;
#endif
		rd = sockreadline(sock, buf, sizeof(buf), &lastactive);
#ifdef SOCKET_DEBUG_READLINE
		socket_debug[sock]&=~SOCKET_DEBUG_READLINE;
#endif
		if(rd<1) {
			if(transfer_inprogress==TRUE) {
				lprintf(LOG_WARNING,"%04d Aborting transfer due to receive error",sock);
				transfer_aborted=TRUE;
			}
			break;
		}
		truncsp(buf);
		lastactive=time(NULL);
		cmd=buf;
		while(((BYTE)*cmd)==TELNET_IAC) {
			cmd++;
			lprintf(LOG_DEBUG,"%04d RX: Telnet cmd: %s",sock,telnet_cmd_desc(*cmd));
			cmd++;
		}
		while(*cmd && *cmd<' ') {
			lprintf(LOG_DEBUG,"%04d RX: %d (0x%02X)",sock,(BYTE)*cmd,(BYTE)*cmd);
			cmd++;
		}
		if(!(*cmd))
			continue;
		if(startup->options&FTP_OPT_DEBUG_RX)
			lprintf(LOG_DEBUG,"%04d RX: %s", sock, cmd);
		if(!stricmp(cmd, "NOOP")) {
			sockprintf(sock,"200 NOOP command successful.");
			continue;
		}
		if(!stricmp(cmd, "HELP SITE") || !stricmp(cmd, "SITE HELP")) {
			sockprintf(sock,"214-The following SITE commands are recognized (* => unimplemented):");
			sockprintf(sock," HELP    VER     WHO     UPTIME");
			if(user.level>=SYSOP_LEVEL)
				sockprintf(sock,
							" RECYCLE [ALL]");
			if(sysop)
				sockprintf(sock,
							" EXEC <cmd>");
			sockprintf(sock,"214 Direct comments to sysop@%s.",scfg.sys_inetaddr);
			continue;
		}
		if(!strnicmp(cmd, "HELP",4)) {
			sockprintf(sock,"214-The following commands are recognized (* => unimplemented, # => extension):");
			sockprintf(sock," USER    PASS    CWD     XCWD    CDUP    XCUP    PWD     XPWD");
			sockprintf(sock," QUIT    REIN    PORT    PASV    LIST    NLST    NOOP    HELP");
			sockprintf(sock," SIZE    MDTM    RETR    STOR    REST    ALLO    ABOR    SYST");
			sockprintf(sock," TYPE    STRU    MODE    SITE    RNFR*   RNTO*   DELE*   DESC#");
			sockprintf(sock," FEAT#   OPTS#   EPRT    EPSV");
			sockprintf(sock,"214 Direct comments to sysop@%s.",scfg.sys_inetaddr);
			continue;
		}
		if(!stricmp(cmd, "FEAT")) {
			sockprintf(sock,"211-The following additional (post-RFC949) features are supported:");
			sockprintf(sock," DESC");
			sockprintf(sock," MDTM");
			sockprintf(sock," SIZE");
			sockprintf(sock," REST STREAM");
			sockprintf(sock,"211 End");
			continue;
		}
		if(!strnicmp(cmd, "OPTS",4)) {
			sockprintf(sock,"501 No options supported.");
			continue;
		}
		if(!stricmp(cmd, "QUIT")) {
			ftp_printfile(sock,"bye",221);
			sockprintf(sock,"221 Goodbye. Closing control connection.");
			break;
		}
		if(!strnicmp(cmd, "USER ",5)) {
			sysop=FALSE;
			user.number=0;
			p=cmd+5;
			SKIP_WHITESPACE(p);
			truncsp(p);
			SAFECOPY(user.alias,p);
			user.number=matchuser(&scfg,user.alias,FALSE /*sysop_alias*/);
			if(!user.number && !stricmp(user.alias,"anonymous"))	
				user.number=matchuser(&scfg,"guest",FALSE);
			if(user.number && getuserdat(&scfg, &user)==0 && user.pass[0]==0) 
				sockprintf(sock,"331 User name okay, give your full e-mail address as password.");
			else
				sockprintf(sock,"331 User name okay, need password.");
			user.number=0;
			continue;
		}
		if(!strnicmp(cmd, "PASS ",5) && user.alias[0]) {
			user.number=0;
			p=cmd+5;
			SKIP_WHITESPACE(p);

			SAFECOPY(password,p);
			user.number=matchuser(&scfg,user.alias,FALSE /*sysop_alias*/);
			if(!user.number) {
				if(scfg.sys_misc&SM_ECHO_PW)
					lprintf(LOG_WARNING,"%04d !UNKNOWN USER: '%s' (password: %s)",sock,user.alias,p);
				else
					lprintf(LOG_WARNING,"%04d !UNKNOWN USER: '%s'",sock,user.alias);
				if(badlogin(sock, &login_attempts, user.alias, p, host_name, &ftp.client_addr))
					break;
				continue;
			}
			if((i=getuserdat(&scfg, &user))!=0) {
				lprintf(LOG_ERR,"%04d !ERROR %d getting data for user #%d (%s)"
					,sock,i,user.number,user.alias);
				sockprintf(sock,"530 Database error %d",i);
				user.number=0;
				continue;
			}
			if(user.misc&(DELETED|INACTIVE)) {
				lprintf(LOG_WARNING,"%04d !DELETED or INACTIVE user #%d (%s)"
					,sock,user.number,user.alias);
				user.number=0;
				if(badlogin(sock, &login_attempts, NULL, NULL, NULL, NULL))
					break;
				continue;
			}
			if(user.rest&FLAG('T')) {
				lprintf(LOG_WARNING,"%04d !T RESTRICTED user #%d (%s)"
					,sock,user.number,user.alias);
				user.number=0;
				if(badlogin(sock, &login_attempts, NULL, NULL, NULL, NULL))
					break;
				continue;
			}
			if(user.ltoday>=scfg.level_callsperday[user.level]
				&& !(user.exempt&FLAG('L'))) {
				lprintf(LOG_WARNING,"%04d !MAXIMUM LOGONS (%d) reached for %s"
					,sock,scfg.level_callsperday[user.level],user.alias);
				sockprintf(sock,"530 Maximum logons per day reached.");
				user.number=0;
				continue;
			}
			if(user.rest&FLAG('L') && user.ltoday>=1) {
				lprintf(LOG_WARNING,"%04d !L RESTRICTED user #%d (%s) already on today"
					,sock,user.number,user.alias);
				sockprintf(sock,"530 Maximum logons per day reached.");
				user.number=0;
				continue;
			}

			SAFEPRINTF2(sys_pass,"%s:%s",user.pass,scfg.sys_pass);
			if(!user.pass[0]) {	/* Guest/Anonymous */
				if(trashcan(&scfg,password,"email")) {
					lprintf(LOG_NOTICE,"%04d !BLOCKED e-mail address: %s",sock,password);
					user.number=0;
					if(badlogin(sock, &login_attempts, NULL, NULL, NULL, NULL))
						break;
					continue;
				}
				lprintf(LOG_INFO,"%04d %s: <%s>",sock,user.alias,password);
				putuserrec(&scfg,user.number,U_NETMAIL,LEN_NETMAIL,password);
			}
			else if(user.level>=SYSOP_LEVEL && !stricmp(password,sys_pass)) {
				lprintf(LOG_INFO,"%04d Sysop access granted to %s", sock, user.alias);
				sysop=TRUE;
			}
			else if(stricmp(password,user.pass)) {
				if(scfg.sys_misc&SM_ECHO_PW)
					lprintf(LOG_WARNING,"%04d !FAILED Password attempt for user %s: '%s' expected '%s'"
						,sock, user.alias, password, user.pass);
				else
					lprintf(LOG_WARNING,"%04d !FAILED Password attempt for user %s"
						,sock, user.alias);
				user.number=0;
				if(badlogin(sock, &login_attempts, user.alias, password, host_name, &ftp.client_addr))
					break;
				continue;
			}

			/* Update client display */
			if(user.pass[0]) {
				client.user=user.alias;
				loginSuccess(startup->login_attempt_list, &ftp.client_addr);
			} else {	/* anonymous */
				sprintf(str,"%s <%.32s>",user.alias,password);
				client.user=str;
			}
			client_on(sock,&client,TRUE /* update */);

			lprintf(LOG_INFO,"%04d %s logged in (%u today, %u total)"
				,sock,user.alias,user.ltoday+1, user.logons+1);
			logintime=time(NULL);
			timeleft=(long)gettimeleft(&scfg,&user,logintime);
			ftp_printfile(sock,"hello",230);

#ifdef JAVASCRIPT
#ifdef JS_CX_PER_SESSION
			if(js_cx!=NULL) {

				if(js_CreateUserClass(js_cx, js_glob, &scfg)==NULL) 
					lprintf(LOG_ERR,"%04d !JavaScript ERROR creating user class",sock);

				if(js_CreateUserObject(js_cx, js_glob, &scfg, "user", user.number, &client)==NULL) 
					lprintf(LOG_ERR,"%04d !JavaScript ERROR creating user object",sock);

				if(js_CreateClientObject(js_cx, js_glob, "client", &client, sock, -1)==NULL) 
					lprintf(LOG_ERR,"%04d !JavaScript ERROR creating client object",sock);

				if(js_CreateFileAreaObject(js_cx, js_glob, &scfg, &user
					,startup->html_index_file)==NULL) 
					lprintf(LOG_ERR,"%04d !JavaScript ERROR creating file area object",sock);

			}
#endif
#endif

			if(sysop)
				sockprintf(sock,"230-Sysop access granted.");
			sockprintf(sock,"230-%s logged in.",user.alias);
			if(!(user.exempt&FLAG('D')) && (user.cdt+user.freecdt)>0)
				sockprintf(sock,"230-You have %lu download credits."
					,user.cdt+user.freecdt);
			sockprintf(sock,"230 You are allowed %lu minutes of use for this session."
				,timeleft/60);
			sprintf(qwkfile,"%sfile/%04d.qwk",scfg.data_dir,user.number);

			/* Adjust User Total Logons/Logons Today */
			user.logons++;
			user.ltoday++;
			SAFECOPY(user.modem,"FTP");
			SAFECOPY(user.comp,host_name);
			SAFECOPY(user.ipaddr,host_ip);
			user.logontime=(time32_t)logintime;
			putuserdat(&scfg, &user);

			continue;
		}

		if(!user.number) {
			sockprintf(sock,"530 Please login with USER and PASS.");
			continue;
		}

		if(!(user.rest&FLAG('G')))
			getuserdat(&scfg, &user);	/* get current user data */

		if((timeleft=(long)gettimeleft(&scfg,&user,logintime))<1L) {
			sockprintf(sock,"421 Sorry, you've run out of time.");
			lprintf(LOG_WARNING,"%04d Out of time, disconnecting",sock);
			break;
		}

		/********************************/
		/* These commands require login */
		/********************************/

		if(!stricmp(cmd, "REIN")) {
			lprintf(LOG_INFO,"%04d %s reinitialized control session",sock,user.alias);
			user.number=0;
			sysop=FALSE;
			filepos=0;
			sockprintf(sock,"220 Control session re-initialized. Ready for re-login.");
			continue;
		}

		if(!stricmp(cmd, "SITE WHO")) {
			sockprintf(sock,"211-Active Telnet Nodes:");
			for(i=0;i<scfg.sys_nodes && i<scfg.sys_lastnode;i++) {
				if((result=getnodedat(&scfg, i+1, &node, 0))!=0) {
					sockprintf(sock," Error %d getting data for Telnet Node %d",result,i+1);
					continue;
				}
				if(node.status==NODE_INUSE)
					sockprintf(sock," Node %3d: %s",i+1, username(&scfg,node.useron,str));
			}
			sockprintf(sock,"211 End (%d active FTP clients)", protected_uint32_value(active_clients));
			continue;
		}
		if(!stricmp(cmd, "SITE VER")) {
			sockprintf(sock,"211 %s",ftp_ver());
			continue;
		}
		if(!stricmp(cmd, "SITE UPTIME")) {
			sockprintf(sock,"211 %s (%lu served)",sectostr((uint)(time(NULL)-uptime),str),served);
			continue;
		}
		if(!stricmp(cmd, "SITE RECYCLE") && user.level>=SYSOP_LEVEL) {
			startup->recycle_now=TRUE;
			sockprintf(sock,"211 server will recycle when not in-use");
			continue;
		}
		if(!stricmp(cmd, "SITE RECYCLE ALL") && user.level>=SYSOP_LEVEL) {
			refresh_cfg(&scfg);
			sockprintf(sock,"211 ALL servers/nodes will recycle when not in-use");
			continue;
		}
		if(!strnicmp(cmd,"SITE EXEC ",10) && sysop) {
			p=cmd+10;
			SKIP_WHITESPACE(p);
#ifdef __unix__
			fp=popen(p,"r");
			if(fp==NULL)
				sockprintf(sock,"500 Error %d opening pipe to: %s",errno,p);
			else {
				while(!feof(fp)) {
					if(fgets(str,sizeof(str),fp)==NULL)
						break;
					sockprintf(sock,"200-%s",str);
				}
				sockprintf(sock,"200 %s returned %d",p,pclose(fp));
			}
#else
			sockprintf(sock,"200 system(%s) returned %d",p,system(p));
#endif
			continue;
		}


#ifdef SOCKET_DEBUG_CTRL
		if(!stricmp(cmd, "SITE DEBUG")) {
			sockprintf(sock,"211-Debug");
			for(i=0;i<sizeof(socket_debug);i++) 
				if(socket_debug[i]!=0)
					sockprintf(sock,"211-socket %d = 0x%X",i,socket_debug[i]);
			sockprintf(sock,"211 End");
			continue;
		}
#endif

		if(strnicmp(cmd, "PORT ",5)==0 || strnicmp(cmd, "EPRT ",5)==0 || strnicmp(cmd, "LPRT ",5)==0) {

			if(pasv_sock!=INVALID_SOCKET) 
				ftp_close_socket(&pasv_sock,__LINE__);

			p=cmd+5;
			SKIP_WHITESPACE(p);
			if(strnicmp(cmd, "PORT ",5)==0) {
				sscanf(p,"%u,%u,%u,%u,%hd,%hd",&h1,&h2,&h3,&h4,&p1,&p2);
				data_addr.in.sin_family=AF_INET;
				data_addr.in.sin_addr.s_addr=htonl((h1<<24)|(h2<<16)|(h3<<8)|h4);
				data_port = (p1<<8)|p2;
			} else if(strnicmp(cmd, "EPRT ", 5)==0) { /* EPRT */
				char	delim = *p;
				int		prot;
				char	addr_str[INET6_ADDRSTRLEN];

				memset(&data_addr, 0, sizeof(data_addr));
				if(*p)
					p++;
				prot=strtol(p,NULL,/* base: */10);
				switch(prot) {
					case 1:
						FIND_CHAR(p,delim);
						if(*p)
							p++;
						ap = p;
						FIND_CHAR(p,delim);
						old_char = *p;
						*p = 0;
						data_addr.in.sin_addr.s_addr=inet_addr(ap);
						*p = old_char;
						if (*p)
							p++;
						data_port=atoi(p);
						data_addr.in.sin_family=AF_INET;
						break;
					case 2:
						FIND_CHAR(p,delim);
						if(*p)
							p++;
						strncpy(addr_str, p, sizeof(addr_str));
						addr_str[sizeof(addr_str)-1]=0;
						tp=addr_str;
						FIND_CHAR(tp, delim);
						*tp=0;
						if(inet_ptoaddr(addr_str, &data_addr, sizeof(data_addr))==NULL) {
							lprintf(LOG_WARNING,"%04d Unable to parse IPv6 address %s",sock,addr_str);
							sockprintf(sock,"522 Unable to parse IPv6 address (1)");
							continue;
						}
						FIND_CHAR(p,delim);
						if(*p)
							p++;
						data_port=atoi(p);
						data_addr.in6.sin6_family=AF_INET6;
						break;
					default:
						lprintf(LOG_WARNING,"%04d UNSUPPORTED protocol: %d", sock, prot);
						sockprintf(sock,"522 Network protocol not supported, use (1)");
						continue;
				}
			}
			else {	/* LPRT */
				if(sscanf(p,"%u,%u",&h1, &h2)!=2) {
					lprintf(LOG_ERR, "Unable to parse LPRT %s", p);
					sockprintf(sock, "521 Address family not supported");
					continue;
				}
				FIND_CHAR(p,',');
				if(*p)
					p++;
				FIND_CHAR(p,',');
				if(*p)
					p++;
				switch(h1) {
					case 4:	/* IPv4 */
						if(h2 != 4) {
							lprintf(LOG_ERR, "Unable to parse LPRT %s", p);
							sockprintf(sock, "501 IPv4 Address is the wrong length");
							continue;
						}
						for(h1 = 0; h1 < h2; h1++) {
							((unsigned char *)(&data_addr.in.sin_addr))[h1]=atoi(p);
							FIND_CHAR(p,',');
							if(*p)
								p++;
						}
						if(atoi(p)!=2) {
							lprintf(LOG_ERR, "Unable to parse LPRT %s", p);
							sockprintf(sock, "501 IPv4 Port is the wrong length");
							continue;
						}
						FIND_CHAR(p,',');
						if(*p)
							p++;
						for(h1 = 0; h1 < 2; h1++) {
							((unsigned char *)(&data_port))[1-h1]=atoi(p);
							FIND_CHAR(p,',');
							if(*p)
								p++;
						}
						data_addr.in.sin_family=AF_INET;
						break;
					case 6:	/* IPv6 */
						if(h2 != 16) {
							lprintf(LOG_ERR, "Unable to parse LPRT %s", p);
							sockprintf(sock, "501 IPv6 Address is the wrong length");
							continue;
						}
						for(h1 = 0; h1 < h2; h1++) {
							((unsigned char *)(&data_addr.in6.sin6_addr))[h1]=atoi(p);
							FIND_CHAR(p,',');
							if(*p)
								p++;
						}
						if(atoi(p)!=2) {
							lprintf(LOG_ERR, "Unable to parse LPRT %s", p);
							sockprintf(sock, "501 IPv6 Port is the wrong length");
							continue;
						}
						FIND_CHAR(p,',');
						if(*p)
							p++;
						for(h1 = 0; h1 < 2; h1++) {
							((unsigned char *)(&data_port))[1-h1]=atoi(p);
							FIND_CHAR(p,',');
							if(*p)
								p++;
						}
						data_addr.in6.sin6_family=AF_INET6;
						break;
					default:
						lprintf(LOG_ERR, "Unable to parse LPRT %s", p);
						sockprintf(sock, "521 Address family not supported");
						continue;
				}
			}

			inet_addrtop(&data_addr, data_ip, sizeof(data_ip));
			if(data_port< IPPORT_RESERVED) {
				lprintf(LOG_WARNING,"%04d !SUSPECTED BOUNCE ATTACK ATTEMPT by %s to %s port %u"
					,sock,user.alias
					,data_ip,data_port);
				ftp_hacklog("FTP BOUNCE", user.alias, cmd, host_name, &ftp.client_addr);
				sockprintf(sock,"504 Bad port number.");	
				continue; /* As recommended by RFC2577 */
			}
			inet_setaddrport(&data_addr, data_port);
			sockprintf(sock,"200 PORT Command successful.");
			mode="active";
			continue;
		}

		if(stricmp(cmd, "PASV")==0 || stricmp(cmd, "P@SW")==0	/* Kludge required for SMC Barricade V1.2 */
			|| stricmp(cmd, "EPSV")==0 || stricmp(cmd, "LPSV")==0) {	

			if(pasv_sock!=INVALID_SOCKET) 
				ftp_close_socket(&pasv_sock,__LINE__);

			if((pasv_sock=ftp_open_socket(pasv_addr.addr.sa_family, SOCK_STREAM))==INVALID_SOCKET) {
				lprintf(LOG_WARNING,"%04d !PASV ERROR %d opening socket", sock,ERROR_VALUE);
				sockprintf(sock,"425 Error %d opening PASV data socket", ERROR_VALUE);
				continue;
			}

			reuseaddr=FALSE;
			if((result=setsockopt(pasv_sock,SOL_SOCKET,SO_REUSEADDR,(char*)&reuseaddr,sizeof(reuseaddr)))!=0) {
				lprintf(LOG_WARNING,"%04d !PASV ERROR %d disabling REUSEADDR socket option"
					,sock,ERROR_VALUE);
				sockprintf(sock,"425 Error %d disabling REUSEADDR socket option", ERROR_VALUE);
				continue;
			}

			if(startup->options&FTP_OPT_DEBUG_DATA)
				lprintf(LOG_DEBUG,"%04d PASV DATA socket %d opened",sock,pasv_sock);

			for(port=startup->pasv_port_low; port<=startup->pasv_port_high; port++) {

				if(startup->options&FTP_OPT_DEBUG_DATA)
					lprintf(LOG_DEBUG,"%04d PASV DATA trying to bind socket to port %u"
						,sock,port);

				inet_setaddrport(&pasv_addr, port);

				if((result=bind(pasv_sock, &pasv_addr.addr,xp_sockaddr_len(&pasv_addr)))==0)
					break;
				if(port==startup->pasv_port_high)
					break;
			}
			if(result!= 0) {
				lprintf(LOG_ERR,"%04d !PASV ERROR %d (%d) binding socket to port %u"
					,sock, result, ERROR_VALUE, port);
				sockprintf(sock,"425 Error %d binding data socket",ERROR_VALUE);
				ftp_close_socket(&pasv_sock,__LINE__);
				continue;
			}
			if(startup->options&FTP_OPT_DEBUG_DATA)
				lprintf(LOG_DEBUG,"%04d PASV DATA socket %d bound to port %u",sock,pasv_sock,port);

			addr_len=sizeof(addr);
			if((result=getsockname(pasv_sock, &addr.addr,&addr_len))!=0) {
				lprintf(LOG_ERR,"%04d !PASV ERROR %d (%d) getting address/port"
					,sock, result, ERROR_VALUE);
				sockprintf(sock,"425 Error %d getting address/port",ERROR_VALUE);
				ftp_close_socket(&pasv_sock,__LINE__);
				continue;
			} 

			if((result=listen(pasv_sock, 1))!= 0) {
				lprintf(LOG_ERR,"%04d !PASV ERROR %d (%d) listening on port %u"
					,sock, result, ERROR_VALUE,port);
				sockprintf(sock,"425 Error %d listening on data socket",ERROR_VALUE);
				ftp_close_socket(&pasv_sock,__LINE__);
				continue;
			}

			port=inet_addrport(&addr);
			if(stricmp(cmd, "EPSV")==0)
				sockprintf(sock,"229 Entering Extended Passive Mode (|||%hu|)", port);
			else if (stricmp(cmd,"LPSV")==0) {
				switch(addr.addr.sa_family) {
					case AF_INET:
						sockprintf(sock, "228 Entering Long Passive Mode (4, 4, %d, %d, %d, %d, 2, %d, %d)"
							,((unsigned char *)&(addr.in.sin_addr))[0]
							,((unsigned char *)&(addr.in.sin_addr))[1]
							,((unsigned char *)&(addr.in.sin_addr))[2]
							,((unsigned char *)&(addr.in.sin_addr))[3]
							,((unsigned char *)&(addr.in.sin_port))[0]
							,((unsigned char *)&(addr.in.sin_port))[1]);
						break;
					case AF_INET6:
						sockprintf(sock, "228 Entering Long Passive Mode (6, 16, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, 2, %d, %d)"
							,((unsigned char *)&(addr.in6.sin6_addr))[0]
							,((unsigned char *)&(addr.in6.sin6_addr))[1]
							,((unsigned char *)&(addr.in6.sin6_addr))[2]
							,((unsigned char *)&(addr.in6.sin6_addr))[3]
							,((unsigned char *)&(addr.in6.sin6_addr))[4]
							,((unsigned char *)&(addr.in6.sin6_addr))[5]
							,((unsigned char *)&(addr.in6.sin6_addr))[6]
							,((unsigned char *)&(addr.in6.sin6_addr))[7]
							,((unsigned char *)&(addr.in6.sin6_addr))[8]
							,((unsigned char *)&(addr.in6.sin6_addr))[9]
							,((unsigned char *)&(addr.in6.sin6_addr))[10]
							,((unsigned char *)&(addr.in6.sin6_addr))[11]
							,((unsigned char *)&(addr.in6.sin6_addr))[12]
							,((unsigned char *)&(addr.in6.sin6_addr))[13]
							,((unsigned char *)&(addr.in6.sin6_addr))[14]
							,((unsigned char *)&(addr.in6.sin6_addr))[15]
							,((unsigned char *)&(addr.in6.sin6_port))[0]
							,((unsigned char *)&(addr.in6.sin6_port))[1]);
						break;
				}
			}
			else {
				/* Choose IP address to use in passive response */
				ip_addr=0;
				/* TODO: IPv6 this here lookup */
				if(startup->options&FTP_OPT_LOOKUP_PASV_IP
					&& (host=gethostbyname(startup->host_name))!=NULL) 
					ip_addr=ntohl(*((ulong*)host->h_addr_list[0]));
				if(ip_addr==0 && (ip_addr=startup->pasv_ip_addr.s_addr)==0)
					ip_addr=ntohl(pasv_addr.in.sin_addr.s_addr);

				if(startup->options&FTP_OPT_DEBUG_DATA)
					lprintf(LOG_INFO,"%04d PASV DATA IP address in response: %u.%u.%u.%u (subject to NAT)"
						,sock
						,(ip_addr>>24)&0xff
						,(ip_addr>>16)&0xff
						,(ip_addr>>8)&0xff
						,ip_addr&0xff
						);
				sockprintf(sock,"227 Entering Passive Mode (%u,%u,%u,%u,%hu,%hu)"
					,(ip_addr>>24)&0xff
					,(ip_addr>>16)&0xff
					,(ip_addr>>8)&0xff
					,ip_addr&0xff
					,(port>>8)&0xff
					,port&0xff
					);
			}
			mode="passive";
			continue;
		}

		if(!strnicmp(cmd, "TYPE ",5)) {
			sockprintf(sock,"200 All files sent in BINARY mode.");
			continue;
		}

		if(!strnicmp(cmd, "ALLO",4)) {
			p=cmd+5;
			SKIP_WHITESPACE(p);
			if(*p)
				l=atol(p);	
			else
				l=0;
			if(local_fsys)
				avail=getfreediskspace(local_dir,0);
			else
				avail=getfreediskspace(scfg.data_dir,0);	/* Change to temp_dir? */
			if(l && l>avail)
				sockprintf(sock,"504 Only %lu bytes available.",avail);
			else
				sockprintf(sock,"200 %lu bytes available.",avail);
			continue;
		}

		if(!strnicmp(cmd, "REST",4)) {
			p=cmd+4;
			SKIP_WHITESPACE(p);
			if(*p)
				filepos=atol(p);
			else
				filepos=0;
			sockprintf(sock,"350 Restarting at %lu. Send STORE or RETRIEVE to initiate transfer."
				,filepos);
			continue;
		}

		if(!strnicmp(cmd, "MODE ",5)) {
			p=cmd+5;
			SKIP_WHITESPACE(p);
			if(toupper(*p)!='S')
				sockprintf(sock,"504 Only STREAM mode supported.");
			else
				sockprintf(sock,"200 STREAM mode.");
			continue;
		}

		if(!strnicmp(cmd, "STRU ",5)) {
			p=cmd+5;
			SKIP_WHITESPACE(p);
			if(toupper(*p)!='F')
				sockprintf(sock,"504 Only FILE structure supported.");
			else
				sockprintf(sock,"200 FILE structure.");
			continue;
		}

		if(!stricmp(cmd, "SYST")) {
			sockprintf(sock,"215 UNIX Type: L8");
			continue;
		}

		if(!stricmp(cmd, "ABOR")) {
			if(!transfer_inprogress)
				sockprintf(sock,"226 No tranfer in progress.");
			else {
				lprintf(LOG_WARNING,"%04d %s aborting transfer"
					,sock,user.alias);
				transfer_aborted=TRUE;
				YIELD(); /* give send thread time to abort */
				sockprintf(sock,"226 Transfer aborted.");
			}
			continue;
		}

		if(!strnicmp(cmd,"SMNT ",5) && sysop && !(startup->options&FTP_OPT_NO_LOCAL_FSYS)) {
			p=cmd+5;
			SKIP_WHITESPACE(p);
			if(!stricmp(p,BBS_FSYS_DIR)) 
				local_fsys=FALSE;
			else {
				if(!direxist(p)) {
					sockprintf(sock,"550 Directory does not exist.");
					lprintf(LOG_WARNING,"%04d !%s attempted to mount invalid directory: %s"
						,sock, user.alias, p);
					continue;
				}
				local_fsys=TRUE;
				SAFECOPY(local_dir,p);
			}
			sockprintf(sock,"250 %s file system mounted."
				,local_fsys ? "Local" : "BBS");
			lprintf(LOG_INFO,"%04d %s mounted %s file system"
				,sock, user.alias, local_fsys ? "local" : "BBS");
			continue;
		}

		/****************************/
		/* Local File System Access */
		/****************************/
		if(sysop && local_fsys && !(startup->options&FTP_OPT_NO_LOCAL_FSYS)) {
			if(local_dir[0] 
				&& local_dir[strlen(local_dir)-1]!='\\'
				&& local_dir[strlen(local_dir)-1]!='/')
				strcat(local_dir,"/");

			if(!strnicmp(cmd, "LIST", 4) || !strnicmp(cmd, "NLST", 4)) {	
				if((fp=fopen(ftp_tmpfname(fname,"lst",sock),"w+b"))==NULL) {
					lprintf(LOG_ERR,"%04d !ERROR %d opening %s",sock,errno,fname);
					sockprintf(sock, "451 Insufficient system storage");
					continue;
				}
				if(!strnicmp(cmd, "LIST", 4))
					detail=TRUE;
				else
					detail=FALSE;

				p=cmd+4;
				SKIP_WHITESPACE(p);

				if(*p=='-') {	/* -Letc */
					FIND_WHITESPACE(p);
					SKIP_WHITESPACE(p);
				}

				filespec=p;
				if(*filespec==0)
					filespec="*";

				SAFEPRINTF2(path,"%s%s",local_dir, filespec);
				lprintf(LOG_INFO,"%04d %s listing: %s in %s mode", sock, user.alias, path, mode);
				sockprintf(sock, "150 Directory of %s%s", local_dir, filespec);

				now=time(NULL);
				if(localtime_r(&now,&cur_tm)==NULL) 
					memset(&cur_tm,0,sizeof(cur_tm));
			
				glob(path,0,NULL,&g);
				for(i=0;i<(int)g.gl_pathc;i++) {
					if(detail) {
						f.size=flength(g.gl_pathv[i]);
						t=fdate(g.gl_pathv[i]);
						if(localtime_r(&t,&tm)==NULL)
							memset(&tm,0,sizeof(tm));
						fprintf(fp,"%crw-r--r--   1 %-8s local %9"PRId32" %s %2d "
							,isdir(g.gl_pathv[i]) ? 'd':'-'
							,scfg.sys_id
							,f.size
							,ftp_mon[tm.tm_mon],tm.tm_mday);
						if(tm.tm_year==cur_tm.tm_year)
							fprintf(fp,"%02d:%02d %s\r\n"
								,tm.tm_hour,tm.tm_min
								,getfname(g.gl_pathv[i]));
						else
							fprintf(fp,"%5d %s\r\n"
								,1900+tm.tm_year
								,getfname(g.gl_pathv[i]));
					} else
						fprintf(fp,"%s\r\n",getfname(g.gl_pathv[i]));
				}
				globfree(&g);
				fclose(fp);
				filexfer(&data_addr,sock,pasv_sock,&data_sock,fname,0L
					,&transfer_inprogress,&transfer_aborted
					,TRUE	/* delfile */
					,TRUE	/* tmpfile */
					,&lastactive,&user,&client,-1,FALSE,FALSE,FALSE,NULL);
				continue;
			} /* Local LIST/NLST */
				
			if(!strnicmp(cmd, "CWD ", 4) || !strnicmp(cmd,"XCWD ",5)) {
			    if(!strnicmp(cmd,"CWD ",4))
					p=cmd+4;
				else
					p=cmd+5;
				SKIP_WHITESPACE(p);
				tp=p;
				if(*tp=='/' || *tp=='\\') /* /local: and /bbs: are valid */
					tp++;
				if(!strnicmp(tp,BBS_FSYS_DIR,strlen(BBS_FSYS_DIR))) {
					local_fsys=FALSE;
					sockprintf(sock,"250 CWD command successful (BBS file system mounted).");
					lprintf(LOG_INFO,"%04d %s mounted BBS file system", sock, user.alias);
					continue;
				}
				if(!strnicmp(tp,LOCAL_FSYS_DIR,strlen(LOCAL_FSYS_DIR))) {
					tp+=strlen(LOCAL_FSYS_DIR);	/* already mounted */
					p=tp;
				}

				if(p[1]==':' || !strncmp(p,"\\\\",2))
					SAFECOPY(path,p);
				else if(*p=='/' || *p=='\\')
					SAFEPRINTF2(path,"%s%s",root_dir(local_dir),p);
				else {
					SAFEPRINTF2(fname,"%s%s",local_dir,p);
					FULLPATH(path,fname,sizeof(path));
				}

				if(!direxist(path)) {
					sockprintf(sock,"550 Directory does not exist (%s).",path);
					lprintf(LOG_WARNING,"%04d !%s attempted to change to an invalid directory: %s"
						,sock, user.alias, path);
				} else {
					SAFECOPY(local_dir,path);
					sockprintf(sock,"250 CWD command successful (%s).", local_dir);
				}
				continue;
			} /* Local CWD */

			if(!stricmp(cmd,"CDUP") || !stricmp(cmd,"XCUP")) {
				SAFEPRINTF(path,"%s..",local_dir);
				if(FULLPATH(local_dir,path,sizeof(local_dir))==NULL)
					sockprintf(sock,"550 Directory does not exist.");
				else
					sockprintf(sock,"200 CDUP command successful.");
				continue;
			}

			if(!stricmp(cmd, "PWD") || !stricmp(cmd,"XPWD")) {
				if(strlen(local_dir)>3)
					local_dir[strlen(local_dir)-1]=0;	/* truncate '/' */

				sockprintf(sock,"257 \"%s\" is current directory."
					,local_dir);
				continue;
			} /* Local PWD */

			if(!strnicmp(cmd, "MKD ", 4) || !strnicmp(cmd,"XMKD",4)) {
				p=cmd+4;
				SKIP_WHITESPACE(p);
				if(*p=='/')	/* absolute */
					SAFEPRINTF2(fname,"%s%s",root_dir(local_dir),p+1);
				else		/* relative */
					SAFEPRINTF2(fname,"%s%s",local_dir,p);

				if((i=MKDIR(fname))==0) {
					sockprintf(sock,"257 \"%s\" directory created",fname);
					lprintf(LOG_NOTICE,"%04d %s created directory: %s",sock,user.alias,fname);
				} else {
					sockprintf(sock,"521 Error %d creating directory: %s",i,fname);
					lprintf(LOG_WARNING,"%04d !%s attempted to create directory: %s (Error %d)"
						,sock,user.alias,fname,i);
				}
				continue;
			}

			if(!strnicmp(cmd, "RMD ", 4) || !strnicmp(cmd,"XRMD",4)) {
				p=cmd+4;
				SKIP_WHITESPACE(p);
				if(*p=='/')	/* absolute */
					SAFEPRINTF2(fname,"%s%s",root_dir(local_dir),p+1);
				else		/* relative */
					SAFEPRINTF2(fname,"%s%s",local_dir,p);

				if((i=rmdir(fname))==0) {
					sockprintf(sock,"250 \"%s\" directory removed",fname);
					lprintf(LOG_NOTICE,"%04d %s removed directory: %s",sock,user.alias,fname);
				} else {
					sockprintf(sock,"450 Error %d removing directory: %s",i,fname);
					lprintf(LOG_WARNING,"%04d !%s attempted to remove directory: %s (Error %d)"
						,sock,user.alias,fname,i);
				}
				continue;
			}

			if(!strnicmp(cmd, "RNFR ",5)) {
				p=cmd+5;
				SKIP_WHITESPACE(p);
				if(*p=='/')	/* absolute */
					SAFEPRINTF2(ren_from,"%s%s",root_dir(local_dir),p+1);
				else		/* relative */
					SAFEPRINTF2(ren_from,"%s%s",local_dir,p);
				if(!fexist(ren_from)) {
					sockprintf(sock,"550 File not found: %s",ren_from);
					lprintf(LOG_WARNING,"%04d !%s attempted to rename %s (not found)"
						,sock,user.alias,ren_from);
				} else
					sockprintf(sock,"350 File exists, ready for destination name");
				continue;
			}

			if(!strnicmp(cmd, "RNTO ",5)) {
				p=cmd+5;
				SKIP_WHITESPACE(p);
				if(*p=='/')	/* absolute */
					SAFEPRINTF2(fname,"%s%s",root_dir(local_dir),p+1);
				else		/* relative */
					SAFEPRINTF2(fname,"%s%s",local_dir,p);

				if((i=rename(ren_from, fname))==0) {
					sockprintf(sock,"250 \"%s\" renamed to \"%s\"",ren_from,fname);
					lprintf(LOG_NOTICE,"%04d %s renamed %s to %s",sock,user.alias,ren_from,fname);
				} else {
					sockprintf(sock,"450 Error %d renaming file: %s",i,ren_from);
					lprintf(LOG_WARNING,"%04d !%s attempted to rename file: %s (Error %d)"
						,sock,user.alias,ren_from,i);
				}
				continue;
			}


			if(!strnicmp(cmd, "RETR ", 5) || !strnicmp(cmd,"SIZE ",5) 
				|| !strnicmp(cmd, "MDTM ",5) || !strnicmp(cmd, "DELE ",5)) {
				p=cmd+5;
				SKIP_WHITESPACE(p);

				if(!strnicmp(p,LOCAL_FSYS_DIR,strlen(LOCAL_FSYS_DIR))) 
					p+=strlen(LOCAL_FSYS_DIR);	/* already mounted */

				if(p[1]==':')		/* drive specified */
					SAFECOPY(fname,p);
				else if(*p=='/')	/* absolute, current drive */
					SAFEPRINTF2(fname,"%s%s",root_dir(local_dir),p+1);
				else		/* relative */
					SAFEPRINTF2(fname,"%s%s",local_dir,p);
				if(!fexist(fname)) {
					lprintf(LOG_WARNING,"%04d !%s file not found: %s",sock,user.alias,fname);
					sockprintf(sock,"550 File not found: %s",fname);
					continue;
				}
				if(!strnicmp(cmd,"SIZE ",5)) {
					sockprintf(sock,"213 %"PRIuOFF,flength(fname));
					continue;
				}
				if(!strnicmp(cmd,"MDTM ",5)) {
					t=fdate(fname);
					if(gmtime_r(&t,&tm)==NULL) /* specifically use GMT/UTC representation */
						memset(&tm,0,sizeof(tm));
					sockprintf(sock,"213 %u%02u%02u%02u%02u%02u"
						,1900+tm.tm_year,tm.tm_mon+1,tm.tm_mday
						,tm.tm_hour,tm.tm_min,tm.tm_sec);					
					continue;
				}
				if(!strnicmp(cmd,"DELE ",5)) {
					if((i=ftp_remove(sock, __LINE__, fname))==0) {
						sockprintf(sock,"250 \"%s\" removed successfully.",fname);
						lprintf(LOG_NOTICE,"%04d %s deleted file: %s",sock,user.alias,fname);
					} else {
						sockprintf(sock,"450 Error %d removing file: %s",i,fname);
						lprintf(LOG_WARNING,"%04d !%s attempted to delete file: %s (Error %d)"
							,sock,user.alias,fname,i);
					}
					continue;
				}
				/* RETR */
				lprintf(LOG_INFO,"%04d %s downloading: %s (%"PRIuOFF" bytes) in %s mode"
					,sock,user.alias,fname,flength(fname)
					,mode);
				sockprintf(sock,"150 Opening BINARY mode data connection for file transfer.");
				filexfer(&data_addr,sock,pasv_sock,&data_sock,fname,filepos
					,&transfer_inprogress,&transfer_aborted,FALSE,FALSE
					,&lastactive,&user,&client,-1,FALSE,FALSE,FALSE,NULL);
				continue;
			} /* Local RETR/SIZE/MDTM */

			if(!strnicmp(cmd, "STOR ", 5) || !strnicmp(cmd, "APPE ", 5)) {
				p=cmd+5;
				SKIP_WHITESPACE(p);

				if(!strnicmp(p,LOCAL_FSYS_DIR,strlen(LOCAL_FSYS_DIR))) 
					p+=strlen(LOCAL_FSYS_DIR);	/* already mounted */

				if(p[1]==':')		/* drive specified */
					SAFECOPY(fname,p);
				else if(*p=='/')	/* absolute, current drive */
					SAFEPRINTF2(fname,"%s%s",root_dir(local_dir),p+1);
				else				/* relative */
					SAFEPRINTF2(fname,"%s%s",local_dir,p);

				lprintf(LOG_INFO,"%04d %s uploading: %s in %s mode", sock,user.alias,fname
					,mode);
				sockprintf(sock,"150 Opening BINARY mode data connection for file transfer.");
				filexfer(&data_addr,sock,pasv_sock,&data_sock,fname,filepos
					,&transfer_inprogress,&transfer_aborted,FALSE,FALSE
					,&lastactive
					,&user
					,&client
					,-1		/* dir */
					,TRUE	/* uploading */
					,FALSE	/* credits */
					,!strnicmp(cmd,"APPE",4) ? TRUE : FALSE	/* append */
					,NULL	/* desc */
					);
				filepos=0;
				continue;
			} /* Local STOR */
		}

		if(!strnicmp(cmd, "LIST", 4) || !strnicmp(cmd, "NLST", 4)) {	
			dir=curdir;
			lib=curlib;

			if(cmd[4]!=0) 
				lprintf(LOG_DEBUG,"%04d LIST/NLST: %s",sock,cmd);

			/* path specified? */
			p=cmd+4;
			SKIP_WHITESPACE(p);

			if(*p=='-') {	/* -Letc */
				FIND_WHITESPACE(p);
				SKIP_WHITESPACE(p);
			}

			parsepath(&p,&user,&client,&lib,&dir);
			filespec=p;
			if(*filespec==0)
				filespec="*";

			if((fp=fopen(ftp_tmpfname(fname,"lst",sock),"w+b"))==NULL) {
				lprintf(LOG_ERR,"%04d !ERROR %d opening %s",sock,errno,fname);
				sockprintf(sock, "451 Insufficient system storage");
				continue;
			}
			if(!strnicmp(cmd, "LIST", 4))
				detail=TRUE;
			else
				detail=FALSE;
			sockprintf(sock,"150 Opening ASCII mode data connection for /bin/ls.");
			now=time(NULL);
			if(localtime_r(&now,&cur_tm)==NULL) 
				memset(&cur_tm,0,sizeof(cur_tm));

			/* ASCII Index File */
			if(startup->options&FTP_OPT_INDEX_FILE && startup->index_file_name[0]
				&& wildmatchi(startup->index_file_name, filespec, FALSE)) {
				if(detail)
					fprintf(fp,"-r--r--r--   1 %-*s %-8s %9ld %s %2d %02d:%02d %s\r\n"
						,NAME_LEN
						,scfg.sys_id
						,lib<0 ? scfg.sys_id : dir<0 
							? scfg.lib[lib]->sname : scfg.dir[dir]->code_suffix
						,512L
						,ftp_mon[cur_tm.tm_mon],cur_tm.tm_mday,cur_tm.tm_hour,cur_tm.tm_min
						,startup->index_file_name);
				else
					fprintf(fp,"%s\r\n",startup->index_file_name);
			} 
			/* HTML Index File */
			if(startup->options&FTP_OPT_HTML_INDEX_FILE && startup->html_index_file[0]
				&& wildmatchi(startup->html_index_file, filespec, FALSE)) {
				if(detail)
					fprintf(fp,"-r--r--r--   1 %-*s %-8s %9ld %s %2d %02d:%02d %s\r\n"
						,NAME_LEN
						,scfg.sys_id
						,lib<0 ? scfg.sys_id : dir<0 
							? scfg.lib[lib]->sname : scfg.dir[dir]->code_suffix
						,512L
						,ftp_mon[cur_tm.tm_mon],cur_tm.tm_mday,cur_tm.tm_hour,cur_tm.tm_min
						,startup->html_index_file);
				else
					fprintf(fp,"%s\r\n",startup->html_index_file);
			} 

			if(lib<0) { /* Root dir */
				lprintf(LOG_INFO,"%04d %s listing: root in %s mode",sock,user.alias, mode);

				/* QWK Packet */
				if(startup->options&FTP_OPT_ALLOW_QWK) {
					SAFEPRINTF(str,"%s.qwk",scfg.sys_id);
					if(wildmatchi(str, filespec, FALSE)) {
						if(detail) {
							if(fexistcase(qwkfile)) {
								t=fdate(qwkfile);
								l=flength(qwkfile);
							} else {
								t=time(NULL);
								l=10240;
							};
							if(localtime_r(&t,&tm)==NULL) 
								memset(&tm,0,sizeof(tm));
							fprintf(fp,"-r--r--r--   1 %-*s %-8s %9ld %s %2d %02d:%02d %s\r\n"
								,NAME_LEN
								,scfg.sys_id
								,scfg.sys_id
								,l
								,ftp_mon[tm.tm_mon],tm.tm_mday,tm.tm_hour,tm.tm_min
								,str);
						} else
							fprintf(fp,"%s\r\n",str);
					}
				} 

				/* File Aliases */
				sprintf(aliasfile,"%sftpalias.cfg",scfg.ctrl_dir);
				if((alias_fp=fopen(aliasfile,"r"))!=NULL) {

					while(!feof(alias_fp)) {
						if(!fgets(aliasline,sizeof(aliasline),alias_fp))
							break;

						alias_dir=FALSE;

						p=aliasline;		/* alias pointer */
						SKIP_WHITESPACE(p);

						if(*p==';')	/* comment */
							continue;

						tp=p;		/* terminator pointer */
						FIND_WHITESPACE(tp);
						if(*tp) *tp=0;

						np=tp+1;	/* filename pointer */
						SKIP_WHITESPACE(np);

						tp=np;		/* terminator pointer */
						FIND_WHITESPACE(tp);
						if(*tp) *tp=0;

						dp=tp+1;	/* description pointer */
						SKIP_WHITESPACE(dp);
						truncsp(dp);

						if(stricmp(dp,BBS_HIDDEN_ALIAS)==0)
							continue;

						if(!wildmatchi(p, filespec, FALSE))
							continue;

						/* Virtual Path? */
						if(!strnicmp(np,BBS_VIRTUAL_PATH,strlen(BBS_VIRTUAL_PATH))) {
							if((dir=getdir(np+strlen(BBS_VIRTUAL_PATH),&user,&client))<0) {
								lprintf(LOG_WARNING,"0000 !Invalid virtual path (%s) for %s",np,user.alias);
								continue; /* No access or invalid virtual path */
							}
							tp=strrchr(np,'/');
							if(tp==NULL) 
								continue;
							tp++;
							if(*tp) {
								SAFEPRINTF2(aliasfile,"%s%s",scfg.dir[dir]->path,tp);
								np=aliasfile;
							}
							else 
								alias_dir=TRUE;
						}

						if(!alias_dir && !fexist(np)) {
							lprintf(LOG_WARNING,"0000 !Missing aliased file (%s) for %s",np,user.alias);
							continue;
						}

						if(detail) {

							if(alias_dir==TRUE) {
								fprintf(fp,"drwxrwxrwx   1 %-*s %-8s %9ld %s %2d %02d:%02d %s\r\n"
									,NAME_LEN
									,scfg.sys_id
									,scfg.lib[scfg.dir[dir]->lib]->sname
									,512L
									,ftp_mon[cur_tm.tm_mon],cur_tm.tm_mday,cur_tm.tm_hour,cur_tm.tm_min
									,p);
							}
							else {
								t=fdate(np);
								if(localtime_r(&t,&tm)==NULL)
									memset(&tm,0,sizeof(tm));
								fprintf(fp,"-r--r--r--   1 %-*s %-8s %9"PRIdOFF" %s %2d %02d:%02d %s\r\n"
									,NAME_LEN
									,scfg.sys_id
									,scfg.sys_id
									,flength(np)
									,ftp_mon[tm.tm_mon],tm.tm_mday,tm.tm_hour,tm.tm_min
									,p);
							}
						} else
							fprintf(fp,"%s\r\n",p);

					}

					fclose(alias_fp);
				}

				/* Library folders */
				for(i=0;i<scfg.total_libs;i++) {
					if(!chk_ar(&scfg,scfg.lib[i]->ar,&user,&client))
						continue;
					if(!wildmatchi(scfg.lib[i]->sname, filespec, FALSE))
						continue;
					if(detail)
						fprintf(fp,"dr-xr-xr-x   1 %-*s %-8s %9ld %s %2d %02d:%02d %s\r\n"
							,NAME_LEN
							,scfg.sys_id
							,scfg.sys_id
							,512L
							,ftp_mon[cur_tm.tm_mon],cur_tm.tm_mday,cur_tm.tm_hour,cur_tm.tm_min
							,scfg.lib[i]->sname);
					else
						fprintf(fp,"%s\r\n",scfg.lib[i]->sname);
				}
			} else if(dir<0) {
				lprintf(LOG_INFO,"%04d %s listing: %s library in %s mode"
					,sock,user.alias,scfg.lib[lib]->sname,mode);
				for(i=0;i<scfg.total_dirs;i++) {
					if(scfg.dir[i]->lib!=lib)
						continue;
					if(i!=(int)scfg.sysop_dir && i!=(int)scfg.upload_dir 
						&& !chk_ar(&scfg,scfg.dir[i]->ar,&user,&client))
						continue;
					if(!wildmatchi(scfg.dir[i]->code_suffix, filespec, FALSE))
						continue;
					if(detail)
						fprintf(fp,"drwxrwxrwx   1 %-*s %-8s %9ld %s %2d %02d:%02d %s\r\n"
							,NAME_LEN
							,scfg.sys_id
							,scfg.lib[lib]->sname
							,512L
							,ftp_mon[cur_tm.tm_mon],cur_tm.tm_mday,cur_tm.tm_hour,cur_tm.tm_min
							,scfg.dir[i]->code_suffix);
					else
						fprintf(fp,"%s\r\n",scfg.dir[i]->code_suffix);
				}
			} else if(chk_ar(&scfg,scfg.dir[dir]->ar,&user,&client)) {
				lprintf(LOG_INFO,"%04d %s listing: %s/%s directory in %s mode"
					,sock,user.alias,scfg.lib[lib]->sname,scfg.dir[dir]->code_suffix,mode);

				SAFEPRINTF2(path,"%s%s",scfg.dir[dir]->path,filespec);
				glob(path,0,NULL,&g);
				for(i=0;i<(int)g.gl_pathc;i++) {
					if(isdir(g.gl_pathv[i]))
						continue;
#ifdef _WIN32
					GetShortPathName(g.gl_pathv[i], str, sizeof(str));
#else
					SAFECOPY(str,g.gl_pathv[i]);
#endif
					padfname(getfname(str),f.name);
					f.dir=dir;
					if((filedat=getfileixb(&scfg,&f))==FALSE
						&& !(startup->options&FTP_OPT_DIR_FILES))
						continue;
					if(detail) {
						f.size=flength(g.gl_pathv[i]);
						getfiledat(&scfg,&f);
						t=fdate(g.gl_pathv[i]);
						if(localtime_r(&t,&tm)==NULL)
							memset(&tm,0,sizeof(tm));
						if(filedat) {
							if(f.misc&FM_ANON)
								SAFECOPY(str,ANONYMOUS);
							else
								dotname(f.uler,str);
						} else
							SAFECOPY(str,scfg.sys_id);
						fprintf(fp,"-r--r--r--   1 %-*s %-8s %9"PRId32" %s %2d "
							,NAME_LEN
							,str
							,scfg.dir[dir]->code_suffix
							,f.size
							,ftp_mon[tm.tm_mon],tm.tm_mday);
						if(tm.tm_year==cur_tm.tm_year)
							fprintf(fp,"%02d:%02d %s\r\n"
								,tm.tm_hour,tm.tm_min
								,getfname(g.gl_pathv[i]));
						else
							fprintf(fp,"%5d %s\r\n"
								,1900+tm.tm_year
								,getfname(g.gl_pathv[i]));
					} else
						fprintf(fp,"%s\r\n",getfname(g.gl_pathv[i]));
				}
				globfree(&g);
			} else 
				lprintf(LOG_INFO,"%04d %s listing: %s/%s directory in %s mode (empty - no access)"
					,sock,user.alias,scfg.lib[lib]->sname,scfg.dir[dir]->code_suffix,mode);

			fclose(fp);
			filexfer(&data_addr,sock,pasv_sock,&data_sock,fname,0L
				,&transfer_inprogress,&transfer_aborted
				,TRUE /* delfile */
				,TRUE /* tmpfile */
				,&lastactive,&user,&client,dir,FALSE,FALSE,FALSE,NULL);
			continue;
		}

		if(!strnicmp(cmd, "RETR ", 5) 
			|| !strnicmp(cmd, "SIZE ",5) 
			|| !strnicmp(cmd, "MDTM ",5)
			|| !strnicmp(cmd, "DELE ",5)) {
			getdate=FALSE;
			getsize=FALSE;
			delecmd=FALSE;
			file_date=0;
			if(!strnicmp(cmd,"SIZE ",5))
				getsize=TRUE;
			else if(!strnicmp(cmd,"MDTM ",5))
				getdate=TRUE;
			else if(!strnicmp(cmd,"DELE ",5))
				delecmd=TRUE;

			if(!getsize && !getdate && user.rest&FLAG('D')) {
				sockprintf(sock,"550 Insufficient access.");
				filepos=0;
				continue;
			}
			credits=TRUE;
			success=FALSE;
			delfile=FALSE;
			tmpfile=FALSE;
			lib=curlib;
			dir=curdir;

			p=cmd+5;
			SKIP_WHITESPACE(p);

			if(!strnicmp(p,BBS_FSYS_DIR,strlen(BBS_FSYS_DIR))) 
				p+=strlen(BBS_FSYS_DIR);	/* already mounted */

			if(*p=='/') {
				lib=-1;
				p++;
			}
			else if(!strncmp(p,"./",2))
				p+=2;

			if(lib<0 && ftpalias(p, fname, &user, &client, &dir)==TRUE) {
				success=TRUE;
				credits=TRUE;	/* include in d/l stats */
				tmpfile=FALSE;
				delfile=FALSE;
				lprintf(LOG_INFO,"%04d %s %.4s by alias: %s"
					,sock,user.alias,cmd,p);
				p=getfname(fname);
				if(dir>=0)
					lib=scfg.dir[dir]->lib;
			}
			if(!success && lib<0 && (tp=strchr(p,'/'))!=NULL) {
				dir=-1;
				*tp=0;
				for(i=0;i<scfg.total_libs;i++) {
					if(!chk_ar(&scfg,scfg.lib[i]->ar,&user,&client))
						continue;
					if(!stricmp(scfg.lib[i]->sname,p))
						break;
				}
				if(i<scfg.total_libs) 
					lib=i;
				p=tp+1;
			}
			if(!success && dir<0 && (tp=strchr(p,'/'))!=NULL) {
				*tp=0;
				for(i=0;i<scfg.total_dirs;i++) {
					if(scfg.dir[i]->lib!=lib)
						continue;
					if(!chk_ar(&scfg,scfg.dir[i]->ar,&user,&client))
						continue;
					if(!stricmp(scfg.dir[i]->code_suffix,p))
						break;
				}
				if(i<scfg.total_dirs) 
					dir=i;
				p=tp+1;
			}

			sprintf(html_index_ext,"%s?",startup->html_index_file);

			sprintf(str,"%s.qwk",scfg.sys_id);
			if(lib<0 && startup->options&FTP_OPT_ALLOW_QWK 
				&& !stricmp(p,str) && !delecmd) {
				if(!fexistcase(qwkfile)) {
					lprintf(LOG_INFO,"%04d %s creating QWK packet...",sock,user.alias);
					sprintf(str,"%spack%04u.now",scfg.data_dir,user.number);
					if(!ftouch(str))
						lprintf(LOG_ERR,"%04d !ERROR creating semaphore file: %s"
							,sock, str);
					t=time(NULL);
					while(fexist(str)) {
						if(!socket_check(sock,NULL,NULL,0))
							break;
						if(time(NULL)-t>startup->qwk_timeout)
							break;
						mswait(1000);
					}
					if(!socket_check(sock,NULL,NULL,0)) {
						ftp_remove(sock, __LINE__, str);
						continue;
					}
					if(fexist(str)) {
						lprintf(LOG_WARNING,"%04d !TIMEOUT waiting for QWK packet creation",sock);
						sockprintf(sock,"451 Time-out waiting for packet creation.");
						ftp_remove(sock, __LINE__, str);
						filepos=0;
						continue;
					}
					if(!fexistcase(qwkfile)) {
						lprintf(LOG_INFO,"%04d No QWK Packet created (no new messages)",sock);
						sockprintf(sock,"550 No QWK packet created (no new messages)");
						filepos=0;
						continue;
					}
				}
				SAFECOPY(fname,qwkfile);
				success=TRUE;
				delfile=TRUE;
				credits=FALSE;
				if(!getsize && !getdate)
					lprintf(LOG_INFO,"%04d %s downloading QWK packet (%"PRIuOFF" bytes) in %s mode"
						,sock,user.alias,flength(fname)
						,mode);
			/* ASCII Index File */
			} else if(startup->options&FTP_OPT_INDEX_FILE 
				&& !stricmp(p,startup->index_file_name)
				&& !delecmd) {
				if(getsize) {
					sockprintf(sock, "550 Size not available for dynamically generated files");
					continue;
				}
				if((fp=fopen(ftp_tmpfname(fname,"ndx",sock),"w+b"))==NULL) {
					lprintf(LOG_ERR,"%04d !ERROR %d opening %s",sock,errno,fname);
					sockprintf(sock, "451 Insufficient system storage");
					filepos=0;
					continue;
				}
				success=TRUE;
				if(getdate)
					file_date=time(NULL);
				else {
					lprintf(LOG_INFO,"%04d %s downloading index for %s in %s mode"
						,sock,user.alias,genvpath(lib,dir,str)
						,mode);
					credits=FALSE;
					tmpfile=TRUE;
					delfile=TRUE;
					fprintf(fp,"%-*s File/Folder Descriptions\r\n"
						,INDEX_FNAME_LEN,startup->index_file_name);
					if(startup->options&FTP_OPT_HTML_INDEX_FILE)
						fprintf(fp,"%-*s File/Folder Descriptions (HTML)\r\n"
							,INDEX_FNAME_LEN,startup->html_index_file);
					if(lib<0) {

						/* File Aliases */
						sprintf(aliasfile,"%sftpalias.cfg",scfg.ctrl_dir);
						if((alias_fp=fopen(aliasfile,"r"))!=NULL) {

							while(!feof(alias_fp)) {
								if(!fgets(aliasline,sizeof(aliasline),alias_fp))
									break;

								p=aliasline;	/* alias pointer */
								SKIP_WHITESPACE(p);

								if(*p==';')	/* comment */
									continue;

								tp=p;		/* terminator pointer */
								FIND_WHITESPACE(tp);
								if(*tp) *tp=0;

								np=tp+1;	/* filename pointer */
								SKIP_WHITESPACE(np);

								np++;		/* description pointer */
								FIND_WHITESPACE(np);

								while(*np && *np<' ') np++;

								truncsp(np);

								fprintf(fp,"%-*s %s\r\n",INDEX_FNAME_LEN,p,np);
							}

							fclose(alias_fp);
						}

						/* QWK Packet */
						if(startup->options&FTP_OPT_ALLOW_QWK /* && fexist(qwkfile) */) {
							sprintf(str,"%s.qwk",scfg.sys_id);
							fprintf(fp,"%-*s QWK Message Packet\r\n"
								,INDEX_FNAME_LEN,str);
						}

						/* Library Folders */
						for(i=0;i<scfg.total_libs;i++) {
							if(!chk_ar(&scfg,scfg.lib[i]->ar,&user,&client))
								continue;
							fprintf(fp,"%-*s %s\r\n"
								,INDEX_FNAME_LEN,scfg.lib[i]->sname,scfg.lib[i]->lname);
						}
					} else if(dir<0) {
						for(i=0;i<scfg.total_dirs;i++) {
							if(scfg.dir[i]->lib!=lib)
								continue;
							if(i!=(int)scfg.sysop_dir && i!=(int)scfg.upload_dir
								&& !chk_ar(&scfg,scfg.dir[i]->ar,&user,&client))
								continue;
							fprintf(fp,"%-*s %s\r\n"
								,INDEX_FNAME_LEN,scfg.dir[i]->code_suffix,scfg.dir[i]->lname);
						}
					} else if(chk_ar(&scfg,scfg.dir[dir]->ar,&user,&client)){
						sprintf(cmd,"%s*",scfg.dir[dir]->path);
						glob(cmd,0,NULL,&g);
						for(i=0;i<(int)g.gl_pathc;i++) {
							if(isdir(g.gl_pathv[i]))
								continue;
	#ifdef _WIN32
							GetShortPathName(g.gl_pathv[i], str, sizeof(str));
	#else
							SAFECOPY(str,g.gl_pathv[i]);
	#endif
							padfname(getfname(str),f.name);
							f.dir=dir;
							if(getfileixb(&scfg,&f)) {
								f.size=flength(g.gl_pathv[i]);
								getfiledat(&scfg,&f);
								fprintf(fp,"%-*s %s\r\n",INDEX_FNAME_LEN
									,getfname(g.gl_pathv[i]),f.desc);
							}
						}
						globfree(&g);
					}
					fclose(fp);
				}
			/* HTML Index File */
			} else if(startup->options&FTP_OPT_HTML_INDEX_FILE 
				&& (!stricmp(p,startup->html_index_file) 
				|| !strnicmp(p,html_index_ext,strlen(html_index_ext)))
				&& !delecmd) {
				success=TRUE;
				if(getsize) {
					sockprintf(sock, "550 Size not available for dynamically generated files");
					continue;
				}
				else if(getdate)
					file_date=time(NULL);
				else {
#ifdef JAVASCRIPT
					if(startup->options&FTP_OPT_NO_JAVASCRIPT) {
						lprintf(LOG_ERR,"%04d !JavaScript disabled, cannot generate %s",sock,fname);
						sockprintf(sock, "451 JavaScript disabled");
						filepos=0;
						continue;
					}
					if(js_runtime == NULL) {
						lprintf(LOG_DEBUG,"%04d JavaScript: Creating runtime: %lu bytes"
							,sock,startup->js.max_bytes);

						if((js_runtime = jsrt_GetNew(startup->js.max_bytes, 1000, __FILE__, __LINE__))==NULL) {
							lprintf(LOG_ERR,"%04d !ERROR creating JavaScript runtime",sock);
							sockprintf(sock,"451 Error creating JavaScript runtime");
							filepos=0;
							continue;
						}
					}

					if(js_cx==NULL) {	/* Context not yet created, create it now */
						/* js_initcx() starts a request */
						if(((js_cx=js_initcx(js_runtime, sock,&js_glob,&js_ftp,&js_callback))==NULL)) {
							lprintf(LOG_ERR,"%04d !ERROR initializing JavaScript context",sock);
							sockprintf(sock,"451 Error initializing JavaScript context");
							filepos=0;
							continue;
						}
						if(js_CreateUserClass(js_cx, js_glob, &scfg)==NULL) 
							lprintf(LOG_ERR,"%04d !JavaScript ERROR creating user class",sock);

						if(js_CreateFileClass(js_cx, js_glob)==NULL) 
							lprintf(LOG_ERR,"%04d !JavaScript ERROR creating file class",sock);

						if(js_CreateUserObject(js_cx, js_glob, &scfg, "user", &user, &client, /* global_user: */TRUE)==NULL) 
							lprintf(LOG_ERR,"%04d !JavaScript ERROR creating user object",sock);

						if(js_CreateClientObject(js_cx, js_glob, "client", &client, sock, -1)==NULL) 
							lprintf(LOG_ERR,"%04d !JavaScript ERROR creating client object",sock);

						if(js_CreateFileAreaObject(js_cx, js_glob, &scfg, &user, &client
							,startup->html_index_file)==NULL) 
							lprintf(LOG_ERR,"%04d !JavaScript ERROR creating file area object",sock);
					}
					else
						JS_BEGINREQUEST(js_cx);

					if((js_str=JS_NewStringCopyZ(js_cx, "name"))!=NULL) {
						js_val=STRING_TO_JSVAL(js_str);
						JS_SetProperty(js_cx, js_ftp, "sort", &js_val);
					}
					js_val=BOOLEAN_TO_JSVAL(FALSE);
					JS_SetProperty(js_cx, js_ftp, "reverse", &js_val);

					if(!strnicmp(p,html_index_ext,strlen(html_index_ext))) {
						p+=strlen(html_index_ext);
						tp=strrchr(p,'$');
						if(tp!=NULL)
							*tp=0;
						if(!strnicmp(p,"ext=",4)) {
							p+=4;
							if(!strcmp(p,"on"))
								user.misc|=EXTDESC;
							else
								user.misc&=~EXTDESC;
							if(!(user.rest&FLAG('G')))
								putuserrec(&scfg,user.number,U_MISC,8,ultoa(user.misc,str,16));
						} 
						else if(!strnicmp(p,"sort=",5)) {
							p+=5;
							tp=strchr(p,'&');
							if(tp!=NULL) {
								*tp=0;
								tp++;
								if(!stricmp(tp,"reverse")) {
									js_val=BOOLEAN_TO_JSVAL(TRUE);
									JS_SetProperty(js_cx, js_ftp, "reverse", &js_val);
								}
							}
							if((js_str=JS_NewStringCopyZ(js_cx, p))!=NULL) {
								js_val=STRING_TO_JSVAL(js_str);
								JS_SetProperty(js_cx, js_ftp, "sort", &js_val);
							}
						}
					}

					js_val=BOOLEAN_TO_JSVAL(INT_TO_BOOL(user.misc&EXTDESC));
					JS_SetProperty(js_cx, js_ftp, "extended_descriptions", &js_val);

					JS_ENDREQUEST(js_cx);
#endif
					if((fp=fopen(ftp_tmpfname(fname,"html",sock),"w+b"))==NULL) {
						lprintf(LOG_ERR,"%04d !ERROR %d opening %s",sock,errno,fname);
						sockprintf(sock, "451 Insufficient system storage");
						filepos=0;
						continue;
					}
					lprintf(LOG_INFO,"%04d %s downloading HTML index for %s in %s mode"
						,sock,user.alias,genvpath(lib,dir,str)
						,mode);
					credits=FALSE;
					tmpfile=TRUE;
					delfile=TRUE;
#ifdef JAVASCRIPT
					JS_BEGINREQUEST(js_cx);
					js_val=INT_TO_JSVAL(timeleft);
					if(!JS_SetProperty(js_cx, js_ftp, "time_left", &js_val))
						lprintf(LOG_ERR,"%04d !JavaScript ERROR setting user.time_left",sock);
					js_generate_index(js_cx, js_ftp, sock, fp, lib, dir, &user, &client);
					JS_ENDREQUEST(js_cx);
#endif
					fclose(fp);
				}
			} else if(dir>=0) {

				if(!chk_ar(&scfg,scfg.dir[dir]->ar,&user,&client)) {
					lprintf(LOG_WARNING,"%04d !%s has insufficient access to /%s/%s"
						,sock,user.alias
						,scfg.lib[scfg.dir[dir]->lib]->sname
						,scfg.dir[dir]->code_suffix);
					sockprintf(sock,"550 Insufficient access.");
					filepos=0;
					continue;
				}

				if(!getsize && !getdate && !delecmd
					&& !chk_ar(&scfg,scfg.dir[dir]->dl_ar,&user,&client)) {
					lprintf(LOG_WARNING,"%04d !%s has insufficient access to download from /%s/%s"
						,sock,user.alias
						,scfg.lib[scfg.dir[dir]->lib]->sname
						,scfg.dir[dir]->code_suffix);
					sockprintf(sock,"550 Insufficient access.");
					filepos=0;
					continue;
				}

				if(delecmd && !dir_op(&scfg,&user,&client,dir) && !(user.exempt&FLAG('R'))) {
					lprintf(LOG_WARNING,"%04d !%s has insufficient access to delete files in /%s/%s"
						,sock,user.alias
						,scfg.lib[scfg.dir[dir]->lib]->sname
						,scfg.dir[dir]->code_suffix);
					sockprintf(sock,"550 Insufficient access.");
					filepos=0;
					continue;
				}
				SAFEPRINTF2(fname,"%s%s",scfg.dir[dir]->path,p);
#ifdef _WIN32
				GetShortPathName(fname, str, sizeof(str));
#else
				SAFECOPY(str,fname);
#endif
				padfname(getfname(str),f.name);
				f.dir=dir;
				f.cdt=0;
				f.size=-1;
				filedat=getfileixb(&scfg,&f);
				if(!filedat && !(startup->options&FTP_OPT_DIR_FILES)) {
					sockprintf(sock,"550 File not found: %s",p);
					lprintf(LOG_WARNING,"%04d !%s file (%s%s) not in database for %.4s command"
						,sock,user.alias,genvpath(lib,dir,str),p,cmd);
					filepos=0;
					continue;
				}

				/* Verify credits */
				if(!getsize && !getdate && !delecmd
					&& !is_download_free(&scfg,dir,&user,&client)) {
					if(filedat)
						getfiledat(&scfg,&f);
					else
						f.cdt=flength(fname);
					if(f.cdt>(user.cdt+user.freecdt)) {
						lprintf(LOG_WARNING,"%04d !%s has insufficient credit to download /%s/%s/%s (%lu credits)"
							,sock,user.alias,scfg.lib[scfg.dir[dir]->lib]->sname
							,scfg.dir[dir]->code_suffix
							,p
							,f.cdt);
						sockprintf(sock,"550 Insufficient credit (%lu required).",f.cdt);
						filepos=0;
						continue;
					}
				}

				if(strcspn(p,ILLEGAL_FILENAME_CHARS)!=strlen(p)) {
					success=FALSE;
					lprintf(LOG_WARNING,"%04d !ILLEGAL FILENAME ATTEMPT by %s: %s"
						,sock,user.alias,p);
					ftp_hacklog("FTP FILENAME", user.alias, cmd, host_name, &ftp.client_addr);
				} else {
					if(fexistcase(fname)) {
						success=TRUE;
						if(!getsize && !getdate && !delecmd)
							lprintf(LOG_INFO,"%04d %s downloading: %s (%"PRIuOFF" bytes) in %s mode"
								,sock,user.alias,fname,flength(fname)
								,mode);
					} 
				}
			}
#if defined(SOCKET_DEBUG_DOWNLOAD)
			socket_debug[sock]|=SOCKET_DEBUG_DOWNLOAD;
#endif

			if(getsize && success)
				sockprintf(sock,"213 %"PRIuOFF, flength(fname));
			else if(getdate && success) {
				if(file_date==0)
					file_date = fdate(fname);
				if(gmtime_r(&file_date,&tm)==NULL)	/* specifically use GMT/UTC representation */
					memset(&tm,0,sizeof(tm));
				sockprintf(sock,"213 %u%02u%02u%02u%02u%02u"
					,1900+tm.tm_year,tm.tm_mon+1,tm.tm_mday
					,tm.tm_hour,tm.tm_min,tm.tm_sec);
			} else if(delecmd && success) {
				if(removecase(fname)!=0) {
					lprintf(LOG_ERR,"%04d !ERROR %d deleting %s",sock,errno,fname);
					sockprintf(sock,"450 %s could not be deleted (error: %d)"
						,fname,errno);
				} else {
					lprintf(LOG_NOTICE,"%04d %s deleted %s",sock,user.alias,fname);
					if(filedat) 
						removefiledat(&scfg,&f);
					sockprintf(sock,"250 %s deleted.",fname);
				}
			} else if(success) {
				sockprintf(sock,"150 Opening BINARY mode data connection for file transfer.");
				filexfer(&data_addr,sock,pasv_sock,&data_sock,fname,filepos
					,&transfer_inprogress,&transfer_aborted,delfile,tmpfile
					,&lastactive,&user,&client,dir,FALSE,credits,FALSE,NULL);
			}
			else {
				sockprintf(sock,"550 File not found: %s",p);
				lprintf(LOG_WARNING,"%04d !%s file (%s%s) not found for %.4s command"
					,sock,user.alias,genvpath(lib,dir,str),p,cmd);
			}
			filepos=0;
#if defined(SOCKET_DEBUG_DOWNLOAD)
			socket_debug[sock]&=~SOCKET_DEBUG_DOWNLOAD;
#endif
			continue;
		}

		if(!strnicmp(cmd, "DESC", 4)) {

			if(user.rest&FLAG('U')) {
				sockprintf(sock,"553 Insufficient access.");
				continue;
			}

			p=cmd+4;
			SKIP_WHITESPACE(p);

			if(*p==0) 
				sockprintf(sock,"501 No file description given.");
			else {
				SAFECOPY(desc,p);
				sockprintf(sock,"200 File description set. Ready to STOR file.");
			}
			continue;
		}

		if(!strnicmp(cmd, "STOR ", 5) || !strnicmp(cmd, "APPE ", 5)) {

			if(user.rest&FLAG('U')) {
				sockprintf(sock,"553 Insufficient access.");
				continue;
			}

			if(transfer_inprogress==TRUE) {
				lprintf(LOG_WARNING,"%04d !TRANSFER already in progress (%s)",sock,cmd);
				sockprintf(sock,"425 Transfer already in progress.");
				continue;
			}

			append=FALSE;
			lib=curlib;
			dir=curdir;
			p=cmd+5;

			SKIP_WHITESPACE(p);

			if(!strnicmp(p,BBS_FSYS_DIR,strlen(BBS_FSYS_DIR))) 
				p+=strlen(BBS_FSYS_DIR);	/* already mounted */

			if(*p=='/') {
				lib=-1;
				p++;
			}
			else if(!strncmp(p,"./",2))
				p+=2;
			/* Need to add support for uploading to aliased directories */
			if(lib<0 && (tp=strchr(p,'/'))!=NULL) {
				dir=-1;
				*tp=0;
				for(i=0;i<scfg.total_libs;i++) {
					if(!chk_ar(&scfg,scfg.lib[i]->ar,&user,&client))
						continue;
					if(!stricmp(scfg.lib[i]->sname,p))
						break;
				}
				if(i<scfg.total_libs) 
					lib=i;
				p=tp+1;
			}
			if(dir<0 && (tp=strchr(p,'/'))!=NULL) {
				*tp=0;
				for(i=0;i<scfg.total_dirs;i++) {
					if(scfg.dir[i]->lib!=lib)
						continue;
					if(i!=(int)scfg.sysop_dir && i!=(int)scfg.upload_dir 
						&& !chk_ar(&scfg,scfg.dir[i]->ar,&user,&client))
						continue;
					if(!stricmp(scfg.dir[i]->code_suffix,p))
						break;
				}
				if(i<scfg.total_dirs) 
					dir=i;
				p=tp+1;
			}
			if(dir<0) {
				sprintf(str,"%s.rep",scfg.sys_id);
				if(!(startup->options&FTP_OPT_ALLOW_QWK)
					|| stricmp(p,str)) {
					lprintf(LOG_WARNING,"%04d !%s attempted to upload to invalid directory"
						,sock,user.alias);
					sockprintf(sock,"553 Invalid directory.");
					continue;
				}
				sprintf(fname,"%sfile/%04d.rep",scfg.data_dir,user.number);
				lprintf(LOG_INFO,"%04d %s uploading: %s in %s mode"
					,sock,user.alias,fname
					,mode);
			} else {

				append=(strnicmp(cmd,"APPE",4)==0);
			
				if(!dir_op(&scfg,&user,&client,dir) && !(user.exempt&FLAG('U'))) {
					if(!chk_ar(&scfg,scfg.dir[dir]->ul_ar,&user,&client)) {
						lprintf(LOG_WARNING,"%04d !%s cannot upload to /%s/%s (insufficient access)"
							,sock,user.alias
							,scfg.lib[scfg.dir[dir]->lib]->sname
							,scfg.dir[dir]->code_suffix);
						sockprintf(sock,"553 Insufficient access.");
						continue;
					}

					if(scfg.dir[dir]->maxfiles && getfiles(&scfg,dir)>=scfg.dir[dir]->maxfiles) {
						lprintf(LOG_WARNING,"%04d !%s cannot upload to /%s/%s (directory full: %u files)"
							,sock,user.alias
							,scfg.lib[scfg.dir[dir]->lib]->sname
							,scfg.dir[dir]->code_suffix
							,getfiles(&scfg,dir));
						sockprintf(sock,"553 Directory full.");
						continue;
					}
				}
				if(*p=='-'
					|| strcspn(p,ILLEGAL_FILENAME_CHARS)!=strlen(p)
					|| trashcan(&scfg,p,"file")) {
					lprintf(LOG_WARNING,"%04d !ILLEGAL FILENAME ATTEMPT by %s: %s"
						,sock,user.alias,p);
					sockprintf(sock,"553 Illegal filename attempt");
					ftp_hacklog("FTP FILENAME", user.alias, cmd, host_name, &ftp.client_addr);
					continue;
				}
				SAFEPRINTF2(fname,"%s%s",scfg.dir[dir]->path,p);
				if((!append && filepos==0 && fexist(fname))
					|| (startup->options&FTP_OPT_INDEX_FILE 
						&& !stricmp(p,startup->index_file_name))
					|| (startup->options&FTP_OPT_HTML_INDEX_FILE 
						&& !stricmp(p,startup->html_index_file))
					) {
					lprintf(LOG_WARNING,"%04d !%s attempted to overwrite existing file: %s"
						,sock,user.alias,fname);
					sockprintf(sock,"553 File already exists.");
					continue;
				}
				if(append || filepos) {	/* RESUME */
#ifdef _WIN32
					GetShortPathName(fname, str, sizeof(str));
#else
					SAFECOPY(str,fname);
#endif
					padfname(getfname(str),f.name);
					f.dir=dir;
					f.cdt=0;
					f.size=-1;
					if(!getfileixb(&scfg,&f) || !getfiledat(&scfg,&f)) {
						if(filepos) {
							lprintf(LOG_WARNING,"%04d !%s file (%s) not in database for %.4s command"
								,sock,user.alias,fname,cmd);
							sockprintf(sock,"550 File not found: %s",p);
							continue;
						}
						append=FALSE;
					}
					/* Verify user is original uploader */
					if((append || filepos) && stricmp(f.uler,user.alias)) {
						lprintf(LOG_WARNING,"%04d !%s cannot resume upload of %s, uploaded by %s"
							,sock,user.alias,fname,f.uler);
						sockprintf(sock,"553 Insufficient access (can't resume upload from different user).");
						continue;
					}
				}
				lprintf(LOG_INFO,"%04d %s uploading: %s to %s (%s) in %s mode"
					,sock,user.alias
					,p						/* filename */
					,genvpath(lib,dir,str)	/* virtual path */
					,scfg.dir[dir]->path	/* actual path */
					,mode);
			}
			sockprintf(sock,"150 Opening BINARY mode data connection for file transfer.");
			filexfer(&data_addr,sock,pasv_sock,&data_sock,fname,filepos
				,&transfer_inprogress,&transfer_aborted,FALSE,FALSE
				,&lastactive
				,&user
				,&client
				,dir
				,TRUE	/* uploading */
				,TRUE	/* credits */
				,append
				,desc
				);
			filepos=0;
			continue;
		}

		if(!stricmp(cmd,"CDUP") || !stricmp(cmd,"XCUP")) {
			if(curdir<0)
				curlib=-1;
			else
				curdir=-1;
			sockprintf(sock,"200 CDUP command successful.");
			continue;
		}

		if(!strnicmp(cmd, "CWD ", 4) || !strnicmp(cmd,"XCWD ",5)) {
			p=cmd+4;
			SKIP_WHITESPACE(p);

			if(!strnicmp(p,BBS_FSYS_DIR,strlen(BBS_FSYS_DIR))) 
				p+=strlen(BBS_FSYS_DIR);	/* already mounted */

			if(*p=='/') {
				curlib=-1;
				curdir=-1;
				p++;
			}
			/* Local File System? */
			if(sysop && !(startup->options&FTP_OPT_NO_LOCAL_FSYS) 
				&& !strnicmp(p,LOCAL_FSYS_DIR,strlen(LOCAL_FSYS_DIR))) {	
				p+=strlen(LOCAL_FSYS_DIR);
				if(!direxist(p)) {
					sockprintf(sock,"550 Directory does not exist.");
					lprintf(LOG_WARNING,"%04d !%s attempted to mount invalid directory: %s"
						,sock, user.alias, p);
					continue;
				}
				SAFECOPY(local_dir,p);
				local_fsys=TRUE;
				sockprintf(sock,"250 CWD command successful (local file system mounted).");
				lprintf(LOG_INFO,"%04d %s mounted local file system", sock, user.alias);
				continue;
			}
			success=FALSE;

			/* Directory Alias? */
			if(curlib<0 && ftpalias(p,NULL,&user,&client,&curdir)==TRUE) {
				if(curdir>=0)
					curlib=scfg.dir[curdir]->lib;
				success=TRUE;
			}

			orglib=curlib;
			orgdir=curdir;
			tp=0;
			if(!strncmp(p,"...",3)) {
				curlib=-1;
				curdir=-1;
				p+=3;
			}
			if(!strncmp(p,"./",2))
				p+=2;
			else if(!strncmp(p,"..",2)) {
				if(curdir<0)
					curlib=-1;
				else
					curdir=-1;
				p+=2;
			}
			if(*p==0)
				success=TRUE;
			else if(!strcmp(p,".")) 
				success=TRUE;
			if(!success  && (curlib<0 || *p=='/')) { /* Root dir */
				if(*p=='/') p++;
				tp=strchr(p,'/');
				if(tp) *tp=0;
				for(i=0;i<scfg.total_libs;i++) {
					if(!chk_ar(&scfg,scfg.lib[i]->ar,&user,&client))
						continue;
					if(!stricmp(scfg.lib[i]->sname,p))
						break;
				}
				if(i<scfg.total_libs) {
					curlib=i;
					success=TRUE;
				}
			}
			if((!success && curdir<0) || (success && tp && *(tp+1))) {
				if(tp)
					p=tp+1;
				tp=lastchar(p);
				if(tp && *tp=='/') *tp=0;
				for(i=0;i<scfg.total_dirs;i++) {
					if(scfg.dir[i]->lib!=curlib)
						continue;
					if(i!=(int)scfg.sysop_dir && i!=(int)scfg.upload_dir
						&& !chk_ar(&scfg,scfg.dir[i]->ar,&user,&client))
						continue;
					if(!stricmp(scfg.dir[i]->code_suffix,p))
						break;
				}
				if(i<scfg.total_dirs) {
					curdir=i;
					success=TRUE;
				} else
					success=FALSE;
			}

			if(success)
				sockprintf(sock,"250 CWD command successful.");
			else {
				sockprintf(sock,"550 %s: No such file or directory.",p);
				curlib=orglib;
				curdir=orgdir;
			}
			continue;
		}

		if(!stricmp(cmd, "PWD") || !stricmp(cmd,"XPWD")) {
			if(curlib<0)
				sockprintf(sock,"257 \"/\" is current directory.");
			else if(curdir<0)
				sockprintf(sock,"257 \"/%s\" is current directory."
					,scfg.lib[curlib]->sname);
			else
				sockprintf(sock,"257 \"/%s/%s\" is current directory."
					,scfg.lib[curlib]->sname
					,scfg.dir[curdir]->code_suffix);
			continue;
		}

		if(!strnicmp(cmd, "MKD", 3) || 
			!strnicmp(cmd,"XMKD",4) || 
			!strnicmp(cmd,"SITE EXEC",9)) {
			lprintf(LOG_WARNING,"%04d !SUSPECTED HACK ATTEMPT by %s: '%s'"
				,sock,user.alias,cmd);
			ftp_hacklog("FTP", user.alias, cmd, host_name, &ftp.client_addr);
		}		
		sockprintf(sock,"500 Syntax error: '%s'",cmd);
		lprintf(LOG_WARNING,"%04d !UNSUPPORTED COMMAND from %s: '%s'"
			,sock,user.alias,cmd);
	} /* while(1) */

#if defined(SOCKET_DEBUG_TERMINATE)
	socket_debug[sock]|=SOCKET_DEBUG_TERMINATE;
#endif

	if(transfer_inprogress==TRUE) {
		lprintf(LOG_DEBUG,"%04d Waiting for transfer to complete...",sock);
		count=0;
		while(transfer_inprogress==TRUE) {
			if(ftp_set==NULL || terminate_server) {
				mswait(2000);	/* allow xfer threads to terminate */
				break;
			}
			if(!transfer_aborted) {
				if(gettimeleft(&scfg,&user,logintime)<1) {
					lprintf(LOG_WARNING,"%04d Out of time, disconnecting",sock);
					sockprintf(sock,"421 Sorry, you've run out of time.");
					ftp_close_socket(&data_sock,__LINE__);
					transfer_aborted=TRUE;
				}
				if((time(NULL)-lastactive)>startup->max_inactivity) {
					lprintf(LOG_WARNING,"%04d Disconnecting due to to inactivity",sock);
					sockprintf(sock,"421 Disconnecting due to inactivity (%u seconds)."
						,startup->max_inactivity);
					ftp_close_socket(&data_sock,__LINE__);
					transfer_aborted=TRUE;
				}
			}
			if(count && (count%60)==0)
				lprintf(LOG_WARNING,"%04d Still waiting for transfer to complete "
					"(count=%lu, aborted=%d, lastactive=%lX) ..."
					,sock,count,transfer_aborted,lastactive);
			count++;
			mswait(1000);
		}
		lprintf(LOG_DEBUG,"%04d Done waiting for transfer to complete",sock);
	}

	if(user.number) {
		/* Update User Statistics */
		if(!logoutuserdat(&scfg, &user, time(NULL), logintime))
			lprintf(LOG_ERR,"%04d !ERROR in logoutuserdat",sock);
		/* Remove QWK-pack semaphore file (if left behind) */
		sprintf(str,"%spack%04u.now",scfg.data_dir,user.number);
		ftp_remove(sock, __LINE__, str);
		lprintf(LOG_INFO,"%04d %s logged off",sock,user.alias);
	}

#ifdef _WIN32
	if(startup->hangup_sound[0] && !(startup->options&FTP_OPT_MUTE)) 
		PlaySound(startup->hangup_sound, NULL, SND_ASYNC|SND_FILENAME);
#endif

#ifdef JAVASCRIPT
	if(js_cx!=NULL) {
		lprintf(LOG_DEBUG,"%04d JavaScript: Destroying context",sock);
		JS_BEGINREQUEST(js_cx);
		JS_RemoveObjectRoot(js_cx, &js_glob);
		JS_ENDREQUEST(js_cx);
		JS_DestroyContext(js_cx);	/* Free Context */
	}

	if(js_runtime!=NULL) {
		lprintf(LOG_DEBUG,"%04d JavaScript: Destroying runtime",sock);
		jsrt_Release(js_runtime);
	}

#endif

/*	status(STATUS_WFC); server thread should control status display */

	if(pasv_sock!=INVALID_SOCKET)
		ftp_close_socket(&pasv_sock,__LINE__);
	if(data_sock!=INVALID_SOCKET)
		ftp_close_socket(&data_sock,__LINE__);

	client_off(sock);

#ifdef SOCKET_DEBUG_CTRL
	socket_debug[sock]&=~SOCKET_DEBUG_CTRL;
#endif

#if defined(SOCKET_DEBUG_TERMINATE)
	socket_debug[sock]&=~SOCKET_DEBUG_TERMINATE;
#endif

	tmp_sock=sock;
	ftp_close_socket(&tmp_sock,__LINE__);

	{
		int32_t	clients = protected_uint32_adjust(&active_clients, -1);
		int32_t	threads = thread_down();
		update_clients();

		lprintf(LOG_INFO,"%04d CTRL thread terminated (%ld clients and %ld threads remain, %lu served)"
			,sock, clients, threads, served);
	}
}

static void cleanup(int code, int line)
{
#ifdef _DEBUG
	lprintf(LOG_DEBUG,"0000 cleanup called from line %d",line);
#endif

	if(protected_uint32_value(thread_count) > 1) {
		lprintf(LOG_DEBUG,"#### FTP Server waiting for %d child threads to terminate", protected_uint32_value(thread_count)-1);
		while(protected_uint32_value(thread_count) > 1) {
			mswait(100);
		}
	}

	free_cfg(&scfg);
	free_text(text);

	semfile_list_free(&recycle_semfiles);
	semfile_list_free(&shutdown_semfiles);

	if(ftp_set != NULL) {
		xpms_destroy(ftp_set, ftp_close_socket_cb, NULL);
		ftp_set = NULL;
	}

	update_clients();	/* active_clients is destroyed below */

	if(protected_uint32_value(active_clients))
		lprintf(LOG_WARNING,"#### !FTP Server terminating with %ld active clients", protected_uint32_value(active_clients));
	else
		protected_uint32_destroy(active_clients);

#ifdef _WINSOCKAPI_
	if(WSAInitialized && WSACleanup()!=0) 
		lprintf(LOG_ERR,"0000 !WSACleanup ERROR %d",ERROR_VALUE);
#endif

	thread_down();
	status("Down");
	if(terminate_server || code)
		lprintf(LOG_INFO,"#### FTP Server thread terminated (%lu clients served)", served);
	if(startup!=NULL && startup->terminated!=NULL)
		startup->terminated(startup->cbdata,code);
}

const char* DLLCALL ftp_ver(void)
{
	static char ver[256];
	char compiler[32];

	DESCRIBE_COMPILER(compiler);

	sscanf("$Revision: 1.424 $", "%*s %s", revision);

	sprintf(ver,"%s %s%s  "
		"Compiled %s %s with %s"
		,FTP_SERVER
		,revision
#ifdef _DEBUG
		," Debug"
#else
		,""
#endif
		,__DATE__, __TIME__, compiler);

	return(ver);
}

void DLLCALL ftp_server(void* arg)
{
	char*			p;
	char			path[MAX_PATH+1];
	char			error[256];
	char			compiler[32];
	char			str[256];
	union xp_sockaddr client_addr;
	socklen_t		client_addr_len;
	SOCKET			client_socket;
	int				i;
	time_t			t;
	time_t			start;
	time_t			initialized=0;
	ftp_t*			ftp;
	char			client_ip[INET6_ADDRSTRLEN];

	ftp_ver();

	startup=(ftp_startup_t*)arg;
	SetThreadName("FTP Server");

#ifdef _THREAD_SUID_BROKEN
	if(thread_suid_broken)
		startup->seteuid(TRUE);
#endif

    if(startup==NULL) {
    	sbbs_beep(100,500);
    	fprintf(stderr, "No startup structure passed!\n");
    	return;
    }

	if(startup->size!=sizeof(ftp_startup_t)) {	/* verify size */
		sbbs_beep(100,500);
		sbbs_beep(300,500);
		sbbs_beep(100,500);
		fprintf(stderr, "Invalid startup structure!\n");
		return;
	}

	/* Setup intelligent defaults */
	if(startup->port==0)					startup->port=IPPORT_FTP;
	if(startup->qwk_timeout==0)				startup->qwk_timeout=FTP_DEFAULT_QWK_TIMEOUT;		/* seconds */
	if(startup->max_inactivity==0)			startup->max_inactivity=FTP_DEFAULT_MAX_INACTIVITY;	/* seconds */
	if(startup->sem_chk_freq==0)			startup->sem_chk_freq=2;		/* seconds */
	if(startup->index_file_name[0]==0)		SAFECOPY(startup->index_file_name,"00index");
	if(startup->html_index_file[0]==0)		SAFECOPY(startup->html_index_file,"00index.html");
	if(startup->html_index_script[0]==0) {	SAFECOPY(startup->html_index_script,"ftp-html.js");
											startup->options|=FTP_OPT_HTML_INDEX_FILE;
	}
	if(startup->options&FTP_OPT_HTML_INDEX_FILE)
		startup->options&=~FTP_OPT_NO_JAVASCRIPT;
	else
		startup->options|=FTP_OPT_NO_JAVASCRIPT;
#ifdef JAVASCRIPT
	if(startup->js.max_bytes==0)			startup->js.max_bytes=JAVASCRIPT_MAX_BYTES;
	if(startup->js.cx_stack==0)				startup->js.cx_stack=JAVASCRIPT_CONTEXT_STACK;

	ZERO_VAR(js_server_props);
	SAFEPRINTF2(js_server_props.version,"%s %s",FTP_SERVER,revision);
	js_server_props.version_detail=ftp_ver();
	js_server_props.clients=&active_clients.value;
	js_server_props.options=&startup->options;
	js_server_props.interfaces=&startup->interfaces;
#endif

	uptime=0;
	served=0;
	startup->recycle_now=FALSE;
	startup->shutdown_now=FALSE;
	terminate_server=FALSE;
	protected_uint32_init(&thread_count, 0);

	do {

		protected_uint32_adjust(&thread_count,1);
		thread_up(FALSE /* setuid */);

		status("Initializing");

		memset(&scfg, 0, sizeof(scfg));

		lprintf(LOG_INFO,"Synchronet FTP Server Revision %s%s"
			,revision
#ifdef _DEBUG
			," Debug"
#else
			,""
#endif
			);

		DESCRIBE_COMPILER(compiler);

		lprintf(LOG_INFO,"Compiled %s %s with %s", __DATE__, __TIME__, compiler);

		sbbs_srand();	/* Seed random number generator */

		if(!winsock_startup()) {
			cleanup(1,__LINE__);
			break;
		}

		t=time(NULL);
		lprintf(LOG_INFO,"Initializing on %.24s with options: %lx"
			,ctime_r(&t,str),startup->options);

		if(chdir(startup->ctrl_dir)!=0)
			lprintf(LOG_ERR,"!ERROR %d changing directory to: %s", errno, startup->ctrl_dir);

		/* Initial configuration and load from CNF files */
		SAFECOPY(scfg.ctrl_dir, startup->ctrl_dir);
		lprintf(LOG_INFO,"Loading configuration files from %s", scfg.ctrl_dir);
		scfg.size=sizeof(scfg);
		SAFECOPY(error,UNKNOWN_LOAD_ERROR);
		if(!load_cfg(&scfg, text, TRUE, error)) {
			lprintf(LOG_CRIT,"!ERROR %s",error);
			lprintf(LOG_CRIT,"!Failed to load configuration files");
			cleanup(1,__LINE__);
			break;
		}

		if(startup->host_name[0]==0)
			SAFECOPY(startup->host_name,scfg.sys_inetaddr);

		if((t=checktime())!=0) {   /* Check binary time */
			lprintf(LOG_ERR,"!TIME PROBLEM (%ld)",t);
		}

		if(uptime==0)
			uptime=time(NULL);	/* this must be done *after* setting the timezone */

		if(startup->temp_dir[0])
			SAFECOPY(scfg.temp_dir,startup->temp_dir);
		else
			SAFECOPY(scfg.temp_dir,"../temp");
	   	prep_dir(scfg.ctrl_dir, scfg.temp_dir, sizeof(scfg.temp_dir));
		MKDIR(scfg.temp_dir);
		lprintf(LOG_DEBUG,"Temporary file directory: %s", scfg.temp_dir);
		if(!isdir(scfg.temp_dir)) {
			lprintf(LOG_CRIT,"!Invalid temp directory: %s", scfg.temp_dir);
			cleanup(1,__LINE__);
			break;
		}

		if(!startup->max_clients) {
			startup->max_clients=scfg.sys_nodes;
			if(startup->max_clients<10)
				startup->max_clients=10;
		}
		lprintf(LOG_DEBUG,"Maximum clients: %d",startup->max_clients);

		/* Sanity-check the passive port range */
		if(startup->pasv_port_low || startup->pasv_port_high) {
			if(startup->pasv_port_low > startup->pasv_port_high
				|| startup->pasv_port_high-startup->pasv_port_low < (startup->max_clients-1)) {
				lprintf(LOG_WARNING,"!Correcting Passive Port Range (Low: %u, High: %u)"
					,startup->pasv_port_low,startup->pasv_port_high);
				if(startup->pasv_port_low)
					startup->pasv_port_high = startup->pasv_port_low+(startup->max_clients-1);
				else
					startup->pasv_port_low = startup->pasv_port_high-(startup->max_clients-1);
			}
			lprintf(LOG_DEBUG,"Passive Port Low: %u",startup->pasv_port_low);
			lprintf(LOG_DEBUG,"Passive Port High: %u",startup->pasv_port_high);
		}

		lprintf(LOG_DEBUG,"Maximum inactivity: %d seconds",startup->max_inactivity);

		protected_uint32_init(&active_clients, 0);
		update_clients();

		strlwr(scfg.sys_id); /* Use lower-case unix-looking System ID for group name */

		for(i=0;i<scfg.total_libs;i++) {
			strlwr(scfg.lib[i]->sname);
			dotname(scfg.lib[i]->sname,scfg.lib[i]->sname);
		}
		/* open a socket and wait for a client */
		ftp_set = xpms_create(startup->bind_retry_count, startup->bind_retry_delay, lprintf);
		
		if(ftp_set == NULL) {
			lprintf(LOG_CRIT,"!ERROR %d creating FTP socket set", ERROR_VALUE);
			cleanup(1, __LINE__);
			return;
		}
		lprintf(LOG_DEBUG,"FTP Server socket set created");

		/*
		 * Add interfaces
		 */
		xpms_add_list(ftp_set, PF_UNSPEC, SOCK_STREAM, 0, startup->interfaces, startup->port, "FTP Server", ftp_open_socket_cb, startup->seteuid, NULL);

		lprintf(LOG_INFO,"FTP Server listening");
		status(STATUS_WFC);

		/* Setup recycle/shutdown semaphore file lists */
		shutdown_semfiles=semfile_list_init(scfg.ctrl_dir,"shutdown","ftp");
		recycle_semfiles=semfile_list_init(scfg.ctrl_dir,"recycle","ftp");
		SAFEPRINTF(path,"%sftpsrvr.rec",scfg.ctrl_dir);	/* legacy */
		semfile_list_add(&recycle_semfiles,path);
		if(!initialized) {
			semfile_list_check(&initialized,recycle_semfiles);
			semfile_list_check(&initialized,shutdown_semfiles);
		}

		/* signal caller that we've started up successfully */
		if(startup->started!=NULL)
    		startup->started(startup->cbdata);

		lprintf(LOG_INFO,"FTP Server thread started");

		while(ftp_set!=NULL && !terminate_server) {

			if(protected_uint32_value(thread_count) <= 1) {
				if(!(startup->options&FTP_OPT_NO_RECYCLE)) {
					if((p=semfile_list_check(&initialized,recycle_semfiles))!=NULL) {
						lprintf(LOG_INFO,"0000 Recycle semaphore file (%s) detected",p);
						break;
					}
					if(startup->recycle_now==TRUE) {
						lprintf(LOG_NOTICE,"0000 Recycle semaphore signaled");
						startup->recycle_now=FALSE;
						break;
					}
				}
				if(((p=semfile_list_check(&initialized,shutdown_semfiles))!=NULL
						&& lprintf(LOG_INFO,"0000 Shutdown semaphore file (%s) detected",p))
					|| (startup->shutdown_now==TRUE
						&& lprintf(LOG_INFO,"0000 Shutdown semaphore signaled"))) {
					startup->shutdown_now=FALSE;
					terminate_server=TRUE;
					break;
				}
			}

			if(ftp_set==NULL || terminate_server)	/* terminated */
				break;

			/* now wait for connection */
			client_addr_len = sizeof(client_addr);
			client_socket = xpms_accept(ftp_set, &client_addr, &client_addr_len, startup->sem_chk_freq*1000, NULL);

			if(client_socket == INVALID_SOCKET)
				continue;

			if(startup->socket_open!=NULL)
				startup->socket_open(startup->cbdata,TRUE);

			inet_addrtop(&client_addr, client_ip, sizeof(client_ip));
			if(trashcan(&scfg,client_ip,"ip-silent")) {
				ftp_close_socket(&client_socket,__LINE__);
				continue;
			}
			
			if(protected_uint32_value(active_clients)>=startup->max_clients) {
				lprintf(LOG_WARNING,"%04d !MAXIMUM CLIENTS (%d) reached, access denied"
					,client_socket, startup->max_clients);
				sockprintf(client_socket,"421 Maximum active clients reached, please try again later.");
				mswait(3000);
				ftp_close_socket(&client_socket,__LINE__);
				continue;
			}

			if((ftp=malloc(sizeof(ftp_t)))==NULL) {
				lprintf(LOG_CRIT,"%04d !ERROR allocating %d bytes of memory for ftp_t"
					,client_socket,sizeof(ftp_t));
				sockprintf(client_socket,"421 System error, please try again later.");
				mswait(3000);
				ftp_close_socket(&client_socket,__LINE__);
				continue;
			}

			ftp->socket=client_socket;
			memcpy(&ftp->client_addr, &client_addr, client_addr_len);
			ftp->client_addr_len = client_addr_len;

			protected_uint32_adjust(&thread_count,1);
			_beginthread(ctrl_thread, 0, ftp);
			served++;
		}

#if 0 /* def _DEBUG */
		lprintf(LOG_DEBUG,"0000 terminate_server: %d",terminate_server);
#endif
		if(protected_uint32_value(active_clients)) {
			lprintf(LOG_DEBUG,"Waiting for %d active clients to disconnect..."
				, protected_uint32_value(active_clients));
			start=time(NULL);
			while(protected_uint32_value(active_clients)) {
				if(time(NULL)-start>startup->max_inactivity) {
					lprintf(LOG_WARNING,"!TIMEOUT waiting for %d active clients"
						, protected_uint32_value(active_clients));
					break;
				}
				mswait(100);
			}
		}

		cleanup(0,__LINE__);

		if(!terminate_server) {
			lprintf(LOG_INFO,"Recycling server...");
			mswait(2000);
			if(startup->recycle!=NULL)
				startup->recycle(startup->cbdata);
		}

	} while(!terminate_server);

	protected_uint32_destroy(thread_count);
}
