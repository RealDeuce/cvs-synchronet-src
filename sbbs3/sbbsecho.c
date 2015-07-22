/* sbbsecho.c */

/* Synchronet FidoNet EchoMail Scanning/Tossing and NetMail Tossing Utility */

/* $Id: sbbsecho.c,v 1.261 2015/07/22 23:46:14 rswindell Exp $ */

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

/* Portions written by Allen Christiansen 1994-1996 						*/

#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef __WATCOMC__
	#include <mem.h>
#endif

#ifndef __unix__
	#include <malloc.h>
#endif

#include "conwrap.h"		/* getch() */
#include "sbbs.h"			/* load_cfg() */
#include "sbbsdefs.h"
#include "smblib.h"
#include "scfglib.h"
#include "lzh.h"
#include "sbbsecho.h"
#include "genwrap.h"		/* PLATFORM_DESC */

smb_t *smb,*email;
long misc=(IMPORT_PACKETS|IMPORT_NETMAIL|IMPORT_ECHOMAIL|EXPORT_ECHOMAIL
			|DELETE_NETMAIL|DELETE_PACKETS);
ulong netmail=0;
char tmp[256],pkt_type=0;
int secure,cur_smb=0;
FILE *fidologfile=NULL;
BOOL twit_list;

faddr_t		sys_faddr = {1,1,1,0};		/* Default system address: 1:1/1.0 */
config_t	cfg;
scfg_t		scfg;
char		revision[16];
char		compiler[32];

BOOL pause_on_exit=FALSE;
BOOL pause_on_abend=FALSE;

#if !defined(_WIN32)
#define delfile(x) remove(x)
#else
int delfile(char *filename)
{
	int i=0;

	while(remove(filename) && i++<120)	/* Wait up to 60 seconds to delete file */
		Sleep(500); 					/* for Win95 bug fix */
	return(i);
}
#endif

#if defined(__unix__)	/* borrowed from MSVC */
unsigned _rotr (
        unsigned val,
        int shift
        )
{
    register unsigned lobit;        /* non-zero means lo bit set */
    register unsigned num = val;    /* number to rotate */

    shift &= 0x1f;                  /* modulo 32 -- this will also make
                                       negative shifts work */
    while (shift--) {
	    lobit = num & 1;        /* get high bit */
        num >>= 1;              /* shift right one bit */
        if (lobit)
			num |= 0x80000000;  /* set hi bit if lo bit was set */
	}

    return num;
}
#endif

void logprintf(char *str, ...);

/****************************************************************************/
/* This is needed by load_cfg.c												*/
/****************************************************************************/
int lprintf(int level, char *fmat, ...)
{
	va_list argptr;
	char sbuf[256];
	int chcount;

	va_start(argptr,fmat);
	chcount=vsnprintf(sbuf,sizeof(sbuf),fmat,argptr);
	sbuf[sizeof(sbuf)-1]=0;
	va_end(argptr);
	truncsp(sbuf);
	printf("%s\n",sbuf);

	if(level<=cfg.log_level)
		logprintf("%s",sbuf);
	return(chcount);
}

/**********************/
/* Log print function */
/**********************/
void logprintf(char *str, ...)
{
    va_list argptr;
    char buf[256];
    time_t now;
    struct tm *gm;

	if(!(misc&LOGFILE) || fidologfile==NULL)
		return;
	va_start(argptr,str);
	vsnprintf(buf,sizeof(buf),str,argptr);
	buf[sizeof(buf)-1]=0;
	va_end(argptr);
	strip_ctrl(buf, buf);
	now=time(NULL);
	gm=localtime(&now);
	fprintf(fidologfile,"%02u/%02u/%02u %02u:%02u:%02u %s\n"
		,(scfg.sys_misc&SM_EURODATE) ? gm->tm_mday : gm->tm_mon+1
		,(scfg.sys_misc&SM_EURODATE) ? gm->tm_mon+1 : gm->tm_mday
		,TM_YEAR(gm->tm_year),gm->tm_hour,gm->tm_min,gm->tm_sec
		,buf);
}

/*****************************************************************************/
/* Returns command line generated from instr with %c replacments             */
/*****************************************************************************/
char *mycmdstr(scfg_t* cfg, char *instr, char *fpath, char *fspec)
{
    static char cmd[MAX_PATH+1];
    char str[256],str2[128];
    int i,j,len;

	len=strlen(instr);
	for(i=j=0;i<len && j<128;i++) {
		if(instr[i]=='%') {
			i++;
			cmd[j]=0;
			switch(toupper(instr[i])) {
				case 'F':   /* File path */
					strcat(cmd,fpath);
					break;
				case 'G':   /* Temp directory */
					if(cfg->temp_dir[0]!='\\' 
						&& cfg->temp_dir[0]!='/' 
						&& cfg->temp_dir[1]!=':') {
						strcpy(str,cfg->node_dir);
						strcat(str,cfg->temp_dir);
						if(FULLPATH(str2,str,40))
							strcpy(str,str2);
						backslash(str);
						strcat(cmd,str);}
					else
						strcat(cmd,cfg->temp_dir);
					break;
				case 'J':
					if(cfg->data_dir[0]!='\\' 
						&& cfg->data_dir[0]!='/' 
						&& cfg->data_dir[1]!=':') {
						strcpy(str,cfg->node_dir);
						strcat(str,cfg->data_dir);
						if(FULLPATH(str2,str,40))
							strcpy(str,str2);
						backslash(str);
						strcat(cmd,str); 
					}
					else
						strcat(cmd,cfg->data_dir);
					break;
				case 'K':
					if(cfg->ctrl_dir[0]!='\\' 
						&& cfg->ctrl_dir[0]!='/' 
						&& cfg->ctrl_dir[1]!=':') {
						strcpy(str,cfg->node_dir);
						strcat(str,cfg->ctrl_dir);
						if(FULLPATH(str2,str,40))
							strcpy(str,str2);
						backslash(str);
						strcat(cmd,str); 
					}
					else
						strcat(cmd,cfg->ctrl_dir);
					break;
				case 'N':   /* Node Directory (same as SBBSNODE environment var) */
					strcat(cmd,cfg->node_dir);
					break;
				case 'O':   /* SysOp */
					strcat(cmd,cfg->sys_op);
					break;
				case 'Q':   /* QWK ID */
					strcat(cmd,cfg->sys_id);
					break;
				case 'S':   /* File Spec */
					strcat(cmd,fspec);
					break;
				case '!':   /* EXEC Directory */
					strcat(cmd,cfg->exec_dir);
					break;
                case '@':   /* EXEC Directory for DOS/OS2/Win32, blank for Unix */
#ifndef __unix__
                    strcat(cmd,cfg->exec_dir);
#endif
                    break;
				case '#':   /* Node number (same as SBBSNNUM environment var) */
					sprintf(str,"%d",cfg->node_num);
					strcat(cmd,str);
					break;
				case '*':
					sprintf(str,"%03d",cfg->node_num);
					strcat(cmd,str);
					break;
				case '%':   /* %% for percent sign */
					strcat(cmd,"%");
					break;
				case '.':	/* .exe for DOS/OS2/Win32, blank for Unix */
#ifndef __unix__
					strcat(cmd,".exe");
#endif
					break;
				case '?':	/* Platform */
#ifdef __OS2__
					strcpy(str,"OS2");
#else
					strcpy(str,PLATFORM_DESC);
#endif
					strlwr(str);
					strcat(cmd,str);
					break;
				default:    /* unknown specification */
					lprintf(LOG_ERR,"ERROR Checking Command Line '%s'",instr);
					bail(1);
					break; 
			}
			j=strlen(cmd); 
		}
		else
			cmd[j++]=instr[i]; 
}
	cmd[j]=0;

	return(cmd);
}

/****************************************************************************/
/* Runs an external program directly using spawnvp							*/
/****************************************************************************/
int execute(char *cmdline)
{
#if 1
	return system(cmdline);
#else
	char c,d,e,cmdlen,*arg[30],str[256];
	int i;

	strcpy(str,cmdline);
	arg[0]=str;	/* point to the beginning of the string */
	cmdlen=strlen(str);
	for(c=0,d=1,e=0;c<cmdlen;c++,e++)	/* Break up command line */
		if(str[c]==' ') {
			str[c]=0;			/* insert nulls */
			arg[d++]=str+c+1;	/* point to the beginning of the next arg */
			e=0; }
	arg[d]=0;
	i=spawnvp(P_WAIT,arg[0],arg);
	return(i);
#endif
}

/******************************************************************************
 Returns the system address with the same zone as the address passed
******************************************************************************/
faddr_t getsysfaddr(short zone)
{
	int i;

	for(i=0;i<scfg.total_faddrs;i++)
		if(scfg.faddr[i].zone==zone)
			return(scfg.faddr[i]);
	return(sys_faddr);
}

/******************************************************************************
 This function creates or appends on existing Binkley compatible .?LO file
 attach file.
 Returns 0 on success.
******************************************************************************/
int write_flofile(char *attachment, faddr_t dest, BOOL bundle)
{
	char fname[MAX_PATH+1];
	char outbound[MAX_PATH+1];
	char str[MAX_PATH+1];
	char ch;
	char searchstr[MAX_PATH+1];
	ushort attr=0;
	int i;
	FILE *stream;

	i=matchnode(dest,0);
	if(i<(int)cfg.nodecfgs)
		attr=cfg.nodecfg[i].attr;

	if(attr&ATTR_CRASH) ch='c';
	else if(attr&ATTR_HOLD) ch='h';
	else if(attr&ATTR_DIRECT) ch='d';
	else ch='f';
	if(dest.zone==sys_faddr.zone)		/* Default zone, use default outbound */
		SAFECOPY(outbound,cfg.outbound);
	else {								/* Inter-zone outbound is OUTBOUND.XXX */
		SAFEPRINTF3(outbound,"%.*s.%03x"
			,(int)strlen(cfg.outbound)-1,cfg.outbound,dest.zone);
		MKDIR(outbound);
		backslash(outbound);
	}
	if(dest.point) {					/* Point destination is OUTBOUND\*.PNT */
		sprintf(str,"%04x%04x.pnt"
			,dest.net,dest.node);
		strcat(outbound,str); 
	}
	if(outbound[strlen(outbound)-1]=='\\'
		|| outbound[strlen(outbound)-1]=='/')
		outbound[strlen(outbound)-1]=0;
	MKDIR(outbound);
	backslash(outbound);
	if(dest.point)
		sprintf(fname,"%s%08x.%clo",outbound,dest.point,ch);
	else
		sprintf(fname,"%s%04x%04x.%clo",outbound,dest.net,dest.node,ch);
	if(bundle && (misc&TRUNC_BUNDLES))
		ch='#';
	else
		ch='^';
	if(*attachment == '^')	/* work-around for BRE/FE inter-BBS attachment bug */
		attachment++;
	fexistcase(attachment);	/* just in-case it's the wrong case for a Unix file system */
	sprintf(searchstr,"%c%s",ch,attachment);
	if(findstr(searchstr,fname))	/* file already in FLO file */
		return(0);
	if((stream=fopen(fname,"a"))==NULL) {
		lprintf(LOG_ERR,"ERROR %u (%s) line %d opening %s",errno,strerror(errno),__LINE__,fname);
		return(-1); 
	}
	fprintf(stream,"%s\n",searchstr);
	fclose(stream);
	return(0);
}

/* Writes text buffer to file, expanding sole LFs to CRLFs */
size_t fwrite_crlf(char* buf, size_t len, FILE* fp)
{
	char	ch,last_ch=0;
	size_t	i;
	size_t	wr=0;	/* total chars written (may be > len) */

	for(i=0;i<len;i++) {
		ch=*buf++;
		if(ch=='\n' && last_ch!='\r') {
			if(fputc('\r',fp)==EOF)
				return(wr);
			wr++;
		}
		if(fputc(ch,fp)==EOF)
			return(wr);
		wr++;
		last_ch=ch;
	}

	return(wr);
}

/******************************************************************************
 This function will create a netmail message (.MSG format).
 If file is non-zero, will set file attachment bit (for bundles).
 Returns 0 on success.
******************************************************************************/
int create_netmail(char *to, char *subject, char *body, faddr_t dest, BOOL file_attached)
{
	FILE *fstream;
	char str[256],fname[MAX_PATH+1];
	ushort attr=0;
	int fmsg;
	uint i;
	static uint startmsg;
	time_t t;
	faddr_t	faddr;
	fmsghdr_t hdr;
	struct tm *tm;

	if(!startmsg) startmsg=1;
	i=matchnode(dest,0);
	if(i<cfg.nodecfgs) {
		attr=cfg.nodecfg[i].attr;
		if(!attr) {
			i=matchnode(dest,2);
			if(i<cfg.nodecfgs)
				attr=cfg.nodecfg[i].attr; 
		} 
	}

	MKDIR(scfg.netmail_dir);
	do {
		for(i=startmsg;i;i++) {
			sprintf(fname,"%s%u.msg",scfg.netmail_dir,i);
			if(!fexistcase(fname))
				break; 
		}
		if(!i) {
			lprintf(LOG_WARNING,"Directory full: %s",scfg.netmail_dir);
			return(-1); 
		}
		startmsg=i+1;
		if((fstream=fnopen(&fmsg,fname,O_RDWR|O_CREAT))==NULL) {
			lprintf(LOG_ERR,"ERROR %u (%s) line %d opening %s",errno,strerror(errno),__LINE__,fname);
			return(-1); 
		}

		faddr=getsysfaddr(dest.zone);
		memset(&hdr,0,sizeof(fmsghdr_t));
		hdr.origzone=faddr.zone;
		hdr.orignet=faddr.net;
		hdr.orignode=faddr.node;
		hdr.origpoint=faddr.point;
		hdr.destzone=dest.zone;
		hdr.destnet=dest.net;
		hdr.destnode=dest.node;
		hdr.destpoint=dest.point;

		hdr.attr=(FIDO_PRIVATE|FIDO_KILLSENT|FIDO_LOCAL);
		if(file_attached)
			hdr.attr|=FIDO_FILE;

		if(attr&ATTR_HOLD)
			hdr.attr|=FIDO_HOLD;
		if(attr&ATTR_CRASH)
			hdr.attr|=FIDO_CRASH;

		sprintf(hdr.from,"SBBSecho");

		t=time(NULL);
		tm=localtime(&t);
		sprintf(hdr.time,"%02u %3.3s %02u  %02u:%02u:%02u"
			,tm->tm_mday,mon[tm->tm_mon],TM_YEAR(tm->tm_year)
			,tm->tm_hour,tm->tm_min,tm->tm_sec);

		if(to)
			SAFECOPY(hdr.to,to);
		else
			SAFECOPY(hdr.to,"SYSOP");

		SAFECOPY(hdr.subj,subject);

		fwrite(&hdr,sizeof(fmsghdr_t),1,fstream);
		sprintf(str,"\1INTL %hu:%hu/%hu %hu:%hu/%hu\r"
			,hdr.destzone,hdr.destnet,hdr.destnode
			,hdr.origzone,hdr.orignet,hdr.orignode);
		fwrite(str,strlen(str),1,fstream);

		/* Add FSC-53 FLAGS kludge */
		fprintf(fstream,"\1FLAGS");
		if(attr&ATTR_DIRECT)
			fprintf(fstream," DIR");
		if(file_attached) {
			if(misc&TRUNC_BUNDLES)
				fprintf(fstream," TFS");
			else
				fprintf(fstream," KFS");
		}
		fprintf(fstream,"\r");

		if(hdr.destpoint) {
			sprintf(str,"\1TOPT %hu\r",hdr.destpoint);
			fwrite(str,strlen(str),1,fstream); 
		}
		if(hdr.origpoint) {
			sprintf(str,"\1FMPT %hu\r",hdr.origpoint);
			fwrite(str,strlen(str),1,fstream); 
		}
		if(!file_attached || (!(attr&ATTR_DIRECT) && file_attached))
			fwrite_crlf(body,strlen(body)+1,fstream);	/* Write additional NULL */
		else
			fwrite("\0",1,1,fstream);               /* Write NULL */
		fclose(fstream);
	} while(!fexistcase(fname));
	return(0);
}

/******************************************************************************
 This function takes the contents of 'infile' and puts it into a netmail
 message bound for addr.
******************************************************************************/
void file_to_netmail(FILE *infile,char *title,faddr_t addr,char *to)
{
	char *buf,*p;
	long l,m,len;

	l=len=ftell(infile);
	if(len>8192L)
		len=8192L;
	rewind(infile);
	if((buf=(char *)malloc(len+1))==NULL) {
		lprintf(LOG_ERR,"ERROR line %d allocating %lu for file to netmail buf",__LINE__,len);
		return; 
	}
	while((m=fread(buf,1,(len>8064L) ? 8064L:len,infile))>0) {
		buf[m]=0;
		if(l>8064L && (p=strrchr(buf,'\n'))!=NULL) {
			p++;
			if(*p) {
				*p=0;
				p++;
				fseek(infile,-1L,SEEK_CUR);
				while(*p) { 			/* Seek back to end of last line */
					p++;
					fseek(infile,-1L,SEEK_CUR); 
				} 
			} 
		}
		if(ftell(infile)<l)
			strcat(buf,"\r\nContinued in next message...\r\n");
		create_netmail(to,title,buf,addr,FALSE); 
	}
	free(buf);
}

/* Returns TRUE if area is linked with specified node address */
BOOL area_is_linked(unsigned area_num, faddr_t* addr)
{
	unsigned i;
	for(i=0;i<cfg.area[area_num].uplinks;i++)
		if(!memcmp(addr,&cfg.area[area_num].uplink[i],sizeof(faddr_t)))
			return TRUE;
	return FALSE;
}

/******************************************************************************
 This function sends a notify list to applicable nodes, this list includes the
 settings configured for the node, as well as a list of areas the node is
 connected to.
******************************************************************************/
void notify_list(void)
{
	FILE *	tmpf;
	char	str[256];
	uint	i,j,k;

	for(k=0;k<cfg.nodecfgs;k++) {

		if(!(cfg.nodecfg[k].attr&SEND_NOTIFY))
			continue;

		if((tmpf=tmpfile())==NULL) {
			lprintf(LOG_ERR,"ERROR line %d couldn't open tmpfile",__LINE__);
			return; 
		}

		fprintf(tmpf,"Following are the options set for your system and a list "
			"of areas\r\nyou are connected to.  Please make sure everything "
			"is correct.\r\n\r\n");
		fprintf(tmpf,"Packet Type       %s\r\n"
			,cfg.nodecfg[k].pkt_type==PKT_TWO ? "2"
			:cfg.nodecfg[k].pkt_type==PKT_TWO_TWO ? "2.2":"2+");
		fprintf(tmpf,"Archive Type      %s\r\n"
			,(cfg.nodecfg[k].arctype>cfg.arcdefs) ?
			 "None":cfg.arcdef[cfg.nodecfg[k].arctype].name);
		fprintf(tmpf,"Mail Status       %s\r\n"
			,cfg.nodecfg[k].attr&ATTR_CRASH ? "Crash"
			:cfg.nodecfg[k].attr&ATTR_HOLD ? "Hold" : "None");
		fprintf(tmpf,"Direct            %s\r\n"
			,cfg.nodecfg[k].attr&ATTR_DIRECT ? "Yes":"No");
		fprintf(tmpf,"Passive           %s\r\n"
			,cfg.nodecfg[k].attr&ATTR_PASSIVE ? "Yes":"No");
		fprintf(tmpf,"Remote AreaMgr    %s\r\n\r\n"
			,cfg.nodecfg[k].password[0] ? "Yes" : "No");

		fprintf(tmpf,"Connected Areas\r\n---------------\r\n");
		for(i=0;i<cfg.areas;i++) {
			sprintf(str,"%s\r\n",cfg.area[i].name);
			if(str[0]=='*')
				continue;
			if(area_is_linked(i,&cfg.nodecfg[k].faddr))
				fprintf(tmpf,"%s",str); 
		}

		if(ftell(tmpf))
			file_to_netmail(tmpf,"SBBSecho Notify List",cfg.nodecfg[k].faddr, /* To: */NULL);
		fclose(tmpf); 
	}
}

/******************************************************************************
 This function creates a netmail to addr showing a list of available areas (0),
 a list of connected areas (1), or a list of removed areas (2).
******************************************************************************/
enum arealist_type {
	 AREALIST_ALL			// %LIST
	,AREALIST_CONNECTED		// %QUERY
	,AREALIST_UNLINKED		// %UNLINKED
};
void netmail_arealist(enum arealist_type type, faddr_t addr, char* to)
{
	char str[256],title[128],match,*p,*tp;
	int i,j,k,x,y;
	str_list_t	area_list;

	if(type == AREALIST_ALL)
		strcpy(title,"List of Available Areas");
	else if(type == AREALIST_CONNECTED)
		strcpy(title,"List of Connected Areas");
	else
		strcpy(title,"List of Unlinked Areas");

	if((area_list=strListInit()) == NULL) {
		lprintf(LOG_ERR,"ERROR line %d couldn't allocate string list",__LINE__);
		return; 
	}

	/* Include relevant areas from the area file (e.g. areas.bbs): */
	for(i=0;i<cfg.areas;i++) {
		if((type == AREALIST_CONNECTED || (misc&ELIST_ONLY)) && !area_is_linked(i,&addr))
			continue;
		if(type == AREALIST_UNLINKED && area_is_linked(i,&addr))
			continue;
		strListPush(&area_list, cfg.area[i].name); 
	} 

	if(type != AREALIST_CONNECTED) {
		i=matchnode(addr,0);
		if(i<cfg.nodecfgs) {
			for(j=0;j<cfg.listcfgs;j++) {
				match=0;
				for(k=0;k<cfg.listcfg[j].numflags;k++) {
					if(match) break;
					for(x=0;x<cfg.nodecfg[i].numflags;x++) {
						if(!stricmp(cfg.listcfg[j].flag[k].flag
							,cfg.nodecfg[i].flag[x].flag)) {
							FILE* fp;
							if((fp=fopen(cfg.listcfg[j].listpath,"r"))==NULL) {
								lprintf(LOG_ERR,"ERROR %u (%s) line %d opening %s"
									,errno,strerror(errno),__LINE__,cfg.listcfg[j].listpath);
								match=1;
								break; 
							}
							while(!feof(fp)) {
								memset(str,0,sizeof(str));
								if(!fgets(str,sizeof(str),fp))
									break;
								truncsp(str);
								p=str;
								SKIP_WHITESPACE(p);
								if(*p==0 || *p==';')     /* Ignore Blank and Comment Lines */
									continue;
								tp=p;
								FIND_WHITESPACE(tp);
								*tp=0;
								for(y=0;y<cfg.areas;y++)
									if(!stricmp(cfg.area[y].name,p))
										break;
								if(y>=cfg.areas || !area_is_linked(y,&addr))
									strListPush(&area_list, p); 
							}
							fclose(fp);
							match=1;
							break; 
						}
					}
				} 
			} 
		} 
	}
	strListSortAlpha(area_list);
	if(!strListCount(area_list))
		create_netmail(to,title,"None.",addr,FALSE);
	else {
		FILE* fp;
		if((fp=tmpfile())==NULL) {
			lprintf(LOG_ERR,"ERROR line %d couldn't open tmpfile",__LINE__);
		} else {
			strListWriteFile(fp, area_list, "\r\n");
			file_to_netmail(fp,title,addr,to);
			fclose(fp);
		}
	}
	lprintf(LOG_INFO,"Created AreaFix response netmail with %s (%u areas)", title, strListCount(area_list));
	strListFree(&area_list);
}

int check_elists(char *areatag,faddr_t addr)
{
	FILE *stream;
	char str[1025],quit=0,*p,*tp;
	int i,j,k,x,match=0;

	i=matchnode(addr,0);
	if(i<cfg.nodecfgs) {
		for(j=0;j<cfg.listcfgs;j++) {
			quit=0;
			for(k=0;k<cfg.listcfg[j].numflags;k++) {
				if(quit) break;
				for(x=0;x<cfg.nodecfg[i].numflags;x++)
					if(!stricmp(cfg.listcfg[j].flag[k].flag
						,cfg.nodecfg[i].flag[x].flag)) {
						if((stream=fopen(cfg.listcfg[j].listpath,"r"))==NULL) {
							lprintf(LOG_ERR,"ERROR %u (%s) line %d opening %s"
								,errno,strerror(errno),__LINE__,cfg.listcfg[j].listpath);
							quit=1;
							break; 
						}
						while(!feof(stream)) {
							if(!fgets(str,sizeof(str),stream))
								break;
							p=str;
							SKIP_WHITESPACE(p);
							if(*p==';')     /* Ignore Comment Lines */
								continue;
							tp=p;
							FIND_WHITESPACE(tp);
							*tp=0;
							if(!stricmp(areatag,p)) {
								match=1;
								break; 
							} 
						}
						fclose(stream);
						quit=1;
						if(match)
							return(match);
						break; 
					} 
			} 
		} 
	}
	return(match);
}
/******************************************************************************
 Used by AREAFIX to add/remove/change areas in the areas file
******************************************************************************/
void alter_areas(str_list_t add_area, str_list_t del_area, faddr_t addr, char* to)
{
	FILE *nmfile,*afilein,*afileout,*fwdfile;
	char str[1024],fields[1024],field1[256],field2[256],field3[256]
		,outpath[MAX_PATH+1]
		,*outname,*p,*tp,nomatch=0,match=0;
	int i,j,k,x,y;
	ulong tagcrc;

	SAFECOPY(outpath,cfg.areafile);
	*getfname(outpath)=0;
	if((outname=tempnam(outpath,"AREAS"))==NULL) {
		lprintf(LOG_ERR,"ERROR tempnam(%s,AREAS)",outpath);
		return; 
	}
	if((nmfile=tmpfile())==NULL) {
		lprintf(LOG_ERR,"ERROR in tmpfile()");
		free(outname);
		return; 
	}
	if((afileout=fopen(outname,"w+"))==NULL) {
		lprintf(LOG_ERR,"ERROR %u (%s) line %d opening %s",errno,strerror(errno),__LINE__,outname);
		fclose(nmfile);
		free(outname);
		return; 
	}
	if((afilein=fopen(cfg.areafile,"r"))==NULL) {
		lprintf(LOG_ERR,"ERROR %u (%s) line %d opening %s",errno,strerror(errno),__LINE__,cfg.areafile);
		fclose(afileout);
		fclose(nmfile);
		free(outname);
		return; 
	}
	while(!feof(afilein)) {
		if(!fgets(fields,sizeof(fields),afilein))
			break;
		truncsp(fields);
		p=fields;
		SKIP_WHITESPACE(p);
		if(*p==';') {    /* Skip Comment Lines */
			fprintf(afileout,"%s\n",fields);
			continue; 
		}
		SAFECOPY(field1,p);         /* Internal Code Field */
		truncstr(field1," \t\r\n");
		FIND_WHITESPACE(p);
		SKIP_WHITESPACE(p);
		SAFECOPY(field2,p);         /* Areatag Field */
		truncstr(field2," \t\r\n");
		FIND_WHITESPACE(p);
		SKIP_WHITESPACE(p);
		if((tp=strchr(p,';'))!=NULL) {
			SAFECOPY(field3,p);     /* Comment Field (if any) */
			FIND_WHITESPACE(tp);
			*tp=0; 
		}
		else
			field3[0]=0;
		if(strListCount(del_area)) { 				/* Check for areas to remove */
			lprintf(LOG_DEBUG,"Removing areas for %s from %s", smb_faddrtoa(&addr,NULL), cfg.areafile);
			for(i=0;del_area[i]!=NULL;i++) {
				if(!stricmp(del_area[i],field2) ||
					!stricmp(del_area[0],"-ALL"))     /* Match Found */
					break; 
			}
			if(del_area[i]!=NULL) {
				for(i=0;i<cfg.areas;i++) {
					if(!stricmp(field2,cfg.area[i].name)) {
						lprintf(LOG_DEBUG,"Unlinking area (%s) for %s in %s", field2, smb_faddrtoa(&addr,NULL), cfg.areafile);
						if(!area_is_linked(i,&addr)) {
							fprintf(afileout,"%s\n",fields);
							/* bugfix here Mar-25-2004 (wasn't breaking for "-ALL") */
							if(stricmp(del_area[0],"-ALL"))
								fprintf(nmfile,"%s not connected.\r\n",field2);
							break; 
						}

						/* Added 12/4/95 to remove uplink from connected uplinks */

						for(k=j;k<cfg.area[i].uplinks-1;k++)
							memcpy(&cfg.area[i].uplink[k],&cfg.area[i].uplink[k+1]
								,sizeof(faddr_t));
						--cfg.area[i].uplinks;
						if(cfg.area[i].uplinks==0) {
							FREE_AND_NULL(cfg.area[i].uplink);
						} else {
							if((cfg.area[i].uplink=(faddr_t *)
								realloc(cfg.area[i].uplink,sizeof(faddr_t)
								*(cfg.area[i].uplinks)))==NULL) {
								lprintf(LOG_ERR,"ERROR line %d allocating memory for area "
									"#%u uplinks.",__LINE__,i+1);
								bail(1); 
								return;
							}
						}

						fprintf(afileout,"%-16s%-23s ",field1,field2);
						for(j=0;j<cfg.area[i].uplinks;j++) {
							if(!memcmp(&cfg.area[i].uplink[j],&addr
								,sizeof(faddr_t)))
								continue;
							fprintf(afileout,"%s "
								,smb_faddrtoa(&cfg.area[i].uplink[j],NULL)); 
						}
						if(field3[0])
							fprintf(afileout,"%s",field3);
						fprintf(afileout,"\n");
						fprintf(nmfile,"%s removed.\r\n",field2);
						break; 
					} 
				}
				if(i==cfg.areas)			/* Something screwy going on */
					fprintf(afileout,"%s\n",fields);
				continue; 
			} 				/* Area match so continue on */
		}
		if(strListCount(add_area)) { 				/* Check for areas to add */
			lprintf(LOG_DEBUG,"Adding areas for %s to %s", smb_faddrtoa(&addr,NULL), cfg.areafile);
			for(i=0;add_area[i]!=NULL;i++)
				if(!stricmp(add_area[i],field2) ||
					!stricmp(add_area[0],"+ALL"))      /* Match Found */
					break;
			if(add_area[i]!=NULL) {
				if(stricmp(add_area[i],"+ALL"))
					add_area[i][0]=0;  /* So we can check other lists */
				for(i=0;i<cfg.areas;i++) {
					if(!stricmp(field2,cfg.area[i].name)) {
						lprintf(LOG_DEBUG,"Linking area (%s) for %s in %s", field2, smb_faddrtoa(&addr,NULL), cfg.areafile);
						if(area_is_linked(i,&addr)) {
							fprintf(afileout,"%s\n",fields);
							fprintf(nmfile,"%s already connected.\r\n",field2);
							break; 
						}
						if((misc&ELIST_ONLY) && !check_elists(field2,addr)) {
							fprintf(afileout,"%s\n",fields);
							break; 
						}

						/* Added 12/4/95 to add uplink to connected uplinks */

						++cfg.area[i].uplinks;
						if((cfg.area[i].uplink=(faddr_t *)
							realloc(cfg.area[i].uplink,sizeof(faddr_t)
							*(cfg.area[i].uplinks)))==NULL) {
							lprintf(LOG_ERR,"ERROR line %d allocating memory for area "
								"#%u uplinks.",__LINE__,i+1);
							bail(1); 
							return;
						}
						memcpy(&cfg.area[i].uplink[cfg.area[i].uplinks-1],&addr,sizeof(faddr_t));

						fprintf(afileout,"%-16s%-23s ",field1,field2);
						for(j=0;j<cfg.area[i].uplinks;j++)
							fprintf(afileout,"%s "
								,smb_faddrtoa(&cfg.area[i].uplink[j],NULL));
						if(field3[0])
							fprintf(afileout,"%s",field3);
						fprintf(afileout,"\n");
						fprintf(nmfile,"%s added.\r\n",field2);
						break; 
					} 
				}
				if(i==cfg.areas)			/* Something screwy going on */
					fprintf(afileout,"%s\n",fields);
				continue;  					/* Area match so continue on */
			}
			nomatch=1; 						/* This area wasn't in there */
		}
		fprintf(afileout,"%s\n",fields);	/* No match so write back line */
	}
	fclose(afilein);
	if(nomatch || (strListCount(add_area) && !stricmp(add_area[0],"+ALL"))) {
		i=matchnode(addr,0);
		if(i<cfg.nodecfgs) {
			for(j=0;j<cfg.listcfgs;j++) {
				match=0;
				for(k=0;k<cfg.listcfg[j].numflags;k++) {
					if(match) break;
					for(x=0;x<cfg.nodecfg[i].numflags;x++) {
						if(!stricmp(cfg.listcfg[j].flag[k].flag
							,cfg.nodecfg[i].flag[x].flag)) {
							if((fwdfile=tmpfile())==NULL) {
								lprintf(LOG_ERR,"ERROR line %d opening forward temp "
									"file",__LINE__);
								match=1;
								break; 
							}
							if((afilein=fopen(cfg.listcfg[j].listpath,"r"))==NULL) {
								lprintf(LOG_ERR,"ERROR %u (%s) line %d opening %s"
									,errno,strerror(errno),__LINE__,cfg.listcfg[j].listpath);
								fclose(fwdfile);
								match=1;
								break; 
							}
							while(!feof(afilein)) {
								if(!fgets(str,sizeof(str),afilein))
									break;
								p=str;
								SKIP_WHITESPACE(p);
								if(*p==';')     /* Ignore Comment Lines */
									continue;
								tp=p;
								FIND_WHITESPACE(tp);
								*tp=0;
								if(add_area[0]!=NULL && stricmp(add_area[0],"+ALL")==0) {
									SAFECOPY(tmp,p);
									tagcrc=crc32(strupr(tmp),0);
									for(y=0;y<cfg.areas;y++)
										if(tagcrc==cfg.area[y].tag)
											break;
									if(y<cfg.areas)
										continue; 
								}
								for(y=0;add_area[y]!=NULL;y++)
									if((!stricmp(add_area[y],str) &&
										add_area[y][0]) ||
										!stricmp(add_area[0],"+ALL"))
										break;
								if(add_area[y]!=NULL) {
									fprintf(afileout,"%-16s%-23s","P",str);
									if(cfg.listcfg[j].forward.zone)
										fprintf(afileout," %s"
											,smb_faddrtoa(&cfg.listcfg[j].forward,NULL));
									fprintf(afileout," %s\n",smb_faddrtoa(&addr,NULL));
									fprintf(nmfile,"%s added.\r\n",str);
									if(stricmp(add_area[0],"+ALL"))
										add_area[y][0]=0;
									if(!(cfg.listcfg[j].misc&NOFWD)
										&& cfg.listcfg[j].forward.zone)
										fprintf(fwdfile,"%s\r\n",str); 
								} 
							}
							fclose(afilein);
							if(!(cfg.listcfg[j].misc&NOFWD) && ftell(fwdfile)>0)
								file_to_netmail(fwdfile,cfg.listcfg[j].password
									,cfg.listcfg[j].forward,/* To: */"Areafix");
							fclose(fwdfile);
							match=1;
							break; 
						}
					}
				} 
			} 
		} 
	}
	if(strListCount(add_area) && stricmp(add_area[0],"+ALL")) {
		for(i=0;add_area[i]!=NULL;i++)
			if(add_area[i][0])
				fprintf(nmfile,"%s not found.\r\n",add_area[i]); 
	}
	if(!ftell(nmfile))
		create_netmail(to,"Area Change Request","No changes made.",addr,FALSE);
	else
		file_to_netmail(nmfile,"Area Change Request",addr,to);
	fclose(nmfile);
	fclose(afileout);
	if(delfile(cfg.areafile))					/* Delete AREAS.BBS */
		lprintf(LOG_ERR,"ERROR line %d removing %s %s",__LINE__,cfg.areafile
			,strerror(errno));
	if(rename(outname,cfg.areafile))		   /* Rename new AREAS.BBS file */
		lprintf(LOG_ERR,"ERROR line %d renaming %s to %s",__LINE__,outname,cfg.areafile);
	free(outname);
}
/******************************************************************************
 Used by AREAFIX to add/remove/change uplink info in the configuration file
 old = the old setting for this option, new = what the setting is changing to
 option = 0 for compression type change
		  1 for areafix password change
		  2 to set this node to passive
		  3 to set this node to active (remove passive)
******************************************************************************/
void alter_config(faddr_t addr, char *old, char *new, int option)
{
	FILE *outfile,*cfgfile;
	char outpath[MAX_PATH+1],cmd[32],arcname[32],*outname,*p,*tp
		,match=0;
	char *afline=NULL;
	size_t afline_size;
	int cfgnum;
	int j,k;
	faddr_t taddr;

	cfgnum=matchnode(addr,0);
	SAFECOPY(outpath,cfg.cfgfile);
	*getfname(outpath)=0;
	if((outname=tempnam(outpath,"CFG"))==NULL) {
		lprintf(LOG_ERR,"ERROR tempnam(%s,CFG)",outpath);
		return;
	}
	if((outfile=fopen(outname,"w+"))==NULL) {
		lprintf(LOG_ERR,"ERROR %u (%s) line %d opening %s",errno,strerror(errno),__LINE__,outname);
		free(outname);
		return;
	}
	if((cfgfile=fopen(cfg.cfgfile,"r"))==NULL) {
		lprintf(LOG_ERR,"ERROR %u (%s) line %d opening %s",errno,strerror(errno),__LINE__,cfg.cfgfile);
		fclose(outfile);
		free(outname);
		return;
	}

	while(!feof(cfgfile)) {
		if(getdelim(&afline, &afline_size, '\n', cfgfile)==-1)
			break;
		truncsp(afline);
		p=afline;
		SKIP_WHITESPACE(p);
		if(*p==';') {
			fprintf(outfile,"%s\n",afline);
			continue;
		}
		SAFECOPY(cmd,p);
		truncstr(cmd," ");						/* Chop off at space */
		strupr(cmd);							/* Convert code to uppercase */
		FIND_WHITESPACE(p);						/* Skip code */
		SKIP_WHITESPACE(p);						/* Skip white space */

		if(option==0 && !strcmp(cmd,"USEPACKER")) {     /* Change Compression */
			if(*p) {
				SAFECOPY(arcname, p);
				truncstr(arcname," ");
				strupr(arcname);
				FIND_WHITESPACE(p);
				if(*p)
					p++;
				if(!stricmp(new,arcname)) {   /* Add to new definition */
					if(match) {
						fprintf(outfile,"%-10s %s %s\n", cmd, arcname, p);
					}
					else {
						fprintf(outfile,"%-10s %s %s %s\n",cmd,arcname
							,smb_faddrtoa(&cfg.nodecfg[cfgnum].faddr,NULL)
							,p);
						match=1;
					}
				}
				else if(!stricmp(old,arcname)) {	/* Remove from old def */
					for(j=k=0;j<cfg.nodecfgs;j++) {
						if(j==cfgnum)
							continue;
						if(cfg.nodecfg[j].arctype < cfg.arcdefs)
							tp = cfg.arcdef[cfg.nodecfg[j].arctype].name;
						else
							tp = "NONE";
						if(!stricmp(tp,arcname)) {
							if(!k) {
								fprintf(outfile,"%-10s %s",cmd,arcname);
								k++;
							}
							fprintf(outfile," %s"
								,smb_faddrtoa(&cfg.nodecfg[j].faddr,NULL));
						}
					}
					fprintf(outfile,"\n");
				}
			}
		}
		else if(option==1 && !strcmp(cmd,"AREAFIX")) {       /* Change Password */
			if(*p) {
				taddr=smb_atofaddr(&sys_faddr,p);
				if(!memcmp(&cfg.nodecfg[cfgnum].faddr,&taddr,sizeof(faddr_t))) {
					FIND_WHITESPACE(p); /* Skip over address */
					SKIP_WHITESPACE(p);	/* Skip over whitespace */
					FIND_WHITESPACE(p); /* Skip over password */
					SKIP_WHITESPACE(p);	/* Skip over whitespace */
					fprintf(outfile,"%-10s %s %s %s\n",cmd
						,smb_faddrtoa(&cfg.nodecfg[cfgnum].faddr,NULL),new,p);
				}
				else
					fprintf(outfile,"%-10s %s\n", cmd, p);
			}
		}
		else if(option>1 && !strcmp(cmd,"PASSIVE")) {        /* Toggle Passive Areas */
			match=1;
			for(j=k=0;j<cfg.nodecfgs;j++) {
				if(option==2 && j==cfgnum) {
					if(!k)
						fprintf(outfile,"%-10s",cmd);
					fprintf(outfile," %s",smb_faddrtoa(&cfg.nodecfg[j].faddr,NULL));
					k++;
					continue;
				}
				if(option==3 && j==cfgnum)
					continue;
				if(cfg.nodecfg[j].attr&ATTR_PASSIVE) {
					if(!k)
						fprintf(outfile,"%-10s",cmd);
					fprintf(outfile," %s",smb_faddrtoa(&cfg.nodecfg[j].faddr,NULL));
					k++;
				}
			}
			if(k)
				fprintf(outfile,"\n");
		}
		else
			fprintf(outfile,"%s\n",afline);
	}

	if(!match) {
		if(option==0)
			fprintf(outfile,"%-10s %s %s\n","USEPACKER",new
				,smb_faddrtoa(&cfg.nodecfg[cfgnum].faddr,NULL));
		if(option==2)
			fprintf(outfile,"%-10s %s\n","PASSIVE"
				,smb_faddrtoa(&cfg.nodecfg[cfgnum].faddr,NULL));
	}

	FREE_AND_NULL(afline);

	fclose(cfgfile);
	fclose(outfile);
	if(delfile(cfg.cfgfile))
		lprintf(LOG_ERR,"ERROR line %d removing %s %s",__LINE__,cfg.cfgfile
			,strerror(errno));
	if(rename(outname,cfg.cfgfile))
		lprintf(LOG_ERR,"ERROR line %d renaming %s to %s",__LINE__,outname,cfg.cfgfile);
	free(outname);
}

/******************************************************************************
 Used by AREAFIX to process any '%' commands that come in via netmail
******************************************************************************/
void command(char* instr, faddr_t addr, char* to)
{
	FILE *stream,*tmpf;
	char str[MAX_PATH+1],temp[256],*buf,*p;
	int  file,i,node;
	long l;

	node=matchnode(addr,0);
	if(node>=cfg.nodecfgs)
		return;
	strupr(instr);
	if((p=strstr(instr,"HELP"))!=NULL) {
		sprintf(str,"%sAREAMGR.HLP",scfg.exec_dir);
		if(!fexistcase(str))
			return;
		if((stream=fnopen(&file,str,O_RDONLY))==NULL) {
			lprintf(LOG_ERR,"ERROR %u (%s) line %d opening %s",errno,strerror(errno),__LINE__,str);
			return; 
		}
		l=filelength(file);
		if((buf=(char *)malloc(l+1L))==NULL) {
			lprintf(LOG_CRIT,"ERROR line %d allocating %lu bytes for %s",__LINE__,l,str);
			return; 
		}
		fread(buf,l,1,stream);
		fclose(stream);
		buf[l]=0;
		create_netmail(to,"Area Manager Help",buf,addr,FALSE);
		free(buf);
		return; 
	}

	if((p=strstr(instr,"LIST"))!=NULL) {
		netmail_arealist(AREALIST_ALL,addr,to);
		return; 
	}

	if((p=strstr(instr,"QUERY"))!=NULL) {
		netmail_arealist(AREALIST_CONNECTED,addr,to);
		return; 
	}

	if((p=strstr(instr,"UNLINKED"))!=NULL) {
		netmail_arealist(AREALIST_UNLINKED,addr,to);
		return; 
	}

	if((p=strstr(instr,"COMPRESSION"))!=NULL) {
		FIND_WHITESPACE(p);
		SKIP_WHITESPACE(p);
		for(i=0;i<cfg.arcdefs;i++)
			if(!stricmp(p,cfg.arcdef[i].name))
				break;
		if(!stricmp(p,"NONE"))
			i=0xffff;
		if(i==cfg.arcdefs) {
			if((tmpf=tmpfile())==NULL) {
				lprintf(LOG_ERR,"ERROR line %d opening tmpfile()",__LINE__);
				return; 
			}
			fprintf(tmpf,"Compression type unavailable.\r\n\r\n"
				"Available types are:\r\n");
			for(i=0;i<cfg.arcdefs;i++)
				fprintf(tmpf,"                     %s\r\n",cfg.arcdef[i].name);
			file_to_netmail(tmpf,"Compression Type Change",addr,to);
			fclose(tmpf);
			return; 
		}
		if(cfg.nodecfg[node].arctype < cfg.arcdefs)
			buf = cfg.arcdef[cfg.nodecfg[node].arctype].name;
		else
			buf = "NONE";
		alter_config(addr,buf
			,(i>=0 && i<cfg.arcdefs)?cfg.arcdef[i].name:p,0);
		cfg.nodecfg[node].arctype=i;
		sprintf(str,"Compression type changed to %s.",(i>=0 && i<cfg.arcdefs)?cfg.arcdef[i].name:p);
		create_netmail(to,"Compression Type Change",str,addr,FALSE);
		return; 
	}

	if((p=strstr(instr,"PASSWORD"))!=NULL) {
		FIND_WHITESPACE(p);
		SKIP_WHITESPACE(p);
		sprintf(temp,"%-.25s",p);
		p=temp;
		FIND_WHITESPACE(p);
		*p=0;
		if(node>=cfg.nodecfgs)		   /* Should never happen */
			return;
		if(!stricmp(temp,cfg.nodecfg[node].password)) {
			sprintf(str,"Your password was already set to %s."
				,cfg.nodecfg[node].password);
			create_netmail(to,"Password Change Request",str,addr,FALSE);
			return; 
		}
		alter_config(addr,cfg.nodecfg[node].password,temp,1);
		sprintf(str,"Your password has been changed from %s to %.25s."
			,cfg.nodecfg[node].password,temp);
		sprintf(cfg.nodecfg[node].password,"%.25s",temp);
		create_netmail(to,"Password Change Request",str,addr,FALSE);
		return; 
	}

	if((p=strstr(instr,"RESCAN"))!=NULL) {
		export_echomail("",addr);
		create_netmail(to,"Rescan Areas"
			,"All connected areas carried by your hub have been rescanned."
			,addr,FALSE);
		return; 
	}

	if((p=strstr(instr,"ACTIVE"))!=NULL) {
		if(!(cfg.nodecfg[node].attr&ATTR_PASSIVE)) {
			create_netmail(to,"Reconnect Disconnected Areas"
				,"Your areas are already connected.",addr,FALSE);
			return; 
		}
		alter_config(addr,0,0,3);
		create_netmail(to,"Reconnect Disconnected Areas"
			,"Temporarily disconnected areas have been reconnected.",addr,FALSE);
		return; 
	}

	if((p=strstr(instr,"PASSIVE"))!=NULL) {
		if(cfg.nodecfg[node].attr&ATTR_PASSIVE) {
			create_netmail(to,"Temporarily Disconnect Areas"
				,"Your areas are already temporarily disconnected.",addr,FALSE);
			return; 
		}
		alter_config(addr,0,0,2);
		create_netmail(to,"Temporarily Disconnect Areas"
			,"Your areas have been temporarily disconnected.",addr,FALSE);
		return; 
	}

	if((p=strstr(instr,"FROM"))!=NULL);

	if((p=strstr(instr,"+ALL"))!=NULL) {
		str_list_t add_area=strListInit();
		strListPush(&add_area, instr);
		alter_areas(add_area,NULL,addr,to);
		strListFree(&add_area);
		return; 
	}

	if((p=strstr(instr,"-ALL"))!=NULL) {
		str_list_t del_area=strListInit();
		strListPush(&del_area, instr);
		alter_areas(NULL,del_area,addr,to);
		strListFree(&del_area);
		return; 
	}
}
/******************************************************************************
 This is where we're gonna process any netmail that comes in for areafix.
 Returns text for message body for the local sysop if necessary.
******************************************************************************/
char* process_areafix(faddr_t addr, char* inbuf, char* password, char* to)
{
	static char body[512];
	char str[128];
	char *p,*tp,action,percent=0;
	int i;
	ulong l,m;
	str_list_t add_area,del_area;

	lprintf(LOG_INFO,"Areafix Request received from %s"
			,smb_faddrtoa(&addr,NULL));
	
	p=(char *)inbuf;

	while(*p==CTRL_A) {			/* Skip kludge lines 11/05/95 */
		FIND_CHAR(p,'\r');
		if(*p) {
			p++;				/* Skip CR (required) */
			if(*p=='\n')
				p++;			/* Skip LF (optional) */
		}
	}

	if(((tp=strstr(p,"---\r"))!=NULL || (tp=strstr(p,"--- "))!=NULL) &&
		(*(tp-1)=='\r' || *(tp-1)=='\n'))
		*tp=0;

	if(!strnicmp(p,"%FROM",5)) {    /* Remote Remote Maintenance (must be first) */
		SAFECOPY(str,p+6);
		truncstr(str,"\r\n");
		lprintf(LOG_NOTICE,"Remote maintenance for %s requested via %s",str
			,smb_faddrtoa(&addr,NULL));
		addr=atofaddr(str); 
	}

	i=matchnode(addr,0);
	if(i>=cfg.nodecfgs) {
		lprintf(LOG_NOTICE,"Areafix not configured for %s", smb_faddrtoa(&addr,NULL));
		create_netmail(to,"Areafix Request"
			,"Your node is not configured for Areafix, please contact your hub.\r\n",addr,FALSE);
		sprintf(body,"An areafix request was made by node %s.\r\nThis node "
			"is not currently configured for areafix.\r\n"
			,smb_faddrtoa(&addr,NULL));
		lprintf(LOG_DEBUG,"areafix debug, nodes=%u",cfg.nodecfgs);
		{
			int j;
			for(j=0;j<cfg.nodecfgs;j++)
				lprintf(LOG_DEBUG,smb_faddrtoa(&cfg.nodecfg[j].faddr,NULL));
		}
		return(body); 
	}

	if(stricmp(cfg.nodecfg[i].password,password)) {
		create_netmail(to,"Areafix Request","Invalid Password.",addr,FALSE);
		sprintf(body,"Node %s attempted an areafix request using an invalid "
			"password.\r\nThe password attempted was %s.\r\nThe correct password "
			"for this node is %s.\r\n",smb_faddrtoa(&addr,NULL),password
			,(cfg.nodecfg[i].password[0]) ? cfg.nodecfg[i].password
			 : "[None Defined]");
		return(body); 
	}

	m=strlen(p);
	add_area=strListInit();
	del_area=strListInit();
	for(l=0;l<m;l++) { 
		while(*(p+l) && isspace((uchar)*(p+l))) l++;
		while(*(p+l)==CTRL_A) {				/* Ignore kludge lines June-13-2004 */
			while(*(p+l) && *(p+l)!='\r') l++;
			continue;
		}
		if(!(*(p+l))) break;
		if(*(p+l)=='+' || *(p+l)=='-' || *(p+l)=='%') {
			action=*(p+l);
			l++; 
		}
		else
			action='+';
		SAFECOPY(str,p+l);
		truncstr(str,"\r\n");
		truncsp(str);	/* Remove trailing white-space, April-4-2014 */
		switch(action) {
			case '+':                       /* Add Area */
				strListPush(&add_area, str);
				break;
			case '-':                       /* Remove Area */
				strListPush(&del_area, str);
				break;
			case '%':                       /* Process Command */
				command(str,addr,to);
				percent++;
				break; 
		}

		while(*(p+l) && *(p+l)!='\r') l++; 
	}

	if(!percent && !strListCount(add_area) && !strListCount(del_area)) {
		create_netmail(to,"Areafix Request","No commands to process.",addr,FALSE);
		sprintf(body,"Node %s attempted an areafix request with an empty message "
			"body or with no valid commands.\r\n",smb_faddrtoa(&addr,NULL));
		strListFree(&add_area);
		strListFree(&del_area);
		return(body); 
	}
	if(strListCount(add_area) || strListCount(del_area))
		alter_areas(add_area,del_area,addr,to);
	strListFree(&add_area);
	strListFree(&del_area);

	return(NULL);
}
/******************************************************************************
 This function will compare the archive signatures defined in the CFG file and
 extract 'infile' using the appropriate de-archiver.
******************************************************************************/
int unpack(char *infile)
{
	FILE *stream;
	char str[256],tmp[128];
	int i,j,ch,file;

	if((stream=fnopen(&file,infile,O_RDONLY))==NULL) {
		lprintf(LOG_ERR,"ERROR %u (%s) line %d opening %s",errno,strerror(errno),__LINE__,infile);
		bail(1); 
		return -1;
	}
	for(i=0;i<cfg.arcdefs;i++) {
		str[0]=0;
		fseek(stream,cfg.arcdef[i].byteloc,SEEK_SET);
		for(j=0;j<strlen(cfg.arcdef[i].hexid)/2;j++) {
			ch=fgetc(stream);
			if(ch==EOF) {
				i=cfg.arcdefs;
				break; 
			}
			sprintf(tmp,"%02X",ch);
			strcat(str,tmp); 
		}
		if(!stricmp(str,cfg.arcdef[i].hexid))
			break; 
	}
	fclose(stream);

	if(i==cfg.arcdefs) {
		lprintf(LOG_ERR,"ERROR line %d determining filetype of %s",__LINE__,infile);
		return(1); 
	}

	j=execute(mycmdstr(&scfg,cfg.arcdef[i].unpack,infile
		,secure ? cfg.secure : cfg.inbound));
	if(j) {
		lprintf(LOG_ERR,"ERROR %d (%d) line %d executing %s"
			,j,errno,__LINE__,mycmdstr(&scfg,cfg.arcdef[i].unpack,infile
				,secure ? cfg.secure : cfg.inbound));
		return(j); 
	}
	return(0);
}
/******************************************************************************
 This function will check the 'dest' for the type of archiver to use (as
 defined in the CFG file) and compress 'srcfile' into 'destfile' using the
 appropriate archive program.
******************************************************************************/
void pack(char *srcfile,char *destfile,faddr_t dest)
{
	int i,j;
	uint use=0;

	i=matchnode(dest,0);
	if(i<cfg.nodecfgs)
		use=cfg.nodecfg[i].arctype;

	j=execute(mycmdstr(&scfg,cfg.arcdef[use].pack,destfile,srcfile));
	if(j) {
		lprintf(LOG_ERR,"ERROR %d (%d) line %d executing %s"
			,j,errno,__LINE__,mycmdstr(&scfg,cfg.arcdef[use].pack,destfile,srcfile)); 
	}
}

/* Reads a single FTS-1 stored message header from the specified file stream and terminates C-strings */
BOOL fread_fmsghdr(fmsghdr_t* hdr, FILE* fp)
{
	if(fread(hdr, sizeof(fmsghdr_t), 1, fp) != 1)
		return FALSE;
	TERMINATE(hdr->from);
	TERMINATE(hdr->to);
	TERMINATE(hdr->subj);
	TERMINATE(hdr->time);
	return TRUE;
}

enum {
	 ATTACHMENT_ADD
	,ATTACHMENT_NETMAIL
	,ATTACHMENT_CHECK
};

int attachment(char *bundlename,faddr_t dest, int mode)
{
	FILE *fidomsg,*stream;
	char str[1025],*path,fname[129],*p;
	int fmsg,file,error=0L;
	long fncrc,*mfncrc=0L,num_mfncrc=0L,crcidx;
    attach_t attach;
	fmsghdr_t hdr;
	size_t		f;
	glob_t		g;

	if(bundlename==NULL && mode!=ATTACHMENT_NETMAIL) {
		lprintf(LOG_ERR,"ERROR line %d NULL bundlename",__LINE__);
		return(1);
	}
	sprintf(fname,"%sBUNDLES.SBE",cfg.outbound);
	if((stream=fnopen(&file,fname,O_RDWR|O_CREAT))==NULL) {
		lprintf(LOG_ERR,"ERROR %u (%s) line %d opening %s",errno,strerror(errno),__LINE__,fname);
		return(1); 
	}

	if(mode==ATTACHMENT_CHECK) {				/* Check for existance in BUNDLES.SBE */
		while(!feof(stream)) {
			if(!fread(&attach,sizeof(attach_t),1,stream))
				break;
			TERMINATE(attach.fname);
			if(!stricmp(attach.fname,bundlename)) {
				fclose(stream);
				return(1); 
			} 
		}
		fclose(stream);
		if(!flength(fname))
			remove(fname);
		return(0); 
	}

	if(mode==ATTACHMENT_NETMAIL) {				/* Create netmail attaches */

		if(!filelength(file)) {
			fclose(stream);
			return(0); 
		}
										/* Get attach names from existing MSGs */
#ifdef __unix__
		sprintf(str,"%s*.[Mm][Ss][Gg]",scfg.netmail_dir);
#else
		sprintf(str,"%s*.msg",scfg.netmail_dir);
#endif
		glob(str,0,NULL,&g);
		for(f=0;f<g.gl_pathc;f++) {

			path=g.gl_pathv[f];

			if((fidomsg=fnopen(&fmsg,path,O_RDWR))==NULL) {
				lprintf(LOG_ERR,"ERROR %u (%s) line %d opening %s",errno,strerror(errno),__LINE__,path);
				continue; 
			}
			if(filelength(fmsg)<sizeof(fmsghdr_t)) {
				lprintf(LOG_ERR,"ERROR line %d %s has invalid length of %lu bytes"
					,__LINE__
					,path
					,filelength(fmsg));
				fclose(fidomsg);
				continue; 
			}
			if(!fread_fmsghdr(&hdr,fidomsg)) {
				fclose(fidomsg);
				lprintf(LOG_ERR,"ERROR line %d reading fido msghdr from %s",__LINE__,path);
				continue; 
			}
			fclose(fidomsg);
			if(!(hdr.attr&FIDO_FILE))		/* Not a file attach */
				continue;
			num_mfncrc++;
			p=getfname(hdr.subj);
			if((mfncrc=(long *)realloc(mfncrc,num_mfncrc*sizeof(long)))==NULL) {
				lprintf(LOG_ERR,"ERROR line %d allocating %lu for bundle name crc"
					,__LINE__,num_mfncrc*sizeof(long));
				continue; 
			}
			mfncrc[num_mfncrc-1]=crc32(strupr(p),0); 
		}
		globfree(&g);

		while(!feof(stream)) {
			if(!fread(&attach,sizeof(attach_t),1,stream))
				break;
			TERMINATE(attach.fname);
			SAFEPRINTF2(str,"%s%s",cfg.outbound,attach.fname);
			if(!fexistcase(str))
				continue;
			fncrc=crc32(strupr(attach.fname),0);
			for(crcidx=0;crcidx<num_mfncrc;crcidx++)
				if(mfncrc[crcidx]==fncrc)
					break;
			if(crcidx==num_mfncrc)
				if(create_netmail(/* To: */NULL,str
					,(misc&TRUNC_BUNDLES) ? "\1FLAGS TFS\r" : "\1FLAGS KFS\r"
					,attach.dest,TRUE))
					error=1; 
		}
		fclose(stream);
		if(!error)			/* remove bundles.sbe if no error occurred */		
			delfile(fname);	/* used to truncate here, August-20-2002 */
		if(num_mfncrc)
			free(mfncrc);
		return(0); 
	}

	while(!feof(stream)) {
		if(!fread(&attach,sizeof(attach_t),1,stream))
			break;
		TERMINATE(attach.fname);
		if(!stricmp(attach.fname,bundlename)) {
			fclose(stream);
			return(0); 
		} 
	}

	memcpy(&attach.dest,&dest,sizeof(faddr_t));
	SAFECOPY(attach.fname,bundlename);
	/* TODO: Write of unpacked struct */
	fwrite(&attach,sizeof(attach_t),1,stream);
	fclose(stream);
	return(0);
}

/******************************************************************************
 This function is called when a message packet has reached it's maximum size.
 It places packets into a bundle until that bundle is full, at which time the
 last character of the extension increments (1 thru 0 and then A thru Z).  If
 all bundles have reached their maximum size remaining packets are placed into
 the Z bundle.
******************************************************************************/
void pack_bundle(char *infile,faddr_t dest)
{
	char str[256],fname[256],outbound[128],day[3],*p;
	int i,j,file,node;
	time_t now;

	if(infile==NULL || infile[0]==0) {
		lprintf(LOG_ERR,"ERROR line %d invalid filename",__LINE__);
		bail(1);
		return;
	}

	node=matchnode(dest,0);
	strcpy(str,infile);
	str[strlen(str)-1]='t';
	if(rename(infile,str))				   /* Change .PK_ file to .PKT file */
		lprintf(LOG_ERR,"ERROR line %d renaming %s to %s",__LINE__,infile,str);
	infile[strlen(infile)-1]='t';
	time(&now);
	sprintf(day,"%-.2s",ctime(&now));
	strupr(day);
	if(misc&FLO_MAILER) {
		if(node<cfg.nodecfgs && !(cfg.nodecfg[node].attr&ATTR_DIRECT) && cfg.nodecfg[node].route.zone) {
			dest=cfg.nodecfg[node].route;
			if(cfg.log&LOG_ROUTING)
				lprintf(LOG_NOTICE,"Routing %s to %s",infile,smb_faddrtoa(&dest,NULL));
		}

		if(dest.zone==sys_faddr.zone)	/* Default zone, use default outbound */
			SAFECOPY(outbound,cfg.outbound);
		else {							/* Inter-zone outbound is OUTBOUND.XXX */
			SAFEPRINTF3(outbound,"%.*s.%03x"
				,(int)strlen(cfg.outbound)-1,cfg.outbound,dest.zone);
			MKDIR(outbound);
			backslash(outbound);
		}
		if(dest.point) {				/* Point destination is OUTBOUND\*.PNT */
			sprintf(str,"%04x%04x.pnt"
				,dest.net,dest.node);
			strcat(outbound,str); 
		}
		}
	else
		strcpy(outbound,cfg.outbound);
	if(outbound[strlen(outbound)-1]=='\\'
		|| outbound[strlen(outbound)-1]=='/')
		outbound[strlen(outbound)-1]=0;
	MKDIR(outbound);
	backslash(outbound);

	if(node<cfg.nodecfgs)
		if(cfg.nodecfg[node].arctype==0xffff) {    /* Uncompressed! */
			if(misc&FLO_MAILER)
				i=write_flofile(infile,dest,TRUE /* bundle */);
			else
				i=create_netmail(/* To: */NULL,infile
					,(misc&TRUNC_BUNDLES) ? "\1FLAGS TFS\r" : "\1FLAGS KFS\r"
					,dest,TRUE);
			if(i) bail(1);
			return; 
		}

	if(dest.point)
		sprintf(fname,"%s0000p%03hx.%s",outbound,(short)dest.point,day);
	else
		sprintf(fname,"%s%04hx%04hx.%s",outbound,(short)(sys_faddr.net-dest.net)
			,(short)(sys_faddr.node-dest.node),day);
	for(i='0';i<='Z';i++) {
		if(i==':')
			i='A';
		sprintf(str,"%s%c",fname,i);
		if(flength(str)==0) {
			/* Feb-10-2003: Don't overwrite or delete 0-byte file less than 24hrs old */
			if((time(NULL)-fdate(str))<24L*60L*60L)
				continue;	
			if(delfile(str))
				lprintf(LOG_ERR,"ERROR line %d removing %s %s",__LINE__,str
					,strerror(errno));
		}
		if(fexistcase(str)) {
			if(i!='Z' && flength(str)>=cfg.maxbdlsize)
				continue;
			file=sopen(str,O_WRONLY,SH_DENYRW);
			if(file==-1)		/* Can't open?!? Probably being sent */
				continue;
			close(file);
			p=getfname(str);
			if(!attachment(p,dest,ATTACHMENT_CHECK))
				attachment(p,dest,ATTACHMENT_ADD);
			pack(infile,str,dest);
			if(delfile(infile))
				lprintf(LOG_ERR,"ERROR line %d removing %s %s",__LINE__,infile
					,strerror(errno));
			return; 
		}
		else {
			if(misc&FLO_MAILER)
				j=write_flofile(str,dest,TRUE /* bundle */);
			else {
				p=getfname(str);
				j=attachment(p,dest,ATTACHMENT_ADD); 
			}
			if(j)
				bail(1);
			pack(infile,str,dest);
			if(delfile(infile))
				lprintf(LOG_ERR,"ERROR line %d removing %s %s",__LINE__,infile
					,strerror(errno));
			return; 
		} 
	}
	lprintf(LOG_WARNING,"All bundle files for %s already exist, adding to: %s"
		,smb_faddrtoa(&dest,NULL), str);
	pack(infile,str,dest);	/* Won't get here unless all bundles are full */
}

/******************************************************************************
 This function checks the inbound directory for the first bundle it finds, it
 will then unpack and delete the bundle.  If no bundles exist this function
 returns a FALSE, otherwise a TRUE is returned.
 ******************************************************************************/
BOOL unpack_bundle(void)
{
	char*				p;
	char				str[MAX_PATH+1];
	char				fname[MAX_PATH+1];
	int				i;
	static glob_t	g;
	static int		gi;

	for(i=0;i<7;i++) {
#if defined(__unix__)	/* support upper or lower case */
		switch(i) {
			case 0:
				p="[Ss][Uu]";
				break;
			case 1:
				p="[Mm][Oo]";
				break;
			case 2:
				p="[Tt][Uu]";
				break;
			case 3:
				p="[Ww][Ee]";
				break;
			case 4:
				p="[Tt][Hh]";
				break;
			case 5:
				p="[Ff][Rr]";
				break;
			default:
				p="[Ss][Aa]";
				break;
		}
#else
		switch(i) {
			case 0:
				p="su";
				break;
			case 1:
				p="mo";
				break;
			case 2:
				p="tu";
				break;
			case 3:
				p="we";
				break;
			case 4:
				p="th";
				break;
			case 5:
				p="fr";
				break;
			default:
				p="sa";
				break;
		}
#endif
		sprintf(str,"%s*.%s?",secure ? cfg.secure : cfg.inbound,p);
		if(gi>=g.gl_pathc) {
			gi=0;
			globfree(&g);
			glob(str,0,NULL,&g);
		}
		if(gi<g.gl_pathc) {
			SAFECOPY(fname,g.gl_pathv[gi]);
			gi++;
			lprintf(LOG_DEBUG,"Unpacking bundle: %s",fname);
			if(unpack(fname)) {	/* failure */
				lprintf(LOG_ERR,"!Unpack failure");
				if(fdate(fname)+(48L*60L*60L)<time(NULL)) {	
					/* If bundle file older than 48 hours, give up and rename
					   to "*.?_?" or (if it exists) "*.?-?" */
					SAFECOPY(str,fname);
					str[strlen(str)-2]='_';
					if(fexistcase(str))
						str[strlen(str)-2]='-';
					if(fexistcase(str))
						delfile(str);
					if(rename(fname,str))
						lprintf(LOG_ERR,"ERROR line %d renaming %s to %s"
							,__LINE__,fname,str); 
				}
				continue;
			}
			lprintf(LOG_DEBUG,"Deleting bundle: %s", fname);
			if(delfile(fname))	/* successful, so delete bundle */
				lprintf(LOG_ERR,"ERROR line %d removing %s %s",__LINE__,fname
					,strerror(errno));
			return(TRUE); 
		} 
	}

	return(FALSE);
}

/****************************************************************************/
/* Moves or copies a file from one dir to another                           */
/* both 'src' and 'dest' must contain full path and filename                */
/* returns 0 if successful, -1 if error                                     */
/****************************************************************************/
int mv(char *src, char *dest, BOOL copy)
{
	char buf[4096],str[256];
	int  ind,outd;
	long length,chunk=4096,l;
    FILE *inp,*outp;

	if(!strcmp(src,dest))	/* source and destination are the same! */
		return(0);
	if(!fexistcase(src)) {
		lprintf(LOG_WARNING,"MV ERROR: Source doesn't exist '%s",src);
		return(-1); 
	}
	if(!copy && fexistcase(dest)) {
		lprintf(LOG_WARNING,"MV ERROR: Destination already exists '%s'",dest);
		return(-1); 
	}
	if(!copy
#ifndef __unix__
		&& ((src[1]!=':' && dest[1]!=':')
		|| (src[1]==':' && dest[1]==':' && toupper(src[0])==toupper(dest[0])))
#endif
		) {
		if(rename(src,dest)==0)		/* same drive, so move */
			return(0); 
		/* rename failed, so attempt copy */
	}
	if((ind=nopen(src,O_RDONLY))==-1) {
		lprintf(LOG_ERR,"MV ERROR %u (%s) opening %s",errno,strerror(errno),src);
		return(-1); 
	}
	if((inp=fdopen(ind,"rb"))==NULL) {
		close(ind);
		lprintf(LOG_ERR,"MV ERROR %u (%s) fdopening %s",errno,strerror(errno),str);
		return(-1); 
	}
	setvbuf(inp,NULL,_IOFBF,8*1024);
	if((outd=nopen(dest,O_WRONLY|O_CREAT|O_TRUNC))==-1) {
		fclose(inp);
		lprintf(LOG_ERR,"MV ERROR %u (%s) opening %s",errno,strerror(errno),dest);
		return(-1); 
	}
	if((outp=fdopen(outd,"wb"))==NULL) {
		close(outd);
		fclose(inp);
		lprintf(LOG_ERR,"MV ERROR %u (%s) fdopening %s",errno,strerror(errno),str);
		return(-1); 
	}
	setvbuf(outp,NULL,_IOFBF,8*1024);
	length=filelength(ind);
	l=0L;
	while(l<length) {
		if(l+chunk>length)
			chunk=length-l;
		fread(buf,chunk,1,inp);
		fwrite(buf,chunk,1,outp);
		l+=chunk; 
	}
	fclose(inp);
	fclose(outp);
	if(!copy && delfile(src)) {
		lprintf(LOG_ERR,"ERROR line %d removing %s %s",__LINE__,src,strerror(errno));
		return(-1); 
	}
	return(0);
}

/****************************************************************************/
/* Returns negative value on error											*/
/****************************************************************************/
long getlastmsg(uint subnum, uint32_t *ptr, /* unused: */time_t *t)
{
	int i;
	smb_t smbfile;

	if(ptr) (*ptr)=0;
	ZERO_VAR(smbfile);
	if(subnum>=scfg.total_subs) {
		lprintf(LOG_ERR,"ERROR line %d getlastmsg %d",__LINE__,subnum);
		bail(1); 
		return -1;
	}
	sprintf(smbfile.file,"%s%s",scfg.sub[subnum]->data_dir,scfg.sub[subnum]->code);
	smbfile.retry_time=scfg.smb_retry_time;
	if((i=smb_open(&smbfile))!=SMB_SUCCESS) {
		lprintf(LOG_ERR,"ERROR %d (%s) line %d opening %s",i,smbfile.last_error,__LINE__,smbfile.file);
		return -1;
	}

	if(!filelength(fileno(smbfile.shd_fp))) {			/* Empty base */
		if(ptr) (*ptr)=0;
		smb_close(&smbfile);
		return(0); 
	}
	smb_close(&smbfile);
	if(ptr) (*ptr)=smbfile.status.last_msg;
	return(smbfile.status.total_msgs);
}


ulong loadmsgs(post_t** post, ulong ptr)
{
	int i;
	long l,total;
	idxrec_t idx;


	if((i=smb_locksmbhdr(&smb[cur_smb]))!=SMB_SUCCESS) {
		lprintf(LOG_ERR,"ERROR %d (%s) line %d locking %s",i,smb[cur_smb].last_error,__LINE__,smb[cur_smb].file);
		return(0); 
	}

	/* total msgs in sub */
	total=filelength(fileno(smb[cur_smb].sid_fp))/sizeof(idxrec_t);

	if(!total) {			/* empty */
		smb_unlocksmbhdr(&smb[cur_smb]);
		return(0); 
	}

	if(((*post)=(post_t*)malloc(sizeof(post_t)*total))    /* alloc for max */
		==NULL) {
		smb_unlocksmbhdr(&smb[cur_smb]);
		lprintf(LOG_ERR,"ERROR line %d allocating %lu bytes for %s",__LINE__
			,sizeof(post_t *)*total,smb[cur_smb].file);
		return(0); 
	}

	fseek(smb[cur_smb].sid_fp,0L,SEEK_SET);
	for(l=0;l<total && !feof(smb[cur_smb].sid_fp); ) {
		if(smb_fread(&smb[cur_smb], &idx,sizeof(idx),smb[cur_smb].sid_fp) != sizeof(idx))
			break;

		if(idx.number==0)	/* invalid message number, ignore */
			continue;

		if(idx.number<=ptr || (idx.attr&MSG_DELETE))
			continue;

		if((idx.attr&MSG_MODERATED) && !(idx.attr&MSG_VALIDATED))
			break;

		(*post)[l++].idx=idx;
	}
	smb_unlocksmbhdr(&smb[cur_smb]);
	if(!l)
		FREE_AND_NULL(*post);
	return(l);
}

void bail(int code)
{
	if((code && pause_on_abend) || pause_on_exit) {
		fprintf(stderr,"\nHit any key...");
		getch();
		fprintf(stderr,"\n");
	}
	exit(code);
}

typedef struct {
	ulong	alias,
			real;
			} username_t;

/****************************************************************************/
/* Note: Wrote another version of this function that read all userdata into */
/****************************************************************************/
/* Looks for a perfect match amoung all usernames (not deleted users)		*/
/* Returns the number of the perfect matched username or 0 if no match		*/
/* Called from functions waitforcall and newuser							*/
/* memory then scanned it from memory... took longer - always.              */
/****************************************************************************/
ulong matchname(char *inname)
{
	static ulong total_users;
	static username_t *username;
	ulong last_user;
	int userdat,i;
	char str[256],name[LEN_NAME+1],alias[LEN_ALIAS+1];
	ulong l,crc;

	if(!total_users) {		/* Load CRCs */
		fprintf(stderr,"\n%-25s","Loading user names...");
		sprintf(str,"%suser/user.dat",scfg.data_dir);
		if((userdat=nopen(str,O_RDONLY|O_DENYNONE))==-1)
			return(0);
		last_user=filelength(userdat)/U_LEN;
		for(total_users=0;total_users<last_user;total_users++) {
			printf("%5ld\b\b\b\b\b",total_users);
			if((username=(username_t *)realloc(username
				,(total_users+1L)*sizeof(username_t)))==NULL)
				break;
			username[total_users].alias=0;
			username[total_users].real=0;
			i=0;
			while(i<LOOP_NODEDAB
				&& lock(userdat,(long)((long)(total_users)*U_LEN)+U_ALIAS
					,LEN_ALIAS+LEN_NAME)==-1)
				i++;
			if(i>=LOOP_NODEDAB) {	   /* Couldn't lock USER.DAT record */
				lprintf(LOG_ERR,"ERROR locking USER.DAT record #%ld",total_users);
				continue; 
			}
			lseek(userdat,(long)((long)(total_users)*U_LEN)+U_ALIAS,SEEK_SET);
			read(userdat,alias,LEN_ALIAS);
			read(userdat,name,LEN_NAME);
			lseek(userdat,(long)(((long)total_users)*U_LEN)+U_MISC,SEEK_SET);
			read(userdat,tmp,8);
			for(i=0;i<8;i++)
				if(tmp[i]==ETX || tmp[i]=='\r') break;
			tmp[i]=0;
			unlock(userdat,(long)((long)(total_users)*U_LEN)+U_ALIAS
				,LEN_ALIAS+LEN_NAME);
			if(ahtoul(tmp)&DELETED)
				continue;
			for(i=0;i<LEN_ALIAS;i++)
				if(alias[i]==ETX || alias[i]=='\r') break;
			alias[i]=0;
			strupr(alias);
			for(i=0;i<LEN_NAME;i++)
				if(name[i]==ETX || name[i]=='\r') break;
			name[i]=0;
			strupr(name);
			username[total_users].alias=crc32(alias,0);
			username[total_users].real=crc32(name,0); 
		}
		close(userdat);
		fprintf(stderr,"     \b\b\b\b\b");  /* Clear counter */
		fprintf(stderr,
			"\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b"
			"%25s"
			"\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b"
			,""); 
	}

	SAFECOPY(str,inname);
	strupr(str);
	crc=crc32(str,0);
	for(l=0;l<total_users;l++)
		if((crc==username[l].alias || crc==username[l].real))
			return(l+1);
	return(0);
}

/****************************************************************************/
/* Converts goofy FidoNet time format into Unix format						*/
/****************************************************************************/
time_t fmsgtime(char *str)
{
	char month[4];
	struct tm tm;

	memset(&tm,0,sizeof(tm));
	tm.tm_isdst=-1;	/* Do not adjust for DST */

	if(isdigit((uchar)str[1])) {	/* Regular format: "01 Jan 86  02:34:56" */
		tm.tm_mday=atoi(str);
		sprintf(month,"%3.3s",str+3);
		if(!stricmp(month,"jan"))
			tm.tm_mon=0;
		else if(!stricmp(month,"feb"))
			tm.tm_mon=1;
		else if(!stricmp(month,"mar"))
			tm.tm_mon=2;
		else if(!stricmp(month,"apr"))
			tm.tm_mon=3;
		else if(!stricmp(month,"may"))
			tm.tm_mon=4;
		else if(!stricmp(month,"jun"))
			tm.tm_mon=5;
		else if(!stricmp(month,"jul"))
			tm.tm_mon=6;
		else if(!stricmp(month,"aug"))
			tm.tm_mon=7;
		else if(!stricmp(month,"sep"))
			tm.tm_mon=8;
		else if(!stricmp(month,"oct"))
			tm.tm_mon=9;
		else if(!stricmp(month,"nov"))
			tm.tm_mon=10;
		else
			tm.tm_mon=11;
		tm.tm_year=atoi(str+7);
		if(tm.tm_year<Y2K_2DIGIT_WINDOW)
			tm.tm_year+=100;
		tm.tm_hour=atoi(str+11);
		tm.tm_min=atoi(str+14);
		tm.tm_sec=atoi(str+17); 
	}

	else {					/* SEAdog  format: "Mon  1 Jan 86 02:34" */
		tm.tm_mday=atoi(str+4);
		sprintf(month,"%3.3s",str+7);
		if(!stricmp(month,"jan"))
			tm.tm_mon=0;
		else if(!stricmp(month,"feb"))
			tm.tm_mon=1;
		else if(!stricmp(month,"mar"))
			tm.tm_mon=2;
		else if(!stricmp(month,"apr"))
			tm.tm_mon=3;
		else if(!stricmp(month,"may"))
			tm.tm_mon=4;
		else if(!stricmp(month,"jun"))
			tm.tm_mon=5;
		else if(!stricmp(month,"jul"))
			tm.tm_mon=6;
		else if(!stricmp(month,"aug"))
			tm.tm_mon=7;
		else if(!stricmp(month,"sep"))
			tm.tm_mon=8;
		else if(!stricmp(month,"oct"))
			tm.tm_mon=9;
		else if(!stricmp(month,"nov"))
			tm.tm_mon=10;
		else
			tm.tm_mon=11;
		tm.tm_year=atoi(str+11);
		if(tm.tm_year<Y2K_2DIGIT_WINDOW)
			tm.tm_year+=100;
		tm.tm_hour=atoi(str+14);
		tm.tm_min=atoi(str+17);
		tm.tm_sec=0; 
	}
	return(mktime(&tm));
}

static short fmsgzone(char* p)
{
	char	hr[4]="";
	char	min[4]="";
	short	val;
	BOOL	west=TRUE;

	if(*p=='-')
		p++;
	else
		west=FALSE;

	if(strlen((char*)p)>=2)
		sprintf(hr,"%.2s",p);
	if(strlen((char*)p+2)>=2)
		sprintf(min,"%.2s",p+2);

	val=atoi(hr)*60;
	val+=atoi(min);

	if(west)
		switch(val|US_ZONE) {
			case AST:
			case EST:
			case CST:
			case MST:
			case PST:
			case YST:
			case HST:
			case BST:
				/* standard US timezone */
				return(val|US_ZONE);
			default: 
				/* non-standard timezone */
				return(-val);
		}
	return(val);
}

char* getfmsg(FILE *stream, ulong *outlen)
{
	char* fbuf;
	int ch;
	ulong l,length,start;

	length=0L;
	start=ftell(stream);						/* Beginning of Message */
	while(1) {
		ch=fgetc(stream);						/* Look for Terminating NULL */
		if(ch==0 || ch==EOF)					/* Found end of message */
			break;
		length++;	 							/* Increment the Length */
	}

	if((fbuf=(char *)malloc(length+1))==NULL) {
		lprintf(LOG_ERR,"ERROR line %d allocating %lu bytes of memory",__LINE__,length+1);
		bail(1); 
		return(NULL);
	}

	fseek(stream,start,SEEK_SET);
	for(l=0;l<length;l++)
		fbuf[l]=fgetc(stream);
	if(ch==0)
		fgetc(stream);		/* Read NULL */

	while(length && fbuf[length-1]<=' ')	/* truncate white-space */
		length--;
	fbuf[length]=0;

	if(outlen)
		*outlen=length;
	return((char*)fbuf);
}

#define MAX_TAILLEN 1024

/****************************************************************************/
/* Coverts a FidoNet message into a Synchronet message						*/
/* Returns 0 on success, 1 dupe, 2 filtered, 3 empty, or other SMB error	*/
/****************************************************************************/
int fmsgtosmsg(char* fbuf, fmsghdr_t fmsghdr, uint user, uint subnum)
{
	uchar	ch,stail[MAX_TAILLEN+1],*sbody;
	char	msg_id[256],str[128],*p;
	BOOL	done,esc,cr;
	int 	i,storage=SMB_SELFPACK;
	uint	col;
	ushort	xlat=XLAT_NONE,net;
	ulong	l,m,length,bodylen,taillen;
	ulong	save;
	long	dupechk_hashes=SMB_HASH_SOURCE_DUPE;
	faddr_t faddr,origaddr,destaddr;
	smb_t*	smbfile;
	char	fname[MAX_PATH+1];
	smbmsg_t	msg;

	if(twit_list) {
		sprintf(fname,"%stwitlist.cfg",scfg.ctrl_dir);
		if(findstr(fmsghdr.from,fname) || findstr(fmsghdr.to,fname)) {
			lprintf(LOG_INFO,"Filtering message from %s to %s",fmsghdr.from,fmsghdr.to);
			return(2);
		}
	}

	memset(&msg,0,sizeof(smbmsg_t));
	if(fmsghdr.attr&FIDO_PRIVATE)
		msg.idx.attr|=MSG_PRIVATE;
	msg.hdr.attr=msg.idx.attr;

	if(fmsghdr.attr&FIDO_FILE)
		msg.hdr.auxattr|=MSG_FILEATTACH;

	msg.hdr.when_imported.time=time(NULL);
	msg.hdr.when_imported.zone=sys_timezone(&scfg);
	msg.hdr.when_written.time=fmsgtime(fmsghdr.time);

	origaddr.zone=fmsghdr.origzone; 	/* only valid if NetMail */
	origaddr.net=fmsghdr.orignet;
	origaddr.node=fmsghdr.orignode;
	origaddr.point=fmsghdr.origpoint;

	destaddr.zone=fmsghdr.destzone; 	/* only valid if NetMail */
	destaddr.net=fmsghdr.destnet;
	destaddr.node=fmsghdr.destnode;
	destaddr.point=fmsghdr.destpoint;

	smb_hfield_str(&msg,SENDER,fmsghdr.from);
	smb_hfield_str(&msg,RECIPIENT,fmsghdr.to);

	if(user) {
		sprintf(str,"%u",user);
		smb_hfield_str(&msg,RECIPIENTEXT,str);
	}

	smb_hfield_str(&msg,SUBJECT,fmsghdr.subj);

	if(fbuf==NULL) {
		lprintf(LOG_ERR,"ERROR line %d allocating fbuf",__LINE__);
		smb_freemsgmem(&msg);
		return(-1); 
	}
	length=strlen((char *)fbuf);
	if((sbody=(uchar*)malloc((length+1)*2))==NULL) {
		lprintf(LOG_ERR,"ERROR line %d allocating %lu bytes for body",__LINE__
			,(length+1)*2L);
		smb_freemsgmem(&msg);
		return(-1); 
	}

	for(col=l=esc=done=bodylen=taillen=0,cr=1;l<length;l++) {

		if(!l && !strncmp((char *)fbuf,"AREA:",5)) {
			save=l;
			l+=5;
			while(l<length && fbuf[l]<=' ' && fbuf[l]>=0) l++;
			m=l;
			while(m<length && fbuf[m]!='\r') m++;
			while(m && fbuf[m-1]<=' ' && fbuf[m-1]>=0) m--;
			if(m>l)
				smb_hfield(&msg,FIDOAREA,(ushort)(m-l),fbuf+l);
			while(l<length && fbuf[l]!='\r') l++;
			/* If unknown echo, keep AREA: line in message body */
			if(cfg.badecho>=0 && subnum==cfg.area[cfg.badecho].sub)
				l=save;
			else
				continue; 
		}

		ch=fbuf[l];
		if(ch==CTRL_A && cr) {	/* kludge line */

			if(!strncmp((char *)fbuf+l+1,"TOPT ",5))
				destaddr.point=atoi((char *)fbuf+l+6);

			else if(!strncmp((char *)fbuf+l+1,"FMPT ",5))
				origaddr.point=atoi((char *)fbuf+l+6);

			else if(!strncmp((char *)fbuf+l+1,"INTL ",5)) {
				faddr=atofaddr((char *)fbuf+l+6);
				destaddr.zone=faddr.zone;
				destaddr.net=faddr.net;
				destaddr.node=faddr.node;
				l+=6;
				while(l<length && fbuf[l]!=' ') l++;
				faddr=atofaddr((char *)fbuf+l+1);
				origaddr.zone=faddr.zone;
				origaddr.net=faddr.net;
				origaddr.node=faddr.node; 
			}

			else if(!strncmp((char *)fbuf+l+1,"MSGID:",6)) {
				l+=7;
				while(l<length && fbuf[l]<=' ' && fbuf[l]>=0) l++;
				m=l;
				while(m<length && fbuf[m]!='\r') m++;
				while(m && fbuf[m-1]<=' ' && fbuf[m-1]>=0) m--;
				if(m>l)
					smb_hfield(&msg,FIDOMSGID,(ushort)(m-l),fbuf+l); 
			}

			else if(!strncmp((char *)fbuf+l+1,"REPLY:",6)) {
				l+=7;
				while(l<length && fbuf[l]<=' ' && fbuf[l]>=0) l++;
				m=l;
				while(m<length && fbuf[m]!='\r') m++;
				while(m && fbuf[m-1]<=' ' && fbuf[m-1]>=0) m--;
				if(m>l)
					smb_hfield(&msg,FIDOREPLYID,(ushort)(m-l),fbuf+l); 
			}

			else if(!strncmp((char *)fbuf+l+1,"FLAGS ",6)		/* correct */
				||  !strncmp((char *)fbuf+l+1,"FLAGS:",6)) {	/* incorrect */
				l+=7;
				while(l<length && fbuf[l]<=' ' && fbuf[l]>=0) l++;
				m=l;
				while(m<length && fbuf[m]!='\r') m++;
				while(m && fbuf[m-1]<=' ' && fbuf[m-1]>=0) m--;
				if(m>l)
					smb_hfield(&msg,FIDOFLAGS,(ushort)(m-l),fbuf+l); 
			}

			else if(!strncmp((char *)fbuf+l+1,"PATH:",5)) {
				l+=6;
				while(l<length && fbuf[l]<=' ' && fbuf[l]>=0) l++;
				m=l;
				while(m<length && fbuf[m]!='\r') m++;
				while(m && fbuf[m-1]<=' ' && fbuf[m-1]>=0) m--;
				if(m>l && (misc&STORE_PATH))
					smb_hfield(&msg,FIDOPATH,(ushort)(m-l),fbuf+l); 
			}

			else if(!strncmp((char *)fbuf+l+1,"PID:",4)) {
				l+=5;
				while(l<length && fbuf[l]<=' ' && fbuf[l]>=0) l++;
				m=l;
				while(m<length && fbuf[m]!='\r') m++;
				while(m && fbuf[m-1]<=' ' && fbuf[m-1]>=0) m--;
				if(m>l)
					smb_hfield(&msg,FIDOPID,(ushort)(m-l),fbuf+l); 
			}

			else if(!strncmp((char *)fbuf+l+1,"TID:",4)) {
				l+=5;
				while(l<length && fbuf[l]<=' ' && fbuf[l]>=0) l++;
				m=l;
				while(m<length && fbuf[m]!='\r') m++;
				while(m && fbuf[m-1]<=' ' && fbuf[m-1]>=0) m--;
				if(m>l)
					smb_hfield(&msg,FIDOTID,(ushort)(m-l),fbuf+l); 
			}

			else if(!strncmp((char *)fbuf+l+1,"TZUTC:",6)) {		/* FSP-1001 */
				l+=7;
				while(l<length && fbuf[l]<=' ' && fbuf[l]>=0) l++;
				msg.hdr.when_written.zone = fmsgzone(fbuf+l);
			}

			else if(!strncmp((char *)fbuf+l+1,"TZUTCINFO:",10)) {	/* non-standard */
				l+=11;
				while(l<length && fbuf[l]<=' ' && fbuf[l]>=0) l++;
				msg.hdr.when_written.zone = fmsgzone(fbuf+l);
			}

			else {		/* Unknown kludge line */
				while(l<length && fbuf[l]<=' ' && fbuf[l]>=0) l++;
				m=l;
				while(m<length && fbuf[m]!='\r') m++;
				while(m && fbuf[m-1]<=' ' && fbuf[m-1]>=0) m--;
				if(m>l && (misc&STORE_KLUDGE))
					smb_hfield(&msg,FIDOCTRL,(ushort)(m-l),fbuf+l); 
			}

			while(l<length && fbuf[l]!='\r') l++;
			continue; 
		}

		if(ch!='\n' && ch!=0x8d) {	/* ignore LF and soft CRs */
			if(cr && (!strncmp((char *)fbuf+l,"--- ",4)
				|| !strncmp((char *)fbuf+l,"---\r",4)))
				done=1; 			/* tear line and down go into tail */
			if(done && cr && !strncmp((char *)fbuf+l,"SEEN-BY:",8)) {
				l+=8;
				while(l<length && fbuf[l]<=' ' && fbuf[l]>=0) l++;
				m=l;
				while(m<length && fbuf[m]!='\r') m++;
				while(m && fbuf[m-1]<=' ' && fbuf[m-1]>=0) m--;
				if(m>l && (misc&STORE_SEENBY))
					smb_hfield(&msg,FIDOSEENBY,(ushort)(m-l),fbuf+l);
				while(l<length && fbuf[l]!='\r') l++;
				continue; 
			}
			if(done) {
				if(taillen<MAX_TAILLEN)
					stail[taillen++]=ch; 
			}
			else
				sbody[bodylen++]=ch;
			col++;
			if(ch=='\r') {
				cr=1;
				col=0;
				if(done) {
					if(taillen<MAX_TAILLEN)
						stail[taillen++]='\n'; 
				}
				else
					sbody[bodylen++]='\n'; 
			}
			else {
				cr=0;
				if(col==1 && !strncmp((char *)fbuf+l," * Origin: ",11)) {
					p=(char*)fbuf+l+11;
					while(*p && *p!='\r') p++;	 /* Find CR */
					while(p && *p!='(') p--;     /* rewind to '(' */
					if(p)
						origaddr=atofaddr(p+1); 	/* get orig address */
					done=1; 
				}
				if(done)
					continue;

				if(ch==ESC) esc=1;		/* ANSI codes */
				if(ch==' ' && col>40 && !esc) {	/* word wrap */
					for(m=l+1;m<length;m++) 	/* find next space */
						if(fbuf[m]<=' ' && fbuf[m]>=0)
							break;
					if(m<length && m-l>80-col) {  /* if it's beyond the eol */
						sbody[bodylen++]='\r';
						sbody[bodylen++]='\n';
						col=0; 
					} 
				}
			} 
		} 
	}

	if(bodylen>=2 && sbody[bodylen-2]=='\r' && sbody[bodylen-1]=='\n')
		bodylen-=2; 						/* remove last CRLF if present */
	sbody[bodylen]=0;

	while(taillen && stail[taillen-1]<=' ')	/* trim all garbage off the tail */
		taillen--;
	stail[taillen]=0;

	if(subnum==INVALID_SUB && !bodylen && !taillen && (misc&KILL_EMPTY_MAIL)) {
		lprintf(LOG_INFO,"Empty NetMail - Ignored ");
		smb_freemsgmem(&msg);
		free(sbody);
		return(3);
	}

	if(!origaddr.zone && subnum==INVALID_SUB)
		net=NET_NONE;						/* Message from SBBSecho */
	else
		net=NET_FIDO;						/* Record origin address */

	if(net) {
		if(origaddr.zone==0)
			origaddr.zone = sys_faddr.zone;
		smb_hfield(&msg,SENDERNETTYPE,sizeof(ushort),&net);
		smb_hfield(&msg,SENDERNETADDR,sizeof(fidoaddr_t),&origaddr); 
	}

	if(subnum==INVALID_SUB) {
		smbfile=email;
		if(net) {
			smb_hfield(&msg,RECIPIENTNETTYPE,sizeof(ushort),&net);
			smb_hfield(&msg,RECIPIENTNETADDR,sizeof(fidoaddr_t),&destaddr); 
		}
		smbfile->status.attr = SMB_EMAIL;
		smbfile->status.max_age = scfg.mail_maxage;
		smbfile->status.max_crcs = scfg.mail_maxcrcs;
		if(scfg.sys_misc&SM_FASTMAIL)
			storage= SMB_FASTALLOC;
	} else {
		smbfile=&smb[cur_smb];
		smbfile->status.max_age	 = scfg.sub[subnum]->maxage;
		smbfile->status.max_crcs = scfg.sub[subnum]->maxcrcs;
		smbfile->status.max_msgs = scfg.sub[subnum]->maxmsgs;
		if(scfg.sub[subnum]->misc&SUB_HYPER)
			storage = smbfile->status.attr = SMB_HYPERALLOC;
		else if(scfg.sub[subnum]->misc&SUB_FAST)
			storage = SMB_FASTALLOC;
		if(scfg.sub[subnum]->misc&SUB_LZH)
			xlat=XLAT_LZH;

		msg.idx.time=msg.hdr.when_imported.time;	/* needed for MSG-ID generation */
		msg.idx.number=smbfile->status.last_msg+1;		/* needed for MSG-ID generation */

		/* Generate default (RFC822) message-id (always) */
		get_msgid(&scfg,subnum,&msg,msg_id,sizeof(msg_id));
		smb_hfield_str(&msg,RFC822MSGID,msg_id);
	}
	if(smbfile->status.max_crcs==0)
		dupechk_hashes&=~(1<<SMB_HASH_SOURCE_BODY);
	/* Bad echo area collects a *lot* of messages, and thus, hashes - so no dupe checking */
	if(cfg.badecho>=0 && subnum==cfg.area[cfg.badecho].sub)
		dupechk_hashes=SMB_HASH_SOURCE_NONE;

	i=smb_addmsg(smbfile, &msg, storage, dupechk_hashes, xlat, sbody, stail);

	if(i!=SMB_SUCCESS) {
		lprintf(LOG_ERR,"ERROR smb_addmsg returned %d: %s"
			,i,smbfile->last_error);
	}
	smb_freemsgmem(&msg);

	return(i);
}

/***********************************************************************/
/* Get zone and point from kludge lines from the stream  if they exist */
/***********************************************************************/
void getzpt(FILE *stream, fmsghdr_t *hdr)
{
	char buf[0x1000];
	int i,len,cr=0;
	long pos;
	faddr_t faddr;

	pos=ftell(stream);
	len=fread(buf,1,0x1000,stream);
	for(i=0;i<len;i++) {
		if(buf[i]=='\n')	/* ignore line-feeds */
			continue;
		if((!i || cr) && buf[i]==CTRL_A) {	/* kludge */
			if(!strncmp(buf+i+1,"TOPT ",5))
				hdr->destpoint=atoi(buf+i+6);
			else if(!strncmp(buf+i+1,"FMPT ",5))
				hdr->origpoint=atoi(buf+i+6);
			else if(!strncmp(buf+i+1,"INTL ",5)) {
				faddr=atofaddr(buf+i+6);
				hdr->destzone=faddr.zone;
				hdr->destnet=faddr.net;
				hdr->destnode=faddr.node;
				i+=6;
				while(buf[i] && buf[i]!=' ') i++;
				faddr=atofaddr(buf+i+1);
				hdr->origzone=faddr.zone;
				hdr->orignet=faddr.net;
				hdr->orignode=faddr.node; 
			}
			while(i<len && buf[i]!='\r') i++;
			cr=1;
			continue; 
		}
		if(buf[i]=='\r')
			cr=1;
		else
			cr=0; 
	}
	fseek(stream,pos,SEEK_SET);
}
/******************************************************************************
 This function will seek to the next NULL found in stream
******************************************************************************/
void seektonull(FILE *stream)
{
	char ch;

	while(!feof(stream)) {
		if(!fread(&ch,1,1,stream))
			break;
		if(!ch)
			break; 
	}
}

/******************************************************************************
 This function returns a packet name - used for outgoing packets
******************************************************************************/
char *pktname(BOOL temp)
{
	static char str[128];
	int i;
    time_t now;
    struct tm *tm;

	now=time(NULL);
	for(i=0;i>=0;i++) {
		now++;
		tm=localtime(&now);
		sprintf(str,"%s%02u%02u%02u%02u.%s",cfg.outbound,tm->tm_mday,tm->tm_hour
			,tm->tm_min,tm->tm_sec,temp ? "pk_" : "pkt");
		if(!fexist(str))				/* Add 1 second if name exists */
			return(str); 
	}
	return(NULL);	/* This should never happen */
}

BOOL foreign_zone(uint16_t zone1, uint16_t zone2)
{
	if(cfg.zone_blind && zone1 <= cfg.zone_blind_threshold && zone2 <= cfg.zone_blind_threshold)
		return FALSE;
	return zone1!=zone2;
}

/******************************************************************************
 This function puts a message into a Fido packet, writing both the header
 information and the message body
******************************************************************************/
void putfmsg(FILE *stream,char *fbuf,fmsghdr_t fmsghdr,areasbbs_t area
	,addrlist_t seenbys,addrlist_t paths)
{
	char str[256],seenby[256];
	short i,j,lastlen=0,net_exists=0;
	faddr_t addr,sysaddr;
	fpkdmsg_t pkdmsg;
	time_t t;
	size_t len;
	struct tm* tm;

	addr=getsysfaddr(fmsghdr.destzone);

	/* Write fixed-length header fields */
	memset(&pkdmsg,0,sizeof(pkdmsg));
	pkdmsg.type		= 2;
	pkdmsg.orignet	= addr.net;
	pkdmsg.orignode	= addr.node;
	pkdmsg.destnet	= fmsghdr.destnet;
	pkdmsg.destnode	= fmsghdr.destnode;
	pkdmsg.attr		= fmsghdr.attr;
	pkdmsg.cost		= fmsghdr.cost;
	SAFECOPY(pkdmsg.time,fmsghdr.time);
	fwrite(&pkdmsg		,sizeof(pkdmsg)			,1,stream);

	/* Write variable-length (ASCIIZ) header fields */
	fwrite(fmsghdr.to	,strlen(fmsghdr.to)+1	,1,stream);
	fwrite(fmsghdr.from	,strlen(fmsghdr.from)+1	,1,stream);
	fwrite(fmsghdr.subj	,strlen(fmsghdr.subj)+1	,1,stream);

	len = strlen((char *)fbuf);

	/* Write message body */
	if(area.name)
		if(strncmp((char *)fbuf,"AREA:",5))                     /* No AREA: Line */
			fprintf(stream,"AREA:%s\r",area.name);              /* So add one */
	fwrite(fbuf,len,1,stream);
	lastlen=9;
	if(len && fbuf[len-1]!='\r')
		fputc('\r',stream);

	if(area.name==NULL)	{ /* NetMail, so add FSP-1010 Via kludge line */
		t=time(NULL);
		tm=gmtime(&t);
		fprintf(stream,"\1Via %s @%04u%02u%02u.%02u%02u%02u.UTC "
			"SBBSecho %u.%02u-%s r%s\r"
			,smb_faddrtoa(&addr,NULL)
			,tm->tm_year+1900
			,tm->tm_mon+1
			,tm->tm_mday
			,tm->tm_hour
			,tm->tm_min
			,tm->tm_sec
			,SBBSECHO_VERSION_MAJOR,SBBSECHO_VERSION_MINOR,PLATFORM_DESC,revision);
	}
			
	if(area.name) { /* EchoMail, Not NetMail */
		if(foreign_zone(addr.zone, fmsghdr.destzone))	/* Zone Gate */
			fprintf(stream,"SEEN-BY: %d/%d\r",fmsghdr.destnet,fmsghdr.destnode);
		else {
			fprintf(stream,"SEEN-BY:");
			for(i=0;i<seenbys.addrs;i++) {			  /* Put back original SEEN-BYs */
				strcpy(seenby," ");
				if(foreign_zone(addr.zone, seenbys.addr[i].zone))
					continue;
				if(seenbys.addr[i].net!=addr.net || !net_exists) {
					net_exists=1;
					addr.net=seenbys.addr[i].net;
					sprintf(str,"%d/",addr.net);
					strcat(seenby,str); 
				}
				sprintf(str,"%d",seenbys.addr[i].node);
				strcat(seenby,str);
				if(lastlen+strlen(seenby)<80) {
					fwrite(seenby,strlen(seenby),1,stream);
					lastlen+=strlen(seenby); 
				}
				else {
					--i;
					lastlen=9; /* +strlen(seenby); */
					net_exists=0;
					fprintf(stream,"\rSEEN-BY:"); 
				} 
			}

			for(i=0;i<area.uplinks;i++) {			/* Add all uplinks to SEEN-BYs */
				int node=matchnode(area.uplink[i],0);
				if(node<cfg.nodecfgs && (cfg.nodecfg[node].attr&ATTR_PASSIVE))
					continue;
				strcpy(seenby," ");
				if(foreign_zone(addr.zone, area.uplink[i].zone) || area.uplink[i].point)
					continue;
				for(j=0;j<seenbys.addrs;j++)
					if(!memcmp(&area.uplink[i],&seenbys.addr[j],sizeof(faddr_t)))
						break;
				if(j==seenbys.addrs) {
					if(area.uplink[i].net!=addr.net || !net_exists) {
						net_exists=1;
						addr.net=area.uplink[i].net;
						sprintf(str,"%d/",addr.net);
						strcat(seenby,str); 
					}
					sprintf(str,"%d",area.uplink[i].node);
					strcat(seenby,str);
					if(lastlen+strlen(seenby)<80) {
						fwrite(seenby,strlen(seenby),1,stream);
						lastlen+=strlen(seenby); 
					}
					else {
						--i;
						lastlen=9; /* +strlen(seenby); */
						net_exists=0;
						fprintf(stream,"\rSEEN-BY:"); 
					} 
				} 
			}

			for(i=0;i<scfg.total_faddrs;i++) {				/* Add AKAs to SEEN-BYs */
				strcpy(seenby," ");
				if(foreign_zone(addr.zone, scfg.faddr[i].zone) || scfg.faddr[i].point)
					continue;
				for(j=0;j<seenbys.addrs;j++)
					if(!memcmp(&scfg.faddr[i],&seenbys.addr[j],sizeof(faddr_t)))
						break;
				if(j==seenbys.addrs) {
					if(scfg.faddr[i].net!=addr.net || !net_exists) {
						net_exists=1;
						addr.net=scfg.faddr[i].net;
						sprintf(str,"%d/",addr.net);
						strcat(seenby,str); 
					}
					sprintf(str,"%d",scfg.faddr[i].node);
					strcat(seenby,str);
					if(lastlen+strlen(seenby)<80) {
						fwrite(seenby,strlen(seenby),1,stream);
						lastlen+=strlen(seenby); 
					}
					else {
						--i;
						lastlen=9; /* +strlen(seenby); */
						net_exists=0;
						fprintf(stream,"\rSEEN-BY:"); 
					} 
				} 
			}

			lastlen=7;
			net_exists=0;
			fprintf(stream,"\r\1PATH:");
			addr=getsysfaddr(fmsghdr.destzone);
			for(i=0;i<paths.addrs;i++) {			  /* Put back the original PATH */
				if(paths.addr[i].net == 0)
					continue;	// Invalid node number/address, don't include "0/0" in PATH
				strcpy(seenby," ");
				if(foreign_zone(addr.zone, paths.addr[i].zone) || paths.addr[i].point)
					continue;
				if(paths.addr[i].net!=addr.net || !net_exists) {
					net_exists=1;
					addr.net=paths.addr[i].net;
					sprintf(str,"%d/",addr.net);
					strcat(seenby,str); 
				}
				sprintf(str,"%d",paths.addr[i].node);
				strcat(seenby,str);
				if(lastlen+strlen(seenby)<80) {
					fwrite(seenby,strlen(seenby),1,stream);
					lastlen+=strlen(seenby); 
				}
				else {
					--i;
					lastlen=7; /* +strlen(seenby); */
					net_exists=0;
					fprintf(stream,"\r\1PATH:"); 
				} 
			}

			strcpy(seenby," ");         /* Add first address with same zone to PATH */
			sysaddr=getsysfaddr(fmsghdr.destzone);
			if(sysaddr.net!=0 && sysaddr.point==0) {
				if(sysaddr.net!=addr.net || !net_exists) {
					net_exists=1;
					addr.net=sysaddr.net;
					sprintf(str,"%d/",addr.net);
					strcat(seenby,str); 
				}
				sprintf(str,"%d",sysaddr.node);
				strcat(seenby,str);
				if(lastlen+strlen(seenby)<80)
					fwrite(seenby,strlen(seenby),1,stream);
				else {
					fprintf(stream,"\r\1PATH:");
					fwrite(seenby,strlen(seenby),1,stream); 
				} 
			}
			fputc('\r',stream); 
		}
	}

	fputc(FIDO_PACKED_MSG_TERMINATOR, stream);
}

size_t terminate_packet(FILE* stream)
{
	WORD	terminator=FIDO_PACKET_TERMINATOR;

	return fwrite(&terminator, sizeof(terminator), 1, stream);
}

long find_packet_terminator(FILE* stream)
{
	WORD	terminator;
	long	offset;

	fseek(stream, 0, SEEK_END);
	offset = ftell(stream);
	if(offset >= sizeof(fpkthdr_t)+sizeof(terminator)) {
		fseek(stream, offset-sizeof(terminator), SEEK_SET);
		if(fread(&terminator, sizeof(terminator), 1, stream)
			&& terminator==FIDO_PACKET_TERMINATOR)
			offset -= sizeof(terminator);
	}
	return(offset);
}

/******************************************************************************
 This function creates a binary list of the message seen-bys and path from
 inbuf.
******************************************************************************/
void gen_psb(addrlist_t *seenbys,addrlist_t *paths,char *inbuf
	,ushort zone)
{
	char str[128],seenby[256],*p,*p1,*p2,*fbuf;
	int i,j,len;
	faddr_t addr;

	if(!inbuf)
		return;
	fbuf=strstr((char *)inbuf,"\r * Origin: ");
	if(!fbuf)
	fbuf=strstr((char *)inbuf,"\n * Origin: ");
	if(!fbuf)
		fbuf=inbuf;
	if(seenbys->addr) {
		FREE_AND_NULL(seenbys->addr);
		seenbys->addrs=0; 
	}
	addr.zone=addr.net=addr.node=addr.point=seenbys->addrs=0;
	p=strstr((char *)fbuf,"\rSEEN-BY:");
	if(!p) p=strstr((char *)fbuf,"\nSEEN-BY:");
	if(p) {
		while(1) {
			sprintf(str,"%-.100s",p+10);
			if((p1=strchr(str,'\r'))!=NULL)
				*p1=0;
			p1=str;
			i=j=0;
			len=strlen(str);
			while(i<len) {
				j=i;
				while(i<len && *(p1+i)!=' ')
					++i;
				if(j>len)
					break;
				sprintf(seenby,"%-.*s",(i-j),p1+j);
				if((p2=strchr(seenby,':'))!=NULL) {
					addr.zone=atoi(seenby);
					addr.net=atoi(p2+1); 
				}
				else if((p2=strchr(seenby,'/'))!=NULL)
					addr.net=atoi(seenby);
				if((p2=strchr(seenby,'/'))!=NULL)
					addr.node=atoi(p2+1);
				else
					addr.node=atoi(seenby);
				if((p2=strchr(seenby,'.'))!=NULL)
					addr.point=atoi(p2+1);
				if(!addr.zone)
					addr.zone=zone; 		/* Was 1 */
				if((seenbys->addr=(faddr_t *)realloc(seenbys->addr
					,sizeof(faddr_t)*(seenbys->addrs+1)))==NULL) {
					lprintf(LOG_ERR,"ERROR line %d allocating memory for message "
						"seenbys.",__LINE__);
					bail(1); 
					return;
				}
				memcpy(&seenbys->addr[seenbys->addrs],&addr,sizeof(faddr_t));
				seenbys->addrs++;
				++i; 
			}
			p1=strstr(p+10,"\rSEEN-BY:");
			if(!p1)
				p1=strstr(p+10,"\nSEEN-BY:");
			if(!p1)
				break;
			p=p1; 
		} 
	}
	else {
		if((seenbys->addr=(faddr_t *)realloc(seenbys->addr
			,sizeof(faddr_t)))==NULL) {
			lprintf(LOG_ERR,"ERROR line %d allocating memory for message seenbys."
				,__LINE__);
			bail(1); 
			return;
		}
		memset(&seenbys->addr[0],0,sizeof(faddr_t)); 
	}

	if(paths->addr) {
		FREE_AND_NULL(paths->addr);
		paths->addrs=0; 
	}
	addr.zone=addr.net=addr.node=addr.point=paths->addrs=0;
	if((p=strstr((char *)fbuf,"\1PATH:"))!=NULL) {
		while(1) {
			sprintf(str,"%-.100s",p+7);
			if((p1=strchr(str,'\r'))!=NULL)
				*p1=0;
			p1=str;
			i=j=0;
			len=strlen(str);
			while(i<len) {
				j=i;
				while(i<len && *(p1+i)!=' ')
					++i;
				if(j>len)
					break;
				sprintf(seenby,"%-.*s",(i-j),p1+j);
				if((p2=strchr(seenby,':'))!=NULL) {
					addr.zone=atoi(seenby);
					addr.net=atoi(p2+1); 
				}
				else if((p2=strchr(seenby,'/'))!=NULL)
					addr.net=atoi(seenby);
				if((p2=strchr(seenby,'/'))!=NULL)
					addr.node=atoi(p2+1);
				else
					addr.node=atoi(seenby);
				if((p2=strchr(seenby,'.'))!=NULL)
					addr.point=atoi(p2+1);
				if(!addr.zone)
					addr.zone=zone; 		/* Was 1 */
				if((paths->addr=(faddr_t *)realloc(paths->addr
					,sizeof(faddr_t)*(paths->addrs+1)))==NULL) {
					lprintf(LOG_ERR,"ERROR line %d allocating memory for message "
						"paths.",__LINE__);
					bail(1); 
					return;
				}
				memcpy(&paths->addr[paths->addrs],&addr,sizeof(faddr_t));
				paths->addrs++;
				++i; 
			}
			if((p1=strstr(p+7,"\1PATH:"))==NULL)
				break;
			p=p1; 
		} 
	}
	else {
		if((paths->addr=(faddr_t *)realloc(paths->addr
			,sizeof(faddr_t)))==NULL) {
			lprintf(LOG_ERR,"ERROR line %d allocating memory for message paths."
				,__LINE__);
			bail(1); 
			return;
		}
		memset(&paths->addr[0],0,sizeof(faddr_t)); 
	}
}

/******************************************************************************
 This function takes the addrs passed to it and compares them to the address
 passed in compaddr.	TRUE is returned if inaddr matches any of the addrs
 otherwise FALSE is returned.
******************************************************************************/
BOOL check_psb(addrlist_t* addrlist, faddr_t compaddr)
{
	int i;

	for(i=0;i<addrlist->addrs;i++) {
		if(foreign_zone(compaddr.zone, addrlist->addr[i].zone))
			continue;
		if(compaddr.net != addrlist->addr[i].net)
			continue;
		if(compaddr.node != addrlist->addr[i].node)
			continue;
		if(compaddr.point != addrlist->addr[i].point)
			continue;
		return(TRUE); /* match found */
	}
	return(FALSE);	/* match not found */
}

/******************************************************************************
 This function strips the message seen-bys and path from inbuf.
******************************************************************************/
void strip_psb(char *inbuf)
{
	char *p,*fbuf;

	if(!inbuf)
		return;
	fbuf=strstr((char *)inbuf,"\r * Origin: ");
	if(!fbuf)
		fbuf=inbuf;
	if((p=strstr((char *)fbuf,"\rSEEN-BY:"))!=NULL)
		*(p)=0;
	if((p=strstr((char *)fbuf,"\r\1PATH:"))!=NULL)
		*(p)=0;
}
void attach_bundles(void)
{
	FILE *fidomsg;
	char str[1025],path[MAX_PATH+1],*packet;
	int fmsg;
	faddr_t pkt_faddr;
	pkthdr_t pkthdr;
	size_t	f;
	glob_t	g;
	two_plus_t* two_plus;

	sprintf(path,"%s*.pk_",cfg.outbound);
	glob(path,0,NULL,&g);
	for(f=0;f<g.gl_pathc && !kbhit();f++) {

		packet=g.gl_pathv[f];

		printf("%21s: %s ","Outbound Packet",packet);
		if((fmsg=sopen(packet,O_RDWR|O_BINARY,SH_DENYRW))==-1) {
			lprintf(LOG_ERR,"ERROR %u (%s) line %d opening %s",errno,strerror(errno),__LINE__,packet);
			continue; 
		}
		if((fidomsg=fdopen(fmsg,"r+b"))==NULL) {
			close(fmsg);
			lprintf(LOG_ERR,"ERROR %u (%s) line %d fdopening %s",errno,strerror(errno),__LINE__,packet);
			continue; 
		}
		if(filelength(fmsg)<sizeof(pkthdr_t)) {
			lprintf(LOG_ERR,"ERROR line %d invalid length of %lu bytes for %s"
				,__LINE__,filelength(fmsg),packet);
			fclose(fidomsg);
			if(delfile(packet))
				lprintf(LOG_ERR,"ERROR line %d removing %s %s",__LINE__,packet
					,strerror(errno));
			continue; 
		}
		if(fread(&pkthdr,sizeof(pkthdr_t),1,fidomsg)!=1) {
			fclose(fidomsg);
			lprintf(LOG_ERR,"ERROR line %d reading %u bytes from %s",__LINE__
				,sizeof(pkthdr_t),packet);
			continue; 
		}
		fseek(fidomsg,-2L,SEEK_END);
		fread(str,2,1,fidomsg);
		fclose(fidomsg);
		if(!str[0] && !str[1]) {	/* Check for two nulls at end of packet */
			pkt_faddr.zone=pkthdr.destzone;
			pkt_faddr.net=pkthdr.destnet;
			pkt_faddr.node=pkthdr.destnode;
			pkt_faddr.point=0;				/* No point info in the 2.0 hdr! */
			two_plus = (two_plus_t*)&pkthdr.empty;
			if(two_plus->cword==_rotr(two_plus->cwcopy,8)  /* 2+ Packet Header */
				&& (two_plus->cword&1))
				pkt_faddr.point=two_plus->destpoint;
			else if(pkthdr.baud==2) 				/* Type 2.2 Packet Header */
				pkt_faddr.point=pkthdr.month; 
			lprintf(LOG_INFO,"Sending to %s",smb_faddrtoa(&pkt_faddr,NULL));
			pack_bundle(packet,pkt_faddr); 
		} else
			lprintf(LOG_WARNING,"Outbound Packet (%s) possibly still in use (invalid terminator: %02X%02X)"
				,packet,(BYTE)str[0],(BYTE)str[1]); 
	}
	globfree(&g);
}
/******************************************************************************
 This is where we put outgoing messages into packets.  Set the 'cleanup'
 parameter to 1 to force all the remaining packets closed and stuff them into
 a bundle.
******************************************************************************/
void pkt_to_pkt(char *fbuf,areasbbs_t area,faddr_t faddr
	,fmsghdr_t fmsghdr,addrlist_t seenbys,addrlist_t paths, int cleanup)
{
	int i,j,k,file;
	short node;
	time_t now;
	struct tm *tm;
	pkthdr_t pkthdr;
	static ushort openpkts,totalpkts;
	static outpkt_t outpkt[MAX_TOTAL_PKTS];
	faddr_t sysaddr;
	two_plus_t* two_plus;

	if(cleanup==1) {
		for(i=0;i<totalpkts;i++) {
			if(i>=MAX_TOTAL_PKTS) {
				lprintf(LOG_ERR,"MAX_TOTAL_PKTS (%d) REACHED!",MAX_TOTAL_PKTS);
				break;
			}
			if(outpkt[i].stream==NULL)
				outpkt[i].stream=fnopen(&file,outpkt[i].filename,O_WRONLY|O_APPEND);
				
			if(outpkt[i].stream==NULL)
				lprintf(LOG_ERR,"ERROR %u (%s) line %d opening %s for termination",errno,strerror(errno),__LINE__,outpkt[i].filename);
			else {
				terminate_packet(outpkt[i].stream);
				fclose(outpkt[i].stream); 
			}
			  /* pack_bundle() disabled.  Why?  ToDo */
			  /* pack_bundle(outpkt[i].filename,outpkt[i].uplink); */
			memset(&outpkt[i],0,sizeof(outpkt_t)); 
		}
		totalpkts=openpkts=0;
		attach_bundles();
		if(!(misc&FLO_MAILER))
			attachment(0,faddr,ATTACHMENT_NETMAIL);
		return; 
	}

	if(fbuf==NULL) {
		lprintf(LOG_ERR,"ERROR line %d allocating fbuf",__LINE__);
		return; 
	}
	/* We want to see if there's already a packet open for this area.   */
	/* If not, we'll open a new one.  Once we have a packet, we'll add  */
	/* messages to it as they come in.	If necessary, we'll close an    */
	/* open packet to open a new one.									*/

	for(j=0;j<area.uplinks;j++) {
		if((cleanup==2 && memcmp(&faddr,&area.uplink[j],sizeof(faddr_t))) ||
			(!cleanup && (!memcmp(&faddr,&area.uplink[j],sizeof(faddr_t)) ||
			check_psb(&seenbys,area.uplink[j]))))
			continue;
		node=matchnode(area.uplink[j],0);
		if(node<cfg.nodecfgs && (cfg.nodecfg[node].attr&ATTR_PASSIVE))
			continue;
		sysaddr=getsysfaddr(area.uplink[j].zone);
		printf("%s ",smb_faddrtoa(&area.uplink[j],NULL));
		for(i=0;i<totalpkts;i++) {
			if(i>=MAX_TOTAL_PKTS) {
				lprintf(LOG_ERR,"MAX_TOTAL_PKTS (%d) REACHED!",MAX_TOTAL_PKTS);
				break;
			}
			if(!memcmp(&area.uplink[j],&outpkt[i].uplink,sizeof(faddr_t))) {
				if(outpkt[i].stream==NULL) {
					if(openpkts==DFLT_OPEN_PKTS) {
						for(k=0;k<totalpkts;k++) {
							if(outpkt[k].stream!=NULL) {
								fclose(outpkt[k].stream);
								outpkt[k].stream=NULL;
								break; 
							} 
						}
					}
					if((outpkt[i].stream=fnopen(&file,outpkt[i].filename
						,O_WRONLY|O_APPEND))==NULL) {
						lprintf(LOG_ERR,"ERROR %u (%s) line %d opening %s",errno,strerror(errno),__LINE__,outpkt[i].filename);
						continue;
					}
				}
				if((strlen((char *)fbuf)+1+ftell(outpkt[i].stream))
					<=cfg.maxpktsize) {
					fmsghdr.destnode=area.uplink[j].node;
					fmsghdr.destnet=area.uplink[j].net;
					fmsghdr.destzone=area.uplink[j].zone;
					putfmsg(outpkt[i].stream,fbuf,fmsghdr,area,seenbys,paths); 
				}
				else {
					terminate_packet(outpkt[i].stream);
					fclose(outpkt[i].stream);
					/* pack_bundle() disabled.  Why?  ToDo */
					/* pack_bundle(outpkt[i].filename,outpkt[i].uplink); */
					outpkt[i].stream=outpkt[totalpkts-1].stream;
					memcpy(&outpkt[i],&outpkt[totalpkts-1],sizeof(outpkt_t));
					memset(&outpkt[totalpkts-1],0,sizeof(outpkt_t));
					--totalpkts;
					--openpkts;
					i=totalpkts; 
				}
				break; 
			} 
		}
		if(i==totalpkts) {
			if(openpkts==DFLT_OPEN_PKTS) {
				for(k=0;k<totalpkts;k++) {
					if(outpkt[k].stream!=NULL) {
						fclose(outpkt[k].stream);
						outpkt[k].stream=NULL;
						--openpkts;
						break; 
					} 
				}
			}
			SAFECOPY(outpkt[i].filename,pktname(/* temp? */ TRUE));
			now=time(NULL);
			tm=localtime(&now);
			if((outpkt[i].stream=fnopen(&file,outpkt[i].filename
				,O_WRONLY|O_CREAT))==NULL) {
				lprintf(LOG_ERR,"ERROR %u (%s) line %d opening %s"
					,errno,strerror(errno),__LINE__,outpkt[i].filename);	/* failhere */
				lprintf(LOG_DEBUG,"i=%d, totalpkts=%d, openpkts=%d", i, totalpkts, openpkts);
				bail(1); 
				return;
			}
			memset(&pkthdr, 0, sizeof(pkthdr));
			pkthdr.orignode=sysaddr.node;
			fmsghdr.destnode=pkthdr.destnode=area.uplink[j].node;
			if(node<cfg.nodecfgs && cfg.nodecfg[node].pkt_type==PKT_TWO_TWO) {
				pkthdr.year=sysaddr.point;
				pkthdr.month=area.uplink[j].point;
				pkthdr.baud=0x0002;	/* Indicates 2.2 */
			}
			else {
				pkthdr.year=tm->tm_year+1900;
				pkthdr.month=tm->tm_mon;
				pkthdr.day=tm->tm_mday;
				pkthdr.hour=tm->tm_hour;
				pkthdr.min=tm->tm_min;
				pkthdr.sec=tm->tm_sec;
			}
			pkthdr.pkttype=0x0002;
			pkthdr.orignet=sysaddr.net;
			fmsghdr.destnet=pkthdr.destnet=area.uplink[j].net;
			pkthdr.prodcode=SBBSECHO_PRODUCT_CODE&0xff;
			pkthdr.sernum=SBBSECHO_VERSION_MAJOR;
			if(node<cfg.nodecfgs)
				memcpy(pkthdr.password,cfg.nodecfg[node].pktpwd,sizeof(pkthdr.password));
			pkthdr.origzone=sysaddr.zone;
			fmsghdr.destzone=pkthdr.destzone=area.uplink[j].zone;

			if(node<cfg.nodecfgs) {
				if(cfg.nodecfg[node].pkt_type==PKT_TWO_TWO) {
					strcpy(((two_two_t*)&pkthdr.empty)->origdomn,"fidonet");
					strcpy(((two_two_t*)&pkthdr.empty)->destdomn,"fidonet");
				}
				else if(cfg.nodecfg[node].pkt_type==PKT_TWO_PLUS) {
					two_plus=(two_plus_t*)&pkthdr.empty;
					if(sysaddr.point) {
						pkthdr.orignet=-1;
						two_plus->auxnet=sysaddr.net; 
					}
					two_plus->cwcopy=0x0100;
					two_plus->prodcode=SBBSECHO_PRODUCT_CODE>>8;
					two_plus->revision=SBBSECHO_VERSION_MINOR;
					two_plus->cword=0x0001;
					two_plus->origzone=pkthdr.origzone;
					two_plus->destzone=pkthdr.destzone;
					two_plus->origpoint=sysaddr.point;
					two_plus->destpoint=area.uplink[j].point;
				}
			}
			fwrite(&pkthdr,sizeof(pkthdr_t),1,outpkt[totalpkts].stream);
			putfmsg(outpkt[totalpkts].stream,fbuf,fmsghdr,area,seenbys,paths);
			memcpy(&outpkt[totalpkts].uplink,&area.uplink[j]
				,sizeof(faddr_t));
			++openpkts;
			++totalpkts;
			if(totalpkts>=MAX_TOTAL_PKTS) {
				fclose(outpkt[totalpkts-1].stream);
				outpkt[totalpkts-1].stream=NULL;
				/* pack_bundle() disabled.  Why?  ToDo */
				/* pack_bundle(outpkt[totalpkts-1].filename
					  ,outpkt[totalpkts-1].uplink); */
				--totalpkts;
				--openpkts; 
			}
		}
	}
}

int pkt_to_msg(FILE* fidomsg, fmsghdr_t* hdr, char* info)
{
	char path[MAX_PATH+1];
	char* fmsgbuf;
	int i,file;
	ulong l;

	if((fmsgbuf=getfmsg(fidomsg,&l))==NULL) {
		lprintf(LOG_ERR,"ERROR line %d netmail allocation",__LINE__);
		return(-1); 
	}

	if(!l && (misc&KILL_EMPTY_MAIL))
		printf("Empty NetMail");
	else {
		printf("Exporting: ");
		for(i=1;i;i++) {
			sprintf(path,"%s%u.msg",scfg.netmail_dir,i);
			if(!fexistcase(path))
				break; 
		}
		if(!i) {
			lprintf(LOG_WARNING,"Too many netmail messages");
			return(-1); 
		}
		if((file=nopen(path,O_WRONLY|O_CREAT))==-1) {
			lprintf(LOG_ERR,"ERROR %u (%s) line %d creating %s",errno,strerror(errno),__LINE__,path);
			return(-1);
		}
		write(file,hdr,sizeof(fmsghdr_t));
		write(file,fmsgbuf,l+1); /* Write the '\0' terminator too */
		close(file);
		printf("%s", path);
		lprintf(LOG_INFO,"%s Exported to %s",info,path);
	}
	free(fmsgbuf); 

	return(0);
}


/**************************************/
/* Send netmail, returns 0 on success */
/**************************************/
int import_netmail(char *path,fmsghdr_t hdr, FILE *fidomsg)
{
	char info[512],str[256],tmp[256],subj[256]
		,*fmsgbuf=NULL,*p,*tp,*sp;
	int i,match,usernumber;
	ulong length;
	faddr_t addr;

	hdr.destzone=sys_faddr.zone;
	hdr.destpoint=hdr.origpoint=0;
	getzpt(fidomsg,&hdr);				/* use kludge if found */
	for(match=0;match<scfg.total_faddrs;match++)
		if((hdr.destzone==scfg.faddr[match].zone || (misc&FUZZY_ZONE))
			&& hdr.destnet==scfg.faddr[match].net
			&& hdr.destnode==scfg.faddr[match].node
			&& hdr.destpoint==scfg.faddr[match].point)
			break;
	if(match<scfg.total_faddrs && (misc&FUZZY_ZONE))
		hdr.origzone=hdr.destzone=scfg.faddr[match].zone;
	if(hdr.origpoint)
		sprintf(tmp,".%hu",hdr.origpoint);
	else
		tmp[0]=0;
	if(hdr.destpoint)
		sprintf(str,".%hu",hdr.destpoint);
	else
		str[0]=0;
	sprintf(info,"%s%s%s (%hu:%hu/%hu%s) To: %s (%hu:%hu/%hu%s)"
		,path,path[0] ? " ":""
		,hdr.from,hdr.origzone,hdr.orignet,hdr.orignode,tmp
		,hdr.to,hdr.destzone,hdr.destnet,hdr.destnode,str);
	printf("%s ",info);

	if(!(misc&IMPORT_NETMAIL)) {
		printf("Ignored");
		if(!path[0]) {
			printf(" - ");
			pkt_to_msg(fidomsg,&hdr,info);
		} else if(cfg.log&LOG_IGNORED)
			lprintf(LOG_INFO,"%s Ignored",info);

		return(-1); 
	}

	if(match>=scfg.total_faddrs && !(misc&IGNORE_ADDRESS)) {
		printf("Foreign address");
		if(!path[0]) {
			printf(" - ");
			pkt_to_msg(fidomsg,&hdr,info);
		}
		return(2); 
	}

	if(path[0]) {	/* .msg file, not .pkt */
		if(hdr.attr&FIDO_ORPHAN) {
			printf("Orphaned");
			return(1); 
		}
		if((hdr.attr&FIDO_RECV) && !(misc&IGNORE_RECV)) {
			printf("Already received");
			return(3); 
		}
		if((hdr.attr&FIDO_LOCAL) && !(misc&LOCAL_NETMAIL)) {
			printf("Created locally");
			return(4); 
		}
		if(hdr.attr&FIDO_INTRANS) {
			printf("In-transit");
			return(5);
		}
	}

	if(email->shd_fp==NULL) {
		sprintf(email->file,"%smail",scfg.data_dir);
		email->retry_time=scfg.smb_retry_time;
		if((i=smb_open(email))!=SMB_SUCCESS) {
			lprintf(LOG_ERR,"ERROR %d (%s) line %d opening %s",i,email->last_error,__LINE__,email->file);
			bail(1); 
			return -1;
		} 
	}

	if(!filelength(fileno(email->shd_fp))) {
		email->status.max_crcs=scfg.mail_maxcrcs;
		email->status.max_msgs=0;
		email->status.max_age=scfg.mail_maxage;
		email->status.attr=SMB_EMAIL;
		if((i=smb_create(email))!=SMB_SUCCESS) {
			lprintf(LOG_ERR,"ERROR %d (%s) creating %s",i,email->last_error,email->file);
			bail(1); 
			return -1;
		} 
	}

	if(!stricmp(hdr.to,"AREAFIX") || !stricmp(hdr.to,"SBBSECHO")) {
		fmsgbuf=getfmsg(fidomsg,NULL);
		if(path[0]) {
			if(misc&DELETE_NETMAIL) {
				fclose(fidomsg);
				if(delfile(path))
					lprintf(LOG_ERR,"ERROR line %d removing %s %s",__LINE__,path
						,strerror(errno)); 
			}
			else {
				hdr.attr|=FIDO_RECV;
				fseek(fidomsg,0L,SEEK_SET);
				fwrite(&hdr,sizeof(fmsghdr_t),1,fidomsg);
				fclose(fidomsg); /* Gotta close it here for areafix stuff */
			}
		}
		addr.zone=hdr.origzone;
		addr.net=hdr.orignet;
		addr.node=hdr.orignode;
		addr.point=hdr.origpoint;
		if(cfg.log&LOG_AREAFIX)
			logprintf(info);
		p=process_areafix(addr,fmsgbuf,/* Password: */hdr.subj, /* To: */hdr.from);
		if(p && cfg.notify) {
			SAFECOPY(hdr.to,scfg.sys_op);
			SAFECOPY(hdr.from,"SBBSecho");
			SAFECOPY(hdr.subj,"Areafix Request");
			hdr.origzone=hdr.orignet=hdr.orignode=hdr.origpoint=0;
			if(fmsgtosmsg(p,hdr,cfg.notify,INVALID_SUB)==0) {
				sprintf(str,"\7\1n\1hSBBSecho \1n\1msent you mail\r\n");
				putsmsg(&scfg,cfg.notify,str); 
			}
		}
		FREE_AND_NULL(fmsgbuf);
		return(-2); 
	}

	usernumber=atoi(hdr.to);
	if(cfg.sysop_alias[0] && stricmp(hdr.to,cfg.sysop_alias)==0)  /* NetMail to configured SYSOP_ALIAS goes to user #1 */
		usernumber=1;
	if(!usernumber && match<scfg.total_faddrs)
		usernumber=matchname(hdr.to);
	if(!usernumber) {
		if(misc&UNKNOWN_NETMAIL && match<scfg.total_faddrs)	/* receive unknown user */
			usernumber=1;								/* mail to 1 */
		else {
			if(match<scfg.total_faddrs) {
				printf("Unknown user");
				if(cfg.log&LOG_UNKNOWN)
					lprintf(LOG_WARNING,"%s Unknown user",info); 
			}

			if(!path[0]) {
				printf(" - ");
				pkt_to_msg(fidomsg,&hdr,info);
			}
			return(2); 
		} 
	}

	/*********************/
	/* Importing NetMail */
	/*********************/

	fmsgbuf=getfmsg(fidomsg,&length);

	switch(i=fmsgtosmsg(fmsgbuf,hdr,usernumber,INVALID_SUB)) {
		case 0:			/* success */
			break;
		case 2:			/* filtered */
			if(cfg.log&LOG_IGNORED)
				lprintf(LOG_WARNING,"%s Filtered - Ignored",info);
			break;
		case 3:			/* empty */
			if(cfg.log&LOG_IGNORED)
				lprintf(LOG_WARNING,"%s Empty - Ignored",info);
			break;
		default:
			lprintf(LOG_ERR,"ERROR (%d) Importing %s",i,info);
			break;
	}
	if(i) {
		FREE_AND_NULL(fmsgbuf);
		return(0);
	}

	addr.zone=hdr.origzone;
	addr.net=hdr.orignet;
	addr.node=hdr.orignode;
	addr.point=hdr.origpoint;
	sprintf(str,"\7\1n\1hSBBSecho: \1m%.*s \1n\1msent you NetMail from \1h%s\1n\r\n"
		,FIDO_NAME_LEN-1
		,hdr.from
		,smb_faddrtoa(&addr,NULL));
	putsmsg(&scfg,usernumber,str);

	if(hdr.attr&FIDO_FILE) {	/* File attachment */
		strcpy(subj,hdr.subj);
		tp=subj;
		while(1) {
			p=strchr(tp,' ');
			if(p) *p=0;
			sp=strrchr(tp,'/');              /* sp is slash pointer */
			if(!sp) sp=strrchr(tp,'\\');
			if(sp) tp=sp+1;
			sprintf(str,"%s%s",secure ? cfg.secure : cfg.inbound,tp);
			sprintf(tmp,"%sfile/%04u.in",scfg.data_dir,usernumber);
			MKDIR(tmp);
			backslash(tmp);
			strcat(tmp,tp);
			mv(str,tmp,0);
			if(!p)
				break;
			tp=p+1; 
		} 
	}
	netmail++;

	FREE_AND_NULL(fmsgbuf);

	/***************************/
	/* Updating message header */
	/***************************/
	/***	NOT packet compatible
	if(!(misc&DONT_SET_RECV)) {
		hdr.attr|=FIDO_RECV;
		fseek(fidomsg,0L,SEEK_SET);
		fwrite(&hdr,sizeof(fmsghdr_t),1,fidomsg); }
	***/
	if(cfg.log&LOG_IMPORTED)
		lprintf(LOG_INFO,"%s Imported",info);
	return(0);
}

static uint32_t read_export_ptr(int subnum, const char* tag)
{
	char		path[MAX_PATH+1];
	char		key[INI_MAX_VALUE_LEN];
	int			file;
	FILE*		fp;
	uint32_t	ptr=0;

	/* New way (July-21-2012): */
	safe_snprintf(path,sizeof(path),"%s%s.ini",scfg.sub[subnum]->data_dir,scfg.sub[subnum]->code);
	if((fp=iniOpenFile(path, /* create: */FALSE)) != NULL) {
		safe_snprintf(key, sizeof(key), "%s.export_ptr", tag);
		if((ptr=iniReadLongInt(fp, "SBBSecho", key, 0)) == 0)
			ptr=iniReadLongInt(fp, "SBBSecho", "export_ptr", 0);	/* the previous .ini method (did not support gating) */
		iniCloseFile(fp);
	}
	if(ptr)	return ptr;
	/* Old way: */
	safe_snprintf(path,sizeof(path),"%s%s.sfp",scfg.sub[subnum]->data_dir,scfg.sub[subnum]->code);
	if((file=nopen(path,O_RDONLY)) != -1) {
		read(file,&ptr,sizeof(ptr));
		close(file); 
	}
	return ptr;
}

static void write_export_ptr(int subnum, uint32_t ptr, const char* tag)
{
	char		path[MAX_PATH+1];
	char		key[INI_MAX_VALUE_LEN];
	FILE*		fp;
	str_list_t	ini_file;

	/* New way (July-21-2012): */
	safe_snprintf(path,sizeof(path),"%s%s.ini",scfg.sub[subnum]->data_dir,scfg.sub[subnum]->code);
	if((fp=iniOpenFile(path, /* create: */TRUE)) != NULL) {
		safe_snprintf(key, sizeof(key), "%s.export_ptr", tag);
		ini_file = iniReadFile(fp);
		iniSetLongInt(&ini_file, "SBBSecho", key, ptr, /* style (default): */NULL);
		iniSetLongInt(&ini_file, "SBBSecho", "export_ptr", ptr, /* style (default): */NULL);
		iniWriteFile(fp, ini_file);
		iniCloseFile(fp);
		strListFree(&ini_file);
	}
}

/******************************************************************************
 This is where we export echomail.	This was separated from function main so
 it could be used for the remote rescan function.  Passing anything but an
 empty address as 'addr' designates that a rescan should be done for that
 address.
******************************************************************************/
void export_echomail(char *sub_code,faddr_t addr)
{
	char	str[1025],tear,cr;
	char	msgid[256];
	char*	buf=NULL;
	char*	minus;
	char*	fmsgbuf=NULL;
	ulong	fmsgbuflen;
	int		tzone;
	int		area;
	int		i,j,k=0;
	ulong	f,l,m,exp,exported=0;
	uint32_t ptr,msgs,lastmsg,posts;
	float	export_time;
	smbmsg_t msg;
	smbmsg_t orig_msg;
	fmsghdr_t hdr;
	struct	tm *tm;
	faddr_t pkt_faddr;
	post_t *post;
	areasbbs_t fakearea;
	addrlist_t msg_seen,msg_path;
    clock_t start_tick=0,export_ticks=0;
	time_t	tt;

	memset(&msg_seen,0,sizeof(addrlist_t));
	memset(&msg_path,0,sizeof(addrlist_t));
	memset(&fakearea,0,sizeof(areasbbs_t));
	memset(&pkt_faddr,0,sizeof(faddr_t));
	memset(&hdr,0,sizeof(hdr));
	start_tick=0;

	lprintf(LOG_DEBUG,"\nScanning for Outbound EchoMail...");

	for(area=0; area<cfg.areas; area++) {
		const char* tag=cfg.area[area].name;
		if(area==cfg.badecho)		/* Don't scan the bad-echo area */
			continue;
		if(!cfg.area[area].uplinks)
			continue;
		i=cfg.area[area].sub;
		if(i<0 || i>=scfg.total_subs)	/* Don't scan pass-through areas */
			continue;
		if(addr.zone) { 		/* Skip areas not meant for this address */
			if(!area_is_linked(area,&addr))
				continue; 
		}
		if(sub_code[0] && stricmp(sub_code,scfg.sub[i]->code))
			continue;
		lprintf(LOG_DEBUG,"\nScanning %-*.*s -> %s"
			,LEN_EXTCODE,LEN_EXTCODE,scfg.sub[i]->code
			,tag);
		ptr=0;
		if(!addr.zone && !(misc&IGNORE_MSGPTRS))
			ptr=read_export_ptr(i, tag);

		msgs=getlastmsg(i,&lastmsg,0);
		if(msgs<1 || (!addr.zone && !(misc&IGNORE_MSGPTRS) && ptr>=lastmsg)) {
			lprintf(LOG_DEBUG,"No new messages.");
			if(msgs>=0 && ptr>lastmsg && !addr.zone && !(misc&LEAVE_MSGPTRS)) {
				lprintf(LOG_DEBUG,"Fixing new-scan pointer (%u, lastmsg=%u).", ptr, lastmsg);
				write_export_ptr(i, lastmsg, tag);
			}
			continue; 
		}

		sprintf(smb[cur_smb].file,"%s%s"
			,scfg.sub[i]->data_dir,scfg.sub[i]->code);
		smb[cur_smb].retry_time=scfg.smb_retry_time;
		if((j=smb_open(&smb[cur_smb]))!=SMB_SUCCESS) {
			lprintf(LOG_ERR,"ERROR %d (%s) line %d opening %s",j,smb[cur_smb].last_error,__LINE__
				,smb[cur_smb].file);
			continue; 
		}

		post=NULL;
		posts=loadmsgs(&post,ptr);

		if(!posts)	{ /* no new messages */
			smb_close(&smb[cur_smb]);
			FREE_AND_NULL(post);
			continue; 
		}

		if(start_tick)
			export_ticks+=msclock()-start_tick;
		start_tick=msclock();

		for(m=exp=0;m<posts;m++) {
			printf("\r%8s %5lu of %-5"PRIu32"  "
				,scfg.sub[i]->code,m+1,posts);
			memset(&msg,0,sizeof(msg));
			msg.idx=post[m].idx;
			if((k=smb_lockmsghdr(&smb[cur_smb],&msg))!=SMB_SUCCESS) {
				lprintf(LOG_ERR,"ERROR %d (%s) line %d locking %s msghdr"
					,k,smb[cur_smb].last_error,__LINE__,smb[cur_smb].file);
				continue; 
			}
			k=smb_getmsghdr(&smb[cur_smb],&msg);
			if(k || msg.hdr.number!=post[m].idx.number) {
				smb_unlockmsghdr(&smb[cur_smb],&msg);
				smb_freemsgmem(&msg);

				msg.hdr.number=post[m].idx.number;
				if((k=smb_getmsgidx(&smb[cur_smb],&msg))!=SMB_SUCCESS) {
					lprintf(LOG_ERR,"ERROR %d line %d reading %s index",k,__LINE__
						,smb[cur_smb].file);
					continue; 
				}
				if((k=smb_lockmsghdr(&smb[cur_smb],&msg))!=SMB_SUCCESS) {
					lprintf(LOG_ERR,"ERROR %d line %d locking %s msghdr",k,__LINE__
						,smb[cur_smb].file);
					continue; 
				}
				if((k=smb_getmsghdr(&smb[cur_smb],&msg))!=SMB_SUCCESS) {
					smb_unlockmsghdr(&smb[cur_smb],&msg);
					lprintf(LOG_ERR,"ERROR %d line %d reading %s msghdr",k,__LINE__
						,smb[cur_smb].file);
					continue; 
				} 
			}

			if((!addr.zone && !(misc&EXPORT_ALL)
				&& (msg.from_net.type==NET_FIDO || msg.from_net.type==NET_FIDO_ASCII))
				|| !strnicmp(msg.subj,"NE:",3)) {   /* no echo */
				smb_unlockmsghdr(&smb[cur_smb],&msg);
				smb_freemsgmem(&msg);
				continue;   /* From a Fido node, ignore it */
			}

			if(msg.from_net.type!=NET_NONE 
				&& msg.from_net.type!=NET_FIDO
				&& msg.from_net.type!=NET_FIDO_ASCII
				&& !(scfg.sub[i]->misc&SUB_GATE)) {
				smb_unlockmsghdr(&smb[cur_smb],&msg);
				smb_freemsgmem(&msg);
				continue; 
			}

			memset(&hdr,0,sizeof(fmsghdr_t));	 /* Zero the header */
			hdr.origzone=scfg.sub[i]->faddr.zone;
			hdr.orignet=scfg.sub[i]->faddr.net;
			hdr.orignode=scfg.sub[i]->faddr.node;
			hdr.origpoint=scfg.sub[i]->faddr.point;

			hdr.attr=FIDO_LOCAL;
			if(msg.hdr.attr&MSG_PRIVATE)
				hdr.attr|=FIDO_PRIVATE;

			SAFECOPY(hdr.from,msg.from);

			tt=msg.hdr.when_written.time;
			if((tm=localtime(&tt)) != NULL)
				sprintf(hdr.time,"%02u %3.3s %02u  %02u:%02u:%02u"
					,tm->tm_mday,mon[tm->tm_mon],TM_YEAR(tm->tm_year)
					,tm->tm_hour,tm->tm_min,tm->tm_sec);

			SAFECOPY(hdr.to,msg.to);

			SAFECOPY(hdr.subj,msg.subj);

			buf=smb_getmsgtxt(&smb[cur_smb],&msg,GETMSGTXT_ALL);
			if(!buf) {
				smb_unlockmsghdr(&smb[cur_smb],&msg);
				smb_freemsgmem(&msg);
				continue; 
			}
			fmsgbuflen=strlen((char *)buf)+4096; /* over alloc for kludge lines */
			fmsgbuf=malloc(fmsgbuflen);
			if(!fmsgbuf) {
				lprintf(LOG_ERR,"ERROR line %d allocating %lu bytes for fmsgbuf"
					,__LINE__,fmsgbuflen);
				smb_unlockmsghdr(&smb[cur_smb],&msg);
				smb_freemsgmem(&msg);
				continue; 
			}
			fmsgbuflen-=1024;	/* give us a bit of a guard band here */

			tear=0;
			f=0;

			tzone=smb_tzutc(msg.hdr.when_written.zone);
			if(tzone<0) {
				minus="-";
				tzone=-tzone;
			} else
				minus="";
			f+=sprintf(fmsgbuf+f,"\1TZUTC: %s%02d%02u\r"		/* TZUTC (FSP-1001) */
				,minus,tzone/60,tzone%60);

			if(msg.ftn_flags!=NULL)
				f+=sprintf(fmsgbuf+f,"\1FLAGS %.256s\r", msg.ftn_flags);

			f+=sprintf(fmsgbuf+f,"\1MSGID: %.256s\r"
				,ftn_msgid(scfg.sub[i],&msg,msgid,sizeof(msgid)));

			if(msg.ftn_reply!=NULL)			/* use original REPLYID */
				f+=sprintf(fmsgbuf+f,"\1REPLY: %.256s\r", msg.ftn_reply);
			else if(msg.hdr.thread_back) {	/* generate REPLYID (from original message's MSG-ID, if it had one) */
				memset(&orig_msg,0,sizeof(orig_msg));
				orig_msg.hdr.number=msg.hdr.thread_back;
				if(smb_getmsgidx(&smb[cur_smb], &orig_msg))
					f+=sprintf(fmsgbuf+f,"\1REPLY: <%s>\r",smb[cur_smb].last_error);
				else {
					smb_lockmsghdr(&smb[cur_smb],&orig_msg);
					smb_getmsghdr(&smb[cur_smb],&orig_msg);
					smb_unlockmsghdr(&smb[cur_smb],&orig_msg);
					if(orig_msg.ftn_msgid != NULL && orig_msg.ftn_msgid[0])
						f+=sprintf(fmsgbuf+f,"\1REPLY: %.256s\r",orig_msg.ftn_msgid);	
				}
			}
			if(msg.ftn_pid!=NULL)	/* use original PID */
				f+=sprintf(fmsgbuf+f,"\1PID: %.256s\r", msg.ftn_pid);
			if(msg.ftn_tid!=NULL)	/* use original TID */
				f+=sprintf(fmsgbuf+f,"\1TID: %.256s\r", msg.ftn_tid);
			else					/* generate TID */
				f+=sprintf(fmsgbuf+f,"\1TID: SBBSecho %u.%02u-%s r%s %s %s\r"
					,SBBSECHO_VERSION_MAJOR,SBBSECHO_VERSION_MINOR,PLATFORM_DESC,revision,__DATE__,compiler);

			/* Unknown kludge lines are added here */
			for(l=0;l<msg.total_hfields && f<fmsgbuflen;l++)
				if(msg.hfield[l].type == FIDOCTRL)
					f+=sprintf(fmsgbuf+f,"\1%.512s\r",(char*)msg.hfield_dat[l]);

			for(l=0,cr=1;buf[l] && f<fmsgbuflen;l++) {
				if(buf[l]==CTRL_A) { /* Ctrl-A, so skip it and the next char */
					char ch;
					l++;
					if(buf[l]==0 || toupper(buf[l])=='Z')	/* EOF */
						break;
					if((ch=ctrl_a_to_ascii_char(buf[l])) != 0)
						fmsgbuf[f++]=ch;
					continue; 
				}
					
				/* Need to support converting sole-LFs to Hard-CR and soft-CR (0x8D) as well */
				if((misc&STRIP_LF) && buf[l]=='\n')	/* Ignore line feeds */
					continue;

				if(cr) {
					char *tp = (char*)buf+l;
					/* Bugfixed: handle tear line detection/conversion and origin line detection/conversion even when line-feeds exist and aren't stripped */
					if(*tp == '\n')	
						tp++;
					if(*tp=='-' && *(tp+1)=='-'
						&& *(tp+2)=='-'
						&& (*(tp+3)==' ' || *(tp+3)=='\r')) {
						if(misc&CONVERT_TEAR)	/* Convert to === */
							*tp=*(tp+1)=*(tp+2)='=';
						else
							tear=1; 
					}
					else if(!strncmp(tp," * Origin: ",11))
						*(tp+1)='#'; 
				} /* Convert * Origin into # Origin */

				if(buf[l]=='\r')
					cr=1;
				else
					cr=0;
				if((scfg.sub[i]->misc&SUB_ASCII) || (misc&ASCII_ONLY)) {
					if(buf[l]<' ' && buf[l]>=0 && buf[l]!='\r'
						&& buf[l]!='\n')			/* Ctrl ascii */
						buf[l]='.';             /* converted to '.' */
					if((uchar)buf[l]&0x80)		/* extended ASCII */
						buf[l]=exascii_to_ascii_char(buf[l]);
				}

				fmsgbuf[f++]=buf[l]; 
			}

			FREE_AND_NULL(buf);
			fmsgbuf[f]=0;

			if(!(scfg.sub[i]->misc&SUB_NOTAG)) {
				if(!tear) {  /* No previous tear line */
					sprintf(str,"--- SBBSecho %u.%02u-%s\r"
						,SBBSECHO_VERSION_MAJOR,SBBSECHO_VERSION_MINOR,PLATFORM_DESC);
					strcat((char *)fmsgbuf,str); 
				}

				sprintf(str," * Origin: %s (%s)\r"
					,scfg.sub[i]->origline[0] ? scfg.sub[i]->origline : scfg.origline
					,smb_faddrtoa(&scfg.sub[i]->faddr,NULL));
				strcat((char *)fmsgbuf,str); 
			}

			for(k=0;k<cfg.areas;k++)
				if(cfg.area[k].sub==i) {
					cfg.area[k].exported++;
					pkt_to_pkt(fmsgbuf,cfg.area[k]
						,(addr.zone) ? addr:pkt_faddr,hdr,msg_seen
						,msg_path,(addr.zone) ? 2:0);
					break; 
				}
			FREE_AND_NULL(fmsgbuf);
			exported++;
			exp++;
			printf("Exp: %lu ",exp);
			smb_unlockmsghdr(&smb[cur_smb],&msg);
			smb_freemsgmem(&msg); 
		}

		smb_close(&smb[cur_smb]);
		FREE_AND_NULL(post);

		if(!addr.zone && !(misc&LEAVE_MSGPTRS) && lastmsg>ptr)
			write_export_ptr(i, lastmsg, tag);
	}

	printf("\n");
	if(start_tick)	/* Last possible increment of export_ticks */
		export_ticks+=msclock()-start_tick;

	pkt_to_pkt(buf,fakearea,pkt_faddr,hdr,msg_seen,msg_path,1);

	if(!addr.zone && cfg.log&LOG_AREA_TOTALS && exported)
		for(i=0;i<cfg.areas;i++)
			if(cfg.area[i].exported)
				lprintf(LOG_INFO,"Exported: %5u msgs %*s -> %s"
					,cfg.area[i].exported,LEN_EXTCODE,scfg.sub[cfg.area[i].sub]->code
					,cfg.area[i].name);

	export_time=((float)export_ticks)/(float)MSCLOCKS_PER_SEC;
	if(cfg.log&LOG_TOTALS && exported && export_time) {
		lprintf(LOG_INFO,"Exported: %5lu msgs in %.1f sec (%.1f/min %.1f/sec)"
			,exported,export_time
			,export_time/60.0 ? (float)exported/(export_time/60.0) :(float)exported
			,(float)exported/export_time);
	}
}

char* freadstr(FILE* fp, char* str, size_t maxlen)
{
	int		ch;
	size_t	rd=0;
	size_t	len=0;

	memset(str,0,maxlen);	/* pre-terminate */

	while(rd<maxlen && (ch=fgetc(fp))!=EOF) {
		if(ch==0)
			break;
		if((uchar)ch>=' ')	/* not a ctrl char (garbage?) */
			str[len++]=ch;
		rd++;
	}

	str[maxlen-1]=0;	/* Force terminator */

	return(str);
}

/***********************************/
/* Synchronet/FidoNet Message util */
/***********************************/
int main(int argc, char **argv)
{
	FILE*	fidomsg;
	char	packet[MAX_PATH+1];
	char	ch,str[1025],fname[MAX_PATH+1],path[MAX_PATH+1]
			,sub_code[LEN_EXTCODE+1]
			,*p,*tp
			,areatagstr[128],outbound[128]
			,password[16];
	char	*fmsgbuf=NULL;
	ushort	attr;
	int 	i,j,k,file,fmsg,node;
	BOOL	grunged;
	uint	subnum[MAX_OPEN_SMBS]={INVALID_SUB};
	ulong	echomail=0,m/* f, */,areatag;
	time_t	now;
	time_t	ftime;
	float	import_time;
	clock_t start_tick=0,import_ticks=0;
	struct	tm *tm;
	size_t	f;
	glob_t	g;
	size_t	offset;
	fmsghdr_t hdr;
	fpkdmsg_t pkdmsg;
	faddr_t addr,pkt_faddr;
	FILE	*stream;
	pkthdr_t pkthdr;
	two_plus_t* two_plus;
	addrlist_t msg_seen,msg_path;
	areasbbs_t fakearea,curarea;
	char *usage="\n"
	"usage: sbbsecho [cfg_file] [-switches] [sub_code] [address]\n"
	"\n"
	"where: cfg_file is the filename of config file (default is ctrl/sbbsecho.cfg)\n"
	"       sub_code is the internal code for a sub-board (default is ALL subs)\n"
	"       address is the link to export for (default is ALL links)\n"
	"\n"
	"valid switches:\n"
	"\n"
	"p: do not import packets                 x: do not delete packets after import\n"
	"n: do not import netmail                 d: do not delete netmail after import\n"
	"i: do not import echomail                e: do not export echomail\n"
	"m: ignore echomail ptrs (export all)     u: update echomail ptrs (export none)\n"
	"j: ignore recieved bit on netmail        t: do not update echomail ptrs\n"
	"l: create log file (data/sbbsecho.log)   r: create report of import totals\n"
	"h: export all echomail (hub rescan)      b: import locally created netmail too\n"
	"a: export ASCII characters only          f: create packets for outbound netmail\n"
	"g: generate notify lists                 =: change existing tear lines to ===\n"
	"y: import netmail for unknown users to sysop\n"
	"o: import all netmail regardless of destination address\n"
	"s: import private echomail override (strip private status)\n"
	"!: notify users of received echomail     @: prompt for key upon exiting (debug)\n"
	"                                         W: prompt for key upon abnormal exit\n";

	if((email=(smb_t *)malloc(sizeof(smb_t)))==NULL) {
		printf("ERROR allocating memory for email.\n");
		bail(1); 
		return -1;
	}
	memset(email,0,sizeof(smb_t));
	if((smb=(smb_t *)malloc(MAX_OPEN_SMBS*sizeof(smb_t)))==NULL) {
		printf("ERROR allocating memory for smbs.\n");
		bail(1); 
		return -1;
	}
	for(i=0;i<MAX_OPEN_SMBS;i++)
		memset(&smb[i],0,sizeof(smb_t));
	memset(&addr,0,sizeof(addr));
	memset(&cfg,0,sizeof(config_t));
	memset(&hdr,0,sizeof(hdr));
	memset(&pkt_faddr,0,sizeof(pkt_faddr));
	memset(&msg_seen,0,sizeof(addrlist_t));
	memset(&msg_path,0,sizeof(addrlist_t));
	memset(&fakearea,0,sizeof(areasbbs_t));

	sscanf("$Revision: 1.261 $", "%*s %s", revision);

	DESCRIBE_COMPILER(compiler);

	printf("\nSBBSecho v%u.%02u-%s (rev %s) - Synchronet FidoNet Packet "
		"Tosser\n"
		,SBBSECHO_VERSION_MAJOR, SBBSECHO_VERSION_MINOR
		,PLATFORM_DESC
		,revision
		);

	putenv("TMP=");
	setvbuf(stdout,NULL,_IONBF,0);

	sub_code[0]=0;

	for(i=1;i<argc;i++) {
		if(argv[i][0]=='-'
#if !defined(__unix__)
			|| argv[i][0]=='/'
#endif
			) {
			j=1;
			while(argv[i][j]) {
				switch(toupper(argv[i][j])) {
					case 'A':
						misc|=ASCII_ONLY;
						break;
					case 'B':
						misc|=LOCAL_NETMAIL;
						break;
					case 'D':
						misc&=~DELETE_NETMAIL;
						break;
					case 'E':
						misc&=~EXPORT_ECHOMAIL;
						break;
					case 'F':
						misc|=PACK_NETMAIL;
						break;
					case 'G':
						misc|=GEN_NOTIFY_LIST;
						break;
					case 'H':
						misc|=EXPORT_ALL;
						break;
					case 'I':
						misc&=~IMPORT_ECHOMAIL;
						break;
					case 'J':
						misc|=IGNORE_RECV;
						break;
					case 'L':
						misc|=LOGFILE;
						break;
					case 'M':
						misc|=IGNORE_MSGPTRS;
						break;
					case 'N':
						misc&=~IMPORT_NETMAIL;
						break;
					case 'O':
						misc|=IGNORE_ADDRESS;
						break;
					case 'P':
						misc&=~IMPORT_PACKETS;
						break;
					case 'R':
						misc|=REPORT;
						break;
					case 'S':
						misc|=IMPORT_PRIVATE;
						break;
					case 'T':
						misc|=LEAVE_MSGPTRS;
						break;
					case 'U':
						misc|=UPDATE_MSGPTRS;
						misc&=~EXPORT_ECHOMAIL;
						break;
					case 'X':
						misc&=~DELETE_PACKETS;
						break;
					case 'Y':
						misc|=UNKNOWN_NETMAIL;
						break;
					case '=':
						misc|=CONVERT_TEAR;
						break;
					case '!':
						misc|=NOTIFY_RECEIPT;
						break;
					case '@':
						pause_on_exit=TRUE;
						break;
					case 'W':
						pause_on_abend=TRUE;
						break;
					case 'Q':
						bail(0);
					default:
						printf(usage);
						bail(0); 
				}
				j++; 
			} 
		}
		else {
			if(strchr(argv[i],'\\') || strchr(argv[i],'/') 
				|| argv[i][1]==':' || strchr(argv[i],'.'))
				SAFECOPY(cfg.cfgfile,argv[i]);
			else if(isdigit((uchar)argv[i][0]))
				addr=atofaddr(argv[i]);
			else
				SAFECOPY(sub_code,argv[i]); 
		}  
	}

	if(!(misc&(IMPORT_NETMAIL|IMPORT_ECHOMAIL)))
		misc&=~IMPORT_PACKETS;

	p=getenv("SBBSCTRL");
	if(p==NULL) {
		printf("\7\nSBBSCTRL environment variable not set.\n");
		bail(1); 
		return -1;
	}
	SAFECOPY(scfg.ctrl_dir,p); 

	if(chdir(scfg.ctrl_dir)!=0)
		printf("!ERROR changing directory to: %s", scfg.ctrl_dir);

    printf("\nLoading configuration files from %s\n", scfg.ctrl_dir);
	scfg.size=sizeof(scfg);
	SAFECOPY(str,UNKNOWN_LOAD_ERROR);
	if(!load_cfg(&scfg, NULL, TRUE, str)) {
		fprintf(stderr,"!ERROR %s\n",str);
		fprintf(stderr,"!Failed to load configuration files\n");
		bail(1);
		return -1;
	}

	sprintf(str,"%stwitlist.cfg",scfg.ctrl_dir);
	twit_list=fexist(str);

	if(scfg.total_faddrs)
		sys_faddr=scfg.faddr[0];

	if(!cfg.cfgfile[0])
		sprintf(cfg.cfgfile,"%ssbbsecho.cfg",scfg.ctrl_dir);
	strcpy(cfg.inbound,scfg.fidofile_dir);
	sprintf(cfg.areafile,"%sareas.bbs",scfg.data_dir);
	sprintf(cfg.logfile,"%ssbbsecho.log",scfg.logs_dir);

	read_echo_cfg();

	if(misc&LOGFILE)
		if((fidologfile=fopen(cfg.logfile,"a"))==NULL) {
			fprintf(stderr,"ERROR %u (%s) line %d opening %s\n",errno,strerror(errno),__LINE__,cfg.logfile);
			bail(1); 
			return -1;
		}

	/******* READ IN AREAS.BBS FILE *********/

	printf("Reading %s",cfg.areafile);
	if((stream=fopen(cfg.areafile,"r"))==NULL) {
		lprintf(LOG_ERR,"ERROR %u (%s) line %d opening %s",errno,strerror(errno),__LINE__,cfg.areafile);
		bail(1); 
		return -1;
	}
	cfg.areas=0;		/* Total number of areas in AREAS.BBS */
	cfg.area=NULL;
	while(1) {
		if(!fgets(str,sizeof(str),stream))
			break;
		truncsp(str);
		p=str;
		SKIP_WHITESPACE(p);	/* Find first printable char */
		if(*p==';' || !*p)          /* Ignore blank lines or start with ; */
			continue;
		if((cfg.area=(areasbbs_t *)realloc(cfg.area,sizeof(areasbbs_t)*
			(cfg.areas+1)))==NULL) {
			lprintf(LOG_ERR,"ERROR allocating memory for area #%u.",cfg.areas+1);
			bail(1); 
			return -1;
		}
		memset(&cfg.area[cfg.areas],0,sizeof(areasbbs_t));

		cfg.area[cfg.areas].sub=INVALID_SUB;	/* Default to passthru */

		sprintf(tmp,"%-.*s",LEN_EXTCODE,p);
		tp=tmp;
		FIND_WHITESPACE(tp);
		*tp=0;
		for(i=0;i<scfg.total_subs;i++)
			if(!stricmp(tmp,scfg.sub[i]->code))
				break;
		if(i<scfg.total_subs)
			cfg.area[cfg.areas].sub=i;
		else if(stricmp(tmp,"P")) {
			lprintf(LOG_WARNING,"%s: Unrecognized internal code, assumed passthru",tmp); 
		}

		FIND_WHITESPACE(p);				/* Skip code */
		SKIP_WHITESPACE(p);				/* Skip white space */
		SAFECOPY(tmp,p);		       /* Area tag */
		truncstr(tmp,"\t ");
		strupr(tmp);
		if(tmp[0]=='*')         /* UNKNOWN-ECHO area */
			cfg.badecho=cfg.areas;
		if((cfg.area[cfg.areas].name=strdup(tmp))==NULL) {
			lprintf(LOG_ERR,"ERROR allocating memory for area #%u tag name."
				,cfg.areas+1);
			bail(1); 
			return -1;
		}
		cfg.area[cfg.areas].tag=crc32(tmp,0);

		FIND_WHITESPACE(p);		/* Skip tag */
		SKIP_WHITESPACE(p);		/* Skip white space */

		while(*p && *p!=';') {
			if((cfg.area[cfg.areas].uplink=(faddr_t *)
				realloc(cfg.area[cfg.areas].uplink
				,sizeof(faddr_t)*(cfg.area[cfg.areas].uplinks+1)))==NULL) {
				lprintf(LOG_ERR,"ERROR allocating memory for area #%u uplinks."
					,cfg.areas+1);
				bail(1); 
				return -1;
			}
			cfg.area[cfg.areas].uplink[cfg.area[cfg.areas].uplinks]=atofaddr(p);
			FIND_WHITESPACE(p);	/* Skip address */
			SKIP_WHITESPACE(p);	/* Skip white space */
			cfg.area[cfg.areas].uplinks++; 
		}

		if(cfg.area[cfg.areas].sub!=INVALID_SUB || cfg.area[cfg.areas].uplinks)
			cfg.areas++;		/* Don't allocate if no tossing */
		}
	fclose(stream);

	printf("\n");

	if(!cfg.areas) {
		lprintf(LOG_ERR,"No areas defined in %s", cfg.areafile);
		bail(1); 
		return -1;
	}

	#if 0	/* AREAS.BBS DEBUG */
		for(i=0;i<cfg.areas;i++) {
			printf("%4u: %-8s"
				,i+1
				,cfg.area[i].sub==INVALID_SUB ? "Passthru" :
				scfg.sub[cfg.area[i].sub]->code);
			for(j=0;j<cfg.area[i].uplinks;j++)
				printf(" %s",smb_faddrtoa(&cfg.area[i].uplink[j],NULL));
			printf("\n"); }
	#endif

	if(misc&GEN_NOTIFY_LIST) {
		lprintf(LOG_DEBUG,"\nGenerating Notify Lists...");
		notify_list(); 
	}

	/* Find any packets that have been left behind in the OUTBOUND directory */
	now=time(NULL);
	lprintf(LOG_DEBUG,"\nScanning for Stray Outbound Packets...");
	sprintf(path,"%s*.pk_",cfg.outbound);
	glob(path,0,NULL,&g);
	for(f=0;f<g.gl_pathc && !kbhit();f++) {

		SAFECOPY(packet,(char*)g.gl_pathv[f]);

		lprintf(LOG_DEBUG,"%21s: %s ","Outbound Packet",packet);
		if((fmsg=sopen(packet,O_RDWR|O_BINARY,SH_DENYRW))==-1) {
			lprintf(LOG_ERR,"ERROR %u (%s) line %d opening %s",errno,strerror(errno),__LINE__,packet);
			continue; 
		}
		if((fidomsg=fdopen(fmsg,"r+b"))==NULL) {
			close(fmsg);
			lprintf(LOG_ERR,"ERROR %u (%s) line %d fdopening %s",errno,strerror(errno),__LINE__,packet);
			continue; 
		}
		if(filelength(fmsg)<sizeof(pkthdr_t)) {
			lprintf(LOG_ERR,"ERROR line %d invalid length of %lu bytes for %s"
				,__LINE__,filelength(fmsg),packet);
			fclose(fidomsg);
			if(delfile(packet))
				lprintf(LOG_ERR,"ERROR line %d removing %s %s",__LINE__,packet
					,strerror(errno));
			continue; 
		}
		if(fread(&pkthdr,sizeof(pkthdr_t),1,fidomsg)!=1) {
			fclose(fidomsg);
			lprintf(LOG_ERR,"ERROR line %d reading %u bytes from %s",__LINE__
				,sizeof(pkthdr_t),packet);
			continue; 
		}
		ftime=fdate(packet);
		if((ftime+(60L*60L))<=now) {
			fseek(fidomsg,-3L,SEEK_END);
			fread(str,3,1,fidomsg);
			if(str[2])						/* No ending NULL, probably junk */
				fputc(FIDO_PACKED_MSG_TERMINATOR,fidomsg);
			if(str[1])
				fputc(FIDO_PACKED_MSG_TERMINATOR,fidomsg);
			if(str[0])
				fputc(FIDO_PACKED_MSG_TERMINATOR,fidomsg);
			fclose(fidomsg);
			pkt_faddr.zone=pkthdr.destzone;
			pkt_faddr.net=pkthdr.destnet;
			pkt_faddr.node=pkthdr.destnode;
			pkt_faddr.point=0;				/* No point info in the 2.0 hdr! */
			two_plus = (two_plus_t*)&pkthdr.empty;
			if(two_plus->cword==_rotr(two_plus->cwcopy,8)  /* 2+ Packet Header */
				&& two_plus->cword && (two_plus->cword&1))
				pkt_faddr.point=two_plus->destpoint;
			else if(pkthdr.baud==2) 				/* Type 2.2 Packet Header */
				pkt_faddr.point=pkthdr.month; 
			lprintf(LOG_DEBUG,"Sending to %s",smb_faddrtoa(&pkt_faddr,NULL));
			pack_bundle(packet,pkt_faddr); 
		}
		else {
			fclose(fidomsg);
			lprintf(LOG_WARNING,"Stray Outbound Packet (%s) possibly still in use (ftime: %.24s)"
				,packet,ctime(&ftime)); 
		} 
	}
	globfree(&g);

	if(misc&IMPORT_PACKETS) {

		lprintf(LOG_DEBUG,"\nScanning for Inbound Packets...");

		/* We want to loop while there are bundles waiting for us, but first we want */
		/* to take care of any packets that may already be hanging around for some	 */
		/* reason or another (thus the do/while loop) */

		echomail=0;
		for(secure=0;secure<2;secure++) {
			if(secure && !cfg.secure[0])
				break;
		do {

		/****** START OF IMPORT PKT ROUTINE ******/

		offset=strlen(secure ? cfg.secure : cfg.inbound);
#ifdef __unix__
		sprintf(path,"%s*.[Pp][Kk][Tt]",secure ? cfg.secure : cfg.inbound);
#else
		sprintf(path,"%s*.pkt",secure ? cfg.secure : cfg.inbound);
#endif
		glob(path,0,NULL,&g);
		for(f=0;f<g.gl_pathc && !kbhit();f++) {

			SAFECOPY(packet,g.gl_pathv[f]);

			if((fidomsg=fnopen(&fmsg,packet,O_RDWR))==NULL) {
				lprintf(LOG_ERR,"ERROR %u (%s) line %d opening %s",errno,strerror(errno),__LINE__,packet);
				continue; 
			}
			if(filelength(fmsg)<sizeof(pkthdr_t)) {
				lprintf(LOG_WARNING,"Invalid length of %s: %lu bytes"
					,packet,filelength(fmsg));
				fclose(fidomsg);
				continue; 
			}

			fseek(fidomsg,-2L,SEEK_END);
			fread(str,2,1,fidomsg);
			if((str[0] || str[1]) &&
				(fdate(packet)+(48L*60L*60L))<=time(NULL)) {
				fclose(fidomsg);
				lprintf(LOG_WARNING,"WARNING line %d packet %s not terminated correctly",__LINE__
					,packet);
				continue; 
			}
			fseek(fidomsg,0L,SEEK_SET);
			if(fread(&pkthdr,sizeof(pkthdr_t),1,fidomsg)!=1) {
				fclose(fidomsg);
				lprintf(LOG_ERR,"ERROR line %d reading %u bytes from %s",__LINE__
					,sizeof(pkthdr_t),packet);
				continue; 
			}

			if(pkthdr.pkttype != 2) {
				fclose(fidomsg);
				lprintf(LOG_WARNING,"%s is not a type 2 packet (type=%u)"
					,packet, pkthdr.pkttype);
				continue; 
			}

			pkt_faddr.zone=pkthdr.origzone ? pkthdr.origzone:sys_faddr.zone;
			pkt_faddr.net=pkthdr.orignet;
			pkt_faddr.node=pkthdr.orignode;
			pkt_faddr.point=0;

			printf("%21s: %s "
				,secure ? "Importing Secure Pkt" : "Importing Packet",packet+offset);
			two_plus = (two_plus_t*)&pkthdr.empty;
			if(two_plus->cword==_rotr(two_plus->cwcopy,8)  /* 2+ Packet Header (see FSC-39 and FSC-48 for explanation of this insanity) */
				&& (two_plus->cword&1)) {
				pkt_type=PKT_TWO_PLUS;
				pkt_faddr.point=two_plus->origpoint;
				if(pkt_faddr.point != 0 && pkthdr.orignet == -1)
					pkt_faddr.net=two_plus->auxnet ? two_plus->auxnet:sys_faddr.net;
				printf("(Type 2+)");
				if(cfg.log&LOG_PACKETS)
					logprintf("Importing %s%s (Type 2+) from %s"
						,secure ? "(secure) ":"",packet+offset,smb_faddrtoa(&pkt_faddr,NULL)); 
			}
			else if(pkthdr.baud==2) {				/* Type 2.2 Packet Header (FSC-45) */
				pkt_type=PKT_TWO_TWO;
				pkt_faddr.point=pkthdr.year ? pkthdr.year:0;
				printf("(Type 2.2)");
				if(cfg.log&LOG_PACKETS)
					logprintf("Importing %s%s (Type 2.2) from %s"
						,secure ? "(secure) ":"",packet+offset,smb_faddrtoa(&pkt_faddr,NULL)); 
			}
			else {	/* Type 2.0, FTS-1 */
				pkt_type=PKT_TWO;
				printf("(Type 2)");
				if(cfg.log&LOG_PACKETS)
					logprintf("Importing %s%s (Type 2) from %s"
						,secure ? "(secure) ":"",packet+offset,smb_faddrtoa(&pkt_faddr,NULL)); 
			}

			printf(" from %s\n",smb_faddrtoa(&pkt_faddr,NULL));

			if(misc&SECURE) {
				k=matchnode(pkt_faddr,1);
				sprintf(password,"%.*s",FIDO_PASS_LEN,pkthdr.password);
				if(k<cfg.nodecfgs && cfg.nodecfg[k].pktpwd[0] &&
					stricmp(password,cfg.nodecfg[k].pktpwd)) {
					sprintf(str,"Packet %s from %s - "
						"Incorrect password ('%s' instead of '%s')"
						,packet+offset,smb_faddrtoa(&pkt_faddr,NULL)
						,password,cfg.nodecfg[k].pktpwd);
					printf("Security Violation (Incorrect Password)\n");
					if(cfg.log&LOG_SECURE)
						logprintf(str);
					fclose(fidomsg);
					continue; 
				} 
			}

			while(!feof(fidomsg)) {

				memset(&hdr,0,sizeof(fmsghdr_t));

				/* Sept-16-2013: copy the origin zone from the packet header
				   as packed message headers don't have the zone information */
				hdr.origzone=pkt_faddr.zone;

				if(start_tick)
					import_ticks+=msclock()-start_tick;
				start_tick=msclock();

				FREE_AND_NULL(fmsgbuf);

				grunged=FALSE;

				/* Read fixed-length header fields */
				if(fread(&pkdmsg,sizeof(BYTE),sizeof(pkdmsg),fidomsg)!=sizeof(pkdmsg))
					continue;
				
				if(pkdmsg.type==2) { /* Recognized type, copy fields */
					hdr.orignode = pkdmsg.orignode;
					hdr.destnode = pkdmsg.destnode;
					hdr.orignet = pkdmsg.orignet;
					hdr.destnet = pkdmsg.destnet;
					hdr.attr = pkdmsg.attr;
					hdr.cost = pkdmsg.cost;
					SAFECOPY(hdr.time,pkdmsg.time);
				} else
					grunged=TRUE;

				/* Read variable-length header fields */
				if(!grunged) {
					freadstr(fidomsg,hdr.to,sizeof(hdr.to));
					freadstr(fidomsg,hdr.from,sizeof(hdr.from));
					freadstr(fidomsg,hdr.subj,sizeof(hdr.subj));
				}
				hdr.attr&=~FIDO_LOCAL;	/* Strip local bit, obviously not created locally */

				str[0]=0;
				for(i=0;!grunged && i<sizeof(str);i++)	/* Read in the 'AREA' Field */
					if(!fread(str+i,1,1,fidomsg) || str[i]=='\r')
						break;
				if(i<sizeof(str))
					str[i]=0;
				else
					grunged=1;

				if(grunged) {
					start_tick=0;
					if(cfg.log&LOG_GRUNGED)
						logprintf("Grunged message");
					seektonull(fidomsg);
					printf("Grunged message!\n");
					continue; 
				}

				if(i)
					fseek(fidomsg,(long)-(i+1),SEEK_CUR);

				truncsp(str);
				p=strstr(str,"AREA:");
				if(p!=str) {					/* Netmail */
					long pos = ftell(fidomsg);
					start_tick=0;
					import_netmail("", hdr, fidomsg);
					fseek(fidomsg, pos, SEEK_SET);
					seektonull(fidomsg);
					printf("\n");
					continue; 
				}

				if(!(misc&IMPORT_ECHOMAIL)) {
					start_tick=0;
					printf("EchoMail Ignored");
					seektonull(fidomsg);
					printf("\n");
					continue; 
				}

				p+=5;								/* Skip "AREA:" */
				SKIP_WHITESPACE(p);					/* Skip any white space */
				printf("%21s: ",p);                 /* Show areaname: */
				SAFECOPY(areatagstr,p);
				strupr(p);
				areatag=crc32(p,0);

				for(i=0;i<cfg.areas;i++)				/* Do we carry this area? */
					if(cfg.area[i].tag==areatag) {
						if(cfg.area[i].sub!=INVALID_SUB)
							printf("%s ",scfg.sub[cfg.area[i].sub]->code);
						else
							printf("(Passthru) ");
						fmsgbuf=getfmsg(fidomsg,NULL);
						gen_psb(&msg_seen,&msg_path,fmsgbuf,pkthdr.origzone);	/* was destzone */
						break; 
					}

				if(i==cfg.areas) {
					printf("(Unknown) ");
					if(cfg.badecho>=0) {
						i=cfg.badecho;
						if(cfg.area[i].sub!=INVALID_SUB)
							printf("%s ",scfg.sub[cfg.area[i].sub]->code);
						else
							printf("(Passthru) ");
						fmsgbuf=getfmsg(fidomsg,NULL);
						gen_psb(&msg_seen,&msg_path,fmsgbuf,pkthdr.origzone);	/* was destzone */
					}
					else {
						start_tick=0;
						printf("Skipped\n");
						seektonull(fidomsg);
						continue; 
					} 
				}

				if((misc&SECURE) && cfg.area[i].sub!=INVALID_SUB) {
					if(!area_is_linked(i,&pkt_faddr)) {
						if(cfg.log&LOG_SECURE)
							logprintf("%s: Security violation - %s not in AREAS.BBS"
								,areatagstr,smb_faddrtoa(&pkt_faddr,NULL));
						printf("Security Violation (Not in AREAS.BBS)\n");
						seektonull(fidomsg);
						continue; 
					} 
				}

				/* From here on out, i = area number and area[i].sub = sub number */

				memcpy(&curarea,&cfg.area[i],sizeof(areasbbs_t));
				curarea.name=areatagstr;

				if(cfg.area[i].sub==INVALID_SUB) {			/* Passthru */
					start_tick=0;
					strip_psb(fmsgbuf);
					pkt_to_pkt(fmsgbuf,curarea,pkt_faddr,hdr,msg_seen,msg_path,0);
					printf("\n");
					continue; 
				} 						/* On to the next message */

				/* TODO: Should circular path detection occur before processing pass-through areas? */
				if(cfg.check_path) {
					for(j=0;j<scfg.total_faddrs;j++)
						if(check_psb(&msg_path,scfg.faddr[j]))
							break;
					if(j<scfg.total_faddrs) {
						start_tick=0;
						printf("Circular path (%s) ",smb_faddrtoa(&scfg.faddr[j],NULL));
						cfg.area[i].circular++;
						if(cfg.log&LOG_CIRCULAR)
							logprintf("%s: Circular path detected for %s"
								,areatagstr,smb_faddrtoa(&scfg.faddr[j],NULL));
						if(cfg.fwd_circular) {
							strip_psb(fmsgbuf);
							pkt_to_pkt(fmsgbuf,curarea,pkt_faddr,hdr,msg_seen,msg_path,0);
						}
						printf("\n");
						continue; 
					}
				}

				for(j=0;j<MAX_OPEN_SMBS;j++)
					if(subnum[j]==cfg.area[i].sub)
						break;
				if(j<MAX_OPEN_SMBS) 				/* already open */
					cur_smb=j;
				else {
					if(smb[cur_smb].shd_fp) 		/* If open */
						cur_smb=!cur_smb;			/* toggle between 0 and 1 */
					smb_close(&smb[cur_smb]);		/* close, if open */
					subnum[cur_smb]=INVALID_SUB; 	/* reset subnum (just incase) */
				}

				if(smb[cur_smb].shd_fp==NULL) { 	/* Currently closed */
					sprintf(smb[cur_smb].file,"%s%s",scfg.sub[cfg.area[i].sub]->data_dir
						,scfg.sub[cfg.area[i].sub]->code);
					smb[cur_smb].retry_time=scfg.smb_retry_time;
					if((j=smb_open(&smb[cur_smb]))!=SMB_SUCCESS) {
						sprintf(str,"ERROR %d opening %s area #%d, sub #%d)"
							,j,smb[cur_smb].file,i+1,cfg.area[i].sub+1);
						printf(str);
						logprintf(str);
						strip_psb(fmsgbuf);
						pkt_to_pkt(fmsgbuf,curarea,pkt_faddr,hdr,msg_seen
							,msg_path,0);
						printf("\n");
						continue; 
					}
					if(!filelength(fileno(smb[cur_smb].shd_fp))) {
						smb[cur_smb].status.max_crcs=scfg.sub[cfg.area[i].sub]->maxcrcs;
						smb[cur_smb].status.max_msgs=scfg.sub[cfg.area[i].sub]->maxmsgs;
						smb[cur_smb].status.max_age=scfg.sub[cfg.area[i].sub]->maxage;
						smb[cur_smb].status.attr=scfg.sub[cfg.area[i].sub]->misc&SUB_HYPER
								? SMB_HYPERALLOC:0;
						if((j=smb_create(&smb[cur_smb]))!=SMB_SUCCESS) {
							sprintf(str,"ERROR %d creating %s",j,smb[cur_smb].file);
							printf(str);
							logprintf(str);
							smb_close(&smb[cur_smb]);
							strip_psb(fmsgbuf);
							pkt_to_pkt(fmsgbuf,curarea,pkt_faddr,hdr,msg_seen
								,msg_path,0);
							printf("\n");
							continue; 
						} 
					}

					subnum[cur_smb]=cfg.area[i].sub;
				}

				if((hdr.attr&FIDO_PRIVATE) && !(scfg.sub[cfg.area[i].sub]->misc&SUB_PRIV)) {
					if(misc&IMPORT_PRIVATE)
						hdr.attr&=~FIDO_PRIVATE;
					else {
						start_tick=0;
						printf("Private posts disallowed.");
						if(cfg.log&LOG_PRIVATE)
							logprintf("%s: Private posts disallowed"
								,areatagstr);
						strip_psb(fmsgbuf);
						pkt_to_pkt(fmsgbuf,curarea,pkt_faddr,hdr,msg_seen
							,msg_path,0);
						printf("\n");
						continue; 
					} 
				}

				if(!(hdr.attr&FIDO_PRIVATE) && (scfg.sub[cfg.area[i].sub]->misc&SUB_PONLY))
					hdr.attr|=MSG_PRIVATE;

				/**********************/
				/* Importing EchoMail */
				/**********************/
				j=fmsgtosmsg(fmsgbuf,hdr,0,cfg.area[i].sub);

				if(start_tick) {
					import_ticks+=msclock()-start_tick;
					start_tick=0; 
				}

				if(j==SMB_DUPE_MSG) {
					if(cfg.log&LOG_DUPES)
						logprintf("%s Duplicate message",areatagstr);
					cfg.area[i].dupes++; 
				}
				else {	   /* Not a dupe */
					strip_psb(fmsgbuf);
					pkt_to_pkt(fmsgbuf,curarea,pkt_faddr
						,hdr,msg_seen,msg_path,0); 
				}

				if(j==0) {		/* Successful import */
					echomail++;
					cfg.area[i].imported++;
					/* Should this check if the user has access to the echo in question? */
					if(i!=cfg.badecho && (misc&NOTIFY_RECEIPT) && (m=matchname(hdr.to))!=0) {
						sprintf(str
						,"\7\1n\1hSBBSecho: \1m%.*s \1n\1msent you EchoMail on "
							"\1h%s \1n\1m%s\1n\r\n"
							,FIDO_NAME_LEN-1
							,hdr.from
							,scfg.grp[scfg.sub[cfg.area[i].sub]->grp]->sname
							,scfg.sub[cfg.area[i].sub]->sname);
						putsmsg(&scfg,m,str); 
					} 
				}
				printf("\n");
			}
			fclose(fidomsg);

			if(misc&DELETE_PACKETS)
				if(delfile(packet))
					lprintf(LOG_ERR,"ERROR line %d removing %s %s",__LINE__,packet
						,strerror(errno)); 
		}
		globfree(&g);

		if(start_tick) {
			import_ticks+=msclock()-start_tick;
			start_tick=0; 
		}

		} while(!kbhit() && unpack_bundle());

		if(kbhit()) lprintf(LOG_NOTICE,"\nKey pressed - premature termination");
		while(kbhit()) getch();

		}	/* End of Secure : Inbound loop */

		if(start_tick)	/* Last possible increment of import_ticks */
			import_ticks+=msclock()-start_tick;

		for(j=MAX_OPEN_SMBS-1;(int)j>=0;j--)		/* Close open bases */
			if(smb[j].shd_fp)
				smb_close(&smb[j]);

		pkt_to_pkt(fmsgbuf,fakearea,pkt_faddr,hdr,msg_seen,msg_path,1);

		/******* END OF IMPORT PKT ROUTINE *******/

		if(cfg.log&LOG_AREA_TOTALS) {
			for(i=0;i<cfg.areas;i++) {
				if(cfg.area[i].imported)
					lprintf(LOG_INFO,"Imported: %5u msgs %8s <- %s"
						,cfg.area[i].imported,scfg.sub[cfg.area[i].sub]->code
						,cfg.area[i].name); 
			}
			for(i=0;i<cfg.areas;i++) {
				if(cfg.area[i].circular)
					lprintf(LOG_INFO,"Circular: %5u detected in %s"
						,cfg.area[i].circular,cfg.area[i].name); 
			}
			for(i=0;i<cfg.areas;i++) {
				if(cfg.area[i].dupes)
					lprintf(LOG_INFO,"Duplicate: %5u detected in %s"
						,cfg.area[i].dupes,cfg.area[i].name); 
			} 
		}

		import_time=((float)import_ticks)/(float)MSCLOCKS_PER_SEC;
		if(cfg.log&LOG_TOTALS && import_time && echomail) {
			lprintf(LOG_INFO,"Imported: %5lu msgs in %.1f sec (%.1f/min %.1f/sec)"
				,echomail,import_time
				,import_time/60.0 ? (float)echomail/(import_time/60.0) :(float)echomail
				,(float)echomail/import_time);
		}
		FREE_AND_NULL(fmsgbuf);
		}

		if(misc&IMPORT_NETMAIL) {

		lprintf(LOG_DEBUG,"\nScanning for Inbound NetMail Messages...");

#ifdef __unix__
		sprintf(str,"%s*.[Mm][Ss][Gg]",scfg.netmail_dir);
#else
		sprintf(str,"%s*.msg",scfg.netmail_dir);
#endif
		glob(str,0,NULL,&g);
		for(f=0;f<g.gl_pathc && !kbhit();f++) {

			SAFECOPY(path,g.gl_pathv[f]);

			if((fidomsg=fnopen(&fmsg,path,O_RDWR))==NULL) {
				lprintf(LOG_ERR,"ERROR %u (%s) line %d opening %s",errno,strerror(errno),__LINE__,path);
				continue; 
			}
			if(filelength(fmsg)<sizeof(fmsghdr_t)) {
				lprintf(LOG_ERR,"ERROR line %d invalid length of %lu bytes for %s",__LINE__
					,filelength(fmsg),path);
				fclose(fidomsg);
				continue; 
			}
			if(!fread_fmsghdr(&hdr,fidomsg)) {
				fclose(fidomsg);
				lprintf(LOG_ERR,"ERROR line %d reading fido msghdr from %s",__LINE__,path);
				continue; 
			}
			i=import_netmail(path,hdr,fidomsg);
			/**************************************/
			/* Delete source netmail if specified */
			/**************************************/
			if(i==0) {
				if(misc&DELETE_NETMAIL) {
					fclose(fidomsg);
					if(delfile(path))
						lprintf(LOG_ERR,"ERROR line %d removing %s %s",__LINE__,path
							,strerror(errno)); 
				}
				else {
					hdr.attr|=FIDO_RECV;
					fseek(fidomsg,0L,SEEK_SET);
					fwrite(&hdr,sizeof(fmsghdr_t),1,fidomsg);
					fclose(fidomsg); 
				} 
			}
			else if(i!=-2)
				fclose(fidomsg);
			printf("\n"); 
			}
		globfree(&g);
	}


	if(misc&EXPORT_ECHOMAIL)
		export_echomail(sub_code,addr);

	if(misc&PACK_NETMAIL) {

		memset(&msg_seen,0,sizeof(addrlist_t));
		memset(&msg_path,0,sizeof(addrlist_t));
		memset(&fakearea,0,sizeof(areasbbs_t));

		lprintf(LOG_DEBUG,"\nPacking Outbound NetMail from %s",scfg.netmail_dir);

#ifdef __unix__
		sprintf(str,"%s*.[Mm][Ss][Gg]",scfg.netmail_dir);
#else
		sprintf(str,"%s*.msg",scfg.netmail_dir);
#endif
		glob(str,0,NULL,&g);
		for(f=0;f<g.gl_pathc && !kbhit();f++) {

			SAFECOPY(path,g.gl_pathv[f]);

			if((fidomsg=fnopen(&fmsg,path,O_RDWR))==NULL) {
				lprintf(LOG_ERR,"ERROR %u (%s) line %d opening %s",errno,strerror(errno),__LINE__,path);
				continue; 
			}
			if(filelength(fmsg)<sizeof(fmsghdr_t)) {
				lprintf(LOG_WARNING,"%s Invalid length of %lu bytes",path,filelength(fmsg));
				fclose(fidomsg);
				continue; 
			}
			if(!fread_fmsghdr(&hdr,fidomsg)) {
				fclose(fidomsg);
				lprintf(LOG_ERR,"ERROR line %d reading fido msghdr from %s",__LINE__,path);
				continue; 
			}
			hdr.destzone=hdr.origzone=sys_faddr.zone;
			hdr.destpoint=hdr.origpoint=0;
			getzpt(fidomsg,&hdr);				/* use kludge if found */
			addr.zone=hdr.destzone;
			addr.net=hdr.destnet;
			addr.node=hdr.destnode;
			addr.point=hdr.destpoint;
			for(i=0;i<scfg.total_faddrs;i++)
				if(!memcmp(&addr,&scfg.faddr[i],sizeof(faddr_t)))
					break;
			if(i<scfg.total_faddrs)	{				  /* In-bound, so ignore */
				fclose(fidomsg);
				continue;
			}
			printf("\n%s to %s ",getfname(path),smb_faddrtoa(&addr,NULL));
			if(hdr.attr&FIDO_SENT) {
				printf("already sent\n");
				fclose(fidomsg);
				continue;
			}
			if(!(misc&DELETE_NETMAIL)) {
				hdr.attr|=FIDO_SENT;
				rewind(fidomsg);
				fseek(fidomsg,offsetof(fmsghdr_t,attr),SEEK_SET);
				fwrite(&hdr.attr,sizeof(hdr.attr),1,fidomsg);
				fseek(fidomsg,sizeof(fmsghdr_t),SEEK_SET);
			}

			if(cfg.log&LOG_PACKING)
				logprintf("Packing %s (%s) attr=%04hX",path,smb_faddrtoa(&addr,NULL),hdr.attr);
			fmsgbuf=getfmsg(fidomsg,NULL);
			if(!fmsgbuf) {
				lprintf(LOG_ERR,"ERROR line %d allocating memory for NetMail fmsgbuf"
					,__LINE__);
				bail(1); 
				return -1;
			}
			fclose(fidomsg);

			attr=0;
			node=matchnode(addr,0);
			if(node<cfg.nodecfgs && cfg.nodecfg[node].route.zone
				&& !(hdr.attr&(FIDO_CRASH|FIDO_HOLD))) {
				addr=cfg.nodecfg[node].route;		/* Routed */
				if(cfg.log&LOG_ROUTING)
					logprintf("Routing %s to %s",path,smb_faddrtoa(&addr,NULL));
				node=matchnode(addr,0); 
			}
			if(node<cfg.nodecfgs)
				attr=cfg.nodecfg[node].attr;

			if(misc&FLO_MAILER) {
				if(hdr.attr&FIDO_CRASH)
					attr|=ATTR_CRASH;
				else if(hdr.attr&FIDO_HOLD)
					attr|=ATTR_HOLD;
				if(attr&ATTR_CRASH) ch='c';
				else if(attr&ATTR_HOLD) ch='h';
				else if(attr&ATTR_DIRECT) ch='d';
				else ch='o';
				if(addr.zone==sys_faddr.zone) { /* Default zone, use default outbound */
					SAFECOPY(outbound,cfg.outbound);
				} else {						 /* Inter-zone outbound is OUTBOUND.XXX */
					SAFEPRINTF3(outbound,"%.*s.%03x"
						,(int)strlen(cfg.outbound)-1,cfg.outbound,addr.zone);
					MKDIR(outbound);
					backslash(outbound);
				}
				if(addr.point) {			/* Point destination is OUTBOUND.PNT */
					sprintf(str,"%04x%04x.pnt"
						,addr.net,addr.node);
					strcat(outbound,str); 
				}
				if(outbound[strlen(outbound)-1]=='\\'
					|| outbound[strlen(outbound)-1]=='/')
					outbound[strlen(outbound)-1]=0;
				MKDIR(outbound);
				backslash(outbound);
				if(addr.point)
					SAFEPRINTF3(packet,"%s%08x.%cut",outbound,addr.point,ch);
				else
					SAFEPRINTF4(packet,"%s%04x%04x.%cut",outbound,addr.net,addr.node,ch);
				if(hdr.attr&FIDO_FILE)
					if(write_flofile(hdr.subj,addr,FALSE /* !bundle */))
						bail(1); 
			}
			else {
				SAFECOPY(packet,pktname(/* Temp? */ FALSE));
			}

			lprintf(LOG_DEBUG,"NetMail packet: %s", packet);
			now=time(NULL);
			tm=localtime(&now);
			if((stream=fnopen(&file,packet,O_RDWR|O_CREAT))==NULL) {
				lprintf(LOG_ERR,"ERROR %u (%s) line %d opening %s",errno,strerror(errno),__LINE__,packet);
				bail(1); 
				return -1;
			}

			if(filelength(file) < sizeof(pkthdr_t)) {
				chsize(file,0);
				rewind(stream);
				memset(&pkthdr,0,sizeof(pkthdr));
				pkthdr.orignode=hdr.orignode;
				pkthdr.destnode=addr.node;
				pkthdr.year=tm->tm_year+1900;
				pkthdr.month=tm->tm_mon;
				pkthdr.day=tm->tm_mday;
				pkthdr.hour=tm->tm_hour;
				pkthdr.min=tm->tm_min;
				pkthdr.sec=tm->tm_sec;
				pkthdr.baud=0x00;
				pkthdr.pkttype=0x0002;
				pkthdr.orignet=hdr.orignet;
				pkthdr.destnet=addr.net;
				pkthdr.prodcode=0x00;
				pkthdr.sernum=0;
				pkthdr.origzone=hdr.origzone;
				pkthdr.destzone=addr.zone;
				if(node<cfg.nodecfgs) {
					if(cfg.nodecfg[node].pkt_type==PKT_TWO_PLUS) {
						two_plus = (two_plus_t*)&pkthdr.empty;
						if(hdr.origpoint) {
							pkthdr.orignet=-1;
							two_plus->auxnet=hdr.orignet; 
						}
						two_plus->cwcopy=0x0100;
						two_plus->prodcode=pkthdr.prodcode;
						two_plus->revision=pkthdr.sernum;
						two_plus->cword=0x0001;
						two_plus->origzone=pkthdr.origzone;
						two_plus->destzone=pkthdr.destzone;
						two_plus->origpoint=hdr.origpoint;
						two_plus->destpoint=addr.point;
					}
					memcpy(pkthdr.password,cfg.nodecfg[node].pktpwd,sizeof(pkthdr.password));
				}
				fwrite(&pkthdr,sizeof(pkthdr_t),1,stream); 
			} else
				fseek(stream,find_packet_terminator(stream),SEEK_SET);

			putfmsg(stream,fmsgbuf,hdr,fakearea,msg_seen,msg_path);

			/* Write packet terminator */
			terminate_packet(stream);

			FREE_AND_NULL(fmsgbuf);
			fclose(stream);
			/**************************************/
			/* Delete source netmail if specified */
			/**************************************/
			if(misc&DELETE_NETMAIL)
				if(delfile(path))
					lprintf(LOG_ERR,"ERROR line %d removing %s %s",__LINE__,path
						,strerror(errno));
			printf("\n"); 
		}
		globfree(&g);
	}

	if(misc&UPDATE_MSGPTRS) {

		lprintf(LOG_DEBUG,"\nUpdating Message Pointers to Last Posted Message...");

		for(j=0; j<cfg.areas; j++) {
			uint32_t lastmsg;
			if(j==cfg.badecho)	/* Don't scan the bad-echo area */
				continue;
			i=cfg.area[j].sub;
			if(i<0 || i>=scfg.total_subs)	/* Don't scan pass-through areas */
				continue;
			lprintf(LOG_DEBUG,"\n%-*.*s -> %s"
				,LEN_EXTCODE, LEN_EXTCODE, scfg.sub[i]->code, cfg.area[j].name);
			if(getlastmsg(i,&lastmsg,0) >= 0)
				write_export_ptr(i, lastmsg, cfg.area[j].name);
		}
	}

	if(misc&(IMPORT_NETMAIL|IMPORT_ECHOMAIL) && (misc&REPORT)) {
		now=time(NULL);
		sprintf(str,"%ssbbsecho.msg",scfg.text_dir);
		if((file=nopen(str,O_WRONLY|O_CREAT|O_TRUNC))==-1) {
			lprintf(LOG_ERR,"ERROR %u (%s) line %d opening %s",errno,strerror(errno),__LINE__,str);
			bail(1); 
			return -1;
		}
		sprintf(fname,"\1c\1h               "
			"\xdc\xdc\xdc\xdc\xdc\xdc\xdc\xdc\xdc\xdc\xdc\xdc\xdc\xdc\xdc\xdc\xdc\xdc\xdc\xdc\xdc\xdc\xdc\xdc\xdc\xdc\xdc\xdc\xdc\xdc\xdc\xdc\xdc\xdc\xdc\xdc\xdc\xdc\xdc\xdc\xdc\xdc\xdc\xdc\xdc\xdc\xdc\xdc\xdc\xdc\xdc\r\n");
		sprintf(path,"\1c\1h               "
			"\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf\xdf\r\n");
		write(file,fname,strlen(fname));
		sprintf(str,"               \1n\1k\0016"
			" Last FidoNet Transfer on %.24s \1n\r\n",ctime(&now));
		write(file,str,strlen(str));
		write(file,path,strlen(path));
		write(file,fname,strlen(fname));
		sprintf(tmp,"Imported %lu EchoMail and %lu NetMail Messages"
			,echomail,netmail);
		sprintf(str,"               \1n\1k\0016 %-50.50s\1n\r\n",tmp);
		write(file,str,strlen(str));
		write(file,path,strlen(path));
		close(file); 
	}

	pkt_to_pkt(NULL,fakearea,pkt_faddr,hdr,msg_seen,msg_path,1);
	if(email->shd_fp)
		smb_close(email);

	free(smb);
	free(email);

	bail(0);
	return(0);
}
