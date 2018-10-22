/* $Id: petdefs.h,v 1.1 2018/10/22 04:18:07 rswindell Exp $ */

/* Commodore/PET definitions */

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
#ifndef _PETDEFS_H_
#define _PETDEFS_H_

#include "gen_defs.h"

enum petscii_char {
	/* Colors */
	PETSCII_BLACK		= 144,
	PETSCII_WHITE		= 5,
	PETSCII_RED			= 28,
	PETSCII_GREEN		= 30,
	PETSCII_BLUE		= 31,
	PETSCII_ORANGE		= 129,		// Dark purple in 80 column mode
	PETSCII_BROWN		= 149,		// Dark yellow in 80 column mode
	PETSCII_YELLOW		= 158,
	PETSCII_CYAN		= 159,		// Light cyan in 80 column mode
	PETSCII_LIGHTRED 	= 150,
	PETSCII_DARKGRAY	= 151,		// Dark cyan in 80 column mode
	PETSCII_MEDIUMGRAY	= 152,
	PETSCII_LIGHTGREEN	= 153,
	PETSCII_LIGHTBLUE	= 154,
	PETSCII_LIGHTGRAY	= 155,
	PETSCII_PURPLE		= 156,
	/* Char-set */
	PETSCII_UPPERLOWER	= 14,
	PETSCII_UPPERGRFX	= 142,
	/* Attributes */
	PETSCII_FLASH_ON	= 15,
	PETSCII_FLASH_OFF	= 143,
	PETSCII_REVERSE_ON	= 18,
	PETSCII_REVERSE_OFF	= 146,
	/* Cursor movement */
	PETSCII_UP			= 145,
	PETSCII_DOWN		= 17,
	PETSCII_LEFT		= 157,
	PETSCII_RIGHT		= 29,
	PETSCII_HOME		= 19,
	PETSCII_CLEAR		= 147,
	PETSCII_INSERT		= 148,
	PETSCII_DELETE		= 20,
	PETSCII_CRLF		= 141,
	/* Symbols (which don't align with ASCII) */
	PETSCII_BRITPOUND	= 92,
	/* Graphic chars */
	PETSCII_LIGHTHASH	= '\xA6',
	PETSCII_MEDIUMHASH	= '\xDE',
	PETSCII_HEAVYHASH	= '\xA9',
	PETSCII_SOLID		= '\xA0',	// Actually inversed solid (empty)
	PETSCII_LEFTHALF	= '\xA1',
	PETSCII_RIGHTHALF	= '\xB6',	// Not quite a full half
	PETSCII_TOPHALF		= '\xB8',	// Not quite a full half
	PETSCII_BOTTOMHALF	= '\xA2',
	PETSCII_CHECKMARK	= '\xBA',
	PETSCII_CROSS		= '\xDB',
	PETSCII_HORZLINE	= '\xC0',
	PETSCII_VERTLINE	= '\xDD',
	PETSCII_LWRRHTBOX	= '\xAC',
	PETSCII_LWRLFTBOX	= '\xBB',
	PETSCII_UPRRHTBOX	= '\xBC',
	PETSCII_UPRLFTBOX	= '\xBE',
	PETSCII_CHECKERBRD	= '\xBF',
	/* Replacement chars (missing ASCII chars) */
	PETSCII_BACKSLASH	= '/',		// the 109 graphics char is an 'M' in shifted/text mode :-(
	PETSCII_BACKTICK	= '\xAD',	// a graphics char actually
	PETSCII_TILDE		= '\xA8',	// a graphics char actually
	PETSCII_UNDERSCORE	= '\xA4',	// a graphics char actually
};

#endif /* Don't add anything after this line */