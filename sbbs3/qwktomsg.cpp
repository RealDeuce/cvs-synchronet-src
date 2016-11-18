/* qwktomsg.cpp */

/* Synchronet QWK to SMB message conversion routine */

/* $Id: qwktomsg.cpp,v 1.61 2016/11/18 09:58:14 rswindell Exp $ */

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

#include "sbbs.h"
#include "qwk.h"

static void qwk_parse_header_list(ulong confnum, smbmsg_t* msg, str_list_t* headers, bool parse_sender_hfields, bool parse_recipient_hfields)
{
	char*		p;
	char		zone[32];
	char		value[INI_MAX_VALUE_LEN+1];
	int			i;
	uint16_t	net_type;
	uint16_t	hfield_type;

	if((p=iniPopKey(headers,ROOT_SECTION,"WhenWritten",value))!=NULL) {
		xpDateTime_t dt=isoDateTimeStr_parse(p);

		msg->hdr.when_written.time=(uint32_t)xpDateTime_to_localtime(dt);
		msg->hdr.when_written.zone=dt.zone;
		sscanf(p,"%*s %s",zone);
		if(zone[0])
			msg->hdr.when_written.zone=(ushort)strtoul(zone,NULL,16);
	}

	if((p=iniPopKey(headers,ROOT_SECTION,smb_hfieldtype(hfield_type=RECIPIENT),value))!=NULL) {
		if(parse_recipient_hfields)
			smb_hfield_str(msg,hfield_type,p);		
	}

	/* Recipient net address and type */
	if((p=iniPopKey(headers,ROOT_SECTION,smb_hfieldtype(hfield_type=RECIPIENTNETADDR),value))!=NULL) {
		if(confnum==0 && parse_recipient_hfields) {
			net_type=smb_get_net_type_by_addr(p);
			if(smb_hfield_netaddr(msg,hfield_type,p,&net_type) == SMB_SUCCESS)
				smb_hfield_bin(msg,RECIPIENTNETTYPE,net_type);
		}
	}

	if((p=iniPopKey(headers,ROOT_SECTION,smb_hfieldtype(hfield_type=SUBJECT),value))!=NULL)
		smb_hfield_str(msg,hfield_type,p);

	if((p=iniPopKey(headers,ROOT_SECTION,smb_hfieldtype(hfield_type=SENDER),value))!=NULL) {
		if(parse_sender_hfields)
			smb_hfield_str(msg,hfield_type,p);
	}

	if((p=iniPopKey(headers,ROOT_SECTION,smb_hfieldtype(hfield_type=SENDERNETADDR),value))!=NULL) {
		if(parse_sender_hfields) {
//			smb_hfield_str(msg,hfield_type,p);	this appears to be unnecessary
			net_type=smb_get_net_type_by_addr(p);
			if(smb_hfield_netaddr(msg,hfield_type,p,&net_type) == SMB_SUCCESS)
				smb_hfield_bin(msg,SENDERNETTYPE,net_type);
		}
	}

	if((p=iniPopKey(headers,ROOT_SECTION,smb_hfieldtype(hfield_type=RFC822MSGID),value))!=NULL)
		smb_hfield_str(msg,hfield_type,p);

	if((p=iniPopKey(headers,ROOT_SECTION,smb_hfieldtype(hfield_type=RFC822REPLYID),value))!=NULL)
		smb_hfield_str(msg,hfield_type,p);

	if((p=iniPopKey(headers,ROOT_SECTION,smb_hfieldtype(hfield_type=RFC822REPLYTO),value))!=NULL)
		smb_hfield_str(msg,hfield_type,p);

	/* Trace header fields */
	while((p=iniPopKey(headers,ROOT_SECTION,smb_hfieldtype(hfield_type=SENDERIPADDR),value))!=NULL) {
		if(parse_sender_hfields)
			smb_hfield_str(msg,hfield_type,p);
	}
	while((p=iniPopKey(headers,ROOT_SECTION,smb_hfieldtype(hfield_type=SENDERHOSTNAME),value))!=NULL) {
		if(parse_sender_hfields)
			smb_hfield_str(msg,hfield_type,p);
	}
	while((p=iniPopKey(headers,ROOT_SECTION,smb_hfieldtype(hfield_type=SENDERPROTOCOL),value))!=NULL) {
		if(parse_sender_hfields)
			smb_hfield_str(msg,hfield_type,p);
	}
	while((p=iniPopKey(headers,ROOT_SECTION,smb_hfieldtype(hfield_type=SENDERORG),value))!=NULL) {
		if(parse_sender_hfields)
			smb_hfield_str(msg,hfield_type,p);
	}

	/* FidoNet header fields */
	while((p=iniPopKey(headers,ROOT_SECTION,smb_hfieldtype(hfield_type=FIDOAREA),value))!=NULL)
		smb_hfield_str(msg,hfield_type,p);
	while((p=iniPopKey(headers,ROOT_SECTION,smb_hfieldtype(hfield_type=FIDOSEENBY),value))!=NULL)
		smb_hfield_str(msg,hfield_type,p);
	while((p=iniPopKey(headers,ROOT_SECTION,smb_hfieldtype(hfield_type=FIDOPATH),value))!=NULL)
		smb_hfield_str(msg,hfield_type,p);
	while((p=iniPopKey(headers,ROOT_SECTION,smb_hfieldtype(hfield_type=FIDOMSGID),value))!=NULL)
		smb_hfield_str(msg,hfield_type,p);
	while((p=iniPopKey(headers,ROOT_SECTION,smb_hfieldtype(hfield_type=FIDOREPLYID),value))!=NULL)
		smb_hfield_str(msg,hfield_type,p);
	while((p=iniPopKey(headers,ROOT_SECTION,smb_hfieldtype(hfield_type=FIDOPID),value))!=NULL)
		smb_hfield_str(msg,hfield_type,p);
	while((p=iniPopKey(headers,ROOT_SECTION,smb_hfieldtype(hfield_type=FIDOFLAGS),value))!=NULL)
		smb_hfield_str(msg,hfield_type,p);
	while((p=iniPopKey(headers,ROOT_SECTION,smb_hfieldtype(hfield_type=FIDOTID),value))!=NULL)
		smb_hfield_str(msg,hfield_type,p);
	while((p=iniPopKey(headers,ROOT_SECTION,smb_hfieldtype(hfield_type=FIDOCTRL),value))!=NULL)
		smb_hfield_str(msg,hfield_type,p);

	/* Synchronet */
	while((p=iniPopKey(headers,ROOT_SECTION,smb_hfieldtype(hfield_type=SMB_EDITOR),value))!=NULL)
		smb_hfield_str(msg,hfield_type,p);

	/* USENET */
	while((p=iniPopKey(headers,ROOT_SECTION,smb_hfieldtype(hfield_type=USENETPATH),value))!=NULL)
		smb_hfield_str(msg,hfield_type,p);
	while((p=iniPopKey(headers,ROOT_SECTION,smb_hfieldtype(hfield_type=USENETNEWSGROUPS),value))!=NULL)
		smb_hfield_str(msg,hfield_type,p);

	/* Others (RFC-822) */
	for(i=0;(*headers)[i]!=NULL;i++)
		if((*headers)[i][0])
			smb_hfield_str(msg,RFC822HEADER,(*headers)[i]);
}

void sbbs_t::qwk_new_msg(ulong confnum, smbmsg_t* msg, char* hdrblk, long offset, str_list_t all_headers, bool parse_sender_hfields)
{
	char str[128];
	char to[128];
	str_list_t	msg_headers;

	smb_freemsgmem(msg);

	sprintf(str,"%lx",offset);
	msg_headers=iniGetSection(all_headers,str);

	memset(msg,0,sizeof(smbmsg_t));		/* Initialize message header */
	msg->hdr.version=smb_ver();

	sprintf(to,"%25.25s",(char *)hdrblk+21);     /* To user */
	truncsp(to);

	if(msg_headers!=NULL)
		qwk_parse_header_list(confnum, msg, &msg_headers, parse_sender_hfields, stricmp(to,"NETMAIL")!=0);

	/* Parse the QWK message header: */
	if(msg->hdr.when_written.time==0) {
		struct		tm tm;
		memset(&tm,0,sizeof(tm));
		tm.tm_mon = ((hdrblk[8]&0xf)*10)+(hdrblk[9]&0xf);
		tm.tm_mday=((hdrblk[11]&0xf)*10)+(hdrblk[12]&0xf);
		tm.tm_year=((hdrblk[14]&0xf)*10)+(hdrblk[15]&0xf);
		if(tm.tm_year<Y2K_2DIGIT_WINDOW)
			tm.tm_year+=100;
		tm.tm_hour=((hdrblk[16]&0xf)*10)+(hdrblk[17]&0xf);
		tm.tm_min=((hdrblk[19]&0xf)*10)+(hdrblk[20]&0xf);

		msg->hdr.when_written.time=(uint32_t)sane_mktime(&tm);
	}

	if(msg->to==NULL)
		smb_hfield_str(msg,RECIPIENT,to);

	if(parse_sender_hfields && msg->from==NULL) {
		sprintf(str,"%25.25s",hdrblk+46);  
		truncsp(str);
		smb_hfield_str(msg,SENDER,str);
	}

	if(msg->subj==NULL) {
		sprintf(str,"%25.25s",hdrblk+71);   /* Subject */
		truncsp(str);
		smb_hfield_str(msg,SUBJECT,str);
	}

	iniFreeStringList(msg_headers);
}

/****************************************************************************/
/* Converts a QWK message packet into a message.							*/
/* Does *not* free the msgmem												*/
/****************************************************************************/
bool sbbs_t::qwk_import_msg(FILE *qwk_fp, char *hdrblk, ulong blocks
							,char fromhub, uint subnum
							,uint touser, smbmsg_t* msg)
{
	char*		body;
	char*		tail;
	char*		qwkbuf;
	char		str[256],col=0,lastch=0,*p;
	char		from[128];
	uint 		i,k;
	long		bodylen,taillen;
	bool		success=false;
	uint16_t	net_type;
	ushort		xlat=XLAT_NONE;
	int			storage=SMB_SELFPACK;
	long		dupechk_hashes=SMB_HASH_SOURCE_DUPE;
	str_list_t	kludges;

	if(subnum!=INVALID_SUB
		&& (hdrblk[0]=='*' || hdrblk[0]=='+' || cfg.sub[subnum]->misc&SUB_PONLY))
		msg->hdr.attr|=MSG_PRIVATE;
	if(subnum!=INVALID_SUB && cfg.sub[subnum]->misc&SUB_AONLY)
		msg->hdr.attr|=MSG_ANONYMOUS;
	if(subnum==INVALID_SUB && cfg.sys_misc&SM_DELREADM)
		msg->hdr.attr|=MSG_KILLREAD;
	if((fromhub || useron.rest&FLAG('Q')) &&
		(hdrblk[0]=='*' || hdrblk[0]=='-' || hdrblk[0]=='`'))
		msg->hdr.attr|=MSG_READ;

	if(subnum!=INVALID_SUB && !fromhub && cfg.sub[subnum]->mod_ar[0]
		&& chk_ar(cfg.sub[subnum]->mod_ar,&useron,&client))
		msg->hdr.attr|=MSG_MODERATED;
	if(subnum!=INVALID_SUB && !fromhub && cfg.sub[subnum]->misc&SUB_SYSPERM
		&& sub_op(subnum))
		msg->hdr.attr|=MSG_PERMANENT;

	if(!(useron.rest&FLAG('Q')) && !fromhub && msg->hdr.when_written.zone==0)
		msg->hdr.when_written.zone=sys_timezone(&cfg);

	msg->hdr.when_imported.time=time32(NULL);
	msg->hdr.when_imported.zone=sys_timezone(&cfg);

	hdrblk[116]=0;	// don't include number of blocks in "re: msg number"
	if(!(useron.rest&FLAG('Q')) && !fromhub)
		msg->hdr.thread_back=atol((char *)hdrblk+108);

	if(subnum==INVALID_SUB) { 		/* E-mail */
		if(cfg.sys_misc&SM_FASTMAIL)
			storage=SMB_FASTALLOC;

		/* duplicate message-IDs must be allowed in mail database */
		dupechk_hashes&=~(1<<SMB_HASH_SOURCE_MSG_ID);

		sprintf(str,"%u",touser);
		smb_hfield_str(msg,RECIPIENTEXT,str); 
	} else {
		if(cfg.sub[subnum]->misc&SUB_HYPER)
			storage = SMB_HYPERALLOC;
		else if(cfg.sub[subnum]->misc&SUB_FAST)
			storage = SMB_FASTALLOC;

		if(cfg.sub[subnum]->misc&SUB_LZH)
			xlat=XLAT_LZH;
	}

	/********************************/
	/* Convert the QWK message text */
	/********************************/

	if((qwkbuf=(char *)malloc((blocks-1)*QWK_BLOCK_LEN))==NULL) {
		errormsg(WHERE,ERR_ALLOC,"QWK msg buf",(blocks-1)*QWK_BLOCK_LEN);
		return(false); 
	}

	if(fread(qwkbuf,QWK_BLOCK_LEN,blocks-1,qwk_fp) != blocks-1) {
		free(qwkbuf);
		errormsg(WHERE,ERR_READ,"QWK msg blocks",(blocks-1)*QWK_BLOCK_LEN);
	}

	bodylen=0;
	if((body=(char *)malloc((blocks-1L)*QWK_BLOCK_LEN*2L))==NULL) {
		free(qwkbuf);
		errormsg(WHERE,ERR_ALLOC,"QWK msg body",(blocks-1L)*QWK_BLOCK_LEN*2L);
		return(false); 
	}

	taillen=0;
	if((tail=(char *)malloc((blocks-1L)*QWK_BLOCK_LEN*2L))==NULL) {
		free(qwkbuf);
		free(body);
		errormsg(WHERE,ERR_ALLOC,"QWK msg tail",(blocks-1L)*QWK_BLOCK_LEN*2L);
		return(false); 
	}

	kludges=strListInit();

	for(k=0;k<(blocks-1)*QWK_BLOCK_LEN;k++) {
		if(qwkbuf[k]==0)
			continue;
		if(bodylen==0 
			&& (qwkbuf[k]=='@' 
				|| (((useron.qwk&QWK_EXT) || subnum==INVALID_SUB)
					&& (strnicmp(qwkbuf+k,"To:",3)==0 
					||  strnicmp(qwkbuf+k,"From:",5)==0 
					||  strnicmp(qwkbuf+k,"Subject:",8)==0)))) {
			if((p=strchr(qwkbuf+k, QWK_NEWLINE))==NULL) {
				body[bodylen++]=qwkbuf[k];
				continue;
			}
			*p=0;	/* Converts QWK_NEWLINE to NUL */
			strListPush(&kludges, qwkbuf+k);
			k+=strlen(qwkbuf+k);
			continue;
		}
		if(!taillen && qwkbuf[k]==' ' && col==3 && bodylen>=3
			&& body[bodylen-3]=='-' && body[bodylen-2]=='-'
			&& body[bodylen-1]=='-') {
			bodylen-=3;
			strcpy(tail,"--- ");	/* DO NOT USE SAFECOPY */
			taillen=4;
			col++;
			continue; 
		}
		if(qwkbuf[k]==QWK_NEWLINE) {		/* expand QWK_NEWLINE to crlf */
			if(!bodylen && !taillen)		/* Ignore blank lines at top of message */
				continue;
			if(!taillen && col==3 && bodylen>=3 && body[bodylen-3]=='-'
				&& body[bodylen-2]=='-' && body[bodylen-1]=='-') {
				bodylen-=3;
				strcpy(tail,"---");	/* DO NOT USE SAFECOPY */
				taillen=3; 
			}
			col=0;
			if(taillen) {
				tail[taillen++]=CR;
				tail[taillen++]=LF; 
			}
			else {
				body[bodylen++]=CR;
				body[bodylen++]=LF; 
			}
			continue; 
		}
		/* beep restrict */
		if(!fromhub && qwkbuf[k]==BEL && useron.rest&FLAG('B'))   
			continue;
		/* ANSI restriction */
		if(!fromhub && (qwkbuf[k]==CTRL_A || qwkbuf[k]==ESC)
			&& useron.rest&FLAG('A'))
			continue;
		if(qwkbuf[k]!=CTRL_A && lastch!=CTRL_A)
			col++;
		if(lastch==CTRL_A && !valid_ctrl_a_code(qwkbuf[k])) {
			if(taillen) taillen--;
			else		bodylen--;
			lastch=0;
			continue; 
		}
		lastch=qwkbuf[k];
		if(taillen)
			tail[taillen++]=qwkbuf[k];
		else
			body[bodylen++]=qwkbuf[k]; 
	} 
	free(qwkbuf);

	while(bodylen && body[bodylen-1]==' ') bodylen--; /* remove trailing spaces */
	if(bodylen>=2 && body[bodylen-2]==CR && body[bodylen-1]==LF)
		bodylen-=2;

	while(taillen && tail[taillen-1]<=' ') taillen--; /* remove trailing garbage */

	/* Parse QWK Kludges (QWKE standard and SyncQNET legacy) here: */
	if(useron.rest&FLAG('Q') || fromhub) {      /* QWK Net */
		if((msg->from_net.type == NET_QWK && (p=(char*)msg->from_net.addr) != NULL)
			|| (p=iniGetString(kludges,ROOT_SECTION,"@VIA",NULL,NULL)) != NULL) {
			if(!fromhub && p != msg->from_net.addr)
				set_qwk_flag(QWK_VIA);
			if(route_circ(p,cfg.sys_id)) {
				bprintf("\r\nCircular message path: %s\r\n",p);
				lprintf(LOG_ERR,"Circular message path: %s from %s"
					,p,fromhub ? cfg.qhub[fromhub-1]->id:useron.alias);
				strListFree(&kludges);
				free(body);
				free(tail);
				return(false); 
			}
			SAFEPRINTF2(str,"%s/%s"
				,fromhub ? cfg.qhub[fromhub-1]->id : useron.alias,p);
			strupr(str);
			update_qwkroute(str); 
		}
		else {
			if(fromhub)
				SAFECOPY(str,cfg.qhub[fromhub-1]->id);
			else
				SAFECOPY(str,useron.alias); 
		}
		/* From network type & address: */
		strupr(str);
		net_type=NET_QWK;
		smb_hfield_netaddr(msg, SENDERNETADDR, str, &net_type);
		smb_hfield_bin(msg,SENDERNETTYPE,net_type);
	} else {
		sprintf(str,"%u",useron.number);
		smb_hfield_str(msg,SENDEREXT,str);
		if((uint)subnum!=INVALID_SUB && cfg.sub[subnum]->misc&SUB_NAME)
			SAFECOPY(from,useron.name);
		else
			SAFECOPY(from,useron.alias);
		smb_hfield_str(msg,SENDER,from);
	}
	if((p=iniGetString(kludges,ROOT_SECTION,"@MSGID",NULL,NULL)) != NULL) {
		if(!fromhub)
			set_qwk_flag(QWK_MSGID);
		truncstr(p," ");				/* Truncate at first space char */
		if(msg->id==NULL)
			smb_hfield_str(msg,RFC822MSGID,p);
	}
	if((p=iniGetString(kludges,ROOT_SECTION,"@REPLY",NULL,NULL)) != NULL) {
		if(!fromhub)
			set_qwk_flag(QWK_MSGID);
		truncstr(p," ");				/* Truncate at first space char */
		if(msg->reply_id==NULL)
			smb_hfield_str(msg,RFC822REPLYID,p);
	}
	if((p=iniGetString(kludges,ROOT_SECTION,"@TZ",NULL,NULL)) != NULL) {
		if(!fromhub)
			set_qwk_flag(QWK_TZ);
		msg->hdr.when_written.zone=(short)ahtoul(p); 
	}
	if((p=iniGetString(kludges,ROOT_SECTION,"@REPLYTO",NULL,NULL)) != NULL) {
		if(msg->replyto==NULL)
			smb_hfield_str(msg,REPLYTO,p);
	}
	/* QWKE standard: */
	if((p=iniGetString(kludges,ROOT_SECTION,"Subject",NULL,NULL)) != NULL)
		smb_hfield_replace_str(msg,SUBJECT,p);
	if((p=iniGetString(kludges,ROOT_SECTION,"To",NULL,NULL)) != NULL)
		smb_hfield_replace_str(msg,RECIPIENT,p);
	/* Don't use the From: kludge, for security reasons */

	strListFree(&kludges);

	/* smb_addmsg requires ASCIIZ strings */
	body[bodylen]=0;
	tail[taillen]=0;

	if(online==ON_REMOTE)
		bputs(text[WritingIndx]);

	if(smb.status.max_crcs==0)	/* no CRC checking means no body text dupe checking */
		dupechk_hashes&=~(1<<SMB_HASH_SOURCE_BODY);

	if((i=smb_addmsg(&smb,msg,storage,dupechk_hashes,xlat,(uchar*)body,(uchar*)tail))==SMB_SUCCESS)
		success=true;
	else if(i==SMB_DUPE_MSG) {
		bprintf("\r\n!%s\r\n",smb.last_error);
		if(!fromhub) {
			if(subnum==INVALID_SUB) {
				SAFEPRINTF2(str,"%s duplicate e-mail attempt (%s)",useron.alias,smb.last_error);
				logline(LOG_NOTICE,"E!",str); 
			} else {
				SAFEPRINTF4(str,"%s duplicate message attempt in %s %s (%s)"
					,useron.alias
					,cfg.grp[cfg.sub[subnum]->grp]->sname
					,cfg.sub[subnum]->lname
					,smb.last_error);
				logline(LOG_NOTICE,"P!",str); 
			}
		}
	}
	else 
		errormsg(WHERE,ERR_WRITE,smb.file,i,smb.last_error);

	free(body);
	free(tail);

	return(success);
}

