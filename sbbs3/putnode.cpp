/* putnode.cpp */

/* Synchronet node information writing routines */

/* $Id: putnode.cpp,v 1.6 2001/06/06 00:28:24 rswindell Exp $ */

/****************************************************************************
 * @format.tab-size 4		(Plain Text/Source Code File Header)			*
 * @format.use-tabs true	(see http://www.synchro.net/ptsc_hdr.html)		*
 *																			*
 * Copyright 2000 Rob Swindell - http://www.synchro.net/copyright.html		*
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

/****************************************************************************/
/* Write the data from the structure 'node' into node.dab  					*/
/* getnodedat(num,&node,1); must have been called before calling this func  */
/*          NOTE: ------^   the indicates the node record has been locked   */
/****************************************************************************/
void sbbs_t::putnodedat(uint number, node_t* node)
{
	char	str[256],firston[25];
	int		wr;
	int		wrerr;
	int		attempts;

	if(!number || number>cfg.sys_nodes) {
		errormsg(WHERE,ERR_CHK,"node number",number);
		return; 
	}
	if(number==cfg.node_num) {
		if((node->status==NODE_INUSE || node->status==NODE_QUIET)
			&& node->action<NODE_LAST_ACTION
			&& text[NodeActionMain+node->action][0]) {
			node->misc|=NODE_EXT;
			memset(str,0,128);
			sprintf(str,text[NodeActionMain+node->action]
				,useron.alias
				,useron.level
				,getage(&cfg,useron.birth)
				,useron.sex
				,useron.comp
				,useron.note
				,unixtodstr(&cfg,useron.firston,firston)
				,node->aux&0xff
				,node->connection
				);
			putnodeext(number,str); }
		else
			node->misc&=~NODE_EXT; }

	if(nodefile==-1) {
		sprintf(str,"%snode.dab",cfg.ctrl_dir);
		if((nodefile=nopen(str,O_CREAT|O_RDWR|O_DENYNONE))==-1) {
			errormsg(WHERE,ERR_OPEN,str,O_CREAT|O_RDWR|O_DENYNONE);
			return; 
		}
	}

	number--;	/* make zero based */
	for(attempts=0;attempts<10;attempts++) {
		lseek(nodefile,(long)number*sizeof(node_t),SEEK_SET);
		wr=write(nodefile,node,sizeof(node_t));
		if(wr==sizeof(node_t))
			break;
		mswait(100);
	}
	wrerr=errno;	/* save write error */
	unlock(nodefile,(long)number*sizeof(node_t),sizeof(node_t));
	close(nodefile);
	nodefile=-1;

	if(wr!=sizeof(node_t)) {
		errno=wrerr;
		errormsg(WHERE,ERR_WRITE,"nodefile",number+1);
	}
}

/****************************************************************************/
/* Creates a short message for node 'num' than contains 'strin'             */
/****************************************************************************/
void sbbs_t::putnmsg(int num, char *strin)
{
    char str[256];
    int file,i;
    node_t node;

	sprintf(str,"%smsgs/n%3.3u.msg",cfg.data_dir,num);
	if((file=nopen(str,O_WRONLY|O_CREAT))==-1) {
		errormsg(WHERE,ERR_OPEN,str,O_WRONLY|O_CREAT);
		return; }
	lseek(file,0L,SEEK_END);	// Instead of opening with O_APPEND
	i=strlen(strin);
	if(write(file,strin,i)!=i) {
		close(file);
		errormsg(WHERE,ERR_WRITE,str,i);
		return; }
	close(file);
	getnodedat(num,&node,0);
	if((node.status==NODE_INUSE || node.status==NODE_QUIET)
		&& !(node.misc&NODE_NMSG)) {
		getnodedat(num,&node,1);
		node.misc|=NODE_NMSG;
		putnodedat(num,&node); }
}

void sbbs_t::putnodeext(uint number, char *ext)
{
    char	str[MAX_PATH];
    int		count;

	if(!number || number>cfg.sys_nodes) {
		errormsg(WHERE,ERR_CHK,"node number",number);
		return; }
	number--;   /* make zero based */

	sprintf(str,"%snode.exb",cfg.ctrl_dir);
	if((node_ext=nopen(str,O_CREAT|O_RDWR|O_DENYNONE))==-1) {
		errormsg(WHERE,ERR_OPEN,str,O_CREAT|O_RDWR|O_DENYNONE);
		return; 
	}
	for(count=0;count<LOOP_NODEDAB;count++) {
		if(count)
			mswait(100);
		lseek(node_ext,(long)number*128L,SEEK_SET);
		if(lock(node_ext,(long)number*128L,128)==-1) 
			continue; 
		if(write(node_ext,ext,128)==128)
			break;
	}
	unlock(node_ext,(long)number*128L,128);
	close(node_ext);
	node_ext=-1;

	if(count>(LOOP_NODEDAB/2) && count!=LOOP_NODEDAB) {
		sprintf(str,"NODE.EXB COLLISION - Count: %d",count);
		logline("!!",str); 
	}
	if(count==LOOP_NODEDAB) {
		errormsg(WHERE,ERR_WRITE,"NODE.EXB",number+1);
		return; 
	}
}


