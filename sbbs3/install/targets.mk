# targets.mk

# Make 'include file' defining targets for Synchronet SBBSINST project

# $Id: targets.mk,v 1.3 2004/09/13 22:57:09 deuce Exp $

# ODIR, SLASH, LIBFILE, EXEFILE, and DELETE must be pre-defined

SBBSINST	=	$(EXEODIR)$(DIRSEP)sbbsinst$(EXEFILE) 

all:	$(EXEODIR) \
		$(MTOBJODIR) \
		$(SBBSINST)
