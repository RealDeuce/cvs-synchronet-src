/* rechocfg.C */

/* Synchronet FidoNet EchoMail Scanning/Tossing and NetMail Tossing Utility */

/* $Id: rechocfg.c,v 1.36 2015/10/24 06:33:41 rswindell Exp $ */

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

#include "sbbs.h"
#include "sbbsecho.h"
#include "filewrap.h"	/* O_DENYNONE */

#ifdef __WATCOMC__
	#include <mem.h>
    #define O_DENYNONE SH_DENYNO
#endif

extern long misc;
extern config_t cfg;

/******************************************************************************
 Here we take a string and put a terminator in place of the first TAB or SPACE
******************************************************************************/
char *cleanstr(char *instr)
{
	int i;

	for(i=0;instr[i];i++)
		if((uchar)instr[i]<=' ')
			break;
	instr[i]=0;
	return(instr);
}

/****************************************************************************/
/* Returns the FidoNet address kept in str as ASCII.                        */
/****************************************************************************/
faddr_t atofaddr(char *instr)
{
	char *p,str[51];
    faddr_t addr;

	sprintf(str,"%.50s",instr);
	cleanstr(str);
	if(!stricmp(str,"ALL")) {
		addr.zone=addr.net=addr.node=addr.point=0xffff;
		return(addr); 
	}
	addr.zone=addr.net=addr.node=addr.point=0;
	if((p=strchr(str,':'))!=NULL) {
		if(!strnicmp(str,"ALL:",4))
			addr.zone=0xffff;
		else
			addr.zone=atoi(str);
		p++;
		if(!strnicmp(p,"ALL",3))
			addr.net=0xffff;
		else
			addr.net=atoi(p);
	}
	else {
	#ifdef SCFG
		if(total_faddrs)
			addr.zone=faddr[0].zone;
		else
	#endif
			addr.zone=1;
		addr.net=atoi(str);
	}
	if(!addr.zone)              /* no such thing as zone 0 */
		addr.zone=1;
	if((p=strchr(str,'/'))!=NULL) {
		p++;
		if(!strnicmp(p,"ALL",3))
			addr.node=0xffff;
		else
			addr.node=atoi(p);
	}
	else {
		if(!addr.net) {
	#ifdef SCFG
			if(total_faddrs)
				addr.net=faddr[0].net;
			else
	#endif
				addr.net=1;
		}
		addr.node=atoi(str);
	}
	if((p=strchr(str,'.'))!=NULL) {
		p++;
		if(!strnicmp(p,"ALL",3))
			addr.point=0xffff;
		else
			addr.point=atoi(p);
	}
	return(addr);
}

/******************************************************************************
 This function returns the number of the node in the SBBSECHO.CFG file which
 matches the address passed to it (or cfg.nodecfgs if no match).
 ******************************************************************************/
int matchnode(faddr_t addr, int exact)
{
	uint i;

	if(exact!=2) {
		for(i=0;i<cfg.nodecfgs;i++) 				/* Look for exact match */
			if(!memcmp(&cfg.nodecfg[i].faddr,&addr,sizeof(faddr_t)))
				break;
		if(exact || i<cfg.nodecfgs)
			return(i);
	}

	for(i=0;i<cfg.nodecfgs;i++) 					/* Look for point match */
		if(cfg.nodecfg[i].faddr.point==0xffff
			&& addr.zone==cfg.nodecfg[i].faddr.zone
			&& addr.net==cfg.nodecfg[i].faddr.net
			&& addr.node==cfg.nodecfg[i].faddr.node)
			break;
	if(i<cfg.nodecfgs)
		return(i);

	for(i=0;i<cfg.nodecfgs;i++) 					/* Look for node match */
		if(cfg.nodecfg[i].faddr.node==0xffff
			&& addr.zone==cfg.nodecfg[i].faddr.zone
			&& addr.net==cfg.nodecfg[i].faddr.net)
			break;
	if(i<cfg.nodecfgs)
		return(i);

	for(i=0;i<cfg.nodecfgs;i++) 					/* Look for net match */
		if(cfg.nodecfg[i].faddr.net==0xffff
			&& addr.zone==cfg.nodecfg[i].faddr.zone)
			break;
	if(i<cfg.nodecfgs)
		return(i);

	for(i=0;i<cfg.nodecfgs;i++) 					/* Look for total wild */
		if(cfg.nodecfg[i].faddr.zone==0xffff)
			break;
	return(i);
}

#define SKIPCTRLSP(p)	while(*p>0 && *p<=' ') p++
#define SKIPCODE(p)		while(*p<0 || *p>' ') p++
void read_echo_cfg()
{
	char *str = NULL;
	size_t str_size;
	char tmp[512],*p,*tp;
	short attr=0;
	int i,j,file;
	uint u;
	FILE *stream;
	faddr_t addr,route_addr;


	/****** READ IN SBBSECHO.CFG FILE *******/

	printf("\nReading %s\n",cfg.cfgfile);
	if((stream=fnopen(&file,cfg.cfgfile,O_RDONLY))==NULL) {
		printf("Unable to open %s for read.\n",cfg.cfgfile);
		bail(1);
	}

	cfg.maxpktsize=DFLT_PKT_SIZE;
	cfg.maxbdlsize=DFLT_BDL_SIZE;
	cfg.badecho=-1;
	cfg.log=LOG_DEFAULTS;
	cfg.log_level=LOG_INFO;
	cfg.check_path=TRUE;
	cfg.fwd_circular=TRUE;
	cfg.zone_blind=FALSE;
	cfg.zone_blind_threshold=0xffff;
	SAFECOPY(cfg.sysop_alias,"SYSOP");

	while(1) {
		if(getdelim(&str,&str_size,'\n',stream)==-1)
			break;
		truncsp(str);
		p=str;
		SKIPCTRLSP(p);
		if(*p==';')
			continue;
		sprintf(tmp,"%-.25s",p);
		tp=strchr(tmp,' ');
		if(tp)
			*tp=0;                              /* Chop off at space */
#if 0
		strupr(tmp);                            /* Convert code to uppercase */
#endif
		SKIPCODE(p);                       /* Skip code */
		SKIPCTRLSP(p);                /* Skip white space */

		if(!stricmp(tmp,"SYSOP_ALIAS")) {
			SAFECOPY(cfg.sysop_alias, p);
			continue;
		}
		if(!stricmp(tmp,"PACKER")) {             /* Archive Definition */
			if((cfg.arcdef=(arcdef_t *)realloc(cfg.arcdef
				,sizeof(arcdef_t)*(cfg.arcdefs+1)))==NULL) {
				printf("\nError allocating %" XP_PRIsize_t "u bytes of memory for arcdef #%u.\n"
					,sizeof(arcdef_t)*(cfg.arcdefs+1),cfg.arcdefs+1);
				bail(1);
			}
			SAFECOPY(cfg.arcdef[cfg.arcdefs].name,p);
			tp=cfg.arcdef[cfg.arcdefs].name;
			while(*tp && *tp>' ') tp++;
			*tp=0;
			SKIPCODE(p);
			SKIPCTRLSP(p);
			cfg.arcdef[cfg.arcdefs].byteloc=atoi(p);
			SKIPCODE(p);
			SKIPCTRLSP(p);
			SAFECOPY(cfg.arcdef[cfg.arcdefs].hexid,p);
			tp=cfg.arcdef[cfg.arcdefs].hexid;
			SKIPCODE(tp);
			*tp=0;
			while((getdelim(&str,&str_size,'\n',stream) != -1) && strnicmp(str,"END",3)) {
				p=str;
				SKIPCTRLSP(p);
				if(!strnicmp(p,"PACK ",5)) {
					p+=5;
					SKIPCTRLSP(p);
					SAFECOPY(cfg.arcdef[cfg.arcdefs].pack,p);
					truncsp(cfg.arcdef[cfg.arcdefs].pack);
					continue;
				}
				if(!strnicmp(p,"UNPACK ",7)) {
					p+=7;
					SKIPCTRLSP(p);
					SAFECOPY(cfg.arcdef[cfg.arcdefs].unpack,p);
					truncsp(cfg.arcdef[cfg.arcdefs].unpack);
				}
			}
			++cfg.arcdefs;
			continue;
		}

		if(!stricmp(tmp,"REGNUM"))
			continue;

		if(!stricmp(tmp,"NOPATHCHECK")) {
			cfg.check_path=FALSE;
			continue;
		}
		if(!stricmp(tmp,"NOCIRCULARFWD")) {
			cfg.fwd_circular=FALSE;
			continue;
		}

		if(!stricmp(tmp,"ZONE_BLIND")) {
			cfg.zone_blind=TRUE;
			if(*p && isdigit(*p))	/* threshold specified (zones > this threshold will be treated normally/separately) */
				cfg.zone_blind_threshold=atoi(p);
			continue;
		}

		if(!stricmp(tmp,"NOTIFY")) {
			cfg.notify=atoi(cleanstr(p));
			continue;
		}

		if(!stricmp(tmp,"LOG")) {
			cleanstr(p);
			if(!stricmp(p,"ALL"))
				cfg.log=0xffffffffUL;
			else if(!stricmp(p,"DEFAULT"))
				cfg.log=LOG_DEFAULTS;
			else if(!stricmp(p,"NONE"))
				cfg.log=0L;
			else
				cfg.log=strtol(cleanstr(p),0,16);
			continue;
		}

		if(!stricmp(tmp,"LOG_LEVEL")) {
			cfg.log_level=atoi(cleanstr(p));
			continue;
		}

		if(!stricmp(tmp,"NOSWAP")) {
			continue;
		}

		if(!stricmp(tmp,"SECURE_ECHOMAIL")) {
			misc|=SECURE;
			continue;
		}

		if(!stricmp(tmp,"STRIP_LF")) {
			misc|=STRIP_LF;
			continue;
		}

		if(!stricmp(tmp,"CONVERT_TEAR")) {
			misc|=CONVERT_TEAR;
			continue;
		}

		if(!stricmp(tmp,"STORE_SEENBY")) {
			misc|=STORE_SEENBY;
			continue;
		}

		if(!stricmp(tmp,"STORE_PATH")) {
			misc|=STORE_PATH;
			continue;
		}

		if(!stricmp(tmp,"STORE_KLUDGE")) {
			misc|=STORE_KLUDGE;
			continue;
		}

		if(!stricmp(tmp,"FUZZY_ZONE")) {
			misc|=FUZZY_ZONE;
			continue;
		}

		if(!stricmp(tmp,"TRUNC_BUNDLES")) {
			misc|=TRUNC_BUNDLES;
			continue;
		}

		if(!stricmp(tmp,"FLO_MAILER")) {
			misc|=FLO_MAILER;
			continue;
		}

		if(!stricmp(tmp,"ELIST_ONLY")) {
			misc|=ELIST_ONLY;
			continue;
		}

		if(!stricmp(tmp,"KILL_EMPTY")) {
			misc|=KILL_EMPTY_MAIL;
			continue;
		}

		if(!stricmp(tmp,"AREAFILE")) {
			SAFECOPY(cfg.areafile,cleanstr(p));
			continue;
		}

		if(!stricmp(tmp,"LOGFILE")) {
			SAFECOPY(cfg.logfile,cleanstr(p));
			continue;
		}

		if(!stricmp(tmp,"INBOUND")) {            /* Inbound directory */
			SAFECOPY(cfg.inbound,cleanstr(p));
			backslash(cfg.inbound);
			continue;
		}

		if(!stricmp(tmp,"SECURE_INBOUND")) {     /* Secure Inbound directory */
			SAFECOPY(cfg.secure,cleanstr(p));
			backslash(cfg.secure);
			continue;
		}

		if(!stricmp(tmp,"OUTBOUND")) {           /* Outbound directory */
			SAFECOPY(cfg.outbound,cleanstr(p));
			backslash(cfg.outbound);
			continue;
		}

		if(!stricmp(tmp,"ARCSIZE")) {            /* Maximum bundle size */
			cfg.maxbdlsize=atol(p);
			continue;
		}

		if(!stricmp(tmp,"PKTSIZE")) {            /* Maximum packet size */
			cfg.maxpktsize=atol(p);
			continue;
		}

		if(!stricmp(tmp,"USEPACKER")) {          /* Which packer to use */
			if(!*p)
				continue;
			strcpy(str,p);
			p=str;
			SKIPCODE(p);
			if(!*p)
				continue;
			*p=0;
			p++;
			for(u=0;u<cfg.arcdefs;u++)
				if(!stricmp(cfg.arcdef[u].name,str))
					break;
			if(u==cfg.arcdefs)				/* i = number of arcdef til done */
				u=0xffff;					/* Uncompressed type if not found */
			while(*p) {
				SKIPCTRLSP(p);
				if(!*p)
					break;
				addr=atofaddr(p);
				SKIPCODE(p);
				j=matchnode(addr,1);
				if(j==cfg.nodecfgs) {
					cfg.nodecfgs++;
					if((cfg.nodecfg=(nodecfg_t *)realloc(cfg.nodecfg
						,sizeof(nodecfg_t)*(j+1)))==NULL) {
						printf("\nError allocating memory for nodecfg #%u.\n"
							,j+1);
						bail(1);
					}
					memset(&cfg.nodecfg[j],0,sizeof(nodecfg_t));
					cfg.nodecfg[j].faddr=addr;
				}
				cfg.nodecfg[j].arctype=u;
			}
		}

		if(!stricmp(tmp,"PKTPWD")) {         /* Packet Password */
			if(!*p)
				continue;
			addr=atofaddr(p);
			SKIPCODE(p);         /* Skip address */
			SKIPCTRLSP(p);        /* Find beginning of password */
			j=matchnode(addr,1);
			if(j==cfg.nodecfgs) {
				cfg.nodecfgs++;
				if((cfg.nodecfg=(nodecfg_t *)realloc(cfg.nodecfg
					,sizeof(nodecfg_t)*(j+1)))==NULL) {
					printf("\nError allocating memory for nodecfg #%u.\n"
						,j+1);
					bail(1);
				}
				memset(&cfg.nodecfg[j],0,sizeof(nodecfg_t));
				cfg.nodecfg[j].faddr=addr;
			}
			SAFECOPY(cfg.nodecfg[j].pktpwd,p);
		}

		if(!stricmp(tmp,"PKTTYPE")) {            /* Packet Type to Use */
			if(!*p)
				continue;
			strcpy(str,p);
			p=str;
			SKIPCODE(p);
			*p=0;
			p++;
			while(*p) {
				SKIPCTRLSP(p);
				if(!*p)
					break;
				addr=atofaddr(p);
				SKIPCODE(p);
				j=matchnode(addr,1);
				if(j==cfg.nodecfgs) {
					cfg.nodecfgs++;
					if((cfg.nodecfg=(nodecfg_t *)realloc(cfg.nodecfg
						,sizeof(nodecfg_t)*(j+1)))==NULL) {
						printf("\nError allocating memory for nodecfg #%u.\n"
							,j+1);
						bail(1);
					}
					memset(&cfg.nodecfg[j],0,sizeof(nodecfg_t));
					cfg.nodecfg[j].faddr=addr;
				}
				if(!strcmp(str,"2+"))
					cfg.nodecfg[j].pkt_type=PKT_TWO_PLUS;
				else if(!strcmp(str,"2.2"))
					cfg.nodecfg[j].pkt_type=PKT_TWO_TWO;
				else if(!strcmp(str,"2"))
					cfg.nodecfg[j].pkt_type=PKT_TWO;
			}
		}

		if(!stricmp(tmp,"SEND_NOTIFY")) {    /* Nodes to send notify lists to */
			while(*p) {
				SKIPCTRLSP(p);
				if(!*p)
					break;
				addr=atofaddr(p);
				SKIPCODE(p);
				j=matchnode(addr,1);
				if(j==cfg.nodecfgs) {
					cfg.nodecfgs++;
					if((cfg.nodecfg=(nodecfg_t *)realloc(cfg.nodecfg
						,sizeof(nodecfg_t)*(j+1)))==NULL) {
						printf("\nError allocating memory for nodecfg #%u.\n"
							,j+1);
						bail(1);
					}
					memset(&cfg.nodecfg[j],0,sizeof(nodecfg_t));
					cfg.nodecfg[j].faddr=addr;
				}
				cfg.nodecfg[j].attr|=SEND_NOTIFY;
			}
		}

		if(!stricmp(tmp,"PASSIVE")
			|| !stricmp(tmp,"HOLD")
			|| !stricmp(tmp,"CRASH")
			|| !stricmp(tmp,"DIRECT")) {         /* Set node attributes */
			if(!stricmp(tmp,"PASSIVE"))
				attr=ATTR_PASSIVE;
			else if(!stricmp(tmp,"CRASH"))
				attr=ATTR_CRASH;
			else if(!stricmp(tmp,"HOLD"))
				attr=ATTR_HOLD;
			else if(!stricmp(tmp,"DIRECT"))
				attr=ATTR_DIRECT;
			while(*p) {
				SKIPCTRLSP(p);
				if(!*p)
					break;
				addr=atofaddr(p);
				SKIPCODE(p);
				j=matchnode(addr,1);
				if(j==cfg.nodecfgs) {
					cfg.nodecfgs++;
					if((cfg.nodecfg=(nodecfg_t *)realloc(cfg.nodecfg
						,sizeof(nodecfg_t)*(j+1)))==NULL) {
						printf("\nError allocating memory for nodecfg #%u.\n"
							,j+1);
						bail(1);
					}
					memset(&cfg.nodecfg[j],0,sizeof(nodecfg_t));
					cfg.nodecfg[j].faddr=addr;
				}
				cfg.nodecfg[j].attr|=attr;
			}
		}

		if(!stricmp(tmp,"ROUTE_TO")) {
			SKIPCTRLSP(p);
			if(*p) {
				route_addr=atofaddr(p);
				SKIPCODE(p);
			}
			while(*p) {
				SKIPCTRLSP(p);
				if(!*p)
					break;
				addr=atofaddr(p);
				SKIPCODE(p);
				j=matchnode(addr,1);
				if(j==cfg.nodecfgs) {
					cfg.nodecfgs++;
					if((cfg.nodecfg=(nodecfg_t *)realloc(cfg.nodecfg
						,sizeof(nodecfg_t)*(j+1)))==NULL) {
						printf("\nError allocating memory for nodecfg #%u.\n"
							,j+1);
						bail(1);
					}
					memset(&cfg.nodecfg[j],0,sizeof(nodecfg_t));
					cfg.nodecfg[j].faddr=addr;
				}
				cfg.nodecfg[j].route=route_addr;
			}
		}

		if(!stricmp(tmp,"AREAFIX")) {            /* Areafix stuff here */
			if(!*p)
				continue;
			addr=atofaddr(p);
			i=matchnode(addr,1);
			if(i==cfg.nodecfgs) {
				cfg.nodecfgs++;
				if((cfg.nodecfg=(nodecfg_t *)realloc(cfg.nodecfg
					,sizeof(nodecfg_t)*(i+1)))==NULL) {
					printf("\nError allocating memory for nodecfg #%u.\n"
						,i+1);
					bail(1);
				}
				memset(&cfg.nodecfg[i],0,sizeof(nodecfg_t));
				cfg.nodecfg[i].faddr=addr;
			}
			cfg.nodecfg[i].flag=NULL;
			SKIPCODE(p); 		/* Get to the end of the address */
			SKIPCTRLSP(p);		/* Skip over whitespace chars */
			tp=p;
			SKIPCODE(p); 		/* Find end of password 	*/
			*p=0;							/* and terminate the string */
			++p;
			SAFECOPY(cfg.nodecfg[i].password,tp);
			SKIPCTRLSP(p);		/* Search for more chars */
			if(!*p) 						/* Nothing else there */
				continue;
			while(*p) {
				tp=p;
				SKIPCODE(p); 	/* Find end of this flag */
				*p=0;						/* and terminate it 	 */
				++p;
				for(j=0;j<cfg.nodecfg[i].numflags;j++)
					if(!stricmp(cfg.nodecfg[i].flag[j].flag,tp))
						break;
				if(j==cfg.nodecfg[i].numflags) {
					if((cfg.nodecfg[i].flag=
						(flag_t *)realloc(cfg.nodecfg[i].flag
						,sizeof(flag_t)*(j+1)))==NULL) {
						printf("\nError allocating memory for nodecfg #%u "
							"flag #%u.\n",cfg.nodecfgs,j+1);
						bail(1);
					}
					cfg.nodecfg[i].numflags++;
					SAFECOPY(cfg.nodecfg[i].flag[j].flag,tp);
				}
				SKIPCTRLSP(p);
			}
		}

		if(!stricmp(tmp,"ECHOLIST")) {           /* Echolists go here */
			if((cfg.listcfg=(echolist_t *)realloc(cfg.listcfg
				,sizeof(echolist_t)*(cfg.listcfgs+1)))==NULL) {
				printf("\nError allocating memory for echolist cfg #%u.\n"
					,cfg.listcfgs+1);
				bail(1);
			}
			memset(&cfg.listcfg[cfg.listcfgs],0,sizeof(echolist_t));
			++cfg.listcfgs;
			/* Need to forward requests? */
			if(!strnicmp(p,"FORWARD ",8) || !strnicmp(p,"HUB ",4)) {
				if(!strnicmp(p,"HUB ",4))
					cfg.listcfg[cfg.listcfgs-1].misc|=NOFWD;
				SKIPCODE(p);
				SKIPCTRLSP(p);
				if(*p)
					cfg.listcfg[cfg.listcfgs-1].forward=atofaddr(p);
				SKIPCODE(p);
				SKIPCTRLSP(p);
				if(*p && !(cfg.listcfg[cfg.listcfgs-1].misc&NOFWD)) {
					tp=p;
					SKIPCODE(p);
					*p=0;
					++p;
					SKIPCTRLSP(p);
					SAFECOPY(cfg.listcfg[cfg.listcfgs-1].password,tp);
				}
			}
			else
				cfg.listcfg[cfg.listcfgs-1].misc|=NOFWD;
			if(!*p)
				continue;
			tp=p;
			SKIPCODE(p);
			*p=0;
			p++;

			SAFECOPY(cfg.listcfg[cfg.listcfgs-1].listpath,tp);
			cfg.listcfg[cfg.listcfgs-1].numflags=0;
			cfg.listcfg[cfg.listcfgs-1].flag=NULL;
			SKIPCTRLSP(p);		/* Skip over whitespace chars */
			while(*p) {
				tp=p;
				SKIPCODE(p); 	/* Find end of this flag */
				*p=0;						/* and terminate it 	 */
				++p;
				for(u=0;u<cfg.listcfg[cfg.listcfgs-1].numflags;u++)
					if(!stricmp(cfg.listcfg[cfg.listcfgs-1].flag[u].flag,tp))
						break;
				if(u==cfg.listcfg[cfg.listcfgs-1].numflags) {
					if((cfg.listcfg[cfg.listcfgs-1].flag=
						(flag_t *)realloc(cfg.listcfg[cfg.listcfgs-1].flag
						,sizeof(flag_t)*(u+1)))==NULL) {
						printf("\nError allocating memory for listcfg #%u "
							"flag #%u.\n",cfg.listcfgs,u+1);
						bail(1);
					}
					cfg.listcfg[cfg.listcfgs-1].numflags++;
					SAFECOPY(cfg.listcfg[cfg.listcfgs-1].flag[u].flag,tp);
				}
				SKIPCTRLSP(p); 
			} 
		}

		/* Message disabled why?  ToDo */
		/* printf("Unrecognized line in SBBSECHO.CFG file.\n"); */
	}
	fclose(stream);

	/* make sure we have some sane "maximum" size values here: */
	if(cfg.maxpktsize<1024)
		cfg.maxpktsize=DFLT_PKT_SIZE;
	if(cfg.maxbdlsize<1024)
		cfg.maxbdlsize=DFLT_BDL_SIZE;

	if(str)
		free(str);
	printf("\n");
}

