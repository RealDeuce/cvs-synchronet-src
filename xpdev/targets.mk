# targets.mk

# Make 'include file' defining targets for xpdel wrappers

# $Id: targets.mk,v 1.7 2004/09/16 19:02:03 deuce Exp $

# ODIR, DIRSEP, LIBFILE, EXEFILE, and DELETE must be pre-defined

WRAPTEST		= $(EXEODIR)$(DIRSEP)wraptest$(EXEFILE)
XPDEV_LIB_BUILD	= $(LIBODIR)$(DIRSEP)$(LIBPREFIX)xpdev$(LIBFILE)
XPDEV-MT_LIB_BUILD	= $(LIBODIR)$(DIRSEP)$(LIBPREFIX)xpdev_mt$(LIBFILE)

all: lib mtlib

lib:	$(OBJODIR) $(LIBODIR) $(XPDEV_LIB_BUILD)

mtlib:	$(MTOBJODIR) $(LIBODIR) $(XPDEV-MT_LIB_BUILD)
