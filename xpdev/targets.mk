# targets.mk

# Make 'include file' defining targets for xpdel wrappers

# $Id: targets.mk,v 1.5 2004/09/13 00:13:47 deuce Exp $

# ODIR, DIRSEP, LIBFILE, EXEFILE, and DELETE must be pre-defined

WRAPTEST	= $(EXEODIR)$(DIRSEP)wraptest$(EXEFILE)

all: lib mtlib

lib:	$(LIBODIR) $(XPDEV_LIB)

mtlib:	$(LIBODIR) $(XPDEV-MT_LIB)
