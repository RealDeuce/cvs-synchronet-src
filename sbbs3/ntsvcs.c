/* ntsrvcs.c */

/* Synchronet BBS as a set of Windows NT Services */

/* $Id: ntsvcs.c,v 1.2 2003/09/26 07:35:59 rswindell Exp $ */

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

/* Synchronet-specific headers */
#include "sbbs.h"		/* various */
#include "sbbs_ini.h"	/* sbbs_read_ini() */
#include "ftpsrvr.h"	/* ftp_startup_t, ftp_server */
#include "websrvr.h"	/* web_startup_t, web_server */
#include "mailsrvr.h"	/* mail_startup_t, mail_server */
#include "services.h"	/* services_startup_t, services_thread */

/* Windows-specific headers */
#include <winsvc.h>

/* Temporary: Do not include web server in 3.1x-Win32 release build */
#if defined(_MSC_VER)
	#define NO_WEB_SERVER
#endif

/* Services doesn't work without JavaScript support */
#if !defined(JAVASCRIPT)
	#define	NO_SERVICES
#endif

static void WINAPI bbs_ctrl_handler(DWORD dwCtrlCode);
static void WINAPI ftp_ctrl_handler(DWORD dwCtrlCode);
static void WINAPI web_ctrl_handler(DWORD dwCtrlCode);
static void WINAPI mail_ctrl_handler(DWORD dwCtrlCode);
static void WINAPI services_ctrl_handler(DWORD dwCtrlCode);

/* Global variables */

bbs_startup_t		bbs_startup;
ftp_startup_t		ftp_startup;
mail_startup_t		mail_startup;
services_startup_t	services_startup;
web_startup_t		web_startup;

typedef struct {
	char*					name;
	char*					display_name;
	char*					description;
	void*					startup;
	void					(*thread)(void* arg);
	void					(WINAPI *ctrl_handler)(DWORD);
	SERVICE_STATUS			status;
	SERVICE_STATUS_HANDLE	status_handle;
} sbbs_ntsvc_t;

sbbs_ntsvc_t bbs ={	
	"SynchronetBBS",
	"Synchronet Telnet/RLogin Server",
	"Provides support for Telnet and RLogin clients and executes timed events. " \
		"This service provides the critical functions of your Synchronet BBS.",
	&bbs_startup,
	bbs_thread,
	bbs_ctrl_handler
};

sbbs_ntsvc_t ftp = {
	"SynchronetFTP",
	"Synchronet FTP Server",
	"Provides support for FTP clients (including web browsers) for file transfers.",
	&ftp_startup,
	ftp_server,
	ftp_ctrl_handler
};

#if !defined(NO_WEB_SERVER)
sbbs_ntsvc_t web = {
	"SynchronetWeb",
	"Synchronet Web Server",
	"Provides support for Web (HTML/HTTP) clients (browsers).",
	&web_startup,
	web_server,
	web_ctrl_handler
};
#endif

sbbs_ntsvc_t mail = {
	"SynchronetMail",
	"Synchronet SMTP/POP3 Mail Server",
	"Sends and receives Internet e-mail (using SMTP) and allows users to remotely " \
		"access their e-mail using an Internet mail client (using POP3).",
	&mail_startup,
	mail_server,
	mail_ctrl_handler
};

#if !defined(NO_SERVICES)
sbbs_ntsvc_t services = {
	"SynchronetServices",
	"Synchronet Services",
	"Plugin servers (usually in JavaScript) for any TCP/UDP protocol. " \
		"Stock services include Finger, Gopher, NNTP, and IRC. Edit your ctrl/services.ini " \
		"file for configuration of individual Synchronet Services.",
	&services_startup,
	services_thread,
	services_ctrl_handler
};
#endif

sbbs_ntsvc_t* ntsvc_list[] = {
	&bbs,
	&ftp,
#if !defined(NO_WEB_SERVER)
	&web,
#endif
	&mail,
#if !defined(NO_SERVICES)
	&services,
#endif
	NULL
};
							
static WSADATA WSAData;

static BOOL winsock_startup(void)
{
	int		status;             /* Status Code */

    if((status = WSAStartup(MAKEWORD(1,1), &WSAData))==0)
		return(TRUE);

    fprintf(stderr,"!WinSock startup ERROR %d\n", status);
	return(FALSE);
}

static BOOL winsock_cleanup(void)	
{
	if(WSACleanup()==0)
		return(TRUE);

	fprintf(stderr,"!WinSock cleanup ERROR %d\n",ERROR_VALUE);
	return(FALSE);
}

/* Service Control Handlers (Callbacks) */

static void WINAPI bbs_ctrl_handler(DWORD dwCtrlCode)
{
	switch(dwCtrlCode) {
		case SERVICE_CONTROL_STOP:
		case SERVICE_CONTROL_SHUTDOWN:
			bbs_terminate();
			bbs.status.dwCurrentState=SERVICE_STOP_PENDING;
			break;
	}
	SetServiceStatus(bbs.status_handle, &bbs.status);
}

static void WINAPI ftp_ctrl_handler(DWORD dwCtrlCode)
{
	switch(dwCtrlCode) {
		case SERVICE_CONTROL_STOP:
		case SERVICE_CONTROL_SHUTDOWN:
			ftp_terminate();
			ftp.status.dwCurrentState=SERVICE_STOP_PENDING;
			break;
	}
	SetServiceStatus(ftp.status_handle, &ftp.status);
}

#if !defined(NO_WEB_SERVER)
static void WINAPI web_ctrl_handler(DWORD dwCtrlCode)
{
	switch(dwCtrlCode) {
		case SERVICE_CONTROL_STOP:
		case SERVICE_CONTROL_SHUTDOWN:
			web_terminate();
			web.status.dwCurrentState=SERVICE_STOP_PENDING;
			break;
	}
	SetServiceStatus(web.status_handle, &web.status);
}
#endif

static void WINAPI mail_ctrl_handler(DWORD dwCtrlCode)
{
	switch(dwCtrlCode) {
		case SERVICE_CONTROL_STOP:
		case SERVICE_CONTROL_SHUTDOWN:
			mail_terminate();
			mail.status.dwCurrentState=SERVICE_STOP_PENDING;
			break;
	}
	SetServiceStatus(mail.status_handle, &mail.status);
}

#if !defined(NO_SERVICES)
static void WINAPI services_ctrl_handler(DWORD dwCtrlCode)
{
	switch(dwCtrlCode) {
		case SERVICE_CONTROL_STOP:
		case SERVICE_CONTROL_SHUTDOWN:
			services_terminate();
			services.status.dwCurrentState=SERVICE_STOP_PENDING;
			break;
	}
	SetServiceStatus(services.status_handle, &services.status);
}
#endif

/****************************************************************************/
/* BBS local/log print routine												*/
/****************************************************************************/
static int svc_lputs(void* p, char* str)
{
    return(0);
}

/****************************************************************************/
/* Event thread local/log print routine										*/
/****************************************************************************/
static int event_lputs(char *str)
{
    return 0;
}

/************************************/
/* Shared Service Callback Routines */
/************************************/
static void svc_started(void* p)
{
	sbbs_ntsvc_t* svc = (sbbs_ntsvc_t*)p;

	svc->status.dwCurrentState=SERVICE_RUNNING;
	svc->status.dwControlsAccepted=SERVICE_ACCEPT_STOP;
	SetServiceStatus(svc->status_handle, &svc->status);
}

static void svc_terminated(void* p, int code)
{
	sbbs_ntsvc_t* svc = (sbbs_ntsvc_t*)p;

	if(code) {
		svc->status.dwWin32ExitCode=ERROR_SERVICE_SPECIFIC_ERROR;
		svc->status.dwServiceSpecificExitCode=code;
		SetServiceStatus(svc->status_handle, &svc->status);
	}
}

/* Generic ServiceMain function */
static void WINAPI svc_start(sbbs_ntsvc_t* svc)
{
    if((svc->status_handle = RegisterServiceCtrlHandler(svc->name, svc->ctrl_handler))==0) {
		fprintf(stderr,"!ERROR %d registering service control handler\n",GetLastError());
		return;
	}

	memset(&svc->status,0,sizeof(SERVICE_STATUS));
	svc->status.dwServiceType=SERVICE_WIN32_SHARE_PROCESS;
	svc->status.dwWaitHint=30000;	/* milliseconds */

	svc->status.dwCurrentState=SERVICE_START_PENDING;
	SetServiceStatus(svc->status_handle, &svc->status);

	svc->thread(&svc->startup);

	svc->status.dwCurrentState=SERVICE_STOPPED;
	SetServiceStatus(svc->status_handle, &svc->status);
}

/* These are the actual ServiceMain stub functions */

static void WINAPI bbs_start(DWORD dwArgc, LPTSTR *lpszArgv)
{
	svc_start(&bbs);
}

static void WINAPI ftp_start(DWORD dwArgc, LPTSTR *lpszArgv)
{
	svc_start(&ftp);
}

#if !defined(NO_WEB_SERVER)
static void WINAPI web_start(DWORD dwArgc, LPTSTR *lpszArgv)
{
	svc_start(&web);
}
#endif

static void WINAPI mail_start(DWORD dwArgc, LPTSTR *lpszArgv)
{
	svc_start(&mail);
}

#if !defined(NO_SERVICES)
static void WINAPI services_start(DWORD dwArgc, LPTSTR *lpszArgv)
{
	svc_start(&services);
}
#endif

/******************************************/
/* NT Serivce Install/Uninstall Functions */
/******************************************/

/* ChangeServiceConfig2 is a Win2K+ API function, must call dynamically */
typedef WINADVAPI BOOL (WINAPI *ChangeServiceConfig2_t)(SC_HANDLE, DWORD, LPCVOID);

static void describe_service(HANDLE hSCMlib, SC_HANDLE hService, char* description)
{
	ChangeServiceConfig2_t changeServiceConfig2;
	static SERVICE_DESCRIPTION service_desc;
  
	if(hSCMlib==NULL)
		return;

	service_desc.lpDescription=description;

	if((changeServiceConfig2 = (ChangeServiceConfig2_t)GetProcAddress(hSCMlib, "ChangeServiceConfig2A"))!=NULL)
		changeServiceConfig2(hService, SERVICE_CONFIG_DESCRIPTION, &service_desc);
}

static SC_HANDLE create_service(HANDLE hSCMlib, SC_HANDLE hSCManager
								,char* name, char* DISP_name, char* description, char* path)
{
    SC_HANDLE   hService;

	printf("Installing service: %-40s ... ", DISP_name);

    hService = CreateService(
        hSCManager,					// SCManager database
        name,						// name of service
        DISP_name,				// name to display
        SERVICE_ALL_ACCESS,         // desired access
        SERVICE_WIN32_OWN_PROCESS,  // service type
		SERVICE_AUTO_START,			// start type (auto or manual)
        SERVICE_ERROR_NORMAL,       // error control type
        path,						// service's binary
        NULL,                       // no load ordering group
        NULL,                       // no tag identifier
        "",							// dependencies
        NULL,                       // LocalSystem account
        NULL);                      // no password

	if(hService==NULL)
		printf("!ERROR %d\n",GetLastError());
	else {
		describe_service(hSCMlib, hService,description);
		CloseServiceHandle(hService);
		printf("Successful\n");
	}

	return(hService);
}


static int install(void)
{
	int			i;
	HANDLE		hSCMlib;
    SC_HANDLE   hSCManager;
    char		path[MAX_PATH+1];

	printf("Installing Synchronet NT Services...\n");

	hSCMlib = LoadLibrary("ADVAPI32.DLL");

    if(GetModuleFileName(NULL,path,sizeof(path))==0)
    {
        fprintf(stderr,"!ERROR %d getting module file name\n",GetLastError());
        return(-1);
    }

    hSCManager = OpenSCManager(
                        NULL,                   // machine (NULL == local)
                        NULL,                   // database (NULL == default)
                        SC_MANAGER_ALL_ACCESS   // access required
                        );
    if(hSCManager==NULL) {
		fprintf(stderr,"!ERROR %d opening SC manager\n",GetLastError());
		return(-1);
	}

	for(i=0;ntsvc_list[i]!=NULL;i++)
		create_service(hSCMlib
			,hSCManager
			,ntsvc_list[i]->name
			,ntsvc_list[i]->display_name
			,ntsvc_list[i]->description
			,path);

	if(hSCMlib!=NULL)
		FreeLibrary(hSCMlib);

	CloseServiceHandle(hSCManager);

	return(0);
}

static void remove_service(SC_HANDLE hSCManager, char* name, char* DISP_name)
{
    SC_HANDLE		hService;
	SERVICE_STATUS	status;

	printf("Removing service: %-40s ... ", DISP_name);

    hService = OpenService(hSCManager, name, SERVICE_ALL_ACCESS);

	if(hService==NULL) {
		printf("\n!ERROR %d opening service: %s\n",GetLastError(),name);
		return;
	}

    // try to stop the service
    if(ControlService( hService, SERVICE_CONTROL_STOP, &status))
    {
        printf("\nStopping: %s ... ",name);

        while(QueryServiceStatus(hService, &status) && status.dwCurrentState == SERVICE_STOP_PENDING)
			Sleep(1000);

        if(status.dwCurrentState == SERVICE_STOPPED)
            printf("Stopped.\n");
        else
            printf("FAILED!\n");
    }

    // now remove the service
    if(DeleteService(hService))
		printf("Successful\n");
	else
		printf("!ERROR %d\n",GetLastError());
    CloseServiceHandle(hService);
}


static int uninstall(void)
{
	int			i;
    SC_HANDLE   hSCManager;

    hSCManager = OpenSCManager(
                        NULL,                   // machine (NULL == local)
                        NULL,                   // database (NULL == default)
                        SC_MANAGER_ALL_ACCESS   // access required
                        );
    if(hSCManager==NULL) {
		fprintf(stderr,"!ERROR %d opening SC manager\n",GetLastError());
		return(-1);
	}

	for(i=0;ntsvc_list[i]!=NULL;i++)
		remove_service(hSCManager
			,ntsvc_list[i]->name
			,ntsvc_list[i]->display_name);

	CloseServiceHandle(hSCManager);

	return(0);
}


/****************************************************************************/
/* Main Entry Point															*/
/****************************************************************************/
int main(int argc, char** argv)
{
	char*	ctrl_dir;
	char	str[MAX_PATH+1];
	char	ini_file[MAX_PATH+1];
	char	host_name[128]="";
	FILE*	fp=NULL;

	SERVICE_TABLE_ENTRY  ServiceDispatchTable[] = 
    { 
        { bbs.name,			bbs_start		}, 
		{ ftp.name,			ftp_start		},
#if !defined(NO_WEB_SERVER)
		{ web.name,			web_start		},
#endif
		{ mail.name,		mail_start		},
#if !defined(NO_SERVICES)
		{ services.name,	services_start	},
#endif
        { NULL,				NULL			}	/* Terminator */
    }; 

	printf("\nSynchronet NT Services  Version %s%c  %s\n\n"
		,VERSION,REVISION,COPYRIGHT_NOTICE);

	if(argc>1 && !stricmp(argv[1],"-install"))
		return install();

	if(argc>1 && !stricmp(argv[1],"-remove"))
		return uninstall();

    printf("%s -install          to install the services\n", getfname(argv[0]) );
    printf("%s -remove           to remove the services\n", getfname(argv[0]) );

	ctrl_dir=getenv("SBBSCTRL");	/* read from environment variable */
	if(ctrl_dir==NULL || ctrl_dir[0]==0) {
		ctrl_dir="/sbbs/ctrl";		/* Not set? Use default */
		printf("!SBBSCTRL environment variable not set, using default value: %s\n\n"
			,ctrl_dir);
	}

	if(!winsock_startup())
		return(-1);

	gethostname(host_name,sizeof(host_name)-1);

	if(!winsock_cleanup())
		return(-1);

	sprintf(ini_file,"%s%c%s.ini",ctrl_dir,PATH_DELIM,host_name);
	if(!fexistcase(ini_file))
		sprintf(ini_file,"%s%csbbs.ini",ctrl_dir,PATH_DELIM);

	/* Initialize BBS startup structure */
    memset(&bbs_startup,0,sizeof(bbs_startup));
    bbs_startup.size=sizeof(bbs_startup);
	bbs_startup.private_data=&bbs;
	bbs_startup.lputs=svc_lputs;
	bbs_startup.event_log=event_lputs;
    bbs_startup.started=svc_started;
    bbs_startup.terminated=svc_terminated;
    strcpy(bbs_startup.ctrl_dir,ctrl_dir);

	/* Initialize FTP startup structure */
    memset(&ftp_startup,0,sizeof(ftp_startup));
	ftp_startup.private_data=&ftp;
    ftp_startup.size=sizeof(ftp_startup);
	ftp_startup.lputs=svc_lputs;
    ftp_startup.started=svc_started;
    ftp_startup.terminated=svc_terminated;
    strcpy(ftp_startup.ctrl_dir,ctrl_dir);

#if !defined(NO_WEB_SERVER)
	/* Initialize Web Server startup structure */
    memset(&web_startup,0,sizeof(web_startup));
	web_startup.private_data=&web;
    web_startup.size=sizeof(web_startup);
	web_startup.lputs=svc_lputs;
    web_startup.started=svc_started;
    web_startup.terminated=svc_terminated;
    strcpy(web_startup.ctrl_dir,ctrl_dir);
#endif

	/* Initialize Mail Server startup structure */
    memset(&mail_startup,0,sizeof(mail_startup));
	mail_startup.private_data=&mail;
    mail_startup.size=sizeof(mail_startup);
	mail_startup.lputs=svc_lputs;
    mail_startup.started=svc_started;
    mail_startup.terminated=svc_terminated;
    strcpy(mail_startup.ctrl_dir,ctrl_dir);

#if !defined(NO_SERVICES)
	/* Initialize Services startup structure */
    memset(&services_startup,0,sizeof(services_startup));
	services_startup.private_data=&services;
    services_startup.size=sizeof(services_startup);
	services_startup.lputs=svc_lputs;
    services_startup.started=svc_started;
    services_startup.terminated=svc_terminated;
    strcpy(services_startup.ctrl_dir,ctrl_dir);
#endif

	/* Read .ini file here */
	if(ini_file[0]!=0 && (fp=fopen(ini_file,"r"))!=NULL) {
		sprintf(str,"Reading %s",ini_file);
		svc_lputs(&bbs, str);
	}

	/* We call this function to set defaults, even if there's no .ini file */
	sbbs_read_ini(fp, 
		NULL,	&bbs_startup,
		NULL,	&ftp_startup, 
		NULL,	&web_startup,
		NULL,	&mail_startup, 
		NULL,	&services_startup);

	/* close .ini file here */
	if(fp!=NULL)
		fclose(fp);

    printf("\nStartServiceCtrlDispatcher being called.\n" );
    printf("This may take several seconds.  Please wait.\n" );

	if(!StartServiceCtrlDispatcher(ServiceDispatchTable)) 
    { 
		sprintf(str,"!Synchronet StartServiceCtrlDispatcher ERROR %d",GetLastError());
		printf("%s\n",str);
        OutputDebugString(str); 
    } 

	return(0);
}
