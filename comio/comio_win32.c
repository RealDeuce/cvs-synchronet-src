/* comio_win32.c */

/* Synchronet Serial Communications I/O Library Functions for Win32 */

/* $Id: comio_win32.c,v 1.8 2018/07/24 01:10:49 rswindell Exp $ */

/****************************************************************************
 * @format.tab-size 4		(Plain Text/Source Code File Header)			*
 * @format.use-tabs true	(see http://www.synchro.net/ptsc_hdr.html)		*
 *																			*
 * Copyright Rob Swindell - http://www.synchro.net/copyright.html			*
 *																			*
 * This library is free software; you can redistribute it and/or			*
 * modify it under the terms of the GNU Lesser General Public License		*
 * as published by the Free Software Foundation; either version 2			*
 * of the License, or (at your option) any later version.					*
 * See the GNU Lesser General Public License for more details: lgpl.txt or	*
 * http://www.fsf.org/copyleft/lesser.html									*
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

#include "comio.h"
#include "genwrap.h"

char* COMIOCALL comVersion(char* str, size_t len)
{
	char revision[16];

	sscanf("$Revision: 1.8 $", "%*s %s", revision);

	safe_snprintf(str,len,"Synchronet Communications I/O Library for "PLATFORM_DESC" v%s", revision);
	return str;
}

COM_HANDLE COMIOCALL comOpen(const char* device)
{
	COM_HANDLE handle;
	COMMTIMEOUTS timeouts;
	DCB	dcb;

	if((handle=CreateFile(device
		,GENERIC_READ|GENERIC_WRITE 	/* Access */
		,0								/* Share mode */
		,NULL							/* Security attributes */
		,OPEN_EXISTING					/* Create access */
		,FILE_ATTRIBUTE_NORMAL			/* File attributes */
		,NULL							/* Template */
		))==INVALID_HANDLE_VALUE)
		return COM_HANDLE_INVALID;


	if(GetCommTimeouts(handle, &timeouts)) {
		timeouts.ReadIntervalTimeout=MAXDWORD;
		timeouts.ReadTotalTimeoutMultiplier=0;
		timeouts.ReadTotalTimeoutConstant=0;		// No-wait read timeout
		timeouts.WriteTotalTimeoutMultiplier=0;
		timeouts.WriteTotalTimeoutConstant=5000;	// 5 seconds
		SetCommTimeouts(handle,&timeouts);
	}

	/* Force N-8-1 mode: */
	if(GetCommState(handle, &dcb)==TRUE) {
		dcb.ByteSize	= 8;
		dcb.Parity		= NOPARITY;
		dcb.StopBits	= ONESTOPBIT;
		SetCommState(handle, &dcb);
	}

	return handle;
}

BOOL COMIOCALL comClose(COM_HANDLE handle)
{
	return CloseHandle(handle);
}

long COMIOCALL comGetBaudRate(COM_HANDLE handle)
{
	DCB dcb;

	if(GetCommState(handle, &dcb)!=TRUE)
		return COM_ERROR;

	return dcb.BaudRate;
}

BOOL COMIOCALL comSetBaudRate(COM_HANDLE handle, unsigned long rate)
{
	DCB dcb;

	if(GetCommState(handle, &dcb)!=TRUE)
		return FALSE;

	dcb.BaudRate=rate;

	return SetCommState(handle, &dcb);
}

int COMIOCALL comGetModemStatus(COM_HANDLE handle)
{
	DWORD status=0;
	
	if(GetCommModemStatus(handle, &status))
		return status;
	else
		return COM_ERROR;
}

BOOL COMIOCALL comRaiseDTR(COM_HANDLE handle)
{
	return EscapeCommFunction(handle, SETDTR);
}

BOOL COMIOCALL comLowerDTR(COM_HANDLE handle)
{
	return EscapeCommFunction(handle, CLRDTR);
}

BOOL COMIOCALL comWriteByte(COM_HANDLE handle, BYTE ch)
{
	DWORD wr=0;

	return WriteFile(handle, &ch, sizeof(ch), &wr, NULL) && wr==sizeof(BYTE);
}

int COMIOCALL comWriteBuf(COM_HANDLE handle, const BYTE* buf, size_t buflen)
{
	DWORD wr=0;

	if(!WriteFile(handle, buf, buflen, &wr, NULL))
		return COM_ERROR;

	return wr;
}

int COMIOCALL comWriteString(COM_HANDLE handle, const char* str)
{
	return comWriteBuf(handle, str, strlen(str));
}

BOOL COMIOCALL comReadByte(COM_HANDLE handle, BYTE* ch)
{
	DWORD rd;

	return ReadFile(handle, ch, sizeof(BYTE), &rd, NULL) && rd==sizeof(BYTE);
}

BOOL COMIOCALL comPurgeInput(COM_HANDLE handle)
{
	return PurgeComm(handle, PURGE_RXCLEAR);
}

BOOL COMIOCALL comPurgeOutput(COM_HANDLE handle)
{
	return PurgeComm(handle, PURGE_TXCLEAR);
}

