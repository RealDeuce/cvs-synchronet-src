# targets.mk

# Make 'include file' defining targets for xpdel wrappers

# $Id: targets.mk,v 1.6 2004/09/13 06:28:34 deuce Exp $

# ODIR, DIRSEP, LIBFILE, EXEFILE, and DELETE must be pre-defined

WRAPTEST	= $(EXEODIR)$(DIRSEP)wraptest$(EXEFILE)

all: lib mtlib

lib:	$(OBJODIR) $(LIBODIR) $(XPDEV_LIB)

mtlib:	$(MTOBJODIR) $(LIBODIR) $(XPDEV-MT_LIB)
