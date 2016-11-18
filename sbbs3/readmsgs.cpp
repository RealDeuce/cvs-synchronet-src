/* Synchronet public message reading function */

/* $Id: readmsgs.cpp,v 1.89 2016/11/18 09:58:14 rswindell Exp $ */

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

int sbbs_t::sub_op(uint subnum)
{
	return(is_user_subop(&cfg, subnum, &useron, &client));
}

uchar sbbs_t::msg_listing_flag(uint subnum, smbmsg_t* msg, post_t* post)
{
	if(msg->hdr.attr&MSG_DELETE)						return '-';
	if((stricmp(msg->to,useron.alias)==0 || stricmp(msg->to,useron.name)==0)
		&& !(msg->hdr.attr&MSG_READ))					return '!';
	if(msg->hdr.attr&MSG_PERMANENT)						return 'p';
	if(msg->hdr.attr&MSG_LOCKED)						return 'L';
	if(msg->hdr.attr&MSG_KILLREAD)						return 'K';
	if(msg->hdr.attr&MSG_NOREPLY)						return '#';
	if(msg->hdr.number > subscan[subnum].ptr)			return '*';
	if(msg->hdr.attr&MSG_PRIVATE)						return 'P';
	if(msg->hdr.attr&MSG_POLL)							return '?'; 
	if(post->upvotes > post->downvotes)					return 251;
	if(post->upvotes || post->downvotes)				return 'v';
	if(msg->hdr.attr&MSG_REPLIED)						return 'R';
	if(sub_op(subnum) && msg->hdr.attr&MSG_ANONYMOUS)	return 'A';
	return ' ';
}

long sbbs_t::listmsgs(uint subnum, long mode, post_t *post, long i, long posts)
{
	smbmsg_t msg;
	long listed=0;

	for(;i<posts && !msgabort();i++) {
		if(mode&SCAN_NEW && post[i].idx.number<=subscan[subnum].ptr)
			continue;
		msg.idx.offset=post[i].idx.offset;
		if(loadmsg(&msg,post[i].idx.number) < 0)
			break;
		smb_unlockmsghdr(&smb,&msg);
		if(listed==0)
			bputs(text[MailOnSystemLstHdr]);
		bprintf(text[SubMsgLstFmt],post[i].num
			,msg.hdr.attr&MSG_ANONYMOUS && !sub_op(subnum)
			? text[Anonymous] : msg.from
			,msg.to
			,msg_listing_flag(subnum, &msg, &post[i])
			,msg.subj);
		smb_freemsgmem(&msg);
		msg.total_hfields=0;
		listed++;
	}

	return(listed);
}

char *binstr(uchar *buf, ushort length, char* str)
{
	char tmp[128];
	int i;

	str[0]=0;
	for(i=0;i<length;i++)
		if(buf[i] && (buf[i]<' ' || buf[i]>=0x7f)
			&& buf[i]!='\r' && buf[i]!='\n' && buf[i]!='\t')
			break;
	if(i==length)		/* not binary */
		return((char*)buf);
	for(i=0;i<length;i++) {
		sprintf(tmp,"%02X ",buf[i]);
		strcat(str,tmp); 
		if(i >= 100) {
			strcat(str,"...");
			break;
		}
	}
	return(str);
}


void sbbs_t::msghdr(smbmsg_t* msg)
{
	int i;
	char str[512];

	CRLF;

	/* variable fields */
	for(i=0;i<msg->total_hfields;i++) {
		char *p;
		bprintf("%-16.16s ",smb_hfieldtype(msg->hfield[i].type));
		switch(msg->hfield[i].type) {
			case SENDERNETTYPE:
			case RECIPIENTNETTYPE:
			case REPLYTONETTYPE:
				p = smb_nettype((enum smb_net_type)*(uint16_t*)msg->hfield_dat[i]);
				break;
			default:
				p = binstr((uchar *)msg->hfield_dat[i],msg->hfield[i].length,str);
				break;
		}
		bprintf("%s\r\n", p);
	}

	/* fixed fields */
	bprintf("%-16.16s %08lX %04hX %.24s %s\r\n","when_written"	
		,msg->hdr.when_written.time, msg->hdr.when_written.zone
		,timestr(msg->hdr.when_written.time)
		,smb_zonestr(msg->hdr.when_written.zone,NULL));
	bprintf("%-16.16s %08lX %04hX %.24s %s\r\n","when_imported"	
		,msg->hdr.when_imported.time, msg->hdr.when_imported.zone
		,timestr(msg->hdr.when_imported.time)
		,smb_zonestr(msg->hdr.when_imported.zone,NULL));
	bprintf("%-16.16s %04Xh\r\n","type"				,msg->hdr.type);
	bprintf("%-16.16s %04Xh\r\n","version"			,msg->hdr.version);
	bprintf("%-16.16s %04Xh\r\n","attr"				,msg->hdr.attr);
	bprintf("%-16.16s %08lXh\r\n","auxattr"			,msg->hdr.auxattr);
	bprintf("%-16.16s %08lXh\r\n","netattr"			,msg->hdr.netattr);
	bprintf("%-16.16s %ld\r\n"	 ,"number"			,msg->hdr.number);
	bprintf("%-16.16s %06lXh\r\n","header offset"	,msg->idx.offset);
	bprintf("%-16.16s %u\r\n"	 ,"header length"	,msg->hdr.length);

	/* optional fixed fields */
	if(msg->hdr.thread_id)
		bprintf("%-16.16s %ld\r\n"	,"thread_id"		,msg->hdr.thread_id);
	if(msg->hdr.thread_back)
		bprintf("%-16.16s %ld\r\n"	,"thread_back"		,msg->hdr.thread_back);
	if(msg->hdr.thread_next)
		bprintf("%-16.16s %ld\r\n"	,"thread_next"		,msg->hdr.thread_next);
	if(msg->hdr.thread_first)
		bprintf("%-16.16s %ld\r\n"	,"thread_first"		,msg->hdr.thread_first);
	if(msg->hdr.delivery_attempts)
		bprintf("%-16.16s %hu\r\n"	,"delivery_attempts",msg->hdr.delivery_attempts);
	if(msg->hdr.times_downloaded)
		bprintf("%-16.16s %lu\r\n"	,"times_downloaded"	,msg->hdr.times_downloaded);
	if(msg->hdr.last_downloaded)
		bprintf("%-16.16s %s\r\n"	,"last_downloaded"	,timestr(msg->hdr.last_downloaded));

	/* convenience integers */
	if(msg->expiration)
		bprintf("%-16.16s %s\r\n"	,"expiration"	
			,timestr(msg->expiration));
	if(msg->priority)
		bprintf("%-16.16s %lu\r\n"	,"priority"			,msg->priority);
	if(msg->cost)
		bprintf("%-16.16s %lu\r\n"	,"cost"				,msg->cost);

	bprintf("%-16.16s %06lXh\r\n"	,"data offset"		,msg->hdr.offset);
	for(i=0;i<msg->hdr.total_dfields;i++)
		bprintf("data field[%u]    %s, offset %lu, length %lu\r\n"
				,i
				,smb_dfieldtype(msg->dfield[i].type)
				,msg->dfield[i].offset
				,msg->dfield[i].length);
}

/****************************************************************************/
/* posts is the actual number of posts returned in the allocated array		*/
/* visible is the number of visible posts to the user (not all included in	*/
/* returned array)															*/
/****************************************************************************/
post_t * sbbs_t::loadposts(uint32_t *posts, uint subnum, ulong ptr, long mode, ulong *unvalidated_num, uint32_t* visible)
{
	char name[128];
	ushort aliascrc,namecrc,sysop;
	int i,skip;
	ulong l=0,total,alloc_len;
	uint32_t	curmsg=0;
	smbmsg_t	msg;
	idxrec_t	idx;
	post_t *	post;

	if(posts==NULL)
		return(NULL);

	(*posts)=0;

	if((i=smb_locksmbhdr(&smb))!=0) {				/* Be sure noone deletes or */
		errormsg(WHERE,ERR_LOCK,smb.file,i,smb.last_error);		/* adds while we're reading */
		return(NULL); 
	}

	total=(long)filelength(fileno(smb.sid_fp))/sizeof(idxrec_t); /* total msgs in sub */

	if(!total) {			/* empty */
		smb_unlocksmbhdr(&smb);
		return(NULL); 
	}

	strcpy(name,useron.name);
	strlwr(name);
	namecrc=crc16(name,0);
	strcpy(name,useron.alias);
	strlwr(name);
	aliascrc=crc16(name,0);
	sysop=crc16("sysop",0);

	rewind(smb.sid_fp);

	alloc_len=sizeof(post_t)*total;
	if((post=(post_t *)malloc(alloc_len))==NULL) {	/* alloc for max */
		smb_unlocksmbhdr(&smb);
		errormsg(WHERE,ERR_ALLOC,smb.file,alloc_len);
		return(NULL); 
	}
	memset(post, 0, alloc_len);

	if(unvalidated_num)
		*unvalidated_num=ULONG_MAX;

	while(!feof(smb.sid_fp)) {
		skip=0;
		if(smb_fread(&smb, &idx,sizeof(idx),smb.sid_fp) != sizeof(idx))
			break;

		if(idx.number==0)	/* invalid message number, ignore */
			continue;

		if(idx.attr&MSG_DELETE) {		/* Pre-flagged */
			if(mode&LP_REP) 			/* Don't include deleted msgs in REP pkt */
				continue;
			if(!(cfg.sys_misc&SM_SYSVDELM)) /* Noone can view deleted msgs */
				continue;
			if(!(cfg.sys_misc&SM_USRVDELM)	/* Users can't view deleted msgs */
				&& !sub_op(subnum)) 	/* not sub-op */
				continue;
			if(!sub_op(subnum)			/* not sub-op */
				&& idx.from!=namecrc && idx.from!=aliascrc) /* not for you */
				continue; 
		}

		if(idx.attr&MSG_MODERATED && !(idx.attr&MSG_VALIDATED)) {
			if(mode&LP_REP || !sub_op(subnum))
				break;
		}
		
		switch(idx.attr&MSG_POLL_VOTE_MASK) {
		case MSG_VOTE:
		case MSG_UPVOTE:
		case MSG_DOWNVOTE:
		{
			ulong u;
			for(u = 0; u < l; u++)
				if(post[u].idx.number == idx.remsg)
					break;
			if(u < l) {
				switch(idx.attr&MSG_VOTE) {
				case MSG_UPVOTE:
					post[u].upvotes++;
					break;
				case MSG_DOWNVOTE:
					post[u].downvotes++;
					break;
				default:
					for(int b=0; b < 16; b++) {
						if(idx.votes&(1<<b))
							post[u].votes[b]++;
					}
				}
			}
			if(!(mode&LP_VOTES))
				continue;
			break;
		}
		case MSG_POLL:
			if(!(mode&LP_POLLS))
				continue;
			break;
		case MSG_POLL_CLOSURE:
			if(!(mode&LP_VOTES))
				continue;
			break;
		}

		if(idx.attr&MSG_PRIVATE && !(mode&LP_PRIVATE)
			&& !sub_op(subnum) && !(useron.rest&FLAG('Q'))) {
			if(idx.to!=namecrc && idx.from!=namecrc
				&& idx.to!=aliascrc && idx.from!=aliascrc
				&& (useron.number!=1 || idx.to!=sysop))
				continue;
			if(!smb_lockmsghdr(&smb,&msg)) {
				if(!smb_getmsghdr(&smb,&msg)) {
					if(stricmp(msg.to,useron.alias)
						&& stricmp(msg.from,useron.alias)
						&& stricmp(msg.to,useron.name)
						&& stricmp(msg.from,useron.name)
						&& (useron.number!=1 || stricmp(msg.to,"sysop")
						|| msg.from_net.type))
						skip=1;
					smb_freemsgmem(&msg); 
				}
				smb_unlockmsghdr(&smb,&msg); 
			}
			if(skip)
				continue; 
		}

		curmsg++;

		if(idx.number<=ptr)
			continue;

		if(idx.attr&MSG_READ && mode&LP_UNREAD) /* Skip read messages */
			continue;

		if(!(mode&LP_BYSELF) && (idx.from==namecrc || idx.from==aliascrc)) {
			msg.idx=idx;
			if(!smb_lockmsghdr(&smb,&msg)) {
				if(!smb_getmsghdr(&smb,&msg)) {
					if(!stricmp(msg.from,useron.alias)
						|| !stricmp(msg.from,useron.name))
						skip=1;
					smb_freemsgmem(&msg); 
				}
				smb_unlockmsghdr(&smb,&msg); 
			}
			if(skip)
				continue; 
		}

		if(!(mode&LP_OTHERS)) {
			if(idx.to!=namecrc && idx.to!=aliascrc
				&& (useron.number!=1 || idx.to!=sysop))
				continue;
			msg.idx=idx;
			if(!smb_lockmsghdr(&smb,&msg)) {
				if(!smb_getmsghdr(&smb,&msg)) {
					if(stricmp(msg.to,useron.alias) && stricmp(msg.to,useron.name)
						&& (useron.number!=1 || stricmp(msg.to,"sysop")
						|| msg.from_net.type))
						skip=1;
					smb_freemsgmem(&msg); 
				}
				smb_unlockmsghdr(&smb,&msg); 
			}
			if(skip)
				continue; 
		}

		if(idx.attr&MSG_MODERATED && !(idx.attr&MSG_VALIDATED)) {
			if(unvalidated_num && *unvalidated_num > l)
				*unvalidated_num=l;
		}

		memcpy(&post[l].idx,&idx,sizeof(idx));
		post[l].num = curmsg;
		l++;
	}
	smb_unlocksmbhdr(&smb);
	if(!l)
		FREE_AND_NULL(post);

	if(visible!=NULL)	/* Total number of currently visible/readable messages to the user */
		*visible=curmsg;
	(*posts)=l;
	return(post);
}

static int64_t get_start_msg(sbbs_t* sbbs, smb_t* smb)
{
	uint32_t	j=smb->curmsg+1;
	int64_t		i;

	if(j<smb->msgs)
		j++;
	else
		j=1;
	sbbs->bprintf(sbbs->text[StartWithN],j);
	if((i=sbbs->getnum(smb->msgs))<0)
		return(i);
	if(i==0)
		return(j-1);
	return(i-1);
}

/****************************************************************************/
/* Reads posts on subboard sub. 'mode' determines new-posts only, browse,   */
/* or continuous read.                                                      */
/* Returns 0 if normal completion, 1 if aborted.                            */
/* Called from function main_sec                                            */
/****************************************************************************/
int sbbs_t::scanposts(uint subnum, long mode, const char *find)
{
	char	str[256],str2[256],do_find=true,mismatches=0
			,done=0,domsg=1,*buf,*p;
	char	subj[128];
	char	find_buf[128];
	char	tmp[128];
	int		i;
	int64_t	i64;
	int		quit=0;
	uint 	usub,ugrp,reads=0;
	uint	lp=0;
	long	org_mode=mode;
	ulong	msgs,l,unvalidated;
	uint32_t last;
	uint32_t u;
	post_t	*post;
	smbmsg_t	msg;

	cursubnum=subnum;	/* for ARS */
	if(cfg.scanposts_mod[0] && !scanposts_inside) {
		char cmdline[256];

		scanposts_inside = true;
		safe_snprintf(cmdline, sizeof(cmdline), "%s %s %u %s", cfg.scanposts_mod, cfg.sub[subnum]->code, mode, find);
		i=exec_bin(cmdline, &main_csi);
		scanposts_inside = false;
		return i;
	}
	find_buf[0]=0;
	if(!chk_ar(cfg.sub[subnum]->read_ar,&useron,&client)) {
		bprintf(text[CantReadSub]
				,cfg.grp[cfg.sub[subnum]->grp]->sname,cfg.sub[subnum]->sname);
		return(0); 
	}
	msg.total_hfields=0;				/* init to NULL, specify not-allocated */
	if(!(mode&SCAN_CONST))
		lncntr=0;
	if((msgs=getlastmsg(subnum,&last,0))==0) {
		if(mode&(SCAN_NEW|SCAN_TOYOU))
			bprintf(text[NScanStatusFmt]
				,cfg.grp[cfg.sub[subnum]->grp]->sname,cfg.sub[subnum]->lname,0L,0L);
		else
			bprintf(text[NoMsgsOnSub]
				,cfg.grp[cfg.sub[subnum]->grp]->sname,cfg.sub[subnum]->sname);
		return(0); 
	}
	if(mode&SCAN_NEW && subscan[subnum].ptr>=last && !(mode&SCAN_BACK)) {
		if(subscan[subnum].ptr>last)
			subscan[subnum].ptr=last;
		if(subscan[subnum].last>last)
			subscan[subnum].last=last;
		bprintf(text[NScanStatusFmt]
			,cfg.grp[cfg.sub[subnum]->grp]->sname,cfg.sub[subnum]->lname,0L,msgs);
		return(0); 
	}

	if((i=smb_stack(&smb,SMB_STACK_PUSH))!=0) {
		errormsg(WHERE,ERR_OPEN,cfg.sub[subnum]->code,i);
		return(0); 
	}
	sprintf(smb.file,"%s%s",cfg.sub[subnum]->data_dir,cfg.sub[subnum]->code);
	smb.retry_time=cfg.smb_retry_time;
	smb.subnum=subnum;
	if((i=smb_open(&smb))!=0) {
		errormsg(WHERE,ERR_OPEN,smb.file,i,smb.last_error);
		smb_stack(&smb,SMB_STACK_POP);
		return(0); 
	}

	if(!(mode&SCAN_TOYOU)
		&& (!mode || mode&SCAN_FIND || !(subscan[subnum].cfg&SUB_CFG_YSCAN)))
		lp=LP_BYSELF|LP_OTHERS;
	if(mode&SCAN_TOYOU && mode&SCAN_UNREAD)
		lp|=LP_UNREAD;
	if(!(cfg.sub[subnum]->misc&SUB_NOVOTING))
		lp|=LP_POLLS;
	post=loadposts(&smb.msgs,subnum,0,lp,&unvalidated);
	if(mode&SCAN_NEW) { 		  /* Scanning for new messages */
		for(smb.curmsg=0;smb.curmsg<smb.msgs;smb.curmsg++)
			if(subscan[subnum].ptr<post[smb.curmsg].idx.number)
				break;
		bprintf(text[NScanStatusFmt]
			,cfg.grp[cfg.sub[subnum]->grp]->sname,cfg.sub[subnum]->lname,smb.msgs-smb.curmsg,msgs);
		if(!smb.msgs) {		  /* no messages at all */
			smb_close(&smb);
			smb_stack(&smb,SMB_STACK_POP);
			return(0); 
		}
		if(smb.curmsg==smb.msgs) {  /* no new messages */
			if(!(mode&SCAN_BACK)) {
				if(post)
					free(post);
				smb_close(&smb);
				smb_stack(&smb,SMB_STACK_POP);
				return(0); 
			}
			smb.curmsg=smb.msgs-1; 
		} 
	}
	else {
		if(mode&SCAN_TOYOU)
			bprintf(text[NScanStatusFmt]
			   ,cfg.grp[cfg.sub[subnum]->grp]->sname,cfg.sub[subnum]->lname,smb.msgs,msgs);
		if(!smb.msgs) {
			if(!(mode&SCAN_TOYOU))
				bprintf(text[NoMsgsOnSub]
					,cfg.grp[cfg.sub[subnum]->grp]->sname,cfg.sub[subnum]->sname);
			smb_close(&smb);
			smb_stack(&smb,SMB_STACK_POP);
			return(0); 
		}
		if(mode&SCAN_FIND) {
			bprintf(text[SearchSubFmt]
				,cfg.grp[cfg.sub[subnum]->grp]->sname,cfg.sub[subnum]->lname,smb.msgs);
			domsg=1;
			smb.curmsg=0; 
		}
		else if(mode&SCAN_TOYOU)
			smb.curmsg=0;
		else {
			for(smb.curmsg=0;smb.curmsg<smb.msgs;smb.curmsg++)
				if(post[smb.curmsg].idx.number>=subscan[subnum].last)
					break;
			if(smb.curmsg==smb.msgs)
				smb.curmsg=smb.msgs-1;

			domsg=1; 
		} 
	}

	if(useron.misc&RIP)
		menu("msgscan");

	if((i=smb_locksmbhdr(&smb))!=0) {
		smb_close(&smb);
		errormsg(WHERE,ERR_LOCK,smb.file,i,smb.last_error);
		smb_stack(&smb,SMB_STACK_POP);
		return(0); 
	}
	if((i=smb_getstatus(&smb))!=0) {
		smb_close(&smb);
		errormsg(WHERE,ERR_READ,smb.file,i,smb.last_error);
		smb_stack(&smb,SMB_STACK_POP);
		return(0); 
	}
	smb_unlocksmbhdr(&smb);
	last=smb.status.last_msg;

	action=NODE_RMSG;
	if(mode&SCAN_CONST) {   /* update action */
		getnodedat(cfg.node_num,&thisnode,1);
		thisnode.action=NODE_RMSG;
		putnodedat(cfg.node_num,&thisnode); 
	}
	current_msg=&msg;	/* For MSG_* @-codes and bbs.msg_* property values */
	while(online && !done) {

		action=NODE_RMSG;

		if(mode&(SCAN_CONST|SCAN_FIND) && sys_status&SS_ABORT)
			break;

		if(post==NULL)	/* Been unloaded */
			post=loadposts(&smb.msgs,subnum,0,lp,&unvalidated);   /* So re-load */

		if(!smb.msgs) {
			done=1;
			continue; 
		}

		while(smb.curmsg>=smb.msgs) smb.curmsg--;

		for(ugrp=0;ugrp<usrgrps;ugrp++)
			if(usrgrp[ugrp]==cfg.sub[subnum]->grp)
				break;
		for(usub=0;usub<usrsubs[ugrp];usub++)
			if(usrsub[ugrp][usub]==subnum)
				break;
		usub++;
		ugrp++;

		msg.idx=post[smb.curmsg].idx;

		if((i=smb_locksmbhdr(&smb))!=0) {
			errormsg(WHERE,ERR_LOCK,smb.file,i,smb.last_error);
			break; 
		}
		if((i=smb_getstatus(&smb))!=0) {
			smb_unlocksmbhdr(&smb);
			errormsg(WHERE,ERR_READ,smb.file,i,smb.last_error);
			break; 
		}
		smb_unlocksmbhdr(&smb);

		if(smb.status.last_msg!=last) { 	/* New messages */
			last=smb.status.last_msg;
			if(post) {
				free((void *)post); 
			}
			post=loadposts(&smb.msgs,subnum,0,lp,&unvalidated);   /* So re-load */
			if(!smb.msgs)
				break;
			for(smb.curmsg=0;smb.curmsg<smb.msgs;smb.curmsg++)
				if(post[smb.curmsg].idx.number==msg.idx.number)
					break;
			if(smb.curmsg>(smb.msgs-1))
				smb.curmsg=(smb.msgs-1);
			continue; 
		}

		if(msg.total_hfields)
			smb_freemsgmem(&msg);
		msg.total_hfields=0;

		if(loadmsg(&msg,post[smb.curmsg].idx.number) < 0) {
			if(mismatches>5) {	/* We can't do this too many times in a row */
				errormsg(WHERE,ERR_CHK,smb.file,post[smb.curmsg].idx.number);
				break; 
			}
			if(post)
				free(post);
			post=loadposts(&smb.msgs,subnum,0,lp,&unvalidated);
			if(!smb.msgs)
				break;
			if(smb.curmsg>(smb.msgs-1))
				smb.curmsg=(smb.msgs-1);
			mismatches++;
			continue; 
		}
		smb_unlockmsghdr(&smb,&msg);

		mismatches=0;

		if(domsg && !(sys_status&SS_ABORT)) {

			if(do_find && mode&SCAN_FIND) { 			/* Find text in messages */
				buf=smb_getmsgtxt(&smb,&msg,GETMSGTXT_ALL);
				if(!buf) {
					if(smb.curmsg<smb.msgs-1) 
						smb.curmsg++;
					else if(org_mode&SCAN_FIND)  
						done=1;
					else if(smb.curmsg>=smb.msgs-1)
							domsg=0;
					continue; 
				}
				strupr(buf);
				strip_ctrl(buf, buf);
				SAFECOPY(subj,msg.subj);
				strupr(subj);
				if(!strstr(buf,find) && !strstr(subj,find)) {
					free(buf);
					if(smb.curmsg<smb.msgs-1) 
						smb.curmsg++;
					else if(org_mode&SCAN_FIND) 
							done=1;
					else if(smb.curmsg>=smb.msgs-1)
							domsg=0;
					continue; 
				}
				free(buf); 
			}

			if(mode&SCAN_CONST)
				bprintf(text[ZScanPostHdr],ugrp,usub,smb.curmsg+1,smb.msgs);

			if(!reads && mode)
				CRLF;

			msg.upvotes = post[smb.curmsg].upvotes;
			msg.downvotes = post[smb.curmsg].downvotes;
			msg.total_votes = total_votes(&post[smb.curmsg]);
			show_msg(&msg
				,msg.from_ext && !strcmp(msg.from_ext,"1") && !msg.from_net.type
					? 0:P_NOATCODES
				,&post[smb.curmsg]);

			reads++;	/* number of messages actually read during this sub-scan */

			/* Message is to this user and hasn't been read, so flag as read */
			if((!stricmp(msg.to,useron.name) || !stricmp(msg.to,useron.alias)
				|| (useron.number==1 && !stricmp(msg.to,"sysop")
				&& !msg.from_net.type))
				&& !(msg.hdr.attr&MSG_READ)) {
				if(msg.total_hfields)
					smb_freemsgmem(&msg);
				msg.total_hfields=0;
				msg.idx.offset=0;
				if(!smb_locksmbhdr(&smb)) { 			  /* Lock the entire base */
					if(loadmsg(&msg,msg.idx.number) >= 0) {
						msg.hdr.attr|=MSG_READ;
						msg.idx.attr=msg.hdr.attr;
						if((i=smb_putmsg(&smb,&msg))!=0)
							errormsg(WHERE,ERR_WRITE,smb.file,i,smb.last_error);
						smb_unlockmsghdr(&smb,&msg); 
					}
					smb_unlocksmbhdr(&smb); 
				}
				if(!msg.total_hfields) {				/* unsuccessful reload */
					domsg=0;
					continue; 
				} 
			}

			subscan[subnum].last=post[smb.curmsg].idx.number;

			if(subscan[subnum].ptr<post[smb.curmsg].idx.number && !(mode&SCAN_TOYOU)) {
				posts_read++;
				subscan[subnum].ptr=post[smb.curmsg].idx.number; 
			} 

			if(sub_op(subnum) && (msg.hdr.attr&(MSG_MODERATED|MSG_VALIDATED)) == MSG_MODERATED) {
				uint16_t msg_attr = msg.hdr.attr;
				SAFEPRINTF2(str,text[ValidatePostQ],smb.curmsg+1,msg.subj);
				if(!noyes(str))
					msg_attr|=MSG_VALIDATED;
				else {
					SAFEPRINTF2(str,text[DeletePostQ],smb.curmsg+1,msg.subj);
					if(yesno(str))
						msg_attr|=MSG_DELETE;
				}
				if(msg_attr!=msg.hdr.attr) {
					if(msg.total_hfields)
						smb_freemsgmem(&msg);
					msg.total_hfields=0;
					msg.idx.offset=0;
					if(!smb_locksmbhdr(&smb)) { 			  /* Lock the entire base */
						if(loadmsg(&msg,msg.idx.number) >= 0) {
							msg.hdr.attr=msg.idx.attr=msg_attr;
							if((i=smb_putmsg(&smb,&msg))!=0)
								errormsg(WHERE,ERR_WRITE,smb.file,i,smb.last_error);
							smb_unlockmsghdr(&smb,&msg); 
						}
						smb_unlocksmbhdr(&smb); 
					}
					if(msg_attr & MSG_DELETE) {
						if(cfg.sys_misc&SM_SYSVDELM)
							domsg=0;	// If you can view deleted messages, don't redisplay.
					}
					else {
						domsg=0;		// If you just validated, don't redisplay.
					}
					if(post)
						free(post);
					post=loadposts(&smb.msgs,subnum,0,lp,&unvalidated);
					if(!smb.msgs)
						break;
					if(smb.curmsg>(smb.msgs-1))
						smb.curmsg=(smb.msgs-1);
					mismatches++;
					continue; 
				}
			}
		}
		else domsg=1;
		if(mode&SCAN_CONST) {
			if(smb.curmsg<smb.msgs-1) smb.curmsg++;
				else done=1;
			continue; 
		}
		if(useron.misc&WIP)
			menu("msgscan");
		ASYNC;
		if(unvalidated < smb.curmsg)
			bprintf(text[UnvalidatedWarning],unvalidated+1);
		bprintf(text[ReadingSub],ugrp,cfg.grp[cfg.sub[subnum]->grp]->sname
			,usub,cfg.sub[subnum]->sname,smb.curmsg+1,smb.msgs);
		sprintf(str,"ABCDEFILMNPQRTUVY?<>[]{}-+()");
		if(sub_op(subnum))
			strcat(str,"O");
		do_find=true;
		l=getkeys(str,smb.msgs);
		if(l&0x80000000L) {
			if((long)l==-1) { /* ctrl-c */
				quit=1;
				break; 
			}
			smb.curmsg=(l&~0x80000000L)-1;
			do_find=false;
			continue; 
		}
		switch(l) {
			case 'A':   
			case 'R':   
				if((char)l==(cfg.sys_misc&SM_RA_EMU ? 'A' : 'R')) { 
					do_find=false;	/* re-read last message */
					break;
				}
				/* Reply to last message */
				domsg=0;
				if(!chk_ar(cfg.sub[subnum]->post_ar,&useron,&client)) {
					bputs(text[CantPostOnSub]);
					break; 
				}
				if(msg.hdr.attr&MSG_NOREPLY && !sub_op(subnum)) {
					bputs(text[CantReplyToMsg]);
					break; 
				}
				quotemsg(&msg,/* include tails: */FALSE);
				FREE_AND_NULL(post);
				postmsg(subnum,&msg,WM_QUOTE);
				if(mode&SCAN_TOYOU)
					domsg=1;
				break;
			case 'B':   /* Skip sub-board */
				if(mode&SCAN_NEW && text[RemoveFromNewScanQ][0] && !noyes(text[RemoveFromNewScanQ]))
					subscan[subnum].cfg&=~SUB_CFG_NSCAN;
				if(msg.total_hfields)
					smb_freemsgmem(&msg);
				if(post)
					free(post);
				smb_close(&smb);
				smb_stack(&smb,SMB_STACK_POP);
				current_msg=NULL;
				return(0);
			case 'C':   /* Continuous */
				mode|=SCAN_CONST;
				if(smb.curmsg<smb.msgs-1) smb.curmsg++;
				else done=1;
				break;
			case 'D':       /* Delete message on sub-board */
				if(!(msg.hdr.attr&MSG_DELETE) && msg.hdr.type == SMB_MSG_TYPE_POLL 
					&& smb_msg_is_from(&msg, cfg.sub[subnum]->misc&SUB_NAME ? useron.name : useron.alias, NET_NONE, NULL)
					&& !(msg.hdr.auxattr&POLL_CLOSED)) {
					if(noyes("Close Poll")) {
						domsg=false;
						break;
					}
					i=closepoll(&cfg, &smb, msg.hdr.number, cfg.sub[subnum]->misc&SUB_NAME ? useron.name : useron.alias);
					if(i != SMB_SUCCESS)
						errormsg(WHERE,ERR_WRITE,smb.file,i,smb.last_error);
					break;
				}
				domsg=0;
				if(!sub_op(subnum)) {
					if(!(cfg.sub[subnum]->misc&SUB_DEL)) {
						bputs(text[CantDeletePosts]);
						domsg=0;
						break; 
					}
					if(cfg.sub[subnum]->misc&SUB_DELLAST && smb.curmsg!=(smb.msgs-1)) {
						bputs(text[CantDeleteMsg]);
						domsg=0;
						break;
					}
					if(stricmp(cfg.sub[subnum]->misc&SUB_NAME
						? useron.name : useron.alias, msg.from)
					&& stricmp(cfg.sub[subnum]->misc&SUB_NAME
						? useron.name : useron.alias, msg.to)) {
						bprintf(text[YouDidntPostMsgN],smb.curmsg+1);
						break; 
					} 
				}
				if(msg.hdr.attr&MSG_PERMANENT) {
					bputs(text[CantDeleteMsg]);
					domsg=0;
					break; 
				}

				FREE_AND_NULL(post);

				if(msg.total_hfields)
					smb_freemsgmem(&msg);
				msg.total_hfields=0;
				msg.idx.offset=0;
				if(smb_locksmbhdr(&smb)==SMB_SUCCESS) {	/* Lock the entire base */
					if(loadmsg(&msg,msg.idx.number) >=0 ) {
						msg.idx.attr^=MSG_DELETE;
						msg.hdr.attr=msg.idx.attr;
						if((i=smb_putmsg(&smb,&msg))!=0)
							errormsg(WHERE,ERR_WRITE,smb.file,i,smb.last_error);
						smb_unlockmsghdr(&smb,&msg);
						if(i==0 && msg.idx.attr&MSG_DELETE) {
							sprintf(str,"%s removed post from %s %s"
								,useron.alias
								,cfg.grp[cfg.sub[subnum]->grp]->sname,cfg.sub[subnum]->lname);
							logline("P-",str);
							if(!stricmp(cfg.sub[subnum]->misc&SUB_NAME
								? useron.name : useron.alias, msg.from))
								useron.posts=(ushort)adjustuserrec(&cfg,useron.number
									,U_POSTS,5,-1); 
						} 
					}
					smb_unlocksmbhdr(&smb);
				}
				domsg=1;
				if((cfg.sys_misc&SM_SYSVDELM		// anyone can view delete msgs
					|| (cfg.sys_misc&SM_USRVDELM	// sys/subops can view deleted msgs
					&& sub_op(subnum)))
					&& smb.curmsg<smb.msgs-1)
					smb.curmsg++;
				if(smb.curmsg>=smb.msgs)
					done=1;
				break;
			case 'E':   /* edit last post */
				if(!sub_op(subnum)) {
					if(!(cfg.sub[subnum]->misc&SUB_EDIT)) {
						bputs(text[CantEditMsg]);
						domsg=0;
						break; 
					}
					if(cfg.sub[subnum]->misc&SUB_EDITLAST && smb.curmsg!=(smb.msgs-1)) {
						bputs(text[CantEditMsg]);
						domsg=0;
						break;
					}
					if(stricmp(cfg.sub[subnum]->misc&SUB_NAME
						? useron.name : useron.alias, msg.from)) {
						bprintf(text[YouDidntPostMsgN],smb.curmsg+1);
						domsg=0;
						break; 
					} 
				}
				FREE_AND_NULL(post);
				editmsg(&msg,subnum);
				break;
			case 'F':   /* find text in messages */
				domsg=0;
				mode&=~SCAN_FIND;	/* turn off find mode */
				if((i64=get_start_msg(this,&smb))<0)
					break;
				i=(int)i64;
				bputs(text[SearchStringPrompt]);
				if(!getstr(find_buf,40,K_LINE|K_UPPER|K_EDIT|K_AUTODEL))
					break;
				if(text[DisplaySubjectsOnlyQ][0] && yesno(text[DisplaySubjectsOnlyQ]))
					searchposts(subnum,post,(long)i,smb.msgs,find_buf);
				else {
					smb.curmsg=i;
					find=find_buf;
					mode|=SCAN_FIND;
					domsg=1;
				}
				break;
			case 'I':   /* Sub-board information */
				domsg=0;
				subinfo(subnum);
				break;
			case 'L':   /* List messages */
				domsg=0;
				if((i64=get_start_msg(this,&smb))<0)
					break;
				i=(int)i64;
				listmsgs(subnum,0,post,i,smb.msgs);
				sys_status&=~SS_ABORT;
				break;
			case 'N':	/* New messages */
				domsg=0;
				if(!listmsgs(subnum,SCAN_NEW,post,0,smb.msgs))
					bputs(text[NoMessagesFound]);
				sys_status&=~SS_ABORT;
				break;
			case 'M':   /* Reply to last post in mail */
				domsg=0;
				if(msg.hdr.attr&(MSG_NOREPLY|MSG_ANONYMOUS) && !sub_op(subnum)) {
					bputs(text[CantReplyToMsg]);
					break; 
				}
				if(!sub_op(subnum) && msg.hdr.attr&MSG_PRIVATE
					&& stricmp(msg.to,useron.name)
					&& stricmp(msg.to,useron.alias))
					break;
				sprintf(str2,text[Regarding]
					,msg.subj
					,timestr(msg.hdr.when_written.time));
				if(msg.from_net.addr==NULL)
					SAFECOPY(str,msg.from);
				else if(msg.from_net.type==NET_FIDO)
					SAFEPRINTF2(str,"%s@%s",msg.from
						,smb_faddrtoa((faddr_t *)msg.from_net.addr,tmp));
				else if(msg.from_net.type==NET_INTERNET || strchr((char*)msg.from_net.addr,'@')!=NULL) {
					if(msg.replyto_net.type==NET_INTERNET)
						SAFECOPY(str,(char *)msg.replyto_net.addr);
					else
						SAFECOPY(str,(char *)msg.from_net.addr);
				}
				else
					SAFEPRINTF2(str,"%s@%s",msg.from,(char *)msg.from_net.addr);
				bputs(text[Email]);
				if(!getstr(str,60,K_EDIT|K_AUTODEL))
					break;

				FREE_AND_NULL(post);
				quotemsg(&msg,/* include tails: */TRUE);
				if(smb_netaddr_type(str)==NET_INTERNET)
					inetmail(str,msg.subj,WM_QUOTE|WM_NETMAIL);
				else {
					p=strrchr(str,'@');
					if(p)								/* FidoNet or QWKnet */
						netmail(str,msg.subj,WM_QUOTE);
					else {
						i=atoi(str);
						if(!i) {
							if(cfg.sub[subnum]->misc&SUB_NAME)
								i=userdatdupe(0,U_NAME,LEN_NAME,str);
							else
								i=matchuser(&cfg,str,TRUE /* sysop_alias */); 
						}
						email(i,str2,msg.subj,WM_EMAIL|WM_QUOTE); 
					} 
				}
				break;
			case 'P':   /* Post message on sub-board */
				domsg=0;
				if(!chk_ar(cfg.sub[subnum]->post_ar,&useron,&client))
					bputs(text[CantPostOnSub]);
				else {
					FREE_AND_NULL(post);
					postmsg(subnum,0,0);
				}
				break;
			case 'Q':   /* Quit */
				quit=1;
				done=1;
				break;
			case 'T':   /* List titles of next ten messages */
				domsg=0;
				if(!smb.msgs)
					break;
				if(smb.curmsg>=smb.msgs-1) {
					 done=1;
					 break; 
				}
				u=smb.curmsg+11;
				if(u>smb.msgs)
					u=smb.msgs;
				listmsgs(subnum,0,post,smb.curmsg+1,u);
				smb.curmsg=u-1;
				if(subscan[subnum].ptr<post[smb.curmsg].idx.number)
					subscan[subnum].ptr=post[smb.curmsg].idx.number;
				break;
			case 'Y':   /* Your messages */
				domsg=0;
				if(!showposts_toyou(subnum, post,0,smb.msgs))
					bputs(text[NoMessagesFound]);
				break;
			case 'U':   /* Your unread messages */
				domsg=0;
				if(!showposts_toyou(subnum, post,0,smb.msgs, SCAN_UNREAD))
					bputs(text[NoMessagesFound]);
				break;
			case 'V':	/* Vote in reply to message */
			{
				smbmsg_t vote;
				const char* notice=NULL;

				if(cfg.sub[subnum]->misc&SUB_NOVOTING) {
					bputs(text[VotingNotAllowed]);
					domsg = false;
					break;
				}

				if(smb_voted_already(&smb, msg.hdr.number
					,cfg.sub[subnum]->misc&SUB_NAME ? useron.name : useron.alias, NET_NONE, NULL)) {
					bputs(text[VotedAlready]);
					domsg = false;
					break;
				}

				if(useron.rest&FLAG('V')) {
					bputs(text[R_Voting]);
					domsg = false;
					break;
				}

				if(msg.hdr.auxattr&POLL_CLOSED) {
					bputs(text[CantReplyToMsg]);
					domsg = false;
					break; 
				}

				ZERO_VAR(vote);
				if(msg.hdr.type == SMB_MSG_TYPE_POLL) {
					unsigned answers=0;
					for(i=0; i<msg.total_hfields; i++) {
						if(msg.hfield[i].type != SMB_POLL_ANSWER)
							continue;
						uselect(1, answers++, msg.subj, (char*)msg.hfield_dat[i], NULL);
					}
					i = uselect(0, 0, NULL, NULL, NULL);
					if(i < 0) {
						domsg = false;
						break;
					}
					vote.hdr.votes = (1<<i);
					vote.hdr.attr = MSG_VOTE;
					notice = text[PollVoteNotice];
				} else {
					mnemonics(text[VoteMsgUpDownOrQuit]);
					long cmd = getkeys("UDQ", 0);
					if(cmd != 'U' && cmd != 'D')
						break;
					vote.hdr.attr = (cmd == 'U' ? MSG_UPVOTE : MSG_DOWNVOTE);
					notice = text[vote.hdr.attr&MSG_UPVOTE ? MsgUpVoteNotice : MsgDownVoteNotice];
				}
				vote.hdr.thread_back = msg.hdr.number;

				smb_hfield_str(&vote, SENDER, (cfg.sub[subnum]->misc&SUB_NAME) ? useron.name : useron.alias);
				if(msg.id != NULL)
					smb_hfield_str(&vote, RFC822REPLYID, msg.id);
				
				sprintf(str, "%u", useron.number);
				smb_hfield_str(&vote, SENDEREXT, str);

				/* Security logging */
				msg_client_hfields(&vote, &client);
				smb_hfield_str(&vote, SENDERSERVER, startup->host_name);

				if((i=votemsg(&cfg, &smb, &vote, notice)) != SMB_SUCCESS)
					errormsg(WHERE,ERR_WRITE,smb.file,i,smb.last_error);

				break;
			}
			case '-':
				if(smb.curmsg>0) smb.curmsg--;
				do_find=false;
				break;
			case 'O':   /* Operator commands */
				while(online) {
					if(!(useron.misc&EXPERT))
						menu("sysmscan");
					bputs(text[OperatorPrompt]);
					strcpy(str,"?ACEHMQUV");
					if(SYSOP)
						strcat(str,"SP");
					switch(getkeys(str,0)) {
						case '?':
							if(useron.misc&EXPERT)
								menu("sysmscan");
							continue;
						case 'A':	/* Add comment */
							bputs(text[UeditComment]);
							if(!getstr(str, LEN_TITLE, K_LINE))
								break;
							smb_hfield_str(&msg, SMB_COMMENT, str);
							msg.idx.offset=0;
							if((i=smb_updatemsg(&smb, &msg)) != SMB_SUCCESS)
								errormsg(WHERE,ERR_WRITE,smb.file,i,smb.last_error);
							break;
						case 'P':   /* Purge user */
							if(noyes(text[AreYouSureQ]))
								break;
							purgeuser(cfg.sub[subnum]->misc&SUB_NAME
								? userdatdupe(0,U_NAME,LEN_NAME,msg.from)
								: matchuser(&cfg,msg.from,FALSE));
							break;
						case 'C':   /* Change message attributes */
							i=chmsgattr(msg);
							if(msg.hdr.attr==i)
								break;
							if(msg.total_hfields)
								smb_freemsgmem(&msg);
							msg.total_hfields=0;
							msg.idx.offset=0;
							if(smb_locksmbhdr(&smb)==SMB_SUCCESS) {	/* Lock the entire base */
								if(loadmsg(&msg,msg.idx.number) >= 0) {
									msg.hdr.attr=msg.idx.attr=i;
									if((i=smb_putmsg(&smb,&msg))!=0)
										errormsg(WHERE,ERR_WRITE,smb.file,i,smb.last_error);
									smb_unlockmsghdr(&smb,&msg); 
								}
								smb_unlocksmbhdr(&smb);
							}
							break;
						case 'E':   /* edit last post */
							FREE_AND_NULL(post);
							editmsg(&msg,subnum);
							break;
						case 'H':   /* View message header */
							msghdr(&msg);
							domsg=0;
							break;
						case 'M':   /* Move message */
							domsg=0;
							FREE_AND_NULL(post);
							if(msg.total_hfields)
								smb_freemsgmem(&msg);
							msg.total_hfields=0;
							msg.idx.offset=0;
							if(smb_locksmbhdr(&smb)==SMB_SUCCESS) {	/* Lock the entire base */
								if(loadmsg(&msg,msg.idx.number) < 0) {
									errormsg(WHERE,ERR_READ,smb.file,msg.idx.number);
									break; 
								}
								sprintf(str,text[DeletePostQ],msg.hdr.number,msg.subj);
								if(movemsg(&msg,subnum) && yesno(str)) {
									msg.idx.attr|=MSG_DELETE;
									msg.hdr.attr=msg.idx.attr;
									if((i=smb_putmsg(&smb,&msg))!=0)
										errormsg(WHERE,ERR_WRITE,smb.file,i,smb.last_error); 
								}
								smb_unlockmsghdr(&smb,&msg);
							}
							smb_unlocksmbhdr(&smb);
							break;

						case 'Q':
							break;
						case 'S':   /* Save/Append message to another file */
	/*	05/26/95
							if(!yesno(text[SaveMsgToFile]))
								break;
	*/
							bputs(text[FileToWriteTo]);
							if(getstr(str,50,K_LINE))
								msgtotxt(&msg,str, /* header: */true, /* mode: */GETMSGTXT_ALL);
							break;
						case 'U':   /* User edit */
							useredit(cfg.sub[subnum]->misc&SUB_NAME
								? userdatdupe(0,U_NAME,LEN_NAME,msg.from)
								: matchuser(&cfg,msg.from,TRUE /* sysop_alias */));
							break;
						case 'V':   /* Validate message */
							if(msg.total_hfields)
								smb_freemsgmem(&msg);
							msg.total_hfields=0;
							msg.idx.offset=0;
							if(smb_locksmbhdr(&smb)==SMB_SUCCESS) {	/* Lock the entire base */
								if(loadmsg(&msg,msg.idx.number) >= 0) {
									msg.idx.attr|=MSG_VALIDATED;
									msg.hdr.attr=msg.idx.attr;
									if((i=smb_putmsg(&smb,&msg))!=0)
										errormsg(WHERE,ERR_WRITE,smb.file,i,smb.last_error);
									smb_unlockmsghdr(&smb,&msg); 
								}
								smb_unlocksmbhdr(&smb);
							}
							break;
						default:
							continue; 
					}
					break; 
				}
				break;
			case ')':   /* Thread forward */
				l=msg.hdr.thread_first;
				if(!l) l=msg.hdr.thread_next;
				if(!l) {
					domsg=0;
					bputs(text[NoMessagesFound]);
					break; 
				}
				for(u=0;u<smb.msgs;u++)
					if(l==post[u].idx.number)
						break;
				if(u<smb.msgs)
					smb.curmsg=u;
				else {
					domsg=0;
					bputs(text[NoMessagesFound]);
				}
				do_find=false;
				break;
			case '(':   /* Thread backwards */
				if(!msg.hdr.thread_back) {
					domsg=0;
					bputs(text[NoMessagesFound]);
					break; 
				}
				for(u=0;u<smb.msgs;u++)
					if(msg.hdr.thread_back==post[u].idx.number)
						break;
				if(u<smb.msgs)
					smb.curmsg=u;
				else {
					domsg=0;
					bputs(text[NoMessagesFound]);
				}
				do_find=false;
				break;
			case '>':   /* Search Title forward */
				for(u=smb.curmsg+1;u<smb.msgs;u++)
					if(post[u].idx.subj==msg.idx.subj)
						break;
				if(u<smb.msgs)
					smb.curmsg=u;
				else {
					domsg=0;
					bputs(text[NoMessagesFound]);
				}
				do_find=false;
				break;
			case '<':   /* Search Title backward */
				for(i=smb.curmsg-1;i>-1;i--)
					if(post[i].idx.subj==msg.idx.subj)
						break;
				if(i>-1)
					smb.curmsg=i;
				else {
					domsg=0;
					bputs(text[NoMessagesFound]);
				}
				do_find=false;
				break;
			case '}':   /* Search Author forward */
				strcpy(str,msg.from);
				for(u=smb.curmsg+1;u<smb.msgs;u++)
					if(post[u].idx.from==msg.idx.from)
						break;
				if(u<smb.msgs)
					smb.curmsg=u;
				else {
					domsg=0;
					bputs(text[NoMessagesFound]);
				}
				do_find=false;
				break;
			case '{':   /* Search Author backward */
				strcpy(str,msg.from);
				for(i=smb.curmsg-1;i>-1;i--)
					if(post[i].idx.from==msg.idx.from)
						break;
				if(i>-1)
					smb.curmsg=i;
				else {
					domsg=0;
					bputs(text[NoMessagesFound]);
				}
				do_find=false;
				break;
			case ']':   /* Search To User forward */
				strcpy(str,msg.to);
				for(u=smb.curmsg+1;u<smb.msgs;u++)
					if(post[u].idx.to==msg.idx.to)
						break;
				if(u<smb.msgs)
					smb.curmsg=u;
				else {
					domsg=0;
					bputs(text[NoMessagesFound]);
				}
				do_find=false;
				break;
			case '[':   /* Search To User backward */
				strcpy(str,msg.to);
				for(i=smb.curmsg-1;i>-1;i--)
					if(post[i].idx.to==msg.idx.to)
						break;
				if(i>-1)
					smb.curmsg=i;
				else {
					domsg=0;
					bputs(text[NoMessagesFound]);
				}
				do_find=false;
				break;
			case 0: /* Carriage return - Next Message */
			case '+':
				if(smb.curmsg<smb.msgs-1) smb.curmsg++;
				else done=1;
				break;
			case '?':
				menu("msgscan");
				domsg=0;
				break;	
		} 
	}
	if(msg.total_hfields)
		smb_freemsgmem(&msg);
	if(post)
		free(post);
	if(!quit
		&& !(org_mode&(SCAN_CONST|SCAN_TOYOU|SCAN_FIND)) && !(cfg.sub[subnum]->misc&SUB_PONLY)
		&& reads && chk_ar(cfg.sub[subnum]->post_ar,&useron,&client) && text[Post][0]
		&& !(useron.rest&FLAG('P'))) {
		sprintf(str,text[Post],cfg.grp[cfg.sub[subnum]->grp]->sname
			,cfg.sub[subnum]->lname);
		if(!noyes(str))
			postmsg(subnum,0,0); 
	}
	if(!(org_mode&(SCAN_CONST|SCAN_TOYOU|SCAN_FIND))
		&& !(subscan[subnum].cfg&SUB_CFG_NSCAN) && text[AddSubToNewScanQ][0] && yesno(text[AddSubToNewScanQ]))
		subscan[subnum].cfg|=SUB_CFG_NSCAN;
	smb_close(&smb);
	smb_stack(&smb,SMB_STACK_POP);
	current_msg=NULL;
	return(quit);
}

/****************************************************************************/
/* This function lists all messages in sub-board							*/
/* Displays msg header information only (no body text)						*/
/* Returns number of messages found/displayed.                              */
/****************************************************************************/
long sbbs_t::listsub(uint subnum, long mode, long start, const char* search)
{
	int 	i;
	uint32_t	posts;
	uint32_t	total=0;
	long	displayed = 0;
	long	lp_mode = LP_BYSELF|LP_OTHERS;
	post_t	*post;

	if((i=smb_stack(&smb,SMB_STACK_PUSH))!=0) {
		errormsg(WHERE,ERR_OPEN,cfg.sub[subnum]->code,i);
		return(0); 
	}
	SAFEPRINTF2(smb.file,"%s%s",cfg.sub[subnum]->data_dir,cfg.sub[subnum]->code);
	smb.retry_time=cfg.smb_retry_time;
	smb.subnum=subnum;
	if((i=smb_open(&smb))!=0) {
		errormsg(WHERE,ERR_OPEN,smb.file,i,smb.last_error);
		smb_stack(&smb,SMB_STACK_POP);
		return(0); 
	}
	if(mode&SCAN_TOYOU)
		lp_mode = 0;
	if(mode&SCAN_UNREAD)
		lp_mode |= LP_UNREAD;
	post=loadposts(&posts,subnum,0,lp_mode,NULL,&total);
	bprintf(text[SearchSubFmt]
		,cfg.grp[cfg.sub[subnum]->grp]->sname,cfg.sub[subnum]->lname,total);
	if(posts) {
		if(mode&SCAN_FIND)
			displayed=searchposts(subnum, post, start, posts, search);
		else
			displayed=listmsgs(subnum, mode, post, start, posts);
		free(post);
	}
	smb_close(&smb);

	smb_stack(&smb,SMB_STACK_POP);
	
	return(displayed);
}

/****************************************************************************/
/* Will search the messages pointed to by 'msg' for the occurance of the    */
/* string 'search' and display any messages (number of message, author and  */
/* title). 'msgs' is the total number of valid messages.                    */
/* Returns number of messages found.                                        */
/****************************************************************************/
long sbbs_t::searchposts(uint subnum, post_t *post, long start, long posts
	, const char *search)
{
	char*	buf;
	char	subj[128];
	long	l,found=0;
	smbmsg_t msg;

	msg.total_hfields=0;
	for(l=start;l<posts && !msgabort();l++) {
		msg.idx.offset=post[l].idx.offset;
		if(loadmsg(&msg,post[l].idx.number) < 0)
			continue;
		smb_unlockmsghdr(&smb,&msg);
		buf=smb_getmsgtxt(&smb,&msg,GETMSGTXT_ALL);
		if(!buf) {
			smb_freemsgmem(&msg);
			continue; 
		}
		strupr(buf);
		strip_ctrl(buf, buf);
		SAFECOPY(subj,msg.subj);
		strupr(subj);
		if(strstr(buf,search) || strstr(subj,search)) {
			if(!found)
				CRLF;
			bprintf(text[SubMsgLstFmt],l+1
				,(msg.hdr.attr&MSG_ANONYMOUS) && !sub_op(subnum) ? text[Anonymous]
				: msg.from
				,msg.to
				,msg_listing_flag(subnum, &msg, &post[l])
				,msg.subj);
			found++; 
		}
		free(buf);
		smb_freemsgmem(&msg); 
	}

	return(found);
}

/****************************************************************************/
/* Will search the messages pointed to by 'msg' for message to the user on  */
/* Returns number of messages found.                                        */
/****************************************************************************/
long sbbs_t::showposts_toyou(uint subnum, post_t *post, ulong start, long posts, long mode)
{
	char	str[128];
	ushort	namecrc,aliascrc,sysop;
	long	l,found;
    smbmsg_t msg;

	strcpy(str,useron.alias);
	strlwr(str);
	aliascrc=crc16(str,0);
	strcpy(str,useron.name);
	strlwr(str);
	namecrc=crc16(str,0);
	sysop=crc16("sysop",0);
	msg.total_hfields=0;
	for(l=start,found=0;l<posts && !msgabort();l++) {

		if((useron.number!=1 || post[l].idx.to!=sysop)
			&& post[l].idx.to!=aliascrc && post[l].idx.to!=namecrc)
			continue;

		if((post[l].idx.attr&MSG_READ) && (mode&SCAN_UNREAD)) /* Skip read messages */
			continue;

		if(msg.total_hfields)
			smb_freemsgmem(&msg);
		msg.total_hfields=0;
		msg.idx.offset=post[l].idx.offset;
		if(loadmsg(&msg,post[l].idx.number) < 0)
			continue;
		smb_unlockmsghdr(&smb,&msg);
		if((useron.number==1 && !stricmp(msg.to,"sysop") && !msg.from_net.type)
			|| !stricmp(msg.to,useron.alias) || !stricmp(msg.to,useron.name)) {
			if(!found)
				bputs(text[MailOnSystemLstHdr]);
			found++;
			bprintf(text[SubMsgLstFmt],l+1
				,(msg.hdr.attr&MSG_ANONYMOUS) && !SYSOP
				? text[Anonymous] : msg.from
				,msg.to
				,msg_listing_flag(subnum, &msg, &post[l])
				,msg.subj); 
		} 
	}

	if(msg.total_hfields)
		smb_freemsgmem(&msg);

	return(found);
}
