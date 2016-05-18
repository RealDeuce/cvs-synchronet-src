/* login.cpp */

/* Synchronet user login routine */

/* $Id: login.cpp,v 1.23 2016/05/18 10:15:12 rswindell Exp $ */

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
#include "cmdshell.h"

int sbbs_t::login(char *username, char *pw)
{
	char	str[128];
	char 	tmp[512];
	long	useron_misc=useron.misc;

	useron.number=0;
#if 0
	if(cfg.node_dollars_per_call && noyes(text[AreYouSureQ]))
		return(LOGIC_FALSE);
#endif

	SAFECOPY(str,username);

	if(str[0]=='*') {
		memmove(str,str+1,strlen(str));
		qwklogon=1; 
	}
	else
		qwklogon=0;

	if(!(cfg.node_misc&NM_NO_NUM) && isdigit((uchar)str[0])) {
		useron.number=atoi(str);
		getuserdat(&cfg,&useron);
		if(useron.number && useron.misc&(DELETED|INACTIVE))
			useron.number=0; 
	}

	if(!useron.number) {
		useron.number=matchuser(&cfg,str,FALSE);
		if(!useron.number && (uchar)str[0]<0x7f && str[1]
			&& isalpha(str[0]) && strchr(str,' ') && cfg.node_misc&NM_LOGON_R)
			useron.number=userdatdupe(0,U_NAME,LEN_NAME,str);
		if(useron.number) {
			getuserdat(&cfg,&useron);
			if(useron.number && useron.misc&(DELETED|INACTIVE))
				useron.number=0; } 
	}

	if(!useron.number) {
		if(cfg.node_misc&NM_LOGON_P) {
			SAFECOPY(useron.alias,str);
			bputs(pw);
			console|=CON_R_ECHOX;
			getstr(str,LEN_PASS*2,K_UPPER|K_LOWPRIO|K_TAB);
			console&=~(CON_R_ECHOX|CON_L_ECHOX);
			badlogin(useron.alias, str);
			bputs(text[InvalidLogon]);	/* why does this always fail? */
			if(cfg.sys_misc&SM_ECHO_PW) 
				sprintf(tmp,"(%04u)  %-25s  FAILED Password attempt: '%s'"
					,0,useron.alias,str);
			else
				sprintf(tmp,"(%04u)  %-25s  FAILED Password attempt"
					,0,useron.alias);
			logline(LOG_NOTICE,"+!",tmp); 
		} else {
			bputs(text[UnknownUser]);
			sprintf(tmp,"Unknown User '%s'",str);
			logline(LOG_NOTICE,"+!",tmp); 
		}
		useron.misc=useron_misc;
		return(LOGIC_FALSE); 
	}

	if(!online) {
		useron.number=0;
		return(LOGIC_FALSE); 
	}

	if(useron.pass[0] || REALSYSOP) {
		bputs(pw);
		console|=CON_R_ECHOX;
		getstr(str,LEN_PASS*2,K_UPPER|K_LOWPRIO|K_TAB);
		console&=~(CON_R_ECHOX|CON_L_ECHOX);
		if(!online) {
			useron.number=0;
			return(LOGIC_FALSE); 
		}
		if(stricmp(useron.pass,str)) {
			badlogin(useron.alias, str);
			bputs(text[InvalidLogon]);
			if(cfg.sys_misc&SM_ECHO_PW) 
				sprintf(tmp,"(%04u)  %-25s  FAILED Password: '%s' Attempt: '%s'"
					,useron.number,useron.alias,useron.pass,str);
			else
				sprintf(tmp,"(%04u)  %-25s  FAILED Password attempt"
					,useron.number,useron.alias);
			logline(LOG_NOTICE,"+!",tmp);
			useron.number=0;
			useron.misc=useron_misc;
			return(LOGIC_FALSE); 
		}
		if(REALSYSOP && !chksyspass()) {
			bputs(text[InvalidLogon]);
			useron.number=0;
			useron.misc=useron_misc;
			return(LOGIC_FALSE); 
		} 
	}

	return(LOGIC_TRUE);
}

void sbbs_t::badlogin(char* user, char* passwd)
{
	char reason[128];
	ulong count;

	SAFEPRINTF(reason,"%s LOGIN", connection);
	count=loginFailure(startup->login_attempt_list, &client_addr, connection, user, passwd);
	if(startup->login_attempt.hack_threshold && count>=startup->login_attempt.hack_threshold)
		::hacklog(&cfg, reason, user, passwd, client_name, &client_addr);
	if(startup->login_attempt.filter_threshold && count>=startup->login_attempt.filter_threshold)
		filter_ip(&cfg, connection, "- TOO MANY CONSECUTIVE FAILED LOGIN ATTEMPTS"
			,client_name, client_ipaddr, user, /* fname: */NULL);

	mswait(startup->login_attempt.delay);
}
