/* Copyright (C), 2007 by Stephen Hurd */

/* $Id: telnet_io.h,v 1.9 2019/07/10 22:28:52 deuce Exp $ */

#ifndef _TELNET_IO_H_
#define _TELNET_IO_H_

#ifndef TELNET_NO_DLL
#define TELNET_NO_DLL
#endif
#include <cterm.h>
#include "telnet.h"

extern uchar	telnet_local_option[0x100];
extern uchar	telnet_remote_option[0x100];

BYTE*	telnet_interpret(BYTE* inbuf, int inlen, BYTE* outbuf, int *outlen, cterm_emulation_t emu);
BYTE*	telnet_expand(BYTE* inbuf, size_t inlen, BYTE* outbuf, size_t *newlen);
void	request_telnet_opt(uchar cmd, uchar opt);

#endif
