/* Copyright (C), 2007 by Stephen Hurd */

/* $Id: term.h,v 1.18 2018/02/13 08:11:36 deuce Exp $ */

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
extern struct cterminal	*cterm;
extern int log_level;

BOOL doterm(struct bbslist *);
void mousedrag(struct vmem_cell *scrollback);

#endif
