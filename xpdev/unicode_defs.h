/* Synchronet Unicode definitions */

/* $Id: unicode_defs.h,v 1.6 2019/07/11 21:36:35 rswindell Exp $ */

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

#ifndef UNICODE_DEFS_H_
#define UNICODE_DEFS_H_

enum unicode_codepoint {
	UNICODE_UNDEFINED = 0x0000,	// UNICODE_NULL() is defined

	UNICODE_NEXT_LINE = 0x0085,
	UNICODE_REVERSE_LINE_FEED = 0x008D,
	UNICODE_NO_BREAK_SPACE = 0x00A0,
	UNICODE_INVERTED_EXCLAMATION_MARK = 0x00A1,
	UNICODE_CENT_SIGN = 0x00A2,
	UNICODE_POUND_SIGN = 0x00A3,
	UNICODE_CURRENCY_SIGN = 0x00A4,
	UNICODE_YEN_SIGN = 0x00A5,
	UNICODE_BROKEN_BAR = 0x00A6,
	UNICODE_SECTION_SIGN = 0x00A7,
	UNICODE_COPYRIGHT_SIGN = 0x0A9,
	UNICODE_FEMININE_ORDINAL_INDICATOR = 0x00AA,
	UNICODE_LEFT_POINTING_DOUBLE_ANGLE_QUOTATION_MARK = 0x00AB,
	UNICODE_NOT_SIGN = 0x00AC,
	UNICODE_SOFT_HYPHEN = 0x00AD,
	UNICODE_REGISTERED_SIGN = 0x00AE,
	UNICODE_MACRON = 0x00AF,
	UNICODE_DEGREE_SIGN = 0x00B0,
	UNICODE_PLUS_MINUS_SIGN = 0x00B1,
	UNICODE_SUPERSCRIPT_TWO = 0x00B2,
	UNICODE_ACUTE_ACCENT = 0x00B4,
	UNICODE_MICRO_SIGN = 0x00B5,
	UNICODE_PILCROW_SIGN = 0x00B6,	// Paragraph mark
	UNICODE_MIDDLE_DOT = 0x00B7,
	UNICODE_MASCULINE_ORDINAL_INDICATOR = 0x00BA,
	UNICODE_RIGHT_POINTING_DOUBLE_ANGLE_QUOTATION_MARK = 0x00BB,
	UNICODE_VULGAR_FRACTION_ONE_QUARTER = 0x00BC,
	UNICODE_VULGAR_FRACTION_ONE_HALF = 0x00BD,

	UNICODE_INVERTED_QUESTION_MARK = 0x00BF,

	UNICODE_LATIN_CAPITAL_LETTER_A_WITH_GRAVE = 0x00C0,
	UNICODE_LATIN_CAPITAL_LETTER_A_WITH_ACUTE = 0x00C1,
	UNICODE_LATIN_CAPITAL_LETTER_A_WITH_CIRCUMFLEX = 0x00C2,
	UNICODE_LATIN_CAPITAL_LETTER_A_WITH_TILDE = 0x00C3,
	UNICODE_LATIN_CAPITAL_LETTER_A_WITH_DIAERESIS = 0x00C4,
	UNICODE_LATIN_CAPITAL_LETTER_A_WITH_RING_ABOVE = 0x00C5,
	UNICODE_LATIN_CAPITAL_LETTER_AE = 0x00C6,
	UNICODE_LATIN_CAPITAL_LETTER_C_WITH_CEDILLA = 0x00C7,
	UNICODE_LATIN_CAPITAL_LETTER_E_WITH_GRAVE = 0x00C8,
	UNICODE_LATIN_CAPITAL_LETTER_E_WITH_ACUTE = 0x00C9,
	UNICODE_LATIN_CAPITAL_LETTER_E_WITH_CIRCUMFLEX = 0x00CA,
	UNICODE_LATIN_CAPITAL_LETTER_E_WITH_DIAERESIS = 0x00CB,
	UNICODE_LATIN_CAPITAL_LETTER_I_WITH_GRAVE = 0x00CC,
	UNICODE_LATIN_CAPITAL_LETTER_I_WITH_ACUTE = 0x00CD,
	UNICODE_LATIN_CAPITAL_LETTER_I_WITH_CIRCUMFLEX = 0x00CE,
	UNICODE_LATIN_CAPITAL_LETTER_I_WITH_DIAERESIS = 0x00CF,
	UNICODE_LATIN_CAPITAL_LETTER_ETH = 0x00D0,
	UNICODE_LATIN_CAPITAL_LETTER_N_WITH_TILDE = 0x00D1,
	UNICODE_LATIN_CAPITAL_LETTER_O_WITH_GRAVE = 0x00D2,
	UNICODE_LATIN_CAPITAL_LETTER_O_WITH_ACUTE = 0x00D3,
	UNICODE_LATIN_CAPITAL_LETTER_O_WITH_CIRCUMFLEX = 0x00D4,
	UNICODE_LATIN_CAPITAL_LETTER_O_WITH_TILDE = 0x00D5,
	UNICODE_LATIN_CAPITAL_LETTER_O_WITH_DIAERESIS = 0x00D6,
	UNICODE_MULTIPLICATION_SIGN = 0x00D7,
	UNICODE_LATIN_CAPITAL_LETTER_O_WITH_STROKE = 0x00D8,
	UNICODE_LATIN_CAPITAL_LETTER_U_WITH_GRAVE = 0x00D9,
	UNICODE_LATIN_CAPITAL_LETTER_U_WITH_ACUTE = 0x00DA,
	UNICODE_LATIN_CAPITAL_LETTER_U_WITH_CIRCUMFLEX = 0x00DB,
	UNICODE_LATIN_CAPITAL_LETTER_U_WITH_DIAERESIS = 0x00DC,
	UNICODE_LATIN_CAPITAL_LETTER_Y_WITH_ACUTE = 0x00DD,
	UNICODE_LATIN_CAPITAL_LETTER_THORN = 0x00DE,
	UNICODE_LATIN_SMALL_LETTER_SHARP_S = 0x00DF,
	UNICODE_LATIN_SMALL_LETTER_A_WITH_GRAVE = 0x00E0,
	UNICODE_LATIN_SMALL_LETTER_A_WITH_ACUTE = 0x00E1,
	UNICODE_LATIN_SMALL_LETTER_A_WITH_CIRCUMFLEX = 0x00E2,
	UNICODE_LATIN_SMALL_LETTER_A_WITH_TILDE = 0x00E3,
	UNICODE_LATIN_SMALL_LETTER_A_WITH_DIAERESIS = 0x00E4,
	UNICODE_LATIN_SMALL_LETTER_A_WITH_RING_ABOVE = 0x00E5,
	UNICODE_LATIN_SMALL_LETTER_AE = 0x00E6,
	UNICODE_LATIN_SMALL_LETTER_C_WITH_CEDILLA = 0x00E7,
	UNICODE_LATIN_SMALL_LETTER_E_WITH_GRAVE = 0x00E8,
	UNICODE_LATIN_SMALL_LETTER_E_WITH_ACUTE = 0x00E9,
	UNICODE_LATIN_SMALL_LETTER_E_WITH_CIRCUMFLEX = 0x00EA,
	UNICODE_LATIN_SMALL_LETTER_E_WITH_DIAERESIS = 0x00EB,
	UNICODE_LATIN_SMALL_LETTER_I_WITH_GRAVE = 0x00EC,
	UNICODE_LATIN_SMALL_LETTER_I_WITH_ACUTE = 0x00ED,
	UNICODE_LATIN_SMALL_LETTER_I_WITH_CIRCUMFLEX = 0x00EE,
	UNICODE_LATIN_SMALL_LETTER_I_WITH_DIAERESIS = 0x00EF,
	UNICODE_LATIN_SMALL_LETTER_ETH = 0x00F0,
	UNICODE_LATIN_SMALL_LETTER_N_WITH_TILDE = 0x00F1,
	UNICODE_LATIN_SMALL_LETTER_O_WITH_GRAVE = 0x00F2,
	UNICODE_LATIN_SMALL_LETTER_O_WITH_ACUTE = 0x00F3,
	UNICODE_LATIN_SMALL_LETTER_O_WITH_CIRCUMFLEX = 0x00F4,
	UNICODE_LATIN_SMALL_LETTER_O_WITH_TILDE = 0x00F5,
	UNICODE_LATIN_SMALL_LETTER_O_WITH_DIAERESIS = 0x00F6,
	UNICODE_DIVISION_SIGN = 0x00F7,
	UNICODE_LATIN_SMALL_LETTER_O_WITH_STROKE = 0x00F8,
	UNICODE_LATIN_SMALL_LETTER_U_WITH_GRAVE = 0x00F9,
	UNICODE_LATIN_SMALL_LETTER_U_WITH_ACUTE = 0x00FA,
	UNICODE_LATIN_SMALL_LETTER_U_WITH_CIRCUMFLEX = 0x00FB,
	UNICODE_LATIN_SMALL_LETTER_U_WITH_DIAERESIS = 0x00FC,
	UNICODE_LATIN_SMALL_LETTER_Y_WITH_ACUTE = 0x00FD,
	UNICODE_LATIN_SMALL_LETTER_THORN = 0x00FE,
	UNICODE_LATIN_SMALL_LETTER_Y_WITH_DIAERESIS = 0x00FF,
	UNICODE_LATIN_CAPITAL_LETTER_Y_WITH_DIAERESIS = 0x0178,
	UNICODE_LATIN_SMALL_LETTER_F_WITH_HOOK = 0x0192,

	UNICODE_GREEK_CAPITAL_LETTER_HETA = 0x0370,
	UNICODE_GREEK_SMALL_LETTER_HETA = 0x0371,
	UNICODE_GREEK_CAPITAL_LETTER_ARCHAIC_SAMPI = 0x0372,
	UNICODE_GREEK_SMALL_LETTER_ARCHAIC_SAMPI = 0x0373,
	UNICODE_GREEK_NUMERAL_SIGN = 0x0374,
	UNICODE_GREEK_LOWER_NUMERAL_SIGN = 0x0375,
	UNICODE_GREEK_CAPITAL_LETTER_PAMPHYLIAN_DIGAMMA = 0x0376,
	UNICODE_GREEK_SMALL_LETTER_PAMPHYLIAN_DIGAMMA = 0x0377,
	UNICODE_GREEK_YPOGEGRAMMENI = 0x037A,
	UNICODE_GREEK_SMALL_REVERSED_LUNATE_SIGMA_SYMBOL = 0x037B,
	UNICODE_GREEK_SMALL_DOTTED_LUNATE_SIGMA_SYMBOL = 0x037C,
	UNICODE_GREEK_SMALL_REVERSED_DOTTED_LUNATE_SIGMA_SYMBOL = 0x037D,
	UNICODE_GREEK_QUESTION_MARK = 0x037E,
	UNICODE_GREEK_CAPITAL_LETTER_YOT = 0x037F,
	UNICODE_GREEK_TONOS = 0x0384,
	UNICODE_GREEK_DIALYTIKA_TONOS = 0x0385,
	UNICODE_GREEK_CAPITAL_LETTER_ALPHA_WITH_TONOS = 0x0386,
	UNICODE_GREEK_ANO_TELEIA = 0x0387,
	UNICODE_GREEK_CAPITAL_LETTER_EPSILON_WITH_TONOS = 0x0388,
	UNICODE_GREEK_CAPITAL_LETTER_ETA_WITH_TONOS = 0x0389,
	UNICODE_GREEK_CAPITAL_LETTER_IOTA_WITH_TONOS = 0x038A,
	UNICODE_GREEK_CAPITAL_LETTER_OMICRON_WITH_TONOS = 0x038C,
	UNICODE_GREEK_CAPITAL_LETTER_UPSILON_WITH_TONOS = 0x038E,
	UNICODE_GREEK_CAPITAL_LETTER_OMEGA_WITH_TONOS = 0x038F,
	UNICODE_GREEK_SMALL_LETTER_IOTA_WITH_DIALYTIKA_AND_TONOS = 0x0390,
	UNICODE_GREEK_CAPITAL_LETTER_ALPHA = 0x0391,
	UNICODE_GREEK_CAPITAL_LETTER_BETA = 0x0392,
	UNICODE_GREEK_CAPITAL_LETTER_GAMMA = 0x0393,
	UNICODE_GREEK_CAPITAL_LETTER_DELTA = 0x0394,
	UNICODE_GREEK_CAPITAL_LETTER_EPSILON = 0x0395,
	UNICODE_GREEK_CAPITAL_LETTER_ZETA = 0x0396,
	UNICODE_GREEK_CAPITAL_LETTER_ETA = 0x397,
	UNICODE_GREEK_CAPITAL_LETTER_THETA = 0x0398,
	UNICODE_GREEK_CAPITAL_LETTER_IOTA = 0x0399,
	UNICODE_GREEK_CAPITAL_LETTER_KAPPA = 0x039A,
	UNICODE_GREEK_CAPITAL_LETTER_LAMDA = 0x039B,
	UNICODE_GREEK_CAPITAL_LETTER_MU = 0x039C,
	UNICODE_GREEK_CAPITAL_LETTER_NU = 0x039D,
	UNICODE_GREEK_CAPITAL_LETTER_XI = 0x039E,
	UNICODE_GREEK_CAPITAL_LETTER_OMICRON = 0x039F,
	UNICODE_GREEK_CAPITAL_LETTER_PI = 0x03A0,
	UNICODE_GREEK_CAPITAL_LETTER_RHO = 0x03A1,
	UNICODE_GREEK_CAPITAL_LETTER_SIGMA = 0x03A3,
	UNICODE_GREEK_CAPITAL_LETTER_TAU = 0x03A4,
	UNICODE_GREEK_CAPITAL_LETTER_UPSILON = 0x03A5,
	UNICODE_GREEK_CAPITAL_LETTER_PHI = 0x03A6,
	UNICODE_GREEK_CAPITAL_LETTER_CHI = 0x03A7,
	UNICODE_GREEK_CAPITAL_LETTER_PSI = 0x03A8,
	UNICODE_GREEK_CAPITAL_LETTER_OMEGA = 0x03A9,
	UNICODE_GREEK_CAPITAL_LETTER_IOTA_WITH_DIALYTIKA = 0x03AA,
	UNICODE_GREEK_CAPITAL_LETTER_UPSILON_WITH_DIALYTIKA = 0x03AB,
	UNICODE_GREEK_SMALL_LETTER_ALPHA_WITH_TONOS = 0x03AC,
	UNICODE_GREEK_SMALL_LETTER_EPSILON_WITH_TONOS = 0x03AD,
	UNICODE_GREEK_SMALL_LETTER_ETA_WITH_TONOS = 0x03AE,
	UNICODE_GREEK_SMALL_LETTER_IOTA_WITH_TONOS = 0x03AF,
	UNICODE_GREEK_SMALL_LETTER_UPSILON_WITH_DIALYTIKA_AND_TONOS = 0x03B0,
	UNICODE_GREEK_SMALL_LETTER_ALPHA = 0x03B1,
	UNICODE_GREEK_SMALL_LETTER_BETA = 0x03B2,
	UNICODE_GREEK_SMALL_LETTER_GAMMA = 0x03B3,
	UNICODE_GREEK_SMALL_LETTER_DELTA = 0x03B4,
	UNICODE_GREEK_SMALL_LETTER_EPSILON = 0x03B5,
	UNICODE_GREEK_SMALL_LETTER_ZETA = 0x03B6,
	UNICODE_GREEK_SMALL_LETTER_ETA = 0x03B7,
	UNICODE_GREEK_SMALL_LETTER_THETA = 0x03B8,
	UNICODE_GREEK_SMALL_LETTER_IOTA = 0x03B9,
	UNICODE_GREEK_SMALL_LETTER_KAPPA = 0x03BA,
	UNICODE_GREEK_SMALL_LETTER_LAMDA = 0x03BB,
	UNICODE_GREEK_SMALL_LETTER_MU = 0x03BC,
	UNICODE_GREEK_SMALL_LETTER_NU = 0x03BD,
	UNICODE_GREEK_SMALL_LETTER_XI = 0x03BE,
	UNICODE_GREEK_SMALL_LETTER_OMICRON = 0x03BF,
	UNICODE_GREEK_SMALL_LETTER_PI = 0x03C0,
	UNICODE_GREEK_SMALL_LETTER_RHO = 0x03C1,
	UNICODE_GREEK_SMALL_LETTER_FINAL_SIGMA = 0x03C2,
	UNICODE_GREEK_SMALL_LETTER_SIGMA = 0x03C3,
	UNICODE_GREEK_SMALL_LETTER_TAU = 0x03C4,
	UNICODE_GREEK_SMALL_LETTER_UPSILON = 0x03C5,
	UNICODE_GREEK_SMALL_LETTER_PHI = 0x03C6,
	UNICODE_GREEK_SMALL_LETTER_CHI = 0x03C7,
	UNICODE_GREEK_SMALL_LETTER_PSI = 0x03C8,
	UNICODE_GREEK_SMALL_LETTER_OMEGA = 0x03C9,
	UNICODE_GREEK_SMALL_LETTER_IOTA_WITH_DIALYTIKA = 0x03CA,
	UNICODE_GREEK_SMALL_LETTER_UPSILON_WITH_DIALYTIKA = 0x03CB,
	UNICODE_GREEK_SMALL_LETTER_OMICRON_WITH_TONOS = 0x03CC,
	UNICODE_GREEK_SMALL_LETTER_UPSILON_WITH_TONOS = 0x03CD,
	UNICODE_GREEK_SMALL_LETTER_OMEGA_WITH_TONOS = 0x03CE,
	UNICODE_GREEK_CAPITLA_KAI_SYMBOL = 0x03CF,
	UNICODE_GREEK_BETA_SYMBOL = 0x03D0,
	UNICODE_GREEK_THETA_SYMBOL = 0x03D1,
	UNICODE_GREEK_UPSILON_WITH_HOOK_SYMBOL = 0x03D2,
	UNICODE_GREEK_UPSILON_WITH_ACUTE_AND_HOOK_SYMBOL = 0x03D3,
	UNICODE_GREEK_UPSILON_WITH_DIAERESIS_AND_HOOK_SYMBOL = 0x03D4,
	UNICODE_GREEK_PHI_SYMBOL = 0x03D5,
	UNICODE_GREEK_PI_SYMBOL = 0x03D6,
	UNICODE_GREEK_KAI_SYMBOL = 0x03D7,
	UNICODE_GREEK_LETTER_ARCHAIC_KOPPA = 0x03D8,
	UNICODE_GREEK_SMALL_LETTER_ARCHAIC_KOPPA = 0x03D9,
	UNICODE_GREEK_LETTER_STIGMA = 0x03DA,
	UNICODE_GREEK_SMALL_LETTER_STIGMA = 0x03DB,
	UNICODE_GREEK_LETTER_DIGAMMA = 0x03DC,
	UNICODE_GREEK_SMALL_LETTER_DIGAMMA = 0x03DD,
	UNICODE_GREEK_LETTER_KOPPA = 0x03DE,
	UNICODE_GREEK_SMALL_LETTER_KOPPA = 0x03DF,
	UNICODE_GREEK_LETTER_SAMPI = 0x03E0,
	UNICODE_GREEK_SMALL_LETTER_SAMPI = 0x03E1,

	UNICODE_EN_QUAD = 0x2000,
	UNICODE_EM_QUAD = 0x2001,
	UNICODE_EN_SPACE = 0x2002,
	UNICODE_EM_SPACE = 0x2003,
	UNICODE_ZERO_WIDTH_SPACE = 0x200B,
	UNICODE_ZERO_WIDTH_NON_JOINER = 0x200C,
	UNICODE_ZERO_WIDTH_JOINER = 0x200D,
	UNICODE_EM_DASH = 0x2014,
	UNICODE_BULLET = 0x2022,
	UNICODE_HORIZONTAL_ELLIPSIS = 0x2026,
	UNICODE_LINE_SEPARATOR = 0x2028,
	UNICODE_DOUBLE_EXCLAMATION_MARK = 0x203c,
	UNICODE_OVERLINE = 0x203E,
	UNICODE_SUPERSCRIPT_LATIN_SMALL_LETTER_N = 0x207F,
	UNICODE_PESETA_SIGN = 0x20A7,

	UNICODE_DEGREE_CELSIUS = 0x2103,
	UNICODE_CARE_OF = 0x2105,
	UNICODE_DEGREE_FAHRENHEIT = 0x2109,
	UNICODE_NUMERO_SIGN = 0x2116,
	UNICODE_SOUND_RECORDING_COPYRIGHT = 0x2117,
	UNICODE_TRADE_MARK_SIGN = 0x2122,
	UNICODE_LEFTWARDS_ARROW = 0x2190,
	UNICODE_UPWARDS_ARROW = 0x2191,
	UNICODE_RIGHTWARDS_ARROW = 0x2192,
	UNICODE_DOWNWARDS_ARROW = 0x2193,
	UNICODE_LEFT_RIGHT_ARROW = 0x2194,
	UNICODE_UP_DOWN_ARROW = 0x2195,

	UNICODE_UP_DOWN_ARROW_WITH_BASE = 0x21A8,

	UNICODE_BULLET_OPERATOR = 0x2219,
	UNICODE_SQUARE_ROOT = 0x221A,
	UNICODE_INFINITY = 0x221E,
	UNICODE_RIGHT_ANGLE = 0x221F,

	UNICODE_INTERSECTION = 0x2229,

	UNICODE_ALMOST_EQUAL_TO = 0x2248,

	UNICODE_IDENTICAL_TO = 0x2261,
	UNICODE_LESS_THAN_OR_EQUAL_TO = 0x2264,
	UNICODE_GREATER_THAN_OR_EQUAL_TO = 0x2265,

	UNICODE_REVERSED_NOT_SIGN = 0x2310,

	UNICODE_TOP_HALF_INTEGRAL = 0x2320,
	UNICODE_BOTTOM_HALF_INTEGRAL = 0x2321,

	UNICODE_LEFT_POINTING_ANGLE_BRACKET = 0x2329,
	UNICODE_RIGHT_POINTING_ANGLE_BRACKET = 0x232A,
	UNICODE_ERASE_TO_THE_LEFT = 0x232B,

	UNICODE_COUNTERSINK = 0x2335,
	UNICODE_APL_FUNCTIONAL_SYMBOL_I_BEAM = 0x2336,

	UNICODE_BOX_DRAWINGS_LIGHT_HORIZONTAL = 0x2500,
	UNICODE_BOX_DRAWINGS_HEAVY_HORIZONTAL = 0x2501,
	UNICODE_BOX_DRAWINGS_LIGHT_VERTICAL = 0x2502,
	UNICODE_BOX_DRAWINGS_HEAVY_VERTICAL	= 0x2503,
	UNICODE_BOX_DRAWINGS_LIGHT_TRIPLE_DASH_HORIZONTAL = 0x2504,
	UNICODE_BOX_DRAWINGS_HEAVY_TRIPLE_DASH_HORIZONTAL = 0x2505,
	UNICODE_BOX_DRAWINGS_LIGHT_TRIPLE_DASH_VERTICAL	= 0x2506,
	UNICODE_BOX_DRAWINGS_HEAVY_TRIPLE_DASH_VERTICAL	= 0x2507,
	UNICODE_BOX_DRAWINGS_LIGHT_QUADRUPLE_DASH_HORIZONTAL = 0x2508,
	UNICODE_BOX_DRAWINGS_HEAVY_QUADRUPLE_DASH_HORIZONTAL = 0x2509,
	UNICODE_BOX_DRAWINGS_LIGHT_QUADRUPLE_DASH_VERTICAL = 0x250A,
	UNICODE_BOX_DRAWINGS_HEAVY_QUADRUPLE_DASH_VERTICAL = 0x250B,
	UNICODE_BOX_DRAWINGS_LIGHT_DOWN_AND_RIGHT = 0x250C,
	UNICODE_BOX_DRAWINGS_250D = 0x250D,
	UNICODE_BOX_DRAWINGS_250E = 0x250E,
	UNICODE_BOX_DRAWINGS_HEAVY_DOWN_AND_RIGHT = 0x250F,
	UNICODE_BOX_DRAWINGS_LIGHT_DOWN_AND_LEFT = 0x2510,
	UNICODE_BOX_DRAWINGS_2511 = 0x2511,
	UNICODE_BOX_DRAWINGS_2512 = 0x2512,
	UNICODE_BOX_DRAWINGS_HEAVY_DOWN_AND_LEFT = 0x2513,
	UNICODE_BOX_DRAWINGS_LIGHT_UP_AND_RIGHT	= 0x2514,
	UNICODE_BOX_DRAWINGS_2515 = 0x2515,
	UNICODE_BOX_DRAWINGS_2516 = 0x2516,
	UNICODE_BOX_DRAWINGS_HEAVY_UP_AND_RIGHT	= 0x2517,
	UNICODE_BOX_DRAWINGS_LIGHT_UP_AND_LEFT	= 0x2518,
	UNICODE_BOX_DRAWINGS_2519 = 0x2519,
	UNICODE_BOX_DRAWINGS_251A = 0x251A,
	UNICODE_BOX_DRAWINGS_HEAVY_UP_AND_LEFT	= 0x251B,
	UNICODE_BOX_DRAWINGS_LIGHT_VERTICAL_AND_RIGHT	= 0x251C,
	UNICODE_BOX_DRAWINGS_251D = 0x251D,
	UNICODE_BOX_DRAWINGS_251E = 0x251E,
	UNICODE_BOX_DRAWINGS_251F = 0x251F,
	UNICODE_BOX_DRAWINGS_2520 = 0x2520,
	UNICODE_BOX_DRAWINGS_2521 = 0x2521,
	UNICODE_BOX_DRAWINGS_2522= 0x2522,
	UNICODE_BOX_DRAWINGS_HEAVY_VERTICAL_AND_RIGHT = 0x2523,
	UNICODE_BOX_DRAWINGS_LIGHT_VERTICAL_AND_LEFT = 0x2524,
	UNICODE_BOX_DRAWINGS_2525 = 0x2525,
	UNICODE_BOX_DRAWINGS_2526 = 0x2526,
	UNICODE_BOX_DRAWINGS_2527 = 0x2527,
	UNICODE_BOX_DRAWINGS_2528 = 0x2528,
	UNICODE_BOX_DRAWINGS_2529 = 0x2529,
	UNICODE_BOX_DRAWINGS_252A = 0x252A,
	UNICODE_BOX_DRAWINGS_252B = 0x252B,
	UNICODE_BOX_DRAWINGS_LIGHT_DOWN_AND_HORIZONTAL = 0x252C,
	UNICODE_BOX_DRAWINGS_252D = 0x252D,
	UNICODE_BOX_DRAWINGS_252E = 0x252E,
	UNICODE_BOX_DRAWINGS_252F = 0x252F,
	UNICODE_BOX_DRAWINGS_2530 = 0x2530,
	UNICODE_BOX_DRAWINGS_2531 = 0x2531,
	UNICODE_BOX_DRAWINGS_LEFT_LIGHT_AND_RIGHT_DOWN_HEAVY = 0x2532,
	UNICODE_BOX_DRAWINGS_HEAVY_DOWN_AND_HORIZONTAL = 0x2533,
	UNICODE_BOX_DRAWINGS_LIGHT_UP_AND_HORIZONTAL = 0x2534,
	UNICODE_BOX_DRAWINGS_LEFT_HEAVY_AND_RIGHT_UP_LIGHT = 0x2535,
	UNICODE_BOX_DRAWINGS_RIGHT_HEAVY_AND_LEFT_UP_LIGHT = 0x2536,
	UNICODE_BOX_DRAWINGS_UP_LIGHT_AND_HORIZONTAL_HEAVY = 0x2537,
	UNICODE_BOX_DRAWINGS_UP_HEAVY_AND_HORIZONTAL_LIGHT = 0x2538,
	UNICODE_BOX_DRAWINGS_RIGHT_LIGHT_AND_LEFT_UP_HEAVY = 0x2539,
	UNICODE_BOX_DRAWINGS_LEFT_LIGHT_AND_RIGHT_UP_HEAVY = 0x253A,
	UNICODE_BOX_DRAWINGS_HEAVY_UP_AND_HORIZONTAL = 0x253B,
	UNICODE_BOX_DRAWINGS_LIGHT_VERTICAL_AND_HORIZONTAL = 0x253C,
	UNICODE_BOX_DRAWINGS_LEFT_HEAVY_AND_RIGHT_VERTICAL_LIGHT = 0x253D,
	UNICODE_BOX_DRAWINGS_RIGHT_HEAVY_AND_LEFT_VERTICAL_LIGHT = 0x253E,
	UNICODE_BOX_DRAWINGS_VERTICAL_LIGHT_AND_HORIZONTAL_HEAVY = 0x253F,
	UNICODE_BOX_DRAWINGS_UP_HEAVY_AND_DOWN_HORIZONTAL_LIGHT	= 0x2540,
	UNICODE_BOX_DRAWINGS_DOWN_HEAVY_AND_UP_HORIZONTAL_LIGHT	= 0x2541,
	UNICODE_BOX_DRAWINGS_VERTICAL_HEAVY_AND_HORIZONTAL_LIGHT = 0x2542,
	UNICODE_BOX_DRAWINGS_LEFT_UP_HEAVY_AND_RIGHT_DOWN_LIGHT	= 0x2543,
	UNICODE_BOX_DRAWINGS_RIGHT_UP_HEAVY_AND_LEFT_DOWN_LIGHT	= 0x2544,
	UNICODE_BOX_DRAWINGS_LEFT_DOWN_HEAVY_AND_RIGHT_UP_LIGHT	= 0x2545,
	UNICODE_BOX_DRAWINGS_RIGHT_DOWN_HEAVY_AND_LEFT_UP_LIGHT	= 0x2546,
	UNICODE_BOX_DRAWINGS_DOWN_LIGHT_AND_UP_HORIZONTAL_HEAVY	= 0x2547,
	UNICODE_BOX_DRAWINGS_UP_LIGHT_AND_DOWN_HORIZONTAL_HEAVY	= 0x2548,
	UNICODE_BOX_DRAWINGS_RIGHT_LIGHT_AND_LEFT_VERTICAL_HEAVY = 0x2549,
	UNICODE_BOX_DRAWINGS_LEFT_LIGHT_AND_RIGHT_VERTICAL_HEAVY = 0x254A,
	UNICODE_BOX_DRAWINGS_HEAVY_VERTICAL_AND_HORIZONTAL = 0x254B,
	UNICODE_BOX_DRAWINGS_LIGHT_DOUBLE_DASH_HORIZONTAL = 0x254C,
	UNICODE_BOX_DRAWINGS_HEAVY_DOUBLE_DASH_HORIZONTAL = 0x254D,
	UNICODE_BOX_DRAWINGS_LIGHT_DOUBLE_DASH_VERTICAL	= 0x254E,
	UNICODE_BOX_DRAWINGS_HEAVY_DOUBLE_DASH_VERTICAL	= 0x254F,
	UNICODE_BOX_DRAWINGS_VERTICAL_SINGLE_AND_LEFT_DOUBLE = 0x2561,
	UNICODE_BOX_DRAWINGS_VERTICAL_DOUBLE_AND_LEFT_SINGLE = 0x2562,
	UNICODE_BOX_DRAWINGS_LIGHT_ARC_DOWN_AND_RIGHT = 0x256D,
	UNICODE_BOX_DRAWINGS_LIGHT_ARC_DOWN_AND_LEFT = 0x256E,
	UNICODE_BOX_DRAWINGS_LIGHT_ARC_UP_AND_LEFT = 0x256F,
	UNICODE_BOX_DRAWINGS_LIGHT_ARC_UP_AND_RIGHT	= 0x2570,
	UNICODE_BOX_DRAWINGS_LIGHT_DIAGONAL_UPPER_RIGHT_TO_LOWER_LEFT = 0x2571,
	UNICODE_BOX_DRAWINGS_LIGHT_DIAGONAL_UPPER_LEFT_TO_LOWER_RIGHT = 0x2572,
	UNICODE_BOX_DRAWINGS_LIGHT_DIAGONAL_CROSS = 0x2573,
	UNICODE_BOX_DRAWINGS_LIGHT_LEFT	= 0x2574,
	UNICODE_BOX_DRAWINGS_LIGHT_UP = 0x2575,
	UNICODE_BOX_DRAWINGS_LIGHT_RIGHT = 0x2576,
	UNICODE_BOX_DRAWINGS_LIGHT_DOWN	= 0x2577,
	UNICODE_BOX_DRAWINGS_HEAVY_LEFT	= 0x2578,
	UNICODE_BOX_DRAWINGS_HEAVY_UP = 0x2579,
	UNICODE_BOX_DRAWINGS_HEAVY_RIGHT = 0x257A,
	UNICODE_BOX_DRAWINGS_HEAVY_DOWN	= 0x257B,
	UNICODE_BOX_DRAWINGS_LIGHT_LEFT_AND_HEAVY_RIGHT	= 0x257C,
	UNICODE_BOX_DRAWINGS_LIGHT_UP_AND_HEAVY_DOWN = 0x257D,
	UNICODE_BOX_DRAWINGS_HEAVY_LEFT_AND_LIGHT_RIGHT	= 0x257E,
	UNICODE_BOX_DRAWINGS_HEAVY_UP_AND_LIGHT_DOWN = 0x257F,

	UNICODE_LOWER_ONE_EIGHTH_BLOCK	= 0x2581,
	UNICODE_LOWER_ONE_QUARTER_BLOCK	= 0x2582,
	UNICODE_LOWER_THREE_EIGHTHS_BLOCK = 0x2583,
	UNICODE_LOWER_FIVE_EIGHTHS_BLOCK = 0x2585,
	UNICODE_LOWER_THREE_QUARTERS_BLOCK = 0x2586,
	UNICODE_LOWER_SEVEN_EIGHTHS_BLOCK = 0x2587,
	UNICODE_FULL_BLOCK = 0x2588,
	UNICODE_LEFT_SEVEN_EIGHTHS_BLOCK = 0x2589,
	UNICODE_LEFT_THREE_QUARTERS_BLOCK = 0x258A,
	UNICODE_LEFT_FIVE_EIGHTHS_BLOCK	= 0x258B,
	UNICODE_LEFT_HALF_BLOCK	= 0x258C,
	UNICODE_LEFT_THREE_EIGHTHS_BLOCK = 0x258D,
	UNICODE_LEFT_ONE_QUARTER_BLOCK = 0x258E,
	UNICODE_LEFT_ONE_EIGHTH_BLOCK = 0x258F,
	UNICODE_RIGHT_HALF_BLOCK = 0x2590,
	UNICODE_LIGHT_SHADE = 0x2591,
	UNICODE_MEDIUM_SHADE = 0x2591,
	UNICODE_DARK_SHADE = 0x2592,
	UNICODE_RIGHT_ONE_EIGHTH_BLOCK = 0x2595,
	UNICODE_UPPER_ONE_EIGHTH_BLOCK = 0x2594,

	UNICODE_BLACK_SQUARE = 0x25A0,
	UNICODE_BLACK_RECTANGLE = 0x25AC,
	UNICODE_BLACK_UP_POINTING_TRIANGLE = 0x25B2,
	UNICODE_BLACK_RIGHT_POINTING_POINTER = 0x25BA,
	UNICODE_BLACK_DOWN_POINTING_TRIANGLE = 0x25BC,
	UNICODE_BLACK_LEFT_POINTING_POINTER = 0x25C4,
	UNICODE_WHITE_CIRCLE = 0x25CB,
	UNICODE_INVERSE_BULLET = 0x25D8,
	UNICODE_INVERSE_WHITE_CIRCLE = 0x25D9,

	UNICODE_WHITE_SMILING_FACE = 0x263A,
	UNICODE_BLACK_SMILING_FACE = 0x263B,
	UNICODE_WHITE_SUN_WITH_RAYS = 0x263C,
	UNICODE_FEMALE_SIGN = 0x2640,
	UNICODE_EIGHTH_NOTE = 0x266A,
	UNICODE_BEAMED_EIGHTH_NOTES = 0x266B,
	UNICODE_MALE_SIGN = 0x2642,
	UNICODE_BLACK_HEART_SUIT = 0x2665,
	UNICODE_BLACK_DIAMOND_SUIT = 0x2666,
	UNICODE_BLACK_CLUB_SUIT = 0x2663,
	UNICODE_BLACK_SPADE_SUIT = 0x2660,

	UNICODE_CHECK_MARK = 0x2713,
	UNICODE_HEAVY_CHECK_MARK = 0x2714,

	UNICODE_MULTIPLICATION_X = 0x2715,
	UNICODE_HEAVY_MULTIPLICATION_X = 0x2716,
	UNICODE_BALLOT_X = 0x2717,
	UNICODE_HEAVY_BALLOT_X = 0x2718,

	UNICODE_IDEOGRAPHIC_SPACE = 0x3000,	// Fullwidth space

	UNICODE_VARIATION_SELECTOR_1 = 0xFE00,
	UNICODE_VARIATION_SELECTOR_2 = 0xFE01,
	UNICODE_VARIATION_SELECTOR_3 = 0xFE02,
	UNICODE_VARIATION_SELECTOR_4 = 0xFE03,
	UNICODE_VARIATION_SELECTOR_5 = 0xFE04,
	UNICODE_VARIATION_SELECTOR_6 = 0xFE05,
	UNICODE_VARIATION_SELECTOR_7 = 0xFE06,
	UNICODE_VARIATION_SELECTOR_8 = 0xFE07,
	UNICODE_VARIATION_SELECTOR_9 = 0xFE08,
	UNICODE_VARIATION_SELECTOR_10 = 0xFE09,
	UNICODE_VARIATION_SELECTOR_11 = 0xFE0A,
	UNICODE_VARIATION_SELECTOR_12 = 0xFE0B,
	UNICODE_VARIATION_SELECTOR_13 = 0xFE0C,
	UNICODE_VARIATION_SELECTOR_14 = 0xFE0D,
	UNICODE_VARIATION_SELECTOR_15 = 0xFE0E,
	UNICODE_VARIATION_SELECTOR_16 = 0xFE0F,
	UNICODE_ZERO_WIDTH_NO_BREAK_SPACE = 0xFEFF,

	UNICODE_FULLWIDTH_EXCLAMATION_MARK = 0xFF01,
	UNICODE_FULLWIDTH_RIGHT_WHITE_PARENTHESIS = 0xFF60,

	UNICODE_HALFWIDTH_BLACK_SQUARE = 0xFFED,
	UNICODE_HALFWIDTH_WHITE_CIRCLE = 0xFFEE,

	UNICODE_REPLACEMENT_CHARACTER = 0xFFFD
};

// Blocks
#define UNICODE_BLOCK_BASIC_LATIN_BEGIN							0x0000
#define UNICODE_BLOCK_BASIC_LATIN_END							0x007F
#define UNICODE_BLOCK_HANGUL_JAMO_BEGIN							0x1100
#define UNICODE_BLOCK_HANGUL_JAMO_END							0x11FF
#define UNICODE_BLOCK_CJK_RADICALS_SUPPLEMENT_BEGIN				0x2E80	// Fullwidth
#define UNICODE_BLOCK_CJK_RADICALS_SUPPLEMENT_END				0x2EFF	// Fullwidth
#define UNICODE_BLOCK_KANGXI_RADICALS_BEGIN						0x2F00	// Fullwidth
#define UNICODE_BLOCK_KANGXI_RADICALS_END						0x2FDF	// Fullwidth
#define UNICODE_BLOCK_IDEOGRAPHIC_DESCRIPTION_CHARACTERS_BEGIN	0x2FF0	// Fullwidth
#define UNICODE_BLOCK_IDEOGRAPHIC_DESCRIPTION_CHARACTERS_END	0x2FFF	// Fullwidth
#define UNICODE_BLOCK_CJK_SYMBOLS_AND_PUNCTUATION_BEGIN			0x3000	// Fullwidth 0x3000 - 0x3029
#define UNICODE_BLOCK_CJK_SYMBOLS_AND_PUNCTUATION_END			0x303F  // Fullwidth 0x3030 - 0x303E
#define UNICODE_BLOCK_HIRAGANA_BEGIN							0x3040	// Fullwidth 0x3040 - 0x3098
#define UNICODE_BLOCK_HIRAGANA_END								0x309F	// Fullwidth 0x309B - 0x309F
#define UNICODE_BLOCK_KATAKANA_BEGIN							0x30A0	// Fullwidth
#define UNICODE_BLOCK_KATAKANA_END								0x30FF	// Fullwidth
#define UNICODE_BLOCK_BOPOMOFO_BEGIN							0x3100	// Fullwidth
#define UNICODE_BLOCK_BOPOMOFO_END								0x312F	// Fullwidth
#define UNICODE_BLOCK_HANGUL_COMPATIBILITY_JAMO_BEGIN			0x3130	// Fullwidth
#define UNICODE_BLOCK_HANGUL_COMPATIBILITY_JAMO_END				0x318F	// Fullwidth
#define UNICODE_BLOCK_KANBUN_BEGIN								0x3190	// Fullwidth
#define UNICODE_BLOCK_KANBUN_END								0x319F	// Fullwidth
#define UNICODE_BLOCK_BOPOMOFO_EXTENDED_BEGIN					0x31A0	// Fullwidth
#define UNICODE_BLOCK_BOPOMOFO_EXTENDED_END						0x31BF	// Fullwidth
#define UNICODE_BLOCK_CJK_STROKES_BEGIN							0x31C0	// Fullwidth
#define UNICODE_BLOCK_CJK_STROKES_END							0x31EF	// Fullwidth
#define UNICODE_BLOCK_KATAKANA_PHONETIC_EXTENSIONS_BEGIN		0x31F0	// Fullwidth
#define UNICODE_BLOCK_KATAKANA_PHONETIC_EXTENSIONS_END			0x31FF	// Fullwidth
#define UNICIDE_BLOCK_YI_RADICALS_BEGIN							0xA490	// Fullwidth	
#define UNICIDE_BLOCK_YI_RADICALS_END							0xA4CF	// Fullwidth
#define UNICIDE_BLOCK_HANGUL_SYLLABLES_BEGIN					0xAC00	// Fullwidth
#define UNICIDE_BLOCK_HANGUL_SYLLABLES_END						0xD7AF	// Fullwidth
#define UNICODE_BLOCK_CJK_COMPATIBILITY_IDEOGRAPHS_BEGIN		0xF900	// Fullwidth
#define UNICODE_BLOCK_CJK_COMPATIBILITY_IDEOGRAPHS_END			0xFAFF	// Fullwidth
#define UNICODE_BLOCK_VERTICAL_FORMS_BEGIN						0xFE10	// Fullwidth
#define UNICODE_BLOCK_VERTICAL_FORMS_END						0xFE1F	// Fullwidth
#define UNICODE_BLOCK_CJK_COMPATIBILITY_FORMS_BEGIN				0xFE30	// Fullwidth
#define UNICODE_BLOCK_CJK_COMPATIBILITY_FORMS_END				0xFE4F	// Fullwidth
#define UNICODE_BLOCK_SMALL_FORM_VARIANTS_BEGIN					0xFE50	// Fullwidth
#define UNICODE_BLOCK_SMALL_FORM_VARIANTS_END					0xFE6F
#define UNICODE_BLOCK_HALFWIDTH_AND_FULLWIDTH_FORMS_BEGIN		0xFF00
#define UNICODE_BLOCK_HALFWIDTH_AND_FULLWIDTH_FORMS_END			0xFFEF

// Sub-Blocks
#define UNICODE_SUBBLOCK_FULLWIDTH_HANGUL_BEGIN					0x1100
#define UNICODE_SUBBLOCK_FULLWIDTH_HANGUL_END					0x115F
#define UNICODE_SUBBLOCK_FULLWIDTH_CHARS_BEGIN					0xFF01
#define UNICODE_SUBBLOCK_FULLWIDTH_CHARS_END					0xFF60
#define UNICODE_SUBBLOCK_FULLWIDTH_SYMBOLS_BEGIN				0xFFE0
#define UNICODE_SUBBLOCK_FULLWIDTH_SYMBOLS_END					0xFFE6

#endif // Don't add anything after this line