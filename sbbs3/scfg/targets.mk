# targets.mk

# Make 'include file' defining targets for Synchronet SCFG project

# $Id: targets.mk,v 1.7 2014/02/16 06:28:52 deuce Exp $

# LIBODIR, SLASH, LIBFILE, EXEFILE, and DELETE must be pre-defined

SCFG	=	$(EXEODIR)$(DIRSEP)scfg$(EXEFILE) 

all:		xpdev-mt \
		uifc-mt \
		ciolib-mt \
		smblib \
		$(EXEODIR) \
		$(MTOBJODIR) \
		$(LIBODIR) \
		$(SCFG) $(SCFGHELP)

$(SCFG):	$(XPDEV-MT_LIB) $(UIFCLIB-MT) $(CIOLIB-MT)
