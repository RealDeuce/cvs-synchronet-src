/* smbdefs.h */

/* Synchronet message base constant and structure definitions */

/* $Id: smbdefs.h,v 1.85 2016/09/07 23:28:50 rswindell Exp $ */

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

#ifndef _SMBDEFS_H
#define _SMBDEFS_H

/* ANSI headers */
#include <stdio.h>
#include <time.h>

/* XPDEV headers */
#include "genwrap.h"	/* truncsp() and get_errno() */
#include "dirwrap.h"	/* MAX_PATH */
#include "filewrap.h"	/* SH_DENYRW */

/* SMBLIB Headers */
#include "md5.h"		/* MD5_DIGEST_SIZE */

/**********/
/* Macros */
/**********/

#define SMB_HEADER_ID	"SMB\x1a"		/* <S> <M> <B> <^Z> */
#define SHD_HEADER_ID	"SHD\x1a"		/* <S> <H> <D> <^Z> */
#define LEN_HEADER_ID	4

#ifndef uchar
	#if defined(TYPEDEF_UCHAR)
		typedef unsigned char uchar;
	#else
		#define uchar unsigned char
	#endif
#endif
#ifdef __GLIBC__
	#include <sys/types.h>
#else
	#ifndef ushort
	#define ushort				unsigned short
	#define ulong				unsigned long
	#define uint				unsigned int
	#endif
#endif

#define HUGE16
#define FAR16
#define REALLOC realloc
#define LMALLOC malloc
#define MALLOC malloc
#define LFREE free
#define FREE free


#define SDT_BLOCK_LEN		256 		/* Size of data blocks */
#define SHD_BLOCK_LEN		256 		/* Size of header blocks */

#define SMB_MAX_HDR_LEN		0xffffU		/* Message header length is 16-bit */

#define SMB_SELFPACK		0			/* Self-packing storage allocation */
#define SMB_FASTALLOC		1			/* Fast allocation */

										/* status.attr bit flags: */
#define SMB_EMAIL			1			/* User numbers stored in Indexes */
#define SMB_HYPERALLOC		2			/* No allocation (also storage value for smb_addmsghdr) */
#define SMB_NOHASH			4			/* Do not calculate or store hashes */

#define SMB_SUCCESS			0			/* Successful result/return code */
#define SMB_FAILURE			-1			/* Generic error (discouraged) */

										/* Standard smblib errors values */
#define SMB_ERR_NOT_OPEN	-100		/* Message base not open */
#define SMB_ERR_HDR_LEN		-101		/* Invalid message header length (>64k) */
#define SMB_ERR_HDR_OFFSET	-102		/* Invalid message header offset */
#define SMB_ERR_HDR_ID		-103		/* Invalid header ID */
#define SMB_ERR_HDR_VER		-104		/* Unsupported version */
#define SMB_ERR_HDR_FIELD	-105		/* Missing header field */
#define SMB_ERR_NOT_FOUND	-110		/* Item not found */
#define SMB_ERR_DAT_OFFSET	-120		/* Invalid data offset (>2GB) */
#define SMB_ERR_DAT_LEN		-121		/* Invalid data length (>2GB) */
#define SMB_ERR_OPEN		-200		/* File open error */
#define SMB_ERR_SEEK		-201		/* File seek/setpos error */
#define SMB_ERR_LOCK		-202		/* File lock error */
#define SMB_ERR_READ		-203		/* File read error */
#define SMB_ERR_WRITE		-204		/* File write error */
#define SMB_ERR_TIMEOUT		-205		/* File operation timed-out */
#define SMB_ERR_FILE_LEN	-206		/* File length invalid */
#define SMB_ERR_DELETE		-207		/* File deletion error */
#define SMB_ERR_UNLOCK		-208		/* File unlock error */
#define SMB_ERR_MEM			-300		/* Memory allocation error */

#define SMB_DUPE_MSG		1			/* Duplicate message detected by smb_addcrc() */

										/* Time zone macros for when_t.zone */
#define DAYLIGHT			0x8000		/* Daylight savings is active */
#define US_ZONE 			0x4000		/* U.S. time zone */
#define WESTERN_ZONE		0x2000		/* Non-standard zone west of UT */
#define EASTERN_ZONE		0x1000		/* Non-standard zone east of UT */

										/* US Time Zones (standard) */
#define AST 				0x40F0		/* Atlantic 			(-04:00) */
#define EST 				0x412C		/* Eastern				(-05:00) */
#define CST 				0x4168		/* Central				(-06:00) */
#define MST 				0x41A4		/* Mountain 			(-07:00) */
#define PST 				0x41E0		/* Pacific				(-08:00) */
#define YST 				0x421C		/* Yukon				(-09:00) */
#define HST 				0x4258		/* Hawaii/Alaska		(-10:00) */
#define BST 				0x4294		/* Bering				(-11:00) */

										/* US Time Zones (daylight) */
#define ADT 				0xC0F0		/* Atlantic 			(-03:00) */
#define EDT 				0xC12C		/* Eastern				(-04:00) */
#define CDT 				0xC168		/* Central				(-05:00) */
#define MDT 				0xC1A4		/* Mountain 			(-06:00) */
#define PDT 				0xC1E0		/* Pacific				(-07:00) */
#define YDT 				0xC21C		/* Yukon				(-08:00) */
#define HDT 				0xC258		/* Hawaii/Alaska		(-09:00) */
#define BDT 				0xC294		/* Bering				(-10:00) */

										/* Non-standard Time Zones */
#define MID 				0x2294		/* Midway				(-11:00) */
#define VAN 				0x21E0		/* Vancouver			(-08:00) */
#define EDM 				0x21A4		/* Edmonton 			(-07:00) */
#define WIN 				0x2168		/* Winnipeg 			(-06:00) */
#define BOG 				0x212C		/* Bogota				(-05:00) */
#define CAR 				0x20F0		/* Caracas				(-04:00) */
#define RIO 				0x20B4		/* Rio de Janeiro		(-03:00) */
#define FER 				0x2078		/* Fernando de Noronha	(-02:00) */
#define AZO 				0x203C		/* Azores				(-01:00) */
#define LON 				0x1000		/* London				(+00:00) */
#define BER 				0x103C		/* Berlin				(+01:00) */
#define ATH 				0x1078		/* Athens				(+02:00) */
#define MOS 				0x10B4		/* Moscow				(+03:00) */
#define DUB 				0x10F0		/* Dubai				(+04:00) */
#define KAB 				0x110E		/* Kabul				(+04:30) */
#define KAR 				0x112C		/* Karachi				(+05:00) */
#define BOM 				0x114A		/* Bombay				(+05:30) */
#define KAT 				0x1159		/* Kathmandu			(+05:45) */
#define DHA 				0x1168		/* Dhaka				(+06:00) */
#define BAN 				0x11A4		/* Bangkok				(+07:00) */
#define HON 				0x11E0		/* Hong Kong			(+08:00) */
#define TOK 				0x121C		/* Tokyo				(+09:00) */
#define SYD 				0x1258		/* Sydney				(+10:00) */
#define NOU 				0x1294		/* Noumea				(+11:00) */
#define WEL 				0x12D0		/* Wellington			(+12:00) */

#define OTHER_ZONE(zone) (zone<=1000 && zone>=-1000)

										/* Valid hfield_t.types */
#define SENDER				0x00
#define SENDERAGENT 		0x01
#define SENDERNETTYPE		0x02
#define SENDERNETADDR		0x03		/* Note: SENDERNETTYPE may be NET_NONE and this field present and contain a valid string */
#define SENDEREXT			0x04
#define SENDERPOS			0x05
#define SENDERORG			0x06
#define SENDERIPADDR		0x07		/* for tracing */
#define SENDERHOSTNAME		0x08		/* for tracing */
#define SENDERPROTOCOL		0x09		/* for tracing */
#define SENDERPORT_BIN		0x0a		/* deprecated */
#define SENDERPORT			0x0b		/* for tracing */

										/* Used for the SMTP Originator-Info header field: */
#define SENDERUSERID		0x0c		/* user-id */
#define SENDERTIME			0x0d		/* authentication/connection time */
#define SENDERSERVER		0x0e		/* server hostname that authenticed user */

#define AUTHOR				0x10
#define AUTHORAGENT 		0x11
#define AUTHORNETTYPE		0x12
#define AUTHORNETADDR		0x13
#define AUTHOREXT			0x14
#define AUTHORPOS			0x15
#define AUTHORORG			0x16

#define REPLYTO 			0x20
#define REPLYTOAGENT		0x21
#define REPLYTONETTYPE		0x22
#define REPLYTONETADDR		0x23
#define REPLYTOEXT			0x24
#define REPLYTOPOS			0x25
#define REPLYTOORG			0x26

#define RECIPIENT			0x30
#define RECIPIENTAGENT		0x31
#define RECIPIENTNETTYPE	0x32
#define RECIPIENTNETADDR	0x33	/* Note: RECIPIENTNETTYPE may be NET_NONE and this field present and contain a valid string */
#define RECIPIENTEXT		0x34
#define RECIPIENTPOS		0x35
#define RECIPIENTORG		0x36

#define FORWARDTO			0x40
#define FORWARDTOAGENT		0x41
#define FORWARDTONETTYPE	0x42
#define FORWARDTONETADDR	0x43
#define FORWARDTOEXT		0x44
#define FORWARDTOPOS		0x45
#define FORWARDTOORG		0x46

#define FORWARDED			0x48

#if 0	/* Deprecating the following fields: (Jan-2009) never used */

#define RECEIVEDBY			0x50
#define RECEIVEDBYAGENT 	0x51
#define RECEIVEDBYNETTYPE	0x52
#define RECEIVEDBYNETADDR	0x53
#define RECEIVEDBYEXT		0x54
#define RECEIVEDBYPOS		0x55
#define RECEIVEDBYORG		0x56

#define RECEIVED			0x58

#endif

#define SUBJECT 			0x60	/* or filename */
#define SMB_SUMMARY 		0x61	/* or file description */
#define SMB_COMMENT 		0x62
#define SMB_CARBONCOPY		0x63
#define SMB_GROUP			0x64
#define SMB_EXPIRATION		0x65
#define SMB_PRIORITY		0x66
#define SMB_COST			0x67
#define	SMB_EDITOR			0x68

#define FILEATTACH			0x70
#define DESTFILE			0x71
#define FILEATTACHLIST		0x72
#define DESTFILELIST		0x73
#define FILEREQUEST 		0x74
#define FILEPASSWORD		0x75
#define FILEREQUESTLIST 	0x76
#define FILEPASSWORDLIST	0x77

#define IMAGEATTACH 		0x80
#define ANIMATTACH			0x81
#define FONTATTACH			0x82
#define SOUNDATTACH 		0x83
#define PRESENTATTACH		0x84
#define VIDEOATTACH 		0x85
#define APPDATAATTACH		0x86

#define IMAGETRIGGER		0x90
#define ANIMTRIGGER 		0x91
#define FONTTRIGGER 		0x92
#define SOUNDTRIGGER		0x93
#define PRESENTTRIGGER		0x94
#define VIDEOTRIGGER		0x95
#define APPDATATRIGGER		0x96

#define FIDOCTRL			0xa0
#define FIDOAREA			0xa1
#define FIDOSEENBY			0xa2
#define FIDOPATH			0xa3
#define FIDOMSGID			0xa4
#define FIDOREPLYID 		0xa5
#define FIDOPID 			0xa6
#define FIDOFLAGS			0xa7
#define FIDOTID 			0xa8

#define RFC822HEADER		0xb0
#define RFC822MSGID 		0xb1
#define RFC822REPLYID		0xb2
#define RFC822TO			0xb3
#define RFC822FROM			0xb4
#define RFC822REPLYTO		0xb5

#define USENETPATH			0xc0
#define USENETNEWSGROUPS	0xc1

#define SMTPCOMMAND			0xd0		/* Arbitrary SMTP command */
#define SMTPREVERSEPATH		0xd1		/* MAIL FROM: argument, "reverse path" */
#define SMTPFORWARDPATH		0xd2		/* RCPT TO: argument, "forward path" */
#define SMTPRECEIVED		0xd3		/* SMTP "Received" information */

#define SMTPSYSMSG			0xd8		/* for delivery failure notification */

#define UNKNOWN 			0xf1
#define UNKNOWNASCII		0xf2
#define UNUSED				0xff

										/* Valid dfield_t.types */
#define TEXT_BODY			0x00
#define TEXT_SOUL			0x01
#define TEXT_TAIL			0x02
#define TEXT_WING			0x03
#define IMAGEEMBED			0x20
#define ANIMEMBED			0x21
#define FONTEMBED			0x22
#define SOUNDEMBED			0x23
#define PRESENTEMBED		0x24
#define VIDEOEMBED			0x25
#define APPDATAEMBED		0x26
#define UNUSED				0xff


										/* Message attributes */
#define MSG_PRIVATE 		(1<<0)
#define MSG_READ			(1<<1)
#define MSG_PERMANENT		(1<<2)
#define MSG_LOCKED			(1<<3)
#define MSG_DELETE			(1<<4)
#define MSG_ANONYMOUS		(1<<5)
#define MSG_KILLREAD		(1<<6)
#define MSG_MODERATED		(1<<7)
#define MSG_VALIDATED		(1<<8)
#define MSG_REPLIED			(1<<9)		/* User replied to this message */
#define MSG_NOREPLY			(1<<10)		/* No replies (or bounces) should be sent to the sender */

										/* Auxillary header attributes */
#define MSG_FILEREQUEST 	(1<<0)		/* File request */
#define MSG_FILEATTACH		(1<<1)		/* File(s) attached to Msg */
#define MSG_TRUNCFILE		(1<<2)		/* Truncate file(s) when sent */
#define MSG_KILLFILE		(1<<3)		/* Delete file(s) when sent */
#define MSG_RECEIPTREQ		(1<<4)		/* Return receipt requested */
#define MSG_CONFIRMREQ		(1<<5)		/* Confirmation receipt requested */
#define MSG_NODISP			(1<<6)		/* Msg may not be displayed to user */

										/* Message network attributes */
#define MSG_LOCAL			(1<<0)		/* Msg created locally */
#define MSG_INTRANSIT		(1<<1)		/* Msg is in-transit */
#define MSG_SENT			(1<<2)		/* Sent to remote */
#define MSG_KILLSENT		(1<<3)		/* Kill when sent */
#define MSG_ARCHIVESENT 	(1<<4)		/* Archive when sent */
#define MSG_HOLD			(1<<5)		/* Hold for pick-up */
#define MSG_CRASH			(1<<6)		/* Crash */
#define MSG_IMMEDIATE		(1<<7)		/* Send Msg now, ignore restrictions */
#define MSG_DIRECT			(1<<8)		/* Send directly to destination */
#define MSG_GATE			(1<<9)		/* Send via gateway */
#define MSG_ORPHAN			(1<<10) 	/* Unknown destination */
#define MSG_FPU 			(1<<11) 	/* Force pickup */
#define MSG_TYPELOCAL		(1<<12) 	/* Msg is for local use only */
#define MSG_TYPEECHO		(1<<13) 	/* Msg is for conference distribution */
#define MSG_TYPENET 		(1<<14) 	/* Msg is direct network mail */


enum smb_net_type {
     NET_NONE				/* Local message */
    ,NET_UNKNOWN			/* Unknown network type */
    ,NET_FIDO				/* FidoNet address, faddr_t format (4D) */
    ,NET_POSTLINK			/* Imported with UTI driver */
    ,NET_QWK				/* QWK networked messsage */
	,NET_INTERNET			/* Internet e-mail, netnews, etc. */
	,NET_WWIV				/* unused */
	,NET_MHS				/* unused */
	,NET_FIDO_ASCII			/* FidoNet address, ASCIIZ format (e.g. 5D) - unused and deprecated */

/* Add new ones here */

    ,NET_TYPES
};

enum smb_agent_type {
     AGENT_PERSON
    ,AGENT_PROCESS			/* unknown process type */
	,AGENT_SMBUTIL			/* imported via Synchronet SMBUTIL */
	,AGENT_SMTPSYSMSG		/* Synchronet SMTP server system message */

/* Add new ones here */

    ,AGENT_TYPES
};

enum smb_xlat_type {
     XLAT_NONE              /* No translation/End of translation list */
    ,XLAT_ENCRYPT           /* Encrypted data */
    ,XLAT_ESCAPED           /* 7-bit ASCII escaping for ctrl and 8-bit data */
    ,XLAT_HUFFMAN           /* Static and adaptive Huffman coding compression */
    ,XLAT_LZW               /* Limpel/Ziv/Welch compression */
    ,XLAT_MLZ78             /* Modified LZ78 compression */
    ,XLAT_RLE               /* Run length encoding compression */
    ,XLAT_IMPLODE           /* Implode compression (PkZIP) */
    ,XLAT_SHRINK            /* Shrink compression (PkZIP) */
	,XLAT_LZH				/* LHarc (LHA) Dynamic Huffman coding */

/* Add new ones here */

    ,XLAT_TYPES
};


/************/
/* Typedefs */
/************/

#if defined(_WIN32) || defined(__BORLANDC__) 
	#define PRAGMA_PACK
#endif

#if defined(PRAGMA_PACK) || defined(__WATCOMC__)
	#define _PACK
#else
	#define _PACK __attribute__ ((packed))
#endif

#if defined(PRAGMA_PACK)
	#pragma pack(push,1)	/* Disk image structures must be packed */
#endif

typedef struct _PACK {		/* Time with time-zone */

	uint32_t	time;			/* Local time (unix format) */
	int16_t		zone;			/* Time zone */

} when_t;

typedef struct _PACK {		/* Index record */

	uint16_t	to; 			/* 16-bit CRC of recipient name (lower case) */
	uint16_t	from;			/* 16-bit CRC of sender name (lower case) */
	uint16_t	subj;			/* 16-bit CRC of subject (lower case, w/o RE:) */
	uint16_t	attr;			/* attributes (read, permanent, etc.) */
	uint32_t	offset; 		/* offset into header file */
	uint32_t	number; 		/* number of message (1 based) */
	uint32_t	time;			/* time/date message was imported/posted */

} idxrec_t;

										/* valid bits in hash_t.flags		*/
#define SMB_HASH_CRC16			(1<<0)	/* CRC-16 hash is valid				*/
#define SMB_HASH_CRC32			(1<<1)	/* CRC-32 hash is valid				*/
#define SMB_HASH_MD5			(1<<2)	/* MD5 digest is valid				*/
#define SMB_HASH_MASK			(SMB_HASH_CRC16|SMB_HASH_CRC32|SMB_HASH_MD5)
								
#define SMB_HASH_MARKED			(1<<4)	/* Used by smb_findhash()			*/

#define SMB_HASH_STRIP_CTRL_A	(1<<5)	/* Strip Ctrl-A codes first			*/
#define SMB_HASH_STRIP_WSP		(1<<6)	/* Strip white-space chars first	*/
#define SMB_HASH_LOWERCASE		(1<<7)	/* Convert A-Z to a-z first			*/
#define SMB_HASH_PROC_MASK		(SMB_HASH_STRIP_CTRL_A|SMB_HASH_STRIP_WSP|SMB_HASH_LOWERCASE)
#define SMB_HASH_PROC_COMP_MASK	(SMB_HASH_STRIP_WSP|SMB_HASH_LOWERCASE)

enum smb_hash_source_type {
	 SMB_HASH_SOURCE_BODY
	,SMB_HASH_SOURCE_MSG_ID
	,SMB_HASH_SOURCE_FTN_ID
	,SMB_HASH_SOURCE_SUBJECT

/* Add new ones here (max value of 31) */

    ,SMB_HASH_SOURCE_TYPES
};

#define SMB_HASH_SOURCE_MASK	0x1f
#define SMB_HASH_SOURCE_NONE	0
#define SMB_HASH_SOURCE_ALL		0xff
								/* These are the hash sources stored/compared for duplicate message detection: */
#define SMB_HASH_SOURCE_DUPE	((1<<SMB_HASH_SOURCE_BODY)|(1<<SMB_HASH_SOURCE_MSG_ID)|(1<<SMB_HASH_SOURCE_FTN_ID))
								/* These are the hash sources stored/compared for SPAM message detection: */
#define SMB_HASH_SOURCE_SPAM	((1<<SMB_HASH_SOURCE_BODY))

typedef struct _PACK {

	uint32_t	number;					/* Message number */
	uint32_t	time;					/* Local time of fingerprinting */
	uint32_t	length;					/* Length (in bytes) of source */
	uchar		source;					/* SMB_HASH_SOURCE* (in low 5-bits) */
	uchar		flags;					/* indications of valid hashes and pre-processing */
	uint16_t	crc16;					/* CRC-16 of source */
	uint32_t	crc32;					/* CRC-32 of source */
	uchar		md5[MD5_DIGEST_SIZE];	/* MD5 digest of source */
	uchar		reserved[28];			/* sizeof(hash_t) = 64 */

} hash_t;

typedef struct _PACK {		/* Message base header (fixed portion) */

    uchar		id[LEN_HEADER_ID];	/* SMB<^Z> */
    uint16_t	version;        /* version number (initially 100h for 1.00) */
    uint16_t	length;         /* length including this struct */

} smbhdr_t;

typedef struct _PACK {		/* Message base status header */

	uint32_t	last_msg;		/* last message number */
	uint32_t	total_msgs; 	/* total messages */
	uint32_t	header_offset;	/* byte offset to first header record */
	uint32_t	max_crcs;		/* Maximum number of CRCs to keep in history */
    uint32_t	max_msgs;       /* Maximum number of message to keep in sub */
    uint16_t	max_age;        /* Maximum age of message to keep in sub (in days) */
	uint16_t	attr;			/* Attributes for this message base (SMB_HYPER,etc) */

} smbstatus_t;

typedef struct _PACK {		/* Message header */

	/* 00 */ uchar		id[LEN_HEADER_ID];	/* SHD<^Z> */
    /* 04 */ uint16_t	type;				/* Message type (normally 0) */
    /* 06 */ uint16_t	version;			/* Version of type (initially 100h for 1.00) */
    /* 08 */ uint16_t	length;				/* Total length of fixed record + all fields */
	/* 0a */ uint16_t	attr;				/* Attributes (bit field) (duped in SID) */
	/* 0c */ uint32_t	auxattr;			/* Auxillary attributes (bit field) */
    /* 10 */ uint32_t	netattr;			/* Network attributes */
	/* 14 */ when_t		when_written;		/* Date/time/zone message was written */
	/* 1a */ when_t		when_imported;		/* Date/time/zone message was imported */
    /* 20 */ uint32_t	number;				/* Message number */
    /* 24 */ uint32_t	thread_back;		/* Message number for backwards threading (aka thread_orig) */
    /* 28 */ uint32_t	thread_next;		/* Next message in thread */
    /* 2c */ uint32_t	thread_first;		/* First reply to this message */
	/* 30 */ uint16_t	delivery_attempts;	/* Delivery attempt counter */
	/* 32 */ uchar		reserved[2];		/* Reserved for future use */
	/* 34 */ uint32_t	thread_id;			/* Number of original message in thread (or 0 if unknown) */
	/* 38 */ uint32_t	times_downloaded;	/* Total number of times downloaded (moved Mar-6-2012) */
	/* 3c */ uint32_t	last_downloaded;	/* Date/time of last download (moved Mar-6-2012) */
    /* 40 */ uint32_t	offset;				/* Offset for buffer into data file (0 or mod 256) */
	/* 44 */ uint16_t	total_dfields;		/* Total number of data fields */

} msghdr_t;

#define thread_orig	thread_back	/* for backwards compatibility with older code */

typedef struct _PACK {		/* Data field */

	uint16_t	type;			/* Type of data field */
    uint32_t	offset;         /* Offset into buffer */ 
    uint32_t	length;         /* Length of data field */

} dfield_t;

typedef struct _PACK {		/* Header field */

	uint16_t	type;
	uint16_t	length; 		/* Length of buffer */

} hfield_t;

typedef struct _PACK {		/* FidoNet address (zone:net/node.point) */

	uint16_t	zone;
	uint16_t	net;
	uint16_t	node;
	uint16_t	point;

} fidoaddr_t;

#if defined(PRAGMA_PACK)
#pragma pack(pop)		/* original packing */
#endif

typedef struct {		/* Network (type and address) */

    uint16_t	type;	// One of enum smb_net_type
	void*		addr;

} net_t;

								/* Valid bits in smbmsg_t.flags					*/
#define MSG_FLAG_HASHED	(1<<0)	/* Message has been hashed with smb_hashmsg()	*/

typedef struct {				/* Message */

	idxrec_t	idx;			/* Index */
	msghdr_t	hdr;			/* Header record (fixed portion) */
	char		*to,			/* To name */
				*to_ext,		/* To extension */
				*from,			/* From name */
				*from_ext,		/* From extension */
				*from_org,		/* From organization */
				*from_ip,		/* From IP address (e.g. "192.168.1.2") */
				*from_host,		/* From host name */
				*from_prot,		/* From protocol (e.g. "Telnet", "NNTP", "HTTP", etc.) */
				*replyto,		/* Reply-to name */
				*replyto_ext,	/* Reply-to extension */
				*id,			/* RFC822 Message-ID */
				*reply_id,		/* RFC822 Reply-ID */
				*forward_path,	/* SMTP forward-path (RCPT TO: argument) */
				*reverse_path,	/* SMTP reverse-path (MAIL FROM: argument) */
				*path,			/* USENET Path */
				*newsgroups,	/* USENET Newsgroups */
				*ftn_pid,		/* FTN PID */
				*ftn_tid,		/* FTN TID */
				*ftn_area,		/* FTN AREA */
				*ftn_flags,		/* FTN FLAGS */
				*ftn_msgid,		/* FTN MSGID */
				*ftn_reply;		/* FTN REPLY */
	char*		summary;		/* Summary  */
	char*		subj;			/* Subject  */
	uint16_t	to_agent,		/* Type of agent message is to */
				from_agent, 	/* Type of agent message is from */
				replyto_agent;	/* Type of agent replies should be sent to */
	net_t		to_net, 		/* Destination network type and address */
                from_net,       /* Origin network address */
                replyto_net;    /* Network type and address for replies */
	uint16_t	total_hfields;	/* Total number of header fields */
	hfield_t	*hfield;		/* Header fields (fixed length portion) */
	void		**hfield_dat;	/* Header fields (variable length portion) */
	dfield_t	*dfield;		/* Data fields (fixed length portion) */
	int32_t		offset; 		/* Offset (number of records) into index */
	BOOL		forwarded;		/* Forwarded from agent to another */
	uint32_t	expiration; 	/* Message will expire on this day (if >0) */
	uint32_t	priority;		/* Message priority (0 is lowest) */
	uint32_t	cost;			/* Cost to download/read */
	uint32_t	flags;			/* Various smblib run-time flags (see MSG_FLAG_*) */

} smbmsg_t;

typedef struct {				/* Message base */

    char		file[128];      /* Path and base filename (no extension) */
    FILE*		sdt_fp;			/* File pointer for data (.sdt) file */
    FILE*		shd_fp;			/* File pointer for header (.shd) file */
    FILE*		sid_fp;			/* File pointer for index (.sid) file */
    FILE*		sda_fp;			/* File pointer for data allocation (.sda) file */
    FILE*		sha_fp;			/* File pointer for header allocation (.sha) file */
	FILE*		hash_fp;		/* File pointer for hash (.hash) file */
	uint32_t	retry_time; 	/* Maximum number of seconds to retry opens/locks */
	uint32_t	retry_delay;	/* Time-slice yield (milliseconds) while retrying */
	smbstatus_t status; 		/* Status header record */
	BOOL		locked;			/* SMB header is locked */
	char		last_error[MAX_PATH*2];		/* Last error message */

	/* Private member variables (not initialized by or used by smblib) */
	uint32_t	subnum;			/* Sub-board number */
	uint32_t	msgs;			/* Number of messages loaded (for user) */
	uint32_t	curmsg;			/* Current message number (for user) */

} smb_t;

#endif /* Don't add anything after this #endif statement */
