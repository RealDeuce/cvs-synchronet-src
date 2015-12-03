/* fido.cpp */

/* Synchronet FidoNet-related routines */

/* $Id: fido.cpp,v 1.56 2015/12/03 10:40:14 rswindell Exp $ */

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

/* TODO: This file is made up almost *entirely* of copy/pasted crap and		*/
/* it needs to go away - FTN netmail should be handled by sbbsecho anyway.	*/

#include "sbbs.h"
#include "qwk.h"

faddr_t atofaddr(scfg_t* cfg, char *str);

void pt_zone_kludge(fmsghdr_t hdr,int fido)
{
	char str[256];

	sprintf(str,"\1INTL %hu:%hu/%hu %hu:%hu/%hu\r"
		,hdr.destzone,hdr.destnet,hdr.destnode
		,hdr.origzone,hdr.orignet,hdr.orignode);
	write(fido,str,strlen(str));

	if(hdr.destpoint) {
		sprintf(str,"\1TOPT %hu\r"
			,hdr.destpoint);
		write(fido,str,strlen(str)); 
	}

	if(hdr.origpoint) {
		sprintf(str,"\1FMPT %hu\r"
			,hdr.origpoint);
		write(fido,str,strlen(str)); 
	}
}

bool sbbs_t::lookup_netuser(char *into)
{
	char to[128],name[26],str[256],q[128];
	int i;
	FILE *stream;

	if(strchr(into,'@'))
		return(false);
	strcpy(to,into);
	strupr(to);
	sprintf(str,"%sqnet/users.dat", cfg.data_dir);
	if((stream=fnopen(&i,str,O_RDONLY))==NULL)
		return(false);
	while(!feof(stream)) {
		if(!fgets(str,sizeof(str),stream))
			break;
		str[25]=0;
		truncsp(str);
		strcpy(name,str);
		strupr(name);
		str[35]=0;
		truncsp(str+27);
		sprintf(q,"Do you mean %s @%s",str,str+27);
		if(strstr(name,to) && yesno(q)) {
			fclose(stream);
			sprintf(into,"%s@%s",str,str+27);
			return(true); 
		}
		if(sys_status&SS_ABORT)
			break; 
	}
	fclose(stream);
	return(false);
}

/****************************************************************************/
/* Send FidoNet/QWK/Internet NetMail from BBS								*/
/****************************************************************************/
bool sbbs_t::netmail(const char *into, const char *title, long mode)
{
	char	str[256],subj[128],to[256],fname[128],*buf,*p,ch;
	char 	tmp[512];
	int		file,fido,x,cc_found,cc_sent;
	uint	i;
	long	length,l;
	faddr_t addr;
	fmsghdr_t hdr;
	struct tm tm;

	if(useron.etoday>=cfg.level_emailperday[useron.level] && !SYSOP && !(useron.exempt&FLAG('M'))) {
		bputs(text[TooManyEmailsToday]);
		return(false); 
	}

	SAFECOPY(subj,title);

	strcpy(to,into);

	lookup_netuser(to);

	p=strrchr(to,'@');      /* Find '@' in name@addr */
	if(p && !isdigit(*(p+1)) && !strchr(p,'.') && !strchr(p,':')) {
		mode&=~WM_FILE;
		qnetmail(to,title,mode|WM_NETMAIL);
		return(false); 
	}
	if(!cfg.total_faddrs || p==NULL || !strchr(p+1,'/')) {
		if(!p && cfg.dflt_faddr.zone)
			addr=cfg.dflt_faddr;
		else if(cfg.inetmail_misc&NMAIL_ALLOW) {
			if(mode&WM_FILE && !SYSOP && !(cfg.inetmail_misc&NMAIL_FILE))
				mode&=~WM_FILE;
			return(inetmail(into,title,mode|WM_NETMAIL));
		}
		else if(cfg.dflt_faddr.zone)
			addr=cfg.dflt_faddr;
		else {
			bputs(text[InvalidNetMailAddr]);
			return(false); 
		} 
	} else {
		addr=atofaddr(&cfg,p+1); 	/* Get fido address */
		*p=0;					/* Chop off address */
	}

	if(mode&WM_FILE && !SYSOP && !(cfg.netmail_misc&NMAIL_FILE))
		mode&=~WM_FILE;

	if((!SYSOP && !(cfg.netmail_misc&NMAIL_ALLOW)) || useron.rest&FLAG('M')
		|| !cfg.total_faddrs) {
		bputs(text[NoNetMailAllowed]);
		return(false); 
	}

	truncsp(to);				/* Truncate off space */

	memset(&hdr,0,sizeof(hdr));   /* Initialize header to null */
	SAFECOPY(hdr.from, cfg.netmail_misc&NMAIL_ALIAS ? useron.alias : useron.name);
	SAFECOPY(hdr.to,to);

	/* Look-up in nodelist? */

	if(cfg.netmail_cost && !(useron.exempt&FLAG('S'))) {
		if(useron.cdt+useron.freecdt<cfg.netmail_cost) {
			bputs(text[NotEnoughCredits]);
			return(false); 
		}
		sprintf(str,text[NetMailCostContinueQ],cfg.netmail_cost);
		if(noyes(str))
			return(false); 
	}

	now=time(NULL);
	if(localtime_r(&now,&tm)!=NULL)
		sprintf(hdr.time,"%02u %3.3s %02u  %02u:%02u:%02u"
			,tm.tm_mday,mon[tm.tm_mon],TM_YEAR(tm.tm_year)
			,tm.tm_hour,tm.tm_min,tm.tm_sec);

	hdr.destzone	=addr.zone;
	hdr.destnet 	=addr.net;
	hdr.destnode	=addr.node;
	hdr.destpoint	=addr.point;

	for(i=0;i<cfg.total_faddrs;i++)
		if(addr.zone==cfg.faddr[i].zone && addr.net==cfg.faddr[i].net)
			break;
	if(i==cfg.total_faddrs) {
		for(i=0;i<cfg.total_faddrs;i++)
			if(addr.zone==cfg.faddr[i].zone)
				break; 
	}
	if(i==cfg.total_faddrs)
		i=0;
	hdr.origzone	=cfg.faddr[i].zone;
	hdr.orignet 	=cfg.faddr[i].net;
	hdr.orignode	=cfg.faddr[i].node;
	hdr.origpoint	=cfg.faddr[i].point;

	smb_faddrtoa(&cfg.faddr[i],str);
	bprintf(text[NetMailing],hdr.to,smb_faddrtoa(&addr,tmp),hdr.from,str);

	hdr.attr=(FIDO_LOCAL|FIDO_PRIVATE);

	if(cfg.netmail_misc&NMAIL_CRASH) hdr.attr|=FIDO_CRASH;
	if(cfg.netmail_misc&NMAIL_HOLD)  hdr.attr|=FIDO_HOLD;
	if(cfg.netmail_misc&NMAIL_KILL)  hdr.attr|=FIDO_KILLSENT;
	if(mode&WM_FILE) hdr.attr|=FIDO_FILE;

	SAFEPRINTF(str,"%snetmail.msg", cfg.node_dir);
	removecase(str);	/* Just incase it's already there */
	// mode&=~WM_FILE;
	if(!writemsg(str,nulstr,subj,WM_NETMAIL|mode,INVALID_SUB,into,hdr.from)) {
		bputs(text[Aborted]);
		return(false); 
	}

	if(mode&WM_FILE) {
		strcpy(fname,subj);
		sprintf(str,"%sfile/%04u.out", cfg.data_dir, useron.number);
		MKDIR(str);
		strcpy(tmp, cfg.data_dir);
		if(tmp[0]=='.')    /* Relative path */
			sprintf(tmp,"%s%s", cfg.node_dir, cfg.data_dir);
		sprintf(str,"%sfile/%04u.out/%s",tmp,useron.number,fname);
		strcpy(subj,str);
		if(fexistcase(str)) {
			bputs(text[FileAlreadyThere]);
			return(false); 
		}
		{ /* Remote */
			xfer_prot_menu(XFER_UPLOAD);
			mnemonics(text[ProtocolOrQuit]);
			sprintf(str,"%c",text[YNQP][2]);
			for(x=0;x<cfg.total_prots;x++)
				if(cfg.prot[x]->ulcmd[0] && chk_ar(cfg.prot[x]->ar,&useron,&client)) {
					sprintf(tmp,"%c",cfg.prot[x]->mnemonic);
					strcat(str,tmp); 
				}
			ch=(char)getkeys(str,0);
			if(ch==text[YNQP][2] || sys_status&SS_ABORT) {
				bputs(text[Aborted]);
				return(false); 
			}
			for(x=0;x<cfg.total_prots;x++)
				if(cfg.prot[x]->ulcmd[0] && cfg.prot[x]->mnemonic==ch
					&& chk_ar(cfg.prot[x]->ar,&useron,&client))
					break;
			if(x<cfg.total_prots)	/* This should be always */
				protocol(cfg.prot[x],XFER_UPLOAD,subj,nulstr,true); 
		}
		sprintf(tmp,"%s%s",cfg.temp_dir,title);
		if(!fexistcase(subj) && fexistcase(tmp))
			mv(tmp,subj,0);
		l=(long)flength(subj);
		if(l>0)
			bprintf(text[FileNBytesReceived],fname,ultoac(l,tmp));
		else {
			bprintf(text[FileNotReceived],fname);
			return(false); 
		} 
	}

	p=subj;
	if((SYSOP || useron.exempt&FLAG('F'))
		&& !strnicmp(p,"CR:",3)) {     /* Crash over-ride by sysop */
		p+=3;				/* skip CR: */
		if(*p==' ') p++; 	/* skip extra space if it exists */
		hdr.attr|=FIDO_CRASH; 
	}

	if((SYSOP || useron.exempt&FLAG('F'))
		&& !strnicmp(p,"FR:",3)) {     /* File request */
		p+=3;				/* skip FR: */
		if(*p==' ') p++;
		hdr.attr|=FIDO_FREQ; 
	}

	if((SYSOP || useron.exempt&FLAG('F'))
		&& !strnicmp(p,"RR:",3)) {     /* Return receipt request */
		p+=3;				/* skip RR: */
		if(*p==' ') p++;
		hdr.attr|=FIDO_RRREQ; 
	}

	if((SYSOP || useron.exempt&FLAG('F'))
		&& !strnicmp(p,"FA:",3)) {     /* File Attachment */
		p+=3;				/* skip FA: */
		if(*p==' ') p++;
		hdr.attr|=FIDO_FILE; 
	}

	SAFECOPY(hdr.subj,p);

	SAFEPRINTF(str,"%snetmail.msg", cfg.node_dir);
	fexistcase(str);
	if((file=nopen(str,O_RDONLY))==-1) {
		errormsg(WHERE,ERR_OPEN,str,O_RDONLY);
		return(false); 
	}
	length=(long)filelength(file);
	if((buf=(char *)malloc(length))==NULL) {
		close(file);
		errormsg(WHERE,ERR_ALLOC,str,length);
		return(false); 
	}
	read(file,buf,length);
	close(file);

	md(cfg.netmail_dir);
	cc_sent=0;
	while(1) {
		for(i=1;i;i++) {
			sprintf(str,"%s%u.msg", cfg.netmail_dir,i);
			if(!fexistcase(str))
				break; 
		}
		if(!i) {
			bputs(text[TooManyEmailsToday]);
			return(false); 
		}
		if((fido=nopen(str,O_WRONLY|O_CREAT|O_EXCL))==-1) {
			errormsg(WHERE,ERR_OPEN,str,O_WRONLY|O_CREAT|O_EXCL);
			return(false); 
		}
		write(fido,&hdr,sizeof(hdr));

		SAFEPRINTF(str,"\1PID: %s\r", msg_program_id(tmp));
		write(fido, str, strlen(str));

		pt_zone_kludge(hdr,fido);
		/* TZUTC (FSP-1001) */
		int tzone=smb_tzutc(sys_timezone(&cfg));
		const char* minus="";
		if(tzone<0) {
			minus="-";
			tzone=-tzone;
		}
		SAFEPRINTF3(str,"\1TZUTC: %s%02d%02u\r", minus, tzone/60, tzone%60);
		write(fido, str, strlen(str));

		if(cfg.netmail_misc&NMAIL_DIRECT) {
			SAFECOPY(str,"\1FLAGS DIR\r\n");
			write(fido,str,strlen(str)); 
		}
		if(mode&WM_FILE) {
			SAFECOPY(str,"\1FLAGS KFS\r\n");
			write(fido,str,strlen(str)); 
		}

		if(cc_sent) {
			SAFEPRINTF(str,"* Originally to: %s\r\n\r\n",into);
			write(fido,str,strlen(str)); 
		}

		l=0L;
		while(l<length) {
			if(buf[l]==CTRL_A) {		/* Ctrl-A, so skip it and the next char */
				l++;
				if(l>=length || toupper(buf[l])=='Z')	/* EOF */
					break;
				if((ch=ctrl_a_to_ascii_char(buf[l])) != 0)
					write(fido,&ch,1);
			}
			else if(buf[l]!=LF) {
				if((uchar)buf[l]==0x8d)   /* r0dent i converted to normal i */
					buf[l]='i';
				write(fido,buf+l,1); 
			}
			l++; 
		}
		l=0;
		write(fido,&l,1);	/* Null terminator */
		close(fido);

		useron.emails++;
		logon_emails++;
		putuserrec(&cfg,useron.number,U_EMAILS,5,ultoa(useron.emails,tmp,10)); 
		useron.etoday++;
		putuserrec(&cfg,useron.number,U_ETODAY,5,ultoa(useron.etoday,tmp,10));

		if(!(useron.exempt&FLAG('S')))
			subtract_cdt(&cfg,&useron,cfg.netmail_cost);
		if(mode&WM_FILE)
			sprintf(str,"%s sent NetMail file attachment to %s (%s)"
				,useron.alias
				,hdr.to,smb_faddrtoa(&addr,tmp));
		else
			sprintf(str,"%s sent NetMail to %s (%s)"
				,useron.alias
				,hdr.to,smb_faddrtoa(&addr,tmp));
		logline("EN",str);

		cc_found=0;
		for(l=0;l<length && cc_found<=cc_sent;l++)
			if(l+3<length && !strnicmp(buf+l,"CC:",3)) {
				cc_found++;
				l+=2; 
			}
			else {
				while(l<length && *(buf+l)!=LF)
					l++; 
			}
		if(!cc_found)
			break;
		while(l<length && *(buf+l)==' ') l++;
		for(i=0;l<length && *(buf+l)!=LF && i<128;i++,l++)
			str[i]=buf[l];
		if(!i)
			break;
		str[i]=0;
		p=strrchr(str,'@');
		if(p) {
			addr=atofaddr(&cfg,p+1);
			*p=0;
			SAFECOPY(hdr.to,str); 
		}
		else {
			atofaddr(&cfg,str);
			strcpy(hdr.to,"Sysop"); 
		}
		hdr.destzone	=addr.zone;
		hdr.destnet 	=addr.net;
		hdr.destnode	=addr.node;
		hdr.destpoint	=addr.point;
		cc_sent++; 
	}

	if(cfg.netmail_sem[0])		/* update semaphore file */
		ftouch(cmdstr(cfg.netmail_sem,nulstr,nulstr,NULL));

	free(buf);
	return(true);
}

/****************************************************************************/
/* Send NetMail from QWK REP Packet 										*/
/****************************************************************************/
void sbbs_t::qwktonetmail(FILE *rep, char *block, char *into, uchar fromhub)
{
	char	*qwkbuf,to[129],name[129],sender[129],senderaddr[129]
			   ,str[256],*p,*cp,*addr,fulladdr[129],ch;
	char 	tmp[512];
	int 	i,fido,inet=0,qnet=0;
	ushort	net;
	uint16_t xlat;
	long	l,offset,length,m,n;
	faddr_t fidoaddr;
    fmsghdr_t hdr;
	smbmsg_t msg;
	struct	tm tm;

	if(useron.rest&FLAG('M')) { 
		bputs(text[NoNetMailAllowed]);
		return; 
	}

	to[0]=0;
	name[0]=0;
	sender[0]=0;
	senderaddr[0]=0;
	fulladdr[0]=0;

	sprintf(str,"%.6s",block+116);
	n=atol(str);	  /* i = number of 128 byte records */

	if(n<2L || n>999999L) {
		errormsg(WHERE,ERR_CHK,"QWK blocks",n);
		return; 
	}
	if((qwkbuf=(char *)malloc(n*QWK_BLOCK_LEN))==NULL) {
		errormsg(WHERE,ERR_ALLOC,nulstr,n*QWK_BLOCK_LEN);
		return; 
	}
	memcpy((char *)qwkbuf,block,QWK_BLOCK_LEN);
	fread(qwkbuf+QWK_BLOCK_LEN,n-1,QWK_BLOCK_LEN,rep);

	if(into==NULL)
		sprintf(to,"%-128.128s",(char *)qwkbuf+QWK_BLOCK_LEN);  /* To user on first line */
	else
		SAFECOPY(to,into);

	p=strchr(to,QWK_NEWLINE);		/* chop off at first CR */
	if(p) *p=0;

	SAFECOPY(name,to);
	p=strchr(name,'@');
	if(p) *p=0;
	truncsp(name);


	p=strrchr(to,'@');       /* Find '@' in name@addr */
	if(p && !isdigit(*(p+1)) && !strchr(p,'.') && !strchr(p,':')) { /* QWKnet */
		qnet=1;
		*p=0; 
	}
	else if(p==NULL || !isdigit(*(p+1)) || !cfg.total_faddrs) {
		if(p==NULL && cfg.dflt_faddr.zone)
			fidoaddr=cfg.dflt_faddr;
		else if(cfg.inetmail_misc&NMAIL_ALLOW) {	/* Internet */
			inet=1;
			}
		else if(cfg.dflt_faddr.zone)
			fidoaddr=cfg.dflt_faddr;
		else {
			bputs(text[InvalidNetMailAddr]);
			free(qwkbuf);
			return; 
		} 
	}
	else {
		fidoaddr=atofaddr(&cfg,p+1); 	/* Get fido address */
		*p=0;					/* Chop off address */
	}


	if(!inet && !qnet &&		/* FidoNet */
		((!SYSOP && !(cfg.netmail_misc&NMAIL_ALLOW)) || !cfg.total_faddrs)) {
		bputs(text[NoNetMailAllowed]);
		free(qwkbuf);
		return; 
	}

	truncsp(to);			/* Truncate off space */

	if(!stricmp(to,"SBBS") && !SYSOP && qnet) {
		free(qwkbuf);
		return; 
	}

	l=QWK_BLOCK_LEN;		/* Start of message text */

	if(qnet || inet) {

		if(into==NULL) {	  /* If name@addr on first line, skip first line */
			while(l<(n*QWK_BLOCK_LEN) && qwkbuf[l]!=QWK_NEWLINE) l++;
			l++; 
		}

		memset(&msg,0,sizeof(smbmsg_t));
		msg.hdr.version=smb_ver();
		msg.hdr.when_imported.time=time32(NULL);
		msg.hdr.when_imported.zone=sys_timezone(&cfg);

		if(fromhub || useron.rest&FLAG('Q')) {
			net=NET_QWK;
			smb_hfield(&msg,SENDERNETTYPE,sizeof(net),&net);
			if(!strncmp(qwkbuf+l,"@VIA:",5)) {
				sprintf(str,"%.128s",qwkbuf+l+5);
				cp=strchr(str,QWK_NEWLINE);
				if(cp) *cp=0;
				l+=strlen(str)+1;
				cp=str;
				while(*cp && *cp<=' ') cp++;
				sprintf(senderaddr,"%s/%s"
					,fromhub ? cfg.qhub[fromhub-1]->id : useron.alias,cp);
				strupr(senderaddr);
				smb_hfield(&msg,SENDERNETADDR,strlen(senderaddr),senderaddr); 
			}
			else {
				if(fromhub)
					SAFECOPY(senderaddr, cfg.qhub[fromhub-1]->id);
				else
					SAFECOPY(senderaddr, useron.alias);
				strupr(senderaddr);
				smb_hfield(&msg,SENDERNETADDR,strlen(senderaddr),senderaddr); 
			}
			sprintf(sender,"%.25s",block+46);     /* From name */
		}
		else {	/* Not Networked */
			msg.hdr.when_written.zone=sys_timezone(&cfg);
			sprintf(str,"%u",useron.number);
			smb_hfield(&msg,SENDEREXT,strlen(str),str);
			SAFECOPY(sender,(qnet || cfg.inetmail_misc&NMAIL_ALIAS)
				? useron.alias : useron.name);
			}
		truncsp(sender);
		smb_hfield(&msg,SENDER,strlen(sender),sender);
		if(fromhub)
			msg.idx.from=0;
		else
			msg.idx.from=useron.number;
		if(!strncmp(qwkbuf+l,"@TZ:",4)) {
			sprintf(str,"%.128s",qwkbuf+l);
			cp=strchr(str,QWK_NEWLINE);
			if(cp) *cp=0;
			l+=strlen(str)+1;
			cp=str+4;
			while(*cp && *cp<=' ') cp++;
			msg.hdr.when_written.zone=(short)ahtoul(cp); 
		}
		else
			msg.hdr.when_written.zone=sys_timezone(&cfg);
		memset(&tm,0,sizeof(tm));
		tm.tm_mon=((qwkbuf[8]&0xf)*10)+(qwkbuf[9]&0xf);
		if(tm.tm_mon) tm.tm_mon--;	/* 0 based */
		tm.tm_mday=((qwkbuf[11]&0xf)*10)+(qwkbuf[12]&0xf);
		tm.tm_year=((qwkbuf[14]&0xf)*10)+(qwkbuf[15]&0xf);
		if(tm.tm_year<Y2K_2DIGIT_WINDOW)
			tm.tm_year+=100;
		tm.tm_hour=((qwkbuf[16]&0xf)*10)+(qwkbuf[17]&0xf);
		tm.tm_min=((qwkbuf[19]&0xf)*10)+(qwkbuf[20]&0xf);  /* From QWK time */
		tm.tm_sec=0;

		tm.tm_isdst=-1;	/* Do not adjust for DST */
		msg.hdr.when_written.time=mktime32(&tm);

		sprintf(str,"%.25s",block+71);              /* Title */
		smb_hfield(&msg,SUBJECT,strlen(str),str);
	}

	if(qnet) {

		p++;
		addr=p;
		msg.idx.to=qwk_route(&cfg,addr,fulladdr, sizeof(fulladdr)-1);
		if(!fulladdr[0]) {		/* Invalid address, so BOUNCE it */
		/**
			errormsg(WHERE,ERR_CHK,addr,0);
			free(qwkbuf);
			smb_freemsgmem(msg);
			return;
		**/
			smb_hfield(&msg,SENDER,strlen(cfg.sys_id),cfg.sys_id);
			msg.idx.from=0;
			msg.idx.to=useron.number;
			SAFECOPY(to,sender);
			SAFECOPY(fulladdr,senderaddr);
			SAFEPRINTF(str,"BADADDR: %s",addr);
			smb_hfield(&msg,SUBJECT,strlen(str),str);
			net=NET_NONE;
			smb_hfield(&msg,SENDERNETTYPE,sizeof(net),&net);
		}
		/* This is required for fixsmb to be able to rebuild the index */
		SAFEPRINTF(str,"%u",msg.idx.to);
		smb_hfield_str(&msg,RECIPIENTEXT,str);

		smb_hfield(&msg,RECIPIENT,strlen(name),name);
		net=NET_QWK;
		smb_hfield(&msg,RECIPIENTNETTYPE,sizeof(net),&net);

		truncsp(fulladdr);
		if(fulladdr[0])
			smb_hfield(&msg,RECIPIENTNETADDR,strlen(fulladdr),fulladdr);

		bprintf(text[NetMailing],to,fulladdr,sender,cfg.sys_id); 
	}

	if(inet) {				/* Internet E-mail */

		if(cfg.inetmail_cost && !(useron.exempt&FLAG('S'))) {
			if(useron.cdt+useron.freecdt<cfg.inetmail_cost) {
				bputs(text[NotEnoughCredits]);
				free(qwkbuf);
				smb_freemsgmem(&msg);
				return; 
			}
			sprintf(str,text[NetMailCostContinueQ],cfg.inetmail_cost);
			if(noyes(str)) {
				free(qwkbuf);
				smb_freemsgmem(&msg);
				return; 
			} 
		}

		net=NET_INTERNET;
		smb_hfield(&msg,RECIPIENT,strlen(name),name);
		msg.idx.to=0;   /* Out-bound NetMail set to 0 */
		smb_hfield(&msg,RECIPIENTNETTYPE,sizeof(net),&net);
		smb_hfield(&msg,RECIPIENTNETADDR,strlen(to),to);

		bprintf(text[NetMailing],name,to
			,cfg.inetmail_misc&NMAIL_ALIAS ? useron.alias : useron.name
			,cfg.sys_inetaddr); 
	}

	if(qnet || inet) {

		bputs(text[WritingIndx]);

		if((i=smb_stack(&smb,SMB_STACK_PUSH))!=0) {
			errormsg(WHERE,ERR_OPEN,"MAIL",i);
			free(qwkbuf);
			smb_freemsgmem(&msg);
			return; 
		}
		sprintf(smb.file,"%smail", cfg.data_dir);
		smb.retry_time=cfg.smb_retry_time;
		smb.subnum=INVALID_SUB;
		if((i=smb_open(&smb))!=0) {
			smb_stack(&smb,SMB_STACK_POP);
			errormsg(WHERE,ERR_OPEN,smb.file,i,smb.last_error);
			free(qwkbuf);
			smb_freemsgmem(&msg);
			return; 
		}

		if(smb_fgetlength(smb.shd_fp)<1L) {   /* Create it if it doesn't exist */
			smb.status.max_crcs=cfg.mail_maxcrcs;
			smb.status.max_msgs=0;
			smb.status.max_age=cfg.mail_maxage;
			smb.status.attr=SMB_EMAIL;
			if((i=smb_create(&smb))!=0) {
				smb_close(&smb);
				smb_stack(&smb,SMB_STACK_POP);
				errormsg(WHERE,ERR_CREATE,smb.file,i,smb.last_error);
				free(qwkbuf);
				smb_freemsgmem(&msg);
				return; 
			} 
		}

		length=n*256L;	// Extra big for CRLF xlat, was (n-1L)*256L (03/16/96)


		if(length&0xfff00000UL || !length) {
			smb_close(&smb);
			smb_stack(&smb,SMB_STACK_POP);
			sprintf(str,"REP msg (%ld)",n);
			errormsg(WHERE,ERR_LEN,str,length);
			free(qwkbuf);
			smb_freemsgmem(&msg);
			return; 
		}

		if((i=smb_open_da(&smb))!=0) {
			smb_close(&smb);
			smb_stack(&smb,SMB_STACK_POP);
			errormsg(WHERE,ERR_OPEN,smb.file,i,smb.last_error);
			free(qwkbuf);
			smb_freemsgmem(&msg);
			return; 
		}
		if(cfg.sys_misc&SM_FASTMAIL)
			offset=smb_fallocdat(&smb,length,1);
		else
			offset=smb_allocdat(&smb,length,1);
		smb_close_da(&smb);

		smb_fseek(smb.sdt_fp,offset,SEEK_SET);
		xlat=XLAT_NONE;
		smb_fwrite(&smb,&xlat,2,smb.sdt_fp);
		m=2;
		for(;l<n*QWK_BLOCK_LEN && m<length;l++) {
			if(qwkbuf[l]==0 || qwkbuf[l]==LF)
				continue;
			if(qwkbuf[l]==QWK_NEWLINE) {
				smb_fwrite(&smb,crlf,2,smb.sdt_fp);
				m+=2;
				continue; 
			}
			smb_fputc(qwkbuf[l],smb.sdt_fp);
			m++; 
		}

		for(ch=0;m<length;m++)			/* Pad out with NULLs */
			smb_fputc(ch,smb.sdt_fp);
		smb_fflush(smb.sdt_fp);

		msg.hdr.offset=offset;

		smb_dfield(&msg,TEXT_BODY,length);

		i=smb_addmsghdr(&smb,&msg,SMB_SELFPACK);
		smb_close(&smb);
		smb_stack(&smb,SMB_STACK_POP);

		smb_freemsgmem(&msg);
		if(i!=SMB_SUCCESS) {
			errormsg(WHERE,ERR_WRITE,smb.file,i,smb.last_error); 
			smb_freemsgdat(&smb,offset,length,1);
		}
		else {		/* Successful */
			if(inet) {
				if(cfg.inetmail_sem[0]) 	 /* update semaphore file */
					ftouch(cmdstr(cfg.inetmail_sem,nulstr,nulstr,NULL));
				if(!(useron.exempt&FLAG('S')))
					subtract_cdt(&cfg,&useron,cfg.inetmail_cost); 
			}

			useron.emails++;
			logon_emails++;
			putuserrec(&cfg,useron.number,U_EMAILS,5,ultoa(useron.emails,tmp,10)); 
			useron.etoday++;
			putuserrec(&cfg,useron.number,U_ETODAY,5,ultoa(useron.etoday,tmp,10));

			sprintf(str,"%s sent %s NetMail to %s (%s) via QWK"
				,useron.alias
				,qnet ? "QWK":"Internet",name,qnet ? fulladdr : to);
			logline("EN",str); 
		}

		free((char *)qwkbuf);
		return; 
	}


	/****************************** FidoNet **********************************/

	if(!fidoaddr.zone || !cfg.netmail_dir[0]) {  // No fido netmail allowed
		bputs(text[InvalidNetMailAddr]);
		free(qwkbuf);
		return; 
	}

	memset(&hdr,0,sizeof(hdr));   /* Initialize header to null */

	if(fromhub || useron.rest&FLAG('Q')) {
		sprintf(str,"%.25s",block+46);              /* From */
		truncsp(str);
		sprintf(tmp,"@%s",fromhub ? cfg.qhub[fromhub-1]->id : useron.alias);
		strupr(tmp);
		strcat(str,tmp); 
	}
	else
		SAFECOPY(str,cfg.netmail_misc&NMAIL_ALIAS ? useron.alias : useron.name);
	SAFECOPY(hdr.from,str);

	SAFECOPY(hdr.to,to);

	/* Look-up in nodelist? */

	if(cfg.netmail_cost && !(useron.exempt&FLAG('S'))) {
		if(useron.cdt+useron.freecdt<cfg.netmail_cost) {
			bputs(text[NotEnoughCredits]);
			free(qwkbuf);
			return; 
		}
		sprintf(str,text[NetMailCostContinueQ],cfg.netmail_cost);
		if(noyes(str)) {
			free(qwkbuf);
			return; 
		} 
	}

	hdr.destzone	=fidoaddr.zone;
	hdr.destnet 	=fidoaddr.net;
	hdr.destnode	=fidoaddr.node;
	hdr.destpoint	=fidoaddr.point;

	for(i=0;i<cfg.total_faddrs;i++)
		if(fidoaddr.zone==cfg.faddr[i].zone && fidoaddr.net==cfg.faddr[i].net)
			break;
	if(i==cfg.total_faddrs) {
		for(i=0;i<cfg.total_faddrs;i++)
			if(fidoaddr.zone==cfg.faddr[i].zone)
				break; 
	}
	if(i==cfg.total_faddrs)
		i=0;
	hdr.origzone	=cfg.faddr[i].zone;
	hdr.orignet 	=cfg.faddr[i].net;
	hdr.orignode	=cfg.faddr[i].node;
	hdr.origpoint   =cfg.faddr[i].point;

	smb_faddrtoa(&cfg.faddr[i],str);
	bprintf(text[NetMailing],hdr.to,smb_faddrtoa(&fidoaddr,tmp),hdr.from,str);
	tm.tm_mon=((qwkbuf[8]&0xf)*10)+(qwkbuf[9]&0xf);
	if (tm.tm_mon) tm.tm_mon--;
	tm.tm_mday=((qwkbuf[11]&0xf)*10)+(qwkbuf[12]&0xf);
	tm.tm_year=((qwkbuf[14]&0xf)*10)+(qwkbuf[15]&0xf)+1900;
	tm.tm_hour=((qwkbuf[16]&0xf)*10)+(qwkbuf[17]&0xf);
	tm.tm_min=((qwkbuf[19]&0xf)*10)+(qwkbuf[20]&0xf);		/* From QWK time */
	tm.tm_sec=0;
	sprintf(hdr.time,"%02u %3.3s %02u  %02u:%02u:%02u"          /* To FidoNet */
		,tm.tm_mday,mon[tm.tm_mon],TM_YEAR(tm.tm_year)
		,tm.tm_hour,tm.tm_min,tm.tm_sec);
	hdr.attr=(FIDO_LOCAL|FIDO_PRIVATE);

	if(cfg.netmail_misc&NMAIL_CRASH) hdr.attr|=FIDO_CRASH;
	if(cfg.netmail_misc&NMAIL_HOLD)  hdr.attr|=FIDO_HOLD;
	if(cfg.netmail_misc&NMAIL_KILL)  hdr.attr|=FIDO_KILLSENT;

	sprintf(str,"%.25s",block+71);      /* Title */
	truncsp(str);
	p=str;
	if((SYSOP || useron.exempt&FLAG('F'))
		&& !strnicmp(p,"CR:",3)) {     /* Crash over-ride by sysop */
		p+=3;               /* skip CR: */
		if(*p==' ') p++;     /* skip extra space if it exists */
		hdr.attr|=FIDO_CRASH; 
	}

	if((SYSOP || useron.exempt&FLAG('F'))
		&& !strnicmp(p,"FR:",3)) {     /* File request */
		p+=3;               /* skip FR: */
		if(*p==' ') p++;
		hdr.attr|=FIDO_FREQ; 
	}

	if((SYSOP || useron.exempt&FLAG('F'))
		&& !strnicmp(p,"RR:",3)) {     /* Return receipt request */
		p+=3;               /* skip RR: */
		if(*p==' ') p++;
		hdr.attr|=FIDO_RRREQ; 
	}

	if((SYSOP || useron.exempt&FLAG('F'))
		&& !strnicmp(p,"FA:",3)) {     /* File attachment */
		p+=3;				/* skip FA: */
		if(*p==' ') p++;
		hdr.attr|=FIDO_FILE; 
	}

	SAFECOPY(hdr.subj,p);

	md(cfg.netmail_dir);
	for(i=1;i;i++) {
		sprintf(str,"%s%u.msg", cfg.netmail_dir,i);
		if(!fexistcase(str))
			break; 
	}
	if(!i) {
		bputs(text[TooManyEmailsToday]);
		return; 
	}
	if((fido=nopen(str,O_WRONLY|O_CREAT|O_EXCL))==-1) {
		free(qwkbuf);
		errormsg(WHERE,ERR_OPEN,str,O_WRONLY|O_CREAT|O_EXCL);
		return; 
	}
	write(fido,&hdr,sizeof(hdr));

	pt_zone_kludge(hdr,fido);

	if(cfg.netmail_misc&NMAIL_DIRECT) {
		sprintf(str,"\1FLAGS DIR\r\n");
		write(fido,str,strlen(str)); 
	}

	l=QWK_BLOCK_LEN;

	if(into==NULL) {	  /* If name@addr on first line, skip first line */
		while(l<n*QWK_BLOCK_LEN && qwkbuf[l]!=QWK_NEWLINE) l++;
		l++; 
	}

	length=n*QWK_BLOCK_LEN;
	while(l<length) {
		if(qwkbuf[l]==CTRL_A) {   /* Ctrl-A, so skip it and the next char */
			l++;
			if(l>=length || toupper(qwkbuf[l])=='Z')	/* EOF */
				break;
			if((ch=ctrl_a_to_ascii_char(qwkbuf[l])) != 0)
				write(fido,&ch,1);
		}
		else if(qwkbuf[l]!=LF) {
			if(qwkbuf[l]==QWK_NEWLINE) /* QWK cr/lf char converted to hard CR */
				qwkbuf[l]=CR;
			write(fido,(char *)qwkbuf+l,1); 
		}
		l++;
	}
	l=0;
	write(fido,&l,1);	/* Null terminator */
	close(fido);
	free((char *)qwkbuf);
	if(cfg.netmail_sem[0])		/* update semaphore file */
		ftouch(cmdstr(cfg.netmail_sem,nulstr,nulstr,NULL));
	if(!(useron.exempt&FLAG('S')))
		subtract_cdt(&cfg,&useron,cfg.netmail_cost);

	useron.emails++;
	logon_emails++;
	putuserrec(&cfg,useron.number,U_EMAILS,5,ultoa(useron.emails,tmp,10)); 
	useron.etoday++;
	putuserrec(&cfg,useron.number,U_ETODAY,5,ultoa(useron.etoday,tmp,10));

	sprintf(str,"%s sent NetMail to %s @%s via QWK"
		,useron.alias
		,hdr.to,smb_faddrtoa(&fidoaddr,tmp));
	logline("EN",str);
}

/****************************************************************************/
/* Returns the FidoNet address kept in str as ASCII.                        */
/****************************************************************************/
faddr_t atofaddr(scfg_t* cfg, char *str)
{
	char *p;
	faddr_t addr;

	addr.zone=addr.net=addr.node=addr.point=0;
	if((p=strchr(str,':'))!=NULL) {
		addr.zone=atoi(str);
		addr.net=atoi(p+1); 
	}
	else {
		if(cfg->total_faddrs)
			addr.zone=cfg->faddr[0].zone;
		else
			addr.zone=1;
		addr.net=atoi(str); 
	}
	if(!addr.zone)              /* no such thing as zone 0 */
		addr.zone=1;
	if((p=strchr(str,'/'))!=NULL)
		addr.node=atoi(p+1);
	else {
		if(cfg->total_faddrs)
			addr.net=cfg->faddr[0].net;
		else
			addr.net=1;
		addr.node=atoi(str); 
	}
	if((p=strchr(str,'.'))!=NULL)
		addr.point=atoi(p+1);
	return(addr);
}
