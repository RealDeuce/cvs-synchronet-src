/* getmsg.cpp */

/* Synchronet message retrieval functions */

/* $Id: getmsg.cpp,v 1.50 2015/12/04 08:48:22 rswindell Exp $ */

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

/***********************************************************************/
/* Functions that do i/o with messages (posts/mail/auto) or their data */
/***********************************************************************/

#include "sbbs.h"

/****************************************************************************/
/* Loads an SMB message from the open msg base the fastest way possible 	*/
/* first by offset, and if that's the wrong message, then by number.        */
/* Returns 1 if the message was loaded and left locked, otherwise			*/
/* !WARNING!: If you're going to write the msg index back to disk, you must */
/* Call this function with a msg->idx.offset of 0 (so msg->offset will be	*/
/* initialized correctly)													*/
/****************************************************************************/
int sbbs_t::loadmsg(smbmsg_t *msg, ulong number)
{
	char str[128];
	int i;

	if(msg->idx.offset) {				/* Load by offset if specified */

		if((i=smb_lockmsghdr(&smb,msg))!=0) {
			errormsg(WHERE,ERR_LOCK,smb.file,i,smb.last_error);
			return(0); 
		}

		i=smb_getmsghdr(&smb,msg);
		if(i==SMB_SUCCESS) {
			if(msg->hdr.number==number)
				return(1);
			/* Wrong offset  */
			smb_freemsgmem(msg);
		}

		smb_unlockmsghdr(&smb,msg); 
	}

	msg->hdr.number=number;
	if((i=smb_getmsgidx(&smb,msg))!=SMB_SUCCESS)				 /* Message is deleted */
		return(0);
	if((i=smb_lockmsghdr(&smb,msg))!=SMB_SUCCESS) {
		errormsg(WHERE,ERR_LOCK,smb.file,i,smb.last_error);
		return(0); 
	}
	if((i=smb_getmsghdr(&smb,msg))!=SMB_SUCCESS) {
		SAFEPRINTF4(str,"(%06"PRIX32") #%"PRIu32"/%lu %s",msg->idx.offset,msg->idx.number
			,number,smb.file);
		smb_unlockmsghdr(&smb,msg);
		errormsg(WHERE,ERR_READ,str,i,smb.last_error);
		return(0); 
	}
	return(msg->total_hfields);
}


void sbbs_t::show_msgattr(ushort attr)
{

	bprintf(text[MsgAttr]
		,attr&MSG_PRIVATE	? "Private  "   :nulstr
		,attr&MSG_READ		? "Read  "      :nulstr
		,attr&MSG_DELETE	? "Deleted  "   :nulstr
		,attr&MSG_KILLREAD	? "Kill  "      :nulstr
		,attr&MSG_ANONYMOUS ? "Anonymous  " :nulstr
		,attr&MSG_LOCKED	? "Locked  "    :nulstr
		,attr&MSG_PERMANENT ? "Permanent  " :nulstr
		,attr&MSG_MODERATED ? "Moderated  " :nulstr
		,attr&MSG_VALIDATED ? "Validated  " :nulstr
		,attr&MSG_REPLIED	? "Replied  "	:nulstr
		,attr&MSG_NOREPLY	? "NoReply  "	:nulstr
		,nulstr
		,nulstr
		,nulstr
		,nulstr
		,nulstr
		);
}

/****************************************************************************/
/* Displays a message header to the screen                                  */
/****************************************************************************/
void sbbs_t::show_msghdr(smbmsg_t* msg)
{
	char	str[MAX_PATH+1];
	char	age[64];
	char	*sender=NULL;
	int 	i;

	attr(LIGHTGRAY);
	if(useron.misc&CLRSCRN)
		outchar(FF);
	else
		CRLF;

	SAFEPRINTF(str,"%smenu/msghdr.*", cfg.text_dir);
	if(fexist(str)) {
		menu("msghdr");
		return; 
	}

	bprintf(text[MsgSubj],msg->subj);
	if(msg->hdr.attr)
		show_msgattr(msg->hdr.attr);

	bprintf(text[MsgTo],msg->to);
	if(msg->to_net.addr)
		bprintf(text[MsgToNet],smb_netaddrstr(&msg->to_net,str));
	if(msg->to_ext)
		bprintf(text[MsgToExt],msg->to_ext);
	if(!(msg->hdr.attr&MSG_ANONYMOUS) || SYSOP) {
		bprintf(text[MsgFrom],msg->from);
		if(msg->from_ext)
			bprintf(text[MsgFromExt],msg->from_ext);
		if(msg->from_net.addr && !strchr(msg->from,'@'))
			bprintf(text[MsgFromNet],smb_netaddrstr(&msg->from_net,str)); 
	}
	bprintf(text[MsgDate]
		,timestr(msg->hdr.when_written.time)
		,smb_zonestr(msg->hdr.when_written.zone,NULL)
		,age_of_posted_item(age, sizeof(age), msg->hdr.when_written.time - (smb_tzutc(msg->hdr.when_written.zone) * 60)));

	CRLF;

	for(i=0;i<msg->total_hfields;i++) {
		if(msg->hfield[i].type==SENDER)
			sender=(char *)msg->hfield_dat[i];
		if(msg->hfield[i].type==FORWARDED && sender)
			bprintf(text[ForwardedFrom],sender
				,timestr(*(time32_t *)msg->hfield_dat[i])); 
	}
	CRLF;
}

/****************************************************************************/
/* Displays message header and text (if not deleted)                        */
/****************************************************************************/
void sbbs_t::show_msg(smbmsg_t* msg, long mode)
{
	char*	text;

	show_msghdr(msg);

	if((text=smb_getmsgtxt(&smb,msg,(console&CON_RAW_IN) ? 0:GETMSGTXT_PLAIN)) != NULL) {
		if(!(console&CON_RAW_IN))
			mode|=P_WORDWRAP;
		putmsg(text, mode);
		smb_freemsgtxt(text);
	}
	if((text=smb_getmsgtxt(&smb,msg,GETMSGTXT_TAIL_ONLY))!=NULL) {
		putmsg(text, mode&(~P_WORDWRAP));
		smb_freemsgtxt(text);
	}
}

/****************************************************************************/
/* Writes message header and text data to a text file						*/
/****************************************************************************/
void sbbs_t::msgtotxt(smbmsg_t* msg, char *str, int header, int tails)
{
	char	*buf;
	char	tmp[128];
	int 	i;
	FILE	*out;

	if((out=fnopen(&i,str,O_WRONLY|O_CREAT|O_APPEND))==NULL) {
		errormsg(WHERE,ERR_OPEN,str,0);
		return; 
	}
	if(header) {
		fprintf(out,"\r\n");
		fprintf(out,"Subj : %s\r\n",msg->subj);
		fprintf(out,"To   : %s",msg->to);
		if(msg->to_ext)
			fprintf(out," #%s",msg->to_ext);
		if(msg->to_net.addr)
			fprintf(out," (%s)",smb_netaddrstr(&msg->to_net,tmp));
		fprintf(out,"\r\nFrom : %s",msg->from);
		if(msg->from_ext && !(msg->hdr.attr&MSG_ANONYMOUS))
			fprintf(out," #%s",msg->from_ext);
		if(msg->from_net.addr)
			fprintf(out," (%s)",smb_netaddrstr(&msg->from_net,tmp));
		fprintf(out,"\r\nDate : %.24s %s"
			,timestr(msg->hdr.when_written.time)
			,smb_zonestr(msg->hdr.when_written.zone,NULL));
		fprintf(out,"\r\n\r\n"); 
	}

	buf=smb_getmsgtxt(&smb,msg,tails);
	if(buf!=NULL) {
		strip_invalid_attr(buf);
		fputs(buf,out);
		smb_freemsgtxt(buf); 
	} else if(smb_getmsgdatlen(msg)>2)
		errormsg(WHERE,ERR_READ,smb.file,smb_getmsgdatlen(msg));
	fclose(out);
}

/****************************************************************************/
/* Returns message number posted at or after time							*/
/****************************************************************************/
ulong sbbs_t::getmsgnum(uint subnum, time_t t)
{
    int			i;
	smb_t		smb;
	idxrec_t	idx;

	if(!t)
		return(0);

	ZERO_VAR(smb);
	SAFEPRINTF2(smb.file,"%s%s",cfg.sub[subnum]->data_dir,cfg.sub[subnum]->code);
	smb.retry_time=cfg.smb_retry_time;
	smb.subnum=subnum;
	if((i=smb_open(&smb)) != SMB_SUCCESS) {
		errormsg(WHERE,ERR_OPEN,smb.file,i,smb.last_error);
		return(0); 
	}
	smb_getmsgidx_by_time(&smb, &idx, t);

	smb_close(&smb);
	return idx.number;
}

/****************************************************************************/
/* Returns the time of the message number pointed to by 'ptr'               */
/****************************************************************************/
time_t sbbs_t::getmsgtime(uint subnum, ulong ptr)
{
	int 		i;
	smb_t		smb;
	smbmsg_t	msg;
	idxrec_t	lastidx;

	ZERO_VAR(smb);
	SAFEPRINTF2(smb.file,"%s%s",cfg.sub[subnum]->data_dir,cfg.sub[subnum]->code);
	smb.retry_time=cfg.smb_retry_time;
	smb.subnum=subnum;
	if((i=smb_open(&smb))!=0) {
		errormsg(WHERE,ERR_OPEN,smb.file,i,smb.last_error);
		return(0); 
	}
	if(!filelength(fileno(smb.sid_fp))) {			/* Empty base */
		smb_close(&smb);
		return(0); 
	}
	msg.offset=0;
	msg.hdr.number=0;
	if(smb_getmsgidx(&smb,&msg)) {				/* Get first message index */
		smb_close(&smb);
		return(0); 
	}
	if(!ptr || msg.idx.number>=ptr) {			/* ptr is before first message */
		smb_close(&smb);
		return(msg.idx.time);   				/* so return time of first msg */
	}

	if(smb_getlastidx(&smb,&lastidx)) { 			 /* Get last message index */
		smb_close(&smb);
		return(0); 
	}
	if(lastidx.number<ptr) {					/* ptr is after last message */
		smb_close(&smb);
		return(lastidx.time);	 				/* so return time of last msg */
	}

	msg.idx.time=0;
	msg.hdr.number=ptr;
	if(!smb_getmsgidx(&smb,&msg)) {
		smb_close(&smb);
		return(msg.idx.time); 
	}

	if(ptr-msg.idx.number < lastidx.number-ptr) {
		msg.offset=0;
		msg.idx.number=0;
		while(msg.idx.number<ptr) {
			msg.hdr.number=0;
			if(smb_getmsgidx(&smb,&msg) || msg.idx.number>=ptr)
				break;
			msg.offset++; 
		}
		smb_close(&smb);
		return(msg.idx.time); 
	}

	ptr--;
	while(ptr) {
		msg.hdr.number=ptr;
		if(!smb_getmsgidx(&smb,&msg))
			break;
		ptr--; 
	}
	smb_close(&smb);
	return(msg.idx.time);
}


/****************************************************************************/
/* Returns the total number of msgs in the sub-board and sets 'ptr' to the  */
/* number of the last message in the sub (0) if no messages.				*/
/****************************************************************************/
ulong sbbs_t::getlastmsg(uint subnum, uint32_t *ptr, time_t *t)
{
	int 		i;
	ulong		total;
	smb_t		smb;
	idxrec_t	idx;

	if(ptr)
		(*ptr)=0;
	if(t)
		(*t)=0;
	if(subnum>=cfg.total_subs)
		return(0);

	ZERO_VAR(smb);
	SAFEPRINTF2(smb.file,"%s%s",cfg.sub[subnum]->data_dir,cfg.sub[subnum]->code);
	smb.retry_time=cfg.smb_retry_time;
	smb.subnum=subnum;
	if((i=smb_open(&smb))!=0) {
		errormsg(WHERE,ERR_OPEN,smb.file,i,smb.last_error);
		return(0); 
	}

	if(!filelength(fileno(smb.sid_fp))) {			/* Empty base */
		smb_close(&smb);
		return(0); 
	}
	if((i=smb_locksmbhdr(&smb))!=0) {
		smb_close(&smb);
		errormsg(WHERE,ERR_LOCK,smb.file,i,smb.last_error);
		return(0); 
	}
	if((i=smb_getlastidx(&smb,&idx))!=0) {
		smb_close(&smb);
		errormsg(WHERE,ERR_READ,smb.file,i,smb.last_error);
		return(0); 
	}
	total=(long)filelength(fileno(smb.sid_fp))/sizeof(idxrec_t);
	smb_unlocksmbhdr(&smb);
	smb_close(&smb);
	if(ptr)
		(*ptr)=idx.number;
	if(t)
		(*t)=idx.time;
	return(total);
}

