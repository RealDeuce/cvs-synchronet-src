/* answer.cpp */

/* Synchronet answer "caller" function */

/* $Id: answer.cpp,v 1.86 2015/08/20 05:19:40 deuce Exp $ */

/****************************************************************************
 * @format.tab-size 4		(Plain Text/Source Code File Header)			*
 * @format.use-tabs true	(see http://www.synchro.net/ptsc_hdr.html)		*
 *																			*
 * Copyright 2014 Rob Swindell - http://www.synchro.net/copyright.html		*
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
#include "telnet.h"

extern "C" void client_on(SOCKET sock, client_t* client, BOOL update);

bool sbbs_t::answer()
{
	char	str[MAX_PATH+1],str2[MAX_PATH+1],c;
	char 	tmp[(MAX_PATH > CRYPT_MAX_TEXTSIZE ? MAX_PATH:CRYPT_MAX_TEXTSIZE)+1];
	char 	tmpname[CRYPT_MAX_TEXTSIZE+1];
	char 	path[MAX_PATH+1];
	int		i,l,in;
	struct tm tm;

	useron.number=0;
	answertime=logontime=starttime=now=time(NULL);
	/* Caller ID is IP address */
	SAFECOPY(cid,client_ipaddr);

	memset(&tm,0,sizeof(tm));
    localtime_r(&now,&tm); 

	safe_snprintf(str,sizeof(str),"%s  %s %s %02d %u            Node %3u"
		,hhmmtostr(&cfg,&tm,str2)
		,wday[tm.tm_wday]
        ,mon[tm.tm_mon],tm.tm_mday,tm.tm_year+1900,cfg.node_num);
	logline("@ ",str);

	safe_snprintf(str,sizeof(str),"%s  %s [%s]", connection, client_name, cid);
	logline("@+:",str);

	if(client_ident[0]) {
		safe_snprintf(str,sizeof(str),"Identity: %s",client_ident);
		logline("@*",str);
	}

	if(sys_status&SS_RLOGIN) {
		if(incom(1000)==0) {
			for(i=0;i<(int)sizeof(str)-1;i++) {
				in=incom(1000);
				if(in==0 || in==NOINP)
					break;
				str[i]=in;
			}
			str[i]=0;
			for(i=0;i<(int)sizeof(str2)-1;i++) {
				in=incom(1000);
				if(in==0 || in==NOINP)
					break;
				str2[i]=in;
			}
			str2[i]=0;
			for(i=0;i<(int)sizeof(terminal)-1;i++) {
				in=incom(1000);
				if(in==0 || in==NOINP)
					break;
				terminal[i]=in;
			}
			terminal[i]=0;
			lprintf(LOG_DEBUG,"Node %d RLogin: '%.*s' / '%.*s' / '%s'"
				,cfg.node_num
				,LEN_ALIAS*2,str
				,LEN_ALIAS*2,str2
				,terminal);
			SAFECOPY(rlogin_term, terminal);
			SAFECOPY(rlogin_name, str2);
			SAFECOPY(rlogin_pass, str);
			/* Truncate terminal speed (e.g. "/57600") from terminal-type string 
			   (but keep full terminal type/speed string in rlogin_term): */
			truncstr(terminal,"/");	
			useron.number=userdatdupe(0, U_ALIAS, LEN_ALIAS, rlogin_name);
			if(useron.number) {
				getuserdat(&cfg,&useron);
				useron.misc&=~TERM_FLAGS;
				SAFEPRINTF(path,"%srlogin.cfg",cfg.ctrl_dir);
				if(!findstr(client.addr,path)) {
					SAFECOPY(tmp, rlogin_pass);
					for(i=0;i<3 && online;i++) {
						if(stricmp(tmp,useron.pass)) {
							if(cfg.sys_misc&SM_ECHO_PW)
								safe_snprintf(str,sizeof(str),"(%04u)  %-25s  FAILED Password attempt: '%s'"
									,0,useron.alias,tmp);
							else
								safe_snprintf(str,sizeof(str),"(%04u)  %-25s  FAILED Password attempt"
									,0,useron.alias);
							logline(LOG_NOTICE,"+!",str);
							badlogin(useron.alias, tmp);
							rioctl(IOFI);       /* flush input buffer */
							bputs(text[InvalidLogon]);
							bputs(text[PasswordPrompt]);
							console|=CON_R_ECHOX;
							getstr(tmp,LEN_PASS*2,K_UPPER|K_LOWPRIO|K_TAB);
							console&=~(CON_R_ECHOX|CON_L_ECHOX);
						}
						else {
							if(REALSYSOP) {
								rioctl(IOFI);       /* flush input buffer */
								if(!chksyspass())
									bputs(text[InvalidLogon]);
								else {
									i=0;
									break;
								}
							}
							else
								break;
						}
					}
					if(i) {
						if(stricmp(tmp,useron.pass)) {
							if(cfg.sys_misc&SM_ECHO_PW)
								safe_snprintf(str,sizeof(str),"(%04u)  %-25s  FAILED Password attempt: '%s'"
									,0,useron.alias,tmp);
							else
								safe_snprintf(str,sizeof(str),"(%04u)  %-25s  FAILED Password attempt"
									,0,useron.alias);
							logline(LOG_NOTICE,"+!",str);
							badlogin(useron.alias, tmp);
							bputs(text[InvalidLogon]);
						}
						lprintf(LOG_WARNING,"Node %d !CLIENT IP NOT LISTED in %s"
							,cfg.node_num,path);
						useron.number=0;
						hangup();
					}
				}
			}
			else {
				if(cfg.sys_misc&SM_ECHO_PW)
					lprintf(LOG_INFO,"Node %d RLogin: UNKNOWN USER: '%s' (password: %s)",cfg.node_num, rlogin_name, rlogin_pass);
				else
					lprintf(LOG_INFO,"Node %d RLogin: UNKNOWN USER: '%s'",cfg.node_num,rlogin_name);
				badlogin(rlogin_name, rlogin_pass);
			}
		}
		if(rlogin_name[0]==0) {
			lprintf(LOG_NOTICE,"Node %d !RLogin: No user name received",cfg.node_num);
			sys_status&=~SS_RLOGIN;
		}
	}

	if(!(telnet_mode&TELNET_MODE_OFF)) {
		/* Disable Telnet Terminal Echo */
		request_telnet_opt(TELNET_WILL,TELNET_ECHO);
		/* Will suppress Go Ahead */
		request_telnet_opt(TELNET_WILL,TELNET_SUP_GA);
		/* Retrieve terminal type and speed from telnet client --RS */
		request_telnet_opt(TELNET_DO,TELNET_TERM_TYPE);
		request_telnet_opt(TELNET_DO,TELNET_TERM_SPEED);
		request_telnet_opt(TELNET_DO,TELNET_SEND_LOCATION);
		request_telnet_opt(TELNET_DO,TELNET_NEGOTIATE_WINDOW_SIZE);
		request_telnet_opt(TELNET_DO,TELNET_NEW_ENVIRON);
	}
#ifdef USE_CRYPTLIB
	if(sys_status&SS_SSH) {
		pthread_mutex_lock(&ssh_mutex);
		cryptGetAttributeString(ssh_session, CRYPT_SESSINFO_USERNAME, tmpname, &i);
		tmpname[i]=0;
		SAFECOPY(rlogin_name, tmpname);
		cryptGetAttributeString(ssh_session, CRYPT_SESSINFO_PASSWORD, tmp, &i);
		tmp[i]=0;
		SAFECOPY(rlogin_pass, tmp);
		pthread_mutex_unlock(&ssh_mutex);
		lprintf(LOG_DEBUG,"Node %d SSH login: '%s'"
			,cfg.node_num, tmpname);
		useron.number=userdatdupe(0, U_ALIAS, LEN_ALIAS, tmpname);
		if(useron.number) {
			getuserdat(&cfg,&useron);
			useron.misc&=~TERM_FLAGS;
			for(i=0;i<3 && online;i++) {
				if(stricmp(tmp,useron.pass)) {
					if(cfg.sys_misc&SM_ECHO_PW)
						safe_snprintf(str,sizeof(str),"(%04u)  %-25s  FAILED Password attempt: '%s'"
							,0,useron.alias,tmp);
					else
						safe_snprintf(str,sizeof(str),"(%04u)  %-25s  FAILED Password attempt"
							,0,useron.alias);
					logline(LOG_NOTICE,"+!",str);
					badlogin(useron.alias, tmp);
					rioctl(IOFI);       /* flush input buffer */
					bputs(text[InvalidLogon]);
					bputs(text[PasswordPrompt]);
					console|=CON_R_ECHOX;
					getstr(tmp,LEN_PASS*2,K_UPPER|K_LOWPRIO|K_TAB);
					console&=~(CON_R_ECHOX|CON_L_ECHOX);
				}
				else {
					if(REALSYSOP) {
						rioctl(IOFI);       /* flush input buffer */
						if(!chksyspass())
							bputs(text[InvalidLogon]);
						else {
							i=0;
							break;
						}
					}
					else
						break;
				}
			}
			if(i) {
				if(stricmp(tmp,useron.pass)) {
					if(cfg.sys_misc&SM_ECHO_PW)
						safe_snprintf(str,sizeof(str),"(%04u)  %-25s  FAILED Password attempt: '%s'"
							,0,useron.alias,tmp);
					else
						safe_snprintf(str,sizeof(str),"(%04u)  %-25s  FAILED Password attempt"
							,0,useron.alias);
					logline(LOG_NOTICE,"+!",str);
					badlogin(useron.alias, tmp);
					bputs(text[InvalidLogon]);
				}
				useron.number=0;
				hangup();
			}
		}
		else {
			if(cfg.sys_misc&SM_ECHO_PW)
				lprintf(LOG_INFO,"Node %d SSH: UNKNOWN USER: '%s' (password: %s)",cfg.node_num,tmpname, rlogin_pass);
			else
				lprintf(LOG_INFO,"Node %d SSH: UNKNOWN USER: '%s'",cfg.node_num,tmpname);
			badlogin(tmpname, rlogin_pass);
		}
	}
#endif

	/* Detect terminal type */
    mswait(200);
	rioctl(IOFI);		/* flush input buffer */
	putcom( "\r\n"		/* locate cursor at column 1 */
			"\x1b[s"	/* save cursor position (necessary for HyperTerm auto-ANSI) */
    		"\x1b[255B"	/* locate cursor as far down as possible */
			"\x1b[255C"	/* locate cursor as far right as possible */
			"\b_"		/* need a printable at this location to actually move cursor */
			"\x1b[6n"	/* Get cursor position */
			"\x1b[u"	/* restore cursor position */
			"\x1b[!_"	/* RIP? */
			"\x1b[30;40m\xc2\x9f""Zuul.connection.write('\\x1b""Are you the gatekeeper?')\xc2\x9c"	/* ZuulTerm? */
			"\x1b[0m_"	/* "Normal" colors */
			"\x1b[2J"	/* clear screen */
			"\x1b[H"	/* home cursor */
			"\xC"		/* clear screen (in case not ANSI) */
			"\r"		/* Move cursor left (in case previous char printed) */
			);
	i=l=0;
	tos=1;
	lncntr=0;
	safe_snprintf(str, sizeof(str), "%s  %s", VERSION_NOTICE, COPYRIGHT_NOTICE);
	strip_ctrl(str, str);
	center(str);

	while(i++<50 && l<(int)sizeof(str)-1) { 	/* wait up to 5 seconds for response */
		c=incom(100)&0x7f;
		if(c==0)
			continue;
		i=0;
		if(l==0 && c!=ESC)	// response must begin with escape char
			continue;
		str[l++]=c;
		if(c=='R') {   /* break immediately if ANSI response */
			mswait(500);
			break; 
		}
	}

	while((c=(incom(100)&0x7f))!=0 && l<(int)sizeof(str)-1)
		str[l++]=c;
	str[l]=0;

    if(l) {
		c_escape_str(str,tmp,sizeof(tmp)-1,TRUE);
		lprintf(LOG_DEBUG,"Node %d received terminal auto-detection response: '%s'"
			,cfg.node_num,tmp);
        if(str[0]==ESC && str[1]=='[' && str[l-1]=='R') {
			int	x,y;

			if(terminal[0]==0)
				SAFECOPY(terminal,"ANSI");
			autoterm|=(ANSI|COLOR);
			if(sscanf(str+2,"%u;%u",&y,&x)==2) {
				lprintf(LOG_DEBUG,"Node %d received ANSI cursor position report: %ux%u"
					,cfg.node_num, x, y);
				/* Sanity check the coordinates in the response: */
				if(x>=40 && x<=255) cols=x; 
				if(y>=10 && y<=255) rows=y;
			}
		}
		truncsp(str);
		if(strstr(str,"RIPSCRIP")) {
			if(terminal[0]==0)
				SAFECOPY(terminal,"RIP");
			logline("@R",strstr(str,"RIPSCRIP"));
			autoterm|=(RIP|COLOR|ANSI); }
		else if(strstr(str,"Are you the gatekeeper?"))  {
			if(terminal[0]==0)
				SAFECOPY(terminal,"HTML");
			logline("@H",strstr(str,"Are you the gatekeeper?"));
			autoterm|=HTML;
		} 
	}
	else if(terminal[0]==0)
		SAFECOPY(terminal,"DUMB");

	rioctl(IOFI); /* flush left-over or late response chars */

	if(!autoterm && str[0]) {
		c_escape_str(str,tmp,sizeof(tmp)-1,TRUE);
		lprintf(LOG_NOTICE,"Node %d terminal auto-detection failed, response: '%s'"
			,cfg.node_num, tmp);
	}

	/* AutoLogon via IP or Caller ID here */
	if(!useron.number && !(sys_status&SS_RLOGIN)
		&& (startup->options&BBS_OPT_AUTO_LOGON) && cid[0]) {
		useron.number=userdatdupe(0, U_IPADDR, LEN_IPADDR, cid);
		if(useron.number) {
			getuserdat(&cfg, &useron);
			if(!(useron.misc&AUTOLOGON) || !(useron.exempt&FLAG('V')))
				useron.number=0;
		}
	}

	if(!online) 
		return(false); 

	if(stricmp(terminal,"sexpots")==0) {	/* dial-up connection (via SexPOTS) */
		SAFEPRINTF2(str,"%s connection detected at %lu bps", terminal, cur_rate);
		logline("@S",str);
		node_connection = (ushort)cur_rate;
		SAFEPRINTF(connection,"%lu",cur_rate);
		SAFECOPY(cid,"Unknown");
		SAFECOPY(client_name,"Unknown");
		if(telnet_location[0]) {			/* Caller-ID info provided */
			SAFEPRINTF(str, "CID: %s", telnet_location);
			logline("@*",str);
			SAFECOPY(cid,telnet_location);
			truncstr(cid," ");				/* Only include phone number in CID */
			char* p=telnet_location;
			FIND_WHITESPACE(p);
			SKIP_WHITESPACE(p);
			if(*p) {
				SAFECOPY(client_name,p);	/* CID name, if provided (maybe 'P' or 'O' if private or out-of-area) */
			}
		}
		SAFECOPY(client.addr,cid);
		SAFECOPY(client.host,client_name);
		client_on(client_socket,&client,TRUE /* update */);
	} else {
		if(telnet_location[0]) {			/* Telnet Location info provided */
			SAFEPRINTF(str, "Telnet Location: %s", telnet_location);
			logline("@*",str);
		}
	}


	useron.misc&=~TERM_FLAGS;
	useron.misc|=autoterm;
	SAFECOPY(useron.comp,client_name);

	if(!useron.number && rlogin_name[0]!=0 && !(cfg.sys_misc&SM_CLOSED) && !matchuser(&cfg, rlogin_name, /* Sysop alias: */FALSE)) {
		lprintf(LOG_INFO,"Node %d UNKNOWN %s-specified username: '%s', starting new user signup",cfg.node_num,client.protocol,rlogin_name);
		bprintf("%s: %s\r\n", text[UNKNOWN_USER], rlogin_name);
		newuser();
	}

	if(!useron.number) {	/* manual/regular logon */

		/* Display ANSWER screen */
		rioctl(IOSM|PAUSE);
		sys_status|=SS_PAUSEON;
		SAFEPRINTF(str,"%sanswer",cfg.text_dir);
		SAFEPRINTF(path,"%s.rip",str);
		if((autoterm&RIP) && fexistcase(path))
			printfile(path,P_NOABORT);
		else {
			SAFEPRINTF(path,"%s.html",str);
			if((autoterm&HTML) && fexistcase(path))
				printfile(path,P_NOABORT);
			else {
				SAFEPRINTF(path,"%s.ans",str);
				if((autoterm&ANSI) && fexistcase(path))
					printfile(path,P_NOABORT);
				else {
					SAFEPRINTF(path,"%s.asc",str);
					if(fexistcase(path))
						printfile(path, P_NOABORT);
				}
			}
		}
		sys_status&=~SS_PAUSEON;
		exec_bin(cfg.login_mod,&main_csi);
	} else	/* auto logon here */
		if(logon()==false)
			return(false);


	if(!useron.number)
		hangup();

	/* Save the IP to the user's note */
	if(cid[0]) {
		SAFECOPY(useron.ipaddr,cid);
		putuserrec(&cfg,useron.number,U_IPADDR,LEN_IPADDR,useron.ipaddr);
	}

	/* Save host name to the user's computer description */
	if(client_name[0]) {
		SAFECOPY(useron.comp,client_name);
		putuserrec(&cfg,useron.number,U_COMP,LEN_COMP,useron.comp);
	}

	if(!online) 
		return(false); 

	if(!(sys_status&SS_USERON)) {
		errormsg(WHERE,ERR_CHK,"User not logged on",0);
		hangup();
		return(false); 
	}

	if(useron.pass[0])
		loginSuccess(startup->login_attempt_list, &client_addr);

	return(true);
}
