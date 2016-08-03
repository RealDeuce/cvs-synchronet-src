/* sbbsecho.h */

/* Synchronet FidoNet Echomail tosser/scanner/areafix program */

/* $Id: sbbsecho.h,v 3.9 2016/08/03 08:03:36 rswindell Exp $ */

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

#include <str_list.h>
#include <stdbool.h>
#include <smbdefs.h>
#include "sbbsdefs.h"
#include "fidodefs.h"

#define SBBSECHO_VERSION_MAJOR		3
#define SBBSECHO_VERSION_MINOR		00

#define SBBSECHO_PRODUCT_CODE		0x12FF	/* from http://ftsc.org/docs/ftscprod.013 */

enum mail_status {
	 MAIL_STATUS_NORMAL
	,MAIL_STATUS_HOLD
	,MAIL_STATUS_CRASH
};

enum pkt_type {
	 PKT_TYPE_2_PLUS				/* Type 2+ Packet Header  (FSC-48)		*/
	,PKT_TYPE_2_2					/* Type 2.2 Packet Header (FSC-45)		*/
	,PKT_TYPE_2_0 					/* Old Type Packet Header (FTS-1)		*/	
};

#define DFLT_PKT_SIZE   (250*1024L)
#define DFLT_BDL_SIZE   (250*1024L)

#define SBBSECHO_MAX_KEY_LEN	25	/* for AreaFix/EchoList keys (previously known as "flags") */

typedef struct {
    uint		sub;						/* Set to INVALID_SUB if pass-thru */
    char*		name;						/* Area tag name */
	uint		imported; 					/* Total messages imported this run */
	uint		exported; 					/* Total messages exported this run */
	uint		circular; 					/* Total circular paths detected */
	uint		dupes;						/* Total duplicate messages detected */
    uint		links;						/* Total number of links for this area */
    fidoaddr_t*	link;						/* Each link */
} area_t;

typedef struct {
	uint			addrs; 		/* Total number of links */
	fidoaddr_t *	addr;		/* Each link */
} addrlist_t;

typedef struct {
	char name[26]				/* Short name of archive type */
		,hexid[26]				/* Hexadecimal ID to search for */
		,pack[MAX_PATH+1]		/* Pack command line */
		,unpack[MAX_PATH+1];	/* Unpack command line */
	uint byteloc;				/* Offset to Hex ID */
} arcdef_t;

typedef struct {
	fidoaddr_t 	addr			/* Fido address of this node */
			   ,route;			/* Address to route FLO stuff through */
	enum pkt_type pkt_type;		/* Packet type to use for outgoing PKTs */
	char		password[FIDO_SUBJ_LEN];	/* Areafix password for this node */
	char		pktpwd[FIDO_PASS_LEN+1];	/* Packet password for this node */
	char		comment[64];	/* Comment for this node */
	char		inbox[MAX_PATH+1];
	char		outbox[MAX_PATH+1];
	str_list_t	keys;
	bool		send_notify;
	bool		passive;
	bool		direct;
	enum mail_status status;
#define SBBSECHO_ARCHIVE_NONE	NULL
	arcdef_t*	archive;
} nodecfg_t;

typedef struct {
	char		listpath[MAX_PATH+1];	/* Path to this echolist */
	str_list_t	keys;
	fidoaddr_t 	hub;			/* Where to forward requests */
	bool		forward;
	char		password[FIDO_SUBJ_LEN];	/* Password to use for forwarding req's */
} echolist_t;

typedef struct {
	fidoaddr_t 	dest;
	char		filename[MAX_PATH+1];	/* The full path to the attached file */
} attach_t;

struct zone_mapping {
	uint16_t	zone;
	char *		root;
	char *		domain;
	struct zone_mapping *next;
};

typedef struct {
	char		inbound[MAX_PATH+1]; 	/* Inbound directory */
	char		secure_inbound[MAX_PATH+1];		/* Secure Inbound directory */
	char		outbound[MAX_PATH+1];	/* Outbound directory */
	char		areafile[MAX_PATH+1];	/* AREAS.BBS path/filename */
	char		logfile[MAX_PATH+1];	/* LOG path/filename */
	char		logtime[64];			/* format of log timestamp */
	char		cfgfile[MAX_PATH+1];	/* Configuration path/filename */
	char		temp_dir[MAX_PATH+1];	/* Temporary file directory */
	char		outgoing_sem[MAX_PATH+1];	/* Semaphore file to creat when there's outgoing data */
	str_list_t	sysop_alias_list;		/* List of sysop aliases */
	ulong		maxpktsize				/* Maximum size for packets */
			   ,maxbdlsize;				/* Maximum size for bundles */
	int			log_level;				/* Highest level (lowest severity) */
	int 		badecho;				/* Area to store bad echomail msgs */
	uint		arcdefs 				/* Number of archive definitions */
			   ,nodecfgs				/* Number of nodes with configs */
			   ,listcfgs				/* Number of echolists defined */
			   ,areas					/* Number of areas defined */
			   ;
	char		default_recipient[LEN_ALIAS+1];
	char		areamgr[LEN_ALIAS+1];	/* User to notify of areafix activity */
	arcdef_t*	arcdef; 				/* Each archive definition */
	nodecfg_t*	nodecfg;				/* Each node configuration */
	echolist_t*	listcfg;				/* Each echolist configuration */
	area_t*		area;					/* Each area configuration */
	bool		check_path;				/* Enable circular path detection */
	bool		zone_blind;				/* Pretend zones don't matter when parsing and constructing PATH and SEEN-BY lines (per Wilfred van Velzen, 2:280/464) */
	uint16_t	zone_blind_threshold;	/* Zones below this number (e.g. 4) will be treated as the same zone when zone_blind is enabled */
	bool		secure_echomail;
	bool		strict_packet_passwords;	/* Packet passwords must always match the configured linked-node */
	bool		strip_lf;
	bool		convert_tear;
	bool		fuzzy_zone;
	bool		flo_mailer;				/* Binkley-Style-Outbound / FLO mailer */
	bool		add_from_echolists_only;
	bool		trunc_bundles;
	bool		kill_empty_netmail;
	bool		delete_netmail;
	bool		delete_packets;
	bool		echomail_notify;
	bool		ignore_netmail_dest_addr;
	bool		ignore_netmail_recv_attr;
	bool		ignore_netmail_local_attr;
	bool		use_ftn_domains;
	bool		relay_filtered_msgs;
	ulong		bsy_timeout;
	ulong		bso_lock_attempts;
	ulong		bso_lock_delay;			/* in seconds */
	ulong		max_netmail_age;
	ulong		max_echomail_age;
	struct zone_mapping *zone_map;	// 
} sbbsecho_cfg_t;

char* pktTypeStringList[4];
char* mailStatusStringList[4];

/***********************/
/* Function prototypes */
/***********************/
bool sbbsecho_read_ini(sbbsecho_cfg_t*);
bool sbbsecho_read_ftn_domains(sbbsecho_cfg_t*, const char*);
bool sbbsecho_write_ini(sbbsecho_cfg_t*);
void bail(int code);
fidoaddr_t atofaddr(const char *str);
const char *faddrtoa(const fidoaddr_t*);
int  matchnode(sbbsecho_cfg_t*, fidoaddr_t, int exact);
nodecfg_t* findnodecfg(sbbsecho_cfg_t*, fidoaddr_t, int exact);

