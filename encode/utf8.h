/* Synchronet UTF-8 translation functions */

/* $Id: utf8.h,v 1.3 2019/07/08 00:07:22 rswindell Exp $ */

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

#ifndef UTF8_H_
#define UTF8_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#define UTF8_MAX_LEN 6	// Longest possible UTF-8 sequence

#if defined(__cplusplus)
extern "C" {
#endif

// Returns true if the string is valid UTF-8
bool utf8_str_is_valid(const char*);
// Normalizes (to ASCII) chars in UTF-8 string 'str', in-place, resulting in string <= original in length
char* utf8_normalize_str(char* str);
// Replace or strip UTF-8 sequences in str
// If table ('tbl') of unicode codepoints if non-NULL is an array of 256 codepoints to map to 8-bit chars
char* utf8_replace_chars(char* str, char (*lookup)(uint32_t), char unsupported_ch, char unsupported_zwch, char error_ch);
// Decode a UTF-8 sequence to a UNICODE code point
int utf8_getc(const char* str, size_t len, uint32_t* codepoint);
// Encode a UNICODE code point into a UTF-8 sequence (str)
int utf8_putc(char* str, size_t len, uint32_t codepoint);

#if defined(__cplusplus)
}
#endif

#endif // Don't add anything after this line
