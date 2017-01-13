/* Copyright (C), 2007 by Stephen Hurd */

/* $Id: telnet_io.h,v 1.8 2017/01/13 01:33:22 rswindell Exp $ */

#ifndef _TELNET_IO_H_
#define _TELNET_IO_H_

#ifndef TELNET_NO_DLL
#define TELNET_NO_DLL
#endif
#include "telnet.h"

extern uchar	telnet_local_option[0x100];
extern uchar	telnet_remote_option[0x100];

BYTE*	telnet_interpret(BYTE* inbuf, int inlen, BYTE* outbuf, int *outlen);
BYTE*	telnet_expand(BYTE* inbuf, size_t inlen, BYTE* outbuf, size_t *newlen);
void	request_telnet_opt(uchar cmd, uchar opt);

#endif
