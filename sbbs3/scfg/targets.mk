# targets.mk

# Make 'include file' defining targets for Synchronet SCFG project

# $Id: targets.mk,v 1.4 2004/03/11 06:26:00 deuce Exp $

# LIBODIR, SLASH, LIBFILE, EXEFILE, and DELETE must be pre-defined

SCFG	=	$(EXEODIR)$(SLASH)scfg$(EXEFILE) 
MAKEHELP=	$(EXEODIR)$(SLASH)makehelp$(EXEFILE) 
SCFGHELP=	$(EXEODIR)$(SLASH)scfghelp.dat

all:	$(EXEODIR) \
		$(LIBODIR) \
		$(SCFG) $(SCFGHELP)
