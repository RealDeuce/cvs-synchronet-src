/* smbdump.c */

/* Synchronet message base (SMB) message header dumper */

/* $Id: smbdump.c,v 1.15 2015/12/07 03:37:35 rswindell Exp $ */

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

#include <time.h>		/* ctime */
#include <string.h>		/* strcat */
#include "smblib.h"

static char *binstr(uchar *buf, uint16_t length)
{
	static char str[512];
	int i;

	str[0]=0;
	for(i=0;i<length;i++)
		if(buf[i] && (buf[i]<' ' || buf[i]>=0x7f) 
			&& buf[i]!='\r' && buf[i]!='\n' && buf[i]!='\t')
			break;
	if(i==length)		/* not binary */
		return((char*)buf);
	for(i=0;i<length;i++) {
		sprintf(str+strlen(str),"%02X ",buf[i]);
		if(i>=100) {
			strcat(str,"...");
			break;
		}
	}
	truncsp(str);
	return(str);
}

void SMBCALL smb_dump_msghdr(FILE* fp, smbmsg_t* msg)
{
	int i;
	time_t	tt;

	fprintf(fp,"%-20.20s %"PRId32"\n"		,"number"			,msg->hdr.number);

	/* convenience strings */
	if(msg->subj)
		fprintf(fp,"%-20.20s \"%s\"\n"	,"subject"			,msg->subj);
	if(msg->to) {
		fprintf(fp,"%-20.20s %s"	,"to"				,msg->to);
		if(msg->to_net.type)
			fprintf(fp," (%s: %s)",smb_nettype(msg->to_net.type), smb_netaddr(&msg->to_net));
		if(msg->to_ext)
			fprintf(fp," #%s",msg->to_ext);
		fprintf(fp,"\n");
	}
	if(msg->from) {
		fprintf(fp,"%-20.20s \"%s\""	,"from"				,msg->from);
		if(msg->from_net.type)
			fprintf(fp," (%s: %s)",smb_nettype(msg->from_net.type), smb_netaddr(&msg->from_net));
		if(msg->from_ext)
			fprintf(fp," #%s",msg->from_ext);
		fprintf(fp,"\n");
	}
	if(msg->replyto) {
		fprintf(fp,"%-20.20s \"%s\""	,"reply-to"			,msg->replyto);
		if(msg->replyto_ext)
			fprintf(fp," #%s",msg->replyto_ext);
		if(msg->replyto_net.type)
			fprintf(fp," (%s: %s)",smb_nettype(msg->replyto_net.type), smb_netaddr(&msg->replyto_net));
		fprintf(fp,"\n");
	}
	if(msg->summary)
		fprintf(fp,"%-20.20s \"%s\"\n"	,"summary"			,msg->summary);

	/* convenience integers */
	if(msg->expiration) {
		tt=msg->expiration;
		fprintf(fp,"%-20.20s %.24s\n","expiration"	
			,ctime(&tt));
	}

	/* fixed header fields */
	tt=msg->hdr.when_written.time;
	fprintf(fp,"%-20.20s %.24s  UTC%+d:%02d\n"	,"when_written"	
		,ctime(&tt)	
		,smb_tzutc(msg->hdr.when_written.zone)/60
		,abs(smb_tzutc(msg->hdr.when_written.zone)%60));
	tt=msg->hdr.when_imported.time;
	fprintf(fp,"%-20.20s %.24s  UTC%+d:%02d\n"	,"when_imported"	
		,ctime(&tt)	
		,smb_tzutc(msg->hdr.when_imported.zone)/60
		,abs(smb_tzutc(msg->hdr.when_imported.zone)%60));
	fprintf(fp,"%-20.20s %04Xh\n"	,"type"				,msg->hdr.type);
	fprintf(fp,"%-20.20s %04Xh\n"	,"version"			,msg->hdr.version);
	fprintf(fp,"%-20.20s %04Xh\n"	,"attr"				,msg->hdr.attr);
	fprintf(fp,"%-20.20s %08"PRIX32"h\n"	,"auxattr"			,msg->hdr.auxattr);
	fprintf(fp,"%-20.20s %08"PRIX32"h\n"	,"netattr"			,msg->hdr.netattr);

	/* optional fixed fields */
	if(msg->hdr.thread_id)
		fprintf(fp,"%-20.20s %"PRId32"\n"	,"thread_id"		,msg->hdr.thread_id);
	if(msg->hdr.thread_back)
		fprintf(fp,"%-20.20s %"PRId32"\n"	,"thread_back"		,msg->hdr.thread_back);
	if(msg->hdr.thread_next)
		fprintf(fp,"%-20.20s %"PRId32"\n"	,"thread_next"		,msg->hdr.thread_next);
	if(msg->hdr.thread_first)
		fprintf(fp,"%-20.20s %"PRId32"\n"	,"thread_first"		,msg->hdr.thread_first);
	if(msg->hdr.delivery_attempts)
		fprintf(fp,"%-20.20s %hu\n"	,"delivery_attempts",msg->hdr.delivery_attempts);
	if(msg->hdr.times_downloaded)
		fprintf(fp,"%-20.20s %"PRIu32"\n"	,"times_downloaded"	,msg->hdr.times_downloaded);
	if(msg->hdr.last_downloaded) {
		tt=msg->hdr.last_downloaded;
		fprintf(fp,"%-20.20s %.24s\n"	,"last_downloaded"	,ctime(&tt));
	}

	fprintf(fp,"%-20.20s %06"PRIX32"h\n"	,"header offset"	,msg->idx.offset);
	fprintf(fp,"%-20.20s %u\n"		,"header length"	,msg->hdr.length);
	fprintf(fp,"%-20.20s %lu\n"		,"calculated length",smb_getmsghdrlen(msg));

	/* variable fields */
	for(i=0;i<msg->total_hfields;i++)
		fprintf(fp,"%-20.20s \"%s\"\n"
			,smb_hfieldtype(msg->hfield[i].type)
			,binstr((uchar *)msg->hfield_dat[i],msg->hfield[i].length));

	/* data fields */
	fprintf(fp,"%-20.20s %06"PRIX32"h\n"	,"data offset"		,msg->hdr.offset);
	for(i=0;i<msg->hdr.total_dfields;i++)
		fprintf(fp,"data field[%u]        %s, offset %"PRIu32", length %"PRIu32"\n"
			,i
			,smb_dfieldtype(msg->dfield[i].type)
			,msg->dfield[i].offset
			,msg->dfield[i].length);
}
