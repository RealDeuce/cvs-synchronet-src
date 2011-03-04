# targets.mk

# Make 'include file' defining targets for Synchronet MenuEdit project

# $Id: targets.mk,v 1.1 2004/07/20 02:09:21 rswindell Exp $

# EXEODIR, LIBODIR, SLASH, and EXEFILE must be pre-defined

MENUEDIT=	$(EXEODIR)$(SLASH)menuedit$(EXEFILE) 

all:		$(EXEODIR) \
		$(LIBODIR) \
		$(MENUEDIT)
