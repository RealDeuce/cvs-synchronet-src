/* telnet.c */

/* Synchronet telnet command/option functions */

/* $Id$ */

/****************************************************************************
 * @format.tab-size 4		(Plain Text/Source Code File Header)			*
 * @format.use-tabs true	(see http://www.synchro.net/ptsc_hdr.html)		*
 *																			*
 * Copyright 2004 Rob Swindell - http://www.synchro.net/copyright.html		*
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

#include <stdio.h>		/* sprintf */
#include "gen_defs.h"
#include "telnet.h"

const char* DLLCALL telnet_cmd_desc(uchar cmd)
{
	static char unknown[32];

	switch(cmd) {
    	case TELNET_IAC: 	return("IAC");
    	case TELNET_DONT:	return("DON'T");
    	case TELNET_DO:		return("DO");
    	case TELNET_WONT:	return("WON'T");
    	case TELNET_WILL:	return("WILL");

    	case TELNET_SB:		return("SB");
    	case TELNET_GA:		return("GA");
    	case TELNET_EL:		return("EL");
    	case TELNET_EC:		return("EC");
    	case TELNET_AYT: 	return("AYT");
    	case TELNET_AO:		return("AO");
    	case TELNET_IP:		return("IP");
    	case TELNET_BRK: 	return("BRK");
    	case TELNET_SYNC:	return("SYNC");
    	case TELNET_NOP: 	return("NOP");

        default:
			sprintf(unknown,"%d",cmd);
            return(unknown);
	}
}

char* telnet_option_descriptions[]={
	
	 "Binary Transmission"
	,"Echo"
	,"Reconnection"
	,"Suppress Go Ahead"                   
	,"Approx Message Size Negotiation"
	,"Status"                               
	,"Timing Mark"                          
	,"Remote Controlled Trans and Echo"     
	,"Output Line Width"                   
	,"Output Page Size"                     
	,"Output Carriage-Return Disposition"   /* 10 */
	,"Output Horizontal Tab Stops"          
	,"Output Horizontal Tab Disposition"    
	,"Output Formfeed Disposition"          
	,"Output Vertical Tab Stops"             
	,"Output Vertical Tab Disposition"      
	,"Output Linefeed Disposition"          
	,"Extended ASCII"                       
	,"Logout"                               
	,"Byte Macro"							
	,"Data Entry Terminal"					/* 20 */
	,"SUPDUP"
	,"SUPDUP Output"
	,"Send Location"
	,"Terminal Type"
	,"End of Record"
	,"TACACS User Identification"
	,"Output Marking"
	,"Terminal Location Number"
	,"Telnet 3270 Regime"					
	,"X.3 PAD"								/* 30 */
	,"Negotiate About Window Size"
	,"Terminal Speed"
	,"Remote Flow Control"
	,"Linemode"
	,"X Display Location"
	,"Environment Option"
	,"Authentication Option"
	,"Encryption Option"
	,"New Environment Option"				
	,"TN3270E"								/* 40 */
};

const char* DLLCALL telnet_opt_desc(uchar opt)
{
	static char unknown[32];

	if(opt<sizeof(telnet_option_descriptions)/sizeof(char*))
		return(telnet_option_descriptions[opt]);

	if(opt==TELNET_EXOPL)
		return("Extended Options List");

	sprintf(unknown,"%d",opt);
    return(unknown);
}

uchar DLLCALL telnet_opt_ack(uchar cmd)
{
	switch(cmd) {
		case TELNET_DO:		return TELNET_WILL;
		case TELNET_DONT:	return TELNET_WONT;
		case TELNET_WILL:	return TELNET_DO;
		case TELNET_WONT:	return TELNET_DONT;
	}
	return 0;
}

uchar DLLCALL telnet_opt_nak(uchar cmd)
{
	switch(cmd) {
		case TELNET_DO:		return TELNET_WONT;
		case TELNET_DONT:	return TELNET_WILL;
		case TELNET_WILL:	return TELNET_DONT;
		case TELNET_WONT:	return TELNET_DO;
	}
	return 0;
}
