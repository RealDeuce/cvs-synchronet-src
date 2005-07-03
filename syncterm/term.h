/* $Id: term.h,v 1.13 2005/06/18 21:44:06 deuce Exp $ */

#ifndef _TERM_H_
#define _TERM_H_

#include "bbslist.h"

struct terminal {
	int	height;
	int	width;
	int	x;
	int	y;
	int nostatus;
};

extern struct terminal term;
extern int backlines;
extern int log_level;

BOOL doterm(struct bbslist *);
void mousedrag(unsigned char *scrollback);

#endif
